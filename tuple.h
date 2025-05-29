#ifndef CPP_TUPLE_H
#define CPP_TUPLE_H


template<typename ...Args>
class Tuple {
};

template<typename Type, typename Tuple>
struct exist_element {
};

template<typename Type, typename Head, typename ...Tail>
struct exist_element<Type, Tuple<Head, Tail...>> : std::conditional_t<std::is_same_v<Type, Head>,
        std::true_type, exist_element<Type, Tuple<Tail...>>> {
};

template<typename Type>
struct exist_element<Type, Tuple<>> : std::false_type {
};

template<size_t N, typename T>
struct tuple_element {
};
template<size_t N, typename Head, typename ...Tail>
struct tuple_element<N, Tuple<Head, Tail...>> {
  using type = tuple_element<N - 1, Tuple<Tail...>>::type;
};
template<typename Head, typename ...Tail>
struct tuple_element<0, Tuple<Head, Tail...>> {
  using type = Head;
};
template<size_t N, typename T>
using tuple_element_t = tuple_element<N, T>::type;

template<typename Head, typename ...Tail>
class Tuple<Head, Tail...> {
  Head head;
  Tuple<Tail...> tail;

  template<typename...>
  friend
  class Tuple;

  template<size_t N, typename ...Args>
  friend constexpr tuple_element_t<N, Tuple<Args...>>& get(Tuple<Args...>& tuple);

  template<size_t N, typename ...Args>
  friend constexpr const tuple_element_t<N, Tuple<Args...>>& get(const Tuple<Args...>& tuple);

  template<size_t N, typename ...Args>
  friend constexpr tuple_element_t<N, Tuple<Args...>>&& get(Tuple<Args...>&& tuple);

  template<typename Type, typename UHead, typename ...UTail>
  friend Type& get(Tuple<UHead, UTail...>& tuple);

  template<typename Type, typename UHead, typename ...UTail>
  friend const Type& get(const Tuple<UHead, UTail...>& tuple);

  template<typename Type, typename UHead, typename ...UTail>
  friend Type&& get(Tuple<UHead, Tail...>&& tuple);

  template<typename Type, typename UHead, typename ...UTail>
  friend const Type&& get(const Tuple<UHead, Tail...>&& tuple);

  using all_is_default_constructible = std::conjunction<std::is_default_constructible<Head>,
          std::is_default_constructible<Tail>...>;
  using all_is_copy_constructible = std::conjunction<std::is_copy_constructible<Head>,
          std::is_copy_constructible<Tail>...>;
  using all_is_move_constructible = std::conjunction<std::is_move_constructible<Head>,
          std::is_move_constructible<Tail>...>;
  using all_is_copy_assignable = std::conjunction<std::is_copy_assignable<Head>,
          std::is_copy_assignable<Tail>...>;
  using all_is_move_assignable = std::conjunction<std::is_move_assignable<Head>,
          std::is_move_assignable<Tail>...>;

  struct all_copy_list_initializable : std::conditional_t<requires(Head x) { x = {}; },
          typename Tuple<Tail...>::all_copy_list_initializable, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_size_equal : std::conditional_t<sizeof(Head) == sizeof(UHead),
          typename Tuple<Tail...>::template all_size_equal<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_size_equal<UHead> : std::conditional_t<sizeof(Head) == sizeof(UHead),
          std::true_type, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_is_convertible_from : std::conditional_t<std::is_convertible_v<UHead, Head>,
          typename Tuple<Tail...>::template all_is_convertible_from<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_is_convertible_from<UHead> : std::conditional_t<std::is_convertible_v<UHead, Head>,
          std::true_type, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_is_convertible_to : std::conditional_t<std::is_convertible_v<Head, UHead>,
          typename Tuple<Tail...>::template all_is_convertible_to<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_is_convertible_to<UHead> : std::conditional_t<std::is_convertible_v<Head, UHead>,
          std::true_type, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_is_constructible : std::conditional_t<std::is_constructible_v<Head, UHead>,
          typename Tuple<Tail...>::template all_is_constructible<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_is_constructible<UHead> : std::conditional_t<std::is_constructible_v<Head, UHead>,
          std::true_type, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_is_same : std::conditional_t<std::is_same_v<Head, UHead>,
          typename Tuple<Tail...>::template all_is_same<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_is_same<UHead> : std::conditional_t<std::is_same_v<Head, UHead>,
          std::true_type, std::false_type> {
  };

  template<typename UHead, typename ...UTail>
  struct all_is_assignable : std::conditional_t<std::is_assignable_v<Head&, UHead>,
          typename Tuple<Tail...>::template all_is_assignable<UTail...>, std::false_type> {
  };
  template<typename UHead>
  struct all_is_assignable<UHead> : std::conditional_t<std::is_assignable_v<Head&, UHead>,
          std::true_type, std::false_type> {
  };

public:
  explicit(!all_copy_list_initializable::value)
  Tuple()
  requires(all_is_default_constructible::value)
          : head(), tail() {}

  explicit(!all_is_convertible_from<const Head&, const Tail& ...>::value)
  Tuple(const Head& head, const Tail& ...tail)
  requires(all_is_copy_constructible::value)
          : head(head), tail(tail...) {}

