/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxlist.h,v 1.4 2005/03/14 19:33:48 bobclark Exp $
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
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
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

#include "hxslist.h"

#if defined(DEFINE_LIST) || defined(DEFINE_KEYED_LIST)

class LIST_NAME
{
public:

#ifdef DEFINE_KEYED_LIST

    typedef VALUE_TYPE::first_type key_type;
    typedef KEY_BEHAVIOUR key_behaviour;
    typedef VALUE_TYPE::second_type referent_type;
#endif //DEFINE_KEYED_LIST

    typedef VALUE_TYPE value_type;

    class _Iterator
    {
    public:
	_Iterator();
	_Iterator
	(
	    const CHXSimpleList* pSimpleListWrapped,
	    LISTPOSITION ListPosition
	);
	_Iterator(const _Iterator& rliocOther);
	~_Iterator();

	_Iterator& operator=
	(
	    const _Iterator& rliocOther
	);

	VALUE_TYPE& operator*();
	_Iterator& operator=(const VALUE_TYPE& rclsNewValue);

	_Iterator& operator++();
	const _Iterator operator++(int);

	_Iterator& operator--();
	const _Iterator operator--(int);
    private:
	CHXSimpleList* m_pSimpleListWrapped;
	LISTPOSITION m_ListPosition;
	friend class LIST_NAME;
	friend HXBOOL operator==
	(
	    const _Iterator& rliocLeft,
	    const _Iterator& rliocRight
	);
	friend HXBOOL operator!=
	(
	    const _Iterator& rliocLeft,
	    const _Iterator& rliocRight
	);
    };
private:
    friend class _Iterator;

public:
    typedef _Iterator iterator;
    typedef const _Iterator const_iterator;

    LIST_NAME();
    LIST_NAME(const LIST_NAME& rlocOther);
    ~LIST_NAME();

    LIST_NAME& operator=(const LIST_NAME& rlocOther);

    _Iterator begin();
    const _Iterator begin() const;
    _Iterator end();
    const _Iterator end() const;

    size_t size() const;

    _Iterator insert(_Iterator itBefore, const value_type&);
    void insert
    (
	_Iterator itBefore,
	const _Iterator itFirst,
	const _Iterator itLast
    );

    _Iterator remove(_Iterator itThis);
    _Iterator remove(_Iterator itFirst, _Iterator itLast);

    void empty();

#ifdef DEFINE_KEYED_LIST
    _Iterator insert(const value_type&);

    _Iterator remove(const key_type&);

    _Iterator find(const key_type&);
#ifndef _UNIX
    const _Iterator find(const key_type&) const;
#endif

    _Iterator find(const _Iterator, const key_type&);
#ifndef _UNIX
    const _Iterator find(const _Iterator, const key_type&) const;
#endif

#endif //DEFINE_KEYED_LIST

protected:
    CHXSimpleList m_SimpleListWrapped;

    void _copy(const LIST_NAME& rlocOther);
};

#ifdef DEFINE_KEYED_LIST

#undef DEFINE_KEYED_LIST
#undef KEY_BEHAVIOUR

#else //DEFINE_LIST

#undef DEFINE_LIST

#endif //DEFINE_KEYED_LIST

#undef VALUE_TYPE

#undef LIST_NAME

#endif //DEFINE_LIST || DEFINE_KEYED_LIST

#if defined(DEFINE_LIST_GLOBALS) || defined(DEFINE_KEYED_LIST_GLOBALS)

#ifndef LIST_SCOPE
#define LIST_FULL_SPEC LIST_NAME
#else
#define LIST_FULL_SPEC LIST_SCOPE::LIST_NAME
#endif //!LIST_SCOPE

HXBOOL operator==
(
    const LIST_FULL_SPEC::_Iterator& rliocLeft,
    const LIST_FULL_SPEC::_Iterator& rliocRight
);

HXBOOL operator!=
(
    const LIST_FULL_SPEC::_Iterator& rliocLeft,
    const LIST_FULL_SPEC::_Iterator& rliocRight
);

#undef LIST_NAME
#undef DEFINE_LIST_GLOBALS
#undef DEFINE_KEYED_LIST_GLOBALS
#undef LIST_SCOPE
#undef LIST_FULL_SPEC

#endif //DEFINE_LIST_GLOBALS || DEFINE_KEYED_LIST_GLOBALS


#if defined(IMPLEMENT_LIST) || defined(IMPLEMENT_KEYED_LIST)

