// NightlyLambda.h - implements a compile time lambda calculus library,
//
// Copyright (c) 2020 Yuan Ruihong all rights reserved.

#pragma once
#ifndef YUAN_NIGHTLY_LAMBDA
#define YUAN_NIGHTLY_LAMBDA

#include "type_list.h"
#include <iostream>

namespace nightly_lambda {

// TYPE TAGS
class nightly_lambda_tag {};
class has_secondary_tag : nightly_lambda_tag {};
class application_tag : has_secondary_tag {};
class variable_tag : nightly_lambda_tag {};
class lambda_tag : has_secondary_tag {};
class subst_tag : has_secondary_tag {};

// STRUCT TEMPLATE is_nightly_lambda
template <class _Ty, class = void>
struct is_nightly_lambda : std::false_type {};

template <class _Ty>
struct is_nightly_lambda<_Ty, std::void_t<typename _Ty::tag>>
    : std::bool_constant<std::is_base_of_v<nightly_lambda_tag, typename _Ty::tag>> {};

template <class _Ty>
constexpr bool is_nightly_lambda_v = is_nightly_lambda<_Ty>::value;

// STRUCT TEMPLATE has_secondary
template <class _Ty, class = void>
struct has_secondary : std::false_type {};

template <class _Ty>
struct has_secondary<_Ty, std::void_t<typename _Ty::tag>>
    : std::bool_constant<std::is_base_of_v<has_secondary_tag, typename _Ty::tag>> {};

template <class _Ty>
constexpr bool has_secondary_v = has_secondary<_Ty>::value;

// STRUCT TEMPLATE is_variable
template <class _Ty, class = void>
struct is_variable : std::false_type {};

template <class _Ty>
struct is_variable<_Ty, std::void_t<typename _Ty::tag>>
    : std::is_same<typename _Ty::tag, variable_tag> {};

template <class _Ty>
constexpr bool is_variable_v = is_variable<_Ty>::value;

// STRUCT TEMPLATE is_lambda
template <class _Ty, class = void>
struct is_lambda : std::false_type {};

template <class _Ty>
struct is_lambda<_Ty, std::void_t<typename _Ty::tag>>
    : std::is_same<typename _Ty::tag, lambda_tag> {};

template <class _Ty>
constexpr bool is_lambda_v = is_lambda<_Ty>::value;

// STRUCT TEMPLATE is_application
template <class _Ty, class = void>
struct is_application : std::false_type {};

template <class _Ty>
struct is_application<_Ty, std::void_t<typename _Ty::tag>>
    : std::is_same<typename _Ty::tag, application_tag> {};

template <class _Ty>
constexpr bool is_application_v = is_application<_Ty>::value;

// FUNCTION TEMPLATE symb_eq
template <class _Ty1, class _Ty2, std::enable_if_t<
    is_nightly_lambda_v<_Ty1> && is_nightly_lambda_v<_Ty2>, int> = 0>
constexpr bool symb_eq(const _Ty1&, const _Ty2&) noexcept
{
    return std::is_same_v<typename _Ty1::self, typename _Ty2::self>;
}

template <class _FuncTy, class _ArgTy>
class application_node;

// STRUCT TEMPLATE alpha_relation
template <class _ExprTy, class _OldName, class _NewName>
struct alpha_relation { // rename a formal parameter
    static constexpr bool has_effect = false;
    using type = typename _ExprTy::self;
};

template <class _ExprTy, class _OldName, class _NewName>
using alpha_relation_t = typename alpha_relation<
    typename _ExprTy::self, typename _OldName::self, typename _NewName::self>::type;

// STRUCT TEMPLATE beta_reduction
template <class _Self, class _ArgTy>
struct beta_reduction {
    static constexpr bool has_effect = false;
    using type = application_node<typename _Self::self, typename _ArgTy::self>;
};

template <class _VarTy, class _ArgTy>
using beta_reduction_t = typename beta_reduction<_VarTy, _ArgTy>::type;

template <class _VarTy, class _ExprTy>
class lambda_node;

// STRUCT TEMPLATE eta_reduction
template <class _VarTy, class _ExprTy>
struct eta_reduction {
    static constexpr bool has_effect = false;
    using type = lambda_node<typename _VarTy::self, typename _ExprTy::self>;
};

template <class _VarTy, class _ExprTy>
using eta_reduction_t = typename eta_reduction<typename _VarTy::self, typename _ExprTy::self>::type;

template <class _ExprTy>
struct _Full_reduction_impl {
    using type = typename _ExprTy::self;
};

template <class _VarTy, class _ExprTy>
struct _Full_reduction_impl<lambda_node<_VarTy, _ExprTy>> {
    using _EvExpr = typename _Full_reduction_impl<typename _ExprTy::self>::type;
    using type = eta_reduction_t<_VarTy, _EvExpr>;
};

template <class _FuncTy, class _ArgTy>
struct _Full_reduction_impl<application_node<_FuncTy, _ArgTy>> {
    using _EvFunc = typename _Full_reduction_impl<typename _FuncTy::self>::type;
    using _EvArg = typename _Full_reduction_impl<typename _ArgTy::self>::type;
    using type = beta_reduction_t<_EvFunc, _EvArg>;
};

// STRUCT TEMPLATE is_irreducible
template <class _ExprTy>
struct is_irreducible
    : std::bool_constant<std::is_same_v<typename _ExprTy::self
    , typename _Full_reduction_impl<typename _ExprTy::self>::type>>
{};

template <class _ExprTy>
constexpr bool is_irreducible_v = is_irreducible<_ExprTy>::value;

// STRUCT TEMPLATE full_reduction
template <class _ExprTy, class = void>
struct full_reduction {
    using _ReduceOnce = typename _Full_reduction_impl<typename _ExprTy::self>::type;
    using type = typename full_reduction<_ReduceOnce>::type;
};

template <class _ExprTy>
struct full_reduction<_ExprTy, std::enable_if_t<is_irreducible_v<_ExprTy>>> {
    using type = typename _ExprTy::self;
};

template <class _ExprTy>
using full_reduction_t = typename full_reduction<_ExprTy>::type;

// STRUCT TEMPLATE substitution_result
template <class _Self, class _SubstTy>
struct substitution_result {
    static_assert(is_variable_v<_Self>, "invalid _Self");

