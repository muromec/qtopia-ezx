/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: assocvector.h,v 1.5 2007/02/13 21:20:44 ping Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Welsey Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// Last update: February 19, 2001

#ifndef ASSOCVECTOR_INC_
#define ASSOCVECTOR_INC_

#include <algorithm>
#include <functional>
#pragma warning (disable : 4530)
#include <vector>
#pragma warning (default : 4530)
#include <utility>

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template Takeover
// Used to convey move semantics to containers
////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class Takeover
    {
    public:
        Takeover(T& obj) : ref_(obj) {}
        operator T&() const { return ref_; } 
    private:
        T& ref_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template TakeoverIterator
// Used to convey move semantics to containers
////////////////////////////////////////////////////////////////////////////////

    template <class T, class Iter>
    class TakeoverIterator
    {
    public:
        TakeoverIterator(Iter i) : i_(i) {}
        Takeover<T> operator*() const { return Takeover<T>(*i_); } 
        T* operator->() const { return &*i_; }
        bool operator!=(const TakeoverIterator& rhs)
        {
            return i_ != rhs.i_;
        }
        TakeoverIterator& operator++()
        {
            ++i_;
            return *this;
        }
    private:
        Iter i_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template AssocVectorCompare
// Used by AssocVector
////////////////////////////////////////////////////////////////////////////////

    namespace Private
    {
        template <class Value, class C>
        class AssocVectorCompare : public C
        {
            typedef std::pair<typename C::first_argument_type, Value>
                Data;
            typedef typename C::first_argument_type first_argument_type;

        public:
            AssocVectorCompare()
            {}
            
            AssocVectorCompare(const C& src) : C(src)
            {}
            
            bool operator()(const first_argument_type& lhs, 
                const first_argument_type& rhs) const
            { return C::operator()(lhs, rhs); }
            
            bool operator()(const Data& lhs, const Data& rhs) const
            { return operator()(lhs.first, rhs.first); }
            
            bool operator()(const Data& lhs, 
                const first_argument_type& rhs) const
            { return operator()(lhs.first, rhs); }
            
            bool operator()(const first_argument_type& lhs,
                const Data& rhs) const
            { return operator()(lhs, rhs.first); }
        };
    }

////////////////////////////////////////////////////////////////////////////////
// class template AssocVector
// An associative vector built as a syntactic drop-in replacement for std::map
// BEWARE: AssocVector doesn't respect all map's guarantees, the most important
//     being:
// * iterators are invalidated by insert and erase operations
// * the complexity of insert/erase is O(N) not O(log N)
// * value_type is std::pair<K, V> not std::pair<const K, V>
// * iterators are random
////////////////////////////////////////////////////////////////////////////////

    template
    <
        class K,
        class V,
        class C = std::less<K>,
        class A = std::allocator< std::pair<K, V> >
    >
    class AssocVector 
        : private std::vector< std::pair<K, V>, A >
        , private Private::AssocVectorCompare<V, C>
    {
        typedef std::vector<std::pair<K, V>, A> Base;
        typedef Private::AssocVectorCompare<V, C> MyCompare;

    public:
        typedef K key_type;
        typedef V mapped_type;
        typedef typename Base::value_type value_type;

        typedef C key_compare;
        typedef A allocator_type;
        typedef typename A::reference reference;
        typedef typename A::const_reference const_reference;
        typedef typename Base::iterator iterator;
        typedef typename Base::const_iterator const_iterator;
        typedef typename Base::size_type size_type;
        typedef typename Base::difference_type difference_type;
        typedef typename A::pointer pointer;
        typedef typename A::const_pointer const_pointer;
        typedef typename Base::reverse_iterator reverse_iterator;
        typedef typename Base::const_reverse_iterator const_reverse_iterator;

    	class value_compare
            : public std::binary_function<value_type, value_type, bool>
            , private key_compare
        {
            friend class AssocVector;
        
        protected:
            value_compare(key_compare pred) : key_compare(pred)
            {}

        public:
            bool operator()(const value_type& lhs, const value_type& rhs) const
            { return key_compare::operator()(lhs.first, rhs.first); }
        };
        
        // 23.3.1.1 construct/copy/destroy

        explicit AssocVector(const key_compare& comp = key_compare(), 
            const A& alloc = A())
        : Base(alloc), MyCompare(comp)
        {}
        
        template <class InputIterator>
        AssocVector(InputIterator first, InputIterator last, 
            const key_compare& comp = key_compare(), 
            const A& alloc = A())
        : Base(first, last, alloc), MyCompare(comp)
        {
            MyCompare& me = *this;
            std::sort(begin(), end(), me);
        }
        
        AssocVector& operator=(const AssocVector& rhs)
        { AssocVector(rhs).swap(*this);return *this;}

        // iterators:
        // The following are here because MWCW gets 'using' wrong
        iterator begin() { return Base::begin(); }
        const_iterator begin() const { return Base::begin(); }
        iterator end() { return Base::end(); }
        const_iterator end() const { return Base::end(); }
        reverse_iterator rbegin() { return Base::rbegin(); }
        const_reverse_iterator rbegin() const { return Base::rbegin(); }
        reverse_iterator rend() { return Base::rend(); }
        const_reverse_iterator rend() const { return Base::rend(); }
        
        // capacity:
        bool empty() const { return Base::empty(); }
        size_type size() const { return Base::size(); }
        size_type max_size() { return Base::max_size(); }

        // 23.3.1.2 element access:
        mapped_type& operator[](const key_type& key)
        { return insert(value_type(key, mapped_type())).first->second; }

        // modifiers:
        std::pair<iterator, bool> insert(const value_type& val)
        {
            bool notFound(false);
            iterator i(lower_bound(val.first));

            if (i == end() || operator()(val.first, i->first))
            {
                i = Base::insert(i, val);
                notFound = true;
            }
            return std::make_pair(i, notFound);
        }

        iterator insert(iterator pos, const value_type& val)
        {
            if (pos != end() && operator()(*pos, val) && 
                (pos == end() - 1 ||
                    !operator()(val, pos[1]) &&
                        operator()(pos[1], val)))
            {
                return Base::insert(pos, val);
            }
            return insert(val).first;
        }
       
        template <class InputIterator>
        iterator insert(InputIterator first, InputIterator last)
        { for (; first != last; ++first) insert(*first); }
        
        void erase(iterator pos)
        { Base::erase(pos); }

        size_type erase(const key_type& k)
        {
            iterator i(find(k));
            if (i == end()) return 0;
            erase(i);
            return 1;
        }

        void erase(iterator first, iterator last)
        { Base::erase(first, last); }

        void swap(AssocVector& other)
        {
            using namespace std;
            Base::swap(other);
            MyCompare& me = *this;
            MyCompare& rhs = other;
            std::swap(me, rhs);
        }
        
        void clear()
        { Base::clear(); }

        // observers:
        key_compare key_comp() const
        { return *this; }

        value_compare value_comp() const
        {
            const key_compare& comp = *this;
            return value_compare(comp);
        }

        // 23.3.1.3 map operations:
        iterator find(const key_type& k)
        {
            iterator i(lower_bound(k));
            if (i != end() && operator()(k, i->first))
            {
                i = end();
            }
            return i;
        }

        const_iterator find(const key_type& k) const
        {       
            const_iterator i(lower_bound(k));
            if (i != end() && operator()(k, i->first))
            {
                i = end();
            }
            return i;
        }

        size_type count(const key_type& k) const
        { return find(k) != end(); }

        iterator lower_bound(const key_type& k)
        {
            MyCompare& me = *this;
            return std::lower_bound(begin(), end(), k, me);
        }

        const_iterator lower_bound(const key_type& k) const
        {
            const MyCompare& me = *this;
            return std::lower_bound(begin(), end(), k, me);
        }

        iterator upper_bound(const key_type& k)
        {
            MyCompare& me = *this;
            return std::upper_bound(begin(), end(), k, me);
        }

        const_iterator upper_bound(const key_type& k) const
        {
            const MyCompare& me = *this;
            return std::upper_bound(begin(), end(), k, me);
        }

        std::pair<iterator, iterator> equal_range(const key_type& k)
        {
            MyCompare& me = *this;
            return std::equal_range(begin(), end(), k, me);
        }

        std::pair<const_iterator, const_iterator> equal_range(
            const key_type& k) const
        {
            const MyCompare& me = *this;
            return std::equal_range(begin(), end(), k, me);
        }
        
        friend bool operator==(const AssocVector& lhs, const AssocVector& rhs)
        {
            const Base& me = lhs;
            return me == rhs;
        } 

        friend bool operator<(const AssocVector& lhs, const AssocVector& rhs)
        {
            const Base& me = lhs;
            return me < rhs;
        } 

        friend bool operator!=(const AssocVector& lhs, const AssocVector& rhs)
        { return !(lhs == rhs); } 

        friend bool operator>(const AssocVector& lhs, const AssocVector& rhs)
        { return rhs < lhs; }

        friend bool operator>=(const AssocVector& lhs, const AssocVector& rhs)
        { return !(lhs < rhs); } 

        friend bool operator<=(const AssocVector& lhs, const AssocVector& rhs)
        { return !(rhs < lhs); }
    };

    // specialized algorithms:
    template <class K, class V, class C, class A>
    void swap(AssocVector<K, V, C, A>& lhs, AssocVector<K, V, C, A>& rhs)
    { lhs.swap(rhs); }
    
} // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// May 20: change operator= - credit due to Cristoph Koegl
////////////////////////////////////////////////////////////////////////////////

#endif // ASSOCVECTOR_INC_
