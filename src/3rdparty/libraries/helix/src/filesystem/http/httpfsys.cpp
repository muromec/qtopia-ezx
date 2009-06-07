/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpfsys.cpp,v 1.104 2007/03/30 23:36:40 gwright Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"

#include "httpfsys.ver"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "hxescapeutil.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxrendr.h"
#include "hxpends.h"
#include "hxauthn.h"
#include "hxtbuf.h"
#include "hxtset.h"
#include "ihxident.h"
#include "perplex.h"
#include "hxcbobj.h"
#include "pckunpck.h"
#include "hxsockutil.h"

#include "chxpckts.h"

#include "dbcs.h"
#include "chunkres.h"
#include "hxslist.h"
#include "httppars.h"
#include "httpmsg.h"
#include "mimehead.h"
#include "portaddr.h"
#include "hxver.h"
#include "hxtick.h"
#include "hxurl.h"
#include "hxscope_lock.h"

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#include "ihxcookies.h"

#include "timerep.h"
#include "cache.h"
#include "unkimp.h"
#include "hxdir.h"
#include "rtsputil.h"
#if defined(HELIX_FEATURE_HTTP_GZIP)
#include "zlib.h"
#include "decoder.h"
#endif
#include "pacutil.h"
#include "httpfsys.h"
#include "ihxcookies2.h"
#include "hxtlogutil.h"

#ifdef _MACINTOSH
//#include "../dcondev/dcon.h"
#endif

#if defined(_CARBON) || defined(_MAC_MACHO)
#define USE_TEMP_CACHE_LOCATION 1
#include "filespecutils.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _AIX
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Httpfsys);
#endif

#define DEF_HTTP_PORT       80
#define DEF_HTTP_PORT_STR   ":80/"

#define DEF_HTTPS_PORT      443
#define DEF_HTTPS_PORT_STR   ":443/"

#ifdef _WIN16
#define MAX_RECURSION_LEVEL     10
#elif _LINUX
#define MAX_RECURSION_LEVEL     30
#elif defined(_MACINTOSH)
#define MAX_RECURSION_LEVEL     20
#else
// XXXNH: reduced from 200 to 50 to fix stack overflow in ActiveX
#define MAX_RECURSION_LEVEL     50
#endif

#define REDIRECT_LIMIT          20

// For PR 152375
#define LARGEST_CHUNK_SIZE_WE_ACCEPT_CONFIG_KEY "config.HTTPChunkedEncodingMaxChunkSize"
#define LARGEST_CHUNK_SIZE_WE_ACCEPT (256L * 1024L)

#undef  LOG_DATA
#define MAX_CACHE_SIZE          (64 * 1024 * 1024)
#define CACHE_DEFAULT_TTL_SECS  (1 * 60 * 60)

#define  LOG_FILE               "C:/Temp/cached-httpfsys.log"
#include "http_debug.h"

#include "db.h"

#include <string.h>

UINT32 g_ulDefTtl;

#ifndef USE_TEMP_CACHE_LOCATION
#define DEF_CACHE_DB            "cache_db"
#define MAX_CACHE_FILENAME      1024
#else
#define DEF_CACHE_DB            "helix_http_cache_db"
#endif // USE_TEMP_CACHE_LOCATION

#define HTTP_MAX_BUFFER_BEFORE_PROCESSIDLE (64 * 1024)

CCacheEntry*   g_pCacheEntry = NULL;
IHXBuffer*    CreateBufferFromValues (IHXValues *pHeaderValues);

// default if no timeouts in preferences.
#define DEF_HTTP_SERVER_TIMEOUT     (20 * MILLISECS_PER_SECOND)
#define DEF_HTTP_CONNECT_TIMEOUT    (30 * MILLISECS_PER_SECOND)

// default user agent header
#define DEF_USER_AGENT          "RMA/1.0 (compatible; RealMedia)"

// threshhold to see if it's better to do a seek or
// just wait for old-fashioned reading to catch up
#define BYTE_RANGE_SEEK_THRESHHOLD (4 * 1024)

// Tresholds for the amount of data to receive before attempting to make
// sense of the response and associated headers
#define HTTP_MIN_STARTUP_LENGTH_NEEDED              5       // bytes
#define HTTP_NONSTANDARD_MIN_STARTUP_LENGTH_NEEDED  1024    // bytes

#define ICECAST_META_SIZE_MULTIPLE                  16      // bytes


const char* CHTTPFileSystem::zm_pDescription    = "RealNetworks HTTP File System with CHTTP Support";
const char* CHTTPFileSystem::zm_pCopyright      = HXVER_COPYRIGHT;
const char* CHTTPFileSystem::zm_pMoreInfoURL    = HXVER_MOREINFO;
const char* CHTTPFileSystem::zm_pShortName      = "pn-http";

/// This name is used to indicate the http server is
/// actually RealServer. If the server team changes
/// the http response header field, Server: then
/// this value needs to be changed accordingly.
#define REALSERVER_RESPONSE_NAME    "RealServer"

//#define SUPPORT_SECURE_SOCKETS
//#define CREATE_LOG_DUMP
//#define LOG_DUMP_FILE "c:/temp/avi.txt"

#ifdef SUPPORT_SECURE_SOCKETS
const char* CHTTPFileSystem::zm_pProtocol       = "http|chttp|https";
#else
const char* CHTTPFileSystem::zm_pProtocol       = "http|chttp";
#endif

HXBOOL CHTTPFileSystem::m_bSaveNextStream = FALSE;
//CHXString CHTTPFileSystem::m_SaveFileName( "" );
CHXString CHTTPFileSystem::m_SaveFileName;


static INT32 g_nRefCount_http  = 0;

static CChunkyResMap g_ChunkyResMap;


#define WWW_AUTHENTICATION_RECENT_KEY "authentication.http.realm.recent"
#define PROXY_AUTHENTICATION_RECENT_KEY "proxy-authentication.http.realm.recent"

#ifdef USE_TEMP_CACHE_LOCATION
static void SetBufferToCacheFilePath(IHXBuffer* pBuffer)
{
    // For Mac Carbon, we put it in the Cleanup at Startup folder since the user may not
    // have permission to write into the app's directory
    CHXDirSpecifier tempDir = CHXFileSpecUtils::GetSystemTempDirectory();
    HX_ASSERT(CHXFileSpecUtils::DirectoryExists(tempDir));

    CHXFileSpecifier cacheFileSpec = tempDir.SpecifyChildFile(DEF_CACHE_DB);

    CHXString strCachePath = cacheFileSpec.GetPathName();

    pBuffer->Set((UINT8*) (const char *) strCachePath, 1 + strCachePath.GetLength());
}
#endif

// ChunkyResMap is a manager-type class that is able to re-use the same ChunkyRes for different
// file objects that use the same url. This happens, for example, if you're http-streaming a
// surestream file which may have many logical streams pasted one after another.

CChunkyResMap::CChunkyResMap()
{
    m_pChunkyResURLMap = new CHXMapStringToOb();
}

CChunkyResMap::~CChunkyResMap()
{
    
    delete m_pChunkyResURLMap;
}

CChunkyRes*
CChunkyResMap::GetChunkyResForURL(const char* pURL, void* pCursorOwner, IUnknown* pContext)
{
    CChunkyRes* pChunkyRes = NULL;

    if ( !m_pChunkyResURLMap->Lookup( pURL, (void*&)pChunkyRes ) )
    {
        pChunkyRes = new CChunkyRes(pContext);
        m_pChunkyResURLMap->SetAt(pURL, pChunkyRes);
    }

    HX_ASSERT(pChunkyRes);
    pChunkyRes->AddCursor(pCursorOwner);

    return pChunkyRes;
}

void
CChunkyResMap::RelinquishChunkyRes(CChunkyRes* pChunkyRes, void* pCursorOwner)
{
    if (!pChunkyRes) return;

    CChunkyRes* pChunkyResInMap = NULL;
    CHXString strURL;

    LISTPOSITION pos;
    LISTPOSITION prevPos = NULL;

    pos = m_pChunkyResURLMap->GetStartPosition();


    while (pos)
    {
        prevPos = pos;
        m_pChunkyResURLMap->GetNextAssoc(pos, strURL, (void*&)pChunkyResInMap);
        if (pChunkyRes == pChunkyResInMap)
        {
            pChunkyRes->RemoveCursor(pCursorOwner);

            if (pChunkyRes->CountCursors() == 0)
            {
                m_pChunkyResURLMap->RemoveKey(strURL);
                delete pChunkyResInMap;
            }
        }
    }
}


/****************************************************************************
 *
 *  Function:
 *
 *      CHTTPFileSystem::HXCreateInstance()
 *
 *  Purpose:
 *
 *      Function implemented by all plugin DLL's to create an instance of
 *      any of the objects supported by the DLL. This method is similar to
 *      Window's CoCreateInstance() in its purpose, except that it only
 *      creates objects from this plugin DLL.
 *
 *      NOTE: Aggregation is never used. Therefore an outer unknown is
 *      not passed to this function, and you do not need to code for this
 *      situation.
 *
 */
