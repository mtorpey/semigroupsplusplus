//
// Semigroups++ - C/C++ library for computing with semigroups and monoids
// Copyright (C) 2016 James D. Mitchell
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// TODO(JDM)
// 1) the other functionality of Semigroupe.

#ifndef SEMIGROUPSPLUSPLUS_SEMIGROUPS_H_
#define SEMIGROUPSPLUSPLUS_SEMIGROUPS_H_

#include <assert.h>

#include <algorithm>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "elements.h"
#include "recvec.h"
#include "report.h"

//
// This is the value used as the default value for the *report* parameter in
// many of the methods for the <Semigroup> class. You might want to change this
// to false, if you find the stuff printed when reporting annoying.

static const bool DEFAULT_REPORT_VALUE = true;

typedef size_t                letter_t;
typedef std::vector<letter_t> word_t;
typedef std::pair<word_t, word_t> relation_t;
typedef RecVec<size_t> cayley_graph_t;

//
// Class for representing a semigroup consisting of <Element>s and defined by a
// generating set.

class Semigroup {
  typedef RecVec<bool> flags_t;
  typedef size_t       index_t;
  typedef size_t       pos_t;

 public:
  // deleted
  //
  // The Semigroup class does not support an assignment contructor to avoid
  // accidental copying. An object in Semigroup may use many gigabytes of
  // memory and might be extremely expensive to copy. A copy constructor is
  // provided in case such a copy should be required anyway.

  Semigroup& operator=(Semigroup const&) = delete;

  // default
  // @gens   the generators of the semigroup
  //
  // This is the default constructor for a semigroup generated by <gens>.
  // The generators <gens> must all be of the same derived subclass of the
  // <Element> base class. Additionally, <gens> must satisfy the following:
  //
  // 1. there must be at least one generator;
  // 2. the generators must have equal degree <Element::degree>;
  //
  // if either of these points is not satisfied, then an asssertion failure
  // will occur.
  //
  // There can be duplicate generators and although they do not count as
  // distinct elements, they do count as distinct generators. In other words,
  // the generators of the semigroup are precisely (a copy of) <gens> in the
  // same order they occur in <gens>.
  //
  // The generators <gens> are copied by the constructor, and so it is the
  // responsibility of the caller to delete the argument <gens>.

  explicit Semigroup(std::vector<Element*>* gens);

  // copy constructor
  // @copy a semigrouup to copy
  //
  // Constructs a new <Semigroup> which is an exact copy of <copy>. No
  // enumeration is triggered by this constructor of <copy> or of the newly
  // constructed semigroup.

  Semigroup(const Semigroup& copy);

  // add generators
  // @copy a semigroup
  // @coll a collection of additional generators
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This is a constructor for a semigroup generated by the generators of the
  // <Semigroup> <copy> and the (possibly) additional generators <gens>.
  //
  // The relevant parts of the data structure of @copy are copied and then
  // <add_generators> is called. Consequently, the semigroup is (partially or
  // fully) enumerated by this constructor.
  //
  // The same effect can be obtained by copying <copy> using the copy
  // constructor and then calling <add_generators>. However, this constructor
  // avoids copying those parts of the data structure of <copy> that
  // <add_generators> invalidates anyway. If <copy> has not been enumerated at
  // all, then these two routes for adding more generators are equivalent.

  Semigroup(const Semigroup&       copy,
            std::vector<Element*>* coll,
            bool                   report = DEFAULT_REPORT_VALUE);

  ~Semigroup();

  // Const methods

  // const
  //
  // This method is const.
  //
  // @return The maximum length of any element so far computed.

  size_t current_max_word_length() const {
    if (is_done()) {
      return _lenindex.size() - 2;
    } else if (_nr > _lenindex.back()) {
      return _lenindex.size();
    } else {
      return _lenindex.size() - 1;
    }
  }

  // const
  //
  // This method is const.
  //
  // @return the degree of the elements in the semigroup.

  size_t degree() const {
    return _degree;
  }

  // const
  //
  // This method is const.
  //
  // @return the number of generators of the semigroup.

  size_t nrgens() const {
    return _gens->size();
  }

  // const
  //
  // This method is const.
  //
  // @return a pointer to a vector containing the generators of the semigroup.

