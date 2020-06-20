#define BASE_REFCOUNT_CC_
#include "Exception.h"
#include "Hashtable.h"
#include "Lock.h"
#include "Logcat.h"
#include "Type.h"
#include "Vertex.h"

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Vector<ULong>* RefMasters{None};
static Vertex<Mutex, True> Secure([](Mutex* mutex) { LOCK(mutex); },
                                  [](Mutex* mutex) { UNLOCK(mutex); },
                                  CreateMutex());
} // namespace Internal

Refcount::~Refcount() { Release(); }

Refcount::Refcount(const Refcount& src): _Status{False} {
  Bool pass = True;

  if (this == &src) {
    throw Except(EBadLogic, "reference itself");
  } else if ((_Count = src._Count)) {
    _Context = src._Context;
    _Init = src._Init;
    _Release = src._Release;

    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (RefMasters) {
        if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
          (*_Count)++;
          _Secure = src._Secure;
        } else {
          pass = False;
        }
      } else {
        pass = False;
      }
    });
  }

  if (!pass) {
    _Count = new Int{0};

    Internal::Secure.Circle([&]() {
      Internal::RefMasters->push_back((ULong)_Count);
    });
    Init();
  }
}

Refcount::Refcount(Refcount&& src): _Status{False} {
  Bool pass = True;

  if (this == &src) {
    throw Except(EBadLogic, "reference itself");
  } else if ((_Count = src._Count)) {
    _Context = src._Context;
    _Init = src._Init;
    _Release = src._Release;

    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (RefMasters) {
        if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
          (*_Count)++;
          _Secure = src._Secure;
        } else {
          pass = False;
        }
      } else {
        pass = False;
      }
    });
  }

  if (!pass) {
    _Count = new Int{0};

    Internal::Secure.Circle([&]() {
      Internal::RefMasters->push_back((ULong)_Count);
    });

    Init();
  }
}

Refcount::Refcount(void (*init)(Refcount* thiz),
                   void (*release)(Refcount* thiz)): _Status{False} {
  _Count = new Int{0};
  _Init = init;
  _Release = release;
  _Secure = new Map<Int, Void*>();

  Internal::Secure.Circle([&]() {
    if (Internal::RefMasters == None) {
      Internal::RefMasters = new Vector<ULong>{};
    }

    Internal::RefMasters->push_back((ULong)_Count);
  });
}

Refcount::Refcount(void (*release)(Refcount* thiz)): _Status{False} {
  _Count = new Int{0};
  _Release = release;
  _Secure = new Map<Int, Void*>();

  Internal::Secure.Circle([&]() {
    if (Internal::RefMasters == None) {
      Internal::RefMasters = new Vector<ULong>{};
    }

    Internal::RefMasters->push_back((ULong)_Count);
  });
}

Refcount::Refcount(): _Status{False} {
  _Count = new Int{0};
  _Init = None;
  _Release = None;
  _Secure = new Map<Int, Void*>();

  Internal::Secure.Circle([&]() {
    if (Internal::RefMasters == None) {
      Internal::RefMasters = new Vector<ULong>{};
    }

    Internal::RefMasters->push_back((ULong)_Count);
  });
}

Bool Refcount::IsExist() {
  Bool result = False;

  /* @NOTE: this function is use to check the existence of the counter but
   * it doesn't gurantee that the counter will existence at the returning
   * time. So it would be dangerous to relay on it too much */

  if (_Count) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (RefMasters) {
        if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
          result = True;
        }
      }
    });
  }

  return result;
}

Int Refcount::Count() {
  Int result = -1;

  /* @NOTE: this function is use to get ref counting and it uses RCU-algorithm
   * to secure the resource */

  if (_Count) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (RefMasters) {
        if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
          result = *_Count;
        }
      }
    });
  }

  return result;
}

Refcount& Refcount::operator=(const Refcount& src) {
  /* @NOTE: this is very tricky since i can't gurantee that the src's counter
   * itself is existing during releasing current counter so secure then inside
   * a block would help to solve this problem */

  Internal::Secure.Circle([&]() {
    using namespace Internal;

    if (RefMasters) {
      if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
        Release(True);

        (*(_Count = src._Count))++;
        _Secure = src._Secure;
        _Context = src._Context;
        _Init = src._Init;
        _Release = src._Release;
      }
    } else {
      Bug(EBadLogic, "Internal::RefMaster shouldn\'t be None");
    }
  });

  return *this;
}