HX_RESULT STDAPICALLTYPE CHTTPFileSystem::HXCreateInstance
(
    IUnknown**  /*OUT*/ ppIUnknown
)
{
    // Do NOT check for expiration.  Needed for Auto Upgrade.

    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CHTTPFileSystem();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

/****************************************************************************
 *
 *  Function:
 *
 *      CHTTPFileSystem::HXShutdown()
 *
 *  Purpose:
 *
 *      Function implemented by all plugin DLL's to free any *global*
 *      resources. This method is called just before the DLL is unloaded.
 *
 */
HX_RESULT STDAPICALLTYPE CHTTPFileSystem::HXShutdown(void)
{
    // Joshe 7/19/2001:
    // because HXShutdown sometimes gets called but httpfsys can be loaded
    // from CHXIndependentHttp, we need this hack check to work around
    // the crashes caused when HXShutdown is called when it shouldn't
    // it's easier to fix here than to change plugin handler to query
    // CanUnload first, though that is probably the right thing to do eventually
    if(FAILED(CanUnload()))
    {
        return HXR_OK;
    }

    if (g_pCacheEntry)
    {
        g_pCacheEntry->close();
        delete g_pCacheEntry;
        g_pCacheEntry = NULL;
    }

    return HXR_OK;
}

/****************************************************************************
 *
 *  Function:
 *
 *      CanUnload()
 *
 *  Purpose:
 *
 *      Function implemented by all plugin DLL's if it returns HXR_OK
 *      then the pluginhandler can unload the DLL
 *
 */
HX_RESULT CHTTPFileSystem::CanUnload(void)
{
    return (g_nRefCount_http ? HXR_FAIL : HXR_OK);
}


BEGIN_INTERFACE_LIST(CHTTPFileSystem)
    INTERFACE_LIST_ENTRY(IID_IHXPlugin, IHXPlugin)
    INTERFACE_LIST_ENTRY(IID_IHXFileSystemObject, IHXFileSystemObject)
    INTERFACE_LIST_ENTRY(IID_IHXFileSystemCache, IHXFileSystemCache)
    INTERFACE_LIST_ENTRY(IID_IHXHTTPAutoStream, IHXHTTPAutoStream)
END_INTERFACE_LIST

CHTTPFileSystem::CHTTPFileSystem() :
    m_pContext(NULL)
    , m_pOptions(NULL)
{
    g_nRefCount_http++;
}

CHTTPFileSystem::~CHTTPFileSystem()
{
    g_nRefCount_http--;
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pOptions);
}

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CHTTPFileSystem::InitPlugin(IUnknown* /*IN*/ pContext)
{
    /* This plugin does not need any context */
    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    if (m_pContext)
    {
        m_pContext->AddRef();
        HX_ENABLE_LOGGING(m_pContext);
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    unInterfaceCount  the number of standard RMA interfaces
 *                      supported by this plugin DLL.
 *    pIIDList          array of IID's for standard RMA interfaces
 *                      supported by this plugin DLL.
 *    bLoadMultiple     whether or not this plugin DLL can be loaded
 *                      multiple times. All File Formats must set
 *                      this value to TRUE.
 *    pDescription      which is used in about UIs (can be NULL)
 *    pCopyright        which is used in about UIs (can be NULL)
 *    pMoreInfoURL      which is used in about UIs (can be NULL)
 */
STDMETHODIMP CHTTPFileSystem::GetPluginInfo
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
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP CHTTPFileSystem::GetFileSystemInfo
(
    REF(const char*) /*OUT*/ pShortName,
    REF(const char*) /*OUT*/ pProtocol
)
{
    pShortName  = zm_pShortName;
    pProtocol   = zm_pProtocol;

    return HXR_OK;
}

STDMETHODIMP
CHTTPFileSystem::InitFileSystem(IHXValues* options)
{
    LOGX ((szDbgTemp, "InitFileSystem()"));

    if (options)
    {
        m_pOptions = options;
        m_pOptions->AddRef();
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXFileSystemObject::CreateFile
//  Purpose:
//      TBD
//
STDMETHODIMP CHTTPFileSystem::CreateFile
(
    IUnknown**  /*OUT*/ ppFileObject
)
{
    HXLOGL2(HXLOG_HTTP, "CreateFile()");
    CHTTPFileObject* pFileObj = CHTTPFileObject::CreateObject();
    if (pFileObj)
    {
        pFileObj->InitObject(NULL, this, m_pContext, m_pOptions);

        if( m_bSaveNextStream )
        {
            pFileObj->SetDestinationFile( m_SaveFileName );
            m_bSaveNextStream = FALSE;
        }

        if(HXR_OK == pFileObj->QueryInterface(IID_IUnknown,
                                            (void**)ppFileObject))
        {
            return HXR_OK;
        }
        return HXR_FAIL;
    }
    return HXR_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//     CHTTPFileSystem::CreateDir
//  Purpose:
//     TBD
//
STDMETHODIMP CHTTPFileSystem::CreateDir
(
    IUnknown**    /*OUT*/ ppDirObject
)
{
    return HXR_NOTIMPL;
}




/////////////////////////////////////////////////////////////////////////
//  Method:
//      CHTTPFileSystem::RefreshCache
//
STDMETHODIMP
CHTTPFileSystem::RefreshCache(void)
{
    HXLOGL3(HXLOG_HTTP, "RefreshCache()");

    if (g_pCacheEntry)
    {
        g_pCacheEntry->CleanCache(0, 0);

        delete g_pCacheEntry;
        g_pCacheEntry = NULL;
    }
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      CHTTPFileSystem::EmptyCache
//
STDMETHODIMP
CHTTPFileSystem::EmptyCache(void)
{
    HXLOGL3(HXLOG_HTTP, "EmptyCache()");

    if (g_pCacheEntry == NULL)
    {
        ///XXXXXXXXXTHIS IS A TEMPORARY THING.  WE NEED TO MAKE CLEANCACHE A STATIC ROUTINE...
        // Get the location of the cache...
        IHXCommonClassFactory *commonClassFactory = NULL;
        IHXPreferences *preferences = NULL;
        IHXBuffer *buffer = NULL;
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void **)(&commonClassFactory));
        m_pContext->QueryInterface(IID_IHXPreferences, (void **)(&preferences));
        if ((preferences != NULL) && (commonClassFactory != NULL))
        {
            if (preferences->ReadPref("CacheFilename", buffer) != HXR_OK)
            {
                if (HXR_OK == commonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)(&buffer)))
                {
#ifdef USE_TEMP_CACHE_LOCATION
                    SetBufferToCacheFilePath(buffer);
#else

                    char  szModule[MAX_CACHE_FILENAME + sizeof DEF_CACHE_DB + 4] = { 0 }; /* Flawfinder: ignore */
#ifdef _MACINTOSH
                    FSSpec fileSpec;
                    GetCurrentAppSpec(&fileSpec);
                    CHXString fullpath;
                    fullpath = fileSpec;
                    SafeStrCpy(szModule, fullpath, sizeof(szModule));
#elif defined(_WIN32)
                    GetModuleFileName(NULL, OS_STRING2(szModule,MAX_CACHE_FILENAME), MAX_CACHE_FILENAME);
#else
                    *szModule = '\0';
#endif

                    INT8* pFilename = ::strrchr(szModule, OS_SEPARATOR_CHAR);

                    if (pFilename == NULL)
                    {
                        pFilename = ::strrchr(szModule, '/');
                    }

                    if (pFilename != NULL) // Found
                    {
                        // module file name starts after the separator charactor
                        *pFilename = '\0';
                    }
                    // Add on the cache database filename portion
                    ::strcat (szModule, OS_SEPARATOR_STRING); /* Flawfinder: ignore */
                    ::strcat (szModule, DEF_CACHE_DB); /* Flawfinder: ignore */
                    buffer->Set((UINT8*)szModule, strlen(szModule) + 1);
#endif // !USE_TEMP_CACHE_LOCATION
                }
            }
        }

        // Get the max size of the cache...
        ULONG32 maxCacheSize = DEFAULT_MAX_CACHE_SIZE;
        IHXBuffer *pBuffer = NULL;
        if (preferences->ReadPref("CacheMaxSize", pBuffer) == HXR_OK)
        {
            maxCacheSize = atoi((const char*)pBuffer->GetBuffer());

            if (maxCacheSize < MININUM_MAX_CACHE_SIZE)
            {
                maxCacheSize = MININUM_MAX_CACHE_SIZE;
            }
            HX_RELEASE(pBuffer);
        }
        HX_RELEASE(preferences);
        HX_RELEASE(commonClassFactory);

        // Create a new cache entry to use in a bit...
        g_pCacheEntry = new CCacheEntry (m_pContext, (char *)buffer->GetBuffer(), maxCacheSize, NULL);
        HX_RELEASE(buffer);
    }

    // We should have a cache now...
    HX_ASSERT(g_pCacheEntry != NULL);

    // Clean out the cache now...
    g_pCacheEntry->CleanCache(time(NULL), 0);

    delete g_pCacheEntry;
    g_pCacheEntry = NULL;
    return HXR_OK;
}
/////////////////////////////////////////////////////////////////////////
//  Method:
//      CHTTPFileSystem::MoveCache
//
//
STDMETHODIMP
CHTTPFileSystem::MoveCache(const char *path)
{
    HXLOGL3(HXLOG_HTTP, "MoveCache()");

    // We want to clean the cache first
    if (g_pCacheEntry)
    {
        g_pCacheEntry->CleanCache(0, 0);

        // Now move the directory sub-tree to the new location

        // Now re-open the database from the new location
        delete g_pCacheEntry;
        g_pCacheEntry = NULL;
    }

    return HXR_OK;
}


STDMETHODIMP_(void)
CHTTPFileSystem::SetDestinationFile( const char *pFilename )
{
    if( pFilename && ::strlen( pFilename ) )
    {
        m_bSaveNextStream = TRUE;
        m_SaveFileName = pFilename;
    }
    else
        m_bSaveNextStream = FALSE;
}


/*****************************************************************
 *
 * FileObject methods
 *
 *****************************************************************/

CHTTPFileObject*
CHTTPFileObject::CreateObject()
{
    HXLOGL2(HXLOG_HTTP, "CreateObject()");
    CHTTPFileObject* pNew = new CHTTPFileObject;
    if (pNew)
    {
        pNew->m_lCount = 1;
        HX_RESULT pnrRes = HXR_OK;//pNew->FinalConstruct();
        pNew->m_lCount = 0;
        if (FAILED(pnrRes))
        {
            delete pNew;
            return NULL;
        }
    }
    return pNew;
}

STDMETHODIMP_(ULONG32)
CHTTPFileObject::AddRef (THIS)
{
    return InterlockedIncrement(&m_lCount);
}

STDMETHODIMP_(ULONG32)
CHTTPFileObject::Release (THIS)
{
    HX_ASSERT(m_lCount>=0);
    if (InterlockedDecrement(&m_lCount) > 0)
    {
        return m_lCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHTTPFileObject::QueryInterface(REFIID riid, void** ppvObj)
{
    HXLOGL4(HXLOG_HTTP, "QI: QueryInterface(0x0%08x)", riid);
    if (!ppvObj)
        return HXR_POINTER;
    if (IsEqualIID(IID_IUnknown, riid))
    {
        AddRef();
        *ppvObj = (this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXRequestHandler, riid))
    {
        AddRef();
        *ppvObj = (IHXRequestHandler*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXFileObject, riid))
    {
        AddRef();
        *ppvObj = (IHXFileObject*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXFileExists, riid))
    {
        AddRef();
        *ppvObj = (IHXFileExists*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXFileStat, riid))
    {
        AddRef();
        *ppvObj = (IHXFileStat*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXFileMimeMapper, riid))
    {
        AddRef();
        *ppvObj = (IHXFileMimeMapper*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXGetFileFromSamePool, riid))
    {
        AddRef();
        *ppvObj = (IHXGetFileFromSamePool*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXPendingStatus, riid))
    {
        AddRef();
        *ppvObj = (IHXPendingStatus*)(this);

        //FNH This is probably not needed
        ProcessCacheCompletions(FALSE);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXRequestHandler, riid))
    {
        AddRef();
        *ppvObj = (IHXRequestHandler*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXTimeoutSettings, riid))
    {
        AddRef();
        *ppvObj = (IHXTimeoutSettings*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXClientAuthResponse, riid))
    {
        AddRef();
        *ppvObj = (IHXClientAuthResponse*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXHTTPRedirect, riid))
    {
        AddRef();
        *ppvObj = (IHXHTTPRedirect*)(this);
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (IsEqualIID(IID_IHXPDStatusMgr, riid))
    {
        AddRef();
        *ppvObj = (IHXPDStatusMgr*)(this);
        return HXR_OK;
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/*
BEGIN_INTERFACE_LIST(CHTTPFileObject)
    INTERFACE_LIST_ENTRY(IID_IHXRequestHandler, IHXRequestHandler)
    INTERFACE_LIST_ENTRY(IID_IHXFileObject, IHXFileObject)
    INTERFACE_LIST_ENTRY(IID_IHXFileExists, IHXFileExists)
    INTERFACE_LIST_ENTRY(IID_IHXFileStat, IHXFileStat)
    INTERFACE_LIST_ENTRY(IID_IHXFileMimeMapper, IHXFileMimeMapper)
    INTERFACE_LIST_ENTRY(IID_IHXGetFileFromSamePool, IHXGetFileFromSamePool)
    INTERFACE_LIST_ENTRY(IID_IHXPendingStatus, IHXPendingStatus)
    INTERFACE_LIST_ENTRY(IID_IHXTimeoutSettings, IHXTimeoutSettings)
    INTERFACE_LIST_ENTRY(IID_IHXClientAuthResponse, IHXClientAuthResponse)
    INTERFACE_LIST_ENTRY(IID_IHXRequestHandler, IHXRequestHandler)
END_INTERFACE_LIST
*/

CHTTPFileObject::CHTTPFileObject()
    : m_LastError (HXR_OK)

    , m_pCommonClassFactory(NULL)
    , m_pPreferences(NULL)
    , m_pScheduler(NULL)
    , m_pRegistry(NULL)
    , m_pContext(NULL)
    , m_pOptions(NULL)
    , m_pInterruptState(NULL)
#ifdef _MACINTOSH
    , m_pReadDoneBuffer(NULL)
    , m_uReadDoneStatus(HXR_OK)
    , m_bReadDoneToBeProcessed(FALSE)
#endif
    , m_pErrorMessages(NULL)

    , m_bMimeResponsePending(FALSE)
    , m_pMimeMapperResponse(NULL)

    , m_bFileExistsResponsePending(FALSE)
    , m_pFileExistsResponse(NULL)

    , m_bStatPending(FALSE)
    , m_pFileStatResponse(NULL)

    , m_bInitResponsePending(FALSE)
    , m_pFileResponse (NULL)

    , m_pFileSystem(NULL)

    , m_bTCPReadPending(FALSE)
    , m_pTCPResponse(NULL)

    , m_szBaseURL(NULL)

    , m_pFilename(NULL)
    , m_pPath(NULL)
    , m_pHost(NULL)

    , m_ulFlags(0)
    , m_pRequest(NULL)
    , m_pRequestHeadersOrig(NULL)
    , m_pParams(NULL)

    , m_pLanguage(NULL)

    , m_bAuthorized(FALSE)

    , m_pCallback(NULL)

    , m_bSeekPending (FALSE)
    , m_bInitPending (FALSE)
    , m_bGetProxyInfoPending(FALSE)

    , m_bInitialized(FALSE)
    , m_bInDestructor(FALSE)
    , m_bClosed(FALSE)

    , m_ulCurrentReadPosition (0)
    , m_ulLastKnownEndOfValidContiguousRange(0)

    , m_pDecoder(NULL)
    , m_pChunkyRes(NULL)
    , m_pMutex(NULL)

    , m_nPort(DEF_HTTP_PORT)

    , m_nRedirectLevel(0)

    , m_nRequestTime(0) // used to trigger request time-outs
    , m_bSocketReadTimeout(FALSE) // a socket read timeout has occurred
    , m_nConnTimeout(DEF_HTTP_CONNECT_TIMEOUT)
    , m_nServerTimeout(DEF_HTTP_SERVER_TIMEOUT)
    , m_bDisableConnectionTimeOut(FALSE)
    , m_bConnTimedOut(FALSE)
    , m_bMangleCookies(FALSE)
    , m_pMangledCookies(NULL)
    , m_bShoutcast(FALSE)
    , m_bConvertFailedSeeksToLinear(TRUE)
    , m_bHaltSocketReadTemporarily(FALSE)
    , m_ulMaxBufSize(MAX_CHUNK_SIZE)

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    , m_bDownloadCompleteReported(FALSE)
    , m_ulFileSizeSoFar(HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
    , m_ulPrgDnTotalFileSize(HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
    , m_ulPrgDnTotalFileDur(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulPriorReportedTotalFileDur(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulCurrentDurOfBytesSoFar(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulTimeOfLastBytesToDur(0)
    , m_pPDSObserverList(NULL)
    , m_pBytesToDur(NULL)
    , m_ulStatusUpdateGranularityInMsec(
            HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC)
    , m_bHaveReportedSupByteRngs(FALSE)
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
    , m_pSocket(NULL)
    , m_pResolve(NULL)

    , m_bHTTP1_1(TRUE)

    , m_bConnectDone(FALSE)
    , m_bWriteDone(FALSE)
    , m_bReadHeaderDone(FALSE)
    , m_bReadContentsDone(FALSE)

    , m_bKnowContentSize(FALSE)
    , m_nContentSize(0)
    , m_nOriginalContentSize(0)
    , m_bEncoded(FALSE)

    , m_bChunkedEncoding(FALSE)
    , m_pChunkedEncoding(NULL)

    , m_nContentRead(0)

    , m_bKnowHTTPResponseContentSize(FALSE)
    , m_ulHTTPResponseContentSize(0)
    , m_ulHTTPResponseContentRead(0)

    , m_ulMetaDataGap(0)
    , m_ulNextMetaDataStartIdx(0)
    , m_ulMetaDataBlockSize(0)
    , m_ulMetaDataBlockRemainingSize(0)

    , m_nTotalRequestSize(0)
    , m_nRequestWritten(0)

    , m_nHeaderRead(0)
    , m_ulBufferAheadAmount(0)

    , m_strHost()
    , m_strRequest()
    , m_strResource()
    , m_strMimeType()

    , m_bUseProxy(FALSE)
    , m_strProxyHost()
    , m_strProxyResource()
    , m_nProxyPort(DEF_HTTP_PORT)
    , m_ulMaxRecursionLevel(MAX_RECURSION_LEVEL)
    , m_ulRecursionCount(0)
    , m_bInReadDone(FALSE)
    , m_bInHandleHeaderRead(FALSE)
    , m_pCookies(NULL)
    , m_pCookies2(NULL)
    , m_pPAC(NULL)
    , m_pPACInfoList(NULL)
    , m_PACInfoPosition(0)
    , m_bOnServer(FALSE)
    , m_pCacheEntry(NULL)
    , m_bCached(FALSE)
    , m_ulCreateTime(0)
    , m_ulExpiryTime(0)
    , m_ulLastModTime(0)
    , m_bReadPending(FALSE)
    , m_bCacheEnabled(TRUE)
    , m_ulCacheMaxSize(DEFAULT_MAX_CACHE_SIZE)
    , m_pCacheFile(NULL)
    , m_ulCutOffDate(0)
    , m_bMirroredServers(FALSE)
    , m_pRedirectResponse(NULL)
    , m_bSaveToFile( FALSE )
    , m_ulOffset(0)
    , m_nPostDataSize(0)
    , m_bPartialData(FALSE)
    , m_bUseHTTPS(FALSE)

#ifdef _IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG
    , m_bSupportsByteRanges(FALSE)
    , m_bDisableByteRanges(TRUE)
#else  // _IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG
    , m_bSupportsByteRanges(TRUE)
    , m_bDisableByteRanges(FALSE)
#endif // _IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG
    , m_bExternalByteRangeRequests(FALSE)
    , m_uByteRangeSeekOffset(0)
    , m_bByteRangeSeekPending(FALSE)
    , m_bCheckingWhetherByteRangeWorks(FALSE)
    , m_bServerPresumablyWorksWithByteRangeRequests(TRUE)
    , m_pLastHeader(NULL)
    , m_ulMinStartupLengthNeeded(HTTP_MIN_STARTUP_LENGTH_NEEDED)
    , m_ulIgnoreBytesYetToBeDownloaded(0)
    , m_ulLastHeaderSize(0)
    , m_bDiscardOrphanLFInChunkedData(FALSE)
/*************************/
{
    SetSupportsByteRanges(m_bSupportsByteRanges);
}

void
CHTTPFileObject::SetSupportsByteRanges(HXBOOL bSupportsByteRanges)
{
    HXLOGL4(HXLOG_HTTP, "Seek: server supports byte ranges? %s!", bSupportsByteRanges?"YES":"NO");
    m_bSupportsByteRanges = bSupportsByteRanges;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (m_pBytesToDur)
    {
        m_bHaveReportedSupByteRngs = TRUE;

        if (m_bSupportsByteRanges)
        {
            ReportChange(PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_CLAIMED);
        }
        else
        {
            ReportChange(PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_NOT_CLAIMED);
        }
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
}

void
CHTTPFileObject::SetReadContentsDone(HXBOOL bReadContentsDone)
{
    m_bReadContentsDone = bReadContentsDone;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (m_pBytesToDur  &&  bReadContentsDone)
    {
        // /We're done downloading so call one last bytes-to-dur conversion
        // (which will kick off an OnDownloadComplete() in FF's that
        // implement it):
        UINT32 ulFileSizeDownloaded = m_ulPrgDnTotalFileSize;
        m_ulFileSizeSoFar = ulFileSizeDownloaded;
        if (!m_bKnowContentSize &&
                m_ulPrgDnTotalFileSize == HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
        {
            // /We haven't been told the total file size so assume the total
            // file size is what we were able to download:
            ulFileSizeDownloaded = m_ulCurrentReadPosition +
                    m_pChunkyRes->GetContiguousLength(m_ulCurrentReadPosition);
        }
        UINT32 ulPrevDurOfBytesSoFar = m_ulCurrentDurOfBytesSoFar;
        m_pBytesToDur->ConvertFileOffsetToDur(
                ulFileSizeDownloaded,
                ulFileSizeDownloaded,
                /*REF*/ m_ulCurrentDurOfBytesSoFar);
        m_ulPrgDnTotalFileDur = m_ulCurrentDurOfBytesSoFar;
        // /If we haven't yet reported download complete or if we have but
        // dur has changed, then notify observers:
        if (!m_bDownloadCompleteReported  ||
                ulPrevDurOfBytesSoFar != m_ulCurrentDurOfBytesSoFar)
        {
            ReportCurrentDurChanged();
            // /NOTE: if we got here because download stopped early, then this
            // will report a lower-than-entire-file-duration value:
            ReportTotalDurChanged();
            ReportDownloadComplete();
        }
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
}


void
CHTTPFileObject::InitObject
(
    char*                       szBaseURL,
    IHXFileSystemObject*       pFS,
    IUnknown*                   pContext,
    IHXValues*                  pOptions
)
{
    HXLOGL2(HXLOG_HTTP, "InitObject(%s)", NULLOK(szBaseURL));
    if (szBaseURL)
    {
        m_szBaseURL = new_string(szBaseURL);
    }

    if (pFS)
    {
        m_pFileSystem = pFS;
        m_pFileSystem->AddRef();
    }

    if (pContext)
    {
        m_pContext = pContext;
        m_pContext->AddRef();

	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
        HX_ASSERT( m_pMutex );

        m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void **)&m_pCommonClassFactory);
        m_pContext->QueryInterface(IID_IHXPreferences, (void **)&m_pPreferences);
        m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrorMessages);
        m_pContext->QueryInterface(IID_IHXInterruptState, (void**)&m_pInterruptState);
        m_pContext->QueryInterface(IID_IHXCookies, (void**)&m_pCookies);
        m_pContext->QueryInterface(IID_IHXCookies2, (void**)&m_pCookies2);

        // Figure out if we're on the server
        IHXServerControl* pServerControl = NULL;
        m_pContext->QueryInterface(IID_IHXServerControl, (void**)&pServerControl);
        if (pServerControl)
        {
            m_bOnServer = TRUE;
        }
        HX_RELEASE(pServerControl);
    }

    if (pOptions)
    {
        m_pOptions = pOptions;
        m_pOptions->AddRef();
    }

    //
    // get proxy info for HTTP protocol
    //
    UINT32 ulValue = 0;
    IHXBuffer* pBuffer = NULL;

    if (m_pPreferences)
    {
        IHXBuffer* pBuffer = NULL;

        // Get Language preference, if available
        if (m_pLanguage==NULL)
        {
            IHXRegistry* pRegistry = NULL;

            if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry) == HXR_OK)
            {
                CHXString strTemp;

                strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"Language");

                pRegistry->GetStrByName(strTemp, m_pLanguage);
                pRegistry->Release();
            }
        }

        // if the language pref is an empty string, we're not
        // interested
        if (m_pLanguage)
        {
            if ( *(m_pLanguage->GetBuffer()) == '\0')
            {
                HX_RELEASE(m_pLanguage);
            }
        }

        // Get connection timeout, if available
        m_pPreferences->ReadPref("ConnectionTimeout", pBuffer);

        // If the connection timeout is an empty string, we're not
        // interested
        if (pBuffer && *(pBuffer->GetBuffer()) != '\0')
        {
            m_nConnTimeout = (UINT32) (atol((const char*)pBuffer->GetBuffer()) * MILLISECS_PER_SECOND);
        }
        HX_RELEASE(pBuffer);

        // Check the file system options last
        if (m_pOptions &&
            HXR_OK == m_pOptions->GetPropertyULONG32("ConnectionTimeout", ulValue))
        {
            m_nConnTimeout = ulValue * MILLISECS_PER_SECOND;
        }

        // Finally, fall back to the default
        if (m_nConnTimeout <= 0)
            m_nConnTimeout = DEF_HTTP_CONNECT_TIMEOUT;



        // Get server timeout, if available
        m_pPreferences->ReadPref("ServerTimeout", pBuffer);

        // If the connection timeout is an empty string, we're not
        // interested
        if (pBuffer && *(pBuffer->GetBuffer()) != '\0')
        {
            m_nServerTimeout = (UINT32) (atol((const char*)pBuffer->GetBuffer()) * MILLISECS_PER_SECOND);
        }
        HX_RELEASE(pBuffer);

        // Check the file system options last
        if (m_pOptions &&
            HXR_OK == m_pOptions->GetPropertyULONG32("ServerTimeout", ulValue))
        {
            m_nServerTimeout = ulValue * MILLISECS_PER_SECOND;
        }

        // Finally, fall back to the default
        if (m_nServerTimeout <= 0)
            m_nServerTimeout = DEF_HTTP_SERVER_TIMEOUT;

        // Find out if Cookie Mangling is enabled
        if (m_pOptions &&
            HXR_OK == m_pOptions->GetPropertyULONG32("MangleCookies", ulValue))
        {
            m_bMangleCookies = (HXBOOL)ulValue;
        }

//      if (szBaseURL && ::strncmp (szBaseURL, "http:", 5) == 0)
        {
            CacheSupport_InitObject();
        }

        // buffer ahead amount for throttling download if desired
        ReadPrefUINT32(m_pPreferences, "HTTPBufferAheadAmount", m_ulBufferAheadAmount);
    }

    if (m_pCallback && m_pCallback->IsCallbackPending())
    {
        m_pCallback->Cancel(m_pScheduler);
    }
    HX_DELETE(m_pCallback);
    m_pCallback = new CHXGenericCallback(this, CallbackProc);
    if (m_pCallback)
    {
        m_pCallback->AddRef();
    }
}

CHTTPFileObject::~CHTTPFileObject()
{
    HXLOGL3(HXLOG_HTTP, "FileObject dtor (%s)", m_bClosed?"Closed":"Not closed");
    if(m_bInDestructor)
    {
        return;
    }

    m_bInDestructor = TRUE;
    Close();
    HX_RELEASE(m_pMutex);
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Init
 *  Purpose:
 *      Associates a file object with the file response object it should
 *      notify of operation completness. This method should also check
 *      for validity of the object (for example by opening it if it is
 *      a local file).
 */
STDMETHODIMP CHTTPFileObject::Init
(
    ULONG32             /*IN*/  ulFlags,
    IHXFileResponse*   /*IN*/  pFileResponse
)
{
    HX_LOCK(m_pMutex);

    HXLOGL1(HXLOG_HTTP, "Init");
    
    HX_RESULT   theErr  = HXR_OK;
    HX_RESULT   lResult = HXR_OK;
    char*       pTemp   = NULL;
    char*       pRes    = NULL;

    HX_RELEASE(m_pFileResponse);

    m_pFileResponse = pFileResponse;
    if (m_pFileResponse)
    {
        m_pFileResponse->AddRef();
    }

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    HX_ASSERT(m_pFileResponse);
    if (m_pFileResponse)
    {
        // /QueryInterface for the IHXMediaBytesToMediaDur interface
        HX_RELEASE(m_pBytesToDur);
        m_pFileResponse->QueryInterface(IID_IHXMediaBytesToMediaDur, (void**) &m_pBytesToDur);
        if (m_pBytesToDur)
        {
            // /The FF will know its complete-file's duration from either the
            // file's header info (if any) or from some other method:
            m_pBytesToDur->GetFileDuration(
                    // /OK if this is still HX_PROGDOWNLD_UNKNOWN_FILE_SIZE,
                    // but if we know it from the URL parameter "?filesize=...":,
                    // then we're passing that knowledge to the ff here:
                    m_ulPrgDnTotalFileSize,
                    /*REF*/ m_ulPrgDnTotalFileDur);
            if (IsPrgDnCompleteFileDurKnown())
            {
                ReportTotalDurChanged();
            }
        }
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

 /* This is temporary for Flash fileformat on Mac. We need to set a really low
 * value for recursion guard for flash fileformat since it puts everything
 * on freaki'n stack thereby blowing the system stack limit on Mac (looks like
 * there is one since it corrupts the heap) very easily.
 */
#if defined (_MACINTOSH)
    IHXGetRecursionLevel* pGet         = NULL;
    HX_RESULT              lThisResult  = HXR_OK;
    lThisResult = m_pFileResponse->QueryInterface(IID_IHXGetRecursionLevel,
                                       (void**) &pGet);
    if (lThisResult == HXR_OK)
    {
            m_ulMaxRecursionLevel = pGet->GetRecursionLevel();
            pGet->Release();
    }
#endif

    if (m_bInitialized)
    {
        if (m_LastError == HXR_OK)
        {
            /* If we have already opened a file, then seek back
             * to zero during re-initialization
             */
            _SetCurrentReadPos(0);
            m_bServerPresumablyWorksWithByteRangeRequests = TRUE;

            HX_UNLOCK(m_pMutex);
            m_pFileResponse->InitDone(HXR_OK);
            return HXR_OK;
        }
        else
        {
            HX_UNLOCK(m_pMutex);
            m_pFileResponse->InitDone(HXR_FAILED);
            return HXR_FAILED;
        }
    }

    theErr = _OpenFile(m_pFilename, ulFlags);

    if (HXR_OK == theErr || HXR_WOULD_BLOCK == theErr)
    {
        if (m_bCached)
        {
            LOG ("    Calling InitDone(OK) [cached]");
            _SetCurrentReadPos(0);
            m_bServerPresumablyWorksWithByteRangeRequests = TRUE;
            m_pFileResponse->InitDone(HXR_OK);
        }
        else
        {
            m_bInitResponsePending = TRUE;
        }
    }
    else
    {
        m_pFileResponse->InitDone(HXR_FAILED);
    }

    HX_UNLOCK(m_pMutex);
    return theErr;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::GetFilename
 *  Purpose:
 *      Returns the filename (without any path information) associated
 *      with a file object.
 */
STDMETHODIMP CHTTPFileObject::GetFilename
(
    REF(const char*) /*OUT*/ pFilename
)
{
    HXScopeLock lock(m_pMutex);

    // Find the separator character before the file name
    pFilename = ::strrchr(m_pFilename, OS_SEPARATOR_CHAR);

    // File may not be local so try a '/' separator
    // it could also be a MAC on the :// protocol point...
    if (pFilename == NULL ||
        ( *pFilename == ':' && *(pFilename + 1) == '/' )  )
    {
        pFilename = ::strrchr(m_pFilename, '/');
    }

    if (pFilename != NULL) // Found
    {
        // File name starts after the separator charactor
        pFilename++;
    }
    else // Not found
    {
        pFilename = m_pFilename;
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Close
 *  Purpose:
 *      Closes the file resource and releases all resources associated
 *      with the object.
 */
STDMETHODIMP CHTTPFileObject::Close()
{
    if (m_bClosed)
    {
	return HXR_OK;
    }

    HX_LOCK(m_pMutex);

    HXLOGL1(HXLOG_HTTP, "Close(%s)", NULLOK(m_pFilename));

    if (m_bGetProxyInfoPending)
    {
        m_bGetProxyInfoPending = FALSE;
        m_pPAC->AbortProxyInfo(this);
    }

    g_ChunkyResMap.RelinquishChunkyRes(m_pChunkyRes, this);
    m_pChunkyRes = NULL;

    HX_RELEASE(m_pFileSystem);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pRequestHeadersOrig);
    HX_RELEASE(m_pParams);
    HX_RELEASE(m_pRedirectResponse);
    HX_RELEASE(m_pCacheFile);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pCookies);
    HX_RELEASE(m_pCookies2);
    HX_RELEASE(m_pPAC);
    HX_RELEASE(m_pMangledCookies);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pOptions);

    // If there is a pending callback, be sure to remove it!
    if (m_pCallback &&
        m_pCallback->IsCallbackPending())
    {
        m_pCallback->Cancel(m_pScheduler);
    }
    HX_RELEASE(m_pCallback);

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pErrorMessages);

    if (m_pTCPResponse)
    {
        // We may be destroyed within a call to EventPending(),
        // in which case we need the weak reference until EventPending() completes.
        //m_pTCPResponse->m_pOwner = NULL;
        m_pTCPResponse->m_bOwnerDestroyed = TRUE;
        HX_RELEASE(m_pTCPResponse);
    }

    if( m_pResolve )
    {
        m_pResolve->Close();
        HX_RELEASE(m_pResolve);
    }
    
    if (m_pSocket)
    {
        m_pSocket->Close();
        HX_RELEASE(m_pSocket);
    }
    

    HX_RELEASE(m_pMimeMapperResponse);
    HX_RELEASE(m_pFileExistsResponse);

    HX_RELEASE(m_pFileStatResponse);
    HX_RELEASE(m_pPreferences);

    HX_RELEASE(m_pLanguage);

    HX_VECTOR_DELETE(m_szBaseURL);

    HX_VECTOR_DELETE(m_pFilename);
    HX_VECTOR_DELETE(m_pPath);
    HX_VECTOR_DELETE(m_pHost);

#if defined(HELIX_FEATURE_HTTP_GZIP)
    HX_DELETE(m_pDecoder);
#endif

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    m_bDownloadCompleteReported = FALSE;
    m_ulFileSizeSoFar = HX_PROGDOWNLD_UNKNOWN_FILE_SIZE;
    m_ulPrgDnTotalFileSize = HX_PROGDOWNLD_UNKNOWN_FILE_SIZE;
    m_ulPrgDnTotalFileDur = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulPriorReportedTotalFileDur = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulCurrentDurOfBytesSoFar = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulTimeOfLastBytesToDur = 0;
    HX_RELEASE(m_pBytesToDur);
    m_bHaveReportedSupByteRngs = FALSE;
    m_ulStatusUpdateGranularityInMsec =
            HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC;
    if (m_pPDSObserverList)
    {
        CHXSimpleList::Iterator i = m_pPDSObserverList->Begin();
        for (; i != m_pPDSObserverList->End(); ++i)
        {
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)(*i);
            HX_RELEASE(pObserver);
        }
        HX_DELETE(m_pPDSObserverList);
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    if (m_pChunkedEncoding)
    {
        HX_VECTOR_DELETE(m_pChunkedEncoding->buf);
        m_pChunkedEncoding->buf_size = 0;
        HX_DELETE(m_pChunkedEncoding);
    }

    while (m_pPACInfoList && m_pPACInfoList->GetCount())
    {
        PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->RemoveHead();
        HX_DELETE(pPACInfo);
    }
    HX_DELETE(m_pPACInfoList);

    HX_VECTOR_DELETE(m_pLastHeader);

    // Flag to let us know that we've been closed
    m_bClosed = TRUE;

    HX_UNLOCK(m_pMutex);

    if (m_bInDestructor)
    {
        HX_RELEASE(m_pFileResponse);
    }
    else
    {
        AddRef();
        if (m_pFileResponse)
        {
            // Call CloseDone()
            m_pFileResponse->CloseDone(HXR_OK);
            // Keeps us from getting into an infinite loop when
            // we have two FileObjects on the same file which causes the
            // ref counts on the objects to be such that this's m_pFileResponse
            // object drops the ref count on this which causes it to destruct
            // which causes it to call ::Close() which *used to* again call
            // HX_RELEASE(m_pFileResponse) which *used to* again release *this which
            // called ::Close()...etc until the stack blew up.  This fix
            // keeps it from spinning out of control by setting m_pFileResponse
            // to NULL before releasing it via temp ptr:
            IHXFileResponse* pTempFileResponse = m_pFileResponse;
            m_pFileResponse = NULL;
            HX_RELEASE(pTempFileResponse);
        }
        Release();
        // we mustn't touch any member variables from here until we exit this
        // function, because we may have just deleted ourself.
    }

    return HXR_OK;
}

void
CHTTPFileObject::CallReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    m_bInReadDone = TRUE;

    HXLOGL4(HXLOG_HTTP, "Read: CallReadDone status %08x buffer size %lu",
        status, pBuffer ? pBuffer->GetSize() : 0);

    // Let the file response sink know about the buffer...
    HX_ASSERT(m_pFileResponse) ;
    if (m_pFileResponse)
    {
        m_pFileResponse->ReadDone(status, pBuffer);
    }

    m_bInReadDone = FALSE;

    if (m_ulRecursionCount > 0)
    {
        InterlockedDecrement(&m_ulRecursionCount);
    }
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Read
 *  Purpose:
 *      Reads a buffer of data of the specified length from the file
 *      and asynchronously returns it to the caller via the
 *      IHXFileResponse interface passed in to Init.
 */
STDMETHODIMP CHTTPFileObject::Read(ULONG32 ulCount)
{
    HXScopeLock lock(m_pMutex);

    HX_RESULT           lResult      = HXR_OK;
    static UINT32       ulReadCount  = 0;
    HX_RESULT           lSocketReadResult = HXR_OK;
    UINT32              ulCurrentContig = 0;

    HXLOGL3(HXLOG_HTTP, "Read #%03lu, Pos is %lu, Size is %4lu, Recurs is %2u, FileObj is '%s'",
            ++ulReadCount, m_ulCurrentReadPosition, ulCount, m_ulRecursionCount + 1, NULLOK(m_pFilename));

    if (m_LastError)
    {
        return m_LastError;
    }

    /* It is illegal to call read if a seek is pending */
    if (m_bSeekPending)
    {
        HXLOGL1(HXLOG_HTTP, "Read (%d) cancelled! Seek is pending!", ulCount);

        HX_UNLOCK(m_pMutex);

        CallReadDone(HXR_SEEK_PENDING ,NULL);

        HX_LOCK(m_pMutex);

        return HXR_UNEXPECTED;
    }

    /* mark it as a pending read request..
     * We do this since a call to readdone may result in a call to read
     * which may mess up the sequence in which we read data and pass it to
     * the caller (specially when some reads are pendiing
     */
    m_PendingReadList.AddTail((void*) ulCount);


    if (m_bInReadDone)
    {
        InterlockedIncrement(&m_ulRecursionCount);
    }

    if (m_ulRecursionCount > m_ulMaxRecursionLevel)
    {
        if (m_pCallback && !m_pCallback->IsCallbackPending())
        {
            m_pCallback->ScheduleRelative(m_pScheduler, 0);
        }

        return HXR_OK;
    }

    // xxxbobclark we should see if we're going to block and if we're going
    // to block we should try to read the socket to see whether we can
    // avoid blocking.

    // xxxbobclark although we should only do this if we're not being called
    // (potentially recursively) from the 206 case of HandleSuccess: reading
    // from the socket here before HandleSuccess has finished unwinding will
    // result in data being stuck into ChunkyRes in the wrong order.

    if (!m_bInHandleHeaderRead)
    {
        lSocketReadResult = HXR_OK;

        ulCurrentContig = _GetContiguousLength();

        while (ulCount > ulCurrentContig && lSocketReadResult == HXR_OK)
        {
            lSocketReadResult = _DoSomeReadingFromSocket(TRUE);

            if (SUCCEEDED(lSocketReadResult))
            {
                UINT32 ulNewContig = _GetContiguousLength();
                if (ulNewContig == ulCurrentContig )
                {
                    break; // get out of this while loop 'cuz we're not getting data
                }
                ulCurrentContig = ulNewContig;
            }
        }
    }
    
    HX_UNLOCK(m_pMutex);
    lResult = ProcessPendingReads();
    HX_LOCK(m_pMutex);

    return lResult;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Write
 *  Purpose:
 *      Writes a buffer of data to the file and asynchronously notifies
 *      the caller via the IHXFileResponse interface passed in to Init,
 *      of the completeness of the operation.
 */
STDMETHODIMP CHTTPFileObject::Write(IHXBuffer* pBuffer)
{
    HXScopeLock lock(m_pMutex);

    HXLOGL3(HXLOG_HTTP, "Write (%lu)", pBuffer ? pBuffer->GetSize() : 0);
    HX_RESULT nRetVal = HXR_FAILED;

    if (m_pSocket && m_nPostDataSize && pBuffer)
    {
        nRetVal = m_pSocket->Write(pBuffer);

        if(nRetVal == HXR_OK)
        {
            if(m_nPostDataSize >= pBuffer->GetSize())
                m_nPostDataSize -= pBuffer->GetSize();
            else
                m_nPostDataSize = 0;
        }
    }

    return nRetVal;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Seek
 *  Purpose:
 *      Seeks to an offset in the file and asynchronously notifies
 *      the caller via the IHXFileResponse interface passed in to Init,
 *      of the completeness of the operation.
 */
STDMETHODIMP CHTTPFileObject::Seek(ULONG32 ulOffset, HXBOOL bRelative)
{
    HXScopeLock lock(m_pMutex);

    HX_RESULT theErr = HXR_OK;

    HXLOGL3(HXLOG_HTTP, "Seek curpos %lu to offset %d %s",
        m_ulCurrentReadPosition, ulOffset, bRelative?"(relative)":"");

    if (m_LastError)
    {
        return m_LastError;
    }

    while (m_PendingReadList.GetCount())
    {
        m_PendingReadList.RemoveHead();
        HX_UNLOCK(m_pMutex);
        m_pFileResponse->ReadDone(HXR_CANCELLED, NULL);
        HX_LOCK(m_pMutex);
    }

    if (bRelative)
    {
        _SetCurrentReadPos(m_ulCurrentReadPosition + ulOffset);
    }
    else
    {
        _SetCurrentReadPos(ulOffset);
    }

    /* check if there were any pending reads */
    if (m_bSeekPending || m_bReadPending)
    {
        HXLOGL1(HXLOG_HTTP, "Seek (%lu) cancelled! Pending %s %s!", ulOffset,
            m_bSeekPending?"Seek":"", m_bReadPending?"Read":"");

        m_bSeekPending  = FALSE;
        HX_UNLOCK(m_pMutex);
        m_pFileResponse->SeekDone(HXR_CANCELLED);
        HX_LOCK(m_pMutex);
    }

    if (_GetContiguousLength() > 0)
    {
        // Don't call SeekDone until we are actaully done with the seek
        if (m_bSupportsByteRanges)
        {
            _EnsureThatWeAreReadingWisely();

            // m_bByteRangeSeekPending means we reconnecting the socket
            // a new location.
            if (m_bByteRangeSeekPending)
            {
                m_bSeekPending = TRUE;
            }
            else
            {
                HXLOGL4(HXLOG_HTTP, "Seek: no reconnect necessary, has data already");

                HX_UNLOCK(m_pMutex);
                m_pFileResponse->SeekDone(HXR_OK);
                HX_LOCK(m_pMutex);
            }
        }
        else
        {
            HXLOGL4(HXLOG_HTTP, "Seek: no byte range support, will wait for data if necessary");

            HX_UNLOCK(m_pMutex);
            m_pFileResponse->SeekDone(HXR_OK);
            HX_LOCK(m_pMutex);
        }
    }
    else
    {
        if (m_bSupportsByteRanges)
        {
            // signal the caller about the end of content
            if (m_ulCurrentReadPosition == m_nContentRead)
            {
                HXLOGL2(HXLOG_HTTP, "Seek: at end of read content");

                HX_UNLOCK(m_pMutex);
                m_pFileResponse->SeekDone(HXR_OK);
                HX_LOCK(m_pMutex);
            }
            else if (m_bKnowContentSize && m_ulCurrentReadPosition == m_nContentSize)
            {
                HXLOGL2(HXLOG_HTTP, "Seek: at end of content");

                HX_UNLOCK(m_pMutex);
                m_pFileResponse->SeekDone(HXR_OK);
                HX_LOCK(m_pMutex);
            }
            else
            {
                // We need to reconnect if:
                // 1. The read pointer is behind the write pointer and there are gaps OR
                // 2. The write pointer is behind the read pointer with too many bytes to simply wait for
                if (!m_bByteRangeSeekPending)
                {
                    if (m_ulCurrentReadPosition < m_nContentRead ||
                        m_ulCurrentReadPosition - m_nContentRead > BYTE_RANGE_SEEK_THRESHHOLD)
                    {
                        HXLOGL2(HXLOG_HTTP, "Seek: need to reconnect");
                        _HandleByteRangeSeek(m_ulCurrentReadPosition);
                    }
                }
                m_bSeekPending = TRUE;
            }
        }
        else
        {
            /* is it a valid value to seek to?*/
            if (m_bReadContentsDone)
            {
                // signal the caller about the end of content
                if (m_ulCurrentReadPosition == m_nContentRead)
                {
                    HX_UNLOCK(m_pMutex);
                    m_pFileResponse->SeekDone(HXR_OK);
                    HX_LOCK(m_pMutex);
                }
                else
                {
                    HX_ASSERT(m_ulCurrentReadPosition > m_nContentRead);
                    HX_UNLOCK(m_pMutex);
                    m_pFileResponse->SeekDone(HXR_FAILED);
                    HX_LOCK(m_pMutex);
                }
            }
            else /* add it to the seek pending queue...*/
            {
                m_bSeekPending = TRUE;
            }
        }
    }

    return (theErr);
}



/************************************************************************
 * Method:
 *      IHXFileObject::Stat
 * Purpose:
 *      Collects information about the file that is returned to the
 *      caller in an IHXStat object
 */
STDMETHODIMP CHTTPFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    HXLOGL2(HXLOG_HTTP, "Stat(%s)", m_pFilename);
    /*
     * XXXGH...Need to get real statistics
     */
    if(!m_bReadHeaderDone)
    {
        m_bStatPending = TRUE;
        m_pFileStatResponse = pFileStatResponse;
        m_pFileStatResponse->AddRef();
    }
    else
    {
        HX_RESULT hxrStatDoneStatus = HXR_OK;
        UINT32 ulStatDoneFileSize = m_nContentSize;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
        if (!m_bKnowContentSize)
        {
            if (m_ulPrgDnTotalFileSize == HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
            {
                // /We have no idea what the correct size of the file is
                // so we just return FAIL w/unknown size:
                hxrStatDoneStatus = HXR_FAIL;
                ulStatDoneFileSize = HX_PROGDOWNLD_UNKNOWN_FILE_SIZE;
            }
            else // /Use URL filesize parameter:
            {
                ulStatDoneFileSize = m_ulPrgDnTotalFileSize;
            }
        }
#endif // /end ifdef HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
        pFileStatResponse->StatDone(hxrStatDoneStatus, ulStatDoneFileSize,
                0, 0, 0, 0);
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Advise
 *  Purpose:
 *      To pass information to the File Object
 */
STDMETHODIMP CHTTPFileObject::Advise(ULONG32 ulInfo)
{
    HXLOGL2(HXLOG_HTTP, "Advise(%s, %u)", m_pFilename, ulInfo);

    HX_RESULT pnr = HXR_OK;

    // XXXJEFFA it's possible that for cached HTTP we want to
    // return HXR_OK from this instead of prefer linear since
    // the file is local in the cache but that's an optimization
    // we can make later JEFFA 11/8/98

    // disable the HTTP1.1 support if the file format
    // doesn't support async seek(i.e. AVI)
    if (ulInfo == HXR_ADVISE_NOASYNC_SEEK)
    {
        m_bDisableByteRanges = TRUE;
        SetSupportsByteRanges(FALSE);
    }
    else if (ulInfo == HX_FILEADVISE_RANDOMACCESS)
    {
        if (m_bSupportsByteRanges && !IsLiveStream(m_strMimeType))
            pnr = HXR_OK;
        else
            pnr = HXR_ADVISE_PREFER_LINEAR;
    }
    else if (HX_FILEADVISE_RANDOMACCESSONLY == ulInfo)
    {
        m_bConvertFailedSeeksToLinear = FALSE;
    }
    else if (HX_FILEADVISE_ANYACCESS == ulInfo)
    {
        m_bConvertFailedSeeksToLinear = TRUE;
    }
    else if (HX_FILEADVISE_NETWORKACCESS == ulInfo)
    {
	pnr = HXR_ADVISE_NETWORK_ACCESS;
    }

    return pnr;
}


/************************************************************************
 *      Method:
 *          IHXFileObject::GetFileObjectFromPool
 *      Purpose:
 *      To get another FileObject from the same pool.
 */
STDMETHODIMP CHTTPFileObject::GetFileObjectFromPool(
    IHXGetFileFromSamePoolResponse* response)
{
    HXLOGL2(HXLOG_HTTP, "GetFileObjectFromPool(%s)", NULLOK(m_pFilename));

    HX_RESULT           lReturnVal = HXR_FAIL;
    CHTTPFileObject*    pFileObject = NULL;
    IUnknown*           pUnknown = NULL;
    CHXString           sBaseURL;
    INT32               nBeforeFile;

    sBaseURL = m_pFilename;

    nBeforeFile = HX_MAX
    (
        sBaseURL.ReverseFind('\\'),
        sBaseURL.ReverseFind('/')
    );

    if(nBeforeFile > -1)
    {
        sBaseURL = sBaseURL.Left(nBeforeFile);

        pFileObject = CHTTPFileObject::CreateObject();
        if (pFileObject)
        {
            pFileObject->InitObject
            (
                sBaseURL.GetBuffer(1),
                m_pFileSystem,
                m_pContext,
                m_pOptions
            );

            lReturnVal = pFileObject->QueryInterface
            (
                IID_IUnknown,
                (void**)&pUnknown
            );
        }
        else
        {
            response->FileObjectReady(HXR_OUTOFMEMORY, NULL);
            return HXR_OUTOFMEMORY;
        }
    }

    response->FileObjectReady
    (
        lReturnVal == HXR_OK ? HXR_OK : HXR_FAIL,
        pUnknown
    );

    HX_RELEASE(pUnknown);

    return lReturnVal;
}

// IHXFileExists interface
/************************************************************************
 *      Method:
 *          IHXFileExists::DoesExist
 *      Purpose:
 */
STDMETHODIMP CHTTPFileObject::DoesExist(
                        const char*             /*IN*/  pPath,
                        IHXFileExistsResponse* /*IN*/  pFileExistsResponse)
{
    HXLOGL2(HXLOG_HTTP, "DoesExist(%s)", NULLOK(pPath));

    HX_RESULT theErr = HXR_OK;
    IHXRequestHandler* pReqHandler = NULL;
    IHXRequest* pRequest = NULL;
    IHXProxyAutoConfig* pPAC = NULL;

    if (m_bInitialized)
    {
        if (m_LastError == HXR_OK)
        {
            AddNoCacheHeader();
            pFileExistsResponse->DoesExistDone(TRUE);
            return HXR_OK;
        }
        else
        {
            pFileExistsResponse->DoesExistDone(FALSE);
            return HXR_OK;
        }
    }

    // If we have received a request object, extract from it the
    // full URL. This will include any URL parameters which we
    // need, but which are not contained in the simple file name
    // that was passed in as our first parameter. - DPS
    HX_ASSERT(m_pRequest);
    if (m_pRequest)
    {
        m_pRequest->GetURL(pPath);
    }

    // we don't want use PAC if we are within the process of PAC detecting
    if (HXR_OK == pFileExistsResponse->QueryInterface(IID_IHXProxyAutoConfig, (void**)&pPAC))
    {
        theErr = _OpenFile(pPath, HX_FILE_READ | HX_FILE_BINARY | HX_FILE_NOPAC);
    }
    else
    {
        theErr = _OpenFile(pPath, HX_FILE_READ | HX_FILE_BINARY);
    }
    HX_RELEASE(pPAC);

    if (HXR_OK == theErr || HXR_WOULD_BLOCK == theErr)
    {
        if (m_bCached)
        {
            pFileExistsResponse->DoesExistDone(TRUE);
        }
        else
        {
        m_pFileExistsResponse = pFileExistsResponse;
        m_pFileExistsResponse->AddRef();
        m_bFileExistsResponsePending = TRUE;
        }
        return HXR_OK;
    }
    else
    {
        pFileExistsResponse->DoesExistDone(FALSE);
        return HXR_OK;
    }
}


/*
 *      IHXFileMimeMapper methods
 */
/************************************************************************
 *      Method:
 *          IHXFileMimeMapper::FindMimeType
 *      Purpose:
 */
STDMETHODIMP CHTTPFileObject::FindMimeType(
                const char*                 /*IN*/  pURL,
                IHXFileMimeMapperResponse* /*IN*/  pMimeMapperResponse)
{
    HXLOGL2(HXLOG_HTTP, "FindMimeType(%s)", NULLOK(pURL));

    HX_RESULT theErr = HXR_OK;

    if (m_bInitialized)
    {
        if (m_LastError == HXR_OK)
        {
            // Determine mime type here!
            const char* pMimeType = NULL;
            if (!m_strMimeType.IsEmpty()) pMimeType = m_strMimeType;

            HXLOGL3(HXLOG_HTTP, "    MimeTypeFound(%s)", NULLOK(pMimeType));
            pMimeMapperResponse->MimeTypeFound(HXR_OK, pMimeType);
            return HXR_OK;
        }
        else
        {
            HXLOGL3(HXLOG_HTTP, "    Mime type NOT found!");
            pMimeMapperResponse->MimeTypeFound(m_LastError, NULL);
            return HXR_FAILED;
        }
    }

    // If we have received a request object, extract from it the
    // full URL. This will include any URL parameters which we
    // need, but which are not contained in the simple file name
    // that was passed in as our first parameter.
    HX_ASSERT(m_pRequest);
    if (m_pRequest)
    {
    m_pRequest->GetURL(pURL);
    }

    theErr = _OpenFile(pURL,HX_FILE_READ|HX_FILE_BINARY);

    if (HXR_OK == theErr || HXR_WOULD_BLOCK == theErr)
    {
        if (m_bCached)
        {
            const char* pMimeType = NULL;
            if (!m_strMimeType.IsEmpty())
            {
                pMimeType = m_strMimeType;
            }

            HXLOGL3(HXLOG_HTTP, "    Cached mime type %s", NULLOK((const char*)m_strMimeType));
            pMimeMapperResponse->MimeTypeFound(HXR_OK, pMimeType);
        }
        else
        {
        m_pMimeMapperResponse = pMimeMapperResponse;
        m_pMimeMapperResponse->AddRef();
        m_bMimeResponsePending = TRUE;
        }
        return HXR_OK;
    }
    else
    {
        pMimeMapperResponse->MimeTypeFound(HXR_INVALID_PATH, NULL);
        return HXR_FAILED;
    }
}

/*
 * IHXPendingStatus methods
 */

/************************************************************************
 *      Method:
 *          IHXPendingStatus::GetStatus
 *      Purpose:
 *          Called by the user to get the current pending status from an object
 */
STDMETHODIMP CHTTPFileObject::GetStatus(REF(UINT16) uStatusCode,
                                        REF(IHXBuffer*) pStatusDesc,
                                        REF(UINT16) ulPercentDone)
{
    HXLOGL2(HXLOG_HTTP, "GetStatus");
    /* Default values*/
    uStatusCode     = HX_STATUS_READY;
    pStatusDesc     = 0;
    ulPercentDone   = 0;

    if (!m_bConnectDone)
    {
        uStatusCode     = HX_STATUS_CONTACTING;
        if (!m_strHost.IsEmpty())
        {
            CHXString statusDesc = "Contacting ";
            statusDesc += m_strHost;
            statusDesc += "...";
	    CreateAndSetBufferCCF(pStatusDesc, (UCHAR*)(const char*) statusDesc,
                             strlen((const char*)statusDesc)+1, m_pContext);
        }
        ulPercentDone   = 0;
    }
    else if (m_bReadContentsDone)
    {
        uStatusCode     = HX_STATUS_READY;
        ulPercentDone   = 0;
    }
    else if (m_bSeekPending || !m_PendingReadList.IsEmpty())
    {
        uStatusCode = HX_STATUS_BUFFERING;

        ULONG32 ulReadCount = 0;
        if (!m_PendingReadList.IsEmpty())
        {
            ulReadCount = (ULONG32)(PTR_INT)m_PendingReadList.GetHead();
        }

        if (m_ulCurrentReadPosition+ulReadCount)
        {
            ulPercentDone = (UINT16) ((m_nContentRead*100)/(m_ulCurrentReadPosition+ulReadCount));
            ulPercentDone = ulPercentDone <= 100 ? (UINT16) ulPercentDone : 100;
        }
        else
        {
            ulPercentDone = (UINT16) 100;
        }
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          Private interface::InitializeChunkyRes
 *      Purpose:
 *          This encapsulates initialization of the chunky res and associated
 *          objects
 */
HX_RESULT CHTTPFileObject::_InitializeChunkyRes(const char* url)
{
    if (!m_pChunkyRes)
    {
        m_pChunkyRes = g_ChunkyResMap.GetChunkyResForURL(url, this, m_pContext);

        HXLOGL1(HXLOG_HTTP, "_InitializeChunkyRes(%s) ==> %lx", NULLOK(url), m_pChunkyRes);

        if (m_bOnServer)
        {
            m_pChunkyRes->DisableDiskIO();
        }

#if defined(HELIX_FEATURE_HTTP_GZIP)
        m_pDecoder = new CDecoder;
        if (m_pDecoder && m_pChunkyRes)
        {
            m_pDecoder->SetOutputSink(m_pChunkyRes);
        }
#endif

    }
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          Private interface::OpenFile
 *      Purpose:
 *          This common method is used from Init() and GetFileObjectFromPool()
 */
HX_RESULT CHTTPFileObject::_OpenFile(const char* url,
                                      ULONG32     ulFlags)
{
    HX_RESULT       theErr  = HXR_OK;
    HX_RESULT       lResult = HXR_OK;
    UINT16          un16Temp = 0;
    char*           pTemp   = NULL;
    IHXBuffer*      pBuffer = NULL;
    IHXBuffer*      pProxyName = NULL;
    IHXBuffer*      pProxyPort = NULL;
    IHXProxyManager*   pProxyManager = NULL;

    _InitializeChunkyRes(url);

    HXLOGL1(HXLOG_HTTP, "_OpenFile(%s)", NULLOK(url));
    // Make local copy of url
    CHXString   strTemp = url;
    char*       pTempURL    = strTemp.GetBuffer(strTemp.GetLength());
    char*       pURL        = NULL;
    char*       pOrigURL    = NULL;

    CHXURL* pCHXURL = new CHXURL(pTempURL, m_pContext);
    if (pCHXURL)
    {
        IHXValues* pHeader = pCHXURL->GetProperties();
        if(pHeader)
        {
            IHXBuffer* pUrlBuffer = NULL;
            if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_URL, pUrlBuffer) &&
                pUrlBuffer)
            {
                pURL = ::new_string((const char*)pUrlBuffer->GetBuffer());
                HXLOGL2(HXLOG_HTTP, "_OpenFile properly escaped URL is %s", NULLOK(pURL));
                HX_RELEASE(pUrlBuffer);
            }
            HX_RELEASE(pHeader);
        }
        delete pCHXURL;
    }

    // if somehow the URL is messed up and the URL parser freaks out,
    // we fall back to the original URL.
    if (!pURL)
    {
        HXLOGL1(HXLOG_HTTP, "_OpenFile url had trouble parsing! Fallback to original URL");
        pURL = ::new_string(pTempURL);
    }

    pOrigURL = pURL;

    if (pOrigURL && (strncasecmp(pOrigURL, "https:", 6) == 0))
    {
        m_bUseHTTPS = TRUE;
    }
    else
    {
        m_bUseHTTPS = FALSE;
    }


    // HTTP requires '/' as the element delimiter.
    // (Or at least the TIS proxy does)
    // So change all '\\' to '/'.
    // Also, try not to proccess any parameters.
    pTemp = pURL;
    while(*pTemp && *pTemp!='?' && *pTemp!='#')
    {
        if(*pTemp == '\\')
        {
            *pTemp = '/';
        }
        pTemp++;
    }

    // We always store the entire original URL as the proxy resource!
    m_strProxyResource = pURL;

    m_strHost           = "";
    m_strResource       = "";

    if(m_bUseHTTPS)
    {
        m_nPort = DEF_HTTPS_PORT;
    }
    else
    {
        m_nPort = DEF_HTTP_PORT;
    }


    // if the url's first five characters are "http:"
    // then jump past them... Otherwise, just assume
    // the URL starts with the host... This will allow
    // someone to mount the HTTP file system on the
    // server and proxy another web server... <g>
    // [Modified to look for ':' character - fnh]
    char* pcColon = (char *) HXFindChar (pURL, ':');
    char* pcQuery = (char *) HXFindChar (pURL, '?');
    if (pcColon && (pcQuery == 0 || pcColon < pcQuery))
    {
        pURL = pcColon + 1;
    }

    // Jump past the double whack if present...
    if (HXCompareStrings(pURL, "//", 2) == 0)
    {
        pURL += 2;
    }

    pTemp = (char *)HXFindChar(pURL, '/');

    if (pTemp)
    {
        m_strResource = pTemp;
        *pTemp = '\0';
    }

    // Look for the '@' character which means there is a username and/or password.  We skip over this part to the
    // hostname.  The '@' character is reserved according to RFC 1738 show it shouldn't appear in a URL
    pTemp = (char *)HXFindChar (pURL, '@');
    if (pTemp)
        pURL += (pTemp - pURL + 1);

    /* Port (optional) */
    pTemp = (char *)HXFindChar (pURL, ':');
    if (pTemp)
    {
        *pTemp = '\0';
        m_nPort = ::atoi(pTemp+1);

        // port '0' is invalid, but we'll get that if the url had ':' but no port number
        // following. Setting to default http_port in this case will mimic the
        // behaviour of pnm: and rtsp: if no port specified after ':'
        if (m_nPort==0)
        {
            if(m_bUseHTTPS)
            {
                m_nPort = DEF_HTTPS_PORT;
            }
            else
            {
                m_nPort = DEF_HTTP_PORT;
            }
        }
    } /* if (pTemp) */

    m_strHost = pURL;

    if(m_pPreferences) 
    {
        if (m_pPreferences->ReadPref("HTTPProxyAutoConfig", pBuffer) == HXR_OK)
        {
            un16Temp = atoi((const char*) pBuffer->GetBuffer());
        }
        // previously released Enterprise player may use "ProxyAutoConfig" for
        // HTTP proxy auto config
        else if (m_pPreferences->ReadPref("ProxyAutoConfig", pBuffer) == HXR_OK)
        {
            un16Temp = atoi((const char*) pBuffer->GetBuffer());
        }
    }
    HX_RELEASE(pBuffer);

    // HTTP Proxy Auto Config
    if (un16Temp && !(HX_FILE_NOPAC & ulFlags))
    {
        if (!m_pPAC)
        {
            m_pContext->QueryInterface(IID_IHXProxyAutoConfig, (void**)&m_pPAC);
        }

        if (m_pPAC &&
            (!m_pPACInfoList || 0 == m_pPACInfoList->GetCount()))
        {
            theErr = m_pPAC->GetHTTPProxyInfo((IHXProxyAutoConfigCallback*)this,
                                              url,
                                              m_strHost);
        }
        // attempt the next proxy info from m_pPACInfoList
        else if (m_pPACInfoList && m_PACInfoPosition)
        {
            PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->GetNext(m_PACInfoPosition);
            if (pPACInfo && pPACInfo->type != PAC_DIRECT)
            {
                m_bUseProxy = TRUE;
                m_nProxyPort = pPACInfo->ulPort;
                m_strProxyHost = pPACInfo->pszHost;
            }
        }

        // XXX HP TBD
        // we should attempt the next proxy info from m_pPACInfoList
        if (HXR_WOULD_BLOCK == theErr)
        {
            m_bGetProxyInfoPending = TRUE;
            goto cleanup;
        }
    }
    else if (m_pPreferences && m_pPreferences->ReadPref("HTTPProxySupport", pBuffer) == HXR_OK)
    {
        if (atoi((const char*)pBuffer->GetBuffer()))
        {
            if(m_pPreferences->ReadPref("HTTPProxyHost", pProxyName) == HXR_OK &&
               m_pPreferences->ReadPref("HTTPProxyPort", pProxyPort) == HXR_OK)
            {
                m_nProxyPort = atoi((const char*)pProxyPort->GetBuffer());
                m_strProxyHost = (const char*)pProxyName->GetBuffer();

                if (m_strProxyHost.GetLength() > 0 && m_nProxyPort > 0)
                {
                    if (HXR_OK == m_pContext->QueryInterface(IID_IHXProxyManager, (void**)&pProxyManager) &&
                        pProxyManager)
                    {
                        m_bUseProxy = !(pProxyManager->IsExemptionHost((char*)(const char*)m_strHost));
                    }
#if defined(HELIX_FEATURE_PROXYMGR)
                    else
                    {
                        pProxyManager = new HXProxyManager();
                        pProxyManager->AddRef();

                        if (HXR_OK == pProxyManager->Initialize(m_pContext))
                        {
                            m_bUseProxy = !(pProxyManager->IsExemptionHost((char*)(const char*)m_strHost));
                        }
                    }
#endif /* #if defined(HELIX_FEATURE_PROXYMGR) */
                    HX_RELEASE(pProxyManager);
                }
            }
            HX_RELEASE(pProxyName);
            HX_RELEASE(pProxyPort);
        }
    }
    HX_RELEASE(pBuffer);

    theErr = _OpenFileExt();

cleanup:

    HX_VECTOR_DELETE(pOrigURL);

    return theErr;
}

HX_RESULT
CHTTPFileObject::_OpenFileExt()
{
    HX_RESULT   theErr = HXR_OK;

    CacheSupport_OpenFile();

    if (m_bCached)
    {
        m_bInitPending = FALSE;
    }
    else
    {
        /* connect to the host and start getting the data */

        theErr = BeginGet(m_uByteRangeSeekOffset);

        if (!theErr)
        {
            m_bInitPending = TRUE;
        }
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//      Method:
//
//              CHTTPFileObject::ProcessIdle()
//
//      Purpose:
//
//              Continues retrieval of an http URL asyncronously.
//
//      Parameters:
//
//              None.
//
//      Return:
//
//              HX_RESULT err
//              HXR_OK if things are continuing correctly.
//
//
HX_RESULT CHTTPFileObject::ProcessIdle()
{
    HXScopeLock lock(m_pMutex);

    HX_RESULT theErr = HXR_OK;
    HX_RESULT  lResult = HXR_OK;
    const UINT16 bufSize = 0xffff;


    HXLOGL4(HXLOG_HTTP, "ProcessIdle");

    // Did we timeout on a ConnectToAny() call?
    if (SUCCEEDED(theErr) && !m_bConnectDone && !m_bDisableConnectionTimeOut)
    {
        if (m_bOnServer)
        {
            ReportConnectionTimeout();
        }

        // connection failed, (ie we've hit a connect time-out)
        m_bConnTimedOut = TRUE;
        theErr = HXR_NET_CONNECT;
        if (m_pSocket)
        {
            HXLOGL2(HXLOG_HTTP, "Connect Timeout: closing socket");
            m_pSocket->Close();
            HX_RELEASE(m_pSocket);
        }
        
    }

    if (m_pSocket)
    {
        if (!m_bWriteDone)
        {
            HX_RESULT lResult = _WriteRequestChunk();
            if (FAILED(lResult)) theErr = lResult;
        }
        else if(m_nPostDataSize)
        {
            // This prompts client to write Post Data.
            if (m_pFileResponse)
            {
                HX_UNLOCK(m_pMutex);
                theErr = m_pFileResponse->WriteDone(HXR_OK);
                HX_LOCK(m_pMutex);
            }
        }
    }

    /* We only consider the server to be disconnected if we are yet in
     * the initialization state. We need to mask this error otherwise
     */
    if (!m_bInitPending && (lResult != HXR_OK))
    {
        SetReadContentsDone(TRUE);
        theErr = HXR_OK;
    }

    // If we have read enough to process are "seek".  We did a small
    // seek forward, so instead of doing a reconnect, we just wait
    // until we receive enough data from the socket.
    if (!theErr && m_bSeekPending && !m_bByteRangeSeekPending)
    {
        if (m_ulCurrentReadPosition <= m_nContentRead)
        {
            HXLOGL2(HXLOG_HTTP, "ProcessIdle notices SeekDone");

            m_bSeekPending  = FALSE;
            HX_UNLOCK(m_pMutex);
            m_pFileResponse->SeekDone(HXR_OK);
            HX_LOCK(m_pMutex);
        }
        else if (m_bReadContentsDone)
        {
            HXLOGL1(HXLOG_HTTP, "ProcessIdle notices SeekDone will fail");

            m_bSeekPending  = FALSE;
            HX_UNLOCK(m_pMutex);
            m_pFileResponse->SeekDone(HXR_FAILED);
            HX_LOCK(m_pMutex);
        }
    }


    _DoSomeReadingFromSocket(TRUE);

    HX_UNLOCK(m_pMutex);
    HX_RESULT ReadErr = ProcessPendingReads();
    HX_LOCK(m_pMutex);

    if(!theErr)
    {
        theErr = ReadErr;
    }

    // xxxbobclark it's possible -- especially if we have several
    // http file objects sharing a single chunky res (like if we're
    // accessing a surestream file) -- that we might stumble onto
    // parts of the file that we've already downloaded; by calling
    // _Ensure() here we can avoid redundant downloading.
    if (m_bSupportsByteRanges)
    {
        _EnsureThatWeAreReadingWisely();
    }

    /* Do we need to read more data */
    if (!theErr && !m_bReadContentsDone && m_pCallback && !m_pCallback->IsCallbackPending())
    {
        m_pCallback->ScheduleRelative(m_pScheduler, 50);
    }

    // Preserve previous errors, if any..
    if (FAILED(m_LastError))
    {
        theErr = m_LastError;
    }
    /* Report this error - crucial */
    else if (theErr)
    {
        /* This value is checked in every function */
        m_LastError = theErr;
    }

    if (m_bInitPending)
    {
        /* report InitDone if we have any error */
        if (theErr)
        {
            if (m_pPACInfoList && m_pPACInfoList->GetCount() && m_PACInfoPosition)
            {
                m_bConnTimedOut = FALSE;
                m_bUseProxy = FALSE;
                m_nProxyPort = 0;
                m_strProxyHost.Empty();

                m_bConvertFailedSeeksToLinear = TRUE;

                theErr = _ReOpen();
                m_LastError = theErr;
            }
            else
            {
                m_bInitPending = FALSE;
                m_bInitialized = TRUE;

                if (m_bInitResponsePending && m_pFileResponse)
                {
                            m_bInitResponsePending = FALSE;
                            m_pFileResponse->InitDone(HXR_FAILED);
                }

                if (m_bFileExistsResponsePending && m_pFileExistsResponse)
                {
                            m_bFileExistsResponsePending = FALSE;
                            m_pFileExistsResponse->DoesExistDone(FALSE);
                            m_pFileExistsResponse->Release();
                            m_pFileExistsResponse = 0;
                }

                if (m_bMimeResponsePending && m_pMimeMapperResponse)
                {
                            m_bMimeResponsePending = FALSE;
                            m_pMimeMapperResponse->MimeTypeFound(theErr, NULL);
                            // MimeTypeFound() may cause us to close if the Response object
                            // decides it doesn't like our mimetype and shuts us down.
                            // Therefore, m_pMimeMapperResponse could be NULL now..
                            HX_RELEASE(m_pMimeMapperResponse);
                }
            }
        }
        /* report InitDone if we have read some data, or
           if ReadContents is done. ContentRead=0 & ContentsDone=TRUE
           happens when the repsonse contains no data, only headers
         */
        else if ((m_nContentRead > 0 || m_bReadContentsDone) &&
                 m_bAuthorized)
        {
            m_bInitPending = FALSE;
            m_bInitialized = TRUE;

            // Determine mime type here!
            const char* pMimeType = NULL;
            if (!m_strMimeType.IsEmpty())
                pMimeType = m_strMimeType;

            if (m_bInitResponsePending && m_pFileResponse)
            {
                m_bInitResponsePending = FALSE;
                HX_UNLOCK(m_pMutex);
                m_pFileResponse->InitDone(m_bPartialData ? HXR_RESOURCE_PARTIALCOPY : HXR_OK);
                HX_LOCK(m_pMutex);
            }

            if (m_bFileExistsResponsePending && m_pFileExistsResponse)
            {
                AddNoCacheHeader();
                m_bFileExistsResponsePending = FALSE;
                HX_UNLOCK(m_pMutex);
                m_pFileExistsResponse->DoesExistDone(TRUE);
                HX_LOCK(m_pMutex);
                HX_RELEASE(m_pFileExistsResponse);
            }

            if (m_bMimeResponsePending && m_pMimeMapperResponse)
            {
                m_bMimeResponsePending = FALSE;
                HX_UNLOCK(m_pMutex);
                m_pMimeMapperResponse->MimeTypeFound(HXR_OK, pMimeType);
                HX_LOCK(m_pMutex);

                // MimeTypeFound() may cause us to close if the Response object
                // decides it doesn't like our mimetype and shuts us down.
                // Therefore, m_pMimeMapperResponse could be NULL now..
                HX_RELEASE(m_pMimeMapperResponse);
            }
        }
    }

    return theErr;
}


/////////////////////////////////////////////////////////////////////////////
//
//  Method:
//
//      CHTTPFileObject::::ProcessPendingReads()
//
//  Purpose:
//
//      See if there are any pending reads and process them if enough data is
//      available.
//
//  Parameters:
//
//      None.
//
//  Return:
//
//
//
HX_RESULT CHTTPFileObject::ProcessPendingReads(void)
{
    HXScopeLock lock(m_pMutex);

    IHXBuffer*             pBuffer             = NULL;
    HXBOOL                    bDone               = FALSE;
    ULONG32                 ulReadCount         = 0;
    HX_RESULT               lResult             = HXR_OK;


    HXLOGL4(HXLOG_HTTP, "Read: %p (socket %p) ProcessPendingReads pending buffers: %lu preprocessed %lu",
        this, m_pSocket, m_PendingReadList.GetCount(), m_PreProcessedReadBuffers.GetCount());


    /* process all pending read requests.. */
    while (!bDone && !m_PendingReadList.IsEmpty())
    {
        ulReadCount = (ULONG32)(PTR_INT)m_PendingReadList.GetHead();

        // If we have enough contiguous bytes we can satisfy this read request
        if (_GetContiguousLength() >= ulReadCount)
        {
            /* IT IS OK */
        }
        else if (m_bReadContentsDone)
        {
            /* Read the last few bytes */
            if (m_ulCurrentReadPosition < m_nContentRead)
            {
                ulReadCount = m_nContentRead - m_ulCurrentReadPosition;
            }
            else
            {
                /* Remove from pending list */
                m_PendingReadList.RemoveHead();

                // We're done (ie, we have read everything we can)
                // but we have pending reads. The user called too many reads, or the connection
                // has died before all data was sent.
                HXLOGL1(HXLOG_HTTP, "ProcessPendingReads Read() failure due to insufficient content");
                HX_UNLOCK(m_pMutex);
                CallReadDone(HXR_FAIL , NULL);
                HX_LOCK(m_pMutex);
                continue; // CallReadDone(HXR_FAIL..) for remaining reads in pending list
            }
        }
        else if (m_bKnowContentSize
            && ((m_ulCurrentReadPosition + ulReadCount) >= m_nContentSize)
            && (_GetContiguousLength()
            + m_ulCurrentReadPosition >= m_nContentSize))
        {
            // OK, so we've read past the end of the file but -- BUT! -- we may
            // be writing "behind ourselves" or something. The math is slightly
            // different from the m_bReadContentsDone case.
            ulReadCount = m_nContentSize - m_ulCurrentReadPosition;
        }
        else
        {
                // We have to wait for more data to come in the from socket
                // before we can process this Read request.
            HXLOGL4(HXLOG_HTTP, "Read: blocked, awaiting data");
            bDone = TRUE;
            break;
        }

        if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer))
        {
            // Ask the buffer to create a space to read into...
            if (HXR_OK == pBuffer->SetSize(ulReadCount))
            {
                // Read directly into the buffer
                ULONG32 ulActual = 0;

                HX_RESULT hxr = m_pChunkyRes->GetData(m_ulCurrentReadPosition,
                                    (char*) pBuffer->GetBuffer(),
                                    ulReadCount,
                                    &ulActual);

                HX_ASSERT(ulActual == ulReadCount);
                _SetCurrentReadPos(m_ulCurrentReadPosition + ulActual);

                /* Remove from pending list */
                m_PendingReadList.RemoveHead();

                HX_UNLOCK(m_pMutex);
                CallReadDone(HXR_OK ,pBuffer);
                HX_LOCK(m_pMutex);
            }
            else
            {
                lResult = HXR_OUTOFMEMORY;
                bDone = TRUE;
            }

            // Release our reference on the buffer!
            pBuffer->Release();
            pBuffer     = 0;
        }
        else
        {
            lResult = HXR_OUTOFMEMORY;
            bDone       = TRUE;
        }
    }

    // if the httpfsys is already in an err state, we'll return that error since all
    // read have been processed (so far).
    if (m_LastError)
    {
        lResult =  m_LastError;
    }

    return lResult;
}

//
// Create a resolver and response object. The response object serves as
// the response for both the socket and the resolver. The socket itself
// will be created when the next resolver request completes.
//
HX_RESULT CHTTPFileObject::_DoResolverSetup()
{
    LOG("_DoResolverSetup()");
  
    HX_ASSERT(m_pContext);
    
    HX_RESULT hr = HXR_OK;

    // Create the resolver
    if (!m_pResolve)
    {
        LOG("_DoResolverSetup(): creating resolver and response objects");
        IHXNetServices* pNetServices = NULL;
        hr = m_pContext->QueryInterface(IID_IHXNetServices,
                                                 (void**) &pNetServices);
        if (FAILED(hr))
        {
            return hr;
        }

  
        hr = pNetServices->CreateResolver(&m_pResolve);
        HX_RELEASE(pNetServices);
        if (FAILED(hr))
        {
            return hr;
        }
    
        // Create the response for the socket and resolver
        if(!m_pTCPResponse)
        {
            m_pTCPResponse = HTTPTCPResponse::CreateObject();
            if (m_pTCPResponse)
            {
                m_pTCPResponse->InitObject(this);
                m_pTCPResponse->AddRef();
            }
            else
            {
                return HXR_OUTOFMEMORY;
            }
        }

        // Configure the resolver with the response
        IHXResolveResponse* pResolveResponse = NULL;
        hr = m_pTCPResponse->QueryInterface(IID_IHXResolveResponse, (void**) &pResolveResponse);
        if (SUCCEEDED(hr))
        {
            hr = m_pResolve->Init(pResolveResponse);
        }
        HX_RELEASE(pResolveResponse);
    }

    return hr;
    
}


/////////////////////////////////////////////////////////////////////////////
//
//  Method:
//
//      CHTTPFileObject::::BeginGet()
//
//  Purpose:
//
//      Begins retrieval of an http URL.
//
//  Parameters:
//
//      start and end of desired range, if it supports http 1.1.
//      if the end is zero then get to the end of the file.
//      Both parameters default to zero.
//
//  Return:
//
//      HX_RESULT err
//      HXR_OK if the URL was good and setup for retrival completes
//      without error.
//
//
HX_RESULT CHTTPFileObject::BeginGet(ULONG32 ulOffsetStart)
{
//    HX_ASSERT(m_pSocket == NULL);

    HX_RESULT       theErr           = HXR_OK;
    CHXURL*         pHXURL           = NULL;
    IHXValues*      pURLProperties   = NULL;
    IHXBuffer*      pHost            = NULL;
    IHXBuffer*      pPath            = NULL;
    IHXBuffer*      pCookies         = NULL;
    IHXBuffer*      pPlayerCookies   = NULL;
    IHXBuffer*      pUserAgent       = NULL;
    IHXNetServices* pNetworkServices = NULL;
    UINT32          uValueLength     = 0;

    HXLOGL1(HXLOG_HTTP, "BeginGet(%lu)", ulOffsetStart);

    // Keep-Alive Header text
    //
    const char szKeepAlive[] = "\r\nConnection: Keep-Alive";

    const char szUserAgent[] = "\r\nUser-Agent: ";

    const char szHostHeader[] = "\r\nHost: ";

    const char szAcceptLang[] = "\r\nAccept-Language: ";

    const char szAcceptEncoding[] = "\r\nAccept-Encoding: gzip";

    const char* pActualHost = GetActualHost();
    const char* pResource = GetActualResource();
    ULONG32     ulPlatformData  = 0;
    IHXValues* pHeaders = 0;
    char*       pOutBuffer = 0;

    HXBOOL        bAlreadyHasAuthorization = FALSE;
    HXBOOL        bAlreadyHasProxyAuthorization = FALSE;
    HX_RESULT retVal = HXR_OK;
    IHXRegistry* pRegistry = NULL;

#if defined (_WINDOWS) || defined (_WIN32)
    ulPlatformData  = (ULONG32) GetModuleHandle(NULL);
#endif // defined (_WINDOWS) || defined (_WIN32)

    if (!pActualHost)
    {
        theErr = HXR_INVALID_URL_HOST;
        goto exit;
    }

    if (!*pActualHost)
    {
        theErr = HXR_INVALID_URL_HOST;
        goto exit;
    }

#if 0
    if (!pResource)
    {
        theErr = HXR_INVALID_URL_PATH;
        goto exit;
    }

    if (!*pResource)
    {
        theErr = HXR_INVALID_URL_PATH;
        goto exit;
    }
#endif

    m_nPostDataSize = 0;
    m_bPartialData  = FALSE;

    /*
     * Get the RFC822 headers from the IHXRequest object
     */

    if (m_pRequest &&
        m_pRequest->GetRequestHeaders(pHeaders) == HXR_OK)
    {
        if (pHeaders)
        {
            if (m_pCookies || m_pCookies2)
            {
                pHXURL = new CHXURL(m_pFilename, m_pContext);

                pURLProperties = pHXURL->GetProperties();
                if (pURLProperties)
                {
                    if (HXR_OK == pURLProperties->GetPropertyBuffer(PROPERTY_HOST, pHost) && pHost &&
                        HXR_OK == pURLProperties->GetPropertyBuffer(PROPERTY_PATH, pPath) && pPath)
                    {
                        HX_VECTOR_DELETE(m_pPath);
                        HX_VECTOR_DELETE(m_pHost);

                        StrAllocCopy(m_pHost, (char*)pHost->GetBuffer());
                        StrAllocCopy(m_pPath, (char*)pPath->GetBuffer());
                        HX_RESULT res = HXR_FAIL;
                        if(m_pCookies2)
                            res = m_pCookies2->GetCookies(m_pHost, m_pPath, pCookies, pPlayerCookies);
                        else if(m_pCookies)
                            res = m_pCookies->GetCookies(m_pHost, m_pPath, pCookies);

                        if (res == HXR_OK && pCookies)
                        {
                            pHeaders->SetPropertyCString("Cookie", pCookies);
                            if(pPlayerCookies)
                            {
                                pHeaders->SetPropertyCString("PlayerCookie", pPlayerCookies);
                            }
                        }
                        HX_RELEASE(pCookies);
                        HX_RELEASE(pPlayerCookies);
                    }
                    HX_RELEASE(pHost);
                    HX_RELEASE(pPath);
                }
                HX_RELEASE(pURLProperties);
                HX_DELETE(pHXURL);
            }

            IHXValues* pValuesRequestHeaders = NULL;	    
	    if (HXR_OK != CreateValuesCCF(pValuesRequestHeaders, m_pContext))
	    {
		goto exit;
	    }

            CHXHeader::mergeHeaders(pValuesRequestHeaders, pHeaders);

            if (m_pRequestHeadersOrig)
            {
                CHXHeader::mergeHeaders
                (
                    pValuesRequestHeaders,
                    m_pRequestHeadersOrig
                );
            }

            /*
             * First spin through the headers to see how much space they
             * require (the extra 4 bytes per header is for overhead)
             */

            const char* pName;
            IHXBuffer* pValue;
            HX_RESULT result;
            UINT32 uBufferSize = 0;
            UINT32 uBufferPtr = 0;

            result = pValuesRequestHeaders->GetFirstPropertyCString(pName, pValue);

            while (result == HXR_OK)
            {
                if (strcasecmp(pName, "User-Agent") &&
                    strcasecmp(pName, "Host") &&
                    (!m_bOnServer || strcasecmp(pName, "GUID")))
                {
                    uBufferSize += strlen(pName);
                    uValueLength = pValue->GetSize();
                    uBufferSize += (uValueLength > 0 ? (uValueLength - 1) : 0);
                    uBufferSize += 4;

                }

                if (!strcasecmp(pName, "Range"))
                {
                        m_bExternalByteRangeRequests = TRUE;
                        SetSupportsByteRanges(FALSE);
                        // xxxbobclark since someone externally is handling
                        // all the range mechanics, internally httpfsys will
                        // behave like it did in the olden days w.r.t.
                        // m_nContentSize and m_ulCurrentReadPosition.
                        // comlib's indhttp is who typically does this. They
                        // never use httpfsys's seek().
                }

                if (!strcasecmp(pName, "Content-length"))
                    m_nPostDataSize = atoi((const char*)pValue->GetBuffer());

                pValue->Release();
                result = pValuesRequestHeaders->GetNextPropertyCString(pName, pValue);
            }

            /*
             * Allocate space for trailing '\0'
             */

            uBufferSize++;

            pOutBuffer = new char[uBufferSize];

            /*
             * Now spin through and build the outgoing string
             */

            result = pValuesRequestHeaders->GetFirstPropertyCString(pName, pValue);

            while (result == HXR_OK)
            {
                if (strcasecmp(pName, "User-Agent") &&
                    strcasecmp(pName, "Host") &&
                    (!m_bOnServer || strcasecmp(pName, "GUID")))
                {
                    uValueLength = pValue->GetSize();
                    if (uValueLength > 0)
                    {
                        uValueLength--;
                    }

                    if (!strcasecmp(pName, "Authorization")
                            || !strcasecmp(pName, "Proxy-Authorization"))
                    {
                        HX_RESULT retVal = HXR_OK;
                        IHXRegistry* pRegistry = NULL;
                        retVal = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

                        if (SUCCEEDED(retVal))
                        {
                            IHXBuffer* pBuffer = NULL;
                            retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                            UINT32 regid = 0;

                            HX_ASSERT(SUCCEEDED(retVal));
                            if (SUCCEEDED(retVal))
                            {
                                IHXBuffer* pHeaderBuffer = NULL;

                                CHXString key;
                                CHXString recentRealmInfo = "";

                                if (!strcasecmp(pName, "Authorization"))
                                {
                                    bAlreadyHasAuthorization = TRUE;

                                    key = "authentication.http:";
                                    retVal = pRegistry->GetStrByName(WWW_AUTHENTICATION_RECENT_KEY,
                                                pHeaderBuffer);

                                    if (SUCCEEDED(retVal))
                                    {
                                        HX_ASSERT(pHeaderBuffer);
                                        recentRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
                                        HX_RELEASE(pHeaderBuffer);
                                    }

                                    key += m_strHost.IsEmpty() ? pActualHost : (const char*)m_strHost;
                                }

                                if (!strcasecmp(pName, "Proxy-Authorization"))
                                {
                                    bAlreadyHasProxyAuthorization = TRUE;

                                    key = "proxy-authentication.http:";
                                    retVal = pRegistry->GetStrByName(PROXY_AUTHENTICATION_RECENT_KEY,
                                                pHeaderBuffer);

                                    if (SUCCEEDED(retVal))
                                    {
                                        HX_ASSERT(pHeaderBuffer);
                                        recentRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
                                        HX_RELEASE(pHeaderBuffer);
                                    }

                                key += pActualHost;
                                }

                                key += ":";
                                key += recentRealmInfo;

                                HX_ASSERT(!key.IsEmpty());
                                pBuffer->Set(pValue->GetBuffer(), pValue->GetSize());

                                regid = pRegistry->GetId((const char*)key);
                                if (!regid)
                                {
                                    pRegistry->AddStr((const char*)key, pBuffer);
                                }
                                else
                                {
                                    pRegistry->SetStrByName((const char*)key, pBuffer);
                                }

                                HX_RELEASE(pBuffer);
                                HX_RELEASE(pHeaderBuffer);
                            }
                            HX_RELEASE(pRegistry);
                        }
                    }

                    memcpy(&pOutBuffer[uBufferPtr], "\r\n", 2); /* Flawfinder: ignore */
                    uBufferPtr += 2;
                    memcpy(&pOutBuffer[uBufferPtr], pName, strlen(pName)); /* Flawfinder: ignore */
                    uBufferPtr += strlen(pName);
                    memcpy(&pOutBuffer[uBufferPtr], ": ", 2); /* Flawfinder: ignore */
                    uBufferPtr += 2;
                    memcpy(&pOutBuffer[uBufferPtr], /* Flawfinder: ignore */
                           pValue->GetBuffer(),
                           uValueLength);
                    uBufferPtr += uValueLength;

                }
                pValue->Release();
                result = pValuesRequestHeaders->GetNextPropertyCString(pName, pValue);
            }

            HX_ASSERT(uBufferPtr == uBufferSize - 1);

            pOutBuffer[uBufferPtr] = '\0';

            HX_RELEASE(pValuesRequestHeaders);

            HX_RELEASE(pHeaders);
        }
    }

    // The chttp: label must appear as a standard http: label to outsiders
    if (pResource && !strncasecmp (pResource, "chttp:", 6))
        pResource++;

    // The request is a standard HTTP based request created from the resource...
    // since string.Format() has a max length of 512 chars, we'll build up the string manually
    if(m_nPostDataSize)
        m_strRequest = "POST ";
    else
        m_strRequest = "GET ";

    m_strRequest += (*pResource ? pResource : "/");
    if (m_bDisableByteRanges)
    {
        m_strRequest += " HTTP/1.0\r\nAccept: */*";
    }
    else
    {
        m_strRequest += " HTTP/1.1\r\nAccept: */*";
    }

    m_strRequest += szUserAgent;
    GetUserAgent(pUserAgent);
    m_strRequest += (const char*)pUserAgent->GetBuffer();
    HX_RELEASE(pUserAgent);

    m_strRequest += "\r\nIcy-MetaData: 1";

    m_strRequest += (pOutBuffer ? pOutBuffer : "");
    m_strRequest += szKeepAlive;

    if(!m_strHost.IsEmpty())
    {
        m_strRequest += szHostHeader;
        // Use actual host, even when going through proxy..
        m_strRequest += m_strHost;
        if(m_nPort != 80)
        {
            m_strRequest += ":";
            m_strRequest.AppendULONG(m_nPort);
        }
    }

    if (m_pLanguage)
    {
        m_strRequest += szAcceptLang;
        m_strRequest += m_pLanguage->GetBuffer();
    }

    // Tell the server that we support encoded content
#if defined(HELIX_FEATURE_HTTP_GZIP)
    m_strRequest += szAcceptEncoding;
#endif

    if (m_bByteRangeSeekPending)
    {
        HX_ASSERT(m_bSupportsByteRanges);

        m_strRequest += "\r\nRange:bytes=";
        m_strRequest.AppendULONG(ulOffsetStart);
        m_strRequest += "-";

        // XXXbobclark when we get smarter we can remember
        // the end of the range we're getting, and simply
        // do m_strRequest.AppendULONG(ulOffsetEnd); but
        // for now it'll just be starting at ulOffsetStart
        // and reading until it hits the end.
    }

    retVal = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (SUCCEEDED(retVal))
    {
        IHXBuffer* pBuffer = NULL;

        CHXString key("no-authentication-information");

        CHXString recentAuthRealmInfo;
        CHXString recentProxyAuthRealmInfo;

        IHXBuffer* pHeaderBuffer = NULL;

        retVal = pRegistry->GetStrByName(WWW_AUTHENTICATION_RECENT_KEY, pHeaderBuffer);
        if (SUCCEEDED(retVal))
        {
            HX_ASSERT(pHeaderBuffer);
            recentAuthRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
            HX_RELEASE(pHeaderBuffer);
        }

        retVal = pRegistry->GetStrByName(PROXY_AUTHENTICATION_RECENT_KEY, pHeaderBuffer);
        if (SUCCEEDED(retVal))
        {
            HX_ASSERT(pHeaderBuffer);
            recentProxyAuthRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
            HX_RELEASE(pHeaderBuffer);
        }

        if (!bAlreadyHasAuthorization)
        {
            key = "authentication.http:";
            key += m_strHost.IsEmpty() ? pActualHost : (const char*)m_strHost;
            key += ":";
            key += recentAuthRealmInfo;

            if (HXR_OK == pRegistry->GetStrByName((const char*)key, pBuffer) )
            {
                if (pBuffer && 0 != strncasecmp((const char*)pBuffer->GetBuffer(), "NTLM", 4))
                {
                    CHXString authString((const char*)pBuffer->GetBuffer());

                    m_strRequest += "\r\nAuthorization: ";
                    m_strRequest += (const char*)authString;
                }
                HX_RELEASE(pBuffer);
            }
        }
        if (!bAlreadyHasProxyAuthorization)
        {
            key = "proxy-authentication.http:";
            key += pActualHost;
            key += ":";
            key += recentProxyAuthRealmInfo;

            if (HXR_OK == pRegistry->GetStrByName((const char*)key, pBuffer) )
            {
                if (pBuffer && 0 != strncasecmp((const char*)pBuffer->GetBuffer(), "NTLM", 4))
                {
                    CHXString authString((const char*)pBuffer->GetBuffer());

                    m_strRequest += "\r\nProxy-Authorization: ";
                    m_strRequest += (const char*)authString;
                }
                HX_RELEASE(pBuffer);
            }
        }

        HX_RELEASE(pRegistry);
    }


    m_strRequest += "\r\n\r\n";

    HX_VECTOR_DELETE(pOutBuffer);

    if (!m_pSocket)
    {
        // Ensure resolver and response objects are created and initialized
        theErr = _DoResolverSetup();
        if (SUCCEEDED(theErr))
        {
            // Put the port in string form
            UINT16 usTmpPort = GetActualPort();
            char szPort[HX_PORTSTRLEN]; /* Flawfinder: ignore */
            sprintf(szPort, "%u", usTmpPort);
            // Call IHXResolve::GetAddrInfo() to resolve the host name
            theErr = m_pResolve->GetAddrInfo(pActualHost, szPort, NULL);
            if (SUCCEEDED(theErr))
            {
                // Set a timeout callback for resolving the host name
                if(!m_bConnectDone)
                {
                    // If there is a pending callback, be sure to remove it!
                    // (there might be the connection timeout callback pending)
                    if (m_pCallback && m_pCallback->IsCallbackPending())
                    {
                        m_pCallback->Cancel(m_pScheduler);
                    }
                }
                // Ask the scheduler for callback to catch connection
                // timeout (in ProcessIdle)
                if (m_pCallback && !m_pCallback->IsCallbackPending())
                {
                    m_pCallback->ScheduleRelative(m_pScheduler, m_nConnTimeout);
                }
            }
        }

    }
    else
    {
        HX_ASSERT(m_pResolve);
        // Ask the scheduler to call ProcessIdle()
        // as m_pResolve!=NULL
        if (m_pCallback && !m_pCallback->IsCallbackPending())
        {
            m_pCallback->ScheduleRelative(m_pScheduler, 0);
        }
    }

    m_nTotalRequestSize = (UINT16)m_strRequest.GetLength();

    HXLOGL2(HXLOG_HTTP, "BeginGet: request size is %lu",m_nTotalRequestSize);

exit:

    return theErr;
}


const char*
CHTTPFileObject::GetActualHost()    const
{
    return !m_bUseProxy ? (const char*)m_strHost : (const char*)m_strProxyHost;
}

const char*
CHTTPFileObject::GetActualResource()    const
{
    return !m_bUseProxy ? (const char*)m_strResource : (const char*)m_strProxyResource;
}

HX_RESULT
CHTTPFileObject::GetUserAgent(REF(IHXBuffer*) pBuffer)
{
    // If we received UserAgent as a parameter to the request, use that
    if (!m_pParams ||
        FAILED(m_pParams->GetPropertyBuffer("Agent", pBuffer)))
    {
        // If not, see if we received it as a file system option
        if (!m_pOptions ||
            FAILED(m_pOptions->GetPropertyBuffer("Agent", pBuffer)))
        {
            // Finally, fall back to the default User Agent value
	    CreateAndSetBufferCCF(pBuffer, (UCHAR*)DEF_USER_AGENT, 
				  strlen(DEF_USER_AGENT) + 1, m_pContext);
        }
    }

    return HXR_OK;
}

int
CHTTPFileObject::GetActualPort()    const
{
    return !m_bUseProxy ? m_nPort : m_nProxyPort;
}

STDMETHODIMP CHTTPFileObject::GetAddrInfoDone(HX_RESULT status, 
                                              UINT32 nVecLen, 
                                              IHXSockAddr** ppAddrVec)
{
    HXLOGL1(HXLOG_HTTP, "GetAddrInfoDone status %08x (going to create socket)", status);
    HX_RESULT retVal = HXR_FAIL;

    if (ppAddrVec && nVecLen && m_pContext && m_pTCPResponse)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Make sure we haven't already timed out
        if (!m_bConnTimedOut)
        {
            // Did the resolve succeed?
            if (SUCCEEDED(status))
            {
                // Get the IHXNetServices interface
                IHXNetServices* pNetServices = NULL;
                retVal = m_pContext->QueryInterface(IID_IHXNetServices, (void **) &pNetServices);
                if (SUCCEEDED(retVal))
                {
                    // Close any existing socket, just in case
                    if (m_pSocket)
                    {
                        m_pSocket->Close();
                        HX_RELEASE(m_pSocket);
                    }
                    
                    // if there are any preprocessed buffers hanging around, they will
                    // confusingly appear to be coming from this brand new socket unless
                    // we clear them out first.
                    _ClearPreprocessedBuffers();

                    retVal = pNetServices->CreateSocket(&m_pSocket);
                    if (SUCCEEDED(retVal))
                    {
#if defined(SUPPORT_SECURE_SOCKETS)
                        if (m_bUseHTTPS)
                        {
                            IHXTCPSecureSocket* pSecureSocket = NULL;
                            m_pSocket->QueryInterface(IID_IHXTCPSecureSocket, (void**) &pSecureSocket);
                            if (pSecureSocket)
                            {
                                pSecureSocket->SetSecure(TRUE);
                                m_nPort = 443;
                            }
                            HX_RELEASE(pSecureSocket);
                        }
#endif /* #if defined(SUPPORT_SECURE_SOCKETS) */
                        // Get the response object's IHXSocketResponse interface
                        IHXSocketResponse* pSocketResponse = NULL;
                        retVal = m_pTCPResponse->QueryInterface(IID_IHXSocketResponse, (void**) &pSocketResponse);
                        if (SUCCEEDED(retVal))
                        {
                            // Set the response interface
                            retVal = m_pSocket->SetResponse(pSocketResponse);
                            if (SUCCEEDED(retVal))
                            {

                                HX_ASSERT(nVecLen > 0);

                                retVal = m_pSocket->Init(HX_SOCK_FAMILY_INANY,
                                                         HX_SOCK_TYPE_TCP,
                                                         HX_SOCK_PROTO_ANY);

                                if (SUCCEEDED(retVal))
                                {
                                    m_pSocket->SelectEvents(HX_SOCK_EVENT_READ    |
                                                            HX_SOCK_EVENT_WRITE   |
                                                            HX_SOCK_EVENT_CONNECT |
                                                            HX_SOCK_EVENT_CLOSE);

                                    // Copy these (convert/filter to proper family supported by socket)
                                    IHXSockAddr** ppConvSockAddr = NULL;
                                    retVal = HXSockUtil::AllocAddrVec(ppAddrVec,
                                                                      nVecLen,
                                                                      ppConvSockAddr,
                                                                      m_pSocket->GetFamily(),
                                                                      true,
                                                                      pNetServices);
                                    if (SUCCEEDED(retVal))
                                    {
                                        // Now call ConnectToAny
                                        m_bSocketReadTimeout = FALSE; 
                                        retVal = m_pSocket->ConnectToAny(nVecLen, ppConvSockAddr);
                                    }
                                    // Free the addresses we converted
                                    HXSockUtil::FreeAddrVec(ppConvSockAddr, nVecLen);                            
                                }
                            }
                        }
                        HX_RELEASE(pSocketResponse);
                    }
                }
                HX_RELEASE(pNetServices);
            }
            else
            {
                // Failed connection
                m_LastError = HXR_INVALID_HOST;
                if( m_pResolve )
                {
                    m_pResolve->Close();
                    HX_RELEASE(m_pResolve);
                }
                
                if (m_bOnServer)
                {
                    ReportConnectionFailure();
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHTTPFileObject::GetNameInfoDone(HX_RESULT status, const char* pszNode, const char* pszService)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP CHTTPFileObject::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HXScopeLock lock(m_pMutex);
    HX_RESULT retVal = HXR_OK;

    HXLOGL4(HXLOG_HTTP, "Network: EventPending(%lu status %08x)", uEvent, status);

    switch (uEvent)
    {
        case HX_SOCK_EVENT_READ:
            {
                // if a Read() call is currently
                // blocked, we'll handle this incoming
                // data with a little more urgency.
                
                HXBOOL bHandleReadImmediately = FALSE;
                if (!m_PendingReadList.IsEmpty())
                {
                    bHandleReadImmediately = TRUE;
                }
                
                _DoSomeReadingFromSocket(bHandleReadImmediately);
                
                if (bHandleReadImmediately)
                {
                    HX_UNLOCK(m_pMutex);
                    ProcessPendingReads();
                    HX_LOCK(m_pMutex);
                }
            }
            break;
        case HX_SOCK_EVENT_WRITE:
            {
                // Are we finished writing?
                if (!m_bWriteDone)
                {
                    // Schedule a callback to write
                    if (m_pCallback && !m_pCallback->IsCallbackPending())
                    {
                        m_pCallback->ScheduleRelative(m_pScheduler, 0);
                    }
                }
            }
            break;
        case HX_SOCK_EVENT_CONNECT:
            {
                if (m_pSocket)
                {
                    if (SUCCEEDED(status))
                    {
                        HXLOGL1(HXLOG_HTTP, "EventPending connect done");

                        // Successful connection
                        //
                        // Make sure we didn't time out
                        if (!m_bConnTimedOut)
                        {
                            // Set the flag saying we connected
                            m_bConnectDone = TRUE;
                            if (m_pCallback)
                            {
                                // Remove the connection time-out callback
                                if (m_pCallback->IsCallbackPending())
                                {
                                    m_pCallback->Cancel(m_pScheduler);
                                }

                                // Add an immediate callback
                                m_pCallback->ScheduleRelative(m_pScheduler, 0);
                            }

                            _WriteRequestChunk();
                        }
                        
                        // Determine whether we need to set the
                        // m_bCheckingWhetherByteRangeWorks flag
                        if (m_bSupportsByteRanges)
                        {
                            // xxxbobclark OK, it's true that we're "lying" by saying
                            // that stuff's been read that really we've merely seeked past.
                            // But for now there are other places that will get confused if
                            // we don't fake it out like this.
                            m_nContentRead = m_uByteRangeSeekOffset;

                            // If we are attempting a byte offset seek,
                            // check the server for byte seek support
                            if (m_bByteRangeSeekPending && m_uByteRangeSeekOffset)
                            {
                                m_bCheckingWhetherByteRangeWorks = TRUE;
                            }
                        }
                    }
                    else
                    {
                        // Failed connection
                        m_LastError = status;
                        if (m_bOnServer)
                        {
                            ReportConnectionFailure();
                        }
                    }
                }
            }
            break;
        case HX_SOCK_EVENT_CLOSE:
            {
                if (m_pSocket)
                {
                    HXLOGL2(HXLOG_HTTP, "EventPending: closing socket (%08x)", status);
                    m_pSocket->Close();
                    HX_RELEASE(m_pSocket);
                }
            }
            break;
    }

    return retVal;
}

//
//
//
//
//
//
HX_RESULT
CHTTPFileObject::_HandleRedirect(HTTPResponseMessage* pMessage)
{
    HX_RESULT   theErr = HXR_OK;
    CHXString   sLocation;
    IHXValues* pReqHeaders = NULL;

    HXLOGL2(HXLOG_HTTP, "_HandleRedirect");

    if (m_nRedirectLevel++ >= REDIRECT_LIMIT)
    {
        return HXR_FAIL;
    }

    sLocation = pMessage->getHeaderValue("location");

    // XXX HP according to RFC on HTTP headers - "location" allows only absolute URL:
    //        http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
    //
    //        it makes support of HTTP redirect on different protocols tricky if we
    //        allow relative URL
    if (strncasecmp((const char*)sLocation, "http://", 7) != 0)
    {
        if (m_bOnServer)
        {
            return HXR_UNEXPECTED;
        }

        HXURLRep urlToBeRedirectedTo(sLocation);

        if (urlToBeRedirectedTo.GetType() == HXURLRep::TYPE_RELPATH)
        {
            // redirect to relative url not allowed
            return HXR_UNEXPECTED;
        }

        // redirects to other schemes (such as rtsp) are allowed
    }

    if (m_pRedirectResponse)
    {
        HXBOOL bContinueRedirect = FALSE;
        if (sLocation.IsEmpty())
        {
            if (HXR_NOTIMPL == m_pRedirectResponse->RedirectDone(NULL))
                bContinueRedirect = TRUE;
        }
        else
        {
            IHXBuffer* pRedirectURL = NULL;
	    
	    CreateAndSetBufferCCF(pRedirectURL, (UCHAR*)(const char*)sLocation,
				  sLocation.GetLength()+1, m_pContext);

            if (HXR_NOTIMPL == m_pRedirectResponse->RedirectDone(pRedirectURL))
                bContinueRedirect = TRUE;

            HX_RELEASE(pRedirectURL);
        }

        if (!bContinueRedirect)
        {
            HX_RELEASE(m_pRedirectResponse);
            return theErr;
        }
    }

    if(!sLocation.IsEmpty())
    {
        if (m_pCacheEntry)
        {
            // If an HTTP redirection is done, maintain the chttp
            // protocol label for the new URL.
            if (sLocation.Find ("http://") == 0)
            {
                sLocation.FindAndReplace ("http://", "chttp://");
            }
            if (sLocation.Find ("HTTP://") == 0)
            {
                sLocation.FindAndReplace ("HTTP://", "chttp://");
            }
        }
        HXLOGL2(HXLOG_HTTP, "Redirected to %s", NULLOK((const char*)sLocation));

        if(m_pRequest)
        {
            m_pRequest->SetURL(sLocation);

            // Keep the request alive..
            m_pRequest->AddRef();

            // Find out if the User-Agent was explicitly set last time
            IHXBuffer* pBuffer = NULL;
            if (m_pParams)
            {
                m_pParams->GetPropertyBuffer("Agent", pBuffer);
            }

            // Reset this object to new request..
            SetRequest(m_pRequest);

            // Reuse the User-Agent from the last request
            if (m_pParams && pBuffer)
            {
                m_pParams->SetPropertyBuffer("Agent", pBuffer);
            }
            HX_RELEASE(pBuffer);

            if (m_bMangleCookies)
            {
                // Reset the Cookie header to its original value
                m_pRequest->GetRequestHeaders(pReqHeaders);
                if (pReqHeaders && m_pMangledCookies)
                {
                    pReqHeaders->SetPropertyCString("Cookie", m_pMangledCookies);
                }
                HX_RELEASE(pReqHeaders);

                // Unmangle all incoming cookies and keep only those that apply
                // to the new request URL
                UnmangleAllCookies(m_pRequest);
            }

            // Release the "keep alive" ref..
            m_pRequest->Release();

            // Cleanup for reopen..
            //

            // Redirect is absolute..
            delete [] m_szBaseURL;
            m_szBaseURL = NULL;

            // Need new connection
            if (m_pSocket)
            {
                m_pSocket->Close();
                HX_RELEASE(m_pSocket);
            }

            m_bHTTP1_1 = TRUE;
            m_bConnectDone = FALSE;

            m_bCached = FALSE;

            m_bConvertFailedSeeksToLinear = TRUE;

            theErr = _ReOpen();
        }
    }

    ProcessCacheCompletions(TRUE);
    return theErr;
}

HX_RESULT CHTTPFileObject::_EnsureThatWeAreReadingWisely()
{
    HX_ASSERT( m_bSupportsByteRanges );

    // if the range from m_ulCurrentReadPosition to m_nContentRead
    // is valid, that's extraordinarily cool because that means
    // that it's correctly reading into the range that it's
    // currently playing. On the other hand, if there's a gap
    // between this valid region and (the valid region) where
    // it's currently reading, then we should stop reading
    // the current place and instead start reading where it
    // stops being valid.

    if (!m_bSeekPending && !m_bByteRangeSeekPending && m_pChunkyRes)
    {
        if (m_nContentRead >= m_ulCurrentReadPosition)
        {
            ULONG32 ulLength = _GetContiguousLength();

            if (ulLength < m_nContentRead - m_ulCurrentReadPosition)
            {
                // That means it's reading way down the line, i.e. a foolish
                // place.

                HXLOGL2(HXLOG_HTTP, "need byte range seek (was reading at %lu)", m_nContentRead);
                _HandleByteRangeSeek(m_ulCurrentReadPosition + ulLength);

                // xxxbobclark here is where it would conceivably be
                // convenient to be able to tell the length of the
                // invalid range that starts at m_ulCurrentReadPosition
                // + ulLength. Then we could seek to the end of that
                // invalid range and save a packet or two of overwriting
                // because currently it will keep reading until it
                // detects that it's not reading as wisely as possible and
                // starts reading at a wiser place.
            }
            else if (ulLength > m_nContentRead - m_ulCurrentReadPosition && m_bReadHeaderDone)
            {
                // This means that it's currently reading into an
                // already-valid place. Maybe we seeked into the file,
                // then seeked backwards, and has now finished reading
                // up to the already-valid place. So we need to
                // kick-start reading way down the line.

                // xxxbobclark now here's the problem: we can easily end up with two separate
                // http file objs writing into the same chunky res here. If they're writing into
                // the same range we'll end up with "leapfrogging" cursors, where each iteration
                // will do a whole nuther byte range seek to the last set byte that the other
                // guy just wrote. (This is kind of fun to watch in action, but less than
                // optimal in the real world.) What I'd rather do instead is see if another obj
                // is actively writing into this range and if so I'll just stop reading.

                HXBOOL bSomebodyElseOwnsThisRange = FALSE;
                {
                    int max = m_pChunkyRes->CountCursors();
                    int ndx;
                    for (ndx = 0; ndx < max; ndx++)
                    {
                        void* pOwner;
                        ULONG32 ulCursorLocation;
                        m_pChunkyRes->GetNthCursorInformation(ndx, pOwner, ulCursorLocation);

                        if (ulCursorLocation == m_ulCurrentReadPosition + ulLength)
                        {
                            if (pOwner != this)
                            {
                                bSomebodyElseOwnsThisRange = TRUE;
                            }
                        }
                    }
                }

                HXBOOL bTheRestHasBeenDownloaded = (m_bKnowContentSize &&
                    ((m_ulCurrentReadPosition + ulLength) >= m_nContentSize));

                HXLOGL3(HXLOG_HTTP, "%p _Ensure: reading into already-valid place. socket %p valid %lu-%lu read to %lu/%lu",
                    this, m_pSocket, m_ulCurrentReadPosition, m_ulCurrentReadPosition+ulLength, m_nContentRead, m_nContentSize);

                if (bSomebodyElseOwnsThisRange || bTheRestHasBeenDownloaded)
                {
                    // If there is a pending callback, be sure to remove it!
                    // (there might be the connection timeout callback pending)
                    if (m_pCallback && m_pCallback->IsCallbackPending())
                    {
                        m_pCallback->Cancel(m_pScheduler);
                    }

                    // note that this release might not break the connection; if
                    // we're between the pSocket->AddRef() and pSocket->Release()
                    // in ProcessIdle. In that case the connection will be broken
                    // by the pSocket->Release() in ProcessIdle.
                    // Need new connection
                    if (m_pSocket)
                    {
                        m_pSocket->Close();
                        HX_RELEASE(m_pSocket);
                    }

                    // add in a callback to ensure that we stay alive for reading.

                    if (m_pCallback && !m_pCallback->IsCallbackPending())
                    {
                        m_pCallback->ScheduleRelative(m_pScheduler, 0);
                    }

                    // although we're going to let the other owner of this range do the
                    // reading, we should still keep the content read up-to-date. Other
                    // places in httpfsys rely on this being updated.

                    UINT32 ulPotentialContentRead = m_ulCurrentReadPosition + ulLength;
                    if (ulPotentialContentRead > m_nContentRead)
                    {
                        HXLOGL3(HXLOG_HTTP, "obj %p _Ensure forcibly setting m_nContent from %lu to %lu",
                            this, m_nContentRead, ulPotentialContentRead);
                        m_nContentRead = ulPotentialContentRead;
                    }

                    if (m_bKnowContentSize && (m_nContentRead > m_nContentSize))
                    {
                        HX_ASSERT("content read should never be greater than content size" == NULL);
                        m_nContentRead = m_nContentSize;
                    }
                }
                else
                {
                    HXLOGL2(HXLOG_HTTP,
                        "need byte range seek (was reading already-valid content at %lu)",
                        m_nContentRead);
                    _HandleByteRangeSeek(m_ulCurrentReadPosition + ulLength);
                }
            }
        }
        else
        {
            // XXXbobclark OK, it's plausible that something could be
            // implemented that, if a Seek jumped over part of the file,
            // then we finished reading from that place to the end of
            // the file, would "fill in" non-valid parts of the file.
            // If that's the case and the reason we're here, then
            // we should ensure that we've actually filled in the
            // whole file from m_ulCurrentReadPosition to the length
            // of the file. If not, then kick-start another byte range
            // seek.

            // This is the number of contiguous bytes we have
            ULONG32 ulLength = _GetContiguousLength();
            HXBOOL bAtTheEnd = (m_bKnowContentSize && ((m_ulCurrentReadPosition + ulLength) == m_nContentSize));
            if (!bAtTheEnd)
            {
                HXLOGL2(HXLOG_HTTP, "need byte range seek (backwards seek happened)");
                _HandleByteRangeSeek(m_ulCurrentReadPosition + ulLength);
            }

        //HX_ASSERT(!"Reading BEHIND the current seek position");
        }
    }

    return HXR_OK;
}

//
// _HandleByteRangeSeek is called when we're connected to an HTTP 1.1
// server that supports grabbing a range of bytes, and Seek has
// been called to an invalid place of the file.
HX_RESULT
CHTTPFileObject::_HandleByteRangeSeek(ULONG32 ulSeekLocation)
{
    HX_RESULT   theErr = HXR_OK;

    HXLOGL1(HXLOG_HTTP, "_HandleByteRangeSeek(%lu)", ulSeekLocation);
    if (m_bKnowContentSize)
    {
        // Only issue a reconnect if there are empty bytes between
        // the current read location and the end of the file.
        if (ulSeekLocation >= m_nContentSize)
            return HXR_FAIL;
    }

    // It should never reach this function unless it's an
    // HTTP 1.1 server that supports byte ranges.

    HX_ASSERT(m_bSupportsByteRanges);
    HX_ASSERT(!m_bSeekPending);
    HX_ASSERT(!m_bByteRangeSeekPending);

    m_uByteRangeSeekOffset = ulSeekLocation;
    m_bByteRangeSeekPending = TRUE;

    delete [] m_szBaseURL;
    m_szBaseURL = NULL;

    // Need new connection

    // XXXbobclark I'm thinking of this kind of like a redirect,
    // only instead of redirecting to a whole nuther URL it seeks
    // within this file.

    // If there is a pending callback, be sure to remove it!
    // (there might be the connection timeout callback pending)
    if (m_pCallback && m_pCallback->IsCallbackPending())
    {
        m_pCallback->Cancel(m_pScheduler);
    }

    if (m_pSocket)
    {
        m_pSocket->Close();
        HX_RELEASE(m_pSocket);
    }
    
    if( m_pResolve )
    {
        m_pResolve->Close();
        HX_RELEASE(m_pResolve);
    }

    m_bHTTP1_1 = TRUE;
    m_bConnectDone = FALSE;

    m_bCached = FALSE;

    _ReOpen();

    return theErr;
}


//
// _WriteRequestChunk, if necessary,
// writes the next bit of the request
// to the socket. Usually this only
// takes one call but longer requests
// can take multiple calls. This
// function does nothing if the request
// has been completely written.
HX_RESULT
CHTTPFileObject::_WriteRequestChunk()
{
    HX_RESULT lResult = HXR_OK;
    if (!m_bWriteDone)
    {
        HXLOGL1(HXLOG_HTTP, "Write: _WriteRequestChunk");
        UINT32     ulActual = m_nTotalRequestSize - m_nRequestWritten;
        IHXBuffer* pBuffer  = NULL;
        lResult = CreateBufferCCF(pBuffer, m_pContext);
        if (SUCCEEDED(lResult))
        {
            lResult = pBuffer->Set((UCHAR*) (const char*) m_strRequest.Mid(m_nRequestWritten),
                                   ulActual);
            if (SUCCEEDED(lResult))
            {
                HXLOGL2(HXLOG_HTTP, "write request chunk:\n%s\n", (const char*)m_strRequest.Mid(m_nRequestWritten));
                lResult = m_pSocket->Write(pBuffer);
                if (SUCCEEDED(lResult))
                {
                    m_nRequestWritten += (INT16)ulActual;
                    if (m_nRequestWritten >= m_nTotalRequestSize)
                    {
                        m_bWriteDone = TRUE;
                    }
                }
            }
        }
        HX_RELEASE(pBuffer);
    }    
    return lResult;
}



HX_RESULT
CHTTPFileObject::_DoSomeReadingFromSocket(HXBOOL bHandleBuffersImmediately)
{
    HXScopeLock lock(m_pMutex);

    HX_RESULT retVal = HXR_OK;

    UINT32 ulPendingReadAmount = 0;

    HXBOOL bShouldReadSocket = FALSE;

    CHXSimpleList::Iterator readIter = m_PendingReadList.Begin();
    for (; readIter != m_PendingReadList.End(); ++readIter)
    {
        UINT32 ulReadCount = (ULONG32)(PTR_INT)(*readIter);
        ulPendingReadAmount += ulReadCount;
    }

    UINT32 ulPreProcessedAmount = 0;
    CHXSimpleList::Iterator preprocessedIter = m_PreProcessedReadBuffers.Begin();
    for (; preprocessedIter != m_PreProcessedReadBuffers.End(); ++ preprocessedIter)
    {
        IHXBuffer* pBuf = (IHXBuffer*)(*preprocessedIter);
        HX_ASSERT(pBuf);

        ulPreProcessedAmount += pBuf->GetSize();
    }

    // if we have finished reading from the socket but still have
    // pre-processed content, we must process that content.
    if (!m_pSocket && bHandleBuffersImmediately && ulPreProcessedAmount > 0)
    {
        while (!m_PreProcessedReadBuffers.IsEmpty())
        {
            IHXBuffer* pBuf = (IHXBuffer*)m_PreProcessedReadBuffers.RemoveHead();
            HX_ASSERT(pBuf);

            HandleSocketRead(HXR_OK, pBuf);
            HX_RELEASE(pBuf);
        }
    }

    if (!m_pSocket)
    {
        return HXR_SOCK_ENDSTREAM; // already finished reading all data from socket
    }

    bShouldReadSocket = FALSE;

    if (bHandleBuffersImmediately && ulPreProcessedAmount > 0)
    {
        // as we're being called from Process Idle, we'll
        // go ahead and process accumulated preprocessed content.
        bShouldReadSocket = TRUE;
    }

    if (m_ulBufferAheadAmount > 0)
    {
        // if m_ulBufferAheadAmount is nonzero, that will tell how
        // far ahead we need to read. This is useful for lower-end
        // machines where simply reading and processing at a much
        // higher bandwidth than we need can starve the CPU.

        if ((m_ulCurrentReadPosition + ulPendingReadAmount + m_ulBufferAheadAmount)
            > (m_nContentRead + ulPendingReadAmount))
        {
            // our current read position is creeping close to what
            // we've downloaded, so we'll download more.
            bShouldReadSocket = TRUE;
        }
    }
    else
    {
        // if m_ulBufferAheadAmount is zero, read and keep on reading
        // as quickly as possible.
        bShouldReadSocket = TRUE;
    }

    if (bShouldReadSocket && !bHandleBuffersImmediately)
    {
        // if it's NOT processidle and we're starting to accrue too
        // much preprocessed content, we may want to bail out now so we
        // don't starve the ProcessIdle-calling timer

        if (ulPendingReadAmount > 0)
        {
            // if we're actively waiting for a read to be processed,
            // we'll bail out immediately to give ProcessIdle time
            // as soon as possible.
            bShouldReadSocket = FALSE;
        }

        if (ulPreProcessedAmount > HTTP_MAX_BUFFER_BEFORE_PROCESSIDLE)
        {
            // if we have been accruing data for awhile without
            // an opportunity for ProcessIdle to process the
            // content, we'll interrupt ourselves to ensure that
            // we don't starve the ProcessIdle timer.
            bShouldReadSocket = FALSE;
        }
    }


    HXLOGL4(HXLOG_HTTP, "Network: _DoSomeReadingFromSocket %s",
        bShouldReadSocket?"WILL try socket read":"will NOT try socket read");

    if (bShouldReadSocket)
    {
        IHXBuffer* pReadBuffer = NULL;

#ifdef HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT

        ULONG32 ulMemUsage = m_pChunkyRes->GetCurrentMemoryUsage();
        
        // If reading from the socket is temporarily halted, to exhaust data from
        // the chunky-resource, check to see whether the memory usage of chunky-resource
        // has approached the lower memory limit - so that reading from the socket can be 
        // resumed.

        if(ulMemUsage < CHUNK_RES_MEM_FLOOR && m_bHaltSocketReadTemporarily)
        {
            m_bHaltSocketReadTemporarily = FALSE;
        }
                
        // Check whether the chunky-resource has approached the memory ceiling. If so, stop 
        // reading from the socket, until the data in chunky-resource is exhausted to reach
        // the lower limit.

        if(ulMemUsage < CHUNK_RES_MEM_CEILING && !m_bHaltSocketReadTemporarily)
        {
            retVal = m_pSocket->Read(&pReadBuffer);
            m_bHaltSocketReadTemporarily = FALSE;
        }
        else
        {
            // setting the return value to "WOULD BLOCK", so that its not treated as an error
            // few lines down below.

            retVal = HXR_SOCK_WOULDBLOCK; 
            m_bHaltSocketReadTemporarily = TRUE;
        }

#else   // HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT

        retVal = m_pSocket->Read(&pReadBuffer);

#endif  // HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT
  
        if (HXR_OK == retVal)
        {
            if (m_ulIgnoreBytesYetToBeDownloaded)
            {
                HXLOGL3(HXLOG_HTTP, "Disposing of up to %lu garbage bytes (buf size %lu)", m_ulIgnoreBytesYetToBeDownloaded, pReadBuffer->GetSize());

                if (m_ulIgnoreBytesYetToBeDownloaded >= pReadBuffer->GetSize())
                {
                    m_ulIgnoreBytesYetToBeDownloaded -= pReadBuffer->GetSize();
                    HX_RELEASE(pReadBuffer);
                }
                else
                {
                    // we must dispose of only part of the buffer!
                    IHXBuffer* pBuffer=NULL;
                    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);

                    HX_ASSERT(pBuffer);

                    if (pBuffer)
                    {
                        pBuffer->Set(pReadBuffer->GetBuffer() + m_ulIgnoreBytesYetToBeDownloaded,
                                pReadBuffer->GetSize() - m_ulIgnoreBytesYetToBeDownloaded);
                        m_ulIgnoreBytesYetToBeDownloaded = 0;
                        HX_RELEASE(pReadBuffer);
                        pReadBuffer = pBuffer;
                    }
                }
            }

            if (pReadBuffer)
            {
                m_PreProcessedReadBuffers.AddTail(pReadBuffer);
                HXLOGL4(HXLOG_HTTP, "Network: _DoSomeReadingFromSocket read %lu", pReadBuffer->GetSize());
            }
        }
        else if (HXR_SOCK_ENDSTREAM == retVal)
        {
            if (!m_bKnowContentSize)
            {
                // if "Content-Length" is not set in HTTP Reponse, 
                // we need to assume we have read all the content if the socket was closed by the server.
                SetReadContentsDone(TRUE);
            }

            // normal end of data reached
            if( m_pSocket )
            {
                m_pSocket->Close();
                HX_RELEASE(m_pSocket);
                HXLOGL1(HXLOG_HTTP, "_DoSomeReadingFromSocket END OF STREAM (closing socket)");

                // we should still kick-start a callback, since we may need to process reads
                // if another owner (like in a surestream file) is reading for us.

                if (m_pCallback && !m_pCallback->IsCallbackPending())
                {
                    m_pCallback->ScheduleRelative(m_pScheduler, 0);
                }
            }
        }
        else if (HXR_SOCK_WOULDBLOCK != retVal)
        {
            HX_ASSERT(false); // investigate
            // severe error
            if( m_pSocket )
            {
                m_pSocket->Close();
                HX_RELEASE(m_pSocket);
            }
            HXLOGL1(HXLOG_HTTP, "_DoSomeReadingFromSocket severe error %08x", retVal);
            m_LastError = retVal;
        } 
    }


    if (m_pChunkyRes && m_pChunkyRes->CountCursors() > 1)
    {
        // if we're in a surestream file for example, there might
        // be other httpfsys objects relying on us to fill up the
        // chunky res! We can't let 'em down!
        bHandleBuffersImmediately = TRUE;
    }

    if (!m_bReadHeaderDone)
    {
        // If we're starting up, let us not delay in handling the
        // header.
        bHandleBuffersImmediately = TRUE;
    }

    if (bHandleBuffersImmediately)
    {
        // if we're being called via the ProcessIdle timer, then
        // we have the luxury of being able to process all content
        // that has been read.

        while (!m_PreProcessedReadBuffers.IsEmpty())
        {
            IHXBuffer* pBuf = (IHXBuffer*)m_PreProcessedReadBuffers.RemoveHead();
            HX_ASSERT(pBuf);

            HandleSocketRead(HXR_OK, pBuf);
            HX_RELEASE(pBuf);
        }
    }

    if (!m_PreProcessedReadBuffers.IsEmpty())
    {
        // we read from the socket but didn't process everything.
        // So we set up the callback to ensure that the preprocessed
        // read buffers get dealt with quickly.
        if (m_pCallback->IsCallbackPending())
        {
            // we don't want to trust the existing callback to
            // be called back in a timely manner, since it may be
            // the lengthy connection timeout callback.
            m_pCallback->Cancel(m_pScheduler);
        }

        m_pCallback->ScheduleRelative(m_pScheduler, 0);
    }

    return retVal;
}



void
CHTTPFileObject::_SetCurrentReadPos(UINT32 ulNewCurrentReadPosition)
{
    // if the new value is between the old value and the max contig,
    // we don't have to change the max contig; otherwise we do.
    if (ulNewCurrentReadPosition < m_ulCurrentReadPosition)
    {
        m_ulLastKnownEndOfValidContiguousRange = 0;
    }
    else if (ulNewCurrentReadPosition >= m_ulLastKnownEndOfValidContiguousRange)
    {
        m_ulLastKnownEndOfValidContiguousRange = 0;
    }
    m_ulCurrentReadPosition = ulNewCurrentReadPosition;
}

UINT32
CHTTPFileObject::_GetContiguousLength()
{
    UINT32 ulContig = 0;
    UINT32 ulCurrentReadPosition = m_ulCurrentReadPosition;

    UINT32 ulStartOffset = ulCurrentReadPosition;

    if (m_ulLastKnownEndOfValidContiguousRange > ulCurrentReadPosition)
    {
        ulStartOffset = m_ulLastKnownEndOfValidContiguousRange;
    }

    UINT32 ulInterimContig = 0;

    HX_ASSERT(m_pChunkyRes);
    
    if (m_pChunkyRes)
    {
        ulInterimContig = m_pChunkyRes->GetContiguousLength(ulStartOffset);
    }

    m_ulLastKnownEndOfValidContiguousRange = ulStartOffset + ulInterimContig;
    ulContig = m_ulLastKnownEndOfValidContiguousRange - ulCurrentReadPosition;
    return ulContig;
}


void
CHTTPFileObject::_ClearPreprocessedBuffers()
{
    while (!m_PreProcessedReadBuffers.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_PreProcessedReadBuffers.RemoveHead();
        HX_ASSERT(pBuf);
        
        HX_RELEASE(pBuf);
    }
}



//
//
//
//
//
//
HX_RESULT
CHTTPFileObject::_HandleUnAuthorized(HTTPResponseMessage* pMessage, IHXBuffer* pBuffer, UINT32 ulHeaderLength)
{
    HXLOGL1(HXLOG_HTTP, "_HandleUnauthorized buffer size %lu hdr size %lu",
           pBuffer->GetSize(), ulHeaderLength);

    HX_RESULT   ResultStatus = HXR_OK;
    UINT32      ulAltURL = 0;
    CHXString   sConnection;
    IHXValues* pRequestHeaders = NULL;

    if (!m_pRequest)
    {
        return HXR_UNEXPECTED;
    }

    HX_RESULT retVal = HXR_OK;
    IHXRegistry* pRegistry = NULL;
    retVal = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (SUCCEEDED(retVal))
    {
        IHXBuffer* pBuffer = NULL;
        retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
        if (SUCCEEDED(retVal))
        {
            HX_ASSERT(pBuffer);

            CHXString authString;

            authString = pMessage->getHeaderValue("Proxy-Authenticate");
            if (!authString.IsEmpty())
            {
                pBuffer->Set((const unsigned char*)(const char*)authString,
                        authString.GetLength()+1);
                UINT32 regid = pRegistry->GetId(PROXY_AUTHENTICATION_RECENT_KEY);
                if (!regid)
                {
                    pRegistry->AddStr(PROXY_AUTHENTICATION_RECENT_KEY, pBuffer);
                }
                else
                {
                    pRegistry->SetStrByName(PROXY_AUTHENTICATION_RECENT_KEY, pBuffer);
                }
            }

            authString = pMessage->getHeaderValue("WWW-Authenticate");
            if (!authString.IsEmpty())
            {
                pBuffer->Set((const unsigned char*)(const char*)authString,
                        authString.GetLength()+1);
                UINT32 regid = pRegistry->GetId(WWW_AUTHENTICATION_RECENT_KEY);
                if (!regid)
                {
                    pRegistry->AddStr(WWW_AUTHENTICATION_RECENT_KEY, pBuffer);
                }
                else
                {
                    pRegistry->SetStrByName(WWW_AUTHENTICATION_RECENT_KEY, pBuffer);
                }
            }

            HX_RELEASE(pBuffer);
        }
        HX_RELEASE(pRegistry);
    }

    if (HXR_OK == m_pRequest->GetRequestHeaders(pRequestHeaders) && pRequestHeaders)
    {
        // determine AltURL
        pRequestHeaders->GetPropertyULONG32("IsAltURL", ulAltURL);
        HX_RELEASE(pRequestHeaders);
    }

    if (ulAltURL == 1)
    {
        // no support for Authenication in AltURL mode
        ResponseReady(HXR_NOT_AUTHORIZED, m_pRequest);
    }
    else
    {
        IHXValues* pResponseHeaders = NULL;
        DECLARE_SMART_POINTER(IHXBuffer) spBufferConnection;

        //XXXkshoop Don't Timeout while we are waiting for authentication!!
        // need to disable relevant timeouts here.
        m_bDisableConnectionTimeOut = TRUE;

        if
        (
            HXR_OK == m_pRequest->GetResponseHeaders(pResponseHeaders)
            &&
            pResponseHeaders
        )
        {
            IHXBuffer* pServerHeaderBuffer = NULL;
            HX_RESULT retVal = HXR_OK;

            const char* pszHost = GetActualHost();
            SetCStringPropertyCCF(pResponseHeaders, "_host", pszHost, m_pContext);

            // Keep this connection alive?
            pResponseHeaders->GetPropertyCString
            (
                "Connection",
                spBufferConnection.ptr_reference()
            );

            HXBOOL bConnectionStaysAlive = TRUE;

            // assume it's persistent connection in HTTP1.1
            if (m_bHTTP1_1)
            {
                if (spBufferConnection.IsValid()    &&
                    !strncasecmp((char*)spBufferConnection->GetBuffer(), "close", 5))
                {
                    // Need new connection
                    if (m_pSocket)
                    {
                        m_pSocket->Close();
                        HX_RELEASE(m_pSocket);
                        bConnectionStaysAlive = FALSE;
                    }
                    m_bHTTP1_1 = TRUE;
                    m_bConnectDone = FALSE;
                }
            }
            // assume it's not a persistent connection in HTTP1.0
            else
            {
                if (!spBufferConnection.IsValid() ||
                    0 != strncasecmp((char*)spBufferConnection->GetBuffer(), "keep-alive", 10))
                {
                    // Get rid of this connection
                    // Need new connection
                    if (m_pSocket)
                    {
                        m_pSocket->Close();
                        HX_RELEASE(m_pSocket);
                        bConnectionStaysAlive = FALSE;
                    }
                    m_bHTTP1_1 = TRUE;
                    m_bConnectDone = FALSE;
                }
            }

            if (bConnectionStaysAlive)
            {
                // xxxbobclark I need to know how much has been read in,
                // how big the header is, and how big the body is.
                // Because if the connection is unbroken (like NTLM authentication)
                // there may be leftover stuff on the connection that httpfsys will
                // think is part of the normal content. YIKES!

                UINT32 ulValue = 0;
                if (pMessage->getHeaderValue("content-length", ulValue))
                {
                    if (ulValue > (pBuffer->GetSize() - ulHeaderLength))
                    {
                        m_ulIgnoreBytesYetToBeDownloaded = ulValue - (pBuffer->GetSize() - ulHeaderLength);
                    }
                }
            }

            UINT32 ulHTTPStatus = atoi(pMessage->errorCode());

            // Add the fake _server value that's used
            // in IHXAuthenticationManager2 implementations. xxxbobclark
            HX_ASSERT(m_pHost);
            if (m_pHost)
            {
                retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                            (void**)&pServerHeaderBuffer);
                if (SUCCEEDED(retVal))
                {
                    if (m_bUseProxy && ulHTTPStatus == 407 && m_strProxyHost.GetLength())
                    {
                        pServerHeaderBuffer->Set((UCHAR*)(const char*)m_strProxyHost,
                                    m_strProxyHost.GetLength()+1);
                    }
                    else
                    {
                        pServerHeaderBuffer->Set((UCHAR*)m_pHost, strlen(m_pHost)+1);
                    }
                    pResponseHeaders->SetPropertyCString("_server", pServerHeaderBuffer);
                    HX_RELEASE(pServerHeaderBuffer);
                }
            }

            // Add the protocol to the response headers because TLC needs it
            IHXBuffer* pProtocol = NULL;
            if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pProtocol)))
            {
                pProtocol->Set((UCHAR*)"http", strlen("http") + 1);
                pResponseHeaders->SetPropertyCString("_protocol", pProtocol);
                HX_RELEASE(pProtocol);
            }

            // Add the response status code to the response headers because the
            // client authentication manager needs it
            pResponseHeaders->SetPropertyULONG32("_statuscode", ulHTTPStatus);
        }
        HX_RELEASE(pResponseHeaders);

        if
        (
            m_spClientAuthConversationAuthenticator.IsValid()
            &&
            m_spClientAuthConversationAuthenticator->IsDone()
        )
        {
            // Well we tried to authenticate already,
            // so it must have failed

            m_spClientAuthConversationAuthenticator->Authenticated(FALSE);

            // Cleanup so that we can re-auth
            m_spClientAuthConversationAuthenticator.Release();
        }

        if (!m_spClientAuthConversationAuthenticator.IsValid())
        {
            DECLARE_SMART_POINTER_UNKNOWN spUnknownAuthenticator;
            DECLARE_SMART_POINTER
            (
                IHXObjectConfiguration
            ) spObjectConfigurationAuthenticator;
            DECLARE_SMART_POINTER
            (
                IHXCommonClassFactory
            ) spCommonClassFactoryHXCore;

            spCommonClassFactoryHXCore = m_pContext;

            // Starting conversation
            ResultStatus = spCommonClassFactoryHXCore->CreateInstance
            (
                CLSID_CHXClientAuthenticator,
                (void**)&spUnknownAuthenticator
            );

            if
            (
                SUCCEEDED(ResultStatus)
                &&
                spUnknownAuthenticator.IsValid()
            )
            {
                spObjectConfigurationAuthenticator =
                (
                    spUnknownAuthenticator
                );

                spObjectConfigurationAuthenticator->SetContext(m_pContext);

                m_spClientAuthConversationAuthenticator =
                (
                    spUnknownAuthenticator
                );
            }
        }

        if
        (
            m_spClientAuthConversationAuthenticator.IsValid()
            &&
            !m_spClientAuthConversationAuthenticator->IsDone()
        )
        {
            HX_ASSERT(m_pRequest);
            if (m_pRequest)
            {
                ResultStatus =
                (
                    m_spClientAuthConversationAuthenticator->MakeResponse
                    (
                        this,
                        m_pRequest
                    )
                );

                // Flow continues in ResponseReady()
            }
            else
            {
                // Auth Failed!
                m_spClientAuthConversationAuthenticator->Authenticated(FALSE);
                ResponseReady(HXR_NOT_AUTHORIZED, m_pRequest);
            }
        }
        else
        {
            // Auth Failed!
            if (m_spClientAuthConversationAuthenticator.IsValid())
            {
                m_spClientAuthConversationAuthenticator->Authenticated(FALSE);
            }
            ResponseReady(HXR_NOT_AUTHORIZED, m_pRequest);
        }
    }

    return ResultStatus;
}

// IHXClientAuthResponse
STDMETHODIMP
CHTTPFileObject::ResponseReady
(
    HX_RESULT   ResultStatus,
    IHXRequest* pRequestResponse
)
{
    if(SUCCEEDED(ResultStatus))
    {
        m_bConvertFailedSeeksToLinear = TRUE;
        _ReOpen();
    }
    else
    {
        m_LastError = HXR_NOT_AUTHORIZED;

        if
        (
            m_bMimeResponsePending
            &&
            m_pMimeMapperResponse
        )
        {
            m_bMimeResponsePending = FALSE;

            m_pMimeMapperResponse->MimeTypeFound
            (
                HXR_NOT_AUTHORIZED,
                NULL
            );
        }

        if(m_bFileExistsResponsePending && m_pFileExistsResponse)
        {
            // *XXXJR I realize it seems a little odd to say a file
            // * exists when actually all we know is we weren't authorized,
            // * but without doing this, we'd never be able to serve any
            // * authentication-required document.  The fs manager would
            // * always forget about us as soon as we said a file didn't exist.
            // *
            AddNoCacheHeader();
            m_bFileExistsResponsePending = FALSE;
            m_pFileExistsResponse->DoesExistDone(TRUE);
        }

        if(m_bInitResponsePending)
        {
            m_bInitResponsePending = FALSE;
            m_pFileResponse->InitDone(HXR_NOT_AUTHORIZED);
        }
    }

    return HXR_OK;
}

void
CHTTPFileObject::AddNoCacheHeader()
{
    // Add the "Pragma: no-cache" response header so that Proxies
    // don't think they can cache our dynamically generated content
    if (m_pRequest)
    {
        IHXValues* pResHeaders = NULL;

        m_pRequest->GetResponseHeaders(pResHeaders);
        if (!pResHeaders)
        {
            IUnknown* pUnknown = NULL;

            m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList,
                (void**)&pUnknown);

            pUnknown->QueryInterface(IID_IHXValues,
                (void**)&pResHeaders);

            m_pRequest->SetResponseHeaders(pResHeaders);

            HX_RELEASE(pUnknown);
        }

        IHXBuffer* pNoCache = NULL;
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pNoCache);
        pNoCache->Set((UCHAR*)"no-cache", 9);

        pResHeaders->SetPropertyCString("Pragma", pNoCache);

        HX_RELEASE(pNoCache);
        HX_RELEASE(pResHeaders);
    }
}

//
//
//
//
//
//
HX_RESULT
CHTTPFileObject::_HandleFail(UINT32 ulHTTPError)
{
    HX_RESULT   theErr = HXR_OK;

    HXLOGL1(HXLOG_HTTP, "_HandleFail(%lu)", ulHTTPError);

    if (ulHTTPError == 400)
    {
        theErr = HXR_FAIL;

        if (m_bOnServer)
        {
            ReportGeneralFailure();
        }
    }
    else
    {
        theErr = HXR_DOC_MISSING;

        if (m_bOnServer)
        {
            ReportDocumentMissing();
        }
    }

    if(m_bInitPending)
    {
        // since any of these callbacks could have us killed
        AddRef();

        if (m_bInitResponsePending && m_pFileResponse)
        {
            m_bInitResponsePending = FALSE;
            m_pFileResponse->InitDone(theErr);
        }

        if
        (
            m_bFileExistsResponsePending
            && m_pFileExistsResponse
        )
        {
            m_bFileExistsResponsePending = FALSE;
            m_pFileExistsResponse->DoesExistDone(FALSE);
        }

        if
        (
            m_bMimeResponsePending
            && m_pMimeMapperResponse
        )
        {
            m_bMimeResponsePending = FALSE;
            m_pMimeMapperResponse->MimeTypeFound(theErr, NULL);
        }

        Release();
    }

    return theErr;
}

//
//
//
//
//
//
HX_RESULT
CHTTPFileObject::_HandleSuccess
(
    HTTPResponseMessage*    pMessage,
    IHXBuffer*              pBuffer,
    UINT32                  ulHeaderLength
)
{
    HX_RESULT   theErr = HXR_OK;

    HXLOGL2(HXLOG_HTTP, "_HandleSuccess");

    // If we've been closed, then exit early
    if (m_bClosed) return HXR_OK;

    m_bAuthorized = TRUE;
    m_bDisableConnectionTimeOut = FALSE;

    if (SUCCEEDED(theErr))
    {
        // Find the content length to support percent done
        // handling...
        UINT32 ulValue = 0;
        if (pMessage->getHeaderValue("content-length", ulValue))
        {
            m_nOriginalContentSize = m_nContentSize = ulValue;

            // xxxbobclark the problem is that m_nContentSize is really
            // the content size of this particular GET. And if we're getting
            // a byte range -- say, from the 7-megabyte mark of an 8-megabyte
            // file, then m_nContentSize will be one megabyte. Kinda
            // right but kinda wrong. So we'll look for the denominator in the
            // content-range header and set m_nContentSize according to the
            // total size of the resource we're GETting.

            m_bKnowContentSize = TRUE;

            CHXString theContentRange = pMessage->getHeaderValue("content-range");

            //HX_ASSERT(theContentRange.GetLength() > 0);

            if (m_bSupportsByteRanges && theContentRange.GetLength() > 0)
            {
                ULONG32 numFields = theContentRange.CountFields('/');

                HX_ASSERT(numFields == 2);

                if (numFields == 2)
                {
                    CHXString theDenominator = theContentRange.NthField('/', 2);
                    m_nOriginalContentSize = m_nContentSize = strtol((const char*)theDenominator, 0, 10);
                }
            }

            m_bKnowHTTPResponseContentSize = TRUE;
            m_ulHTTPResponseContentSize = m_nContentSize;
        }

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
        // /If we have been told the content length in the URL via ?filesize=n,
        // use that if it's not less than m_nContentSize:
        if (!m_bKnowContentSize  &&
                m_ulPrgDnTotalFileSize != HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
        {
            m_bKnowContentSize = TRUE;
            m_nOriginalContentSize = m_nContentSize = m_ulPrgDnTotalFileSize;
        }
        else if (m_bKnowContentSize)
        {
            m_ulPrgDnTotalFileSize = m_nContentSize;
        }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

        if (m_pCacheEntry)
        {
            CacheSupport_HandleSuccess(pMessage);
        }

        // Find the mime type to support mime reporting...
        CHXString sMimeType;
        sMimeType = pMessage->getHeaderValue("content-type");

        // Some Java Server Pages (JSP) servers (version 2.0
        // and higher) may append the "charset" after the
        // mime type in the "Content-Type" header like this:
        //
        // Content-Type: application/smil;charset=ISO-8859-1
        //
        // as specified in the JSP 2.0 specification:
        // http://www.jcp.org/aboutJava/communityprocess/review/jsr152
        //
        // Therefore, we should check to see if there's a ';'
        // in the content type. If there is, then only use the string
        // up to that point
        INT32 lSemi = sMimeType.Find(';');
        if (lSemi > 0)
        {
            // We have a semi-colon, so only use the content-type
            // up to the semi-colon but not include it or anything
            // past it
            sMimeType = sMimeType.Left(lSemi);
        }

        // IF "application/octet-stream"
        // mask mimetype to get actual mimetype
        // based on extension later in the core.

        // XXXBJP
        // Apache (with it's 44% share of the web server market):
        // if it doesn't know the file type (ie, unrecognized extension)
        // will look at the contents. If the contents are binary it'll return
        // "application/octet-stream". If the contents look like text, it returns
        // "text/plain". This means a standard Apache installation will return
        // "text/plain" for imf/rp/rt files). Since we don't
        // use text/plain, we'll wipe it out and let the core use the file
        // extension to determine the content type.
        if (sMimeType.IsEmpty() ||
            (sMimeType == "application/octet-stream") ||
            (sMimeType == "text/plain"))
        {
            m_strMimeType = "*";
        }
        else
        {
            m_strMimeType = sMimeType;
        }

        if (IsLiveStream((const char*)sMimeType))
        {
            // This is a live stream, so to avoid using up all available
            // disk space, we will tell chunky res to discard data after
            // we have read it once.
            m_pChunkyRes->DiscardUsedData();
        }

#ifdef HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT
        m_pChunkyRes->DiscardUsedData();
#endif // HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT


        // Find out if the content is encoded
        CHXString sEncoding;
        m_bEncoded = FALSE;
        sEncoding = pMessage->getHeaderValue("content-encoding");
        if (sEncoding == "gzip")
        {
            // The content is gzipped. Make a note of this, so
            // we can unzip it properly after it is received
            m_bEncoded = TRUE;

            // We don't know what the unencoded size will be
            m_nContentSize = 0;
            m_bKnowContentSize = FALSE;

            // but m_bKnowHTTPResponseContentSize and
            // m_ulHTTPResponseContentSize remain set.
        }

        CHXString sTransferEncoding;
        m_bChunkedEncoding = FALSE;
        sTransferEncoding = pMessage->getHeaderValue("transfer-encoding");
        if (sTransferEncoding == "chunked")
        {
            m_bChunkedEncoding = TRUE;
        }

        // Handle adding any trailing content data to the start
        // of the content buffers...
        INT32 nContentLen = pBuffer->GetSize() - ulHeaderLength;
        if (nContentLen > 0)
        {
            theErr = _SetDataJustDownloaded((const char*)pBuffer->GetBuffer() + ulHeaderLength,
                                            nContentLen);
        }

        // See if the data trailing after the header contains all
        // the content (or note the fact if their won't be any contents
        // data at all...)
        if(m_bKnowContentSize && m_nContentRead >= m_nContentSize)
        {
            // XXX HP
            // set m_bReadContentsDone to TRUE whenever we have
            // read to the end of the content regardless of the seek
            // since m_bReadContentDone will be reset to FALSE when
            // seek occurs

            // xxxbobclark if there are several HTTP objects writing
            // to a single chunky res (like a surestream file or
            // a situation involving ram or smil with multiple references
            // to the same HTTP-streamed media) it's possible that the
            // file may be completed at a "surprising" time -- i.e.
            // without this object really knowing about it. So we'll
            // only mark it done if we can verify that the whole chunky
            // res has been filled, or if there's only a single cursor
            // writing to it.

            if ((_GetContiguousLength()
                + m_ulCurrentReadPosition >= m_nContentSize)
                || (m_pChunkyRes->CountCursors() == 1))
            {
                HXLOGL1(HXLOG_HTTP, "_HandleSuccess Finished Reading Content");
                SetReadContentsDone(TRUE);
            }
        }
        else if (m_bKnowHTTPResponseContentSize &&
                m_ulHTTPResponseContentRead >= m_ulHTTPResponseContentSize)
        {
            // this else clause should only be hit if the response was
            // gzip encoded.

            HX_ASSERT(m_bEncoded);

            HXLOGL1(HXLOG_HTTP, "_HandleSuccess finished reading encoded content");
            SetReadContentsDone(TRUE);
        }

        if(m_bStatPending)
        {
            m_bStatPending = FALSE;

            HX_RESULT hxrStatDoneStatus = HXR_OK;
            UINT32 ulStatDoneFileSize = m_nContentSize;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
            if (!m_bKnowContentSize)
            {
                if (m_ulPrgDnTotalFileSize == HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
                {
                    // /We have no idea what the correct size of the file is
                    // so we just return FAIL w/unknown size:
                    hxrStatDoneStatus = HXR_FAIL;
                    ulStatDoneFileSize = HX_PROGDOWNLD_UNKNOWN_FILE_SIZE;
                }
                else // /Use URL filesize parameter:
                {
                    ulStatDoneFileSize = m_ulPrgDnTotalFileSize;
                }
            }
#endif // /end ifdef HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
            m_pFileStatResponse->StatDone(hxrStatDoneStatus,
                    ulStatDoneFileSize, 0, 0, 0, 0);

            m_pFileStatResponse->Release();
        }
    }

    return theErr;
}

HXBOOL CHTTPFileObject::IsLiveStream(const char* pMimeType)
{
    HXBOOL bLive = FALSE;

    // Note: We know this is a live stream if we previously determined
    // the server to be a Shoutcast server, or the length is set to
    // one of the magic numbers that some of these mpeg audio servers
    // use to indicate a "live" stream - DPS
    if (m_bShoutcast)
    {
        bLive = TRUE;
    }
    else if (m_bKnowContentSize &&
                (m_nContentSize == 99999999 ||
                 m_nContentSize == 54000000))
    {
        if (!strcasecmp(pMimeType, "audio/mpeg"))
        {
            bLive = TRUE;
        }
    }
    else if (!m_bKnowHTTPResponseContentSize && !m_bEncoded)
    {
        // We add this case because we have encountered live HTTP streams
        // for which there is no Content-Length, and we have no other
        // indication that this is a live stream. Currently, httpfsys
        // uses IsLiveStream to determine two things:
        // a) If IsLiveStream() == TRUE, then return PREFER_LINEAR to
        //    the Advise(RANDOM_ACCESS) query. Fileformats use this
        //    information to know not to seek to the end of the file.
        // b) If IsLiveStream() == TRUE, then we tell chunky-res to
        //    throw data away after it has read it once.
        // For truly live streams, we want to do both (a) and (b). 
        // However, we may encounter a case that a web server is
        // mis-configured and does not report a Content-Length for
        // on-demand files. For such mis-configured web servers, we
        // really want to do just (a) and not (b). However, we currently
        // don't have any way to distinguish between these truly
        // live streams and mis-configured on-demand streams. The
        // downside to declaring these streams live is that
        // some data may have to be downloaded twice if the
        // data needs to be read twice. However, this seems like
        // a reasonable trade-off, since if a server is mis-configured,
        // it is expected that clients would have to be less efficient.
        bLive = TRUE;
    }

    return bLive;
}

HX_RESULT
CHTTPFileObject::_ReOpen()
{
    LOG("_ReOpen");
    LOGX((szDbgTemp, "    URL='%s'", m_pFilename));

    HX_RESULT   rc = HXR_OK;

    m_bWriteDone = FALSE;
    m_nRequestWritten = 0;
    m_nTotalRequestSize = 0;

    // xxxbobclark set these member variables only if we're not in the middle of
    // an http 1.1 byte range seek

    if (!m_bByteRangeSeekPending)
    {
        m_bKnowContentSize = FALSE;
        m_nContentSize = 0;

        m_bKnowHTTPResponseContentSize = FALSE;
        m_ulHTTPResponseContentSize = 0;
    }

    m_bEncoded = FALSE;

    if (!m_bSupportsByteRanges)
    {
        m_nContentRead = 0;
        m_ulHTTPResponseContentRead = 0;
        _SetCurrentReadPos(0);
    }

    m_nHeaderRead = 0;
    m_bReadHeaderDone = FALSE;
    SetReadContentsDone(FALSE);
    m_strRequest = "";

    m_bTCPReadPending = FALSE;

    if (m_pCallback && m_pCallback->IsCallbackPending())
    {
        m_pCallback->Cancel(m_pScheduler);
    }

#ifdef _MACINTOSH
    m_bReadDoneToBeProcessed = FALSE;
    HX_RELEASE(m_pReadDoneBuffer);
    m_uReadDoneStatus = HXR_OK;
#endif

    rc = _OpenFile(m_pFilename, HX_FILE_READ|HX_FILE_BINARY);

    // mask HXR_WOULD_BLOCK so we dont report this as an error
    if (HXR_WOULD_BLOCK == rc)
    {
        rc = HXR_OK;
    }

    return rc;
}

STDMETHODIMP
CHTTPFileObject::SetRequest
(
    IHXRequest* pRequest
)
{
    char*  pTemp            = NULL;
    const char*  pURL               = NULL;;
    IHXValues*   pReqHeaders = NULL;

    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;

    if (m_pRequest)
    {
        m_pRequest->AddRef();

        CHXString   sPath;

        if (m_pRequest->GetURL(pURL) != HXR_OK)
        {
            return HXR_FAIL;
        }

        LOGX((szDbgTemp, "    GetURL() returns: '%s'", pURL));

        // Handle RP bug PR6655
        char *pStr;
        pStr = (char *)strstr (pURL, "/chttp://");
        if (pStr)
        {
                LOG ("    Jumping past buggy prefix");
                pURL = pStr + strlen("/chttp:");
                LOGX((szDbgTemp, "    pURL = %s", pURL));
        }

        // if URL contains <protocol>:// then DON'T reuse basepath
        // from original request, instead just use the new URL.
        if (m_szBaseURL && pURL
         && (strncasecmp(pURL,"chttp://", 8) != 0)
         && (strncasecmp(pURL,"http://",  7) != 0))
        {
            sPath = m_szBaseURL;
            if(sPath[sPath.GetLength()-1] != '/')
            {
                sPath += '/';
            }
        }
        else
        {
            sPath = "";
        }

        if (pURL)
        {
            sPath += pURL;
        }

        // Remove default HTTP port specification, if present
        if(m_bUseHTTPS)
        {
            sPath.FindAndReplace(DEF_HTTPS_PORT_STR, "/");
        }
        else
        {
            sPath.FindAndReplace(DEF_HTTP_PORT_STR, "/");
        }

        delete[] m_pFilename;
        m_pFilename = new_string(sPath.GetBuffer(1));

        // Save these headers.
        HX_RELEASE(m_pRequestHeadersOrig);
        m_pRequest->GetRequestHeaders(m_pRequestHeadersOrig);

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
        // /File's total size might be in the URL prarameters:
        const char* pFs = "?filesize=";
        UINT32 ulFsLen = strlen(pFs);
        const char* pFilesize = strstr(pURL, pFs);
        if (pFilesize  &&  strlen(pFilesize) > ulFsLen  &&
                // /Make sure there's a number there:
                '0' <= pFilesize[ulFsLen]  &&  '9' >= pFilesize[ulFsLen])
        {
            m_ulPrgDnTotalFileSize = atol(&pFilesize[ulFsLen]);
        }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
    }

    // Set if URL is cachable
    if (m_bCacheEnabled && m_pCacheFile && m_pFilename &&
        strncasecmp(m_pFilename, "chttp://", 8) == 0)
    {
        LOGX((szDbgTemp, "    m_pFilename='%s'", m_pFilename));
        LOGX((szDbgTemp, "    m_pCacheFile='%s'", (char *)m_pCacheFile->GetBuffer()));

        // Create the cache database if need be
        if (g_pCacheEntry == NULL)
        {
            g_pCacheEntry = new CCacheEntry (m_pContext,
					     (char *)m_pCacheFile->GetBuffer(),
                                             m_ulCacheMaxSize,
                                             m_pFilename);
        }
        if (m_pCacheEntry == NULL)
            m_pCacheEntry = g_pCacheEntry;
    }

    // Strip off any URL parameters and store them
    HX_RELEASE(m_pParams);
    if (m_pFilename)
    {
        char* pTemp = NULL;

        if (m_bOnServer)
        {
            // Check for a bitrate parameter. If it exists, remove it
            // from the URL we are about to request
            char* pBitrate = strstr(m_pFilename, "bitrate=");
            if (pBitrate)
            {
                char* pEnd = pBitrate;
                while (*pEnd && *pEnd != '&')
                {
                    pEnd++;
                }

                // Make sure the URL is properly fixed up
                INT32 lSize = strlen(m_pFilename) + 1;
                pTemp = new char[lSize];
                if ((*(pBitrate - 1) == '?') && (*pEnd != '\0'))
                {
                    pEnd++;
                }
                else
                {
                    *(pBitrate - 1) = '\0';
                }
                SafeSprintf(pTemp, lSize, "%s%s", m_pFilename, *pEnd ? pEnd : "");

                // Set the request object to use this URL instead
                m_pRequest->SetURL(pTemp);

                HX_VECTOR_DELETE(m_pFilename);
                m_pFilename = pTemp;
                pTemp = NULL;
            }
        }

        // In order to parse the URL with the CHXURL class, we must ensure
        // that the URL is fully qualified. Add a bogus protocol and domain
        // if necessary...
        if ((strncasecmp(pURL,"chttp://", 8) != 0) &&
            (strncasecmp(pURL,"http://",  7) != 0))
        {
                int lenTemp = strlen(m_pFilename) + strlen("http://x/") + 1;
            pTemp = new char[lenTemp];
            SafeSprintf(pTemp, lenTemp, "http://x/%s", m_pFilename); /* Flawfinder: ignore */
        }
        else
        {
            pTemp = new_string(m_pFilename);
        }

        CHXURL urlParser(pTemp, m_pContext);
        if (SUCCEEDED(urlParser.GetLastError()))
        {
            IHXBuffer* pBuffer = NULL;
            IHXBuffer* pRealURL = NULL;
            IHXBuffer* pFinalURL = NULL;

            m_pParams = (IHXValues*)urlParser.GetOptions();

            // If there is a parameter called "RealURL", the value is the
            // URL that we should request instead of m_pFilename
            if (HXR_OK == m_pParams->GetPropertyBuffer("RealURL", pRealURL))
            {
                // The RealURL parameter is perplexed, so we must deperplex
                // it before using it.
                DePerplexBuffer(pRealURL, pFinalURL);

                // Set the request object to use this URL instead
                m_pRequest->SetURL((char*)pFinalURL->GetBuffer());

                HX_VECTOR_DELETE(m_pFilename);
                m_pFilename = new_string((char*)pFinalURL->GetBuffer());
                HX_RELEASE(pFinalURL);

                // If there is a parameter called "Cookies", it is an encoded
                // set of cookies that should be added to the request headers
                // when we actually make the "RealURL" request
                if (HXR_OK == m_pParams->GetPropertyBuffer("Cookies", pBuffer))
                {
                    // Create a temporary buffer
                    char* pBuf = new char[pBuffer->GetSize()];

                    // Unescape the cookies
                    INT32 nLength = URLUnescapeBuffer((char*)pBuffer->GetBuffer(),
                        pBuffer->GetSize(), pBuf);

                    if (nLength)
                    {
                        pBuf[nLength] = '\0';

                        // Add it to the request headers
                        IHXBuffer* pCookie = NULL;

                        m_pRequest->GetRequestHeaders(pReqHeaders);
                        if (pReqHeaders)
                        {
                            // Find out if a Cookie Request header already exists
                            pReqHeaders->GetPropertyCString("Cookie", pCookie);
                            if (pCookie)
                            {
                                // Concatenate this cookie to the end
                                INT32 nCatLen = nLength + pCookie->GetSize() + 1;
                                char* pCatBuf = new char[nCatLen];
                                SafeSprintf(pCatBuf, nCatLen, "%s;%s", pBuf, (char*)pCookie->GetBuffer());

                                // Replace the existing cookie header with the new one
                                HX_RELEASE(pCookie);
                                m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pCookie);
                                if (pCookie)
                                {
                                    pCookie->Set((UCHAR*)pCatBuf, nCatLen);
                                    pReqHeaders->SetPropertyCString("Cookie", pCookie);
                                }

                                HX_VECTOR_DELETE(pCatBuf);
                            }
                            else
                            {
                                // Create a new buffer
                                m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pCookie);
                                if (pCookie)
                                {
                                    pCookie->Set((UCHAR*)pBuf, nLength + 1);
                                    pReqHeaders->SetPropertyCString("Cookie", pCookie);
                                }
                            }
                            HX_RELEASE(pCookie);
                        }
                        HX_RELEASE(pReqHeaders);
                    }

                    HX_VECTOR_DELETE(pBuf);
                }
                HX_RELEASE(pBuffer);
            }
            HX_RELEASE(pRealURL);
        }
        HX_VECTOR_DELETE(pTemp);
    }

    if (m_bMangleCookies)
    {
        // Store the master set of cookies, in case we get redirected
        // and need to redecide which cookies to send
        HX_RELEASE(m_pMangledCookies);
        m_pRequest->GetRequestHeaders(pReqHeaders);
        if (pReqHeaders)
        {
            pReqHeaders->GetPropertyCString("Cookie", m_pMangledCookies);
        }
        HX_RELEASE(pReqHeaders);

        // If Cookie mangling is enabled, unmangle all incoming cookies,
        // keeping only those which apply to the request URL
        UnmangleAllCookies(m_pRequest);
    }

    return HXR_OK;
}

STDMETHODIMP
CHTTPFileObject::GetRequest
(
    REF(IHXRequest*) pRequest
)
{
    pRequest = m_pRequest;

    if (pRequest)
    {
        pRequest->AddRef();
    }

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXTimeSettings::Get/SetConnnectionTimeout
 *      Purpose:
 *          Get/Set the connection timeout setting, in seconds
 */
STDMETHODIMP
CHTTPFileObject::GetConnectionTimeout(REF(UINT32)   /*OUT*/ nSeconds)
{
    nSeconds = m_nConnTimeout;
    return HXR_OK;
}

STDMETHODIMP
CHTTPFileObject::SetConnectionTimeout(UINT32 /*IN*/ nSeconds)
{
    m_nConnTimeout = nSeconds;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXTimeSettings::Get/SetServerTimeout
 *      Purpose:
 *          Get/Set the server timeout setting, in seconds
 */
STDMETHODIMP
CHTTPFileObject::GetServerTimeout(REF(UINT32) /*OUT*/ nSeconds)
{
    nSeconds = m_nServerTimeout;
    return HXR_OK;
}

STDMETHODIMP
CHTTPFileObject::SetServerTimeout(UINT32 /*IN*/ nSeconds)
{
    m_nServerTimeout = nSeconds;
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXHTTPRedirect::Init
 *  Purpose:
 *      Initialize the response object
 */
STDMETHODIMP
CHTTPFileObject::Init(IHXHTTPRedirectResponse* pRedirectResponse)
{
    HXLOGL1(HXLOG_HTTP, "Init (file %s)", NULLOK(m_pFilename));
    HX_RESULT hr = SetResponseObject(pRedirectResponse);

    if (HXR_OK == hr)
    {
        hr = _OpenFile(m_pFilename, HX_FILE_READ);
    }		
    
    return hr;
}

/************************************************************************
 *  Method:
 *      IHXHTTPRedirect::Init
 *  Purpose:
 *      Initialize the response object
 */
STDMETHODIMP
CHTTPFileObject::SetResponseObject(IHXHTTPRedirectResponse* pRedirectResponse)
{
    HX_RESULT hr = HXR_FAILED;

    if (pRedirectResponse)
    {
        HX_RELEASE(m_pRedirectResponse);

        m_pRedirectResponse = pRedirectResponse;
        m_pRedirectResponse->AddRef();

        hr = HXR_OK;
    }

    return hr;
}


BEGIN_INTERFACE_LIST(HTTPTCPResponse)
    INTERFACE_LIST_ENTRY(IID_IHXSocketResponse,  IHXSocketResponse)
    INTERFACE_LIST_ENTRY(IID_IHXResolveResponse, IHXResolveResponse)
END_INTERFACE_LIST

HTTPTCPResponse::HTTPTCPResponse()
    : m_pOwner(NULL)
    , m_bOwnerDestroyed(FALSE)
{
}

void
HTTPTCPResponse::InitObject(CHTTPFileObject* pOwner)
{
    m_pOwner = pOwner;
}

HTTPTCPResponse::~HTTPTCPResponse()
{
    m_pOwner = NULL;
}

STDMETHODIMP HTTPTCPResponse::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    return m_pOwner ? m_pOwner->GetAddrInfoDone(status, nVecLen, ppAddrVec) : HXR_OK;
}

STDMETHODIMP HTTPTCPResponse::GetNameInfoDone(HX_RESULT status, const char* pszNode, const char* pszService)
{
    return m_pOwner ? m_pOwner->GetNameInfoDone(status, pszNode, pszService) : HXR_OK;
}

STDMETHODIMP HTTPTCPResponse::EventPending(UINT32 uEvent, HX_RESULT status)
{
    AddRef();
    HX_RESULT hr = HXR_OK;
    if (m_pOwner && !m_bOwnerDestroyed)
    {
        m_pOwner->AddRef();
        hr = m_pOwner->EventPending(uEvent, status);
        m_pOwner->Release();
    }
    Release();
    return hr;
}

/**************************************************************
 *
 *  Additional routines
 *
 **************************************************************/

/*
 * This supplies the bulk of processing needed by the _OpenFile method
 */
STDMETHODIMP_(void)
CHTTPFileObject::CacheSupport_InitObject (void)
{
        HX_ASSERT(m_pCacheFile == NULL);
        HX_ASSERT(m_pPreferences);

        HXLOGL2(HXLOG_HTTP, "CacheSupport_InitObject");

        // get preferences for caching
        if (m_pPreferences)
        {
            // Get cache directory, if not set and available
            IHXBuffer*     pBuffer  = NULL;

            // Check to see if the cache is enabled
            m_bCacheEnabled = TRUE;
            if (m_pPreferences->ReadPref("CacheEnabled", pBuffer) == HXR_OK)
            {
                if (atoi((const char*)pBuffer->GetBuffer()))
                {
                    HXLOGL3(HXLOG_HTTP, "Cache enabled entry exists and is enabled");
                    m_bCacheEnabled = TRUE;
                }
                else
                {
                    HXLOGL3(HXLOG_HTTP, "Cache enabled entry exists but is disabled");
                    m_bCacheEnabled = FALSE;
                }
                HX_RELEASE(pBuffer);
            }
            else if (m_bOnServer)
            {
                m_bCacheEnabled = FALSE;
            }

            // Here we look for version number to activate the cache...
            if (m_pContext != NULL)
            {
                IHXProductIdentity *productIdentity = NULL;
                m_pContext->QueryInterface(IID_IHXProductIdentity, (void **)(&productIdentity));
                if (productIdentity != NULL)
                {
                    // Get the version info...
                    UINT32 majorVersion = productIdentity->GetMajorVersion();
                    UINT32 minorVersion = productIdentity->GetMinorVersion();
                    UINT32 releaseNumber = productIdentity->GetReleaseNumber();

                    // XXXXGB Looks like a hack, feels like a hack, smells like a hack, must be a hack...it disables the cache for U2...
                    if ((majorVersion == 6) && (minorVersion == 0) && (releaseNumber == 6)) m_bCacheEnabled = FALSE;
                }
                HX_RELEASE(productIdentity);
            }

            // If the cache is disabled, do not set it up
            if (m_bCacheEnabled == FALSE)
            {
                return;
            }

            if (m_pCacheFile == NULL)
            {
                if (m_pPreferences->ReadPref("CacheFilename", m_pCacheFile) != HXR_OK)
                {
                    HXLOGL3(HXLOG_HTTP, "    Cannot get CacheDirectory from preferences; using default instead");
                    if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&m_pCacheFile))
                    {
#ifdef USE_TEMP_CACHE_LOCATION
                        SetBufferToCacheFilePath(m_pCacheFile);
#else
                        char  szModule[MAX_CACHE_FILENAME + sizeof DEF_CACHE_DB + 4] = { 0 }; /* Flawfinder: ignore */
#ifdef _MACINTOSH
                        FSSpec fileSpec;
                        GetCurrentAppSpec(&fileSpec);
                        CHXString fullpath;
                        fullpath = fileSpec;
                        SafeStrCpy(szModule, fullpath, sizeof(szModule));
#elif defined(_WIN32)
                        GetModuleFileName(NULL, OS_STRING2(szModule,MAX_CACHE_FILENAME), MAX_CACHE_FILENAME);
#else
                        *szModule = '\0';
#endif
                        // Find the separator character before the file name
                        INT8* pFilename = ::strrchr(szModule, OS_SEPARATOR_CHAR);

                        // Perhaps they are using the '/' separator
                        if (pFilename == NULL)
                        {
                            pFilename = ::strrchr(szModule, '/');
                        }

                        if (pFilename != NULL) // Found
                        {
                            // module file name starts after the separator charactor
                            *pFilename = '\0';
                        }
                        // Add on the cache database filename portion
                        ::strcat (szModule, OS_SEPARATOR_STRING); /* Flawfinder: ignore */
                        ::strcat (szModule, DEF_CACHE_DB); /* Flawfinder: ignore */
                        m_pCacheFile->Set((UINT8*)szModule, strlen(szModule) + 1);
#endif // !USE_TEMP_CACHE_LOCATION
                    }
                }
                else
                {
#ifndef _MACINTOSH
                      // Make sure that the given filename belongs to a directory
                      struct stat statbuf;
                      // If cache database filename does not refer to a file, drop the filename part
                      if (!stat ((const char*)m_pCacheFile->GetBuffer(), &statbuf))
                      {
                          if (!(statbuf.st_mode & S_IFDIR))
                          {
                              HXLOGL2(HXLOG_HTTP, "Invalid cache directory specified!");
                              m_pCacheFile->Set((UINT8*)"cache_db", strlen("cache_db") + 1);
                          }
                      }
#endif
                }
                LOG("    Cache directory set");
            }

            // Check to see if the this is an Authenticated reponse
            if (m_pPreferences->ReadPref("Authorization", pBuffer) == HXR_OK)
            {
                
                HXLOGL2(HXLOG_HTTP, "    Authorized content not cached");
                m_bCacheEnabled = FALSE;

                HX_RELEASE(pBuffer);
            }

            // Check to see if the max cache size is given
            m_ulCacheMaxSize = DEFAULT_MAX_CACHE_SIZE;
            if (m_pPreferences->ReadPref("CacheMaxSize", pBuffer) == HXR_OK)
            {
                m_ulCacheMaxSize = atoi((const char*)pBuffer->GetBuffer());

                if (m_ulCacheMaxSize < MININUM_MAX_CACHE_SIZE)
                {
                    m_ulCacheMaxSize = MININUM_MAX_CACHE_SIZE;
                }
                HX_RELEASE(pBuffer);
            }
            HXLOGL3(HXLOG_HTTP, "    Maximum cache size is %lu", m_ulCacheMaxSize);

            if (m_pPreferences->ReadPref("CacheDefaultTTL", pBuffer) == HXR_OK)
            {
                g_ulDefTtl = atoi((const char*)pBuffer->GetBuffer());
                HX_RELEASE(pBuffer);
            }
            else if (m_pPreferences->ReadPref("DefaultTTL", pBuffer) == HXR_OK)
            {
                g_ulDefTtl = atoi((const char*)pBuffer->GetBuffer());
                HX_RELEASE(pBuffer);
            }

        else
        {
            HXLOGL3(HXLOG_HTTP, "    No default TTL given in registry");
            g_ulDefTtl = CACHE_DEFAULT_TTL_SECS;
        }

        HXLOGL3(HXLOG_HTTP, "    Default TTL is %lu", g_ulDefTtl);

        IHXBuffer*     pCacheCutOffDate    = NULL;

        if (m_pPreferences && m_pPreferences->ReadPref("CacheCutOffDate", pCacheCutOffDate) == HXR_OK)
        {
            m_ulCutOffDate = atol((const char*)pCacheCutOffDate->GetBuffer());
            LOGX((szDbgTemp, "    CacheCutOffDate = %lu (%ld)",
                  m_ulCutOffDate, time(NULL) - m_ulCutOffDate));

            if (m_ulCutOffDate)         // Make sure date is reasonable
            {
                if (m_ulCutOffDate < (UINT32)time(NULL) - (2 * SECS_IN_YEAR)
                 || m_ulCutOffDate > (UINT32)time(NULL))
                {
                    HXLOGL2(HXLOG_HTTP, "    CacheCutOffDate invalid");
                    m_ulCutOffDate = 0;
                }
            }
            HX_RELEASE(pCacheCutOffDate);
        }
    }
}

STDMETHODIMP_(void)
CHTTPFileObject::CacheSupport_OpenFile (void)
{
    if (m_bCacheEnabled && m_pFilename && m_pCacheEntry && !m_bCached)
    {
        // Look up the URL in the cache
        DBT             dbtKey       = { m_pFilename, strlen (m_pFilename) };
        DBT             dbtHeader    = { NULL, 0 };
        DBT             dbtContent   = { NULL, 0 };
        int             rc           = m_pCacheEntry->ReadCache(&dbtKey, &dbtHeader, &dbtContent, m_bMirroredServers);
        PCacheHeader    pCacheHeader = (CacheHeader*) dbtHeader.data;
        IHXBuffer*     pHttpHeaders = NULL;

        HXLOGL2(HXLOG_HTTP, "    rc=%lu, pHeader=0x%08X, pContent=0x%08X", rc, dbtHeader.data, dbtContent.data);
        if ((rc == HXR_OK) && pCacheHeader && pCacheHeader->m_ulExpiryTime < (UINT32)time(NULL))
        {
            HXLOGL2(HXLOG_HTTP, "  Cache hit (but entry has expired)");
            m_pCacheEntry->del(&dbtKey);    // Remove expired entry from cache
        }
        else if ((rc == HXR_OK) && pCacheHeader && pCacheHeader->m_ulCreateTime < m_ulCutOffDate)
        {
            HXLOGL2(HXLOG_HTTP, " Cache hit (but entry is before cut-off date)");
            m_pCacheEntry->del(&dbtKey);    // Remove ill-treated entry from cache
        }
        else if ((rc == HXR_OK) && dbtHeader.data)
        {
            HXLOGL2(HXLOG_HTTP, " Cache hit");

            ASSERT (dbtHeader.size >= sizeof(CacheHeader));

            // extract the HTTP header values from the cache entry
            UINT32 ulRet = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pHttpHeaders);
            pHttpHeaders->SetSize (pCacheHeader->m_ulHttpHeaderSize);
            memcpy (pHttpHeaders->GetBuffer(), /* Flawfinder: ignore */
                    (const char *)dbtHeader.data + sizeof(CacheHeader),
                    pCacheHeader->m_ulHttpHeaderSize);

            IHXValues *pValues = CreateValuesFromBuffer (pHttpHeaders);
            if (HXR_OK != m_pRequest->SetResponseHeaders(pValues))
            {
                HXLOGL1(HXLOG_HTTP, "CacheSupport_OpenFile: Error setting Response Headers");
            }
            HX_RELEASE(pHttpHeaders);
            HX_RELEASE(pValues);

            HXLOGL3(HXLOG_HTTP, "WriteChunky at %ld size %ld\n", 0, dbtContent.size);

            // copy the data from the cache
            m_pChunkyRes->SetData(0, (const char *)dbtContent.data, dbtContent.size, this);

            m_bCached               = TRUE;
            // These are values expected to be set when data is downloaded
            m_bConnectDone          = TRUE;
            m_bWriteDone            = TRUE;
            m_bReadHeaderDone       = TRUE;
            SetReadContentsDone(TRUE); // /Does: m_bReadContentsDone = TRUE;
            m_bKnowContentSize      = TRUE;
            m_nContentSize          = dbtContent.size;
            m_nContentRead          = dbtContent.size;
            m_nHeaderRead           = dbtContent.size;
            
            m_bKnowHTTPResponseContentSize = TRUE;
            m_ulHTTPResponseContentSize = m_nContentSize;
            m_ulHTTPResponseContentRead = m_nContentRead;

            _SetCurrentReadPos(0);

            m_strMimeType = (pCacheHeader->m_szMimeType[0]) ? (char*)pCacheHeader->m_szMimeType : "*";

        }
        else
        {
            HXLOGL2(HXLOG_HTTP, "CacheSupport_OpenFile: Cache miss");
        }

        if (dbtHeader.data)
            free(dbtHeader.data);
        if (dbtContent.data)
            free(dbtContent.data);
    }
    else
    {
        if (m_pFilename && m_pCacheEntry)
        {
            HXLOGL3(HXLOG_HTTP, "CacheSupport_OpenFile: Already loaded");
            LOG ("... Already loaded");
        }
        else
        {
            HXLOGL3(HXLOG_HTTP, "CacheSupport_OpenFile: %s", m_pFilename ? "... no database" : "... no filename");
        }
    }

    if (m_bCached)
    {
        m_bInitPending = FALSE;
    }
}

STDMETHODIMP_(void)
CHTTPFileObject::CacheSupport_ReadDone (void)
{
    /*
     * cache the data
     */
    if (!m_bCacheEnabled)
    {
        // Quietly skip the caching
    }
    else if (m_nContentRead < m_nContentSize)
    {
        // If the size is not right, don't cache
    }
    else if (m_pChunkyRes && !m_pChunkyRes->HasPartialData(m_nContentSize,0))
    {
        // if the chunky res doesn't have the whole resource (yet), don't cache
    }
    else if (m_pCacheEntry == NULL)
    {
        // This should not happen
        HXLOGL2(HXLOG_HTTP, "cache: >!  m_pCacheEntry is NULL");
    }
    else if (!m_bKnowContentSize)
    {
        // If the HTTP header did not contain size info, don't cache
        HXLOGL2(HXLOG_HTTP, "cache: >   Unknown Content-Size");
    }
    else if (m_pFilename == NULL)
    {
        // If the URL is null, don't cache
        HXLOGL2(HXLOG_HTTP, "cache: >!  No Filename");
    }
    else if (m_bCached == TRUE)
    {
        // This should not happen
        HXLOGL2(HXLOG_HTTP, "cache: >   Already cached");
    }
    else if (m_ulExpiryTime <= (UINT32)time(NULL))
    {
        // If already expired, don't cache
    }
    else if (m_nContentRead > MAX_CACHE_SIZE)
    {
        // If too large, don't cache
        HXLOGL2(HXLOG_HTTP, "cache: >   Too large to cache");
    }
    else if (m_nContentRead == 0)
    {
        // If too large, don't cache
        HXLOGL2(HXLOG_HTTP, "cache: >   Content size is zero, possible redirect");
    }
    else
    {
        // Everthing looks good for caching
        HXLOGL4(HXLOG_HTTP, "cache: >   Writing to cache");

        const char* pszMimeType      = m_strMimeType;
        IHXBuffer* pHttpHeader      = NULL;
        IHXValues* pResponseHeaders = NULL;

        if (m_pRequest)
        {
            if (HXR_OK == m_pRequest->GetResponseHeaders(pResponseHeaders))
            {
                pHttpHeader = CreateBufferFromValues(pResponseHeaders);
            }
            else
            {
                HXLOGL2(HXLOG_HTTP, "cache:    SetResponseHeaders(pResponseHeaders) failed");
            }
        }
        else
        {
            HXLOGL2(HXLOG_HTTP, "cache:    m_pRequest is NULL");
        }

        m_pCacheEntry->CleanCache(m_ulCutOffDate);
        m_pCacheEntry->WriteCache (m_nContentRead, m_ulExpiryTime, m_pFilename, pszMimeType,
                                   pHttpHeader, m_pChunkyRes, m_bMirroredServers);

        HX_RELEASE(pResponseHeaders);
        HX_RELEASE(pHttpHeader);
    }
}

STDMETHODIMP_(void)
CHTTPFileObject::CacheSupport_HandleSuccess(HTTPResponseMessage* pMessage)
{
    MIMEHeaderValue* pHeaderValue    = NULL;
    MIMEHeader*          pHeader         = pMessage->getFirstHeader();
    INT32            lTimeOffset     = 0;

    while (pHeader)
    {
        pHeaderValue = pHeader->getFirstHeaderValue();

        CHXString HeaderString;

        while (pHeaderValue)
        {
            CHXString TempString;

            pHeaderValue->asString(TempString);
            HeaderString += TempString;

            pHeaderValue = pHeader->getNextHeaderValue();
            if (pHeaderValue)
            {
                HeaderString += ", ";
            }
        }
        HXLOGL2(HXLOG_HTTP, "cache:    HTTP Header: %s = '%s'", pHeader->name(), NULLOK((const char*)HeaderString));

        if (!strcasecmp(pHeader->name(), "Date"))
        {
            UTCTimeRep* pTimeRepUTCTimeRep = new UTCTimeRep(HeaderString);
            UINT32 ulDateTime = pTimeRepUTCTimeRep->asUTCTimeT();
            lTimeOffset = (INT32)ulDateTime - time(NULL);

            HXLOGL2(HXLOG_HTTP, "cache:    Date value is %lu [%lds/%ldh offset]",
                 ulDateTime, lTimeOffset, (lTimeOffset + (60 * 30))/(60 * 60));

            delete pTimeRepUTCTimeRep;
        }
        if (!strcasecmp(pHeader->name(), "Expires"))
        {
            UTCTimeRep* pTimeRepUTCTimeRep = new UTCTimeRep(HeaderString);
            m_ulExpiryTime = pTimeRepUTCTimeRep->asUTCTimeT();
            delete pTimeRepUTCTimeRep;
        }
        if (!strcasecmp(pHeader->name(), "Last-Modified"))
        {
            UTCTimeRep* pTimeRepUTCTimeRep = new UTCTimeRep(HeaderString);
            m_ulLastModTime = pTimeRepUTCTimeRep->asUTCTimeT();
            delete pTimeRepUTCTimeRep;
        }
        pHeader = pMessage->getNextHeader();
    }
    if (m_ulLastModTime)
    {
        m_ulLastModTime -= lTimeOffset;
        HXLOGL2(HXLOG_HTTP, "cache:    Last-Modified value is %lu [%lus ago]",
             m_ulLastModTime, time(NULL) - m_ulLastModTime);
    }
    if (m_ulExpiryTime)
    {
        m_ulExpiryTime -= lTimeOffset;
        HXLOGL2(HXLOG_HTTP, "cache:    Expiry value is %lu [%lus hence]",
             m_ulExpiryTime, m_ulExpiryTime - time(NULL));
    }

    // Find the expiry time by looking at the Time-To-Live value
    CHXString sCacheControl;
    sCacheControl = pMessage->getHeaderValue("Cache-Control");
    if (!sCacheControl.IsEmpty())
    {
        const char *pszCacheControl = NULL;
        UINT32      ulMaxAgeSeconds = 0;

        sCacheControl.MakeLower();
        pszCacheControl = sCacheControl;

        HXLOGL2(HXLOG_HTTP, "cache:    Cache-Control string is '%s'", pszCacheControl);

        if (sscanf(pszCacheControl, "max-age=%lu", &ulMaxAgeSeconds) == 1)
        {
            HXLOGL2(HXLOG_HTTP, "cache:    MaxAgeSeconds = %lu", ulMaxAgeSeconds);
            m_ulExpiryTime = time(NULL) + ulMaxAgeSeconds;
        }

        // Check for No-Cache entry in Cache-Control
        if (!strncmp(pszCacheControl, "no-cache", 8)
         || !strncmp(pszCacheControl, "no-store", 8)
         || !strncmp(pszCacheControl, "private", 7)
         || !strncmp(pszCacheControl, "must-revalidate", 8))
        {
            HXLOGL2(HXLOG_HTTP, "cache:    Cacheing disabled for this item");
            m_ulExpiryTime = time(NULL) - 1;
        }
    }
    else if (g_ulDefTtl > 0)
    {
        LOGX((szDbgTemp, "cache:    No expiry time specified, assigning default TTL (%lu sec)", g_ulDefTtl));

        m_ulExpiryTime = time(NULL) + g_ulDefTtl;
    }
}

STDMETHODIMP_(IHXBuffer*)
CHTTPFileObject::CreateBufferFromValues (IHXValues *pHeaderValues)
{
    IHXBuffer*  pBuffer        = NULL;
    char*        pCurrent       = NULL;
    const char*  pPropName      = NULL;
    IUnknown*    pUnknown   = NULL;
    IHXBuffer*  pPropValue     = NULL;

    HXLOGL3(HXLOG_HTTP, "cache: CreateBufferFromValues (0x%08X)", pHeaderValues);

    HX_RESULT theErr = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
    pBuffer->SetSize(4);  // four signature bytes at beginning   '0x03 0x03 0x03 0x03'
    memset (pBuffer->GetBuffer(), 0x03, 4);

    if (!pHeaderValues)
    {
        HX_ASSERT(0);  // showell
        theErr = HXR_FAIL;
    }

    // for each item in the list, write an entry into the buffer
    if (theErr == HXR_OK)
    {
        // XXX showell - Eventually we want to make callers pass us an
        // IHXKeyValueList, not an IHXValues, because headers should
        // really only be IHXKeyValueList.  When we make that change,
        // we will want to use IHXKeyValueList::GetIter and then do
        // iterate with GetPair calls.  But, until then, it's perfectly
        // fine to rely on GetFirstPropertyCString/GetNextPropertyCString,
        // because CKeyValueList supports those methods w/the right behavior.

        theErr = pHeaderValues->GetFirstPropertyCString (pPropName, pPropValue);

//      LOGX ((szDbgTemp, "    GetFirstPropertyCString() returns 0x%08X/0x%08X/Err=0x%08X", pPropName, pPropValue, theErr));

        while(theErr == HXR_OK)
        {
            // copy value pair into aggregate buffer
            UINT32 ulStartLength = pBuffer->GetSize();
            pBuffer->SetSize(ulStartLength + 8 + strlen(pPropName) + pPropValue->GetSize());

            UINT8* pStart =  pBuffer->GetBuffer() + ulStartLength;

            // write key size as octet stream of length four
            UINT32 ulLength = strlen (pPropName);
            *pStart++ = (UINT8) ((ulLength >> 24) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >> 16) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >>  8) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >>  0) & 0x000000ff);
            // write data
            memcpy (pStart, pPropName, ulLength); /* Flawfinder: ignore */
            pStart += ulLength;

            // write data size as octet stream of length four
            ulLength  = pPropValue->GetSize();
            *pStart++ = (UINT8) ((ulLength >> 24) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >> 16) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >>  8) & 0x000000ff);
            *pStart++ = (UINT8) ((ulLength >>  0) & 0x000000ff);
            // write data
            memcpy (pStart, pPropValue->GetBuffer(), ulLength); /* Flawfinder: ignore */
            pStart += ulLength;

            HX_RELEASE(pPropValue);

            theErr = pHeaderValues->GetNextPropertyCString (pPropName, pPropValue);
        }
    }
    else
    {
        LOG ("*** Error creating IHXBuffer for HTTP headers");
    }

    // add {{0, NULL}, {0, NULL}} to terminate list
    pBuffer->SetSize(pBuffer->GetSize() + 16);
    memset (pBuffer->GetBuffer() + pBuffer->GetSize() - 16, 0, 16);

    HXLOGL3(HXLOG_HTTP, "cache:    HTTP Header buffer is %lu bytes", pBuffer->GetSize());
    return(pBuffer);
}

STDMETHODIMP_(IHXValues*)
CHTTPFileObject::CreateValuesFromBuffer (IHXBuffer *pBuffer)
{
    IUnknown*       pUnknown = NULL;
    IHXValues*     pValues = NULL;
    IHXKeyValueList* pNewList = NULL;
    UINT32      ulBufLen = pBuffer->GetSize();
    UINT32      ulIndex = 0;
    UINT8*      pBufData = pBuffer->GetBuffer();

    HXLOGL3(HXLOG_HTTP, "cache: CreateValuesFromBuffer (Size=%4lu)", ulBufLen);

    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList, (void**)&pUnknown))
    {
        goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXKeyValueList, (void**)&pNewList))
    {
        goto cleanup;
    }

    ulIndex = 4;    // Offset of '4' is due to signature bytes
    while (ulIndex < ulBufLen)
    {
        // extract value pair from aggregate buffer

        // write size as octet stream of length four
        UINT32 ulPropLength = 0;
        ulPropLength += (pBufData[ulIndex++] << 24) & 0xff000000;
        ulPropLength += (pBufData[ulIndex++] << 16) & 0x00ff0000;
        ulPropLength += (pBufData[ulIndex++] <<  8) & 0x0000ff00;
        ulPropLength += (pBufData[ulIndex++] <<  0) & 0x000000ff;
        // write data
        UINT8* pPropBuf = new UINT8[ulPropLength + 1];
        memcpy (pPropBuf, pBufData + ulIndex, ulPropLength); /* Flawfinder: ignore */
        pPropBuf[ulPropLength] = '\0';

        ulIndex += ulPropLength;

        // write size as octet stream of length four
        UINT32 ulLength = 0;
        ulLength += (pBufData[ulIndex++] << 24) & 0xff000000;
        ulLength += (pBufData[ulIndex++] << 16) & 0x00ff0000;
        ulLength += (pBufData[ulIndex++] <<  8) & 0x0000ff00;
        ulLength += (pBufData[ulIndex++] <<  0) & 0x000000ff;
        // write data
        IHXBuffer* pNewBuffer = NULL;	
	CreateAndSetBufferCCF(pNewBuffer, pBufData + ulIndex, ulLength, m_pContext);

	ulIndex += ulLength;

        if (ulPropLength && ulLength)
        {
            pNewList->AddKeyValue((const char*)pPropBuf, pNewBuffer);
        }
        else
        {
            LOG ("*** Error creating IHXValues from IHXBuffer");
        }

        HX_VECTOR_DELETE(pPropBuf);
        pNewBuffer->Release();
    }

    if (HXR_OK != pNewList->QueryInterface(IID_IHXValues, (void**)&pValues))
    {
        // XXX showell - Eventually we want to pass around IHXKeyValueLists
        // internally, so we don't have to do this strange stuff w/IHXValues.
        // Until then, we can rely on CKeyValueList implementing IHXValues,
        // so we should never hit this code.
        HX_ASSERT(0);
        pValues = NULL;
    }

cleanup:

    HX_RELEASE(pNewList);
    HX_RELEASE(pUnknown);

    return(pValues);
}

STDMETHODIMP
CHTTPFileObject::ProcessCacheCompletions (HXBOOL bRedirected)
{
    if (m_bCached)
    {
        if (m_pMimeMapperResponse && m_bMimeResponsePending == TRUE)
        {
            const char* pMimeType = NULL;
            if (!m_strMimeType.IsEmpty())
            {
                pMimeType = m_strMimeType;
            }

            LOGX ((szDbgTemp, "    Idle: Cached MimeType='%s'", NULLOK((const char*)m_strMimeType)));
            m_bMimeResponsePending = FALSE;
            m_pMimeMapperResponse->MimeTypeFound(HXR_OK, pMimeType);
        }
        else if (bRedirected)
        {
            // Addref m_pFileResponse here since it may be released before being
            // assigned in Init(), below.
            IHXFileResponse* pFileResponse = m_pFileResponse;
            if (pFileResponse)  pFileResponse->AddRef();

            // The client hasn't implemented a MimeMapperResponse, but we still
            // need to make sure that we deal with the cached data.  Call Init()
            // to do this, which is what is normally done by the MimeTypeFound
            // callback.
            Init(HX_FILE_READ, pFileResponse);
            HX_RELEASE(pFileResponse);
        }
    }
    return HXR_OK;
}


STDMETHODIMP_( void )
CHTTPFileObject::SetDestinationFile( const char *pFilename )
{
    if( pFilename && *pFilename )
    {
        m_bSaveToFile = TRUE;
        m_SaveFileName = pFilename;
        //try deleting existing file with the same name
        ASSERT( !m_SaveFileName.IsEmpty() );
        CHXDirectory Dir;
        Dir.DeleteFile(m_SaveFileName);
    }
    else
        m_bSaveToFile = FALSE;
}

/************************************************************************
*  Method:
*      IHXProxyAutoConfigCallback::GetProxyInfoDone
*  Purpose:
*/
STDMETHODIMP
CHTTPFileObject::GetProxyInfoDone(HX_RESULT status,
                                  char*     pszProxyInfo)
{
    HX_RESULT   rc = HXR_OK;
    PACInfo*    pPACInfo = NULL;

    m_bGetProxyInfoPending = FALSE;

    if (HXR_OK == status && pszProxyInfo)
    {
        ::ParsePACInfo(pszProxyInfo, m_pPACInfoList);
        // at least one PAC entry
        HX_ASSERT(m_pPACInfoList && m_pPACInfoList->GetCount());

        m_PACInfoPosition = m_pPACInfoList->GetHeadPosition();
        pPACInfo = (PACInfo*)m_pPACInfoList->GetNext(m_PACInfoPosition);

        if (pPACInfo && pPACInfo->type != PAC_DIRECT)
        {
            m_bUseProxy = TRUE;
            m_nProxyPort = pPACInfo->ulPort;
            m_strProxyHost = pPACInfo->pszHost;
        }
    }

    rc = _OpenFileExt();

    if (HXR_OK != rc && HXR_OK == m_LastError)
    {
        m_LastError = rc;
    }

    return rc;
}


#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
/*
 *  Helper methods that are used by IHXPDStatusMgr methods, below
 */        
void
CHTTPFileObject::ReportCurrentDurChanged()
{  
    ReportChange(PRDNFLAG_REPORT_CUR_DUR);
}    
 
void
CHTTPFileObject::ReportTotalDurChanged()
{ 
    ReportChange(PRDNFLAG_REPORT_TOTAL_DUR);
} 
 
void
CHTTPFileObject::ReportDownloadComplete()
{ 
    ReportChange(PRDNFLAG_REPORT_DNLD_COMPLETE);
} 
 
void
CHTTPFileObject::ReportChange(UINT32 ulFlags)
{  
    HX_ASSERT(!(ulFlags & PRDNFLAG_REPORT_TOTAL_DUR)  ||
            IsPrgDnCompleteFileDurKnown());
 
    // /Set this whether we have observers or not:
    if (ulFlags & PRDNFLAG_REPORT_DNLD_COMPLETE)
    {
        m_bDownloadCompleteReported = TRUE;
    }

    if (m_pPDSObserverList)
    { 
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        { 
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            { 
                // /Pass what we know about or duration along to our observers:
                // (It's OK to pass a NULL IHXStreamSource since our observers
                // are themselves IHXStreamSources) :
                if (ulFlags & PRDNFLAG_REPORT_CUR_DUR)
                { 
                    pObserver->OnDownloadProgress(NULL,
                            m_ulCurrentDurOfBytesSoFar,
                            m_ulFileSizeSoFar,
                            // /lTimeSurplus is calculated farther along the
                            // chain (HXPlayer) and is thus passed as an
                            // invalid value from here:
                            HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS);
                } 
                if (ulFlags & PRDNFLAG_REPORT_TOTAL_DUR)
                { 
                    m_ulPriorReportedTotalFileDur = m_ulPrgDnTotalFileDur;
                    pObserver->OnTotalDurChanged(NULL, m_ulPrgDnTotalFileDur);
                } 
                if (ulFlags & PRDNFLAG_REPORT_DNLD_COMPLETE)
                { 
                    m_bDownloadCompleteReported = TRUE;
                    pObserver->OnDownloadComplete(NULL);
                } 

                if (ulFlags & PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_CLAIMED)
                { 
                    pObserver->SrcClaimsSeekSupport(NULL, TRUE);
                } 
                else if (ulFlags & PRDNFLAG_REPORT_SEEK_SUPPORT_NOW_NOT_CLAIMED)
                {
                    pObserver->SrcClaimsSeekSupport(NULL, FALSE);
                }
            } 
        } 
    } 
} 
 
 
 
HX_RESULT 
CHTTPFileObject::EstablishPDSObserverList()
{ 
    HX_RESULT  hxrslt = HXR_OK;
 
    if (!m_pPDSObserverList)
    { 
        m_pPDSObserverList = new CHXSimpleList();
 
        if (!m_pPDSObserverList)
        { 
            hxrslt = HXR_OUTOFMEMORY;
        } 
    } 
  
    return hxrslt; 
} 

/*
 *  IHXPDStatusMgr methods
 */        

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::AddObserver
 *  Purpose:
 *      Lets an observer register so it can be notified of file changes
 */
STDMETHODIMP
CHTTPFileObject::AddObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    { 
        if (!m_pPDSObserverList)
        { 
            hxrslt = EstablishPDSObserverList();
        } 
        if (SUCCEEDED(hxrslt)  &&  m_pPDSObserverList)
        { 
            if (!m_pPDSObserverList->Find(pObserver))
            { 
                pObserver->AddRef();
                m_pPDSObserverList->AddTail(pObserver);
            } 
        } 
    } 
  
    return hxrslt; 
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::RemoveObserver
 *  Purpose:
 *      Lets an observer unregister so it can stop being notified of
 *      file changes
 */
STDMETHODIMP
CHTTPFileObject::RemoveObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    { 
        hxrslt = HXR_FAIL; 
        if (m_pPDSObserverList)
        { 
            LISTPOSITION lPosition = m_pPDSObserverList->Find(pObserver);
            if (lPosition)
            { 
                m_pPDSObserverList->RemoveAt(lPosition);
                HX_RELEASE(pObserver);
            } 
  
            hxrslt = HXR_OK; 
        } 
    } 

    return hxrslt; 
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
 *  Purpose:
 *      Lets an observer set the interval that the reporter (fsys) takes
 *      between status updates.  Value is in milliseconds.
 */
STDMETHODIMP
CHTTPFileObject::SetStatusUpdateGranularityMsec(
        UINT32 /*IN*/ ulStatusUpdateGranularityInMsec)
{
    m_ulStatusUpdateGranularityInMsec = ulStatusUpdateGranularityInMsec;
    return HXR_OK;
}

#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.



void CHTTPFileObject::CallbackProc(void* pArg)
{
    if (pArg)
    {
        CHTTPFileObject* pObj = (CHTTPFileObject*) pArg;
        pObj->AddRef();
        pObj->ProcessIdle();
        pObj->Release();
    }
}

HX_RESULT
CHTTPFileObject::MangleAllSetCookies(IHXRequest* pRequest)
{
    IHXValues* pResponseHdrs = NULL;
    IHXKeyValueList* pKeyedResHdrs = NULL;
    IUnknown* pUnknown = NULL;
    IUnknown* pNewUnknown = NULL;
    IHXBuffer* pSetCookie = NULL;
    IHXBuffer* pMangledSetCookie = NULL;

    // Get the response headers from the request
    pRequest->GetResponseHeaders(pResponseHdrs);
    if (pResponseHdrs)
    {
        // Find out if the response headers support IHXKeyValueList
        pResponseHdrs->QueryInterface(IID_IHXKeyValueList,
            (void**)&pKeyedResHdrs);

        if (pKeyedResHdrs)
        {
            IHXKeyValueListIterOneKey* pListIter = NULL;

            pKeyedResHdrs->GetIterOneKey("Set-Cookie",pListIter);

            while (pListIter->GetNextString(pSetCookie) == HXR_OK)
            {
                // If we found a buffer and it's not already mangled...
                if (pSetCookie &&
                    strncmp((char*)pSetCookie->GetBuffer(), "RSG2!", 5))
                {
                    // *** Mangle it *** //
                    MangleSetCookie(pSetCookie, pMangledSetCookie);
                    if (pMangledSetCookie)
                    {
                        pListIter->ReplaceCurr(pMangledSetCookie);
                    }
                    HX_RELEASE(pMangledSetCookie);
                }
                HX_RELEASE(pSetCookie);
            }
            HX_RELEASE(pListIter);
        }
        else
        {
            // Retrieve the only "Set-Cookie" value from the response headers
            pResponseHdrs->GetPropertyCString("Set-Cookie", pSetCookie);

            // If we found a buffer and it is not already mangled...
            if (pSetCookie &&
                strncmp((char*)pSetCookie->GetBuffer(), "RSG2!", 5))
            {
                // *** Mangle it *** //
                MangleSetCookie(pSetCookie, pMangledSetCookie);
                if (pMangledSetCookie)
                {
                    // Replace the original Set-Cookie value
                    pResponseHdrs->SetPropertyCString("Set-Cookie",
                        pMangledSetCookie);
                }
                HX_RELEASE(pMangledSetCookie);
            }
            HX_RELEASE(pSetCookie);
        }
        HX_RELEASE(pKeyedResHdrs);
    }
    HX_RELEASE(pResponseHdrs);

    return HXR_OK;
}

HX_RESULT
CHTTPFileObject::MangleSetCookie(IHXBuffer* pInput,
                                 REF(IHXBuffer*) pOutput)
{
    HX_RESULT hResult = HXR_OK;
    char* pDomain = NULL;
    char* pPath = NULL;
    char* pSetCookie = NULL;
    char* pCursor = NULL;
    char* pStart = NULL;
    char* pEnd = NULL;
    CHXURL* pHXURL = NULL;
    IHXValues* pURLProps = NULL;
    IHXBuffer* pBuffer = NULL;
    char* pFinal = NULL;
    UINT32 ulDomainLen = 0;
    UINT32 ulPathLen = 0;
    char* pOutputBuf = NULL;
    char* pOutputStart = NULL;
    HXBOOL bIsDomain = FALSE;

    pSetCookie = (char*)pInput->GetBuffer();

    for (pCursor = pSetCookie; *pCursor; pCursor++)
    {
        if (*pCursor == ';')
        {
            pCursor++;

            // Check if the next value is the domain or path
            if (strncasecmp(pCursor, "domain", 6) == 0)
            {
                pStart = pCursor + 7;
                pEnd = pStart;

                while (*pEnd && *pEnd != ';')
                {
                    pEnd++;
                }

                if (pEnd > pStart)
                {
                    pDomain = new char[pEnd - pStart + 1];
                    strncpy(pDomain, pStart, pEnd - pStart); /* Flawfinder: ignore */
                    pDomain[pEnd - pStart] = '\0';
                    ulDomainLen = pEnd - pStart;

                    bIsDomain = TRUE;
                }
            }
            else if (strncasecmp(pCursor, "path", 4) == 0)
            {
                pStart = pCursor + 5;
                pEnd = pStart;

                while (*pEnd && *pEnd != ';')
                {
                    pEnd++;
                }

                if (pEnd > pStart)
                {
                    pPath = new char[pEnd - pStart + 1];
                    strncpy(pPath, pStart, pEnd - pStart); /* Flawfinder: ignore */
                    pPath[pEnd - pStart] = '\0';
                    ulPathLen = pEnd - pStart;
                }
            }

            pCursor--;
        }

        // If we've found both, we can stop looking
        if (pDomain && pPath)
        {
            break;
        }
    }

    // If we didn't find a domain or path setting
    if (!pDomain || !pPath)
    {
        // Figure it out from the URL
        pHXURL = new CHXURL(m_pFilename, m_pContext);

        pURLProps = pHXURL->GetProperties();
        if (pURLProps)
        {
            if (!pDomain)
            {
                pURLProps->GetPropertyBuffer(PROPERTY_HOST, pBuffer);
                if (pBuffer)
                {
                    StrAllocCopy(pDomain, (char*)pBuffer->GetBuffer());
                    ulDomainLen = pBuffer->GetSize() - 1;
                }
                HX_RELEASE(pBuffer);
            }

            if (!pPath)
            {
                pURLProps->GetPropertyBuffer(PROPERTY_PATH, pBuffer);
                if (pBuffer)
                {
                    StrAllocCopy(pPath, (char*)pBuffer->GetBuffer());
                    ulPathLen = pBuffer->GetSize() - 1;
                }
                HX_RELEASE(pBuffer);
            }
        }
        HX_RELEASE(pURLProps);
        HX_DELETE(pHXURL);
    }

    if (pDomain && pPath)
    {
        HXBOOL bPastName = FALSE;

        // Create the final output buffer
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)&pOutput);

        // Set the maximum buffer size, allowing for the "RSG2!!%lu!",
        // "RSG2!!" and ";path=/", that we are going to add
        int lenOutBuf = pInput->GetSize() + ulDomainLen + ulPathLen + 21;
        pOutput->SetSize(lenOutBuf);

        pOutputBuf = (char*)pOutput->GetBuffer();
        pOutputStart = pOutputBuf;

        // Step through each character in the Set-Cookie value
        for (pCursor = pSetCookie; *pCursor; pCursor++)
        {
            if (pCursor == pSetCookie)
            {
                // Add RSG2!domain!bIsDomain! to the
                // beginning of the cookie name
                SafeSprintf(pOutputBuf, lenOutBuf, "RSG2!%s!%s!", /* Flawfinder: ignore */
                pDomain, bIsDomain ? "1" : "0");
                pOutputBuf += ulDomainLen + 8;

                *pOutputBuf++ = *pCursor;
            }
            else if (!bPastName && *pCursor == '=')
            {
                *pOutputBuf++ = *pCursor;

                // Add RSG2!path! to the beginning of the cookie value
                SafeSprintf(pOutputBuf, lenOutBuf,  "RSG2!%s!", pPath); /* Flawfinder: ignore */
                pOutputBuf += ulPathLen + 6;

                bPastName = TRUE;
            }
            else if (*pCursor == ';' &&
                     (strncasecmp(pCursor + 1, "domain", 6) == 0 ||
                      strncasecmp(pCursor + 1, "path", 4) == 0))
            {
                // Skip the domain or path
                pCursor++;
                while (*pCursor && *pCursor != ';')
                {
                    pCursor++;
                }
                pCursor--;
            }
            else if (*pCursor == ';' &&
                     (strncasecmp(pCursor + 1, "expires", 7) == 0))
            {
                // Make sure that "expires" is lowercase, to work around a
                // client bug in 7.0 Beta1 and earlier players that will
                // ignore it if it is not all lowercase - DPS
                strncpy(pOutputBuf, ";expires", 8); /* Flawfinder: ignore */
                pOutputBuf += 8;
                pCursor += 7;
            }
            else
            {
                *pOutputBuf++ = *pCursor;
            }
        }

        // Add a path of "/", so that this cookie will always get sent
        // back to this server
        strncpy(pOutputBuf, ";path=/", 7); /* Flawfinder: ignore */
        pOutputBuf += 7;

        *pOutputBuf++ = '\0';

        // Copy the mangled name and value onto the end of the
        // mangled cookies value, so that it will be sent with
        // the next URL request if we are immediately redirected
        StoreMangledCookie(pOutputStart);

        // Set the output buffer size
        pOutput->SetSize(pOutputBuf - pOutputStart);
    }
    else
    {
        hResult = HXR_FAIL;
        pOutput = NULL;
    }

    HX_VECTOR_DELETE(pDomain);
    HX_VECTOR_DELETE(pPath);

    return hResult;
}

