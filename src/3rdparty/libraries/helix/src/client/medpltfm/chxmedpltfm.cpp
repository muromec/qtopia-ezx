/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxmedpltfm.cpp,v 1.46 2007/04/14 04:38:50 ping Exp $
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

#include "hxmedpltfm.ver"

#include "hxcom.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxwin.h"
#include "chxmedpltfm.h"
#include "chxmedpltfmex.h"
#include "chxmedpltfmsched.h"
#include "hxoptsc.h"
#include "hxobjbrokrids.h"

#include "chxpckts.h"
#include "pckunpck.h"
#include "hxpref.h"
#include "hxtbuf.h"
#include "hxmutex.h"
#include "timebuff.h"
#include "hxcore.h"

#include "hxvalues.h"
#include "hxrquest.h"
#include "hxfiles.h"
#include "hxlistp.h"
#include "dllaccesserver.h"
#include "cachobj.h"
#include "chxfgbuf.h"
#include "hxfsmgr.h"
#include "hxgrpen2.h"
#include "hxclreg.h"
#include "hxvsurf.h"
#include "hxsite2.h"
#include "hxmisus.h"
#include "cpacemkr.h"
#include "chxthread.h"
#include "hxpreferences.h"
#include "recognizer.h"
#include "hxxml.h"
#include "hxxmlprs.h"

#include "hxver.h"
#include "hxstrutl.h"
#include "dbcs.h"

//------------------------------- CHXMediaPlatform

BEGIN_INTERFACE_LIST( CHXMediaPlatform )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXMediaPlatform ) 
    INTERFACE_LIST_ENTRY_SIMPLE( IHXCommonClassFactory ) 
    INTERFACE_LIST_ENTRY_SIMPLE( IHXObjectManagerPrivate )
    INTERFACE_LIST_ENTRY_DELEGATE_BLIND( _InternalQI )
END_INTERFACE_LIST


CHXMediaPlatform* CHXMediaPlatform::CreateInstance(CHXMediaPlatform* pParent, 
                                                   CHXMediaPlatform* pRoot)
{
#ifdef HELIX_FEATURE_EXTENDED_MEDIAPLATFORM
    return new CHXMediaPlatformEx((CHXMediaPlatformEx*)pParent, (CHXMediaPlatformEx*)pRoot);
#else   // HELIX_FEATURE_EXTENDED_MEDIAPLATFORM
    return new CHXMediaPlatform(pParent, pRoot);
#endif  // HELIX_FEATURE_EXTENDED_MEDIAPLATFORM
}


CHXMediaPlatform::CHXMediaPlatform(CHXMediaPlatform* pParent, CHXMediaPlatform* pRoot)
    :m_bInitialized(FALSE)
    ,m_lastError(HXR_OK)
    ,m_pExtContext(NULL)
    ,m_pExtCCF(NULL)
    ,m_pPreferences(NULL)
    ,m_pScheduler(NULL)
    ,m_pScheduler2(NULL)
    ,m_pOptimizedScheduler(NULL)
    ,m_pOptimizedScheduler2(NULL)
    ,m_pMutex(NULL)
    ,m_pNetServices(NULL)
    ,m_pNetInterfaces(NULL)
    ,m_pKicker(NULL)
    ,m_pRegistry(NULL)
    ,m_pExtScheduler(NULL)
    ,m_pExtKicker(NULL)
    ,m_pPluginHandlerUnkown(NULL)
    ,m_pPluginPaths(NULL)
    ,m_pParent(pParent)
    ,m_pRoot(pRoot)
    ,m_pChildren(NULL)
    ,m_pSingleLoadPlugins(NULL)
    ,m_pLoadAtStartupPlugins(NULL)
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    ,m_pHyperNavigate(NULL)
    ,m_pDefaultHyperNavigate(NULL)
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */
#if defined(HELIX_FEATURE_REGISTRY)
    ,m_pClientRegistry(NULL)
#endif /* HELIX_FEATURE_REGISTRY */
#if defined(HELIX_FEATURE_VIDEO)    
    ,m_pSiteEventHandler(NULL)
#endif /* HELIX_FEATURE_VIDEO */
{
    HX_ADDREF(m_pParent);
    if (!m_pRoot)
    {
        m_pRoot = this;
    }
}

CHXMediaPlatform::~CHXMediaPlatform()
{
    Close();
}

HX_RESULT
CHXMediaPlatform::_InternalQI(REFIID riid, void** ppvObj)
{
    HX_RESULT           rc = HXR_OK;
    *ppvObj = NULL;

    // check instrinsic type which is kept at the root, instrinsic type is
    // prevented from being overwritten by the external context
    rc = CheckAndQueryInterface(HX_MP_COM_INSTRINSIC, riid, ppvObj);
    if (HXR_NOT_SUPPORTED == rc)
    {
        // this is regular type
        rc = CheckAndQueryInterface(HX_MP_COM_REGULAR, riid, ppvObj);
    }
        
    return rc;
}