  std::vector<Element*>* gens() const {
    return _gens;
  }

  // const
  //
  // This method is const.
  //
  // @return true if the semigroup is fully enumerated and false if not

  bool is_done() const {
    return (_pos >= _nr);
  }

  // const
  //
  // This method is const.
  //
  // @return true if no elements (other than the generators) have
  // been enumerated and false otherwise

  bool is_begun() const {
    assert(_lenindex.size() > 1);
    return (_pos >= _lenindex[1]);
  }

  // const
  // @x pointer to an element of the same type as those in the semigroup.
  //
  // This method is const.
  //
  // @return true if the element <x> is already known to belong to the
  // semigroup, and false if not. If the semigroup is not fully enumerated,
  // then this method may return false when <x> is in the semigroup, but not
  // this is not yet known. See also <position> and <position_sorted>.

  pos_t position_current(Element* x) const {
    if (x->degree() != _degree) {
      return UNDEFINED;
    }

    auto it = _map.find(x);
    return (it == _map.end() ? UNDEFINED : it->second);
  }

  // const
  //
  // This method is const.
  // @return the number of elements in the semigroup that have been enumerated
  // so far.

  size_t current_size() const {
    return _elements->size();
  }

  // const
  //
  // This method is const.
  //
  // @return the number of relations in the presentation for the semigroup
  // that have been found so far

  size_t current_nrrules() const {
    return _nrrules;
  }

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and it asserts that <pos> is valid.
  //
  // @return the position of the prefix of the element **s** in position
  // <pos> (of the semigroup) of length one less than the length of **s**.

  pos_t prefix(pos_t pos) const {
    assert(pos < _nr);
    return _prefix[pos];
  }

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and it asserts that <pos> is valid.
  //
  // @return the position of the suffix of the element **s** in position
  // <pos> (of the semigroup) of length one less than the length of **s**.

  pos_t suffix(pos_t pos) const {
    assert(pos < _nr);
    return _suffix[pos];
  }

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and it asserts that <pos> is valid.
  //
  // @return the first letter of the element in position <pos> of the
  // semigroup, i.e. the index of the generator corresponding to the first
  // letter of the element. Note that <gens>[<first_letter>(<pos>)] may not
  // equal _elements[<first_letter>(<pos>)].

  letter_t first_letter(pos_t pos) const {
    assert(pos < _nr);
    return _first[pos];
  }

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and it asserts that <pos> is valid.
  //
  // @return the final letter of the element in position <pos> of the
  // semigroup, i.e. the index of the generator corresponding to the final
  // letter of the element. Note that <gens>[<final_letter>(<pos>)] may not
  // equal _elements[<final_letter>(<pos>)].

  letter_t final_letter(pos_t pos) const {
    assert(pos < _nr);
    return _final[pos];
  }

  // const
  // This method is const.
  //
  // @return the current value of the batch size. This is the minimum number
  // of elements enumerated in any call to the <enumerate> method.

  size_t batch_size() const {
    return _batch_size;
  }

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and it asserts that <pos> is valid.
  //
  // @return the length of the element in position <pos>. This assumes that
  // such an element has already been enumerated and will cause an assertion
  // failure if it is not.

  size_t length_const(pos_t pos) const {
    assert(pos < _nr);
    return _length[pos];
  }

  // non-const
  // @pos a valid position of an element of the semigroup.
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // @return the length of the element in position <pos>, assuming that
  // there is such an element, if not, this will cause an assertion failure.

  size_t length_non_const(pos_t pos, bool report = DEFAULT_REPORT_VALUE) {
    if (pos >= _nr) {
      enumerate(report);
    }
    return length_const(pos);
  }

  // const
  // @i  a valid position of an already enumerated element of the semigroup
  // @j  a valid position of an already enumerated element of the semigroup
  //
  // This method is const and it asserts that the values <i> and <j> are valid.
  // @return the position <pos_t> in the semigroup of the product of the <i>
  // and <j>th elements by following the path in the right or left Cayley graph
  // from <i> to <j>, whichever is shorter.

  pos_t product_by_reduction(pos_t i, pos_t j) const;

