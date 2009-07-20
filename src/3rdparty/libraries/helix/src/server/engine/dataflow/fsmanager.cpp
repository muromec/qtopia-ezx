/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fsmanager.cpp,v 1.8 2006/11/20 13:30:22 asrivastava Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "hxstring.h"
#include "hxmap.h"

#include "fsmanager.h"
#include "hxplugn.h"
#include "plgnhand.h"
#include "url.h"
#include "fsys_wrap.h"
#include "flob_wrap.h"
#include "server_context.h"
#include "hxmap.h"
#include "hxrquest.h"
#include "servbuffer.h"
#include "chxpckts.h"
#include "misc_plugin.h"
#include "hxxfile.h"
#include "tsfob.h"
#include "cdist_wrappers.h"
#include "timeval.h"
#include "globals.h"
#include "servsked.h"

#include <signal.h>

IMPLEMENT_SAFE_DELETION(FSManager, m_scheduler)

FSManager::FSManager(Process *p)
{
    proc        = p;
    m_scheduler = (IHXScheduler*)proc->pc->scheduler;
    m_scheduler->AddRef();
    m_pCachedFSMap = proc->pc->cached_fs_map;
    m_ulRefCount = 0;
    m_pool     = NULL;
    m_pRequest = NULL;
    m_pAdvise  = NULL;
    m_bHandleRedirect = FALSE;
    m_bCheckingCache  = FALSE;
    m_pRedirectResponse = NULL;
    m_pResponse = NULL;
    m_url       = NULL;
    m_file_object = NULL;
    m_file_exists = NULL;
    m_last_plugin = NULL;
    m_file_system_wrapper = NULL;
    m_last_options = NULL;
    m_first_options = NULL;
    m_bIsDone = FALSE;
    m_ulHandlingPlusUrl = 0;
    m_bIsCDistEligible = FALSE;
    m_bCollectReadStats = FALSE;
}

FSManager::~FSManager()
{
    /*
     * Do not delete this, it belongs to the process!
     */
    m_pCachedFSMap = 0;

    HX_RELEASE(m_pool);
    HX_RELEASE(m_file_object);
    HX_RELEASE(m_file_exists);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pRedirectResponse);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_file_system_wrapper);
    HX_RELEASE(m_last_options);
    HX_RELEASE(m_first_options);
    HX_RELEASE(m_scheduler);
    HX_RELEASE(m_pAdvise);

    HX_VECTOR_DELETE(m_url);
}

STDMETHODIMP FSManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileSystemManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileSystemManager))
    {
        AddRef();
        *ppvObj = (IHXFileSystemManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileExistsResponse))
    {
        AddRef();
        *ppvObj = (IHXFileExistsResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGetFileFromSamePoolResponse))
    {
        AddRef();
        *ppvObj = (IHXGetFileFromSamePoolResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXHTTPRedirect))
    {
        AddRef();
        *ppvObj = (IHXHTTPRedirect*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXHTTPRedirectResponse))
    {
        AddRef();
        *ppvObj = (IHXHTTPRedirectResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistributionAdviseResponse))
    {
        AddRef();
        *ppvObj = (IHXContentDistributionAdviseResponse*)this;
        return HXR_OK;
    }


    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) FSManager::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) FSManager::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    DELETE_SAFELY();
    return 0;
}

STDMETHODIMP
FSManager::Init(IHXFileSystemManagerResponse* /*IN*/  pFileManagerResponse)
{
    if(m_pResponse)
    {
        m_pResponse->Release();
    }
    m_pResponse = pFileManagerResponse;
    m_pResponse->AddRef();
    m_pResponse->InitDone(HXR_OK);

    return HXR_OK;
}

