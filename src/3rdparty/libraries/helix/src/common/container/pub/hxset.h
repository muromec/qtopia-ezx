/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxset.h,v 1.7 2007/07/06 20:35:02 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _HXSET_H
#define _HXSET_H

#include "hxmap.h"

class CHXSet
{
public:

	CHXSet(void);
	~CHXSet(void);

// Attributes
	// number of elements
	int GetCount() const;
	HXBOOL IsEmpty() const;

	// member management
	void Add(void* pMember);	
	HXBOOL Remove(void* pMember);
	void RemoveAll();

	HXBOOL Lookup(void *pMember);

	// iteration methods/object
	POSITION GetStartPosition() const;
	void *GetNext(POSITION& rNextPosition) const;

	class Iterator
	{
	public:
		friend class CHXSet;

		// standard iteration methods
		Iterator();
		Iterator&	operator++();
		HXBOOL		operator==(const Iterator& iter) const;
		HXBOOL		operator!=(const Iterator& iter) const;
		void*		operator*();

	private:
		Iterator(CHXSet* pSet, POSITION node);

		// iteration state members
		POSITION	m_Node;
		POSITION	m_NextNode;
		void*		m_pValue;
		CHXSet*		m_pSet;
	};

	// begin the iteration
	Iterator	Begin();
	Iterator	End();


private:

	CHXMapPtrToPtr m_Map;		// the map we currently use
};


inline CHXSet::CHXSet(void) : m_Map()
{
}

inline CHXSet::~CHXSet(void)
{
}

inline int CHXSet::GetCount(void) const
{
	return(m_Map.GetCount());
}

inline HXBOOL CHXSet::IsEmpty(void) const
{
	return(m_Map.IsEmpty()); 
}

inline void CHXSet::Add(void *pMember)
{
	m_Map.SetAt(pMember, NULL);
}

inline HXBOOL CHXSet::Remove(void *pMember)
{
	return(m_Map.RemoveKey(pMember));
}

inline HXBOOL CHXSet::Lookup(void *pMember)
{
    return m_Map.Lookup(pMember) != NULL;
}

inline void CHXSet::RemoveAll(void)
{
	m_Map.RemoveAll();
}

inline POSITION CHXSet::GetStartPosition(void) const
{
	return(m_Map.GetStartPosition()); 
}

inline void* CHXSet::GetNext(POSITION &rNextPosition) const
{
	void *pMember = NULL;
	void *pTemp;
	m_Map.GetNextAssoc(rNextPosition, pMember, pTemp);
	return(pMember);
}

inline CHXSet::Iterator::Iterator(CHXSet* pSet, POSITION node)
    : 	m_Node(node),
    	m_NextNode(node),
    	m_pValue(NULL),
    	m_pSet(pSet)
{
    if (m_Node)
	m_pValue = m_pSet->GetNext(m_NextNode);
}

inline HXBOOL CHXSet::Iterator::operator==(const CHXSet::Iterator& iter) const
{
    return (m_Node == iter.m_Node);
}

inline HXBOOL CHXSet::Iterator::operator!=(const CHXSet::Iterator& iter) const
{
    return !(*this == iter);
}

inline void* CHXSet::Iterator::operator*()
{
    return(m_pValue);
}

inline
CHXSet::Iterator& CHXSet::Iterator::operator++()
{
    m_Node = m_NextNode;
    if (m_Node)
	m_pValue = m_pSet->GetNext(m_NextNode);
    return *this;
}

inline CHXSet::Iterator CHXSet::Begin()
{
    return Iterator(this, GetStartPosition());
}

inline CHXSet::Iterator CHXSet::End()
{
    return Iterator(this, 0);
}


#endif		// _HXSET_H