STDMETHODIMP
CHXMediaPlatform::GetVersion(UINT32* pVersion)
{
    *pVersion = TARVER_ULONG32_VERSION;
    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatform::AddPluginPath(const char* pszName,
                                const char* pszPath)
{
    HX_RESULT       rc = HXR_OK;
    UINT32          ulBufSize = 0;
    IHXBuffer*      pPathBuffer = NULL;
    PluginPathInfo* pPluginPathInfo = NULL;

    // TODO: m_pPluginPaths needs to be protected by Mutex

    if (m_pParent)
    {
        return m_pParent->AddPluginPath(pszName, pszPath);
    }

    if (!pszPath || !pszName || !pszName[0] || !pszPath[0])
    {
        return HXR_INVALID_PARAMETER;
    }

    // check whether the same name or path has been added
    if (m_pPluginPaths)
    {
        CHXSimpleList::Iterator i;
        for (i = m_pPluginPaths->Begin(); i != m_pPluginPaths->End(); ++i)
        {
            PluginPathInfo* pTempPluginPathInfo = (PluginPathInfo*)*i;
            if (0 == stricmp(pTempPluginPathInfo->pszName, pszName) ||
                0 == stricmp((const char*)pTempPluginPathInfo->pPath->GetBuffer(), pszPath))
            {
                rc = HXR_OK;
                goto exit;
            }
        }
    }

    pPluginPathInfo = new PluginPathInfo();
    if (!pPluginPathInfo)
    {
        rc = HXR_OUTOFMEMORY;
        goto exit;
    }

    pPluginPathInfo->pszName = new char[strlen(pszName) + 1];
    strcpy(pPluginPathInfo->pszName, pszName);

    rc = CreateStringBufferCCF(pPathBuffer, pszPath, (IUnknown*)(IHXMediaPlatform*)this);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    pPluginPathInfo->pPath = pPathBuffer;
    HX_ADDREF(pPluginPathInfo->pPath);

    if (!m_pPluginPaths)
    {
        m_pPluginPaths = new CHXSimpleList();
        if (!m_pPluginPaths)
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }
    }

    m_pPluginPaths->AddTail(pPluginPathInfo);

    // load the plugins directly if the platform has been initialized
    // otherwise, the plugins will be loaded at Init()
    if (m_bInitialized)
    {
        IHXPluginHandler3* pPluginHandler3 = NULL;

        rc = m_pPluginHandlerUnkown->QueryInterface(IID_IHXPluginHandler3, (void**)&pPluginHandler3);
        if (HXR_OK == rc)
        {
            rc = pPluginHandler3->AddPluginMountPoint(pszName, TARVER_MAJOR_VERSION, 
                                                      TARVER_MINOR_VERSION, pPathBuffer);
        }
        HX_RELEASE(pPluginHandler3);
    }

  exit:

    if (HXR_OK != rc)
    {
        HX_DELETE(pPluginPathInfo);
    }
    HX_RELEASE(pPathBuffer);

    return rc;
}

STDMETHODIMP
CHXMediaPlatform::Init(IUnknown* pContext)
{
    HX_RESULT           rc = HXR_OK;
    IHXPluginDatabase*  pPluginDatabase = NULL;
    IHXPlugin2Handler*  pPlugin2Handler = NULL;
    IHXPluginHandler3*  pPluginHandler3 = NULL;

    if (m_bInitialized)
    {
        return HXR_FAILED;
    }

    m_pExtContext = pContext;
    HX_ADDREF(m_pExtContext);

    rc = InitExtendableServices();

    if (SUCCEEDED(rc) && !m_pParent)
    {
        // prepare preferences, scheduler etc
        rc = InitBasicServices();
        if (HXR_OK != rc)
        {
            goto exit;
        }

        // create, aggregate and initialize the plugin handler
        if (!m_pPluginHandlerUnkown)
        {
#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
            m_pPluginHandlerUnkown = (IUnknown*)(IHXPlugin2Handler*)new BaseHandler();
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
            m_pPluginHandlerUnkown = (IUnknown*)(IHXPlugin2Handler*)new HXPluginManager();
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else
            m_pPluginHandlerUnkown = (IUnknown*)(IHXPlugin2Handler*)new Plugin2Handler();
#endif /* _STATICALLY_LINKED */

            m_pPluginHandlerUnkown->AddRef();

            /*  XXX HP there is some issue with aggregation, need to investigate
            rc = Plugin2Handler::CreateInstance((IUnknown*)(IHXMediaPlatform*)this, (IUnknown**)&m_pPluginHandlerUnkown);
            if (HXR_OK != rc)
            {
            goto exit;
            }
            */
        }

#if !defined(_STATICALLY_LINKED) 
        // PluginDatabase manages the component plugins
        if (HXR_OK == m_pPluginHandlerUnkown->QueryInterface(IID_IHXPluginDatabase, 
                                                             (void**)&pPluginDatabase))
        {
            pPluginDatabase->AddPluginIndex(PLUGIN_COMPONENT_CLSID, kIndex_GUIDType, FALSE);
            pPluginDatabase->AddPluginIndex(PLUGIN_COMPONENT_NAME, kIndex_StringType, FALSE);
        }
        HX_RELEASE(pPluginDatabase);

        if (HXR_OK == m_pPluginHandlerUnkown->QueryInterface(IID_IHXPluginHandler3, (void**)&pPluginHandler3))
        {
            pPluginHandler3->RegisterContext((IUnknown*)(IHXMediaPlatform*)this);

            // tell the plugin handler where to look for additional plugins
            if (m_pPluginPaths)
            {
                CHXSimpleList::Iterator i;
                for (i = m_pPluginPaths->Begin(); i != m_pPluginPaths->End(); ++i)
                {
                    PluginPathInfo* pTempPluginPathInfo = (PluginPathInfo*)*i;
                    pPluginHandler3->AddPluginMountPoint(pTempPluginPathInfo->pszName, TARVER_MAJOR_VERSION,
                                                         TARVER_MINOR_VERSION, pTempPluginPathInfo->pPath);
                }
            }
        
            HX_RELEASE(pPluginHandler3);
        }

        // Load the plugins which have a property that
        // says to load them at media platform startup
        LoadStartupPlugins();
#else
	IHXPlugin2Handler* pPlugin2Handler = NULL;
	if (HXR_OK == m_pPluginHandlerUnkown->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPlugin2Handler))
	{
            pPlugin2Handler->Init((IUnknown*)(IHXMediaPlatform*)this);
        }
        HX_RELEASE(pPlugin2Handler);
#endif /* _STATICALLY_LINKED */	
    }

  exit:
    
    if (HXR_OK == rc)
    {
        m_bInitialized = TRUE;
    }

    return rc;
}

