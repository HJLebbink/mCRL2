// Author(s): Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file next_state_generator.cpp

#include "mcrl2/lps/next_state_generator.h"

#include <algorithm>
#include <set>
#include "mcrl2/lps/detail/instantiate_global_variables.h"

using namespace mcrl2;
using namespace mcrl2::data;
using namespace mcrl2::lps;
using namespace mcrl2::lps::detail;

static specification do_instantiate_global_variables(specification spec)
{
  instantiate_global_variables(spec);
  return spec;
}

static float condition_selectivity(data_expression e, variable v)
{
  if (sort_bool::is_and_application(e))
  {
     return condition_selectivity(application(e).left(), v) + condition_selectivity(application(e).right(), v);
  }
  else if (sort_bool::is_or_application(e))
  {
    float sum = 0;
    size_t count = 0;
    std::list<data_expression> terms;
    terms.push_back(e);
    while (!terms.empty())
    {
      data_expression expression = terms.front();
      terms.pop_front();
      if (sort_bool::is_or_application(expression))
      {
        terms.push_back(application(expression).left());
        terms.push_back(application(expression).right());
      }
      else
      {
        sum += condition_selectivity(expression, v);
        count++;
      }
    }
    return sum / count;
  }
  else if (is_equal_to_application(e))
  {
    data_expression left = application(e).left();
    data_expression right = application(e).right();

    if (is_variable(left) && variable(left) == v)
    {
      return 1;
    }
    else if(is_variable(right) && variable(right) == v)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

struct parameter_score
{
  size_t parameter_id;
  float score;
};

static bool parameter_score_compare(parameter_score left, parameter_score right)
{
  return left.score > right.score;
}

next_state_generator::next_state_generator(
  const specification& spec,
  const data::rewriter &rewriter,
  bool use_enumeration_caching,
  bool use_summand_pruning)
  : m_specification(do_instantiate_global_variables(spec)),
    m_rewriter(rewriter),
    m_enumerator(m_specification.data(), m_rewriter),
    m_use_enumeration_caching(use_enumeration_caching),
    m_use_summand_pruning(use_summand_pruning)
{
  declare_constructors();

  m_process_parameters = data::variable_vector(m_specification.process().process_parameters().begin(), m_specification.process().process_parameters().end());
  m_state_function = atermpp::function_symbol("STATE", m_process_parameters.size());
  m_state_function.protect();
  m_false = m_rewriter.convert_to(data::sort_bool::false_());
  m_false.protect();

  for (action_summand_vector::iterator i = m_specification.process().action_summands().begin(); i != m_specification.process().action_summands().end(); i++)
  {
    summand_t summand;
    summand.variables = i->summation_variables();
    summand.condition = m_rewriter.convert_to(i->condition());
    summand.result_state = get_internal_state(i->next_state(m_specification.process().process_parameters()));

    for (action_list::iterator j = i->multi_action().actions().begin(); j != i->multi_action().actions().end(); j++)
    {
      action_internal_t action_label;
      action_label.label = j->label();

      for (data_expression_list::iterator k = j->arguments().begin(); k != j->arguments().end(); k++)
      {
        action_label.arguments.push_back(m_rewriter.convert_to(*k));
      }

      summand.action_label.push_back(action_label);
    }

    for (size_t j = 0; j < m_process_parameters.size(); j++)
    {
      if (data::search_free_variable(i->condition(), m_process_parameters[j]))
      {
        summand.condition_parameters.push_back(j);
      }
    }
    summand.condition_arguments_function = atermpp::function_symbol("condition_arguments", summand.condition_parameters.size());
    atermpp::vector<atermpp::aterm_int> dummy(summand.condition_arguments_function.arity(), atermpp::aterm_int(0));
    summand.condition_arguments_function_dummy = atermpp::aterm_appl(summand.condition_arguments_function, dummy.begin(), dummy.end());

    m_summands.push_back(summand);
  }

  if (use_summand_pruning)
  {
    parameter_score parameters[m_process_parameters.size()];
    for (size_t i = 0; i < m_process_parameters.size(); i++)
    {
      parameters[i].parameter_id = i;
      parameters[i].score = 0;

      for (action_summand_vector::iterator j = m_specification.process().action_summands().begin(); j != m_specification.process().action_summands().end(); j++)
      {
        parameters[i].score += condition_selectivity(j->condition(), m_process_parameters[i]);
      }
    }

    std::sort(parameters, parameters + m_process_parameters.size(), parameter_score_compare);

    for (size_t i = 0; i < m_process_parameters.size(); i++)
    {
      if (parameters[i].score > 0)
      {
        m_pruning_tree_parameters.push_back(parameters[i].parameter_id);
        mCRL2log(log::verbose) << "using pruning parameter " << m_process_parameters[parameters[i].parameter_id].name() << std::endl;
      }
    }

    m_pruning_tree.summand_subset = atermpp::shared_subset<summand_t>(m_summands);
  }
}

next_state_generator::~next_state_generator()
{
  m_false.unprotect();
  m_state_function.unprotect();
}

void next_state_generator::declare_constructors()
{
  // Declare all constructors to the rewriter to prevent unnecessary compilation.
  // This can be removed if the jittyc or innerc compilers are not in use anymore.
  // In certain cases it could be useful to add the mappings also, but this appears to
  // give a substantial performance penalty, due to the addition of symbols to the
  // rewriter that are not used.

  std::set<variable> variables = mcrl2::lps::find_variables(m_specification);
  std::set<variable> free_variables = mcrl2::lps::find_free_variables(m_specification);
  std::set<variable> nonfree_variables;
  std::set_difference(free_variables.begin(), free_variables.end(), variables.begin(), variables.end(), std::inserter(nonfree_variables, nonfree_variables.begin()));

  std::set<sort_expression> bounded_sorts;
  for (std::set<variable>::const_iterator i = nonfree_variables.begin(); i != nonfree_variables.end(); i++)
  {
    bounded_sorts.insert(i->sort());
  }
  for (std::set<sort_expression>::const_iterator i = bounded_sorts.begin(); i != bounded_sorts.end(); i++)
  {
    const function_symbol_vector constructors(m_specification.data().constructors(*i));
    for (function_symbol_vector::const_iterator j = constructors.begin(); j != constructors.end(); j++)
    {
      m_rewriter.convert_to(data_expression(*j));
    }
  }

  const function_symbol_vector constructors(m_specification.data().constructors());
  for (function_symbol_vector::const_iterator i = constructors.begin(); i != constructors.end(); i++)
  {
    m_rewriter.convert_to(data_expression(*i));
  }
}

next_state_generator::internal_state_t next_state_generator::get_internal_state(state s) const
{
  rewriter_term_t arguments[s.size()];
  for (size_t i = 0; i < s.size(); i++)
  {
    arguments[i] = m_rewriter.convert_to(s[i]);
  }
  return internal_state_t(m_state_function, arguments, arguments + s.size());
}

state next_state_generator::get_state(next_state_generator::internal_state_t internal_state) const
{
  state s;
  for (internal_state_t::const_iterator i = internal_state.begin(); i != internal_state.end(); i++)
  {
    s.push_back(m_rewriter.convert_from(*i));
  }
  return s;
}

bool next_state_generator::is_not_false(next_state_generator::summand_t &summand)
{
  return m_rewriter.rewrite_internal(summand.condition, m_pruning_tree_substitution) != m_false;
}

atermpp::shared_subset<next_state_generator::summand_t>::iterator next_state_generator::summand_subset(internal_state_t state)
{
  assert(m_use_summand_pruning);

  for (size_t i = 0; i < m_pruning_tree_parameters.size(); i++)
  {
    m_pruning_tree_substitution[m_process_parameters[m_pruning_tree_parameters[i]]] = rewriter_term_t();
  }

  pruning_tree_node_t *node = &m_pruning_tree;
  for (size_t i = 0; i < m_pruning_tree_parameters.size(); i++)
  {
    size_t parameter = m_pruning_tree_parameters[i];
    rewriter_term_t argument = state(parameter);
    m_pruning_tree_substitution[m_process_parameters[parameter]] = argument;
    atermpp::map<rewriter_term_t, pruning_tree_node_t>::iterator position = node->children.find(argument);
    if (position == node->children.end())
    {
      pruning_tree_node_t child;
      child.summand_subset = atermpp::shared_subset<summand_t>(node->summand_subset, boost::bind(&next_state_generator::is_not_false, this, _1));
      node->children[argument] = child;
      node = &node->children[argument];
    }
    else
    {
      node = &position->second;
    }
  }

  return node->summand_subset.begin();
}

next_state_generator::iterator::iterator(next_state_generator *generator, next_state_generator::internal_state_t state, next_state_generator::substitution_t *substitution)
  : m_generator(generator),
    m_state(state),
    m_substitution(substitution),
    m_use_summand_pruning(generator->m_use_summand_pruning),
    m_summand(0),
    m_caching(false)
{
#if 1
  for (size_t i = 0; i < generator->m_process_parameters.size(); i++)
  {
    (*m_substitution)[generator->m_process_parameters[i]] = state(i);
  }
  m_generator->m_sigma = m_substitution;
#endif

  if (m_use_summand_pruning)
  {
    m_summand_subset_iterator = generator->summand_subset(state);
  }
  else
  {
    m_summand_iterator = generator->m_summands.begin();
    m_summand_iterator_end = generator->m_summands.end();
  }

  m_transition.m_generator = m_generator;

  for (size_t i = 0; i < generator->m_process_parameters.size(); i++)
  {
    (*m_substitution)[generator->m_process_parameters[i]] = state(i);
  }

  increment();
}

next_state_generator::iterator::iterator(next_state_generator *generator, next_state_generator::internal_state_t state, next_state_generator::substitution_t *substitution, size_t summand_index)
  : m_generator(generator),
    m_state(state),
    m_substitution(substitution),
    m_use_summand_pruning(false),
    m_summand_iterator(generator->m_summands.begin() + summand_index),
    m_summand_iterator_end(m_summand_iterator + 1),
    m_summand(0),
    m_caching(false)
{
  m_transition.m_generator = m_generator;

  for (size_t i = 0; i < generator->m_process_parameters.size(); i++)
  {
    (*m_substitution)[generator->m_process_parameters[i]] = state(i);
  }

  increment();
}

void next_state_generator::iterator::increment()
{
  while (!m_summand ||
    (m_cached && m_enumeration_cache_iterator == m_enumeration_cache_end) ||
    (!m_cached && m_enumeration_iterator == enumerator_t::iterator_internal()))
  {
    if (m_caching)
    {
      m_summand->enumeration_cache[m_enumeration_cache_key] = m_enumeration_log;
    }

    if (m_use_summand_pruning)
    {
      if (!m_summand_subset_iterator)
      {
        m_generator = 0;
        return;
      }
      m_summand = &(*m_summand_subset_iterator++);
    }
    else
    {
      if (m_summand_iterator == m_summand_iterator_end)
      {
        m_generator = 0;
        return;
      }
      m_summand = &(*m_summand_iterator++);
    }

    if (m_generator->m_use_enumeration_caching)
    {
      rewriter_term_t condition_arguments[m_summand->condition_parameters.size()];
      for (size_t i = 0; i < m_summand->condition_parameters.size(); i++)
      {
        condition_arguments[i] = m_state(m_summand->condition_parameters[i]);
      }
      m_enumeration_cache_key = condition_arguments_t(m_summand->condition_arguments_function, condition_arguments, condition_arguments + m_summand->condition_parameters.size());
      atermpp::map<condition_arguments_t, summand_enumeration_t>::iterator position = m_summand->enumeration_cache.find(m_enumeration_cache_key);
      if (position == m_summand->enumeration_cache.end())
      {
        m_cached = false;
        m_caching = true;
        m_enumeration_log.clear();
      }
      else
      {
        m_cached = true;
        m_caching = false;
        m_enumeration_cache_iterator = position->second.begin();
        m_enumeration_cache_end = position->second.end();
      }
    }
    else
    {
      m_cached = false;
      m_caching = false;
    }
    if (!m_cached)
    {
      for (data::variable_list::iterator i = m_summand->variables.begin(); i != m_summand->variables.end(); i++)
      {
        (*m_substitution)[*i] = rewriter_term_t();
      }
      m_enumeration_iterator = m_generator->m_enumerator.begin_internal(m_summand->variables, m_summand->condition, *m_substitution);
    }
  }

  valuation_t valuation;
  if (m_cached)
  {
    valuation = *m_enumeration_cache_iterator;
    m_enumeration_cache_iterator++;
  }
  else
  {
    valuation = *m_enumeration_iterator++;
  }

  if (m_caching)
  {
    m_enumeration_log.push_back(valuation);
  }

  assert(valuation.size() == m_summand->variables.size());
  valuation_t::iterator v = valuation.begin();
  for (variable_list::iterator i = m_summand->variables.begin(); i != m_summand->variables.end(); i++, v++)
  {
    (*m_substitution)[*i] = *v;
  }

  rewriter_term_t state_arguments[m_summand->result_state.size()];
  for (size_t i = 0; i < m_summand->result_state.size(); i++)
  {
    state_arguments[i] = m_generator->m_rewriter.rewrite_internal(m_summand->result_state(i), *m_substitution);
  }
  m_transition.m_state = internal_state_t(m_generator->m_state_function, state_arguments, state_arguments + m_summand->result_state.size());

  action actions[m_summand->action_label.size()];
  for (size_t i = 0; i < m_summand->action_label.size(); i++)
  {
    data_expression arguments[m_summand->action_label[i].arguments.size()];
    for (size_t j = 0; j < m_summand->action_label[i].arguments.size(); j++)
    {
      arguments[j] = m_generator->m_rewriter.convert_from(m_generator->m_rewriter.rewrite_internal(m_summand->action_label[i].arguments[j], *m_substitution));
    }
    actions[i] = action(m_summand->action_label[i].label, data_expression_list(arguments, arguments + m_summand->action_label[i].arguments.size()));
  }
  m_transition.m_action = multi_action(action_list(actions, actions + m_summand->action_label.size()));

  for (variable_list::iterator i = m_summand->variables.begin(); i != m_summand->variables.end(); i++)
  {
    (*m_substitution)[*i] = rewriter_term_t();
  }
}
