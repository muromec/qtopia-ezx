/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: srcfinder.cpp,v 1.27 2007/04/17 12:51:12 npatil Exp $
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
#include "url.h"
#include "timerep.h"
#include "errmsg_macros.h"
#include "server_context.h"
#include "source_container.h"
#include "server_request.h"
#include "ff_source.h"
#include "static_source_container.h"

#include "config.h"
#include "hxstats.h"
#include "hxprot.h"
#include "hxxfile.h"
#include "bcastmgr.h"

#include "ispifs.h"
#include "isifs.h"

#include "hxqos.h"
#include "qos_cfg_names.h"

#include "fileformat_handler.h"
#include "asmstreamfilter.h"
#include "srcfinder.h"
#include "fsmanager.h"

BasicSourceFinder::BasicSourceFinder(Process* pProc, Player::Session* pPlayerSession)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pConfig(NULL)
    , m_pPlayerSession(pPlayerSession) // if only used by Player::Session
    , m_pResp(NULL)
    , m_pRequest(NULL)
    , m_pURL(NULL)
    , m_pClientProfile(NULL)
    , m_pFSManager(NULL)
    , m_pMimeMapper(NULL)
    , m_pBroadcastMapper(NULL)
    , m_pFileObject(NULL)
    , m_pFileRequest(NULL)
    , m_pFileFormatObject(NULL)
    , m_pFileFormatRequest(NULL)
    , m_pSourceControl(NULL)
    , m_pCurrentPlugin(NULL)
    , m_findState(SF_CLOSED)
{
    HX_ASSERT(m_pProc);
}

BasicSourceFinder::~BasicSourceFinder(void)
{
    Close();
}

HX_RESULT
BasicSourceFinder::Init(IHXSourceFinderFileResponse* pResp,
                        IHXQoSProfileConfigurator* pQoSConfig)
{
    HX_RESULT theErr;
    if (SF_CLOSED != m_findState)
    {
        theErr = HXR_UNEXPECTED;
    }
    else if (pResp)
    {
        m_pResp = pResp;
        m_pResp->AddRef();

        m_findState = SF_INITIALIZED;
        theErr = HXR_OK;
    }
    else
    {
        m_findState = SF_CLOSED;
        theErr = HXR_INVALID_PARAMETER;
    }

    if (HXR_OK == theErr)
    {
        if (pQoSConfig)
        {
            m_pConfig = pQoSConfig;
            m_pConfig->AddRef();
        }
    }

    return theErr;
}

void
BasicSourceFinder::Close(void)
{
    m_findState = SF_CLOSED;

    HX_RELEASE(m_pFSManager);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pMimeMapper);
    HX_RELEASE(m_pBroadcastMapper);
    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pClientProfile);
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pFileRequest);
    HX_RELEASE(m_pFileFormatObject);
    HX_RELEASE(m_pFileFormatRequest);
    HX_RELEASE(m_pSourceControl);
    HX_RELEASE(m_pConfig);

    m_pCurrentPlugin = NULL;

    // this needs to be a COM obj.
    m_pURL = NULL;

}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::FindSource
// Purpose:
//  Given an URL, instantiates an appropriate source object and
//  calls back IHXSourceFinderFileResponse::FindDone()
HX_RESULT
BasicSourceFinder::FindSource(URL* pURL, ServerRequest* pRequest)
{
    HX_ASSERT(pURL);
    HX_ASSERT(pRequest);
    HX_ASSERT(m_pResp);
    HX_ASSERT(!m_pRequest);
    HX_ASSERT(!m_pFSManager);
    HX_ASSERT(!m_pFileObject);
    HX_ASSERT(!m_pMimeMapper);
    HX_ASSERT(!m_pBroadcastMapper);

    if (SF_INITIALIZED != m_findState)
    {
        HX_ASSERT(!(SF_CLOSED == m_findState) &&
            "BasicSourceFinder::FindSource() - Please Init()");
        HX_ASSERT(!(SF_FINDING == m_findState) &&
            "BasicSourceFinder::FindSource() - Already in Progress...");
        return HXR_UNEXPECTED;
    }
    else if (!pRequest || !pURL)
    {
        return HXR_INVALID_PARAMETER;
    }

    m_findState = SF_FINDING;

    m_pRequest = pRequest;
    m_pRequest->AddRef();

    m_pURL = pURL;

    if (m_pProc->pc->config->GetInt(m_pProc,
        "config.InputSource.UseASMFilter"))
    {
        GetClientProfileInfo();
    }

    m_pFSManager = new FSManager(m_pProc);
    m_pFSManager->AddRef();
    return m_pFSManager->Init(this);
    // Flow continues at BasicSourceFinder::InitDone()
}

