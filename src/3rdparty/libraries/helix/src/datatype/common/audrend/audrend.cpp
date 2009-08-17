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

// #define _AUDREND_FLOW_LOG

#define TIME_SYNC_GRANULARITY	100	// in milliseconds
#define MAX_AUDIO_WRITE_TIME	200	// in milliseconds

#define STREAM_MAJOR_VERSION  0
#define STREAM_MINOR_VERSION  0

#define CONTENT_MAJOR_VERSION 0
#define CONTENT_MINOR_VERSION 0

#define TIME_FUDGE 5

#define BASE_AUDIO_RENDERER_NAME    "Basic Audio"
#define MINIMAL_AUDIO_PUSHDOWN 120

#ifdef _AUDREND_FLOW_LOG
#define AUDREND_FLOW_FILE	    "C:\\audrend.txt"
#else	// _AUDREND_FLOW_LOG
#define AUDREND_FLOW_FILE	    NULL
#endif	// _AUDREND_FLOW_LOG


/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "audrend.ver"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxcore.h"
#include "hxrendr.h"
#include "hxplugn.h"
#include "hxasm.h"
#include "hxver.h"
#include "hxupgrd.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxerror.h"
#include "hxausvc.h"
#include "hxthread.h"
#include "hxstrutl.h"
#include "hxtlogutil.h"

#include "hxtick.h"
#include "hxassert.h"
#include "hxbuffer.h"
#include "addupcol.h"
#include "errdbg.h"
#include "adjtime.h"

#include "hxver.h"

#include "audrend.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif	// 	HX_THIS_FILE


/****************************************************************************
 *  Constants
 */
const char* const CAudioRenderer::zm_pDescription    = "RealNetworks Audio Renderer Plugin";
const char* const CAudioRenderer::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const CAudioRenderer::zm_pMoreInfoURL    = HXVER_MOREINFO;
const char* const CAudioRenderer::zm_pStreamMimeTypes[] =
{
    "audio/NULL",
    NULL
};
const char* const CAudioRenderer::zm_pAudioStreamType[AUDREND_NUM_STREAMTYPESTRINGS] =
{
    "STREAMING_AUDIO",                // = 0
    "INSTANTANEOUS_AUDIO",            // = 1
    "TIMED_AUDIO",                    // = 2
    "STREAMING_INSTANTANEOUS_AUDIO"   // = 3
};

/************************************************************************
 *  CAudioRenderer
 */
/************************************************************************
 *  Constructor/Destructor
 */
CAudioRenderer::CAudioRenderer()
	: m_pAudioPlayer(NULL)
	, m_ppAudioStream(NULL)
	, m_ulNumAudioStreams(0)
	, m_ulCurAudioStream(0)
        , m_ulNumAudioStreamPtrAlloc(0)
	, m_ulPreroll(0)
	, m_ulDelay(0)
	, m_ulDuration(0)
	, m_ulAudioWantedTime(NO_TIME_SET)
	, m_ulLastWriteTime(NO_TIME_SET)
	, m_lTimeOffset(0)
	, m_PlayState(stopped)
	, m_bDoneWritingPackets(FALSE)
	, m_bEndOfPackets(FALSE)
	, m_bProcessingPacket(FALSE)
	, m_bInSeekMode(FALSE)
	, m_bFirstPacket(TRUE)
	, m_bDelayOffsetSet(FALSE)
	, m_bCanChangeAudioStream(FALSE)
	, m_bNeedStartTime(TRUE)
	, m_pMutex(NULL)
        , m_pSrcProps(NULL)
	, m_pContext(NULL)
	, m_pStream(NULL)
	, m_pBackChannel(NULL)
	, m_pHeader(NULL)
	, m_pErrorMessages(NULL)
	, m_pCommonClassFactory(NULL)
	, m_pPreferences(NULL)
	, m_pRegistry(NULL)
	, m_ulRegistryID(0)
	, m_pAudioFormat(NULL)
        , m_pAudioStats(NULL)
	, m_lRefCount(0)
{ 
    // Allocate space for an initial number of pointers
    m_ppAudioStream = new IHXAudioStream* [AUDREND_INITIAL_NUMSTREAMPTRS];
    if (m_ppAudioStream)
    {
        memset((void*) m_ppAudioStream, 0, AUDREND_INITIAL_NUMSTREAMPTRS * sizeof(IHXAudioStream*));
        m_ulNumAudioStreamPtrAlloc = AUDREND_INITIAL_NUMSTREAMPTRS;
    }
}

CAudioRenderer::~CAudioRenderer()
{
#if defined(HELIX_FEATURE_STATS)
    HX_DELETE(m_pAudioStats); 
#endif /* HELIX_FEATURE_STATS */
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pSrcProps);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pHeader);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pRegistry);
}


/************************************************************************
 *  IHXPlugin Methods
 */
/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CAudioRenderer::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pContext);

    m_pContext = pContext;
    m_pContext->AddRef();

    retVal = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					(void**) &m_pCommonClassFactory);

    if (m_pCommonClassFactory)
    {
        m_pCommonClassFactory->CreateInstanceAggregatable(CLSID_IHXValues,
                                                          m_pSrcProps,
                                                          (IHXPlugin*)this);
    }

    m_pContext->QueryInterface(IID_IHXPreferences,
                               (void**) &m_pPreferences);

    m_pContext->QueryInterface(IID_IHXRegistry, (void**) &m_pRegistry);

    if (SUCCEEDED(retVal) && !m_pMutex)
    {
	retVal = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    }

#if defined(HELIX_FEATURE_STATS)
    if (SUCCEEDED(retVal))
    {
	m_pAudioStats = new CAudioStatistics(m_pContext);

	retVal = HXR_OUTOFMEMORY;
	if (m_pAudioStats)
	{
	    retVal = HXR_OK;
	}
    }
