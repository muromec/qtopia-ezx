/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxflsrc.h,v 1.38 2008/09/15 06:04:01 vtyagi Exp $
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

#ifndef _HX_FILE_SOURCE_
#define _HX_FILE_SOURCE_

#include "hxbsrc.h"
#include "hxsrc.h"

// forward decl..
class CHXString;

struct IHXFileMimeMapperResponse;
struct IHXFormatResponse;
struct IHXPluginSearchEnumerator;

#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxfiles.h"
#include "recognizer.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

class HXSourceBufferStats;

class HXFileSource : public HXSource, 
		      public IHXFormatResponse,
                      public IHXHTTPRedirectResponse
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                    , public IHXPDStatusMgr
                    , public IHXPDStatusObserver
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
                    , public IHXFileFormatFinder
{
public:
			 HXFileSource(void);

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPendingStatus methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPendingStatus::GetStatus
     *	Purpose:
     *	    Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus)	(THIS_
				REF(UINT16) uStatusCode, 
				REF(IHXBuffer*) pStatusDesc, 
				REF(UINT16) ulPercentDone);

    /*
     *	IHXRegistryID methods
     */

    /************************************************************************
     *	Method:
     *	    IHXRegistryID::GetID
     *	Purpose:
     *	    Get registry ID(hash_key) of the objects(player, source and stream)
     *
     */
    STDMETHOD(GetID)		(THIS_
				REF(UINT32) /*OUT*/  ulRegistryID);

    /************************************************************************
     *	Method:
     *	    IHXInfoLogger::LogInformation
     *	Purpose:
     *	    Logs any user defined information in form of action and 
     *	    associated data.
     */
    STDMETHOD(LogInformation)		(THIS_				
					const char* /*IN*/ pAction,
					const char* /*IN*/ pData);

    void	ReSetup();

	    HX_RESULT 	Setup(const CHXURL* pURL, HXBOOL bAltURL);

    virtual HX_RESULT	DoCleanup(EndCode endCode = END_STOP);

    virtual HX_RESULT	DoSeek(ULONG32 seekTime);
    
    virtual HX_RESULT	DoPause(void);

    virtual HX_RESULT	DoResume(UINT32 ulLoopEntryTime = 0,
				 UINT32 ulProcessingTimeAllowance = 0);

    virtual HX_RESULT	StartInitialization(void);
    virtual HX_RESULT StopInitialization(void);

    virtual UINT16	GetNumStreams(void);

    virtual HX_RESULT	GetStreamInfo(ULONG32 ulStreamNumber,
				      STREAM_INFO*& theStreamInfo);

    virtual HX_RESULT	GetEvent(UINT16 usStreamNumber, 
				 CHXEvent* &theEvent, 
				 UINT32 ulLoopEntryTime,
				 UINT32 ulProcessingTimeAllowance);

    virtual HXBOOL	IsStatisticsReady(void);

#if defined(HELIX_FEATURE_ASM)
    virtual HXBOOL	IsSimulatedNetworkPlayback()  {return (m_pSimulatedSourceBandwidth != NULL);};
