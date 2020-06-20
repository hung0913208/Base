#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#else
#include <Macro.h>
#endif

#if !defined(BASE_TYPE_TUPLE_H_) && (USE_BASE_TUPLE || APPLE)
#define BASE_TYPE_TUPLE_H_

namespace Base {
namespace Implement {
template<unsigned...> struct Sequence{ using Type = Sequence; };
} // namespace Implement

template<unsigned I, unsigned... Is>
struct Sequence : Sequence<I - 1, I - 1, Is...>::Type{};

template<unsigned... Is>
struct Sequence<0, Is...> : Implement::Sequence<Is...>{};

namespace Implement {
namespace Tuple {
template<unsigned I, typename T>
struct Element{ 
  typedef T Type;

  T value; 
};

template<typename Seq, typename... Ts> struct Gather;

template<unsigned... Is, typename... Ts>
struct Gather<Implement::Sequence<Is...>, Ts...> : Element<Is, Ts>...{
  template <unsigned I, typename T>
  using Element = Tuple::Element<I, T>;

  using BaseType = Gather;

  template<class... Us>
  Gather(Us&&... us) : Element<Is, Ts>{static_cast<Us&&>(us)}...{}
};
} // namespace Tuple
} // namespace Implement

template<typename... Ts>
struct Tuple :
      Implement::Tuple::Gather<typename Sequence<sizeof...(Ts)>::Type, Ts...> {
  template<class... Us>
  Tuple(Us&... us) : Tuple::BaseType(static_cast<Us&&>(us)...){}

  template<class... Us>
  Tuple(Us&&... us) : Tuple::BaseType(static_cast<Us&&>(us)...){}
};

template<>
struct Tuple<> {
  template<typename... Ts>
  static Tuple<Ts...> Make(Ts&&... value) {
    return Tuple<Ts...>(value...);
  }

  template<typename... Ts>
  static Tuple<Ts...> Make(const Ts&... value) {
    return Tuple<Ts...>(value...);
  }

  template<unsigned I, typename T>
  static T& Get(Implement::Tuple::Element<I, T>& e){ return e.value; }
};


template<unsigned I, typename T>
T& Get(Implement::Tuple::Element<I, T>& e) { return e.value; }

template<unsigned I, typename T>
T& Get(Implement::Tuple::Element<I, T>&& e) { return e.value; }
} // namespace Base
#else
#include <tuple>

namespace Base {
template<std::size_t __i, typename... _Elements>
  constexpr const std::__tuple_element_t<__i, std::tuple<_Elements...>>&
Get(const std::tuple<_Elements...>& __t) noexcept {
  return std::__get_helper<__i>(__t);
}

template<std::size_t __i, typename... _Elements>
  constexpr std::__tuple_element_t<__i, std::tuple<_Elements...>>&&
Get(std::tuple<_Elements...>&& __t) noexcept {
  typedef std::__tuple_element_t<__i, std::tuple<_Elements...>> __element_type;
    
  return std::forward<__element_type&&>(std::get<__i>(__t));
}

template<std::size_t __i, typename... _Elements>
  constexpr const std::__tuple_element_t<__i, std::tuple<_Elements...>>&&
Get(const std::tuple<_Elements...>&& __t) noexcept {
  typedef std::__tuple_element_t<__i, std::tuple<_Elements...>> __element_type;

  return std::forward<const __element_type&&>(std::get<__i>(__t));
}
} // namespace Base
#endif  // BASE_TYPE_TUPLE_H_