STDMETHODIMP
CHXMediaPlatform::Close(void)
{
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    HX_RELEASE(m_pHyperNavigate);

    if (m_pDefaultHyperNavigate)
    {
        m_pDefaultHyperNavigate->Stop();
        HX_RELEASE(m_pDefaultHyperNavigate);
    }
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */

#if defined(HELIX_FEATURE_NETINTERFACES)
    if (m_pNetInterfaces)
    {
        m_pNetInterfaces->Close();
        HX_RELEASE(m_pNetInterfaces);
    }
#endif /* HELIX_FEATURE_NETINTERFACES */

    if (m_pNetServices)
    {
        IHXNetServices2* pNetServices2 = NULL;
        if (HXR_OK == m_pNetServices->QueryInterface(IID_IHXNetServices2, (void**)&pNetServices2))
        {
            pNetServices2->Close();
            HX_RELEASE(pNetServices2);
        }
        HX_RELEASE(m_pNetServices);
    }

    HX_RELEASE(m_pRegistry);

#if defined(HELIX_FEATURE_REGISTRY)
    if (m_pClientRegistry)
    {
        m_pClientRegistry->Close();
        HX_RELEASE(m_pClientRegistry);
    }
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_OPTIMIZED_SCHEDULER)
    if (m_pOptimizedScheduler2)
    {
        m_pOptimizedScheduler2->StopScheduler();
        HX_RELEASE(m_pOptimizedScheduler2);
    }
    HX_RELEASE(m_pOptimizedScheduler);
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */

    if (m_pScheduler2)
    {
        m_pScheduler2->StopScheduler();
        HX_RELEASE(m_pScheduler2);
    }
    HX_RELEASE(m_pScheduler);

#if defined(HELIX_FEATURE_VIDEO)
    HX_RELEASE(m_pSiteEventHandler);
#endif

#if defined(HELIX_FEATURE_FILEPREFS)
    SPIHXPreferencesFile spFilePrefs = m_pPreferences;

    if (spFilePrefs)
    {
        spFilePrefs->Flush();
    }

#endif // HELIX_FEATURE_FILEPREFS

    if (m_pSingleLoadPlugins)
    {
        // Delete our list of single loaded plugins.
        CHXSimpleList::Iterator i;
        for(i = m_pSingleLoadPlugins->Begin(); i != m_pSingleLoadPlugins->End(); ++i)
        {
            IHXPlugin* pPlugin = (IHXPlugin*)(*i);
            HX_RELEASE(pPlugin);
        }
        HX_DELETE(m_pSingleLoadPlugins);
    }

    // Unload all plugins that were loaded
    // at media platform startup
    UnloadStartupPlugins();

    if (m_pPluginPaths)
    {
        CHXSimpleList::Iterator i;
        for (i = m_pPluginPaths->Begin(); i != m_pPluginPaths->End(); ++i)
        {
            PluginPathInfo* pTempPluginPathInfo = (PluginPathInfo*)*i;
            HX_DELETE(pTempPluginPathInfo);
        }
        HX_DELETE(m_pPluginPaths);
    }

    if (m_pPluginHandlerUnkown)
    {
        IHXPlugin2Handler* pPlugin2Handler = NULL;
        if (HXR_OK == m_pPluginHandlerUnkown->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPlugin2Handler))
        {
            pPlugin2Handler->Close();
            HX_RELEASE(pPlugin2Handler);
        }
        HX_RELEASE(m_pPluginHandlerUnkown);
    }

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pParent);

    if (m_pKicker)
    {
        m_pKicker->Close();
        HX_RELEASE(m_pKicker);
    }

    // Extended context must be released last as it may have overriden 
    // services used by pther platform objects and it may also have 
    // aggregated itself into the platform.
    // In such case, releasing of the external context will result in its
    // destruction and thus any objects that may use its service must
    // be released/closed first.
    HX_RELEASE(m_pExtCCF);
    HX_RELEASE(m_pExtContext);

    m_bInitialized = FALSE;

    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatform::Reset(IUnknown* pContext, HXBOOL bPlatformOnly)
{
    HX_RESULT                   rc = HXR_OK;
    IHXBuffer*                  pPrefKey = NULL;
    IHXPreferences2*            pPref2 = NULL;
    IHXPreferences3*            pPref3 = NULL;
    IHXPreferenceEnumerator*    pPrefEnumerator = NULL;
    IHXPluginEnumerator*        pPluginEnumerator = NULL;

    if (!bPlatformOnly && m_pPluginHandlerUnkown)
    {
        if (HXR_OK == m_pPluginHandlerUnkown->QueryInterface(IID_IHXPluginEnumerator,
                                                             (void**)&pPluginEnumerator))
        {
            UINT32      ulIndex = 0;
            UINT32      ulNumOfPlugins = pPluginEnumerator->GetNumOfPlugins();
            IUnknown*   pPlugin = NULL;
            IHXPlugin2* pPlugin2 = NULL;

            for(ulIndex = 0; ulIndex < ulNumOfPlugins; ulIndex++)
            {
                if (SUCCEEDED(pPluginEnumerator->GetPlugin(ulIndex, pPlugin)))
                {
                    if (SUCCEEDED(pPlugin->QueryInterface(IID_IHXPlugin2, (void**)&pPlugin2)))
                    {
                        pPlugin2->Reset();
                    }
                    HX_RELEASE(pPlugin2);
                }
                HX_RELEASE(pPlugin);
            }
        }
        HX_RELEASE(pPluginEnumerator);   
    }

    // We reset the m_pPreferences if it's already initialized via Init()
    //
    // On the other hand, Reset() can be called without Init(), this can
    // happen when the app simply wants to reset the media platform
    // during its un-installation
    if (!m_pPreferences)
    {       
        if (!m_pExtContext ||
            HXR_OK != m_pExtContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences))
        {
            InitDefaultPreferences();
        }
    }

    // remove all the entries within m_pPreferences
    if (m_pPreferences &&
        HXR_OK == m_pPreferences->QueryInterface(IID_IHXPreferences2, (void**)&pPref2) &&
        HXR_OK == m_pPreferences->QueryInterface(IID_IHXPreferences3, (void**)&pPref3) &&
        HXR_OK == pPref2->GetPreferenceEnumerator(pPrefEnumerator))
    {
        while (HXR_OK == pPrefEnumerator->GetPrefKey(0, pPrefKey))
        {
            pPref3->DeletePref((const char*)pPrefKey->GetBuffer());
            HX_RELEASE(pPrefKey);
        }
        HX_RELEASE(pPrefEnumerator);
        HX_RELEASE(pPref3);
        HX_RELEASE(pPref2);
    }

    Close();

    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatform::Purge(void)
{
    // Purge is not supported on child media platform context
    if (m_pParent)
    {
        return m_pParent->Purge();
    }

#if !defined(_STATICALLY_LINKED) && defined(HELIX_FEATURE_PLUGINHANDLER2)
    if (m_pPluginHandlerUnkown)
    {
        // shortcut, UnloadDeadDLLs() needs to be part of the interface 
        ((Plugin2Handler*)(IHXPlugin2Handler*)m_pPluginHandlerUnkown)->UnloadDeadDLLs();
    }
    
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* !_STATICALLY_LINKED && HELIX_FEATURE_PLUGINHANDLER2 */
}

