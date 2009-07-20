/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxfsmgr.cpp,v 1.16 2006/10/06 21:03:39 ping Exp $
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

#include "hxtypes.h"
#include "hxresult.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxchkpt.h"
#include "hxfiles.h"
#include "hxauth.h"

#include "hxlist.h"
#include "hxstrutl.h"
#include "dbcs.h"
#include "hxplugn.h"
#include "hxrquest.h"
#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#include "basehand.h"
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
#include "hxpluginmanager.h"
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else
#include "plghand2.h"
#endif /* _STATICALLY_LINKED */

#include "hxplugn.h"
#include "hxfsmgr.h"
#include "hxmon.h"
#include "chxpckts.h"
#include "pckunpck.h"

#ifdef _MACINTOSH
#include "hx_moreprocesses.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if !defined(HELIX_CONFIG_NOSTATICS)
HXBOOL HXFileSystemManager::zm_IsInited = FALSE;
CHXMapStringToOb    HXFileSystemManager::zm_ShortNameMap;
CHXMapStringToOb    HXFileSystemManager::zm_ProtocolMap;
CHXSimpleList	    HXFileSystemManager::zm_CacheList;
#else
#include "globals/hxglobals.h"
const HXBOOL HXFileSystemManager::zm_IsInited = FALSE;
const CHXMapStringToOb* const HXFileSystemManager::zm_ShortNameMap = NULL;
const CHXMapStringToOb* const HXFileSystemManager::zm_ProtocolMap = NULL;
const CHXSimpleList* const HXFileSystemManager::zm_CacheList = NULL;
#endif


HXFileSystemManager::HXFileSystemManager(IUnknown* pContext)
    : m_lRefCount(0)
    , m_pFSManagerResponse(0)
    , m_pSamePool(0)
    , m_pContext(pContext)
    , m_pRequest(NULL)
    , m_pCallback(NULL)
    , m_pScheduler(NULL)
    , m_pOriginalObject(NULL)
    , m_pRelativePath(NULL)
{
    if (m_pContext)
    {
	m_pContext->AddRef();
	InitMountPoints(m_pContext);
    }
}


void HXFileSystemManager::InitMountPoints(IUnknown* pContext)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    HXBOOL& zm_IsInited =
	(HXBOOL&)HXGlobalInt32::Get(&HXFileSystemManager::zm_IsInited);
#endif

    if (zm_IsInited)
	return;
    zm_IsInited = TRUE;

    IHXBuffer*			mount_point = 0;
    IHXBuffer*			real_short_name = 0;
    const char*			short_name = 0;
    IHXValues*			options = 0;

    IHXRegistry* pRegistry;
    IHXValues* pNameList = NULL;

    if(HXR_OK != pContext->QueryInterface(IID_IHXRegistry,
					    (void**)&pRegistry))
    {
	return;
    }

    if(HXR_OK != pRegistry->GetPropListByName("config.FSMount", pNameList))
    {
	pRegistry->Release();
	return;
    }

    HX_RESULT res;
    const char* plugName;
    UINT32 plug_id;

    res = pNameList->GetFirstPropertyULONG32(plugName, plug_id);
    while(res == HXR_OK)
    {
	HXPropType plugtype = pRegistry->GetTypeById(plug_id);
	if(plugtype != PT_COMPOSITE)
	    res = HXR_FAIL;
	else
	{
	    short_name = strrchr(plugName, '.');
	    if(!short_name)
		short_name = plugName;
	    else
		short_name++;

	    IHXValues* pPropList;
	    if(HXR_OK == pRegistry->GetPropListById(plug_id, pPropList))
	    {
		const char* propName;
		UINT32 prop_id;

		CreateValuesCCF(options, pContext);

		res = pPropList->GetFirstPropertyULONG32(propName, prop_id);
		while(res == HXR_OK)
		{
		    HXPropType type = pRegistry->GetTypeById(prop_id);
		    const char*propSubName = strrchr(propName, '.') + 1;
		    switch(type)
		    {
			case PT_INTEGER:
			{
			    INT32 val;
			    if(HXR_OK == pRegistry->GetIntById(prop_id, val))
			    {
				options->SetPropertyULONG32(propSubName, val);
			    }
			    break;
			}
			case PT_STRING:
			{
			    IHXBuffer* pBuffer;
			    if(HXR_OK == pRegistry->GetStrById(prop_id,
							       pBuffer))
			    {
				options->SetPropertyBuffer(propSubName,
							    pBuffer);
				pBuffer->Release();
			    }
			    break;
			}
			case PT_BUFFER:
			{
			    IHXBuffer* pBuffer;
			    if(HXR_OK == pRegistry->GetBufById(prop_id,
							       pBuffer))
			    {
				options->SetPropertyBuffer(propSubName,
							   pBuffer);
				pBuffer->Release();
			    }
			    break;
			}
			default:
			    break;
		    }
		    res = pPropList->GetNextPropertyULONG32(propName, prop_id);
		}
		res = HXR_OK;
	    }
	    
	    if(HXR_OK == options->GetPropertyBuffer("MountPoint",
						     mount_point))
	    {
		if(HXR_OK == options->GetPropertyBuffer("ShortName",
							real_short_name))
		    short_name = (const char*) real_short_name->GetBuffer();
							

		AddMountPoint(short_name,(const char*)mount_point->GetBuffer(),
						  options, pContext);
		if(real_short_name)
		{
		    real_short_name->Release();
		    real_short_name = 0;
		}
		mount_point->Release();
	    }
	    res = pNameList->GetNextPropertyULONG32(plugName, plug_id);
	}
    }
    pNameList->Release();
    pRegistry->Release();
}