#endif /* HELIX_FEATURE_STATS */

    // Get the IHXErrorMessages interface. Not an error
    // if it's not supported
    HX_RELEASE(m_pErrorMessages);
    m_pContext->QueryInterface(IID_IHXErrorMessages,
                               (void**) &m_pErrorMessages);

    if (FAILED(retVal))
    {
	HX_RELEASE(m_pCommonClassFactory);
	HX_RELEASE(m_pPreferences);
	HX_RELEASE(m_pRegistry);
    }

    return retVal;
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
STDMETHODIMP CAudioRenderer::GetPluginInfo
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
STDMETHODIMP CAudioRenderer::GetRendererInfo
(
 REF(const char**) /*OUT*/ pStreamMimeTypes,
 REF(UINT32)      /*OUT*/ unInitialGranularity
)
{
    pStreamMimeTypes = (const char**)zm_pStreamMimeTypes;
    unInitialGranularity = TIME_SYNC_GRANULARITY;
    return HXR_OK;
}


// *** IUnknown methods ***
/****************************************************************************
*  IUnknown::QueryInterface                                    ref:  hxcom.h
*
*  This routine indicates which interfaces this object supports. If a given
*  interface is supported, the object's reference count is incremented, and
*  a reference to that interface is returned. Otherwise a NULL object and
*  error code are returned. This method is called by other objects to
*  discover the functionality of this object.
*/
STDMETHODIMP CAudioRenderer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
    {
	{ GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this},
	{ GET_IIDHANDLE(IID_IHXDryNotification), (IHXDryNotification*)this},
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*) this},
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*)this},
	{ GET_IIDHANDLE(IID_IHXRenderer), (IHXRenderer*)this},
#if defined(HELIX_FEATURE_STATS)
	{ GET_IIDHANDLE(IID_IHXStatistics), (IHXStatistics*)this},
#endif	// HELIX_FEATURE_STATS
    };

    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    if ((HXR_NOINTERFACE == res) && m_pSrcProps)
    {
        res = m_pSrcProps->QueryInterface(riid, ppvObj);
    }

    return res;
}

/****************************************************************************
*  IUnknown::AddRef                                            ref:  hxcom.h
*
*  This routine increases the object reference count in a thread safe
*  manner. The reference count is used to manage the lifetime of an object.
*  This method must be explicitly called by the user whenever a new
*  reference to an object is used.
*/
STDMETHODIMP_(ULONG32) CAudioRenderer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*  IUnknown::Release                                           ref:  hxcom.h
*
*  This routine decreases the object reference count in a thread safe
*  manner, and deletes the object if no more references to it exist. It must
*  be called explicitly by the user whenever an object is no longer needed.
*/
STDMETHODIMP_(ULONG32) CAudioRenderer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/************************************************************************
 *  IHXRenderer Methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::StartStream
//  Purpose:
//	Called by client engine to inform the renderer of the stream it
//	will be rendering. The stream interface can provide access to
//	its source or player. This method also provides access to the
//	primary client controller interface.
//
STDMETHODIMP
CAudioRenderer::StartStream
(
    IHXStream*	    pStream,
    IHXPlayer*	    pPlayer
)
{
    HX_RESULT retVal = HXR_OK;

    IHXStream2* pStream2 = NULL;

    if (pStream &&
        HXR_OK == pStream->QueryInterface(IID_IHXStream2, (void**) &pStream2))
    {
        m_pStream = pStream2;
        m_bStream2 = TRUE;
    }
    else
    {
        m_pStream  = pStream;
        HX_ADDREF(m_pStream);
    }
 
    if (m_pStream)
    {
	IHXStreamSource* pSource = 0;
	if (m_pStream->GetSource(pSource) == HXR_OK)
	{
	    /* It is OK if the source does not support backchannel. Reasons:
	     *
	     * 1. This stream may not be coming from h261 fileformat.
	     *	  It may instead be merged into a container fileformat which
	     *	  may be does not support BackChannel.
	     *
	     * 2. The protocol used to serve this stream may not support
	     *	  BackChannel.
	     */
	    pSource->QueryInterface(IID_IHXBackChannel, (void**) &m_pBackChannel);

	    pSource->Release();
	}
    }

    IHXAudioPushdown2 * pAudioPushdown2 = NULL;

    // get interface to audio player
    if (pPlayer)
    {
	retVal = pPlayer->QueryInterface(IID_IHXAudioPlayer,
					   (void**) &m_pAudioPlayer);
#ifdef HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES
        if( m_pAudioPlayer )
        {
            m_pAudioPlayer->QueryInterface(IID_IHXAudioPushdown2, (void**) &pAudioPushdown2);
            if( pAudioPushdown2 )
            {
                pAudioPushdown2->SetAudioPushdown( MINIMAL_AUDIO_PUSHDOWN );
                pAudioPushdown2->Release();
            }
        }
#endif // HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES
    }

    return retVal;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::EndStream