STDMETHODIMP
CHXMediaPlatform::CreateChildContext(IHXMediaPlatform** ppChildContext)
{
    CHXMediaPlatform* pNewMedPltfm = new CHXMediaPlatform(this, m_pRoot);
    if (!pNewMedPltfm)
    {
        return HXR_OUTOFMEMORY;
    }

    return pNewMedPltfm->QueryInterface(IID_IHXMediaPlatform, (void**)ppChildContext);
}

STDMETHODIMP
CHXMediaPlatform::CreateInstance(REFCLSID   rclsid,
                                 void**     ppUnknown)
{
    IUnknown* pUnk = NULL;
    HX_RESULT nResult = ObjectFromCLSIDPrivate( rclsid, pUnk, NULL, GetUnknown() );

    *ppUnknown = pUnk;
    return nResult;
}

STDMETHODIMP
CHXMediaPlatform::CreateInstanceAggregatable(REFCLSID       rclsid,
                                             REF(IUnknown*) ppUnknown,
                                             IUnknown*      pUnkOuter)
{
    return ObjectFromCLSIDPrivate( rclsid, ppUnknown, pUnkOuter, GetUnknown() );
}


HX_RESULT       
CHXMediaPlatform::InitBasicServices(void)
{
    HX_RESULT               rc = HXR_OK;

    if (!m_pMutex)
    {
        rc = CreateInstance(CLSID_IHXMutex, (void**)&m_pMutex);
        if (HXR_OK != rc)
        {
            goto exit;
        }
    }

    if (!m_pScheduler)
    {
        rc = CreateInstance(CLSID_IHXScheduler, (void**)&m_pScheduler);
        if (HXR_OK != rc)
        {
            goto exit;
        }

        if (HXR_OK == m_pScheduler->QueryInterface(IID_IHXScheduler2, (void**)&m_pScheduler2))
        {
            m_pScheduler2->SetMutex(m_pMutex);
            m_pScheduler2->SetInterrupt(TRUE);
            m_pScheduler2->StartScheduler();
        }
    }

#ifdef HELIX_FEATURE_OPTIMIZED_SCHEDULER
    if (!m_pOptimizedScheduler)
    {
        rc = CreateInstance(CLSID_IHXOptimizedScheduler, (void**)&m_pOptimizedScheduler);
        if (HXR_OK != rc)
        {
            goto exit;
        }

        if (HXR_OK == m_pOptimizedScheduler->QueryInterface(IID_IHXOptimizedScheduler2, 
                                                            (void**)&m_pOptimizedScheduler2))
        {
            m_pOptimizedScheduler2->StartScheduler();
        }
    }
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */

    if (!m_pPreferences)
    {
        rc = InitDefaultPreferences();
        if (HXR_OK != rc)
        {
            goto exit;
        }
    }

#if defined(HELIX_FEATURE_REGISTRY)
    if (!m_pRegistry)
    {
        m_pClientRegistry = new HXClientRegistry();
        if (!m_pClientRegistry)
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }

        m_pClientRegistry->AddRef();
        m_pClientRegistry->Init((IUnknown*)(IHXMediaPlatform*)this);
        m_pRegistry = m_pClientRegistry;
        m_pRegistry->AddRef();
    }
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_NETINTERFACES)
    if (!m_pNetInterfaces)
    {
        m_pNetInterfaces = new HXNetInterface((IUnknown*)(IHXMediaPlatform*)this);
        if (!m_pNetInterfaces)
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }

        m_pNetInterfaces->AddRef();
        m_pNetInterfaces->UpdateNetInterfaces(); 
    }
#endif /* HELIX_FEATURE_NETINTERFACES */

#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    if (!m_pHyperNavigate)
    {
        m_pDefaultHyperNavigate = new HXThreadHyperNavigate(); 
        if (!m_pDefaultHyperNavigate)
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }

        m_pDefaultHyperNavigate->AddRef();
        m_pDefaultHyperNavigate->Init((IUnknown*)(IHXMediaPlatform*)this);
#if !defined(_UNIX)
        //XXXgfw Right now UNIX uses forks to get threaded hypernave done. It
        //also allows programs to be executed that way which is not supported
        //by our threaded hypernav class. We need to look at extending that
        //class or maybe the UNIX TLC does not need that any more???
        m_pDefaultHyperNavigate->UseThread(TRUE);
#endif /* _UNIX */
        m_pDefaultHyperNavigate->QueryInterface(IID_IHXHyperNavigate, (void**)&m_pHyperNavigate);
    }
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */

exit:

    return rc;
}

