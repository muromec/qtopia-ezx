/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: source_finder.cpp,v 1.9 2003/11/18 18:16:23 jmevissen Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hxcom.h"
#include "hxtypes.h"
#include "hxauth.h"
#include "hxstring.h"
#include "hxmap.h"
#include "proc.h"
#include "server_context.h"
#include "streamer_container.h"
#include "source_finder.h"
#include "source_container.h"
#include "static_source_container.h"
#include "url.h"
#include "base_errmsg.h"
#include "misc_plugin.h"
#include "srcerrs.h"
#include "server_request.h"
#include "ff_source.h"

CServerSourceFinder::CServerSourceFinder
(
    Process* proc
)
{
    m_pProc      = proc;
    m_lRefCount  = 0;
    m_pFSManager = 0;
    m_pResponse  = 0;
    m_pURL	 = 0;
    m_bFindPending	 = FALSE;
    m_pBroadcastMapper = NULL;
    m_pMimeMapper = NULL;
    m_pFileObject = NULL;
    m_pRequest    = NULL;

}

CServerSourceFinder::~CServerSourceFinder()
{
    Done();
}

/* IHXUnknown methods */

STDMETHODIMP
CServerSourceFinder::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXSourceFinderObject*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXSourceFinderObject))
    {
	AddRef();
	*ppvObj = (IHXSourceFinderObject*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileMimeMapperResponse))
    {
        AddRef();
        *ppvObj = (IHXFileMimeMapperResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
CServerSourceFinder::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
CServerSourceFinder::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/* IHXSourceFinderObject methods */
STDMETHODIMP
CServerSourceFinder::Init
(
    IUnknown* pUnknown
)
{
    HX_RESULT hresult;

    hresult = pUnknown->QueryInterface(IID_IHXSourceFinderResponse, 
                                       (void**)&m_pResponse);

    /* Make no assumptions about the pointer returned in the failure case */

    if (hresult != HXR_OK)
    {
	m_pResponse = 0;
	return hresult;
    }

    m_pFSManager = new FSManager(m_pProc);
    m_pFSManager->AddRef();

    hresult = m_pFSManager->Init((IHXFileSystemManagerResponse*)this);

    return hresult;
}

STDMETHODIMP
CServerSourceFinder::Find
(
    IHXRequest* pRequest
)
{
    if (!m_pResponse)
    {
	return HXR_FAIL;
    }

    if (pRequest)
    {
	HX_RELEASE(m_pRequest);
	m_pRequest = pRequest;
	m_pRequest->AddRef();
    }

    const char* pURL;

    if (HXR_OK != pRequest->GetURL(pURL))
    {
	return m_pResponse->FindDone(HXR_FAIL, 0);
    }

    if (!pURL)
    {
	return HXR_FAIL;
    }

    m_pURL = new URL(pURL, strlen(pURL));

    m_bFindPending = TRUE;
    //XXXJR see who's using this and how to get an authenticator here
    return m_pFSManager->GetFileObject(pRequest, NULL);
}

STDMETHODIMP
CServerSourceFinder::Done()
{
    if (m_bFindPending)
    {
	HX_RELEASE(m_pBroadcastMapper);
	m_bFindPending = FALSE;
    }
    
    if (m_pFSManager)
    {
	m_pFSManager->Release();
	m_pFSManager = 0;
    }

    if (m_pResponse)
    {
	m_pResponse->Release();
	m_pResponse = 0;
    }

    HX_RELEASE(m_pMimeMapper);
    HX_RELEASE(m_pFileObject);
    
    HX_RELEASE(m_pRequest);
    HX_DELETE(m_pURL);
    return HXR_OK;
}


/* IHXFileSystemManagerResponse */

STDMETHODIMP
CServerSourceFinder::InitDone
(
    HX_RESULT status
)
{
    if (status != HXR_OK)
    {
	m_pFSManager->Release();
	m_pFSManager = 0;
    }

    m_pResponse->InitDone(status);

    return HXR_OK;
}

STDMETHODIMP
CServerSourceFinder::FileObjectReady
(
    HX_RESULT status,
    IUnknown* pFileObject
)
{
    if (!m_bFindPending)
    {
	return HXR_OK;
    }

    if (status == HXR_OK)
    {
        HX_ASSERT(!m_pBroadcastMapper);
	if (HXR_OK == pFileObject->QueryInterface(IID_IHXBroadcastMapper,
						  (void**)&m_pBroadcastMapper))
	{
	    m_pBroadcastMapper->FindBroadcastType(m_pURL->full, this);
	}
	else
	{
	    HX_RELEASE(m_pMimeMapper);
	    
	    if (HXR_OK == pFileObject->QueryInterface(IID_IHXFileMimeMapper, 
						      (void**)&m_pMimeMapper))
	    {
		HX_RELEASE(m_pFileObject);
		m_pFileObject = pFileObject;
		m_pFileObject->AddRef();
		
		m_pMimeMapper->FindMimeType(m_pURL->full, this);
	    }
	    else
	    {
		FindStatic(pFileObject, NULL, m_pURL->ext);
	    }
	}
    }
    else
    {
	m_pResponse->FindDone(SE_UNKNOWN_PATH, 0);
    }

    return HXR_OK;
}

STDMETHODIMP
CServerSourceFinder::DirObjectReady
(
    HX_RESULT      status,
    IUnknown* pDirObject
)
{
    return HXR_NOTIMPL;
}

HX_RESULT
CServerSourceFinder::FindStatic(IUnknown* pFileObject,
				const char* pMimeType,
				const char* extension)
{
    if (!m_bFindPending)
    {
	return HXR_OK;
    }
    m_bFindPending = FALSE;

    HX_RELEASE(m_pMimeMapper);
    
    PluginHandler::FileFormat*  pFileFormatHandler = NULL;
    IHXFileFormatObject*       pFileFormatObject  = NULL;
    FileFormatSource*           pFileFormatSource  = NULL;
    IUnknown*                   pInstance          = NULL;
    IHXPlugin*                 pPlugin            = NULL;
    HX_RESULT                   h_result           = HXR_OK;
    PluginHandler::Errors	plugin_result;

    PluginHandler::FileFormat::PluginInfo* pPluginInfo = NULL;
 
    pFileFormatHandler = m_pProc->pc->plugin_handler->m_file_format_handler;
    if (pFileFormatHandler)
    {
	pPluginInfo = pFileFormatHandler->FindPluginInfo(pMimeType,m_pURL->ext);
    }

    if (pPluginInfo)
    {
	plugin_result = PluginHandler::NO_ERRORS;
    }
    else
    {
	plugin_result = PluginHandler::PLUGIN_NOT_FOUND;
    }

    HX_RESULT rc = HXR_OK;
    if (PluginHandler::NO_ERRORS != plugin_result)
    {
        ERRMSG(m_pProc->pc->error_handler, "No handler for %s\n", m_pURL->ext);
        rc = HXR_INVALID_FILE;
    }
    else if (PluginHandler::NO_ERRORS != (plugin_result =
	    pPluginInfo->m_pPlugin->GetInstance(&pInstance)))
    {	
        ERRMSG(m_pProc->pc->error_handler, "No handler for %s\n", m_pURL->ext);
        rc = HXR_INVALID_FILE;
    }
    else if ( (HXR_OK == pInstance->QueryInterface(IID_IHXPlugin, 
						   (void**)&pPlugin))
	      && (HXR_OK == pInstance->QueryInterface(IID_IHXFileFormatObject,
						   (void**)&pFileFormatObject))
	      && (HXR_OK == pPlugin->InitPlugin(m_pProc->pc->server_context)) )
    {
	HX_RELEASE(pInstance);

	pFileFormatSource = new FileFormatSource(m_pProc, pFileFormatObject, 
						  m_pFSManager->m_mount_point_len, 
						  pFileObject, m_pRequest, FALSE);
	rc = HXR_OK;
    }

    if (rc != HXR_OK)
    {
	m_pResponse->FindDone(rc, NULL);
    }
    else
    {
        IHXSourceFinderFileResponse* pFileResponse = NULL;
        if (HXR_OK == m_pResponse->QueryInterface(IID_IHXSourceFinderFileResponse,
                                                  (void**)&pFileResponse))
        {
            pFileResponse->FindDone(HXR_OK, (IUnknown*)(IHXRawSourceObject*)
                        (new StaticSourceContainer(m_pProc->pc->server_context,
                        pFileFormatSource)), pFileObject);
        }
        else
        {
	        m_pResponse->FindDone(HXR_OK, (IUnknown*)(IHXRawSourceObject*)
                      (new StaticSourceContainer(m_pProc->pc->server_context,
							 pFileFormatSource)));
        }
        HX_RELEASE(pFileResponse);

    }

    HX_RELEASE(pPlugin);
    HX_RELEASE(pInstance);
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(pFileFormatObject);
    HX_RELEASE(m_pRequest);

    return HXR_OK;
}

STDMETHODIMP
CServerSourceFinder::MimeTypeFound(HX_RESULT status,
				   const char* pMimeType)
{
  if (pMimeType)
  {
      FindStatic(NULL, pMimeType, NULL);
  }
  else
  {
      FindStatic(NULL, NULL, m_pURL->ext);
  }
  return HXR_OK;
}

STDMETHODIMP
CServerSourceFinder::BroadcastTypeFound
(
    HX_RESULT   status,
    const char* pType
)
{
    //XXXJHUG we will ignor any callbacks after we have been closed.
    if (!m_bFindPending)
    {
	return HXR_OK;
    }
    m_bFindPending = FALSE;

    PluginHandler::BroadcastFormat* pBroadcastHandler;
    PluginHandler::Errors           plugin_result;
    PluginHandler::Plugin*          pPlugin;
    IHXPSourceControl*		    pSource;
    IHXBuffer*                     pBroadcastAlias = NULL;
    IHXValues*                     pRequestHeaders = NULL;

    /*
     * Since FindBroadcastType is an asynchronous call, the BroadcastMapper
     * can only be released now (when the callback executes)
     */

    HX_RELEASE(m_pBroadcastMapper);

    pBroadcastHandler = m_pProc->pc->plugin_handler->m_broadcast_handler;

    plugin_result = pBroadcastHandler->Find(pType, pPlugin);

    if(PluginHandler::NO_ERRORS != plugin_result)
    {
	ERRMSG(m_pProc->pc->error_handler, "No live handler for %s\n", pType);
	return HXR_FAIL;
    }

    BroadcastPlugin* pBCPlugin;

    BOOL bRet = m_pProc->pc->misc_plugins->Lookup(pPlugin, (void*&)pBCPlugin);

    ASSERT(bRet == TRUE); 

    /*
     *  The BroadcastAlias can be set by the broadcast format object
     *  if this session exists under a name other than that requested
     *  e.g. wildcard requests to broadcast distribution
     */

    if (m_pRequest)
    {
	if (HXR_OK == m_pRequest->GetRequestHeaders(pRequestHeaders))
	{
 	    if (pRequestHeaders)
	    {
		pRequestHeaders->GetPropertyCString("BroadcastAlias", pBroadcastAlias);
	    }
	}
    }

    /*
     * Make sure that LiveSourceList is initialized
     */

    if (pBroadcastAlias != NULL)
    {
	m_pProc->pc->broadcast_manager->GetStream(pType,
						  (const char*)pBroadcastAlias->GetBuffer(),
						  pSource, m_pProc, FALSE);
    }
    else
    {
	m_pProc->pc->broadcast_manager->GetStream(pType,
						  m_pURL->name +
						  m_pFSManager->m_mount_point_len - 1,
						  pSource, m_pProc, FALSE);
    }

    IUnknown* pUnk = (IUnknown*)(IHXPSinkPackets*)
        new SourceContainer(m_pProc->pc->server_context, pSource);
    pUnk->AddRef();
    pSource->Release();

    m_pResponse->FindDone(HXR_OK, pUnk);
    pUnk->Release();

    HX_RELEASE(pBroadcastAlias);
    HX_RELEASE(pRequestHeaders);

    return HXR_OK;
}