STDMETHODIMP
FSManager::GetFileObject(IHXRequest* pRequest,
                         IHXAuthenticator* pAuth)
{
    PluginHandler::Errors       plugin_result;
    PluginHandler::FileSystem*  file_sys_handler;
    UINT32                      mount_point_len = 0;

    if (!pRequest)
    {
        return HXR_FAIL;
    }
    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;
    m_pRequest->AddRef();

    const char* temp;

    if (m_pRequest->GetURL(temp) != HXR_OK)
    {
        m_pResponse->FileObjectReady(HXR_FAILED, NULL);
        return HXR_FAIL;
    }

    HX_VECTOR_DELETE(m_url);

    if (*temp == '/')
    {
        m_url = new_string(temp);               //XXXGH
    }
    else
    {
        m_url = new char[strlen(temp) + 2];     //XXXSMP
        sprintf(m_url,"/%s", temp);             //XXXSMP
    }

    file_sys_handler = proc->pc->plugin_handler->m_file_sys_handler;

    plugin_result = file_sys_handler->Find(m_url, 0,
                                           mount_point_len,
                                           m_last_plugin,
                                           m_last_options);

    if (PluginHandler::NO_ERRORS != plugin_result)
    {
        m_pResponse->FileObjectReady(HXR_FAILED, NULL);
        return HXR_FAIL;
    }
    m_mount_point_len = mount_point_len;

    // remember first match for cdist-eligiblity

    if (m_first_options) m_first_options->Release();
    m_first_options = m_last_options;
    if (m_first_options) m_first_options->AddRef();

    return CheckNextPlugin(m_last_plugin);
}

STDMETHODIMP
FSManager::GetNewFileObject(IHXRequest* pRequest,
                            IHXAuthenticator* pAuth)
{
    PluginHandler::Errors       plugin_result;
    PluginHandler::FileSystem*  file_sys_handler;
    UINT32                      mount_point_len = 0;
    HX_RESULT                   h_result = HXR_OK;
    BOOL                        found_plugin = FALSE;
    IUnknown*                   instance;
    IHXPlugin*                  plugin_interface;
    IHXFileSystemObject*        file_system;
    PluginHandler::Plugin*      plugin;
    IHXRequestHandler*         pRequestHandler;

    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;
    m_pRequest->AddRef();

    const char* temp;

    if (m_pRequest->GetURL(temp) != HXR_OK)
    {
        m_pResponse->FileObjectReady(HXR_FAILED, NULL);
        return HXR_FAIL;
    }

    HX_VECTOR_DELETE(m_url);

    if (*temp == '/')
    {
        m_url = new_string(temp);               //XXXGH
    }
    else
    {
        m_url = new char[strlen(temp) + 2];     //XXXSMP
        sprintf(m_url,"/%s", temp);             //XXXSMP
    }

    file_sys_handler = proc->pc->plugin_handler->m_file_sys_handler;

    plugin_result = file_sys_handler->Find(m_url, 0,
                                           mount_point_len,
                                           m_last_plugin,
                                           m_last_options);

    if (PluginHandler::NO_ERRORS != plugin_result)
    {
        m_pResponse->FileObjectReady(HXR_FAILED, NULL);
        return HXR_FAIL;
    }

    m_mount_point_len = mount_point_len;
    plugin = m_last_plugin->m_pPlugin;

    if(plugin->m_load_multiple)
    {
        if (PluginHandler::NO_ERRORS != plugin->GetInstance(&instance))
        {
            h_result = HXR_FAIL;
        }

        if (HXR_OK == h_result)
        {
            h_result = instance->QueryInterface(IID_IHXPlugin,
                                                (void**) &plugin_interface);
            if (HXR_OK == h_result)
            {
                h_result = plugin_interface->InitPlugin(proc->pc->server_context);
            }

            plugin_interface->Release();

            if (HXR_OK == h_result)
            {
                h_result = instance->QueryInterface(IID_IHXFileSystemObject,
                                                    (void**) &file_system);
            }

            if(HXR_OK == h_result)
            {
                if(m_last_options)
                {
                    h_result = file_system->InitFileSystem(m_last_options);
                    m_last_options->Release();
                    m_last_options = NULL;
                }
            }

            if(m_file_object)
            {
                m_file_object->Release();
                m_file_object = NULL;
            }

            if (HXR_OK == h_result)
            {
                h_result = file_system->CreateFile(&m_file_object);

                if (!g_bForceThreadSafePlugins)
                {
                    ThreadSafeFileObjectWrapper::MakeThreadSafe(m_file_object,
                                                                proc);
                }

                file_system->Release();
            }

            if(HXR_OK == h_result)
            {
                const char* pURL = m_url + m_mount_point_len;

                // pURL shouldn't have a leading slash.  However, we need a
                // trailing slash, so something at the top level needs a fixup.

                if (*pURL == '\0' && m_mount_point_len)
                {
                    --pURL;
                    HX_ASSERT(*pURL == '/');
                }

                m_pRequest->SetURL(pURL);

                if (HXR_OK ==
                    m_file_object->QueryInterface(IID_IHXRequestHandler,
                                                  (void**)&pRequestHandler))
                {
                    pRequestHandler->SetRequest(m_pRequest);
                    pRequestHandler->Release();
                }

                m_pResponse->FileObjectReady(HXR_OK, m_file_object);
            }
            else
            {
                m_pResponse->FileObjectReady(h_result, NULL);
            }

            HX_RELEASE(instance);
        }
    }
    else // Not load multiple
    {
        //XXXJR FIXME
        ASSERT(0);
    }
    return h_result;
}

