/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smplfsys.cpp,v 1.50 2009/03/04 15:10:25 qluo Exp $
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

/////////////////////////////////////////////////////////////////////////////
//
//  Simple File System for simple synchronous local files
//
//  This is a very simple file system, it just calls basic standard lib
//  calls for open, seek, read, etc...
//

#define INITGUID    1

#include "hxcom.h"
#include "hxtypes.h"
#include "../smplfsys.ver"

#include "hxcomm.h"
#include "ihxpckts.h"
#include "ihxfgbuf.h"
#include "ihxlist.h" // XXXAJC remove when CHXURL is no longer used
#include "hxlistp.h" // XXXAJC remove when CHXURL is no longer used
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxrendr.h"
#include "hxmon.h"
#include "hxauth.h"
#include "hxauthn.h"
#include "hxplgns.h"
#include "hxtbuf.h"
#include "hxdataf.h"
#include "hxtick.h"
#include "hxtbuf.h"
#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"
#include "hxdllaccess.h"
#include "hxtlogutil.h"

#include "debug.h"

#undef INITGUID

#include "hxathsp.h"
#include "hxcorsp.h"
#include "hxpktsp.h"
#include "hxcomsp.h"
#include "hxplnsp.h"
#include "hxspriv.h"
#include "hlxosstr.h"

#include "timeval.h"
#include "tparse.h"
#include "dbcs.h"	// for HXCompareStrings
#include "hxstring.h"	// for CHXString
#include "hxxfile.h"	// for HXXFile::GetReasonableLocalFileName()
#include "hxstrutl.h"
#include "hxver.h"
#include "chxpckts.h"
#include "hxurl.h"
#include "hxperf.h"
#include "hxcbobj.h"

#include "hxdir.h"
#include "hlxclib/errno.h"

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#ifdef _MACINTOSH
#include <fcntl.h>
#include "chxdataf.h"	// Macintosh file i/o
#include "macasyncfile.h"  // Macintosh interrupt file i/o
#ifdef _MAC_MACHO
#include <sys/stat.h>
#include "hlxclib/fcntl.h"
#include <unistd.h> // for unlink
#else
#include <unix.h> // for unlink
#endif
#ifdef _UNIX /* including unix.h defines _UNIX */
#undef _UNIX
#endif
#elif (defined (_WINDOWS ) || defined (_WIN32)) && !defined(WIN32_PLATFORM_PSPC)
#include <direct.h>
#include "datffact.h"
#else
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/fcntl.h"
#include "datffact.h"
#endif
#include "findfile.h"
#include "smplmlog.h"
#include "baseobj.h"
#if defined(HELIX_FEATURE_PROGDOWN)
#include "progdown.h"
#endif
#include "microsleep.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#include "hxslist.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
#include "smplfsys.h"
#include "hxperf.h"
#include "hxescapeutil.h"

#include "hxerror.h"

#ifdef _BREW

#ifdef AEE_SIMULATOR
#define _WIN32
#endif

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEFile.h"

#ifdef AEE_SIMULATOR 
#undef _WIN32
#endif

extern const IShell* g_pIShell;
#endif

#ifdef _AIX
#include "hxtbuf.h"
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Smplfsys);
#endif

// same for all the platforms...may need to tweak it, if necessary
#define MAX_ITERATION_COUNT    200

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef HELIX_CONFIG_NOSTATICS
INT32 smpl_nRefCount = 0;
#endif	// HELIX_CONFIG_NOSTATICS

HX_ENABLE_CHECKPOINTS_FOR_MODULE( "SmplFsys", "SmplFsysPerf.log" )

const char* const CSimpleFileSystem::zm_pDescription	= "RealNetworks Local File System";
const char* const CSimpleFileSystem::zm_pCopyright	= HXVER_COPYRIGHT;
const char* const CSimpleFileSystem::zm_pMoreInfoURL	= HXVER_MOREINFO;
const char* const CSimpleFileSystem::zm_pShortName	= "pn-local";
const char* const CSimpleFileSystem::zm_pProtocol	= "file|fileproxy|fd";

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
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    // Do NOT check for expiration.  Needed for Auto Upgrade.

    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CSimpleFileSystem();
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
 *	CanUnload()
 *
 *  Purpose:
 *
 *	Function implemented by all plugin DLL's if it returns HXR_OK
 *	then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
#ifdef HELIX_CONFIG_NOSTATICS
    return HXR_FAIL;
#else	// HELIX_CONFIG_NOSTATICS
    return (smpl_nRefCount ? HXR_FAIL : HXR_OK);
#endif	// HELIX_CONFIG_NOSTATICS
}


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
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXSHUTDOWN)(void)
{
    return HXR_OK;
}

CSimpleFileSystem::CSimpleFileSystem()
    : m_lRefCount(0)
    , m_pContext(0)
    , m_options(NULL)
    , m_ulMaxIterationLevel(MAX_ITERATION_COUNT)
    , m_pCommonObj(NULL)
    , m_bDisableMemoryMappedIO(FALSE)
    , m_bEnableFileLocking(FALSE)
    , m_ulChunkSize(0)
{
#ifndef HELIX_CONFIG_NOSTATICS
    smpl_nRefCount++;
#endif	// HELIX_CONFIG_NOSTATICS
}