//  Purpose:
//	Called by client engine to inform the renderer that the stream
//	is was rendering is closed.
//
STDMETHODIMP CAudioRenderer::EndStream()
{
    HX_LOCK(m_pMutex);

    m_bDoneWritingPackets = TRUE;
    m_PlayState = stopped;

    HX_UNLOCK(m_pMutex);

    if (m_pAudioFormat)
    {
        m_pAudioFormat->Release();
        m_pAudioFormat = NULL;
    }
    HX_RELEASE(m_pStream);
    HX_RELEASE(m_pBackChannel);
    HX_RELEASE(m_pAudioPlayer);
    for (UINT32 i = 0; i < m_ulNumAudioStreams; i++)
    {
        HX_RELEASE(m_ppAudioStream[i]);
    }
    HX_VECTOR_DELETE(m_ppAudioStream);
    m_ulNumAudioStreams = 0;

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnHeader
//  Purpose:
//	Called by client engine when a header for this renderer is
//	available. The header will arrive before any packets.
//
STDMETHODIMP
CAudioRenderer::OnHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;
    UINT32 ulTrackStartTime = NO_TIME_SET;
    UINT32 ulTrackEndTime = NO_TIME_SET;

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    // check the stream versions
    pHeader->AddRef();
    retVal = CheckStreamVersions(pHeader);
    pHeader->Release();
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */

    if (SUCCEEDED(retVal))
    {
	pHeader->GetPropertyULONG32("Duration", m_ulDuration);
	pHeader->GetPropertyULONG32("Delay", m_ulDelay);

	pHeader->GetPropertyULONG32("TrackStartTime", ulTrackStartTime);
	pHeader->GetPropertyULONG32("TrackEndTime", ulTrackEndTime);
    }

    if (SUCCEEDED(retVal))
    {
	m_pAudioFormat = CreateFormatObject(pHeader);

	retVal = HXR_OUTOFMEMORY;
	if (m_pAudioFormat)
	{
	    m_pAudioFormat->AddRef();
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pAudioFormat->Init(pHeader);
#if defined(HELIX_FEATURE_AUTOUPGRADE)
        if (FAILED(retVal) && retVal == HXR_REQUEST_UPGRADE)
        {
            AddToAutoUpgradeCollection(m_pAudioFormat->GetAutoUpgradeString(),
                                       m_pContext);
        }
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */
    }

    if (SUCCEEDED(retVal))
    {
        retVal = CreateAndAddNewAudioStream(m_ulCurAudioStream);
        if (SUCCEEDED(retVal))
        {
            retVal = InitAudioStream(pHeader, &m_ppAudioStream[m_ulCurAudioStream]);
        }
    }

    // Setup preroll
    if (SUCCEEDED(retVal))
    {
        pHeader->GetPropertyULONG32("Preroll", m_ulPreroll);

        // check that stream header preroll value is set
        if (m_ulPreroll == 0)
	{
	    // preroll is not set for this stream - assume default
	    m_ulPreroll = m_pAudioFormat->GetDefaultPreroll(pHeader);
	}
        
        // check that stream header preroll value is not too big
        ULONG32 ulMaxPreroll = m_pAudioFormat->GetMaximumPreroll(pHeader);
        if (m_ulPreroll > ulMaxPreroll)
        {
	    m_ulPreroll = ulMaxPreroll;
        }

        pHeader->SetPropertyULONG32( "Preroll", m_ulPreroll );

        // Set the flag saying whether or not the audio
        // stream parameters can change on the fly
        m_bCanChangeAudioStream = m_pAudioFormat->CanChangeAudioStream();

        // Save the stream header
        HX_RELEASE(m_pHeader);
        m_pHeader = pHeader;
        m_pHeader->AddRef();
    }

    return retVal;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:
//	CAudioRenderer::CheckStreamVersions
//  copied from CRealAudioRenderer
HX_RESULT CAudioRenderer::CheckStreamVersions(IHXValues* pHeader)
{
    // check stream and content versions so an upgrade can
    // be called if necessary...
    HX_RESULT pnr = HXR_OK;

#if defined(HELIX_FEATURE_AUTOUPGRADE)
    HXBOOL bVersionOK = TRUE;

    UINT32 ulStreamVersion = 0;
    UINT32 ulContentVersion = 0;

    if(HXR_OK == pHeader->GetPropertyULONG32("StreamVersion",
	ulStreamVersion))
    {
	UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulStreamVersion);
	UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulStreamVersion);
	ULONG32 ulThisMajorVersion = STREAM_MAJOR_VERSION;
	ULONG32 ulThisMinorVersion = STREAM_MINOR_VERSION;

	GetStreamVersion(ulThisMajorVersion, ulThisMinorVersion);

	if((ulMajorVersion > ulThisMajorVersion) ||
	   ((ulMinorVersion > ulThisMinorVersion) &&
	    (ulMajorVersion == ulThisMajorVersion)))
	{
	    bVersionOK = FALSE;
	}
    }

    if(bVersionOK &&
       (HXR_OK == pHeader->GetPropertyULONG32("ContentVersion",
					      ulContentVersion)))
    {
	UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulContentVersion);
	UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulContentVersion);
	ULONG32 ulThisMajorVersion = CONTENT_MAJOR_VERSION;
	ULONG32 ulThisMinorVersion = CONTENT_MINOR_VERSION;

	GetContentVersion(ulThisMajorVersion, ulThisMinorVersion);

	if((ulMajorVersion > ulThisMajorVersion) ||
	   ((ulMinorVersion > ulThisMinorVersion) &&
	    (ulMajorVersion == ulMajorVersion)))
	{
	    bVersionOK = FALSE;
	}
    }

    if(!bVersionOK)
    {
        AddToAutoUpgradeCollection(GetUpgradeMimeType(), m_pContext);

	pnr = HXR_FAIL;
    }
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */

    return pnr;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXRenderer::OnPacket
