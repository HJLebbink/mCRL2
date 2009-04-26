// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file parser_test.cpp
/// \brief Regression test for parsing a data specification.

#include <iostream>
#include <boost/range/iterator_range.hpp>
#include <boost/test/minimal.hpp>

#include "mcrl2/data/pos.h"
#include "mcrl2/data/standard_utility.h"
#include "mcrl2/data/parser.h"
#include "mcrl2/data/standard.h"
#include "mcrl2/core/garbage_collection.h"

using namespace mcrl2;

void parser_test()
{
  const std::string text(
    "sort S;\n"
    "cons s:S;\n"
    "map f:S -> List(S);\n"
  );

  data::data_specification spec1(data::parse_data_specification(text));
  data::data_specification spec(data::remove_all_system_defined(data::parse_data_specification(text)));
  BOOST_CHECK(boost::copy_range< data::sort_expression_vector >(spec.sorts()).size() == 1);
  BOOST_CHECK(boost::copy_range< data::function_symbol_vector >(spec.constructors()).size() == 1);
  BOOST_CHECK(boost::copy_range< data::function_symbol_vector >(spec.mappings()).size() == 1);

  BOOST_CHECK(data::parse_data_expression("2") == data::sort_pos::pos(2));
  BOOST_CHECK(data::parse_data_expression("0") == data::sort_nat::nat(0));
//  BOOST_CHECK(data::parse_data_expression("-1") == data::sort_int_::int_(-1));
//  BOOST_CHECK(data::parse_data_expression("1/2") == data::sort_real_::real_(1, 2));
}

int test_main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv);

  parser_test();
  core::garbage_collect();

  return EXIT_SUCCESS;
}
