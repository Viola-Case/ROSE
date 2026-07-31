/**

    @file      tuple.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      30.07.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

namespace ROSE {

  template<size_t Idx, typename TupleT>
  struct TupleElement;

  template<typename Head, typename... Tail>
  struct Tuple : public Tuple<Tail...> {
    Head value;
    Tuple() : Tuple<Tail...>(), value() {}

    Tuple(Head head, Tail... tail) : Tuple<Tail...>(tail...), value(head) {}

    template <size_t Idx, typename ...Types>
    auto &Get(Tuple<Types...> &t) {
      using BaseType = typename TupleElement<Idx, Tuple<Types...>>::TupleType;
      return static_cast<BaseType &>(t).value;
    }
  };

  template<typename Head, typename... Tail>
  struct TupleElement<0, Tuple<Head, Tail...>> {
    using Type = Head;
    using TupleType = Tuple<Head, Tail...>;
  };

  template<size_t Idx, typename Head, typename... Tail>
  struct TupleElement<Idx, Tuple<Head, Tail...>> {
    using Type = typename TupleElement<Idx-1, Tuple<Tail...>>::Type;
    using TupleType = typename TupleElement<Idx-1, Tuple<Tail...>>::TupleType;
  };


}