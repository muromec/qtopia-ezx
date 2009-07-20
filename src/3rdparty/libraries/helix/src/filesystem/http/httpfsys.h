/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpfsys.h,v 1.39 2007/04/04 00:21:01 gwright Exp $
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

#ifndef _HTTPFSYS_H_
#define _HTTPFSYS_H_

#include "unkimp.h"
#include "smartptr.h"
#include "hxathsp.h"
#include "hxcomsp.h"
#include "hxplnsp.h"
#include "hxpktsp.h"
#include "hxflche.h"
#include "hxauto.h"
#include "hxpac.h"
#include "hxnet.h"

#include "miscsp.h"


/* forward decl. */
struct IUnknown;
struct IHXBuffer;
struct IHXCallback;
struct IHXRegistry;
struct IHXFileResponse;
struct IHXFileSystemObject;
struct IHXFileObject;
struct IHXPlugin;
struct IHXSocket;
struct IHXFileMimeMapperResponse;
struct IHXFileExistsResponse;
struct IHXPendingStatus;
struct IHXTimeoutSettings;
struct IHXErrorMessages;
struct IHXInterruptState;
struct IHXCookies;
struct IHXCookies2;
struct IHXHTTPRedirect;
struct IHXSockAddr;
struct IHXSocket;
struct IHXResolve;

class CHXString;
class CChunkyRes;
class CHXSimpleList;
class CDecoder;
class CHXMapStringToOb;
class CHXGenericCallback;
class HXMutex;

class HTTPTCPResponse;
class HTTPFileObjCallback;


#define MAX_CHUNK_SIZE     1024 //max size of chunk in hex digits.
#define DEFAULT_CHUNK_SIZE 1024 //default size of chunk buffer

#ifdef HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT
# if !defined(CHUNK_RES_MEM_FLOOR)
#   define CHUNK_RES_MEM_FLOOR     512000
# endif
# if !defined(CHUNK_RES_MEM_CEILING)
#   define CHUNK_RES_MEM_CEILING   1000000
# endif
#endif //HELIX_HTTPFSYS_MEM_GROWTH_LIMIT




typedef enum
{
    CE_HEADER_PENDING,
    CE_BODY_PENDING,
    CE_HEADER_READY,
    CE_BODY_READY
} CEState;

typedef struct _HTTPChunkedEncoding
{
    unsigned long    size;
    unsigned long    read;
    unsigned long    maxChunkSizeAccepted;
    HXBOOL             lastchunk;
    CEState          state;
    char*            buf;
    unsigned long    buf_size;
} HTTPChunkedEncoding;


class CChunkyResMap
{
public:
    CChunkyResMap();
    virtual ~CChunkyResMap();
    CChunkyRes* GetChunkyResForURL(const char* pURL, void* pCursorOwner, IUnknown* pContext);
    void RelinquishChunkyRes(CChunkyRes* pChunkyRes, void* pCursorOwner);

private:
    CHXMapStringToOb*	m_pChunkyResURLMap;
};



/////////////////////////////////////////////////////////////////////////////
//
//  Class:
//
//      CHTTPFileSystem
//
//  Purpose:
//
//      Example implementation of HTTP file system.
//