#endif /* HELIX_FEATURE_ASM */

    virtual HXBOOL     IsNetworkAccess();

	    HXBOOL	IsSourceDone(void);

    /*
     *	IHXFormatResponse methods
     */

    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status);

    STDMETHOD(FileHeaderReady)		(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader);

    STDMETHODIMP	StreamHeaderReady(HX_RESULT status, IHXValues* pHeader);
    STDMETHODIMP	PacketReady(HX_RESULT status, IHXPacket* pPacket);
    STDMETHODIMP	StreamDone(UINT16   unStreamNumber);


    STDMETHODIMP	SeekDone(HX_RESULT status);

    /************************************************************************
     *	Method:
     *	    IHXHTTPRedirectResponse::RedirectDone
     *	Purpose:
     *	    return the redirect URL
     */
    STDMETHOD(RedirectDone)		(THIS_ IHXBuffer* pURL);
    
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
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
                           IHXPDStatusObserver* /*IN*/ pObserver);

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::RemoveObserver
     *  Purpose:
     *      Lets an observer unregister so it can stop being notified of
     *      file changes
     */
    STDMETHOD(RemoveObserver) (THIS_
                              IHXPDStatusObserver* /*IN*/ pObserver);

    /************************************************************************
     *  Method:
     *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
     *  Purpose:
     *      Lets an observer set the interval that the reporter (fsys) takes
     *      between status updates:
     */
    STDMETHOD(SetStatusUpdateGranularityMsec) (THIS_
                             UINT32 /*IN*/ ulStatusUpdateGranularityInMsec);

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
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    // IHXFileFormatFinder methods
    STDMETHOD(FindFileFormat) (THIS_ REF(IHXFileFormatObject*) rpFileFormat);

    // tell about end of source...
    virtual void	SetEndOfClip(HXBOOL bForcedEndofClip = FALSE);

	    void	AdjustClipBandwidthStats(HXBOOL bActivate = FALSE);

	    HXBOOL	CanBeResumed(void);
	    void	CheckForDefaultUpgrade(HX_RESULT status);
            HX_RESULT   ContinueWithFileHeader(HX_RESULT status, IHXValues* pHeader);
    virtual HX_RESULT	ContinueWithFileHeaderExt(HX_RESULT status, IHXValues* pHeader);
    virtual HX_RESULT	StreamHeaderReadyExt(IHXValues* pHeader);

    virtual HX_RESULT   ProcessFileHeader(void);
    virtual HX_RESULT   ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo);

    virtual HX_RESULT	UpdateRegistry(UINT32 ulRegistryID);

    virtual HX_RESULT	FillRecordControl(UINT32 ulLoopEntryTime = 0);
    virtual HXBOOL      IsPacketlessSource() { return m_bPacketlessSource; }
    virtual HXBOOL      ShouldDisableFastStart(void);
protected:
    LONG32			m_lRefCount;

    virtual 		~HXFileSource(void);


    virtual HX_RESULT	UpdateStatistics(void);

    virtual HX_RESULT	_ProcessIdle(HXBOOL atInterrupt = 0, 
				     UINT32 ulLoopEntryTime = 0,
				     UINT32 ulProcessingTimeAllowance = 0);
    virtual HX_RESULT	_ProcessIdleExt(HXBOOL atInterrupt = 0);
	    
	    HX_RESULT   FillBuffers(UINT32 ulLoopEntryTime = 0, 
				    UINT32 ulProcessingTimeAllowance = 0);
	    HXBOOL	SelectNextStreamToFill(STREAM_INFO* &lpStreamInfoOut);

	    void	ReBuffer(UINT32 ulLoopEntryTime = 0,
				 UINT32 ulProcessingTimeAllowance = 0);

	    void	ReportError(HX_RESULT theErr);
	    void	CleanupFileObjects();
	    HX_RESULT	InitializeFileFormat();
	    void	CalculateCurrentBuffering(void);
            void        GetFileDone(HX_RESULT rc, IHXBuffer* pFile);

            HX_RESULT HandleSDPData(IHXValues* pHeader);
            HX_RESULT HandleOutOfPackets(STREAM_INFO* pStreamInfo, 
					 UINT32 ulLoopEntryTime,
					 UINT32 ulProcessingTimeAllowance);

    UINT32		    m_ulLastBufferingReturned;
    UINT32		    m_ulInitialTime;
    UINT16		    m_uNumStreamsToBeFilled : 16;
    HX_BITFIELD		    m_bInFillMode : 1;
    HX_BITFIELD		    m_bInitialPacket : 1;
    HX_BITFIELD		    m_bFastStartInProgress : 1;
    HX_BITFIELD		    m_bAddDefaultUpgrade : 1;
    HX_BITFIELD		    m_bCurrentFileFormatUnkInUse: 1;
    HX_BITFIELD		    m_bValidateMetaDone: 1;
    HX_BITFIELD             m_bPacketlessSource : 1;
    HXBOOL                  m_bSeparateFragment;	
    char*		    m_pDefaultUpgradeString;

    IHXFileSystemObject*   m_pFSObject;
    IHXFileFormatObject*   m_pFFObject;
    IHXFileFormatObject*   m_pRAMFFObject;
    IHXFileResponse*	    m_pFileResponse;
    IHXPluginSearchEnumerator*	    m_pFileFormatEnumerator;
    IHXPluginSearchEnumerator* m_pFFClaimURLEnumerator;
    IUnknown*		    m_pCurrentFileFormatUnk;
    
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    CHXSimpleList*          m_pPDSObserverList;
    HX_RESULT               EstablishPDSObserverList();
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    //////////////////////////////////////////////////////
    // The following members and encapsulated classes are
    // used to determine the mime-type of the file we are
    // asked to read.
