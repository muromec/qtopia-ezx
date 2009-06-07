/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: medblock.cpp,v 1.9 2006/02/16 23:07:04 ping Exp $
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

#include "pckunpck.h"
#include "medblock.h"
#include "hlxclib/string.h"

CMediumBlockAllocator::CMediumBlockAllocator(IUnknown* pContext)
    :   m_nMinBucketSize(400)
    ,	m_nNumberBuckets(256)
    ,	m_nBucketSize(128)
    ,	m_ppBuckets(0)
    ,	m_pTimeStampArray(0)
    ,	m_pCountArray(0)
    ,	m_nLastTimeStamp(0)
    ,	m_nCurrentTimeStamp(0)
    ,	m_nLastRelaxTime(0)
    ,	m_nLastMinimizeTime(0)
    ,	m_nHeaderSize(0)
    ,	m_nRelaxTime(5000)
    ,	m_nCallbackTime(500)
    ,	m_nGarbageTime(10000)
    ,	m_nGarbageDivider(10)
    ,	m_lRefCount(0)
    ,	m_hCallback(0)
    ,	m_pScheduler(0)
    ,	m_pMutex(NULL)
    ,   m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);	
    
    m_hCallback = 0;

    m_ppBuckets		= new CMemoryNode*[m_nNumberBuckets];
	
    m_pTimeStampArray	= new UINT32[m_nNumberBuckets];
    m_pTimeStampArray	= new UINT32[m_nNumberBuckets];
    m_pCountArray	= new UINT32[m_nNumberBuckets];

    if (m_ppBuckets)
    {
	for(int i = 0; i< (int)m_nNumberBuckets; i++)
	{
	    m_ppBuckets[i] = 0;
	    m_pCountArray[i] = 0;
	}
    }
    
    m_nCurrentTimeStamp = HX_GET_TICKCOUNT();
    m_nLastRelaxTime = m_nCurrentTimeStamp;
    m_nLastMinimizeTime = m_nCurrentTimeStamp;


    // round to the nearest dword size. 
    // I think this is the correct alignment for all platforms.
    m_nHeaderSize = (sizeof(CMemoryNode) + sizeof(double) - 1) & ~(sizeof(double) - 1);
}

