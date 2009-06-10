/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpluginmanager.cpp,v 1.17 2009/03/04 00:42:59 girish2080 Exp $
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
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hxcom.h"
#include "hxccf.h"      // IHXCommonClassFactory
#include "ihxpckts.h"   // IHXBuffer
#include "pckunpck.h"
#include "hxplugn.h"    // IHXComponentPlugin
#include "hxprefs.h"	// IHXPreferences
#include "hxprefutil.h"
#include "findfile.h"
#include "chxpckts.h"   // CHXHeader
#include "dllacces.h"
#include "dllpath.h"
#include "pathutil.h"
#include "hxver.h"

#include "hlxclib/stdlib.h" //atoi
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_STATICALLY_LINKED)
#include "staticff.h"
#endif

#include "hxpluginarchive.h"
#include "hxplugindll.h"
#include "hxplugin.h"
#include "hxpluginmanager.h"

#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
#include "symbiannameonlyff.h"
#endif

#if defined (HELIX_FEATURE_PREFERENCES) && defined (HELIX_DEFINE_DLL_NAMESPACE)
#include "hxstrutl.h"
#endif //HELIX_FEATURE_PREFERENCES && HELIX_DEFINE_DLL_NAMESPACE

#if(0)
// helper
static bool IsRealNetworksPlugin(HXPlugin* pPlugin)
{
    bool isRNPlugin = false;

    HX_ASSERT(pPlugin);
    IHXValues* pval = 0;
    if (SUCCEEDED(pPlugin->GetPluginInfo(pval)))
    {
        IHXBuffer* pbuff = 0;
	if (SUCCEEDED(pval->GetPropertyCString(PLUGIN_DESCRIPTION2, pbuff)))
	{
	    isRNPlugin =  (0 != strstr((const char*)pbuff->GetBuffer(), "RealNetworks"));
            HX_RELEASE(pbuff);
        }
        HX_RELEASE(pval);
    }

    return isRNPlugin;
}
#endif


IMPLEMENT_COM_CREATE_FUNCS( HXPluginManager )

BEGIN_INTERFACE_LIST_NOCREATE( HXPluginManager )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPlugin2Handler )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginHandler3 )
END_INTERFACE_LIST

HXPluginManager::HXPluginManager() 
: m_pContext(0)
, m_pClassFactory(0)
{
}

HXPluginManager::~HXPluginManager()
{
    Close();
}

STDMETHODIMP HXPluginManager::Init(IUnknown* pContext)
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager::Init()");
    HX_RESULT result = HXR_FAIL;

    if( SUCCEEDED( result = RegisterContext( pContext ) ) )
    {
	result = ReadFromRegistry();
    }

    return result;
}


STDMETHODIMP_(ULONG32) HXPluginManager::GetNumOfPlugins2()
{
    return m_plugins.GetCount();
}


STDMETHODIMP
HXPluginManager::GetPluginInfo(UINT32 unIndex, REF(IHXValues*) /*OUT*/ pValues)
{
    return HXR_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
//
HX_RESULT HXPluginManager::ReloadPluginsNoPropagate()
{
    return ReloadPlugins();
}

///////////////////////////////////////////////////////////////////////////////
//
HX_RESULT HXPluginManager::ReloadPlugins()
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::ReloadPlugins()");

    HX_RESULT result = HXR_OK;

    // reload plugins for all mountpoints
    for(CHXMapStringToString::Iterator iter = m_mountPoints.Begin(); iter != m_mountPoints.End(); ++iter)
    {
	const char* pMountPoint = (const char*) *iter;
	if( FAILED( ReloadPlugins( pMountPoint ) ) )
	{
	    result = HXR_FAIL;
	}
    }

    return result;
}

/////////////////////////////////
//
// do we already have the given dll in our collection?
//
// we use case insensitive match in all OS/filesystem cases (even if technically inappropriate)
//
bool HXPluginManager::DoesDLLExist(const char* pszName, const char* pszMountPoint)
{
    CHXSimpleList::Iterator iter;
    for(iter = m_pluginDlls.Begin(); iter != m_pluginDlls.End(); ++iter)
    {
	HXPluginDLL* pLib = (HXPluginDLL*) *iter;

        if( !pLib->GetFileName().CompareNoCase(pszName) && !pLib->GetMountPoint().CompareNoCase(pszMountPoint) )
        {
            HXLOGL3(HXLOG_CORE, "HXPluginManager::DoesDLLExist(): plugin dll exists '%s'", (const char*)pLib->GetFileName());
            return true;
        }
    }

    for(iter = m_otherDlls.Begin(); iter != m_otherDlls.End(); ++iter)
    {
	HXOtherDLL* pLib = (HXOtherDLL*) *iter;
        if( !pLib->GetFileName().CompareNoCase(pszName) && !pLib->GetMountPoint().CompareNoCase(pszMountPoint) )
        {
            HXLOGL3(HXLOG_CORE, "HXPluginManager::DoesDLLExist(): other dll exists '%s'", (const char*)pLib->GetFileName());
            return true;
        }
    }
    return false;
}