HX_RESULT       
CHXMediaPlatform::LoadStartupPlugins(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pPluginHandlerUnkown)
    {
        // Get the IHXPluginHandler3 interface
        IHXPluginHandler3* pHandler3 = NULL;
        retVal = m_pPluginHandlerUnkown->QueryInterface(IID_IHXPluginHandler3, (void**) &pHandler3);
        if (SUCCEEDED(retVal))
        {
            // Get a plugin enumerator for PLUGIN_LOAD_AT_STARTUP = PLUGIN_MEDIA_PLATFORM.
            // This indicates that they want to be loaded when the media platform starts up.
            IHXPluginSearchEnumerator* pEnum = NULL;
            retVal = pHandler3->FindGroupOfPluginsUsingStrings(PLUGIN_LOAD_AT_STARTUP,
                                                               PLUGIN_MEDIA_PLATFORM,
                                                               NULL, NULL, NULL, NULL, pEnum);
            if (SUCCEEDED(retVal))
            {
                // Get the number of load-at-startup plugins we found
                UINT32 ulNumPlugins = pEnum->GetNumPlugins();
                // Do we have any load-at-startup plugins? (It's OK if we don't)
                if (ulNumPlugins)
                {
                    // Create the list of load-at-startup plugins if we don't already have it
                    if (!m_pLoadAtStartupPlugins)
                    {
                        m_pLoadAtStartupPlugins = new CHXSimpleList();
                    }
                    if (m_pLoadAtStartupPlugins)
                    {
                        // Loop through all the load-at-startup plugins
                        for (UINT32 i = 0; i < ulNumPlugins && SUCCEEDED(retVal); i++)
                        {
                            // Get the i-th load-at-startup plugin from the enumerator
                            IUnknown* pUnk = NULL;
                            retVal = pEnum->GetPluginAt(i, pUnk, NULL);
                            if (SUCCEEDED(retVal))
                            {
                                // Get the IHXPlugin interface
                                IHXPlugin* pPlugin = NULL;
                                retVal = pUnk->QueryInterface(IID_IHXPlugin, (void**) &pPlugin);
                                if (SUCCEEDED(retVal))
                                {
                                    // Call InitPlugin on this plugin
                                    retVal = pPlugin->InitPlugin((IUnknown*)(IHXMediaPlatform*) this);
                                    if (SUCCEEDED(retVal))
                                    {
                                        // Add the list's ref on the plugin
                                        pUnk->AddRef();
                                        // Add this plugin to the list
                                        m_pLoadAtStartupPlugins->AddTail((void*) pUnk);
                                    }
                                }
                                HX_RELEASE(pPlugin);
                            }
                            HX_RELEASE(pUnk);
                        }
                    }
                    else
                    {
                        retVal = HXR_OUTOFMEMORY;
                    }
                }
            }
            else
            {
                // There are no load-at-startup plugins. This is not an error,
                // so clear the return value
                retVal = HXR_OK;
            }
            HX_RELEASE(pEnum);
        }
        HX_RELEASE(pHandler3);
    }

    return retVal;
}

void
CHXMediaPlatform::UnloadStartupPlugins(void)
{
    // Release all the load-at-startup plugins
    if (m_pLoadAtStartupPlugins)
    {
        while (m_pLoadAtStartupPlugins->GetCount() > 0)
        {
            IUnknown* pUnk = (IUnknown*) m_pLoadAtStartupPlugins->RemoveHead();
            HX_RELEASE(pUnk);
        }
    }
    HX_DELETE(m_pLoadAtStartupPlugins);
}

HX_RESULT       
CHXMediaPlatform::InitExtendableServices(void)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pExtContext)
    {
        if (HXR_OK != m_pExtContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences))
        {
            HX_RELEASE(m_pPreferences);
        }

        HX_RELEASE(m_pMutex);
        if (HXR_OK == m_pExtContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler))
        {   
            // If scheduler is overwridden, it must supply the mutex to be
            // used as the platform mutex.
            IHXScheduler2* pScheduler2 = NULL;
            retVal = m_pScheduler->QueryInterface(IID_IHXScheduler2, (void**)&pScheduler2);
            if (SUCCEEDED(retVal))
            {
                retVal = pScheduler2->GetMutex(m_pMutex);
            }
            HX_RELEASE(pScheduler2);
        }
        else
        {
            HX_RELEASE(m_pScheduler);
        }

        if (HXR_OK != m_pExtContext->QueryInterface(IID_IHXOptimizedScheduler, (void**)&m_pOptimizedScheduler))
        {
            HX_RELEASE(m_pOptimizedScheduler);
        }

        if (HXR_OK != m_pExtContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pExtCCF))
        {
            HX_RELEASE(m_pExtCCF);
        }

        if (HXR_OK != m_pExtContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
        {
            HX_RELEASE(m_pRegistry);
        }

        if (HXR_OK != m_pExtContext->QueryInterface(IID_IHXHyperNavigate, (void**)&m_pHyperNavigate))
        {
            HX_RELEASE(m_pHyperNavigate);
        }
    }
    
    return retVal;
}    

HX_RESULT                   
CHXMediaPlatform::InitDefaultPreferences(void)
{
    HX_RESULT           rc = HXR_OK;
    IHXPreferences3*    pPref3 = NULL;

#if defined(HELIX_FEATURE_PREFERENCES) && !defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
    rc = CreateInstance(CLSID_IHXPreferences, (void**)&m_pPreferences);
    if (HXR_OK == rc &&
        HXR_OK == m_pPreferences->QueryInterface(IID_IHXPreferences3, (void**)&pPref3))
    {
        // Assemble registry's key for the preferences 
        char* pCompanyName = new char[strlen(HXVER_COMMUNITY) + 1];
        strcpy(pCompanyName, HXVER_COMMUNITY); /* Flawfinder: ignore */

        char* pProductName = new char[strlen(HXVER_SDK_PRODUCT) + 1];
        strcpy(pProductName, HXVER_SDK_PRODUCT); /* Flawfinder: ignore */

        char * pComa = HXFindChar(pCompanyName, ',');
        (pComa) ? (*pComa = 0) : 0;

        pComa = HXFindChar(pProductName, ',');
        (pComa) ? (*pComa = 0) : 0;

        rc = pPref3->Open((const char*) pCompanyName, 
                          (const char*) pProductName, 
                          TARVER_MAJOR_VERSION, 
                          TARVER_MINOR_VERSION);

        HX_VECTOR_DELETE(pCompanyName);
        HX_VECTOR_DELETE(pProductName);
    }

    HX_RELEASE(pPref3);
#endif

    return rc;
}