class CHTTPFileSystem : public CUnknownIMP,
                       public IHXPlugin,
                       public IHXFileSystemObject,
                       public IHXFileSystemCache,
                       public IHXHTTPAutoStream
{
    DECLARE_UNKNOWN(CHTTPFileSystem)

public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT CanUnload();
    static HX_RESULT STDAPICALLTYPE HXShutdown(void);

    CHTTPFileSystem();

    ~CHTTPFileSystem();


    static char* GetProtocol(void)  { return ((char *) zm_pProtocol); }

    /*** IHXPlugin methods ***/

    /************************************************************************
     *  Method:
     *      IHXPlugin::GetPluginInfo
     *  Purpose:
     *      Returns the basic information about this plugin. Including:
     *
     *      unInterfaceCount    the number of standard RMA interfaces
     *                          supported by this plugin DLL.
     *      pIIDList            array of IID's for standard RMA interfaces
     *                          supported by this plugin DLL.
     *      bLoadMultiple       whether or not this plugin DLL can be loaded
     *                          multiple times. All File Formats must set
     *                          this value to TRUE.
     *      pDescription        which is used in about UIs (can be NULL)
     *      pCopyright          which is used in about UIs (can be NULL)
     *      pMoreInfoURL        which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)    (THIS_
                                 REF(HXBOOL)        /*OUT*/ bLoadMultiple,
                                 REF(const char*) /*OUT*/ pDescription,
                                 REF(const char*) /*OUT*/ pCopyright,
                                 REF(const char*) /*OUT*/ pMoreInfoURL,
                                 REF(ULONG32)     /*OUT*/ ulVersionNumber
                                );

    /************************************************************************
     *  Method:
     *      IHXPlugin::InitPlugin
     *  Purpose:
     *      Initializes the plugin for use. This interface must always be
     *      called before any other method is called. This is primarily needed
     *      so that the plugin can have access to the context for creation of
     *      IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
                            IUnknown*   /*IN*/  pContext);

    STDMETHOD(GetFileSystemInfo)    (THIS_
                                    REF(const char*) /*OUT*/ pShortName,
                                    REF(const char*) /*OUT*/ pProtocol);
    STDMETHOD(InitFileSystem) (THIS_ IHXValues* options);

    STDMETHOD(CreateFile)       (THIS_
                                IUnknown**    /*OUT*/   ppFileObject);

    STDMETHOD(CreateDir)        (THIS_
                                IUnknown**     /*OUT*/     ppDirObject);

    // IHXFileSystemCache methods...
    STDMETHOD (RefreshCache) (THIS);
    STDMETHOD (EmptyCache) (THIS);
    STDMETHOD (MoveCache) (THIS_ const char *path);

    // IHXHTTPAutoStream methods...
    STDMETHOD_( void, SetDestinationFile) ( THIS_ const char *pFilename );


private:
    IUnknown*                   m_pContext;
    IHXValues*                  m_pOptions;
    static const char*          zm_pDescription;
    static const char*          zm_pCopyright;
    static const char*          zm_pMoreInfoURL;
    static const char*          zm_pShortName;
    static const char*          zm_pProtocol;

    // Autostreaming support
    static HXBOOL                 m_bSaveNextStream;
    static CHXString            m_SaveFileName;
};

/////////////////////////////////////////////////////////////////////////////
//
//  Class:
//
//      CHTTPFileObject
//
//  Purpose:
//
//      Example implementation of a basic file system file object.
//