HX_RESULT HXPluginManager::SaveToArchive(const char* pszArchiveFile)
{
    HXLOGL3(HXLOG_CORE, "SaveToArchive(): writing '%s' (%d plugin dlls; %d other dlls)", pszArchiveFile, m_pluginDlls.GetCount(), m_otherDlls.GetCount());
    HXPluginArchiveWriter ar;
    HX_RESULT hr = ar.Open(m_pContext, pszArchiveFile);
    if(SUCCEEDED(hr))
    {
        CHXSimpleList::Iterator iter;
        for(iter = m_pluginDlls.Begin(); iter != m_pluginDlls.End(); ++iter)
        {
            HXPluginDLL* pLib = (HXPluginDLL*) *iter;
            pLib->Archive(ar);
        }
        for(iter = m_otherDlls.Begin(); iter != m_otherDlls.End(); ++iter)
        {
            HXOtherDLL* pLib = (HXOtherDLL*) *iter;
            pLib->Archive(ar);
        }
    }
    ar.Close();
    return hr;
}

HX_RESULT HXPluginManager::LoadPluginDLLFromArchive(const char* pszMountPoint, HXPluginArchiveReader& ar)
{
    HXPluginDLL* pPluginDLL = new HXPluginDLL(m_pContext, pszMountPoint, ar);
    if( !pPluginDLL )
    {
        return HXR_OUTOFMEMORY;
    }
    pPluginDLL->AddRef();

    HX_ASSERT(!DoesDLLExist(pPluginDLL->GetFileName(), pPluginDLL->GetMountPoint()));

    // no need to load! that's the point of the archive
    HX_ASSERT(pPluginDLL->GetNumPlugins() > 0);

    m_pluginDlls.AddTail(pPluginDLL);

    return HXR_OK;
}

HX_RESULT HXPluginManager::LoadOtherDLLFromArchive(const char* pszMountPoint, HXPluginArchiveReader& ar)
{
    HXOtherDLL* pOtherDLL = new HXOtherDLL(pszMountPoint, ar);
    if( !pOtherDLL )
    {
        return HXR_OUTOFMEMORY;
    }

    pOtherDLL->AddRef();
    HX_ASSERT(!DoesDLLExist(pOtherDLL->GetFileName(), pOtherDLL->GetMountPoint()));
    m_otherDlls.AddTail(pOtherDLL);

    return HXR_OK;
}