CMediumBlockAllocator::~CMediumBlockAllocator()
{
    HX_RELEASE(m_pScheduler);
    HeapMinimize();
    HX_VECTOR_DELETE(m_ppBuckets);
    HX_VECTOR_DELETE(m_pTimeStampArray);
    HX_VECTOR_DELETE(m_pCountArray);
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

void CMediumBlockAllocator::SetScheduler(IUnknown* pUnk)
{
    if (m_pScheduler && m_hCallback)
    {	
	m_pScheduler->Remove(m_hCallback);
	m_hCallback = 0;
    }
    HX_RELEASE(m_pScheduler);
    if (pUnk)
    {
	pUnk->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
	if (m_pScheduler)
	{
	    m_hCallback = m_pScheduler->RelativeEnter((IHXCallback*)this, m_nCallbackTime);
	}
    }
}

/****************************************************************************
*	Method:
*		IUnknown::QueryInterface
*	Purpose:
*		Implement this to export the interfaces supported by your 
*		object.
*
****************************************************************************/

STDMETHODIMP CMediumBlockAllocator::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IMalloc*)this },
            { GET_IIDHANDLE(IID_IMalloc), (IMalloc*)this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/****************************************************************************
*	Method:
*		IUnknown::AddRef
*	Purpose:
*		Everyone usually implements this the same... feel free to use
*		this implementation.
*
****************************************************************************/

STDMETHODIMP_(ULONG32) CMediumBlockAllocator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*	Method:
*		IUnknown::Release
*	Purpose:
*		Everyone usually implements this the same... feel free to use
*		this implementation.
*
****************************************************************************/

STDMETHODIMP_(ULONG32) CMediumBlockAllocator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/****************************************************************************
*	Method:
*		IMalloc::Alloc
*	Purpose:
*		If there is a block of memory within the current pools
*		to satistify then it will be given, if there is no such 
*		object then we will ask the default memory allocator for 
*		it. 
*
****************************************************************************/

STDMETHODIMP_(void*) CMediumBlockAllocator::Alloc(UINT32 ulLength)
{
    m_pMutex->Lock();
    void*	    returnValue = NULL;
    CMemoryNode*    pMemNode = NULL;

    // figure out what bucket this belongs in

    UINT32 bucketIndex = ulLength/m_nBucketSize;

    if (ulLength> m_nMinBucketSize && bucketIndex < m_nNumberBuckets)
    {
	if (m_ppBuckets[bucketIndex])
	{
	    pMemNode = m_ppBuckets[bucketIndex];
	    m_ppBuckets[bucketIndex] = pMemNode->m_pNext;
	    m_pCountArray[bucketIndex] = m_pCountArray[bucketIndex] -1;
	    m_pTimeStampArray[bucketIndex] = m_nCurrentTimeStamp;
	    returnValue = (void*) (((char*) (pMemNode)) + m_nHeaderSize);
	}
	if (!returnValue)
	{
	    pMemNode = (CMemoryNode*) new UCHAR[((bucketIndex+1)*m_nBucketSize + m_nHeaderSize - 1)];
	    if (pMemNode)
	    {
		pMemNode->m_nSize = (bucketIndex+1)* m_nBucketSize - 1;
		pMemNode->m_pNext = NULL;
		returnValue = (void*) ((char*)pMemNode + m_nHeaderSize);
	    }
	}
    }
    else
    {
	pMemNode = (CMemoryNode*) new UCHAR[(ulLength+ m_nHeaderSize)];
	if (pMemNode)
	{
	    pMemNode->m_nSize = ulLength;
	    pMemNode->m_pNext = NULL;
	    returnValue = (void*) ((char*)pMemNode + m_nHeaderSize);
	}
    }
    m_pMutex->Unlock();
    return returnValue;
}

/****************************************************************************
*	Method:
*		IMalloc::Free
*	Purpose:
*		Returns a given block of memory to the memory buckets 
*
****************************************************************************/

STDMETHODIMP_(void) CMediumBlockAllocator::Free(void* pMem)
{
    m_pMutex->Lock();
    CMemoryNode* pMemNode   = (CMemoryNode*) (((char*)pMem) - m_nHeaderSize);

    UINT32 bucketIndex = (pMemNode->m_nSize)/m_nBucketSize;

    if (pMemNode->m_nSize > m_nMinBucketSize && bucketIndex < m_nNumberBuckets)
    {
	m_pTimeStampArray[bucketIndex] = m_nCurrentTimeStamp;

	CMemoryNode* pBufferHead = m_ppBuckets[bucketIndex];
	
	m_pCountArray[bucketIndex] = m_pCountArray[bucketIndex]+1;
	if (pBufferHead)
	{
	    m_ppBuckets[bucketIndex] = pMemNode;
	    pMemNode->m_pNext = pBufferHead;
	}
	else
	{
	    m_ppBuckets[bucketIndex] = pMemNode;
	    pMemNode->m_pNext = NULL;
	}
    }
    else
    {
	HX_DELETE(pMemNode);
    }
    m_pMutex->Unlock();
}


/****************************************************************************
*	Method:
*		IMalloc::Realloc
*	Purpose:
*		Incrediably stupid. Creates a new block and memcopies it.
*
****************************************************************************/

STDMETHODIMP_(void*) CMediumBlockAllocator::Realloc(void* pMem, UINT32 count)
{
    m_pMutex->Lock();
    CMemoryNode* pMemNode   = (CMemoryNode*) ((char*)pMem - sizeof(CMemoryNode));
    void* pNewMemory = Alloc(count);
    if (pNewMemory)
    {
	memcpy(pNewMemory, pMem, pMemNode->m_nSize); /* Flawfinder: ignore */
	Free(pMem);
    }
    m_pMutex->Unlock();
    return pNewMemory;
}

/****************************************************************************
*	Method:
*		IMalloc::GetSize
*	Purpose:
*		Simply returns the size requested when the block was allocated.
*
****************************************************************************/
STDMETHODIMP_(UINT32) CMediumBlockAllocator::GetSize(void* pMem)
{
    CMemoryNode* pMemNode   = (CMemoryNode*) ((char*)pMem - m_nHeaderSize);
    return pMemNode->m_nSize;
}

/****************************************************************************
*	Method:
*		IMalloc::DidAlloc
*	Purpose:
*		Have not thought of a nice way to do this.
*
****************************************************************************/
STDMETHODIMP_(HXBOOL) CMediumBlockAllocator::DidAlloc(void* pMem)
{
    return FALSE;
}

/****************************************************************************
*	Method:
*		IMalloc::HeapMinimize
*	Purpose:
*		Deletes all blocks within the free list.
*
****************************************************************************/
STDMETHODIMP_(void) CMediumBlockAllocator::HeapMinimize()
{
    m_pMutex->Lock();
    CMemoryNode* pNode;
    CMemoryNode* pTempNode;

    for(UINT32 i = 0; i<m_nNumberBuckets; i++)
    {
	pNode = m_ppBuckets[i];
	m_pCountArray[i] = 0;
	m_pTimeStampArray = 0;

	while (pNode)
	{
	    pTempNode = pNode;
	    pNode = pTempNode->m_pNext;
	    HX_DELETE(pTempNode);
	}
	m_ppBuckets[i] = NULL;
    }
    m_pMutex->Unlock();
}


/****************************************************************************
*	Method:
*		IHXCallback::Func
*	Purpose:
*		Resets the m_nCurrentTimeStamp to the granularity of 
*		m_nCallbackTime so we needn't call gettimeofday() all of the 
*		time. Also attempts to reduce the size of the free lists
*		if a certian tolerance has been exceeded.
*
****************************************************************************/
STDMETHODIMP
CMediumBlockAllocator::Func()
{
    // every time through this function we must reset the current timestamp
    m_nCurrentTimeStamp = HX_GET_TICKCOUNT();

    // now if the current 
    if (CALCULATE_ELAPSED_TICKS(m_nLastRelaxTime, m_nCurrentTimeStamp) > m_nRelaxTime)
    {
	m_nLastRelaxTime = m_nCurrentTimeStamp;
	RelaxBuckets();
    }
    
    m_hCallback = m_pScheduler->RelativeEnter((IHXCallback*)this, m_nCallbackTime);

    return HXR_OK;
}

/****************************************************************************
*	Method:
*		CMediumBlockAllocator::Func
*	Purpose:
*		Helper function called by Func. Attempts to slowly reduce 
*		the size of the buckets if no one has used them in a while.
*
****************************************************************************/

void CMediumBlockAllocator::RelaxBuckets()
{
    // go through the heap look for places that have a list of nodes
    // if the time stamp on that node is greater than relax time then 
    // throw away a few items off of that head. 

    m_pMutex->Lock();
    for(UINT32 i = 0; i<m_nNumberBuckets; i++)
    {
	CMemoryNode*  pListItem = m_ppBuckets[i];

	if (pListItem)
	{
	    if (CALCULATE_ELAPSED_TICKS(m_pTimeStampArray[i], m_nCurrentTimeStamp) > m_nGarbageTime)
	    {
		// find out how many nodes we have
		UINT32 nNodesToThrowAway = m_pCountArray[i]/m_nGarbageDivider;
		if (!nNodesToThrowAway)
		{
		    nNodesToThrowAway = 1;
		}
		m_pCountArray[i] -= nNodesToThrowAway;
		for (;nNodesToThrowAway; nNodesToThrowAway--)
		{
		    m_ppBuckets[i] = pListItem->m_pNext;
		    HX_DELETE(pListItem);
		    pListItem = m_ppBuckets[i];
		}
	    }
	}
    }
    m_pMutex->Unlock();
}

