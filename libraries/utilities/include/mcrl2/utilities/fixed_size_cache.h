// Author(s): Maurice Laveaux
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MCRL2_UTILITIES_FIXED_SIZE_CACHE_H
#define MCRL2_UTILITIES_FIXED_SIZE_CACHE_H

#include "mcrl2/utilities/cache_policy.h"

namespace mcrl2
{
namespace utilities
{

/// \brief A cache keeps track of key-value pairs similar to a map. The difference is that a cache
///        has (an optional) maximum size and a policy that determines what element gets evicted when
///        the cache is full.
/// \details Works with arbirary maps that implement the unordered_map interface.
template<typename Key,
  typename T,
  typename Policy = no_policy<Key, T>>
class fixed_size_cache
{
private:
  using Map = typename Policy::Map;
  using Pair = std::pair<Key, T>;

public:
  using iterator = typename Map::iterator;

  explicit fixed_size_cache(std::size_t max_size = 1024)
    : m_map(max_size)
  {
    if (max_size == 0)
    {
      m_maximum_size = std::numeric_limits<std::size_t>::max();
    }
    else
    {
      // The reason for this is that the internal mapping might only support powers of two and as such
      // we might as well use the additional capacity.
      m_maximum_size = m_map.capacity();
    }
  }

  iterator begin() { return m_map.begin(); }
  iterator end() { return m_map.end(); }

  void clear() { m_map.clear(); m_policy.clear(); }

  std::size_t count(const Key& key) const { return m_map.count(key); }

  iterator find(const Key& key)
  {
    return m_map.find(key);
  }

  /// \brief Stores the given key-value pair in the cache. Depending on the cache policy and capacity an existing elements
  ///        might be removed.
  template<typename ...Args>
  std::pair<iterator, bool> emplace(Args&&... args)
  {
    // The reason to split the find and emplace is that when we insert an element the replacement_candidate should not be
    // the key that we just inserted. The other way around, when an element that we are looking for was first removed and
    // then searched for also leads to unnecessary inserts.
    auto result = m_map.find(args...);
    if (result == m_map.end())
    {
      // If the cache would be full after an inserted.
      if (m_map.size() + 1 >= m_maximum_size)
      {
        // Remove an existing element defined by the policy.
        m_map.erase(m_policy.replacement_candidate(m_map));
      }

      // Insert an element and inform the policy that an element was inserted.
      auto emplace_result = m_map.emplace(std::forward<Args>(args)...);
      m_policy.inserted((*emplace_result.first).first);
      return emplace_result;
    }

    assert(m_map.size() <= m_maximum_size);
    return std::make_pair(result, false);
  }

private:
  Map    m_map;    ///< The underlying mapping from keys to their cached results.
  Policy m_policy; ///< The replacement policy for keys in the cache.

  std::size_t m_maximum_size; ///< The maximum number of elements to cache.
};

template<typename Key,
  typename T>
using fifo_cache = fixed_size_cache<Key, T, fifo_policy<Key, T>>;

} // namespace utilities
} // namespace mcrl2

#endif // MCRL2_UTILITIES_FIXED_SIZE_CACHE_H