//
// Try to re-construct plugin dll and associated plugin objects
// from saved archive for faster plugin discovery
//
// If you want to override this (i.e., after new dlls installed), delete the archive
//
HX_RESULT HXPluginManager::LoadFromArchive(const char* pszArchiveFile, const char* pszMountPoint)
{
    HXLOGL3(HXLOG_CORE, "LoadFromArchive(): looking for '%s'", pszArchiveFile);
    HXPluginArchiveReader ar;
    HX_RESULT hr = ar.Open(m_pContext, pszArchiveFile);
    if(SUCCEEDED(hr))
    {
        while(SUCCEEDED(hr) && !ar.AtEnd())
        {
            UINT32 type;
            ar.Read(type);
            switch(type)
            {
            case ARCHIVE_ID_PLUGIN_DLL:
                hr = LoadPluginDLLFromArchive(pszMountPoint,ar);
                break;
            case ARCHIVE_ID_OTHER_DLL:
                hr = LoadOtherDLLFromArchive(pszMountPoint, ar);
                break;
            default:
                hr = HXR_FAIL;
                break;
            }
        }
    
        ar.Close();

        HXLOGL3(HXLOG_CORE, "LoadFromArchive(): found %ld plugin dlls, %ld other dlls", m_pluginDlls.GetCount(), m_otherDlls.GetCount());
    }
    else
    {
        HXLOGL3(HXLOG_CORE, "LoadFromArchive(): archive missing");
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// 'query plugin dlls'
//
// load all plugin dlls and determine associated plugin attributes
//
// called when:
//
//   1) a mount point is added
//   2) a mount point is removed and re-added (e.g., dynamic re-load)
//
// does nothing if dlls for mount point have already been loaded and queried
//
HX_RESULT HXPluginManager::ReloadPluginsWithFindFile(
               const char* pMountPoint, CFindFile* pFileFinder,
               IHXBuffer* pPathBuffer, const char* pszPluginDir)
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::ReloadPluginsWithFindFile()");
  
    HX_RESULT hr = HXR_OK;

#if defined(HELIX_FEATURE_PREFERENCES)
    CHXString strArchiveFile;
    HXBOOL bCanWriteArchive = FALSE;
    IHXPreferences* pPrefs = 0;
    m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
    CHXString preference("PluginArchiveFileName");
#ifdef HELIX_DEFINE_DLL_NAMESPACE
    const char* const pNameSpace = STRINGIFY(HELIX_DEFINE_DLL_NAMESPACE);
    preference = pNameSpace + preference;
#endif //HELIX_DEFINE_DLL_NAMESPACE
    hr = ReadPrefCSTRING(pPrefs, preference, strArchiveFile);
    HX_RELEASE(pPrefs);
    if(SUCCEEDED(hr))
    {
        // first try to recreate dll and plugin state from archive
        hr = LoadFromArchive(strArchiveFile, pMountPoint); 
        bCanWriteArchive = TRUE;
    }
    if(HXR_OUTOFMEMORY == hr)
    {
        return hr;
    }
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
    
/* optimize dll loading so that startup time is reduced.
   if plugin archive reading is successful assume there is no
   other dll to be loaded. No need to search through
   all the dlls. We can still achieve a dynamic update by removing the plugin
   archive.
*/
    bool bWriteArchive = false;
#ifdef HELIX_CONFIG_OPTIMIZE_DLL_LOADING
    if (hr != HXR_OK)
    {
#endif
    // iterate files in this plugin directory
    hr = HXR_OK;
    const char* pszDllName = pFileFinder->FindFirst();
    for (; pszDllName; pszDllName = pFileFinder->FindNext())
    {
        if( DoesDLLExist(pszDllName, pMountPoint) )
        {
            // we already have info about this dll (perhaps from achive)
            continue;
        }

        // since we found a dll we don't know about yet we need to update the archive
        bWriteArchive = true;

        // create plugin dll wrapper, assuming this is a plugin dll
        HXPluginDLL* pPluginDll  = new HXPluginDLL(m_pContext, pszDllName, pMountPoint);
        if(!pPluginDll)
        {
            // oom
	    hr = HXR_OUTOFMEMORY;
            break;
        }
        pPluginDll->AddRef();

        // load dll to force query of supported plugins
        hr = pPluginDll->Load();
        if (SUCCEEDED(hr))
        {
            // add successfully loaded dll to list
            pPluginDll->AddRef();
            m_pluginDlls.AddTail(pPluginDll);

            // Unload the dll. The dll was loaded only to query supported plugin 
            // info. It will be re-loaded only when actually needed, i.e., when
            // an instance of a plugin that it implements is requested.
            pPluginDll->Unload();
        }
        HX_RELEASE(pPluginDll);
        if (FAILED(hr))
        {
            if(HXR_OUTOFMEMORY == hr)
            {
                break;
            }

            // lib load attempt failed; add this to 'other' dll list (maybe it is a codec dll, e.g.)
            HXOtherDLL* pOtherDll  = new HXOtherDLL(pszDllName, pMountPoint);
            if( !pOtherDll)
            {
                //oom
                hr = HXR_OUTOFMEMORY;
                break;
            }
            pOtherDll->AddRef();
            m_otherDlls.AddTail(pOtherDll);
            bWriteArchive = true;
        }	
    }
#ifdef HELIX_CONFIG_OPTIMIZE_DLL_LOADING
    } // hr != HXR_OK
#endif

    if ( hr != HXR_OUTOFMEMORY)
    {
    RebuildPluginList();

#if defined(HELIX_FEATURE_PREFERENCES)
    // write archive now if it needs updating
    if(bWriteArchive && bCanWriteArchive)
    {
        SaveToArchive(strArchiveFile);
    }
#endif /* #if defined(HELIX_FEATURE_PREFERENCES) */
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
HX_RESULT HXPluginManager::ReloadPlugins( const char* pMountPoint )
{
    CFindFile*	pFileFinder = NULL;
    IHXBuffer*	pPathBuffer = NULL;

    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(m_pContext);
    if (m_pContext)
    {
#if defined(_STATICALLY_LINKED) && !defined(HELIX_CONFIG_CONSOLIDATED_CORE)
        const char* const pszPluginDir = "";
        pFileFinder = CStaticFindFile::CreateFindFile(pszPluginDir,0, OS_DLL_PATTERN_STRING);
#else
        const char* const pszPluginDir = pMountPoint;
#if defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
        pFileFinder = new HXSymbianNameOnlyFindFile(m_pContext);
#else
        pFileFinder = CFindFile::CreateFindFile(pszPluginDir, 0, OS_DLL_PATTERN_STRING);
#endif // End of #if defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)

#endif
        if (pFileFinder)
        {
            hr = ReloadPluginsWithFindFile(pMountPoint, pFileFinder, pPathBuffer, pszPluginDir);
            HX_DELETE(pFileFinder);
        }
    }

    return hr;
}

STDMETHODIMP HXPluginManager::FindIndexUsingValues(IHXValues* pValues,REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP HXPluginManager::GetInstance (UINT32 index, REF(IUnknown*) pUnknown)
{
    pUnknown = NULL;
    LISTPOSITION pPos = m_plugins.FindIndex(index);
    if (pPos)
    {
	HXPlugin* pPlugin = (HXPlugin*) m_plugins.GetAt(pPos);
	if (pPlugin)
	{
	    return pPlugin->GetInstance(pUnknown);
	}
    }
    return HXR_FAIL;
}

STDMETHODIMP HXPluginManager::FindIndexUsingStrings (char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP HXPluginManager::FindPluginUsingValues(IHXValues* pValues,REF(IUnknown*) pUnk)
{
    return FindPluginUsingValues( pValues, pUnk, NULL );
}

HX_RESULT
HXPluginManager::FindGroupOfPluginsUsingBuffers(char* PropName1,
                                                void* PropVal1,
						REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    HX_RESULT   retVal = HXR_FAIL;
    IHXValues*  pValues = NULL;
    HXPluginEnumerator* pEnumerator = NULL;

    pIEnumerator = NULL;

    if (!PropName1 || !PropVal1)
    {
        return retVal;
    }

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
        AddToValues(pValues, PropName1, PropVal1, eBuffer);
        retVal = FindGroupOfPluginsUsingValues(pValues, pEnumerator);
    }
    HX_RELEASE(pValues);

    // If we have our enumerator, get the appropriate interface
    if (SUCCEEDED(retVal))
    {
	retVal = pEnumerator->QueryInterface(IID_IHXPluginSearchEnumerator, (void**)&pIEnumerator);
    }

    return retVal;
}

HX_RESULT HXPluginManager::FindGroupOfPluginsUsingStrings(char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(HXPluginEnumerator*) pEnumerator)
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::FindGroupOfPluginsUsingStrings(): '%s' = '%s', etc.", PropName1, PropVal1);

    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
	(PropName2 && !PropVal2)    ||
	(PropName3 && !PropVal3)    ||
	(!PropName1 && PropVal1)    ||
	(!PropName2 && PropVal2)    ||
	(!PropName3 && PropVal3))
	return HXR_FAIL;

    IHXValues* pValues = NULL;
    HX_RESULT   retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
        AddToValues(pValues, PropName1, PropVal1, eString);
        AddToValues(pValues, PropName2, PropVal2, eString);
        AddToValues(pValues, PropName3, PropVal3, eString);
        retVal = FindGroupOfPluginsUsingValues(pValues, pEnumerator);
    }
    HX_RELEASE(pValues);
    return retVal;
}

HX_RESULT HXPluginManager::FindGroupOfPluginsUsingValues(IHXValues* pValues,
							REF(HXPluginEnumerator*) pEnumerator)
{
    
    HX_RESULT hr = HXR_FAIL;
    pEnumerator = NULL;

    for(CHXSimpleList::Iterator iter = m_plugins.Begin();
        iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;

	if (pPlugin->DoesMatch(pValues))
	{
	    if (!pEnumerator)
	    {
		pEnumerator = new HXPluginEnumerator();
                if(pEnumerator)
                {
                    hr = HXR_OK;
                }
                else
                {
                    hr = HXR_OUTOFMEMORY;
                    break;
                }
	    }
    
            pEnumerator->Add(pPlugin); 
        }
    }

    return hr;
}


// IHXPluginHandler3
STDMETHODIMP
HXPluginManager::RegisterContext( IUnknown* pContext )
{
    HX_ASSERT(pContext);
    if( !pContext )
    {
	return HXR_INVALID_PARAMETER;
    }

    if( m_pContext )
    {
	return HXR_UNEXPECTED;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pClassFactory);

    return HXR_OK;
}

// IHXPluginHandler3
STDMETHODIMP
HXPluginManager::AddPluginMountPoint( const char* pName, UINT32 majorVersion, UINT32 minorVersion, IHXBuffer* pPath )
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::AddPluginMountPoint(): name = '%s'", pName);

    const char* pMPKey = pName ? pName : (const char*) pPath->GetBuffer();

    // Make sure this mount point is in the list
    CHXString strMountPoint;
    if( !m_mountPoints.Lookup(pMPKey, strMountPoint) && pPath )
    {
        strMountPoint = (const char*)pPath->GetBuffer();
        
        // Put new mount point in list
        m_mountPoints.SetAt( pMPKey, strMountPoint );
    }

    // Load information from registry, and sync DLLs that aren't up to date
    return ReloadPlugins( strMountPoint );
}