    // x[x <= a] -> a
    // x[y <= a] -> x
    using type = std::conditional_t<std::is_same_v<typename _Self::self, typename _SubstTy::prim::self>
        , typename _SubstTy::sec::self, typename _Self::self>;
};

template <class _ExprTy, class _SubstTy>
using substitution_result_t = typename 
    substitution_result<typename _ExprTy::self, typename _SubstTy::self>::type;

template <class _ExprTy, class _NameList>
struct _Unshadow {
    static constexpr bool has_effect = false;
    using type = typename _ExprTy::self;
    using remaining_names = _NameList;
};

// CLASS TEMPLATE _Node_base
template <class _Self>
class _Node_base {
public:
    // static_assert(std::is_base_of_v<_Node_base<_Self>, _Self>, "invalid _Self");

    using self = _Self; // use `self` whenever you can to resolve cv differences

    template <class _Right>
    constexpr bool symb_eq(const _Right&) const noexcept
    {
        return std::is_same_v<self, typename _Right::self>;
    }

    template <class _ArgTy, std::enable_if_t<is_nightly_lambda_v<_ArgTy>, int> = 0>
    constexpr auto operator()(const _ArgTy&) const noexcept
    {
        return full_reduction_t<application_node<self, typename _ArgTy::self>>{};
    }

    template <class _SubstTy, std::enable_if_t<
        std::is_same_v<typename _SubstTy::tag, subst_tag>, int> = 0>
    constexpr auto operator[](const _SubstTy& _Subst) const noexcept
    {
        return substitution_result_t<self, typename _SubstTy::self>{};
    }

    template <class _OldName, class _NewName, std::enable_if_t<
        is_variable_v<_OldName> && is_variable_v<_NewName>, int> = 0>
    constexpr auto rename(const _OldName&, const _NewName&) const noexcept
    {
        return alpha_relation_t<self, _OldName, _NewName>{};
    }

    template <class... _NewNames>
    constexpr auto unshadow(const _NewNames&...) const noexcept
    {
        static_assert(std::conjunction_v<is_variable<_NewNames>...>, "invalid args");
        return typename _Unshadow<self, std_ext::type_list<_NewNames...>>::type{};
    }
};

// CLASS TEMPLATE: variable_node
template <size_t _N>
class variable_node : public _Node_base<variable_node<_N>> {
public:
    using tag = variable_tag;
    // using prim = void;
    // using sec = void;
    static constexpr size_t number = _N;