HX_RESULT
CHTTPFileObject::StoreMangledCookie(char* pCookie)
{
    char* pStart = NULL;
    char* pEnd = NULL;
    UINT32 ulLen = 0;
    UINT32 ulOrigSize = 0;
    UINT32 ulNewSize = 0;
    IHXBuffer* pMangledCookies = NULL;
    char* pOldBuffer = NULL;
    char* pBuffer = NULL;

    // Create a new cookie buffer
    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
        (void**)&pMangledCookies);
    if (!pMangledCookies)
    {
        return HXR_FAIL;
    }

    pStart = pCookie;
    pEnd = pStart;
    while (*pEnd && *pEnd != ';')
    {
        pEnd++;
    }
    ulLen = pEnd - pStart;

    if (m_pMangledCookies)
    {
        ulOrigSize = m_pMangledCookies->GetSize();
        pOldBuffer = (char*)m_pMangledCookies->GetBuffer();
    }

    // Set the new buffer size
    ulNewSize = ulOrigSize + ulLen + 1;
    pMangledCookies->SetSize(ulNewSize);
    pBuffer = (char*)pMangledCookies->GetBuffer();

    if (ulOrigSize)
    {
        // Add it to the end of the existing cookies
        strncpy(pBuffer, pOldBuffer, ulOrigSize - 1); /* Flawfinder: ignore */
        pBuffer[ulOrigSize - 1] = ';';
        strncpy(pBuffer + ulOrigSize, pCookie, ulLen); /* Flawfinder: ignore */
    }
    else
    {
        // No cookies exist, so just add it
        strncpy(pBuffer, pCookie, ulLen); /* Flawfinder: ignore */
    }
    pBuffer[ulOrigSize + ulLen] = '\0';

    // Replace the existing value
    HX_RELEASE(m_pMangledCookies);
    m_pMangledCookies = pMangledCookies;

    return HXR_OK;
}

