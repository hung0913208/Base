#if !defined(BASE_VERTEX_H_) && __cplusplus
#define BASE_VERTEX_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Property.h>
#include <Base/Type.h>
#else
#include <Property.h>
#include <Type.h>
#endif

namespace Base {
template <typename Type, Bool templated = False>
class Vertex {
 public:
  using Enter = Function<void(Type*)>;
  using Exit = Function<void(Type*)>;
  using Delete = Function<void(Type*)>;

  virtual ~Vertex() {
    if (_Exit && templated == False) {
      (*_Exit)(_Content);
    }

    if (_Delete) {
      (*_Delete)(_Content);

      delete _Delete;
      _Delete = None;
    }

    if (_Exit) {
      delete _Exit;
      _Exit = None;
    }

    if (_Enter) {
      delete _Enter;
      _Enter = None;
    }
  }

  explicit Vertex(Exit exit) : _Called{True} {
    if (exit) {
      _Exit = new Exit(exit);
    }
  }

  explicit Vertex(Exit exit, Type&& value) : _Content{&value}, _Called{True} {
    if (exit) {
      _Exit = new Exit(exit);
    }
  }

  explicit Vertex(Exit exit, Type& value) : _Content{&value}, _Called{True} {
    if (exit) {
      _Exit = new Exit(exit);
    }
  }

  explicit Vertex(Enter enter, Exit exit) : _Called{False} {
    if (enter) {
      _Enter = new Enter(enter);
    }
    if (exit) {
      _Exit = new Exit(exit);
    }
  }

  explicit Vertex(Enter enter, Exit exit, Type* value)
      : _Content{value}, _Called{False} {
    if (templated == True) {
      _Delete = new Delete([](Type* context) { free(context); });
    }

    if (enter) {
      _Enter = new Enter(enter);
    }
    if (exit) {
      _Exit = new Exit(exit);
    }

    if (_Enter && templated == False) {
      (*_Enter)(_Content);
    }

    _Called = !templated;
  }

  explicit Vertex(Enter enter, Exit exit, Delete del, Type* value)
      : _Content{value}, _Called{False} {
    if (del && templated == True) {
      _Delete = new Delete(del);
    }

    if (enter) {
      _Enter = new Enter(enter);
    }

    if (exit) {
      _Exit = new Exit(exit);
    }

    if (_Enter && templated == False) {
      (*_Enter)(_Content);
    }

    _Called = !templated;
  }

  explicit Vertex(Enter enter, Exit exit, Type& value)
      : _Content{&value}, _Called{False} {
    if (enter) {
      _Enter = new Enter(enter);
    }

    if (exit) {
      _Exit = new Exit(exit);
    }

    if (_Enter && templated == False) {
      (*_Enter)(_Content);
    }

    _Called = !templated;
  }

  explicit Vertex(Vertex<Type, True>&& src) {
    if (templated == True) {
      throw Exception(ENoSupport);
    } else {
      _Content = src._Content;
      _Called = False;
      _Enter = src._Enter;
      _Exit = src._Exit;
    }
  }

  explicit Vertex(Vertex<Type, False>&& src) {
    if (templated == True) {
      throw Exception(ENoSupport);
    } else {
      _Content = src._Content;
      _Called = False;
      _Enter = src._Enter;
      _Exit = src._Exit;
    }
  }

  Vertex(const Vertex<Type, True>& src) {
    if (templated == True) {
      throw Exception(ENoSupport);
    } else {
      _Content = src._Content;
      _Called = False;
      _Enter = src._Enter;
      _Exit = src._Exit;
    }
  }

  Vertex(const Vertex<Type, False>& src) {
    if (templated == True) {
      throw Exception(ENoSupport);
    } else {
      _Content = src._Content;
      _Called = False;
      _Enter = src._Enter;
      _Exit = src._Exit;
    }
  }

  void Circle(Function<void()> callback) {
    if (templated == True) {
      if (_Enter) {
        (*_Enter)(_Content);
      }
      try {
        callback();

        if (_Exit) {
          (*_Exit)(_Content);
        }
      } catch (Base::Exception& except) {
        if (_Exit) {
          (*_Exit)(_Content);
        }

        throw except;
      } catch (std::exception& except) {
        if (_Exit) {
          (*_Exit)(_Content);
        }

        throw except;
      } catch (...) {
        if (_Exit) {
          (*_Exit)(_Content);
        }

        throw Except(EBadLogic, "catch unknown exception");
      }
    }
  }

  /* @NOTE: if templated is False, call this object will activate function enter
   * and after that, function exit will be called automatically at the end of
   * this object lifecycle
   */
  void operator()(Type&& value) {
    if (_Called == False && templated == False) {
      _Content = &value;

      if (_Enter) {
        (*_Enter)(_Content);
      }

      _Called = True;
    }
  }

  void operator()(Type& value) {
    if (_Called == False && templated == False) {
      _Content = &value;

      if (_Enter) {
        (*_Enter)(_Content);
      }

      _Called = True;
    }
  }

  /* @NOTE: this function is used to generate itself if Vertex is intended to
   * use like a Vertex builder
   */
  Vertex<Type, False> generate() {
    if (_Content == None) {
      if (_Enter) {
        return Vertex<Type, False>(*_Enter, *_Exit);
      } else {
        return Vertex<Type, False>(*_Exit);
      }
    } else {
      if (_Enter) {
        return Vertex<Type, False>(*_Enter, *_Exit, *_Content);
      } else {
        return Vertex<Type, False>(*_Exit, *_Content);
      }
    }
  }

  /* @NOTE: this property will provide a easy way to access to member _Content
   * but we can't modify it from out side
   */
  Property<Type> content;

 private:
  /* @NOTE: define parameters of this object */
  Type* _Content = None;
  Bool _Called = False;

  /* @NOTE: define callback exit and enter */
  Delete* _Delete = None;
  Enter* _Enter = None;
  Exit* _Exit = None;
};

template <>
class Vertex<void, False> {
 public:
  using Enter = Function<void()>;
  using Exit = Function<void()>;

  explicit Vertex(Enter enter, Exit exit = None) : _Enter{enter}, _Exit{exit} {
    if (_Enter) {
      _Enter();
    }
  }

  explicit Vertex(Exit exit) : _Enter{None}, _Exit{exit} {}

  virtual ~Vertex() {
    if (_Exit) {
      _Exit();
    }
  }

 private:
  Enter _Enter = None;
  Exit _Exit = None;
};
}  // namespace Base
#endif  // BASE_VERTEX_H_