HX_RESULT
CHXMediaPlatform::CheckAndQueryInterface(HX_MP_COM_TYPE type, REFIID riid, void** ppvObj)
{
    HX_RESULT   rc = HXR_NOTIMPL;

    if (HX_MP_COM_INSTRINSIC == type)
    {
        if (m_pPluginHandlerUnkown &&
            HXR_OK == m_pPluginHandlerUnkown->QueryInterface(riid, ppvObj))
        {
            rc = HXR_OK;
        }
        else if ((IsEqualIID(riid, IID_IHXPreferences)  ||
                  IsEqualIID(riid, IID_IHXPreferences2) ||
                  IsEqualIID(riid, IID_IHXPreferences3)))
        {
            rc = m_pPreferences ? m_pPreferences->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
        }
        else if (IsEqualIID(riid, IID_IHXMutex))
        {
            rc = m_pMutex ? m_pMutex->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
        }
        else if ((IsEqualIID(riid, IID_IHXScheduler) ||
                  IsEqualIID(riid, IID_IHXScheduler2)))
        {
            rc = m_pScheduler ? m_pScheduler->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
        }
        else if (IsEqualIID(riid, IID_IHXMediaPlatformKicker))
        {
            rc = m_pKicker ? m_pKicker->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
        }
        else if (IsEqualIID(riid, IID_IHXHyperNavigate) ||
                 IsEqualIID(riid, IID_IHXHyperNavigate2))
        {
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
            rc = m_pHyperNavigate ? m_pHyperNavigate->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */
        }
        else if (IsEqualIID(riid, IID_IHXOptimizedScheduler))
        {
#if defined(HELIX_FEATURE_OPTIMIZED_SCHEDULER)
            rc = m_pOptimizedScheduler ? m_pOptimizedScheduler->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */
        }
        else if (IsEqualIID(riid, IID_IHXRegistry))
        {
#if defined(HELIX_FEATURE_REGISTRY)
            rc = m_pRegistry ? m_pRegistry->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
#endif /* HELIX_FEATURE_REGISTRY */
        }
        else if (IsEqualIID(riid, IID_IHXNetInterfaces))
        {
#if defined(HELIX_FEATURE_NETINTERFACES)
            rc = m_pNetInterfaces ? m_pNetInterfaces->QueryInterface(riid, ppvObj) : HXR_NOTIMPL;
#endif /* HELIX_FEATURE_NETINTERFACES */
        }
        else
        {
            rc = HXR_NOT_SUPPORTED;
        }
    }
    else if (HX_MP_COM_REGULAR == type)
    {
        if (m_pExtContext && 
            (HXR_OK == m_pExtContext->QueryInterface(riid, ppvObj)))
        {
            rc = HXR_OK;
        }

        if ((rc != HXR_OK) && !m_pParent)
        {
            // Regular Root services are looked-up last
#if defined(HELIX_FEATURE_NETSERVICES)
            if (IsEqualIID(riid, IID_IHXNetServices)
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
                || IsEqualIID(riid, IID_IHXNetworkServices)
#endif
                )
            {
                if( !m_pNetServices )
                {
                    rc = CreateInstance(CLSID_IHXNetServices, (void**)&m_pNetServices);
                }

                if( m_pNetServices )
                {
                    rc = m_pNetServices->QueryInterface(riid, ppvObj);
                }
            }
#endif /* HELIX_FEATURE_NETSERVICES */
#if defined(HELIX_FEATURE_VIDEO)
            if( IsEqualIID(riid, IID_IHXSiteEventHandler) )
            {
                if( !m_pSiteEventHandler )
                {
                    rc = CreateInstance(IID_IHXSiteEventHandler,
                                        (void**) &m_pSiteEventHandler);
                }
                if( m_pSiteEventHandler )
                {
                    rc = m_pSiteEventHandler->QueryInterface(riid, ppvObj );
                }
            }
#endif	// HELIX_FEATURE_VIDEO
        }
    }

    if ((rc != HXR_OK) && m_pParent)
    {
        rc = m_pParent->CheckAndQueryInterface(type, riid, ppvObj);
    } 

    return rc;
}

// IHXObjectManagerPrivate methods
STDMETHODIMP
CHXMediaPlatform::ObjectFromCLSIDPrivate(THIS_ REFCLSID clsid, REF(IUnknown *)pObject, 
                                         IUnknown *pUnkOuter, IUnknown* pContext )
{
    HX_RESULT nResult = CreateIntrinsicType(clsid, pObject, pUnkOuter);

    if (!SUCCEEDED(nResult))
    {
        nResult = CreateGeneralType(clsid, pObject, pUnkOuter, pContext);
    }

    if (SUCCEEDED(nResult))
    {
        SPIHXContextUser spUser(pObject);
        if (spUser)
        {
            spUser->RegisterContext(pContext);
        }
    }

    return nResult;
}

