#ifndef BASE_CONFIG_H_
#define BASE_CONFIG_H_
#include <Atomic.h>
#include <Lock.h>
#include <Type.h>

namespace Base {
namespace Config {
namespace Locks {
extern Base::Lock Global;
extern Mutex Error;
} // namespace Locks
} // namespace Config
} // namespace Base
#endif  // BASE_CONFIG_H_