HX_RESULT 
HXFileSystemManager::AddMountPoint(const char*	pszShortName,
					 const char* pszMountPoint,
					 IHXValues* pOptions,
					 IUnknown* pContext)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    CHXSimpleList& zm_CacheList =
	HXGlobalList::Get(&HXFileSystemManager::zm_CacheList);
    CHXMapStringToOb& zm_ShortNameMap =
	HXGlobalMapStringToOb::Get(&HXFileSystemManager::zm_ShortNameMap);
    CHXMapStringToOb& zm_ProtocolMap =
	HXGlobalMapStringToOb::Get(&HXFileSystemManager::zm_ProtocolMap);
#endif

    HX_RESULT		    result	    = HXR_OK;
    IHXPlugin2Handler*	    pPlugin2Handler = NULL;
    CCacheInstance*	    pCCacheInstance = NULL;

    if (HXR_OK != pContext->QueryInterface(IID_IHXPlugin2Handler, (void**) &pPlugin2Handler))
	return HXR_FAIL;

    if (!pszShortName)
    {
	result = HXR_FAIL;
	goto cleanup;
    }
    
    UINT32 nIndex; 
    if (HXR_OK != pPlugin2Handler->FindIndexUsingStrings(PLUGIN_FILESYSTEMSHORT, 
	    (char*)pszShortName, NULL, NULL, NULL, NULL, nIndex))
    {
	result = HXR_FAIL;
	goto cleanup;
    }

    IHXValues* pValues;
    IHXBuffer* pProtocol;

    pPlugin2Handler->GetPluginInfo(nIndex, pValues);

    pValues->GetPropertyCString(PLUGIN_FILESYSTEMPROTOCOL, pProtocol);

    char*   pszProtocol;

    pszProtocol = (char*)pProtocol->GetBuffer();

    pCCacheInstance		    = new CCacheInstance;
    pCCacheInstance->m_mount_point  = pszMountPoint;
    pCCacheInstance->m_szProtocol   = pszProtocol;
    pCCacheInstance->m_szShortName  = pszShortName;
    pCCacheInstance->m_pOptions	    = pOptions;

    zm_ShortNameMap.SetAt(pszMountPoint, pCCacheInstance);
    zm_ProtocolMap.SetAt(pszMountPoint, pCCacheInstance);
    zm_CacheList.AddTail((void*)pCCacheInstance);

cleanup:

    return result;
}


