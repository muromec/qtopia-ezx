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

#ifndef _HXPRDNLD_H_
#define _HXPRDNLD_H_

/*
 * Forward declarations of interfaces defined in this file:
 */
typedef _INTERFACE  IHXPDStatusObserver     IHXPDStatusObserver;

/*
 * Forward declarations of interfaces referenced in this file:
 */
typedef _INTERFACE  IHXStreamSource         IHXStreamSource;


/****************************************************************************
 *  Defines:
 *    HX_PROGDOWNLD_...
 *  Purpose:
 *    Flags for getting and reporting media duration from file bytes:
 */
#define HX_PROGDOWNLD_UNKNOWN_DURATION     ((UINT32)(-1))
/*1981342000 is 22d+22h+22m+22s as used by Helix SMIL2 implmentation*/
#define HX_PROGDOWNLD_INDEFINITE_DURATION  1981342000
#define HX_PROGDOWNLD_UNKNOWN_FILE_SIZE     ((UINT32)(-1))
#define HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS  ((INT32)0x7FFFFFFF)
#define HX_PROGDOWNLD_MIN_TIME_SURPLUS      ((INT32)0x80000010)
#define HX_PROGDOWNLD_MAX_TIME_SURPLUS      ((INT32)0x7FFFFFF0)

#define PRDNFLAG_REPORT_CUR_DUR         0x01
#define PRDNFLAG_REPORT_CUR_DUR_UNKNOWN 0x02
#define PRDNFLAG_REPORT_TOTAL_DUR       0x04
#define PRDNFLAG_REPORT_DNLD_COMPLETE   0x08
#define PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_CLAIMED     0x10
#define PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_NOT_CLAIMED 0x20
#define PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_PAUSE  0x100
#define PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_RESUME 0x200

#define HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC 3000 /* in millisec.*/
#define PRDN_DEFAULT_STATUSREPORT_INTERVAL_MSEC HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC

#define HX_PROGDOWNLD_DNLD_RATE_MOVING_WINDOW_SZ        60000 /* in millisec.*/

 /****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPDStatusMgr
 * 
 *  Purpose:
 * 
 *      Object that reports change in file download status to each registered
 *      IHXPDStatusReporterObserver
 * 
 *  IID_IHXPDStatusMgr:
 * 
 *      {00004500-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPDStatusMgr, 0x00004500, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPDStatusMgr

DECLARE_INTERFACE_(IHXPDStatusMgr, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXPDStatusMgr methods
     */        

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::AddObserver
     *  Purpose:
     *      Lets an observer register so it can be notified of file changes
     */
    STDMETHOD(AddObserver) (THIS_
            IHXPDStatusObserver* /*IN*/ pObserver) PURE;

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::RemoveObserver
     *  Purpose:
     *      Lets an observer unregister so it can stop being notified of
     *      file changes
     */
    STDMETHOD(RemoveObserver) (THIS_
            IHXPDStatusObserver* /*IN*/ pObserver) PURE;

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
     *  Purpose:
     *      Lets an observer set the interval that the reporter (fsys) takes
     *      between status updates:
     */
    STDMETHOD(SetStatusUpdateGranularityMsec) (THIS_
            UINT32 /*IN*/ ulStatusUpdateGranularityInMsec) PURE;


};


 /****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPDStatusObserver
 * 
 *  Purpose:
 * 
 *      Object that registers with a IHXPDStatusMgr to be notified of
        file download status changes
 * 
 *  IID_IHXPDStatusObserver:
 * 
 *      {00004501-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPDStatusObserver, 0x00004501, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPDStatusObserver

DECLARE_INTERFACE_(IHXPDStatusObserver, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)  (THIS) PURE;

    STDMETHOD_(UINT32,Release) (THIS) PURE;

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
     *      IHXMediaBytesToMediaDur was not available to, or was
     *      unable to convert the bytes to a duration for the IHXPDStatusMgr
     *      calling this:
     */
    STDMETHOD(OnDownloadProgress) (THIS_
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 /*IN*/ ulNewDurSoFar,
            UINT32 /*IN*/ ulNewBytesSoFar,
            INT32  /*IN*/ lTimeSurplus) PURE;

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
            UINT32 /*IN*/ ulNewDur) PURE;

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
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource) PURE;

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
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            HXBOOL /*IN*/ bSrcClaimsSeekSupport) PURE;

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
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource) PURE;

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
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource) PURE;
};


 /****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXMediaBytesToMediaDur
 * 
 *  Purpose:
 * 
 *      Object that converts a progressively-downloading source file's current
 *      size to an associated duration, exact or best guess.
 * 
 *  IID_IHXMediaBytesToMediaDur:
 * 
 *      {00004510-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMediaBytesToMediaDur, 0x00004510, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMediaBytesToMediaDur

DECLARE_INTERFACE_(IHXMediaBytesToMediaDur, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;


    /*
     *  IHXMediaBytesToMediaDur methods
     */        

    /************************************************************************
     *  Method:
     *      IHXMediaBytesToMediaDur::ConvertFileOffsetToDur
     *  Purpose:
     *      Gets the duration associated with the last byte read so far.
     *      
     *      With ulLastReadOffset, the FF can match up where its last Read
     *      was with what it knows the dur was for that Read and thus
     *      better estimate the duration at the given file offset.
     *
     *      It is possible that the duration is resolved to be indefinite
     *      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
     *
     *      Also, l64FileSize can be HX_PROGDOWNLD_UKNOWN_FILE_SIZE and
     *      this method should still try to associate a duration with the
     *      bytes up to the last read.
     *
     *      If this fails (returns HXR_FAIL) then calling object should
     *      instead notify its IHXPDStatusMgr that the file size
     *      changed but that it doesn't have new dur info.  That way, at
     *      least the observer can know (& show) progress is happening.
     */
    STDMETHOD(ConvertFileOffsetToDur) (THIS_
             UINT32 /*IN*/ ulLastReadOffset,
             UINT32 /*IN*/ ulFileSize,
             REF(UINT32) /*OUT*/ ulREFDuration) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaBytesToMediaDur::GetFileDuration
     *  Purpose:
     *      Gets the duration associated with the entire file.
     *
     *      It is possible that the duration is resolved to be indefinite
     *      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
     *
     *      Callers can pass in HX_PROGDOWNLD_UKNOWN_FILE_SIZE if file size
     *      is not known to them and FF may still be able to determine
     *      ulREFDur.  However, if FF can't determine the duration of whole
     *      file (e.g., in a SMIL2 file), it returns HXR_FAIL and ulREFDur is
     *      set to HX_PROGDOWNLD_UNKNOWN_DURATION.
     */
    STDMETHOD(GetFileDuration) (THIS_
            UINT32 /*IN*/ ulCompleteFileSize,
            REF(UINT32) /*OUT*/ ulREFDur) PURE;

};



#endif  /* _HXPRDNLD_H_ */
