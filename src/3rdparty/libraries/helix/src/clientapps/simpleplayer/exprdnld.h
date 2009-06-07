/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
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

#ifndef _EXPRDNLD_H_
#define _EXPRDNLD_H_

// /#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)

/****************************************************************************
 * 
 *  Class:
 *
 *	ExamplePDStatusObserver
 *
 *  Purpose:
 *
 *	Implementation for IHXPDStatusObserver which receives progressive-
 *      download status reports:
 *
 */
class ExamplePDStatusObserver : 
	public IHXPDStatusObserver
{
private:
    INT32		    m_lRefCount;
    IHXPDStatusMgr*         m_pPrgDnldStatusMgr;
    IUnknown*		    m_pUnkPlayer;
    IHXPlayer*              m_pHXPlayer;
    HXBOOL                  m_bPlayerIsPausedByThis;
    HXBOOL                  m_bFirstPDStatusMessage;
    UINT32                  m_ulTotalDurReported;
    UINT32                  m_ulDurSoFar;
    UINT32                  m_ulCurStatusUpdateGranularity;
    HXBOOL                  m_bInitialPrerollUpateGranularitySet;
    HXBOOL                  m_bDownloadIsComplete;

    ExamplePDStatusObserver();
    ~ExamplePDStatusObserver();

public:
    ExamplePDStatusObserver(IUnknown* pUnkPlayer);
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *  IHXPDStatusObserver methods
     */

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadProgress
     *
     *  Purpose:
     *      Notification from IHXPDStatusMgr of download progress when
     *      file size changes.
     *
     *      lTimeSurplus:
     *      - When negative, the absolute value of it is the estimated number
     *      of milliseconds of wall-clock time that need to pass while
     *      downloading continues before reaching the point at which playback
     *      can resume and play the remainder of the stream without having to
     *      buffer, assuming that playback is paused and remains so during
     *      that period.
     *      - When positive, it is the estimated number of milliseconds of
     *      wall-clock time between when the download should complete and when
     *      the natural content play-out duration will be reached, assuming
     *      playback is currently progressing and that no pause will occur.
     *
     *      Note: ulNewDurSoFar can be HX_PROGDOWNLD_UNKNOWN_DURATION if the
     *      IHXMediaBytesToMediaDurConverter was not available to, or was
     *      unable to convert the bytes to a duration for the IHXPDStatusMgr
     *      calling this:
     */
    STDMETHOD(OnDownloadProgress) (THIS_
            IHXStreamSource* /*IN*/  /*NULL is valid value*/ pStreamSource,
            UINT32 /*IN*/ ulNewDurSoFar,
            UINT32 /*IN*/ ulNewBytesSoFar,
            INT32  /*IN*/ lTimeSurplus);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnTotalDurChanged
     *  Purpose:
     *      This is a notification if the total file duration becomes known
     *      or becomes better-known during download/playback
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnTotalDurChanged)  (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 /*IN*/ ulNewDur);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadComplete
     *
     *  Purpose:
     *      Notification that the entire file has been downloaded.
     *
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     *      
     */
    STDMETHOD(OnDownloadComplete)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::SrcClaimsSeekSupport
     *
     *  Purpose:
     *      Passes along notification from file sys that seek support
     *      is claimed to be available (although sometimes HTTP server
     *      claims this when it doesn't actually support it).
     *
     */
    STDMETHOD(SrcClaimsSeekSupport)    (THIS_
            IHXStreamSource* pStreamSource,
            HXBOOL /*IN*/ bSrcClaimsSeekSupport);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadPause
     *  Purpose:
     *      Notification that the file-download process has purposefully
     *      and temporarily halted downloading of the file
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnDownloadPause)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);

    /************************************************************************
     *  Method:
     *      IHXPDStatusObserver::OnDownloadResume
     *  Purpose:
     *      Notification that the file-download process has resumed
     *      the process of downloading the remainder of the file
     *      
     *      Note: pStreamSource can be NULL.  This will be true when
     *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
     *      object.
     */
    STDMETHOD(OnDownloadResume)    (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource);
};
// /#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#endif // _EXPRDNLD_H_

