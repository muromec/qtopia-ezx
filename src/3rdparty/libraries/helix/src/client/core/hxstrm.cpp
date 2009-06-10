/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstrm.cpp,v 1.18 2007/07/06 21:58:11 jfinnecy Exp $
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

#include "hlxclib/stdio.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxasm.h"
#include "hxgroup.h"
#include "hxplay.h"
#include "hxsmbw.h"
#include "hxprefs.h"
#include "hxslist.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxsmstr.h"
#include "hxstrm.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif



HXStream::HXStream() :
     m_lRefCount (0)
    ,m_bPostSeekToBeSent(FALSE)
    ,m_pSource (NULL)
    ,m_pHeader (NULL)
    ,m_pUnkRenderer(NULL)
    ,m_uStreamNumber(0)
#if defined(HELIX_FEATURE_ASM)
    ,m_pASMStream(NULL)
#endif /* HELIX_FEATURE_ASM */
    ,m_ulRegistryID(0)
{
}

HXStream::~HXStream()
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_ulRegistryID)
    {
	if (m_pSource && m_pSource->m_pRegistry)
	{
	    m_pSource->m_pRegistry->DeleteById(m_ulRegistryID);
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    if (m_pSource)
    {
	m_pSource->Release();
    }

    if (m_pHeader)
    {
	m_pHeader->Release();
    }

    if (m_pUnkRenderer)
    {
	m_pUnkRenderer->Release();
    }

#if defined(HELIX_FEATURE_ASM)
    HX_RELEASE(m_pASMStream);
#endif /* HELIX_FEATURE_ASM */
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXStream::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXStream), (IHXStream*)this },
            { GET_IIDHANDLE(IID_IHXStream2), (IHXStream2*)this },
	    { GET_IIDHANDLE(IID_IHXStream3), (IHXStream3*)this },
            { GET_IIDHANDLE(IID_IHXRegistryID), (IHXRegistryID*)this },
            { GET_IIDHANDLE(IID_IHXLayoutStream), (IHXLayoutStream*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXStream*)this },
        };
    
    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    // if it succeeded, return immediately...
    if (SUCCEEDED(res))
    {
        return res;
    }
    // ...otherwise proceed
    
#if defined(HELIX_FEATURE_ASM)
    /* Let HXASMStream respond to ASM interface if it wants to. */
    if (m_pASMStream && (HXR_OK == m_pASMStream->QueryInterface(riid, ppvObj)))
    {
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_ASM */

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXStream::AddRef()
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
STDMETHODIMP_(ULONG32) HXStream::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}



/*
 * IHXStream methods
 */

/************************************************************************
 *	Method:
 *	    IHXStream::GetSource
 *	Purpose:
 *	    Get the interface to the source object of which the stream is
 *	    a part of.
 *
 */