HX_RESULT
BasicSourceFinder::FindNextSource()
{
    HX_RESULT theErr = HXR_FAIL;

    if (SF_FINDING == m_findState &&
        m_pCurrentPlugin &&
        m_pCurrentPlugin->m_pNextPlugin &&
        m_pFileRequest)
    {
        theErr = m_pFSManager->GetFileObject(m_pFileRequest, NULL);
        // Continue in FileObjectReady
    }

    if (HXR_OK != theErr)
    {
        FindSourceDone(theErr, NULL);
    }
    return theErr;
}


HX_RESULT
BasicSourceFinder::FindSourceDone(HX_RESULT status, IUnknown* pSource)
{
    HX_RELEASE(m_pMimeMapper);
    HX_RELEASE(m_pBroadcastMapper);

    if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }

    m_pResp->FindDone(status, pSource, m_pFileObject);
    return HXR_OK;
}

/*
* IHXFileSystemManagerResponse
*/
STDMETHODIMP
BasicSourceFinder::InitDone(HX_RESULT status)
{
    HX_ASSERT(SF_INITIALIZED != m_findState);

    if (SF_FINDING == m_findState && HXR_OK == status)
    {
        HX_ASSERT(m_pFileRequest == NULL);
        m_pFileRequest = new ServerRequestWrapper(FS_HEADERS, m_pRequest);
        m_pFileRequest->AddRef();
        return m_pFSManager->GetFileObject(m_pFileRequest, NULL);
        // Continue in FileObjectReady
    }

    return HXR_OK;
}


STDMETHODIMP
BasicSourceFinder::FileObjectReady(HX_RESULT status, IUnknown* pFileObject)
{
    HX_RESULT hResult = HXR_OK;
    HX_ASSERT(!m_pBroadcastMapper);
    HX_ASSERT(!m_pMimeMapper);

    if (SF_FINDING == m_findState && HXR_OK == status)
    {
        HX_RELEASE(m_pFileObject);

        m_pFileObject = pFileObject;
        m_pFileObject->AddRef();

        // If we've already got the plugin list and are just
        // looking for the next one
        if (m_pCurrentPlugin)
        {
            hResult = FindStatic(NULL, NULL);
            if (FAILED(hResult))
            {
                FindSourceDone(hResult, NULL);
            }
            return HXR_OK;
        }

        hResult = pFileObject->QueryInterface(IID_IHXBroadcastMapper,
            (void**)&m_pBroadcastMapper);
        if(HXR_OK == hResult)
        {
            // Must be a live object
            hResult = m_pBroadcastMapper->FindBroadcastType(m_pURL->full, this);
            // continues in BroadcastTypeFound()
            return HXR_OK;
        }
        else
        {
            if (m_pPlayerSession && m_pPlayerSession->m_bBlockTransfer)
            {
                // See if the "client" has requested a block transfer ... this is
                // a hack but the url is irrelevant in this case
                hResult = FindStatic("application/x-pn-block-transfer", NULL);

                if (HXR_FAIL == hResult)
                {
                    m_pPlayerSession->SendAlert(SE_DATATYPE_UNSUPPORTED);
                }
                // XXXAAK, XXXSMP - quick fix for now -- need to handle errors
                else if (HXR_OK != hResult)
                {
                    m_pPlayerSession->SendAlert(SE_INTERNAL_ERROR);
                }
                return HXR_OK;
            }

            hResult = m_pFileObject->QueryInterface(IID_IHXFileMimeMapper,
                (void**)&m_pMimeMapper);

            if(HXR_OK == hResult)
            {
                m_pMimeMapper->FindMimeType(m_pURL->full, this);
                // Continues in MimeTypeFound()..
            }
            else
            {
                DPRINTF(D_INFO, ("No BroadcastMapper, no FileMimeMapper\n"));

                /*
                * "+" URL Hack
                */
                if (m_pPlayerSession && HXXFile::IsPlusURL(m_pURL->full))
                {
                    hResult = FindStatic("application/x-pn-plusurl", NULL);
                }
                else
                {
                    hResult = FindStatic(NULL, m_pURL->ext);
                }

                if(m_pPlayerSession && !m_pPlayerSession->m_pClient->m_bIsAProxy)
                {
                    if (HXR_NOT_SUPPORTED == hResult || HXR_FAIL == hResult)
                    {
                        m_pPlayerSession->SendAlert(SE_DATATYPE_UNSUPPORTED);
                    }
                    // XXXAAK, XXXSMP - quick fix for now -- need to handle errors
                    else if (HXR_OK != hResult)
                    {
                        m_pPlayerSession->SendAlert(SE_INTERNAL_ERROR);
                    }
                }
                else if (HXR_OK != hResult)
                {
                    FindSourceDone(hResult, NULL);
                }
            }
        }

        return HXR_OK;
    }
    else if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }
    else
    {
        FindSourceDone(HXR_FAIL, NULL);
        return HXR_OK; //XXXSMP Handle Errors
    }

}