  template<typename UHead, typename ...UTail>
  explicit(!all_is_convertible_to<UHead, UTail...>::value)
  Tuple(UHead&& head, UTail&& ... tail)
  requires(all_is_constructible<UHead, UTail...>::value)
          : head(std::forward<UHead>(head)), tail(std::forward<UTail>(tail)...) {}

  Tuple(const Tuple& other) requires all_is_copy_constructible::value
          : head(other.head), tail(other.tail) {}

  Tuple(const Tuple& other) requires (!all_is_copy_constructible::value) = delete;

  Tuple(Tuple&& other)
          : head(std::move(other.head)),
            tail(std::move(other.tail)) {}

  template<typename UHead, typename ...UTail>
  explicit(!all_is_convertible_from<UHead, UTail...>::value)
  Tuple(const Tuple<UHead, UTail...>& other)
  requires (all_is_constructible<UHead, UTail...>::value &&
  (sizeof(Head) != 1 && sizeof...(Tail) != 1 || !all_is_convertible_from<decltype(other)>::value &&
  !all_is_constructible<decltype(other)>::value && !all_is_same<UHead, UTail...>::value))
          : head(other.head), tail(other.tail) {
    static_assert(all_is_copy_constructible::value);
  }

  template<typename UHead, typename ...UTail>
  explicit(!all_is_convertible_from<UHead, UTail...>::value)
  Tuple(Tuple<UHead, UTail...>&& other)
  requires (all_is_constructible<UHead, UTail...>::value &&
  (sizeof(Head) != 1 && sizeof...(Tail) != 1 || !all_is_convertible_from<decltype(other)>::value &&
  !all_is_constructible<decltype(other)>::value && !all_is_same<UHead, UTail...>::value))
          : head(get<0>(std::forward<decltype(other)>(other))),
            tail(std::move(other.tail)) {}

  Tuple& operator=(const Tuple& other) requires all_is_copy_assignable::value {
    if (&other == this) {
      return *this;
    }
    head = other.head;
    tail = other.tail;
    return *this;
  }

  Tuple& operator=(const Tuple& other) requires (!all_is_copy_assignable::value) = delete;

  Tuple& operator=(Tuple&& other) noexcept requires all_is_move_assignable::value {
    head = std::forward<Head>(get<0>(other));
    tail = std::move(other.tail);
    return *this;
  }

  Tuple& operator=(Tuple&& other) noexcept requires (!all_is_move_assignable::value) = delete;

  template<typename UHead, typename ...UTail>
  Tuple& operator=(const Tuple<UHead, UTail...>& other)
          requires (all_is_assignable<const UHead&, const UTail& ...>::value&&
          all_size_equal<UHead, UTail...>::value && all_is_copy_assignable::value) {
    head = other.head;
    tail = other.tail;
    return *this;
  }

  template<typename UHead, typename ...UTail>
  Tuple& operator=(const Tuple<UHead, UTail...>& other)
          requires (all_is_assignable<const UHead&, const UTail& ...>::value&&
          all_size_equal<UHead, UTail...>::value && !all_is_copy_assignable::value) = delete;

  template<typename UHead, typename ...UTail>
  Tuple& operator=(Tuple<UHead, UTail...>&& other)
          requires (all_is_assignable<UHead, UTail...>::value &&
          all_size_equal<UHead, UTail...>::value) {
    head = std::forward<UHead>(get<0>(other));
    tail = std::move(other.tail);
    return *this;
  }

  template<typename T1, typename T2>
  Tuple(const std::pair<T1, T2>& pair) : head(pair.first), tail(pair.second) {}

  template<typename T1, typename T2>
  Tuple(std::pair<T1, T2>&& pair) noexcept : head(std::move(pair.first)),
                                             tail(std::move(pair.second)) {}

  template<typename T1, typename T2>
  Tuple& operator=(const std::pair<T1, T2>& pair) {
    head = pair.first;
    tail = pair.second;
    return *this;
  }

  template<typename T1, typename T2>
  Tuple& operator=(std::pair<T1, T2>&& pair) {
    head = std::move(pair.first);
    tail = std::move(pair.second);
    return *this;
  }
};

template<typename T1, typename T2>
Tuple(const std::pair<T1, T2>&) -> Tuple<T1, T2>;

template<typename T1, typename T2>
Tuple(std::pair<T1, T2>&&) -> Tuple<T1, T2>;

template<>
class Tuple<> {
  template<typename...>
  friend
  class Tuple;

  struct all_copy_list_initializable : std::true_type {
  };
};

template<size_t N, typename ...Args>
constexpr tuple_element_t<N, Tuple<Args...>>& get(Tuple<Args...>& tuple) {
  if constexpr (N == 0) {
    return tuple.head;
  } else {
    return get<N - 1>(tuple.tail);
  }
}

template<size_t N, typename ...Args>
constexpr const tuple_element_t<N, Tuple<Args...>>& get(const Tuple<Args...>& tuple) {
  if constexpr (N == 0) {
    return tuple.head;
  } else {
    return get<N - 1>(tuple.tail);
  }
}