IHXValues* HXFileSystemManager::GetOptionsGivenURL(const char* pURL)
{
    const char* pszFilePath = HXFindChar(pURL,':');
    if (pszFilePath == 0)
    {
	pszFilePath = pURL;
    }
    else
    {
	pszFilePath++;
    }

    /*
     * Must check all plugins, we cannot take just the first one who's mount point is the start
     * of the url.  We must find the one who matches with the mount point of the greatest length.
     * If p1 = /test and p2 = /test/test2 and the url is /test/test2/file we must use p2.
     */

#if defined(HELIX_CONFIG_NOSTATICS)
    CHXSimpleList& zm_CacheList =
	HXGlobalList::Get(&HXFileSystemManager::zm_CacheList);
#endif
    
    ULONG32	    mount_point_len = 0;
    CCacheInstance* pBestOptions = NULL;
    for(CHXSimpleList::Iterator i = zm_CacheList.Begin(); i != zm_CacheList.End(); ++i)
    {
	CCacheInstance* pCacheOptions = (CCacheInstance*) *i;

	if (pCacheOptions->m_mount_point.GetLength() > 0 &&
	    (strncmp(pCacheOptions->m_mount_point, pszFilePath, pCacheOptions->m_mount_point.GetLength()) == 0))
	{
	    if((UINT32)pCacheOptions->m_mount_point.GetLength() >= mount_point_len)
	    {
		mount_point_len = pCacheOptions->m_mount_point.GetLength();
		pBestOptions	= pCacheOptions;
	    }
	}
    }

    if (pBestOptions)
    {
	return pBestOptions->m_pOptions;
    }
	
    return NULL;
}



HXFileSystemManager::~HXFileSystemManager()
{
    if (m_pContext)
    {
	m_pContext->Release();
	m_pContext = 0;
    }

    if(m_pSamePool)
    {
	m_pSamePool->Release();
	m_pSamePool = NULL;
    }

    if (m_pFSManagerResponse)
    {
	m_pFSManagerResponse->Release();
	m_pFSManagerResponse = 0;
    }

    HX_RELEASE(m_pRequest);

    if (m_pCallback &&
	m_pCallback->m_bIsCallbackPending &&
	m_pScheduler)
    {
	m_pScheduler->Remove(m_pCallback->m_Handle);
    }

    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pOriginalObject);
    HX_VECTOR_DELETE(m_pRelativePath);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HXFileSystemManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXFileSystemManager), (IHXFileSystemManager*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFileSystemManager*)this },
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
STDMETHODIMP_(ULONG32) HXFileSystemManager::AddRef()
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
STDMETHODIMP_(ULONG32) HXFileSystemManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *	IHXFileSystemManager methods
 */

/************************************************************************
 *	Method:
 *	    IHXFileSystemManager::Init
 *	Purpose:
 */
STDMETHODIMP 
HXFileSystemManager::Init 
	    (
	    IHXFileSystemManagerResponse* /*IN*/  pFileManagerResponse
	    )
{
    if (!pFileManagerResponse)
    {
	return HXR_FAIL;
    }

    /* Release any earlier response */
    if (m_pFSManagerResponse)
    {
	m_pFSManagerResponse->Release();
	m_pFSManagerResponse = 0;
    }

    m_pFSManagerResponse = pFileManagerResponse;
    m_pFSManagerResponse->AddRef();


    /* 
     * We might get released (and deleted) in the response object. so 
     * Addref here and Release after the response function is called
     */
    AddRef();
    
    m_pFSManagerResponse->InitDone(HXR_OK);

    /* Release for extra Addref */
    Release();

    return HXR_OK;
}

