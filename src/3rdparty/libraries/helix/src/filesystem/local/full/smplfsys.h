/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smplfsys.h,v 1.20 2008/08/12 23:54:37 svaidhya Exp $
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

#ifndef _SMPLFSYS_H_
#define _SMPLFSYS_H_

#include "hlxclib/stdio.h"

/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore an outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */
STDAPI HXCreateInstance(IUnknown**  /*OUT*/	ppIUnknown);


/****************************************************************************
 * 
 *  Function:
 * 
 *	HXShutdown()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to free any *global* 
 *	resources. This method is called just before the DLL is unloaded.
 *
 */
STDAPI HXShutdown(void);

// Forward declarations
class CHXGenericCallback;
#if defined(HELIX_FEATURE_PROGDOWN)
class CProgressiveDownloadMonitor;
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
class CHXSimpleList;
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.


/////////////////////////////////////////////////////////////////////////////
// 
//  Class:
//
//  	CSimpleFileSystem
//
//  Purpose:
//
//  	Example implementation of a basic file system.
//

class CSimpleFileSystem :   public IHXPlugin, 
			    public IHXFileSystemObject
{
private:
    LONG32			m_lRefCount;
    static const IID            zm_myIID;
    static const char* const    zm_pDescription;
    static const char* const    zm_pCopyright;
    static const char* const    zm_pMoreInfoURL;
    static const char* const    zm_pShortName;
    static const char* const	zm_pProtocol;
    CHXString			m_base_path;
    IUnknown*			m_pContext;
    IHXValues*			m_options;
    UINT32			m_ulMaxIterationLevel;

public:
    CSimpleFileSystem();
    ~CSimpleFileSystem();

    HXBOOL			m_bDisableMemoryMappedIO;
    HXBOOL			m_bEnableFileLocking;
    UINT32			m_ulChunkSize;

    IUnknown*			m_pCommonObj;

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // *** IHXPlugin methods ***

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
				REF(HXBOOL)        /*OUT*/ bLoadMultiple,
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

    // *** IHXFileSystemObject methods ***
    STDMETHOD(GetFileSystemInfo)    (THIS_
				    REF(const char*) /*OUT*/ pShortName,
				    REF(const char*) /*OUT*/ pProtocol);

    STDMETHOD(InitFileSystem) (THIS_ IHXValues* options);

    STDMETHOD(CreateFile)	(THIS_
				IUnknown**    /*OUT*/	ppFileObject);

    STDMETHOD(CreateDir)        (THIS_
                                IUnknown**     /*OUT*/  ppDirObject);
};

#ifdef _MACINTOSH
enum SeekReason
{
    REINIT_SEEK = 0,
    EXTERNAL_SEEK,
    PRE_TELL_SEEK,
    POST_TELL_SEEK
};
#endif

#define DATAFILE_FAIL_INTERVAL_DEFAULT   80 // Retry failed reads at this interval

/////////////////////////////////////////////////////////////////////////////
// 
//  Class:
//
//  	CSimpleFileObject
//
//  Purpose:
//
//  	Example implementation of a basic file system file object.
//

class CSimpleFileObject :   public IHXFileObject, 
			    public IHXDirHandler,
                            public IHXNestedDirHandler,
			    public IHXFileStat,
			    public IHXFileStat2,
			    public IHXFileExists,
			    public IHXGetFileFromSamePool,
			    public IHXRequestHandler,
			    public IHXFileRename,
			    public IHXFileRemove,
			    public IHXFileMove
