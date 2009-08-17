/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramrendr.cpp,v 1.25 2007/02/26 10:55:39 lovish Exp $
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

/******************************************************************************
 * TODO
 *
 * - take care of m_nEventSensitivity assert in pnvideo
 * - mem leak of RAMRenderer under RAM in SMIL
 * - revisit the relation between m_pRendererSiteMap and m_pPlayToAssocList->m_pSite
 * - propertly handle root/region layout hide/show
 * - handle background color repaint
 * - handle resize
 *
******************************************************************************/

#include "ramrender.ver"

// system
#include "hlxclib/time.h"
// include
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxrendr.h"
#include "hxplugn.h"
#include "hxcore.h"
#include "hxwin.h"
#include "hxgroup.h"
#include "hxupgrd.h"
#include "hxver.h"
#include "hxescapeutil.h"
#include "hxstring.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "hxurl.h"
#include "hxslist.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "smiltype.h"
#include "hxmeta.h"
#include "hxrquest.h"
#include "getreqhdr.h"
#include "hxplayvelocity.h"
#include "ramrendr.h"

#ifdef _CARBON
#include "platform/mac/fullpathname.h"
#endif

// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_URL_STRING	    4096
#define MAX_RAM_URLS	    1000

/* We should really define it in a common header file */
#if defined (_WINDOWS ) || defined (WIN32)
#define OS_SEPARATOR_CHAR	'\\'
#define OS_SEPARATOR_STRING	"\\"
#elif defined (_UNIX)
#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"
#elif defined (_MACINTOSH)
#define OS_SEPARATOR_CHAR	':'
#define OS_SEPARATOR_STRING	":"
#endif // defined (_WINDOWS ) || defined (WIN32)

INT32 g_nRefCount_ramr  = 0;