#ifndef LIST_SCOPE
#define LIST_FULL_SPEC LIST_NAME
#else
#define LIST_FULL_SPEC LIST_SCOPE::LIST_NAME
#endif //!LIST_SCOPE

    LIST_FULL_SPEC::LIST_NAME()
    {
    }
    LIST_FULL_SPEC::LIST_NAME(const LIST_NAME& rlocOther)
    {
	_copy(rlocOther);
    }
    LIST_FULL_SPEC::~LIST_NAME()
    {
	empty();
    }

    LIST_NAME&
    LIST_FULL_SPEC::operator=(const LIST_NAME& rlocOther)
    {
	empty();
	_copy(rlocOther);

	return *this;
    }

    void
    LIST_FULL_SPEC::_copy(const LIST_NAME& rlocOther)
    {
    	iterator itOther;

	for
	(
	    itOther = rlocOther.begin();
	    itOther != rlocOther.end();
	    ++itOther
	)
	{
	    insert(end(), *itOther);
	}
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::begin()
    {
	return iterator(&m_SimpleListWrapped, m_SimpleListWrapped.GetHeadPosition());
    }

    const LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::begin() const
    {
	return iterator(&m_SimpleListWrapped, m_SimpleListWrapped.GetHeadPosition());
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::end()
    {
	return iterator(&m_SimpleListWrapped, NULL);
    }

    const LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::end() const
    {
	return iterator(&m_SimpleListWrapped, NULL);
    }

    size_t 
    LIST_FULL_SPEC::size() const
    {
	return m_SimpleListWrapped.GetCount();
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::insert(LIST_FULL_SPEC::iterator itBefore, const LIST_FULL_SPEC::value_type& rclsNew)
    {
	if ((&m_SimpleListWrapped) != itBefore.m_pSimpleListWrapped)
	{
	    return begin();
	}

	value_type* pNew = new value_type;
	LISTPOSITION ListPositionNew;
	LISTPOSITION ListPosition;

	HX_ASSERT(pNew);

	*pNew = rclsNew;

	if (!itBefore.m_ListPosition)
	{
	    ListPositionNew = m_SimpleListWrapped.AddTail((void*)pNew);
	}
	else
	{
	    ListPosition = itBefore.m_ListPosition;
	    ListPositionNew = m_SimpleListWrapped.InsertBefore(ListPosition, (void*)pNew);
	}
	return iterator(&m_SimpleListWrapped, ListPositionNew);
    }

    void
    LIST_FULL_SPEC::insert
    (
	LIST_FULL_SPEC::iterator itBefore,
	const LIST_FULL_SPEC::iterator itFirst,
	const LIST_FULL_SPEC::iterator itLast
    )
    {
	iterator itOther;
	LISTPOSITION ListPosition;

	if ((&m_SimpleListWrapped) != itBefore.m_pSimpleListWrapped)
	{
	    return;
	}

	for (itOther = itFirst; itOther != itLast && itOther != end(); ++itOther)
	{
	    value_type* pNew = new value_type;

	    HX_ASSERT(pNew);

	    *pNew = *((value_type*)itOther.m_pSimpleListWrapped->GetAt(itOther.m_ListPosition));

	    if (!itBefore.m_ListPosition)
	    {
		m_SimpleListWrapped.AddTail((void*)pNew);
	    }
	    else
	    {
		ListPosition = itBefore.m_ListPosition;
		m_SimpleListWrapped.InsertBefore(ListPosition, (void*)pNew);
	    }
	}
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(LIST_FULL_SPEC::iterator itThis)
    {
	if (!itThis.m_ListPosition || (&m_SimpleListWrapped) != itThis.m_pSimpleListWrapped)
	{
	    return end();
	}

	LISTPOSITION ListPositionOld;

	ListPositionOld = itThis.m_ListPosition;

	++itThis;

	delete ((value_type*)m_SimpleListWrapped.GetAt(ListPositionOld));
	m_SimpleListWrapped.RemoveAt(ListPositionOld);

	return itThis;
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(LIST_FULL_SPEC::iterator itFirst, LIST_FULL_SPEC::iterator itLast)
    {
	if (!itFirst.m_ListPosition || (&m_SimpleListWrapped) != itFirst.m_pSimpleListWrapped)
	{
	    return end();
	}

	iterator itOther;
	LISTPOSITION ListPositionOld;

	for (itOther = itFirst; itOther != itLast && itOther != end();)
	{
	    ListPositionOld = itOther.m_ListPosition;

	    ++itOther;

	    delete ((value_type*)m_SimpleListWrapped.GetAt(ListPositionOld));
	    m_SimpleListWrapped.RemoveAt(ListPositionOld);
	}

	return itOther;
    }

    void
    LIST_FULL_SPEC::empty()
    {
	remove(begin(), end());
    }

#ifdef IMPLEMENT_KEYED_LIST

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::insert(const LIST_FULL_SPEC::value_type& rclsNew)
    {
	return insert(begin(), rclsNew);
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(const LIST_FULL_SPEC::key_type& key)
    {
	iterator itNext = begin();
	while ((itNext = remove(find(itNext, key))) != end()) {};
	return itNext;
    }

    LIST_FULL_SPEC::iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::key_type& key)
    {
	return find(begin(), key);
    }

#ifndef _UNIX
    LIST_FULL_SPEC::const_iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::key_type& key) const
    {
	return find(begin(), key);
    }
#endif

    LIST_FULL_SPEC::iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::iterator itBegin, const LIST_FULL_SPEC::key_type& key)
    {
	iterator itCurrent;

	for (itCurrent = itBegin; itCurrent != end(); ++itCurrent)
	{
	    if (!key_behaviour::compare((*itCurrent).first, key))
	    {
		break;
	    }
	}
	return itCurrent;
    }

#ifndef _UNIX
    LIST_FULL_SPEC::const_iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::iterator itBegin, const LIST_FULL_SPEC::key_type& key) const
    {
	iterator itCurrent;
	for (itCurrent = itBegin; itCurrent != end(); ++itCurrent)
	{
	    if (!key_behaviour::compare((*itCurrent).first, key))
	    {
		break;
	    }
	}
	return itCurrent;
    }
#endif

#endif //IMPLEMENT_KEYED_LIST


    LIST_FULL_SPEC::_Iterator::_Iterator()
	: m_pSimpleListWrapped(NULL)
	, m_ListPosition(NULL)
    {
    }
    LIST_FULL_SPEC::_Iterator::_Iterator
    (
	const CHXSimpleList* pSimpleListWrapped,
	LISTPOSITION ListPosition
    )
	: m_pSimpleListWrapped((CHXSimpleList*)pSimpleListWrapped)
	, m_ListPosition(ListPosition)
    {
    }
    LIST_FULL_SPEC::_Iterator::_Iterator
    (
	const LIST_FULL_SPEC::_Iterator& rliocOther
    )
	: m_pSimpleListWrapped(rliocOther.m_pSimpleListWrapped)
	, m_ListPosition(rliocOther.m_ListPosition)
    {
    }
    LIST_FULL_SPEC::_Iterator::~_Iterator()
    {
    }

    LIST_FULL_SPEC::_Iterator&
    LIST_FULL_SPEC::_Iterator::operator=
    (
	const LIST_FULL_SPEC::_Iterator& rliocOther
    )
    {
	m_pSimpleListWrapped = (rliocOther.m_pSimpleListWrapped);
	m_ListPosition = (rliocOther.m_ListPosition);

	return *this;
    }

    LIST_FULL_SPEC::value_type&
    LIST_FULL_SPEC::_Iterator::operator*()
    {
	HX_ASSERT(m_pSimpleListWrapped && m_ListPosition);
	return *((LIST_FULL_SPEC::value_type*)m_pSimpleListWrapped->GetAt(m_ListPosition));
    }

    LIST_FULL_SPEC::_Iterator&
    LIST_FULL_SPEC::_Iterator::operator=(const LIST_FULL_SPEC::value_type& rclsNewValue)
    {
	if(m_pSimpleListWrapped && m_ListPosition)
	{
	    *((LIST_FULL_SPEC::value_type*)m_pSimpleListWrapped->GetAt(m_ListPosition)) = rclsNewValue;
	}
	return *this;
    }

    LIST_FULL_SPEC::_Iterator&
    LIST_FULL_SPEC::_Iterator::operator++()
    {
	if(m_pSimpleListWrapped && m_ListPosition)
	{
	    m_pSimpleListWrapped->GetNext(m_ListPosition);
	}
	return *this;
    }

    const LIST_FULL_SPEC::_Iterator
    LIST_FULL_SPEC::_Iterator::operator++(int)
    {
	_Iterator liocRet(*this);

	++(*this);

	return liocRet;
    }

    LIST_FULL_SPEC::_Iterator&
    LIST_FULL_SPEC::_Iterator::operator--()
    {
	if(m_pSimpleListWrapped)
	{
	    if (!m_ListPosition)
	    {
		m_ListPosition = m_pSimpleListWrapped->GetTailPosition();
	    }
	    else
	    {
		m_pSimpleListWrapped->GetPrev(m_ListPosition);
		if (!m_ListPosition)
		{
		    m_ListPosition = m_pSimpleListWrapped->GetHeadPosition();
		}
	    }
	}
	return *this;
    }

    const LIST_FULL_SPEC::_Iterator
    LIST_FULL_SPEC::_Iterator::operator--(int)
    {
	LIST_FULL_SPEC::_Iterator liocRet(*this);

	--(*this);

	return liocRet;
    }

    HXBOOL operator==
    (
	const LIST_FULL_SPEC::_Iterator& rliocLeft,
	const LIST_FULL_SPEC::_Iterator& rliocRight
    )
    {
	return (rliocLeft.m_pSimpleListWrapped == rliocRight.m_pSimpleListWrapped && rliocLeft.m_ListPosition == rliocRight.m_ListPosition);
    }
    HXBOOL operator!=
    (
	const LIST_FULL_SPEC::_Iterator& rliocLeft,
	const LIST_FULL_SPEC::_Iterator& rliocRight
    )
    {
	return (rliocLeft.m_pSimpleListWrapped != rliocRight.m_pSimpleListWrapped || rliocLeft.m_ListPosition != rliocRight.m_ListPosition);
    }

#undef IMPLEMENT_KEYED_LIST
#undef IMPLEMENT_LIST
#undef LIST_NAME
#undef LIST_SCOPE
#undef LIST_FULL_SPEC

#endif //IMPLEMENT_LIST

#ifdef DEFINE_UNIQUELY_KEYED_LIST
//#define KEYED_LIST
//#define LOOKUP_MAP
//#define LIST_NAME

class LIST_NAME : public KEYED_LIST
{
public:

    typedef KEYED_LIST::key_type key_type;
    typedef KEYED_LIST::key_behaviour key_behaviour;
    typedef KEYED_LIST::referent_type referent_type;
    typedef KEYED_LIST::value_type value_type;
    typedef KEYED_LIST::iterator iterator;
    typedef KEYED_LIST::const_iterator const_iterator;

    typedef KEYED_LIST base_list;
    typedef LOOKUP_MAP lookup_map;
public:
    LIST_NAME();
    LIST_NAME(const LIST_NAME& rlocOther);
    ~LIST_NAME();

    LIST_NAME& operator=(const LIST_NAME& rlocOther);

    iterator insert(iterator itBefore, const value_type&);
    void insert
    (
	iterator itBefore,
	const iterator itFirst,
	const iterator itLast
    );
    iterator insert(const value_type&);

    iterator remove(iterator itThis);
    iterator remove(iterator itFirst, iterator itLast);
    iterator remove(const key_type&);

    void empty();

    KEYED_LIST::_Iterator find(const key_type&);
#ifndef _UNIX
    const KEYED_LIST::_Iterator find(const key_type&) const;
#endif

private:
    KEYED_LIST::_Iterator find(const KEYED_LIST::_Iterator, const key_type&);
#ifndef _UNIX
    const KEYED_LIST::_Iterator find(const KEYED_LIST::_Iterator, const key_type&) const;
#endif

private:
    LOOKUP_MAP m_LookupMap;

    void _copy(const LIST_NAME& rlocOther);
};

#undef KEYED_LIST
#undef LIST_NAME
#undef LOOKUP_MAP
#undef DEFINE_UNIQUELY_KEYED_LIST

#endif //DEFINE_UNIQUELY_KEYED_LIST


#ifdef DEFINE_UNIQUELY_KEYED_LIST_GLOBALS

#ifndef LIST_SCOPE
#define LIST_FULL_SPEC LIST_NAME
#else
#define LIST_FULL_SPEC LIST_SCOPE::LIST_NAME
#endif //!LIST_SCOPE


#undef LIST_NAME
#undef DEFINE_UNIQUELY_KEYED_LIST_GLOBALS
#undef LIST_SCOPE
#undef LIST_FULL_SPEC

#endif //DEFINE_UNIQUELY_KEYED_LIST_GLOBALS


#ifdef IMPLEMENT_UNIQUELY_KEYED_LIST

#ifndef LIST_SCOPE
#define LIST_FULL_SPEC LIST_NAME
#else
#define LIST_FULL_SPEC LIST_SCOPE::LIST_NAME
#endif //!LIST_SCOPE

    LIST_FULL_SPEC::LIST_NAME()
    {
    }
    LIST_FULL_SPEC::LIST_NAME(const LIST_FULL_SPEC& rlocOther)
    {
	_copy(rlocOther);
    }
    LIST_FULL_SPEC::~LIST_NAME()
    {
	empty();
    }

    LIST_FULL_SPEC&
    LIST_FULL_SPEC::operator=(const LIST_FULL_SPEC& rlocOther)
    {
	empty();
	_copy(rlocOther);

	return *this;
    }

    void
    LIST_FULL_SPEC::_copy(const LIST_FULL_SPEC& rlocOther)
    {
    	iterator itOther;

	for
	(
	    itOther = rlocOther.begin();
	    itOther != rlocOther.end();
	    ++itOther
	)
	{
	    insert(end(), *itOther);
	}
    }


    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::insert(LIST_FULL_SPEC::iterator itBefore, const LIST_FULL_SPEC::value_type& rvalue)
    {
	lookup_map::iterator itValue;
	iterator itNew;

	itValue = m_LookupMap.find(rvalue.first);
	if (itValue == m_LookupMap.end())
	{
	    //New insert

	    // insert into ordered list
	    itNew = base_list::insert(itBefore, rvalue);

	    // insert into map
	    m_LookupMap.insert(lookup_map::value_type(rvalue.first, itNew));
	}
	else
	{
	    // dup insert, replace previous with this.

	    // remove existing from ordered list
	    base_list::remove((*itValue).second);

	    // insert new into ordered list
	    itNew = base_list::insert(itBefore, rvalue);

	    // replace old iterator in the map
	    itValue = lookup_map::value_type(rvalue.first, itNew);
	}
	return itNew;
    }

    void
    LIST_FULL_SPEC::insert
    (
	LIST_FULL_SPEC::iterator itBefore,
	const LIST_FULL_SPEC::iterator itFirst,
	const LIST_FULL_SPEC::iterator itLast
    )
    {
	iterator itOther;

	for (itOther = itFirst; itOther != itLast && itOther != end(); ++itOther)
	{
	    insert(itBefore, *itOther);
	}
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::insert(const LIST_FULL_SPEC::value_type& rclsNew)
    {
	return insert(begin(), rclsNew);
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(LIST_FULL_SPEC::iterator itThis)
    {
	// remove from map
	m_LookupMap.remove((*itThis).first);

	// remove from ordered list
	return base_list::remove(itThis);
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(LIST_FULL_SPEC::iterator itFirst, LIST_FULL_SPEC::iterator itLast)
    {
	iterator itOther;

	for (itOther = itFirst; itOther != itLast && itOther != end();)
	{
	    itOther = remove(itOther);
	}

	return itOther;
    }

    LIST_FULL_SPEC::iterator
    LIST_FULL_SPEC::remove(const LIST_FULL_SPEC::key_type& key)
    {
	iterator itNext;
	lookup_map::iterator itValue;

	itValue = m_LookupMap.find(key);
	if (itValue != m_LookupMap.end())
	{
	    // Save iterator
	    itNext = (*itValue).second;

	    // remove from map
	    m_LookupMap.remove((*itValue).first);

	    // remove from ordered list
	    itNext = remove(itNext);
	}
	else
	{
	    itNext = end();
	}

	return itNext;
    }

    void
    LIST_FULL_SPEC::empty()
    {
	remove(begin(), end());
    }

    LIST_FULL_SPEC::iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::key_type& key)
    {
	iterator itFound;
	lookup_map::iterator itValue;

	itValue = m_LookupMap.find(key);
	if (itValue != m_LookupMap.end())
	{
	    itFound = (*itValue).second;
	}
	else
	{
	    itFound = end();
	}

	return itFound;
    }

#ifndef _UNIX
    LIST_FULL_SPEC::const_iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::key_type& key) const
    {
	iterator itFound;
	lookup_map::iterator itValue;

	itValue = m_LookupMap.find(key);
	if (itValue != m_LookupMap.end())
	{
	    itFound = (*itValue).second;
	}
	else
	{
	    itFound = end();
	}

	return itFound;
    }
#endif

    LIST_FULL_SPEC::iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::iterator , const LIST_FULL_SPEC::key_type& )
    {
	return end();
    }

#ifndef _UNIX
    LIST_FULL_SPEC::const_iterator 
    LIST_FULL_SPEC::find(const LIST_FULL_SPEC::iterator , const LIST_FULL_SPEC::key_type& ) const
    {
	return end();
    }
#endif

#undef LIST_NAME
#undef IMPLEMENT_UNIQUELY_KEYED_LIST
#undef LIST_SCOPE
#undef LIST_FULL_SPEC

#endif //IMPLEMENT_UNIQUELY_KEYED_LIST