HX_RESULT
CHTTPFileObject::UnmangleAllCookies(IHXRequest* pRequest)
{
    HX_RESULT hResult = HXR_OK;
    const char* pURL = NULL;
    IHXValues* pRequestHdrs = NULL;
    IHXBuffer* pCookie = NULL;
    char* pInput = NULL;
    char* pTemp = NULL;
    UINT32 ulTempPos = 0;
    IHXBuffer* pFinalBuf = NULL;
    char* pFinal = NULL;
    UINT32 ulFinalPos = 0;
    HXBOOL bValidCookie = FALSE;
    UINT32 i = 0;
    IHXBuffer* pURLHost = NULL;
    IHXBuffer* pURLPath = NULL;
    IHXBuffer* pDomain = NULL;
    IHXBuffer* pPath = NULL;
    HXBOOL bIsDomain = FALSE;

    // Get the requested URL
    hResult = pRequest->GetURL(pURL);
    if (FAILED(hResult) || !pURL)
    {
        HX_ASSERT(0);
        return HXR_UNEXPECTED;
    }

    // Separate the URL into a domain and path
    hResult = GetHostAndPath(pRequest, pURLHost, pURLPath);
    if (FAILED(hResult))
    {
        return HXR_FAIL;
    }

    // Get the Cookie header if it exists
    pRequest->GetRequestHeaders(pRequestHdrs);
    if (pRequestHdrs)
    {
        pRequestHdrs->GetPropertyCString("Cookie", pCookie);
        if (pCookie && pCookie->GetSize())
        {
            // Create a temporary buffer for use in unmangling each cookie,
            // and another buffer for the final unmangled output
            pTemp = new char[pCookie->GetSize()];

            // Create the final output buffer
	    CreateBufferCCF(pFinalBuf, m_pContext);

            pFinalBuf->SetSize(pCookie->GetSize());
            pFinal = (char*)pFinalBuf->GetBuffer();

            // Iterate through each cookie in the buffer
            pInput = (char*)pCookie->GetBuffer();

            for (UINT32 i = 0; i < pCookie->GetSize(); i++)
            {
                if (pInput[i] != ';' && pInput[i] != '\0')
                {
                    pTemp[ulTempPos++] = pInput[i];
                }
                else
                {
                    // We've got a complete cookie
                    pTemp[ulTempPos] = '\0';

                    // Find out if the cookie is mangled
                    if (!strncmp(pTemp, "RSG2!", 5))
                    {
                        // If so, unmangle it
                        hResult = UnmangleCookie(pTemp, ulTempPos,
                            pDomain, bIsDomain, pPath);
                        if (HXR_OK == hResult && pDomain && pPath)
                        {
                            // Compare the domain and path against those
                            // of the requested URL, to see if this cookie
                            // should be sent on to the http server
                            hResult = CompareDomains(pURLHost,
                                pDomain, bIsDomain);
                            if (SUCCEEDED(hResult))
                            {
                                hResult = ComparePaths(pURLPath, pPath);
                                if (SUCCEEDED(hResult))
                                {
                                    // This cookie does apply to the URL
                                    // we are about to request
                                    bValidCookie = TRUE;
                                }
                            }
                        }
                        HX_RELEASE(pDomain);
                        HX_RELEASE(pPath);
                    }
                    else
                    {
                        bValidCookie = TRUE;
                    }

                    if (bValidCookie)
                    {
                        if (ulFinalPos)
                        {
                            pFinal[ulFinalPos++] = ';';
                        }

                        // Copy this cookie to the final output buffer
                        for (UINT32 j = 0; pTemp[j]; j++)
                        {
                            pFinal[ulFinalPos++] = pTemp[j];
                        }
                    }

                    HX_RELEASE(pDomain);
                    HX_RELEASE(pPath);

                    ulTempPos = 0;
                    bValidCookie = FALSE;
                }
            }
            pFinal[ulFinalPos++] = '\0';
            pFinalBuf->SetSize(ulFinalPos);

            // Add it to the request headers to replace the earlier one
            pRequestHdrs->SetPropertyCString("Cookie", pFinalBuf);
            HX_RELEASE(pFinalBuf);

            HX_VECTOR_DELETE(pTemp);
        }
        HX_RELEASE(pCookie);
    }
    HX_RELEASE(pRequestHdrs);
    HX_RELEASE(pURLHost);
    HX_RELEASE(pURLPath);

    return HXR_OK;
}

