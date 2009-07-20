/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxphook.cpp,v 1.11 2006/10/06 21:03:40 ping Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "hxmap.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxrendr.h"
#include "chxphook.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

PacketHookManager::PacketHookManager(IHXPlayer* pPlayer)
	: m_lRefCount(0)
	, m_ulTotalStreams(0)
	, m_ulRecordableStreams(0)
	, m_pPlayer(NULL)
	, m_pPacketHook(NULL)
{
    if (pPlayer)
    {
	m_pPlayer = pPlayer;
	m_pPlayer->AddRef();
    }
}

PacketHookManager::~PacketHookManager()
{
    HX_RELEASE(m_pPacketHook);
    HX_RELEASE(m_pPlayer);
}

STDMETHODIMP
PacketHookManager::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPacketHookManager), (IHXPacketHookManager*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPacketHookManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
PacketHookManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
PacketHookManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
    
/*
 * IHXPacketHookManager methods
 */

/************************************************************************
 *	Method:
 *	    IHXPacketHookManager::InitHook
 *	Purpose:
 *	    called by the top level client to pass the IHXPacketHook object
 */
STDMETHODIMP 
PacketHookManager::InitHook (IHXPacketHook* pPacketHook)
{
    HX_RESULT hr = HXR_OK;
     
    if (!m_pPlayer || !pPacketHook)
    {
	return HXR_FAILED;
    }

    m_pPacketHook = pPacketHook;
    m_pPacketHook->AddRef();

    HXBOOL bRecordAllowed = FALSE;

    UINT16 ulSources = 0;
    UINT16 ulStreams = 0;
    IUnknown* pUnknown = NULL;
    IHXStreamSource* pSource = NULL;
    IHXPrivateStreamSource *pPrivateSource = NULL; // for IsSaveAllowed, take this out; XXXCP

    // caculate the total number of streams + TAC info.
    if (!(ulSources = m_pPlayer->GetSourceCount()))
    {
	return HXR_FAILED;
    }

    for (UINT16 i = 0; i < ulSources; i++)
    {
	if (m_pPlayer->GetSource(i, pUnknown) != HXR_OK)
	{
	    continue;
	}

	HXBOOL bIsSaveAllowed = FALSE;

	if (pUnknown->QueryInterface(IID_IHXPrivateStreamSource, (void**)&pPrivateSource) == HXR_OK)
	{
	    bIsSaveAllowed = pPrivateSource->IsSaveAllowed();

	    HX_RELEASE(pPrivateSource);

	    if (!bIsSaveAllowed)
	    {
		HX_RELEASE(pUnknown);
		continue;
	    }
	}

	if(bIsSaveAllowed)
	    bRecordAllowed = TRUE;

	hr = pUnknown->QueryInterface(IID_IHXStreamSource, (void**)&pSource);

	HX_RELEASE(pUnknown);

	if (hr != HXR_OK)
	{
	    break;
	}

	ulStreams = pSource->GetStreamCount();

	for (UINT16 j = 0; j < ulStreams; j++)
	{
	    IHXStream* pStream = NULL;

	    pSource->GetStream(j, (IUnknown*&)pStream);
	  
	    UINT16 ulRenderers = pStream->GetRendererCount();

	    for (UINT16 k = 0; k < ulRenderers; k++)
	    {
		IHXRenderer* pRenderer = NULL;
		IHXPacketHookSink* pSink = NULL;

		pStream->GetRenderer(k, (IUnknown*&)pRenderer);

		if (HXR_OK == pRenderer->QueryInterface(IID_IHXPacketHookSink,
		                                        (void**)&pSink))
		{
		    pSink->StartSink();
		    HX_RELEASE(pSink);
		}

		HX_RELEASE(pRenderer);
	    }
    
	    HX_RELEASE(pStream);
	}

	HX_RELEASE(pSource);
    }

    if (!bRecordAllowed)
    {
	hr = HXR_NO_DATA;
    }

    return hr;
}

/************************************************************************
 *	Method:
 *	    IHXPacketHookManager::CloseHook
 *	Purpose:
 *	    called by the top level client to close the hook connection
 */
STDMETHODIMP 
PacketHookManager::CloseHook ()
{
     HX_RESULT hr = HXR_OK;

     HX_RELEASE(m_pPacketHook);

     return hr;
}

/************************************************************************
 *	Method:
 *	    IHXPacketHookManager::StartHook
 *	Purpose:
 *	    called by the top level client to start recording
 */
STDMETHODIMP 
PacketHookManager::StartHook ()
{
    HX_RESULT		hr =  HXR_OK;
    UINT16		i = 0;
    UINT16		j = 0;
    UINT16		k = 0;
    UINT16		ulSources = 0;
    UINT16		ulStreams = 0;
    UINT16		ulStreamIndex = 0;
    IHXBuffer*		pTitle = NULL;
    IHXBuffer*		pAuthor = NULL;
    IHXBuffer*		pCopyright = NULL;
    IHXValues*		pFileHeader = NULL;
    IHXValues*		pStreamHeader = NULL;
    IUnknown*		pUnknown = NULL;
    IHXPrivateStreamSource *pPrivateSource = NULL; // for IsSaveAllowed, take this out; XXXCP
    IHXStreamSource*	pSource = NULL;
    IHXInfoLogger*	pInfoLogger = NULL;
    
    // make sure everything has been initialized
    if (!m_pPlayer || !m_pPacketHook)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // caculate the total number of streams + TAC info.
    if (!(ulSources = m_pPlayer->GetSourceCount()))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    m_ulTotalStreams = 0;    
    for (i = 0; i < ulSources; i++)
    {
	if (HXR_OK != m_pPlayer->GetSource(i, pUnknown))
	{
	    continue;
	}

	if (HXR_OK != pUnknown->QueryInterface(IID_IHXStreamSource, (void**)&pSource))
	{
	    HX_RELEASE(pUnknown);
	    continue;
	}

	if (HXR_OK == pUnknown->QueryInterface(IID_IHXPrivateStreamSource, (void**)&pPrivateSource))
	{
	    if (!(pPrivateSource->IsSaveAllowed()))
	    {
		pPrivateSource->Release();
		continue;
	    }
	    pPrivateSource->Release();
	}

	m_ulTotalStreams += pSource->GetStreamCount();

	HX_RELEASE(pSource);
	HX_RELEASE(pUnknown);
    }

    if (!m_ulTotalStreams)
    {

        hr = HXR_FAILED;
        goto cleanup;
    }

    // prepare the file header
    CreateBufferCCF(pTitle, m_pPlayer);
    CreateBufferCCF(pAuthor, m_pPlayer);
    CreateBufferCCF(pCopyright, m_pPlayer);

    // XXX HP: find better way to collect TAC info
#define szTitle	    "title"
#define szAuthor    "author"
#define szCopyright "copyright"

    pTitle->Set((const unsigned char*)szTitle, strlen(szTitle)+1);
    pAuthor->Set((const unsigned char*)szAuthor, strlen(szAuthor)+1);
    pCopyright->Set((const unsigned char*)szCopyright, strlen(szCopyright)+1);

    if (HXR_OK == CreateValuesCCF(pFileHeader, m_pPlayer))
    {
	// set attributes(i.e. num. of streams + TAC)
	pFileHeader->SetPropertyBuffer("Title", pTitle);
	pFileHeader->SetPropertyBuffer("Author", pAuthor);
	pFileHeader->SetPropertyBuffer("Copyright", pCopyright);
	pFileHeader->SetPropertyULONG32("StreamCount", m_ulTotalStreams);
    }
	    
    // signal the top level client of upcoming content
    m_pPacketHook->OnStart();

    // send file header to its top level client
    hr = m_pPacketHook->OnFileHeader(pFileHeader);

    if (hr != HXR_OK)
    {
	HX_RELEASE(m_pPacketHook);
        goto cleanup;
    }

    // prepare the stream headers
    m_ulRecordableStreams = 0;
    ulStreamIndex = 0;
    for (i = 0; i < ulSources; i++)
    {
	if (HXR_OK != m_pPlayer->GetSource(i, pUnknown))
	{
	    HX_RELEASE(pUnknown);
	    continue;
	}

	if (HXR_OK != pUnknown->QueryInterface(IID_IHXStreamSource, (void**)&pSource))
	{
	    HX_RELEASE(pSource);
	    continue;
	}

	if (HXR_OK == pUnknown->QueryInterface(IID_IHXPrivateStreamSource, (void**)&pPrivateSource))
	{
	    if (!(pPrivateSource->IsSaveAllowed()))
	    {
		pPrivateSource->Release();
		continue;
	    }
	    pPrivateSource->Release();
	}
	
	if (HXR_OK == pUnknown->QueryInterface(IID_IHXInfoLogger, (void**)&pInfoLogger))
	{
	    pInfoLogger->LogInformation("RECSTART", NULL);
	}
	HX_RELEASE(pInfoLogger);

	ulStreams = pSource->GetStreamCount();

	for (j = 0; j < ulStreams; j++)
	{
	    const char*	    pszPropName = NULL;
	    UINT16	    ulRenderers = 0;
	    ULONG32	    ulPropValue = 0;
	    HXBOOL	    bRecordable = FALSE;
	    IHXValues*	    pHeader = NULL;
	    IHXBuffer*	    pPropValueSource = NULL;
	    IHXBuffer*	    pPropValueTarget = NULL;
	    IHXStream*	    pStream = NULL;

	    // retrieve the stream info
	    pSource->GetStream(j, (IUnknown*&)pStream);

	    pHeader = pStream->GetHeader();
	
	    // make a copy of this stream header 
	    // XXX HP: this could be wrapped up into one method in CHXHeader	   
	    CreateValuesCCF(pStreamHeader, m_pPlayer);

	    // copy all the ULONG32 attributes
	    if (HXR_OK == pHeader->GetFirstPropertyULONG32(pszPropName, ulPropValue))
	    {
		pStreamHeader->SetPropertyULONG32(pszPropName, ulPropValue);

		while (HXR_OK == pHeader->GetNextPropertyULONG32(pszPropName, ulPropValue))
		{
		    pStreamHeader->SetPropertyULONG32(pszPropName, ulPropValue);
		}
	    }

	    // copy all the buffer attributes
	    if (HXR_OK == pHeader->GetFirstPropertyBuffer(pszPropName, (IHXBuffer*&)pPropValueSource))
	    {
		if (HXR_OK == CreateAndSetBufferCCF(pPropValueTarget, pPropValueSource->GetBuffer(), 
						    pPropValueSource->GetSize(), m_pPlayer))
		{
		    pStreamHeader->SetPropertyBuffer(pszPropName, pPropValueTarget);
		    HX_RELEASE(pPropValueTarget);
		}
		HX_RELEASE(pPropValueSource);

		while (HXR_OK == pHeader->GetNextPropertyBuffer(pszPropName, (IHXBuffer*&)pPropValueSource))
		{
		    if (HXR_OK == CreateAndSetBufferCCF(pPropValueTarget, pPropValueSource->GetBuffer(), 
							pPropValueSource->GetSize(), m_pPlayer))
		    {
			pStreamHeader->SetPropertyBuffer(pszPropName, pPropValueTarget);
			HX_RELEASE(pPropValueTarget);
		    }
		    HX_RELEASE(pPropValueSource);
		}
	    }

	    // copy all the CString attributes
	    if (HXR_OK == pHeader->GetFirstPropertyCString(pszPropName, (IHXBuffer*&)pPropValueSource))
	    {
		if (HXR_OK == CreateAndSetBufferCCF(pPropValueTarget, pPropValueSource->GetBuffer(), 
						    pPropValueSource->GetSize(), m_pPlayer))
		{
		    pStreamHeader->SetPropertyCString(pszPropName, pPropValueTarget);
		    HX_RELEASE(pPropValueTarget);
		}
		HX_RELEASE(pPropValueSource);

		while (HXR_OK == pHeader->GetNextPropertyCString(pszPropName, (IHXBuffer*&)pPropValueSource))
		{
		    if (HXR_OK == CreateAndSetBufferCCF(pPropValueTarget, pPropValueSource->GetBuffer(), 
							pPropValueSource->GetSize(), m_pPlayer))
		    {
			pStreamHeader->SetPropertyCString(pszPropName, pPropValueTarget);
			HX_RELEASE(pPropValueTarget);
		    }
		    HX_RELEASE(pPropValueSource);
		}
	    }

	    HX_RELEASE(pHeader);

	    // modify some values
	    pStreamHeader->SetPropertyULONG32("StreamNumber", ulStreamIndex);
	    
	    // the stream is recordable as long as there is one renderer supports
	    // IHXPacketHookHelper. Multiple renderers can serve the packets with 
	    // the same stream number on it, One solution for this is to choose the 
	    // first renderer which supports IHXPacketHookHelper as the only source
	    ulRenderers = pStream->GetRendererCount();
	    for (k = 0; k < ulRenderers; k++)
	    {
		IUnknown*			pUnknown = NULL;
		IHXPacketHookHelper*		pPacketHookHelper = NULL;
	
		pStream->GetRenderer(k, pUnknown);

		if (HXR_OK == pUnknown->QueryInterface(IID_IHXPacketHookHelper, (void**)&pPacketHookHelper))
		{
		    bRecordable = TRUE;
		    
		    pPacketHookHelper->StartHook(ulStreamIndex, 0, new PacketHookHelperResponse(this, ulStreamIndex));
		}

		HX_RELEASE(pPacketHookHelper);
		HX_RELEASE(pUnknown);
	    }
    
	    if (bRecordable)
	    {
		m_ulRecordableStreams++;
	    }

	    pStreamHeader->SetPropertyULONG32("Recordable", (ULONG32)bRecordable);

	    /*
	     * It's possible that StartHook will cause the m_pPacketHook to
	     * be released
	     */

	    if (!m_pPacketHook)
	    {
		hr = HXR_UNEXPECTED;
	        goto cleanup;
	    }

	    // send stream header to its top level client
	    hr = m_pPacketHook->OnStreamHeader(pStreamHeader);

	    if (hr != HXR_OK)
	    {
		HX_RELEASE(m_pPacketHook);
	        goto cleanup;
	    }

	    ulStreamIndex++;
	    HX_RELEASE(pStreamHeader);
	    HX_RELEASE(pStream);
	}
	
	HX_RELEASE(pSource);
	HX_RELEASE(pUnknown);
    }
	    	    
cleanup:
    
    HX_RELEASE(pTitle);
    HX_RELEASE(pAuthor);
    HX_RELEASE(pCopyright);
    HX_RELEASE(pFileHeader);
 
    return hr;
}
  
/************************************************************************
 *	Method:
 *	    IHXPacketHookManager::StopHook
 *	Purpose:
 *	    called by the top level client to stop recording
 */
STDMETHODIMP 
PacketHookManager::StopHook ()
{
    HX_RESULT		hr = HXR_OK;

    UINT16		i = 0;
    UINT16		j = 0;
    UINT16		k = 0;
    UINT16		ulSources = 0;
    UINT16		ulStreams = 0;
    IUnknown*		pUnknown = NULL;
    IHXStreamSource*	pSource = NULL;
    IHXInfoLogger*	pInfoLogger = NULL;
  
    if (!m_pPlayer || !m_pPacketHook)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // prepare the stream headers
    ulSources = m_pPlayer->GetSourceCount();
    for (i = 0; i < ulSources; i++)
    {
	if (m_pPlayer->GetSource(i, pUnknown) != HXR_OK)
	    continue;

	if (HXR_OK != pUnknown->QueryInterface(IID_IHXStreamSource, (void**)&pSource))
	{
	    break;
	}

	if (HXR_OK == pUnknown->QueryInterface(IID_IHXInfoLogger, (void**)&pInfoLogger))
	{
	    pInfoLogger->LogInformation("RECEND", NULL);
	}
	HX_RELEASE(pInfoLogger);

	ulStreams = pSource->GetStreamCount();

	for (j = 0; j < ulStreams; j++)
	{
	    UINT16	    ulRenderers = 0;	 
	    IHXStream*	    pStream = NULL;

	    // retrieve the stream info
	    pSource->GetStream(j, (IUnknown*&)pStream);
	  
	    ulRenderers = pStream->GetRendererCount();
	    for (k = 0; k < ulRenderers; k++)
	    {
		IHXRenderer*		pRenderer = NULL;
		IHXPacketHookHelper*	pPacketHookHelper = NULL;
		IHXPacketHookSink*	pSink = NULL;

		pStream->GetRenderer(k, (IUnknown*&)pRenderer);

		if (HXR_OK == pRenderer->QueryInterface(IID_IHXPacketHookSink,
		                                        (void**)&pSink))
		{
		    pSink->StopSink();
		    HX_RELEASE(pSink);
		}

		if (HXR_OK == pRenderer->QueryInterface(IID_IHXPacketHookHelper, (void**)&pPacketHookHelper))
		{		  
		    pPacketHookHelper->StopHook();
		}

		HX_RELEASE(pSink);
		HX_RELEASE(pPacketHookHelper);
		HX_RELEASE(pRenderer);
	    }
    
	    HX_RELEASE(pStream);
	}

	HX_RELEASE(pSource);
	HX_RELEASE(pUnknown);
    }

cleanup:
   
    return hr;
}

STDMETHODIMP 
PacketHookManager::OnPacket (IHXPacket* pPacket)
{
    HX_RESULT	hr = HXR_OK;

    if (!m_pPacketHook)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // pass packet back to the top-level client
    hr = m_pPacketHook->OnPacket(pPacket);

    if (hr != HXR_OK)
    {
	HX_RELEASE(m_pPacketHook);
    }

cleanup:

    return hr;
}

STDMETHODIMP 
PacketHookManager::OnEndOfPackets (ULONG32 ulStreamID)
{
    HX_RESULT	hr = HXR_OK;

    if (!m_pPacketHook)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // this is the last stream
    if (!--m_ulRecordableStreams)
    {
	m_pPacketHook->OnEnd();
	
	HX_RELEASE(m_pPacketHook);
    }

cleanup:

    return hr;
}


PacketHookHelperResponse::PacketHookHelperResponse(PacketHookManager* pPacketHookManager,
						   ULONG32 ulStreamID)
    : m_lRefCount (0)
    , m_ulStreamID (0)
    , m_pPacketHookManager (NULL)
{
    m_pPacketHookManager = pPacketHookManager;
    m_pPacketHookManager->AddRef();

    m_ulStreamID = ulStreamID;
}

PacketHookHelperResponse::~PacketHookHelperResponse()
{
   HX_RELEASE(m_pPacketHookManager);
}


/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP 
PacketHookHelperResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPacketHookHelperResponse), (IHXPacketHookHelperResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPacketHookHelperResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) 
PacketHookHelperResponse::AddRef()
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
STDMETHODIMP_(ULONG32) 
PacketHookHelperResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXPacketHookHelperResponse methods
 */

/************************************************************************
 *	Method:
 *	    IHXPacketHookHelperResponse::OnPacket
 *	Purpose:
 *	    called by the renderer to pass the packet for recording
 */
STDMETHODIMP 
PacketHookHelperResponse::OnPacket (IHXPacket* pPacket)
{
    HX_RESULT	hr = HXR_OK;

    if (!m_pPacketHookManager)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // pass packet back to the top-level client
    hr = m_pPacketHookManager->OnPacket(pPacket);

    if (hr != HXR_OK)
    {
	HX_RELEASE(m_pPacketHookManager);
    }

cleanup:

    return hr;
}

/************************************************************************
 *	Method:
 *	    IHXPacketHookManager::OnEndOfPackets
 *	Purpose:
 *	    called by the renderer to notify the end of this stream
 */
STDMETHODIMP 
PacketHookHelperResponse::OnEndOfPackets ()
{
    HX_RESULT	hr = HXR_OK;

    if (!m_pPacketHookManager)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    m_pPacketHookManager->OnEndOfPackets(m_ulStreamID);

cleanup:

    return hr;
}