STDMETHODIMP
HXFileSystemManager::GetFileObject(IHXRequest* pRequest,
				    IHXAuthenticator* pAuthenticator)
{
    HX_LOG_BLOCK( "HXFileSystemManager::GetFileObject" );
    
    HX_RELEASE(m_pRequest);

    m_pRequest = pRequest;
    if (m_pRequest)
    {
	m_pRequest->AddRef();
    }

    m_State = e_GetFileObjectPending;

#ifdef _MACINTOSH
    if (!IsMacInCooperativeThread())
    {
	if (!m_pScheduler)
	{
	    HX_VERIFY(m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler) == HXR_OK);
	}
	
	if (!m_pCallback)
	{
	    m_pCallback = new RMAFSManagerCallback(this);
	    m_pCallback->AddRef();
	}

	HX_ASSERT(m_pCallback->m_bIsCallbackPending == FALSE);

	if (!m_pCallback->m_bIsCallbackPending)
	{
	    m_pCallback->m_bIsCallbackPending = TRUE;
	    m_pCallback->m_Handle = m_pScheduler->RelativeEnter(m_pCallback, 0);
	}
			
        return HXR_OK;
    }
#endif	// _MACINTOSH

    return (ProcessGetFileObjectPending());
}

HX_RESULT
HXFileSystemManager::ProcessGetFileObjectPending()
{
    HX_LOG_BLOCK( "HXFileSystemManager::ProcessGetFileObjectPending" );

    HX_RESULT theErr = HXR_OK;

    IUnknown*		    pUnknownFS		= NULL;
    IUnknown*		    pUnknownFileObject	= NULL;
    IHXFileSystemObject*   pFileSystem		= NULL;
    IHXRequestHandler*	    pRequestHandler	= NULL;
    IHXPlugin2Handler*	    pPlugin2Handler	= NULL; 

    if (!m_pContext)
    {
	return HXR_FAILED;
    }

    /* 
     * We might get released (and deleted) in the response object. so 
     * Addref here and Release after the response function is called
     */
    AddRef();
    
    // get the plugin handler 
    if (HXR_OK != m_pContext->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPlugin2Handler))
    {
        theErr = HXR_FAILED;
	goto exit;
    }

    const char* pURL;

    HX_ASSERT( NULL != m_pRequest );
    if ( ( NULL == m_pRequest ) || ( m_pRequest->GetURL(pURL) != HXR_OK ) )
    {
        theErr = HXR_FAILED;
	goto exit;
    }

    const char* pProtocolEnd;
    pProtocolEnd = HXFindChar(pURL,':');

    if (!pProtocolEnd)
    {
        theErr = HXR_FAILED;
    }

    if (!theErr)
    {
	int nLength = pProtocolEnd - pURL;
	CHXString strProtocol(pURL,nLength);

	if (HXR_OK != (theErr = pPlugin2Handler->FindPluginUsingStrings(PLUGIN_CLASS, PLUGIN_FILESYSTEM_TYPE, 
	    PLUGIN_FILESYSTEMPROTOCOL, (char*)(const char*)strProtocol, NULL, NULL, pUnknownFS)))
	{
	    goto exit;
	}

	IHXPlugin* pPluginInterface = NULL;

	if(!theErr)
	{
	    theErr = pUnknownFS->QueryInterface(IID_IHXPlugin,
						(void**)&pPluginInterface);
	}

	if(!theErr)
	{
	    theErr = pPluginInterface->InitPlugin(m_pContext);
	    pPluginInterface->Release();
	}
	
	if(!theErr)
	{
	    theErr = pUnknownFS->QueryInterface(IID_IHXFileSystemObject,
						(void**)&pFileSystem);
	}
	
	// At this point we should initalize the file system.to do this we must find the 
	// IHXValues for this mount path in the Options Cache.

	IHXValues* pOptions = NULL;

	pOptions = GetOptionsGivenURL(pURL);

	pFileSystem->InitFileSystem(pOptions);

	HX_RELEASE(pOptions);

	if(!theErr)
	{
	    theErr = pFileSystem->CreateFile(&pUnknownFileObject);
	}
	
	if(!theErr)
	{
	    if(HXR_OK == pUnknownFileObject->QueryInterface(
		IID_IHXRequestHandler,
		(void**)&pRequestHandler))
	    {
		pRequestHandler->SetRequest(m_pRequest);
	    }
	    else
	    {
		theErr = HXR_FAILED;
	    }
	}
    }
    else
    {
	theErr = HXR_FAILED;
    }

    if (!theErr && pUnknownFileObject)
    {
	m_pFSManagerResponse->FileObjectReady(HXR_OK, pUnknownFileObject);
    }
    else
    {
	m_pFSManagerResponse->FileObjectReady(HXR_FAILED, NULL);
    }