HX_RESULT
CHTTPFileObject::UnmangleCookie(char* pCookie,
                                UINT32 ulCookieLen,
                                REF(IHXBuffer*) pDomain,
                                REF(HXBOOL) bIsDomain,
                                REF(IHXBuffer*) pPath)
{
    HX_RESULT hResult = HXR_OK;
    HXBOOL bValid = FALSE;
    char* pInput = NULL;
    char* pOutput = NULL;
    char* pData = NULL;
    UINT32 ulDataPos = 0;

    pDomain = NULL;
    pPath = NULL;

    // Check if the name is mangled
    if (!strncmp(pCookie, "RSG2!", 5))
    {
        pOutput = pCookie;
        pInput = pCookie + 5;

        if (*pInput != '!')
        {
            // Copy the real domain into a new buffer
	    CreateBufferCCF(pDomain, m_pContext);

            pDomain->SetSize(ulCookieLen);
            pData = (char*)pDomain->GetBuffer();

            for ( ; *pInput && *pInput != '!'; pInput++)
            {
                pData[ulDataPos++] = *pInput;
            }
            pData[ulDataPos++] = '\0';

            // Finalize the domain buffer
            pDomain->SetSize(ulDataPos);

            if (*pInput && ulDataPos > 1)
            {
                // Skip the ending '!'
                pInput++;

                // Find out if the domain string we just
                // retrieved was a domain or a host
                if (*pInput && *(pInput + 1) && *(pInput + 2))
                {
                    if (*pInput == '0')
                    {
                        bIsDomain = FALSE;
                    }
                    else
                    {
                        bIsDomain = TRUE;
                    }

                    // Skip the digit and ending '!'
                    pInput += 2;

                    // Copy the cookie name into the output buffer
                    for ( ; *pInput && *pInput != '='; pInput++)
                    {
                        *pOutput++ = *pInput;
                    }

                    *pOutput++ = '=';

                    if (*pInput)
                    {
                        // Skip the ending '='
                        pInput++;

                        // Make sure the value is mangled
                        if (!strncmp(pInput, "RSG2!", 5))
                        {
                            pInput += 5;

                            // Copy the real path into a new buffer
			    CreateBufferCCF(pPath, m_pContext);

                            pPath->SetSize(ulCookieLen);
                            pData = (char*)pPath->GetBuffer();

                            ulDataPos = 0;
                            for ( ; *pInput && *pInput != '!'; pInput++)
                            {
                                pData[ulDataPos++] = *pInput;
                            }
                            pData[ulDataPos++] = '\0';

                            // Finalize the path buffer
                            pPath->SetSize(ulDataPos);

                            if (*pInput && ulDataPos > 1)
                            {
                                // Skip the ending '!'
                                pInput++;

                                // Copy the cookie value into the output buffer
                                for ( ; *pInput; pInput++)
                                {
                                    *pOutput++ = *pInput;
                                }

                                // NULL terminate the final output buffer
                                *pOutput++ = '\0';

                                // If we made it this far, we must have a
                                // valid cookie, domain, and path
                                bValid = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!bValid)
    {
        HX_RELEASE(pDomain);
        HX_RELEASE(pPath);
    }

    return (bValid ? HXR_OK : HXR_FAIL);
}

//
// Method: CompareDomains
//
// Description: Compares the test domain against the URL domain in accordance
//     with the HTTP Cookie specification. If the two match, returns HXR_OK.
//
HX_RESULT
CHTTPFileObject::CompareDomains(IHXBuffer* pURLHost,
                                IHXBuffer* pTestDomain,
                                HXBOOL bIsDomain)
{
    HX_RESULT hResult = HXR_OK;

    if (bIsDomain)
    {
        CHXString cDomainCopy((const char*)pTestDomain->GetBuffer());

        CHXString cHostCopy((const char*)pURLHost->GetBuffer());

        // Now we compare the domain (from the cookie itself) with
        // the rightmost characters of the host. For instance,
        // a domain of ".bar.com" would match with a host (passed in)
        // of "foo.bar.com", "www.bar.com", etc. but would NOT match
        // a host of "bar.com".
        CHXString cHostRight = cHostCopy.Right(cDomainCopy.GetLength());

        if (cHostRight != cDomainCopy)
        {
            // No match
            hResult = HXR_FAIL;
        }
    }
    else if (strcasecmp((const char*)pTestDomain->GetBuffer(),
                        (const char*)pURLHost->GetBuffer()))
    {
        // Hostname matchup failed
        hResult = HXR_FAIL;
    }

    return hResult;
}

//
// Method: ComparePaths
//
// Description: Compares the test path against the URL path in accordance
//     with the HTTP Cookie specification. If the two match, returns HXR_OK.
//
HX_RESULT
CHTTPFileObject::ComparePaths(IHXBuffer* pURLPath,
                              IHXBuffer* pTestPath)
{
    // Shorter strings always come last so there can be no ambiquity
    if (pTestPath &&
        !strncmp((const char*)pURLPath->GetBuffer(),
                 (const char*)pTestPath->GetBuffer(),
                 pTestPath->GetSize() - 1))
    {
        return HXR_OK;
    }

    return HXR_FAIL;
}

//
// Method: GetHostAndPath
//
// Description: Parses the URL in the given request object into a domain/host
//     and path, in accordance with the HTTP Cookie specification.
//
HX_RESULT
CHTTPFileObject::GetHostAndPath(IHXRequest* pRequest,
                                REF(IHXBuffer*) pHostStr,
                                REF(IHXBuffer*) pPathStr)
{
    HX_RESULT hResult = HXR_OK;
    const char* pURL = NULL;

    hResult = pRequest->GetURL(pURL);
    if (SUCCEEDED(hResult) && pURL)
    {
        CHXURL cURL(pURL, m_pContext); //XXXLCM urlrep?
        hResult = cURL.GetLastError();
        if (SUCCEEDED(hResult))
        {
            IHXBuffer* pHost = NULL;

            IHXValues* pHeader = cURL.GetProperties();
            if (pHeader)
            {
                hResult = pHeader->GetPropertyBuffer(PROPERTY_HOST, pHost);
                if (SUCCEEDED(hResult))
                {
                    IHXBuffer* pFullPath = NULL;

                    hResult = pHeader->GetPropertyBuffer(PROPERTY_FULLPATH, pFullPath);

                    if (SUCCEEDED(hResult))
                    {
                        HX_RELEASE(pHostStr);
                        pHostStr = pHost;
                        pHostStr->AddRef();

                        HX_RELEASE(pPathStr);
                        pPathStr = pFullPath;
                        pPathStr->AddRef();
                    }
                    HX_RELEASE(pFullPath);
                }
                HX_RELEASE(pHost);
            }
            else
            {
                hResult = HXR_FAIL;
            }
            HX_RELEASE(pHeader);
        }
    }
    else
    {
        hResult = HXR_INVALID_PARAMETER;
    }

    return hResult;
}

HX_RESULT
CHTTPFileObject::DePerplexBuffer(IHXBuffer* pInput,
                                 REF(IHXBuffer*) pOutput)
{
    HX_RESULT hResult = HXR_OK;
    CHXPerplex perplex;

    hResult = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
        (void**)&pOutput);
    if (SUCCEEDED(hResult))
    {
        hResult = perplex.DePerplex(pInput, pOutput);
    }

    return hResult;
}

HX_RESULT CHTTPFileObject::HandleSocketRead(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pBuffer)
    {
        // Clear the return value
        retVal = HXR_OK;

        // There are several points in this function (like when we call
        // HandleFail()) that we can be destructed due to our last ref
        // being released. So we'll AddRef() ourselves here and Release()
        // ourselves at the end to prevent crashes due to early cleanup.
        AddRef();

        // XXXMEH - needed anymore, now that we've gone to
        // new socket API?
        m_bTCPReadPending = FALSE;

        UINT32 ulSize = (pBuffer ? pBuffer->GetSize() : 0);

        if (SUCCEEDED(status) && ulSize > 0)
        {
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
            if (!m_bHaveReportedSupByteRngs)
            {
                // /In case this was tried too early at init time, do it now:
                SetSupportsByteRanges(m_bSupportsByteRanges);
            }

            UINT32 ulCurTime = HX_GET_BETTERTICKCOUNT();

            // /ulSize more bytes were read, so, if it's been more than the
            // defined interval since the last report, have the file format
            // translate total bytes so far into the associated duration.
            // And/or if the known filesize has been met, report download
            // completed:
            HXBOOL bDownloadIsComplete = (m_bKnowContentSize  &&
                    m_nContentRead+ulSize >= m_nContentSize);
            if (!bDownloadIsComplete && m_bKnowHTTPResponseContentSize)
            {
                bDownloadIsComplete = m_ulHTTPResponseContentRead + ulSize >=
                        m_ulHTTPResponseContentSize;
            }
            if (m_pBytesToDur  &&
                    (0==m_ulTimeOfLastBytesToDur  ||
                    (ulCurTime - m_ulTimeOfLastBytesToDur >
                    m_ulStatusUpdateGranularityInMsec)  ||
                    bDownloadIsComplete))
            {
                m_ulTimeOfLastBytesToDur = ulCurTime;
                // /Get The FF to convert the bytes to associated dur,
                // then notify observers if it changed:
                m_ulFileSizeSoFar = ulSize+m_ulCurrentReadPosition +
                        _GetContiguousLength();
                m_pBytesToDur->ConvertFileOffsetToDur(
                        m_ulFileSizeSoFar,
                        // /It's OK if the following is ...UNKNOWN...:
                        m_ulPrgDnTotalFileSize,
                        /*REF*/ m_ulCurrentDurOfBytesSoFar);
                
                ReportCurrentDurChanged();

                // /In case FF didn't know its duration yet when we asked
                // it at init time, or maybe its duration changes over time as
                // it recalculates when it has more data, so ask it again.
                // The FF will know its complete-file's duration from either
                // the file's header info (if any) or from some other method:
                m_pBytesToDur->GetFileDuration(
                        // /OK if this is still HX_PROGDOWNLD_UNKNOWN_FILE_SIZE,
                        // but if we know it from the URL parameter "?filesize=...":,
                        // then we're passing that knowledge to the ff here:
                        m_ulPrgDnTotalFileSize,
                        /*REF*/ m_ulPrgDnTotalFileDur);
                if (m_ulPriorReportedTotalFileDur != m_ulPrgDnTotalFileDur)
                {
                    ReportTotalDurChanged();
                }

                // /If we haven't yet reported download complete and file size
                // has been reached, notify observers:
                if (!m_bDownloadCompleteReported  &&  bDownloadIsComplete)
                {
                    ReportDownloadComplete();
                }
            }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

            // If we've already read the header. then we
            // know we can just slap the contents onto the
            // end of the results buffer.
            if (m_bReadHeaderDone)
            {
                retVal = _SetDataJustDownloaded((const char*)pBuffer->GetBuffer(), ulSize);

                if (m_bKnowContentSize && m_nContentRead >= m_nContentSize)
                {
                    // XXX HP
                    // set m_bReadContentsDone to TRUE whenever we have
                    // read to the end of the content regardless of the seek
                    // since m_bReadContentDone will be reset to FALSE when
                    // seek occurs

                    // xxxbobclark if there are several HTTP objects writing
                    // to a single chunky res (like a surestream file or
                    // a situation involving ram or smil with multiple references
                    // to the same HTTP-streamed media) it's possible that the
                    // file may be completed at a "surprising" time -- i.e.
                    // without this object really knowing about it. So we'll
                    // only mark it done if we can verify that the whole chunky
                    // res has been filled, or if there's only a single cursor
                    // writing to it.

                    if ((_GetContiguousLength()
                        + m_ulCurrentReadPosition >= m_nContentSize)
                        || (m_pChunkyRes->CountCursors() == 1))
                    {
                        HXLOGL1(HXLOG_HTTP, "HandleSocketRead finished reading content (%lu)", m_nContentSize);
                        SetReadContentsDone(TRUE);
                    }
                }
                else if (m_bKnowHTTPResponseContentSize &&
                        m_ulHTTPResponseContentRead >= m_ulHTTPResponseContentSize)
                {
                    // this else clause should only be hit if the response was
                    // gzip encoded.

                    HX_ASSERT(m_bEncoded);

                    HXLOGL1(HXLOG_HTTP, "_HandleSuccess finished reading encoded content");
                    SetReadContentsDone(TRUE);
                }
            }
            else /* if (m_bReadHeaderDone) */
            {
                retVal = HandleHeaderRead(pBuffer);
                if (FAILED(retVal))
                {
                    goto cleanup;
                }
            }
        }
        else /* if (SUCCEEDED(status) && ulSize > 0) */
        {
            // XXXBJP : Old behaviour to support Mac TCP bug (where initial
            // read returns no data, but subsequent OK) removed since it
            // broke httpfsys. If MacTCP bug still needs to be handled, then
            // MACTCP-only implementation should handle it.
            SetReadContentsDone(TRUE);

            if(m_bKnowContentSize && m_nContentRead < m_nContentSize)
            {
                retVal = HXR_SERVER_DISCONNECTED;
            }
            else if (m_bKnowHTTPResponseContentSize &&
                    m_ulHTTPResponseContentRead < m_ulHTTPResponseContentSize)
            {
                // this should only happen with gzip content
                HX_ASSERT(m_bEncoded);

                retVal = HXR_SERVER_DISCONNECTED;
            }
            else
            {
                // an error was recevied in ReadDone(), so nothing more is coming from the server.
                // Have we parsed the header yet??
                if (!m_bReadHeaderDone)
                {
                    // We couldn't recognize the headers, so fail.
                    retVal = _HandleFail(400);
                }
                else
                {
                    // either we read all the data we knew we had to, or we didn't
                    // know how much data to read in the first place (!m_bKnowContentSize)
                    // If we didn't know content size, we have to assume we're ok.

                    retVal = HXR_OK;
                }
            }
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
            // /Done reading so do one last byte conversion request:
            if (m_pBytesToDur)
            {
                // /Get The FF to convert the bytes to associated dur,
                // and the FF will notify its observers of the dur
                // as well as that the download completed:
                m_ulFileSizeSoFar = m_ulCurrentReadPosition + ulSize +
                        _GetContiguousLength();
                
                UINT32 ulPrevDurOfBytesSoFar = m_ulCurrentDurOfBytesSoFar;

                m_pBytesToDur->ConvertFileOffsetToDur(
                        m_ulFileSizeSoFar,
                        m_ulPrgDnTotalFileSize,
                        /*REF*/ m_ulCurrentDurOfBytesSoFar);

                ReportCurrentDurChanged();

                m_pBytesToDur->GetFileDuration(m_ulPrgDnTotalFileSize,
                        /*REF*/ m_ulPrgDnTotalFileDur);
                if (m_ulPriorReportedTotalFileDur != m_ulPrgDnTotalFileDur)
                {
                    ReportTotalDurChanged();
                }

                // /If we haven't yet reported download complete or if we have but
                // dur has changed, then do notify observers:
                if (!m_bDownloadCompleteReported  ||
                        ulPrevDurOfBytesSoFar != m_ulCurrentDurOfBytesSoFar)
                {
                    ReportDownloadComplete();
                }
            }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
        }

        HXLOGL4(HXLOG_HTTP, "Network:    Read %5d of %5d bytes of '%s'",
               m_nContentRead,
               m_bKnowContentSize ? m_nContentSize : 0,
               m_pFilename);

        if (m_pCacheEntry)
        {
            CacheSupport_ReadDone();
        }

        // Saving a copy of the file if appropriate
        if(m_bSaveToFile)
        {
            ASSERT( !m_SaveFileName.IsEmpty() );
            // FIX  This is SUCH bad code.
            if( m_nContentRead )
            {
                ULONG32 ulActual = 0;
                ULONG32 ulCurBlockSize = m_nContentRead-m_ulOffset;
                char *pContent = new char[ ulCurBlockSize  ];

                m_pChunkyRes->GetData( m_ulOffset, pContent, ulCurBlockSize , &ulActual );
                FILE *pFile = ::fopen( m_SaveFileName, "ab" );//opens for binary mode writing at end of file
                if( pFile )
                {

                    ::fwrite( pContent, sizeof(char), ulActual, pFile );
                    ::fclose( pFile );
                }
                delete pContent;
                m_ulOffset=m_nContentRead;
            }
            //m_bSaveToFile = FALSE;
        }
cleanup:
        if(SUCCEEDED(m_LastError))
        {
            m_LastError = retVal;

            if (FAILED(retVal) && m_pCallback && !m_pCallback->IsCallbackPending())
            {
                m_pCallback->ScheduleRelative(m_pScheduler, 0);
            }
        }

        // It's now safe to clean ourselves up if our last reference
        // was released within this function.
        Release();
    }

    return retVal;
}


HX_RESULT CHTTPFileObject::ContainsMP3MimeType(const char* pData, UINT32 ulLength)
{
    UINT32 ulIdx;
    HXBOOL bRetVal = FALSE;

    const char* const pzFileMimeTypes[] =
    {
        "audio/mp3",
        "audio/x-mp3",
        "audio/mpeg",
        "audio/x-mpeg"
    };

    for (ulIdx = 0; 
         ulIdx < (sizeof(pzFileMimeTypes) / sizeof(pzFileMimeTypes[0]));
         ulIdx++)
    {
        if (StrNStr(pData, pzFileMimeTypes[ulIdx], ulLength, strlen(pzFileMimeTypes[ulIdx])) != 0)
        {
            bRetVal = TRUE;
            break;
        }
    }

    return bRetVal;
}

HX_RESULT CHTTPFileObject::HandleHeaderRead(IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    m_bInHandleHeaderRead = TRUE;

    if (pBuffer)
    {
        HXLOGL4(HXLOG_HTTP, "Network: HandleHeaderRead buffer size %lu", pBuffer->GetSize());
        // Clear the return value
        retVal = HXR_OK;

        // Get the buffer
        char* szHeader = (char*) pBuffer->GetBuffer();
        UINT32 ulLength = pBuffer->GetSize();
        
        // Assume this is not Shoutcast
        if (!m_pLastHeader)
        {
            m_bShoutcast = FALSE;
            m_ulMinStartupLengthNeeded = HTTP_MIN_STARTUP_LENGTH_NEEDED;
            m_ulMetaDataGap = 0;
        }

        // Concatenate previous header data with new
        if (m_pLastHeader)
        {
            retVal = pBuffer->SetSize(ulLength + m_ulLastHeaderSize);
            szHeader = (char*) pBuffer->GetBuffer();

            if (SUCCEEDED(retVal))
            {
                // Move New Data to make room for insertion of previous header data
                memmove(szHeader + m_ulLastHeaderSize,
                        szHeader,
                        ulLength);

                // Insert previous header data
                memcpy(szHeader, /* Flawfinder: ignore */
                       m_pLastHeader,
                       m_ulLastHeaderSize);

                ulLength += m_ulLastHeaderSize;
            }

            m_ulLastHeaderSize = 0;
            HX_VECTOR_DELETE(m_pLastHeader);
        }
            
        // If there is no HTTP/ at the beginning of the response message,
        // add a default OK response code. The popular browsers all handle
        // this case, so we should too.
        if (ulLength > 4)
        {
            // XXX HP
            // Darwin HTTP server returns RTSP response on a HTTP request?!
            if (0 == strncasecmp((const char*)szHeader, "RTSP/", 5))

            {
                retVal = _HandleFail(400);
                goto cleanup;
            }

            
            if (strncasecmp((const char*)szHeader, "HTTP/", 5) != 0)
            {
                // If we have a non-standard response, we'll need more data
                // to help us determine how to construct the standard
                // header.
                m_ulMinStartupLengthNeeded = HTTP_NONSTANDARD_MIN_STARTUP_LENGTH_NEEDED;

                HXLOGL1(HXLOG_HTTP, "Non-standard Header (no HTTP prefix) in http response.");
            }

            // The Darwin server streams mp3 using ShoutCast (or at least they
            // they use the ShoutCast mechanism for including meta data in
            // the mp3 stream).  So, check the header for "icy-metaint", and
            // enable ShoutCast mode.

            // only way to determine if it's a shoutcast server
            if ( (strncasecmp((const char*)szHeader, "ICY", 3) == 0) ||
                 (StrNStr((const char*)szHeader,"icy-metaint", ulLength, 11))

               )
            {
                m_bShoutcast = TRUE;
                m_ulMinStartupLengthNeeded = HTTP_NONSTANDARD_MIN_STARTUP_LENGTH_NEEDED;
            }
        }

        if ((ulLength > 4) && (ulLength >= m_ulMinStartupLengthNeeded))
        {
            if (strncasecmp((const char*)szHeader, "HTTP/", 5) != 0 || m_bShoutcast)
            {
                const char pDefHdr[] = "HTTP/1.1 200 OK\r\n\r\n";
                UINT32 ulDefHdrSize = sizeof(pDefHdr) - 1;
                UINT32 ulHdrShift = ulDefHdrSize;
                char* pHdr = (char*)pDefHdr;

                // Use heuristic to decide whether to treat this non-standard 
                // HTTP stream as a non-MP3 icecast stream.
                // The reason for differentiation between MP3 streams and other 
                // datatypes (e.g. AAC) is due to legacy treatment of MP3 which used 
                // to equate shoutcast or icecast streams to MP3.
                // In this old implementation, the response headers were treated as
                // content body and sent to mp3 file format for parsing.
                // While this implementation remains for MP3, icecast stream
                // headers for other datatypes will be parsed so that content-type
                // can be properly discovered and thus data stream properly routed
                // to the file format for parsing.
                // The new file format will be required to obtain the needed header
                // information from response headers.
                if (m_bShoutcast &&
                    (strncasecmp((const char*) szHeader, "ICY", 3) == 0) &&
                    ((StrNStr((const char*) szHeader,
                              "Content-Type", ulLength, 12) != 0) ||
                     (StrNStr((const char*) szHeader,
                              "content-type", ulLength, 12) != 0)) &&
                    (!ContainsMP3MimeType((const char*) szHeader, ulLength)))
                {
                    // This is an non-mp3 Icecast stream. 
                    // Modify the ulDefHdrSize so that it will turn
                    // ICY 200 OK   into   HTTP/1.1 200 OK
                    ulDefHdrSize = 8;
                    ulHdrShift = 5;
                }

                // If we had an http packet that was just a header, concatenate
                // the current packet to it.
                if (m_pLastHeader)
                {
                    pHdr = m_pLastHeader;
                    ulDefHdrSize = m_ulLastHeaderSize;
                    ulHdrShift = ulDefHdrSize;
                }

                // Grow Buffer...
                HX_RESULT res = pBuffer->SetSize(pBuffer->GetSize()+ulHdrShift);

                HX_ASSERT(SUCCEEDED(res));

                if (SUCCEEDED(res))
                {
                    // Move Existing Data
                    memmove(pBuffer->GetBuffer() + ulHdrShift,
                            pBuffer->GetBuffer(),
                            ulLength);

                    // Insert Default Response Header
                    memcpy(pBuffer->GetBuffer(), /* Flawfinder: ignore */
                           (const char*)pHdr,
                           ulDefHdrSize);
                }

                HX_VECTOR_DELETE(m_pLastHeader);

                szHeader = (char*)pBuffer->GetBuffer();
                ulLength = pBuffer->GetSize();
            }
        }

        HTTPParser Parser(m_pContext);
        HTTPResponseMessage* pMessage = NULL;

        UINT32 ulHeaderLength = ulLength;

        // Parse headers from message
        //
        if (ulLength >= m_ulMinStartupLengthNeeded)
        {
#if !defined HELIX_FEATURE_SERVER
            pMessage = (HTTPResponseMessage*) Parser.parse((const char*) szHeader,
                                                           ulHeaderLength);
#else
            BOOL bMsgTooLarge = FALSE;
            pMessage = (HTTPResponseMessage*) Parser.parse((const char*) szHeader,
                                                           ulHeaderLength, bMsgTooLarge);
#endif /* !HELIX_FEATURE_SERVER */
        }

        if (pMessage &&
            (pMessage->tag() == HTTPMessage::T_RESP))
        {
            // We now have the entire header!
            //
            m_bReadHeaderDone = TRUE;

            if (pMessage->majorVersion() <= 1 &&
                pMessage->minorVersion() == 0)
            {
                m_bHTTP1_1 = FALSE;
            }

            HX_VECTOR_DELETE(m_pLastHeader);

            // setup the MIME-type appropriately for shoutcast situations
            if (m_bShoutcast)
            {
                UINT32 ulIcyMetaInt = 0;
                CHXString sMimeType = pMessage->getHeaderValue("content-type");
                if (!sMimeType.GetLength())
                {
                    pMessage->addHeader("content-type", "audio/rn-mpeg");
                }

                if (pMessage->getHeaderValue("icy-metaint", ulIcyMetaInt))
                {
                    m_ulMetaDataGap = ulIcyMetaInt;
                    m_ulNextMetaDataStartIdx = m_ulMetaDataGap;
                }
            }

            IUnknown*   pUnknown = NULL;
            IHXKeyValueList* pResponseHeaders = NULL;

            if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList, (void**)&pUnknown))
            {
                if (HXR_OK == pUnknown->QueryInterface(IID_IHXKeyValueList, (void**)&pResponseHeaders))
                {
                    MIMEHeaderValue*        pHeaderValue = NULL;
                    MIMEHeader*             pHeader = pMessage->getFirstHeader();

                    HXBOOL bNotAcceptRanges = FALSE;
                    HXBOOL bRealServer = FALSE;
                    while (pHeader)
                    {
                        pHeaderValue = pHeader->getFirstHeaderValue();
                        CHXString HeaderString;
                        while (pHeaderValue)
                        {
                            CHXString TempString;
                            pHeaderValue->asString(TempString);
                            HeaderString += TempString;
                            pHeaderValue = pHeader->getNextHeaderValue();
                            if (pHeaderValue)
                            {
                                HeaderString += ", ";
                            }
                        }

                        IHXBuffer *pTmpBuffer = NULL;
                        CHXBuffer::FromCharArray((const char*)HeaderString, &pTmpBuffer);
                        pResponseHeaders->AddKeyValue(pHeader->name(),pTmpBuffer);

                        // Disable http 1.1 support if the server explicitly says to
                        if (!strcmpi(pHeader->name(), "Accept-Ranges"))
                        {
                            if (!strcmpi((const char*)pTmpBuffer->GetBuffer(), "none"))
                            {
                                bNotAcceptRanges = TRUE;
                            }
                        }
                        else if (!strcmpi(pHeader->name(), "Server"))
                        {
                            if (!strcmpi((const char*)pTmpBuffer->GetBuffer(), REALSERVER_RESPONSE_NAME))
                            {
                                bRealServer = TRUE;
                            }
                        }

                        HX_RELEASE(pTmpBuffer);
                        pHeader = pMessage->getNextHeader();
                    }

                    /// Now the logic to decide if we want to support byte range
                    if ((bRealServer && !m_bHTTP1_1) || bNotAcceptRanges)
                    {
                        SetSupportsByteRanges(FALSE);
                    }

                    if (m_pCookies)
                    {
                        // retrieve cookies from the response header
                        // XXX showell - possible performance enhancement --
                        // you can put the call to SetCookie in the above
                        // loop, rather than iterating again through the
                        // list
                        IHXKeyValueListIterOneKey *pListIter = NULL;
                        pResponseHeaders->GetIterOneKey("Set-Cookie", pListIter);

                        IHXBuffer *pCookie = NULL;
                        while (pListIter->GetNextString(pCookie) == HXR_OK)
                        {
                            m_pCookies->SetCookies(m_pHost, m_pPath, pCookie);
                            HX_RELEASE(pCookie);
                        }
                        HX_RELEASE(pListIter);
                    }

                    if (m_pRequest)
                    {
                        // XXX showell - Maybe in the future we will able to
                        // change IHXRequest::GetResponseHeaders to return
                        // an IHXKeyValueList*, so we don't have to support
                        // old-style IHXValues.  Until then, we have to play
                        // these little games.
                        IHXKeyValueList* pKeyedResponseValues = NULL;
                        IHXValues* pResponseValues = NULL;

                        if ((HXR_OK == m_pRequest->GetResponseHeaders(pResponseValues) &&
                            pResponseValues))
                        {
                            if (HXR_OK == pResponseValues->QueryInterface(IID_IHXKeyValueList,
                                (void**)&pKeyedResponseValues))
                            {
                                const char*             pName = NULL;
                                IHXBuffer*              pValue = NULL;
                                IHXKeyValueListIter*   pListIter = NULL;

                                pKeyedResponseValues->GetIter(pListIter);
                                HX_ASSERT(pListIter);

                                // merge the previous response header to the current HTTP response
                                // excluding the ones already existing in current HTTP response
                                while (pListIter->GetNextPair(pName, pValue) == HXR_OK)
                                {
                                    if (!pResponseHeaders->KeyExists(pName))
                                    {
                                        pResponseHeaders->AddKeyValue(pName, pValue);
                                    }
                                    HX_RELEASE(pValue);
                                }
                                HX_RELEASE(pListIter);
                            }
                            HX_RELEASE(pKeyedResponseValues);
                            HX_RELEASE(pResponseValues);

                            if (HXR_OK == pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseValues))
                            {
                                retVal = m_pRequest->SetResponseHeaders(pResponseValues);
                            }
                        }
                        else
                        {
                            if (HXR_OK == pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseValues))
                            {
                                retVal = m_pRequest->SetResponseHeaders(pResponseValues);
                            }
                        }
                        HX_RELEASE(pResponseValues);
                    }
                }
                HX_RELEASE(pResponseHeaders);
            }
            HX_RELEASE(pUnknown);

            if (m_bMangleCookies)
            {
                // If cookie mangling is enabled,
                // mangle all received Set-Cookie values
                MangleAllSetCookies(m_pRequest);
            }

            UINT32 ulHTTPStatus = atoi(pMessage->errorCode());

            HXLOGL1(HXLOG_HTTP, "HandleHeaderRead: HTTP header status ==> %lu", ulHTTPStatus);

            if(pMessage->majorVersion() > 0)
            {
                switch(ulHTTPStatus)
                {
                    case 206: // Success. Partial data.
                        m_bPartialData = TRUE;
                        if (m_bCheckingWhetherByteRangeWorks)
                        {
                            m_bCheckingWhetherByteRangeWorks = FALSE;
                            // woo hoo, the byte range seek was successful
                            // xxxbobclark there are servers that advertise
                            // "accept-ranges: bytes" but DON'T! That's why
                            // we must verify that byte ranges work!

#ifdef _IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG
                            SetSupportsByteRanges(FALSE);
#else  //_IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG
                            HXLOGL2(HXLOG_HTTP, "Confirmed that server supports byte ranges!");
                            SetSupportsByteRanges(TRUE);
#endif //_IIS_HTTP_SERVER_NO_SEEK_SUPPORT_BUG

                            // Confirm the byte offset seek worked
                            if (m_bByteRangeSeekPending)
                            {
                                m_bByteRangeSeekPending = FALSE;
                                m_bSeekPending = FALSE;

                                m_uByteRangeSeekOffset = 0;
                                Seek(m_ulCurrentReadPosition, FALSE);
                            }
                        }
                        // No break, intentional fall-through

                    case 200: // Success
                        {
                            if (m_bCheckingWhetherByteRangeWorks)
                            {
                                // xxxbobclark Oh no, we've stumbled across
                                // one of the servers that doesn't really
                                // support byte ranges. So what we'll do is
                                // revert so httpfsys does NOT support byte
                                // ranges for this guy, and reset m_nContentRead
                                // to match what we're expecting with what we're
                                // getting.

                                // turn off byte range support for this file
                                m_bCheckingWhetherByteRangeWorks = FALSE;
                                m_bServerPresumablyWorksWithByteRangeRequests = FALSE;
                                HXLOGL1(HXLOG_HTTP, "SERVER CLAIMS BYTE RANGE SUPPORT BUT DOESN'T REALLY! (%s)", NULLOK(m_pFilename));
                                SetSupportsByteRanges(FALSE);

                                // The byte offset seek failed so the server will give
                                // us the data at byte offset 0. Reset our read/write
                                // pointers to offset 0.
                                m_nContentRead = 0;
                                m_ulHTTPResponseContentRead = 0;

                                if (m_bByteRangeSeekPending)
                                {
                                    m_bByteRangeSeekPending = FALSE;
                                    m_uByteRangeSeekOffset = 0;

                                    // Turn this failed byte-ranged seek into a pending
                                    // 1.0 seek (wait for the data to be downloaded) if
                                    // we were advised to in the Advise method.
                                    if (m_bConvertFailedSeeksToLinear)
                                        m_bSeekPending = TRUE;
                                    else
                                    {
                                        m_bSeekPending = FALSE;
                                        m_pFileResponse->SeekDone(HXR_FAILED);
                                    }
                                }

                                SetReadContentsDone(FALSE);
                            }
                            retVal = _HandleSuccess(pMessage, pBuffer, ulHeaderLength);
                        }
                        break;
                    case 400: // Fail
                    case 404: // Not Found
                        {
                            retVal = _HandleFail(ulHTTPStatus);
                        }
                        break;
                    case 401: // Not Authorized
                    case 407: // Proxy Authentication Required
                        {
                            retVal = _HandleUnAuthorized(pMessage, pBuffer, ulHeaderLength);
                        }
                        break;
                    case 301: // Redirect
                    case 302: // Redirect
                        {
                            retVal = _HandleRedirect(pMessage);
                        }
                        break;

                    case 416: // Invalid range request
                        m_LastError = HXR_INVALID_PARAMETER;
                        m_pFileResponse->SeekDone(HXR_FAILED);
                        retVal = _HandleFail(ulHTTPStatus);
                        break;

                    default:
                        {
                            retVal = _HandleFail(400);
                        }
                        break;
                };
            }
        }

        // If the http packet is just the header, save for the next packet
        else if (ulLength == ulHeaderLength)
        {
            m_pLastHeader = new char[ulLength];
            if (m_pLastHeader)
            {
                memcpy(m_pLastHeader, szHeader, ulLength);
                m_ulLastHeaderSize = ulLength;
            }
            else
            {
                retVal = HXR_OUTOFMEMORY;
            }
        }
        delete pMessage;
    }

cleanup:
    
    m_bInHandleHeaderRead = FALSE;
    
    return retVal;
}