class CHTTPFileObject : public IHXFileObject,
                        public IHXFileExists,
                        public IHXFileStat,
                        public IHXFileMimeMapper,
                        public IHXGetFileFromSamePool,
                        public IHXPendingStatus,
                        public IHXRequestHandler,
                        public IHXTimeoutSettings,
                        public IHXClientAuthResponse,
                        public IHXHTTPRedirect,
                        public IHXFileAutoStream,
                        public IHXProxyAutoConfigCallback
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                      , public IHXPDStatusMgr
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
{
public:
    CHTTPFileObject();
    ~CHTTPFileObject();

    INT32 m_lCount;

    static CHTTPFileObject* CreateObject();
    STDMETHOD(QueryInterface)
    (
        THIS_
        REFIID riid,
        void** ppvObj
    );
    STDMETHOD_(ULONG32,AddRef) (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    void InitObject
    (
        char*                   szBaseURL,
        IHXFileSystemObject*    pFS,
        IUnknown*               pContext,
        IHXValues*              pOptions
    );

    /*
     *  IHXFileObject methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileObject::Init
     *  Purpose:
     *      Associates a file object with the file response object it should
     *      notify of operation completness. This method should also check
     *      for validity of the object (for example by opening it if it is
     *      a local file).
     */
    STDMETHOD(Init)             (THIS_
                                ULONG32             /*IN*/      ulFlags,
                                IHXFileResponse*   /*IN*/       pFileResponse);

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
     *  Method:
     *      IHXFileObject::Close
     *  Purpose:
     *      Closes the file resource and releases all resources associated
     *      with the object.
     */
    STDMETHOD(Close)            (THIS);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Read
     *  Purpose:
     *      Reads a buffer of data of the specified length from the file
     *      and asynchronously returns it to the caller via the
     *      IHXFileResponse interface passed in to Init.
     */
    STDMETHOD(Read)             (THIS_
                                ULONG32             ulCount);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Write
     *  Purpose:
     *      Writes a buffer of data to the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Write)            (THIS_
                                IHXBuffer*          pBuffer);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Seek
     *  Purpose:
     *      Seeks to an offset in the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Seek)             (THIS_
                                ULONG32     ulOffset,
                                HXBOOL        bRelative);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Stat
     *  Purpose:
     *      Collects information about the file that is returned to the
     *      caller in an IHXStat object
     */
    STDMETHOD(Stat)             (THIS_
                                IHXFileStatResponse* pFileStatResponse);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Advise
     *  Purpose:
     *      To pass information to the File Object
     */
    STDMETHOD(Advise)   (THIS_
                        ULONG32 ulInfo);

    /************************************************************************
     *  Method:
     *      IHXGetFileFromSamePool::GetFileObjectFromPool
     *  Purpose:
     *      To get another FileObject from the same pool.
     */
    STDMETHOD(GetFileObjectFromPool)    (THIS_
                                         IHXGetFileFromSamePoolResponse*);

    // IHXFileExists interface
    /************************************************************************
     *  Method:
     *      IHXFileExists::DoesExist
     *  Purpose:
     */
    STDMETHOD(DoesExist) (THIS_
                        const char*             /*IN*/  pPath,
                        IHXFileExistsResponse* /*IN*/  pFileResponse);

    /*
     *  IHXFileMimeMapper methods
     */
    /************************************************************************
     *  Method:
     *      IHXFileMimeMapper::FindMimeType
     *  Purpose:
     */
    STDMETHOD(FindMimeType) (THIS_
                    const char*             /*IN*/  pURL,
                    IHXFileMimeMapperResponse* /*IN*/  pMimeMapperResponse
                            );


    /*
     * IHXPendingStatus methods
     */

    /************************************************************************
     *  Method:
     *      IHXPendingStatus::GetStatus
     *  Purpose:
     *      Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus)        (THIS_
                                REF(UINT16) uStatusCode,
                                REF(IHXBuffer*) pStatusDesc,
                                REF(UINT16) ulPercentDone);

    HX_RESULT _ReOpen();

    HX_RESULT _InitializeChunkyRes(const char* url);
    HX_RESULT _DoResolverSetup();

    /************************************************************************
     *  Method:
     *      Private interface::_OpenFile
     *  Purpose:
     *      This common method is used from Init() and GetFileObjectFromPool()
     */
    HX_RESULT _OpenFile( const char* pFilename,
                        ULONG32     ulFlags);

    HX_RESULT ProcessIdle(void);

    // IHXResolveResponse methods. We don't derive from this
    // interface, but rather this is proxied through
    // HTTPTCPResponse
    STDMETHOD(GetAddrInfoDone) (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone) (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);

    // IHXSocketResponse methods. We don't derive from this
    // interface, but rather this is proxied through
    // HTTPTCPResponse
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent, HX_RESULT status);

    /*
     * IHXRequestHandler methods
     */
    /************************************************************************
     *  Method:
     *      IHXRequestHandler::SetRequest
     *  Purpose:
     *      Associates an IHXRequest with an object
     */
    STDMETHOD(SetRequest)       (THIS_
                                IHXRequest*        /*IN*/  pRequest);

    /************************************************************************
     *  Method:
     *      IHXRequestHandler::GetRequest
     *  Purpose:
     *      Gets the IHXRequest object associated with an object
     */
    STDMETHOD(GetRequest)       (THIS_
                                REF(IHXRequest*)  /*OUT*/  pRequest);



    /**************************
        IHXTimeoutSettings
    ***************************/


    /************************************************************************
     *  Method:
     *      IHXTimeSettings::Get/SetConnnectionTimeout
     *  Purpose:
     *      Get/Set the connection timeout setting, in seconds
     */
    STDMETHOD(GetConnectionTimeout) (THIS_
                                      REF(UINT32)   /*OUT*/ nSeconds);
    STDMETHOD(SetConnectionTimeout) (THIS_
                                      UINT32        /*OUT*/ nSeconds);

    /************************************************************************
     *  Method:
     *      IHXTimeSettings::Get/SetServerTimeout
     *  Purpose:
     *      Get/Set the server timeout setting, in seconds
     */
    STDMETHOD(GetServerTimeout) (THIS_
                                      REF(UINT32)   /*OUT*/ nSeconds);
    STDMETHOD(SetServerTimeout) (THIS_
                                      UINT32        /*OUT*/ nSeconds);

    /************************************************************************
     *  Method:
     *      IHXHTTPRedirect::Init
     *  Purpose:
     *      Initialize the response object
     */
    STDMETHOD(Init)         (THIS_
                             IHXHTTPRedirectResponse* pRedirectResponse);

    /************************************************************************
     *  Method:
     *      IHXHTTPRedirect::SetResponseObject
     *  Purpose:
     *      Initialize the response object w/o calling Init
     */
    STDMETHOD(SetResponseObject) (THIS_
                                  IHXHTTPRedirectResponse* pRedirectResponse);


    // IHXClientAuthResponse
    STDMETHODIMP ResponseReady
    (
        HX_RESULT   HX_RESULTStatus,
        IHXRequest* pIHXRequestResponse
    );

    // Support for cache
    STDMETHOD_ (IHXBuffer*, CreateBufferFromValues)     (THIS_ IHXValues /*IN*/ *pHeaderValues);
    STDMETHOD_ (IHXValues*, CreateValuesFromBuffer)     (THIS_ IHXBuffer *pBuffer);
    STDMETHOD_ (void,        CacheSupport_OpenFile)      (THIS);
    STDMETHOD_ (void,        CacheSupport_InitObject)    (THIS);
    STDMETHOD_ (void,        CacheSupport_ReadDone)      (THIS);
    STDMETHOD_ (void,        CacheSupport_HandleSuccess) (THIS_ HTTPResponseMessage* pMessage);
    STDMETHOD  (ProcessCacheCompletions)                 (THIS_ HXBOOL bRedirected);

    // Support for autostreaming
    STDMETHOD_( void, SetDestinationFile) ( THIS_ const char *pFilename );

    /*
     *  IHXProxyAutoConfigCallback methods
     */
    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfigCallback::GetProxyInfoDone
    *  Purpose:
    */
    STDMETHOD(GetProxyInfoDone)     (THIS_
                                    HX_RESULT   status,
                                    char*       pszProxyInfo);

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
    
    // Methods that the generic callback will use
    static void CallbackProc(void* pArg);
