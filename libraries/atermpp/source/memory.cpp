#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdexcept>

#include <set>
#include <vector>
#include <string.h>
#include <sstream>


#include "mcrl2/utilities/logger.h"
#include "mcrl2/utilities/detail/memory_utility.h"
#include "mcrl2/atermpp/detail/memory.h"
#include "mcrl2/atermpp/aterm.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

namespace atermpp
{


namespace detail
{

// The hashtables are not vectors to prevent them from being
// destroyed prematurely.

static const size_t INITIAL_TERM_TABLE_SIZE = 1<<17;  // Must be a power of 2.
static size_t aterm_table_size;
size_t aterm_table_mask;            // used in memory.h
_aterm* * aterm_hashtable;  // used in memory.h

aterm static_undefined_aterm;
aterm static_empty_aterm_list;

/* The constants below are not static to prevent some compiler warnings */
const size_t INITIAL_MAX_TERM_SIZE = 256;

static const size_t BLOCK_SIZE = 1<<13;

typedef struct Block
{
  size_t data[BLOCK_SIZE];

  size_t size;
#ifndef NDEBUG
  struct Block* next_by_size;
#endif

  size_t* end;
} Block;

typedef struct TermInfo
{
  Block*       at_block;
  size_t* top_at_blocks;
  _aterm*       at_freelist;

  TermInfo():at_block(NULL),top_at_blocks(NULL),at_freelist(NULL)
  {}

} TermInfo;

static std::vector<TermInfo> terminfo(INITIAL_MAX_TERM_SIZE);

static size_t total_nodes = 0;


static void remove_from_hashtable(_aterm *t)
{
  /* Remove the node from the aterm_hashtable */
  _aterm *prev=NULL;
  const HashNumber hnr = hash_number(t) & aterm_table_mask;
  _aterm *cur = aterm_hashtable[hnr];

  do
  {
    assert(cur!=NULL); // This only occurs if the hashtable is in error.
    if (cur == t)
    {
      if (prev)
      {
        prev->next() = cur->next();
      }
      else
      {
        aterm_hashtable[hnr] = cur->next();
      }
      /* Put the node in the appropriate free list */
      total_nodes--;
      return;
    }
  }
  while (((prev=cur), (cur=cur->next())));
  assert(0);
}


/* Free a term, without removing it from the
   hashtable, and destroying its function symbol */
void simple_free_term(_aterm *t, const size_t arity)
{
  for(size_t i=0; i<arity; ++i)
  {
    reinterpret_cast<_aterm_appl<aterm> *>(t)->arg[i].decrease_reference_count();
  }

  TermInfo &ti = terminfo[TERM_SIZE_APPL(arity)];
  t->next()  = ti.at_freelist;
  ti.at_freelist = t; 
}




static void resize_aterm_hashtable()
{
  const size_t old_size=aterm_table_size;
  aterm_table_size <<=1; // Double the size.
  _aterm* * new_hashtable;

  {
    new_hashtable=reinterpret_cast<_aterm**>(calloc(aterm_table_size,sizeof(_aterm*)));
  }
  
  if (new_hashtable==NULL)
  {
    mCRL2log(mcrl2::log::warning) << "could not resize hashtable to size " << aterm_table_size << ". "; 
    aterm_table_size = old_size;
    return;
  }
  aterm_table_mask = aterm_table_size-1;
  
  /*  Rehash all old elements */
  for (size_t p=0; p<old_size; ++p) 
  {
    _aterm* aterm_walker=aterm_hashtable[p];

    while (aterm_walker)
    {
      assert(aterm_walker->reference_count()>0);
      _aterm* next = aterm_walker->next();
      const HashNumber hnr = hash_number(aterm_walker) & aterm_table_mask;
      aterm_walker->next() = new_hashtable[hnr];
      new_hashtable[hnr] = aterm_walker;
      assert(aterm_walker->next()!=aterm_walker);
      aterm_walker = next;
    }
  }
  free(aterm_hashtable);
  aterm_hashtable=new_hashtable;
}

#ifndef NDEBUG
static void check_that_all_objects_are_free()
{
#if 0
  std::cerr << "CHECKING THAT ALL OBJECTS ARE FREE \n";
  bool result=true;

  for(size_t size=0; size<terminfo.size(); ++size)
  {
    TermInfo *ti=&terminfo[size];
    for(Block* b=ti->at_block; b!=NULL; b=b->next_by_size)
    {
      for(_aterm* p=(_aterm*)b->data; p!=NULL && ((b==ti->at_block && p<(_aterm*)ti->top_at_blocks) || p<(_aterm*)b->end); p=p + size)
      {
        if (p->reference_count()!=0 && p->function()!=function_adm.AS_EMPTY_LIST)
        {
          fprintf(stderr,"CHECK: Non free term %p (size %lu). ",&*p,size);
          fprintf(stderr,"Reference count %ld\n",p->reference_count());
          result=false;
          assert(result);
        }
      }
    }
  }

  for(size_t i=4; i<function_lookup_table_size; ++i) // We do not check the first four function symbols
  {
    if (function_lookup_table[i].reference_count>0)  
    {
      result=false;
      fprintf(stderr,"Symbol %s has positive reference count (nr. %ld, ref.count %ld)\n",
                function_lookup_table[i].name.c_str(),i,function_lookup_table[i].reference_count);
    }

  }
#endif
}
#endif

void initialise_aterm_administration()
{
  // Explict initialisation on first use. This first
  // use is when a function symbol is created for the first time,
  // which may be due to the initialisation of a global variable in
  // a .cpp file, or due to the initialisation of a pre-main initialisation
  // of a static variable, which some compilers do.

  aterm_table_size=INITIAL_TERM_TABLE_SIZE;
  aterm_table_mask=aterm_table_size-1;

  aterm_hashtable=reinterpret_cast<_aterm**>(calloc(aterm_table_size,sizeof(_aterm*)));
  if (aterm_hashtable==NULL)
  {
    throw std::runtime_error("Out of memory. Cannot create an aterm symbol hashtable.");
  }

  // Check at exit that all function symbols and terms have been cleaned up properly.
  assert(!atexit(check_that_all_objects_are_free)); // zero is returned when registering is successful.
  
}


static void allocate_block(size_t size)
{
  Block* newblock = (Block*)calloc(1, sizeof(Block));
  if (newblock == NULL)
  {
    std::runtime_error("Out of memory. Could not allocate a block of memory to store terms.");
  }

  assert(size < terminfo.size());

  TermInfo &ti = terminfo[size];

  newblock->end = (newblock->data) + (BLOCK_SIZE - (BLOCK_SIZE % size));

  newblock->size = size;
#ifndef NDEBUG
  newblock->next_by_size = ti.at_block;
#endif
  ti.at_block = newblock;
  ti.top_at_blocks = newblock->data;
  assert(ti.at_block != NULL);
  assert(ti.at_freelist == NULL);
}



_aterm* allocate_term(const size_t size)
{
  if (size >= terminfo.size())
  {
    terminfo.resize(size+1);
  }

  if (total_nodes>=(aterm_table_size>>1))
  {
    // The hashtable is not big enough to hold nr_of_nodes_for_the_next_garbage_collect. So, resizing
    // is wise (although not necessary, due to the structure of the hastable, which allows is to contain
    // an arbitrary number of element, at some performance penalty.
    resize_aterm_hashtable();
  }

  _aterm *at;
  TermInfo &ti = terminfo[size];
  if (ti.at_block && ti.top_at_blocks < ti.at_block->end)
  {
    /* the first block is not full: allocate a cell */
    at = (_aterm *)ti.top_at_blocks;
    ti.top_at_blocks += size;
    at->reference_count()=0;
  }
  else if (ti.at_freelist)
  {
    /* the freelist is not empty: allocate a cell */
    at = ti.at_freelist;
    ti.at_freelist = ti.at_freelist->next();
    assert(ti.at_block != NULL);
    assert(ti.top_at_blocks == ti.at_block->end);
    assert(at->reference_count()==0);
  }
  else
  {
    /* there is no more memory of the current size allocate a block */
    allocate_block(size);
    assert(ti.at_block != NULL);
    at = (_aterm *)ti.top_at_blocks;
    ti.top_at_blocks += size;
    at->reference_count()=0;
  }

  total_nodes++;
  return at;
} 

_aterm* aterm_int(size_t val)
{
  HashNumber hnr = COMBINE(function_adm.AS_INT.number(), val);

  _aterm* cur = aterm_hashtable[hnr & aterm_table_mask];
  while (cur && (cur->function()!=function_adm.AS_INT || (reinterpret_cast<_aterm_int*>(cur)->value != val)))
  {
    cur = cur->next();
  }

  if (!cur)
  {
    cur = allocate_term(TERM_SIZE_INT);
    /* Delay masking until after allocate */
    hnr &= aterm_table_mask;
    new (&cur->function()) function_symbol(function_adm.AS_INT);
    reinterpret_cast<_aterm_int*>(cur)->value = val;

    cur->next() = aterm_hashtable[hnr];
    aterm_hashtable[hnr] = cur;
  }

  assert((hnr & aterm_table_mask) == (hash_number(cur) & aterm_table_mask));
  return cur;
}
} //namespace detail


void aterm::free_term() const
{
  detail::_aterm* t=this->m_term;
  assert(t->reference_count()==0);

  remove_from_hashtable(t);  // Remove from hash_table

  if (t->function()!=detail::function_adm.AS_INT)
  {
    for(size_t i=0; i<function().arity(); ++i)
    {
      reinterpret_cast<detail::_aterm_appl<aterm> *>(t)->arg[i].decrease_reference_count();
    }
  }
/* #ifndef NDEBUG
  const size_t function_symbol_index=function().number();
  const size_t ref_count=detail::function_lookup_table[function_symbol_index].reference_count;
#endif */
  const size_t size=detail::TERM_SIZE_APPL(t->function().arity());

  t->function().~function_symbol(); 

  detail::TermInfo &ti = detail::terminfo[size];
  t->next()  = ti.at_freelist;
  ti.at_freelist = t; 

/* #ifndef NDEBUG
  if (function_symbol_index==detail::function_adm.AS_EMPTY_LIST.number() && ref_count<=2) // When destroying the one but last empty_list function symbol, it 
                                                                                          // is likely that all other terms have been removed.
  {
    assert(detail::check_that_all_objects_are_free());
  }
#endif */
}

aterm::aterm(const function_symbol &sym)
{
  assert(sym.arity()==0);

  HashNumber hnr = sym.number();

  detail::_aterm* prev = NULL;
  detail::_aterm **hashspot = &(detail::aterm_hashtable[hnr & detail::aterm_table_mask]);

  detail::_aterm *cur = *hashspot;
  while (cur)
  {
    if (cur->function()==sym)
    {
      /* Promote current entry to front of hashtable */
      if (prev!=NULL)
      {
        prev->next() = cur->next();
        cur->next() = (detail::_aterm*) &**hashspot;
        *hashspot = cur;
      }

      m_term=cur;
      increase_reference_count<false>();
      return;
    }
    prev = cur;
    cur = cur->next();
  }

  cur = detail::allocate_term(detail::TERM_SIZE_APPL(0));
  /* Delay masking until after allocate */
  hnr &= detail::aterm_table_mask;
  new (&cur->function()) function_symbol(sym);
  
  cur->next() = &*detail::aterm_hashtable[hnr];
  detail::aterm_hashtable[hnr] = cur;

  m_term=cur;
  increase_reference_count<false>();
}


} // namespace atermpp