void
CHTTPFileObject::ReportConnectionFailure()
{
    if (m_pErrorMessages && m_pFilename)
    {
        HXLOGL2(HXLOG_HTTP, "Connection Failure! %s", m_pFilename);
        char* pTemp = NULL;

        int lenTemp = strlen(m_pFilename) + 256;
        pTemp = new char[lenTemp];
        SafeSprintf(pTemp, lenTemp,/* Flawfinder: ignore */
            "HTTPFSys: Failed to connect to the server at "
            "the following URL: %s.",
            m_pFilename);

        m_pErrorMessages->Report(HXLOG_ERR,
                                 HXR_FAIL,
                                 HXR_OK,
                                 pTemp,
                                 NULL);
        HX_VECTOR_DELETE(pTemp);
    }
}

void
CHTTPFileObject::ReportConnectionTimeout()
{
    if (m_pErrorMessages && m_pFilename)
    {
        HXLOGL2(HXLOG_HTTP, "Connection Timeout %s", m_pFilename);
        char* pTemp = NULL;
        int lenTemp = strlen(m_pFilename) + 256;
        pTemp = new char[lenTemp];
        SafeSprintf(pTemp, lenTemp,/* Flawfinder: ignore */
            "HTTPFSys: Failed to retrieve the following "
            "URL because the web server could not be contacted "
            "within %lu seconds: %s.",
            m_nConnTimeout / 1000, m_pFilename);

        m_pErrorMessages->Report(HXLOG_ERR,
                                 HXR_NET_CONNECT,
                                 HXR_OK,
                                 pTemp,
                                 NULL);
        HX_VECTOR_DELETE(pTemp);
    }
}

