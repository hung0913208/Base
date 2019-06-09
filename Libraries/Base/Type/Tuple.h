#if !defined(BASE_TYPE_TUPLE_H_) && !USE_STD_TUPLE
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
template<unsigned I, class T>
struct Element{ T value; };

template<class Seq, class... Ts> struct Gather;

template<unsigned... Is, class... Ts>
struct Gather<Implement::Sequence<Is...>, Ts...> : Element<Is, Ts>...{
  template <unsigned I, typename T>
  using Element = Tuple::Element<I, T>;

  using BaseType = Gather;

  template<class... Us>
  Gather(Us&&... us) : Element<Is, Ts>{static_cast<Us&&>(us)}...{}
};
} // namespace Tuple
} // namespace Implement

template<class... Ts>
struct Tuple :
      Implement::Tuple::Gather<typename Sequence<sizeof...(Ts)>::Type, Ts...> {
  template<class... Us>
  Tuple(Us&... us) : Tuple::BaseType(static_cast<Us&&>(us)...){}

  template<class... Us>
  Tuple(Us&&... us) : Tuple::BaseType(static_cast<Us&&>(us)...){}

  template<unsigned I, class T>
  static T& Get(Implement::Tuple::Element<I, T>& e){ return e.value; }
};

template<>
struct Tuple<> {
  template<typename ...Ts>
  static Tuple<Ts...> Make(Ts&&... value) {
    return Tuple<Ts...>(value...);
  }

  template<typename ...Ts>
  static Tuple<Ts...> Make(const Ts&... value) {
    return Tuple<Ts...>(value...);
  }
};
} // namespace Base
#endif  // BASE_TYPE_TUPLE_H_