exit:
    HX_RELEASE(pUnknownFS);
    HX_RELEASE(pUnknownFileObject);
    HX_RELEASE(pRequestHandler);
    HX_RELEASE(pFileSystem);
    HX_RELEASE(pPlugin2Handler);

#ifndef _MACINTOSH
    // Note: This change is necessary for the Macintosh build due to the fact
    // that this platform uses a different approach in GetFileObject.  The problem
    // is that file object processing had generally been done recursively, with
    // GetFileObject calling ProcessGetFileObjectPending, which in turn indirectly
    // invoked GetFileObject in a pattern of mutual recursion.  The recursion had
    // always ended with a call to ProcessGetFileObjectPending.  With the change
    // in GetFileObject:
    //     #ifdef _MACINTOSH
    //      if (!IsMacInCooperativeThread())
    // the recursion would terminate in a GetFileObject call.  This call would
    // unwind to the scheduler, which would then process the queued file object
    // by calling ProcessGetFileObjectPending.  However, since the request object
    // was freed during the unwinding of the recursion, this object was no longer
    // available and hence the process failed.
    //
    // The best short term fix appears to be to remove this release.  The best long
    // term fix is to eliminate the recursion (which would also simplify maintenance). 
    //     -cconover 	XXX
    
    HX_RELEASE(m_pRequest);
#endif

    /* 
     * Release for extra Addref
     */
    Release();

    return theErr;
}

/*
 * In this implementation of FSManager, GetNewFileObject is wholly
 * equivalent to GetFileObject.  GetNewFileObject exists for the
 * server's FSManager since it would otherwise be impossible to get
 * a brand new file object for writing, as the DoesExist checks
 * would fail for every file system checked.  This implementation has
 * ambiguities between URL's and file systems, so there is no
 * different functionality needed.
 */
STDMETHODIMP
HXFileSystemManager::GetNewFileObject(IHXRequest* pRequest,
				       IHXAuthenticator* pAuthenticator)
{
    return GetFileObject(pRequest, pAuthenticator);
}

STDMETHODIMP
HXFileSystemManager::GetRelativeFileObject(IUnknown* pOriginalObject,
				 const char* pRelativePath)
{
    if(!pRelativePath)
    {
	return HXR_FAIL;
    }

    HX_RELEASE(m_pOriginalObject);

    m_pOriginalObject = pOriginalObject;
    if (m_pOriginalObject)
    {
	m_pOriginalObject->AddRef();
    }

    HX_VECTOR_DELETE(m_pRelativePath);
    m_pRelativePath = new_string(pRelativePath);

    m_State = e_GetRelativeFileObjectPending;

#ifdef _MACINTOSH
    if (!IsMacInCooperativeThread())
    {
	if (!m_pScheduler)
	{
	    HX_VERIFY(m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler) == HXR_OK);
	}
	
	if (!m_pCallback)
	{
	    m_pCallback = new RMAFSManagerCallback(this);
	    m_pCallback->AddRef();
	}

	HX_ASSERT(m_pCallback->m_bIsCallbackPending == FALSE);

	if (!m_pCallback->m_bIsCallbackPending)
	{
	    m_pCallback->m_bIsCallbackPending = TRUE;
	    m_pCallback->m_Handle = m_pScheduler->RelativeEnter(m_pCallback, 0);
	}
			
        return HXR_OK;
    }
#endif	// _MACINTOSH

    return (ProcessGetRelativeFileObjectPending());

}

