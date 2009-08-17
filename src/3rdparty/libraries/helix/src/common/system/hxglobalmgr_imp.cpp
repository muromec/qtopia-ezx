/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxglobalmgr_imp.cpp,v 1.5 2006/12/03 23:07:44 ehyche Exp $
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

#include "hxglobalmgr_imp.h"
#include "hxassert.h"
#include "hxthread.h"

class HXGlobalMap
{
public:
    HXGlobalMap();
    ~HXGlobalMap();
    
    void       Add(const int* pContext, GlobalID id, GlobalType pObj,
		   HXGlobalDestroyFunc fpDestroy);
    void       Remove(GlobalID id);
    void       RemoveAll(const int* pContext);

    GlobalPtr  Get(GlobalID id) const;
    HXBOOL       IsMember(GlobalID id) const;

private: 
    class MapNode
    {
    public:
	MapNode();
	MapNode(const int* pContext, GlobalID id, GlobalType pObj, 
		HXGlobalDestroyFunc fpDestroy);
	~MapNode();
	
	MapNode*   GetNext() const { return m_pNext; }
	void       SetNext(MapNode* pNext) { m_pNext = pNext; }
	const int* Context() const { return m_pContext;}
        GlobalID   ID() const { return m_id; }
	GlobalPtr  ObjPtr() { return &m_pObj; }

    private:
	MapNode*            m_pNext;
	const int*          m_pContext;
	GlobalID            m_id;
	GlobalType          m_pObj;
	HXGlobalDestroyFunc m_fpDestroy;
    };

    unsigned int GetBucket(GlobalID id) const;
    void     ResizeIfNeeded();
    MapNode* FindNode(GlobalID id, MapNode** ppLastNode) const;
    void     AddNode(MapNode* pNewNode);

    int      m_bucketIndex;
    int      m_bucketCount;
    MapNode* m_pBuckets;
    int      m_elementCount;

    static const int zm_bucketCounts[];
};


// List of primes that are approximately double the
// previous prime
const int HXGlobalMap::zm_bucketCounts[] =
{
    3, 5, 11, 23, 53, 97, 193, 389, 
    769, 1543, 3079, 6151, 12289, 
    24593, 49157, 98317, 196613, 
    393241, 786433, 1572869, 
    3145739, 6291469, 12582917
};


HXGlobalMap::HXGlobalMap()
    : m_bucketIndex(0)
    , m_bucketCount(0)
    , m_pBuckets(0)
    , m_elementCount(0)
{
    m_bucketCount = zm_bucketCounts[m_bucketIndex];
    m_pBuckets = new MapNode[m_bucketCount];
    HX_ASSERT(m_pBuckets != NULL );
}


HXGlobalMap::~HXGlobalMap()
{
    for (int i = 0; i < m_bucketCount; i++)
    {
	MapNode* pCur = m_pBuckets[i].GetNext();

	while (pCur)
	{
	    MapNode* pNext = pCur->GetNext();

	    delete pCur;

	    pCur = pNext;
	}
    }

    delete [] m_pBuckets;
    m_pBuckets = 0;
    m_bucketIndex = 0;
    m_elementCount = 0;
}


void
HXGlobalMap::Add(const int* pContext, GlobalID id, GlobalType pObj, 
		 HXGlobalDestroyFunc fpDestroy)
{
    AddNode(new MapNode(pContext, id, pObj, fpDestroy));
    m_elementCount++;
    ResizeIfNeeded();
}


void
HXGlobalMap::Remove(GlobalID id)
{
    MapNode* pLastNode = 0;
    MapNode* pCurNode = FindNode(id, &pLastNode);

    if (pCurNode)
    {
	pLastNode->SetNext(pCurNode->GetNext());
	delete pCurNode;
	m_elementCount--;

	ResizeIfNeeded();
    }
}


GlobalPtr
HXGlobalMap::Get(GlobalID id) const
{
    MapNode* pNode = FindNode(id, 0);
    if (pNode)
    {
	return pNode->ObjPtr();
    }

    return NULL;
}


HXBOOL
HXGlobalMap::IsMember(GlobalID id) const
{
    return (FindNode(id, 0) != 0);
}

void
HXGlobalMap::RemoveAll(const int* pContext)
{
    for (int i = 0; i < m_bucketCount; i++)
    {
	MapNode* pLast = &m_pBuckets[i];
	MapNode* pCur = pLast->GetNext();
	
	while (pCur)
	{
	    if (pCur->Context() == pContext)
	    {
		MapNode* pNext = pCur->GetNext();

		delete pCur;
		m_elementCount--;

		pLast->SetNext(pNext);
		pCur = pNext;
	    }
	    else
	    {
		pLast = pCur;
		pCur = pCur->GetNext();
	    }
	}
    }

    ResizeIfNeeded();
}


unsigned int HXGlobalMap::GetBucket(GlobalID id) const
{
    unsigned int unRetVal = ((unsigned int)id) % m_bucketCount;
    HX_ASSERT( unRetVal < m_bucketCount && unRetVal>=0 );
    return unRetVal;
}