    static constexpr std::enable_if_t<(number < 26), char> alpha
        = 'a' + static_cast<char>(number);

    using _Char_seq_rep = std_ext::char_sequence<alpha>;

    variable_node() = default;
    variable_node(const variable_node&) = delete;
    variable_node& operator=(const variable_node&) = delete;
};

namespace names {
constexpr variable_node<0> a;
constexpr variable_node<1> b;
constexpr variable_node<2> c;
constexpr variable_node<3> d;
constexpr variable_node<4> e;
constexpr variable_node<5> f;
constexpr variable_node<6> g;
constexpr variable_node<7> h;
constexpr variable_node<8> i;
constexpr variable_node<9> j;
constexpr variable_node<10> k;
constexpr variable_node<11> l;
constexpr variable_node<12> m;
constexpr variable_node<13> n;
constexpr variable_node<14> o;
constexpr variable_node<15> p;
constexpr variable_node<16> q;
constexpr variable_node<17> r;
constexpr variable_node<18> s;
constexpr variable_node<19> t;
constexpr variable_node<20> u;
constexpr variable_node<21> v;
constexpr variable_node<22> w;
constexpr variable_node<23> x;
constexpr variable_node<24> y;
constexpr variable_node<25> z;
}

// CLASS TEMPLATE shadowed
template <class _VarTy>
class shadowed : public _VarTy {
public:
    using self = shadowed<_VarTy>; // shadow
    using original = _VarTy;       // (possibly) shadow
    using _Char_seq_rep = std_ext::concat_integer_sequences<char, // shadow
        typename original::_Char_seq_rep, std_ext::char_sequence<'\''>>;
};

// STRUCT TEMPLATE is_shadowed
template <class _Ty>
struct is_shadowed : std::false_type {};

template <class _VarTy>
struct is_shadowed<shadowed<_VarTy>> : std::true_type {};

template <class _Ty>
constexpr bool is_shadowed_v = is_shadowed<_Ty>::value;

// IMPL _Unshadow FOR lambda
template <class _VarTy, class _ExprTy, class _First, class... _Rest>
struct _Unshadow<lambda_node<shadowed<_VarTy>, _ExprTy>
    , std_ext::type_list<_First, _Rest...>>
{
    static constexpr bool has_effect = true;
    using type = alpha_relation_t<_ExprTy, shadowed<_VarTy>, _First>;
    using remaining_names = _Unshadow<type, std_ext::type_list<_Rest...>>;
};

// IMPL _Unshadow FOR application
template <class _FuncTy, class _ArgTy, class _First, class... _Rest>
struct _Unshadow<application_node<_FuncTy, _ArgTy>, std_ext::type_list<_First, _Rest...>>
{
    using _UnshadowFunc = _Unshadow<typename _FuncTy::type, std_ext::type_list<_First, _Rest...>>;
    using _FuncResult = typename _UnshadowFunc::type;
    using _FuncRem = typename _UnshadowFunc::remaining_names;
    using _UnshadowArg = _Unshadow<typename _ArgTy::self, _FuncRem>;
    using _ArgResult = typename _UnshadowArg::type;
    
    using type = application_node<_FuncResult, _ArgResult>;
    using remaining_names = typename _UnshadowArg::remaining_names;
    static constexpr bool has_effect = _UnshadowFunc::has_effect || _UnshadowArg::has_effect;
};

template <template <class _Ty1, class _Ty2> class _Self, class _First, class _Second>
class _Binary_node_base : public _Node_base<_Self<_First, _Second>> {
public:
    template <class _Other1, class _Other2>
    using rebind = _Self<_Other1, _Other2>;

    using prim = _First;
    using sec = _Second;
};

// CLASS TEMPLATE application_node
template <class _FuncTy, class _ArgTy>
class application_node : public _Binary_node_base<application_node, _FuncTy, _ArgTy> {
public:
    using tag = application_tag;

