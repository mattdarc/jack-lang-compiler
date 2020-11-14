#ifndef jcc_DataStructures_hpp
#define jcc_DataStructures_hpp

namespace jcc::ds {

template <typename FirstType, typename SecondType, FirstType FirstValue,
          SecondType SecondValue>
struct entry {
  using first_type = FirstType;
  using second_type = SecondType;
  static constexpr FirstType first_value = FirstValue;
  static constexpr SecondType second_value = SecondValue;
};

template <typename Default, typename...>
struct constexpr_bimap;

template <typename Default>
struct constexpr_bimap<Default> {
  template <typename Default::first_type>
  struct get_second {
    static constexpr typename Default::second_type value =
        Default::second_value;
  };
  template <typename Default::second_type>
  struct get_first {
    static constexpr typename Default::first_type value = Default::first_value;
  };
};

template <typename Default, typename Default::first_type FirstKey,
          typename Default::second_type SecondKey, typename... Rest>
struct constexpr_bimap<
    Default,
    entry<typename Default::first_type, typename Default::second_type, FirstKey,
          SecondKey>,
    Rest...> {
  template <typename Default::first_type Key>
  struct get_second {
    static constexpr typename Default::second_type value =
        (FirstKey == Key)
            ? SecondKey
            : constexpr_bimap<Default,
                              Rest...>::template get_second<Key>::value;
  };
  template <typename Default::second_type Key>
  struct get_first {
    static constexpr typename Default::second_type value =
        (SecondKey == Key)
            ? FirstKey
            : constexpr_bimap<Default, Rest...>::template get_first<Key>::value;
  };
};

}  // namespace jcc::ds

#endif  // jcc_DataStructures_hpp