// IHXPluginHandler3
STDMETHODIMP
HXPluginManager::RefreshPluginMountPoint( const char* pName )
{
    HX_RESULT result = HXR_FAIL;

    CHXString strMountPoint;
    if( m_mountPoints.Lookup( pName, strMountPoint ) )
    {
	result = ReloadPlugins( strMountPoint );
    }

    return result;
}

// IHXPluginHandler3
STDMETHODIMP
HXPluginManager::RemovePluginMountPoint( const char* pName )
{
    HX_RESULT hr = HXR_FAIL;

    CHXString strMountPoint;
    if( m_mountPoints.Lookup( pName, strMountPoint ) )
    {
	// Clean up plugin dlls associated with mountpoint
	LISTPOSITION listPos = m_pluginDlls.GetHeadPosition();
	while( listPos )
	{
	    LISTPOSITION curPos = listPos;
	    HXPluginDLL* pLibrary = (HXPluginDLL*) m_pluginDlls.GetNext( listPos );
	    if( pLibrary && ( pLibrary->GetMountPoint() == strMountPoint ) )
	    {
		m_pluginDlls.RemoveAt( curPos ); 
                pLibrary->Unload(); // just in case; this should be last ref
		HX_RELEASE( pLibrary );
	    }
	}

        
        // Clean up 'other' dlls associated with mountpoint
        listPos = m_otherDlls.GetHeadPosition();
	while( listPos )
	{
	    LISTPOSITION curPos = listPos;
	    HXOtherDLL* pLibrary = (HXOtherDLL*) m_otherDlls.GetNext( listPos );
	    if( pLibrary && ( pLibrary->GetMountPoint() == strMountPoint ) )
	    {
		m_otherDlls.RemoveAt( curPos ); 
		HX_RELEASE( pLibrary );
	    }
	}
       
	m_mountPoints.RemoveKey( pName );

        // Plugin list must be rebuilt
        RebuildPluginList();

        hr = HXR_OK;

    }

    return hr;
}