template<size_t N, typename ...Args>
constexpr tuple_element_t<N, Tuple<Args...>>&& get(Tuple<Args...>&& tuple) {
  if constexpr (N == 0) {
    return std::forward<decltype(tuple.head)>(tuple.head);
  } else {
    return get<N - 1>(std::move(tuple.tail));
  }
}

template<typename Type, typename UHead, typename ...UTail>
Type& get(Tuple<UHead, UTail...>& tuple) {
  if constexpr (std::is_same_v<Type, UHead>) {
    static_assert(!exist_element<Type, Tuple<UTail...>>::value);
    return tuple.head;
  } else {
    return get<Type>(tuple.tail);
  }
}

template<typename Type, typename UHead, typename ...UTail>
const Type& get(const Tuple<UHead, UTail...>& tuple) {
  if constexpr (std::is_same_v<Type, UHead>) {
    return tuple.head;
  } else {
    return get<Type>(tuple.tail);
  }
}

template<typename Type, typename UHead, typename ...UTail>
Type&& get(Tuple<UHead, UTail...>&& tuple) {
  if constexpr (std::is_same_v<Type, UHead>) {
    return std::forward<decltype(tuple.head)>(tuple.head);
  } else {
    return get<Type>(std::move(tuple.tail));
  }
}

template<typename Type, typename UHead, typename ...UTail>
const Type&& get(const Tuple<UHead, UTail...>&& tuple) {
  if constexpr (std::is_same_v<Type, UHead>) {
    return std::forward<decltype(tuple.head)>(tuple.head);
  } else {
    return get<Type>(std::move(tuple.tail));
  }
}

template<typename Tuple>
struct tupleSize;
template<typename... Types>
struct tupleSize<Tuple<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {
};

template<typename ...Args>
Tuple<std::decay_t<Args>...> makeTuple(Args&& ... args) {
  return {std::forward<Args>(args)...};
}

template<typename ...Args>
Tuple<Args& ...> tie(Args& ... args) {
  return {args...};
}

template<typename ...Args>
Tuple<Args&& ...> forwardAsTuple(Args&& ... args) {
  return {std::forward<Args>(args)...};
}

template<typename T1, typename T2>
struct catTypeHelper;

template<typename... T1, typename... T2>
struct catTypeHelper<Tuple<T1...>, Tuple<T2...>> {
  using type = Tuple<T1..., T2...>;
};

template<typename... T1, typename... T2>
struct catTypeHelper<Tuple<T1...>&, Tuple<T2...>&> {
  using type = Tuple<T1..., T2...>;
};

template<typename... T>
struct catsTypeHelper;

template<typename... T>
struct catsTypeHelper<Tuple<T...>> {
  using type = Tuple<T...>;
};
template<typename... T>
struct catsTypeHelper<Tuple<T...>&> {
  using type = Tuple<T...>;
};

template<typename T1, typename T2, typename... Other>
struct catsTypeHelper<T1, T2, Other...> {
  using type = typename catsTypeHelper<typename catTypeHelper<T1, T2>::type, Other...>::type;
};

template<typename... T>
using catsType = typename catsTypeHelper<T...>::type;

template<typename Res, typename Seq>
struct resultTuple;

template<typename Res, size_t... Ind>
struct resultTuple<Res, std::index_sequence<Ind...>> {
  template<typename Tuple>
  static constexpr auto create(Tuple&& t) {
    return Res(get<Ind>(std::forward<Tuple>(t))...);
  }
};

template<typename Seq1, typename Seq2>
struct concat;

template<size_t... Ind1, size_t... Ind2>
struct concat<std::index_sequence<Ind1...>, std::index_sequence<Ind2...>> {
  template<typename Tuple1, typename Tuple2>
  static constexpr auto cat(Tuple1&& t1, Tuple2&& t2) {
    return forwardAsTuple(get<Ind1>(std::forward<Tuple1>(t1))...,
                          get<Ind2>(std::forward<Tuple2>(t2))...);
  }
};

template<typename Res>
struct tupleCatHelper {
  template<typename Tuple1, typename Tuple2, typename... Tuples>
  static constexpr auto cat(Tuple1&& t1, Tuple2&& t2, Tuples&& ... ts) {
    return cat(concat<std::make_index_sequence<tupleSize<std::remove_cvref_t<Tuple1>>::value>,
                       std::make_index_sequence<tupleSize<std::remove_cvref_t<Tuple2>>::value>>::cat(
                       std::forward<Tuple1>(t1),
                       std::forward<Tuple2>(t2)),
               std::forward<Tuples>(ts)...);
  }

  template<typename Tuple>
  static constexpr auto cat(Tuple&& t) {
    return resultTuple<Res, std::make_index_sequence<tupleSize<Tuple>::value>>::create(
            std::forward<Tuple>(t));
  }
};

template<typename... Tuples>
constexpr auto tupleCat(Tuples&& ... tuples) {
  return tupleCatHelper<catsType<Tuples...>>::cat(std::forward<Tuples>(tuples)...);
}

#endif //CPP_TUPLE_H
