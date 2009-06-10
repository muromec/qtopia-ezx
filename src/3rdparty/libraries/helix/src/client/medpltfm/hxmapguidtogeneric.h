/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmapguidtogeneric.h,v 1.3 2007/07/06 21:58:19 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _RNMAPGUIDTOGENERIC_H_
#define _RNMAPGUIDTOGENERIC_H_

#include "hxmap.h"
#include "hxcom.h"

#define BEFORE_START_POSITION ((void *)-1L)

#define HX_GUID_TO_GENERIC_MAP_INLINE(MapName_,GenericType_) \
class MapName_ \
{ \
protected: \
    struct CAssoc \
    { \
	CAssoc* pNext; \
	ULONG32 nHashValue;   \
	GUID  	key; \
	GenericType_   value; \
    }; \
public: \
 \
    class Iterator \
    { \
    public: \
	friend class MapName_; \
 \
	Iterator() : map(0), node(0), next_node(0), key(0), value (0) {} \
 \
	Iterator&   operator++() \
	{ \
	    node = next_node; \
	    if( node ) \
	    { \
		value = map->GetNextAssoc(next_node, key); \
	    } \
	    return *this; \
	} \
 \
	HXBOOL        operator==(const Iterator& iter) const { return(node == iter.node); } \
	HXBOOL        operator!=(const Iterator& iter) const { return !(*this == iter); } \
	GenericType_& operator*() { return *value; } \
	const GUID* get_key() { return key; } \
 \
    private: \
	Iterator(MapName_* _map, POSITION _node) \
	    : node(_node), next_node(_node), map(_map), key(0), value (0) \
	{ \
	    if( node ) \
	    { \
		value = map->GetNextAssoc(next_node, key); \
	    } \
	} \
 \
 \
	MapName_*	map; \
	POSITION    		node; \
	POSITION    		next_node; \
	 \
	GUID*       		key; \
	GenericType_*  		value; \
    }; \
 \
    explicit MapName_ ()  \
	: m_pHashTable (NULL), m_nHashTableSize (17), m_nCount (0) {} \
 \
    int GetCount() const { return m_nCount; } \
    HXBOOL IsEmpty() const { return m_nCount == 0; } \
 \
    HXBOOL Lookup(const GUID& key, GenericType_& rValue) const \
    { \
	HX_ASSERT_VALID_PTR(this); \
 \
	if( IsEmpty() ) \
	    return FALSE; \
 \
	ULONG32 nHash; \
	CAssoc* pAssoc = GetAssocAt(key, nHash); \
	if( pAssoc == NULL ) return FALSE;   \
 \
	rValue = pAssoc->value; \
	return TRUE; \
    } \
 \
    GenericType_& operator[](const GUID& key) \
    { \
	HX_ASSERT_VALID_PTR(this); \
 \
	ULONG32 nHash; \
	CAssoc* pAssoc; \
	if( (pAssoc = GetAssocAt(key, nHash)) == NULL ) \
	{ \
	    if( m_pHashTable == NULL ) InitHashTable(m_nHashTableSize); \
 \
	    pAssoc = new CAssoc; \
	    ++m_nCount; \
	    pAssoc->nHashValue = nHash; \
	    SetGUID( pAssoc->key, key ); \
	    pAssoc->pNext = m_pHashTable[nHash]; \
	    m_pHashTable[nHash] = pAssoc; \
	} \
 \
	return pAssoc->value;   \
    } \
 \
 \
    void SetAt(const GUID& key, GenericType_ const& newValue) { (*this)[key] = newValue; } \
 \
    HXBOOL RemoveKey(const GUID& key) \
    { \
	HX_ASSERT_VALID_PTR(this); \
 \
	if( m_pHashTable == NULL ) return FALSE; \
 \
	CAssoc** ppAssocPrev; \
	ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize]; \
 \
	CAssoc* pAssoc; \
	for( pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext ) \
	{ \
	    if( IsEqualGUID( pAssoc->key, key ) ) \
	    { \
		*ppAssocPrev = pAssoc->pNext;   \
		--m_nCount; \
		delete pAssoc; \
		pAssoc = NULL; \
		return TRUE; \
	    } \
	    ppAssocPrev = &pAssoc->pNext; \
	} \
	return FALSE;   \
    } \
 \
    void RemoveAll() \
    { \
	HX_ASSERT_VALID_PTR(this); \
 \
	if( m_pHashTable ) \
	{ \
	    for (ULONG32 i = 0; i < m_nHashTableSize; ++i) delete m_pHashTable [i]; \
	    delete[] m_pHashTable; \
	    m_pHashTable = NULL; \
	} \
 \
	m_nCount = 0; \
    } \
 \
 \
    POSITION GetStartPosition() const { return(m_nCount == 0) ? NULL : BEFORE_START_POSITION; } \
 \
    GenericType_* GetNextAssoc(POSITION& rNextPosition, GUID*& rKey) const \
    { \
	HX_ASSERT_VALID_PTR(this); \
	HX_ASSERT(m_pHashTable != NULL);  \
 \
	CAssoc* pAssocRet = (CAssoc*)rNextPosition; \
	HX_ASSERT(pAssocRet != NULL); \
 \
	if( pAssocRet == (CAssoc*) BEFORE_START_POSITION ) \
	{ \
	    for( ULONG32 nBucket = 0; nBucket < m_nHashTableSize; nBucket++ ) \
	    { \
		if( (pAssocRet = m_pHashTable[nBucket]) != NULL ) \
		    break; \
	    } \
	    HX_ASSERT(pAssocRet != NULL); \
	} \
 \
	HX_ASSERT(HXIsValidAddress(pAssocRet, sizeof(CAssoc))); \
 \
	CAssoc* pAssocNext; \
	if( (pAssocNext = pAssocRet->pNext) == NULL ) \
	{ \
	    for( ULONG32 nBucket = pAssocRet->nHashValue + 1; \
	       nBucket < m_nHashTableSize; nBucket++ ) \
		if( (pAssocNext = m_pHashTable[nBucket]) != NULL ) \
		    break; \
	} \
 \
	rNextPosition = (POSITION) pAssocNext; \
 \
	rKey = &pAssocRet->key; \
	return &pAssocRet->value; \
    } \
 \
    Iterator Begin() { return Iterator(this, GetStartPosition()); } \
    Iterator End() { return Iterator(this, 0); } \
 \
    ULONG32 GetHashTableSize() const { return m_nHashTableSize; } \
    void InitHashTable(ULONG32 nHashSize, HXBOOL bAllocNow = TRUE ) \
    { \
	HX_ASSERT_VALID_PTR(this); \
	HX_ASSERT(m_nCount == 0); \
	HX_ASSERT(nHashSize > 0); \
 \
	if( m_pHashTable ) \
	{ \
	    delete[] m_pHashTable; \
	    m_pHashTable = NULL; \
	} \
 \
	m_nHashTableSize = nHashSize; \
 \
	if( bAllocNow ) \
	{ \
	    m_pHashTable = new CAssoc* [nHashSize]; \
	    memset(m_pHashTable, 0,(size_t)(sizeof(CAssoc*) * nHashSize)); \
	} \
    } \
 \
 \
    ULONG32 HashKey(const GUID& key) const { return key.Data1; } \
 \
protected: \
    CAssoc** m_pHashTable; \
    ULONG32 m_nHashTableSize; \
    int m_nCount; \
 \
    CAssoc* GetAssocAt(const GUID& key, ULONG32& nHash) const \
    { \
	nHash = HashKey(key) % m_nHashTableSize; \
 \
	if( m_pHashTable == NULL ) return NULL; \
 \
	CAssoc* pAssoc; \
	for( pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext ) \
	{ \
	    if( IsEqualGUID( pAssoc->key, key ) ) \
		return pAssoc; \
	} \
	return NULL; \
    } \
 \
public: \
    ~MapName_() \
    { \
	RemoveAll(); \
	HX_ASSERT(m_nCount == 0); \
    } \
}

#endif