private:

    HX_RESULT       _OpenFileExt        ();
    HX_RESULT       BeginGet            (ULONG32 ulOffsetStart=0);
    const char*     GetActualHost       () const;
    const char*     GetActualResource   () const;
    HX_RESULT       GetUserAgent        (REF(IHXBuffer*) pBuffer);
    int             GetActualPort       () const;
    HX_RESULT       ProcessPendingReads (void);
    void            AddNoCacheHeader    ();
    HXBOOL            IsLiveStream        (const char* pMimeType);

    HX_RESULT       _SendAuthentication(IHXValues* pValues);

    HX_RESULT       _HandleSuccess
                            (
                                HTTPResponseMessage*    pMessage,
                                IHXBuffer*              pBuffer,
                    UINT32                  ulHeaderLength
                            );
    HX_RESULT       _HandleFail(UINT32 ulHTTPError);
    HX_RESULT       _HandleUnAuthorized(HTTPResponseMessage* pMessage, IHXBuffer* pBuffer, UINT32 ulHeaderLength);
    HX_RESULT       _HandleRedirect(HTTPResponseMessage* pMessage);

    HX_RESULT       _EnsureThatWeAreReadingWisely();

    HX_RESULT       _HandleByteRangeSeek(ULONG32 ulSeekLocation);

    HX_RESULT       _WriteRequestChunk();

    HX_RESULT       _DoSomeReadingFromSocket(HXBOOL bHandleBuffersImmediately);

    void            _SetCurrentReadPos(UINT32 ulNewCurrentReadPosition); // so it can clear out cached contig
    UINT32          _GetContiguousLength();

    void            _ClearPreprocessedBuffers();

    void            CallReadDone(HX_RESULT status, IHXBuffer* pBuffer);

    // /Call these methods instead of directly setting the member variables so
    // progressive-download processing can be done when these values are set:
    void            SetSupportsByteRanges(HXBOOL bSupportsByteRanges);
    void            SetReadContentsDone(HXBOOL bReadContentsDone);


    // Support for cookie mangling
    HX_RESULT       MangleAllSetCookies     (IHXRequest* pRequest);
    HX_RESULT       MangleSetCookie         (IHXBuffer* pInput,
                                             REF(IHXBuffer*) pOutput);
    HX_RESULT       StoreMangledCookie      (char* pCookie);
    HX_RESULT       UnmangleAllCookies      (IHXRequest* pRequest);
    HX_RESULT       UnmangleCookie          (char* pCookie,
                                             UINT32 ulCookieLen,
                                             REF(IHXBuffer*) pDomain,
                                             REF(HXBOOL) bIsDomain,
                                             REF(IHXBuffer*) pPath);
    HX_RESULT       CompareDomains          (IHXBuffer* pURLHost,
                                             IHXBuffer* pTestDomain,
                                             HXBOOL bIsDomain);
    HX_RESULT       ComparePaths            (IHXBuffer* pURLPath,
                                             IHXBuffer* pTestPath);
    HX_RESULT       GetHostAndPath          (IHXRequest* pRequest,
                                             REF(IHXBuffer*) pHostStr,
                                             REF(IHXBuffer*) pPathStr);
    HX_RESULT       DePerplexBuffer         (IHXBuffer* pInput,
                                             REF(IHXBuffer*) pOutput);
    HX_RESULT       HandleSocketRead(HX_RESULT status, IHXBuffer* pBuffer);
    HX_RESULT       HandleHeaderRead(IHXBuffer* pBuffer);


    // Error reporting
    void            ReportDNSFailure        ();
    void            ReportConnectionFailure ();
    void            ReportConnectionTimeout ();
    void            ReportServerTimeout     ();
    void            ReportDocumentMissing   ();
    void            ReportGeneralFailure    ();

    HX_RESULT       _SetDataJustDownloaded(const char* pRawBuf, UINT32 ulLength);
    HX_RESULT	    _SetDataPlain(const char* pRawBuf, UINT32 ulLength);
    HX_RESULT       _SetDataGzipEncoded(const char* pRawBuf, UINT32 ulLength);
    HX_RESULT	    _SetMetaData(const char* pMetaDataSegmentBuffer,
				 UINT32 ulMetaDataSegmentSize,
				 UINT32 ulMetaDataSegmentReach,
				 UINT32 ulMetaDataSize);

    unsigned long   GetMaxChunkSizeAccepted();
    HX_RESULT       DecodeChunkedEncoding(HTTPChunkedEncoding*& pChunkedEncoding,
                                          const char*           pChunk,
                                          int                   l);

    HX_RESULT	    ContainsMP3MimeType (const char* pData, UINT32 ulLength);

    friend class HTTPFileObjCallback;
    friend class HTTPTCPResponse;

    HX_RESULT                   m_LastError;

    IHXCommonClassFactory*      m_pCommonClassFactory;
    IHXPreferences*             m_pPreferences;
    IHXScheduler*               m_pScheduler;
    IHXRegistry*                m_pRegistry;
    IHXErrorMessages*           m_pErrorMessages;
    IHXHTTPRedirectResponse*    m_pRedirectResponse;
    IHXCookies*         m_pCookies;
    IHXCookies2*                m_pCookies2;
    IHXProxyAutoConfig* m_pPAC;
    CHXSimpleList*              m_pPACInfoList;
    LISTPOSITION                m_PACInfoPosition;
    HXBOOL                        m_bOnServer;
    IUnknown*                   m_pContext;
    IHXValues*                  m_pOptions;

    IHXInterruptState* m_pInterruptState;

