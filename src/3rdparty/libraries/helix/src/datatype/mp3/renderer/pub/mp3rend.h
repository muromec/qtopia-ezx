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

#ifndef _HXMP3REN_H_
#define _HXMP3REN_H_

#include "ihxdefpackethookhlp.h"
#include "baseobj.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
//

#define MY_PLUGIN_VERSION    0
#define MY_DESCRIPTION       "RealNetworks MPEG Renderer Plugin"
#define MY_COPYRIGHT         "(c) 1999-2000 RealNetworks, Inc. All rights reserved."
#define MY_MORE_INFO_URL     "http://www.real.com"

#ifdef DEMUXER
#define MY_STREAM_MIME_TYPES {  "audio/X-MP3-draft-00",     \
                                "audio/X-MP3-draft-00-RN",  \
                                "audio/MPEG-ELEMENTARY",    \
                                "audio/MPEG-ELEMENTARY-RN", \
                                "audio/MPEG-ELEMENTARY-RAW",\
                                "audio/rn-mpeg",            \
                                "audio/mpa-robust",         \
                                "audio/MPA",                \
                                "audio/mp1s",               \
                                "audio/mp2p",               \
                                "audio/vnd.rn-mp1s",        \
                                "audio/vnd.rn-mp2p",        \
				NULL}


const int MIME_MPG1_2250[]  = { 8, -1 };
const int MIME_MPG2_2250[]  = { 9, -1 };
const int MIME_MPG1_SYS []  = { 10, -1 };
const int MIME_MPG2_SYS[]   = { 11, -1 };

#else
#define MY_STREAM_MIME_TYPES {  "audio/X-MP3-draft-00",     \
                                "audio/X-MP3-draft-00-RN",  \
                                "audio/MPEG-ELEMENTARY",    \
                                "audio/MPEG-ELEMENTARY-RN", \
                                "audio/MPEG-ELEMENTARY-RAW",\
                                "audio/rn-mpeg",            \
                                "audio/mpa-robust",         \
                                "audio/MPA",                \
				NULL}

#endif

const int MIME_FMT_BASIC[]  = { 0, 1, -1 };
const int MIME_MPA_BASIC[]  = { 2, 3, 4, 5, -1 };
const int MIME_FMT_3119[]   = { 6, -1 };
const int MIME_MPA_2250[]   = { 7, -1 };

#define STREAM_MAJOR_VERSION  0
#define STREAM_MINOR_VERSION  0

#define CONTENT_MAJOR_VERSION 0
#define CONTENT_MINOR_VERSION 0

///////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CPacketParser;

///////////////////////////////////////////////////////////////////////////////
//
//  CRnMp3Ren Class
//
//  This class inherits the minimum number of interfaces required to create
//  an RMA Renderer plug-in. The IHXRenderer interface provides routines
//  for handling all rendering operations. The IHXPlugin interface is
//  required by all plug-ins to handle fundamental plug-in operations. Since
//  this is a COM object, this class also inherits COM's IUnknown interface
//  to handle object reference counting and interface query.
//