/****************************************************************************
 * 
 *  Function:
 * 
 *	CanUnload()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's if it returns HXR_OK 
 *	then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINT(CanUnload)(void)
{
    return (g_nRefCount_ramr ? HXR_FAIL : HXR_OK);
}

const char* CRAMRenderer::zm_pName	     = "RAM";
const char* CRAMRenderer::zm_pDescription    = "RealNetworks RAM Driver Renderer Plugin";
const char* CRAMRenderer::zm_pCopyright      = HXVER_COPYRIGHT;
const char* CRAMRenderer::zm_pMoreInfoURL    = HXVER_MOREINFO;
const char* CRAMRenderer::zm_pStreamMimeTypes[] = {"application/ram", NULL};

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CRAMRenderer::InitPlugin(IUnknown* /*IN*/ pContext)
{    
    m_pContext = pContext;
    m_pContext->AddRef();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CRAMRenderer::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetRendererInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CRAMRenderer::GetRendererInfo
(
    REF(const char**) /*OUT*/ pStreamMimeTypes,
    REF(UINT32)      /*OUT*/ unInitialGranularity
)
{
    pStreamMimeTypes = zm_pStreamMimeTypes;
    unInitialGranularity = 0;

    return HXR_OK;
}

CRAMRenderer::CRAMRenderer()
	: m_lRefCount(0)
	, m_pContext(NULL)
	, m_pStream(NULL)
	, m_pPlayer(NULL)
	, m_bRAMProcessed(FALSE)
	, m_bFirstTrack(TRUE)
	, m_pURLFragment(NULL)
	, m_ulGroupIndex(0)
	, m_ulPersistentComponentID(0)
	, m_uPersistentGroupID(0)
	, m_uPersistentTrackID(0)
	, m_uGroupIndexWithin(0)
	, m_ulPersistentComponentDelay(0)
	, m_ulPersistentComponentDuration(0)
	, m_ulPersistentVersion(0)
	, m_persistentType(PersistentRAM)
	, m_elementWithinTag(WithinUnknown)
	, m_pTrackMap(NULL)
	, m_pPlayToAssocList(NULL)
	, m_pPersistentProperties(NULL)
	, m_pStreamProperties(NULL)
	, m_pPersistentParentRenderer(NULL)
	, m_pPersistentComponentManager(NULL)
        , m_pOriginalURL(NULL)
{
    g_nRefCount_ramr++;
}

CRAMRenderer::~CRAMRenderer()
{
    g_nRefCount_ramr--;

    Cleanup();

    HX_VECTOR_DELETE(m_pURLFragment);
    HX_RELEASE(m_pContext);
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CRAMRenderer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRenderer))
    {
	AddRef();
	*ppvObj = (IHXRenderer*)this;
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_NESTEDMETA)
    else if (IsEqualIID(riid, IID_IHXPersistentRenderer))
    {
	AddRef();
	*ppvObj = (IHXPersistentRenderer*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRendererAdviseSink))
    {
	AddRef();
	*ppvObj = (IHXRendererAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGroupSink))
    {
	AddRef();
	*ppvObj = (IHXGroupSink*)this;
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_NESTEDMETA */
    else if (IsEqualIID(riid, IID_IHXPlaybackVelocity))
    {
	AddRef();
	*ppvObj = (IHXPlaybackVelocity*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CRAMRenderer::AddRef()
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
STDMETHODIMP_(ULONG32) CRAMRenderer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXRenderer methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::StartStream
//  Purpose:
//	Called by client engine to inform the renderer of the stream it
//	will be rendering. The stream interface can provide access to
//	its source or player. This method also provides access to the 
//	primary client controller interface.
//
STDMETHODIMP CRAMRenderer::StartStream
(
    IHXStream*	    pStream,
    IHXPlayer*	    pPlayer
)
{
    HX_RESULT			rc = HXR_OK;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    m_pStream  = pStream;
    m_pStream->AddRef();

    m_pPlayer  = pPlayer;
    m_pPlayer->AddRef();

#if defined(HELIX_FEATURE_NESTEDMETA)
    if(HXR_OK == m_pPlayer->QueryInterface(IID_IHXPersistentComponentManager, 
					   (void**)&m_pPersistentComponentManager))
    {
	m_pPersistentComponentManager->CreatePersistentComponent(pPersistentComponent);

	pPersistentComponent->Init((IHXPersistentRenderer*)this);
	pPersistentComponent->AddRendererAdviseSink((IHXRendererAdviseSink*)this);
	pPersistentComponent->AddGroupSink((IHXGroupSink*)this);

	rc = m_pPersistentComponentManager->AddPersistentComponent(pPersistentComponent);
    }
#endif /* HELIX_FEATURE_NESTEDMETA */


    // /See if there is an OriginalURL header which would mean we're rendering
    // a browser-cached file:
    UINT32 rulValue = 0;
    // /Returns HX_RESULT, but no need to check for failure because
    // "OriginalURL" req hdr does not exist in many cases.  Ignore
    // rulValue in this case, as well:
    GetRequestHeader(pPlayer, "OriginalURL", rulValue, m_pOriginalURL);

    GeneratePreFix();

    return rc;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::EndStream
//  Purpose:
//	Called by client engine to inform the renderer that the stream
//	is was rendering is closed.
//
STDMETHODIMP CRAMRenderer::EndStream()
{
#if defined(HELIX_FEATURE_NESTEDMETA)
    IHXPersistentComponent* pPersistentComponent = NULL;

    if (m_pPersistentComponentManager &&
	HXR_OK == m_pPersistentComponentManager->GetPersistentComponent(m_ulPersistentComponentID,
	    								pPersistentComponent))
    {
	pPersistentComponent->RemoveRendererAdviseSink((IHXRendererAdviseSink*)this);
	pPersistentComponent->RemoveGroupSink((IHXGroupSink*)this);
    }
    HX_RELEASE(pPersistentComponent);
#endif /* HELIX_FEATURE_NESTEDMETA */

    Cleanup();
    
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnHeader
//  Purpose:
//	Called by client engine when a header for this renderer is 
//	available. The header will arrive before any packets.
//
STDMETHODIMP CRAMRenderer::OnHeader(IHXValues* pHeader)
{    
    HX_RESULT	rc = HXR_OK;
    UINT32	ulParentVersion = 0;
    UINT32	ulElementAsUINT = 0;
    IHXValues*	pProperties = NULL;

    if (pHeader)
    {
	pHeader->GetPropertyULONG32("PersistentVersion", m_ulPersistentVersion);
    }

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_pPersistentParentRenderer)
    {
	if (!IsNestedMetaSupported())
	{
	    rc = HXR_INVALID_METAFILE;
	    goto cleanup;
	}

	if (HXR_OK == m_pPersistentParentRenderer->GetElementProperties(m_uPersistentGroupID,
									m_uPersistentTrackID,
									m_pPersistentProperties))
	{
	    m_pPersistentProperties->GetPropertyULONG32("Delay", m_ulPersistentComponentDelay);
	    m_pPersistentProperties->GetPropertyULONG32("Duration", m_ulPersistentComponentDuration);
	    m_pPersistentProperties->GetPropertyULONG32("ElementWithinTag", ulElementAsUINT);
	    m_elementWithinTag = (ElementWithinTag)ulElementAsUINT;
	}

	m_elementWithinTag = AdjustElementWithinTag(m_elementWithinTag);
    }

cleanup:
#endif /* HELIX_FEATURE_NESTEDMETA */

    return rc;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnPacket
//  Purpose:
//	Called by client engine when a packet for this renderer is 
//	due.
//
STDMETHODIMP CRAMRenderer::OnPacket(IHXPacket* pPacket, 
				    LONG32 lTimeOffset)
{
    HX_RESULT		rc = HXR_OK;

    HX_ASSERT(!m_bRAMProcessed);
    HX_ASSERT(lTimeOffset <= 0);

    IHXBuffer* pBuffer = pPacket->GetBuffer();
    if(pBuffer)
    {
	rc = ProcessRAM(pBuffer);
    }
    HX_RELEASE(pBuffer);

    return rc;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnTimeSync
//  Purpose:
//	Called by client engine to inform the renderer of the current
//	time relative to the streams synchronized time-line. The 
//	renderer should use this time value to update its display or
//	render it's stream data accordingly.
//
STDMETHODIMP CRAMRenderer::OnTimeSync(ULONG32 ulTime)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnPreSeek
//  Purpose:
//	Called by client engine to inform the renderer that a seek is
//	about to occur. The render is informed the last time for the 
//	stream's time line before the seek, as well as the first new
//	time for the stream's time line after the seek will be completed.
//
STDMETHODIMP CRAMRenderer::OnPreSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnPostSeek
//  Purpose:
//	Called by client engine to inform the renderer that a seek has
//	just occured. The render is informed the last time for the 
//	stream's time line before the seek, as well as the first new
//	time for the stream's time line after the seek.
//
STDMETHODIMP CRAMRenderer::OnPostSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnPause
//  Purpose:
//	Called by client engine to inform the renderer that a pause has
//	just occured. The render is informed the last time for the 
//	stream's time line before the pause.
//
STDMETHODIMP CRAMRenderer::OnPause(ULONG32 ulTime)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnBegin
//  Purpose:
//	Called by client engine to inform the renderer that a begin or
//	resume has just occured. The render is informed the first time 
//	for the stream's time line after the resume.
//
STDMETHODIMP CRAMRenderer::OnBegin(ULONG32 ulTime)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnBuffering
//  Purpose:
//	Called by client engine to inform the renderer that buffering
//	of data is occuring. The render is informed of the reason for
//	the buffering (start-up of stream, seek has occured, network
//	congestion, etc.), as well as percentage complete of the 
//	buffering process.
//
STDMETHODIMP CRAMRenderer::OnBuffering(ULONG32 ulFlags, UINT16 unPercentComplete)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::GetDisplayType
//  Purpose:
//	Called by client engine to ask the renderer for it's preferred
//	display type. When layout information is not present, the 
//	renderer will be asked for it's prefered display type. Depending
//	on the display type a buffer of additional information may be 
//	needed. This buffer could contain information about preferred
//	window size.
//
STDMETHODIMP CRAMRenderer::GetDisplayType
(
    REF(HX_DISPLAY_TYPE)   ulFlags,
    REF(IHXBuffer*)	    pBuffer
)
{
    ulFlags = HX_DISPLAY_NONE;

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXRenderer::OnEndofPackets
 *	Purpose:
 *	    Called by client engine to inform the renderer that all the
 *	    packets have been delivered. However, if the user seeks before
 *	    EndStream() is called, renderer may start getting packets again
 *	    and the client engine will eventually call this function again.
 */
STDMETHODIMP CRAMRenderer::OnEndofPackets(void)
{
    return HXR_OK;
}

#if defined(HELIX_FEATURE_NESTEDMETA)
// IHXGroupSink methods
STDMETHODIMP
CRAMRenderer::GroupAdded(UINT16 uGroupIndex, 
			 IHXGroup* pGroup)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::GroupRemoved(UINT16 uGroupIndex, 
			   IHXGroup* pGroup)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::AllGroupsRemoved()
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::TrackAdded(UINT16 uGroupIndex, 
			 UINT16 uTrackIndex, 
			 IHXValues* pTrack)
{
    HX_RESULT	    rc = HXR_OK;
    UINT32	    ulDelay = 0;
    UINT32	    ulDuration = 0;
    const char*	    pszID = NULL;
    IHXBuffer*	    pBuffer = NULL;
    RAMPlayToAssoc* pPlayToAssoc = NULL;

    if (!pTrack)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pPlayToAssoc		= new RAMPlayToAssoc;
    pPlayToAssoc->m_uGroupIndex	= uGroupIndex;
    pPlayToAssoc->m_uTrackIndex	= uTrackIndex;
    pPlayToAssoc->m_ulDelay	= 0;
    pPlayToAssoc->m_ulDuration	= 0;

    if (HXR_OK == pTrack->GetPropertyULONG32("Delay", ulDelay))
    {
	pPlayToAssoc->m_ulDelay = ulDelay;
    }

    if (HXR_OK == pTrack->GetPropertyULONG32("Duration", ulDuration))
    {
	pPlayToAssoc->m_ulDuration = ulDuration;
    }

    if(HXR_OK == pTrack->GetPropertyCString("id", pBuffer))
    {
        pszID = (const char*)pBuffer->GetBuffer();
	pPlayToAssoc->m_id = pszID;
    }
    HX_RELEASE(pBuffer);

    if (!m_pPlayToAssocList)
    {
	m_pPlayToAssocList = new CHXSimpleList;
    }

    m_pPlayToAssocList->AddTail(pPlayToAssoc);

cleanup:

    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::TrackRemoved(UINT16 uGroupIndex, 
			   UINT16 uTrackIndex, 
			   IHXValues* pTrack)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::TrackStarted(UINT16 uGroupIndex, 
			   UINT16 uTrackIndex, 
			   IHXValues* pTrack)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::TrackStopped(UINT16 uGroupIndex, 
			   UINT16 uTrackIndex, 
			   IHXValues* pTrack)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::CurrentGroupSet(UINT16 uGroupIndex, 
			      IHXGroup* pGroup)
{
    return HXR_OK;
}

// IHXRendererAdviseSink methods
STDMETHODIMP
CRAMRenderer::TrackDurationSet(UINT32 ulGroupIndex, 
			       UINT32 ulTrackIndex,
                               UINT32 ulDuration,   
			       UINT32 ulDelay,
			       HXBOOL   bIsLive)
{
    HX_RESULT		rc = HXR_OK;
    UINT16		uTrackIndex = 0;
    UINT16		uID = 0;
    char		szID[128] = {0}; /* Flawfinder: ignore */
    char*		pTrack = NULL;
    IHXBuffer*		pBuffer = NULL;
    IHXValues*		pTrackProperties = NULL;
    IHXGroup*		pGroup = NULL;
    IHXGroupManager*	pGroupManager = NULL;
    IHXRendererAdviseSink* pRendererAdviseSink = NULL;
    RAMPlayToAssoc* pPlayToAssoc = NULL;

    if (WithinSeqInPar != m_elementWithinTag || bIsLive)
    {
	goto cleanup;
    }

    pPlayToAssoc = GetPlayToAssoc((UINT16)ulGroupIndex, (UINT16)ulTrackIndex);

    if (pPlayToAssoc && m_pTrackMap)
    {
	// m_id is in the format of "<group_id>_<track_id>"
	UINT64 state = 0;
	uID = atoi((const char*)(pPlayToAssoc->m_id.GetNthField('_', 2, state)));
	uTrackIndex = uID + 1;

	if (uTrackIndex < (UINT32)m_pTrackMap->GetCount())
	{	
	    // check for maximum number of tracks
	    if (uTrackIndex > MAX_RAM_URLS)
	    {
		goto cleanup;
	    }

	    if (HXR_OK == m_pContext->QueryInterface(IID_IHXGroupManager, (void**)&pGroupManager))
	    {
		pGroupManager->GetGroup(m_uGroupIndexWithin, pGroup);
    
                pTrack = (char*)(const char*)(*(CHXString*)(*m_pTrackMap)[uTrackIndex]);
		if (HXR_OK == PrepareTrack(pTrack, pTrackProperties))
		{
		    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
		    {
			sprintf(szID, "%lu_%lu", m_uGroupIndexWithin, uTrackIndex); /* Flawfinder: ignore */
			pBuffer->Set((UCHAR*)szID, strlen(szID)+1);
			pTrackProperties->SetPropertyCString("id", pBuffer);
			HX_RELEASE(pBuffer);
		    }

		    pTrackProperties->SetPropertyULONG32("Delay", ulDuration);

		    pGroup->AddTrack(pTrackProperties);
		}
		HX_RELEASE(pTrackProperties);
		HX_RELEASE(pGroup);
	    }
	    HX_RELEASE(pGroupManager);
	}
	else
	{
	    HX_ASSERT(uTrackIndex == m_pTrackMap->GetCount());
	    
	    if (m_pPersistentParentRenderer &&
		HXR_OK == m_pPersistentParentRenderer->QueryInterface(IID_IHXRendererAdviseSink, (void**)&pRendererAdviseSink))
	    {
		if (m_ulPersistentComponentDuration)
		{
		    HX_ASSERT(m_ulPersistentComponentDuration == ulDuration);
		}

		rc = pRendererAdviseSink->TrackDurationSet(m_uPersistentGroupID,
							   m_uPersistentTrackID,
							   ulDuration,
							   m_ulPersistentComponentDelay,
							   bIsLive);
	    }
	    HX_RELEASE(pRendererAdviseSink);
	}
    }

cleanup:

    return rc;
}

STDMETHODIMP
CRAMRenderer::RepeatedTrackDurationSet(const char* pID, 
				       UINT32 ulDuration,
                                       HXBOOL bIsLive)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::TrackUpdated(UINT32 ulGroupIndex, 
			   UINT32 ulTrackIndex,
			   IHXValues* pValues)
{
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::RendererInitialized(IHXRenderer* pRend, 
				  IUnknown* pStream,
				  IHXValues* pInfo)
{
    HX_RESULT		rc = HXR_OK;
    HXBOOL		bIsWindowed = FALSE;
    HX_DISPLAY_TYPE	ulFlags = HX_DISPLAY_NONE;
    RAMPlayToAssoc*	pPlayToAssoc = NULL;
    IHXBuffer*		pBuffer = NULL;

    UINT32		ulGroupIndex = 0;
    UINT32		ulTrackIndex = 0;
    UINT32		ulDelay = 0;
    UINT32		ulDuration = 0;

    pInfo->GetPropertyULONG32("GroupIndex", ulGroupIndex);
    pInfo->GetPropertyULONG32("TrackIndex", ulTrackIndex);
    pInfo->GetPropertyULONG32("Delay", ulDelay);
    pInfo->GetPropertyULONG32("Duration", ulDuration);  

    if (HXR_OK == pInfo->GetPropertyCString("id", pBuffer))
    {
	pPlayToAssoc = GetPlayToAssocByMedia((const char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    if (!pPlayToAssoc)
    {
	// XXX HP we shouldn't be here
	HX_ASSERT(FALSE);	
	pPlayToAssoc = GetPlayToAssoc((UINT16)ulGroupIndex, (UINT16)ulTrackIndex);
    }
 
    if (pPlayToAssoc)
    {
	pPlayToAssoc->m_ulDelay = ulDelay;
	pPlayToAssoc->m_ulDuration = ulDuration;
    }

    if (HXR_OK == pRend->GetDisplayType(ulFlags, pBuffer) &&
	HX_DISPLAY_WINDOW == (HX_DISPLAY_WINDOW & ulFlags))
    {
	bIsWindowed = TRUE;
    }
    HX_RELEASE(pBuffer);

    if (bIsWindowed && m_pPersistentParentRenderer)
    {
	m_pPersistentParentRenderer->AttachElementLayout(m_uPersistentGroupID,
	    						 m_uPersistentTrackID,
							 pRend,
							 (IHXStream*)pStream,
							 pInfo);
    }

    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::RendererClosed(IHXRenderer* pRend, 
			     IHXValues* pInfo)
{
    return HXR_OK;
}

// IHXPersistentRenderer methods
STDMETHODIMP
CRAMRenderer::InitPersistent(UINT32			ulPersistentComponentID,
			     UINT16			uPersistentGroupID,
			     UINT16			uPersistentTrackID,
			     IHXPersistentRenderer*	pPersistentParent)
{
    m_ulPersistentComponentID = ulPersistentComponentID;
    m_uPersistentGroupID = uPersistentGroupID;
    m_uPersistentTrackID = uPersistentTrackID;
    
    m_pPersistentParentRenderer = pPersistentParent;
    HX_ADDREF(m_pPersistentParentRenderer);

    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::GetPersistentID(REF(UINT32) ulPersistentComponentID)
{
    ulPersistentComponentID = m_ulPersistentComponentID;
    return HXR_OK;
}

STDMETHODIMP
CRAMRenderer::GetPersistentProperties(REF(IHXValues*) pProperties)
{
    HX_RESULT	rc = HXR_OK;

    rc = CreateValuesCCF(pProperties, m_pContext);
    if (HXR_OK == rc)
    {
	pProperties->SetPropertyULONG32("PersistentType", m_persistentType);
	pProperties->SetPropertyULONG32("PersistentVersion", m_ulPersistentVersion);
    }

    return rc;
}

STDMETHODIMP
CRAMRenderer::GetElementProperties(UINT16	    uGroupID,
				   UINT16	    uTrackID,
                               	   REF(IHXValues*) pProperties)
{
    HX_RESULT		rc = HXR_OK;
    RAMPlayToAssoc*	pPlayToAssoc = NULL;
    ElementWithinTag	elementWithinTag = WithinUnknown;
    
    rc = CreateValuesCCF(pProperties, m_pContext);
    if (HXR_OK == rc)
    {
	elementWithinTag = AdjustElementWithinTag(m_elementWithinTag);
	pProperties->SetPropertyULONG32("ElementWithinTag", elementWithinTag);

	pPlayToAssoc = GetPlayToAssoc(uGroupID, uTrackID);
	HX_ASSERT(pPlayToAssoc);

	if (pPlayToAssoc)
	{
	    if (pPlayToAssoc->m_ulDelay)
	    {
		pProperties->SetPropertyULONG32("Delay", pPlayToAssoc->m_ulDelay);
	    }

	    if (pPlayToAssoc->m_ulDuration)
	    {	
		pProperties->SetPropertyULONG32("Duration", pPlayToAssoc->m_ulDuration);
	    }
	}
    }

    return rc;
}
	
STDMETHODIMP
CRAMRenderer::AttachElementLayout(UINT16	uGroupID,
				  UINT16	uTrackID,
				  IHXRenderer*	pRenderer,
				  IHXStream*	pStream,
				  IHXValues*	pProps)
{
    HX_RESULT               rc = HXR_OK;
    UINT32                  ulPersistentType = 0;
    
    if (!pRenderer)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }
    
    if (m_pPersistentParentRenderer)
    {
        rc = m_pPersistentParentRenderer->AttachElementLayout(m_uPersistentGroupID,
                                                              m_uPersistentTrackID,
                                                              pRenderer,
                                                              pStream,
                                                              pProps);
    }
    else if (pProps &&
	     HXR_OK == pProps->GetPropertyULONG32("PersistentType", ulPersistentType))
    {
        // layout site is only needed for the SMIL renderer
        if (PersistentSMIL == ulPersistentType)
        {
            IHXLayoutSiteGroupManager* pLSGMgr = NULL;
            if (HXR_OK == m_pContext->QueryInterface(IID_IHXLayoutSiteGroupManager, (void**)&pLSGMgr))
            {
                rc = pLSGMgr->AddLayoutSiteGroup((IUnknown*)pRenderer);
            }
            HX_RELEASE(pLSGMgr);
        }
    }
    
cleanup:

    return rc;
}

STDMETHODIMP
CRAMRenderer::DetachElementLayout(IUnknown* pLSG)
{
    HX_RESULT rc = HXR_OK;

    if (m_pPersistentParentRenderer)
    {
	rc = m_pPersistentParentRenderer->DetachElementLayout(pLSG);
    }
    else
    {
	IHXLayoutSiteGroupManager* pLSGMgr = NULL;

	if(HXR_OK == m_pContext->QueryInterface(IID_IHXLayoutSiteGroupManager, (void**)&pLSGMgr))
	{
	    rc = pLSGMgr->RemoveLayoutSiteGroup(pLSG);
	}
	HX_RELEASE(pLSGMgr);
    }

    return rc;
}

STDMETHODIMP
CRAMRenderer::GetElementStatus(UINT16		uGroupID,
			       UINT16		uTrackID,
			       UINT32		ulCurrentTime,
			       REF(IHXValues*)	pStatus)
{
    pStatus = NULL;
    return HXR_NOTIMPL;
}
#endif /* HELIX_FEATURE_NESTEDMETA */

HX_RESULT
CRAMRenderer::ProcessRAM(IHXBuffer* pRAMBuffer)
{
    HX_RESULT		rc = HXR_INVALID_METAFILE;    
    UINT16		uTrackID = 0;
    UINT16		uTotalTracks = 0;
    UINT16		uTrackIndex = 0;
    UINT16		uRAMVersion = 0;
    HXBOOL		bRAMVersionSectionEnded = TRUE;
    int			i = 0; 
    int			j = 0;
    int			iLen = 0;
    int			iSize = 0;
    int			iNumURLs = 0;
    char		szID[128] = {0}; /* Flawfinder: ignore */
    char*		pUrl = NULL; 
    char*		pContent = NULL;
    IHXValidator*	pValidator = NULL;
    IHXGroupManager*	pGroupManager = NULL;
    IHXGroup*		pGroup = NULL;
    IHXGroup2*		pGroup2 = NULL;
    IHXBuffer*		pBuffer = NULL;
    IHXValues*		pGroupProperties = NULL;
    IHXValues*		pTrackProperties = NULL;

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXValidator, (void**)&pValidator) ||
	HXR_OK != m_pContext->QueryInterface(IID_IHXGroupManager, (void**)&pGroupManager))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pContent = (char*) pRAMBuffer->GetBuffer();
    iLen = pRAMBuffer->GetSize();   
    pUrl = new char[iLen + 1];

    while( i <= iLen )
    {		
	pUrl[j] = pContent[i]; 
	
	// Look for line terminators.
	if (pUrl[j] == '\n' || pUrl[j] == '\r' || pUrl[j] == 0 || i == iLen)
	{
	    pUrl[j] = 0;

	    CHXString* pString = new CHXString(pUrl);

	    pString->TrimLeft();
	    pString->TrimRight();

	    iSize = pString->GetLength();
	    if (iSize)
	    {
		memset(pUrl, 0, iLen + 1);
		SafeStrCpy(pUrl,  pString->GetBuffer(iSize), iLen+1);

		// A URL must have at least 4 chars. for protocol. This will
		// take care of lines with junk or just CR on them.
		
		if (strncasecmp(pUrl, HX_RAM30_START_TAG, HX_RAM30_START_TAGSIZE) == 0)
		{
		    uRAMVersion = 3;
		    // no nested version sections allowed
		    if (!bRAMVersionSectionEnded)
		    {
			HX_DELETE(pString);
			rc = HXR_INVALID_METAFILE;
			goto cleanup;
		    }
		    bRAMVersionSectionEnded = FALSE;
		}
		else if (strncasecmp(pUrl, HX_RAM30_END_TAG, HX_RAM30_END_TAGSIZE) == 0)
		{		    
		    HX_DELETE(pString);
		    bRAMVersionSectionEnded = TRUE;
		    break;
		}
		else if (strncasecmp(pUrl, HX_RAM20_START_TAG, HX_RAM20_START_TAGSIZE) == 0)
		{
		    uRAMVersion = 2;		    
		    // no nested version sections allowed
		    if (!bRAMVersionSectionEnded)
		    {
			HX_DELETE(pString);
			rc = HXR_INVALID_METAFILE;
			goto cleanup;
		    }
		    bRAMVersionSectionEnded = FALSE;
		}
		// we mis-documented the use of "## .RAM_V2.0_STOP"
		else if (strncasecmp(pUrl, HX_RAM20_END_TAG, HX_RAM20_END_TAGSIZE) == 0 ||
		         strncasecmp(pUrl, "## .RAM_V2.0_STOP", 21) == 0)
		{
    		    HX_DELETE(pString);
		    bRAMVersionSectionEnded = TRUE;
		    break;
		}
		// handle "--stop--" tag in 6.0
		else if (strncasecmp(pUrl, "--stop--", 8) == 0)
		{
		    HX_DELETE(pString);
		    break;
		}
		else
		{
		    if (uRAMVersion >= 2)
		    {
			// a URL must have at least 4 chars + "##"
			if (strncasecmp(pUrl, HX_RAM_ENTRY_TAG, HX_RAM_ENTRY_TAGSIZE) == 0 && 
			    (iSize >= (HX_RAM_ENTRY_TAGSIZE + 4)))
			{
			    CHXString* pStringAfterTag = new CHXString(pUrl+2);

			    pStringAfterTag->TrimLeft();
			    pStringAfterTag->TrimRight();
    
			    iSize = pStringAfterTag->GetLength();
			    if (iSize)
			    {				
				memset(pUrl, 0, iLen);
				SafeStrCpy(pUrl,  pStringAfterTag->GetBuffer(iSize), iLen+1);
			    }
			    HX_DELETE(pStringAfterTag);
			}
			else
			{
			    memset(pUrl, 0, iLen);
			}			    
		    }

		    char*   pTrack = NULL;
		    char*   pProtocol = NULL;
		    char*   pCursor = NULL;
		    UINT32  ulProtocol = 0;
	    
		    pCursor = strstr(pUrl, ":");
		    if (pCursor)
		    {
			ulProtocol = pCursor - pUrl;

			if (ulProtocol > 0)
			{
			    pProtocol = new char[ulProtocol+1];
			    memset(pProtocol, 0, ulProtocol+1);
			    strncpy(pProtocol, pUrl, ulProtocol); /* Flawfinder: ignore */

                            rc = ProcessURL(pUrl);
                            if (HXR_NOTIMPL == rc)
                            {
                                rc = HXR_OK;
                                if (pValidator->ValidateProtocol(pProtocol))
			        {
				    if (iSize >= 8)
				    {
				        if (!m_pTrackMap)
				        {
					    m_pTrackMap = new CHXMapLongToObj;
				        }

				        CHXString* encodedURL = new CHXString();
                                        *encodedURL = pUrl;

				        if (strcasecmp(pProtocol, "pnm") != 0)
				        {//XXXLCM
                                            HXEscapeUtil::EnsureEscapedURL(*encodedURL);
                                        }
                                                                                            
				        (*m_pTrackMap)[uTotalTracks] = encodedURL;
				        uTotalTracks++;
				    }
			        }
			        HX_VECTOR_DELETE(pProtocol);
			    }
                        }
		    }
		}
	    }

	    j = 0;
	    HX_DELETE(pString);
	}
	else
	{	
	    j++; 
	}

	i++;
    }

    // version section is not properly ended 
    if (!bRAMVersionSectionEnded)
    {
	rc = HXR_INVALID_METAFILE;
	goto cleanup;
    }

    // add tracks
    if (m_pTrackMap)
    {	
	char*	    pTrack = NULL;
	CHXMapLongToObj::Iterator i;

	for (uTrackIndex = 0; uTrackIndex < uTotalTracks; uTrackIndex++)
	{
	    pTrack = (char*)(const char*)(*(CHXString*)(*m_pTrackMap)[uTrackIndex]);

	    if (m_bFirstTrack)
	    {
		pGroupManager->GetCurrentGroup(m_uGroupIndexWithin);
		pGroupManager->GetGroup(m_uGroupIndexWithin, pGroup);
	    }
	    else
	    {
		pGroupManager->CreateGroup(pGroup);
	    }

	    if (HXR_OK == PrepareGroup(pGroupProperties))
	    {		
		pGroup->SetGroupProperties(pGroupProperties);
		if(HXR_OK == pGroup->QueryInterface(IID_IHXGroup2, (void**)&pGroup2))
		{    
		pGroup2->SetPersistentComponentProperties(m_ulPersistentComponentID,
							  pGroupProperties);
		}
		if (HXR_OK == PrepareTrack(pTrack, pTrackProperties))
		{
		    if (!m_bFirstTrack)
		    {
			pGroupManager->AddGroup(pGroup);
		    }
		    m_bFirstTrack = FALSE;

		    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
		    {
			SafeSprintf(szID, 128, "%lu_%lu", m_ulGroupIndex, uTrackID); 
			pBuffer->Set((UCHAR*)szID, strlen(szID)+1);
			pTrackProperties->SetPropertyCString("id", pBuffer);
			HX_RELEASE(pBuffer);
		    }

		    if (m_ulPersistentComponentDelay)
		    {
			pTrackProperties->SetPropertyULONG32("Delay", m_ulPersistentComponentDelay);
		    }

		    if (m_ulPersistentComponentDuration)
		    {
			pTrackProperties->SetPropertyULONG32("Duration", m_ulPersistentComponentDuration);
		    }

		    pGroup->AddTrack(pTrackProperties);
		    uTrackID++;
	    
		    // a metafile consists of sequential groups
		    // so create a group with 1 track in it for evey ram file line
		    if (m_elementWithinTag != WithinSeqInPar)
		    {
			uTrackID = 0;
			m_ulGroupIndex++;

			// check for maximum number of tracks
			if (m_ulGroupIndex > MAX_RAM_URLS)
			{
			    break;
			}
		    }
		    // a meta file consists of sequential tracks
		    // so create the first track and the rest of them will
		    // be added in TrackDurationResolved()
		    else
		    {
			break;
		    }
		}
		HX_RELEASE(pTrackProperties);
	    }
	    HX_RELEASE(pGroupProperties);
	    HX_RELEASE(pGroup2);
	    HX_RELEASE(pGroup);
	}
    }

cleanup:

    HX_VECTOR_DELETE(pUrl);

    HX_RELEASE(pTrackProperties);
    HX_RELEASE(pGroupProperties);
    HX_RELEASE(pGroup2);
    HX_RELEASE(pGroup);    
    HX_RELEASE(pGroupManager);
    HX_RELEASE(pValidator);

    m_bRAMProcessed = TRUE;

    if (HXR_OK != rc)
    {
	RemoveTracks();
    }

    return rc;
}

HX_RESULT
CRAMRenderer::PrepareGroup(REF(IHXValues*) pGroupProperties)
{
    HX_RESULT	rc = HXR_OK;

    HX_RELEASE(pGroupProperties);
    
    rc = CreateValuesCCF(pGroupProperties, m_pContext);
    if (HXR_OK == rc)
    {
	pGroupProperties->SetPropertyULONG32("PersistentComponentID", m_ulPersistentComponentID);
	pGroupProperties->SetPropertyULONG32("PersistentGroupID", m_ulGroupIndex);
    }

    return rc;
}

HX_RESULT
CRAMRenderer::PrepareTrack(char* pszURL, REF(IHXValues*) pTrackProperties)
{
    HX_RESULT	rc = HXR_OK;
    char	szID[5] = {0}; /* Flawfinder: ignore */
    CHXURL*	pURL = NULL;
    IHXValues*  pURLOptions = NULL;
    CHXString	urlString;
    IHXBuffer* pBuffer = NULL;

    if (!pszURL)
    {
	rc = HXR_INVALID_PARAMETER;
	goto cleanup;
    }

    HX_RELEASE(pTrackProperties);
    
    rc = CreateValuesCCF(pTrackProperties, m_pContext);
    if (HXR_OK != rc)
    {
	goto cleanup;
    }

    ConvertURL(pszURL, urlString);

    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
    {
	pBuffer->Set((UCHAR*)(const char*)urlString, strlen((const char*)urlString)+1);
	pTrackProperties->SetPropertyCString("src", pBuffer);
	HX_RELEASE(pBuffer);
    }

    pTrackProperties->SetPropertyULONG32("PersistentComponentID", m_ulPersistentComponentID);
    
    // Get any TAC info in URL parameters
    pURL = new CHXURL(pszURL, m_pContext);

    if (pURL->GetLastError() == HXR_OK &&
	((pURLOptions = pURL->GetOptions()) != NULL))
    {
	if (HXR_OK == 
	    pURLOptions->GetPropertyBuffer("Title", pBuffer))
	{
	    pTrackProperties->SetPropertyBuffer("Title", pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (HXR_OK == 
	    pURLOptions->GetPropertyBuffer("Author", pBuffer))
	{
	    pTrackProperties->SetPropertyBuffer("Author", pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (HXR_OK == 
	    pURLOptions->GetPropertyBuffer("Copyright", pBuffer))
	{
	    pTrackProperties->SetPropertyBuffer("Copyright", pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (HXR_OK == 
	    pURLOptions->GetPropertyBuffer("Abstract", pBuffer))
	{
	    pTrackProperties->SetPropertyBuffer("Abstract", pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (HXR_OK == 
	    pURLOptions->GetPropertyBuffer("Keywords", pBuffer))
	{
	    pTrackProperties->SetPropertyBuffer("Keywords", pBuffer);
	    HX_RELEASE(pBuffer);
	}
    }

cleanup:

    HX_RELEASE(pURLOptions);    
    HX_DELETE(pURL);

    return rc;
}

void
CRAMRenderer::GeneratePreFix()
{
    IHXStreamSource* pSource = NULL;

    m_pStream->GetSource(pSource);
    
    HX_ASSERT(pSource);
    if (pSource)
    {
	const char* pURL = pSource->GetURL();
	HX_ASSERT(pURL);
    
	// we only set (and use) these for local ram files.
	if (pURL && !strncasecmp(pURL,"file:",5))
        {
            // /Use original URL if it exists:
            HXBOOL bPrefixGenHandled = FALSE;
            if (m_pOriginalURL)
            {
                const char* pszURL = (const char*)m_pOriginalURL->GetBuffer();
                HX_ASSERT(pszURL);
                if (pszURL)
                {
                    bPrefixGenHandled = TRUE;
                    CHXURL::GeneratePrefixRootFragment(pszURL, m_urlPrefix,
						       m_urlRoot, m_pURLFragment,
						       m_pContext);
                }
            }

            if (!bPrefixGenHandled)
            {
                CHXURL::GeneratePrefixRootFragment(pURL, m_urlPrefix,
						   m_urlRoot, m_pURLFragment,
						   m_pContext);
            }
        }
    }    
    HX_RELEASE(pSource);

    return;
}

HX_RESULT 
CRAMRenderer::ConvertURL(const char* pURL, CHXString& newURL)
{
    HX_RESULT	retVal = HXR_OK;
    HXBOOL	bPartial = FALSE;
    const char*	pszCursor = NULL;

    pszCursor = pURL;

#if defined(_CARBON) || defined(_MAC_UNIX)
    const char *pLochost = "file://localhost/";
    if (!strncasecmp(pszCursor, pLochost, strlen(pLochost)))
    {
    	// it's already a valid URL; don't continue else it would be
    	// misconstrued as a relative URL
    	newURL = pURL;
    	goto cleanup;
    }
#endif

    /* so how do we tell if a full or partial path was specified?
       for now, if 'file:' or 'file://' then assume partial path
       'file:///' or 'file:/' for full path. However file://E: works
       as a full path. 
       file://E:\rmfiles\test.rm - full path
       file:test.rm - look for it in same dir as .rtsl file
       file://hxfiles/test.ra - we assume there is a hxfiles dir in 
       the same dir as the .rtsl file and just prepend full path
     */
    if (!m_urlPrefix.IsEmpty() && !strncasecmp(pszCursor,"file:",5))
    {
	if (!strncasecmp(pszCursor,"file:///",8))
	{    
    	    bPartial = FALSE;
	}
	else if (!strncasecmp(pszCursor,"file://",7))
	{ 	    
    	    bPartial = TRUE;
    	    pszCursor += 7;    	    
	}
	else if (!strncasecmp(pszCursor,"file:/",6))
	{
    	    bPartial = FALSE;
	}
	else
	{    
    	    bPartial = TRUE;
    	    pszCursor += 5;
	}
    }


    if (bPartial)
    {
#ifndef _MACINTOSH
    	//check if its a windows full path
    	const char* pVolumeSep = pszCursor + 1;
	/* There are valid full paths:
	 * file://e:\foo...
	 * file://e|foo...
	 * file://\\oprah\...
	 */
    	if (*pszCursor	    != '\\' && 
	    *pVolumeSep != ':'  && 
	    *pVolumeSep != '\\' &&
	    *pVolumeSep != '|' ) 
#endif
    	{
            // /Note: if m_pOriginalURL exists then m_urlPrefix is from it
            // so that relative URL will be properly resolved:
    	    newURL = m_urlPrefix + pszCursor;
	    goto cleanup;
    	}
    }

    newURL = pURL;

cleanup:

#ifdef _CARBON
    // At this point, we supposedly have a good URL
    //
    // On Mac OS X, this may have failed in assuming that file:/ and file:/// are followed
    // by valid full URLs, which is untrue, so we need to turn the path that follows those
    // into a real URL
    
    int numSlashes = 0;
    
    pszCursor = (const char *) newURL;
    if (!strncasecmp(pszCursor,"file:///", 8)) 		numSlashes = 3;
    else if (!strncasecmp(pszCursor,"file://", 7)) 	numSlashes = 2;
    else if (!strncasecmp(pszCursor,"file:/", 6)) 	numSlashes = 1;
    
    if (numSlashes == 1 || numSlashes == 3)
    {
    	CHXString hfsPath, macURL, params;
    	HXBOOL kReplaceAll = TRUE;
    	
    	// the path starts after the "file:" and slashes
    	hfsPath = (pszCursor + strlen("file:") + numSlashes);
    	
    	// don't include any parameters in the path
    	INT32 paramsOffset = hfsPath.Find('?');
    	if (paramsOffset != -1)
    	{
    	    params = hfsPath.Mid( paramsOffset );    	    
    	    hfsPath = hfsPath.Left( paramsOffset );
    	}
    	
    	// make slashes into colons like an HFS path
    	hfsPath.FindAndReplace("/", ":", kReplaceAll);
    	
    	OSStatus err = URLFromHFSPath((const char *) hfsPath, macURL);
    	if (err == noErr)
    	{
    	    newURL = macURL;
    	    newURL += params;
    	}
    }
#endif

    return retVal;
    
}

ElementWithinTag
CRAMRenderer::AdjustElementWithinTag(ElementWithinTag elementWithinTag)
{
    switch (elementWithinTag)
    {
    case WithinUnknown:
    case WithinSeq:
	return WithinSeq;
    case WithinPar:
    case WithinSeqInPar:
	return WithinSeqInPar;
    default:
	return elementWithinTag;
    }
}

HXBOOL
CRAMRenderer::IsNestedMetaSupported(void)
{
    HXBOOL	    bResult = TRUE;    
    UINT32	    ulParentPersistentVersion = 0;
    UINT32	    ulParentPersistentMajorVersion = 0;
    UINT32	    ulPersistentMajorVersion = 0;
    UINT32	    ulParentPersistentType = PersistentUnknown;
    IHXValues*	    pProperties = NULL;

    HX_ASSERT(m_pPersistentParentRenderer);

    if (HXR_OK == m_pPersistentParentRenderer->GetPersistentProperties(pProperties))
    {
	pProperties->GetPropertyULONG32("PersistentType", ulParentPersistentType);
	pProperties->GetPropertyULONG32("PersistentVersion", ulParentPersistentVersion);

	ulParentPersistentMajorVersion = HX_GET_MAJOR_VERSION(ulParentPersistentVersion);
	ulPersistentMajorVersion = HX_GET_MAJOR_VERSION(m_ulPersistentVersion);

	switch (ulParentPersistentType)
	{
	case PersistentUnknown:
	    bResult = FALSE;
	    break;
	case PersistentSMIL:
	    if (ulParentPersistentMajorVersion == 1)
	    {
		bResult = FALSE;
	    }
	    break;
	default:
	    break;
	}
    }
    HX_RELEASE(pProperties);
    
    return bResult;
}
		
RAMPlayToAssoc*
CRAMRenderer::GetPlayToAssoc(UINT16 uGroupIndex, 
			     UINT16 uTrackIndex)
{
    RAMPlayToAssoc* pPlayToAssoc = NULL;

    if(m_pPlayToAssocList)
    {
	CHXSimpleList::Iterator i;
	for(i=m_pPlayToAssocList->Begin();i!=m_pPlayToAssocList->End();++i)
	{
	    RAMPlayToAssoc* pThisAssoc = (RAMPlayToAssoc*)(*i);
	    if((pThisAssoc->m_uGroupIndex == uGroupIndex) &&
	       (pThisAssoc->m_uTrackIndex == uTrackIndex))
	    {
		pPlayToAssoc = pThisAssoc;
		break;
	    }
	}
    }

    return pPlayToAssoc;
}

RAMPlayToAssoc* 
CRAMRenderer::GetPlayToAssocByMedia(const char* pszMediaID)
{
    RAMPlayToAssoc* pPlayToAssoc = NULL;

    if (pszMediaID && m_pPlayToAssocList)
    {
        LISTPOSITION pos = m_pPlayToAssocList->GetHeadPosition();
        while (pos)
        {
            RAMPlayToAssoc* pListMember =
                (RAMPlayToAssoc*) m_pPlayToAssocList->GetNext(pos);
            if (pListMember && pListMember->m_id == pszMediaID)
            {
                pPlayToAssoc = pListMember;
                break;
            }
        }
     }
 
     return pPlayToAssoc;
}

void
CRAMRenderer::RemoveTracks()
{
    if (m_pTrackMap)
    {
	CHXMapLongToObj::Iterator i;
	for(i = m_pTrackMap->Begin(); i != m_pTrackMap->End();++i)
	{
	    CHXString* pTrack = (CHXString*)(*i);
	    HX_DELETE(pTrack);
	}
	HX_DELETE(m_pTrackMap);
    }
}

void
CRAMRenderer::RemoveAllPlayToAssoc()
{
    if(m_pPlayToAssocList)
    {
	CHXSimpleList::Iterator i = m_pPlayToAssocList->Begin();
	for(; i != m_pPlayToAssocList->End(); ++i)
	{
	    RAMPlayToAssoc* pPlayToAssoc = (RAMPlayToAssoc*)(*i);
	    HX_DELETE(pPlayToAssoc);
	}
    }
    HX_DELETE(m_pPlayToAssocList);
}

void
CRAMRenderer::Cleanup(void)
{
    RemoveTracks();
    RemoveAllPlayToAssoc();
    
    HX_RELEASE(m_pPersistentProperties);
    HX_RELEASE(m_pStreamProperties);
    HX_RELEASE(m_pPersistentParentRenderer);
    HX_RELEASE(m_pPersistentComponentManager);
    HX_RELEASE(m_pStream);
    HX_RELEASE(m_pPlayer);
    
    HX_RELEASE(m_pOriginalURL);

    return;
}    