CSimpleFileSystem::~CSimpleFileSystem()
{
#ifndef HELIX_CONFIG_NOSTATICS
    smpl_nRefCount--;
#endif	// HELIX_CONFIG_NOSTATICS

    if (m_pContext)
    {
	m_pContext->Release();
	m_pContext = 0;
    }

    if(m_options)
    {
	m_options->Release();
	m_options = 0;
    }

    HX_RELEASE(m_pCommonObj);
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
STDMETHODIMP CSimpleFileSystem::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_LOG_BLOCK( "CSimpleFileSystem::InitPlugin" );

    HX_RESULT		lResult;
    IHXPreferences*	prefs = 0;
    IHXBuffer*		base_path_buf = 0;

    if (pContext && !m_pContext)
    {
	HX_ENABLE_LOGGING(pContext);

        m_pContext = pContext;
	m_pContext->AddRef();

	IHXRegistry* pReg = NULL;
	if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pReg) == HXR_OK)
	{
	    INT32 lCS = 0;
	    if (HXR_OK == pReg->GetIntByName("config.MMapChunkSize", lCS) && lCS)
	    {
		m_ulChunkSize = lCS;
	    }
	    pReg->Release();
	}

	if(!m_options ||
	   (HXR_OK != m_options->GetPropertyBuffer("BasePath", base_path_buf)))
	{
	    lResult = pContext->QueryInterface(IID_IHXPreferences,
					       (void**) &prefs);
	    if (lResult == HXR_OK)
	    {
		lResult = prefs->ReadPref("BasePath", base_path_buf);
		if (lResult == HXR_OK)
		{
		    m_base_path = CHXString((char*)base_path_buf->GetBuffer());
		}
	    }
	}
	else
	{
	    m_base_path = CHXString((char*)base_path_buf->GetBuffer());
	}
    }

    if (prefs)
    {
	prefs->Release();
	prefs = 0;
    }

    if (base_path_buf)
    {
	base_path_buf->Release();
	base_path_buf = 0;
    }

    IHXGetRecursionLevel* pGet;
    lResult = pContext->QueryInterface(IID_IHXGetRecursionLevel,
				       (void**) &pGet);
    if (lResult == HXR_OK)
    {
	m_ulMaxIterationLevel = pGet->GetRecursionLevel();
	pGet->Release();
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    unInterfaceCount	the number of standard RMA interfaces
 *			supported by this plugin DLL.
 *    pIIDList		array of IID's for standard RMA interfaces
 *			supported by this plugin DLL.
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CSimpleFileSystem::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP CSimpleFileSystem::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this },
	{ GET_IIDHANDLE(IID_IHXFileSystemObject), (IHXFileSystemObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CSimpleFileSystem::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CSimpleFileSystem::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CSimpleFileSystem::GetFileSystemInfo
(
    REF(const char*) /*OUT*/ pShortName,
    REF(const char*) /*OUT*/ pProtocol
)
{
    pShortName	= zm_pShortName;
    pProtocol	= zm_pProtocol;

    return HXR_OK;
}

STDMETHODIMP
CSimpleFileSystem::InitFileSystem(IHXValues* options)
{
    HX_LOG_BLOCK( "CSimpleFileSystem::InitFileSystem" );

    IHXBuffer*		base_path_buf = 0;

    m_options = options;

    if (m_options)
    {
	m_options->AddRef();

	if (HXR_OK == m_options->GetPropertyBuffer("BasePath", base_path_buf))
	{
	    m_base_path = CHXString((char*)base_path_buf->GetBuffer());
	}

	ULONG32 ulTemp = 0;
	HXBOOL bLog = FALSE;
	/*
         *  XXX PM this is weak.  I need a way to show these options
	 *  but only if explicitly asked to do so.  Ideally we would
	 *  use the IRMErrorMessages with HXLOG_DEBUG and --sdm for
	 *  server but I can't here because no context gets passed
	 *  to file system object until its first connection.
	 *  Can you believe that?
	 */
	m_options->GetPropertyULONG32("LogOptionalParams",
		ulTemp);
	bLog = ulTemp ? TRUE : FALSE;

	ulTemp = 0;
	m_options->GetPropertyULONG32("DisableMemoryMappedIO",
	    ulTemp);
	m_bDisableMemoryMappedIO = ulTemp ? TRUE : FALSE;

	ulTemp = 0;
	m_options->GetPropertyULONG32("EnableFileLocking",
		ulTemp);
	m_bEnableFileLocking = ulTemp ? TRUE : FALSE;

	ulTemp = 0;
	m_options->GetPropertyULONG32("MaxIterationLevel", ulTemp);
	if (ulTemp)
	{
            m_ulMaxIterationLevel = ulTemp;
	}
	if (bLog)
	{
	    char pNumericMount[50]; /* Flawfinder: ignore */
	    IHXBuffer* pBuffer = 0;
	    const char* pMount;
	    m_options->GetPropertyCString("MountPoint", pBuffer);
	    if (!pBuffer)
	    {
		m_options->GetPropertyBuffer("MountPoint", pBuffer);
		if (!pBuffer)
		{
		    m_options->GetPropertyULONG32("MountPount", ulTemp);
		}
	    }
	    if (pBuffer)
	    {
		pMount = (const char*)pBuffer->GetBuffer();
	    }
	    else
	    {
		pMount = pNumericMount;
		sprintf(pNumericMount, "%lu", ulTemp); /* Flawfinder: ignore */
	    }

	    printf("Optional smplfsys (pn-local) parameters for"
		    " MountPoint: %s\n", pMount);
	    HX_RELEASE(pBuffer);
	    printf("DisableMemoryMappedIO: %s\n",
		    m_bDisableMemoryMappedIO ? "TRUE" : "FALSE");
	    printf("EnableFileLocking: %s\n",
		    m_bEnableFileLocking ? "TRUE" : "FALSE");
	    printf("MaxIterationLevel: %lu\n",
		    m_ulMaxIterationLevel);
	    ulTemp = 0;
	    m_options->GetPropertyULONG32("MMapChunkSize", ulTemp);
	    if (ulTemp)
	    {
		printf("MMapChunkSize: %lu\n", ulTemp);
	    }
	}
    }

    if (base_path_buf)
    {
	base_path_buf->Release();
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileSystemObject::CreateFile
//  Purpose:
//	TBD
//
STDMETHODIMP CSimpleFileSystem::CreateFile
(
    IUnknown**	/*OUT*/	ppFileObject
)
{
    HX_LOG_BLOCK( "CSimpleFileSystem::CreateFile" );

    CSimpleFileObject* pFileObj =
	new CSimpleFileObject(m_base_path,
			      this,
			      m_pContext,
			      m_ulMaxIterationLevel);
    if (pFileObj)
    {
	if (HXR_OK == pFileObj->QueryInterface(IID_IUnknown,
					    (void**)ppFileObject))
	{
	    return HXR_OK;
	}
	return HXR_UNEXPECTED;
    }
    return HXR_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	CSimpleFileSystem::CreateDir
//  Purpose:
//	TBD
//
STDMETHODIMP CSimpleFileSystem::CreateDir
(
    IUnknown**	/*OUT*/	ppDirObject
)
{
    return HXR_NOTIMPL;
}

CSimpleFileObject::CSimpleFileObject(CHXString& base_path,
				     CSimpleFileSystem *pFS,
				     IUnknown* pContext,
				     UINT32 ulMaxIterationLevel)
    : m_lRefCount(0)
    , m_ulFlags(0)
    , m_bLocalClose(FALSE)
    , m_bProxyMode(FALSE)
    , m_pContext(pContext)
    , m_pCommonClassFactory(NULL)
    , m_pFileResponse(NULL)
    , m_pFileSystem(pFS)
    , m_pFilename(NULL)
    , m_pRequest(0)
    , m_pDescriptorReg(0)
    , m_pDirResponse(NULL)
    , m_pDirList(NULL)
    , m_pScheduler(NULL)
    , m_pStackCallback(NULL)
    , m_bInRead(FALSE)
    , m_bReadPending(FALSE)
    , m_bAsyncReadPending(FALSE)
    , m_ulPendingReadCount(0)
    , m_ulSize(0)
    , m_ulPos(0)
    , m_bAsyncAccess(TRUE)
    , m_bCanBeReOpened(0)
    , m_pDataFile(NULL)
#ifdef _MACINTOSH
    , m_pAsyncFileResponse(NULL)
    , m_pInterruptState(NULL)
    , m_pFileStatResponse(NULL)
    , m_eSeekReason(EXTERNAL_SEEK)
    , m_ulPreSeekPosition(0)
    , m_bFileToBeClosed(FALSE)
#endif
    , m_nFd(-1)
    , m_ulMaxIterationLevel(ulMaxIterationLevel)
    , m_pUnknownUserContext(NULL)
    , m_ulPendingSeekOffset(0)
    , m_usPendingSeekWhence(SEEK_SET)
#if defined(HELIX_FEATURE_PROGDOWN)
    , m_pProgDownMon(NULL)
    , m_ulCallbackState(CallbackStateUnknown)
    , m_bProgDownEnabled(TRUE)
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    , m_ulPrgDnTotalFileSize(HX_PROGDOWNLD_UNKNOWN_FILE_SIZE)
    , m_ulPrgDnTotalFileDur(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulPriorReportedTotalFileDur(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulCurrentDurOfBytesSoFar(HX_PROGDOWNLD_UNKNOWN_DURATION)
    , m_ulTimeOfLastBytesToDur(0)
    , m_pBytesToDur(NULL)
    , m_ulStatusUpdateGranularityInMsec(HX_PROGDOWNLD_DEFAULT_STATUSREPORT_INTERVAL_MSEC)
    , m_pPDSObserverList(NULL)
    , m_bDownloadProgressReported(FALSE)
    , m_bDownloadCompleteReported(FALSE)
    , m_bDidNotifyOfDownloadPause(FALSE)
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
{
    MLOG_LEAK("CON CSimpleFileObject this=0x%08x\n", this);
#ifndef HELIX_CONFIG_NOSTATICS
    smpl_nRefCount++;
#endif	//  HELIX_CONFIG_NOSTATICS
    m_base_path = base_path;

    if (m_pFileSystem)
    {
	m_pFileSystem->AddRef();
    }

    if (m_pContext)
    {
	m_pContext->AddRef();
#ifdef HELIX_FEATURE_SERVER
	IHXThreadSafeScheduler* pTSScheduler = 0;
        m_pContext->QueryInterface(IID_IHXThreadSafeScheduler, (void**) &pTSScheduler);
        if (pTSScheduler)
        {
	    pTSScheduler->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
            pTSScheduler->Release();
        }
        else
        {
	    m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
        }
        HX_ASSERT(m_pScheduler);
#else
	m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
#endif //HELIX_FEATURE_SERVER

	m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				   (void **)&m_pCommonClassFactory);
    }

    m_pStackCallback = new CHXGenericCallback(this, CSimpleFileObject::StackCallback);
    if (m_pStackCallback)
    {
        m_pStackCallback->AddRef();
    }

#if defined (_MACINTOSH)
    m_pAsyncFileResponse = new SMPLAsyncResponse(this);
    if (m_pContext)
    {
	m_pContext->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
    }
#endif

#if defined(HELIX_FEATURE_PROGDOWN)
    m_pProgDownMon = new CProgressiveDownloadMonitor();
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
};

CSimpleFileObject::~CSimpleFileObject()
{
    MLOG_LEAK("DES CSimpleFileObject this=0x%08x\n", this);
#ifndef HELIX_CONFIG_NOSTATICS
    smpl_nRefCount--;
#endif 	// HELIX_CONFIG_NOSTATICS
    m_bLocalClose = TRUE;
    Close();
};


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//  	object.
//
STDMETHODIMP CSimpleFileObject::QueryInterface(REFIID riid, void** ppvObj)
{
    if (riid == IID_IHXFileMimeMapper &&
	m_bProxyMode && m_pDataFile &&
	m_pDataFile->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXFileObject), (IHXFileObject*) this },
	{ GET_IIDHANDLE(IID_IHXDirHandler), (IHXDirHandler*) this },
	{ GET_IIDHANDLE(IID_IHXFileStat), (IHXFileStat*) this },
#if defined(HELIX_FEATURE_SERVER) && defined(_UNIX)
	{ GET_IIDHANDLE(IID_IHXFileStat2), (IHXFileStat2*) this },
#endif
	{ GET_IIDHANDLE(IID_IHXFileExists), (IHXFileExists*) this },
	{ GET_IIDHANDLE(IID_IHXGetFileFromSamePool), (IHXGetFileFromSamePool*) this },
	{ GET_IIDHANDLE(IID_IHXRequestHandler), (IHXRequestHandler*) this },
	{ GET_IIDHANDLE(IID_IHXFileRename), (IHXFileRename*) this },
	{ GET_IIDHANDLE(IID_IHXFileRemove), (IHXFileRemove*) this },
	{ GET_IIDHANDLE(IID_IHXFileMove), (IHXFileMove*) this },
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
	{ GET_IIDHANDLE(IID_IHXPDStatusMgr), (IHXPDStatusMgr*) this },
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
        { GET_IIDHANDLE(IID_IHXNestedDirHandler), (IHXNestedDirHandler*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CSimpleFileObject::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CSimpleFileObject::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *	IHXFileObject::Init
 *  Purpose:
 *	Associates a file object with the file response object it should
 *	notify of operation completness. This method should also check
 *	for validity of the object (for example by opening it if it is
 *	a local file).
 */
STDMETHODIMP
CSimpleFileObject::Init
(
    ULONG32		/*IN*/	ulFlags,
    IHXFileResponse*   /*IN*/	pFileResponse
)
{
    MLOG_GEN(NULL, "CSimpleFileObject::Init(0x%08x,0x%08x) this=0x%08x\n",
             ulFlags, pFileResponse, this);
    HX_LOG_BLOCK( "CSimpleFileObject::Init" );

    DPRINTF(0x5d000000, ("CSFO::Init(flags(0x%x), pFileResponse(%p)) "
	    "-- fd(%d)\n", ulFlags, pFileResponse, m_nFd));

    HX_RESULT lReturnVal = HXR_OK;
    HX_RESULT resultInitDone = HXR_OK;
    IHXRequestContext* pIHXRequestContextCurrent = NULL;

    if (!pFileResponse) return HXR_INVALID_PARAMETER;
    if (!m_pRequest) return HXR_INVALID_PARAMETER;
    if (!m_pDataFile) return HXR_INVALID_PARAMETER;

    /* Release any previous reponses */
    if (m_pFileResponse)
    {
	m_pFileResponse->Release();
    }

    m_pFileResponse = pFileResponse;
    m_pFileResponse->AddRef();

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    // /QueryInterface for the IHXMediaBytesToMediaDur interface
    HX_RELEASE(m_pBytesToDur);
    m_pFileResponse->QueryInterface(IID_IHXMediaBytesToMediaDur, (void**) &m_pBytesToDur);
    if (m_pBytesToDur)
    {
        // /The FF will know its complete-file's duration from either the
        // file's header info (if any) or from some other method:
        if (SUCCEEDED(m_pBytesToDur->GetFileDuration(
                // /OK if this is still HX_PROGDOWNLD_UNKNOWN_FILE_SIZE:
                m_ulPrgDnTotalFileSize,
                m_ulPrgDnTotalFileDur)))
        {
            ReportTotalDurChanged();
        }
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    /* have we already opened/created a file */
    if (m_nFd != -1)
    {
	m_bReadPending	     = FALSE;
	m_ulPendingReadCount = 0;

	/* remove any pending callbacks */
        if (m_pStackCallback)
        {
            m_pStackCallback->Cancel(m_pScheduler);
        }

	/* if flags are same, then we are all set and there
	 * is no need to do anything further
	 */
	if (m_ulFlags == ulFlags || ulFlags == 0)
	{
	    /* If we have already opened a file, then seek back
	     * to zero during re-initialization
	     */
#ifndef _MACINTOSH
	    HX_RESULT result = m_pDataFile->Seek(0, SEEK_SET);
	    m_pFileResponse->InitDone(result);
	    return result;
#else
	    HXBOOL bAtInterrupt = FALSE;
	    if (m_pInterruptState)
	    {
		bAtInterrupt = m_pInterruptState->AtInterruptTime();
	    }

	    /* I want to know if we ever reach here at interrupt time
	     * Please e-mail me a repro case if you hit this assert
	     * - RA
	     */
	    HX_ASSERT(bAtInterrupt == FALSE);

	    m_eSeekReason = REINIT_SEEK;
	    HX_RESULT theResult = m_pDataFile->SafeSeek(0, SEEK_SET, bAtInterrupt);
	    if (theResult != HXR_OK)
	    {
		m_pFileResponse->InitDone(HXR_FAIL);
	    }

	    return theResult;
#endif
	    /*
	    m_pFileResponse->InitDone(HXR_OK);
	    return HXR_OK;
	    */
	}

#ifdef _MACINTOSH
	HX_RELEASE(m_pDataFile);
	m_pDataFile = NULL;
#else
	DPRINTF(0x5d000000, ("CSFO::Init() -- m_pDataFile->Close()\n"));

        if (m_pDescriptorReg)
	{
	    m_pDescriptorReg->UnRegisterDescriptors(1);
	}

        m_pDataFile->Close();
	m_nFd = -1;
#endif
    }

    m_ulFlags = ulFlags;

    if (!m_pCommonClassFactory)
    {
	m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	                                    (void **)&m_pCommonClassFactory);
    }

    HX_RELEASE(m_pUnknownUserContext);

    if (m_pRequest && SUCCEEDED(m_pRequest->QueryInterface(
	IID_IHXRequestContext, (void**)&pIHXRequestContextCurrent)))
    {
	pIHXRequestContextCurrent->GetUserContext(m_pUnknownUserContext);
	pIHXRequestContextCurrent->Release();
    }

    DPRINTF(0x5d000000, ("CSFO::Init() -- (2) _OpenFile()\n"));
    lReturnVal = _OpenFile(ulFlags);
    DPRINTF(0x5d000000, ("CSFO::Init(flags(0x%x), pFileResponse(%p)) "
	    "-- result(0x%x), m_nFd(%d)\n",
	    ulFlags, pFileResponse, lReturnVal, m_nFd));

    if ((m_nFd == -1 || FAILED(lReturnVal)))
    {
	// XXXSSH -- probably obsolete
	if (lReturnVal != HXR_NOT_AUTHORIZED && lReturnVal != HXR_NO_MORE_FILES)
	{
	    lReturnVal = HXR_DOC_MISSING;
	}
    }
    else
    {
	lReturnVal = HXR_OK;
    }

#if defined(HELIX_FEATURE_PROGDOWN)
    // If we are writing to the file, then disable
    // the progressive download features
    m_bProgDownEnabled = (m_ulFlags & HX_FILE_WRITE ? FALSE : TRUE);
    if (m_pProgDownMon && m_bProgDownEnabled)
    {
        m_pProgDownMon->Init(m_pContext, m_pDataFile, this);
    }
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

    resultInitDone =
	m_pFileResponse->InitDone(lReturnVal);
    lReturnVal = resultInitDone;

    DPRINTF(0x5d000000, ("CSFO::Init() "
	    "-- resultInitDone(0x%x), m_nFd(%d)\n",
	    resultInitDone, m_nFd));

    return (lReturnVal);
}

/************************************************************************
 *  Method:
 *      IHXFileObject::GetFilename
 *  Purpose:
 *      Returns the filename (without any path information) associated
 *      with a file object.
 */
STDMETHODIMP CSimpleFileObject::GetFilename
(
    REF(const char*) /*OUT*/ pFilename
)
{
    UpdateFileNameMember();
    // Find the separator character before the file name
    pFilename = ::strrchr(m_pFilename, OS_SEPARATOR_CHAR);

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
 *	IHXFileObject::Close
 *  Purpose:
 *	Closes the file resource and releases all resources associated
 *	with the object.
 */
STDMETHODIMP CSimpleFileObject::Close()
{
    MLOG_GEN(NULL, "CSimpleFileObject::Close() this=0x%08x\n", this);
    HX_LOG_BLOCK( "CSimpleFileObject::Close" );

    // If there is a pending callback, be sure to remove it!
    if (m_pStackCallback) m_pStackCallback->Cancel(m_pScheduler);

    HX_RELEASE(m_pStackCallback);
    HX_RELEASE(m_pScheduler);

    HX_RELEASE(m_pUnknownUserContext);

    if (m_pContext)
    {
	m_pContext->Release();
	m_pContext = NULL;
    }

    if (m_pCommonClassFactory)
    {
	m_pCommonClassFactory->Release();
	m_pCommonClassFactory = NULL;
    }

    if (m_pFileSystem)
    {
	m_pFileSystem->Release();
	m_pFileSystem = NULL;
    }

    if (m_pRequest)
    {
	m_pRequest->Release();
	m_pRequest = NULL;
    }

    if (m_pDescriptorReg && (m_nFd != -1))
    {
	m_pDescriptorReg->UnRegisterDescriptors(1);
	m_pDescriptorReg->Release();
	m_pDescriptorReg = 0;
    }
#if defined(HELIX_FEATURE_PROGDOWN)
    // XXXMEH
    // IMPORTANT!!
    // Since the progressive download monitor does now not
    // hold a ref on m_pDataFile, then we MUST make sure
    // and call m_pProgDownMon->Close() BEFORE we close
    // and release m_pDataFile. This is because there could
    // be pending callbacks which will attempt to use
    // the IHXDataFile. Callling m_pProgDownMon->Close()
    // will cancel these pending callbacks.
    if (m_pProgDownMon)
    {
        // Close the progressive download monitor. Closing
        // also cancels any pending callbacks
        m_pProgDownMon->Close();
    }
    HX_DELETE(m_pProgDownMon);
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    m_ulPrgDnTotalFileSize = HX_PROGDOWNLD_UNKNOWN_FILE_SIZE;
    m_ulPrgDnTotalFileDur = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulPriorReportedTotalFileDur = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulCurrentDurOfBytesSoFar = HX_PROGDOWNLD_UNKNOWN_DURATION;
    m_ulTimeOfLastBytesToDur = 0;
    m_bDownloadProgressReported = FALSE;
    m_bDownloadCompleteReported = FALSE;
    m_bDidNotifyOfDownloadPause = FALSE;
    HX_RELEASE(m_pBytesToDur);
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


    if (m_pDataFile)
    {
#if defined _MACINTOSH
    HX_RELEASE(m_pDataFile);
    HX_DELETE(m_pAsyncFileResponse);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pFileStatResponse);
#else
	DPRINTF(0x5d000000, ("CSFO::Close() -- m_pDataFile->Close()\n"));
	m_pDataFile->Close();
	HX_RELEASE(m_pDataFile);
#endif
    }
    m_nFd = -1;

    if(m_pFilename)
    {
	HX_VECTOR_DELETE(m_pFilename);
	m_pFilename = NULL;
    }

    if (m_pDirResponse)
    {
	m_pDirResponse->Release();
	m_pDirResponse = NULL;
    }

    if (m_pDirList)
    {
	delete m_pDirList;
	m_pDirList = NULL;
    }

    // Make sure we do not attempt to schedule any more replies which
    // would result in a crash after this point
    m_bReadPending = FALSE;

	// It is vitally important that the CloseDone be the last step in
    // this method, as the last reference to this object may be
    // released inside CloseDone!  Do not place code after this if block!
    // MBO: Access to stack variables after CloseDone is OK.

    if (!m_bLocalClose)
    {
	if(m_pFileResponse)
	{
	    // This keeps us from getting into an infinite loop when
	    // CSimpleFileObject::Close() calls m_pFileResponse->Release()
	    // which calls ~CRIFFReader() which calls Close() and so on,
	    // never hitting the line that sets m_pFileResponse to NULL...
	    IHXFileResponse* pTempFileResponse = m_pFileResponse;
	    m_pFileResponse = NULL;

	    pTempFileResponse->CloseDone(HXR_OK);

	    pTempFileResponse->Release();
        }
    }
    else if (m_pFileResponse)
    {
	m_pFileResponse->Release();
	m_pFileResponse = NULL;
    }



    return HXR_OK;
}

/************************************************************************
 *  Method:
 *	IHXFileObject::Read
 *  Purpose:
 *	Reads a buffer of data of the specified length from the file
 *	and asynchronously returns it to the caller via the
 *	IHXFileResponse interface passed in to Init.
 */
STDMETHODIMP CSimpleFileObject::Read(ULONG32 ulCount)
{
    MLOG_GEN(NULL, "CSimpleFileObject::Read(%lu) this=0x%08x tick=%lu\n",
             ulCount, this, HX_GET_BETTERTICKCOUNT());
    HX_LOG_BLOCK( "CSimpleFileObject::Read" );

    // XXXBHG, For now, you cant read more than 1MB at a time!
    if (ulCount > 0x000FFFFF)
    {
	//Force the system to recognize a failed Read so infinite
	// buffering does not occur with the core waiting for EOF:
	ActualAsyncReadDone(HXR_FAIL, NULL);

	return HXR_INVALID_PARAMETER;
    }

    if((m_nFd == -1) && m_bCanBeReOpened)
    {
	DPRINTF(0x5d000000, ("CSFO::Read() -- _OpenFile()\n"));
	_OpenFile(m_ulFlags);
	m_bCanBeReOpened = FALSE;
#ifndef _MACINTOSH
	m_pDataFile->Seek(m_ulPos, SEEK_SET);
#endif
    }
    if (m_nFd != -1)
    {
	if(!(m_ulFlags & HX_FILE_READ))
	    return HXR_UNEXPECTED;

	if (m_bReadPending)
	{
	    return HXR_UNEXPECTED;
	}

	m_bReadPending = TRUE;
	m_ulPendingReadCount = ulCount;

	if (m_bInRead && m_bAsyncAccess)
	{
	    return HXR_OK;
	}

	m_bInRead = TRUE;

	HX_RESULT theErr = HXR_OK;
	UINT16 uIterationCount;
	uIterationCount = 0;

	AddRef(); // Make sure we do not destruct in ReadDone()

        HXBOOL bProgFail = FALSE;
	do
	{
            bProgFail = FALSE;
	    theErr = DoRead(bProgFail);
	    uIterationCount++;
	} while (m_bReadPending && !m_bAsyncReadPending && !theErr && uIterationCount < m_ulMaxIterationLevel && !bProgFail);

	/* have we exceeded our iteration count? */
	if (m_bReadPending && !m_bAsyncReadPending && !theErr && m_bAsyncAccess && !bProgFail)
	{
            MLOG_PD(NULL, "\tScheduling stack callback\n");
	    HX_ASSERT(!m_pStackCallback->IsCallbackPending() &&
		     uIterationCount >= m_ulMaxIterationLevel);
            // Schedule a callback if there is not one already scheduled
            m_pStackCallback->ScheduleRelative(m_pScheduler, 0);
	}

	m_bInRead = FALSE;

	Release();

	return theErr;

    }

    return HXR_UNEXPECTED;
}

HX_RESULT
CSimpleFileObject::DoRead(REF(HXBOOL) rbProgFail)
{
    MLOG_PD(NULL, "CSimpleFileObject::DoRead() this=0x%08x tick=%lu curoffset=%lu\n",
            this, HX_GET_BETTERTICKCOUNT(), m_ulPos);
    HX_LOG_BLOCK( "CSimpleFileObject::DoRead" );

    HX_ASSERT(m_bReadPending);

    HX_RESULT	theErr	= HXR_OK;
    UINT32	ulCount = m_ulPendingReadCount;

#ifndef _MACINTOSH
    // Create buffer object here, notice that we call the
    // CreateInstance method of the controller, but we could
    // have implemented our own object that exposed the IHXBuffer
    // interface.
    IHXBuffer* pBuffer = NULL;
    ULONG32 actual = m_pDataFile->Read(pBuffer, ulCount);

    if (0 == actual)
    {
	// 0 == actual is considered as EOF
	// we need to differentiate the abnormal file read error from 
	// EOF so that we can terminate the playback immediately.
	//
	// Normally, we will pass the error to the File Format via ReadDone()
	// which will then pass on to the core.
	//
	// Unfortunately, most File Fomat consider the error from ReadDone()
	// as EOF and keeps the playback alive even though it's not EOF.
	//
	// Instead of modifying each File Format, the following is the 
	// short-cut to bypass the File Format and report error directly
	// back to the core.
	theErr = m_pDataFile->GetLastError();
	if (HXR_ABORT == theErr)
	{
	    IHXErrorMessages*	pErrMsg = NULL;

	    if (m_pContext &&
    		HXR_OK == m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg))
	    {
		pErrMsg->Report(HXLOG_ERR, theErr, 0, NULL, NULL);
		HX_RELEASE(pErrMsg);
	    }
	    return FinishDoRead(actual, pBuffer);
	}
    }

    // Check for IHXDataFile::Read status codes, if we did not get the number of bytes requested,
    // otherwise use PD logic
    if ((INT32)actual < (INT32)ulCount)
    {
	switch ((INT32)actual) 
	{
	    case HX_FILESTATUS_FATAL_ERROR :
	    case HX_FILESTATUS_ERROR :
	    case HX_FILESTATUS_LOGICAL_EOF :
		{
		    HXLOGL3(HXLOG_GENE,"DoRead()******** FAILED to read requested bytes, error code=%ld", actual);
		    rbProgFail = FALSE;
		    return FinishDoRead(0, pBuffer);
		}
		break;
	    case HX_FILESTATUS_DATA_PENDING :
		{
		    // Seek backwards if necessary
		    SeekBackwards(actual);

		    if (m_bAsyncAccess)
		    {
#if defined(HELIX_FEATURE_PROGDOWN)
			// Set the callback state
			m_ulCallbackState = CallbackStateRead;
			// Schedule a callback to try the read again
			m_pProgDownMon->ScheduleCallback();
#else /* #if defined(HELIX_FEATURE_PROGDOWN) */
			if (m_pScheduler && m_pStackCallback &&
			    !m_pStackCallback->IsCallbackPending())
			{
			    m_pStackCallback->ScheduleRelative(m_pScheduler, DATAFILE_FAIL_INTERVAL_DEFAULT);
			}
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
		    }
		    else
		    {
			// Since we can't employ callback when m_bAsyncAccess is FALSE,
			// then we must sleep. We'll sleep for 100ms (or 100000 microsec)
			microsleep(100000);
		    }
		    return HXR_OK;
		}
	        break;
	    default:
		// do nothing
		break;
	}
    }

#if defined(HELIX_FEATURE_PROGDOWN)
    MLOG_PD(NULL, "\tulCount = %lu actual = %lu\n", ulCount, actual);
    // Sanity check. Unlikely we'd even make it here
    // if we failed to allocate m_pProgDownMon, but 
    // if so, then just behave like HELIX_FEATURE_PROGDOWN
    // is not defined
    if (m_pProgDownMon && m_bProgDownEnabled)
    {
        HXBOOL bIsProgressive = m_pProgDownMon->HasBeenProgressive()  &&
                m_pProgDownMon->IsProgressive();

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
        UINT32 ulPrevFileSize = m_pProgDownMon->GetLastFileSize();
        m_pProgDownMon->MonitorFileSize(); // updates lastFileSize.
        UINT32 ulCurFileSize = m_pProgDownMon->GetLastFileSize();

        // /If we know the complete file size from URL options and it
        // differs from current file size stat'd so far, it's progressive:
        if (IsPrgDnCompleteFileSizeKnown())
        {
            bIsProgressive = (m_ulPrgDnTotalFileSize > ulCurFileSize);
        }
        if (bIsProgressive)
        {
            MaybeDoProgressiveDownloadStatusRept(ulPrevFileSize, ulCurFileSize);
        }

        // /To fix PR 129618 so we get callbacks when we haven't yet hit EOF
        // and TLC is paused (waiting for more data or by user), we need to
        // kick-start file-size monitoring now if we detect that this is
        // progressive:
        if (ulPrevFileSize != ulCurFileSize  ||
                (IsPrgDnCompleteFileSizeKnown()  &&
                m_ulPrgDnTotalFileSize > ulCurFileSize)  ||
                (m_pProgDownMon->HasBeenProgressive()  &&
                m_pProgDownMon->IsProgressive()) )
        {
            m_pProgDownMon->BeginSizeMonitoring();
        }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.




        // Did we read all the bytes we were asked for?
        if (actual < ulCount)
        {
            MLOG_PD(NULL, "\t\t******** FAILED to read requested bytes\n");
            // The read failed to read all the requested bytes
            //

            // Do we have a history of progressive download
            // with this file?
            if (bIsProgressive  ||  m_pProgDownMon->HasBeenProgressive())
            {
                MLOG_PD(NULL, "\t\t\tFile HAS been progressive\n");
                // This file has been progressive sometime in
                // the past. However, we need to
                // check the *current* state.
                if (bIsProgressive  ||  m_pProgDownMon->IsProgressive())
                {
                    MLOG_PD(NULL, "\t\t\t\tFile currently IS progressive\n");
                    // The file is still currently downloading.
                    //

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                    // /If we only know it's progressive because we have the
                    // file size, let's watch for a while to see if the
                    // external download manager is paused:
                    // /We know the filesize so if it's greater than bytes so
                    // far, then we know we're paused if retry count == 0
                    if (!m_bDidNotifyOfDownloadPause  &&
                            !m_pProgDownMon->IsProgressive()  &&
                            IsPrgDnCompleteFileSizeKnown()  &&
                            m_ulPrgDnTotalFileSize > ulCurFileSize)
                    {
                        // Decrement the retry count. If the file becomes
                        // progressive again, then this count gets reset.
                        m_pProgDownMon->DecrementFormerProgressiveRetryCount();
                        if (m_pProgDownMon->GetFormerProgressiveRetryCount() == 0)
                        {
                            // /Send a pause notification:
                            ReportChange(PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_PAUSE);
                        }
                    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

                    // Can we callback asynchronously?
                    if (m_bAsyncAccess)
                    {
                        // Set the flag saying we failed due to
                        // progressive download
                        rbProgFail = TRUE;
                        HX_RELEASE(pBuffer);
                        // Schedule the callback
                        return FinishDoReadWithCallback(actual);
                    }
                    else
                    {
                        HX_RELEASE(pBuffer);
                        // Try synchronously
                        return FinishDoReadWithoutCallback(actual);
                    }
                }
                else
                {
                    MLOG_PD(NULL, "\t\t\t\tFile currently is NOT progressive\n");
                    // The file has been progressive in the past
                    // but the current state is NOT progressive.
                    // That could mean that either the download has
                    // finished or that the download agent is paused.
                    // With our current implementation we can't tell
                    // the difference (we would need to know the "true"
                    // file size to know for sure). Therefore, we will
                    // ask the fileformat whether or not it would "expect"
                    // a failure at this point. The fileformat should have
                    // knowledge to know whether or not a failure of this
                    // read would indicate failure or not. Sometimes fileformats
                    // read in large chunks and *expect* to get a failure.
                    // On the other hand, file formats sometimes do a read
                    // and are NOT expecting it to fail.
                    if (RequireFullRead())
                    {
                        MLOG_PD(NULL, "\t\t\t\tFileResponse requires full reads\n");
                        // The response says it requires full reads
                        //
                        // Can we callback asynchronously?
                        if (m_bAsyncAccess)
                        {
                            // Set the flag saying we failed due to
                            // progressive download
                            rbProgFail = TRUE;
                            HX_RELEASE(pBuffer);
                            // Schedule the callback
                            return FinishDoReadWithCallback(actual);
                        }
                        else
                        {
                            HX_RELEASE(pBuffer);
                            // Try synchronously
                            return FinishDoReadWithoutCallback(actual);
                        }
                    }
                    else if (m_pProgDownMon->GetFormerProgressiveRetryCount() > 0)
                    {
                        MLOG_PD(NULL, "\t\t\t\tRetrying former progressive, count = %lu\n",
                                m_pProgDownMon->GetFormerProgressiveRetryCount());
                        // We do not require a full read, but we are going
                        // to retry a certain number of times before calling
                        // back with ReadDone().
                        //
                        // Decrement the retry count. If the file becomes
                        // progressive again, then this count gets reset.
                        m_pProgDownMon->DecrementFormerProgressiveRetryCount();

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                        // /If we drop to 0 count, we have to assume the file
                        // has completely downloaded and notify the observer
                        // chain:
                        if (!m_pProgDownMon->GetFormerProgressiveRetryCount())
                        {
                            if (SUCCEEDED(m_pBytesToDur->ConvertFileOffsetToDur(
                                    ulCurFileSize,
                                    // /It's OK if the following is ...UNKNOWN...:
                                    m_ulPrgDnTotalFileSize,
                                    /*REF*/ m_ulCurrentDurOfBytesSoFar)))
                            {
                                ReportCurrentDurChanged();
                            }
                            ReportDownloadComplete();
                        }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

                        // Can we callback asynchronously?
                        if (m_bAsyncAccess)
                        {
                            // Set the flag saying we failed due to
                            // progressive download
                            rbProgFail = TRUE;
                            HX_RELEASE(pBuffer);
                            // Schedule the callback
                            return FinishDoReadWithCallback(actual);
                        }
                        else
                        {
                            HX_RELEASE(pBuffer);
                            // Try synchronously
                            return FinishDoReadWithoutCallback(actual);
                        }
                    }
                    else
                    {
                        MLOG_PD(NULL, "\t\t\t\tCannot retry former progressive any more\n");
                        // We have retried on this formerly progressive
                        // file as long as we can, so just call back
                        // with ReadDone().
                        //
                        // Clear the progressive download failure flag
                        rbProgFail = FALSE;
                        // Finish the DoRead
                        return FinishDoRead(actual, pBuffer);
                    }
                }
            }
            else /* if (m_pProgDownMon->HasBeenProgressive()) */
            {
                MLOG_PD(NULL, "\t\t\tFile has NOT been progressive, checking file size\n");
                // This file has never yet been progressive
                //
                // Do a filesize check. This will check
                // the filesize against the filesize at
                // the time that m_pProgDownMon->Init()
                // was called. If the filesize has changed,
                // then the m_pProgDownMon->HasBeenProgressive()
                // flag will change.
                m_pProgDownMon->MonitorFileSize();
                // Has the filesize changed?
                if (m_pProgDownMon->HasBeenProgressive())
                {
                    MLOG_PD(NULL, "\t\t\t\tFile size HAS changed, beginning size monitoring and retrying\n");
                    // The filesize has indeed changed.
                    //
                    // Begin the monitoring of the file size
                    m_pProgDownMon->BeginSizeMonitoring();
                    // Can we callback asynchronously?
                    if (m_bAsyncAccess)
                    {
                        // Set the flag saying we failed due to
                        // progressive download
                        rbProgFail = TRUE;
                        HX_RELEASE(pBuffer);
                        // Schedule the callback
                        return FinishDoReadWithCallback(actual);
                    }
                    else
                    {
                        HX_RELEASE(pBuffer);
                        // Try synchronously
                        return FinishDoReadWithoutCallback(actual);
                    }
                }
                else
                {
                    MLOG_PD(NULL, "\t\t\t\tFile size has NOT changed.\n");
                    // The filesize has NOT changed. This is most likely
                    // just a normal file and the fileformat has attempted
                    // to read past the end of the file. However, we will
                    // first ask the fileformat if it expected a failure.
                    if (RequireFullRead() &&
                        m_pProgDownMon->GetNotProgressiveRetryCount() > 0)
                    {
                        MLOG_PD(NULL, "\t\t\t\t\tRetrying not progressive, count = %lu\n",
                                m_pProgDownMon->GetNotProgressiveRetryCount());
                        // Decrement the retry count. If the file becomes
                        // progressive, then this count gets reset.
                        m_pProgDownMon->DecrementNotProgressiveRetryCount();
                        // The fileformat had the Advise interface and 
                        // said it did not expect a failure here. Also,
                        // we have not retried our maximum number of attempts
                        // yet, so we will retry the read.
                        //
                        // Can we callback asynchronously?
                        if (m_bAsyncAccess)
                        {
                            // Set the flag saying we failed due to
                            // progressive download
                            rbProgFail = TRUE;
                            HX_RELEASE(pBuffer);
                            // Schedule the callback
                            return FinishDoReadWithCallback(actual);
                        }
                        else
                        {
                            HX_RELEASE(pBuffer);
                            // Try synchronously
                            return FinishDoReadWithoutCallback(actual);
                        }
                    }
                    else
                    {
                        // Either:
                        // a) the fileformat either didn't have the Advise interface;
                        // b) the fileformat did have the interface and said it
                        //    was expecting a failure; or
                        // c) the fileformat had the Advise interface and said it
                        //    wasn't expecting a failure, but we've retried all the
                        //    times we can.
                        //
                        // Clear the progressive download failure flag
                        rbProgFail = FALSE;
                        // Finish the DoRead
                        return FinishDoRead(actual, pBuffer);
                    }
                }
            } /* if (m_pProgDownMon->HasBeenProgressive()) else */
        }
        else /* if (actual < ulCount) */
        {
            // The read succeeded
            //
            // Reset the not progressive retry count
            m_pProgDownMon->ResetNotProgressiveRetryCount();
            // Clear the progressive download failure flag
            rbProgFail = FALSE;

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
            // /Send resume notification if we sent a pause notification:
            if (m_bDidNotifyOfDownloadPause)
            {
                ReportChange(PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_RESUME);

                // /Have to put this here in case TLC has paused the player
                // which somehow seems to be killing our callbacks:
                m_pProgDownMon->BeginSizeMonitoring();
            }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
            
            // Finish the DoRead
            return FinishDoRead(actual, pBuffer);
        }
    }
    else /* if (m_pProgDownMon) */
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
    {
        // Clear the progressive download failure flag
        rbProgFail = FALSE;
        // Finish the DoRead
        return FinishDoRead(actual, pBuffer);
    }

#else /* #ifndef _MACINTOSH */
    HXBOOL bAtInterrupt = FALSE;
    if (m_pInterruptState)
    {
	bAtInterrupt = m_pInterruptState->AtInterruptTime();
    }

    m_bAsyncReadPending = TRUE;
    theErr = m_pDataFile->SafeRead(ulCount, bAtInterrupt);
#endif /* #ifndef _MACINTOSH #else */

    return theErr;
}

/************************************************************************
 *  Method:
 *	IHXFileObject::Write
 *  Purpose:
 *	Writes a buffer of data to the file and asynchronously notifies
 *	the caller via the IHXFileResponse interface passed in to Init,
 *	of the completeness of the operation.
 */
STDMETHODIMP CSimpleFileObject::Write(IHXBuffer* pBuffer)
{
    if (m_nFd == -1 || !(m_ulFlags & HX_FILE_WRITE))
    {
	return HXR_UNEXPECTED;
    }

    pBuffer->AddRef();
#ifdef _MACINTOSH
	HXBOOL bAtInterrupt = FALSE;
	if (m_pInterruptState)
	{
	    bAtInterrupt = m_pInterruptState->AtInterruptTime();
	}
    UINT32 actual = m_pDataFile->SafeWrite(pBuffer, bAtInterrupt);
#else
    UINT32 actual = m_pDataFile->Write(pBuffer);
#endif
    pBuffer->Release();

    if (actual > 0)
    {
	m_ulPos += actual;
    }

    if(actual == pBuffer->GetSize())
	m_pFileResponse->WriteDone(HXR_OK);
    else
	m_pFileResponse->WriteDone(HXR_FAILED);
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *	IHXFileObject::Seek
 *  Purpose:
 *	Seeks to an offset in the file and asynchronously notifies
 *	the caller via the IHXFileResponse interface passed in to Init,
 *	of the completeness of the operation.
 */
STDMETHODIMP CSimpleFileObject::Seek(ULONG32 ulOffset, HXBOOL bRelative)
{
    MLOG_GEN(NULL, "CSimpleFileObject::Seek(%lu,%lu) this=0x%08x tick=%lu\n",
             ulOffset, bRelative, this, HX_GET_BETTERTICKCOUNT());
    HX_LOG_BLOCK( "CSimpleFileObject::Seek" );

    if((m_nFd == -1) && m_bCanBeReOpened)
    {
	DPRINTF(0x5d000000, ("CSFO::Seek() -- _OpenFile()\n"));
	_OpenFile(m_ulFlags);
    }
    if (m_nFd != -1)
    {
	/* remove any pending callbacks */
        if (m_pStackCallback) m_pStackCallback->Cancel(m_pScheduler);

#if defined(HELIX_FEATURE_PROGDOWN)
        // Seeks can interrupt other seeks, and cancel them
        // out. Therefore, if we had scheduled a callback to
        // handle a failed seek or a failed read, then we need
        // to cancel that callback.
        if (m_pProgDownMon && m_pProgDownMon->IsCallbackPending())
        {
            m_pProgDownMon->CancelCallback();
        }
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

	AddRef(); // Make sure we do not destruct on possible ReadDone

	/*
	 *  Seek cancels pending Reads.
	 */
	if (m_bReadPending)
	{
	    ActualAsyncReadDone(HXR_CANCELLED, NULL);
	}

	int whence = SEEK_SET;

	if(bRelative)
	    whence = SEEK_CUR;

#ifndef _MACINTOSH
        // Save the pending seek parameters
        m_ulPendingSeekOffset = ulOffset;
        m_usPendingSeekWhence = (UINT16) whence;

        // Do the seek
        HX_RESULT seekDoneResult = HXR_OK;
        HX_RESULT result = DoSeek(seekDoneResult);

        // Undo the AddRef();
        Release();

	return ((result == HXR_OK) ? seekDoneResult : result);
#else
	HXBOOL bAtInterrupt = FALSE;
	if (m_pInterruptState)
	{
	    bAtInterrupt = m_pInterruptState->AtInterruptTime();
	}

	m_eSeekReason = EXTERNAL_SEEK;
	HX_RESULT seekDoneResult = m_pDataFile->SafeSeek(ulOffset, whence, bAtInterrupt);

	Release();

	return seekDoneResult;
#endif
    }
    return HXR_UNEXPECTED;
}

HX_RESULT CSimpleFileObject::DoSeek(REF(HX_RESULT) rSeekDoneResult)
{
    MLOG_PD(NULL, "CSimpleFileObject::DoSeek() this=0x%08x tick=%lu offset=%lu whence=%u\n",
            this, HX_GET_BETTERTICKCOUNT(), m_ulPendingSeekOffset, m_usPendingSeekWhence);
    HX_RESULT result = m_pDataFile->Seek(m_ulPendingSeekOffset,
                                         m_usPendingSeekWhence);

    if (result == HXR_OK)
    {
	if (m_usPendingSeekWhence == SEEK_SET)
	{
	    m_ulPos = m_ulPendingSeekOffset;
	}
	else
	{
	    m_ulPos += m_ulPendingSeekOffset;
	}
    }
#if defined(HELIX_FEATURE_PROGDOWN)
    // For the purposes of progressive download, we
    // will assume that seeking never fails. We will
    // add the assert below to catch anytime it does.
    // If we are finding that this assert is tripping,
    // then we need to add equivalent support in DoSeek()
    // for progressive download that we have in DoRead().
    HX_ASSERT(SUCCEEDED(result));
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

    rSeekDoneResult = ActualAsyncSeekDone(result);

    return result;
}

void CSimpleFileObject::SeekBackwards(UINT32 ulNumBytes)
{
    if (m_pDataFile && ulNumBytes)
    {
        // Get the current offset
        UINT32 ulCurOffset = m_pDataFile->Tell();
        // Make sure the number of bytes we 
        // are supposed to back up is not greater
        // than the current offset
        if (ulNumBytes > ulCurOffset) ulNumBytes = ulCurOffset;
        // Compute the new absolute offset
        UINT32 ulNewOffset = ulCurOffset - ulNumBytes;
        // Seek the data file to this offset
        m_pDataFile->Seek(ulNewOffset, SEEK_SET);
    }
}
HX_RESULT CSimpleFileObject::CheckForCorruptFile(HX_RESULT incomingError, UINT32 actual, HXBOOL bRead)
{
    HX_RESULT statusReturned = incomingError;
    HX_RESULT statResult = HXR_FAIL;
    HXBOOL   bErrorToBeReported = FALSE;
#if defined(HELIX_FEATURE_PROGDOWN)

	if (m_pProgDownMon && m_bProgDownEnabled)
    {

        UINT32 ulPrevFileSize = m_pProgDownMon->GetLastFileSize();
        m_pProgDownMon->MonitorFileSize(); // updates lastFileSize.
        UINT32 ulCurFileSize = m_pProgDownMon->GetLastFileSize();
	
	/* Did the media get corrupted or is no longer available */
	if (ulCurFileSize < ulPrevFileSize)
	{
            statusReturned = HXR_CORRUPT_FILE;
            bErrorToBeReported = TRUE;
	    goto exit;
	}
	else if (ulCurFileSize > ulPrevFileSize ||
		(m_pProgDownMon->HasBeenProgressive() || m_pProgDownMon->IsProgressive()))
	{
	    // is being progressive..so follow default progressive logic. 
	    // no need to check for corrupt file
	    goto exit;
	}
    }
#endif /* #if defned(HELIX_FEATURE_PROGDOWN) */

    // Read
    if (bRead)
    {
        UINT32 ulDesiredReadCount = m_ulPendingReadCount;

        // if actual > 0, m_ulPos has alredy been updated to account for partial
        // read, so subtract actual no. of bytes from the m_ulPendingReadCount count.
        if (actual > 0)
        {
            ulDesiredReadCount = actual < m_ulPendingReadCount ? m_ulPendingReadCount-actual : 0;
        }

        if (m_ulPos+ulDesiredReadCount <= m_ulSize)
        {
            statusReturned = HXR_CORRUPT_FILE;
            bErrorToBeReported = TRUE;
        }
    }
    else // Seek
    {
        UINT32 ulDesiredSeekPosition = 0;
        if (m_usPendingSeekWhence == SEEK_SET)
        {
            ulDesiredSeekPosition = m_ulPendingSeekOffset;
        }
        else
        {
            ulDesiredSeekPosition = m_ulPos + m_ulPendingSeekOffset;
        }

        if (ulDesiredSeekPosition <= m_ulSize)
        {
            statusReturned = HXR_CORRUPT_FILE;
            bErrorToBeReported = TRUE;
        }
    }

exit:
    if (bErrorToBeReported)
    {
        IHXErrorMessages *pErrMsg;
        if (m_pContext  &&
            HXR_OK == m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg))
        {
            pErrMsg->Report(HXLOG_ERR, statusReturned, 0, "Requested file is corrupted.", NULL);
            HX_RELEASE(pErrMsg);
        }
    }

    return statusReturned;
}

HX_RESULT CSimpleFileObject::FinishDoRead(UINT32 actual, REF(IHXBuffer*) pBuffer)
{
    HX_RESULT ret = HXR_OK;
    /*
     * If they get to the end of the file, then close it and reset.
     */
    if (actual > 0 && pBuffer)
    {
        m_ulPos += actual;
    }

#if defined(HELIX_FEATURE_PROGDOWN)
    if (m_ulSize && (!m_bProgDownEnabled || (m_pProgDownMon && !m_pProgDownMon->HasBeenProgressive())))
#else
    if (m_ulSize)
#endif
    {
        if(m_ulPos >= m_ulSize)
        {
	    DPRINTF(0x5d000000, ("CSFO::Read() -- m_pDataFile->Close()\n"));
	    m_pDataFile->Close();
	    m_nFd = -1;
	    if (m_pDescriptorReg)
	    {
	        m_pDescriptorReg->UnRegisterDescriptors(1);
	    }
	    m_bCanBeReOpened = 1;
        }
    }
    // corrupt file check (e.g. SD Card being taken out
    // while playback is in progress
    ret = (actual > 0 && pBuffer) ? HXR_OK : HXR_FAILED;
    if (!pBuffer || actual < m_ulPendingReadCount)
    {
        ret = CheckForCorruptFile(ret, actual, TRUE);
    }

    /* ignore return value from readdone! */
    ActualAsyncReadDone(ret, pBuffer);
    // Release our reference on the buffer!
    HX_RELEASE(pBuffer);

    return HXR_OK;
}

STDMETHODIMP CSimpleFileObject::InitDirHandler
(
    IHXDirHandlerResponse*    /*IN*/  pDirResponse
)
{
    m_pDirResponse = pDirResponse;
    m_pDirResponse->AddRef();
    m_pDirResponse->InitDirHandlerDone(HXR_OK);

    return HXR_OK;
}

STDMETHODIMP CSimpleFileObject::CloseDirHandler()
{
    // Members must not be accessed after async ...Done() response
    if (m_pDirResponse)
    {
	IHXDirHandlerResponse *pTmpDirResponse = m_pDirResponse;

	m_pDirResponse = NULL;
	pTmpDirResponse->CloseDirHandlerDone(HXR_OK);
	pTmpDirResponse->Release();
    }

    return HXR_OK;
}

STDMETHODIMP CSimpleFileObject::MakeDir()
{
    CHXString strFileName;
    HX_RESULT retVal = HXR_OK;

    UpdateFileNameMember();

    GetFullPathname(m_pFilename, &strFileName);

#if defined (_WINDOWS ) || defined (_WIN32)
    if (CreateDirectory(OS_STRING((const char*) strFileName), 0) == 0)
    {
	retVal = HXR_FAIL;
    }
#elif defined (_UNIX)
    if (mkdir((const char*) strFileName, 0755) < 0)
    {
	retVal = HXR_FAIL;
    }
#elif defined (_MACINTOSH)
    //XXXGH...don't know how to do this on Mac
    retVal = HXR_FAIL;
#else
    XHXDirectory* pHXDirectory = XHXDirectory::Construct(m_pContext, &m_pFileSystem->m_pCommonObj);

    retVal = HXR_OUTOFMEMORY;
    if (pHXDirectory)
    {
	retVal = HXR_FAIL;

	pHXDirectory->SetPath((const char*) strFileName);
	if (pHXDirectory->Create())
	{
	    retVal = HXR_OK;
	}

	delete pHXDirectory;
    }
#endif

    m_pDirResponse->MakeDirDone(retVal);

    return HXR_OK;
}

STDMETHODIMP CSimpleFileObject::ReadDir()
{
    const char* pDirname = 0;

    if (!m_pDirList)
    {
	CHXString strFileName;

	UpdateFileNameMember();

	GetFullPathname(m_pFilename, &strFileName);

	m_pDirList =
	    CFindFile::CreateFindFile((const char*)strFileName, 0, "*");

	if (!m_pDirList)
	{
	    m_pDirResponse->ReadDirDone(HXR_FAIL, 0);
	    return HXR_OK;
	}

	pDirname = m_pDirList->FindFirst();
    }
    else
    {
	pDirname = m_pDirList->FindNext();
    }

    if (!pDirname)
    {
	delete m_pDirList;
	m_pDirList = 0;
	m_pDirResponse->ReadDirDone(HXR_FILE_NOT_FOUND, 0);

	return HXR_OK;
    }

    HX_RESULT result;

    if (!m_pCommonClassFactory)
    {
	result = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	                                    (void **)&m_pCommonClassFactory);

	if (HXR_OK != result)
	{
	    return result;
	}
    }

    IHXBuffer* pBuffer = 0;

    result = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                   (void**)&pBuffer);

    if (HXR_OK != result)
    {
	return result;
    }

    pBuffer->Set((Byte*)pDirname, strlen(pDirname)+1);
    m_pDirResponse->ReadDirDone(HXR_OK, pBuffer);
    pBuffer->Release();

    return HXR_OK;
}

HX_RESULT
CSimpleFileObject::DirCreatePath( CHXString strPath )
{
    if (strPath.GetLength() == 0) return HXR_FAILED;

#if defined (_WINDOWS ) || defined (_WIN32)
    // Trim off trailing slash - ::CreateDirectory() will crash if present
    if ( strPath.ReverseFind( '\\' ) == strPath.GetLength() - 1 )
        strPath = strPath.Left( strPath.GetLength() - 1 );
#endif

    // Try to create the directory as-is
#if defined (_WINDOWS ) || defined (_WIN32)
    if ( 0 != ::CreateDirectory( OS_STRING((const char*)strPath), NULL ) )
        return HXR_OK;

    DWORD dwErr = ::GetLastError();
    if ( ERROR_ALREADY_EXISTS == dwErr )
        return HXR_OK;
    else if ( ERROR_ACCESS_DENIED == dwErr)
        return HXR_ACCESSDENIED;

#elif defined (_UNIX) || defined (_SOLARIS)
    if (mkdir((const char*) strPath, 0755) == 0)
        return HXR_OK;
    else if(errno == EEXIST)
        return HXR_OK;
    else if(errno == EACCES)
        return HXR_ACCESSDENIED;

#else  //In any other case
    return HXR_FAILED;

#endif

    // If that failed, trim off one sub dir and try to create that
    int nPos = strPath.ReverseFind( OS_SEPARATOR_CHAR );
    if ( -1 == nPos )
        return HXR_FAILED;

    CHXString szSubPath = strPath.Left( nPos + 1 );
    HX_RESULT res = DirCreatePath( szSubPath );
    if (HXR_OK != res)
        return res;
    // Creating the subdir succeeded, so now we should be able to create the original dir
#if defined (_WINDOWS ) || defined (_WIN32)
    if ( 0 != ::CreateDirectory( OS_STRING((const char*)strPath), NULL ) )
        return HXR_OK;
    else if (ERROR_ALREADY_EXISTS == ::GetLastError())
        return HXR_OK;
    else if ( ERROR_ACCESS_DENIED == ::GetLastError())
        return HXR_ACCESSDENIED;
    else
        return HXR_FAILED;

#elif defined (_UNIX) || defined (_SOLARIS)
    if (mkdir((const char*) strPath, 0755) == 0)
        return HXR_OK;
    else if (errno == EEXIST)
        return HXR_OK;
    else if (errno == EACCES)
        return HXR_ACCESSDENIED;
    else
        return HXR_FAILED;

#else  //In other case
    return HXR_FAILED;

#endif
}

STDMETHODIMP
CSimpleFileObject::InitNestedDirHandler
(
    IHXDirHandlerResponse*    /*IN*/  pDirResponse
 )
{
    m_pDirResponse = pDirResponse;
    m_pDirResponse->AddRef();
    m_pDirResponse->InitDirHandlerDone(HXR_OK);

    return HXR_OK;
}

STDMETHODIMP
CSimpleFileObject::MakeNestedDir()
{
    CHXString strFileName;
    HX_RESULT retVal = HXR_OK;

    UpdateFileNameMember();
    GetFullPathname(m_pFilename, &strFileName);

    retVal = DirCreatePath(strFileName);

    m_pDirResponse->MakeDirDone(retVal);
    return HXR_OK;
}

STDMETHODIMP
CSimpleFileObject::CloseNestedDirHandler()
{
    if (m_pDirResponse)
    {
        IHXDirHandlerResponse *pTmpDirResponse = m_pDirResponse;

        m_pDirResponse = NULL;
        pTmpDirResponse->CloseDirHandlerDone(HXR_OK);
        pTmpDirResponse->Release();
    }
    return HXR_OK;
}

/************************************************************************
 * Method:
 *	IHXFileObject::Stat
 * Purpose:
 *	Collects information about the file that is returned to the
 *	caller in an IHXStat object
 */
STDMETHODIMP CSimpleFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    MLOG_GEN(NULL, "CSimpleFileObject::Stat(0x%08x) this=0x%08x\n", pFileStatResponse, this);
    HX_LOG_BLOCK( "CSimpleFileObject::Stat" );

    struct stat StatBuffer;
    CHXString	strFileName;
    memset(&StatBuffer, 0, sizeof(struct stat));

#ifdef _MACINTOSH

        // if fstat fails
#ifdef _MAC_MACHO
	GetFullPathname(m_pFilename, &strFileName);
        int res = stat(strFileName, &StatBuffer);
#else
        int res = stat(m_pFilename, &StatBuffer);
#endif
	if ( res != 0 )
	{
		// return failure code
		return HXR_FAIL;
	}

#else
    if(m_nFd == -1)
    {
	CHXString   strURL;

	UpdateFileNameMember();
	strURL = m_pFilename;

	GetFullPathname(strURL, &strFileName);

	m_pDataFile->Bind((const char *)strFileName);
    }
    if (m_pDataFile->Stat(&StatBuffer) != HXR_OK)
    {
	pFileStatResponse->StatDone(HXR_FAIL,
	    0,
	    0,
	    0,
	    0,
	    0);
	return HXR_OK;
    }
#endif

    /*
     * XXXSMP:
     *    We we need this because RealPix opens up tons of files
     *    just to stat them.  If you are reading this comment
     *    and the date is past July 31, then please yell at
     *    Sujal.
     */
#if !defined(_MACINTOSH)
    if(m_nFd != -1)
    {
	DPRINTF(0x5d000000, ("CSFO::Stat() -- m_pDataFile->Close()\n"));
	if (m_pDescriptorReg)
	{
	    m_pDescriptorReg->UnRegisterDescriptors(1);
	}
	m_pDataFile->Close();
	m_nFd = -1;
	m_bCanBeReOpened = 1;
    }
#endif

    m_ulSize = StatBuffer.st_size;

    // /If we're progressively downloading and there was no ?filesize URL
    // parameter to tell us the size and we don't think we're done getting
    // all the bytes yet, then return FAIL since we don't have the answer:
    UINT32 ulTrueSize = m_ulSize;
    HX_RESULT hxrsltForStatDone = HXR_OK;

#if defined(HELIX_FEATURE_PROGDOWN)
    if (m_pProgDownMon  &&  m_pProgDownMon->IsProgressive())
    { 
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
        // /See if we know the complete file size from URL options:
        if (IsPrgDnCompleteFileSizeKnown())
        {
            // /If this asserts, then the URL?filesize=n is probably wrong:
            HX_ASSERT(m_ulPrgDnTotalFileSize >= m_ulSize);
            m_ulSize = m_ulPrgDnTotalFileSize;
            ulTrueSize = m_ulPrgDnTotalFileSize;
        }
        else
        {
            // /Since we don't know the file size, return HXR_FAIL:
            hxrsltForStatDone = HXR_FAIL;
            ulTrueSize = 0;
        }
#else  // /else of HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS
        // /Since we don't know the file size, return HXR_FAIL:
        hxrsltForStatDone = HXR_FAIL;
        ulTrueSize = 0;
#endif // /end else of HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
    } 
#endif // /HELIX_FEATURE_PROGDOWN.


    pFileStatResponse->StatDone(hxrsltForStatDone,
				ulTrueSize,
				StatBuffer.st_ctime,
				StatBuffer.st_atime,
				StatBuffer.st_mtime,
				StatBuffer.st_mode);
    return HXR_OK;
}

/************************************************************************
 * Method:
 *	IHXFileStat2::Stat
 * Purpose:
 *	Collects extended information about the file 
 */
STDMETHODIMP CSimpleFileObject::Stat(IHXFileStat2Response* pFileStat2Response)
{
#if defined(HELIX_FEATURE_SERVER) && defined(_UNIX)
    //printf("CSimpleFileObject::Stat #2 called\n");

    struct stat StatBuffer;
    CHXString	strFileName;
    memset(&StatBuffer, 0, sizeof(struct stat));

    if(m_nFd == -1)
    {
	CHXString   strURL;

	UpdateFileNameMember();
	strURL = m_pFilename;

	GetFullPathname(strURL, &strFileName);

	m_pDataFile->Bind((const char *)strFileName);
    }
    if (m_pDataFile->Stat(&StatBuffer) != HXR_OK)
    {
	pFileStat2Response->Stat2Done(HXR_FAIL, NULL);
    }

    if(m_nFd != -1)
    {
	DPRINTF(0x5d000000, ("CSFO::Stat2() -- m_pDataFile->Close()\n"));
	if (m_pDescriptorReg)
	{
	    m_pDescriptorReg->UnRegisterDescriptors(1);
	}
	m_pDataFile->Close();
	m_nFd = -1;
	m_bCanBeReOpened = 1;
    }

    pFileStat2Response->Stat2Done(HXR_OK, &StatBuffer);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif
}


/************************************************************************
 *  Method:
 *	IHXFileObject::Advise
 *  Purpose:
 *	To pass information to the File Object
 */
STDMETHODIMP CSimpleFileObject::Advise(ULONG32 ulInfo)
{
    MLOG_GEN(NULL, "CSimpleFileObject::Advise(%s) this=0x%08x\n",
             (ulInfo == HX_FILEADVISE_RANDOMACCESS ? "HX_FILEADVISE_RANDOMACCESS" :
                (ulInfo == HX_FILEADVISE_SYNCACCESS ? "HX_FILEADVISE_SYNCACCESS" :
                   (ulInfo == HX_FILEADVISE_ASYNCACCESS ? "HX_FILEADVISE_ASYNCACCESS" :
                      (ulInfo == HX_FILEADVISE_RANDOMACCESSONLY ? "HX_FILEADVISE_RANDOMACCESSONLY" :
                         (ulInfo == HX_FILEADVISE_ANYACCESS ? "HX_FILEADVISE_ANYACCESS" : "Unknown"))))),
             this);
    HX_RESULT retVal = HXR_FAIL;

#ifndef _MACINTOSH
    if (ulInfo == HX_FILEADVISE_SYNCACCESS)
    {
	m_bAsyncAccess = FALSE;
	retVal = HXR_OK;
    }
    else if (ulInfo == HX_FILEADVISE_ASYNCACCESS)
    {
	m_bAsyncAccess = TRUE;
	retVal = HXR_OK;
    }
    else if (ulInfo == HX_FILEADVISE_NETWORKACCESS)
    {
	if (m_bProxyMode)
	{
	    // We play it conservative here
	    retVal = HXR_ADVISE_NETWORK_ACCESS;
	}
	else
	{
	    retVal = HXR_ADVISE_LOCAL_ACCESS;
	}
    }
#if defined(HELIX_FEATURE_PROGDOWN)
    else if (ulInfo == HX_FILEADVISE_RANDOMACCESS)
    {
        retVal = HXR_OK;
	if (m_bProxyMode)
	{
             retVal = HXR_ADVISE_PREFER_LINEAR;
	}
	else if (m_pProgDownMon && m_bProgDownEnabled)
        {
            // Has this file ever been progressive?
            if (m_pProgDownMon->HasBeenProgressive())
            {
                // It has been in the past, but
                // is it currently progressive?
                if (m_pProgDownMon->IsProgressive())
                {
                    // Set the linear return value
                    retVal = HXR_ADVISE_PREFER_LINEAR;
                }
            }
            else
            {
                // Manually monitor the file size. This
                // will trigger HasBeenProgressive() to 
                // be true if the file is progressive
                m_pProgDownMon->MonitorFileSize();
                // Are we now progressive?
                if (m_pProgDownMon->HasBeenProgressive())
                {
                    // Set the linear return value
                    retVal = HXR_ADVISE_PREFER_LINEAR;
                    // Begin the monitoring of the file size
                    m_pProgDownMon->BeginSizeMonitoring();
                }
            }
        }
    }
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
#endif	// _MACINTOSH

    MLOG_GEN(NULL, "\treturn %s\n",
             (retVal == HXR_OK ? "HXR_OK" :
                (retVal == HXR_FAIL ? "HXR_FAIL" :
                   (retVal == HXR_ADVISE_PREFER_LINEAR ? "HXR_ADVISE_PREFER_LINEAR" :
                      "Unknown"))));

    return retVal;
}

/************************************************************************
 *	Method:
 *	    IHXFileObject::GetFileObjectFromPool
 *	Purpose:
 *      To get another FileObject from the same pool.
 */
STDMETHODIMP CSimpleFileObject::GetFileObjectFromPool (
    IHXGetFileFromSamePoolResponse* response
)
{
    HX_RESULT lReturnVal = HXR_FAILED;
    CSimpleFileObject* pFileObject = 0;
    CHXString new_path;
    CHXString strFileName;
    CHXString strURL;
    IUnknown* pUnknown = 0;

    char* pNewPath    = 0;
    char* pSeparator  = 0;

    UpdateFileNameMember();
    if(!m_pFilename)
    {
	pNewPath = new char[strlen(m_base_path) + 1];
	strcpy(pNewPath, m_base_path); /* Flawfinder: ignore */
    }
    else
    {
	strURL = m_pFilename;

	// Make a nice local file name from the URL!
	GetFullPathname(strURL, &strFileName);

	pNewPath = new char[strlen(strFileName) + 1];
	strcpy(pNewPath, (const char*)strFileName); /* Flawfinder: ignore */

	pSeparator = ::strrchr(pNewPath, OS_SEPARATOR_CHAR);
	if(pSeparator)
	{
	    /* Separator will be added in seturl */
	    *pSeparator = 0;
	}
	else
	{
	    // started w/filename. no separator implies no path
	    pNewPath[0] = 0;
	}
    }
    new_path = pNewPath;
    if (pNewPath)
    {
	delete [] pNewPath;
	pNewPath = 0;
    }

    pFileObject = new CSimpleFileObject(new_path,
					m_pFileSystem,
					m_pContext,
					m_ulMaxIterationLevel);

    if (!pFileObject)
    {
	return HXR_OUTOFMEMORY;
    }

    lReturnVal = pFileObject->QueryInterface(IID_IUnknown,
					     (void**)&pUnknown);

    response->FileObjectReady(lReturnVal == HXR_OK ?
			      HXR_OK : HXR_FAILED,
			      pUnknown);
    if(pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }

    return lReturnVal;
}

// IHXFileExists interface
/************************************************************************
 *	Method:
 *	    IHXFileExists::DoesExist
 *	Purpose:
 */
STDMETHODIMP CSimpleFileObject::DoesExist(const char* /*IN*/  pPath,
				IHXFileExistsResponse* /*IN*/  pFileResponse)
{
    HXBOOL bExists = FALSE;
    HXBOOL bPlusURL = FALSE;
    CHXString   strFileName;
    CHXString	strPath;
    CHXString   plusFileName;
    CHXString   plusPath;

    strPath = pPath;

    bPlusURL = HXXFile::IsPlusURL(pPath);
    if (bPlusURL)
    {
	INT32 index = strPath.ReverseFind('+');

	plusFileName = strPath.Right(strPath.GetLength() - (index+1));
	strPath = strPath.Left(index);
	index = strPath.ReverseFind('/');

	if (index >= 0)
	{
	    plusPath = strPath.Left(index+1);
	    plusPath = plusPath + plusFileName;
	}
	else
	{
	    plusPath = plusFileName;
	}

	HXXFile::GetReasonableLocalFileName(plusPath);
	GetFullPathname(plusPath, &plusFileName);
    }

    // Make a nice local file name from the URL!

    HXXFile::GetReasonableLocalFileName(strPath);
    GetFullPathname(strPath, &strFileName);

#ifdef _MACINTOSH
    CHXDataFile*	    pDataFile = 0; /* cross-platform file object */
    pDataFile = CHXDataFile::Construct(m_pContext);
    if (pDataFile->Open((const char*)strFileName,O_RDONLY) == HXR_OK)
    {
	if (bPlusURL)
	{
	    if (pDataFile->Open((const char*)plusFileName,O_RDONLY) == HXR_OK)
	    {
		bExists = TRUE;
	    }
	}
	else
	{
	    bExists = TRUE;
	}
    }
    delete pDataFile;
#else
    struct stat statbuf;

    if (!m_pDataFile)
    {
        InitDataFile();
    } 

    if (m_pDataFile)
    {
        m_pDataFile->Bind((const char *)strFileName);
        if (m_pDataFile->Stat(&statbuf) == 0)
        {
	    if (bPlusURL)
	    {
	        m_pDataFile->Bind((const char *)plusFileName);
	        if (m_pDataFile->Stat(&statbuf) == 0)
	        {
		    DPRINTF(0x5d000000, ("CSFO::DoesExist() -- both r TRUE\n"));
		    bExists = TRUE;
	        }
	    }
	    else
	    {
	        DPRINTF(0x5d000000, ("CSFO::DoesExist() -- TRUE\n"));
	        bExists = TRUE;
	    }
        }
        else
	    DPRINTF(0x5d000000, ("CSFO::DoesExist() -- FALSE\n"));
    }
#endif

    pFileResponse->DoesExistDone(bExists);
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    Private interface::OpenFile
 *	Purpose:
 *	    This common method is used from Init() and GetFileObjectFromPool()
 */
STDMETHODIMP CSimpleFileObject::_OpenFile(ULONG32     ulFlags)
{
    HX_LOG_BLOCK( "CSimpleFileObject::_OpenFile" );

    DPRINTF(0x5d000000, ("CSFO::_OpenFile(%lu)\n", ulFlags));
    HX_RESULT lReturnVal = HXR_OK;
    IHXUserImpersonation* pUserImpersonationThis = NULL;
    CHXString strFileName;
    CHXString strURL;

    m_ulFlags = ulFlags;

    UpdateFileNameMember();
    strURL = m_pFilename;

    // Make a nice local file name from the URL!
    GetFullPathname(strURL, &strFileName);

    if (m_pUnknownUserContext)
    {
	m_pUnknownUserContext->QueryInterface(IID_IHXUserImpersonation,
	    (void**)&pUserImpersonationThis);
    }

    if (pUserImpersonationThis)
    {
	// see ntauth plugin
	pUserImpersonationThis->Start();
    }

    UINT16 flags = 0;

#ifdef _MACINTOSH
    // use native modes
    if (ulFlags & HX_FILE_READ)
	flags |= O_RDONLY;
    if (ulFlags & HX_FILE_WRITE)
	flags |= O_WRONLY;
    if (ulFlags & HX_FILE_BINARY)
	flags |= O_BINARY;
    if (!ulFlags)
    {
	flags = O_RDONLY | O_BINARY;
	m_ulFlags = HX_FILE_READ | HX_FILE_BINARY;
    }

    // XXXGH must do HX_FILE_REPLACE for MAC
    m_pDataFile = (CMacAsyncFile*) CMacAsyncFile::Construct();
    m_pDataFile->SetAsyncResponse(m_pAsyncFileResponse);
    HXBOOL bAtInterrupt = FALSE;
    if (m_pInterruptState)
    {
         bAtInterrupt = m_pInterruptState->AtInterruptTime();
    }

    m_nFd = m_pDataFile->SafeOpen((const char*)strFileName,flags, 0, bAtInterrupt);
#else
    if (ulFlags & HX_FILE_READ)
	flags |= HX_FILEFLAG_READ;
    if (ulFlags & HX_FILE_WRITE)
	flags |= HX_FILEFLAG_WRITE;
    if (ulFlags & HX_FILE_BINARY)
	flags |= HX_FILEFLAG_BINARY;
    if (!ulFlags)
    {
	flags = HX_FILEFLAG_READ | HX_FILEFLAG_BINARY;
	m_ulFlags = HX_FILE_READ | HX_FILE_BINARY;
    }
    DPRINTF(0x5d000000, ("CSFO::_OpenFile() -- flags(%u)\n", flags));

    HX_ASSERT(m_pDataFile);

    MLOG_GEN(NULL, "CSimpleFileObject::_OpenFile() this=0x%08x tick=%lu filename=%s\n",
             this, HX_GET_BETTERTICKCOUNT(), (const char*) strFileName);
    m_pDataFile->Bind((const char *)strFileName);
    if (HXR_OK == (lReturnVal = m_pDataFile->Open((UINT16)m_ulFlags)))
    {
	m_nFd = m_pDataFile->GetFd();
	DPRINTF(0x5d000000, ("CSFO::_OpenFile() "
		"-- fd(%d), filename(%s)\n", m_nFd,
		((const char *)strFileName) ? (const char *)strFileName : "NULL"));
    }
    else
    {
	m_nFd = -1;
    }

#endif

    if (pUserImpersonationThis)
    {
	pUserImpersonationThis->Stop();
    }

    /* InitDone may result in close/destruction of this object.Mime mapper
     * is only interested in the mimetype. This is the sequence of events:
     * InitDone->FinishSetup->Close MimeMapper.. Release it. load right ff
     * hand over file system to the file format.
     * So by the time control returns back here, this object may already be
     * closed, resulting in m_nFd = -1
     */
    if (m_nFd != -1)
    {
	lReturnVal = HXR_OK;
    }
    else if (pUserImpersonationThis)
    {
	lReturnVal = HXR_NOT_AUTHORIZED;
    }
    else if (lReturnVal != HXR_NO_MORE_FILES)
    {
	DPRINTF(0x5d000000, ("Error: file missing\n"));
	lReturnVal = HXR_DOC_MISSING;
    }

    HX_RELEASE(pUserImpersonationThis);

    if (lReturnVal == HXR_OK)
    {
	if (!m_pDescriptorReg)
	{
	    m_pContext->QueryInterface(IID_IHXDescriptorRegistration,
						(void **)&m_pDescriptorReg);
	}

	if (m_pDescriptorReg)
	{
	    m_pDescriptorReg->RegisterDescriptors(1);
	}
    }

    return lReturnVal;
}

void
CSimpleFileObject::UpdateFileNameMember()
{
    const char* pURL;

    if (m_base_path.GetLength() > 0)
    {
       /* Running on the Server, do this without wasting CPU! */
       if (m_pRequest->GetURL(pURL) != HXR_OK)
       {
	   HX_VECTOR_DELETE(m_pFilename);
	   return;
       }
    //XXXLCM use urlrep here?
       UINT32 ulIndex = 0;
       while (pURL[ulIndex] != 0)
       {

	   // HP - allow '$' in directory/file names
	   //
	   // still need to take care of that obsolete $ sign option:
	   // rtsp://moe.cr.prognet.com/ambush.rm$1:00
	   // time after the $ is assumed to be the start time.
	   //
	   // the solution is to compare the string following the $ to
	   // a properly formed time. If the string is a time and only
	   // a time, then we know its the old-style start-time option
	   // otherwise, '$' is part of the directory/file name and we
	   // will keep it.
	   if (pURL[ulIndex] == '$')
	   {
	       const char* pszOption = &pURL[ulIndex + 1];
	       if (::TimeParse(pszOption))
	       {
		   goto strip_and_duplicate;
	       }
	   }
	   else
	   {

#if !defined(_MACINTOSH) && !defined(_MAC_UNIX)	// ? # and + are legal in Mac file names and shouldn't be stripped (yes, we're seeing a file name, not a URL here)

	       switch (pURL[ulIndex])
	       {


	       case '?':
	       case '#':
	       case '+':
		   goto strip_and_duplicate;
	       default:
		   break;
	       }
#endif
	   }
	   ulIndex++;
       }
       HX_VECTOR_DELETE(m_pFilename);

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
       m_pFilename = new_string(pURL); // on Mac, new_path_string mangles legal filenames which have / or \ in them
#else
       m_pFilename = new_path_string(pURL);
#endif

       return;

strip_and_duplicate:
       HX_VECTOR_DELETE(m_pFilename);

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
       m_pFilename = new_string(pURL); // on Mac, new_path_string mangles legal filenames which have / or \ in them
#else
       m_pFilename = new_path_string(pURL);
#endif

       m_pFilename[ulIndex] = 0;

       return;
    }

    pURL = 0;

    if (m_pRequest)
    {
#if defined(_UNIX) && !defined(_MAC_UNIX)
	HXBOOL bIsAbsolute = FALSE;
#endif
	if (m_pRequest->GetURL(pURL) != HXR_OK)
	{
	    HX_VECTOR_DELETE(m_pFilename);

	    m_pFilename = 0;
	}

	HXBOOL bVerbatimFileName = FALSE;

#ifdef _MAC_UNIX
	bVerbatimFileName = TRUE; // In/out argument passed to HXXFile::GetReasonableLocalFileName() is expected to be an URL.
#else
	IHXValues* pReqHeader = NULL;
	if (SUCCEEDED(m_pRequest->GetRequestHeaders(pReqHeader)) &&
						    pReqHeader)
	{
	    ULONG32 ulVal = 0;
	    if (SUCCEEDED(pReqHeader->GetPropertyULONG32("VerbatimFileName",
							 ulVal)))
	    {
		bVerbatimFileName = (ulVal ? TRUE : FALSE);
	    }
	}
	HX_RELEASE(pReqHeader);
#endif

	CHXString strFilename;

	if (bVerbatimFileName)
	{
	    if (pURL)
	    {
		strFilename = pURL;
	    }
	}
	else
	{
#if defined(_UNIX) && !defined(_MAC_UNIX)
	    bIsAbsolute = ::strstr(pURL, "file:///") == pURL;
#endif
	    CHXURL* pCHXURL = new CHXURL(pURL, m_pCommonClassFactory);
	    if (pCHXURL)
	    {
		HXBOOL bURLUnderstood = FALSE;
		IHXValues* pHeader = pCHXURL->GetProperties();
		if (pHeader)
		{
		    IHXBuffer* pUrlBuffer = NULL;
		    if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_FULLPATH, pUrlBuffer) &&
			pUrlBuffer)
		    {
                        strFilename = (const char*)pUrlBuffer->GetBuffer();
			HX_RELEASE(pUrlBuffer);
			bURLUnderstood = TRUE;
		    }
		    HX_RELEASE(pHeader);
		}
		delete pCHXURL;

		// If URL was not udersttod by the parser, assume the "URL"
		// is a file-name/path to be used directly.
		if (!bURLUnderstood)
		{
		    strFilename = pURL;
		}
	    }

	    /*
	    * Strip off the parameters
	    */
	    if (HXXFile::IsPlusURL(pURL))
	    {
		INT32 index = strFilename.ReverseFind('+');

		if (index >= 0)
		{
		    strFilename = strFilename.Left(index);
		}
	    }
	}

	HXXFile::GetReasonableLocalFileName(strFilename);

	HX_VECTOR_DELETE(m_pFilename);

#if defined(_CARBON) || defined(_MAC_UNIX)
	// Under Carbon, GetReasonableLocalFileName returns a proper Mac path, possibly including
	// slashes which shouldn't be mucked with.
	m_pFilename = new_string((const char *) strFilename);
#else
# ifdef _UNIX
	if(bIsAbsolute)
	{
	    m_pFilename = new char[strFilename.GetLength()+2];
	    *m_pFilename = '/';
	    ::strcpy(m_pFilename+1, (const char *)strFilename);
	} else
# endif
	{
	  m_pFilename = new_path_string((const char*)strFilename);
	}
#endif
    }
}

STDMETHODIMP CSimpleFileObject::SetRequest
(
    IHXRequest* pRequest
)
{
    if (!pRequest)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pRequest);

    m_pRequest = pRequest;
    m_pRequest->AddRef();

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    const char* pFname = NULL;
    m_pRequest->GetURL(pFname);
    // /If this is progressively-downloading file, file's total size might be
    // in the URL prarameters:
    const char* pFs = "?filesize=";
    UINT32 ulFsLen = strlen(pFs);
    const char* pFilesize = strstr(pFname, pFs);
    if (pFilesize  &&  strlen(pFilesize) > ulFsLen  &&
            // /Make sure there's a number there:
            '0' <= pFilesize[ulFsLen]  &&  '9' >= pFilesize[ulFsLen])
    {
        m_ulPrgDnTotalFileSize = atol(&pFilesize[ulFsLen]);
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    UpdateFileNameMember();

    HX_RESULT res = InitDataFile();
 
#if defined(HELIX_FEATURE_FILESYSTEM_LOCAL_FD)
    // To support "fd" scheme, the m_pDataFile object must implement IHXRequestHandler
    // to receive and parse the URL request for the file descriptor parameters.
    if (res == HXR_OK)
    {
	const char* pszURL = NULL;
	m_pRequest->GetURL(pszURL);
	if (strncmp(pszURL, "fd://fileinput", 14) == 0)
	{
	    IHXRequestHandler* pRequestHandler = NULL;
	    res = m_pDataFile->QueryInterface(IID_IHXRequestHandler, (void**)&pRequestHandler);
	    if (HXR_OK == res)
	    {
		pRequestHandler->SetRequest(m_pRequest);
	    }
	    HX_RELEASE(pRequestHandler);
	}
	
	if (res != HXR_OK)
	{
	    if (m_pDataFile)
	    {
		m_pDataFile->Close();
		HX_RELEASE(m_pDataFile);
	    }
	    HX_RELEASE(m_pRequest);
	}
    }
#endif // HELIX_FEATURE_FILESYSTEM_LOCAL_FD
    
    return res;
}

HX_RESULT
CSimpleFileObject::InitDataFile()
{
	HX_RESULT res = HXR_OK;

    if (m_pDataFile)
    {
        m_pDataFile->Close();
        HX_RELEASE(m_pDataFile);
    }
    
#if !defined _MACINTOSH
	// Check for fileproxy protocol, query for IHXDataFile
	IHXRequestContext* pRequestContext=NULL;
	const char* pURL;
	
	if (m_pRequest &&
	    m_pRequest->GetURL(pURL) == HXR_OK &&
	    strncmp(pURL, "fileproxy:", strlen("fileproxy:")) == 0 &&
	    m_pRequest->QueryInterface(IID_IHXRequestContext, (void**)&pRequestContext) == HXR_OK)
	{
	    IUnknown* pUnknown;

	    m_bProxyMode = TRUE;

	    if (pRequestContext->GetRequester(pUnknown) == HXR_OK)
	    {
		if (pUnknown->QueryInterface(IID_IHXDataFile, (void**) &m_pDataFile) == HXR_OK)
		{
		    HXLOGL3(HXLOG_GENE, "CSimpleFileObject: found fileproxy datafile");
		}
		else
		{
		    res = HXR_FAIL;
		}
	        HX_RELEASE(pUnknown);
	    }
	    HX_RELEASE(pRequestContext);
	}

	if (!m_bProxyMode)
	{
        if (m_pDataFile)
        {
            m_pDataFile->Close();
            HX_RELEASE(m_pDataFile);
        }
	    IHXDataFileFactory* pDFFact = new HXDataFileFactory;;
	    pDFFact->AddRef();
	    DPRINTF(0x5d000000, ("CSFO::CSFO() -- after QI\n"));
	    pDFFact->CreateFile(m_pDataFile, 
            m_pContext,
	        m_pFileSystem->m_pCommonObj, 
            m_pFileSystem->m_bDisableMemoryMappedIO,
	        m_pFileSystem->m_ulChunkSize, 
            m_pFileSystem->m_bEnableFileLocking,
	        TRUE);  // Always prefer async I/O
	    
        if (!m_pDataFile)
        {
#ifndef _WIN16
		    DPRINTF(0x5d000000, ("Internal Error s/546\n"));
#endif
		    res = HXR_INVALID_FILE;
		}
	    HX_RELEASE(pDFFact);
	}
#endif /* !defined _MACINTOSH */
	return res;
}


STDMETHODIMP CSimpleFileObject::GetRequest
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

static HXBOOL DoRename(const char* pOldName,
		       const char* pNewName,
		       IUnknown*   pContext,
		       IUnknown**  ppCommonObj)
{
    HXBOOL ret = FALSE;

#if defined(_MACINTOSH) || defined(_SYMBIAN)
    XHXDirectory* pHXDir =  XHXDirectory::Construct(pContext, ppCommonObj);

    if (pHXDir)
    {
	if (SUCCEEDED(pHXDir->Rename(pOldName, pNewName)))
	{
	    ret = TRUE;
	}

	delete pHXDir;
    }
#elif defined(WIN32_PLATFORM_PSPC)
    if (MoveFile(OS_STRING(pOldName), OS_STRING(pNewName)) != 0)
    {
	ret = TRUE;
    }
#elif defined(_BREW)
    IShell*& pIShell =(IShell*&)HXGlobalPtr::Get(&::g_pIShell);
    IFileMgr * pIFileMgr = NULL;
    if (ISHELL_CreateInstance(pIShell, AEECLSID_FILEMGR, (void **)&pIFileMgr) == SUCCESS)
    {			
	if( IFILEMGR_Rename(pIFileMgr, pOldName, pNewName) == SUCCESS)
	{
	    ret = true;
	}
	IFILEMGR_Release(pIFileMgr);
    }

#else
    if (rename(pOldName, pNewName) == 0)
    {
	ret = TRUE;
    }
#endif /* !defined(WIN32_PLATFORM_PSPC) */

    return ret;
}

STDMETHODIMP CSimpleFileObject::Rename
(
    const char* pFilename
)
{
    CHXString strFilename;
    CHXString newFilename;

    UpdateFileNameMember();
    strFilename = m_pFilename;

    INT32 index = strFilename.ReverseFind(OS_SEPARATOR_CHAR);

    if (index != -1)
    {
	newFilename = strFilename.Left(index+1);
	newFilename += pFilename;
    }
    else
    {
	newFilename = pFilename;
    }

    GetFullPathname((const char*)newFilename, &newFilename);
    GetFullPathname((const char*)strFilename, &strFilename);

    if (DoRename(strFilename, newFilename, m_pContext, &m_pFileSystem->m_pCommonObj) == FALSE)
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}

STDMETHODIMP CSimpleFileObject::Move
(
    const char* pFilename
)
{
    CHXString strFilename;
    CHXString newFilename;
    HX_RESULT retVal = HXR_OK;

    UpdateFileNameMember();

    newFilename = pFilename;
    HXXFile::GetReasonableLocalFileName(newFilename);

    GetFullPathname((const char*) newFilename, &newFilename);
    GetFullPathname(m_pFilename, &strFilename);

    if (DoRename(strFilename, newFilename, m_pContext, &m_pFileSystem->m_pCommonObj) == FALSE)
    {
#ifdef _UNIX
	// We'll attempt to move the file manually since on UNIX
	// rename accross devices (file systems) is not supported
	FILE* pSource = fopen((const char*) strFilename, "rb");
	FILE* pTarget = fopen((const char*) newFilename, "rb");

	retVal = HXR_FAIL;

	if (pTarget)
	{
	    // Target file already exists -> fail.
	    fclose(pTarget);
	    pTarget = NULL;
	}
	else
	{
	    pTarget = fopen((const char*) newFilename, "wb");
	}

	if (pSource && pTarget)
	{
	    retVal = HXR_OK;
	}

	if (retVal == HXR_OK)
	{
	    const ULONG32 COPY_BUF_SIZE = 1024;
	    UINT8 pBuffer[COPY_BUF_SIZE];
	    size_t bufLen;

	    do
	    {
		bufLen = fread(pBuffer, 1, COPY_BUF_SIZE, pSource);
		if (bufLen > 0)
		{
		    if (fwrite(pBuffer, 1, bufLen, pTarget) != bufLen)
		    {
			retVal = HXR_FAIL;
			break;
		    }
		}
	    } while (bufLen == COPY_BUF_SIZE);
	}

	// Make sure no erorrs occured
	if (retVal == HXR_OK)
	{
	    retVal = HXR_FAIL;

	    if (feof(pSource) &&
		(ferror(pSource) == 0) &&
		(ferror(pTarget) == 0))
	    {
		retVal = HXR_OK;
	    }
	}

	if (pSource)
	{
	    fclose(pSource);
	    pSource = NULL;
	}

	// Remove the source file - only if copied OK
	if (retVal == HXR_OK)
	{
	    if (remove((const char*) strFilename) != 0)
	    {
		retVal = HXR_FAIL;
	    }
	}

	if (pTarget)
	{
	    fclose(pTarget);
	    pTarget = NULL;

	    // In case of failure, remove the target
	    if (retVal != HXR_OK)
	    {
		remove((const char*) newFilename);
	    }
	}
#else	// _UNIX
	retVal = HXR_FAIL;
#endif	// _UNIX
    }

    return retVal;
}

STDMETHODIMP
CSimpleFileObject::Remove()
{
#ifdef _MACINTOSH
    CHXString strFilename;

    UpdateFileNameMember();
    strFilename = m_pFilename;

    GetFullPathname((const char*)strFilename, &strFilename);

    if (unlink((const char*)strFilename) != 0)
    {
	return HXR_FAIL;
    }

    return HXR_OK;
#else
    UpdateFileNameMember();
    CHXString strFileName;
    CHXString strUrl = m_pFilename;
    // Make a nice local file name from the URL!

    HXXFile::GetReasonableLocalFileName(strUrl);
    GetFullPathname(strUrl, &strFileName);

    m_pDataFile->Bind((const char*)strFileName);
    HX_RESULT res = m_pDataFile->Delete();
    if (res == HXR_OK)
    {
	m_nFd = -1;
    }
    return res;
#endif
}

void
CSimpleFileObject::GetFullPathname(const char* pPath, CHXString* pPathname)
{

    if (m_base_path.GetLength() > 0)
    {
	INT32 lLevel = 0;
	for (const char* pTemp = pPath; *pTemp; pTemp++)
	{
	    /*
	     * Increment directory level if we have a slash, and it's
	     * not a leading slash, and it's not right after a slash
	     * (doesn't count on most OSes).
	     */
	    if (((*pTemp == OS_SEPARATOR_CHAR) ||
	         (*pTemp == '/')) && (pTemp != pPath) &&
	    	 ((*(pTemp-1)) != OS_SEPARATOR_CHAR) &&
	    	 ((*(pTemp-1)) != '/'))

	    {
		lLevel++;
	    }

	    if ((pTemp != pPath) &&
	    	(*pTemp == '.') &&
	    	(*(pTemp - 1) == '.'))

	    {
		lLevel--;
		if (((*(pTemp + 1)) == OS_SEPARATOR_CHAR) ||
		    ((*(pTemp + 1)) == '/'))
		{
		    pTemp++;
		}
		if (lLevel < 0)
		{
		    *pPathname = "!$InvalidPath";
		    return;
		}
	    }
	}

	if (*pPath)
	{
#if defined(_MACINTOSH) && !defined(_CARBON)

	    CHXString strPath = HXEscapeUtil::UnEscape(pPath);
            pPath = (const char*)strPath;
            UINT32 ulPathLen = strPath.GetLength();
#else
            UINT32 ulPathLen = strlen(pPath);
#endif
            UINT32 ulBaseLen = m_base_path.GetLength();
            char* pStr = new char[ulBaseLen + ulPathLen + 2];
            char* pTmp = pStr;
            if (ulBaseLen)
            {
                memcpy(pTmp, (const char*)m_base_path, ulBaseLen); /* Flawfinder: ignore */
                pTmp += ulBaseLen;
            }
            *pTmp = OS_SEPARATOR_CHAR;
            pTmp++;
            memcpy(pTmp, pPath, ulPathLen + 1); /* Flawfinder: ignore */
            CHXString strResult(pStr, ulBaseLen + ulPathLen + 2);
            *pPathname = strResult;
            delete[] pStr;
	}
	else
	{
	    *pPathname = m_base_path;
	}
    }
    else
    {

#if defined (_MACINTOSH) && !defined(_CARBON)
        CHXString strPath = HXEscapeUtil::UnEscape(pPath);
#else
        CHXString strPath(pPath, strlen(pPath));
#endif
        *pPathname = strPath;
    }
}

HX_RESULT
CSimpleFileObject::ActualAsyncReadDone(HX_RESULT result, IHXBuffer* pBuffer)
{
    MLOG_PD(NULL, "CSimpleFileObject::ActualAsyncReadDone(0x%08x,0x%08x) tick=%lu\n",
            result, pBuffer, HX_GET_BETTERTICKCOUNT());
    HX_LOG_BLOCK( "CSimpleFileObject::ActualAsyncReadDone" );

    m_bReadPending = FALSE;
    m_bAsyncReadPending = FALSE;

    // Let the file response sink know about the buffer...
    HX_RESULT rv = m_pFileResponse->ReadDone(result, pBuffer);

    MLOG_PD(NULL, "\treturning 0x%08x from ActualAsyncReadDone() tick=%lu\n",
            rv, HX_GET_BETTERTICKCOUNT());

    return rv;
}

HX_RESULT
CSimpleFileObject::ActualAsyncSeekDone(HX_RESULT result)
{
    // corrupt file check (e.g. SD Card being taken out
    // while playback is in progress
    if (result != HXR_OK)
    {
        result = CheckForCorruptFile(result);
    }

#ifndef _MACINTOSH
    MLOG_PD(NULL, "CSimpleFileObject::ActualAsyncSeekDone(0x%08x) tick=%lu\n",
            result, HX_GET_BETTERTICKCOUNT());
    HX_RESULT rv = m_pFileResponse->SeekDone(result);
    MLOG_PD(NULL, "\treturning 0x%08x from ActualAsyncSeekDone() tick=%lu\n",
            rv, HX_GET_BETTERTICKCOUNT());
    return rv;
#else
    switch (m_eSeekReason)
    {
	case REINIT_SEEK:
	    return m_pFileResponse->InitDone(!result ? HXR_OK : HXR_FAIL);
	case EXTERNAL_SEEK:
	    return m_pFileResponse->SeekDone(result);
	case PRE_TELL_SEEK:
	    {
		m_ulSize = m_pDataFile->Tell();
		HXBOOL bAtInterrupt = FALSE;
		if (m_pInterruptState)
		{
		    bAtInterrupt = m_pInterruptState->AtInterruptTime();
		}

		m_eSeekReason = POST_TELL_SEEK;

		/* If we need to close the file, why waste time in seeking back
		 * to the original position
		 */
		if (!m_bFileToBeClosed)
		{
		    m_pDataFile->SafeSeek(m_ulPreSeekPosition,SEEK_SET);
		}
		else
		{
		    ActualAsyncSeekDone(result);
		}
	    }
	    break;
	case POST_TELL_SEEK:
	    {
		if (m_bFileToBeClosed && m_pDataFile)
		{
		    m_bFileToBeClosed = FALSE;
		    HX_RELEASE(m_pDataFile);
		    m_nFd = -1;
		}

		struct stat StatBuffer;
		StatBuffer.st_size = m_ulSize;
		StatBuffer.st_ctime = 0;
		StatBuffer.st_atime = 0;
		StatBuffer.st_mtime = 0;
		StatBuffer.st_mode = 0;
		if (m_pFileStatResponse)
		{
		    // Access to members after async ..Done() call is invalid
		    IHXFileStatResponse *pTmpFileStatResponse = m_pFileStatResponse;

		    m_pFileStatResponse = NULL;
		    pTmpFileStatResponse->StatDone( HXR_OK,
						    StatBuffer.st_size,
						    StatBuffer.st_ctime,
						    StatBuffer.st_atime,
						    StatBuffer.st_mtime,
						    StatBuffer.st_mode);
		    pTmpFileStatResponse->Release();
		}
	    }
	    break;
	default:
	    HX_ASSERT(FALSE);
	    return HXR_FAIL;
    }

    return HXR_OK;
#endif
}

#ifdef _MACINTOSH
// SMPLAsyncResponse
CSimpleFileObject::SMPLAsyncResponse::SMPLAsyncResponse(CSimpleFileObject* pFileObject)
{
    m_pSMPLFileObject = pFileObject;
}

CSimpleFileObject::SMPLAsyncResponse::~SMPLAsyncResponse()
{
}

HX_RESULT
CSimpleFileObject::SMPLAsyncResponse::AsyncReadDone (HX_RESULT result, IHXBuffer* pBuffer)
{
    m_pSMPLFileObject->ActualAsyncReadDone(result, pBuffer);
    return HXR_OK;
}

HX_RESULT
CSimpleFileObject::SMPLAsyncResponse::AsyncSeekDone (HX_RESULT result)
{
    m_pSMPLFileObject->ActualAsyncSeekDone(result);
    return HXR_OK;
}

#endif /*_MACINTOSH*/

#if defined(HELIX_FEATURE_PROGDOWN)

STDMETHODIMP
CSimpleFileObject::ProgressiveStatCallback()
{
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    UINT32 ulPrevFileSize = m_pProgDownMon->GetLastFileSize();
    m_pProgDownMon->MonitorFileSize(); // updates lastFileSize.
    UINT32 ulCurFileSize = m_pProgDownMon->GetLastFileSize();
    MaybeDoProgressiveDownloadStatusRept(ulPrevFileSize, ulCurFileSize);
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    return HXR_OK;
}

STDMETHODIMP
CSimpleFileObject::ProgressiveCallback()
{
    if (m_ulCallbackState == CallbackStateSeek)
    {
        MLOG_PD(NULL, "CSimpleFileObject::ProgressiveCallback() this=0x%08x tick=%lu state=%s\n",
                this, HX_GET_BETTERTICKCOUNT(), "CallbackStateSeek");
        // Make sure we do not destruct during SeekDone()
        AddRef();
        // Do the seek
        HX_RESULT seekDoneResult = HXR_OK;
        DoSeek(seekDoneResult);
        // Undo the AddRef();
        Release();
    }
    else if (m_ulCallbackState == CallbackStateRead)
    {
        MLOG_PD(NULL, "CSimpleFileObject::ProgressiveCallback() this=0x%08x tick=%lu state=%s\n",
                this, HX_GET_BETTERTICKCOUNT(), "CallbackStateRead");
        // Make sure we do not destruct in ReadDone()
        AddRef();
        // Call DoRead()
        HXBOOL bProgFail = FALSE;
        DoRead(bProgFail);
        // Undo the AddRef()
        Release();
    }

    return HXR_OK;
}

HXBOOL CSimpleFileObject::RequireFullRead()
{
    HXBOOL bRet = FALSE;

    if (m_pFileResponse)
    {
        // QueryInterface for the IHXAdvise interface
        IHXAdvise* pAdvise = NULL;
        m_pFileResponse->QueryInterface(IID_IHXAdvise, (void**) &pAdvise);
        if (pAdvise)
        {
            if (SUCCEEDED(pAdvise->Advise(HX_FILERESPONSEADVISE_REQUIREFULLREAD)))
            {
                bRet = TRUE;
            }
        }
        HX_RELEASE(pAdvise);
    }

    return bRet;
}

HX_RESULT CSimpleFileObject::FinishDoReadWithCallback(UINT32 actual)
{
    // Seek backwards if necessary
    SeekBackwards(actual);
    // Set the callback state
    m_ulCallbackState = CallbackStateRead;
    // Schedule a callback to try the read again
    m_pProgDownMon->ScheduleCallback();

    return HXR_OK;
}

HX_RESULT CSimpleFileObject::FinishDoReadWithoutCallback(UINT32 actual)
{
    // Seek backwards if necessary
    SeekBackwards(actual);
    // Since we can't employ callback when m_bAsyncAccess is FALSE,
    // then we must sleep. We'll sleep for 100ms (or 100000 microsec)
    microsleep(100000);

    return HXR_OK;
}

#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

void CSimpleFileObject::StackCallback(void* pArg)
{
    MLOG_PD(NULL, "CSimpleFileObject::StackCallback() tick=%lu\n", HX_GET_BETTERTICKCOUNT());
    if (pArg)
    {
        CSimpleFileObject* pFileObject = (CSimpleFileObject*) pArg;
        pFileObject->AddRef();
        HXBOOL bProgFail = FALSE;
        pFileObject->DoRead(bProgFail);
        pFileObject->Release();
    }
}



#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)

void
CSimpleFileObject::MaybeDoProgressiveDownloadStatusRept(UINT32 ulPrevFileSize,
                                                        UINT32 ulCurFileSize)
{
    UINT32 ulCurTime = HX_GET_BETTERTICKCOUNT();
    HXBOOL bDurationReached = FALSE;
    HXBOOL bFileSizeFullyDownloaded = IsPrgDnCompleteFileSizeKnown()? 
            (ulCurFileSize >= m_ulPrgDnTotalFileSize) : FALSE;
    if (!bFileSizeFullyDownloaded  &&
            m_pBytesToDur  &&  (m_ulTimeOfLastBytesToDur==0  ||
            ulCurTime - m_ulTimeOfLastBytesToDur > m_ulStatusUpdateGranularityInMsec))
    {
        if (SUCCEEDED(m_pBytesToDur->GetFileDuration(
                // /OK if this is still HX_PROGDOWNLD_UNKNOWN_FILE_SIZE:
                m_ulPrgDnTotalFileSize,
                m_ulPrgDnTotalFileDur)))
        {
            if (m_ulPriorReportedTotalFileDur != m_ulPrgDnTotalFileDur)
            {
                ReportTotalDurChanged();
            }
        }

        m_ulTimeOfLastBytesToDur = ulCurTime;
        // /Report new size's associated duration to observers:
        // /Get The FF to convert the bytes to associated dur:
        if (SUCCEEDED(m_pBytesToDur->ConvertFileOffsetToDur(
                ulCurFileSize,
                // /It's OK if the following is ...UNKNOWN...:
                m_ulPrgDnTotalFileSize,
                /*REF*/ m_ulCurrentDurOfBytesSoFar)))
        {
            if (ulPrevFileSize < ulCurFileSize)
            {
                ReportCurrentDurChanged();
            }
            if (HX_PROGDOWNLD_UNKNOWN_DURATION !=
                    m_ulCurrentDurOfBytesSoFar  &&
                    HX_PROGDOWNLD_UNKNOWN_DURATION !=
                    m_ulPrgDnTotalFileDur &&
                    m_ulCurrentDurOfBytesSoFar >= m_ulPrgDnTotalFileDur)
            {
                // /We've exceeded our duration so claim that we're done:
                bDurationReached = TRUE;
            }
        }
        else // /Failed to convert bytes to dur; report that:
        {
            ReportChange(PRDNFLAG_REPORT_CUR_DUR_UNKNOWN);
        }
    }
    // /Now, report if we're finished downloading:
    if (!m_bDownloadCompleteReported  &&  m_pBytesToDur  &&
            (bDurationReached  ||
            (IsPrgDnCompleteFileSizeKnown()  &&
            ulCurFileSize >= m_ulPrgDnTotalFileSize)) )
    {
        ReportDownloadComplete();
        // /Put an end to p.d.monitor callbacks now that we're at the end:
        m_pProgDownMon->EndSizeMonitoring();
    }
}

/*
 *  Helper methods that are used by IHXPDStatusMgr methods, below
 */        
void
CSimpleFileObject::ReportCurrentDurChanged()
{
    ReportChange(PRDNFLAG_REPORT_CUR_DUR);
}    

void
CSimpleFileObject::ReportTotalDurChanged()
{
    ReportChange(PRDNFLAG_REPORT_TOTAL_DUR);
}

void
CSimpleFileObject::ReportDownloadComplete()
{
    ReportChange(PRDNFLAG_REPORT_DNLD_COMPLETE);
}

void
CSimpleFileObject::ReportChange(UINT32 ulFlags)
{
    // /Don't report anything if it's not progressive or if we don't yet know
    // if it's progressive:
    HXBOOL bIsProgressive = m_pProgDownMon->HasBeenProgressive()  &&
            m_pProgDownMon->IsProgressive();
    // /If we know the complete file size from URL options and it
    // differs from current file size stat'd so far, it's progressive:
    if (IsPrgDnCompleteFileSizeKnown())
    {
        UINT32 ulCurFileSize = m_pProgDownMon->GetLastFileSize();
        bIsProgressive = (m_ulPrgDnTotalFileSize > ulCurFileSize);
    }

    // /Only report if it is (or has been) a progressive-download file:
    if (bIsProgressive  ||  m_bDownloadProgressReported)
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
                    // /Pass what we observed along to our observers:
                    // (It's OK to pass a NULL IHXStreamSource since our observers
                    // are themselves IHXStreamSources) :
                    if (ulFlags & PRDNFLAG_REPORT_CUR_DUR  ||
                            ulFlags & PRDNFLAG_REPORT_CUR_DUR_UNKNOWN)
                    {
                        m_bDownloadProgressReported = TRUE;
                        pObserver->OnDownloadProgress(NULL,
                                m_ulCurrentDurOfBytesSoFar,
                                m_pProgDownMon->GetLastFileSize(),
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

                    if (ulFlags & PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_PAUSE)
                    {
                        m_bDidNotifyOfDownloadPause = TRUE;
                        pObserver->OnDownloadPause(NULL);
                    }
                    if (ulFlags & PRDNFLAG_REPORT_NOTIFY_DOWNLOAD_RESUME)
                    {
                        m_bDidNotifyOfDownloadPause = FALSE;
                        pObserver->OnDownloadResume(NULL);
                    }
                }
            }
        }
    }
}


HX_RESULT
CSimpleFileObject::EstablishPDSObserverList()
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
CSimpleFileObject::AddObserver(IHXPDStatusObserver* /*IN*/ pObserver)
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
CSimpleFileObject::RemoveObserver(IHXPDStatusObserver* /*IN*/ pObserver)
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
CSimpleFileObject::SetStatusUpdateGranularityMsec(
        UINT32 /*IN*/ ulStatusUpdateGranularityInMsec)
{
    m_ulStatusUpdateGranularityInMsec = ulStatusUpdateGranularityInMsec;
    return HXR_OK;
}


#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

