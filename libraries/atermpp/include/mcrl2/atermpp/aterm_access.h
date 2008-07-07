// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/atermpp/aterm_access.h
/// \brief Convenience functions for accessing the child nodes of aterm_appl and aterm_list. 

#ifndef MCRL2_ATERMPP_ATERM_ACCESS_H
#define MCRL2_ATERMPP_ATERM_ACCESS_H

#include "mcrl2/atermpp/aterm.h"
#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/atermpp/aterm_list.h"

namespace atermpp
{
  /// Returns the first child of t casted to an aterm_appl.
  inline
  aterm_appl arg1(ATermAppl t)
  {
    return aterm_appl(t)(0);
  }
  
  /// Returns the second child of t casted to an aterm_appl.
  inline
  aterm_appl arg2(ATermAppl t)
  {
    return aterm_appl(t)(1);
  }
  
  /// Returns the third child of t casted to an aterm_appl.
  inline
  aterm_appl arg3(ATermAppl t)
  {
    return aterm_appl(t)(2);
  }

  /// Returns the fourth child of t casted to an aterm_appl.
  inline
  aterm_appl arg4(ATermAppl t)
  {
    return aterm_appl(t)(3);
  }
  
  /// Returns the first child of t casted to an aterm_list.
  inline
  ATermList list_arg1(ATermAppl t)
  {
    return aterm_list(aterm_appl(t)(0));
  }
  
  /// Returns the second child of t casted to an aterm_list.
  inline
  ATermList list_arg2(ATermAppl t)
  {
    return aterm_list(aterm_appl(t)(1));
  }
  
  /// Returns the third child of t casted to an aterm_list.
  inline
  ATermList list_arg3(ATermAppl t)
  {
    return aterm_list(aterm_appl(t)(2));
  }

  /// Returns the fourth child of t casted to an aterm_list.
  inline
  ATermList list_arg4(ATermAppl t)
  {
    return aterm_list(aterm_appl(t)(3));
  }

} // namespace atermpp

#endif // MCRL2_ATERMPP_ATERM_ACCESS_H