class CRnMp3Ren :   public IHXRenderer,
                    public IHXInterruptSafe,
                    public IHXDryNotification,
                    public IHXStatistics,
                    public IHXPlugin,
		    public IHXUpdateProperties,
		    public CHXBaseCountingObject
{
public:
    CRnMp3Ren();
    virtual ~CRnMp3Ren(void);

    ///////////////////////////////////////////////////////////////////////////
    //  IHXRenderer Interface Methods                       ref:  hxrendr.h
    //
    STDMETHOD(StartStream) (THIS_ IHXStream* pStream, IHXPlayer* pPlayer);
    STDMETHOD(EndStream )  (THIS);
    STDMETHOD(OnHeader  )  (THIS_ IHXValues* pStreamHeaderObj);
    STDMETHOD(OnPacket  )  (THIS_ IHXPacket* pPacket, INT32 timeOffset);
    STDMETHOD(OnTimeSync)  (THIS_ UINT32 currentPlayBackTime);
    STDMETHOD(OnPreSeek )  (THIS_ UINT32 timeBefore, UINT32 timeAfter);
    STDMETHOD(OnPostSeek)  (THIS_ UINT32 timeBefore, UINT32 timeAfter);
    STDMETHOD(OnPause   )  (THIS_ UINT32 timeBeforePause);
    STDMETHOD(OnBegin   )  (THIS_ UINT32 timeAfterBegin);
    STDMETHOD(OnBuffering) (THIS_ UINT32 reason, UINT16 percentComplete);

    STDMETHOD(GetRendererInfo)
        (THIS_
         REF(const char**) pStreamMimeTypes,
         REF(UINT32)       initialGranularity
        );

    STDMETHOD(GetDisplayType)
        (THIS_
         REF(HX_DISPLAY_TYPE) displayType,
         REF(IHXBuffer*)      pDisplayInfo
        );

    STDMETHOD(OnEndofPackets) (THIS);

    ///////////////////////////////////////////////////////////////////////////
    //  IHXInterruptSafe Interface Methods
    //
    STDMETHOD_(int,IsInterruptSafe)()
    {return TRUE;};
    
    ///////////////////////////////////////////////////////////////////////////
    //  IHXDryNotification Interface Methods                ref:  hxausvc.h
    //
    STDMETHOD(OnDryNotification)
    (THIS_
     UINT32 /*IN*/ ulCurrentStreamTime,
     UINT32 /*IN*/ ulMinimumDurationRequired
    );

    ///////////////////////////////////////////////////////////////////////////
    //  IHXStatistics Interface Methods                    ref:  hxausvc.h
    //
    STDMETHOD (InitializeStatistics)(THIS_ UINT32 ulRegistryID);
    STDMETHOD (UpdateStatistics)(THIS);
    
    ///////////////////////////////////////////////////////////////////////////
    //  IHXPlugin Interface Methods                         ref:  hxplugn.h
    //
    STDMETHOD(GetPluginInfo)
        (THIS_ 
          REF(int)         bLoadMultiple,
          REF(const char*) pDescription,
          REF(const char*) pCopyright,
          REF(const char*) pMoreInfoURL,
          REF(UINT32)      versionNumber
        );

    STDMETHOD(InitPlugin) (THIS_ IUnknown* pHXCore);

    ///////////////////////////////////////////////////////////////////////////
    //  IUnknown COM Interface Methods                          ref:  hxcom.h
    //
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePacketTimeOffset
     *	Purpose:
     *	    Call this method to update the timestamp offset of cached packets
     */
    STDMETHOD(UpdatePacketTimeOffset) (THIS_ INT32 lTimeOffset);

    /************************************************************************
     *	Method:
     *	    IHXUpdateProperties::UpdatePlayTimes
     *	Purpose:
     *	    Call this method to update the playtime attributes
     */
    STDMETHOD(UpdatePlayTimes)	      (THIS_
				       IHXValues* pProps);

    HXBOOL InitAudioStream(HXAudioFormat audioFmt);
    HXBOOL ReInitAudioStream(HXAudioFormat audioFmt);
    virtual void Render(IHXBuffer* pPCMBuffer, double dTime);

protected:
    // Private Class Methods

    STDMETHOD (CheckStreamVersions) (THIS_ IHXValues* pHeader);    

    void    SetPacketFormat(const char* szMimeType);

    // Private Class Variables
    INT32                       m_RefCount;         // Object's reference count
    IUnknown*		            m_pContext;
    IHXCommonClassFactory*     m_pClassFactory;    // Creates common RMA classes
    IHXStream                  *m_pStream;
    IHXAudioPlayer             *m_pAudioPlayer;    // Audio services
    IHXAudioStream             *m_pDefAudStream,   // Default pcm stream properties
                                *m_aAudStreamList[256],
                                *m_pAudioStream;    // Buffer to write PCM data
    IHXAudioPushdown2		*m_pAudioPushdown2;
    IHXValues                  *m_pHeader;         // For audio init

    CPacketParser               *m_pPacketParser;   // Packet parser

    UCHAR                       m_bInSeekMode,      // In the process of seeking
                                m_bDiscontinuity,   // Break in the PCM data
                                m_bEndOfPackets,    // No more packets are coming
                                m_bStarving;        // We are rebuffering

    UINT8                       m_wAudStream,       // Index of our audio stream list
                                m_nPacketsNeeded;   // Num packets in preroll

    UINT32                      m_ulDelay,          // ms of delay for presentation
                                m_ulNumPackets,    
                                m_ulPreroll;
    LONG32			m_lTimeLineOffset;  // ms of presentation time-line offset

    // Statistics members
    IHXRegistry                 *m_pRegistry;
    UINT32                      m_ulRegistryID,    
                                m_ulChannelsRegID,
                                m_ulCodecRegID;

    
    UINT32                      m_ulPacketsDecoded,
                                m_ulFramesDecoded,
                                m_ulBadPackets;
    UCHAR                       m_bStarted;
    HXBOOL                      m_bTrustPackets;
    HXBOOL                      m_bStream2;
    UINT32                      m_ulLastTimeSync;

    // Private Static Class Variables
    static const char* const      zm_pDescription;
    static const char* const     zm_pCopyright;
    static const char* const     zm_pMoreInfoURL;
    static const char* const     zm_pStreamMimeTypes[];

    IHXDefaultPacketHookHelper*	m_pDefaultPacketHookHelper;

};

#endif

