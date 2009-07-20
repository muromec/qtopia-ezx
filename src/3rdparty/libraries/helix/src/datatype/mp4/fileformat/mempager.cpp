/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 *  Includes
 */
#include "mempager.h"
#include "hxfswch.h"
#include "hxassert.h"
#include "qtffrefcounter.h"

#ifndef QTCONFIG_SPEED_OVER_SIZE
#include "mempager_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE


/****************************************************************************
 *  Local Defines
 */
#if defined(HELIX_FEATURE_MIN_HEAP)
#define MEMPAGER_DFLT_MIN_PAGE_SIZE 0x00000400  // 1K
#else	// HELIX_FEATURE_MIN_HEAP
#define MEMPAGER_DFLT_MIN_PAGE_SIZE 0x0000FFFF  // 64K
#endif	// HELIX_FEATURE_MIN_HEAP
#define MEMPAGER_BASE_VIRTUAL_ADDR  0x00000001


/****************************************************************************
 *  Class CMemPager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CMemPager::CMemPager(void)
    : m_pFileSwitcher(NULL)
    , m_pResponse(NULL)
    , m_State(MEMPGR_Offline)
    , m_pPageVirtualAddr(NULL)
    , m_pPagePhysicalAddr(NULL)
    , m_ulPageSize(0)
    , m_pFaultedPageVirtualAddr(NULL)
    , m_ulFaultedPageSize(0)
    , m_ulAddrSpaceOffset(0)
    , m_ulAddrSpaceSize(0)
    , m_ulMinPageSize(MEMPAGER_DFLT_MIN_PAGE_SIZE)
    , m_pBaseVirtualAddr(NULL)
    , m_bSyncMode(FALSE)
    , m_pPageBuffer(NULL)
    , m_lRefCount(0)
{
    g_nRefCount_qtff++;
}


CMemPager::~CMemPager()
{
    Reset();
    g_nRefCount_qtff--;
}


/****************************************************************************
 *  CMemPager private methods
 */
/****************************************************************************
 *  Reset
 */
void CMemPager::Reset(void)
{
    HX_RELEASE(m_pFileSwitcher);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pPageBuffer);
    m_pBaseVirtualAddr = NULL;
    m_pPageVirtualAddr = NULL;
    m_pPagePhysicalAddr = NULL;
    m_ulPageSize = 0;
    m_pFaultedPageVirtualAddr = NULL;
    m_ulFaultedPageSize = 0;
    m_ulAddrSpaceOffset = 0;
    m_ulAddrSpaceSize = 0;
    m_ulMinPageSize = MEMPAGER_DFLT_MIN_PAGE_SIZE;
    m_State = MEMPGR_Offline;
}


/****************************************************************************
 *  CMemPager methods - Main interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CMemPager::Init(IUnknown* pSource,
			  ULONG32 ulOffset,
			  ULONG32 ulSize,
			  UINT8** ppVirtualBaseAddr)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State != MEMPGR_Offline)
    {
	retVal = HXR_UNEXPECTED;
    }

    if (SUCCEEDED(retVal))
    {
	Reset();

	// Find Input Interface
	retVal = pSource->QueryInterface(IID_IHXFileSwitcher, 
					 (void**) &m_pFileSwitcher);
    }

    if (SUCCEEDED(retVal))
    {
	m_ulAddrSpaceOffset = ulOffset;
	m_ulAddrSpaceSize = ulSize;
	m_pBaseVirtualAddr = (UINT8*) MEMPAGER_BASE_VIRTUAL_ADDR;
	m_pFaultedPageVirtualAddr = m_pBaseVirtualAddr;
	m_ulFaultedPageSize = m_ulMinPageSize;
	if (m_ulFaultedPageSize > m_ulAddrSpaceSize)
	{
	    m_ulFaultedPageSize = m_ulAddrSpaceSize;
	}
   
	if (ppVirtualBaseAddr)
	{
	    *ppVirtualBaseAddr = m_pBaseVirtualAddr;
	}

	m_State = MEMPGR_Ready;
    }

    return retVal;
}

/****************************************************************************
 *  _PageIn
 */
