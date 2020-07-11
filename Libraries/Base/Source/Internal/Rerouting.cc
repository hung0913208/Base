#include <Atomic.h>
#include <Exception.h>
#include <Macro.h>
#include <Monitor.h>
#include <List.h>
#include <Lock.h>
#include <Logcat.h>
#include <Thread.h>
#include <Type.h>
#include <Vertex.h>
#include <Utils.h>

#include <signal.h>
#include <thread>

using TimeSpec = struct timespec;

namespace Base {
namespace Internal {
void Idle(TimeSpec* spec) {
  nanosleep(spec, None);
}

Int Random(Int min, Int max) {
  static Bool first = True;
        
  if (first) {  
    srand(time(None)); //seeding for the first time only!
    first = False;
  }

  return min + rand() % (( max + 1 ) - min);
}
} // namespace Internal
} // namespace Base