STDMETHODIMP
BasicSourceFinder::DirObjectReady(HX_RESULT status,
                                  IUnknown* pDirObject)
{
    HX_ASSERT(!"Not Implemented");
    return HXR_NOTIMPL;
}

/*
* IHXFileMimeMapperResponse
*/
STDMETHODIMP
BasicSourceFinder::MimeTypeFound(HX_RESULT status, const char* pMimeType)
{
    // XXXSMP This isn't good enough, what if the fileobject doesn't know
    //  This is possible
    if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }

    HX_ASSERT(m_pFileObject);

    m_CurrentSourceType = pMimeType;

    if (pMimeType)
    {
        FindStatic(pMimeType, NULL);
    }
    else
    {
        FindStatic(NULL, m_pURL->ext);
    }

    return HXR_OK;
}


/*
* IHXBroadcastMapperResponse
*/
STDMETHODIMP
BasicSourceFinder::BroadcastTypeFound(HX_RESULT status, const char* pType)
{
    if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }

    m_CurrentSourceType = pType;

    HX_ASSERT(m_pBroadcastMapper);
    HX_RELEASE(m_pBroadcastMapper);

    PluginHandler::BroadcastFormat* broadcast_handler;
    PluginHandler::Errors            plugin_result;
    PluginHandler::Plugin*          plugin;

    broadcast_handler =
        m_pProc->pc->plugin_handler->m_broadcast_handler;
    plugin_result = broadcast_handler->Find(pType, plugin);

    if(PluginHandler::NO_ERRORS != plugin_result)
    {
        ERRMSG(m_pProc->pc->error_handler, "No live handler for %s\n",
               pType?pType:"(null)");
        return HXR_FAIL;
    }


    /*
    * Depending on a request
    */
    HX_RESULT  theErr = HXR_FAIL;
    IHXPSourceControl* pSource = NULL;
    IHXValues* pVal = NULL;
    IHXBuffer* pBuf = NULL;

    const char* pFileName = m_pURL->name + m_pFSManager->m_mount_point_len - 1;
    UINT32     bUseSourceContainer = FALSE;
    BOOL       bUseMediaDeliveryPipeline = FALSE;

    /* XXDWL
     * For live (broadcast) sessions, the media delivery pipeline needs to be turned off.
     * This is a temporary solution until a number of issues dealing with timestamp
     * dependent media, and ASM rule handling are worked out.
     * See also: server/engine/session/player.cpp:1947

     bUseMediaDeliveryPipeline = m_pPlayerSession->m_bUseMediaDeliveryPipeline;
    */