public:
    void FinishSetup(HX_RESULT status, const char* pMimeType);
    void AttempToLoadFilePlugin(const char* pMimeType);
    HX_RESULT ExtendedSetup(const char* pszURL);
    void      SetSchemeExtensionPairClaimedEnumerator(IHXPluginSearchEnumerator* pFFClaim);

protected:
    IHXFileObject*		m_pFileObject;
    IHXRequestHandler*		m_pRequestHandler;
    char*			m_pMimeType;		    
    char*			m_pExtension;		    

    class CMimeFinderFileResponse : public IHXFileMimeMapperResponse,
                                    public IHXFileRecognizerResponse
    {
    private:
	HXFileSource*	m_pSource;
	LONG32		m_lRefCount;
    public:
	CMimeFinderFileResponse(HXFileSource* pSource)
	    { 
		m_pSource = pSource; 
		m_lRefCount = 0;
	    };

	// IUnknown methods
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)  (THIS);
	STDMETHOD_(ULONG32,Release) (THIS);
    
	// IHXFileMimeMapperResponse methods
	STDMETHOD(MimeTypeFound) (THIS_
				  HX_RESULT   status,
				  const char* pMimeType);
	
        // IHXFileRecognizerResponse methods
        STDMETHOD(GetMimeTypeDone) (THIS_ HX_RESULT status, IHXBuffer* pMimeType);
    };

    CMimeFinderFileResponse*	m_pMimeFinderResponse;

#if defined(HELIX_FEATURE_ASM)
    class SourceBandwidthInfo : public IHXSourceBandwidthInfo
    {
    private:
	LONG32		m_lRefCount;
    public:
	SourceBandwidthInfo() {m_lRefCount = 0;};
	~SourceBandwidthInfo() {};
	/*
	 *	IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
				    REFIID riid,
				    void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

	/*
	 *	IHXSourceBandwidthInfo methods
	 */
	STDMETHOD(InitBw)		(THIS_
				    IHXBandwidthManagerInput* pBwMgr);

	STDMETHOD(SetTransmitRate)	(THIS_
				    UINT32 ulBitRate);

    };

    SourceBandwidthInfo*    m_pSimulatedSourceBandwidth;
#endif /* HELIX_FEATURE_ASM */

    class CFileReader : public IHXFileResponse
    {
    public:
        CFileReader(HXFileSource* pOwner);
        ~CFileReader();

        STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)(THIS);
        STDMETHOD_(ULONG32,Release)(THIS);

        // IHXFileResponse methods
        STDMETHOD(InitDone)  (THIS_ HX_RESULT status);
        STDMETHOD(SeekDone)  (THIS_ HX_RESULT status);
        STDMETHOD(ReadDone)  (THIS_ HX_RESULT status, IHXBuffer *pBuffer);
        STDMETHOD(WriteDone) (THIS_ HX_RESULT status);
        STDMETHOD(CloseDone) (THIS_ HX_RESULT status);

        HX_RESULT   GetFile(IHXFileObject* /*IN*/ pFile);
        void        Close(void);

    protected:
        HXBOOL            m_bGetFilePending;
        IHXBuffer*      m_pBuffer;
        HXFileSource*   m_pOwner;
        IHXFileObject*  m_pFile;
        LONG32          m_lRefCount;
    };

    friend class CFileReader;

    CFileReader*                m_pFileReader;
#if defined(HELIX_FEATURE_FILE_RECOGNIZER)
    CHXFileRecognizer*          m_pFileRecognizer;
#endif /* #if defined(HELIX_FEATURE_FILE_RECOGNIZER) */
    HXSourceBufferStats*        m_pHXSrcBufStats;
};

#endif // _HX_FILE_SOURCE