#ifdef _MACINTOSH
    IHXBuffer*                  m_pReadDoneBuffer;
    HX_RESULT                   m_uReadDoneStatus;
    HXBOOL                                m_bReadDoneToBeProcessed;
#endif

    HXBOOL                        m_bMimeResponsePending;
    IHXFileMimeMapperResponse* m_pMimeMapperResponse;

    HXBOOL                        m_bFileExistsResponsePending;
    IHXFileExistsResponse*      m_pFileExistsResponse;

    HXBOOL                        m_bStatPending;
    IHXFileStatResponse*        m_pFileStatResponse;

    HXBOOL                        m_bInitResponsePending;
    IHXFileResponse*            m_pFileResponse;

    IHXFileSystemObject*        m_pFileSystem;

    HXBOOL                        m_bTCPReadPending;
    HTTPTCPResponse*            m_pTCPResponse;

    char*                       m_szBaseURL;

    char*                       m_pFilename;
    char*                       m_pPath;
    char*                       m_pHost;
    ULONG32                     m_ulFlags;
    IHXRequest*         m_pRequest;
    IHXValues*                  m_pRequestHeadersOrig;
    IHXValues*                  m_pParams;

    IHXBuffer*                  m_pLanguage; // suggested content lang

    HXBOOL                        m_bAuthorized;
    DECLARE_SMART_POINTER
    (
        IHXClientAuthConversation
    )                           m_spClientAuthConversationAuthenticator;

    CHXGenericCallback*         m_pCallback;

    HXBOOL                        m_bSeekPending;
    HXBOOL                        m_bInitPending;
    HXBOOL                        m_bGetProxyInfoPending;

    HXBOOL                        m_bInitialized;
    HXBOOL                        m_bInDestructor;

    UINT16                      m_uDataLength;
    char*                       m_pData;

    ULONG32                     m_ulCurrentReadPosition;
    UINT32                      m_ulLastKnownEndOfValidContiguousRange;