STDMETHODIMP
FSManager::GetRelativeFileObject(IUnknown* pOriginalObject,
                                 const char* pPath)
{
    IHXRequestHandler* pRequestHandlerOrigional = NULL;
    IHXRequest* pRequestOld = NULL;

    if(m_pool)
    {
        m_pool->Release();
        m_pool = 0;
    }

    if (HXR_OK != pOriginalObject->QueryInterface(IID_IHXGetFileFromSamePool,
           (void**)&m_pool))
    {
        return HXR_FAIL;
    }

    if ((HXR_OK != pOriginalObject->QueryInterface(IID_IHXRequestHandler,
        (void**)&pRequestHandlerOrigional)) || (pRequestHandlerOrigional ==
        NULL) || (HXR_OK != pRequestHandlerOrigional->GetRequest(
        pRequestOld)) || (pRequestOld == NULL))
    {
        HX_RELEASE(pRequestHandlerOrigional);
        HX_RELEASE(m_pRequest);

        return HXR_FAIL;
    }
    HX_RELEASE(pRequestHandlerOrigional);

    /*
     *  Did the old object collect read statistics?
     *  This will only be true if cdist is active, and the content
     *  was found locally.
     */
    IHXMIIReadStatCollection* pMSC;
    if (HXR_OK == pOriginalObject->QueryInterface(IID_IHXMIIReadStatCollection,
                                                  (void**)&pMSC))
    {
        pMSC->GetMIIReadStatsEnabled(m_bCollectReadStats);
        pMSC->Release();
    }

    /*
     * Create an IHXRequest object for the new file
     */

    HX_RELEASE(m_pRequest);

    CHXRequest::CreateFromCCF(pRequestOld, &m_pRequest, proc->pc->server_context);
    HX_RELEASE(pRequestOld);
    m_pRequest->SetURL(pPath);

    if(HXR_OK != m_pool->GetFileObjectFromPool(this))
    {
        m_pool->Release();
        m_pool = NULL;

        return HXR_FAIL;
    }
    return HXR_OK;
}

