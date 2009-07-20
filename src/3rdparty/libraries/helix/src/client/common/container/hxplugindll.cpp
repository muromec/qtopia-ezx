/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplugindll.cpp,v 1.9 2007/07/06 21:57:56 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/stdio.h"

#include "hxresult.h"
#include "hxassert.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxplugn.h" //IHXPluginFactory
#include "ihxpckts.h" // IHXBuffer
#include "hxstring.h"
#include "hxslist.h"
#include "pathutil.h"

#include "dllacces.h"
#include "dllpath.h"

#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_STATICALLY_LINKED)
#include "staticff.h"
#endif

#include "hxpluginarchive.h"
#include "hxplugin.h"
#include "hxplugindll.h"



BEGIN_INTERFACE_LIST_NOCREATE(HXPluginDLL)
END_INTERFACE_LIST

HXPluginDLL::HXPluginDLL( IUnknown* pContext,
                         const char* pszFileName, 
                         const char * pszMountPoint)
: m_fpCreateInstance(0)
, m_fpShutdown(0)
, m_fCanUnload(0)
, m_strMountPoint(pszMountPoint)
, m_strFileName(pszFileName)
, m_pluginCount(0)
, m_bHasFactory(FALSE)
, m_bLoaded(FALSE)
, m_pDLLAccess(0)
, m_pContext(0)
, m_pClassFactory(0)
{
    HXLOGL3(HXLOG_CORE, "HXPluginDLL::HXPluginDLL()");
    Init(pContext);
    
}

// common ctor init
void HXPluginDLL::Init(IUnknown* pContext)
{
    HX_ASSERT(pContext);
    m_pContext = pContext;
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pClassFactory);
    m_pDLLAccess = new DLLAccess();
}
 
// deserializing ctor
HXPluginDLL::HXPluginDLL( IUnknown* pContext,
                         const char* pszMountPoint, 
                         HXPluginArchiveReader& ar)
: m_fpCreateInstance(0)
, m_fpShutdown(0)
, m_fCanUnload(0)
, m_strMountPoint( pszMountPoint )
, m_pluginCount(0)
, m_bHasFactory(FALSE)
, m_bLoaded(FALSE)
, m_pDLLAccess(0)
, m_pContext(0)
{
    HXLOGL3(HXLOG_CORE, "HXPluginDLL::HXPluginDLL()");
    Init(pContext);

    HX_ASSERT(!ar.AtEnd());

    ar.Read(m_strFileName);
    ar.Read(m_bHasFactory);
    ar.Read(m_pluginCount);
    HX_ASSERT(m_pluginCount > 0);
    
    HXLOGL3(HXLOG_CORE, "HXPluginDLL::HXPluginDLL(): '%s' - %u plugins", (const char*)m_strFileName, m_pluginCount);
    for(UINT16 idx = 0; idx < m_pluginCount; ++idx)
    {
	HXPlugin* pPlugin = new HXPlugin(m_pContext, ar);
        if(pPlugin)
        {
            pPlugin->AddRef();
	    HX_RESULT hr = pPlugin->Init(this, idx);
            if(SUCCEEDED(hr))
            {
                m_plugins.AddTail(pPlugin);
            }
        }
    }
    
}

// serialize object
void HXPluginDLL::Archive(HXPluginArchiveWriter& ar)
{
    ar.Write(UINT32(ARCHIVE_ID_PLUGIN_DLL));
    ar.Write(m_strFileName);
    ar.Write(m_bHasFactory);
    ar.Write(m_pluginCount);
  
    for(CHXSimpleList::Iterator iter = m_plugins.Begin(); iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;
        pPlugin->Archive(ar);
    }
    ar.Break();
}




HXPluginDLL::~HXPluginDLL()
{
    HXLOGL3(HXLOG_CORE, "HXPluginDLL::~HXPluginDLL()");

    // clear plugins
    CHXSimpleList::Iterator iter = m_plugins.Begin();
    for( ; iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlug = (HXPlugin*) *iter;
	pPlug->Release();
    }
    m_plugins.RemoveAll();

    // force DLL to unload. This must be called last, after the DLL, and and
    // code in it, is no longer needed.
    Unload(true);


    HX_DELETE(m_pDLLAccess);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
}



void HXPluginDLL::AddPlugins(CHXSimpleList& list)
{
    for(CHXSimpleList::Iterator iter = m_plugins.Begin(); iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;
        pPlugin->AddRef();
	list.AddTail(pPlugin);
    }
}

