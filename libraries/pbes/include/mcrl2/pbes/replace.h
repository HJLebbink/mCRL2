// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/replace.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_REPLACE_H
#define MCRL2_PBES_REPLACE_H

#include "mcrl2/data/replace.h"
#include "mcrl2/pbes/add_binding.h"
#include "mcrl2/pbes/builder.h"
#include "mcrl2/pbes/find.h"

namespace mcrl2
{

namespace pbes_system
{

namespace detail {

/// \cond INTERNAL_DOCS
template <template <class> class Builder, class Derived, class Substitution>
struct add_capture_avoiding_replacement: public data::detail::add_capture_avoiding_replacement<Builder, Derived, Substitution>
{
  typedef data::detail::add_capture_avoiding_replacement<Builder, Derived, Substitution> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;
  using super::update_sigma;

  pbes_expression apply(const forall& x)
  {
    data::variable_list v = update_sigma.push(x.variables());
    pbes_expression result = forall(v, apply(x.body()));
    update_sigma.pop(v);
    return result;
  }

  pbes_expression apply(const exists& x)
  {
    data::variable_list v = update_sigma.push(x.variables());
    pbes_expression result = exists(v, apply(x.body()));
    update_sigma.pop(v);
    return result;
  }

  void update(pbes_equation& x)
  {
    data::variable_list v = update_sigma.push(x.variable().parameters());
    x.variable() = propositional_variable(x.variable().name(), v);
    x.formula() = apply(x.formula());
    update_sigma.pop(v);
  }

  void update(pbes& x)
  {
    std::set<data::variable> v = update_sigma(x.global_variables());
    x.global_variables() = v;
    update(x.equations());
    update_sigma.pop(v);
  }

  add_capture_avoiding_replacement(Substitution& sigma, std::multiset<data::variable>& V)
    : super(sigma, V)
  { }
};

template <template <class> class Builder, class Substitution>
struct substitute_pbes_expressions_builder: public Builder<substitute_pbes_expressions_builder<Builder, Substitution> >
{
  typedef Builder<substitute_pbes_expressions_builder<Builder, Substitution> > super;
  using super::apply;

  Substitution sigma;
  bool innermost;

  substitute_pbes_expressions_builder(Substitution sigma_, bool innermost_)
    : sigma(sigma_),
      innermost(innermost_)
  {}

  pbes_expression apply(const pbes_expression& x)
  {
    if (innermost)
    {
      pbes_expression y = super::apply(x);
      return sigma(y);
    }
    return sigma(x);
  }
};

template <template <class> class Builder, class Substitution>
substitute_pbes_expressions_builder<Builder, Substitution>
make_replace_pbes_expressions_builder(Substitution sigma, bool innermost)
{
  return substitute_pbes_expressions_builder<Builder, Substitution>(sigma, innermost);
}

template <template <class> class Builder, class Substitution>
struct replace_propositional_variables_builder: public Builder<replace_propositional_variables_builder<Builder, Substitution> >
{
  typedef Builder<replace_propositional_variables_builder<Builder, Substitution> > super;
  using super::apply;

  const Substitution& sigma;

  explicit replace_propositional_variables_builder(const Substitution& sigma_)
    : sigma(sigma_)
  {}

  pbes_expression apply(const propositional_variable_instantiation& x)
  {
    return sigma(x);
  }
};

template <template <class> class Builder, class Substitution>
replace_propositional_variables_builder<Builder, Substitution>
make_replace_propositional_variables_builder(const Substitution& sigma)
{
  return replace_propositional_variables_builder<Builder, Substitution>(sigma);
}

// Below the code for the capture avoiding replacement with an identifier generator is provided.

template <template <class> class Builder, class Derived, class Substitution, class IdentifierGenerator>
struct add_capture_avoiding_replacement_with_an_identifier_generator: public data::detail::add_capture_avoiding_replacement_with_an_identifier_generator<Builder, Derived, Substitution, IdentifierGenerator>
{
  typedef data::detail::add_capture_avoiding_replacement_with_an_identifier_generator<Builder, Derived, Substitution, IdentifierGenerator> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;
  using super::update_sigma;