STDMETHODIMP
FSManager::FileObjectReady(HX_RESULT status,
                           IUnknown* pUnknown)
{
    IHXRequestHandler* pRequestHandler = NULL;

    if (m_pool)
    {
        m_pool->Release();
        m_pool = NULL;
    }

    if (HXR_OK == status)
    {
        pUnknown->AddRef();

        ThreadSafeFileObjectWrapper::MakeThreadSafe(pUnknown, proc);

        if (HXR_OK == pUnknown->QueryInterface(IID_IHXRequestHandler,
                                            (void**)&pRequestHandler))
        {
            pRequestHandler->SetRequest(m_pRequest);
        }
        // did original object collect read stats for cdist?

        IHXMIIReadStatCollection* pMSC;
        if (m_bCollectReadStats &&
            HXR_OK == pUnknown->QueryInterface(IID_IHXMIIReadStatCollection,
                                               (void**)&pMSC))
        {
            pMSC->SetMIIReadStatsEnabled(TRUE, NULL);
            pMSC->Release();
        }
    }

    m_pResponse->FileObjectReady(status,
                                 pUnknown);
    if (pRequestHandler)
    {
        pRequestHandler->Release();
    }

    if ((HXR_OK == status) && pUnknown)
    {
        pUnknown->Release();
    }

    /*
     * The IHXRequest object belongs to the IHXFileObject
     */
    HX_RELEASE(m_pRequest);

    return HXR_OK;
}