//
// Load() Helper
//
// Instanciate a plugin wrapper for each plugin this dll
// can instanciate. Called after Load().
//
HX_RESULT HXPluginDLL::CreatePlugins()
{
    HX_ASSERT(m_bLoaded);

    HXLOGL3(HXLOG_CORE, "HXPluginDLL::CreatePlugins()");

    HX_RESULT hr = HXR_FAIL;
    
    for(UINT16 idx = 0; idx < m_pluginCount; ++idx)
    {
	HXPlugin* pPlugin = new HXPlugin(m_pContext);
	if(!pPlugin)
	{
	    hr = HXR_OUTOFMEMORY;
            break;
	}

	pPlugin->AddRef();
 
        pPlugin->Init(this, idx);
     

	IUnknown* pUnk = NULL;
	if( SUCCEEDED(pPlugin->GetPlugin(pUnk)) )
	{
	    IHXComponentPlugin* pComponentPlugin = NULL;
	    if( SUCCEEDED( pUnk->QueryInterface( IID_IHXComponentPlugin, (void**) &pComponentPlugin ) ) )
	    {
		// We don't need this.
		HX_RELEASE( pPlugin );

		CreateComponentPlugins(pComponentPlugin );
		HX_RELEASE( pComponentPlugin );
	    }
	    else
	    {
		IHXPlugin* pIHXPlugin;
		if( SUCCEEDED( pUnk->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
		{
		    pPlugin->GetValuesFromDLL(pIHXPlugin);
		    m_plugins.AddTail(pPlugin);
		    pIHXPlugin->Release();
		}
	    }
	}
        else
        {
            // This plugin doesn't work.  Delete it.
	    HX_RELEASE( pPlugin );
	}
	
	HX_RELEASE( pUnk );
    }

    return hr;
}

//
// CreatePlugins() helper
//
void HXPluginDLL::CreateComponentPlugins( IHXComponentPlugin* pComponentPlugin )
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::LoadComponentPlugins()");

    IHXPlugin* pIHXPlugin = NULL;
    if( SUCCEEDED( pComponentPlugin->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
    {
        pIHXPlugin->InitPlugin(m_pContext);
	for( UINT32 index = 0; index < pComponentPlugin->GetNumComponents(); index++ )
	{
	    IHXValues* pVal = 0;
	    if( SUCCEEDED( pComponentPlugin->GetComponentInfoAtIndex( index, pVal ) ) )
	    {
		// create a new plugin object
		HXPlugin* pPlugin = new HXPlugin( m_pContext );
		HX_ASSERT( pPlugin );

		// Setup plugin object (index is set 0 for component plugins)
		pPlugin->AddRef();
		pPlugin->Init(this, 0);
                pPlugin->AddComponentInfo(pVal); 

		// Put in plugin list
		m_plugins.AddTail(pPlugin);

		HX_RELEASE(pVal);
	    }
	}

	HX_RELEASE (pIHXPlugin);
    }
}


HX_RESULT HXPluginDLL::Load()
{

    HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Load()");
    

    HX_ASSERT(m_pContext);
    HX_ASSERT(m_pClassFactory);


    IUnknown*	pInstance   = NULL;
    IHXPlugin* pPlugin = NULL;
    IHXPluginFactory* pIFactory = NULL;

    HX_ASSERT(!m_bLoaded);

    // dll full path
#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    // we're not statically linked so we must use the mount point path
    CHXString dllPath = HXPathUtil::CombinePath(m_strMountPoint, m_strFileName);
#else
    CHXString dllPath = m_strFileName;
#endif
    
    HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Load(): fullpath = '%s'", (const char*)dllPath);

    // load the DLL into memory
    HX_RESULT hr = HXR_FAIL;
    HX_ASSERT(dllPath.GetLength() > 0);
    int dllLoadResult = m_pDLLAccess->open(dllPath);
    if( dllLoadResult != DLLAccess::DLL_OK )
    {
        if ( DLLAccess::OUT_OF_MEMORY == dllLoadResult)
        {
            hr = HXR_OUTOFMEMORY;
        }
        HXLOGL2(HXLOG_CORE, "HXPluginDLL()::Load(): dll open failed");
	goto cleanup;
    }

    // HXCreateInstance is required
    m_fpCreateInstance = (FPCREATEINSTANCE) m_pDLLAccess->getSymbol(HXCREATEINSTANCESTR);
    if (NULL == m_fpCreateInstance)
    {
        HXLOGL2(HXLOG_CORE, "HXPluginDLL()::Load(): dll missing 'create instance'");
	goto cleanup;
    }

    // HXShutdown not required
    m_fpShutdown    = (FPSHUTDOWN) m_pDLLAccess->getSymbol(HXSHUTDOWNSTR);

    // CanUnload2 not required; CanUnload not used (deprecated)
    m_fCanUnload    = (FPSHUTDOWN) m_pDLLAccess->getSymbol("CanUnload2");

    // CreateInstance() must succeed
    hr = m_fpCreateInstance( &pInstance );
    if( FAILED(hr) )
    {
        HXLOGL2(HXLOG_CORE, "HXPluginDLL()::Load(): dll 'create instance' failed");
	goto cleanup;
    }

    
    // A valid plugin must expose IHXPlugin and/or IHXPluginFactory.
    if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPluginFactory, (void**) &pIFactory ) ) )
    {
	m_bHasFactory = TRUE;
	m_pluginCount = pIFactory->GetNumPlugins();
	HX_RELEASE( pIFactory );
    }
    else if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPlugin, (void**) &pPlugin ) ) )
    {
	m_bHasFactory = FALSE;
	m_pluginCount = 1;
	HX_RELEASE( pPlugin );
    }
    else
    {
        hr = HXR_FAIL;
	goto cleanup;
    }

    HX_RELEASE(pInstance);
    m_bLoaded = TRUE;

    // load plugins on first DLL load
    if( m_plugins.GetCount() == 0)
    {
        HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Load(): querying dll for plugins...");
        CreatePlugins();
    }

    hr = HXR_OK;