  pbes_expression apply(const forall& x)
  {
    data::variable_list v = update_sigma.push(x.variables());
    pbes_expression result = forall(v, apply(x.body()));
    update_sigma.pop(v);
    return result;
  }

  pbes_expression apply(const exists& x)
  {
    data::variable_list v = update_sigma.push(x.variables());
    pbes_expression result = exists(v, apply(x.body()));
    update_sigma.pop(v);
    return result;
  }

  void update(pbes_equation& x)
  {
    data::variable_list v = update_sigma.push(x.variable().parameters());
    x.variable() = propositional_variable(x.variable().name(), v);
    x.formula() = apply(x.formula());
    update_sigma.pop(v);
  }

  void update(pbes& x)
  {
    std::set<data::variable> v = update_sigma(x.global_variables());
    x.global_variables() = v;
    update(x.equations());
    update_sigma.pop(v);
  }

  add_capture_avoiding_replacement_with_an_identifier_generator(Substitution& sigma, IdentifierGenerator& id_generator)
    : super(sigma, id_generator)
  { }
};


/// \endcond

} // namespace detail

//--- start generated pbes_system replace code ---//
template <typename T, typename Substitution>
void replace_sort_expressions(T& x,
                              const Substitution& sigma,
                              bool innermost,
                              typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                             )
{
  data::detail::make_replace_sort_expressions_builder<pbes_system::sort_expression_builder>(sigma, innermost).update(x);
}

template <typename T, typename Substitution>
T replace_sort_expressions(const T& x,
                           const Substitution& sigma,
                           bool innermost,
                           typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                          )
{
  return data::detail::make_replace_sort_expressions_builder<pbes_system::sort_expression_builder>(sigma, innermost).apply(x);
}

template <typename T, typename Substitution>
void replace_data_expressions(T& x,
                              const Substitution& sigma,
                              bool innermost,
                              typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                             )
{
  data::detail::make_replace_data_expressions_builder<pbes_system::data_expression_builder>(sigma, innermost).update(x);
}

template <typename T, typename Substitution>
T replace_data_expressions(const T& x,
                           const Substitution& sigma,
                           bool innermost,
                           typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                          )
{
  return data::detail::make_replace_data_expressions_builder<pbes_system::data_expression_builder>(sigma, innermost).apply(x);
}

template <typename T, typename Substitution>
void replace_variables(T& x,
                       const Substitution& sigma,
                       typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                      )
{
  core::make_update_apply_builder<pbes_system::data_expression_builder>(sigma).update(x);
}

template <typename T, typename Substitution>
T replace_variables(const T& x,
                    const Substitution& sigma,
                    typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                   )
{
  return core::make_update_apply_builder<pbes_system::data_expression_builder>(sigma).apply(x);
}

template <typename T, typename Substitution>
void replace_all_variables(T& x,
                           const Substitution& sigma,
                           typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                          )
{
  core::make_update_apply_builder<pbes_system::variable_builder>(sigma).update(x);
}

template <typename T, typename Substitution>
T replace_all_variables(const T& x,
                        const Substitution& sigma,
                        typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                       )
{
  return core::make_update_apply_builder<pbes_system::variable_builder>(sigma).apply(x);
}

/// \brief Applies the substitution sigma to x.
/// \pre { The substitution sigma must have the property that FV(sigma(x)) is included in {x} for all variables x. }
template <typename T, typename Substitution>
void replace_free_variables(T& x,
                            const Substitution& sigma,
                            typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                           )
{
  assert(data::is_simple_substitution(sigma));
  data::detail::make_replace_free_variables_builder<pbes_system::data_expression_builder, pbes_system::add_data_variable_binding>(sigma).update(x);
}

/// \brief Applies the substitution sigma to x.
/// \pre { The substitution sigma must have the property that FV(sigma(x)) is included in {x} for all variables x. }
template <typename T, typename Substitution>
T replace_free_variables(const T& x,
                         const Substitution& sigma,
                         typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                        )
{
  assert(data::is_simple_substitution(sigma));
  return data::detail::make_replace_free_variables_builder<pbes_system::data_expression_builder, pbes_system::add_data_variable_binding>(sigma).apply(x);
}

/// \brief Applies the substitution sigma to x, where the elements of bound_variables are treated as bound variables.
/// \pre { The substitution sigma must have the property that FV(sigma(x)) is included in {x} for all variables x. }
template <typename T, typename Substitution, typename VariableContainer>
void replace_free_variables(T& x,
                            const Substitution& sigma,
                            const VariableContainer& bound_variables,
                            typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                           )
{
  assert(data::is_simple_substitution(sigma));
  data::detail::make_replace_free_variables_builder<pbes_system::data_expression_builder, pbes_system::add_data_variable_binding>(sigma).update(x, bound_variables);
}

/// \brief Applies the substitution sigma to x, where the elements of bound_variables are treated as bound variables.
/// \pre { The substitution sigma must have the property that FV(sigma(x)) is included in {x} for all variables x. }
template <typename T, typename Substitution, typename VariableContainer>
T replace_free_variables(const T& x,
                         const Substitution& sigma,
                         const VariableContainer& bound_variables,
                         typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                        )
{
  assert(data::is_simple_substitution(sigma));
  return data::detail::make_replace_free_variables_builder<pbes_system::data_expression_builder, pbes_system::add_data_variable_binding>(sigma).apply(x, bound_variables);
}
//--- end generated pbes_system replace code ---//

//--- start generated pbes_system replace_capture_avoiding code ---//
/// \brief Applies sigma as a capture avoiding substitution to x.
/// \param x The object to which the subsitution is applied.
/// \param sigma A mutable substitution.
/// \param sigma_variables a container of variables.
/// \pre { sigma_variables must contain the free variables appearing in the right hand side of sigma }.
template <typename T, typename Substitution, typename VariableContainer>
void replace_variables_capture_avoiding(T& x,
                       Substitution& sigma,
                       const VariableContainer& sigma_variables,
                       typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                      )
{
  std::multiset<data::variable> V;
  pbes_system::find_free_variables(x, std::inserter(V, V.end()));
  V.insert(sigma_variables.begin(), sigma_variables.end());
  data::detail::apply_replace_capture_avoiding_variables_builder<pbes_system::data_expression_builder, pbes_system::detail::add_capture_avoiding_replacement>(sigma, V).update(x);
}

/// \brief Applies sigma as a capture avoiding substitution to x.
/// \param x The object to which the substiution is applied.
/// \param sigma A mutable substitution.
/// \param sigma_variables a container of variables.
/// \pre { sigma_variables must contain the free variables appearing in the right hand side of sigma }.
template <typename T, typename Substitution, typename VariableContainer>
T replace_variables_capture_avoiding(const T& x,
                    Substitution& sigma,
                    const VariableContainer& sigma_variables,
                    typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                   )
{
  std::multiset<data::variable> V;
  pbes_system::find_free_variables(x, std::inserter(V, V.end()));
  V.insert(sigma_variables.begin(), sigma_variables.end());
  return data::detail::apply_replace_capture_avoiding_variables_builder<pbes_system::data_expression_builder, pbes_system::detail::add_capture_avoiding_replacement>(sigma, V).apply(x);
}
//--- end generated pbes_system replace_capture_avoiding code ---//

//--- start generated pbes_system replace_capture_avoiding_with_identifier_generator code ---//
/// \brief Applies sigma as a capture avoiding substitution to x using an identifier generator.
/// \details This substitution function is much faster than replace_variables_capture_avoiding, but
///          it requires an identifier generator that generates strings for fresh variables. These 
///          strings must be unique in the sense that they have not been used for other variables.
/// \param x The object to which the subsitution is applied.
/// \param sigma A mutable substitution of which it can efficiently be checked whether a variable occurs in its 
///              right hand side. The class maintain_variables_in_rhs is useful for this purpose. 
/// \param id_generator A generator that generates unique strings, not yet used as variable names.

template <typename T, typename Substitution, typename IdentifierGenerator>
void replace_variables_capture_avoiding_with_an_identifier_generator(T& x,
                       Substitution& sigma,
                       IdentifierGenerator& id_generator,
                       typename std::enable_if<!std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                      )
{
  data::detail::apply_replace_capture_avoiding_variables_builder_with_an_identifier_generator<pbes_system::data_expression_builder, pbes_system::detail::add_capture_avoiding_replacement_with_an_identifier_generator>(sigma, id_generator).update(x);
}

/// \brief Applies sigma as a capture avoiding substitution to x using an identifier generator..
/// \details This substitution function is much faster than replace_variables_capture_avoiding, but
///          it requires an identifier generator that generates strings for fresh variables. These 
///          strings must be unique in the sense that they have not been used for other variables.
/// \param x The object to which the substiution is applied.
/// \param sigma A mutable substitution of which it can efficiently be checked whether a variable occurs in its 
///              right hand side. The class maintain_variables_in_rhs is useful for this purpose. 
/// \param id_generator A generator that generates unique strings, not yet used as variable names.
/// \return The result is the term x to which sigma has been applied. 
template <typename T, typename Substitution, typename IdentifierGenerator>
T replace_variables_capture_avoiding_with_an_identifier_generator(const T& x,
                    Substitution& sigma,
                    IdentifierGenerator& id_generator,
                    typename std::enable_if<std::is_base_of<atermpp::aterm, T>::value>::type* = nullptr
                   )
{
  return data::detail::apply_replace_capture_avoiding_variables_builder_with_an_identifier_generator<pbes_system::data_expression_builder, pbes_system::detail::add_capture_avoiding_replacement_with_an_identifier_generator>(sigma, id_generator).apply(x);
}
//--- end generated pbes_system replace_capture_avoiding_with_identifier_generator code ---//

/// \brief Applies a propositional variable substitution.
template <typename T, typename Substitution>
void replace_propositional_variables(T& x,
                                     const Substitution& sigma,
                                     typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                                    )
{
  pbes_system::detail::make_replace_propositional_variables_builder<pbes_system::pbes_expression_builder>(sigma).update(x);
}

/// \brief Applies a propositional variable substitution.
template <typename T, typename Substitution>
T replace_propositional_variables(const T& x,
                                  const Substitution& sigma,
                                  typename std::enable_if<std::is_base_of< atermpp::aterm, T>::value>::type* = nullptr
                                 )
{
  return pbes_system::detail::make_replace_propositional_variables_builder<pbes_system::pbes_expression_builder>(sigma).apply(x);
}

template <typename T, typename Substitution>
void replace_pbes_expressions(T& x,
                              const Substitution& sigma,
                              bool innermost = true,
                              typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = 0
                             )
{
  pbes_system::detail::make_replace_pbes_expressions_builder<pbes_system::pbes_expression_builder>(sigma, innermost).update(x);
}

template <typename T, typename Substitution>
T replace_pbes_expressions(const T& x,
                           const Substitution& sigma,
                           bool innermost = true,
                           typename std::enable_if< std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                          )
{
  return pbes_system::detail::make_replace_pbes_expressions_builder<pbes_system::pbes_expression_builder>(sigma, innermost).apply(x);
}

} // namespace pbes_system

} // namespace mcrl2

#ifndef MCRL2_PBES_SUBSTITUTIONS_H
#include "mcrl2/pbes/substitutions.h"
#endif

#endif // MCRL2_PBES_REPLACE_H
