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
} // namespace Internal

ErrorCodeE Monitor::Reroute(Monitor* UNUSED(child), Auto UNUSED(fd), Perform& UNUSED(callback)) {
	return ENoSupport;
}
} // namespace Base
