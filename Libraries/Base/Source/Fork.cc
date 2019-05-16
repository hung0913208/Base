#include <Config.h>
#include <Logcat.h>
#include <Utils.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace Base {
namespace Internal {
static Vector<Fork*> Forks;

Bool IsPipeAlive(Int pipe) {
  Bool result = False;

  Configs::Locks::Global.Safe([&]() {
    for (auto& fork: Forks) {
      if ((fork->Input() != pipe) && (fork->Output() != pipe) &&
          (fork->Error() != pipe)) {
        continue;
      }

      result = fork->Status() >= 0;
      break;
    }
  });

  return result;
}

Bool IsPipeWaiting(Int pipe) {
  Bool result = False;

  Configs::Locks::Global.Safe([&]() {
    for (auto& fork: Forks) {
      if ((fork->Output() != pipe) && (fork->Error() != pipe)) {
        continue;
      }

      result = True;
      break;
    }
  });

  return result;
}
} // namespace Internal

Fork::Fork(Function<Int()> callback, Bool redirect): 
    _PID{-1}, _Input{-1}, _Output{-1}, _Error{-1}, _ECode{None} {
  Int input[2], output[2], error[2];

  if (redirect) {
    pipe(input);
    pipe(output);
    pipe(error);
  }

  if ((_PID = fork()) < 0) {
    throw Except(EWatchErrno, "");
  } else if (_PID == 0) {
    Internal::Forks.clear();

    /* @NOTE: redirect standard I/O to the pipelines using function dup2 */
    if (redirect) {
      dup2(input[0], STDIN_FILENO);
      dup2(error[1], STDERR_FILENO);
      dup2(output[1], STDOUT_FILENO);
    }

    /* @NOTE: close the pipeline since we have duplicated the pipelines and
     * redirect the standard I/O to them so we don't need to consider it
     * too much recently */
    if (redirect) {
      close(input[0]); close(input[1]);
      close(error[0]); close(error[1]);
      close(output[0]); close(output[1]);
    }

    exit(callback());
  } else {
    if (redirect) {
      _Input = input[1];
      _Error = error[0];
      _Output = output[0];

      close(input[0]);
      close(error[1]);
      close(output[1]);
    }

    _ECode = new Int(-1);
    Internal::Forks.push_back(this);
  }
}

Fork::~Fork() {
  /* @NOTE: just to make sure everything is safe from a single process POV, i
   * need to secure the cleaning process here with the highest lock */

  Configs::Locks::Global.Safe([&]() {
    for (UInt i = 0; i < Internal::Forks.size(); ++i) {
      /* @NOTE: search the fork on our database and clean it now to prevent bad
       * access if another threads try to access the deadly fork like this */

      if (Internal::Forks[i] == this) {
        Internal::Forks.erase(Internal::Forks.begin() + i);

        /* @NOTE: close pipeline from here, we don't need them and we should
         * notify monitors too about this event */
        close(_Input);
        close(_Output);
        close(_Error);

        /* @NOTE: free shared memory */
        if (_ECode) delete _ECode;
        break;
      }
    }
  });
}

Int Fork::PID() { return _PID; }

Int Fork::Input() { return _Input; }

Int Fork::Error() { return _Error; }

Int Fork::Output() { return _Output; }

Int Fork::ECode() { return *_ECode; }

Fork::StatusE Fork::Status() {
  Int status;

  if (waitpid(_PID, &status, WUNTRACED | WCONTINUED) < 0) {
    return Fork::EBug;
  }

  /* @NOTE: check if we catch a signal continue from children */
  if (WIFCONTINUED(status)) {
    return Fork::EContSigned;
  }

  /* @NOTE: check if the child was stoped by delivered signals */
  if (WIFSTOPPED(status)) {
    if (WSTOPSIG(status)) {
      return Fork::EStopSigned;
    }
  }

  /* @NOTE: check if the child was crash by unavoided signals */
  if (WIFSIGNALED(status)) {
    if (WCOREDUMP(status)) {
      return Fork::ESegmented;
    } else if (WTERMSIG(status)) {
      return Fork::ETerminated;
    } else {
      return Fork::EInterrupted;
    }
  }

  /* @NOTE: check if the child was exited and collect its exit code */
  if (WIFEXITED(status)) {
    DEBUG(Format{"pid {}\'s exit code {}"}.Apply(_PID, WEXITSTATUS(status)));

    if (*_ECode < 0) {
      *_ECode = WEXITSTATUS(status);
    }
    return Fork::EExited;
  }

  return Fork::ERunning;
}
} // namespace Base