HX_RESULT
HXFileSystemManager::ProcessGetRelativeFileObjectPending()
{
    HX_RESULT theErr = HXR_OK;
    IHXRequestHandler* pRequestHandlerOrigional = NULL;
    IHXRequest* pRequestOld = NULL;

    AddRef();
    
    if (!m_pOriginalObject)
    	goto exit;

    if(HXR_OK != m_pOriginalObject->QueryInterface(IID_IHXGetFileFromSamePool,
					       (void**)&m_pSamePool))
    {
	theErr = HXR_FAIL;
	goto exit;
    }

    if 
    (
	HXR_OK != m_pOriginalObject->QueryInterface
	(
	    IID_IHXRequestHandler,
	    (void**)&pRequestHandlerOrigional
	)
	||
	!pRequestHandlerOrigional
	||
	HXR_OK != pRequestHandlerOrigional->GetRequest(pRequestOld)
	||
	!pRequestOld
    )
    {
	HX_RELEASE(pRequestHandlerOrigional);
	HX_RELEASE(pRequestOld);

	theErr = HXR_FAIL;
	goto exit;
    }

    HX_RELEASE(pRequestHandlerOrigional);

    /*
     * Create an IHXRequest object for the new file
     */

    HX_RELEASE(m_pRequest);

    CHXRequest::CreateFromCCF(pRequestOld, &m_pRequest, m_pContext);
    HX_RELEASE(pRequestOld);

    m_pRequest->SetURL(m_pRelativePath);

    if(HXR_OK != m_pSamePool->GetFileObjectFromPool(this))
    {
	theErr = HXR_FAIL;
	goto exit;
    }

exit:
    HX_RELEASE(m_pSamePool);
    HX_RELEASE(m_pOriginalObject);
    HX_VECTOR_DELETE(m_pRelativePath);

    Release();

    return theErr;
}

STDMETHODIMP
HXFileSystemManager::FileObjectReady(HX_RESULT status, IUnknown* pUnknown)
{
    IHXRequestHandler* pRequestHandler = NULL;

    if(HXR_OK == status)
    {
	if(HXR_OK == pUnknown->QueryInterface(IID_IHXRequestHandler,
					    (void**)&pRequestHandler))
	{
	    pRequestHandler->SetRequest(m_pRequest);
	}
	else
	{
	    pUnknown = 0;
	    status = HXR_FAILED;
	}

	pRequestHandler->Release();
    }

    HX_RELEASE(m_pRequest);

    /* 
     * We might get released (and deleted) in the response object. so 
     * Addref here and Release after the response function is called
     */
    AddRef();
    
    if (m_pFSManagerResponse)
    {
	m_pFSManagerResponse->FileObjectReady(status, pUnknown);
    }

    /* 
     * Release for extra Addref
     */
    Release();

    return HXR_OK;
}



STDMETHODIMP 
HXFileSystemManager::GetDirObjectFromURL(const char* pURL)
{
    return HXR_NOTIMPL;
}

void
HXFileSystemManager::ProcessPendingRequest()
{

    switch(m_State)
    {
	case e_GetFileObjectPending:
	    ProcessGetFileObjectPending();
	    break;
	case e_GetRelativeFileObjectPending:
	    ProcessGetRelativeFileObjectPending();
	    break;
	default:
	    HX_ASSERT(FALSE);
	    break;
    }    
}

// HXFileSystemManager::RMAFSManagerCallback

HXFileSystemManager::RMAFSManagerCallback::RMAFSManagerCallback(HXFileSystemManager*	pFSManager) :
     m_pFSManager (pFSManager)
    ,m_Handle (0)
    ,m_bIsCallbackPending(FALSE)
    ,m_lRefCount (0)
{
}

HXFileSystemManager::RMAFSManagerCallback::~RMAFSManagerCallback()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXFileSystemManager::RMAFSManagerCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXFileSystemManager::RMAFSManagerCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXFileSystemManager::RMAFSManagerCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *	UDPSchedulerCallback methods
 */
STDMETHODIMP HXFileSystemManager::RMAFSManagerCallback::Func(void)
{
    m_Handle = 0;
    m_bIsCallbackPending = FALSE;
    if (m_pFSManager)
    {
	m_pFSManager->ProcessPendingRequest();
    }

    return HXR_OK;
}