void
HXGlobalMap::ResizeIfNeeded()
{
    MapNode* pOldBuckets = 0;
    int oldBucketCount = m_bucketCount;

    if ((m_elementCount > (m_bucketCount << 1)) &&
	(m_bucketIndex < ((int)(sizeof(zm_bucketCounts) / sizeof(int)))))
    {

        // Grow the map if the number of elements is
        // larger than 2 times the bucket count
       	
        m_bucketIndex++;
        m_bucketCount = zm_bucketCounts[m_bucketIndex];
        
        pOldBuckets = m_pBuckets; // Save old map
        m_pBuckets = new MapNode[m_bucketCount];
    }
    else if ((m_elementCount < (m_bucketCount >> 2)) &&
	     (m_bucketIndex > 0))
    {
        
        // Shrink the map if the number of elements is
        // smaller than 1/4th the bucket count
        
        pOldBuckets = m_pBuckets; // Save old map
        
        m_bucketIndex--;
        
        m_bucketCount = zm_bucketCounts[m_bucketIndex];
        m_pBuckets = new MapNode[m_bucketCount];
    }
    
    if (pOldBuckets)
    {
        // Transfer the nodes from the old buckets to
        // the new buckets
        
        for (int i = 0; i < oldBucketCount; i++)
        {
            MapNode* pCurNode = pOldBuckets[i].GetNext();
	    
            while(pCurNode)
            {
                // Get the pointer to the next node before
                // we add the node since the AddNode() will
                // clobber it's next pointer
                MapNode* pNextNode = pCurNode->GetNext();
                
                AddNode(pCurNode);
                
                // Move onto the next node
                pCurNode = pNextNode;
            }
        }
        
        delete [] pOldBuckets;
    }
}


HXGlobalMap::MapNode*
HXGlobalMap::FindNode(GlobalID id, MapNode** ppLastNode) const
{
    MapNode* pRet = 0;

    int bucket = GetBucket(id);
    MapNode* pLastNode = &(m_pBuckets[bucket]);
    MapNode* pCurNode = pLastNode->GetNext();

    while(!pRet && pCurNode)
    {
	if (pCurNode->ID() == id)
	    pRet = pCurNode;
	else
	{
	    pLastNode = pCurNode;
	    pCurNode = pCurNode->GetNext();
	}
    }

    if (ppLastNode)
	*ppLastNode = pLastNode;

    return pRet;
}


void
HXGlobalMap::AddNode(MapNode* pNewNode)
{
    int bucket = GetBucket(pNewNode->ID());
    pNewNode->SetNext(m_pBuckets[bucket].GetNext());
    m_pBuckets[bucket].SetNext(pNewNode);
}


HXGlobalMap::MapNode::MapNode()
    : m_pNext(NULL)
    , m_pContext(NULL)
    , m_id(NULL)
    , m_pObj(NULL)
    , m_fpDestroy(NULL)
{
}


HXGlobalMap::MapNode::MapNode(const int* pContext,
			      GlobalID id,
			      GlobalType pObj, 
			      HXGlobalDestroyFunc fpDestroy)
    : m_pNext(NULL)
    , m_pContext(pContext)
    , m_id(id)
    , m_pObj(pObj)
    , m_fpDestroy(fpDestroy)
{
}

HXGlobalMap::MapNode::~MapNode()
{
    if (m_pObj && m_fpDestroy)
    {
	m_fpDestroy(m_pObj);
    }
}

HXGlobalManagerImp::HXGlobalManagerImp()
    : m_pMap(NULL)
    , m_pMutex(NULL)
{
    m_pMap = new HXGlobalMap();
    HX_ASSERT(m_pMap);
    
#if defined(THREADS_SUPPORTED) || defined(_UNIX_THREADS_SUPPORTED)
    HXMutex::MakeMutex(m_pMutex);
#else
    HXMutex::MakeStubMutex(m_pMutex);
#endif
}


HXGlobalManagerImp::~HXGlobalManagerImp()
{
    HX_DELETE(m_pMutex);
    HX_DELETE(m_pMap);
}

GlobalPtr
HXGlobalManagerImp::DoAdd(const int* pContext,
		       GlobalID id, 
		       GlobalType pObj,
		       HXGlobalDestroyFunc fpDestroy)
			      
{
    m_pMutex->Lock();

    if (!m_pMap->IsMember(id))
    {
       m_pMap->Add(pContext, id, pObj, fpDestroy);
    }
    
    GlobalPtr ptr = m_pMap->Get(id);

    m_pMutex->Unlock();

    return ptr;
}


void
HXGlobalManagerImp::DoRemove(GlobalID id)
{
    m_pMutex->Lock();

    m_pMap->Remove(id);

    m_pMutex->Unlock();
}


GlobalPtr
HXGlobalManagerImp::DoGet(GlobalID id) const
{
    m_pMutex->Lock();

    GlobalPtr ptr = m_pMap->Get(id);

    m_pMutex->Unlock();

    return ptr;
}

void
HXGlobalManagerImp::DoShutdown(const int* pContext)
{
    m_pMutex->Lock();

    m_pMap->RemoveAll(pContext);
    
    m_pMutex->Unlock();
}
