/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cachobj.cpp,v 1.12 2007/07/06 20:39:16 jfinnecy Exp $
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

/****************************************************************************
 * 
 *  cachobj.cpp
 *
 *  Purpose:
 *	To cache objects in a temporary storage for later retrieval
 *	Can be created off of IHXCommonClassFactory.
 *	Currently supported only on the client side.
 *
 */

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxguid.h"
#include "timeval.h"
#include "basepkt.h"
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxcache.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxtbuf.h"
#include "chunkres.h"
#include "cachobj.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// HXFIFOCache...
HXFIFOCache::HXFIFOCache(IUnknown* pContext) :
      m_lRefCount(0)
    , m_pChunkyRes(NULL)
    , m_ulCurrentReadPosition(0)
    , m_ulCurrentWritePosition(0)
    , m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}

HXFIFOCache::~HXFIFOCache()
{
    Flush();
    HX_RELEASE(m_pContext);
}

/*
 * IUnknown methods
 */



/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXFIFOCache::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXFIFOCache), (IHXFIFOCache*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXFIFOCache::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXFIFOCache::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *		IHXFIFOCache::Cache
 *	Purpose:
 *	    To cache objects in a temporary storage for later retrieval
 *
 *	    Currently supported objects:
 *		IHXBuffer
 *		IHXValues
 *		IHXPacket
// $Private:
 *		IHXTimeStampedBuffer
// $EndPrivate.
 *
 */
STDMETHODIMP HXFIFOCache::Cache(IUnknown*	    pObject)
{
    HX_RESULT		    theErr = HXR_UNEXPECTED;
    IHXClientPacket*	    pClientPacket = NULL;
    IHXTimeStampedBuffer*  pTimeStampBuffer = NULL;
    IHXBuffer*		    pBuffer = NULL;
    IHXPacket*		    pPacket = NULL;
    IHXValues*		    pValues = NULL;

    if (!m_pChunkyRes)
    {
	m_pChunkyRes = new CChunkyRes(m_pContext);
    }

    if (pObject->QueryInterface(IID_IHXClientPacket, (void**) &pClientPacket) == HXR_OK)
    {
	theErr = CacheClientPacket(pClientPacket);
	pClientPacket->Release();
    }
    else if (pObject->QueryInterface(IID_IHXTimeStampedBuffer, (void**) &pTimeStampBuffer) == HXR_OK)
    {
	theErr = CacheTimestampBuffer(pTimeStampBuffer);
	pTimeStampBuffer->Release();
    }
    else if (pObject->QueryInterface(IID_IHXBuffer, (void**) &pBuffer) == HXR_OK)
    {
	theErr = CacheBuffer(pBuffer);
	pBuffer->Release();
    }
    else if (pObject->QueryInterface(IID_IHXPacket, (void**) &pPacket) == HXR_OK)
    {
	theErr = CachePacket(pPacket);
	pPacket->Release();
    }
    else if (pObject->QueryInterface(IID_IHXValues, (void**) &pValues) == HXR_OK)
    {
	theErr = CacheValues(pValues);
	pValues->Release();
    }

    return theErr;
}

/************************************************************************
 *	Method:
 *	    IHXFIFOCache::Retrieve
 *	Purpose:
 *
 *
 */
STDMETHODIMP HXFIFOCache::Retrieve(REF(IUnknown*)  pObject)
{
    HX_RESULT		theErr = HXR_OK;
    UINT32		ulBytesRead = 0;
    UINT32		ulBytesToRead = 0;
    char*		pData = NULL;
    char*		pCursor = NULL;
    CHXBuffer*		pBuffer = NULL;
    ChunkyCacheLayout*	pChunkyCacheLayout = NULL;

    if (!m_pChunkyRes)
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    pObject = NULL;
    
    pChunkyCacheLayout = new ChunkyCacheLayout;
    ulBytesToRead = sizeof(ChunkyCacheLayout);

    // read Chunk Header Info
    m_pChunkyRes->GetData(m_ulCurrentReadPosition,
			  (char*)pChunkyCacheLayout,
			  ulBytesToRead,
			  &ulBytesRead);
    m_ulCurrentReadPosition += ulBytesRead;

    if (ulBytesRead)
    {
	HX_ASSERT(ulBytesToRead == ulBytesRead);	

	ulBytesToRead = pChunkyCacheLayout->size - ulBytesRead;
	pData = new char[ulBytesToRead];
	
	// read the Chunk Data
	m_pChunkyRes->GetData(m_ulCurrentReadPosition,
			      (char*)pData,
			      ulBytesToRead,
			      &ulBytesRead);
	HX_ASSERT(ulBytesToRead == ulBytesRead);
	m_ulCurrentReadPosition += ulBytesRead;

	pCursor = pData;
	if (::IsEqualGUID(IID_IHXClientPacket, pChunkyCacheLayout->guid)) 
	{	    	    
	    ClientPacket::UnPack((IHXClientPacket*&)pObject, pCursor, ulBytesRead, m_pContext);
	}
	else if (::IsEqualGUID(IID_IHXPacket, pChunkyCacheLayout->guid))
	{
	    CHXPacket::UnPack((IHXPacket*&)pObject, pCursor, ulBytesRead, m_pContext);
	}
	else if (::IsEqualGUID(IID_IHXTimeStampedBuffer, pChunkyCacheLayout->guid))
	{
	    CHXTimeStampedBuffer::UnPack((IHXTimeStampedBuffer*&)pObject, pCursor, ulBytesRead);
	}
	else if (::IsEqualGUID(IID_IHXBuffer, pChunkyCacheLayout->guid))
	{
	    IHXBuffer* pBuffer = NULL;
	    theErr = CreateAndSetBufferCCF(pBuffer, (UCHAR*)pCursor, ulBytesRead, m_pContext);
	    if (HXR_OK == theErr)
	    {
		pObject = (IUnknown*)(IHXBuffer*)pBuffer;
	    }
	}
	else
	{
	    theErr = HXR_NOTIMPL;
	}
    }

cleanup:

    HX_DELETE(pChunkyCacheLayout);
    HX_VECTOR_DELETE(pData);
    
    return theErr;
}

/************************************************************************
 *	Method:
 *	    IHXFIFOCache::Flush
 *	Purpose:
 *
 *
 */
STDMETHODIMP HXFIFOCache::Flush(void)
{
    m_ulCurrentReadPosition	= 0;
    m_ulCurrentWritePosition	= 0;

    HX_DELETE(m_pChunkyRes);

    return HXR_OK;
}

HX_RESULT 
HXFIFOCache::CacheClientPacket(IHXClientPacket* pClientPacket)
{
    HX_RESULT	theErr = HXR_OK;
    HXBOOL	bContiguousDataPointer = FALSE;
    UINT32	ulBytesWrote = 0;
    UINT32	ulBytesToWrite = 0;
    char*	pData = NULL;
    char*	pCursor = NULL;
#if !defined(HELIX_FEATURE_FULLGUID)
    GUID	tmp = IID_IHXClientPacket;
#endif
    if (!pClientPacket)
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // caculate the size of client packet
    ClientPacket::Pack(pClientPacket, NULL, ulBytesToWrite, m_pContext);
    ulBytesToWrite += sizeof(ChunkyCacheLayout);

    // check whether we have cont. memory chunk of the size needed
    if (HXR_OK == m_pChunkyRes->GetContiguousDataPointer(m_ulCurrentWritePosition,
							 pData,
							 ulBytesToWrite) && pData)
    {	
	bContiguousDataPointer = TRUE;
	pCursor = pData;
    }
    // otherwise we allocated our own
    else
    {
	pData = new char[ulBytesToWrite];
	pCursor = pData;
    }

    // total chunk bytes
    *pCursor++ = (BYTE)ulBytesToWrite; *pCursor++ = (BYTE)(ulBytesToWrite >> 8);
    ulBytesWrote += 2;
    // GUID
#if !defined(HELIX_FEATURE_FULLGUID)
    memcpy(pCursor, (char*)&tmp, sizeof(GUID)); /* Flawfinder: ignore */
#else
    memcpy(pCursor, (char*)&IID_IHXClientPacket, sizeof(GUID)); /* Flawfinder: ignore */
#endif
    pCursor += sizeof(GUID);
    ulBytesWrote += sizeof(GUID);

    // pack data
    ClientPacket::Pack(pClientPacket, pCursor, ulBytesWrote, m_pContext);	

    if (!bContiguousDataPointer)
    {
	// let the memory manager take care of the contingency
	theErr = m_pChunkyRes->SetData(m_ulCurrentWritePosition, pData, ulBytesWrote);
    }

    HX_ASSERT(ulBytesToWrite == ulBytesWrote);
    // advance the write cursor
    m_ulCurrentWritePosition += ulBytesToWrite;

cleanup:

    if (!bContiguousDataPointer)
    {
	HX_VECTOR_DELETE(pData);
    }

    return theErr;
}

HX_RESULT 
HXFIFOCache::CacheTimestampBuffer(IHXTimeStampedBuffer* pTimeStampBuffer)
{
    HX_RESULT	theErr = HXR_OK;
    HXBOOL	bContiguousDataPointer = FALSE;
    UINT32	ulBytesWrote = 0;
    UINT32	ulBytesToWrite = 0;
    char*	pData = NULL;
    char*	pCursor = NULL;
#if !defined(HELIX_FEATURE_FULLGUID)
    GUID	tmp = IID_IHXBuffer;
#endif

    if (!pTimeStampBuffer)
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // caculate the size of buffer
    CHXTimeStampedBuffer::Pack(pTimeStampBuffer, NULL, 0, ulBytesToWrite);
    ulBytesToWrite += sizeof(ChunkyCacheLayout);

    // check whether we have cont. memory chunk of the size needed
    if (HXR_OK == m_pChunkyRes->GetContiguousDataPointer(m_ulCurrentWritePosition,
							 pData,
							 ulBytesToWrite) && pData)
    {	
	bContiguousDataPointer = TRUE;
	pCursor = pData;
    }
    // otherwise we allocated our own
    else
    {
	pData = new char[ulBytesToWrite];
	pCursor = pData;
    }

    // total chunk bytes
    *pCursor++ = (BYTE)ulBytesToWrite; *pCursor++ = (BYTE)(ulBytesToWrite >> 8);
    ulBytesWrote += 2;
    // GUID
#if !defined(HELIX_FEATURE_FULLGUID)
    memcpy(pCursor, (char*)&tmp, sizeof(GUID)); /* Flawfinder: ignore */
#else
    memcpy(pCursor, (char*)&IID_IHXBuffer, sizeof(GUID)); /* Flawfinder: ignore */
#endif
    pCursor += sizeof(GUID);
    ulBytesWrote += sizeof(GUID);

    // pack data
    CHXTimeStampedBuffer::Pack(pTimeStampBuffer, pCursor,
                               ulBytesToWrite - ulBytesWrote, ulBytesWrote);	

    if (!bContiguousDataPointer)
    {
	// let the memory manager take care of the contingency
	theErr = m_pChunkyRes->SetData(m_ulCurrentWritePosition, pData, ulBytesWrote);
    }

    HX_ASSERT(ulBytesToWrite == ulBytesWrote);
    // advance the write cursor
    m_ulCurrentWritePosition += ulBytesToWrite;

cleanup:

    if (!bContiguousDataPointer)
    {
	HX_VECTOR_DELETE(pData);
    }

    return theErr;
}

HX_RESULT 
HXFIFOCache::CacheBuffer(IHXBuffer* pBuffer)
{
    HX_RESULT	theErr = HXR_OK;
    HXBOOL	bContiguousDataPointer = FALSE;
    UINT32	ulBytesWrote = 0;
    UINT32	ulBytesToWrite = 0;
    char*	pData = NULL;
    char*	pCursor = NULL;
#if !defined(HELIX_FEATURE_FULLGUID)
    GUID	tmp = IID_IHXBuffer;
#endif

    if (!pBuffer)
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // caculate the size of client packet
    ulBytesToWrite = pBuffer->GetSize();
    ulBytesToWrite += sizeof(ChunkyCacheLayout);

    // check whether we have cont. memory chunk of the size needed
    if (HXR_OK == m_pChunkyRes->GetContiguousDataPointer(m_ulCurrentWritePosition,
							 pData,
							 ulBytesToWrite) && pData)
    {	
	bContiguousDataPointer = TRUE;
	pCursor = pData;
    }
    // otherwise we allocated our own
    else
    {
	pData = new char[ulBytesToWrite];
	pCursor = pData;
    }

    // total chunk bytes
    *pCursor++ = (BYTE)ulBytesToWrite; *pCursor++ = (BYTE)(ulBytesToWrite >> 8);
    ulBytesWrote += 2;
    // GUID
#if !defined(HELIX_FEATURE_FULLGUID)
    memcpy(pCursor, (char*)&tmp, sizeof(GUID)); /* Flawfinder: ignore */
#else
    memcpy(pCursor, (char*)&IID_IHXBuffer, sizeof(GUID)); /* Flawfinder: ignore */
#endif
    pCursor += sizeof(GUID);
    ulBytesWrote += sizeof(GUID);

    // pack data
    memcpy(pCursor, (char*)pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */

    if (!bContiguousDataPointer)
    {
	// let the memory manager take care of the contingency
	theErr = m_pChunkyRes->SetData(m_ulCurrentWritePosition, pData, ulBytesWrote);
    }

    HX_ASSERT(ulBytesToWrite == ulBytesWrote);
    // advance the write cursor
    m_ulCurrentWritePosition += ulBytesToWrite;

cleanup:

    if (!bContiguousDataPointer)
    {
	HX_VECTOR_DELETE(pData);
    }

    return theErr;
}

HX_RESULT 
HXFIFOCache::CachePacket(IHXPacket* pPacket)
{
    HX_RESULT	theErr = HXR_OK;
    HXBOOL	bContiguousDataPointer = FALSE;
    UINT32	ulBytesWrote = 0;
    UINT32	ulBytesToWrite = 0;
    char*	pData = NULL;
    char*	pCursor = NULL;
#if !defined(HELIX_FEATURE_FULLGUID)
    GUID	tmp = IID_IHXPacket;
#endif

    if (!pPacket)
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // caculate the size of client packet
    CHXPacket::Pack(pPacket, NULL, ulBytesToWrite, m_pContext);
    ulBytesToWrite += sizeof(ChunkyCacheLayout);

    // check whether we have cont. memory chunk of the size needed
    if (HXR_OK == m_pChunkyRes->GetContiguousDataPointer(m_ulCurrentWritePosition,
							 pData,
							 ulBytesToWrite) && pData)
    {	
	bContiguousDataPointer = TRUE;
	pCursor = pData;
    }
    // otherwise we allocated our own
    else
    {
	pData = new char[ulBytesToWrite];
	pCursor = pData;
    }

    // total chunk bytes
    *pCursor++ = (BYTE)ulBytesToWrite; *pCursor++ = (BYTE)(ulBytesToWrite >> 8);
    ulBytesWrote += 2;
    // GUID
#if !defined(HELIX_FEATURE_FULLGUID)
    memcpy(pCursor, (char*)&tmp, sizeof(GUID)); /* Flawfinder: ignore */
#else
    memcpy(pCursor, (char*)&IID_IHXPacket, sizeof(GUID)); /* Flawfinder: ignore */
#endif
    pCursor += sizeof(GUID);
    ulBytesWrote += sizeof(GUID);

    // pack data
    CHXPacket::Pack(pPacket, pCursor, ulBytesWrote, m_pContext);	

    if (!bContiguousDataPointer)
    {
	// let the memory manager take care of the contingency
	theErr = m_pChunkyRes->SetData(m_ulCurrentWritePosition, pData, ulBytesWrote);
    }

    HX_ASSERT(ulBytesToWrite == ulBytesWrote);
    // advance the write cursor
    m_ulCurrentWritePosition += ulBytesToWrite;

cleanup:

    if (!bContiguousDataPointer)
    {
	HX_VECTOR_DELETE(pData);
    }

    return theErr;
}

HX_RESULT 
HXFIFOCache::CacheValues(IHXValues* pValues)
{
    // TBD
    HX_RESULT theErr = HXR_NOTIMPL;
    return theErr;
}