HX_RESULT
FSManager::CheckNextPlugin(PluginHandler::FileSystem::PluginInfo* plugininfo)
{
    // XXXAAK -- just to see how often we recursively enter in here
    static int in_here = 0;
    in_here++;

    DPRINTF(D_ENTRY, ("FSM::CNP(%p, %d)\n", plugininfo, in_here));
    HX_RESULT h_result = HXR_OK;
    IUnknown*                   instance;
    IHXPlugin*                  plugin_interface;
    IHXFileSystemObject*        file_system;
    PluginHandler::Plugin*      plugin = plugininfo->m_pPlugin;
    IHXRequestHandler*          pRequestHandler;

    if(plugin->m_load_multiple)
    {
        /*
         * Check for this file sys cached in our table.  If it is not there,
         * create a new one and add it to the map.
         */
        file_system = 0;
        if(!m_pCachedFSMap->Lookup(plugininfo->m_ulID, (void*&)file_system) ||
            !file_system)
        {

            if (PluginHandler::NO_ERRORS != plugin->GetInstance(&instance))
            {
                h_result = HXR_FAIL;
            }

            if (HXR_OK == h_result)
            {
                h_result = instance->QueryInterface(IID_IHXPlugin,
                                                    (void**) &plugin_interface);
                if (HXR_OK == h_result)
                {
                    h_result = plugin_interface->InitPlugin(proc->pc->server_context);
                }

                plugin_interface->Release();

                if (HXR_OK == h_result)
                {
                    h_result = instance->QueryInterface(IID_IHXFileSystemObject,
                                                        (void**) &file_system);
                }

                if(m_last_options)
                {
                    file_system->InitFileSystem(m_last_options);
                    m_last_options->Release();
                    m_last_options = NULL;
                }

                if(m_file_object)
                {
                    m_file_object->Release();
                    m_file_object = NULL;
                }
                /*
                 * Cache this one away in our map.
                 */
                m_pCachedFSMap->SetAt(plugininfo->m_ulID, (void*)file_system);

                HX_RELEASE(instance);
            }
        }

        if (HXR_OK == h_result)
        {
            if (m_file_object)
            {
                m_file_object->Release();
            }
            h_result = file_system->CreateFile(&m_file_object);

            ThreadSafeFileObjectWrapper::MakeThreadSafe(m_file_object, proc);
        }

        if(m_file_exists)
        {
            m_file_exists->Release();
            m_file_exists = NULL;
        }

        if (HXR_OK == h_result)
        {
            BOOL bOk = TRUE;

            IHXFileRestrictor* pRes;
            if (HXR_OK == m_file_object->QueryInterface(IID_IHXFileRestrictor,
                (void**)&pRes))
            {
                IHXValues* pValues = NULL;
                IHXBuffer* pLocalPortBuf = NULL;
                const char* pLocalPort = NULL;

                m_pRequest->GetRequestHeaders(pValues);
                if (pValues)
                {
                    pValues->GetPropertyCString("LocalPort", pLocalPortBuf);
                    pValues->Release();
                }
                if (pLocalPortBuf != NULL)
                {
                    pLocalPort = (const char*)pLocalPortBuf->GetBuffer();
                }
                if (!pRes->IsAllowed(NULL, NULL, pLocalPort, NULL, NULL))
                {
                    bOk = FALSE;
                    h_result = DoesExistDone(FALSE);
                }
                HX_RELEASE(pLocalPortBuf);
                HX_RELEASE(pRes);
            }

            if (bOk)
            {
                const char* pURL = m_url + m_mount_point_len;

                m_pRequest->SetURL(pURL);

                if(HXR_OK == m_file_object->QueryInterface(IID_IHXRequestHandler,
                                                         (void**)&pRequestHandler))
                {
                    pRequestHandler->SetRequest(m_pRequest);
                    pRequestHandler->Release();
                }

                if (m_bHandleRedirect)
                {
                    IHXHTTPRedirect* pRedirect = NULL;

                    h_result = m_file_object->QueryInterface(IID_IHXHTTPRedirect,
                        (void**)&pRedirect);

                    if (HXR_OK == h_result)
                    {
                        pRedirect->Init((IHXHTTPRedirectResponse*)this);
                        HX_RELEASE(pRedirect);
                    }
                }

                h_result = m_file_object->QueryInterface(IID_IHXFileExists,
                                                       (void**)&m_file_exists);

                if (HXR_OK == h_result)
                {
                    /*
                     * Strip off parameters
                     */

                    char* pPtr = (char*)::strrchr(pURL, '?');

                    if (pPtr)
                    {
                        *pPtr = '\0';
                    }

                    /*
                     * Plus url hack. Need to does exist each side.
                     */
                    if (HXXFile::IsPlusURL(pURL))
                    {
                        m_ulHandlingPlusUrl = 1;
                        m_plusPathLeft = pURL;
                        CHXString plusFileName;

                        INT32 index = m_plusPathLeft.ReverseFind('+');

                        plusFileName = m_plusPathLeft.Right(m_plusPathLeft.GetLength() - (index+1));
                        m_plusPathLeft = m_plusPathLeft.Left(index);
                        index = m_plusPathLeft.ReverseFind('/');

                        if (index >= 0)
                        {
                            m_plusPathRight = m_plusPathLeft.Left(index+1);
                            m_plusPathRight = m_plusPathRight + plusFileName;
                        }
                        else
                        {
                            m_plusPathRight = plusFileName;
                        }

                        HXXFile::GetReasonableLocalFileName(m_plusPathRight);
                        HXXFile::GetReasonableLocalFileName(m_plusPathLeft);

                        h_result = m_file_exists->DoesExist(m_plusPathRight, this);
                    }
                    else
                    {
                        if (strlen(pURL))
                        {
                            h_result = m_file_exists->DoesExist(pURL, this);
                        }
                        else
                        {
                            h_result = HXR_FAIL;
                        }
                    }

                    if (HXR_OK != h_result)
                        DoesExistDone(FALSE);

                    DPRINTF(D_ENTRY, ("FSM::CNP(%d) -- after %ld "
                            "m_file_exists(%p)->DoesExist(%s)\n",
                            in_here, h_result, m_file_exists, pURL));
                }
            }
        }
        else
        {
            m_pResponse->FileObjectReady(h_result, NULL);
        }
    }
    else // Not load multiple
    {
        Process*                    fsproc;
        IHXFileSystemObject*        pFS;
        FSPlugin*                   pFSPlugin;
        BOOL                        bRet = TRUE;

        bRet = proc->pc->misc_plugins->Lookup(plugin, (void *&)pFSPlugin);
        ASSERT(bRet == TRUE);

        if (bRet)
        {
            fsproc = pFSPlugin->m_pProc;
            pFS    = pFSPlugin->m_pFS;

            m_file_system_wrapper =
                new FileSystemWrapper(this, proc, fsproc, pFS);
            m_file_system_wrapper->AddRef();

            if(m_last_options)
            {
                m_file_system_wrapper->InitFileSystem(m_last_options);
                m_last_options->Release();
                m_last_options = NULL;
            }

            h_result = m_file_system_wrapper->AsyncCreateFile();
        }
        else
        {
            h_result = HXR_FAIL;
        }
    }

    in_here--;
    return h_result;
}