//
// called every time dll list has been altered (we could be more efficient in many cases)
//
void HXPluginManager::RebuildPluginList()
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::RebuildPluginList()");

    CHXSimpleList::Iterator iter;
        
    // clean up (now invalid) plugin list
    for(iter = m_plugins.Begin(); iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;
	pPlugin->Release();
    }
    m_plugins.RemoveAll();

    // rebuild list of plugins we can instanciate
    for(iter = m_pluginDlls.Begin(); iter != m_pluginDlls.End(); ++iter)
    {
	HXPluginDLL* pLibrary = (HXPluginDLL*) *iter;
        pLibrary->AddPlugins(m_plugins);
    }
}


STDMETHODIMP
HXPluginManager::FindImplementationFromClassID( REFGUID GUIDClassID, REF(IUnknown*) pIUnknownInstance,
					    IUnknown* pIUnkOuter, IUnknown* pContext )
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::FindImplementationFromClassID()");
    HX_RESULT			    rc = HXR_OK;
    IUnknown*			    pUnknown = NULL;
    IHXPlugin*			    pPlugin = NULL;
    IHXCommonClassFactory*	    pFactory = NULL;
    IHXPluginSearchEnumerator*	    pPluginSearchEnumerator = NULL;

    // a much simpler version than Plugin2Handler, it's not as efficient as Plugin2Handler but it
    // should be good enough for static build.
    
    pIUnknownInstance = NULL;

    if (HXR_OK == FindGroupOfPluginsUsingBuffers(PLUGIN_COMPONENT_CLSID, (void*)&GUIDClassID, pPluginSearchEnumerator))
    {
        HX_ASSERT(1 == pPluginSearchEnumerator->GetNumPlugins());
        if (HXR_OK != pPluginSearchEnumerator->GetPluginAt(0, pIUnknownInstance, pIUnkOuter))
        {
            HX_ASSERT(FALSE);
            HX_RELEASE(pIUnknownInstance);
        }
    }
    else
    {                  
        rc = FindGroupOfPluginsUsingStrings(PLUGIN_CLASS, PLUGIN_CLASS_FACTORY_TYPE,
					    NULL, NULL, NULL, NULL, pPluginSearchEnumerator);

        if (SUCCEEDED(rc))
        {
            while (HXR_OK == pPluginSearchEnumerator->GetNextPlugin(pUnknown, NULL) && pUnknown)
            {
	        if (HXR_OK == pUnknown->QueryInterface(IID_IHXPlugin, (void**)&pPlugin))
	        {
	            pPlugin->InitPlugin(pContext);

	            if (HXR_OK == pUnknown->QueryInterface(IID_IHXCommonClassFactory, (void**)&pFactory))
	            {
		        if (HXR_OK == pFactory->CreateInstance(GUIDClassID, (void**)&pIUnknownInstance) &&
		            pIUnknownInstance)
		        {
		            break;
		        }
		        HX_RELEASE(pIUnknownInstance);
	            }
	            HX_RELEASE(pFactory);
	        }
	        HX_RELEASE(pPlugin);
	        HX_RELEASE(pUnknown);
            }
        }
    }

    if (!pIUnknownInstance)
    {
	rc = HXR_FAILED;
    }

    // cleanup
    HX_RELEASE(pFactory);
    HX_RELEASE(pPlugin);
    HX_RELEASE(pUnknown);
    HX_RELEASE(pPluginSearchEnumerator);

    return rc;
}

