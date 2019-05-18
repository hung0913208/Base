#include <Exception.h>
#include <Monitor.h>

namespace Base {
namespace Internal {
} // namespace Internal

ErrorCodeE Monitor::Reroute(Monitor* UNUSED(child), Auto UNUSED(fd), Perform& UNUSED(callback)) {
  return NoSupport("reroute still on developing").code();
}
} // namespace Base