void
CHTTPFileObject::ReportServerTimeout()
{
    if (m_pErrorMessages && m_pFilename)
    {
        if (m_bOnServer)
        {
            char* pTemp = NULL;
                int lenTemp = strlen(m_pFilename) + 256;
            pTemp = new char[lenTemp];
            SafeSprintf(pTemp, lenTemp,/* Flawfinder: ignore */
                "HTTPFSys: Failed to retrieve the following "
                "URL because the web server stopped sending data "
                "for more than %lu seconds: %s.",
                m_nServerTimeout / 1000, m_pFilename);

            m_pErrorMessages->Report(HXLOG_ERR,
                                     HXR_SERVER_TIMEOUT,
                                     HXR_OK,
                                     pTemp,
                                     NULL);
            HX_VECTOR_DELETE(pTemp);
        }
        else
        {
            HXLOGL2(HXLOG_HTTP, "Server timeout %s", m_pFilename);
            // The client will know how to present this to the user
            m_pErrorMessages->Report(HXLOG_ERR,
                                     HXR_SERVER_TIMEOUT,
                                     HXR_OK,
                                     NULL,
                                     NULL);
        }
    }
}

void
CHTTPFileObject::ReportDocumentMissing()
{
    if (m_pErrorMessages && m_pFilename)
    {
        HXLOGL2(HXLOG_HTTP, "Document Missing: %s", m_pFilename);
        char* pTemp = NULL;
        int lenTemp = strlen(m_pFilename) + 256;
        pTemp = new char[lenTemp];
        SafeSprintf(pTemp, lenTemp,/* Flawfinder: ignore */
            "HTTPFSys: Web server reported 'File Not Found' "
            "for the following URL: %s.",
            m_pFilename);

        m_pErrorMessages->Report(HXLOG_ERR,
                                 HXR_DOC_MISSING,
                                 HXR_OK,
                                 pTemp,
                                 NULL);
        HX_VECTOR_DELETE(pTemp);
    }
}