  // const
  // @i  a valid position of an already enumerated element of the semigroup
  // @j  a valid position of an already enumerated element of the semigroup
  //
  // This method is const, it asserts that the values <i> and <j> are valid,
  // and it either:
  //
  // * follows the path in the right or left Cayley graph from <i> to <j>,
  //   whichever is shorter (<product_by_reduction>); or
  //
  // * multiplies the elements in postions <i> and <j> together;
  //
  // whichever is better. This is determined by comparing <Element::complexity>
  // and the <length> of <i> and <j>.
  //
  // For example, if the <Element::complexity> of the multiplication is linear
  // and the semigroup is a semigroup of transformations of degree 20, and the
  // shortest paths in the left and right Cayley graphs from <i> to <j> are of
  // length 100 and 1131, then it better to just multiply the transformation.
  //
  // @return the position <pos_t> in the semigroup of the product of the <i>
  // and <j>th elements.

  pos_t fast_product(pos_t i, pos_t j) const;

  // const
  // @i  a valid position of a generator of the semigroup
  //
  // This method is const, and it asserts that the value <i> is valid.
  // In many cases <genslookup>(<i>) will equal <i>, examples of when this
  // will not be the case are:
  //
  // * there are duplicate generators;
  //
  // * <add_generators> was called after the semigroup was already partially
  // enumerated.
  //
  // @return the position <pos_t> of the <i>th generator.

  pos_t genslookup(letter_t i) const {
    assert(i < _nrgens);
    return _genslookup[i];
  }

  // non-const
  // @report report during enumeration and/or counting, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  // @nr_threads the number of threads to use (defaults to 1)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // If the size of the semigroup is less than 65537 or the number of threads
  // is 1, then this is a single-threaded function. Otherwise, the elements of
  // the semigroup are tested for idempotency in <nr_threads> concurrent
  // threads.
  //
  // The value of the positions, and number, of idempotents is stored after
  // they are first computed.
  //
  // @return the total number of idempotents in the semigroup.

  size_t nr_idempotents(bool   report     = DEFAULT_REPORT_VALUE,
                        size_t nr_threads = 1);

  // non-const
  // @pos a valid position of an element of the semigroup.
  // @report report during enumeration and/or counting, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  // @nr_threads the number of threads to use (defaults to 1)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // @return true if the <pos> element of the semigroup is an idempotent and
  // return false otherwise.

  bool is_idempotent(pos_t  pos,
                     bool   report     = DEFAULT_REPORT_VALUE,
                     size_t nr_threads = 1);

  // non-const
  // @report report during enumeration and/or counting, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  // @nr_threads the number of threads to use (defaults to 1)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // If the size of the semigroup is less than 65537 or the number of threads
  // is 1, then this is a single-threaded function. Otherwise, the elements of
  // the semigroup are tested for idempotency in <nr_threads> concurrent
  // threads.
  //
  // The value of the positions, and number, of idempotents is stored after
  // they are first computed.
  //
  // @return a const iterator for the positions <pos_t> of idempotents in
  // the semigroup.

  std::vector<pos_t>::const_iterator
  idempotents_cbegin(bool report = DEFAULT_REPORT_VALUE, size_t nr_threads = 1);

  // non-const
  // @report report during enumeration and/or counting, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  // @nr_threads the number of threads to use (defaults to 1)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // If the size of the semigroup is less than 65537 or the number of threads
  // is 1, then this is a single-threaded function. Otherwise, the elements of
  // the semigroup are tested for idempotency in <nr_threads> concurrent
  // threads.
  //
  // The value of the positions, and number, of idempotents is stored after
  // they are first computed.
  //
  // @return a const iterator for the positions <pos_t> of idempotents in
  // the semigroup.

  std::vector<pos_t>::const_iterator
  idempotents_cend(bool report = DEFAULT_REPORT_VALUE, size_t nr_threads = 1);

  // non-const
  // @report report during enumeration and/or counting, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup. See <next_relation>.
  //
  // @return the total number of relations in the presentation defining the
  // semigroup.

  size_t nrrules(bool report) {
    enumerate(report);
    return _nrrules;
  }

  // non-const
  // @batch_size a new value for the batch size.
  //
  // This method is non-const since it changes a class data member, but it does
  // not enumerate the semigroup any further.
  //
  // The *batch size* is the number of new elements to be found by any call to
  // <enumerate>. A call to <enumerate> returns between 0 and approximately the
  // batch size.
  //
  // The default value of the batch size is 8192.
  //
  // This is used by, for example, <position> so that it is possible to find
  // the position of an element without fully enumerating the semigroup.