HX_RESULT
CHXMediaPlatform::CreateIntrinsicType( REFCLSID rclsid, REF(IUnknown*) pUnknown, IUnknown* pOuter )
{
    HX_RESULT rc = HXR_OUTOFMEMORY;
    pUnknown = NULL;

    HXBOOL      bAddRef = TRUE;

    if (IsEqualCLSID(rclsid, CLSID_IHXBuffer))
    {
        pUnknown = (IUnknown*)(IHXBuffer*)(new CHXBuffer());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXTimeStampedBuffer))
    {
        pUnknown = (IUnknown*)(IHXTimeStampedBuffer*)(new CHXTimeStampedBuffer());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues))
    {
        pUnknown = (IUnknown*)(IHXValues*)(new CHXHeader());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues2))
    {
        pUnknown = (IUnknown*)(IHXValues2*)(new CHXHeader());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXList))
    {
        pUnknown = (IUnknown*)(IHXList*)(new CHXList(NULL));
    }
///////////////////////////////////////////////////////////////////////////////
    else if (IsEqualCLSID(rclsid, CLSID_IHXPacket))
    {
        pUnknown = (IUnknown*)(IHXPacket*)(new CHXPacket());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXRTPPacket))
    {
        pUnknown = (IUnknown*)(IHXPacket*)(new CHXRTPPacket());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXMultiPayloadPacket))
    {
        pUnknown = (IUnknown*)(IHXPacket*)(new CHXMultiPayloadPacket());
    }
    else if(IsEqualCLSID(rclsid, CLSID_IHXRequest))
    {
        pUnknown = (IUnknown*)(IHXRequest*)(new CHXRequest());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXThread))
    {
        pUnknown = (IUnknown*)(IHXThread*)(new CHXThread());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXEvent))
    {
        pUnknown = (IUnknown*)(IHXEvent*)(new CHelixEvent());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXMutex))
    {
        pUnknown = (IUnknown*)(IHXMutex*)(new CHXMutex());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXAsyncTimer))
    {
        pUnknown = (IUnknown*)(IHXAsyncTimer*)(new CHXAsyncTimer((IUnknown*)(IHXMediaPlatform*)this));
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXScheduler))
    {
        CHXMediaPlatformScheduler* pScheduler = new CHXMediaPlatformScheduler();
        if (pScheduler)
        {
            pScheduler->Init((IUnknown*)(IHXMediaPlatform*)this);

            if (!m_pKicker)
            {
                m_pKicker = new CHXMediaPlatformKicker();
                if (m_pKicker)
                {
                    m_pKicker->AddRef();
                    m_pKicker->RegisterContext((IUnknown*)(IHXMediaPlatform*)this);
                }
            }

            if (m_pKicker)
            {
                pScheduler->AttachKicker(m_pKicker);
            }

            pUnknown = (IUnknown*)(IHXScheduler*)(pScheduler);
        }
    }
#if defined(HELIX_FEATURE_FILE_RECOGNIZER)
    else if(IsEqualCLSID(rclsid, CLSID_IHXFileRecognizer)) 
    { 
       pUnknown = (IUnknown*)(IHXFileRecognizer*)(new CHXFileRecognizer((IUnknown*)(IHXMediaPlatform*)this)); 
    } 
#endif /* HELIX_FEATURE_FILE_RECOGNIZER */ 
#if defined(HELIX_FEATURE_OPTIMIZED_SCHEDULER)
    else if (IsEqualCLSID(rclsid, CLSID_IHXOptimizedScheduler))
    {
        pUnknown = (IUnknown*)(IHXOptimizedScheduler*)
            (new HXOptimizedScheduler((IUnknown*)(IHXMediaPlatform*)this)); 
    }
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */
    else if(IsEqualCLSID(rclsid, CLSID_IHXKeyValueList))
    {
        pUnknown = (IUnknown*)(IHXKeyValueList*)(new CKeyValueList());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXDllAccess))
    {
        pUnknown = (IUnknown*)(IHXDllAccess*)(new DLLAccessServer());
    }
#if defined(HELIX_FEATURE_PREFERENCES) && !defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
    else if (IsEqualCLSID(rclsid, CLSID_IHXPreferences))
    {
#if defined(HELIX_FEATURE_LITEPREFS)
        pUnknown = (IUnknown*)(IHXPreferences*) CHXLitePrefs::CreateObject();
#elif defined(HELIX_FEATURE_FILEPREFS)
        CHXPreferences::CreateInstance( pOuter, &pUnknown );
        bAddRef = FALSE;
#else
        pUnknown = (IUnknown*)(IHXPreferences*)(new HXPreferences());
#endif  // HELIX_FEATURE_LITEPREFS
    }
#endif /* HELIX_FEATURE_PREFERENCES && !HELIX_FEATURE_NO_INTERNAL_PREFS */
#if defined(HELIX_FEATURE_REGISTRY)
    else if (IsEqualCLSID(rclsid, CLSID_IHXRegistry))
    {
        HXClientRegistry* pRegistry = new HXClientRegistry();
        if (pRegistry)
        {
            pRegistry->Init((IUnknown*)(IHXMediaPlatform*) this);
        }
        pUnknown = (IUnknown*)(IHXRegistry*) pRegistry;
    }
#endif /* HELIX_FEATURE_REGISTRY */
#if defined(HELIX_FEATURE_FIFOCACHE)
    else if(IsEqualCLSID(rclsid, CLSID_IHXFIFOCache))
    {
        pUnknown = (IUnknown*)(IHXFIFOCache*)(new HXFIFOCache((IUnknown*)(IHXMediaPlatform*)this));
    }
#endif /* HELIX_FEATURE_FIFOCACHE */
#if defined(HELIX_FEATURE_REGION) && (_WIN32)
    else if(IsEqualCLSID(rclsid, CLSID_IHXRegion))
    {
        pUnknown = (IUnknown*)(IHXRegion*)(new HXRegion());
    }
#endif /* HELIX_FEATURE_REGION */
#if defined(HELIX_FEATURE_FRAGMENTBUFFER)
    else if (IsEqualCLSID(rclsid, CLSID_IHXFragmentedBuffer))
    {
        pUnknown = (IUnknown*)(IHXFragmentedBuffer*)(new CHXFragmentedBuffer((IUnknown*)(IHXMediaPlatform*)this));
    }