HX_RESULT
FSManager::AsyncCreateFileDone(HX_RESULT status,
                               FileObjectWrapper* file_object_wrapper)
{
    HX_RESULT h_result;
    IHXRequestHandler* pRequestHandler = NULL;

    m_file_system_wrapper->Release();
    m_file_system_wrapper = NULL;

    if(HXR_OK != status || m_bIsDone)
    {
        return DoesExistDone(FALSE);
    }

    h_result = file_object_wrapper->QueryInterface(IID_IUnknown,
                                                   (void**)&m_file_object);
    file_object_wrapper->Release();

    if (HXR_OK == h_result)
    {
        if (m_bHandleRedirect)
        {
            IHXHTTPRedirect* pRedirect = NULL;

            h_result = m_file_object->QueryInterface(IID_IHXHTTPRedirect,
                (void**)&pRedirect);

            if (HXR_OK == h_result)
            {
                pRedirect->Init((IHXHTTPRedirectResponse*)this);
                HX_RELEASE(pRedirect);
            }
        }

        h_result = m_file_object->QueryInterface(IID_IHXFileExists,
                                                 (void**)&m_file_exists);
    }

    const char* pURL = m_url + m_mount_point_len;

    m_pRequest->SetURL(pURL);

    if (HXR_OK == h_result)
    {
        if(HXR_OK == m_file_object->QueryInterface(IID_IHXRequestHandler,
                                                 (void**)&pRequestHandler))
        {
            pRequestHandler->SetRequest(m_pRequest);
            pRequestHandler->Release();
        }

        /*
         * Strip off parameters
         */

        char* pPtr = (char*)::strrchr(pURL, '?');

        if (pPtr)
        {
            *pPtr = '\0';
        }

        h_result = m_file_exists->DoesExist(pURL, this);

        if (HXR_OK != h_result)
            DoesExistDone(FALSE);
    }

    return h_result;
}

