// type_list.h - defines facilities for a list of types
//
// Copyright (c) 2020 Yuan Ruihong all rights reserved.

#pragma once
#ifndef YUAN_TYPE_LIST_H
#define YUAN_TYPE_LIST_H

// A TAKER is the `root` name of a class template. Say, the TAKER of `std::tuple<bool, int>`
// is `std::tuple`. When using the word `taker`, we typically refer to a template taking a
// parameter pack, a feature introduced in C++11 that allows the template to take arbitrary
// arguments in a type-safe manner. However, this feature has proven to be very difficult to
// use, with numerous limitations. A flexible design that employs parameter packs requires
// forwarding of packs between different takers, concatenation, subscript designation, finding,
// and even insertion and deletion of parameters. Unfortunately, all these utility requirements
// are poorly supported by the standard library and are what this header file aims to address. 
// In this file, I propose a `type_list` type that serves as the general container of types,
// which fully supports concatenation, subscripting, finding, and insertion/deletion. One can
// conveniently convert between different taker types using the `type_forward` template. To
// facilitate related usage, the standard library `integer_sequence` and `index_sequence` are
// also augmented: `make_integer_range`, `concat_integer_sequences`, `invert_integer_sequence`
// and their index counterparts are implemented. The distributed file is standalone and assumes
// dependencies only on C++14 and standard library <tuple>. All implementations are arranged
// inside namespace `std_ext` and are written in a style akin to the standard library.

#include <tuple>

namespace std_ext {

template <class... _Args>
struct type_list;

template <class _Ty, class _Input, template<class _Uty, _Uty...> class _OutputTaker>
struct _Value_sequence_forward { // forwards a value parameter pack between different takers
                                 // This template is considered too specific to be public.
};

template <class _Ty, template<class _Uty, _Uty...> class _InputTaker,
    template<class _Vty, _Vty...> class _OutputTaker, _Ty... _Args>
struct _Value_sequence_forward<_Ty, _InputTaker<_Ty, _Args...>, _OutputTaker> {
    using type = _OutputTaker<_Ty, _Args...>;

    template <_Ty... _Additionals>
    struct place_after { // for concatenating value sequences
        using type = _OutputTaker<_Ty, _Additionals..., _Args...>;
    };