#if defined(HELIX_FEATURE_PROGDOWN)
                            , public CProgressiveDownloadMonitorResponse
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                            , public IHXPDStatusMgr
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
{
private:
    UINT32		    m_ulSize;
    UINT32		    m_ulPos;
    HXBOOL		    m_bCanBeReOpened;
    HXBOOL		    m_bProxyMode; // For the 'fileproxy://' filesystem
    void		    UpdateFileNameMember();
    LONG32		    m_lRefCount;
    UINT32	    	    m_ulFlags;
    UINT32		    m_ulMaxIterationLevel;
    IHXScheduler*	    m_pScheduler;
    IUnknown*		    m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;
    IHXFileResponse*	    m_pFileResponse;
    CSimpleFileSystem*	    m_pFileSystem;
    IHXRequest*	    m_pRequest;
    IHXDescriptorRegistration* m_pDescriptorReg;
    HXBOOL		    m_bLocalClose;
    IHXDirHandlerResponse* m_pDirResponse;
    char*		    m_pFilename;
#ifdef _MACINTOSH
    CMacAsyncFile*	    m_pDataFile; /* cross-platform file object */
#else
    IHXDataFile*	    m_pDataFile; /* cross-platform file object */
#endif
    int			    m_nFd;
    CHXString		    m_base_path;

    HXBOOL		    m_bAsyncAccess;
    HXBOOL		    m_bInRead;
    HXBOOL		    m_bReadPending;
    HXBOOL		    m_bAsyncReadPending;
    UINT32		    m_ulPendingReadCount;
    CFindFile*		    m_pDirList;
    IUnknown*		    m_pUnknownUserContext;
    UINT32                  m_ulPendingSeekOffset;
    UINT16                  m_usPendingSeekWhence;

    void GetFullPathname(const char* pPath, CHXString* pPathname);
    HX_RESULT               DirCreatePath(CHXString strPath);

    HXBOOL		    m_bReadCancelled;

    HX_RESULT CheckForCorruptFile(HX_RESULT incomingError, UINT32 actual = 0, HXBOOL bRead = FALSE);
    HX_RESULT ActualAsyncReadDone(HX_RESULT result, IHXBuffer* pBuffer);
    HX_RESULT ActualAsyncSeekDone(HX_RESULT result);
    HX_RESULT DoRead(REF(HXBOOL) rbProgFail);
    HX_RESULT DoSeek(REF(HX_RESULT) rSeekDoneResult);
    void      SeekBackwards(UINT32 ulNumBytes);
    HX_RESULT FinishDoRead(UINT32 actual, REF(IHXBuffer*) pBuffer);

    HX_RESULT InitDataFile();

#ifdef _MACINTOSH

    class SMPLAsyncResponse : public CMacAsyncFileResponse
    {
    public:
	SMPLAsyncResponse(CSimpleFileObject* pFileObject);
	~SMPLAsyncResponse();

	HX_RESULT AsyncReadDone(HX_RESULT result, IHXBuffer* pBuffer);
	HX_RESULT AsyncSeekDone(HX_RESULT result);

	CSimpleFileObject*  m_pSMPLFileObject;
    };

    friend class SMPLAsyncResponse;
    SMPLAsyncResponse*	    m_pAsyncFileResponse;
    IHXInterruptState*	    m_pInterruptState;
    IHXFileStatResponse*    m_pFileStatResponse;
    IHXFileStat2Response*   m_pFile2StatResponse;
    SeekReason		    m_eSeekReason;
    UINT32		    m_ulPreSeekPosition;
    HXBOOL		    m_bFileToBeClosed;

#endif

    CHXGenericCallback* m_pStackCallback;

#if defined(HELIX_FEATURE_PROGDOWN)
    enum
    {
        CallbackStateUnknown,
        CallbackStateSeek,
        CallbackStateRead
    };
    CProgressiveDownloadMonitor* m_pProgDownMon;
    UINT32                       m_ulCallbackState;
    HXBOOL                         RequireFullRead();
    HX_RESULT                    FinishDoReadWithCallback(UINT32 actual);
    HX_RESULT                    FinishDoReadWithoutCallback(UINT32 actual);
    HXBOOL                         m_bProgDownEnabled;
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    UINT32                   m_ulPrgDnTotalFileSize;
    UINT32                   m_ulPrgDnTotalFileDur;
    UINT32                   m_ulPriorReportedTotalFileDur;
    UINT32                   m_ulCurrentDurOfBytesSoFar;
    UINT32                   m_ulTimeOfLastBytesToDur;
    IHXMediaBytesToMediaDur* m_pBytesToDur;
    UINT32                   m_ulStatusUpdateGranularityInMsec;
    CHXSimpleList*           m_pPDSObserverList;
    HXBOOL                     m_bDownloadProgressReported;
    HXBOOL                     m_bDownloadCompleteReported;
    HXBOOL                     m_bDidNotifyOfDownloadPause;
    HX_RESULT                EstablishPDSObserverList();
    void MaybeDoProgressiveDownloadStatusRept(UINT32 ulPrevFileSize,
                                              UINT32 ulCurFileSize);
    void                     ReportCurrentDurChanged();
    void                     ReportTotalDurChanged();
    void                     ReportDownloadComplete();
    void                     ReportChange(UINT32 ulFlags);
    HXBOOL                     IsPrgDnCompleteFileDurKnown()
        { return m_ulPrgDnTotalFileDur != HX_PROGDOWNLD_UNKNOWN_DURATION; }
    HXBOOL                     IsPrgDnCompleteFileSizeKnown()
        { return m_ulPrgDnTotalFileSize != HX_PROGDOWNLD_UNKNOWN_FILE_SIZE; }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.



public:
    CSimpleFileObject
    (
	CHXString& base_path, 
	CSimpleFileSystem*, 
	IUnknown*,
	UINT32 ulMaxRecursionLevel
    );
    ~CSimpleFileObject();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXFileObject methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Init
     *	Purpose:
     *	    Associates a file object with the file response object it should
     *	    notify of operation completness. This method should also check
     *	    for validity of the object (for example by opening it if it is
     *	    a local file).
     */
    STDMETHOD(Init)		(THIS_
				ULONG32		    /*IN*/	ulFlags,
				IHXFileResponse*   /*IN*/	pFileResponse);

    /************************************************************************
     *  Method:
     *      IHXFileObject::GetFilename
     *  Purpose:
     *      Returns the filename (without any path information) associated
     *      with a file object.
     */
    STDMETHOD(GetFilename)      (THIS_
				REF(const char*)    /*OUT*/  pFilename);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Close
     *	Purpose:
     *	    Closes the file resource and releases all resources associated
     *	    with the object.
     */
    STDMETHOD(Close)	    	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Read
     *	Purpose:
     *	    Reads a buffer of data of the specified length from the file
     *	    and asynchronously returns it to the caller via the 
     *	    IHXFileResponse interface passed in to Init.
     */
    STDMETHOD(Read)		(THIS_
    	    	    	    	ULONG32	    	    ulCount);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Write
     *	Purpose:
     *	    Writes a buffer of data to the file and asynchronously notifies
     *	    the caller via the IHXFileResponse interface passed in to Init,
     *	    of the completeness of the operation.
     */
    STDMETHOD(Write)	    	(THIS_
    	    	    	    	IHXBuffer*	    pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Seek
     *	Purpose:
     *	    Seeks to an offset in the file and asynchronously notifies
     *	    the caller via the IHXFileResponse interface passed in to Init,
     *	    of the completeness of the operation.
     */
    STDMETHOD(Seek)		(THIS_
    	    	    	    	 ULONG32	    ulOffset,
				 HXBOOL               bRelative);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Stat
     *	Purpose:
     *	    Collects information about the file that is returned to the
     *      caller in an IHXStat object
     */
    STDMETHOD(Stat)		(THIS_
				IHXFileStatResponse* pFileStatResponse);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Stat
     *	Purpose:
     *	    Collects extended information about the file that is returned to the
     *      caller in an IHXStat object
     */
    STDMETHOD(Stat)		(THIS_
				IHXFileStat2Response* pFileStat2Response);

    /************************************************************************
     *	Method:
     *	    IHXFileObject::Advise
     *	Purpose:
     *      To pass information to the File Object
     */
    STDMETHOD(Advise)	(THIS_
			ULONG32 ulInfo);

    /************************************************************************
     *	Method:
     *	    IHXDirHandler::InitDirHandler
     *	Purpose:
     *	    Associates a directory handler with the directory handler
     *	    response, it should notify of operation completness.
     */
    STDMETHOD(InitDirHandler)	(THIS_
				IHXDirHandlerResponse*    /*IN*/  pDirResponse);

    /************************************************************************
     *	Method:
     *	    IHXDirHandler::CloseDirHandler
     *	Purpose:
     *	    Closes the directory handler resource and releases all resources
     *	    associated with the object.
     */
    STDMETHOD(CloseDirHandler)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXDirHandler::MakeDir
     *	Purpose:
     *	    Create the directory
     */
    STDMETHOD(MakeDir)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXDirHandler::ReadDir
     *	Purpose:
     *	    Get a dump of the directory
     */
    STDMETHOD(ReadDir)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXGetFileFromSamePool::GetFileObjectFromPool
     *	Purpose:
     *      To get another FileObject from the same pool. 
     */
    STDMETHOD(GetFileObjectFromPool)	(THIS_
					 IHXGetFileFromSamePoolResponse*);

    // IHXFileExists interface
    /************************************************************************
     *	Method:
     *	    IHXFileExists::DoesExist
     *	Purpose:
     */
    STDMETHOD(DoesExist) (THIS_
			const char*		/*IN*/  pPath, 
			IHXFileExistsResponse* /*IN*/  pFileResponse);

    //IHXRequestHandler methods
    /************************************************************************
     *	Method:
     *	    IHXRequestHandler::SetRequest
     *	Purpose:
     *	    Associates an IHXRequest with an object
     */
    STDMETHOD(SetRequest)   	(THIS_
			    	IHXRequest*        /*IN*/  pRequest);

    /************************************************************************
     *	Method:
     *	    IHXRequestHandler::GetRequest
     *	Purpose:
     *	    Gets the IHXRequest object associated with an object
     */
    STDMETHOD(GetRequest)   	(THIS_
			    	REF(IHXRequest*)  /*OUT*/  pRequest);

    /************************************************************************
     *	Method:
     *	    IHXFileRename::Rename
     *	Purpose:
     *	    Renames the file to the new file name
     */
    STDMETHOD(Rename)			(THIS_
					const char* /*IN*/  pFilename);


    /************************************************************************
     *	Method:
     *	    IHXFileMove::Move
     *	Purpose:
     *	    Renames the file to the new file name
     */
    STDMETHOD(Move)			(THIS_
					const char* /*IN*/  pFilename);


    /************************************************************************
     *	Method:
     *	    IHXFileRemove::Remove
     *	Purpose:
     *	    Removes the file from the file system
     */
    STDMETHOD(Remove)			(THIS);


    /************************************************************************
     *	Method:
     *	    Private interface::_OpenFile
     *	Purpose:
     *	    This common method is used from Init() and GetFileObjectFromPool()
     */
    STDMETHOD(_OpenFile) (THIS_
			  ULONG32	    ulFlags);

#if defined(HELIX_FEATURE_PROGDOWN)
    // CProgressiveDownloadMonitorResponse methods
    STDMETHOD(ProgressiveCallback) (THIS);
    STDMETHOD(ProgressiveStatCallback) (THIS);
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

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

#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.


    // CSimpleFileObject methods
    static void StackCallback(void* pArg);

    //IHXNestedDirHandler Methods
    /************************************************************************
     *  Method:
     *      IHXNestedDirHandler::InitNestedDirHandler
     *  Purpose:
     *      Associates a Nested directory handler with the directory handler
     *      response, it should notify of operation completness.
     */
    STDMETHOD(InitNestedDirHandler)     (THIS_
                                        IHXDirHandlerResponse*    /*IN*/  pDirResponse);

    /************************************************************************
     *  Method:
     *      IHXNestedDirHandler::CloseNestedDirHandler
     *  Purpose:
     *      Closes the Nested directory handler resource and releases all resources
     *      associated with the object.
     */
    STDMETHOD(CloseNestedDirHandler)  (THIS);

    /************************************************************************
     *  Method:
     *      IHXNestedDirHandler::MakeNestedDir
     *  Purpose:
     *      Create the Nested directory structure
     */
    STDMETHOD(MakeNestedDir)  (THIS);
};

#endif // ndef _SMPLFSYS_H_