//  Purpose:
//	Called by client engine when a packet for this renderer is
//	due.
//	lTimeOffset is the amount of time that we lag behind the main player time line
//	so if the start time of the track is 10 seconds, lTimeOffset will be 10000 (msec)
//	(the first packet's time stamp will be 0 but the player will be at time=10sec)
//
STDMETHODIMP
CAudioRenderer::OnPacket(IHXPacket* pPacket, LONG32 lTimeOffset)
{
    HX_RESULT	    retVal = HXR_OK;

    /* Ignore any pre-seek packets or NULL packets */
    if (m_bInSeekMode || pPacket == NULL)
    {
	return HXR_OK;
    }

    m_lTimeOffset = lTimeOffset;

    m_bProcessingPacket = TRUE;
    m_pMutex->Lock();

    m_bFirstPacket = FALSE;

    if (m_bNeedStartTime)
    {
        m_pAudioFormat->SetStartTime(pPacket->GetTime());
        m_pAudioFormat->Reset();

        m_bNeedStartTime = FALSE;
    }

    m_pAudioFormat->Enqueue(pPacket);

    if (m_PlayState != playing)
    {
	// Take this chance to write audio to audio services
	UINT32 ulAudioTime;
	DoAudio(ulAudioTime);
    }

    m_bProcessingPacket = FALSE;
    m_pMutex->Unlock();

    return retVal;
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
STDMETHODIMP CAudioRenderer::OnTimeSync(ULONG32 ulTime)
{
    HXLOGL4(HXLOG_BAUD, "OnTimeSync(%lu)", ulTime);
    // we never enter the play state on Mac so that we write audio
    // in both packet and timesync
#ifndef _MACINTOSH
    // if we get a timesync we must be playing
    m_PlayState = playing;
#endif

    m_pMutex->Lock();

#ifdef _MACINTOSH
    /* On Mac, since we do not have Mutex, we do not want to process
     * data if we are within OnPacket call
     */
    if (m_bProcessingPacket)
    {
	goto exit;
    }

    m_bProcessingPacket = TRUE;
#endif /*_MACINTOSH*/

#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    // Write to AS
    UINT32 ulAudioTime;
    DoAudio(ulAudioTime);
#endif

#ifdef _MACINTOSH
    m_bProcessingPacket = FALSE;

exit:
#endif /*_MACINTOSH*/

    m_pMutex->Unlock();
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
STDMETHODIMP CAudioRenderer::OnPreSeek(ULONG32 ulOldTime,
				       ULONG32 ulNewTime)
{
    m_pMutex->Lock();

    m_PlayState = seeking;
    m_bEndOfPackets = FALSE;
    m_bDoneWritingPackets = FALSE;
    m_bInSeekMode = TRUE;
    m_bFirstPacket = TRUE;
    m_ulLastWriteTime = NO_TIME_SET;

    // get out of our buffering state if we are in one
    if (IsRebuffering())
    {
        EndRebuffer();
    }

    m_pAudioFormat->SetStartTime(ulNewTime);
    m_pAudioFormat->Reset();

    m_pMutex->Unlock();

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
STDMETHODIMP CAudioRenderer::OnPostSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    m_bInSeekMode   = FALSE;
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
STDMETHODIMP CAudioRenderer::OnPause(ULONG32 ulTime)
{
    m_pMutex->Lock();

    m_PlayState = paused;

    m_pMutex->Unlock();

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
STDMETHODIMP CAudioRenderer::OnBegin(ULONG32 ulTime)
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
STDMETHODIMP CAudioRenderer::OnBuffering(ULONG32 ulFlags, UINT16 unPercentComplete)
{
    m_PlayState = buffering;
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
STDMETHODIMP CAudioRenderer::GetDisplayType(REF(HX_DISPLAY_TYPE) ulFlags,
					    REF(IHXBuffer*) pBuffer)
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
STDMETHODIMP CAudioRenderer::OnEndofPackets(void)
{
    /* we should release any remaining sub-superblocks to audio services here*/
    m_bEndOfPackets = TRUE;

    if (IsRebuffering())
    {
        EndRebuffer();
    }

    m_pMutex->Lock();

#ifdef _MACINTOSH
    // since we don't get as many time sync calls on the mac
    // and we want to make sure to write all of the packets
    // to audio services, make sure we have 2 seconds of
    // data buffered in audio services now.
    AttemptToSatisfyDryRequest(m_ulLastWriteTime + 2000);
#endif

    m_pMutex->Unlock();

    return HXR_OK;
}


/*
 *  IHXDryNotification methods
 */
/************************************************************************
 *  Method:
 *      OnDryNotification
 *  Purpose:
 *	    This function is called when it is time to write to audio device
 *	    and there is not enough data in the audio stream. The renderer can
 *	    then decide to add more data to the audio stream. This should be
 *	    done synchronously within the call to this function.
 *	    It is OK to not write any data. Silence will be played instead.
 */
STDMETHODIMP CAudioRenderer::OnDryNotification(UINT32 /*IN*/ ulCurrentStreamTime,
						   UINT32 /*IN*/ ulMinimumDurationRequired)
{
    HX_RESULT hr = HXR_OK;

    HXLOGL2(HXLOG_BAUD, "OnDryNotification(%lu,%lu)", ulCurrentStreamTime, ulMinimumDurationRequired);
    /* If the renderer is delayed, do not report rebuffer status until the
     * packets are really due i.e. until Current time + Preroll is greater
     * than the Delay time.
     */
    m_pMutex->Lock();

    if (m_bDoneWritingPackets)
    {
	goto exit;
    }

    if (NO_TIME_SET != m_ulLastWriteTime &&
	IsTimeGreater(ulCurrentStreamTime, m_ulLastWriteTime + TIME_FUDGE))
    {
	// if the stream time reported by audio services is ahead of the
	// current writing time of the renderer, update the writing time
	// of the renderer so it catches up with the audio services stream
	m_ulLastWriteTime = ulCurrentStreamTime;
    }

    if (!m_bFirstPacket &&
	IsTimeGreater(ulCurrentStreamTime + m_ulPreroll, m_ulDelay) &&
	(NO_TIME_SET == m_ulLastWriteTime ||
	IsTimeGreater(ulCurrentStreamTime + TIME_FUDGE, m_ulLastWriteTime)))
    {
	// Try to write some audio to satisfy the audio stream
	HX_RESULT pnr = HXR_OK;

	UINT32 ulAudioWantedTime =
	    (ulCurrentStreamTime + ulMinimumDurationRequired);

	pnr = AttemptToSatisfyDryRequest(ulAudioWantedTime);

	// if we couldn't satisfy the request and we have not
	// recieved all of our packets, and we have started playing,
	// tell the core to rebuffer
        if ((FAILED(pnr) || HXR_NO_DATA == pnr) && !m_bEndOfPackets &&
            IsTimeGreaterOrEqual(ulCurrentStreamTime, m_ulDelay) &&
            IsTimeLess(m_ulLastWriteTime, ulAudioWantedTime))
        {
            StartRebuffer(ulAudioWantedTime);
	    hr = HXR_WOULD_BLOCK;
        }
    }

exit:
    m_pMutex->Unlock();

    return hr;
}


/************************************************************************
 *  IHXStatistics Methods
 */
/************************************************************************
 *  InitializeStatistics
 */
STDMETHODIMP CAudioRenderer::InitializeStatistics(UINT32 ulRegistryID)
{
    m_ulRegistryID = ulRegistryID;

#if defined(HELIX_FEATURE_STATS)
    HXBOOL bCodecNameKnown = FALSE;
    char* pValue = NULL;
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pAudioStats)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	pValue = (char*) GetCodecName();
	if (pValue != NULL)
	{
	    ReportStat(AS_CODEC_NAME, pValue);
	    bCodecNameKnown = TRUE;
	}
    }

    if (SUCCEEDED(retVal))
    {
	pValue = (char*) GetRendererName();
	if (pValue != NULL)
	{
	    ReportStat(AS_REND_NAME, pValue);
	    // If Codec name is unknown, use a more generic renderer name
	    if (!bCodecNameKnown)
	    {
		ReportStat(AS_CODEC_NAME, pValue);
	    }
	}
    }

    if (SUCCEEDED(retVal))
    {
	pValue = (char*) GetCodecFourCC();
	if (pValue != NULL)
	{
	    ReportStat(AS_CODEC_4CC, pValue);
	}
    }

    if (SUCCEEDED(retVal))
    {
	HXAudioFormat audioFmt;

	audioFmt.uChannels = 0;
	audioFmt.ulSamplesPerSec = 0;
	audioFmt.uBitsPerSample = 0;

	if (m_pAudioFormat)
	{
	    m_pAudioFormat->GetAudioFormat(audioFmt);
	}

	ReportStat(AS_CHANNELS, (INT32) audioFmt.uChannels);
	ReportStat(AS_SAMPLING_RATE, (INT32) audioFmt.ulSamplesPerSec);
	ReportStat(AS_SAMPLE_SIZE, (INT32) audioFmt.uBitsPerSample);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pAudioStats->DisplayStats(m_ulRegistryID);
    }

    return retVal;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}

/************************************************************************
 *  UpdateStatistics
 */
STDMETHODIMP CAudioRenderer::UpdateStatistics()
{
#if defined(HELIX_FEATURE_STATS)
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pAudioStats)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pAudioStats->DisplayStats(m_ulRegistryID);
    }

    return retVal;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}


/****************************************************************************
 *  Renderer's customizable fuctions - can be called any time
 */
/****************************************************************************
 *  GetStreamVersion
 */
void CAudioRenderer::GetStreamVersion(ULONG32 &ulThisMajorVersion,
					      ULONG32 &ulThisMinorVersion)
{
    ulThisMajorVersion = STREAM_MAJOR_VERSION;
    ulThisMinorVersion = STREAM_MINOR_VERSION;
}

/****************************************************************************
 *  GetContentVersion
 */
void CAudioRenderer::GetContentVersion(ULONG32 &ulThisMajorVersion,
					       ULONG32 &ulThisMinorVersion)
{
    ulThisMajorVersion = CONTENT_MAJOR_VERSION;
    ulThisMinorVersion = CONTENT_MINOR_VERSION;
}

/****************************************************************************
 *  GetUpgradeMimeType
 */
const char* CAudioRenderer::GetUpgradeMimeType(void)
{
    const char** pStreamMimeTypes = NULL;
    UINT32 ulInitialGranularity;

    GetRendererInfo(pStreamMimeTypes, ulInitialGranularity);

    if (pStreamMimeTypes)
    {
	return pStreamMimeTypes[0];
    }

    return NULL;
}

/****************************************************************************
 *  GetRendererName
 */
const char* CAudioRenderer::GetRendererName(void)
{
    return BASE_AUDIO_RENDERER_NAME;
}

/****************************************************************************
 *  GetCodecName
 */
const char* CAudioRenderer::GetCodecName(void)
{
    return NULL;
}

/****************************************************************************
 *  GetCoGetCodecFourCCdec4CC
 */
const char* CAudioRenderer::GetCodecFourCC(void)
{
    return NULL;
}


/****************************************************************************
 *  CreateFormatObject
 */
CAudioFormat* CAudioRenderer::CreateFormatObject(IHXValues* pHeader)
{
    return new CAudioFormat(m_pCommonClassFactory,
			    this);
}


/////////////////////////////////////////////////////////////////////////////
//  Method:
//	CAudioRenderer::InitAudioStream
HX_RESULT CAudioRenderer::InitAudioStream(IHXValues* pHeader,
					  IHXAudioStream** ppAudioStream)
{
    HX_RESULT retVal = HXR_OK;

    // init so we can HX_RELEASE on error.
    *ppAudioStream = NULL;

    retVal = m_pAudioPlayer->CreateAudioStream(ppAudioStream);
    if (SUCCEEDED(retVal))
    {
	IHXCommonClassFactory* pCommonClassFactory;
	if (HXR_OK == (*ppAudioStream)->QueryInterface(IID_IHXCommonClassFactory,
			    (void**)&pCommonClassFactory))
	{
	    m_pAudioFormat->OverrideFactory(pCommonClassFactory);
	    pCommonClassFactory->Release();
	}

	HXAudioFormat audioFmt;

	m_pAudioFormat->GetAudioFormat(audioFmt);

#if defined(HELIX_FEATURE_STATS)
	ReportStat(AS_CHANNELS, (INT32) audioFmt.uChannels);
	ReportStat(AS_SAMPLING_RATE, (INT32) audioFmt.ulSamplesPerSec);
	ReportStat(AS_SAMPLE_SIZE, (INT32) audioFmt.uBitsPerSample);
#endif /* #if defined(HELIX_FEATURE_STATS) */

	/* Add default dry notification BEFORE initializing the audio
	 * stream. This is so that if we are started mid presentation
	 * and there was no audio present earlier, the timeline will
	 * change from being a fake timeline to audio timeline and
	 * the audio services will write audio for initial pushdown
	 * time. We need to get dry notifications so that we can halt
	 * the timeline, if the renderer does not have enough data.
	 */
	IHXDryNotification* pDryNot = NULL;
	// Get my own DryNotification interface with an add
	QueryInterface(IID_IHXDryNotification, (void**)&pDryNot);

	retVal = (*ppAudioStream)->AddDryNotification(pDryNot);

	HX_ASSERT(SUCCEEDED(retVal));

        HXLOGL2(HXLOG_BAUD, "IHXAudioStream(0x%08x)::Init((%u,%u,%lu,%u))",
                *ppAudioStream, audioFmt.uChannels, audioFmt.uBitsPerSample,
                audioFmt.ulSamplesPerSec, audioFmt.uMaxBlockSize);

	retVal = (*ppAudioStream)->Init(&audioFmt, pHeader);

	HX_RELEASE(pDryNot);
    }

    if (HXR_OK != retVal)
    {
	HX_RELEASE((*ppAudioStream));
    }

    return retVal;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:
//	CAudioRenderer::WriteToAudioServices
HX_RESULT CAudioRenderer::WriteToAudioServices(HXAudioData* pAudioData)
{
    HX_RESULT pnr = HXR_OK;
    HXBOOL bTryWrite = TRUE;

    // Can the audio stream change on the fly?
    // If so, then check for any change. If not,
    // then skip the check.
    if (m_bCanChangeAudioStream)
    {
        HXBOOL bAudioStreamChanged = FALSE;
        pnr = CheckForAudioStreamChange(bAudioStreamChanged);
        if (FAILED(pnr))
        {
            return pnr;
        }
        if (bAudioStreamChanged)
        {
            pAudioData->uAudioStreamType = TIMED_AUDIO;
        }
    }

    while (bTryWrite)
    {
	pnr = CheckAudioServices();
        if (FAILED(pnr))
        {
            return pnr;
        }

        HXLOGL4(HXLOG_BAUD, "IHXAudioStream(0x%08x)::Write((0x%08x,%lu,%s)) bytes=%lu ms=%lu samps=%lu",
                m_ppAudioStream[m_ulCurAudioStream],
                pAudioData->pData,
                pAudioData->ulAudioTime,
                GetAudioStreamTypeString(pAudioData->uAudioStreamType),
                (pAudioData->pData ? pAudioData->pData->GetSize() : 0),
                m_pAudioFormat->ConvertBytesToMs((pAudioData->pData ? pAudioData->pData->GetSize() : 0)),
                m_pAudioFormat->ConvertBytesToSamples((pAudioData->pData ? pAudioData->pData->GetSize() : 0)));

        // Write to AS
        if (m_ppAudioStream[m_ulCurAudioStream])
            pnr = m_ppAudioStream[m_ulCurAudioStream]->Write( pAudioData );

	if (SUCCEEDED(pnr))
	{
	    CalculateMaxTimeStamp(pAudioData);
	    bTryWrite = FALSE;
	}
	else
	{
	    // we got an error on write, check what time the audio stream
	    // expects data for
	    HXAudioData audioData;
	    audioData.pData = NULL;

            if (m_ppAudioStream[m_ulCurAudioStream])
                m_ppAudioStream[m_ulCurAudioStream]->Write( &audioData );

	    if (IsTimeLess(audioData.ulAudioTime, pAudioData->ulAudioTime))
	    {
		if( pAudioData->uAudioStreamType == TIMED_AUDIO )
		{
    		    bTryWrite = FALSE;
		}
		// we are skipping ahead and should just mark this packet
		// as timed and write it again
		pAudioData->uAudioStreamType = TIMED_AUDIO;

	    }
	    else if (IsTimeGreater(audioData.ulAudioTime, pAudioData->ulAudioTime) &&
		IsTimeLessOrEqual(audioData.ulAudioTime, pAudioData->ulAudioTime +
		m_pAudioFormat->ConvertBytesToMs(pAudioData->pData->GetSize())))
	    {
		// we are a little behind but at least part of this stream
		// is on time, we should clip off this buffer to the time
		// the audio stream wants and try again.
		bTryWrite = m_pAudioFormat->ClipAudioBuffer(pAudioData,
		    audioData.ulAudioTime, TRUE);
	    }
	    else
	    {
		// we are a lot behind and should tell this format to discard
		// data until the audio stream time.
		m_pAudioFormat->
		    DiscardAudioUntil(audioData.ulAudioTime);

		// we don't want to try again with this data
		bTryWrite = FALSE;
	    }
	}
    }

    // Handle exiting rebuffer started by OnDryNotification from
    // Audio Services
    // if we just wrote audio to audio services that is greater than
    // or equal to the audio wanted time from the dry notification
    // call then we can leave buffering
    
    if (IsRebuffering() &&
        IsTimeGreaterOrEqual(m_ulLastWriteTime, m_ulAudioWantedTime))
    {
        EndRebuffer();
    }

    return pnr;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:
//	CAudioRenderer::DoAudio
//
//  Note:  See Switchsod.txt for how this is supposed to work, please keep
//	the sod up to date with changes here too.
//
HX_RESULT CAudioRenderer::DoAudio(UINT32& ulAudioTime,
				  AUDIO_STATE audioState)
{
    HX_RESULT retVal;
    HXAudioData audioData;
    ULONG32 ulPreviousLastWriteTime;
    LONG32 lTimeDelta;

    audioData.pData = NULL;
    audioData.ulAudioTime = ulAudioTime = 0;

    // write the lowest stream to audio services
    ulPreviousLastWriteTime = m_ulLastWriteTime;

    do
    {
	retVal = m_pAudioFormat->CreateAudioFrame(
	    audioData,
	    (m_bEndOfPackets) ? AUDIO_END_OF_PACKETS : audioState);

	if (retVal == HXR_OK)
	{
	    audioData.uAudioStreamType = TIMED_AUDIO;

	    // Update the timestamp of the audio services write
	    audioData.ulAudioTime = AdjustTimestamp(audioData.ulAudioTime, m_lTimeOffset);

	    if (m_ulLastWriteTime != NO_TIME_SET)
	    {
		lTimeDelta = audioData.ulAudioTime -
			     m_ulLastWriteTime;

//		HX_ASSERT(lTimeDelta >= (-TIME_FUDGE));

		if (lTimeDelta <= TIME_FUDGE)
		{
		    audioData.uAudioStreamType = STREAMING_AUDIO;
		}
#ifdef _AUDREND_FLOW_LOG
		else if (lTimeDelta < (-TIME_FUDGE))
		{
		    DEBUG_OUTF_IDX(m_ulCurAudioStream, AUDREND_FLOW_FILE, 
		       (s, "Overlapping Audio: Time=%u, LastWriteTime=%u, Overlap=%d\n",
			    audioData.ulAudioTime,
			    m_ulLastWriteTime,
			    -lTimeDelta
			    ));
		}
#endif	// _AUDREND_FLOW_LOG
	    }

	    retVal = WriteToAudioServices(&audioData);        
	}
	else
	{
	    break;
	}
                               // do not loop here if writing to
                               // satisfy dry notification
     } while (audioState != AUDIO_DRYNOTIFICATION &&
            ((m_ulLastWriteTime - ulPreviousLastWriteTime) <
	     MAX_AUDIO_WRITE_TIME));

    // release the data buffer if we got one
    HX_RELEASE(audioData.pData);

    // update the out param
    ulAudioTime = audioData.ulAudioTime;

    return retVal;
}

HX_RESULT CAudioRenderer::AttemptToSatisfyDryRequest(UINT32 ulAudioWantedTime)
{
    HX_RESULT pnr = HXR_OK;
    UINT32 ulAudioTime = 0;

    while (HXR_OK == pnr &&
	IsTimeGreaterOrEqual(ulAudioWantedTime, m_ulLastWriteTime))
    {
	pnr = DoAudio(ulAudioTime, AUDIO_DRYNOTIFICATION);
    }

    return pnr;
}

void
CAudioRenderer::CalculateMaxTimeStamp(HXAudioData* pAudioData)
{
    UINT32 ulTimestamp = pAudioData->ulAudioTime +
	m_pAudioFormat->ConvertBytesToMs(pAudioData->pData->GetSize());

    if (m_ulLastWriteTime == NO_TIME_SET ||
	IsTimeLess(m_ulLastWriteTime, ulTimestamp))
    {
	m_ulLastWriteTime = ulTimestamp;
    }
}

HX_RESULT CAudioRenderer::CheckForAudioStreamChange(REF(HXBOOL) rbAudioStreamChanged)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAudioFormat)
    {
        // Clear the return value
	retVal = HXR_OK;
	// Check to see if the audio format has changed
	if (HasAudioFormatChanged())
	{
            // See if we can find a compatible audio stream from
            // the currently allocated array of audio streams.
            // If we are switching formats for the first time,
            // we will not expect to find a compatible format.
            // If, however, we are switching for a second time
            // (or more), then we may be able to find a compatible
            // audio stream from the currently allocated array.
            UINT32 ulNewAudioStreamIndex = 0;
            HX_RESULT rv = FindCompatibleAudioStream(ulNewAudioStreamIndex);
            if (FAILED(rv))
            {
                // We didn't find a compatible audio stream, so
                // we need to create an additional audio stream.
                retVal = CreateAndAddNewAudioStream(ulNewAudioStreamIndex);
            }
            if (SUCCEEDED(retVal))
            {
                // Switch to the new audio stream
                retVal = SwitchToNewAudioStream(m_ulCurAudioStream, ulNewAudioStreamIndex);
                if (SUCCEEDED(retVal))
                {
                    // Set the current audio stream index
                    m_ulCurAudioStream = ulNewAudioStreamIndex;
                }
            }
            // Set the out parameter
	    rbAudioStreamChanged = SUCCEEDED(retVal);
	}
    }

    return retVal;
}

HX_RESULT CAudioRenderer::IsAudioStreamFormatCompatible(UINT32 i, HXBOOL& rbIsCompatible)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAudioFormat && m_ppAudioStream && i < m_ulNumAudioStreams &&  m_ppAudioStream[i])
    {
        // Get the audio format from the CAudioFormat object
        HXAudioFormat cAudioFormat1;
        retVal = m_pAudioFormat->GetAudioFormat(cAudioFormat1);
        if (SUCCEEDED(retVal))
        {
            IHXAudioStream2* pStream2 = NULL;
            retVal = m_ppAudioStream[i]->QueryInterface(IID_IHXAudioStream2, (void**) &pStream2);
            if (SUCCEEDED(retVal))
            {
                HXAudioFormat cAudioFormat2;
                retVal = pStream2->GetAudioFormat(&cAudioFormat2);
                if (SUCCEEDED(retVal))
                {
                    // Check if the formats are different
                    if (cAudioFormat1.uChannels       != cAudioFormat2.uChannels       ||
                        cAudioFormat1.uBitsPerSample  != cAudioFormat2.uBitsPerSample  ||
                        cAudioFormat1.ulSamplesPerSec != cAudioFormat2.ulSamplesPerSec ||
                        cAudioFormat1.uMaxBlockSize    > cAudioFormat2.uMaxBlockSize)
                    {
                        // This audio stream is not compatible with the current format
                        rbIsCompatible = FALSE;
                    }
                    else
                    {
                        // This audio stream is compatible with the current format
                        rbIsCompatible = TRUE;
                    }
                }
            }
            HX_RELEASE(pStream2);
        }
    }

    return retVal;
}