#if NOTYET
    /* XXTDM
     * According to ghori, this was an attempt to merge the various source
     * finding code into one place.  Due to time constraints, it was never
     * finished.  Therefore, this code is currently unused and untested.
     */
    theErr = m_pRequest->GetRequestHeaders(FS_HEADERS, pVal);
    if (HXR_OK == theErr)
    {
        theErr = pVal->GetPropertyULONG32("UseSourceContainer", bUseSourceContainer);
        if (HXR_OK != theErr)
        {
            bUseSourceContainer = FALSE;
        }

        theErr = pVal->GetPropertyCString("BroadcastAlias", pBuf);
        if (HXR_OK == theErr)
        {
            pFileName = (const char*)pBuf->GetBuffer();
        }
    }
#endif /* NOTYET */

    HX_ASSERT(pFileName);
    theErr = m_pProc->pc->broadcast_manager->GetStream(pType, pFileName,
                           pSource, m_pProc, bUseMediaDeliveryPipeline,
                           m_pPlayerSession ? m_pPlayerSession->GetSessionStats()
                                            : NULL);
    if (HXR_OK == theErr)
    {
        if (bUseSourceContainer)
        {
            SourceContainer* pContainer =
                new SourceContainer(m_pProc->pc->server_context, pSource);

            if (pContainer)
            {
                pContainer->AddRef();
                FindSourceDone(HXR_OK, (IUnknown*)(IHXPSinkPackets*)pContainer);
                pContainer->Release();
            }
            else
            {
                theErr = HXR_OUTOFMEMORY;
                HX_RELEASE(pSource);
            }
        }
    }

    FindSourceDone(theErr, pSource);

    HX_RELEASE(pSource);
    HX_RELEASE(pVal);
    HX_RELEASE(pBuf);
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::FindFileFormat
// Purpose:
//  Given a mime type or file extension, creates a corresponding
//  FF plugin and initializes it
HX_RESULT
BasicSourceFinder::FindFileFormat(const char* pMimeType,
                                  const char* pExt,
                                  REF(IHXFileFormatObject*)pFF)
{
    if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }

    PluginHandler::FileFormat*  pFFHandler =
        m_pProc->pc->plugin_handler->m_file_format_handler;
    PluginHandler::Errors   pluginResult = PluginHandler::PLUGIN_NOT_FOUND;
    IUnknown*               pInstance = NULL;
    IHXPlugin*              pPlugin = NULL;;
    HX_RESULT               theErr = HXR_OK;

    HX_ASSERT((!m_pCurrentPlugin && (pMimeType || pExt) ||
        (m_pCurrentPlugin && (!pMimeType || !pExt))));

    // if m_pCurrentPlugin, this is a call from FindNextPlugin.  Simply
    // take the next plugin and see what happens.
    if (!m_pCurrentPlugin)
    {
        m_pCurrentPlugin = pFFHandler->FindPluginInfo(pMimeType, pExt);
    }
    else
    {
        m_pCurrentPlugin = m_pCurrentPlugin->m_pNextPlugin;
        // FindNextSource() is checking so this shouldn't happen.
        HX_ASSERT(m_pCurrentPlugin);
    }

    if (m_pCurrentPlugin)
    {
        pluginResult = m_pCurrentPlugin->m_pPlugin->GetInstance(&pInstance);
    }

    if (PluginHandler::NO_ERRORS == pluginResult)
    {
        theErr = pInstance->QueryInterface(IID_IHXPlugin, (void**)&pPlugin);
    }
    else
    {
        theErr = HXR_NOT_SUPPORTED;
    }

    if (HXR_OK == theErr)
    {
        theErr = pInstance->QueryInterface(IID_IHXFileFormatObject, (void**)&pFF);
    }

    // Init the FF plugin with the server context
    if (HXR_OK == theErr)
    {
        theErr = pPlugin->InitPlugin(m_pProc->pc->server_context);
    }

    HX_RELEASE(pPlugin);
    HX_RELEASE(pInstance);
    return theErr;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::FindStatic