void
CHTTPFileObject::ReportGeneralFailure()
{
    if (m_pErrorMessages && m_pFilename)
    {
        HXLOGL2(HXLOG_HTTP, "General Failure: %s", m_pFilename);
        char* pTemp = NULL;
        int lenTemp = strlen(m_pFilename) + 256;
        pTemp = new char[lenTemp];
        SafeSprintf(pTemp, lenTemp,/* Flawfinder: ignore */
            "HTTPFSys: An error occurred while attempting "
            "to retrieve the following URL: %s.",
            m_pFilename);

        m_pErrorMessages->Report(HXLOG_ERR,
                                 HXR_FAIL,
                                 HXR_OK,
                                 pTemp,
                                 NULL);
        HX_VECTOR_DELETE(pTemp);
    }
}


// consolidated from a couple places (HandleSocketRead and HandleSuccess)
// that both perform an identical task...

HX_RESULT
CHTTPFileObject::_SetDataJustDownloaded(const char* pRawBuf, UINT32 ulLength)
{
    HX_RESULT theErr = HXR_OK;

    // first check for chunked-encoding, THEN check for gzip encoding. If
    // content is both gzip-encoded AND chunked, then it must be de-chunked
    // before it's ungzipped.

    if (m_bChunkedEncoding)
    {
        if (!m_pChunkedEncoding)
        {
            m_pChunkedEncoding = new HTTPChunkedEncoding;
            m_pChunkedEncoding->size = 0;
            m_pChunkedEncoding->read = 0;
            m_pChunkedEncoding->maxChunkSizeAccepted = GetMaxChunkSizeAccepted ();
            m_pChunkedEncoding->lastchunk = FALSE;
            m_pChunkedEncoding->state = CE_HEADER_PENDING;
            m_pChunkedEncoding->buf = new char[DEFAULT_CHUNK_SIZE];
            m_pChunkedEncoding->buf_size = m_pChunkedEncoding->buf?DEFAULT_CHUNK_SIZE:0;
        }

        theErr = DecodeChunkedEncoding(m_pChunkedEncoding, pRawBuf, ulLength);
    }
    else if (m_bEncoded)
    {
        theErr = _SetDataGzipEncoded(pRawBuf, ulLength);
    }
    else
    {
        // plain old vanilla http content, non-chunked, non-gzipped.
        theErr = _SetDataPlain(pRawBuf, ulLength);
    }
    return theErr;
}

HX_RESULT
CHTTPFileObject::_SetDataPlain(const char* pRawBuf, UINT32 ulLength)
{
    UINT32 ulThisBufferMetaStartIdx;
    HX_RESULT retVal = HXR_OK;

    // Loop over buffer handling (content-data, meta-data) chunks in pairs
    do
    {
        ulThisBufferMetaStartIdx = ulLength;    // assume meta data not present
        
        if (m_ulMetaDataGap != 0)
        {
            // If we are currently consuming meta block, set start of meta index
            // to the beginning of the buffer.
            // Otherwise, if content buffer we are about to read contains the start 
            // of meta-data determine the size of meta data to read.
            if (m_ulMetaDataBlockRemainingSize != 0)
            {
                // Continue consuming meta-data
                ulThisBufferMetaStartIdx = 0;
            }
            else if (((LONG32) (m_nContentRead + ulLength - m_ulNextMetaDataStartIdx)) > 0)
            {
                // Setup for new meta data chunk
                HX_ASSERT(((LONG32) (m_ulNextMetaDataStartIdx - m_nContentRead)) >= 0);
                
                if (((LONG32) (m_ulNextMetaDataStartIdx - m_nContentRead)) >= 0)
                {
                    // Determine start of data in this buffer
                    ulThisBufferMetaStartIdx = (m_ulNextMetaDataStartIdx - m_nContentRead);
                    // Determine the meta-data chunk size
                    m_ulMetaDataBlockSize = 1 +
                        ((UINT8) pRawBuf[ulThisBufferMetaStartIdx]) * ICECAST_META_SIZE_MULTIPLE;
                    // Reset the meta-data chunk remaining size
                    m_ulMetaDataBlockRemainingSize = m_ulMetaDataBlockSize;
                    // Compute the start idx (0 based) of the next meta-data chunk
                    m_ulNextMetaDataStartIdx += m_ulMetaDataGap;
                }
                else
                {
                    // This should never happen.
                    // If it does, we turn off meta-data extraction since it 
                    // isn't working properly.
                    m_ulMetaDataGap = 0;
                }
            }
        }
        
        // Queue content prior to meta-data
        if (ulThisBufferMetaStartIdx != 0)
        {
            HXLOGL4(HXLOG_HTTP,
                    "Network: _SetDataPlain WriteChunky at %ld size %ld",
                    m_nContentRead, ulThisBufferMetaStartIdx);

            m_pChunkyRes->SetData(m_nContentRead, pRawBuf, ulThisBufferMetaStartIdx, this);
            m_nContentRead += ulThisBufferMetaStartIdx;
            m_ulHTTPResponseContentRead += ulThisBufferMetaStartIdx;

            pRawBuf += ulThisBufferMetaStartIdx;
            ulLength -= ulThisBufferMetaStartIdx;
        }
        
        // Handle Meta-data
        if (ulLength != 0)
        {
            // Determine how much of meta-data we need to extract in current buffer
            ULONG32 ulThisBufferMetaLength = 
                (m_ulMetaDataBlockRemainingSize < ulLength) ? 
                    m_ulMetaDataBlockRemainingSize : ulLength;
            
            // Update the remaining meta-data to extract in the subsequent buffers
            m_ulMetaDataBlockRemainingSize -= ulThisBufferMetaLength;
            
            // Process meta data chunk
            _SetMetaData(pRawBuf,
                         ulThisBufferMetaLength,
                         m_ulMetaDataBlockSize - m_ulMetaDataBlockRemainingSize,
                         m_ulMetaDataBlockSize);
            
            pRawBuf += ulThisBufferMetaLength;
            ulLength -= ulThisBufferMetaLength;         
        }
    } while (ulLength != 0);

    return retVal;
}

HX_RESULT
CHTTPFileObject::_SetMetaData(const char* pMetaDataSegmentBuffer,
                              UINT32 ulMetaDataSegmentSize,
                              UINT32 ulMetaDataSegmentReach,
                              UINT32 ulMetaDataSize)
{
    HXLOGL4(HXLOG_HTTP,
        "Network: _SetMetaData at %ld, segment size %ld, segment reach %ld, chunk size %ld",
        m_nContentRead, ulMetaDataSegmentSize, ulMetaDataSegmentReach, ulMetaDataSize);
    // To do: assable, parse and report meta data
    return HXR_OK;
}
    
    
HX_RESULT
CHTTPFileObject::_SetDataGzipEncoded(const char* pRawBuf, UINT32 ulLength)
{
    HX_RESULT theErr = HXR_OK;

    HX_ASSERT(m_pDecoder);

#if defined(HELIX_FEATURE_HTTP_GZIP)
    theErr = m_pDecoder->SetData(m_nContentRead, pRawBuf, ulLength);
    m_nContentRead = m_pDecoder->GetContentRead();
    m_ulHTTPResponseContentRead += ulLength;
    if (m_pDecoder->SeenIncomplete()
        && m_nOriginalContentSize
        && ulLength >= m_nOriginalContentSize)
    {
        // If we read all the data the header told us was coming,
        // we're not incomplete, we failed.
        theErr = HXR_FAIL;
    }
 
    if (FAILED(theErr))
    {
        SetReadContentsDone(TRUE);
    }
#else
    HXLOGL1(HXLOG_HTTP, "gzip content found when we didn't request it!");
    theErr = HXR_FAIL;
    SetReadContentsDone(TRUE);
#endif

    return theErr;
}

unsigned long
CHTTPFileObject::GetMaxChunkSizeAccepted()
{
    INT32 max = LARGEST_CHUNK_SIZE_WE_ACCEPT;
    if (m_pRegistry)
    {
        if (HXR_OK != m_pRegistry->GetIntByName(
                LARGEST_CHUNK_SIZE_WE_ACCEPT_CONFIG_KEY, max))
        {
            max = LARGEST_CHUNK_SIZE_WE_ACCEPT;
        }

        if (max < 0)
        {
          max = LARGEST_CHUNK_SIZE_WE_ACCEPT;
        }
    }

    return max;
}

#ifdef HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT

HX_RESULT
CHTTPFileObject::DecodeChunkedEncoding(HTTPChunkedEncoding*&    pChunkedEncoding,
                                       const char*              pChunk,
                                       int                      l)
{
    HX_RESULT   rc = HXR_OK;
    char*       errstr = NULL;
    unsigned long        so_far_read = 0;    
    

    // If the HTTP Chunk is split between more than one socket read and size 
    // is greater than DEFAULT_TCP_READ_SIZE (1500) , make sure we write only 
    // the data resulting from the current socket read - since the previous data
    // has already been saved in the chunky-resource.

    // Storing the size of the data so far read, so that it can be updated at the function exit
    
    if( (pChunkedEncoding->state == CE_BODY_PENDING) &&
                (pChunkedEncoding->size > DEFAULT_TCP_READ_SIZE) )
    {
        so_far_read = pChunkedEncoding->read;
        pChunkedEncoding->read = 0;
    }


    while (l > 0)
    {
        if (CE_HEADER_READY == pChunkedEncoding->state)
        {
            // parse the chunk head
            pChunkedEncoding->size = strtoul(pChunkedEncoding->buf, &errstr, 16);
            HX_ASSERT(pChunkedEncoding->size <= pChunkedEncoding->maxChunkSizeAccepted);
            if (pChunkedEncoding->size > pChunkedEncoding->maxChunkSizeAccepted)
            {
                // It is not reasonable to accept chunks of arbitrary size.
                // Although RFC 2616 sets no limits on it, we do. But since
                // the limit was chosen arbitrarily, we can revise it if required.
                rc = HXR_UNEXPECTED;
                break;
            };

            if (pChunkedEncoding->size > 0)
            {
                //Allocate enough memory to fit the size 

                if (pChunkedEncoding->size >= MAX_CHUNK_SIZE)
                {              
                    if((pChunkedEncoding->size > DEFAULT_TCP_READ_SIZE))
                    {
                        // Allocate DEFAULT_TCP_READ_SIZE memory only if 
                        // already not allocated for some previous chunk

                        if(m_ulMaxBufSize < DEFAULT_TCP_READ_SIZE)
                        {                            
                            HX_VECTOR_DELETE(pChunkedEncoding->buf);
                            pChunkedEncoding->buf = new char[DEFAULT_TCP_READ_SIZE + 1];                            
                            m_ulMaxBufSize = DEFAULT_TCP_READ_SIZE;
                        }
                        memset(pChunkedEncoding->buf, 0, DEFAULT_TCP_READ_SIZE + 1);
                    }
                    else // MAX_CHUNK_SIZE < size < DEFAULT_TCP_READ_SIZE
                    {

                        // if the current buffer cannot handle the upcoming chunk size, proceed
                        // to allcoate new buffer and update the max. size

                        if(m_ulMaxBufSize < pChunkedEncoding->size)
                        {
                            HX_VECTOR_DELETE(pChunkedEncoding->buf);
                            pChunkedEncoding->buf = new char[pChunkedEncoding->size + 1];
                            m_ulMaxBufSize = pChunkedEncoding->size;
                        }
                        memset(pChunkedEncoding->buf, 0, pChunkedEncoding->size + 1);
                    }
                }
                else
                {
                    memset(pChunkedEncoding->buf, 0, pChunkedEncoding->size + 1);
                }

                pChunkedEncoding->read = 0;
                pChunkedEncoding->state = CE_BODY_PENDING;
            }
            else if (pChunkedEncoding->size == 0)
            {
                pChunkedEncoding->lastchunk = TRUE;
                pChunkedEncoding->read = 0;
                pChunkedEncoding->state = CE_BODY_PENDING;
            }
            else
            {
                rc = HXR_FAILED;
                break;
            }
        }
        else if (CE_BODY_READY == pChunkedEncoding->state)
        {
            // parse the chunk body
            if (pChunkedEncoding->lastchunk)
            {
                HX_VECTOR_DELETE(pChunkedEncoding->buf);
                HX_DELETE(pChunkedEncoding);
                SetReadContentsDone(TRUE);
                break;
            }

            HXLOGL4(HXLOG_HTTP, "Encoding: WriteChunky at %ld size %ld",
                m_nContentRead, pChunkedEncoding->size);

            //m_pChunkyRes->SetData(m_nContentRead, pChunkedEncoding->buf, pChunkedEncoding->size, this);
            //m_nContentRead += pChunkedEncoding->size;

            m_pChunkyRes->SetData(m_nContentRead, pChunkedEncoding->buf, pChunkedEncoding->read, this);
            m_nContentRead += pChunkedEncoding->read;

            memset(pChunkedEncoding->buf, 0, MAX_CHUNK_SIZE);
            pChunkedEncoding->read = 0;
            pChunkedEncoding->state = CE_HEADER_PENDING;
        }
        else if (*pChunk == CR && *(pChunk+1) == LF)
        {
            // first CRLF marks the chunk header
            if (CE_HEADER_PENDING == pChunkedEncoding->state)
            {
                pChunkedEncoding->state = CE_HEADER_READY;
            }
            // ignore the CRLF within the chunk body while we are
            // still reading it
            else if (pChunkedEncoding->read + so_far_read< pChunkedEncoding->size)
            {
                pChunkedEncoding->buf[pChunkedEncoding->read++] = *pChunk;
            }
            // last CRLF marks the end of chunk
            else if (CE_BODY_PENDING == pChunkedEncoding->state)
            {
               // HX_ASSERT((pChunkedEncoding->read + so_far_read)== pChunkedEncoding->size);

                // Remote HTTP server is sending more data than specified by 
                // chunk-size. This may be an attack on the server, so don't 
                // accept data sent in this fashion.

                // This should NEVER happen. This condition should be caught
                // earlier, but just in case fail gracefully anyway.
                if ((pChunkedEncoding->read + so_far_read) > pChunkedEncoding->size)
                {
                    rc = HXR_UNEXPECTED;
                    break;
                }

                pChunkedEncoding->state = CE_BODY_READY;
            }
        }
        else
        {
            // chunk-size exceeded MAX_CHUNK_SIZE in length.
            // A hex-number >=1024 digits is very suspicious.

            if ((CE_HEADER_PENDING == pChunkedEncoding->state)
            &&  (pChunkedEncoding->read >= MAX_CHUNK_SIZE))
            {
                rc = HXR_UNEXPECTED;
                break;
            }

            // Remote HTTP server is sending more data than specified by 
            // chunk-size. This may be an attack on the server, so don't 
            // accept data sent in this fashion.

            else if ((CE_BODY_PENDING == pChunkedEncoding->state)
                 &&  ((pChunkedEncoding->read + so_far_read) >= pChunkedEncoding->size))
            {
                HX_ASSERT(pChunkedEncoding->size > 0);

                // If read is ever > size, we're in trouble--
                // the buffer has already been overrun.
                HX_ASSERT(pChunkedEncoding->read == pChunkedEncoding->size);

                rc = HXR_UNEXPECTED;
                break;
            }

            HX_ASSERT((CE_HEADER_PENDING == pChunkedEncoding->state) || (CE_BODY_PENDING == pChunkedEncoding->state));

            pChunkedEncoding->buf[pChunkedEncoding->read++] = *pChunk;
        }
        pChunk++, l--;
    } // end of buffer

    // Check to see whether the chunk body is still pending. If the size is greater than
    // the DEFAULT_TCP_READ_SIZE, store the processed data, in the chunky-resource.

    if( (pChunkedEncoding->state == CE_BODY_PENDING) &&
                (pChunkedEncoding->size > DEFAULT_TCP_READ_SIZE) )
    {
        m_pChunkyRes->SetData(m_nContentRead, pChunkedEncoding->buf, pChunkedEncoding->read, this);
        m_nContentRead += pChunkedEncoding->read;
        pChunkedEncoding->read += so_far_read;
        
    }

    return rc;
}

#else //HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT

HX_RESULT
CHTTPFileObject::DecodeChunkedEncoding(HTTPChunkedEncoding*&    pChunkedEncoding,
                                       const char*              pChunk,
                                       int                      l)
{
    HX_RESULT   rc                  = HXR_OK;
    char*       errstr              = NULL;
    const char* pChunkStart         = pChunk;
    HXBOOL      bProcessStateChange = FALSE;

    while (l > 0 || bProcessStateChange)
    {
        if (CE_HEADER_READY == pChunkedEncoding->state)
        {
            // parse the chunk head
            pChunkedEncoding->size = 0;
            if( pChunkedEncoding->buf )
            {
                pChunkedEncoding->size = (unsigned long)strtol(pChunkedEncoding->buf, &errstr, 16);
                if( errstr != pChunkedEncoding->buf )
                {
                    if( pChunkedEncoding->size > 0 )
                    {
                        if (pChunkedEncoding->size >= pChunkedEncoding->buf_size)
                        {
                            HX_VECTOR_DELETE(pChunkedEncoding->buf);

                            pChunkedEncoding->buf_size = pChunkedEncoding->size+1;
                            pChunkedEncoding->buf = new char[pChunkedEncoding->buf_size];
                        }
                
                        if( pChunkedEncoding->buf )
                        {
                            memset(pChunkedEncoding->buf, 0, pChunkedEncoding->buf_size);
                            pChunkedEncoding->read = 0;
                            pChunkedEncoding->state = CE_BODY_PENDING;
                        }
                        else
                        {
                            rc = HXR_OUTOFMEMORY;
                            pChunkedEncoding->buf_size = 0;
                            break;
                        }
                    }
                    else
                    {
                        pChunkedEncoding->lastchunk = TRUE;
                        pChunkedEncoding->read = 0;
                        pChunkedEncoding->state = CE_BODY_PENDING;
                    }
                    bProcessStateChange = FALSE; // we've processed the header
                }
                else
                {
                    //There was an illegal hex string where we
                    //expected to find the chunk size.
                    HX_ASSERT("Illegal chunk size format"==NULL);
                    rc = HXR_UNEXPECTED;
                    break;
                }
            }
            else
            {
                //No buffer, no chunk size.
                HX_ASSERT("No chunk size present, bad data"==NULL);
                rc = HXR_UNEXPECTED;
                break;
            }
            bProcessStateChange = FALSE; // we've processed the header
        }
        else if (CE_BODY_READY == pChunkedEncoding->state)
        {
            // parse the chunk body
            if (pChunkedEncoding->lastchunk)
            {
                HX_VECTOR_DELETE(pChunkedEncoding->buf);
                pChunkedEncoding->buf_size = 0;
                HX_DELETE(pChunkedEncoding);
                SetReadContentsDone(TRUE);
                break;
            }

            if (m_bEncoded)
            {
                // it's kind of an odd thing, but here we are processing
                // content that is both chunked and gzip encoded. The only
                // place I've seen this in the wild is at shoutcast.com, and
                // then I've never seen more than one chunk.

                // SetDataGzipEncoded will update m_nContentRead
                rc = _SetDataGzipEncoded(pChunkedEncoding->buf, pChunkedEncoding->size);
            }
            else
            {
                HXLOGL4(HXLOG_HTTP, "Encoding: WriteChunky at %ld size %ld",
                        m_nContentRead, pChunkedEncoding->size);

                m_pChunkyRes->SetData(m_nContentRead, pChunkedEncoding->buf, pChunkedEncoding->size, this);
                m_nContentRead += pChunkedEncoding->size;
                m_ulHTTPResponseContentRead += pChunkedEncoding->size;
            }

            memset(pChunkedEncoding->buf, 0, pChunkedEncoding->buf_size);
            pChunkedEncoding->read = 0;
            pChunkedEncoding->state = CE_HEADER_PENDING;

            bProcessStateChange = FALSE; // we've processed the body
        }
        else if (m_bDiscardOrphanLFInChunkedData)
        {
            // If we get here, it means that we're processing a CRLF
            // where the CR was the last byte of the last chunk,
            // and the LF is the first byte of the current chunk
            HX_ASSERT(pChunkStart == pChunk);
            HX_ASSERT(*pChunk == LF);
            m_bDiscardOrphanLFInChunkedData = FALSE;
            // Fall through & discard this byte
            pChunk++, l--;
        }
        else if ((CE_HEADER_PENDING == pChunkedEncoding->state) && (*pChunk == CR))
        {
            pChunkedEncoding->state = CE_HEADER_READY;
            pChunk++, l--;
            bProcessStateChange = TRUE; // Run through the loop again to 
                                        // process the header

            if(l > 0)
            {
                HX_ASSERT(*pChunk == LF);
                pChunk++, l--;
            }
            else
            {
                // LF should be in the next packet
                m_bDiscardOrphanLFInChunkedData = TRUE;
            }
        }
        else if ((CE_BODY_PENDING == pChunkedEncoding->state) &&
                 (pChunkedEncoding->read == pChunkedEncoding->size))
        {
            HX_ASSERT(*pChunk == CR);

            pChunkedEncoding->state = CE_BODY_READY;
            pChunk++, l--;
            bProcessStateChange = TRUE; // Run through the loop again to 
                                        // process the header

            if(l > 0)
            {
                HX_ASSERT(*pChunk == LF);
                pChunk++, l--;
            }
            else
            {
                // LF should be in the next packet
                m_bDiscardOrphanLFInChunkedData = TRUE;
            }
        }
        else
        {
            // chunk-size exceeded MAX_CHUNK_SIZE in length.
            // A hex-number >1024 digits is very suspicious.

            if ((CE_HEADER_PENDING == pChunkedEncoding->state)
                &&  (pChunkedEncoding->read >= MAX_CHUNK_SIZE))
            {
                rc = HXR_UNEXPECTED;
                break;
            }
            else if ((CE_BODY_PENDING == pChunkedEncoding->state)
                     &&  (pChunkedEncoding->read >= pChunkedEncoding->size))
            {
                // Remote HTTP server is sending more data than
                // specified by chunk-size. This may be an attack on
                // the, so don't accept data sent in this fashion.
                HX_ASSERT(pChunkedEncoding->size > 0);

                // If read is ever > size, we're in trouble--
                // the buffer has already been overrun.
                HX_ASSERT(pChunkedEncoding->read <= pChunkedEncoding->buf_size);
                rc = HXR_UNEXPECTED;
                break;
            }

            HX_ASSERT((CE_HEADER_PENDING == pChunkedEncoding->state) ||
                      (CE_BODY_PENDING == pChunkedEncoding->state));

            pChunkedEncoding->buf[pChunkedEncoding->read++] = *pChunk;
            pChunk++, l--;
        }
    }

    return rc;
}

#endif //HELIX_FEATURE_HTTPFSYS_MEM_GROWTH_LIMIT
                              