HX_RESULT CAudioRenderer::FindCompatibleAudioStream(UINT32& rulAudioStreamIndex)
{
    // Look through all audio streams until we
    // either run out of audio streams, fail,
    // or find a compatible audio stream.
    HX_RESULT retVal        = HXR_OK;
    HXBOOL    bIsCompatible = FALSE;
    UINT32    i             = 0;
    for (i = 0; i < m_ulNumAudioStreams && SUCCEEDED(retVal); i++)
    {
        retVal = IsAudioStreamFormatCompatible(i, bIsCompatible);
        if (SUCCEEDED(retVal) && bIsCompatible)
        {
            break;
        }
    }
    if (SUCCEEDED(retVal))
    {
        // If we examined all the audio streams and didn't
        // find a compatible stream, then i == m_ulNumAudioStreams
        // If we found a compatible stream, then i < m_ulNumAudioStreams
        if (i < m_ulNumAudioStreams)
        {
            // Set the out parameter and leave the
            // return value as successful
            rulAudioStreamIndex = i;
        }
        else
        {
            // We didn't find a compatible audio stream,
            // so change the return value to failure
            retVal = HXR_FAIL;
        }
    }

    return retVal;
}

HX_RESULT CAudioRenderer::SwitchToNewAudioStream(UINT32 ulOldStreamIndex, UINT32 ulNewStreamIndex)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_ppAudioStream &&
        ulOldStreamIndex < m_ulNumAudioStreams &&
        ulNewStreamIndex < m_ulNumAudioStreams &&
        m_ppAudioStream[ulOldStreamIndex])
    {
        // Remove the dry notification from the old stream
        IHXAudioStream2* pStream2 = NULL;
        retVal = m_ppAudioStream[ulOldStreamIndex]->QueryInterface(IID_IHXAudioStream2, (void**) &pStream2);
        if (SUCCEEDED(retVal))
        {
            // Get our own IHXDryNotification interface
            IHXDryNotification* pDryNot = NULL;
            retVal = QueryInterface(IID_IHXDryNotification, (void**) &pDryNot);
            if (SUCCEEDED(retVal))
            {
                // Remove the dry notification
                retVal = pStream2->RemoveDryNotification(pDryNot);
                if (SUCCEEDED(retVal))
                {
                    // Init the new stream
                    retVal = InitAudioStream(m_pHeader, &m_ppAudioStream[ulNewStreamIndex]);
                }
            }
            HX_RELEASE(pDryNot);
        }
        HX_RELEASE(pStream2);
    }

    return retVal;
}