STDMETHODIMP
HXPluginManager::FindCLSIDFromName( const char* pName, REF(IHXBuffer*) pCLSID )
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
HXPluginManager::FindGroupOfPluginsUsingValues( IHXValues* pValues,
				REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    HX_RESULT result = HXR_OK;
    HXPluginEnumerator *pEnumerator = NULL;
	
	
    result = FindGroupOfPluginsUsingValues(pValues, pEnumerator);
							
    // If we have our enumerator, get the appropriate interface
    if( SUCCEEDED( result ) )
    {
        result = pEnumerator->QueryInterface( IID_IHXPluginSearchEnumerator,
                                              (void**) &pIEnumerator );
    }

    return result;
}

STDMETHODIMP
HXPluginManager::FindGroupOfPluginsUsingStrings( char* PropName1, char* PropVal1,
				char* PropName2, char* PropVal2,
				char* PropName3, char* PropVal3,
				REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::FindGroupOfPluginsUsingStrings()");
    // Regardless of how the API can be used, the reality is that this function
    // is essentially the highest level point of access for the core when
    // starting playback of a new stream, so this is a great time to unload
    // all our dead dlls.
    // But the memory efficiency of DLL unloading comes at the expense of
    // setup time, so we probably only want to do this on platforms where
    // memory optimization is our highest priority.
#if defined(HELIX_CONFIG_UNLOAD_DEAD_DLLS)
    UnloadDeadDLLs();
#endif

    // Initialize out params
    pIEnumerator = NULL;

    // Use the internal function to build up an enumerator object
    HXPluginEnumerator* pEnumerator = NULL;
    HX_RESULT result = FindGroupOfPluginsUsingStrings( PropName1, PropVal1,
			    PropName2, PropVal2, PropName3, PropVal3, pEnumerator );

    // If we have our enumerator, get the appropriate interface
    if( SUCCEEDED( result ) )
    {
	result = pEnumerator->QueryInterface( IID_IHXPluginSearchEnumerator,
						(void**) &pIEnumerator );
    }

    return result;
}



STDMETHODIMP
HXPluginManager::FindPluginUsingValues( IHXValues* pCriteria,
					REF(IUnknown*) pIUnkResult,
					IUnknown* pIUnkOuter )
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::FindPluginUsingValues()");

    // out
    pIUnkResult = NULL;

    CHXSimpleList   matches;
    CHXSimpleList::Iterator iter = m_plugins.Begin();
    for(; iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;
	if (pPlugin->DoesMatch(pCriteria))
	{
	    matches.AddTail(pPlugin);
	}
    }

    HX_RESULT hr = HXR_FAIL;

    if (matches.GetCount() > 0)
    {
        // return first match by default
        HXPlugin* pPlug = (HXPlugin*) *(matches.Begin());

#if (0)
        if (matches.GetCount() > 1)
        {
            //
            // strategy:
            //
            // with multiple matching plugins, pick first one with description "RealNetworks"
            //
	    for(iter = matches.Begin(); iter != matches.End(); ++iter)
	    {
                HXPlugin* pThisPlugin = (HXPlugin*) *iter;
                if( IsRealNetworksPlugin(pThisPlugin) )
                {
                    // use this one
                    pPlug = pThisPlugin;
                    break;
	            
                }
	    }
        }
#endif

        hr = pPlug->GetInstance( pIUnkResult, pIUnkOuter );
    }

    return hr;
}


