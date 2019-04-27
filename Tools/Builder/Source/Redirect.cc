#include <Build.h>
#include <thread>

#define IsSingleCoreCPU()                   \
  std::thread::hardware_concurrency() == 1

using namespace Base;

namespace Build {
namespace Internal {
Shared<Redirect> MakeRedirect(String UNUSED(type), Vector<CString> UNUSED(parameters)) {
    return std::make_shared<Redirect>();
}
} // namespace Internal

Redirect::~Redirect() {
  for (auto& node: _Status) {
    Bool done = True;

    if (node.Right) {
      /* @NOTE: call depart several time to make sure that i have done
       *  closing connection with the node */

      while (Depart(node.Left)) {
        /* @NOTE: check heartbeat to make sure the connection is stable */

        if (!(done = (Heartbeat(node.Left) != ENoError))) {
          DEBUG(Format{"Lost contact with {}"}.Apply(node.Left));
        }
      }

      if (done) {
        DEBUG(Format{"Good bye {}, see you soon"}.Apply(node.Left));
      }
    }
  }
}

ErrorCodeE Redirect::Register(Shared<Plugin> plugin) {
  /* @NOTE: register a new plugin to solve specific problems */
  return ENoSupport;
}

ErrorCodeE Redirect::RunAtHere(Shared<Base::Rule> rule) {
  return Consume(rule);
}

ErrorCodeE Redirect::Put(Shared<Base::Rule> rule) {
  auto apply = False;

  /* @NOTE: this is an IOSync since we will lock the thread until we select
   *  a best place to apply the job */

  if (Status() >= Redirect::EOverload) {
    do {
      /* @NOTE: lock the system until system cool down or there was another
       *  node adapts this job */

      if (!Broadcast(rule)) {
        return ENoError;
      } else if ((Status() < Redirect::EOverload)) {
        apply = True;
      } else {
        auto keep = True;

        /* @NOTE: check status with remote nodes to make sure our connections
         *  still be alive */
        for (auto& node: _Status) {
          if (!(keep = !Heartbeat(node.Left))) {
            break;
          } else if (Status() < Redirect::EOverload) {
            break;
          }
        }

        /* @NOTE: if connection still alive, we must do again to */
        if (!keep) {
          return EDoAgain;
        }
      }
    } while (!apply);
  } else {
    apply = True;
  }

  /* @NOTE: the node has been closed recently thus we can't apply anything
   * from now on */
  if (Status() == EClosed) {
    return EDoNothing;
  }

  if (apply) {
    /* @NOTE: we will force to run the job right here if our system is single
     * core cpu, doing this will lock the system but we can optimize the way
     * of working better since our resource must be used to consume the jobs
     **/

    if (IsSingleCoreCPU()) {
      return Consume(rule);
    } else {
      _Jobqueue.push(rule);
    }
  }

  return ENoError;
}

ErrorCodeE Redirect::Code() {
  return _Code;
}

ErrorCodeE Redirect::Consume(Shared<Base::Rule> rule) {
  return ENoError;
}

ErrorCodeE Redirect::Transit(Shared<Base::Rule> UNUSED(rule)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Broadcast(Shared<Base::Rule> UNUSED(rule)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Done(Shared<Base::Rule> UNUSED(rule)) {
  return ENoError;
}

ErrorCodeE Redirect::Heartbeat(String UNUSED(uri)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Join(String UNUSED(uri)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Depart(String UNUSED(uri)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Lock(String UNUSED(uri)) {
  return ENoSupport;
}

ErrorCodeE Redirect::Unlock(String UNUSED(uri)) {
  return ENoSupport;
}

Redirect::StatusE Redirect::Status() {
  return EGood;
}
} // namespace Build