  void set_batch_size(size_t batch_size) {
    _batch_size = batch_size;
  }

  // non-const
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // @return the size of the semigroup.

  size_t size(bool report = DEFAULT_REPORT_VALUE) {
    enumerate(report);
    return _elements->size();
  }

  // non-const
  // @x      a possible element of the semigroup
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // This method can be used to check if the element <x> is an element of the
  // semigroup. The semigroup is enumerated in batches until <x> is found or
  // the semigroup is fully enumerated but <x> was not found (see
  // <set_batch_size>).
  //
  // @return true or false.

  bool test_membership(Element* x, bool report = DEFAULT_REPORT_VALUE) {
    return (position(x, report) != UNDEFINED);
  }

  // non-const
  // @x      a possible element of the semigroup
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // This method can be used to find the <pos_t> position of the element <x> if
  // it belongs to the semigroup. The semigroup is enumerated in batches until
  // <x> is found or the semigroup is fully enumerated but <x> was not found
  // (see <set_batch_size>).
  //
  // @return the <pos_t> position of the element <x> in the semigroup, or
  // <UNDEFINED>.

  pos_t position(Element* x, bool report = DEFAULT_REPORT_VALUE);

  // non-const
  // @x      a possible element of the semigroup
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // This method can be used to find the position of the element <x> in the
  // sorted array of elements of the semigroup.
  //
  // @return the position of the element <x> in the sorted array of elements of
  // the semigroup, or <UNDEFINED>.

  size_t position_sorted(Element* x, bool report = DEFAULT_REPORT_VALUE);

  // non-const
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // @return the vector consisting of pointers to all of the elements of the
  // semigroup.
  //
  // TODO(JDM) replace this with a method for cbegin and cend, don't allow
  // access to _elements.

  std::vector<Element*>* elements(bool report = DEFAULT_REPORT_VALUE) {
    enumerate(report);
    return _elements;
  }

  // non-const
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // @return a vector consisting of pairs **x** where **x.first** is a pointer
  // to an element of the semigroup and **x.second** is the <pos_t> position of
  // that element in the semigroup. This vector is sorted.
  //
  // TODO(JDM) replace this with a method for sorted_cbegin and sorted_cend.

  std::vector<std::pair<Element*, pos_t>>*
  sorted_elements(bool report = DEFAULT_REPORT_VALUE) {
    sort_elements(report);
    return _sorted;
  }