#endif /* HELIX_FEATURE_FRAGMENTBUFFER */
#if defined(HELIX_FEATURE_FILESYSTEMMGR)
    else if (IsEqualCLSID(rclsid, CLSID_IHXFileSystemManager))
    {
        pUnknown = (IUnknown*)(IHXFileSystemManager*)(new HXFileSystemManager((IUnknown*)(IHXMediaPlatform*)this));
    }
#endif /* HELIX_FEATURE_FILESYSTEMMGR */
#if defined(HELIX_FEATURE_VIDEO) && defined (HELIX_FEATURE_MISU)
    else if(IsEqualCLSID(rclsid, CLSID_IHXMultiInstanceSiteUserSupplier))
    {
        pUnknown = (IUnknown*)(IHXMultiInstanceSiteUserSupplier*)(new CHXMultiInstanceSiteUserSupplier());
    }
#endif /* HELIX_FEATURE_VIDEO && HELIX_FEATURE_MISU */
#ifdef HELIX_FEATURE_HTTP_SERVICE
    else if (IsEqualCLSID(rclsid, CLSID_IHXHttp))
    {
        pUnknown = (IUnknown*)(IHXHttp*)(new CHXHttp());
    }
#endif /* HELIX_FEATURE_HTTP_SERVICE */
#if defined(HELIX_FEATURE_XMLPARSER)
    else if (IsEqualCLSID(rclsid, CLSID_IHXXMLParser))
    {
        pUnknown = (IUnknown*)(IHXXMLParser*)(new HXXMLParser( (IUnknown*)(IHXClientEngine*)this ));
    }
#endif /* HELIX_FEATURE_XMLPARSER */
    else if (IsEqualCLSID(rclsid, CLSID_IHXPaceMaker))
    {
        pUnknown = (IUnknown*)(IHXPaceMaker*)(new CVideoPaceMaker((IUnknown*)(IHXMediaPlatform*)this));
    }
///////////////////////////////////////////////////////////////////////////////
#if !defined(_STATICALLY_LINKED)
    else if(IsEqualCLSID(rclsid, CLSID_IHXPluginGroupEnumerator))
    {
        pUnknown = (IUnknown*)(IHXPluginGroupEnumerator*)
            (new CHXPlugin2GroupEnumerator((IHXPlugin2Handler*)m_pPluginHandlerUnkown));
    }
#endif /* _STATICALLY_LINKED */
    else
    {
        rc = HXR_NOT_SUPPORTED;
    }

    if( pUnknown )
    {
        rc = HXR_OK;
        if (bAddRef)
        {
            pUnknown->AddRef();
        }
    }

    return rc;
}

HX_RESULT
CHXMediaPlatform::CreateGeneralType(REFCLSID clsid, REF(IUnknown*) pObject, 
                                    IUnknown* pUnkOuter, IUnknown* pContext)
{
    HX_RESULT nResult = HXR_NOT_SUPPORTED;

    if (m_pExtCCF)
    {
        if (pUnkOuter)
        {
            nResult = m_pExtCCF->CreateInstanceAggregatable(clsid, pObject, pUnkOuter);
        }
        else
        {
            nResult = m_pExtCCF->CreateInstance(clsid, (void**)&pObject);
        }
    }

    if (!SUCCEEDED(nResult))
    {
        if (m_pParent)
        {
            nResult = m_pParent->CreateGeneralType(clsid,
                                                   pObject,
                                                   pUnkOuter,
                                                   pContext);
        }
        else
        {
            HX_ASSERT(m_pRoot == this);

            nResult = CreateInstanceFromPluginHandler(clsid, 
                                                      (void**) &pObject, 
                                                      pUnkOuter, 
                                                      pContext);
        }
    }

    return nResult;
}

HX_RESULT
CHXMediaPlatform::CreateInstanceFromPluginHandler(REFCLSID rclsid, void** ppUnknown, IUnknown *pUnkOuter, IUnknown* pContext)
{
    HX_RESULT           rc = HXR_FAILED;
    IUnknown*           pUnknown = NULL;

    SPIHXPluginHandler3 spPluginHandler3(m_pPluginHandlerUnkown);
    if(spPluginHandler3.IsValid())
    {
        HXAutoLock lock( &m_PluginHandlerLockKey );
        rc = spPluginHandler3->FindImplementationFromClassID(rclsid, pUnknown, pUnkOuter, pContext);
    }
    *ppUnknown = pUnknown;

    SPIHXPlugin spPlugin(pUnknown);
    if(spPlugin.IsValid())
    {
        // check whether the plugin is single-loaded, if so, we will keep
        // it loaded by AddRef() it.
        const char*     pszDescription = NULL;
        const char*     pszCopyright = NULL;
        const char*     pszMoreInfoUrl = NULL;
        ULONG32         ulVersionNumber = 0;
        HXBOOL          nload_multiple = 0;

        if (HXR_OK == spPlugin->GetPluginInfo(nload_multiple, pszDescription, pszCopyright, pszMoreInfoUrl, ulVersionNumber))
        {
            if (0 == nload_multiple)
            {
                if (!m_pSingleLoadPlugins)
                {
                    m_pSingleLoadPlugins = new CHXSimpleList();
                }

                if (m_pSingleLoadPlugins)
                {
                    spPlugin->AddRef();
                    m_pSingleLoadPlugins->AddTail((void*)spPlugin.Ptr());
                }                   
            }
        }
    }

    return rc;
}

STDMETHODIMP
CHXMediaPlatform::UnloadPluginPrivate(REFCLSID clsid)
{
    HX_RESULT           rc = HXR_FAILED;

    SPIHXPluginHandler3 spPluginHandler3(m_pPluginHandlerUnkown);
    if(spPluginHandler3.IsValid())
    {
        HXAutoLock lock( &m_PluginHandlerLockKey );
        rc = spPluginHandler3->UnloadPluginFromClassID( clsid );
    }

    return rc;
}