HXBOOL CAudioRenderer::HasAudioFormatChanged()
{
    HXBOOL bRet = FALSE;

    HXBOOL    bCompat = FALSE;
    HX_RESULT rv      = IsAudioStreamFormatCompatible(m_ulCurAudioStream, bCompat);
    if (SUCCEEDED(rv))
    {
        // If the current format is not compatible, then
        // the audio format has changed. If the current
        // format IS compatible, then the audio format
        // has NOT changed. So the return value is
        // the logical NOT of bCompat.
        bRet = !bCompat;
    }

    return bRet;
}

HX_RESULT CAudioRenderer::CreateAndAddNewAudioStream(UINT32& rulNewAudioStreamIndex)
{
    HX_RESULT retVal = HXR_FAIL;

    // Get the next audio stream index we will assign
    UINT32 ulNextAudioStreamIndex = m_ulNumAudioStreams;
    // Is our buffer of audio stream pointers big enough?
    if (ulNextAudioStreamIndex >= m_ulNumAudioStreamPtrAlloc)
    {
        // We need to create a larger buffer to hold the
        // audio stream pointers. Double the current allocation
        UINT32 ulNewSize = m_ulNumAudioStreamPtrAlloc * 2;
        IHXAudioStream** ppAudioStream = new IHXAudioStream* [ulNewSize];
        if (ppAudioStream)
        {
            // NULL out the buffer
            memset((void*) ppAudioStream, 0, ulNewSize * sizeof(IHXAudioStream*));
            // Copy the current pointers (if there are any)
            if (m_ppAudioStream && m_ulNumAudioStreams)
            {
                memcpy((void*) ppAudioStream, /* Flawfinder: ignore */
                    (const void*) m_ppAudioStream,
                    m_ulNumAudioStreams * sizeof(IHXAudioStream*));
            }
            // Delete the old array
            HX_VECTOR_DELETE(m_ppAudioStream);
            // Assign the new one
            m_ppAudioStream = ppAudioStream;
            // Assign the new number of allocated pointers
            m_ulNumAudioStreamPtrAlloc = ulNewSize;
        }
    }
    if (ulNextAudioStreamIndex < m_ulNumAudioStreamPtrAlloc)
    {
        // Set the out parameter
        rulNewAudioStreamIndex = ulNextAudioStreamIndex;
        // Increment the number of active streams
        m_ulNumAudioStreams++;
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

const char* CAudioRenderer::GetAudioStreamTypeString(AudioStreamType eType)
{
    const char* pszRet = "Unknown";

    if (eType < AUDREND_NUM_STREAMTYPESTRINGS)
    {
        pszRet = zm_pAudioStreamType[eType];
    }

    return pszRet;
}

HXBOOL CAudioRenderer::IsRebuffering() const
{
    return (NO_TIME_SET != m_ulAudioWantedTime) ? TRUE : FALSE;
}

void CAudioRenderer::StartRebuffer(UINT32 ulAudioWantedTime)
{
    if (m_pStream)
    {
        m_ulAudioWantedTime = ulAudioWantedTime;

        if (m_bStream2)
        {
	    ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(1,0);
        }
        else
        {
            m_pStream->ReportRebufferStatus(1,0);
        }
    }
}

void CAudioRenderer::EndRebuffer()
{
    m_ulAudioWantedTime = NO_TIME_SET;

    if (m_pStream)
    {
        if (m_bStream2)
        {
	    ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(1,1);
        }
        else
        {
            m_pStream->ReportRebufferStatus(1,1);
        }
    }
}
