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

#ifndef _AUDREND_H_
#define _AUDREND_H_

/****************************************************************************
 *  Includes
 */
#include "audrendf.h"
#include "hxplugn.h"
#include "hxrendr.h"
#include "hxasm.h"
#include "hxmon.h"
#include "hxcore.h"
#include "hxtick.h"
#include "hxerror.h"

#include "audstats.h"

#define AUDREND_INITIAL_NUMSTREAMPTRS  4
#define AUDREND_NUM_STREAMTYPESTRINGS  4


/****************************************************************************
 *  CAudioRenderer
 */
class CAudioRenderer : public IHXPlugin,
		       public IHXRenderer,
		       public IHXStatistics,
		       public IHXInterruptSafe,
		       public IHXDryNotification
{
protected:
    typedef enum 
    {
	stopped, 
	buffering, 
	playing, 
	paused, 
	seeking
    } PlayState;
    
    static const char*	const zm_pDescription;
    static const char*	const zm_pCopyright;
    static const char*	const zm_pMoreInfoURL;
    static const char*	const zm_pStreamMimeTypes[];
    static const char*  const zm_pAudioStreamType[AUDREND_NUM_STREAMTYPESTRINGS];

    HX_RESULT InitAudioStream (IHXValues* pHeader,
			       IHXAudioStream** ppAudioStream);

    HX_RESULT DoAudio	      (UINT32 &ulAudioTime, 
			       AUDIO_STATE audioState  = AUDIO_NORMAL);

    HX_RESULT WriteToAudioServices (HXAudioData* pAudioData);

    HX_RESULT AttemptToSatisfyDryRequest (UINT32 ulAudioTimeWanted);

    void CalculateMaxTimeStamp(HXAudioData* pAudioData);

    HX_RESULT CheckForAudioStreamChange(REF(HXBOOL) rbAudioStreamChanged);
    HX_RESULT IsAudioStreamFormatCompatible(UINT32 i, HXBOOL& rbIsCompatible);
    HX_RESULT FindCompatibleAudioStream(UINT32& rulAudioStreamIndex);
    HX_RESULT SwitchToNewAudioStream(UINT32 ulOldStreamIndex, UINT32 ulNewStreamIndex);
    HXBOOL    HasAudioFormatChanged();
    HX_RESULT CreateAndAddNewAudioStream(UINT32& rulNewAudioStreamIndex);
    const char* GetAudioStreamTypeString(AudioStreamType eType);

    HXBOOL IsRebuffering() const;
    void StartRebuffer(UINT32 ulAudioWantedTime);
    void EndRebuffer();

    IHXAudioPlayer*	    m_pAudioPlayer;
    IHXAudioStream**        m_ppAudioStream;
    UINT32                  m_ulNumAudioStreams;
    UINT32                  m_ulCurAudioStream;
    UINT32                  m_ulNumAudioStreamPtrAlloc;
    
    UINT32		    m_ulPreroll;
    UINT32		    m_ulDelay;
    UINT32		    m_ulDuration;
    UINT32		    m_ulAudioWantedTime;
    UINT32		    m_ulLastWriteTime;
    UINT32		    m_ulBaseWriteTime;
    INT32                   m_lTimeOffset;

    PlayState		    m_PlayState;
  
    HX_BITFIELD		    m_bDoneWritingPackets:1;
    HX_BITFIELD		    m_bEndOfPackets:1;
    HX_BITFIELD		    m_bProcessingPacket:1;
    HX_BITFIELD		    m_bInSeekMode:1;
    HX_BITFIELD		    m_bFirstPacket:1;
    HX_BITFIELD		    m_bDelayOffsetSet:1;
    HX_BITFIELD             m_bCanChangeAudioStream : 1;
    HX_BITFIELD		    m_bNeedStartTime:1;
    HX_BITFIELD             m_bStream2:1;

    IHXMutex*		    m_pMutex;
    IUnknown*               m_pSrcProps;

    HX_RESULT CheckStreamVersions (IHXValues* pHeader);
    virtual HX_RESULT CheckAudioServices() {return HXR_OK;}

public:
    /*
     *	Costructor
     */
    CAudioRenderer();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(UINT32,AddRef)	(THIS);

    STDMETHOD_(UINT32,Release)	(THIS);

    /*
     *	IHXPlugin methods
     */
    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    
    /*
     *	IHXRenderer methods
     */
    /************************************************************************
     *	Method:
     *	    IHXRenderer::GetRendererInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of rendering 
     *	    plugins.
     */
    STDMETHOD(GetRendererInfo)	(THIS_
				REF(const char**) /*OUT*/ pStreamMimeTypes,
				REF(UINT32)      /*OUT*/ unInitialGranularity
				);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::StartStream
    //	Purpose:
    //	    Called by client engine to inform the renderer of the stream it
    //	    will be rendering. The stream interface can provide access to
    //	    its source or player. This method also provides access to the 
    //	    primary client controller interface.
    //
    STDMETHOD (StartStream)	(THIS_			
				IHXStream*	    pStream,
				IHXPlayer*	    pPlayer);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::EndStream
    //	Purpose:
    //	    Called by client engine to inform the renderer that the stream
    //	    is was rendering is closed.
    //
    STDMETHOD (EndStream)	(THIS);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnHeader
    //	Purpose:
    //		Called by client engine when a header for this renderer is 
    //		available. The header will arrive before any packets.
    //
    STDMETHOD (OnHeader)	(THIS_
				IHXValues*	    pHeader);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPacket
    //	Purpose:
    //	    Called by client engine when a packet for this renderer is 
    //	    due.
    //
    STDMETHOD (OnPacket)	(THIS_
				IHXPacket*	    pPacket,
				LONG32		    lTimeOffset);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnTimeSync
    //	Purpose:
    //	    Called by client engine to inform the renderer of the current
    //	    time relative to the streams synchronized time-line. The 
    //	    renderer should use this time value to update its display or
    //	    render it's stream data accordingly.
    //
    STDMETHOD (OnTimeSync)	(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //	    IHXRenderer::OnPreSeek
    //  Purpose:
    //	    Called by client engine to inform the renderer that a seek is
    //	    about to occur. The render is informed the last time for the 
    //	    stream's time line before the seek, as well as the first new
    //	    time for the stream's time line after the seek will be completed.
    //
    STDMETHOD (OnPreSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPostSeek
    //	Purpose:
    //	    Called by client engine to inform the renderer that a seek has
    //	    just occured. The render is informed the last time for the 
    //	    stream's time line before the seek, as well as the first new
    //	    time for the stream's time line after the seek.
    //
    STDMETHOD (OnPostSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //	    IHXRenderer::OnPause
    //	Purpose:
    //	    Called by client engine to inform the renderer that a pause has
    //	    just occured. The render is informed the last time for the 
    //	    stream's time line before the pause.
    //
    STDMETHOD (OnPause)		(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnBegin
    //	Purpose:
    //		Called by client engine to inform the renderer that a begin or
    //		resume has just occured. The render is informed the first time 
    //		for the stream's time line after the resume.
    //
    STDMETHOD (OnBegin)		(THIS_
				ULONG32		    ulTime);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::OnBuffering
    //	Purpose:
    //		Called by client engine to inform the renderer that buffering
    //		of data is occuring. The render is informed of the reason for
    //		the buffering (start-up of stream, seek has occured, network
    //		congestion, etc.), as well as percentage complete of the 
    //		buffering process.
    //
    STDMETHOD (OnBuffering)	(THIS_
				ULONG32		    ulFlags,
				UINT16		    unPercentComplete);

    /////////////////////////////////////////////////////////////////////////
    //	Method:
    //		IHXRenderer::GetDisplayType
    //	Purpose:
    //		Called by client engine to ask the renderer for it's preferred
    //		display type. When layout information is not present, the 
    //		renderer will be asked for it's prefered display type. Depending
    //		on the display type a buffer of additional information may be 
    //		needed. This buffer could contain information about preferred
    //		window size.
    //
    STDMETHOD (GetDisplayType)	(THIS_
				REF(HX_DISPLAY_TYPE)	ulFlags,
				REF(IHXBuffer*)	pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnEndofPackets
     *	Purpose:
     *	    Called by client engine to inform the renderer that all the
     *	    packets have been delivered. However, if the user seeks before
     *	    EndStream() is called, renderer may start getting packets again
     *	    and the client engine will eventually call this function again.
     */
    STDMETHOD(OnEndofPackets)	(THIS);
    
    /*
     *  IHXDryNotification methods
     */
    /************************************************************************
     *  Method:
     *      IHXDryNotificationOnDryNotification
     *  Purpose:
     *	    This function is called when it is time to write to audio device 
     *	    and there is not enough data in the audio stream. The renderer can
     *	    then decide to add more data to the audio stream. This should be 
     *	    done synchronously within the call to this function.
     *	    It is OK to not write any data. Silence will be played instead.
     */
    STDMETHOD(OnDryNotification) (THIS_
				  UINT32 /*IN*/ ulCurrentStreamTime,
				  UINT32 /*IN*/ ulMinimumDurationRequired);

    /*
     * IHXStatistics methods
     */
    STDMETHOD(InitializeStatistics) (THIS_ UINT32 ulRegistryID);
    STDMETHOD(UpdateStatistics)     (THIS);

    /*
     *	IHXInterruptSafe methods
     */
    /************************************************************************
     *	Method:
     *	    IHXInterruptSafe::IsInterruptSafe
     *	Purpose:
     *	    This is the function that will be called to determine if
     *	    interrupt time execution is supported.
     */
    STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS) 
    					{ return TRUE; };

    IUnknown* GetContext(void)
    {
	return m_pContext;
    }

    static LONG32 CmpTime(ULONG32 ulTime1, ULONG32 ulTime2)
    {
	return ((LONG32) (ulTime1 - ulTime2));
    }

    /************************************************************************
     *	Public Statistics Methods
     */
#if defined(HELIX_FEATURE_STATS)
    HX_RESULT ReportStat(AudioStatEntryID eEntryID, char* pVal)
    {
	return m_pAudioStats->ReportStat(eEntryID, pVal);
    }
    
    HX_RESULT ReportStat(AudioStatEntryID eEntryID, INT32 lVal)
    {
	return m_pAudioStats->ReportStat(eEntryID, lVal);
    }
#else	// HELIX_FEATURE_STATS

    HX_RESULT ReportStat(AudioStatEntryID eEntryID, char* pVal)
    {
	return HXR_OK;
    }
    
    HX_RESULT ReportStat(AudioStatEntryID eEntryID, INT32 lVal)
    {
	return HXR_OK;
    }
#endif	// HELIX_FEATURE_STATS

protected:
    virtual ~CAudioRenderer();

    /*
     *	Renderer's customizable fuctions - can be called any time
     */
    /*
     *	Fixed Renderer Configuration - must be callable at any time
     */
    virtual const char* GetUpgradeMimeType(void);

    virtual const char* GetRendererName(void);

    virtual const char* GetCodecName(void);

    virtual const char* GetCodecFourCC(void);

    /*
     *	Renderer Configuration - must be callable any time
     *                           after reception of the header
     */
    virtual void GetStreamVersion(ULONG32 &ulThisMajorVersion, 
				  ULONG32 &ulThisMinorVersion);

    virtual void GetContentVersion(ULONG32 &ulThisMajorVersion, 
				   ULONG32 &ulThisMinorVersion);

    virtual CAudioFormat* CreateFormatObject(IHXValues* pHeader);

    /*
     *	Renderer's member variables sharable with the derived renderer
     */
    IUnknown*				m_pContext;
    IHXStream*				m_pStream;
    IHXBackChannel*			m_pBackChannel;
    IHXValues*				m_pHeader;
    IHXErrorMessages*                   m_pErrorMessages;
    IHXCommonClassFactory*		m_pCommonClassFactory;
    IHXPreferences*			m_pPreferences;
    IHXRegistry*			m_pRegistry;
    ULONG32				m_ulRegistryID;

    CAudioFormat*			m_pAudioFormat;

#if defined(HELIX_FEATURE_STATS)
    CAudioStatistics*			m_pAudioStats;
#else
    void*                               m_pAudioStats;
#endif /* HELIX_FEATURE_STATS */

    LONG32				m_lRefCount;
};

#endif // _AUDREND_H_