    template <_Ty... _Additionals>
    struct place_before {
        using type = _OutputTaker<_Ty, _Args..., _Additionals...>;
    };
};

// STRUCT TEMPLATE to_array_constant
template <class _Ty, class _TyList>
struct to_array_constant {};

template <class _Ty, _Ty... _Items>
struct to_array_constant<_Ty, std::integer_sequence<_Ty, _Items...>> {
    static constexpr _Ty value[] = { _Items... };
};

// ALIAS TEMPLATE char_sequence
template <char... _Chars>
using char_sequence = std::integer_sequence<char, _Chars...>;

// STRUCT TEMPLATE to_string_constant
template <class _CharSeq>
struct to_string_constant {};

template <char... _Chars>
struct to_string_constant<char_sequence<_Chars...>> {
    static constexpr char value[] = { _Chars..., 0 }; // trailing 0
};

template <class _Int, _Int _StartIndex, _Int...>
struct _Make_integer_range_impl { // empty or out-of-range _Indices
    using type = std::integer_sequence<_Int>;
};

template <class _Int, _Int _StartIndex, _Int... _Rest>
struct _Make_integer_range_impl<_Int, _StartIndex, _StartIndex, _Rest...> { // found
    using type = std::integer_sequence<_Int, _StartIndex, _Rest...>;
};

template <class _Int, _Int _StartIndex, _Int _First, _Int... _Rest>
struct _Make_integer_range_impl<_Int, _StartIndex, _First, _Rest...>
    : _Make_integer_range_impl<_Int, _StartIndex, _Rest...> {};

template <class _Int, _Int _StartIndex, class _InputSeq>
struct _Make_integer_range {};

template <class _Int, _Int _StartIndex, _Int... _Args>
struct _Make_integer_range<_Int, _StartIndex
    , std::integer_sequence<_Int, _Args...>> { // extracts integers from `integer_sequence`
    using type = typename _Make_integer_range_impl<_Int, _StartIndex, _Args...>::type;
};

// ALIAS TEMPLATE make_integer_range
template <class _Int, _Int _StartIndex, _Int _EndIndex> // makes [_StartIndex, _EndIndex) range
                                                        // gives empty sequence if _StartIndex >= _EndIndex
using make_integer_range = typename _Make_integer_range<_Int, _StartIndex
    , std::make_integer_sequence<_Int, _EndIndex>>::type;

template <size_t _StartIndex, size_t _EndIndex>
using make_index_range = make_integer_range<size_t, _StartIndex, _EndIndex>;

template <class _Int, class _IntSeq1, class... _IntSequences>
struct _Concat_integer_sequences {
    using type = typename _Concat_integer_sequences<_Int, _IntSeq1,
        typename _Concat_integer_sequences<_Int, _IntSequences...>::type>::type;
};

template <class _Int, class _IntSeq1, template <class _Uty, _Uty...> class _IntSeq2, _Int... _Args>
struct _Concat_integer_sequences<_Int, _IntSeq1, _IntSeq2<_Int, _Args...>> {
    using type = typename _Value_sequence_forward<_Int, _IntSeq1, _IntSeq2>
        ::template place_before<_Args...>::type;
};

// ALIAS TEMPLATE concat_integer_sequences
template <class _Int, class _IntSeq1, class... _IntSequences>
using concat_integer_sequences = typename 
    _Concat_integer_sequences<_Int, _IntSeq1, _IntSequences...>::type;

template <class _IndexSeq1, class... _IndexSequences>
using concat_index_sequences = concat_integer_sequences<size_t, _IndexSeq1, _IndexSequences...>;

template <class _ResSeq, class _Int, _Int... _Integers>
struct _Invert_integer_sequence_impl {
    using type = _ResSeq;
};

template <class _ResSeq, class _Int, _Int _First, _Int _Second, _Int... _Rest>
struct _Invert_integer_sequence_impl<_ResSeq, _Int, _First, _Second, _Rest...>
    : _Invert_integer_sequence_impl<std::conditional_t<_First + 1 == _Second, _ResSeq // omit
        , concat_integer_sequences<_Int, _ResSeq, make_integer_range<_Int, _First + 1, _Second>>>
    , _Int, _Second, _Rest...> {};

template <class _Int, class _IntSeq>
struct _Invert_integer_sequence {};

template <class _Int, _Int _First, _Int... _Integers>
struct _Invert_integer_sequence<_Int, std::integer_sequence<_Int, _First, _Integers...>> {
    using type = typename _Invert_integer_sequence_impl<
        std::conditional_t<_First == 0, std::integer_sequence<_Int>, std::make_integer_sequence<_Int, _First>>
        , _Int, _First, _Integers...>::type;
};

// ALIAS TEMPLATE invert_integer_sequence
template <class _Int, class _IntSeq> // [3] -> [0, 1, 2]; [0, 2, 3, 5] -> [1, 4]
                                     // assuming ascending order
using invert_integer_sequence = typename _Invert_integer_sequence<_Int, _IntSeq>::type;

template <class _IndexSeq>
using invert_index_sequence = invert_integer_sequence<size_t, _IndexSeq>;

template <class _ResSeq, class _Int, template <_Int> class _Pred, _Int... _Rest>
struct _Choose_integer_sequence_impl {
    using type = _ResSeq;
};

template <class _ResSeq, class _Int, template <_Int> class _Pred, _Int _First, _Int... _Rest>
struct _Choose_integer_sequence_impl<_ResSeq, _Int, _Pred, _First, _Rest...>
    : _Choose_integer_sequence_impl<std::conditional_t<_Pred<_First>::value
        , typename _Value_sequence_forward<_Int, _ResSeq, std::integer_sequence>
            ::template place_before<_First>::type
        , _ResSeq>, _Int, _Pred, _Rest...>
{
};

template <class _Int, class _IndexSeq, template <_Int> class _Pred>
struct _Choose_integer_sequence {};

template <class _Int, template <_Int> class _Pred, _Int... _Integers>
struct _Choose_integer_sequence<_Int, std::integer_sequence<_Int, _Integers...>, _Pred> {
    using type = typename _Choose_integer_sequence_impl<
        std::integer_sequence<_Int>, _Int, _Pred, _Integers...>::type;
};

// ALIAS TEMPLATE choose_integer_sequence
template <class _Int, class _IndexSeq, template <_Int> class _Pred>
using choose_integer_sequence = typename
    _Choose_integer_sequence<_Int, _IndexSeq, _Pred>::type;

template <class _IndexSeq, template <size_t> class _Pred>
using choose_index_sequence = choose_integer_sequence<size_t, _IndexSeq, _Pred>;

// STRUCT TEMPLATE type_forward
template <class _Input, template<class...> class _OutputTaker>
struct type_forward { // analogous to _Value_sequence_forward, but forwards
                      // type parameter packs and is public
};

template <template<class...> class _InputTaker,
    template<class...> class _OutputTaker, class... _Args>
struct type_forward<_InputTaker<_Args...>, _OutputTaker> {
    using type = _OutputTaker<_Args...>;

    template <class... _Additionals>
    struct place_after {
        using type = _OutputTaker<_Additionals..., _Args...>;
    };