void Refcount::Init() {
  Bool pass = True;

  if (_Count) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
        pass = False;
      }
    });
  }

  if (pass && _Init) _Init(this);
}

Void Refcount::Secure(Int index, Void* address) {
  Internal::Secure.Circle([&]() {
    _Secure[index] = address;
  });
}

Void* Refcount::Access(Int index){
  Void* result = None;

  if (_Status) {
    if (_Secure.find(index) == _Secure.end()) {
      result = None;
    } else {
      result = _Secure[index];
    }
  } else {
    Internal::Secure.Circle([&]() {
      if (_Secure.find(index) == _Secure.end()) {
        result = None;
      } else {
        result = _Secure[index];
      }
    });
  }

  return result;
}

void Refcount::Release() { Release(False); }

void Refcount::Release(Bool safed) {
  Bool pass = False;

  if (_Count) {
    if (safed) {
      using namespace Internal;

      /* @NOTE: check the exitence of this counter and reduce counting */
      if (RefMasters) {
        if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
          if (*_Count == 0) {
            pass = True;
          } else if (*_Count < 0){
            Bug(EBadLogic, "Counter below zero");
          }

          (*_Count)--;
        }
      } else {
        Bug(EBadLogic, "Internal::RefMasters shouldn\'t be None")
      }
    } else {
      /* @NOTE: non-safed area should be run here */

      Internal::Secure.Circle([&]() {
        using namespace Internal;
        /* @NOTE: check the exitence of this counter and reduce counting */

        if (RefMasters) {
          if (Find(RefMasters->begin(), RefMasters->end(),
                   (ULong)_Count) >= 0) {
            if (*_Count == 0) {
              pass = True;
            } else if (*_Count < 0) {
              Bug(EBadLogic, "Counter below zero");
            }

            (*_Count)--;
          }
        } else {
          Bug(EBadLogic, "Internal::RefMasters shouldn\'t be None");
        }
      });
    }
  }

  if (pass) {
    using namespace Internal;

    if (safed) {
      if (RefMasters) {
        auto idx = Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count);

        /* @NOTE: release this counter since we are reaching the last one */
        RefMasters->erase(RefMasters->begin() + idx);
        delete _Count;
      } else {
        Bug(EBadLogic, "Internal::RefMasters shouldn\'t be None");
      }
    } else {
      /* @NOTE: non-safed area should be run here */

      Internal::Secure.Circle([&]() {
        if (RefMasters) {
          auto idx = Find(RefMasters->begin(), RefMasters->end(),
                          (ULong)_Count);

          /* @NOTE: release this counter since we are reaching the last one */
          RefMasters->erase(RefMasters->begin() + idx);
          delete _Count;
        } else {
          Bug(EBadLogic, "Internal::RefMasters shouldn\'t be None");
        }
      });
    }

    if (_Release) {
      Vertex<void> secure{[&]() { _Status = safed; },
                          [&]() { _Status = False; }};

      _Release(this);
    }

    if (safed) {
      delete _Secure;
      _Secure = None;

      if (RefMasters->size() == 0) {
        delete RefMasters;

        RefMasters = None;
      }
    } else {
      Internal::Secure.Circle([&]() {
        delete _Secure;
        _Secure = None;

        if (RefMasters->size() == 0) {
          delete RefMasters;
          RefMasters = None;
        }
      });
    }
  }

  _Count = None;
}

void Refcount::Secure(Function<void()> callback) {
  Internal::Secure.Circle([&]() {
    callback();
  });
}

Bool Refcount::Reference(const Refcount* src) {
  Bool result = False;

  /* @NOTE: this function is used to make a new reference with src which is
   * used to increase the new reference counting and decrease the old reference
   * counting */

  if (!src || src == this) {
    throw Except(EBadLogic, "reference itself");
  }

  Internal::Secure.Circle([&]() {
    using namespace Internal;

    if (RefMasters) {
      if (Find(RefMasters->begin(), RefMasters->end(), (ULong)_Count) >= 0) {
        Release(True);
      }
    } else {
      Bug(EBadLogic, "Internal::RefMasters shouldn\'t be None");
    }

    if ((result = !_Count)) {
      (*(_Count = src->_Count))++;
    }
  });

  return result;
}
} // namespace Base
