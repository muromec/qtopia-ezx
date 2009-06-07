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

// system
#include "hlxclib/stdio.h"
// include
#include "hxcom.h"              // IUnknown
#include "hxcomm.h"            // IHXCommonClassFactory
#include "ihxpckts.h"           // IHXBuffer, IHXPacket, IHXValues
#include "hxplugn.h"           // IHXPlugin
#include "hxrendr.h"           // IHXRenderer
#include "hxengin.h"           // IHXInterruptSafe
#include "hxcore.h"            // IHXStream
#include "hxausvc.h"           // Audio Services
#include "hxmon.h"             // IHXStatistics
#include "hxupgrd.h"           // IHXUpgradeCollection
#include "rmfftype.h"		// AudioTypeSpecificData
#include "hxprefs.h"
#include "hxcore.h"		// IHXUpdateProperties
#include "hxver.h"
// pnmisc
#include "hxstrutl.h"           // strcasecmp()
// rmpadec
#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
// rmp3lib
#include "mp3format.h"          // MP3 formatter
// mpgcommon
#ifdef DEMUXER
#include "xmddemuxer.h"         // Demuxer
#include "xmdtypes.h"
#endif
#include "addupcol.h"
// mp3rend
#include "mp3rend.h"           // CRnMp3Ren
#include "pktparse.h"           // CPacketParser
#if defined (MPA_FMT_RAW) || defined (MPA_FMT_RFC2250) || defined (MPA_FMT_SYSTEM) || defined (MPA_FMT_RFC2250_SYSTEM)
#include "mpapktparse.h"        // CMpaPacketParser, C2250PacketParser
#endif /* #if defined (MPA_FMT_RAW) */
#if defined(MPA_FMT_DRAFT00)
#include "fmtpktparse.h"        // CFmtPacketParser
#endif /* #if defined(MPA_FMT_DRAFT00) */
#if defined(MPA_FMT_RFC3119)
#include "robpktparse.h"        // CRobustPacketParser
#endif /* #if defined(MPA_FMT_RFC3119) */
#include "hxdefpackethookhlp.h"

#include "adjtime.h"
#include "hxtlogutil.h"

// The value below is derived from trial and error as the lowest we can
// get away with on the platforms tested. Feel free to experiment with
// different values on your platform.
#define OPTIMAL_AUDIO_PUSHDOWN 160
#ifdef PCM_CAPTURE
#include <stdio.h>
#include "wavep.h"

static UINT32 g_dwTotalPcmBytes = 0;
static FILE  *fpOut = NULL;
#endif // PCM_CAPTURE

#if defined(HELIX_FEATURE_MIN_HEAP)
#define MAXIMUM_MP3_PREROLL 1500
#else
#define MAXIMUM_MP3_PREROLL 7500
#endif

#define SAMPLES_PER_MP3_FRAME 1152
#define MAX_MP3_BITRATE 320000

///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Ren static variables                      ref:  ff1rendr.h
//
//  These variables are passed to the RMA core to provide information about
//  this plug-in. They are required to be static in order to remain valid
//  for the lifetime of the plug-in.
//
const char* const CRnMp3Ren::zm_pDescription = MY_DESCRIPTION;
const char* const CRnMp3Ren::zm_pCopyright   = HXVER_COPYRIGHT;
const char* const CRnMp3Ren::zm_pMoreInfoURL = HXVER_MOREINFO;
const char* const CRnMp3Ren::zm_pStreamMimeTypes[] = MY_STREAM_MIME_TYPES;

///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Ren::CRnMp3Ren                   ref:  ff1rendr.h
//
//  Constructor
//
CRnMp3Ren::CRnMp3Ren(void)
	: m_RefCount        (0),
	  m_pContext        (NULL),
      m_pClassFactory   (NULL),
      m_pStream         (NULL),
      m_pAudioPlayer    (NULL),
      m_pDefAudStream   (NULL),
      m_pAudioStream    (NULL),
      m_pAudioPushdown2 (NULL),
      m_pHeader         (NULL),      
      m_pPacketParser   (NULL),
      m_bInSeekMode     (FALSE),
      m_bDiscontinuity  (TRUE),
      m_bEndOfPackets   (0),
      m_bStarving       (0),
      m_wAudStream      (0),
      m_nPacketsNeeded  (0),
      m_ulDelay         (0),
      m_ulNumPackets    (0),      
      m_ulPreroll       (0),
      m_lTimeLineOffset	(0),
      m_pRegistry       (NULL),      
      m_ulRegistryID    (0),
      m_ulChannelsRegID (0),
      m_ulCodecRegID    (0),

      m_ulPacketsDecoded(0),
      m_ulFramesDecoded (0),
      m_ulBadPackets    (0),
      m_bStarted        (0),
      m_bTrustPackets(FALSE),
      m_bStream2(FALSE),
      m_ulLastTimeSync(0),

      m_pDefaultPacketHookHelper(NULL)
{

    memset(m_aAudStreamList, 0, sizeof(m_aAudStreamList));
}