    template <class... _Additionals>
    struct place_before {
        using type = _OutputTaker<_Args..., _Additionals...>;
    };
};

// ALIAS TEMPLATE type_forward_t
template <class _Input, template<class...> class _OutputTaker>
using type_forward_t = typename type_forward<_Input, _OutputTaker>::type;

// ALIAS TEMPLATE type_forward_after_t
template <class _Input, template <class...> class _OutputTaker, class... _Additionals>
using type_forward_after_t = typename
    type_forward<_Input, _OutputTaker>::template place_after<_Additionals...>::type;

// ALIAS TEMPLATE type_forward_before_t
template <class _Input, template <class...> class _OutputTaker, class... _Additionals>
using type_forward_before_t = typename
    type_forward<_Input, _OutputTaker>::template place_before<_Additionals...>::type;

// STRUCT TEMPLATE to_type_list
template <class _Taker>
struct to_type_list {
    using type = type_forward_t<_Taker, type_list>;
};

template <class _Taker>
using to_type_list_t = typename to_type_list<_Taker>::type;

template <class _Item, size_t _K, class... _Types>
struct _Find_type_impl { // find the location of type `_Item` in a parameter pack
    static constexpr bool has_found = false;
};

template <class _Item, size_t _K, class _First, class... _Rest>
struct _Find_type_impl<_Item, _K, _First, _Rest...>
    : _Find_type_impl<_Item, _K + 1, _Rest...> {};

template <class _Item, size_t _K, class... _Rest>
struct _Find_type_impl<_Item, _K, _Item, _Rest...> {
    static constexpr bool has_found = true;
    static constexpr size_t index = _K;
};

template <class _Item, class... _Args>
using _Find_type = _Find_type_impl<_Item, 0, _Args...>;

template <class _TypeList, class _IndexSeq>
struct _Extract_and_choose {};

template <class _TypeList, size_t... _Indices>
struct _Extract_and_choose<_TypeList, std::index_sequence<_Indices...>> {
    using type = typename _TypeList::template choose<_Indices...>;
};

template <class _TypeList, size_t... _Indices>
struct _Remove_at {
    using type = typename _Extract_and_choose<_TypeList
        , invert_index_sequence<std::index_sequence<_Indices..., _TypeList::size>>>::type;
};

template <class _TypeList>
struct _Remove_at<_TypeList> {
    using type = _TypeList;
};

template <class _Item, class _TypeList, bool _Contains>
struct _Remove {
    using type = _TypeList;
};

template <class _Item, class _TypeList>
struct _Remove<_Item, _TypeList, true> {
    using type = typename _TypeList::template remove_at<_TypeList::template index_of<_Item>>;
};

template <class _Item, class _TypeList>
struct _Remove_completely {
    template <size_t _Index>
    struct _Is_not_item {
        static constexpr bool value = !std::is_same_v<_Item, typename
            _TypeList::template get<_Index>>;
    };

    using type = typename _Extract_and_choose<_TypeList
        , choose_index_sequence<std::make_index_sequence<_TypeList::size>
            , _Is_not_item>>::type;
};

template <class...>
struct _Remove_duplicates { // empty case
    using type = type_list<>;
};

template <class _First, class... _Rest>
struct _Remove_duplicates<_First, _Rest...> {
    template <class... _Types>
    using rebind = _Remove_duplicates<_Types...>;

    using _ClearFirst = typename type_list<_Rest...>::template remove_completely<_First>;
    using _ClearRest = typename type_forward_t<_ClearFirst, rebind>::type;
    using type = type_forward_after_t<_ClearRest, type_list, _First>;
};

// STRUCT TEMPLATE type_list
template <class... _Args>
struct type_list
{ // a type alias ending with `_types` has taker `type_list`
    using tuple_type = std::tuple<_Args...>;

    static constexpr size_t size = sizeof...(_Args);

    template <size_t _Index>
    using get = std::tuple_element_t<_Index, tuple_type>;

    template <size_t... _Indices>
    using choose = type_list<std::tuple_element_t<_Indices, tuple_type>...>;

    using first_type = get<0>;

    using last_type = get<size - 1>;

    template <class... _Types>
    using append = type_forward_before_t<type_list<_Args...>, type_list, _Types...>;

    template <class _Other>
    using concat = type_forward_after_t<_Other, type_list, _Args...>;

    template <class _Item>
    static constexpr size_t index_of = _Find_type<_Item, _Args...>::index;

    template <class _Item>
    static constexpr bool contains = _Find_type<_Item, _Args...>::has_found;

    template <size_t... _Indices>
    using remove_at = typename _Remove_at<type_list<_Args...>, _Indices...>::type;

    template <class _Item>
    using remove = typename _Remove<_Item, type_list<_Args...>, contains<_Item>>::type;

    template <class _Item>
    using remove_completely = typename
        _Remove_completely<_Item, type_list<_Args...>>::type;

    using remove_duplicates = typename _Remove_duplicates<_Args...>::type;
};

template <>
struct type_list<> {
    using tuple_type = std::tuple<>;

    static constexpr size_t size = 0;

    template <size_t... _Indices>
    using choose = type_list<>;

    template <class... _Types>
    using append = type_list<_Types...>;

    template <class _Other>
    using concat = _Other;

    template <class _Item>
    static constexpr bool contains = false;

    template <class _Item>
    using remove = type_list<>;

    template <class _Item>
    using remove_completely = type_list<>;

    using remove_duplicates = type_list<>;
};

} // namespace std_ext

#endif // header guard