cleanup:

    HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Load(): result = 0x%08x", hr);
    return hr;
}


HX_RESULT HXPluginDLL::Unload(bool bForce)
{
    HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Unload(): is_loaded = %d; force = %d", m_bLoaded, bForce);
    
    HX_RESULT hr = HXR_OK;
    if (m_bLoaded)
    {
        hr = HXR_FAIL;
        HX_ASSERT(m_pDLLAccess);
	if (bForce || (m_fCanUnload && m_fCanUnload() == HXR_OK) )
	{
            HXLOGL3(HXLOG_CORE, "HXPluginDLL()::Unload(): unloading ('%s')", (const char*)m_strFileName);

	    if (m_fpShutdown)
	    {
		m_fpShutdown();
		m_fpShutdown = NULL;
	    }

            m_pDLLAccess->close();
	    m_bLoaded = FALSE;
	    hr = HXR_OK;
	}
    }
    return hr;
}

HXBOOL HXPluginDLL::IsLoaded()
{
    return m_bLoaded;
}

HX_RESULT HXPluginDLL::CreateInstance(IUnknown** ppUnk, UINT32 uIndex)
{
    HXLOGL3(HXLOG_CORE, "HXPluginDLL()::CreateInstance(): '%s': idx = %lu", (const char*)m_strFileName, uIndex);
    HX_ASSERT(m_bLoaded);
    HX_RESULT hr = HXR_FAIL;
    if (m_bLoaded)
    {
        if (!m_bHasFactory)
        {
	    hr = m_fpCreateInstance(ppUnk);
        }
        else
        {
            HX_ASSERT( uIndex < m_pluginCount);
	    if (uIndex < m_pluginCount)
            {
	        IUnknown*		pUnk;
	        IHXPluginFactory*	pPluginFactory;

	        m_fpCreateInstance(&pUnk);
                HX_ASSERT(pUnk);
	        hr = pUnk->QueryInterface(IID_IHXPluginFactory, (void**) &pPluginFactory);
                HX_ASSERT(pPluginFactory);
                if(SUCCEEDED(hr))
                {
	            hr = pPluginFactory->GetPlugin((UINT16)uIndex, ppUnk);
                    HX_RELEASE(pPluginFactory);
                }
                HX_RELEASE(pUnk);
            }
        }
    }
    
    return hr;
}





BEGIN_INTERFACE_LIST_NOCREATE(HXOtherDLL)
END_INTERFACE_LIST

// ctor
HXOtherDLL::HXOtherDLL(const char* pszFileName, const char* pszMountPoint)
: m_strFileName(pszFileName)
, m_strMountPoint(pszMountPoint)
{
}

// deserializing ctor
HXOtherDLL::HXOtherDLL(const char* pszMountPoint, HXPluginArchiveReader& ar)
: m_strMountPoint(pszMountPoint)
{
    HX_ASSERT(!ar.AtEnd());
    ar.Read(m_strFileName);
}

// serialize
void HXOtherDLL::Archive(HXPluginArchiveWriter& ar)
{
    ar.Write(UINT32(ARCHIVE_ID_OTHER_DLL));
    ar.Write(m_strFileName);
    ar.Break();
}