///////////////////////////////////////////////////////////////////////////////
//  IHXPlugin::GetPluginInfo                                ref:  hxplugn.h
//
//  This routine returns descriptive information about the plug-in, most
//  of which is used in the About box of the user interface. It is called
//  when the RMA core application is launched.
//
STDMETHODIMP
CRnMp3Ren::GetPluginInfo(REF(int) bLoadMultiple,
                         REF(const char*) pDescription,
                         REF(const char*) pCopyright,
                         REF(const char*) pMoreInfoURL,
                         REF(UINT32) versionNumber)
{
	bLoadMultiple = TRUE;
	pDescription  = zm_pDescription;
	pCopyright    = zm_pCopyright;
	pMoreInfoURL  = zm_pMoreInfoURL;
	versionNumber = MY_PLUGIN_VERSION;
	
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::GetRendererInfo                            ref:  hxrendr.h
//
//  This routine returns crucial information required to associate this
//  plug-in with a given stream MIME type. This information tells the HX
//  core which Renderer to use to display a particular type of stream. This
//  method is called when the RMA core application is launched.
//
STDMETHODIMP
CRnMp3Ren::GetRendererInfo(REF(const char**) pStreamMimeTypes,
                           REF(UINT32) initialGranularity)
{
	// Associate this Renderer with a given stream MIME type
	pStreamMimeTypes = (const char**) zm_pStreamMimeTypes;

	// Set frequency of OnTimeSync() updates
	initialGranularity = 50;
	
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXPlugin::InitPlugin                                   ref:  hxplugn.h
//
//  This routine performs initialization steps such as determining if
//  required interfaces are available. It is called when the RMA core 
//  application is launched, and whenever a stream associated with this
//  plug-in needs to be rendered.
//
STDMETHODIMP
CRnMp3Ren::InitPlugin(IUnknown* pHXCore)
{
    m_pContext = pHXCore;
    m_pContext->AddRef();

    HX_ENABLE_LOGGING(m_pContext);
    
    // Store a reference to the IHXCommonClassFactory interface which is
    // used to create commonly used HX objects such as IHXPacket, 
    // IHXValues, and IHXBuffers.
	pHXCore->QueryInterface(IID_IHXCommonClassFactory, 
	                        (void**)&m_pClassFactory);
	if (!m_pClassFactory)
		return HXR_NOTIMPL;
	
    pHXCore->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

#ifdef DEMUXER
    if (m_pRegistry) m_pRegistry->AddInt("FirstPts", -1);
#endif
    
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::StartStream                                ref:  hxrendr.h
//
//  The RMA core calls this routine to provide access to the stream and
//  player. It is called when the plug-in is being initialized.
//
STDMETHODIMP
CRnMp3Ren::StartStream(IHXStream *pStream,
                       IHXPlayer *pPlayer)
{
    m_bEndOfPackets = FALSE;

    IHXStream2* pStream2 = NULL;

    if (pStream &&
        HXR_OK == pStream->QueryInterface(IID_IHXStream2, (void**)&pStream2))
    {
        m_pStream = pStream2;
        m_bStream2 = TRUE;
    }
    else
    {
        m_pStream = pStream;
        HX_ADDREF(m_pStream);
    }

    // Get interface to audio player
    if (HXR_OK != pPlayer->QueryInterface(IID_IHXAudioPlayer, (void**) &m_pAudioPlayer))
        return !HXR_OK;

    pPlayer->QueryInterface(IID_IHXAudioPushdown2, (void**) &m_pAudioPushdown2);
#ifdef HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES
    m_pAudioPushdown2->SetAudioPushdown( OPTIMAL_AUDIO_PUSHDOWN );
#endif // HELIX_CONFIG_MIN_PUSHDOWN_BYTES

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnHeader                                   ref:  hxrendr.h
//
//  This routine is passed the stream header object created by the associated
//  file format plug-in. Use the methods defined in the IHXValues interface
//  to access the stream header data. This method is called from the HX
//  core when the plug-in is being initialized.
//
STDMETHODIMP
CRnMp3Ren::OnHeader(IHXValues* pStreamHeaderObj)
{
    UINT32  ulAvgBitRate = 0,
            ulAvgPacketSize = 0,
            ulSampRate = 0,
            ulChannels = 0;

    HX_RESULT  pr;
    pr = CheckStreamVersions(pStreamHeaderObj);
    
    if (HXR_OK != pr)
        return pr;
        
    m_pHeader = pStreamHeaderObj;
    m_pHeader->AddRef();

    pStreamHeaderObj->GetPropertyULONG32("AvgBitRate",      ulAvgBitRate);
    pStreamHeaderObj->GetPropertyULONG32("AvgPacketSize",   ulAvgPacketSize);
    pStreamHeaderObj->GetPropertyULONG32("SampleRate",      ulSampRate);
    pStreamHeaderObj->GetPropertyULONG32("NumChannels",     ulChannels);
    pStreamHeaderObj->GetPropertyULONG32("Delay",           m_ulDelay);

    pStreamHeaderObj->SetPropertyULONG32("IsAudioStream",   1);

    // Check standard stream properties if custom properties fail
    if (!ulSampRate)
        pStreamHeaderObj->GetPropertyULONG32("SamplesPerSecond", ulSampRate);

    if (!ulChannels)
        pStreamHeaderObj->GetPropertyULONG32("Channels", ulChannels);

    // Try Opaque Data for SampleRate and/or channels
    if (!ulSampRate || !ulChannels)
    {
    	IHXBuffer* pOpaqueData = NULL;

	if (SUCCEEDED(pStreamHeaderObj->GetPropertyBuffer("OpaqueData", 
							  pOpaqueData)) &&
	    pOpaqueData)
	{
	    AudioTypeSpecificData audioData;

	    memset(&audioData, 0, sizeof(audioData));
		    
	    audioData.unpack(pOpaqueData->GetBuffer(), 
			     pOpaqueData->GetSize());
		    
	    if (audioData.numChannels)
	    {
		ulChannels = audioData.numChannels;
	    }

	    if (audioData.sampleRate)
	    {
		ulSampRate = audioData.sampleRate;
	    }
	}

	HX_RELEASE(pOpaqueData);
    }
    
    // Check MIME type
    IHXBuffer* pStringObj = NULL;
    pStreamHeaderObj->GetPropertyCString("MimeType", pStringObj);	
    if (pStringObj)
    {
        SetPacketFormat((const char*)(pStringObj->GetBuffer()));

        // Check if we know these packets can be trusted. These
        // are the mime types that we *know* came from our file format
        const char* pszMimeType = (const char*) pStringObj->GetBuffer();
        if (!strcmp(pszMimeType, "audio/MPEG-ELEMENTARY")    ||
            !strcmp(pszMimeType, "audio/MPEG-ELEMENTARY-RN") ||
            !strcmp(pszMimeType, "audio/X-MP3-draft-00")     ||
            !strcmp(pszMimeType, "audio/X-MP3-draft-00-RN"))
        {
            m_bTrustPackets = TRUE;
        }
        // Set this in the packet parser
        m_pPacketParser->SetTrustPackets(m_bTrustPackets);
        
        HX_RELEASE(pStringObj);
    }
    
    if(m_pPacketParser)
    {
        m_pPacketParser->Init(this, m_pClassFactory);
    }

    // Setup preroll
    // Set to default value if not set in stream header.
    
    m_ulPreroll = 0;
    pStreamHeaderObj->GetPropertyULONG32("Preroll", m_ulPreroll);
    if (m_ulPreroll == 0)
    {
        // Set the default preroll to the duration of ulDefaultPrerollFrames 
        // MP3 frames
        ULONG32 ulDefaultPrerollFrames = 5; // Tunable value
        ULONG32 ulTmpSampleRate = (ulSampRate) ? ulSampRate : 44100;
        ULONG32 ulDefaultPreroll = 
            (ulDefaultPrerollFrames * 1000 * SAMPLES_PER_MP3_FRAME) / 
            ulTmpSampleRate;
        
        ULONG32 ulMaxBitrate = MAX_MP3_BITRATE;
        ULONG32 ulTmp;
        if (HXR_OK == pStreamHeaderObj->GetPropertyULONG32("MaxBitRate",
                                                           ulTmp) &&
            (ulTmp != 0) && (ulAvgBitRate <= ulTmp))
        {
            ulMaxBitrate = ulTmp;
        }
        
        if (ulAvgBitRate)
        {
            // Dialate the default preroll under the assumption
            // that we could get ulDefaultPrerollFrames at 
            // the max possible bitrate
            
            ULONG32 ulQ = ulDefaultPreroll / ulAvgBitRate;
            ULONG32 ulR = ulDefaultPreroll - ulQ * ulAvgBitRate;
            ulDefaultPreroll = (ulMaxBitrate * ulQ +
                                (ulMaxBitrate * ulR) / ulAvgBitRate);
        }
        
	m_ulPreroll = ulDefaultPreroll;
    }

    // Check that stream header value is not over our max.    
    if( m_ulPreroll > MAXIMUM_MP3_PREROLL )
    {
        m_ulPreroll = MAXIMUM_MP3_PREROLL;
    }
    pStreamHeaderObj->SetPropertyULONG32("Preroll", m_ulPreroll);

    if (!ulAvgPacketSize)
        ulAvgPacketSize = 1200;

    UINT32 dwAudBytes = m_ulPreroll * ulAvgBitRate / 8000;
    m_nPacketsNeeded = (UINT8)(dwAudBytes / ulAvgPacketSize);
    
    if (!m_pAudioStream && ulChannels && ulSampRate)
    {        
        // Create our audio stream
        m_pAudioPlayer->CreateAudioStream(&m_aAudStreamList[m_wAudStream]);
        m_pAudioStream = m_aAudStreamList[m_wAudStream];

        if (!m_pAudioStream)
            return HXR_OK;
        // Let's override the default class factory and use the one in
        // client/audiosvc that caches our pcm buffers and avoids 
        // unnecessary heap fragmentation.

        IHXCommonClassFactory* pCommonClassFactory;
        if (HXR_OK == m_pAudioStream->QueryInterface(IID_IHXCommonClassFactory, 
				(void**)&pCommonClassFactory))
        {
	    if( m_pPacketParser )
            {
                m_pPacketParser->OverrideFactory(pCommonClassFactory);
            }
            HX_RELEASE(pCommonClassFactory);
        }

        // Init PCM device
        HXAudioFormat audioFmt;
        audioFmt.uChannels = (unsigned short)ulChannels;
        audioFmt.uBitsPerSample	= 16;
        audioFmt.ulSamplesPerSec = ulSampRate;
        audioFmt.uMaxBlockSize = (unsigned short)(SAMPLES_PER_MP3_FRAME *
                                 (audioFmt.uBitsPerSample>>3) *
                                 audioFmt.uChannels);

        m_pAudioStream->Init(&audioFmt, m_pHeader);

        m_pAudioStream->AddDryNotification(this);
        m_bDiscontinuity = TRUE;
    }
    
#if 0
    // Create a default stream to prevent rebuffering
    else
    {
        // If the server does not send these properties
        if (!ulSampRate)
            ulSampRate = 44100;

        if (!ulChannels)
            ulChannels = 2;

        ulSampRate = HX_MIN(ulSampRate, 44100);
        ulChannels = HX_MIN(ulChannels, 2);

        // Create our audio stream
        m_pAudioPlayer->CreateAudioStream(&m_pDefAudStream);

        if (!m_pDefAudStream)
            return HXR_OK;

        // Init PCM device
        HXAudioFormat audioFmt;
        audioFmt.uChannels = (unsigned short)ulChannels;
        audioFmt.uBitsPerSample	= 16;
        audioFmt.ulSamplesPerSec = ulSampRate;
        audioFmt.uMaxBlockSize = (unsigned short)(SAMPLES_PER_MP3_FRAME *
                                 (audioFmt.uBitsPerSample>>3) *
                                 audioFmt.uChannels);

        m_pDefAudStream->Init(&audioFmt, m_pHeader);
    }
#endif


    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnBegin                                   ref:  hxrendr.h
//
//  This routine is called by the RMA core to inform the Renderer that 
//  playback has just begun or has been resumed. The stream's time value just
//  after resuming the playback is provided.
//
STDMETHODIMP
CRnMp3Ren::OnBegin(UINT32 timeAfterBegin)
{
    #ifdef _WIN32
    #ifdef _DEBUG
    TCHAR szTmp[128];
    wsprintf(szTmp, TEXT("OnBegin timeAfter %ld\n"),timeAfterBegin);
    OutputDebugString(szTmp);
    #endif
    #endif

    if(m_pPacketParser)
    {
        m_pPacketParser->Begin(timeAfterBegin);
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::GetDisplayType                             ref:  hxrendr.h
//
//  This routine returns the preferred display type of the Renderer. Any
//  other additional information necessary for the display can also be
//  provided. This method is called by the RMA core when the plug-in is being
//  initialized.
//
STDMETHODIMP
CRnMp3Ren::GetDisplayType(REF(HX_DISPLAY_TYPE) displayType,
                          REF(IHXBuffer*) pDisplayInfo)
{
    displayType = HX_DISPLAY_NONE;

	return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnPacket                                   ref:  hxrendr.h
//
//  This routine is passed the packet object streamed by the associated
//  file format plug-in. It is called by the RMA core when a packet is due
//  to be delivered. In most cases, the actual rendering of the packet is
//  done in this method. The stream's time offset with respect to the master
//  timeline is provided.
//
STDMETHODIMP
CRnMp3Ren::OnPacket(IHXPacket* pPacketObj,
                    INT32 streamOffsetTime)
{
    HXLOGL4(HXLOG_MP3X, "OnPacket(,%ld) %s pts=%lu rule=%u flags=0x%02x lastTS=%lu",
            streamOffsetTime,
            (pPacketObj->IsLost() ? "LOST" : ""),
            pPacketObj->GetTime(),
            pPacketObj->GetASMRuleNumber(),
            pPacketObj->GetASMFlags(),
            m_ulLastTimeSync);
    if (m_bInSeekMode)
        return HXR_OK;
    
    if(!m_pPacketParser)
       return HXR_NOT_INITIALIZED;

    if (!pPacketObj)
        return HXR_INVALID_PARAMETER;
    
    #ifdef _DEBUG
    ++m_ulPacketsDecoded;
    #endif

    m_lTimeLineOffset = streamOffsetTime;

    HX_RESULT res = m_pPacketParser->AddPacket(pPacketObj, 0);

    if(FAILED(res))
    {            
        return HXR_OK;
    }

    if(m_pDefaultPacketHookHelper)
	m_pDefaultPacketHookHelper->OnPacket(pPacketObj);
#if 0
#ifdef HELIX_CONFIG_ONLY_DECODE_IF_DRY
    // Only decode audio if audio device is low
    // IMPORTANT NOTE: since the implementation of AddPacket in the parser
    // does not have a queue, in general if we return here and we do NOT get
    // an OnDryNotification, the next time OnPacket is called, it will clobber
    // an undecoded packet. As it turns out, this doesn't seem to happen in
    // testing to date, but this is a TBD.
    // Testing without this code seems to show that we can get the same results
    // without it. Nevertheless, we are going to leave this code here until an
    // absolute final decision can be made on the best solution.
    UINT32 ulBytesInAudioDevice = 0;
    m_pAudioPushdown2->GetCurrentAudioDevicePushdown( ulBytesInAudioDevice );
    if( ulBytesInAudioDevice >= OPTIMAL_AUDIO_PUSHDOWN )
    {
       return HXR_OK;
    }
#endif // HELIX_CONFIG_ONLY_DECODE_IF_DRY
#endif // 0

    // Decode and render all frames available
    HX_RESULT theErr = m_pPacketParser->RenderAll();
	if(theErr == HXR_OUTOFMEMORY || theErr == HXR_UNSUPPORTED_AUDIO)
    {
        return theErr;
    }

    // Handle rebuffering
    if (m_bStarving)
    {
        // Waiting for more packets
        if (++m_ulNumPackets < m_nPacketsNeeded)
        {
            if (m_bStream2)
            {
	        ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(m_nPacketsNeeded, (UINT8)m_ulNumPackets);
            }
            else
            {
	        m_pStream->ReportRebufferStatus(m_nPacketsNeeded, (UINT8)m_ulNumPackets);
            }
            return HXR_OK;
        }

        #ifdef _WIN32
        #ifdef _DEBUG
        OutputDebugString(TEXT("Rebuffer Complete\n"));
        #endif
        #endif

        if (m_bStream2)
        {
	    ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(m_nPacketsNeeded, m_nPacketsNeeded);
        }
        else
        {
	    m_pStream->ReportRebufferStatus(m_nPacketsNeeded, m_nPacketsNeeded);
        }
        m_bStarving = FALSE;
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnTimeSync                                 ref:  hxrendr.h
//
//  This routine is called by the RMA core periodically passing the current
//  playback time. The Renderer should use this time value to synchronize the
//  playback of various streams.
//
STDMETHODIMP
CRnMp3Ren::OnTimeSync(UINT32 currentPlayBackTime)
{
    m_ulLastTimeSync = currentPlayBackTime;
#if 0
    #ifdef _DEBUG
    #ifdef _WIN32
    if (!m_bStarted)
    {
        TCHAR szTmp[128];
        wsprintf(szTmp, TEXT("OnTimeSync %ld Preroll %ld\n"),
                         currentPlayBackTime, m_ulPacketsDecoded*m_dFrameTime);
        OutputDebugString(szTmp);

        m_bStarted = 1;
    }
    #endif
    #endif
#endif

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnPreSeek                                  ref:  hxrendr.h
//
//  This routine is called by the RMA core before a seek is about to occur.
//  The stream's time value just before the seek, and the time value just
//  after the seek are provided.
//
STDMETHODIMP
CRnMp3Ren::OnPreSeek(UINT32 timeBeforeSeek,
                     UINT32 timeAfterSeek)
{
    #ifdef _WIN32
    #ifdef _DEBUG
    TCHAR szTmp[128];
    wsprintf(szTmp, TEXT("OnPreSeek timeb4 %ld timeAfter %ld\n"), timeBeforeSeek, timeAfterSeek);
    OutputDebugString(szTmp);
    m_ulPacketsDecoded = 0;
    m_ulFramesDecoded = 0;
    m_bStarted = 0;
    #endif
    #endif

    // Wait for decode thread to become idle and
    // release all buffered packets
    m_bInSeekMode = TRUE;
    m_bDiscontinuity = TRUE;

    if(m_pPacketParser)
    {
        m_pPacketParser->PreSeek();
    }

#ifdef DEMUXER
    if (m_pRegistry) m_pRegistry->SetIntByName("FirstPts", -1);
#endif

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnPostSeek                                 ref:  hxrendr.h
//
//  This routine is called by the RMA core just after a seek has occured.
//  The stream's time value just before the seek, and the time value just
//  after the seek are provided.
//
STDMETHODIMP
CRnMp3Ren::OnPostSeek(UINT32 timeBeforeSeek,
                      UINT32 timeAfterSeek)
{
    #ifdef _WIN32
    #ifdef _DEBUG
    TCHAR szTmp[128];
    wsprintf(szTmp, TEXT("OnPostSeek timeb4 %ld timeAfter %ld\n"), timeBeforeSeek, timeAfterSeek);
    OutputDebugString(szTmp);
    #endif
    #endif

    m_bInSeekMode = FALSE;

    if(m_pPacketParser)
    {
        m_pPacketParser->PostSeek(timeAfterSeek);
    }
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnPause                                    ref:  hxrendr.h
//
//  This routine is called by the RMA core just after a pause has occured.
//  The stream's time value just before pausing is provided.
//
STDMETHODIMP
CRnMp3Ren::OnPause(UINT32 timeBeforePause)
{
    #ifdef _WIN32
    #ifdef _DEBUG
    TCHAR szTmp[128];
    wsprintf(szTmp, TEXT("OnPause timeb4 %ld\n"), timeBeforePause);
    OutputDebugString(szTmp);
    #endif
    #endif

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnBuffering                                ref:  hxrendr.h
//
//  This routine is called by the RMA core to inform the Renderer that
//  buffering of data is occuring. The reason for buffering (e.g. start-up
//  of stream, seek has occured, network congestion, etc.), as well as the
//  percentage of the buffering process complete are provided.
//
STDMETHODIMP
CRnMp3Ren::OnBuffering(UINT32 reason,
                       UINT16 percentComplete)
{
    HXLOGL3(HXLOG_MP3X, "OnBuffering(%lu,%u)", reason, percentComplete);
    #ifdef _WIN32
    #ifdef _DEBUG
    TCHAR szTmp[128];
    wsprintf(szTmp, TEXT("OnBuffer percent %d\n"), percentComplete);
    OutputDebugString(szTmp);
    #endif
    #endif
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::OnEndofPackets                             ref:  hxrendr.h
//
//  Called by client engine to inform the renderer that all the
//  packets have been delivered. However, if the user seeks before
//  EndStream() is called, renderer may start getting packets again
//  and the client engine will eventually call this function again.
//
STDMETHODIMP
CRnMp3Ren::OnEndofPackets(void)
{
    m_bEndOfPackets = TRUE;
    if (m_bStarving)
    {
        m_bStarving = FALSE;
        if (m_bStream2)
        {
	    ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(m_nPacketsNeeded,m_nPacketsNeeded);
        }
        else
        {
	    m_pStream->ReportRebufferStatus(m_nPacketsNeeded,m_nPacketsNeeded);
        }
    }

    if(m_pPacketParser)
    {
        m_pPacketParser->EndOfPackets();
    }

    if(m_pDefaultPacketHookHelper)
	m_pDefaultPacketHookHelper->OnEndOfPackets();

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXRenderer::EndStream                                  ref:  hxrendr.h
//
//  This routine is called by the RMA core when the rendering of the stream
//  has completed. Deallocation of any resources should be done here.
//
STDMETHODIMP
CRnMp3Ren::EndStream(void)
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);

    HX_DELETE(m_pPacketParser);

    if(m_pDefaultPacketHookHelper)
    {
	m_pDefaultPacketHookHelper->Terminate();
	HX_RELEASE(m_pDefaultPacketHookHelper);
    }

#ifdef DEMUXER
    if (m_pRegistry)
        m_pRegistry->DeleteByName("FirstPts");
#endif

    for (int i=0; i<=m_wAudStream; i++)
        HX_RELEASE(m_aAudStreamList[i]);

    HX_RELEASE(m_pDefAudStream);
    HX_RELEASE(m_pStream);
    HX_RELEASE(m_pHeader);
    HX_RELEASE(m_pAudioPlayer);
    HX_RELEASE(m_pAudioPushdown2);
    HX_RELEASE(m_pRegistry);

    #ifdef PCM_CAPTURE
    if (fpOut)
    {
        write_pcm_tailer_wave(fpOut, g_dwTotalPcmBytes);
        fclose(fpOut);
        fpOut = NULL;
    }
    #endif //PCM_CAPTURE

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXDryNotification::OnDryNotification                   ref:  hxausvc.h
//
//  This function is called when it is time to write to audio device 
//  and there is not enough data in the audio stream. The renderer can
//  then decide to add more data to the audio stream. This should be 
//  done synchronously within the call to this function.
//  It is OK to not write any data. Silence will be played instead.
//
STDMETHODIMP
CRnMp3Ren::OnDryNotification(UINT32 /*IN*/ ulCurrentStreamTime,
                             UINT32 /*IN*/ ulMinimumDurationRequired)
{
    HXLOGL3(HXLOG_MP3X, "OnDryNotification(%lu,%lu)",
            ulCurrentStreamTime, ulMinimumDurationRequired);
    HX_RESULT hr = HXR_OK;

    // We do not set our rebuffer flag at the end of a stream
    if (m_bEndOfPackets || !m_pStream)
        return HXR_OK;

    #ifdef _DEBUG
        #ifdef _WIN32
        TCHAR szTmp[128];
        wsprintf(szTmp, TEXT("DryNot: dur %ld\n"), ulMinimumDurationRequired);
        OutputDebugString(szTmp);
        #endif
    #endif

    // NOTE: We still need to scrutinize this low-heap solution because it
    // skips the rebuffering call. XXXJHHB
#if defined(HELIX_CONFIG_ONLY_DECODE_IF_DRY)
    HX_RESULT theErr = m_pPacketParser->RenderAll();
    if( theErr == HXR_OUTOFMEMORY )
    {
        return theErr;
    }
#else
    // Rebuffer if we are late
    if (ulCurrentStreamTime+m_ulPreroll > m_ulDelay)
    {
        m_ulNumPackets = 0;
        m_bStarving = TRUE;
        
        if(m_pPacketParser)
        {
            m_nPacketsNeeded = (UINT8)(ulMinimumDurationRequired / 
                                m_pPacketParser->GetTimePerPkt() + 1);
        }

        if (m_bStream2)
        {
	    ((IHXStream2*)m_pStream)->ReportAudioRebufferStatus(m_nPacketsNeeded,0);
        }
        else
        {
	    m_pStream->ReportRebufferStatus(m_nPacketsNeeded,0);
        }

	hr = HXR_WOULD_BLOCK;
    }
#endif

    return hr;
}

STDMETHODIMP
CRnMp3Ren::InitializeStatistics(UINT32 ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS)
    m_ulRegistryID = ulRegistryID;
    m_ulChannelsRegID = 0;

    m_ulCodecRegID = 0;

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}

STDMETHODIMP
CRnMp3Ren::UpdateStatistics()
{
#if defined(HELIX_FEATURE_STATS)
    if (m_pRegistry)
    {
        char szRegistryEntry[MAX_DISPLAY_NAME] = "0"; /* Flawfinder: ignore */

        // Get the registry name for the specified id
        IHXBuffer* pszRegistryName = NULL;
        UINT32 ulChannels = m_pPacketParser ? m_pPacketParser->GetChannels() : 0;
        if (!m_ulChannelsRegID && HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pszRegistryName))
        {
            // Get the channel registry and set the value
            SafeSprintf(szRegistryEntry, MAX_DISPLAY_NAME, "%s.Channels", pszRegistryName->GetBuffer());        
            m_ulChannelsRegID = m_pRegistry->AddInt (szRegistryEntry, ulChannels);
            HX_RELEASE(pszRegistryName);
        }
        // Set the channels value based on id
        else
            m_pRegistry->SetIntById(m_ulChannelsRegID, ulChannels);

        // Set the codec name in registry
        if (!m_ulCodecRegID)
        {
            // Get the current registry key name
            if (!m_ulCodecRegID && (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pszRegistryName)))
            {
                SafeSprintf(szRegistryEntry, MAX_DISPLAY_NAME, "%s.Codec", pszRegistryName->GetBuffer());
            
                IHXBuffer *pValue = NULL;
                m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pValue);
            
                if (pValue)
                {
                    UCHAR pCodecName[] = "MPEG Audio";

                    pValue->Set((const UCHAR*)pCodecName, strlen((const char*)pCodecName)+1);
                    m_ulCodecRegID = m_pRegistry->AddStr(szRegistryEntry, pValue);

                    HX_RELEASE(pValue);
                }

                HX_RELEASE(pszRegistryName);
            }
        }
        else
        {
            IHXBuffer *pValue = NULL;
            m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pValue);
        
            if (pValue)
            {
                UCHAR pCodecName[] = "MPEG Audio";

                pValue->Set((const UCHAR*)pCodecName, strlen((const char*)pCodecName)+1);
                m_pRegistry->SetStrById(m_ulCodecRegID, pValue);

                HX_RELEASE(pValue);
            }
        }
    }


    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS */
}

/************************************************************************
 *  IHXUpdateProperties Methods
 */
/************************************************************************
 *  UpdatePacketTimeOffset
 *	Call this method to update the timestamp offset of cached packets
 */
STDMETHODIMP CRnMp3Ren::UpdatePacketTimeOffset(INT32 lTimeOffset)
{
    m_lTimeLineOffset -= lTimeOffset;

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXUpdateProperties::UpdatePlayTimes
 *	Purpose:
 *	    Call this method to update the playtime attributes
 */
STDMETHODIMP CRnMp3Ren::UpdatePlayTimes(IHXValues* pProps)
{
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Ren::~CRnMp3Ren                  ref:  ff1rendr.h
//
//  Destructor
//
CRnMp3Ren::~CRnMp3Ren(void)
{
    //--_gbInit;
    EndStream();
}

// IUnknown COM Interface Methods

///////////////////////////////////////////////////////////////////////////////
//  IUnknown::AddRef                                            ref:  hxcom.h
//
//  This routine increases the object reference count in a thread safe
//  manner. The reference count is used to manage the lifetime of an object.
//  This method must be explicitly called by the user whenever a new
//  reference to an object is used.
//
STDMETHODIMP_(UINT32)
CRnMp3Ren::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown::Release                                           ref:  hxcom.h
//
//  This routine decreases the object reference count in a thread safe
//  manner, and deletes the object if no more references to it exist. It must
//  be called explicitly by the user whenever an object is no longer needed.
//
STDMETHODIMP_(UINT32)
CRnMp3Ren::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
	    return m_RefCount;

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown:]Interface                                    ref:  hxcom.h
//
//  This routine indicates which interfaces this object supports. If a given
//  interface is supported, the object's reference count is incremented, and
//  a reference to that interface is returned. Otherwise a NULL object and
//  error code are returned. This method is called by other objects to
//  discover the functionality of this object.
//
STDMETHODIMP
CRnMp3Ren::QueryInterface(REFIID interfaceID,
                          void** ppInterfaceObj)
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
	    AddRef();
	    *ppInterfaceObj = (IUnknown*)(IHXPlugin*)this;
	    return HXR_OK;
    }

    // IHXPlugin interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXPlugin))
    {
	    AddRef();
	    *ppInterfaceObj = (IHXPlugin*)this;
	    return HXR_OK;
    }

    // IHXInterruptSafe interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXInterruptSafe))
    {
        AddRef();
        *ppInterfaceObj = (IHXInterruptSafe*)this;
        return HXR_OK;
    }
    // IHXRenderer interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXRenderer))
    {
	    AddRef();
	    *ppInterfaceObj = (IHXRenderer*)this;
	    return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXDryNotification))
    {
	    AddRef();
	    *ppInterfaceObj = (IHXDryNotification*)this;
	    return HXR_OK;
    }

#if defined(HELIX_FEATURE_STATS)
    else if (IsEqualIID(interfaceID, IID_IHXStatistics))
    {
	    AddRef();
	    *ppInterfaceObj = (IHXStatistics*)this;
	    return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_STATS) */

    else if (IsEqualIID(interfaceID, IID_IHXUpdateProperties))
    {
	AddRef();
	*ppInterfaceObj = (IHXUpdateProperties*)this;
	return HXR_OK;
    }

    else if (IsEqualIID(interfaceID, IID_IHXPacketHookHelper))
    {
	if(!m_pDefaultPacketHookHelper)
	{
	    m_pDefaultPacketHookHelper = new CHXDefaultPacketHookHelper;
	    if(m_pDefaultPacketHookHelper)
	    {
		m_pDefaultPacketHookHelper->AddRef();
		m_pDefaultPacketHookHelper->Initialize(m_pContext);
	    }
	}

	if(m_pDefaultPacketHookHelper)
	    return m_pDefaultPacketHookHelper->QueryInterface(interfaceID, ppInterfaceObj);
    }

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP
CRnMp3Ren::CheckStreamVersions(IHXValues* pHeader)
{
    // check stream and content versions so an upgrade can
    // be called if necessary...
    HX_RESULT pnr = HXR_OK;

    HXBOOL bVersionOK = TRUE;

    UINT32 ulStreamVersion = 0;
    UINT32 ulContentVersion = 0;

    if(HXR_OK == pHeader->GetPropertyULONG32("StreamVersion", 
	                                         ulStreamVersion))
    {
	    UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulStreamVersion);
	    UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulStreamVersion);

	    if((ulMajorVersion > STREAM_MAJOR_VERSION) ||
	       (ulMinorVersion > STREAM_MINOR_VERSION &&
		    ulMajorVersion == STREAM_MAJOR_VERSION))
	    {
		    bVersionOK = FALSE;
	    }
    }

    if(bVersionOK &&
       HXR_OK == pHeader->GetPropertyULONG32("ContentVersion",
                                             ulContentVersion))
    {
	    UINT32 ulMajorVersion = HX_GET_MAJOR_VERSION(ulContentVersion);
	    UINT32 ulMinorVersion = HX_GET_MINOR_VERSION(ulContentVersion);

	    if((ulMajorVersion > CONTENT_MAJOR_VERSION) ||
	       (ulMinorVersion > CONTENT_MINOR_VERSION &&
		    ulMajorVersion == CONTENT_MAJOR_VERSION))
	    {
		    bVersionOK = FALSE;
	    }
    }

    if(!bVersionOK)
    {
        AddToAutoUpgradeCollection(zm_pStreamMimeTypes[0], m_pContext);

    	pnr = HXR_FAIL;
    }

    return pnr;
}

HXBOOL
CRnMp3Ren::InitAudioStream(HXAudioFormat audioFmt)
{
    if(m_pAudioStream)
    {
        // if already inited, don't do anything
        return TRUE;
    }

    // If we could not create the stream in OnHeader
    // Create our audio stream
    m_pAudioPlayer->CreateAudioStream(&m_pAudioStream);

    if (!m_pAudioStream)
        return FALSE;

    m_pAudioStream->Init(&audioFmt, m_pHeader);

    #ifdef PCM_CAPTURE

    #ifdef _WIN32
    char                    szFile[64] = "c:\\temp.wav"; /* Flawfinder: ignore */
    #elif _MACINTOSH
    char                    szFile[64] = "temp.wav"; /* Flawfinder: ignore */
    #endif

    fpOut = fopen(szFile, "wb");
    write_pcm_header_wave(fpOut,
                          lPCMSampRate,
                          nPCMChannels,
                          nBitsPerSample,
                          0);
    g_dwTotalPcmBytes = 0;
    
    #endif // PCM_CAPTURE

    
    m_pAudioStream->AddDryNotification(this);
    m_bDiscontinuity = TRUE;

    return TRUE;
}

HXBOOL
CRnMp3Ren::ReInitAudioStream(HXAudioFormat audioFmt)
{
    #if defined _DEBUG && defined _WIN32
    OutputDebugString(TEXT("Dynamic audio src change\n"));
    #endif

    IHXAudioStream2 *pStream = NULL;
    m_pAudioStream->QueryInterface(IID_IHXAudioStream2, (void**)&pStream);

    if (pStream)
    {
        pStream->RemoveDryNotification((IHXDryNotification*)this);
        HX_RELEASE(pStream);
    }
    
    m_pAudioStream = m_aAudStreamList[++m_wAudStream];

    return InitAudioStream(audioFmt);
}

void CRnMp3Ren::Render(IHXBuffer* pPCMBuffer, double dTime)
{
    #ifdef PCM_CAPTURE
    if (fpOut)
    {
        fwrite(pPCMBuffer->GetBuffer(), dwPCM, 1, fpOut);
        g_dwTotalPcmBytes += dwPCM;
    }
    #endif //PCM_CAPTURE

    // Write to Audio Services
    HXAudioData  audioData;
    audioData.pData = pPCMBuffer;
    audioData.ulAudioTime = (UINT32)dTime;

    #ifdef _DEBUG
    ++m_ulFramesDecoded;
    #endif
    
    #if defined _DEBUG && defined _WIN32
    //wsprintf(szTmp, "Audio Pts %ld\n", audioData.ulAudioTime);
    //OutputDebugString(szTmp);
    #endif

    if (m_bDiscontinuity)
    {
        audioData.uAudioStreamType = TIMED_AUDIO;
        m_bDiscontinuity = FALSE;
    }
    else
        audioData.uAudioStreamType = STREAMING_AUDIO;


    audioData.ulAudioTime = AdjustTimestamp( (ULONG32) dTime, m_lTimeLineOffset );
    m_pAudioStream->Write(&audioData);
}

void
CRnMp3Ren::SetPacketFormat(const char* szMimeType)
{
    if(!szMimeType)
    {
        return;
    }

    int i, j;
    for (i=0; zm_pStreamMimeTypes[i] != NULL; i++)
        if (!strcasecmp(szMimeType, zm_pStreamMimeTypes[i]))
            break;

    if(zm_pStreamMimeTypes[i] == NULL)
    {
        return;
    }
    for (j=0; MIME_FMT_BASIC[j] != -1; j++)
    {
        if(MIME_FMT_BASIC[j] == i)
        {
#if defined (MPA_FMT_DRAFT00)
            m_pPacketParser = new CFmtPacketParser();
#if defined(HELIX_FEATURE_MIN_HEAP)
            m_ulPreroll = 0;
            m_pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
#endif
#endif //MPA_FMT_DRAFT00
            return;
        }
    }
    for (j=0; MIME_MPA_BASIC[j] != -1; j++)
    {
        if(MIME_MPA_BASIC[j] == i)
        {    
#if defined (MPA_FMT_RAW)         
            m_pPacketParser = new CMpaPacketParser();
#if defined(HELIX_FEATURE_MIN_HEAP)
            m_ulPreroll = 0;
            m_pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
#endif
#endif //MPA_FMT_RAW
            return;
        }
    }
    for (j=0; MIME_FMT_3119[j] != -1; j++)
    {
        if(MIME_FMT_3119[j] == i)
        {
#if defined (MPA_FMT_RFC3119)
            m_pPacketParser = new CRobustPacketParser();
#endif //MPA_FMT_RFC3119
            return;
        }
    }
    for (j=0; MIME_MPA_2250[j] != -1; j++)
    {
        if(MIME_MPA_2250[j] == i)
        {
#if defined (MPA_FMT_RFC2250)
            m_pPacketParser = new C2250PacketParser();
#if defined(HELIX_FEATURE_MIN_HEAP)
            m_ulPreroll = 0;
            m_pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
#endif
#endif //MPA_FMT_RFC2250
            return;
        }
    }

#ifdef DEMUXER
    for (j=0; MIME_MPG1_2250[j] != -1; j++)
    {
        if(MIME_MPG1_2250[j] == i)
        {
#if defined (MPA_FMT_RFC2250_SYSTEM)
            m_pPacketParser = (CPacketParser*)(CSysPacketParser*)
                              (new C2250SysPacketParser(FALSE, m_pRegistry));
#if defined(HELIX_FEATURE_MIN_HEAP)
            m_ulPreroll = 0;
            m_pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
#endif
#endif //MPA_FMT_RFC2250_SYSTEM
            return;
        }
    }
    for (j=0; MIME_MPG2_2250[j] != -1; j++)
    {
        if(MIME_MPG2_2250[j] == i)
        {
#if defined (MPA_FMT_RFC2250_SYSTEM)
            m_pPacketParser = (CPacketParser*)(CSysPacketParser*)
                              (new C2250SysPacketParser(TRUE, m_pRegistry));
#if defined(HELIX_FEATURE_MIN_HEAP)
            m_ulPreroll = 0;
            m_pHeader->SetPropertyULONG32("Preroll", m_ulPreroll);
#endif
#endif //MPA_FMT_RFC2250_SYSTEM
            return;
        }
    }
    for (j=0; MIME_MPG1_SYS[j] != -1; j++)
    {
        if(MIME_MPG1_SYS[j] == i)
        {
#if defined (MPA_FMT_SYSTEM)
            m_pPacketParser = new CSysPacketParser(FALSE, m_pRegistry);
#endif //MPA_FMT_SYSTEM
            return;
        }
    }
    for (j=0; MIME_MPG2_SYS[j] != -1; j++)
    {
        if(MIME_MPG2_SYS[j] == i)
        {
#if defined (MPA_FMT_SYSTEM)
            m_pPacketParser = new CSysPacketParser(TRUE, m_pRegistry);
#endif //MPA_FMT_SYSTEM
            return;
        }
    }
#endif // DEMUXER
}