STDMETHODIMP
FSManager::DoesExistDone(BOOL bExists)
{
    DPRINTF(D_ENTRY, ("FSM(%p)::DoesExistDone(bExists(%lu))\n",
            this, bExists));
    PluginHandler::Errors       plugin_result;
    PluginHandler::FileSystem*  file_sys_handler;
    UINT32                      mount_point_len = 0;

    if (bExists)
    {
        if (m_ulHandlingPlusUrl == 1)
        {
            m_ulHandlingPlusUrl = 2;
            HX_RESULT res;

            /*
             * I can't see any path through the code where this
             * could get released before we get here again, but
             * just to make sure...
             */
            HX_ASSERT(m_file_exists);
            if (!m_file_exists)
            {
                HX_ASSERT(m_file_object);
                if (!m_file_object)
                {
                    DoesExistDone(0);
                    return HXR_OK;
                }

                if (m_bHandleRedirect)
                {
                    IHXHTTPRedirect* pRedirect = NULL;

                    res = m_file_object->QueryInterface(IID_IHXHTTPRedirect,
                        (void**)&pRedirect);

                    if (HXR_OK == res)
                    {
                        pRedirect->Init((IHXHTTPRedirectResponse*)this);
                        HX_RELEASE(pRedirect);
                    }
                }

                m_file_object->QueryInterface(IID_IHXFileExists,
                    (void**)m_file_exists);
            }
            res = m_file_exists->DoesExist(m_plusPathLeft, this);
            if (res != HXR_OK)
            {
                DoesExistDone(0);
            }
            return HXR_OK;
        }

        ASSERT(m_file_object != NULL);

        /* It is possible (and indeed it happens with hmonfsys) for this
           FSManager object to be completely released inside the call to
           FileObjectReady below.  But we can't release the file object
           pointers until after that call.  And if we've been released
           and then try to use thos pointers again, even to check if
           they're NULL, we will be accessing deleted memory.

           Solution: AddRef ourself here, Release ourself at the end
           of this code block.
           */
        AddRef();

        ASSERT(m_pResponse != NULL);
        if (m_pResponse)
        {
            FileReadyHook(HXR_OK, m_file_object);
        }

        /* In fact, the things that can happen inside FileObjectReady
           when using a FS like hmonfsys are truly mind boggling.
           These things may be released by a different call to
           DoesExistDone as well, so in spite of the ASSERT above
           it's possible that they're NULL here. I need some aspirin. */
        if(m_file_object)
        {
            m_file_object->Release();
            m_file_object = NULL;
        }

        if(m_file_exists)
        {
            m_file_exists->Release();
            m_file_exists = NULL;
        }

        /*
         * The IHXRequest object belongs to the IHXFileObject
         */
        HX_RELEASE(m_pRequest);

        Release();
        return HXR_OK;
    }
    else
    {
        HX_RESULT res;
        /*
           AddRef ourself here, Release ourself at the end of this code block.
           The code block above does this and this fix was apparently never
           bubbled down to this code path. If the client is destroyed at just
           the right time we can wind up in here after this FSManager has been
           completely released.
        */
        AddRef();

        if(m_file_object)
        {
            m_file_object->Release();
            m_file_object = NULL;
        }

        if(m_file_exists)
        {
            m_file_exists->Release();
            m_file_exists = NULL;
        }

        file_sys_handler = proc->pc->plugin_handler->m_file_sys_handler;
        plugin_result = file_sys_handler->FindAfter(m_url,
                                                    mount_point_len,
                                                    m_last_plugin,
                                                    m_last_options);

        DPRINTF(D_INFO, ("FSM::DED -- after FindAfter() - result(%lu)\n",
                plugin_result));
        if (PluginHandler::NO_ERRORS != plugin_result)
        {
            ASSERT(m_pResponse != NULL);
            if (m_pResponse)
            {
                FileReadyHook(HXR_FAILED, NULL);
            }

            Release();
            return HXR_FAIL;
        }
        m_mount_point_len = mount_point_len;

        res = CheckNextPlugin(m_last_plugin);

        Release();
        return res;
    }
}

STDMETHODIMP
FSManager::GetDirObjectFromURL(const char* pURL)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
FSManager::Init(IHXHTTPRedirectResponse* pRedirectResponse)
{
    m_bHandleRedirect = TRUE;

    HX_RELEASE(m_pRedirectResponse);
    m_pRedirectResponse = pRedirectResponse;
    m_pRedirectResponse->AddRef();

    return HXR_OK;
}

STDMETHODIMP
FSManager::SetResponseObject(IHXHTTPRedirectResponse* pRedirectResponse)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
FSManager::RedirectDone(IHXBuffer* pURL)
{
    HX_ASSERT(m_bHandleRedirect && m_pRedirectResponse);

    return m_pRedirectResponse->RedirectDone(pURL);
}

/***********************************************************************
 *      FSManager::FileReadyHook
 *
 * This hook intervenes in the call to the response object's
 * FileObjectReady() method, in cases of interest.  (That is, I didn't
 * replace all calls to FileObjectReady().)
 *
 * We:
 *      - call the content distribution advise sink with file-found results
 *      - redirect pn-local misses to the content distribution system.
 *
 * 4/2001, jmevissen
 *
 */