    using _Char_seq_rep = std_ext::concat_integer_sequences<char
        , std_ext::char_sequence<'('>
        , typename _FuncTy::_Char_seq_rep
        , std_ext::char_sequence<' '>
        , typename _ArgTy::_Char_seq_rep
        , std_ext::char_sequence<')'>>;
};

// CLASS TEMPLATE subst
template <class _VarTy, class _ExprTy>
class subst : public _Binary_node_base<subst, _VarTy, _ExprTy> {
public:
    using tag = subst_tag;
};

// CLASS TEMPLATE lambda_node
template <class _VarTy, class _ExprTy>
class lambda_node : public _Binary_node_base<lambda_node, _VarTy, _ExprTy> {
public:
    using tag = lambda_tag;

    using _Char_seq_rep = std_ext::concat_integer_sequences<char
        , std_ext::char_sequence<'[','l','a','m','b','d','a',' '>
        , typename _VarTy::_Char_seq_rep
        , std_ext::char_sequence<'.', ' '>
        , typename _ExprTy::_Char_seq_rep
        , std_ext::char_sequence<']'>>;
};

// FUNCTION TEMPLATE operator<= (make substitution)
template <class _VarTy, class _ExprTy, std::enable_if_t<
    is_nightly_lambda_v<_ExprTy>, int> = 0>
inline constexpr auto operator<=(const _VarTy&, const _ExprTy&) noexcept
{
    return subst<_VarTy, _ExprTy>{};
}

// STRUCT TEMPLATE free_variables
template <class _ExprTy>
struct free_variables {
    static_assert(is_variable_v<_ExprTy>, "invalid _ExprTy");
    using type = std_ext::type_list<typename _ExprTy::self>;
};

// IMPL free_variables FOR lambda
template <class _VarTy, class _ExprTy>
struct free_variables<lambda_node<_VarTy, _ExprTy>> {
    using type = typename free_variables<typename _ExprTy::self>::type
        ::template remove_completely<typename _VarTy::self>;
};

// IMPL free_variables FOR application
template <class _FuncTy, class _ArgTy>
struct free_variables<application_node<_FuncTy, _ArgTy>> {
    using _FuncFV = typename free_variables<typename _FuncTy::self>::type;
    using _ArgFV = typename free_variables<typename _ArgTy::self>::type;
    using type = typename _FuncFV::template concat<_ArgFV>
        ::template remove_duplicates;
};

template <class _ExprTy>
using free_variables_t = typename free_variables<typename _ExprTy::self>::type;

// IMPL substitution_result FOR lambda
template <class _VarTy, class _ExprTy, class _SubstTy>
struct substitution_result<lambda_node<_VarTy, _ExprTy>, _SubstTy>
{ // lambda(x, a)[x <= b] -> lambda(x, a);
  // lambda(y, a)[x <= b{}] -> lambda(y, a[x <= b])
  // lambda(y, a)[x <= b{y}] -> lambda(y', a[y <= y'][x <= b]) // protect inner y
    using _SubstFreeVars = free_variables_t<typename _SubstTy::sec>;
    using _Param = typename _VarTy::self;
    using _ShadowParamIfNeeded = std::conditional_t<_SubstFreeVars::template contains<_Param>
        , shadowed<_Param>, _Param>;

    using type = std::conditional_t<std::is_same_v<
        typename _VarTy::self, typename _SubstTy::prim::self>
            , typename lambda_node<typename _VarTy::self, typename _ExprTy::self>
            , lambda_node<_ShadowParamIfNeeded, substitution_result_t<
                typename _ExprTy::self, _SubstTy>>>;
};

// IMPL substitution_result FOR application
template <class _FuncTy, class _ArgTy, class _SubstTy>
struct substitution_result<application_node<_FuncTy, _ArgTy>, _SubstTy>
{ // f(a)[s] -> f[a](y[s])
    using type = application_node<substitution_result_t<typename _FuncTy::self, _SubstTy>,
        substitution_result_t<typename _ArgTy::self, _SubstTy>>;
};

// IMPL alpha_relation FOR lambda
template <class _VarTy, class _ExprTy, class _OldName, class _NewName>
struct alpha_relation<lambda_node<_VarTy, _ExprTy>, _OldName, _NewName> {
    using _Reduce_body = alpha_relation<typename _ExprTy::self, _OldName, _NewName>;