// Purpose:
//  Creates/inits an appropriate FF plugin for the mimetype/extension
HX_RESULT
BasicSourceFinder::FindStatic(const char* pMimeType,
                              const char* pExt)
{
    if (SF_CLOSED == m_findState)
    {
        return HXR_OK;
    }

    HX_ASSERT(m_pFileObject);

    HX_RESULT                   theErr = HXR_OK;
    enum Type
    {
        FILE_FORMAT_SOURCE,  // default
        STATIC_PUSH,
        STATIC_SOURCE_CONTAINER,
        RAW_FILE_FORMAT
    };

    const char* pName = NULL;
    Type type = FILE_FORMAT_SOURCE;

    /*
     * For debugging to enabled old FileFormatSource in ff_source.
     * Not a public config option
     */
    if (pName = m_pProc->pc->config->GetString(m_pProc, "config.IS"))
    {
        if (!strcmp(pName, "MDP"))
        {
            type = STATIC_PUSH;
        }
        else
        {
            type = FILE_FORMAT_SOURCE;
        }
    }
    else
    {
        type = STATIC_PUSH;
    }

    IHXFileFormatObject* pFF = NULL;

    // Create/init an appropriate FF plugin for the mimetype/extension
    theErr = FindFileFormat(pMimeType, pExt, pFF);

    if (HXR_OK == theErr)
    {
        DPRINTF(D_INFO, ("Found handler for %s\n", m_pURL->ext));

        if (m_pPlayerSession->m_bUseMediaDeliveryPipeline)
        {
            type = STATIC_PUSH;
        }
#if NOTYET
        /* XXXTDM: more of the source finder merge attempt */
        else
        {
            // Get type
            IHXValues* pVal = NULL;
            theErr = m_pRequest->GetRequestHeaders(FS_HEADERS, pVal);
            if (HXR_OK == theErr)
            {
                UINT32 ulVal;
                theErr = pVal->GetPropertyULONG32("UseStaticSourceContainer", ulVal);
                if (HXR_OK == theErr && ulVal)
                {
                    type = STATIC_SOURCE_CONTAINER;
                }
            }
            HX_RELEASE(pVal);
        }
#endif /* NOTYET */

        theErr = HXR_OK;

        if (m_pFileFormatRequest == NULL)
        {
            m_pFileFormatRequest = new ServerRequestWrapper(FF_HEADERS, m_pRequest);
            m_pFileFormatRequest->AddRef();
        }

        // Legacy FF wrapper
        if (FILE_FORMAT_SOURCE == type)
        {
            FileFormatSource* pFileFormatSource =
                new FileFormatSource(m_pProc, pFF,
                m_pFSManager->m_mount_point_len,
                m_pFileObject, m_pFileFormatRequest, FALSE);
            if (pFileFormatSource)
            {
                pFileFormatSource->AddRef();
                FindSourceDone(theErr, (IUnknown*)(IHXPSourceControl*)pFileFormatSource);
                pFileFormatSource->Release();
            }
            else
            {
                theErr = HXR_OUTOFMEMORY;
            }
        }

        // CPacketHandler / ASM filter
        else if (STATIC_PUSH == type)
        {
            // Create the CPacketHandler
            CPacketHandler* pPushSource = NULL;
            theErr = MakePacketHandler(pFF, pPushSource);

            // Determine whether to use the ASM filter, and create/attach it
            IUnknown* pSource = NULL;
            if (HXR_OK == theErr)
            {
                theErr = HandleFilters((IHXServerPacketSource*)pPushSource, pSource);
            }

            if (HXR_OK == theErr)
            {
                FindSourceDone(theErr, pSource);
            }
            HX_RELEASE(pPushSource);
            HX_RELEASE(pSource);
        }
        else if (STATIC_SOURCE_CONTAINER == type)
        {
            StaticSourceContainer* pSource = NULL;
            FileFormatSource* pFileFormatSource =
                new FileFormatSource(m_pProc, pFF,
                m_pFSManager->m_mount_point_len,
                m_pFileObject, m_pFileFormatRequest, FALSE);

            if (pFileFormatSource)
            {
                pFileFormatSource->AddRef();
                pSource =
                    new StaticSourceContainer(m_pProc->pc->server_context,
                    pFileFormatSource);
            }

            if (pSource)
            {
                pSource->AddRef();
                FindSourceDone(theErr, (IUnknown*)(IHXPSinkPackets*)pSource);
                pSource->Release();
            }
            else
            {
                theErr = HXR_OUTOFMEMORY;
            }
            HX_RELEASE(pFileFormatSource);
        }
        else if (RAW_FILE_FORMAT == type)
        {
            // well we've got everything we need
            pFF->AddRef();
            FindSourceDone(theErr, (IUnknown*)(IHXFileFormatObject*)pFF);
            pFF->Release();
        }
        else
        {
            HX_ASSERT(!"unknown type for static");
            theErr = HXR_UNEXPECTED;
        }
    }

    if (HXR_OK != theErr)
    {
        ERRMSG(m_pProc->pc->error_handler, "No handler for %s\n",
               m_pURL->ext?m_pURL->ext:"(null)");
        if(m_pPlayerSession &&
            !m_pPlayerSession->m_pClient->m_bIsAProxy &&
            HXR_NOT_SUPPORTED == theErr)
        {
            m_pPlayerSession->SendAlert(SE_DATATYPE_UNSUPPORTED);
        }
    }

    HX_RELEASE(pFF);
    return theErr;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::MakePacketHandler
// Purpose:
//  Creates a CPacketHandler
HX_RESULT
BasicSourceFinder::MakePacketHandler(IHXFileFormatObject* pFF,
                                     REF(CPacketHandler*) pPushSource)
{
    pPushSource = new CPacketHandler(m_pProc, pFF,
        m_pFileObject, m_pFileFormatRequest);

    if (pPushSource)
    {
        pPushSource->AddRef();
        return HXR_OK;
    }
    else
    {
        return HXR_OUTOFMEMORY;

    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::HandleFilters
// Purpose:
//  Determines whether to use the ASM filter, and if so, creates/initializes one
// Returns:
//  pOut -- Either the ASM filter or pPushSource (if ASM filter is not created)
HX_RESULT
BasicSourceFinder::HandleFilters(IUnknown* pPushSource, REF(IUnknown*) pOut)
{
    HX_ASSERT(pPushSource);

    HX_RESULT        theErr = HXR_OK;

    // Default to using the ASM filter
    BOOL bUseASMFilter = TRUE;

    // Only use the ASM filter if the underlying source supports IHXASMSource
    if (bUseASMFilter)
    {
        IHXASMSource* pASMSource = NULL;
        theErr = pPushSource->QueryInterface(IID_IHXASMSource, (void**)&pASMSource);

        if (HXR_OK == theErr)
        {
            pASMSource->Release();
        }
        else
        {
            bUseASMFilter = FALSE;
        }
    }

    // Create/initialize the ASM filter, if necessary
    if (bUseASMFilter)
    {
        theErr = AttachASMFilter(pPushSource, pOut);
    }
    else
    {
        theErr = HXR_OK;
        pOut = pPushSource;
        pOut->AddRef();
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  BasicSourceFinder::AttachASMFilter
// Purpose:
//  Creates an ASM filter and hooks it up to pPushSource.  Also sets
//  the inital bandwidth (if specified in the URL)
HX_RESULT
BasicSourceFinder::AttachASMFilter(IUnknown* pPushSource, REF(IUnknown*) pOut)
{
    HX_ASSERT(pPushSource);

    HX_RESULT theErr = HXR_OK;

    IHXPSourceControl* pCtrlSource = NULL;
    if (HXR_OK == theErr)
    {
        theErr = pPushSource->QueryInterface(IID_IHXPSourceControl, (void**)&pCtrlSource);
    }

    IHXServerPacketSource*  pPacketSource = NULL;
    if (HXR_OK == theErr)
    {
        theErr = pPushSource->QueryInterface(IID_IHXServerPacketSource, (void**)&pPacketSource);
    }

    // Get the client profile, if necessary
    if (HXR_OK == theErr && !m_pClientProfile)
    {
        GetClientProfileInfo();
    }

    // Create the ASM filter
    CASMStreamFilter* pASMFilter = NULL;
    if (HXR_OK == theErr)
    {
        HX_ASSERT(m_pPlayerSession->m_pStats);
        pASMFilter = new CASMStreamFilter(m_pProc, m_pPlayerSession->m_pStats, m_pConfig);
        pASMFilter->AddRef();
    }

    if (!pASMFilter)
    {
        theErr = HXR_OUTOFMEMORY;
    }

    IHXPSinkControl* pCtrlSink = NULL;
    if (HXR_OK == theErr)
    {
        theErr = pASMFilter->QueryInterface(IID_IHXPSinkControl, (void**)&pCtrlSink);
    }

    IHXServerPacketSink* pPacketSink = NULL;
    if (HXR_OK == theErr)
    {
        theErr = pASMFilter->QueryInterface(IID_IHXServerPacketSink, (void**)&pPacketSink);
    }

    // Set the ASM filter as the sink for the underlying source
    if (HXR_OK == theErr)
    {
        HX_ASSERT(pCtrlSource && pPacketSource);
        HX_ASSERT(pCtrlSink && pPacketSink);

        // set sink to source
        theErr = pPacketSource->SetSink(pPacketSink);
    }

    // Set the packet control interface
    if (HXR_OK == theErr)
    {
        theErr = pPacketSink->SetSource(pPacketSource);
    }

    // Set the header control interface
    if (HXR_OK == theErr)
    {
        theErr = pASMFilter->SetControlSource(pCtrlSource);
    }

#if 1 // initial rate hack
    // Allow the initial rate to be set via the URL
    if (HXR_OK == theErr)
    {
        const char* pcURL = NULL;
        const char* pc = NULL;
        const char* pcir = NULL;

        UINT32 ulInitialVal = 0;

        theErr = m_pRequest->GetURL(pcURL);
        if (HXR_OK == theErr)
        {
            pc = strstr(pcURL, "?");
        }
        if (pc)
        {
            pcir = strstr(pc, "ir=");
        }
        if (pcir)
        {
            pcir+=3;
            ulInitialVal = atoi(pcir);
            pASMFilter->SetInitial(0, ulInitialVal);
        }
    }
#endif

    HX_RELEASE(pCtrlSource);
    HX_RELEASE(pPacketSource);
    HX_RELEASE(pCtrlSink);
    HX_RELEASE(pPacketSink);

    if (HXR_OK == theErr)
    {
        theErr = pASMFilter->QueryInterface(IID_IUnknown, (void**)&pOut);
    }
    HX_RELEASE(pASMFilter);

    return theErr;
}

HX_RESULT
BasicSourceFinder::GetClientProfileInfo()
{
    HX_ASSERT(m_pPlayerSession);
    HX_ASSERT(!m_pClientProfile);
    if(m_pPlayerSession->m_pStats)
    {
        return m_pPlayerSession->m_pStats->
            GetClientProfileInfo(m_pClientProfile);
    }

    return HXR_FAIL;
}

STDMETHODIMP_(ULONG32)
BasicSourceFinder::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
BasicSourceFinder::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}
STDMETHODIMP
BasicSourceFinder::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileSystemManagerResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileSystemManagerResponse))
    {
        AddRef();
        *ppvObj = (IHXFileSystemManagerResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileMimeMapperResponse))
    {
        AddRef();
        *ppvObj = (IHXFileMimeMapperResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXBroadcastMapperResponse))
    {
        AddRef();
        *ppvObj = (IHXBroadcastMapperResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

