#if USE_RUBY && !defined(BASE_RUBY_WRAPPER_HH_)
#define BASE_RUBY_WRAPPER_HH_

#ifndef BASE_WRAPPING_H_
#include <Wrapping.h>
#endif

namespace Base {
class Ruby: Wrapping {
};
} // namespace Base

#define RUBY_MODULE(Module)

#if INTERFACE == RUBY
#define MODULE RUBY_MODULE
#endif
#endif  // BASE_PYTHON_WRAPPER_HH_