STDMETHODIMP
HXPluginManager::FindPluginUsingStrings( char* PropName1, char* PropVal1,
					char* PropName2, char* PropVal2,
					char* PropName3, char* PropVal3,
					REF(IUnknown*) pIUnkResult,
					IUnknown* pIUnkOuter )
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::FindPluginUsingStrings()");
    
    // out 
    pIUnkResult = NULL;

    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
	(PropName2 && !PropVal2)    ||
	(PropName3 && !PropVal3)    ||
	(!PropName1 && PropVal1)    ||
	(!PropName2 && PropVal2)    ||
	(!PropName3 && PropVal3))
	return HXR_FAIL;

    IHXValues*  pValues = NULL;
    HX_RESULT   retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
	AddToValues(pValues, PropName1, PropVal1, eString);
	AddToValues(pValues, PropName2, PropVal2, eString);
	AddToValues(pValues, PropName3, PropVal3, eString);
	retVal = FindPluginUsingValues( pValues, pIUnkResult, pIUnkOuter );
    }
    pValues->Release();
    return retVal;
}


#if defined(HELIX_CONFIG_UNLOAD_DEAD_DLLS)
//
// Call Unload() on each plugins (library unloads only if no plugin references)
//
void
HXPluginManager::UnloadDeadDLLs()
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::UnloadDeadDLLs()");
    
    for(CHXSimpleList::Iterator iter = m_pluginDlls.Begin(); iter != m_pluginDlls.End(); ++iter)
    {
	HXPluginDLL* pPlugDLL = (HXPluginDLL*) *iter;
	pPlugDLL->Unload();
    }
}
#endif


STDMETHODIMP
HXPluginManager::GetPlugin( ULONG32 ulIndex, REF(IUnknown*) pIUnkResult,
					    IUnknown* pIUnkOuter )
{
    if( ulIndex <= (ULONG32)(m_plugins.GetCount()-1) && m_plugins.GetCount() )
    {
	LISTPOSITION pPos = m_plugins.FindIndex( ulIndex );
	if (pPos)
	{
	    HXPlugin* pPlugin = (HXPlugin*) m_plugins.GetAt( pPos );
	    if( pPlugin )
	    {
		return pPlugin->GetInstance( pIUnkResult, pIUnkOuter );
	    }
	}
    }
    return HXR_FAIL;

}


STDMETHODIMP
HXPluginManager::UnloadPluginFromClassID(REFGUID GUIDClassID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXPluginManager::UnloadPackageByName(const char* pName)
{
    return HXR_NOTIMPL;
}



STDMETHODIMP HXPluginManager::FindPluginUsingStrings (char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(IUnknown*) pRetUnk)
{
    return FindPluginUsingStrings( PropName1, PropVal1, PropName2, PropVal2,
				   PropName3, PropVal3, pRetUnk, NULL );
}


STDMETHODIMP
HXPluginManager::FindImplementationFromClassID
(
    REFGUID GUIDClassID,
    REF(IUnknown*) pIUnknownInstance
)
{
    return FindImplementationFromClassID( GUIDClassID, pIUnknownInstance, NULL, m_pContext );
}

STDMETHODIMP HXPluginManager::Close()
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::Close()");

    // Release all dlls and associated plugins
   
    CHXSimpleList::Iterator iter;
       
    //HXPlugin
    for(iter = m_plugins.Begin(); iter != m_plugins.End(); ++iter)
    {
	HXPlugin* pPlugin = (HXPlugin*) *iter;
	pPlugin->Release();
    }
    m_plugins.RemoveAll();
 
    //HXPluginDLL
    for(iter = m_pluginDlls.Begin(); iter != m_pluginDlls.End(); ++iter)
    {
	HXPluginDLL* pLib = (HXPluginDLL*) *iter;
	pLib->Release();
    }
    m_pluginDlls.RemoveAll();

    //HXOtherDLL
    for(iter = m_otherDlls.Begin(); iter != m_otherDlls.End(); ++iter)
    {
	HXOtherDLL* pLib = (HXOtherDLL*) *iter;
	pLib->Release();
    }
    m_otherDlls.RemoveAll();

    m_mountPoints.RemoveAll();

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);

    return HXR_OK;
}

STDMETHODIMP HXPluginManager::SetRequiredPlugins (const char** ppszRequiredPlugins)
{
    return HXR_OK;
}