#if defined(HELIX_FEATURE_HTTP_GZIP)
    CDecoder*                   m_pDecoder;
#else
    void*                       m_pDecoder;
#endif
    CChunkyRes*                 m_pChunkyRes;

    IHXMutex*                   m_pMutex;

    HXBOOL                        m_bSupportsByteRanges;
    HXBOOL                        m_bDisableByteRanges;
    HXBOOL                        m_bExternalByteRangeRequests;
    ULONG32                     m_uByteRangeSeekOffset;
    HXBOOL                        m_bByteRangeSeekPending;
    HXBOOL                        m_bCheckingWhetherByteRangeWorks;
    HXBOOL                        m_bServerPresumablyWorksWithByteRangeRequests;

    CHXSimpleList               m_PendingReadList;
    CHXSimpleList               m_PreProcessedReadBuffers;
    static CHXSimpleList        zm_pList;

    int                         m_nPort;

    UINT32                      m_nRequestTime; // for time-outs waiting for a read to complete
    HXBOOL                        m_bSocketReadTimeout;

    UINT32                      m_nConnTimeout;
    UINT32                      m_nServerTimeout;
    HXBOOL                        m_bDisableConnectionTimeOut;
    HXBOOL                        m_bConnTimedOut;

    HXBOOL                        m_bMangleCookies;
    IHXBuffer*      m_pMangledCookies;
    HXBOOL            m_bShoutcast;
    HXBOOL            m_bConvertFailedSeeksToLinear;

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    // /Progressive download vars:
    HXBOOL                        m_bDownloadCompleteReported;
    UINT32                      m_ulFileSizeSoFar;
    UINT32                      m_ulPrgDnTotalFileSize;
    UINT32                      m_ulPrgDnTotalFileDur;
    UINT32                      m_ulPriorReportedTotalFileDur;
    UINT32                      m_ulCurrentDurOfBytesSoFar;
    UINT32                      m_ulTimeOfLastBytesToDur;
    CHXSimpleList*              m_pPDSObserverList;
    IHXMediaBytesToMediaDur*    m_pBytesToDur;
    HXBOOL                        m_bHaveReportedSupByteRngs;
    UINT32                      m_ulStatusUpdateGranularityInMsec;

    // /Progressive-download-related methods:
    HX_RESULT       EstablishPDSObserverList();
    void            ReportCurrentDurChanged();
    void            ReportTotalDurChanged();
    void            ReportDownloadComplete();
    void            ReportChange(UINT32 ulFlags);
    HXBOOL            IsPrgDnCompleteFileDurKnown()
        { return m_ulPrgDnTotalFileDur != HX_PROGDOWNLD_UNKNOWN_DURATION; }
    HXBOOL            IsPrgDnCompleteFileSizeKnown()
        { return m_ulPrgDnTotalFileSize != HX_PROGDOWNLD_UNKNOWN_FILE_SIZE; }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    /**************************/
    /* These member variables need to go in a struct
     * if we wanna keep HTTP connection and related downloaded
     * stuff live after the http object is destroyed.
     * This is needed in the case when the first ff plug-in realizes
     * that the file is not of its type and we then load the next
     * related plug-in which then connects to the same URL. We do not
     * want to start downloading from scratch and instead want to use data
     * that had been dowloaded during the lifetime of earlier plug-in
     */
    IHXSocket*                  m_pSocket;
    IHXResolve*                 m_pResolve;

    HXBOOL                        m_bHTTP1_1;

    HXBOOL                        m_bConnectDone;
    HXBOOL                        m_bWriteDone;
    HXBOOL                        m_bReadHeaderDone;
    HXBOOL                        m_bReadContentsDone;

    HXBOOL                        m_bKnowContentSize;
    ULONG32                     m_nContentSize;
    ULONG32                     m_nOriginalContentSize;
    HXBOOL                        m_bEncoded;

    HXBOOL                        m_bChunkedEncoding;
    HTTPChunkedEncoding*        m_pChunkedEncoding;

    ULONG32                     m_nContentRead;

    HXBOOL                      m_bKnowHTTPResponseContentSize;
    ULONG32                     m_ulHTTPResponseContentSize;
    ULONG32                     m_ulHTTPResponseContentRead;

    ULONG32			m_ulMetaDataGap;
    ULONG32			m_ulNextMetaDataStartIdx;
    ULONG32			m_ulMetaDataBlockSize;
    ULONG32			m_ulMetaDataBlockRemainingSize;

    UINT16                      m_nTotalRequestSize;
    UINT16                      m_nRequestWritten;

    ULONG32                     m_nHeaderRead;

    UINT32                      m_ulBufferAheadAmount;

    CHXString                   m_strHost;
    CHXString                   m_strRequest;
    CHXString                   m_strResource;
    CHXString                   m_strMimeType;

    HXBOOL                        m_bUseProxy;
    CHXString                   m_strProxyHost;
    CHXString                   m_strProxyResource;
    int                         m_nProxyPort;

    UINT32                      m_ulMaxRecursionLevel;
    UINT32                      m_ulRecursionCount;
    HXBOOL                        m_bInReadDone;
    HXBOOL                        m_bInHandleHeaderRead;

    // Monitor redirection
    UINT32         m_nRedirectLevel;

    // Support for cache
    UINT32         m_ulCreateTime;
    UINT32         m_ulExpiryTime;
    UINT32         m_ulLastModTime;

    HXBOOL           m_bCacheEnabled;
    UINT32         m_ulCacheMaxSize;
    IHXBuffer*    m_pCacheFile;
    UINT32         m_ulCutOffDate;

    HXBOOL           m_bCached;
    CCacheEntry*   m_pCacheEntry;
    HXBOOL           m_bReadPending;          // Data available, ReadDone sked'd
    HXBOOL           m_bMirroredServers;      // Cache ignores leftmost label in domain

    // Support for saving streamed files
    HXBOOL                        m_bSaveToFile;
    CHXString                   m_SaveFileName;
    ULONG32                     m_ulOffset;

    // HTTP POST support.
    UINT32                      m_nPostDataSize;
    // Resume support.
    HXBOOL                        m_bPartialData;

    HXBOOL                        m_bUseHTTPS;
    HXBOOL                        m_bClosed;

    char*                       m_pLastHeader;
    UINT32                      m_ulLastHeaderSize;
    UINT32			m_ulMinStartupLengthNeeded;
    UINT32                      m_ulIgnoreBytesYetToBeDownloaded;
    HXBOOL                      m_bDiscardOrphanLFInChunkedData;

    HXBOOL                      m_bHaltSocketReadTemporarily;
    ULONG32                     m_ulMaxBufSize ;
};

class HTTPTCPResponse : public CUnknownIMP,
                        public IHXSocketResponse,
                        public IHXResolveResponse
{
    DECLARE_UNKNOWN(HTTPTCPResponse)
public:

    HTTPTCPResponse();
    ~HTTPTCPResponse();

    void InitObject(CHTTPFileObject* pOwner);

    // IHXResolveResponse methods
    STDMETHOD(GetAddrInfoDone) (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone) (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);

    // IHXSocketResponse methods
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent, HX_RESULT status);
protected:
    LONG32                      m_lRefCount;
    CHTTPFileObject*            m_pOwner;
    HXBOOL                      m_bOwnerDestroyed;

    friend class CHTTPFileObject;
};

#endif // _HTTPFSYS_H_
