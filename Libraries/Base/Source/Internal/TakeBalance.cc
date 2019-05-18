#include <Exception.h>
#include <Monitor.h>

namespace Base {
namespace Internal {
} // namespace Internal

ErrorCodeE Monitor::Reroute(Monitor* child, Auto fd, Perform& callback) {
  return ENoSupport;
}
} // namespace Base
