/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chunkres.cpp,v 1.26 2006/02/16 23:03:01 ping Exp $
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
#include "pckunpck.h"
#include "chunkres.h"
#include "hlxclib/stdlib.h"		// needed for MAX_PATH
#include "hlxclib/stdio.h"		// for fopen(), etc.
#include "hlxclib/limits.h"		// for INT_MAX, etc.

#include "hlxclib/fcntl.h"	// for O_CREAT, etc.
#include "hlxclib/io.h"
#include "hlxclib/windows.h"

#include "chxdataf.h"	// cross platform file object

#ifdef _MACINTOSH
#ifdef _MAC_MACHO
#include <unistd.h> // for unlink call
#else
#include <unix.h>		// for unlink call
#undef _UNIX	//defined in unixmac.h
#endif
#endif

#ifdef _UNIX
#include <unistd.h> 	// for unlink
#ifndef _MAX_PATH
#define _MAX_PATH		256
#endif
#endif

#ifdef _SYMBIAN
#include <unistd.h> //for unlink
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::OpenResource()
//
//	Purpose:
//
//		Opens an existing resource or creates a new resource.
//
//	Parameters:
//
//		CChunkyRes** ppChunkyRes
//		Memory location that will be filled in on output with the pointer
//		to the opened CChunkRes object.
//
//		const char* pResName
//		Unique name of the resource to open or create.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResMgr::OpenResource(CChunkyRes** ppChunkyRes, const char* pResName)
{
    HX_RESULT theErr = HXR_OK;

    HX_ASSERT(ppChunkyRes && pResName);
    
    void* pData;
    if (m_OpenResources.Lookup(pResName, pData))
    {
	*ppChunkyRes = (CChunkyRes*)pData;
    }
    else if (m_ClosedResources.Lookup(pResName, pData))
    {
	*ppChunkyRes = (CChunkyRes*)pData;
	HX_VERIFY(m_ClosedResources.RemoveKey(pResName));
	m_OpenResources.SetAt(pResName, pData);

	RemoveFromLRU(pResName);
    }
    else
    {
	*ppChunkyRes = new CChunkyRes(m_pContext);
	if (*ppChunkyRes)
	{
	    m_OpenResources.SetAt(pResName, (void*)*ppChunkyRes);
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	}
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::CloseResource()
//
//	Purpose:
//
//		Closes an existing resource. Closed resources may be discarded.
//
//	Parameters:
//
//		CChunkyRes* pChunkyRes
//		Pointer to a previously opened CChunkRes object.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResMgr::CloseResource(CChunkyRes* pChunkyRes)
{
    HX_RESULT theErr = HXR_FAIL;

    POSITION pPos = m_OpenResources.GetStartPosition();
    while (pPos)
    {
	CHXString key;
	void* pData;
	m_OpenResources.GetNextAssoc(pPos, key, pData);

	if (pData == (void*)pChunkyRes)
	{
	    HX_VERIFY(m_OpenResources.RemoveKey(key));

	    m_ClosedResources.SetAt(key, pData);

	    HX_ASSERT(!m_LRUResources.FindString(key));
	    m_LRUResources.AddTailString(key);

	    theErr = HXR_OK;
	}
    }

    if (theErr == HXR_OK)
    {
	DiscardDiskData();
    }

    return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::CloseResource()
//
//	Purpose:
//
//		Closes an existing resource. Closed resources may be discarded.
//
//	Parameters:
//
//		const char* pResName
//		Unique name of a previously opened resource.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResMgr::CloseResource(const char* pResName)
{
    HX_RESULT theErr = HXR_FAIL;

    void* pData;
    if (m_OpenResources.Lookup(pResName, pData))
    {
	HX_VERIFY(m_OpenResources.RemoveKey(pResName));

	m_ClosedResources.SetAt(pResName, pData);

	HX_ASSERT(!m_LRUResources.FindString(pResName));
	m_LRUResources.AddTailString(pResName);

	theErr = HXR_OK;
    }

    if (theErr == HXR_OK)
    {
	DiscardDiskData();
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::DiscardResource()
//
//	Purpose:
//
//		Discards a resource. Closed resources may be discarded.
//
//	Parameters:
//
//		const char* pResName
//		Unique name of a previously opened resource.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResMgr::DiscardResource(const char* pResName)
{
    HX_RESULT theErr = HXR_FAIL;

    void* pData;
    if (m_OpenResources.Lookup(pResName, pData))
    {
	HX_VERIFY(m_OpenResources.RemoveKey(pResName));

	CChunkyRes* pRes = (CChunkyRes*)pData;

	delete pRes;

	theErr = HXR_OK;
    }

    if (m_ClosedResources.Lookup(pResName, pData))
    {
	HX_VERIFY(m_ClosedResources.RemoveKey(pResName));

	RemoveFromLRU(pResName);

	CChunkyRes* pRes = (CChunkyRes*)pData;

	delete pRes;

	theErr = HXR_OK;
    }

    return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::FindResource()
//
//	Purpose:
//
//		Looks to see if the resource exists.
//
//	Parameters:
//
//		const char* pResName
//		Unique name of a previously opened resource.
//
//	Return:
//
//		HX_RESULT
HX_RESULT
CChunkyResMgr::FindResource(const char* pResName)
{
    void* pData;
    if (m_OpenResources.Lookup(pResName, pData) ||
	m_ClosedResources.Lookup(pResName, pData))
    {
	return HXR_OK;
    }

    return HXR_FAIL;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::SetDiskUsageThreshold()
//
//	Purpose:
//
//		Sets the Disk Usage threshold for the chunky resource manager.
//		If closed resources amount to more than the threshold, then they
//		will be discarded.
//
//	Parameters:
//
//		ULONG32 diskUsage
//		Disk usage in bytes which will be allowed for closed resources.
//
//	Return:
//
//		None.
//
void CChunkyResMgr::SetDiskUsageThreshold(ULONG32 diskUsage)
{
    m_ulDiskUsage = diskUsage;

    DiscardDiskData();
}


void CChunkyResMgr::DiscardDiskData()
{
    void* pData = NULL;
    CChunkyRes* pRes = NULL;
    ULONG32 ulTotal = 0;

    // Count the total disk usage
    POSITION pPos = m_ClosedResources.GetStartPosition();
    while (pPos)
    {
	CHXString key;
	m_ClosedResources.GetNextAssoc(pPos, key, pData);

	HX_ASSERT(pData);
	pRes = (CChunkyRes*)pData;

	ulTotal += pRes->GetDiskUsage();
    }

    // Trim as much as we need until we're under the disk usage threshold.
    pPos = m_LRUResources.GetHeadPosition();
    while (pPos && ulTotal > m_ulDiskUsage)
    {
	CHXString* pResName = m_LRUResources.GetNext(pPos);

	HX_ASSERT(pResName);
	if (m_ClosedResources.Lookup(*pResName, pData))
	{
	    HX_ASSERT(pData);
	    pRes = (CChunkyRes*)pData;

	    ULONG32 ulSize = pRes->GetDiskUsage();
	    if (ulSize)
	    {
		HX_ASSERT(ulSize <= ulTotal);
		ulTotal -= ulSize;

		m_ClosedResources.RemoveKey(*pResName);

		RemoveFromLRU(*pResName);

		delete pRes;
	    }
	}
    }
}


void CChunkyResMgr::RemoveFromLRU(const char* pResName)
{
    POSITION pPos = m_LRUResources.GetHeadPosition();
    POSITION pPrev; 
    while (pPos)
    {
	pPrev = pPos;
	CHXString* pStr = m_LRUResources.GetNext(pPos);
	if (!strcmp(*pStr, pResName))
	{
	    m_LRUResources.RemoveAt(pPrev);
	}
    }
}




/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::CChunkyResMgr()
//
//	Purpose:
//
//		Construtor for chunky resource manager.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		N/A
//
CChunkyResMgr::CChunkyResMgr(IUnknown* pContext)
    : m_ulDiskUsage(0)
    , m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResMgr::~CChunkyResMgr()
//
//	Purpose:
//
//		Destructor for chunky resource manager.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		N/A
//
CChunkyResMgr::~CChunkyResMgr()
{
    CHXString	key;
    CChunkyRes* pRes;
    POSITION	p;
    
    p = m_OpenResources.GetStartPosition();
    while (p)
    {
	m_OpenResources.GetNextAssoc(p, key, (void*&)pRes);
	HX_DELETE(pRes);
    }

    p = m_ClosedResources.GetStartPosition();
    while (p)
    {
	m_ClosedResources.GetNextAssoc(p, key, (void*&)pRes);
	HX_DELETE(pRes);
    }
    
    HX_RELEASE(m_pContext);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::DiscardRange()
//
//	Purpose:
//
//		Discards the specified range of the file.
//
//	Parameters:
//
//		The location and length of the range to be discarded.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyRes::DiscardRange( ULONG32 offset, ULONG32 count )
{
    HX_RESULT theErr = HXR_OK;
    
    // Big picture of this function is that it takes
    // care of the end cases, where part of a chunk may
    // be invalidated; then it totally removes all the
    // chunks wholly contained by the range.
    
    ULONG32 ulOffsetIntoChunk;
    
    ULONG32 ulFirstChunk = offset/DEF_CHUNKYRES_CHUNK_SIZE;
    
    ulOffsetIntoChunk = offset % DEF_CHUNKYRES_CHUNK_SIZE;
    
    ULONG32 ulLastChunk  = (offset+count)/DEF_CHUNKYRES_CHUNK_SIZE;
    
    if (ulFirstChunk == ulLastChunk)
    {
	// if the range is all in one chunk, deal with that simplest
	// case and ignore the more complicated scenarios.
	
	CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ulFirstChunk];
	
	HX_ASSERT(pChunk);
	
	pChunk->AddValidRange(ulOffsetIntoChunk, count, FALSE);
	
	return theErr;
    }
    
    if (ulOffsetIntoChunk)
    {
        // OK, we have a chunk that needs to be partially invalidated.
        
	CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ulFirstChunk];
	
	HX_ASSERT(pChunk);
	
	pChunk->AddValidRange(ulOffsetIntoChunk, DEF_CHUNKYRES_CHUNK_SIZE - ulOffsetIntoChunk, FALSE);
	
        ulFirstChunk++;
    }
    
    ulOffsetIntoChunk = (offset+count) % DEF_CHUNKYRES_CHUNK_SIZE;
    if (ulOffsetIntoChunk)
    {
        // OK, the final chunk needs to be partially invalidated.
        
	CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ulLastChunk];
	
	HX_ASSERT(pChunk);
	
	pChunk->AddValidRange(0, ulOffsetIntoChunk, FALSE);
    }
    
    for (ULONG32 ulWhichChunk = ulFirstChunk; ulWhichChunk < ulLastChunk; ulWhichChunk++)
    {
	CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ulWhichChunk];

	// if the chunk doesn't (yet?) exist, then it's considered invalid.
	
	if (pChunk)
	{
	    ULONG32 ulTempOffset = pChunk->GetTempFileOffset();
	    if (ulTempOffset)
	    {
		m_FreeDiskOffsets.AddHead((void*)ulTempOffset);
	    }
	    
	    delete pChunk;
	    m_Chunks[ulWhichChunk] = NULL;
	}
    }
    
    return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::GetDiskUsage()
//
//	Purpose:
//
//		Returns the entire disk usage of a resource.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		ULONG32
//		Total amount of diskspace the resource's chunks consume.
//
ULONG32 CChunkyRes::GetDiskUsage() const
{
	return (m_Chunks.GetSize() * DEF_CHUNKYRES_CHUNK_SIZE);
}

ULONG32 CChunkyRes::GetCurrentMemoryUsage() const
{
    return m_CurMemUsage;
    
}

HXBOOL CChunkyRes::HasPartialData(ULONG32 length, ULONG32 offset /* = 0 */)
{
	return (GetContiguousLength(offset) >= length);
}

ULONG32 CChunkyRes::GetContiguousLength(ULONG32 offset /* = 0 */)
{
    Lock();
	ULONG32 contiguousLength = 0;
	int ndx;

	int startNdx = offset / DEF_CHUNKYRES_CHUNK_SIZE;
	if (startNdx < m_Chunks.GetSize())
	{
		CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[startNdx];
		
		if (!pChunk) goto exit;
		
		contiguousLength = pChunk->GetValidLength(offset % DEF_CHUNKYRES_CHUNK_SIZE);
		
		if (contiguousLength != DEF_CHUNKYRES_CHUNK_SIZE - (offset % DEF_CHUNKYRES_CHUNK_SIZE))
		{
			goto exit;
		}
	}
	startNdx++;
	
	for (ndx = startNdx; ndx < m_Chunks.GetSize(); ndx++)
	{
		CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ndx];

		// if there is no chunk then we are no longer contiguous.
		if (!pChunk)
		{
			break;
		}

		ULONG32 chunkLength = pChunk->GetValidLength();

		contiguousLength += chunkLength;

		// if this chunk is not the max length then we are no longer contiguous
		if (chunkLength < DEF_CHUNKYRES_CHUNK_SIZE)
		{
			break;
		}
	}

exit:
    Unlock();
	return contiguousLength;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::GetData()
//
//	Purpose:
//
//		Gets a block of data out of a resource.
//
//	Parameters:
//
//		ULONG32 offset
//		char* buf
//		ULONG32 count
//		ULONG32* actual
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyRes::GetData(ULONG32 offset, char* buf, ULONG32 count, ULONG32* actual)
{
    Lock();
	HX_RESULT theErr = HXR_OK;
	int ndx;

	ULONG32 ulFirstChunk = offset/DEF_CHUNKYRES_CHUNK_SIZE;
	ULONG32 ulLastChunk  = (offset+count)/DEF_CHUNKYRES_CHUNK_SIZE;

	HX_ASSERT(ulFirstChunk < INT_MAX);
	HX_ASSERT(ulLastChunk < INT_MAX);

	int		nFirstChunk  = (int)ulFirstChunk;
	int		nLastChunk   = (int)ulLastChunk;

	HX_ASSERT(m_Chunks.GetSize() >= nLastChunk+1);

	ULONG32 chunkOffset = offset - (ulFirstChunk*DEF_CHUNKYRES_CHUNK_SIZE);
	ULONG32 chunkCount  = count;
	ULONG32 baseOffset  = 0;

	*actual = 0;	// -fst
	for (ndx = nFirstChunk; (ndx <= nLastChunk) && chunkCount; ndx++)
	{
		CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ndx];

		if (!pChunk)
		{
		    // with random access, it's feasible that there's a null chunk.
		    
		    theErr = HXR_CHUNK_MISSING;
		    goto exit;
		}
		
		HX_ASSERT_VALID_PTR(pChunk);

		ULONG32 chunkActual = 0;

		// Actually get the data from the chunk!
		ULONG32 chunkAmount = HX_MIN(DEF_CHUNKYRES_CHUNK_SIZE-chunkOffset,chunkCount);
		theErr = pChunk->GetData(chunkOffset,buf+baseOffset,chunkAmount,&chunkActual);
		if (theErr != HXR_OK)
		{
			goto exit;
		}

		// What?!?!
		HX_ASSERT(chunkActual == chunkAmount);

		*actual += chunkActual;		// -fst

		// reduce the chunk count...
		chunkCount -= chunkAmount;
		baseOffset += chunkAmount;

		// only the first chunk has an offset!
		chunkOffset = 0;
	}

	// Remember how many bytes have been served to the user,
	// in case they want us to discard used data
	m_ulUsedBytes = offset + *actual;

	// Discard chunks that have been fully served to the user
	if (m_bDiscardUsedData)
	{
		nLastChunk = (int)(m_ulUsedBytes/DEF_CHUNKYRES_CHUNK_SIZE);
		for (ndx = m_ulFirstChunkIdx; ndx < nLastChunk - 1; ndx++)
		{
			CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ndx];
			HX_ASSERT_VALID_PTR(pChunk);

			UINT32 ulTempOffset = pChunk->GetTempFileOffset();
			pChunk->DiscardDiskData();

			// Increment the first valid chunk index
			m_ulFirstChunkIdx++;

			if (ulTempOffset)
			{
			    // Add the disk space to the free space list
			    m_FreeDiskOffsets.AddHead((void*)ulTempOffset);
			}
		}
	}

exit:

    Unlock();
	return theErr;
}

HX_RESULT 
CChunkyRes::GetContiguousDataPointer(ULONG32 offset, char*& buf, ULONG32 count)
{
    Lock();
    HX_RESULT theErr = HXR_OK;
    HX_ASSERT(m_bDiscardUsedData == FALSE && m_bDisableDiskIO == FALSE);

    ULONG32 ulFirstChunk = offset/DEF_CHUNKYRES_CHUNK_SIZE;
    ULONG32 ulLastChunk  = (offset+count)/DEF_CHUNKYRES_CHUNK_SIZE;

    HX_ASSERT(ulFirstChunk < INT_MAX);
    HX_ASSERT(ulLastChunk < INT_MAX);

    // if the required data length spans two chunks, we cannot have 
    // contiguous memory
    if (ulFirstChunk != ulLastChunk)
    {
	theErr = HXR_FAIL;
    }
    else
    {
        int nFirstChunk  = (int)ulFirstChunk;
        if (m_Chunks.GetSize() < nFirstChunk+1)
        {
            m_Chunks.SetSize(nFirstChunk+1);
        }
        
        CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[nFirstChunk];
        if (!pChunk)
        {
            pChunk = new CChunkyResChunk(this);
            if (m_bDisableDiskIO)
            {
                pChunk->DisableDiskIO();
            }
            m_Chunks[nFirstChunk] = pChunk;
        }

        HX_ASSERT(m_Chunks.GetSize() >= nFirstChunk+1);
        HX_ASSERT_VALID_PTR(pChunk);

        ULONG32 chunkOffset = offset - (ulFirstChunk*DEF_CHUNKYRES_CHUNK_SIZE);
        // Actually get the data from the chunk!
        ULONG32 chunkAmount = HX_MIN(DEF_CHUNKYRES_CHUNK_SIZE-chunkOffset, count);

        theErr = pChunk->GetContiguousDataPointer(chunkOffset,buf,chunkAmount);
    }

    Unlock();

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::SetData()
//
//	Purpose:
//
//		Sets a block of data in a resource.
//
//	Parameters:
//
//		ULONG32 offset
//		const char* buf
//		ULONG32 count
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//

HX_RESULT CChunkyRes::SetData(ULONG32 offset, const char* buf, ULONG32 count, void* pOwner /* = NULL */)
{
	Lock();
        
	HX_RESULT theErr = HXR_OK;

	ULONG32 ulFirstChunk = offset/DEF_CHUNKYRES_CHUNK_SIZE;
	ULONG32 ulLastChunk  = (offset+count)/DEF_CHUNKYRES_CHUNK_SIZE;

	HX_ASSERT(ulFirstChunk < INT_MAX);
	HX_ASSERT(ulLastChunk < INT_MAX);

	int		nFirstChunk  = (int)ulFirstChunk;
	int		nLastChunk   = (int)ulLastChunk;

	if (m_Chunks.GetSize() < nLastChunk+1)
	{
		m_Chunks.SetSize(nLastChunk+1);
	}

	ULONG32 chunkOffset = offset - (ulFirstChunk*DEF_CHUNKYRES_CHUNK_SIZE);
	ULONG32 chunkCount  = count;
	ULONG32 baseOffset  = 0;

	for (int ndx = nFirstChunk; ndx <= nLastChunk; ndx++)
	{
		CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ndx];

		if (!pChunk)
		{
			pChunk = new CChunkyResChunk(this);
			if (m_bDisableDiskIO)
			{
			    pChunk->DisableDiskIO();
			}
			m_Chunks[ndx] = pChunk;
		}

		// Actually set the data for the chunk!
		theErr = pChunk->SetData(chunkOffset,buf+baseOffset,HX_MIN(DEF_CHUNKYRES_CHUNK_SIZE-chunkOffset,chunkCount));
		if (theErr != HXR_OK)
		{
			goto exit;
		}

		// reduce the chunk count...
		chunkCount -= (DEF_CHUNKYRES_CHUNK_SIZE-chunkOffset);
		baseOffset += (DEF_CHUNKYRES_CHUNK_SIZE-chunkOffset);

		// only the first chunk has an offset!
		chunkOffset = 0;
	}
        
        // maintain the cursor, if this SetData includes a cursor owner.
        if (pOwner)
        {
            SetCursor(pOwner, offset + count);
        }

exit:

	Unlock();
	return theErr;
}

void CChunkyRes::TrimDownMemoryMRU()
{
	// If we have just reduced our allowed memory usage, then
	// discard the least recently used chunks till we are under
	// the ne threshold...
	if (m_CurMemUsage > m_MemUsageThreshold)
	{
		while (!m_ChunksMemoryMRU->IsEmpty() && (m_CurMemUsage > m_MemUsageThreshold))
		{
			// Get the least recently used chunk.
			CChunkyResChunk* pChunk = (CChunkyResChunk*)m_ChunksMemoryMRU->GetTail();
			HX_ASSERT_VALID_PTR(pChunk);

			// Discount its usage.
			m_CurMemUsage -= pChunk->GetSize();

			// Spill this chunk to disk...
			pChunk->SpillToDisk();

			// Remove the chunk from the end of the Memory MRU
			m_ChunksMemoryMRU->RemoveTail();

			// And add the chunk to the front of the Disk MRU
			m_ChunksDiskMRU->AddHead(pChunk);
		}

		// How can this be?!?! Did you really mean to set the memory usage such
		// that there are no chunks in memory?!?
		HX_ASSERT(!m_ChunksMemoryMRU->IsEmpty());
	}
}

void
CChunkyRes::AddCursor(void* pCursorOwner, ULONG32 ulCursorLocation)
{
    HX_ASSERT(m_CursorMap.Lookup(pCursorOwner) == NULL);
    m_CursorMap.SetAt(pCursorOwner, (void*)ulCursorLocation);
}

void
CChunkyRes::SetCursor(void* pCursorOwner, ULONG32 ulCursorLocation)
{
    HX_ASSERT(m_CursorMap.Lookup(pCursorOwner) != NULL);
    m_CursorMap.SetAt(pCursorOwner, (void*)ulCursorLocation);
}

void
CChunkyRes::RemoveCursor(void* pCursorOwner)
{
    HX_ASSERT(m_CursorMap.Lookup(pCursorOwner) != NULL);
    m_CursorMap.Remove(pCursorOwner);
}

ULONG32
CChunkyRes::CountCursors()
{
    return m_CursorMap.GetCount();
}

HX_RESULT
CChunkyRes::GetNthCursorInformation(int nIndex, REF(void*)pCursorOwner, REF(ULONG32)ulCursorLocation)
{
    HX_ASSERT(nIndex >= 0);
    HX_ASSERT(nIndex < m_CursorMap.GetCount());
    
    int ndx = 0;
    
    LISTPOSITION pos = m_CursorMap.GetStartPosition();
    while (ndx < nIndex)
    {
        void* pDummyOwner = NULL;
        ULONG32 ulDummyLocation = 0;
        ndx++;
        m_CursorMap.GetNextAssoc(pos, pDummyOwner, (void*&)ulDummyLocation);
        
        if (!pos)
        {
            return HXR_UNEXPECTED;
        }
    }
    
    HX_ASSERT(pos);
    
    if (!pos)
    {
        return HXR_UNEXPECTED;
    }
    
    m_CursorMap.GetNextAssoc(pos, pCursorOwner, (void*&)ulCursorLocation);
    
    return HXR_OK;
}

HX_RESULT
CChunkyRes::GetCursorInformation(void* pInCursorOwner, REF(ULONG32)ulOutCursorLocation)
{
    if (m_CursorMap.Lookup(pInCursorOwner, (void*&)ulOutCursorLocation))
    {
        return HXR_OK;
    }
    else
    {
        return HXR_UNEXPECTED;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::SetMemUsageThreshold()
//
//	Purpose:
//
//		Sets the memory usage threshold for the chunky resource chunks.
//		If if chunk sizes amount to more than the threshold, then the
//		least recently used ones will be spilled to disk.
//
//	Parameters:
//
//		ULONG32 memUsage
//		Memory usage in bytes which will be allowed for all chunks before
//		least recently used chunks will be spilled to disk.
//
//	Return:
//
//		None.
//
void CChunkyRes::SetMemUsageThreshold(ULONG32 memUsage)
{
	m_MemUsageThreshold = memUsage;
	TrimDownMemoryMRU();
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::CChunkyRes()
//
//	Purpose:
//
//		Constructor for a chunky resource.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		N/A
//
CChunkyRes::CChunkyRes(IUnknown* pContext)
	: m_Chunks()
	, m_strTempFileName()
	, m_ulNextTempFileChunk(DEF_START_CHUNK_OFFSET)
	, m_bHasBeenOpened(FALSE)
	, m_bDisableDiskIO(FALSE)
	, m_bDiscardUsedData(FALSE)
	, m_ulFirstChunkIdx(0)
	, m_ulUsedBytes(0)
	, m_pMutex(NULL)
	, m_MemUsageThreshold(DEF_CHUNKYRES_MEM_THRESHOLD)
	, m_CurMemUsage(0)
	, m_ChunksMemoryMRU(NULL)
	, m_ChunksDiskMRU(NULL)
	, m_ChunkSize(DEF_CHUNKYRES_CHUNK_SIZE)
	, m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    HX_ASSERT(m_pMutex);

    m_ChunksMemoryMRU = new CHXSimpleList;
    m_ChunksDiskMRU = new CHXSimpleList;

    HX_ASSERT(m_ChunksMemoryMRU);
    HX_ASSERT(m_ChunksDiskMRU);
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::~CChunkyRes()
//
//	Purpose:
//
//		Destructor for a chunky resource.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		N/A
//
CChunkyRes::~CChunkyRes()
{
	// If we are getting rid of the resource, then
	// we should discard all of the chunks...
	for (int ndx = 0; ndx < m_Chunks.GetSize(); ndx++)
	{
		CChunkyResChunk* pChunk = (CChunkyResChunk*)m_Chunks[ndx];
		if (pChunk)
		{
			delete pChunk;
		}
	}
	HX_RESULT theErr = HXR_OK;
        theErr = DiscardDiskData();
	HX_ASSERT(theErr == HXR_OK);

	if(m_ChunksMemoryMRU)
	{
	    HX_ASSERT(m_ChunksMemoryMRU->GetCount() == 0);
	    delete m_ChunksMemoryMRU;
	    m_ChunksMemoryMRU = NULL;
	}
	
	if(m_ChunksDiskMRU)
	{
	    HX_ASSERT(m_ChunksDiskMRU->GetCount() == 0);
	    delete m_ChunksDiskMRU;
	    m_ChunksDiskMRU = NULL;
	}

	HX_RELEASE(m_pMutex);
	HX_RELEASE(m_pContext);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::MakeSureChunkIsInMemory()
//
//	Purpose:
//
//		Get a portion of the data for a chunk.
//
//	Parameters:
//
//		ULONG32 offset
//		char* buf
//		ULONG32 count
//		ULONG32* actual
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::MakeSureChunkIsInMemory()
{
	HX_RESULT theErr = HXR_OK;

	// If we don't have a chunk pointer, then we aren't in
	// memory...
	if (!m_pChunkData)
	{
		// Find ourselves in the MRU list for Disk chunks...
		LISTPOSITION pos = m_pChunkRes->m_ChunksDiskMRU->Find(this);

		// If were found in the disk MRU list, then we have
		// some work to do...
		if (pos)
		{
			// First, remove ourselves from the disk list...
			m_pChunkRes->m_ChunksDiskMRU->RemoveAt(pos);

			// Load from disk...
			theErr = LoadFromDisk();
			if (theErr != HXR_OK)
			{
				goto exit;
			}
		}
		else
		{
			#ifdef _DEBUG
			{
				// We shouldn't find ourselves in the MRU list
				// for memory chunks... but we want to check!
				LISTPOSITION pos = m_pChunkRes->m_ChunksMemoryMRU->Find(this);

				// We shouldn't be in this list!!!!
				HX_ASSERT(pos == NULL);
			}
			#endif // end _DEBUG section

			m_pChunkData = new UCHAR[m_pChunkRes->m_ChunkSize];
			if (!m_pChunkData) 
			{
				theErr = HXR_OUTOFMEMORY;
				goto exit;
			}
			HX_ASSERT(GetValidLength() == 0);
		}

		// Add to the front of the Memory list...
		m_pChunkRes->m_ChunksMemoryMRU->AddHead(this);

		m_pChunkRes->m_CurMemUsage += GetSize();

		// Make sure we don't have to much info in memory...
		if (!m_bDisableDiskIO)
		{
		    m_pChunkRes->TrimDownMemoryMRU();
		    HX_ASSERT(m_pChunkData);
		}
	}
	// If we are already in memory, then make sure we are at the
	// top of the Memory MRU list!!!
	else
	{
		// We should find ourselves in the MRU list
		// for memory chunks... but we want to check!
		LISTPOSITION pos = m_pChunkRes->m_ChunksMemoryMRU->Find(this);

		// XXXNH: If we aren't in this list it means we were paged out,
		// so we only need to put ourselves at the top of the MRU list
		if (pos)
		{
		    // First, remove ourselves from wherever we are in the
		    // Memory MRU list...
		    m_pChunkRes->m_ChunksMemoryMRU->RemoveAt(pos);
		}

		// And add ourselves to the top of the list!
		m_pChunkRes->m_ChunksMemoryMRU->AddHead(this);
	}

exit:

	return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::GetValidLength()
//
//	Purpose:
//
//		Determines how much of a chunk is valid
//
//	Parameters:
//
//		ULONG32 offset
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
ULONG32 CChunkyResChunk::GetValidLength(ULONG32 offset /* = 0 */) const
{
    
    HX_ASSERT(offset < GetSize());
    
    ULONG32 ulValidLength = 0;
    
	LISTPOSITION rangePos = m_ValidRanges.GetHeadPosition();
	
	if (rangePos)
	{
		do
		{
			ValidRange* pRange = (ValidRange*)m_ValidRanges.GetNext(rangePos);

			HX_ASSERT(pRange);

			// see if the offset points into this particular range

			if (offset >= pRange->offset
				&& offset <= pRange->offset + pRange->length)
			{
			    ulValidLength = pRange->offset + pRange->length - offset;
			}
		}
		while (rangePos);
	}
	
    return ulValidLength;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::GetData()
//
//	Purpose:
//
//		Get a portion of the data for a chunk.
//
//	Parameters:
//
//		ULONG32 offset
//		char* buf
//		ULONG32 count
//		ULONG32* actual
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::GetData(ULONG32 offset, char* buf, ULONG32 count, ULONG32* actual)
{
	HX_RESULT theErr;

	if(!count)
	{
		*actual = count;
		return HXR_OK;
	}

	// We should have a non-zero valid size
	if (!GetValidLength(offset))
	{
		// This chunk must have been discarded
		theErr = HXR_CHUNK_MISSING;
		goto exit;
	}

	// Make sure this chunk is in memory!
	theErr = MakeSureChunkIsInMemory();

	if (theErr != HXR_OK)
	{
		goto exit;
	}

	// You can't read more than there is room in this chunk.
	// CChunkyRes should prevent this case...
	HX_ASSERT(offset+count <= GetSize());

	// The call to MakeSureChunkIsInMemory() should have handled this!
	HX_ASSERT_VALID_PTR(m_pChunkData);

	*actual = HX_MIN(count,GetValidLength(offset));

	HX_ASSERT(*actual < UINT_MAX);

	memcpy(buf,m_pChunkData+offset,(int)(*actual)); /* Flawfinder: ignore */

exit:

	return theErr;
}

HX_RESULT 
CChunkyResChunk::GetContiguousDataPointer(ULONG32 offset, char*& buf, ULONG32 count)
{
    HX_RESULT theErr = HXR_OK;

    if(!count)
    {
	theErr = HXR_FAIL;
	goto exit;
    }

    // First, make sure this chunk is in memory!
    theErr = MakeSureChunkIsInMemory();
    if (theErr != HXR_OK)
    {
	goto exit;
    }

    // You can't write more than there is room in this chunk.
    // CChunkyRes should prevent this case...
    HX_ASSERT(offset+count <= GetSize());

    // Currently, you must write to chunks in order from the
    // start of the chunk first. Random access may come in the
    // future...
    if (GetValidLength(offset) <= 0)
    {
        // /This can happen in the SMIL2 <prefetch> cases.
        theErr = HXR_FAIL;
        goto exit;
    }

    // The call to MakeSureChunkIsInMemory() should have handled this!
    HX_ASSERT_VALID_PTR(m_pChunkData);

    AddValidRange(offset, count);
    
    HX_ASSERT(count < UINT_MAX);

    buf = (char*) (m_pChunkData+offset);

    m_bModified = TRUE;

exit:

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::AddValidRange()
//
//	Purpose:
//
//		Mark a part of this CChunkyResChunk as valid
//		Called from SetData.
//
//	Parameters:
//
//		ULONG32 offset
//		ULONG32 length
//		HXBOOL	bValid
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::AddValidRange(ULONG32 offset, ULONG32 length, HXBOOL bValid /* = TRUE */)
{
	HX_RESULT theErr = HXR_OK;
	
    int nCount = m_ValidRanges.GetCount();
    LISTPOSITION pos = m_ValidRanges.GetHeadPosition();
	
    if (bValid)
	{
		// I don't think we need to ensure that the chunk is in
		// memory, although it always probably will be if this is
		// called from SetData.
		
		// Ensure that it's saying that a legal range is valid.
		
		HX_ASSERT(offset+length <= GetSize());
		
		// Create a new range element.
		
		ValidRange* pNewRange = new ValidRange;
		pNewRange->offset = offset;
		pNewRange->length = length;
		
		// Iterate through the valid ranges to ensure that
		// none of them overlap the range we're adding now.
		
        for (int i=0; i<nCount; i++)
		{
            ValidRange* pRange = (ValidRange*)m_ValidRanges.GetAt(pos);
			
            HXBOOL bNeedToMerge = FALSE;

            // See if this range element overlaps the front end of the
            // new range element.
            if (pRange->offset <= pNewRange->offset
	            && pRange->offset + pRange->length >= pNewRange->offset)
            {
	            bNeedToMerge = TRUE;
            }

            // see if this range element overlaps the back end of the
            // new range element.

            if (pRange->offset <= pNewRange->offset + pNewRange->length
	            && pRange->offset + pRange->length >= pNewRange->offset + pNewRange->length)
            {
	            bNeedToMerge = TRUE;
            }

			// if an overlap happened, make the new range element hold
			// the union of both ranges.
            if (bNeedToMerge)
            {
                ULONG32 ulStartOfRange = HX_MIN(pNewRange->offset, pRange->offset);
                ULONG32 ulEndOfRange = HX_MAX(pNewRange->offset+pNewRange->length,
                pRange->offset+pRange->length);

                HX_ASSERT(ulEndOfRange >= ulStartOfRange);

                pNewRange->offset = ulStartOfRange;
                pNewRange->length = ulEndOfRange-ulStartOfRange;
				
                // delete the one we overlap with since we've ensured
                // that pNewRange's range covers both.
                pos = m_ValidRanges.RemoveAt(pos);
                delete pRange;
			}
            else
                m_ValidRanges.GetAtNext(pos);
		}
		
		// Now that we're sure that nobody overlaps us, we can
		// add this range.
		
		m_ValidRanges.AddTail((void*)pNewRange);
	}
    else
    {
        // bValid is false, so we're INVALIDATING a range.

        // iterate through the list of valid ranges, and for each of them
        // that overlaps the incoming range, either trim it appropriately
        // or delete it entirely.

        for (int i=0; i<nCount; i++)
        {
            ValidRange* pRange = (ValidRange*)m_ValidRanges.GetAt(pos);
            HX_ASSERT(pRange);

            // see if it's totally covered by the incoming range

            if (offset <= pRange->offset && offset+length >= pRange->offset + pRange->length)
            {
                pos = m_ValidRanges.RemoveAt(pos);
                delete pRange;
            }
            else
            {
                // see if it needs to be trimmed

                ULONG32 ulCurrentRangeEnd = pRange->offset + pRange->length;
                ULONG32 ulRangeEnd = offset + length;

                HXBOOL bNeedToTrimOffBackEnd = pRange->offset < offset && ulCurrentRangeEnd >= offset;
                HXBOOL bNeedToTrimOffFrontEnd = pRange->offset < ulRangeEnd
                && ulCurrentRangeEnd > ulRangeEnd;

                if (bNeedToTrimOffBackEnd)
                {
                    pRange->length = offset - pRange->offset;
                }

                if (bNeedToTrimOffFrontEnd)
                {
                    // if we've also trimmed off the back end
                    // then we need to create a new range element
                    // to hold this sans-front-end element.

                    if (bNeedToTrimOffBackEnd)
                    {
                        pRange = new ValidRange;
                        m_ValidRanges.AddHead(pRange);
                    }
                    pRange->offset = ulRangeEnd;
                    pRange->length = ulCurrentRangeEnd - pRange->offset;
                }

                m_ValidRanges.GetAtNext(pos);
            }
		}
    }
	
    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::SetData()
//
//	Purpose:
//
//		Set a portion of the data for a chunk.
//
//	Parameters:
//
//		ULONG32 offset
//		const char* buf
//		ULONG32 count
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::SetData(ULONG32 offset, const char* buf, ULONG32 count)
{
	// First, make sure this chunk is in memory!
	HX_RESULT theErr = MakeSureChunkIsInMemory();

	if (theErr != HXR_OK)
	{
		goto exit;
	}

	// You can't write more than there is room in this chunk.
	// CChunkyRes should prevent this case...
	HX_ASSERT(offset+count <= GetSize());

	// The call to MakeSureChunkIsInMemory() should have handled this!
	HX_ASSERT_VALID_PTR(m_pChunkData);

	HX_ASSERT(count < UINT_MAX);

        memcpy(m_pChunkData+offset,buf,(int)(offset+count <= GetSize() ? count : GetSize() - offset));

	m_bModified = TRUE;
	
	// Make sure that it remembers that this is now a valid
	// range.
	
	AddValidRange(offset, count);

exit:

	return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::SpillToDisk()
//
//	Purpose:
//
//		Spills to disk the data of a chunk.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::SpillToDisk()
{
	Lock();
	
	HX_RESULT	theErr		= HXR_OK;
	CHXDataFile*	pFile		= NULL;
	ULONG32		actualCount	= 0;

	// Don't waste any time unless we are actually modified.
	// And only actually spill, if there is something to spill
	if (!m_bModified || !m_pChunkData)
	{
		goto exit;
	}

	// If we have never spilled to disk, then ask the ChunkyRes
	// for the temp file name and a slot to spill to.
	if (!m_bPreviouslySpilled)
	{
		theErr = m_pChunkRes->GetTempFileChunk(pFile,m_ulTempFileOffset);
	}
	// Otherwise, just get the temp file name.
	else
	{
		theErr = m_pChunkRes->GetTempFile(pFile);
	}

	// If we failed to open the file, then set the valid
	// size to 0. If the user wants to use the data, they
	// will need to handle the case of not having the data!
	if (theErr != HXR_OK)
	{
		HX_ASSERT(pFile == NULL);
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	theErr = pFile->Seek(m_ulTempFileOffset,SEEK_SET);

	if (theErr != HXR_OK)
	{
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	HX_ASSERT(m_pChunkData);
	actualCount = pFile->Write((char *)m_pChunkData, m_pChunkRes->m_ChunkSize);

	m_bPreviouslySpilled = TRUE;

	if (actualCount != m_pChunkRes->m_ChunkSize)
	{
		theErr = HXR_TEMP_FILE;
	}

exit:

	// If we created a file, then clean it up!
	if (pFile)
	{
		delete pFile;
	}

	// If we had an error then record that our size is now invalid.
	if (theErr != HXR_OK)
	{
		AddValidRange(0, m_pChunkRes->m_ChunkSize, FALSE);
		m_bPreviouslySpilled = FALSE;
	}

	// Never the less, we do get rid of the data!
	if (m_pChunkData)
	{
		delete[] m_pChunkData;
		m_pChunkData = NULL;
	}

	Unlock();
	return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::LoadFromDisk()
//
//	Purpose:
//
//		Loads into memory the data from the chunk previously spilled to
//		disk.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::LoadFromDisk()
{
	Lock();
	
	HX_RESULT	theErr		= HXR_OK;
	CHXDataFile*	pFile		= NULL;
	ULONG32		amountRead	= 0;

	// We shouldn't be here if we have memory already allocated!
	HX_ASSERT(m_pChunkData == NULL);

	// If we have never spilled to disk, then there is nothing to
	// load from disk!
	if (!m_bPreviouslySpilled)
	{
		// Even if we've never been spilled, we need to make
		// sure we have memory available...

		m_pChunkData = new UCHAR[m_pChunkRes->m_ChunkSize];

		if(!m_pChunkData)
		{
			theErr = HXR_OUTOFMEMORY;
			goto exit;
		}


		goto exit;
	}

	// Get the temp file name.
	theErr = m_pChunkRes->GetTempFile(pFile);

	// If we failed to open the file, then set the valid
	// size to 0. If the user wants to use the data, they
	// will need to handle the case of not having the data!
	if (theErr != HXR_OK)
	{
		HX_ASSERT(pFile == NULL);
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	theErr = pFile->Seek(m_ulTempFileOffset,SEEK_SET);

	if (theErr != HXR_OK)
	{
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	m_pChunkData = new UCHAR[m_pChunkRes->m_ChunkSize];

	if(!m_pChunkData)
	{
		theErr = HXR_OUTOFMEMORY;
		goto exit;
	}

	amountRead = pFile->Read((char *)m_pChunkData, m_pChunkRes->m_ChunkSize);

	if(amountRead != m_pChunkRes->m_ChunkSize)
	{
		theErr = HXR_TEMP_FILE;
		delete[] m_pChunkData;
		m_pChunkData = NULL;

		goto exit;
	}

exit:

	// If we actually, loaded the data from disk, then
	// we are not modified!
	if (theErr == HXR_OK)
	{
		m_bModified = FALSE;
	}

	// If we created a file, then clean it up!
	if (pFile)
	{
		delete pFile;
	}

	// If we had an error then record that our size is now invalid.
	if (theErr != HXR_OK)
	{
		AddValidRange(0, m_pChunkRes->m_ChunkSize, FALSE);
		m_bPreviouslySpilled = FALSE;
	}

	Unlock();
	return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::CChunkyResChunk()
//
//	Purpose:
//
//		Constructor for CChunkyResChunk.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		N/A
//
CChunkyResChunk::CChunkyResChunk(CChunkyRes* pChunkyRes)
	: m_ChunkOffset(0)
	, m_pChunkData(NULL)
	, m_ulTempFileOffset(0)
	, m_bPreviouslySpilled(FALSE)
	, m_bModified(FALSE)
	, m_pChunkRes(pChunkyRes)
	, m_bDisableDiskIO(FALSE)
{
    HX_ASSERT(m_pChunkRes);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::~CChunkyResChunk()
//
//	Purpose:
//
//		Destructor for CChunkyResChunk.
//
//	Parameters:
//
//		N/A
//
//	Return:
//
//		N/A
//
CChunkyResChunk::~CChunkyResChunk()
{
	HX_RESULT theErr = HXR_OK;
        theErr = DiscardDiskData();
	HX_ASSERT(theErr == HXR_OK);

	if (m_pChunkData)
	{
		delete[] m_pChunkData;
		m_pChunkData = NULL;
	}
	
	while (!m_ValidRanges.IsEmpty())
	{
		ValidRange* pRange = (ValidRange*)m_ValidRanges.RemoveHead();
		delete pRange;
	}
}


HX_RESULT CChunkyRes::GetTempFileChunk(CHXDataFile*& pFile,ULONG32& ulTempFileOffset)
{
	// You should set this to NULL on input.
	HX_ASSERT(pFile == NULL);

	// Get the temporary file...
	HX_RESULT theErr = GetTempFile(pFile);

	if (theErr == HXR_OK)
	{
		// If there are free chunk spaces in the file, use those first
		if (!m_FreeDiskOffsets.IsEmpty())
		{
			ulTempFileOffset = (UINT32)(PTR_INT)m_FreeDiskOffsets.GetTail();
			m_FreeDiskOffsets.RemoveTail();
		}
		else
		{
		    // return the previous next chunk offset...
		    ulTempFileOffset = m_ulNextTempFileChunk;

		    // bump the next chunk offset
		    m_ulNextTempFileChunk += m_ChunkSize;
		}
	}

	return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyResChunk::DiscardDiskData()
//
//	Purpose:
//
//		Discard the disk data for a chunk. This is normally done on
//		destruction of the chunk when the resource associated with this
//		chunk is discarded from disk, but can also be done when we
//		are downloading a live stream and want to discard chunks that
//		have already been served up to the user.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyResChunk::DiscardDiskData()
{
	HX_RESULT theErr = HXR_OK;

	// Remove ourselves from the Memory MRU list...
	LISTPOSITION posMem = m_pChunkRes->m_ChunksMemoryMRU->Find(this);

	if (posMem)
	{
		m_pChunkRes->m_ChunksMemoryMRU->RemoveAt(posMem);
		m_pChunkRes->m_CurMemUsage -= GetSize();
	}

	// Remove ourselves from the Disks MRU list...
	LISTPOSITION posDisk = m_pChunkRes->m_ChunksDiskMRU->Find(this);

	if (posDisk)
	{
		m_pChunkRes->m_ChunksDiskMRU->RemoveAt(posDisk);
	}

	// Reset a bunch of our members in case someone tries
	// to access this chunk after its data has been discarded
	m_ChunkOffset		= 0;
	AddValidRange(0, m_pChunkRes->m_ChunkSize, FALSE);
	HX_VECTOR_DELETE(m_pChunkData);
	m_ulTempFileOffset	= 0;
	m_bPreviouslySpilled	= FALSE;
	m_bModified		= FALSE;
	m_bDisableDiskIO	= TRUE;

	return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		CChunkyRes::DiscardDiskData()
//
//	Purpose:
//
//		Discard the disk data for a chunk. This is normally done on
//		destruction of the chunk when the resource associated with this
//		chunk is discarded from disk.
//
//	Parameters:
//
//		None.
//
//	Return:
//
//		HX_RESULT
//		Possible errors include: TBD.
//
HX_RESULT CChunkyRes::DiscardDiskData()
{
	HX_RESULT theErr = HXR_OK;

	const char* pFileName = m_strTempFileName;

	if (pFileName && *pFileName)
	{
		int nRet = 0;				
#if defined (_MACINTOSH) || defined (_UNIX) || defined(_SYMBIAN) || defined(_OPENWAVE)
		nRet = unlink(pFileName);	
#else
		nRet =-1;					
		if (DeleteFile(OS_STRING(pFileName)))
		    nRet = 0;
#endif
		HX_ASSERT(nRet == 0);
		m_strTempFileName = "";
	}

	return theErr;
}

HX_RESULT CChunkyRes::GetTempFile(CHXDataFile*& pFile)
{
	// You should set this to NULL on input.
	HX_ASSERT(pFile == NULL);

	HX_RESULT		theErr			= HXR_OK;
	const char*		pFileName		= m_strTempFileName;
	char			szTempFileName[_MAX_PATH]; /* Flawfinder: ignore */

	// Create the OS Specific File object...
	pFile = CHXDataFile::Construct(m_pContext);

	if (!pFile)
	{
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	// If we don't have a filename, then we need to
	// get a temp filename, and we know we are creating
	// a file.
	if (!pFileName || !*pFileName)
	{
#if defined(_MAC_MACHO) || defined(_MAC_CFM) // GR 7/15/03 other platforms may want a clearer name here too, since 8.3 restrictions don't apply
 		if(!pFile->GetTemporaryFileName("Helix", szTempFileName, _MAX_PATH))
#else
		if(!pFile->GetTemporaryFileName("PNX",szTempFileName, _MAX_PATH))
#endif
		{
			goto exit;
		}

		m_strTempFileName = szTempFileName;
		pFileName = m_strTempFileName;
	}

	// Open the file...
	if (!pFileName)
	{
		theErr = HXR_TEMP_FILE;
		goto exit;
	}

	if (!m_bHasBeenOpened)
	{
		// Note: _O_RDWR does not work on the mac. O_RDWR is standard ANSI.
		theErr = pFile->Open(pFileName,O_CREAT + O_RDWR);
		
		if (!theErr)
		{
			m_bHasBeenOpened = TRUE;
		}
	}
	else
	{
		// Note: _O_RDWR does not work on the mac. O_RDWR is standard ANSI.
		theErr = pFile->Open(pFileName, O_RDWR);
			
	}
exit:

	return theErr;
}