HX_RESULT HXPluginManager::AddToValues(IHXValues* pValues, char* pPropName, void* pPropVal, eValueTypes eValueType)
{
    HX_RESULT hr = HXR_OK;
    CHXString theValue;

    if (!pPropName || !pPropVal)
	return HXR_FAIL;

    if (eValueType != eBuffer)
    {
        // trim value
        theValue = (const char*)pPropVal;
        theValue.TrimLeft();
        theValue.TrimRight();
    }

    switch (eValueType)
    {
	case eString:
	{
            hr = SetCStringPropertyCCF(pValues, pPropName, (const char*)pPropVal, m_pContext);
            break;
	}
	case eInt:
	{
	    int val = atoi(theValue);
	    pValues->SetPropertyULONG32(pPropName, (ULONG32)val);
	    break;
	}
	case eBuffer:
        {
            hr = SetBufferPropertyCCF(pValues, pPropName, (BYTE*)pPropVal, sizeof(GUID), m_pContext);
            break;
        }
        default:
        {
            hr = HXR_NOTIMPL;
            break;
        }
    }
    return hr;
}

STDMETHODIMP_(ULONG32) HXPluginManager::GetNumOfPlugins()
{
    return m_plugins.GetCount();
}

STDMETHODIMP HXPluginManager::GetPlugin(ULONG32 ulIndex, REF(IUnknown*)  /*OUT*/ pInstance)
{
    return GetPlugin( ulIndex, pInstance, NULL );
}


STDMETHODIMP HXPluginManager::FlushCache()
{
    return HXR_OK;
}

STDMETHODIMP HXPluginManager::SetCacheSize(ULONG32 nSizeKB)
{
    return HXR_OK;
}

STDMETHODIMP HXPluginManager::ReadFromRegistry()
{
    HXLOGL3(HXLOG_CORE, "HXPluginManager()::ReadFromRegistry()");
    HX_RESULT result;
    IHXBuffer* pBuffer = NULL;

#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)

    const char* pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
    pBuffer = HXBufferUtil::CreateBuffer(m_pClassFactory, pPath);
#endif

    // Set up a mount point with the default plugin location
    result = AddPluginMountPoint(HXVER_SDK_PRODUCT, 0, 0, pBuffer);

    HX_RELEASE(pBuffer);

    return result;
}


/********************************************************************
*
*	Plugin Enumeration
*
********************************************************************/

BEGIN_INTERFACE_LIST_NOCREATE( HXPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginSearchEnumerator )
END_INTERFACE_LIST

HXPluginEnumerator::HXPluginEnumerator() :
    m_nIndex(0)
{
}

HXPluginEnumerator::~HXPluginEnumerator()
{
}


STDMETHODIMP_( UINT32 )
HXPluginEnumerator::GetNumPlugins()
{
    return m_plugins.GetCount();
}

STDMETHODIMP_( void )
HXPluginEnumerator::GoHead()
{
    m_nIndex = 0;
}


STDMETHODIMP
HXPluginEnumerator::GetNextPlugin( REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    HX_RESULT res = GetPluginAt( m_nIndex, pIUnkResult, pIUnkOuter );
    m_nIndex++;

    return res;
}

STDMETHODIMP
HXPluginEnumerator::GetNextPluginInfo( REF(IHXValues*) pRetValues )
{
    pRetValues = NULL;

    HX_RESULT res = GetPluginInfoAt( m_nIndex, pRetValues );
    m_nIndex++;

    return res;
}


STDMETHODIMP
HXPluginEnumerator::GetPluginAt( UINT32 index, REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    pIUnkResult = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_plugins.FindIndex(index);
    if (pos)
    {
	HXPlugin* pPlugin = (HXPlugin*) m_plugins.GetAt(pos);
	if (pPlugin)
	{
	    res = pPlugin->GetInstance( pIUnkResult, pIUnkOuter );
	}
    }
    return res;
}


STDMETHODIMP
HXPluginEnumerator::GetPluginInfoAt( UINT32 index, REF(IHXValues*) pRetValues )
{
    // Initialize out params
    pRetValues = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_plugins.FindIndex(m_nIndex);
    m_nIndex++;
    if (pos)
    {
	HXPlugin* pPlugin = (HXPlugin*) m_plugins.GetAt(pos);
	if (pPlugin)
	{
	    res = pPlugin->GetPluginInfo( pRetValues );
	}
    }

    return res;
}


void HXPluginEnumerator::Add(HXPlugin* pPlugin)
{
    HX_ASSERT(pPlugin);
    m_plugins.AddTail(pPlugin);
}