  // non-const
  // @pos a position of an element of the semigroup.
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>).
  //
  // This method is non-const since it may involve fully enumerating the
  // semigroup.
  //
  // @return the element of the semigroup in position <pos>, or a *nullptr* if
  // there is no such element.

  Element* at(pos_t pos, bool report = DEFAULT_REPORT_VALUE);

  // const
  // @pos a valid position of an already enumerated element of the semigroup.
  //
  // This method is const and does no checks on its arguments.
  //
  // @return the element of the semigroup in position <pos>.

  Element* operator[](pos_t pos) const {
    return (*_elements)[pos];
  }

  // non-const
  // @pos a position of an already enumerated element of the semigroup.
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>).
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // @return the element of the semigroup in position <pos> of the sorted array
  // of elements, or nullptr in <pos> is not valid (i.e. too big).

  Element* sorted_at(pos_t pos, bool report = DEFAULT_REPORT_VALUE);

  // non-const
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>).
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // @return a pointer to the right Cayley graph of the semigroup.

  cayley_graph_t* right_cayley_graph(bool report = DEFAULT_REPORT_VALUE) {
    enumerate(report);
    return _right;
  }

  // non-const
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>).
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // @return a pointer to the left Cayley graph of the semigroup.

  cayley_graph_t* left_cayley_graph(bool report = DEFAULT_REPORT_VALUE) {
    enumerate(report);
    return _left;
  }

  // non-const
  // @word   changed in-place to contain a word in the generators equal to the
  //         <pos> element of the semigroup
  // @pos    a possible position of element of the semigroup.
  // @report report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it may involve enumerating the semigroup.
  //
  // If <pos> is less than the size of this semigroup, then this method
  // changes its first argument <word> in-place (first clearing it and then)
  // to contain a minimal factorization of the element in position <pos> of
  // the semigroup with respect to the generators of the semigroup.  This method
  // enumerates the semigroup until at least the <pos> element is known. If
  // <pos> is greater than the size of the semigroup, then nothing happens
  // and <word> is not modified, in particular not cleared.

  void
  factorisation(word_t& word, pos_t pos, bool report = DEFAULT_REPORT_VALUE);

  // non-const
  //
  // This method is non-const since it involves changing some private data
  // members but it does not call <enumerate>.
  //
  // This resets the private data members in the semigroup that govern the
  // behaviour of <next_relation>. After a call to this function, the next call
  // to <next_relation> will return the first relation of the presentation
  // defining the semigroup.

  void reset_next_relation() {
    _relation_pos = UNDEFINED;
    _relation_gen = 0;
  }

  // non-const
  // @relation container for the next relation, changed in-place.
  // @report   report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This method is non-const since it involves fully enumerating the
  // semigroup.
  //
  // This method changes <relation> in-place so that one of the following
  // holds:
  //
  // * <relation> is a vector consisting of a <letter_t> and a <letter_t> such
  //   that <gens>()[<relation>[0]] == <gens>()[<relation>[1]],
  //   i.e. if the semigroup was defined with duplicate generators;
  //
  // * <relation> is a vector consisting of a <pos_t>, <letter_t>, and <pos_t>
  //   such that
  //   **this**[<relation>[0]] \* <gens>()[<relation>[1]] ==
  //   **this**[<relation>[2]]
  //
  // * <relation> is empty if there are no more relations.
  //
  // <next_relation> is guaranteed to output all relations of length 2 before
  // any relations of length 3. If called repeatedly after
  // <reset_next_relation>, and until <relation> is empty, then the values
  // placed in <relation> correspond to a length-reducing confluent rewriting
  // system that defines the semigroup.
  //
  // This can be used in conjunction with <factorisation> to obtain a
  // presentation defining the semigroup.
  //
  // See also <reset_next_relation>.

  void next_relation(std::vector<size_t>& relation,
                     bool                 report = DEFAULT_REPORT_VALUE);

  // non-const
  // @limit    the number of elements to enumerate (defaults to <LIMIT_MAX>)
  // @report   report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This is the main method of the Semigroup class, and is where the
  // Froidure-Pin Algorithm is implemented.  This method is non-const since it
  // involves partially enumerating the semigroup.
  //
  // If the semigroup is already fully enumerated, or the number of elements
  // previously enumerated exceeds <limit>, then calling this function does
  // nothing. Otherwise, <enumerate> attempts to find at least the maximum
  // of <limit> and the batch size elements of the semigroup. If the semigroup
  // is fully enumerated, then it knows its left and right Cayley graphs, and a
  // minimal factorisation of every element (in terms of its generating set).
  // All of the elements are stored in memory until the object is destroyed.

  void enumerate(size_t limit = LIMIT_MAX, bool report = DEFAULT_REPORT_VALUE);

  // non-const
  // @report   report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // Calls <enumerate>(LIMIT_MAX, report), and so triggers a full enumeration
  // of the semigroup.

  void inline enumerate(bool report = DEFAULT_REPORT_VALUE) {
    enumerate(LIMIT_MAX, report);
  }

  // non-const
  // @coll    the new generators to be added
  // @report  report during enumeration, if any (defaults to
  // <DEFAULT_REPORT_VALUE>)
  //
  // This function can be used to add new generators to the existing semigroup
  // in such a way that any previously enumerated data is preserved and not
  // recomputed, or copied. This can be faster than recomputing the semigroup
  // generated by the old generators and the new in <coll>.
  //
  // This method is non-const, and changes the semigroup in place, thereby
  // invalidating possibly previously known data about the semigroup, such as
  // the left or right Cayley graphs.
  //
  // A generator is only added if it is does not belong in the existing data
  // structure (it may belong to the semigroup but just not be known to
  // belong). If <coll> is empty or only contains elements that are known to
  // belong to the semigroup, then the semigroup is left unchanged.
  //
  // Otherwise, the semigroup is returned in a state where all of the
  // previously enumerated elements which had been multiplied by all of the old
  // generators, have now been multiplied by the new generators. This means
  // that after this function is called the semigroup might contain many more
  // elements than before (whether it is fully enumerating or not). It can also
  // be the case that the new generators are the only new elements, unlike say,
  // in the case of groups.
  //
  // The elements the argument <coll> are copied into the semigroup, and should
  // be deleted by the caller.

  void add_generators(const std::unordered_set<Element*>* coll,
                      bool report = DEFAULT_REPORT_VALUE);

  // static
  //
  // This variable is used to indicate that a value is undefined, such as, for
  // example, the position of an element that does not belong to a semigroup.

  static size_t UNDEFINED;

  // static
  //
  // This variable is used to indicate the maximum possible limit that can be
  // used with <enumerate>.

  static size_t LIMIT_MAX;

 private:
  // Initialise the data member _sorted. We store a list of pairs
  // <Element*, pos_t> which is sorted on the first entry using the Less
  // subclass. This is done so that we can both get the elements in sorted
  // order, and find the position of an element in the sorted list of
  // elements.

  void sort_elements(bool report = DEFAULT_REPORT_VALUE);

  // Find the idempotents and store their positions and their number
  void find_idempotents(bool   report     = DEFAULT_REPORT_VALUE,
                        size_t nr_threads = 1);

  // Function for counting idempotents in a thread, changes the parameter <nr>
  // in place.

  void idempotents_thread(size_t              thread_id,
                          size_t&             nr,
                          std::vector<pos_t>& idempotents,
                          std::vector<bool>&  is_idempotent,
                          pos_t               begin,
                          pos_t               end);

  // Expand the data structures in the semigroup with space for <nr> elements

  void inline expand(size_t nr) {
    _left->add_rows(nr);
    _reduced.add_rows(nr);
    _right->add_rows(nr);
    _multiplied.resize(_multiplied.size() + nr, false);
  }

  // Check if an element is the identity, <x> should be in the position <pos> of
  // _elements.

  void inline is_one(Element const* x, pos_t pos) {
    if (!_found_one && *x == *_id) {
      _pos_one   = pos;
      _found_one = true;
    }
  }

  // Update the data structure in add_generators
  void inline closure_update(pos_t              i,
                             letter_t           j,
                             letter_t           b,
                             letter_t           s,
                             std::vector<bool>& old_new,
                             pos_t              old_nr);
  // For sorting
  struct Less {
    explicit Less(Semigroup const& semigroup) : _semigroup(semigroup) {}

    bool operator()(std::pair<Element const*, size_t> const& x,
                    std::pair<Element const*, size_t> const& y) {
      return *(x.first) < *(y.first);
    }

    Semigroup const& _semigroup;
  };

  size_t _batch_size;
  size_t _degree;
  std::vector<std::pair<size_t, size_t>> _duplicate_gens;
  std::vector<Element*>*    _elements;
  std::vector<size_t>       _final;
  std::vector<size_t>       _first;
  bool                      _found_one;
  std::vector<Element*>*    _gens;
  std::vector<size_t>       _genslookup;
  Element*                  _id;
  std::vector<size_t>       _idempotents;
  bool                      _idempotents_found;
  pos_t                     _idempotents_start_pos;
  std::vector<bool>         _is_idempotent;
  std::vector<size_t>       _index;
  cayley_graph_t*           _left;
  std::vector<size_t>       _length;
  std::vector<size_t>       _lenindex;
  std::unordered_map<const Element*, size_t, hash<const Element*>, myequal>
                      _map;
  std::vector<bool>   _multiplied;
  size_t              _nr;
  size_t              _nrgens;
  size_t              _nr_idempotents;
  size_t              _nrrules;
  size_t              _pos;
  size_t              _pos_one;
  std::vector<size_t> _prefix;
  flags_t             _reduced;
  size_t              _relation_gen;
  size_t              _relation_pos;
  cayley_graph_t*     _right;
  std::vector<std::pair<Element*, size_t>>* _sorted;
  std::vector<size_t>* _pos_sorted;
  std::vector<size_t>  _suffix;
  Element*             _tmp_product;
  size_t               _wordlen;

  Reporter      _reporter;

  static uint_fast8_t IS_IDEMPOTENT;
};

#endif  // SEMIGROUPSPLUSPLUS_SEMIGROUPS_H_