HX_RESULT
FSManager::FileReadyHook(HX_RESULT result, IUnknown* pFileObject)
{
    // early exit, if not a cdist-type mount point

    // (m_bIsCDistEligible can only be false on the first entry
    // to this routine.)

    if (!m_bIsCDistEligible)    // initialized false
    {
        if (m_first_options)
        {
            ULONG32 value;
            if (HXR_OK == m_first_options->
                GetPropertyULONG32("UseContentDistribution", value))
            {
                m_bIsCDistEligible = value != 0;
            }
        }

        // is now initialized, so check it

        if (!m_bIsCDistEligible)
        {
            m_pResponse->FileObjectReady(result, pFileObject);
            return HXR_OK;
        }
    }

    // object for distributing advise calls

    if (!m_pAdvise)
    {
        m_pAdvise = new CDistAdviseWrapper(proc->pc->server_context,
                                           proc);
        m_pAdvise->AddRef();
    }

    // is cdist operational?  (at least one cdist-advise plugin and
    // license okay)

    if (!m_pAdvise->IsOK())
    {
        m_pResponse->FileObjectReady(result, pFileObject);
        return HXR_OK;
    }

    // Okay, do the work.

    m_iResult = result;
    m_bFound = (result == HXR_OK);
    m_pFileObject = pFileObject;
    if (m_pFileObject) m_pFileObject->AddRef();

    HX_RESULT status;

    if (!m_bCheckingCache)
    {
        // cdist advise sink regarding file-found locally

        status = m_pAdvise->OnLocalResult(this, m_pRequest, m_bFound);

        // if found locally, try to turn on cdist statistic collection

        if (m_bFound && pFileObject)
        {
            IHXMIIReadStatCollection* pMSC;
            if (HXR_OK == pFileObject->QueryInterface(IID_IHXMIIReadStatCollection,
                                                      (void**)&pMSC))
            {
                pMSC->SetMIIReadStatsEnabled(TRUE, NULL);
                pMSC->Release();
            }
        }
    }
    else
    {
        // cdist advise sink regarding file-found in cache

        m_pAdvise->OnCacheResult(this, m_pRequest, m_bFound);
    }

    return HXR_OK;
}

    /*
     * IHXContentDistributionAdviseResponse methods
     */

STDMETHODIMP
FSManager::OnLocalResultDone(THIS_
                             HX_RESULT status)
{
    // if not found, check (modified) request in cache

    if (status == HXR_OK &&
        !m_bFound)
    {
        m_bCheckingCache = TRUE;

        // add mii transport header before going to mii/cache

        if (m_pAdvise->ForceRTSPImport())
        {
            AddRTSPImportHeader();
        }

        m_pAdvise->OnCacheRequest(this, m_pRequest);
    }
    else
    {
        m_pResponse->FileObjectReady(m_iResult, m_pFileObject);
        HX_RELEASE(m_pAdvise);
        HX_RELEASE(m_pFileObject);
    }

    return HXR_OK;
}

STDMETHODIMP
FSManager::OnCacheRequestDone(THIS_
                              HX_RESULT status)
{
    IHXRequest* pRequest = m_pRequest;
    pRequest->AddRef();
    GetFileObject(m_pRequest, NULL);  // call FOReady next time
    pRequest->Release();

    return HXR_OK;
}

STDMETHODIMP
FSManager::OnCacheResultDone(THIS_
                             HX_RESULT status)
{
    m_bCheckingCache = FALSE;
    m_pResponse->FileObjectReady(m_iResult, m_pFileObject);

    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pAdvise);

    return HXR_OK;
}

STDMETHODIMP
FSManager::OnSiteCacheResultDone(THIS_
                                 HX_RESULT status)
{
    return HXR_OK;
}

STDMETHODIMP
FSManager::OnPurgeCacheURLDone(THIS_
                               HX_RESULT status)
{
    return HXR_OK;
}

void
FSManager::AddRTSPImportHeader()
{
    IHXValues* pValues = NULL;
    IHXBuffer* pBuffer = NULL;

    m_pRequest->GetRequestHeaders(pValues);
    if (!pValues)
    {
        pValues = new CHXHeader;
        pValues->AddRef();
        m_pRequest->SetRequestHeaders(pValues);
    }

    if (pValues)
    {
        if (SUCCEEDED(ServerBuffer::FromCharArray("rtsp", 5, &pBuffer)))
        {
            pValues->SetPropertyCString("x-real-MEITransport", pBuffer);
        }
        else
        {
            // shouldn't happen
            HX_ASSERT(0);
        }
    }
    else
    {
        // shouldn't happen.
        HX_ASSERT(0);
    }

    HX_RELEASE(pValues);
    HX_RELEASE(pBuffer);
}