STDMETHODIMP HXStream::GetSource(IHXStreamSource*	&pSource)
{
    pSource = m_pSource;

    if (pSource)
    {
	pSource->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXStream::GetStreamNumber
 *	Purpose:
 *	    Get the stream number for this stream relative to the source 
 *	    object of which the stream is a part of.
 *
 */
STDMETHODIMP_(UINT16) HXStream::GetStreamNumber(void)
{
    HX_ASSERT(m_pHeader);
    return (UINT16)m_uStreamNumber;
}


/************************************************************************
 *	Method:
 *	    IHXStream::GetStreamType
 *	Purpose:
 *	    Get the MIME type for this stream. NOTE: The returned string is
 *	    assumed to be valid for the life of the IHXStream from which it
 *	    was returned.
 *
 */
STDMETHODIMP_(const char*) HXStream::GetStreamType(void)
{
    HX_ASSERT(m_pHeader);

    IHXBuffer*	pMimeTypeBuffer = 0;

    m_pHeader->GetPropertyCString("MimeType",pMimeTypeBuffer);

    HX_ASSERT(pMimeTypeBuffer);

    const char* pMimeTypeString = (const char* )pMimeTypeBuffer->GetBuffer();

    pMimeTypeBuffer->Release();

    return pMimeTypeString;
}

/************************************************************************
 *	Method:
 *	    IHXStream::GetHeader
 *	Purpose:
 *      Get the header for this stream.
 *
 */
STDMETHODIMP_(IHXValues*) HXStream::GetHeader(void)
{
    HX_ASSERT(m_pHeader);
    m_pHeader->AddRef();
    return m_pHeader;
}

/************************************************************************
 *	Method:
 *	    IHXStream::ReportQualityOfService
 *	Purpose:
 *	    Call this method to report to the playback context that the 
 *	    quality of service for this stream has changed. The unQuality
 *	    should be on a scale of 0 to 100, where 100 is the best possible
 *	    quality for this stream. Although the transport engine can 
 *	    determine lost packets and report these through the user
 *	    interface, only the renderer of this stream can determine the 
 *	    "real" perceived damage associated with this loss.
 *
 *	    NOTE: The playback context may use this value to indicate loss
 *	    in quality to the user interface. When the effects of a lost
 *	    packet are eliminated the renderer should call this method with
 *	    a unQuality of 100.
 *
 */
STDMETHODIMP HXStream::ReportQualityOfService(UINT8 unQuality)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *	Method:
 *	    IHXStream::ReportRebufferStatus
 *	Purpose:
 *	    Call this method to report to the playback context that the
 *	    available data has dropped to a critically low level, and that
 *	    rebuffering should occur. The renderer should call back into this
 *	    interface as it receives additional data packets to indicate the
 *	    status of its rebuffering effort.
 *
 *	    NOTE: The values of unNeeded and unAvailable are used to indicate
 *	    the general status of the rebuffering effort. For example, if a
 *	    renderer has "run dry" and needs 5 data packets to play smoothly
 *	    again, it should call ReportRebufferStatus() with 5,0 then as
 *	    packet arrive it should call again with 5,1; 5,2... and eventually
 *	    5,5.
 *
 */
STDMETHODIMP HXStream::ReportRebufferStatus(UINT8 unNeeded, UINT8 unAvailable)
{
    if (m_pSource && m_pHeader)
    {
	return m_pSource->ReportRebufferStatus(GetStreamNumber(), FALSE,
					       unNeeded, unAvailable);
    }
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXStream2::ReportAudioRebufferStatus
 *	Purpose:
 *      For audio only, when it's called, the rebuffer will only occur when
 *      there aren't any packets in the transport and the amount of audio in
 *      audio device falls below the minimum startup audio pushdown(1000ms
 *      by default)
 *      
 *      Non-audio renderers should still call ReportRebufferStatus(), the 
 *      rebuffer will occur when the core drains out all the packets from
 *      the transport buffer
 *
 *      The rest semantic are the same between the 2 calls.
 */
STDMETHODIMP
HXStream::ReportAudioRebufferStatus    (THIS_
					UINT8   unNeeded,
					UINT8   unAvailable)
{
    if (m_pSource && m_pHeader)
    {
	return m_pSource->ReportRebufferStatus(GetStreamNumber(), TRUE,
					       unNeeded, unAvailable);
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXStream3::GetMinimumBufferingInMs
 *  Purpose:
 *      Provides the minimum buffering in ms that is being enforced by
 *	    the player for a particualr stream.  Media packets are dispatched
 *	    to the renderer the "minimum buffering in ms" ahead of playback
 *	    timeline. Determination of "minimum buffering in ms" is typically
 *	    based on required preroll associated with the media stream,
 *	    post decode delay associated with the media stream rendering and
 *	    additional delays imposed by the system due to explict user setting
 *	    of minimum preroll or dynamic discovery of network conditions 
 *	    causing playback disruptions.
 */
STDMETHODIMP_(UINT32)
HXStream::GetMinimumBufferingInMs()
{
    UINT32 ulRetVal = 0;
    STREAM_INFO* pStreamInfo = NULL;    

    if (m_pSource &&
	SUCCEEDED(m_pSource->GetStreamInfo(m_uStreamNumber, pStreamInfo)) &&
	pStreamInfo)
    {
	ulRetVal = pStreamInfo->BufferingState().GetMinBufferingInMs();
    }

    return ulRetVal;
}

STDMETHODIMP
HXStream::SetGranularity (ULONG32 ulGranularity) 
{
    if (m_pSource && m_pHeader)
    {
	return m_pSource->SetGranularity(GetStreamNumber(),ulGranularity);
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *    IHXStream::GetRendererCount
 *  Purpose:
 *    Returns the current number of renderer instances supported by
 *    this stream instance.
 */
STDMETHODIMP_(UINT16) HXStream::GetRendererCount()
{
    // In RMA 1.0 there is only one renderer per stream.
    return 1;
}

/************************************************************************
 *  Method:
 *    IHXStream::GetRenderer
 *  Purpose:
 *    Returns the Nth renderer instance supported by this stream.
 */
STDMETHODIMP HXStream::GetRenderer
(
    UINT16		nIndex,
    REF(IUnknown*)	pUnknown
)
{
    if (nIndex >= 1)
    {
	return HXR_INVALID_PARAMETER;
    }

    pUnknown = m_pUnkRenderer;
    if (pUnknown)
    {
	pUnknown->AddRef();
	return HXR_OK;
    }
    else
    {
	return HXR_FAIL;
    }
}

HX_RESULT    HXStream::Init(HXPlayer* pPlayer, HXSource* pSource, IHXValues* pHeader, 
			     IUnknown* pUnkRenderer)
{
    m_pSource	    = pSource;
    m_pHeader	    = pHeader;

    if (m_pSource)
    {
	m_pSource->AddRef();
    }

    if (m_pHeader)
    {
	m_pHeader->AddRef();

	ULONG32 uStreamNumber = 0;
	m_pHeader->GetPropertyULONG32("StreamNumber",uStreamNumber);
	m_uStreamNumber = (UINT16) uStreamNumber;
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    UINT32 ulSourceID;
    char pStreamEntry[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
    IHXBuffer* pPropName = NULL;

    m_pSource->GetID(ulSourceID);
    m_pSource->m_pRegistry->GetPropName(ulSourceID, pPropName);
    SafeSprintf(pStreamEntry, MAX_DISPLAY_NAME, "%s.Stream%d", pPropName->GetBuffer(), m_uStreamNumber);

    m_ulRegistryID = m_pSource->m_pRegistry->GetId(pStreamEntry);
    HX_ASSERT(m_ulRegistryID);
    pPropName->Release();
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    if (pUnkRenderer)
    {
	m_pUnkRenderer = pUnkRenderer;
	m_pUnkRenderer->AddRef();
    }

#if defined(HELIX_FEATURE_ASM)
    m_pASMStream = new HXASMStream(this, m_pSource);
    if(m_pASMStream)
    {
        m_pASMStream->AddRef();
    }
    else
    {
        return HXR_OUTOFMEMORY;
    }
#endif /* HELIX_FEATURE_ASM */

    return HXR_OK;
}

HX_RESULT	
HXStream::SetRenderer(IUnknown* pUnkRenderer)
{
    if (pUnkRenderer && m_pUnkRenderer != pUnkRenderer)
    {
	HX_RELEASE(m_pUnkRenderer);
	m_pUnkRenderer = pUnkRenderer;
	m_pUnkRenderer->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXRegistryID::GetID
 *	Purpose:
 *	    Get registry ID(hash_key) of the objects(player, source and stream)
 *
 */
STDMETHODIMP
HXStream::GetID(REF(UINT32) /*OUT*/ ulRegistryID)
{
    ulRegistryID = m_ulRegistryID;
   
    return HXR_OK;
}

STDMETHODIMP
HXStream::GetProperties(REF(IHXValues*) pProps)
{
    HX_RESULT	    rc = HXR_OK;
    STREAM_INFO*    pStreamInfo = NULL;    

    if (!m_pSource || HXR_OK != m_pSource->GetStreamInfo(m_uStreamNumber, pStreamInfo))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pProps = pStreamInfo->m_pStreamProps;
    HX_ADDREF(pProps);

cleanup:

    return rc;
}

STDMETHODIMP
HXStream::SetProperties(IHXValues* pProps)
{
    HX_RESULT	    rc = HXR_OK;
    UINT32	    ulDelay = 0;
    UINT32	    ulDuration = 0;
    STREAM_INFO*    pStreamInfo = NULL;

    if (!m_pSource || HXR_OK != m_pSource->GetStreamInfo(m_uStreamNumber, pStreamInfo))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (HXR_OK == pProps->GetPropertyULONG32("delay", ulDelay))
    {
	m_pSource->UpdateDelay(ulDelay);
    }

    if (HXR_OK == pProps->GetPropertyULONG32("duration", ulDuration))
    {
	m_pSource->UpdateDuration(ulDuration);
    }

    HX_RELEASE(pStreamInfo->m_pStreamProps);

    pStreamInfo->m_pStreamProps = pProps;
    HX_ADDREF(pStreamInfo->m_pStreamProps);

cleanup:

    return rc;
}

HX_RESULT
HXStream::ResetASMSource(IHXASMSource* pASMSource)
{
    HX_RESULT	hr = HXR_OK;

#if defined(HELIX_FEATURE_ASM)
    if (!m_pASMStream)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    hr = m_pASMStream->ResetASMSource(pASMSource);

cleanup:
#endif /* HELIX_FEATURE_ASM */

    return hr;
}

void
HXStream::SetBufferingFullfilled(void)
{
    STREAM_INFO* pStreamInfo = NULL;    

    if (m_pSource &&
	SUCCEEDED(m_pSource->GetStreamInfo(m_uStreamNumber, pStreamInfo)) &&
	pStreamInfo)
    {
	pStreamInfo->BufferingState().Stop();
    }
}

HXBOOL	
HXStream::IsTimeStampDelivery()
{
    HXBOOL bTimeStampDelivered = FALSE;

#if defined(HELIX_FEATURE_ASM)
    if (m_pASMStream)
    {
	bTimeStampDelivered = m_pASMStream->IsTimeStampDelivery();
    }
#endif /* HELIX_FEATURE_ASM */

    return bTimeStampDelivered;
}

void	
HXStream::PostEndTimePacket(IHXPacket* pPacket, HXBOOL& bSentMe, HXBOOL& bEndMe)
{
    bSentMe = TRUE;
    bEndMe = TRUE;

#if defined(HELIX_FEATURE_ASM)
    if (!pPacket || !m_pASMStream)
    {
	goto cleanup;
    }

    m_pASMStream->PostEndTimePacket(pPacket, bSentMe, bEndMe);

cleanup:
#endif /* HELIX_FEATURE_ASM */

    return;
}
    
void
HXStream::ResetASMRuleState(void)
{
#if defined(HELIX_FEATURE_ASM)
    if (m_pASMStream)
    {
	m_pASMStream->ResetASMRuleState();
    }
#endif /* HELIX_FEATURE_ASM */
    return;
}

HXSource*
HXStream::GetHXSource(void)
{
    if (m_pSource)
    {
	m_pSource->AddRef(); 
    }
    return m_pSource;
}

HXBOOL
HXStream::IsSureStream(void)
{
    UINT32 ulThresholds = 0;

#if defined(HELIX_FEATURE_ASM)
    if (m_pASMStream)
    {
	ulThresholds = m_pASMStream->GetNumThresholds();
	if (ulThresholds > 1)
	{
	    return TRUE;
	}
    }
#endif /* HELIX_FEATURE_ASM */

    return FALSE;
}