    using type = std::conditional_t<
          std::is_same_v<typename _OldName::self, typename _VarTy::self>
        , lambda_node<typename _NewName::self // lambda(x, a) -> lambda(y, a[x <= y])
            , substitution_result_t<typename _ExprTy::self
                , subst<typename _OldName::self, typename _NewName::self>>>
        , lambda_node<typename _VarTy::self, typename _Reduce_body::type>>;

    static constexpr bool has_effect = std::is_same_v<typename _OldName::self, typename _VarTy::self>
        || _Reduce_body::has_effect;
};

// IMPL alpha_relation FOR application
template <class _FuncTy, class _ArgTy, class _OldName, class _NewName>
struct alpha_relation<application_node<_FuncTy, _ArgTy>, _OldName, _NewName> {
    using type1 = alpha_relation<typename _FuncTy::self
        , typename _OldName::self, typename _NewName::self>;
    using type2 = alpha_relation<typename _ArgTy::self
        , typename _OldName::self, typename _NewName::self>;

    using type = std::conditional_t<type1::has_effect // change at most one place
        , application_node<typename type1::type, typename _ArgTy::self>
        , std::conditional_t<type2::has_effect
            , application_node<typename _FuncTy::self, typename type2::type>
            , application_node<typename _FuncTy::self, typename _ArgTy::self>>>;
    static constexpr bool has_effect = type1::has_effect || type2::has_effect;
};

// IMPL beta_reduction FOR lambda
template <class _VarTy, class _ExprTy, class _ArgTy>
struct beta_reduction<lambda_node<_VarTy, _ExprTy>, _ArgTy>
{ // lambda(x, a)(b) -> a[x <= b]
    static constexpr bool has_effect = true;
    using type = substitution_result_t<typename _ExprTy::self
        , subst<typename _VarTy::self, typename _ArgTy::self>>;
};

// IMPL eta_reduction FOR application
template <class _VarTy, class _FuncTy, class _ArgTy>
struct eta_reduction<_VarTy, application_node<_FuncTy, _ArgTy>>
{ // lambda(x, f(x)) -> f
  // lambda(x, f{x}(x)) -> lambda(x, f(x))
    using _FuncFreeVars = free_variables_t<typename _FuncTy::self>;

    static constexpr bool has_effect = std::is_same_v<
        typename _VarTy::self, typename _ArgTy::self>
        && !_FuncFreeVars::template contains<typename _VarTy::self>;

    using type = std::conditional_t<has_effect, typename _FuncTy
        , lambda_node<typename _VarTy::self
            , typename application_node<typename _FuncTy::self, typename _ArgTy::self>>>;
};

// FUNCTION TEMPLATE lambda
template <class _VarTy, class _ExprTy, std::enable_if_t<
    is_nightly_lambda_v<_ExprTy>, int> = 0>
inline constexpr auto lambda(const _VarTy&, const _ExprTy&) noexcept
{
    return eta_reduction_t<_VarTy, _ExprTy>{};
}

template <class _VarTy, class... _Args>
constexpr auto lambda(const _VarTy& _FirstVar, const _Args&... _ConsecArgs) noexcept
{
    return lambda(_FirstVar, lambda(_ConsecArgs...));
}

// FUNCTION TEMPLATE evaluate
template <class _ExprTy, std::enable_if_t<is_nightly_lambda_v<_ExprTy>, int> = 0>
inline constexpr auto evaluate(const _ExprTy&) noexcept
{ // Normally all expressions are instantly reduced. But should one construct an
  // expression type by hand, bypassing the functions, it would be more convenient
  // to call the function `evaluate`.

    return full_reduction_t<_ExprTy>{};
}

// FUNCTION TEMPLATE full_simplify
template <class _ExprTy, class... _NewNames>
inline constexpr auto full_simplify(const _ExprTy& _Expr, const _NewNames&... _Names) noexcept
{
    return evaluate(_Expr).unshadow(_Names...);
}

// FUNCTION TEMPLATE operator<<
template <class _ExprTy, std::enable_if_t<is_nightly_lambda_v<_ExprTy>, int> = 0>
std::ostream& operator<<(std::ostream& _Lhs, const _ExprTy&)
{
    return (_Lhs << std_ext::to_string_constant<typename _ExprTy::_Char_seq_rep>::value);
}

} // namespace nightly_lambda

#endif // header guard