HX_RESULT CMemPager::_PageIn(UINT8* pVirtualAddr,
			     ULONG32 ulSize,
			     UINT8* &pPhysicalBaseAddr)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileSwitcher && (m_State == MEMPGR_Ready))
    {
	retVal = HXR_NO_DATA;

	m_pFaultedPageVirtualAddr = pVirtualAddr;
	m_ulFaultedPageSize = ulSize;
	if (m_ulFaultedPageSize < m_ulMinPageSize)
	{
	    m_ulFaultedPageSize = m_ulMinPageSize;
	}
	if ((m_pBaseVirtualAddr + m_ulAddrSpaceSize) < 
	    (m_pFaultedPageVirtualAddr + m_ulFaultedPageSize))
	{
	    m_ulFaultedPageSize = (m_pBaseVirtualAddr + m_ulAddrSpaceSize) -
				  m_pFaultedPageVirtualAddr;
	}

	if (m_ulFaultedPageSize == 0)
	{
	    m_pFaultedPageVirtualAddr = NULL;
	    return HXR_FAIL;
	}

	// Page in here only if it can be done synchronously
	if (SUCCEEDED(m_pFileSwitcher->Advise(HX_FILEADVISE_SYNCACCESS)))
	{
	    m_bSyncMode = TRUE;
	    retVal = _LoadPage();
	    if (retVal == HXR_OK)
	    {
		retVal = HXR_FAIL;
		if ((m_ulFaultedPageSize == 0) &&
		    m_pPagePhysicalAddr)
		{
		    pPhysicalBaseAddr = m_pPagePhysicalAddr;
		    retVal = HXR_OK;
		}
	    }
	    m_bSyncMode = FALSE;

	    m_pFileSwitcher->Advise(HX_FILEADVISE_ASYNCACCESS);
	}
    }

    return retVal;	
}

/****************************************************************************
 *  _LoadPage
 */
HX_RESULT CMemPager::_LoadPage(void)
{
    ULONG32 ulOffset;
    HX_RESULT retVal;

    m_State = MEMPGR_ProcLoad;

    ReleasePage();
    HX_ASSERT(m_ulFaultedPageSize);

    ulOffset = m_pFaultedPageVirtualAddr - 
	       m_pBaseVirtualAddr + 
	       m_ulAddrSpaceOffset;

    retVal = m_pFileSwitcher->Seek(ulOffset,
				   FALSE,
				   (IHXFileResponse*) this);

    if (FAILED(retVal))
    {
	retVal = HandleResponse(retVal, NULL);
    }

    return retVal;
}


/****************************************************************************
 *  ReleasePage
 */
void CMemPager::ReleasePage(void)
{
    m_ulPageSize = 0;
    m_pPageVirtualAddr = NULL;
    m_pPagePhysicalAddr = NULL;
    HX_RELEASE(m_pPageBuffer);
}


/****************************************************************************
 *  IHXFileResponse methods
 */
/****************************************************************************
 *  InitDone
 */
STDMETHODIMP CMemPager::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    return retVal;
}

/****************************************************************************
 *  CloseDone
 */
STDMETHODIMP CMemPager::CloseDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    return retVal;
}

/****************************************************************************
 *  ReadDone
 */
STDMETHODIMP CMemPager::ReadDone(HX_RESULT status,
				 IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == MEMPGR_ProcLoad)
    {	
	if (SUCCEEDED(status))
	{
	    status = HXR_FAIL;

	    if (pBuffer)
	    {
		ULONG32 ulSize = pBuffer->GetSize();

		if (ulSize == m_ulFaultedPageSize)
		{
		    HX_ASSERT(m_pPageBuffer == NULL);
		    m_pPageBuffer = pBuffer;
		    pBuffer->AddRef();
		    
		    m_pPageVirtualAddr = m_pFaultedPageVirtualAddr;
		    m_ulPageSize = m_ulFaultedPageSize;
		    m_pPagePhysicalAddr = pBuffer->GetBuffer() - 
					  (m_pPageVirtualAddr - 
					   m_pBaseVirtualAddr);
		    m_pFaultedPageVirtualAddr = NULL;
		    m_ulFaultedPageSize = 0;
		    status = HXR_OK;
		}
	    }

	    retVal = HandleResponse(status, pBuffer);
	}
    }

    return retVal;
}

/****************************************************************************
 *  WriteDone
 */
STDMETHODIMP CMemPager::WriteDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    return retVal;
}

/****************************************************************************
 *  SeekDone
 */
STDMETHODIMP CMemPager::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HX_ASSERT(m_State == MEMPGR_ProcLoad);

    if (m_State == MEMPGR_ProcLoad)
    {
	HX_ASSERT(m_pFileSwitcher);

	if (SUCCEEDED(status))
	{
	    retVal = m_pFileSwitcher->Read(m_ulFaultedPageSize,
					   (IHXFileResponse*) this);
	}
	else
	{
	    retVal = HandleResponse(status);
	}
    }

    return retVal;
}

/****************************************************************************
 *  HandleResponse
 */
HX_RESULT CMemPager::HandleResponse(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT retVal = status;

    if (m_State == MEMPGR_ProcLoad)
    {
	m_State = MEMPGR_Ready;
	if ((!m_bSyncMode) && m_pResponse)
	{
	    retVal = m_pResponse->ReadDone(status, pBuffer);
	}
    }
	    
    return retVal;
}


/****************************************************************************
 *  IHXThreadSafeMethods method
 */
/****************************************************************************
 *  IsThreadSafe
 */
STDMETHODIMP_(UINT32)
CMemPager::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FSR_READDONE;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CMemPager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
	AddRef();
	*ppvObj = (IHXThreadSafeMethods*) this;
	return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CMemPager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CMemPager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
