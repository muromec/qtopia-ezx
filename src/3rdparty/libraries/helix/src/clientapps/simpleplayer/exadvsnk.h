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

#ifndef _EXAMPLECLSNK_
#define _EXAMPLECLSNK_

struct IHXClientAdviseSink;
struct IHXGroupSink;
struct IUnknown;
struct IHXRegistry;
struct IHXScheduler;
struct IHXCallback;
struct IHXPlayer;
struct IHXGroup;

class ExampleClientAdviceSink : public IHXClientAdviseSink,
                                public IHXGroupSink,
                                public IHXCallback
{
  private:
    LONG32          m_lRefCount;
    LONG32          m_lClientIndex;
    
    IUnknown*       m_pUnknown;
    IHXRegistry*    m_pRegistry;
    IHXScheduler*   m_pScheduler;
    
    UINT32          m_ulStartTime;
    UINT32          m_ulStopTime;
    
    UINT32    m_lCurrentBandwidth;
    UINT32    m_lAverageBandwidth;
    HXBOOL      m_bOnStop;
  
    HXBOOL      m_bWaitForTrackStart;
    
    
    // IHXCallback
	IHXPlayer*		m_pPlayer;
    ULONG32         m_hCallback;
    ~ExampleClientAdviceSink();
    HX_RESULT DumpRegTree(const char* pszTreeName );


        void GetStatistics (char* /*IN*/ pszRegistryKey);
    void GetAllStatistics (void);
	void SetClipInfo( IHXPlayer* m_pRMAPlayer);
	void PrintPropName( IHXValues* pHeader );

  public:

    ExampleClientAdviceSink(IUnknown* /*IN*/ pUnknown, LONG32 /*IN*/ lClientIndex);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef) (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXClientAdviseSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPosLength
     *  Purpose:
     *      Called to advise the client that the position or length of the
     *      current playback context has changed.
     */
    STDMETHOD(OnPosLength) (THIS_
                            UINT32    ulPosition,
                            UINT32    ulLength);
    
    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationOpened
     *  Purpose:
     *      Called to advise the client a presentation has been opened.
     */
    STDMETHOD(OnPresentationOpened) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationClosed
     *  Purpose:
     *      Called to advise the client a presentation has been closed.
     */
    STDMETHOD(OnPresentationClosed) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStatisticsChanged
     *  Purpose:
     *      Called to advise the client that the presentation statistics
     *      have changed. 
     */
    STDMETHOD(OnStatisticsChanged) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPreSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek is
     *      about to occur. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek will be completed.
     *
     */
    STDMETHOD (OnPreSeek) (THIS_
                           ULONG32 ulOldTime,
                           ULONG32  ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPostSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek has
     *      just occured. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek.
     *
     */
    STDMETHOD (OnPostSeek) (THIS_
                            ULONG32 ulOldTime,
                            ULONG32 ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStop
     *  Purpose:
     *      Called by client engine to inform the client that a stop has
     *      just occured. 
     *
     */
    STDMETHOD (OnStop) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPause
     *  Purpose:
     *      Called by client engine to inform the client that a pause has
     *      just occured. The render is informed the last time for the 
     *      stream's time line before the pause.
     *
     */
    STDMETHOD (OnPause) (THIS_
                         ULONG32 ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBegin
     *  Purpose:
     *      Called by client engine to inform the client that a begin or
     *      resume has just occured. The render is informed the first time 
     *      for the stream's time line after the resume.
     *
     */
    STDMETHOD (OnBegin) (THIS_
                         ULONG32 ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBuffering
     *  Purpose:
     *      Called by client engine to inform the client that buffering
     *      of data is occuring. The render is informed of the reason for
     *      the buffering (start-up of stream, seek has occured, network
     *      congestion, etc.), as well as percentage complete of the 
     *      buffering process.
     *
     */
    STDMETHOD (OnBuffering) (THIS_
                             ULONG32 ulFlags,
                             UINT16 unPercentComplete);


    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnContacting
     *  Purpose:
     *      Called by client engine to inform the client is contacting
     *      hosts(s).
     *
     */
    STDMETHOD (OnContacting) (THIS_
                              const char* pHostName);

    // IHXCallback
    STDMETHOD(Func)            (THIS);

    /*
     *  IHXGroupSink methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupAdded
    *  Purpose:
    *		Notification of a new group being added to the presentation.
    */
    STDMETHOD(GroupAdded)    (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    IHXGroup*	    /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupRemoved
    *  Purpose:
    *		Notification of a group being removed from the presentation.
    */
    STDMETHOD(GroupRemoved)    (THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::AllGroupsRemoved
    *  Purpose:
    *		Notification that all groups have been removed from the 
    *		current presentation.
    */
    STDMETHOD(AllGroupsRemoved)  (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackAdded
    *  Purpose:
    *		Notification of a new track being added to a group.
    */
    STDMETHOD(TrackAdded)  (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    UINT16 	    /*IN*/ uTrackIndex,
			    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackRemoved
    *  Purpose:
    *		Notification of a track being removed from a group.
    */
    STDMETHOD(TrackRemoved)    (THIS_
				UINT16 		/*IN*/ uGroupIndex,
				UINT16 		/*IN*/ uTrackIndex,
				IHXValues*	/*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStarted
    *  Purpose:
    *		Notification of a track being started (to get duration, for
    *		instance...)
    */
    STDMETHOD (TrackStarted)	(THIS_
				UINT16	    /*IN*/ uGroupIndex,
				UINT16	    /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack) ;
    
    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStopped
    *  Purpose:
    *		Notification of a track being stopped
    *
    */
    STDMETHOD(TrackStopped)	(THIS_
				UINT16	    /*IN*/ uGroupIndex,
				UINT16	    /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::CurrentGroupSet
    *  Purpose:
    *		This group is being currently played in the presentation.
    */
    STDMETHOD(CurrentGroupSet)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup);

};

#endif /* _EXAMPLECLSNK_ */
