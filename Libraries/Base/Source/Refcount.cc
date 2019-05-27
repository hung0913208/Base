#define BASE_REFCOUNT_CC_
#include <Exception.h>
#include <Lock.h>
#include <Logcat.h>
#include <Type.h>
#include <Vertex.h>

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Vector<ULong> RefMasters{};
static Vertex<Mutex, True> Secure([](Mutex* mutex) { pthread_mutex_lock(mutex); },
                                  [](Mutex* mutex) { pthread_mutex_unlock(mutex); },
                                  CreateMutex());
} // namespace Internal

Refcount::~Refcount() { Release(); }

Refcount::Refcount(const Refcount& src) {
  Bool pass = True;

  if (this == &src) {
    throw Except(EBadLogic, "reference itself");
  } else if ((_Count = src._Count)) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        (*_Count)++;
      } else {
        pass = False;
      }
    });
  }

  if (!pass) {
    _Count = new Int{0};

    Internal::Secure.Circle([&]() {
      Internal::RefMasters.push_back((ULong)_Count);
    });
    Init();
  }
}

Refcount::Refcount(Refcount&& src) {
  Bool pass = True;

  if (this == &src) {
    throw Except(EBadLogic, "reference itself");
  } else if ((_Count = src._Count)) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        (*_Count)++;
      } else {
        pass = False;
      }
    });
  }

  if (!pass) {
    _Count = new Int{0};

    Internal::Secure.Circle([&]() {
      Internal::RefMasters.push_back((ULong)_Count);
    });

    Init();
  }
}

Refcount::Refcount() {
  _Count = new Int{0};

  Internal::Secure.Circle([&]() {
    Internal::RefMasters.push_back((ULong)_Count);
  });
  Init();
}

Bool Refcount::IsExist() {
  Bool result = False;

  /* @NOTE: this function is use to check the existence of the counter but
   * it doesn't gurantee that the counter will existence at the returning
   * time. So it would be dangerous to relay on it too much */

  if (_Count) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        result = True;
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

      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        result = *_Count;
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

    if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
      Release(True);
      (*(_Count = src._Count))++;
    }
  });

  return *this;
}

void Refcount::Init() {
  Bool pass = True;

  if (_Count) {
    Internal::Secure.Circle([&]() {
      using namespace Internal;

      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        pass = False;
      }
    });
  }

  if (pass && _Init) _Init();
}

void Refcount::Release() { Release(False); }

void Refcount::Release(Bool safed) {
  Bool pass = False;

  if (_Count) {
    if (safed) {
      using namespace Internal;

      /* @NOTE: check the exitence of this counter and reduce counting */
      if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
        if (!(pass = *_Count == 0)) {
          (*_Count)--;
        } else if (*_Count < 0){
          Bug(EBadLogic, "Counter below zero");
        } else {
          pass = True;
        }
      }
    } else {
      /* @NOTE: non-safed area should be run here */

      Internal::Secure.Circle([&]() {
        using namespace Internal;
        /* @NOTE: check the exitence of this counter and reduce counting */

        if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
          if (!(pass = *_Count == 0)) {
            (*_Count)--;
          } else if (*_Count < 0) {
            Bug(EBadLogic, "Counter below zero");
          } else {
            pass = True;
          }
        }
      });
    }
  }

  if (pass) {
    using namespace Internal;

    if (safed) {
      auto idx = Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count);

      /* @NOTE: release this counter since we are reaching the last one */
      RefMasters.erase(RefMasters.begin() + idx);
      delete _Count;
    } else {
      /* @NOTE: non-safed area should be run here */

      Internal::Secure.Circle([&]() {
        auto idx = Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count);

        /* @NOTE: release this counter since we are reaching the last one */
        RefMasters.erase(RefMasters.begin() + idx);
        delete _Count;
      });
    }

    if (_Release) _Release();
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

    if (Find(RefMasters.begin(), RefMasters.end(), (ULong)_Count) >= 0) {
      Release(True);
    }

    if ((result = !_Count)) {
      (*(_Count = src->_Count))++;
    }
  });

  return result;
}
} // namespace Base
