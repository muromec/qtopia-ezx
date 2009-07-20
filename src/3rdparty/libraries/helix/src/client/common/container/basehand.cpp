/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basehand.cpp,v 1.28 2007/04/13 17:57:19 ping Exp $
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

#ifdef _WINDOWS
#if defined(_WINCE)
#include <stdlib.h>
#define itoa _itoa
#endif
#include <windows.h>
#include <ctype.h>
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#include "filespec.h"
#include "filespecutils.h"
#endif

#if defined _UNIX
#include <stdlib.h>
#include <sys/param.h>
#define _MAX_PATH	MAXPATHLEN
#elif defined (__MWERKS__)
#include <stdlib.h>
#include "fullpathname.h"
#include "chxdataf.h"
#include <stat.h>
#include <fcntl.h>
#else
#include "hlxclib/sys/stat.h"
#endif

#include "hlxclib/stdio.h"

#include "hxresult.h"
#include "hxassert.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxfwrtr.h"
#include "hxrendr.h"
#include "hxprefs.h"
#include "hxplugn.h"
#include "hxdtcvt.h"
#include "hxphand.h"
#include "hxmeta.h"
#include "hxsdesc.h"
#include "hxauth.h"
#include "hxallow.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxshtdn.h"
#include "hxplgns.h"
#include "hxmon.h"
#include "chxpckts.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "hxdir.h"
#include "findfile.h"
#include "dbcs.h"
#include "hxbdwdth.h"
#include "basehand.h"
#include "chxuuid.h"
#include "md5.h"

#include "dllacces.h"
#include "dllpath.h"

#include "hxperf.h"
#include "rtsputil.h"

#include "hxver.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WINCE
#include <wincestr.h>
#endif

#if defined(_STATICALLY_LINKED)
#include "staticff.h"
#endif

/*
 * XXXND These are also defined in geminc/gemplatformdata.h
 */

#define NAMESPACE_SEPARATOR ':'

/*
 *  Win98 does not allow reg keys that are larger than 16k a pop. we used this value to break up into more manageable bites
 */

#define PREF_CACHE_SIZE 10000

/*
    Load each plug-in, read info, store in memory and update prefs
    This is only done if the plugin hash within the registery is not the same
    as the plugin hash of the plugins directory.
*/

const char* const BaseHandler::zm_pszValueSeperator = "|";
const char* const BaseHandler::zm_pszListStart	= "{";
const char* const BaseHandler::zm_pszListEnd	= "}";
const char* const BaseHandler::zm_pszValueSeperator2= ",";

const char* const BaseHandler::zm_pszKeyNameRegKey = "~KeyNames~";
const char* const BaseHandler::zm_pszRegKeySeperator = "\\";

const char* const BaseHandler::zm_pszFileExtension = OS_DLL_PATTERN_STRING;
const char* const BaseHandler::zm_pszDirectorySeperator = OS_SEPARATOR_STRING;
const char BaseHandler::zm_cDirectorySeperator = OS_SEPARATOR_CHAR;

IMPLEMENT_COM_CREATE_FUNCS( BaseHandler )

BEGIN_INTERFACE_LIST_NOCREATE( BaseHandler )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPlugin2Handler )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginHandler3 )
END_INTERFACE_LIST

BaseHandler::BaseHandler() 
            : m_pContext(NULL)
            , m_bLoadStaticLinkedPlugins(FALSE)
{
}

BaseHandler::~BaseHandler()
{
    // Make sure Close() got called
    if( m_pContext )
    {
	Close();
    }
}

STDMETHODIMP BaseHandler::Init(IUnknown* pContext)
{
    HX_RESULT result = HXR_FAIL;

    if( SUCCEEDED( result = RegisterContext( pContext ) ) )
    {
	result = ReadFromRegistry();
    }

    return result;
}


STDMETHODIMP_(ULONG32) BaseHandler::GetNumOfPlugins2()
{
    return m_PluginList.GetCount();
}


STDMETHODIMP
BaseHandler::GetPluginInfo(UINT32 unIndex,
			   REF(IHXValues*) /*OUT*/ pValues)
{
    return HXR_NOTIMPL;
}

BaseHandler::Errors
BaseHandler::LoadDLL(char* pszDllName,
		     PluginMountPoint* pMountPoint )
{
    Errors			result	    = NO_ERRORS;
    UINT32			i	    = 0;
    struct stat			stat_stuct;

    // Make sure there is no path in the pszDllName
    HX_ASSERT( !strrchr(pszDllName, BaseHandler::zm_cDirectorySeperator) );

    BaseHandler::PluginDLL* pPluginDll  = NULL;
    if( !( pPluginDll = new BaseHandler::PluginDLL( pszDllName, pMountPoint, this ) ) )
    {
	return MEMORY_ERROR;
    }

    pPluginDll->AddRef();

    CHXString sFileWithPath = pszDllName;
    if (NO_ERRORS==Stat(sFileWithPath, &stat_stuct))
    {
        pPluginDll->SetFileSize((INT32)stat_stuct.st_size);
    }

    result = pPluginDll->Load(m_pContext);
    if (NO_ERRORS != result )
    {
	goto cleanup;
    }

    for(i=0;i<pPluginDll->GetNumPlugins();i++)
    {
	Plugin*	pPlugin	= NULL;

    	// create a new plugin object
	if (!(pPlugin = new Plugin(m_pContext)))
	{
	    return MEMORY_ERROR;
	}

	// Setup plugin information
	pPlugin->AddRef();
	pPlugin->SetDLL(pPluginDll);
	pPlugin->SetIndex((UINT16)i);
	pPlugin->SetInfoNeedsRefresh(TRUE);

	IUnknown* pUnk = NULL;
	if( NO_ERRORS != pPlugin->GetPlugin( pUnk ) )
	{
	    // This plugin doesn't work.  Delete it.
	    HX_RELEASE( pPlugin );
	}
	else
	{
	    IHXPluginNamespace* pPluginNamespace = NULL;
	    if (SUCCEEDED(pUnk->QueryInterface(IID_IHXPluginNamespace, (void**) &pPluginNamespace)))
	    {
		/*
		 * Memory for the IHXBuffer is allocated in the plugin
		 */
		IHXBuffer* pBuffer = NULL;
		if (SUCCEEDED(pPluginNamespace->GetPluginNamespace(pBuffer)))
		{
		    pPluginDll->SetNamespace(pBuffer);
		    HX_RELEASE(pBuffer);
		}
		HX_RELEASE(pPluginNamespace);
	    }

	    IHXComponentPlugin* pIIterator = NULL;
	    if( SUCCEEDED( pUnk->QueryInterface( IID_IHXComponentPlugin, (void**) &pIIterator ) ) )
	    {
		// We don't need this.
		HX_RELEASE( pPlugin );

		LoadPluginsFromComponentDLL( pPluginDll, pIIterator );
		HX_RELEASE( pIIterator );
	    }
	    else
	    {
		IHXPlugin* pIHXPlugin;
		if( SUCCEEDED( pUnk->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
		{
		    pPlugin->GetValuesFromDLL(pIHXPlugin);
		    m_PluginList.AddTail(pPlugin);
		    pIHXPlugin->Release();
		}
	    }
	}
	HX_RELEASE( pUnk );
    }

cleanup:

    HX_RELEASE( pPluginDll );

    return result;
}


void BaseHandler::LoadPluginsFromComponentDLL( BaseHandler::PluginDLL* pPluginDll,
						    IHXComponentPlugin* pIIterator )
{
    IHXPlugin* pIHXPlugin = NULL;
    if( SUCCEEDED( pIIterator->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
    {
        pIHXPlugin->InitPlugin(m_pContext);
	for( UINT32 index = 0; index < pIIterator->GetNumComponents(); index++ )
	{
	    IHXValues* pIValues = NULL;
	    if( SUCCEEDED( pIIterator->GetComponentInfoAtIndex( index, pIValues ) ) )
	    {
		IHXBuffer* pBuffer = NULL;

		if (SUCCEEDED(pIValues->GetPropertyCString(PLUGIN_COMPONENT_NAME, pBuffer)))
		{
		    IHXBuffer* pNamespace = pPluginDll->GetNamespace();

		    if (pNamespace)
		    {
			CHXString TempNamespace = pNamespace->GetBuffer();
			TempNamespace += NAMESPACE_SEPARATOR;
			TempNamespace += pBuffer->GetBuffer();

			IHXBuffer* pTempBuffer = NULL;
			if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (BYTE*)(const char*)TempNamespace,
							    TempNamespace.GetLength()+1, m_pContext))
			{
			    pIValues->SetPropertyCString(PLUGIN_COMPONENT_NAME, pTempBuffer);
			    HX_RELEASE(pTempBuffer);
			    HX_RELEASE(pNamespace);
			}
		    }

		    HX_RELEASE(pBuffer);
		}

		// create a new plugin object
		Plugin* pPlugin = new Plugin( m_pContext );
		HX_ASSERT( pPlugin );

		// Setup plugin object
		pPlugin->AddRef();
		pPlugin->SetDLL( pPluginDll );

		pPlugin->SetInfoNeedsRefresh( TRUE );

		pPlugin->InitializeComponentPlugin( pIHXPlugin, pIValues );

		// Put in plugin list
		m_PluginList.AddTail(pPlugin);

		HX_RELEASE( pIValues );
	    }
	}

	HX_RELEASE (pIHXPlugin);
    }
}

///////////////////////////////////////////////////////////////////////////////
//  These functions will find all plugins which are different
//  then those loaded into the registry.
//  It will then load them into memory, get their data, and unload them.
//  It will return HXR_FAIL if some DLL has different values within the
//  registry, and is presently in memory (how could this happen??)

//  If anyone was keeping an index to a loaded DLL and assuming that it
//  would remain constant ... that's not going to work!!!

HX_RESULT BaseHandler::ReloadPluginsNoPropagate()
{
    HX_LOG_BLOCK( "BaseHandler::ReloadPluginsNoPropagate" );

    HX_RESULT result = HXR_OK;

    // Reload them all.
    for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
    {
	PluginMountPoint* pMountPoint = (PluginMountPoint*) *mp;
	if( FAILED( ReloadPluginsNoPropagate( pMountPoint ) ) )
	{
	    result = HXR_FAIL;
	}
    }

    return result;
}


HX_RESULT BaseHandler::ReloadPluginsNoPropagateWithFindFile(
               PluginMountPoint* pMountPoint, CFindFile* pFileFinder,
               IHXBuffer* pPathBuffer, char* pszPluginDir)
{
    HX_LOG_BLOCK( "BaseHandler::ReloadPluginsNoPropagateWithFindFile" );
    char*	    pszDllName	    = 0;
    IHXBuffer*	    pNewChecksum    = 0;

    // if we have no context do not proceed.
    if (!m_pContext)
    {
	return INVALID_CONTEXT;
    }

    // If this is the 1st time, load everything into the registry.

    pszDllName = pFileFinder->FindFirst();

    while (pszDllName)
    {
	BaseHandler::Errors loadResult;
	loadResult = LoadDLL( pszDllName, pMountPoint );

	if (loadResult!= NO_ERRORS)
	{
	    // The DLL had one of the following problems:
	    // (1) the DLL was unloadable
	    // (2) the DLL did not have an HXCreateInstance
	    // (3) an instance could not be created.
	    // (4) It did not implement the PLUGIN interface

	    // if it was case 2,3,4 then we can safely never attempt to
	    // load the DLL again. However if it was (1) then we must attempt
	    // to load the DLL ever time through since it was possibly unloadable due
	    // to an imp-lib that will be satisfied lated (without modifing the
	    // dll). Jeeze. That comment is UNREADABLE. I have to take effective written
	    // english again!

	    if (loadResult!=CANT_OPEN_DLL)
	    {
		BaseHandler::OtherDLL* pDLLData = new BaseHandler::OtherDLL;
		pDLLData->m_filename = pszDllName;
		pDLLData->m_pMountPoint = pMountPoint;

		pNewChecksum = ChecksumFile(pszDllName, pPathBuffer);
		if (pNewChecksum)
		{
		    pDLLData->m_fileChecksum = (char*)pNewChecksum->GetBuffer();
		    HX_RELEASE(pNewChecksum);
		    m_MiscDLLList.AddTail(pDLLData);
		}
		else
		{
		    HX_DELETE(pDLLData);
		}
	    }
	}

	pszDllName = pFileFinder->FindNext();
    }

    return HXR_OK;
}

HX_RESULT BaseHandler::ReloadPluginsNoPropagate( PluginMountPoint* pMountPoint )
{
    CFindFile*	pFileFinder = NULL;
    IHXBuffer*	pPathBuffer = NULL;
    char*	pszPluginDir = NULL;
    ULONG32	nPluginDirLen = NULL;

    if (!m_pContext)
    {
        return INVALID_CONTEXT;
    }

    HX_RESULT retVal = HXR_FAIL;

#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    pPathBuffer = pMountPoint->Path();
    if (pPathBuffer)
    {
        pPathBuffer->Get((UCHAR*&)pszPluginDir, nPluginDirLen);

        if (nPluginDirLen)
        {
            pFileFinder = CFindFile::CreateFindFile(pszPluginDir,
                        0, BaseHandler::zm_pszFileExtension);
            if (pFileFinder)
            {
                retVal = ReloadPluginsNoPropagateWithFindFile(pMountPoint,
                        pFileFinder, pPathBuffer, pszPluginDir);

                HX_DELETE(pFileFinder);
            }
        }
    }
#endif

#if defined(_STATICALLY_LINKED)
    if (!m_bLoadStaticLinkedPlugins)
    {
        m_bLoadStaticLinkedPlugins = TRUE;

	pszPluginDir="";
        pFileFinder = CStaticFindFile::CreateFindFile(pszPluginDir,
                0, BaseHandler::zm_pszFileExtension);
        if (pFileFinder)
        {
            retVal = ReloadPluginsNoPropagateWithFindFile(pMountPoint,
                    pFileFinder, pPathBuffer, pszPluginDir);

            HX_DELETE(pFileFinder);
        }
    }
#endif

#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    HX_RELEASE(pPathBuffer);
#endif

    return retVal;
}

STDMETHODIMP BaseHandler::FindIndexUsingValues	(IHXValues* pValues,
							REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP BaseHandler::GetInstance (UINT32 index, REF(IUnknown*) pUnknown)
{
    pUnknown = NULL;
    LISTPOSITION pPos = m_PluginList.FindIndex(index);
    if (pPos)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_PluginList.GetAt(pPos);
	if (pPlugin)
	{
	    Errors retVal = pPlugin->GetInstance(pUnknown);
	    if (retVal== NO_ERRORS)
	    {
		return HXR_OK;
	    }
	}
    }
    return HXR_FAIL;
}

STDMETHODIMP BaseHandler::FindIndexUsingStrings (char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP BaseHandler::FindPluginUsingValues	(IHXValues* pValues,
							REF(IUnknown*) pUnk)
{
    return FindPluginUsingValues( pValues, pUnk, NULL );
}

HX_RESULT BaseHandler::FindGroupOfPluginsUsingStrings(char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(CPluginEnumerator*) pEnumerator)
{
    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
	(PropName2 && !PropVal2)    ||
	(PropName3 && !PropVal3)    ||
	(!PropName1 && PropVal1)    ||
	(!PropName2 && PropVal2)    ||
	(!PropName3 && PropVal3))
	return HXR_FAIL;

    IHXValues*	pValues = NULL;
    HX_RESULT	retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
	AddToValues(pValues, PropName1, PropVal1, eString);
	AddToValues(pValues, PropName2, PropVal2, eString);
	AddToValues(pValues, PropName3, PropVal3, eString);
	retVal = FindGroupOfPluginsUsingValues(pValues, pEnumerator);
    }
    pValues->Release();
    return retVal;
}

HX_RESULT BaseHandler::FindGroupOfPluginsUsingValues(IHXValues* pValues,
							REF(CPluginEnumerator*) pEnumerator)
{
    CHXSimpleList::Iterator i = m_PluginList.Begin();
    pEnumerator = NULL;

    for(; i!= m_PluginList.End(); ++i)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) *i;
	if (pPlugin->DoesMatch(pValues))
	{
	    if (!pEnumerator)
	    {
		pEnumerator = new CPluginEnumerator();
	    }
	    pEnumerator->Add(pPlugin);
	}
    }

    if (!pEnumerator)
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}

/********************************************************************
*
*	IHXPluginHandler3
*
********************************************************************/

STDMETHODIMP
BaseHandler::RegisterContext( IUnknown* pContext )
{
    if( !pContext )
    {
	return INVALID_CONTEXT;
    }

    if( m_pContext )
    {
	return HXR_UNEXPECTED;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    return HXR_OK;
}

STDMETHODIMP
BaseHandler::AddPluginMountPoint( const char* pName, UINT32 majorVersion, UINT32 minorVersion, IHXBuffer* pPath )
{
    HX_LOG_BLOCK( "BaseHandler::AddPluginMountPoint" );

    const char* pMPKey = pName ? pName : (const char*) pPath->GetBuffer();

    // Make sure this mount point is in the list
    PluginMountPoint* pMountPoint = NULL;
    if( !m_MountPoints.Lookup( pMPKey, (void*&) pMountPoint ) )
    {
	// Create new mount point
	pMountPoint = new PluginMountPoint( m_pContext, this, pName, majorVersion, minorVersion, pPath );
	pMountPoint->AddRef();

	// Put new mount point in list
	m_MountPoints.SetAt( pMPKey, pMountPoint );
    }

    // Load information from registry, and sync DLLs that aren't up to date
    return RefreshPluginInfo( pMountPoint );
}


STDMETHODIMP
BaseHandler::RefreshPluginMountPoint( const char* pName )
{
    HX_RESULT result = HXR_FAIL;

    // If this mount point is in the list, refresh it
    PluginMountPoint* pMountPoint = NULL;
    if( m_MountPoints.Lookup( pName, (void*&) pMountPoint ) )
    {
	result = RefreshPluginInfo( pMountPoint );
    }

    return result;
}


STDMETHODIMP
BaseHandler::RemovePluginMountPoint( const char* pName )
{
    HX_RESULT result = HXR_FAIL;

    // Make sure this is a valid mount point
    PluginMountPoint* pMountPoint = NULL;
    if( m_MountPoints.Lookup( pName, (void*&) pMountPoint ) )
    {
	// Clean up plugins
	if( m_PluginList.GetCount() )
	{
	    LISTPOSITION listPos = m_PluginList.GetHeadPosition();
	    while( listPos )
	    {
		// Save off current position for delete
		LISTPOSITION posAt = listPos;

		// Get current item, and increment position
		BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_PluginList.GetNext( listPos );

		// If this plugin belongs to the mountpoint, remove it.
		if( pPlugin && ( pPlugin->GetDLL()->GetMountPoint() == pMountPoint ) )
		{
		    // Delete from the saved position
		    m_PluginList.RemoveAt( posAt );
		    HX_RELEASE( pPlugin );
		}
	    }
	}

	// Remove mount point from list
	m_MountPoints.RemoveKey( pName );
	HX_RELEASE( pMountPoint );
    }

    return result;
}


STDMETHODIMP
BaseHandler::FindImplementationFromClassID( REFGUID GUIDClassID, REF(IUnknown*) pIUnknownInstance,
					    IUnknown* pIUnkOuter, IUnknown* pContext )
{
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

HX_RESULT
BaseHandler::RefreshPluginInfo( PluginMountPoint* pMountPoint )
{
    return ReloadPluginsNoPropagate( pMountPoint );
}

STDMETHODIMP
BaseHandler::FindCLSIDFromName( const char* pName, REF(IHXBuffer*) pCLSID )
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
BaseHandler::FindGroupOfPluginsUsingValues( IHXValues* pValues,
				REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    return HXR_NOTIMPL;
}

HX_RESULT
BaseHandler::FindGroupOfPluginsUsingBuffers(char* PropName1,
                                            void* PropVal1,
				            REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    HX_RESULT   retVal = HXR_FAIL;
    IHXValues*  pValues = NULL;
    CPluginEnumerator* pEnumerator = NULL;

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

STDMETHODIMP
BaseHandler::FindGroupOfPluginsUsingStrings( char* PropName1, char* PropVal1,
				char* PropName2, char* PropVal2,
				char* PropName3, char* PropVal3,
				REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
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
    CPluginEnumerator* pEnumerator = NULL;
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

void BaseHandler::ReportError( UINT8 severity, const char* pDLLName, const char* pDesc )
{
}


STDMETHODIMP
BaseHandler::FindPluginUsingValues( IHXValues* pCriteria,
					REF(IUnknown*) pIUnkResult,
					IUnknown* pIUnkOuter )
{
    HX_SETUP_CHECKPOINTLIST( "BaseHandler::FindPluginUsingValues()" );
    HX_PRIME_ACCUMULATOR( 'fpuv', "Plugin lookup with IHXValues" );

    // Initialize out params
    pIUnkResult = NULL;

    CHXSimpleList   PossibleValues;
    IHXValues*	    pPluginValues = NULL;
    IHXBuffer*	    pBuffer = NULL;
    CHXSimpleList::Iterator i = m_PluginList.Begin();

    for(; i!= m_PluginList.End(); ++i)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) *i;
	if (pPlugin->DoesMatch(pCriteria))
	{
	    PossibleValues.AddTail(pPlugin);
	}
    }

    HX_UPDATE_ACCUMULATOR( 'fpuv' );

    if (PossibleValues.Begin() == PossibleValues.End())
    {
	pIUnkResult = 0;
	return HXR_FAIL;
    }

    /****************************************************************
    ** Presently when we arrive at this spot with more than one
    ** plugin which matches the search criteria, we simply take
    ** the first one found. If this is not satisfactory then
    ** some method can be added which will process the list based
    ** upon some criteria.
    ****************************************************************/

    // if there are multiple plugins found, we will pick the one whose
    // plugin description contains "RealNetworks"
    if (PossibleValues.GetCount() > 1)
    {
	for(i = PossibleValues.Begin(); i!= PossibleValues.End(); ++i)
	{
	    BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) *i;
	    if (HXR_OK == pPlugin->GetPluginInfo(pPluginValues) && pPluginValues)
	    {
		if (HXR_OK == pPluginValues->GetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer) &&
		    pBuffer)
		{
		    if (strstr((const char*)pBuffer->GetBuffer(), "RealNetworks"))
		    {
			HX_RELEASE(pBuffer);
			if ( NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
			{
			    return HXR_OK;
			}
			else
			{
			    return HXR_FAIL;
			}
		    }
		}
		HX_RELEASE(pBuffer);
	    }
	}
    }

    BaseHandler::Plugin* pPlug = (BaseHandler::Plugin*) *(PossibleValues.Begin());
    Errors retVal = pPlug->GetInstance( pIUnkResult, pIUnkOuter );

    return ( retVal == NO_ERRORS ) ? HXR_OK : HXR_FAIL;
}


STDMETHODIMP
BaseHandler::FindPluginUsingStrings( char* PropName1, char* PropVal1,
					char* PropName2, char* PropVal2,
					char* PropName3, char* PropVal3,
					REF(IUnknown*) pIUnkResult,
					IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
	(PropName2 && !PropVal2)    ||
	(PropName3 && !PropVal3)    ||
	(!PropName1 && PropVal1)    ||
	(!PropName2 && PropVal2)    ||
	(!PropName3 && PropVal3))
	return HXR_FAIL;

    IHXValues*	pValues = NULL;
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


void
BaseHandler::UnloadDeadDLLs()
{
    LISTPOSITION pos = m_PluginList.GetHeadPosition();
    while( pos )
    {
        BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_PluginList.GetNext( pos );
        if( pPlugin )
        {
            PluginDLL * pPluginDll = pPlugin->GetDLL();
            if( pPluginDll )
            {
                pPluginDll->Unload();
            }
        }
    }
}

STDMETHODIMP
BaseHandler::GetPlugin( ULONG32 ulIndex, REF(IUnknown*) pIUnkResult,
					    IUnknown* pIUnkOuter )
{
    if( ulIndex <= (ULONG32)(m_PluginList.GetCount()-1) && m_PluginList.GetCount() )
    {
	LISTPOSITION pPos = m_PluginList.FindIndex( ulIndex );
	if (pPos)
	{
	    BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_PluginList.GetAt( pPos );
	    if( pPlugin )
	    {
		if (NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
		{
		    return HXR_OK;
		}
		else
		{
		    return HXR_FAIL;
		}
	    }
	}
    }
    return HXR_FAIL;

}


STDMETHODIMP
BaseHandler::UnloadPluginFromClassID(REFGUID GUIDClassID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
BaseHandler::UnloadPackageByName(const char* pName)
{
    return HXR_NOTIMPL;
}


//------------------------------------ Class Methods

STDMETHODIMP BaseHandler::FindPluginUsingStrings (char* PropName1,
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
BaseHandler::FindImplementationFromClassID
(
    REFGUID GUIDClassID,
    REF(IUnknown*) pIUnknownInstance
)
{
    // Defer to the new version
    return FindImplementationFromClassID( GUIDClassID, pIUnknownInstance, NULL, m_pContext );
}

STDMETHODIMP BaseHandler::Close ()
{
    CHXSimpleList::Iterator i = m_PluginList.Begin();

    // Release all Plugins and Their Associated DLLs
    for(; i!=m_PluginList.End(); ++i)
    {
	BaseHandler::Plugin* pPlug = (BaseHandler::Plugin*) *i;
	pPlug->Release();
    }
    m_PluginList.RemoveAll();

    for(i = m_PluginDLLList.Begin(); i!=m_PluginDLLList.End(); ++i)
    {
	BaseHandler::PluginDLL* pPlugDLL = (BaseHandler::PluginDLL*) *i;
	pPlugDLL->Release();
    }
    m_PluginDLLList.RemoveAll();

    for(i = m_MiscDLLList.Begin(); i!=m_MiscDLLList.End(); ++i)
    {
	BaseHandler::OtherDLL* pOtherDLL = (BaseHandler::OtherDLL*) *i;
	delete pOtherDLL;
    }
    m_MiscDLLList.RemoveAll();

    for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
    {
	BaseHandler::PluginMountPoint* pMountPoint = (BaseHandler::PluginMountPoint*) *mp;
	pMountPoint->Release();
    }
    m_MountPoints.RemoveAll();

    // release all of the CORE stuff...
    HX_RELEASE(m_pContext);
    m_bLoadStaticLinkedPlugins = FALSE;

    return HXR_OK;
}

STDMETHODIMP BaseHandler::SetRequiredPlugins (const char** ppszRequiredPlugins)
{
    return HXR_OK;
}

HX_RESULT BaseHandler::AddToValues(IHXValues* pValues, char* pPropName, void* pPropVal, eValueTypes eValueType)
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

STDMETHODIMP_(ULONG32) BaseHandler::GetNumOfPlugins()
{
    return m_PluginList.GetCount();
}

STDMETHODIMP BaseHandler::GetPlugin(ULONG32 ulIndex, REF(IUnknown*)  /*OUT*/ pInstance)
{
    return GetPlugin( ulIndex, pInstance, NULL );
}


STDMETHODIMP BaseHandler::FlushCache()
{
    return HXR_OK;
}

STDMETHODIMP BaseHandler::SetCacheSize(ULONG32 nSizeKB)
{
    return HXR_OK;
}

STDMETHODIMP BaseHandler::ReadFromRegistry()
{
    return HXR_OK;
}


HXBOOL BaseHandler::FindPlugin(const char* pFileName, UINT32 nDLLIndex, REF(UINT32) nIndex)
{
    UINT32 nTempIndex = 0;

    for(CHXSimpleList::Iterator i = m_PluginList.Begin(); i!=m_PluginList.End(); ++i)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) *i;
	IHXBuffer* pBuffer = pPlugin->GetFileName();
	char* pPluginFileName = (char*) pBuffer->GetBuffer();

	if (!strcasecmp(pPluginFileName, pFileName))
	{
	    if (pPlugin->GetIndex() == nDLLIndex)
	    {
		nIndex = nTempIndex;
		HX_RELEASE(pBuffer);
		return TRUE;
	    }
	}
	HX_RELEASE(pBuffer);
	nTempIndex++;
    }
    return FALSE;
}

/**********************************************************************************
***		    BaseHandler::Plugin					***
***********************************************************************************/


BaseHandler::Plugin::Plugin(IUnknown* pContext) :
	m_lRefCount(0)
    ,	m_pValues(0)
    ,	m_pPluginDLL(0)
    ,	m_pContext(pContext)
    , 	m_bInfoNeedsRefresh(FALSE)
    ,	m_nPluginIndex(0)
    ,	m_pClassFactory(NULL)
{
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
    			       (void**)&m_pClassFactory);

    CreateValuesCCF(m_pValues, m_pClassFactory);
}

BaseHandler::Plugin::~Plugin()
{
    HX_RELEASE(m_pValues);
    HX_RELEASE(m_pPluginDLL);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pContext);
    //HX_RELEASE(m_pPluginWatcher);
}

void BaseHandler::Plugin::SetPluginProperty(const char* pszPluginType)
{
    IHXBuffer* pBuffer = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    pBuffer->Set((UCHAR*)pszPluginType, strlen(pszPluginType)+1);
    m_pValues->SetPropertyCString(PLUGIN_CLASS, pBuffer);
    pBuffer->Release();
}


HXBOOL BaseHandler::Plugin::DoesMatch(IHXValues* pValues)
{
    CHXSimpleList   PossibleValues1;
    CHXSimpleList   PossibleValues2;
    const char*	    pPropName = NULL;
    ULONG32	    nInValue;
    ULONG32	    nOutValue;
    IHXBuffer*	    pInBuffer = NULL;
    IHXBuffer*	    pOutBuffer = NULL;

    // Check ULONGS 1st
    if (HXR_OK == pValues->GetFirstPropertyULONG32(pPropName, nInValue))
    {
	if (HXR_OK==m_pValues->GetPropertyULONG32(pPropName, nOutValue))
	{
	    if (nInValue != nOutValue)
	    {
		goto notFoundexit;
	    }
	}
	else
	{
	    goto notFoundexit;
	}
	while (HXR_OK == pValues->GetNextPropertyULONG32(pPropName, nInValue))
	{
	    if (HXR_OK == m_pValues->GetPropertyULONG32(pPropName, nOutValue))
	    {
		if (nInValue != nOutValue)
		{
		    goto notFoundexit;
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }
	}
    }

    // Test code to look at all of the data in the map.
#ifdef _DEBUG
    HX_RESULT tempresult;

    tempresult = HXR_OK;

    tempresult = m_pValues->GetFirstPropertyCString(pPropName, pInBuffer);
    HX_RELEASE(pInBuffer);

    while (tempresult == HXR_OK)
    {
	tempresult = m_pValues->GetNextPropertyCString(pPropName, pInBuffer);
	if (tempresult == HXR_OK)
	{
	    HX_RELEASE(pInBuffer);
	}
    }
#endif /*_DEBUG*/

    // Check String Props.
    if (HXR_OK == pValues->GetFirstPropertyCString(pPropName, pInBuffer))
    {
	if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
	{
	    if (!AreBufferEqual(pOutBuffer, pInBuffer))
	    {
		goto notFoundexit;
	    }
	}
	else
	{
	    goto notFoundexit;
	}

	HX_RELEASE(pInBuffer);
	HX_RELEASE(pOutBuffer);

	while (HXR_OK == pValues->GetNextPropertyCString(pPropName, pInBuffer))
	{
	    if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
	    {
		if ( !AreBufferEqual(pOutBuffer, pInBuffer))
		{
		    goto notFoundexit;
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }

	    HX_RELEASE(pInBuffer);
	    HX_RELEASE(pOutBuffer);
	}
    }


     // Check Buffer Properties
    if (HXR_OK == pValues->GetFirstPropertyBuffer(pPropName, pInBuffer))
    {
	// XXXND  Make some utility functions for doing this...
	if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
	{
	    if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
	    {
		if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
		{
		    goto notFoundexit;
		}
	    }
	}
	else
	{
	    goto notFoundexit;
	}

	HX_RELEASE(pInBuffer);
	HX_RELEASE(pOutBuffer);

	while (HXR_OK == pValues->GetNextPropertyBuffer(pPropName, pInBuffer))
	{
	    if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
	    {
		// XXXND  Make some utility functions for doing this...
		if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
		{
		    if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
		    {
			goto notFoundexit;
		    }
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }

	    HX_RELEASE(pInBuffer);
	    HX_RELEASE(pOutBuffer);
	}
    }

    return TRUE;    // we made it!

notFoundexit:
    HX_RELEASE(pInBuffer);
    HX_RELEASE(pOutBuffer);
    return FALSE;
}

void	BaseHandler::Plugin::SetDLL(PluginDLL * pPluginDll)
{
    m_pPluginDLL = pPluginDll;
    m_pPluginDLL->AddRef();

    IHXBuffer* pBuffer = pPluginDll->GetFileName();
    HX_ASSERT(pBuffer);

    m_pValues->SetPropertyCString(PLUGIN_FILENAME, pBuffer);
    HX_RELEASE(pBuffer);
}

void	BaseHandler::Plugin::SetIndex(UINT16 nIndex)
{
    m_nPluginIndex = nIndex;
    m_pValues->SetPropertyULONG32(PLUGIN_INDEX, nIndex);
}


void	BaseHandler::Plugin::SetPropertyULONG32(char* pName, char* pValue)
{
    if (m_pValues)
    {
	m_pValues->SetPropertyULONG32(pName, atoi(pValue));
    }
}

void	BaseHandler::Plugin::SetPropertyCString(char* pName, char* pValue)
{
    if (m_pValues)
    {
	IHXBuffer* pTempBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (UCHAR*)pValue, 
					    strlen(pValue)+1, m_pContext))
	{
	    m_pValues->SetPropertyCString(pName, pTempBuffer);
	    HX_RELEASE(pTempBuffer);
	}
    }
}

void	BaseHandler::Plugin::SetPropertyBuffer(char* pName, BYTE* pData, UINT32 size )
{
    if (m_pValues)
    {
	// XXXND  FIX  THis really shouldn't have to do this copy
	IHXBuffer* pTempBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pData, 
					    size, m_pContext))
	{
	    m_pValues->SetPropertyBuffer(pName, pTempBuffer);
	    HX_RELEASE(pTempBuffer);
	}
    }
}

HXBOOL BaseHandler::Plugin::AreBufferEqual(IHXBuffer* pBigBuff,
					   IHXBuffer* pSmallBuff)
{
    char*   pTemp;
    HXBOOL    bRetVal = FALSE;

    pTemp = new char[pBigBuff->GetSize()];
    strcpy(pTemp, (char*)pBigBuff->GetBuffer()); /* Flawfinder: ignore */

    char* token;
    token = strtok(pTemp, zm_pszValueSeperator);
    while (token)
    {
	CHXString tokenCHXstring;
	CHXString smallCHXstring;

	tokenCHXstring = token;
	smallCHXstring = (char*)pSmallBuff->GetBuffer();
	tokenCHXstring.TrimLeft();
	tokenCHXstring.TrimRight();
	smallCHXstring.TrimLeft();
	smallCHXstring.TrimRight();

	if (!strcasecmp(tokenCHXstring, smallCHXstring))
	{
	    bRetVal = TRUE;
	    break;
	}
	token = strtok(NULL, zm_pszValueSeperator);
    }
    delete[] pTemp;

    return bRetVal;
}

BaseHandler::Errors BaseHandler::Plugin::GetValuesFromDLL(IHXPlugin* pHXPlugin)
{
    BaseHandler::Errors  retVal;

    retVal = GetBasicValues(pHXPlugin);
    if (retVal == NO_ERRORS)
    {
	retVal = GetExtendedValues(pHXPlugin);
    }
    return retVal;
}

BaseHandler::Errors BaseHandler::Plugin::GetPlugin(REF(IUnknown*) pUnknown )
{
    pUnknown = NULL;
    BaseHandler::Errors retVal = NO_ERRORS;

    if (!m_pPluginDLL)
    {
	return PLUGIN_NOT_FOUND;
    }
    if (!m_pPluginDLL->IsLoaded())
    {
	if (NO_ERRORS != (retVal = m_pPluginDLL->Load(m_pContext)))
	{
	    return retVal;
	}
    }
    if (HXR_OK != m_pPluginDLL->CreateInstance(&pUnknown, m_nPluginIndex))
    {
	return CREATE_INSTANCHXR_FAILURE;
    }

    return retVal;
}

BaseHandler::Errors BaseHandler::Plugin::GetInstance(REF(IUnknown*) pUnknown, IUnknown* pIUnkOuter )
{
    HX_RESULT  hr = HXR_FAIL;
    IHXPlugin* pIHXPlugin = NULL;
    IHXComponentPlugin* pComponentPlugin = NULL;

    // Initialize out parameter
    pUnknown = NULL;

    IUnknown* pUnkPlugin = NULL;
    BaseHandler::Errors retVal = GetPlugin( pUnkPlugin );
    if( retVal == NO_ERRORS )
    {
        if( SUCCEEDED( pUnkPlugin->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
        {
            pIHXPlugin->InitPlugin(m_pContext);
        }

	if( SUCCEEDED( pUnkPlugin->QueryInterface( IID_IHXComponentPlugin, (void**) &pComponentPlugin ) ) )
	{
	    // Ask for the correct object by CLSID
	    IHXBuffer* pCLSID = NULL;
	    if( SUCCEEDED( m_pValues->GetPropertyBuffer( PLUGIN_COMPONENT_CLSID, pCLSID ) ) )
	    {
		hr = pComponentPlugin->CreateComponentInstance( *(GUID*) pCLSID->GetBuffer(), pUnknown, pIUnkOuter );
		HX_RELEASE( pCLSID );
	    }
	    else
	    {
		// component plugins must have CLSID
                HX_ASSERT(false);
	    }

	    HX_RELEASE( pComponentPlugin );
	}
	else
	{
	    if( !pIUnkOuter )
	    {
		pUnknown = pUnkPlugin;
                pUnknown->AddRef();
                hr = HXR_OK;
	    }
            else
            {
                // we can't aggregate anything because this is not a component plugin
                HX_ASSERT(false);
            }
	}
    }

    HX_RELEASE(pIHXPlugin);
    HX_RELEASE(pUnkPlugin);

    if (HXR_OK != hr)
    {
        retVal = CREATE_INSTANCHXR_FAILURE;
    }

    return retVal;
}

HXBOOL	BaseHandler::Plugin::IsLoaded()
{
    if (!m_pPluginDLL)
	return FALSE;

    return m_pPluginDLL->IsLoaded();
}

HX_RESULT BaseHandler::Plugin::GetPluginInfo(REF(IHXValues*) pVals)
{
    if (m_pValues)
    {
	pVals = m_pValues;
	return HXR_OK;
    }
    pVals = NULL;
    return HXR_FAIL;
}

IHXBuffer* BaseHandler::Plugin::GetFileName()
{
    IHXBuffer* retVal = NULL;

    // Get the filename from m_pValues.  We can't get it from the DLL,
    // because we may be in the process of loading, and we just got it
    // from the preferences
    if( m_pValues )
    {
	m_pValues->GetPropertyCString( PLUGIN_FILENAME, retVal );
    }

    return retVal;
}

BaseHandler::Errors
BaseHandler::Plugin::GetBasicValues(IHXPlugin* pHXPlugin)
{
    return NO_ERRORS;

    const char*	pszDescription = NULL;
    const char* pszCopyright = NULL;
    const char* pszMoreInfoUrl = NULL;
    ULONG32	ulVersionNumber = 0;
    HXBOOL	nload_multiple = 0;

    if (HXR_OK != pHXPlugin->GetPluginInfo(nload_multiple, pszDescription,
				       pszCopyright, pszMoreInfoUrl, ulVersionNumber))
    {
        return BAD_PLUGIN;
    }

    IHXBuffer* pBuffer = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszDescription)
    {
	pBuffer->Set((UCHAR*)pszDescription, strlen(pszDescription)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer);
    pBuffer->Release();
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszCopyright)
    {
	pBuffer->Set((UCHAR*)pszCopyright, strlen(pszCopyright)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT2, pBuffer);
    pBuffer->Release();
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszMoreInfoUrl)
    {
	pBuffer->Set((UCHAR*)pszMoreInfoUrl, strlen(pszMoreInfoUrl)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT, pBuffer);
    pBuffer->Release();

    m_pValues->SetPropertyULONG32(PLUGIN_LOADMULTIPLE, nload_multiple);
    m_pValues->SetPropertyULONG32(PLUGIN_VERSION, ulVersionNumber);
    return NO_ERRORS;
}


BaseHandler::Errors
BaseHandler::Plugin::GetExtendedValues(IHXPlugin* pHXPlugin)
{
//    Errors				result		    = NO_ERRORS;
    IHXFileFormatObject*		pFileFormat	    = NULL;
//    IHXMetaFileFormatObject*		pMetaFileFormat	    = NULL;
    IHXFileWriter*			pFileWriter	    = NULL;
    IHXBroadcastFormatObject*		pBroadcastFormat    = NULL;
    IHXFileSystemObject*		pFileSystem	    = NULL;
    IHXRenderer*			pRenderer	    = NULL;
    IHXDataRevert*			pDataRevert	    = NULL;
    IHXStreamDescription*		pStreamDescription  = NULL;
    IHXPlayerConnectionAdviseSink*	pAllowanceFormat    = NULL;
    IHXCommonClassFactory*		pClassFactory       = NULL;
    IHXPluginProperties*		pIHXPluginPropertiesThis = NULL;
    UINT32				nCountInterfaces    = 0;


    // file system
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileSystemObject, (void**) &pFileSystem))
    {
	const char* pszShortName;
	const char* pszProtocol;

	if (HXR_OK != pFileSystem->GetFileSystemInfo(pszShortName, pszProtocol))
	{
	    HX_RELEASE (pFileSystem);
	    return  CANT_GET_RENDERER_INFO; //XXXAH Cleanup?
	}

	SetPluginProperty(PLUGIN_FILESYSTEM_TYPE);

	IHXBuffer* pBuffer = NULL;
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszShortName)
	{
	    pBuffer->Set((UCHAR*)pszShortName, strlen(pszShortName)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_FILESYSTEMSHORT, pBuffer);
	pBuffer->Release();
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszProtocol)
	{
	    pBuffer->Set((UCHAR*)pszProtocol, strlen(pszProtocol)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_FILESYSTEMPROTOCOL, pBuffer);
	pBuffer->Release();

	pFileSystem->Release();
	nCountInterfaces++;
    }

    // file format
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat) ||
	HXR_OK == pHXPlugin->QueryInterface(IID_IHXMetaFileFormatObject, (void**)&pFileFormat) ||
	HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
    {
	// fine we are in now we will get the correct type.
	if (pFileFormat)
	{
	    pFileFormat->Release();
	}
	else
	{
	    pFileWriter->Release();
	}

	IHXMetaFileFormatObject* pMetaFileFormat;

	const char**		ppszMimeTypes = NULL;
	const char**		ppszExtensions = NULL;
	const char**		ppszOpenNames = NULL;

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat))
	{
	    pFileFormat->GetFileFormatInfo( ppszMimeTypes,
					    ppszExtensions,
					    ppszOpenNames);
	    pFileFormat->Release();
	    SetPluginProperty(PLUGIN_FILEFORMAT_TYPE);
	}

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXMetaFileFormatObject, (void**)&pMetaFileFormat))
	{
	    pMetaFileFormat->GetMetaFileFormatInfo( ppszMimeTypes,
						    ppszExtensions,
						    ppszOpenNames);
	    pMetaFileFormat->Release();

	    SetPluginProperty(PLUGIN_METAFILEFORMAT_TYPE);
	}

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
	{
	    pFileWriter->GetFileFormatInfo( ppszMimeTypes,
					    ppszExtensions,
					    ppszOpenNames);
	    pFileWriter->Release();

	    SetPluginProperty(PLUGIN_FILEWRITER_TYPE);
	}

	IHXBuffer* pBuffer = NULL;
	if (ppszMimeTypes)
	{
            CatStringsCCF(pBuffer, ppszMimeTypes, zm_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEMIMETYPES, pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (ppszExtensions)
	{
            CatStringsCCF(pBuffer, ppszExtensions, zm_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEEXTENSIONS, pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (ppszOpenNames)
	{
            CatStringsCCF(pBuffer, ppszOpenNames, zm_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEOPENNAMES, pBuffer);
	    HX_RELEASE(pBuffer);
	}
	nCountInterfaces++;
    }

    // renderer
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXRenderer, (void**)&pRenderer))
    {
	const char**	ppszMimeTypes = NULL;
        UINT32	initial_granularity = 0;

	// get the basic info
	if (HXR_OK == pRenderer->GetRendererInfo((const char**&)ppszMimeTypes, initial_granularity))
	{
	    IHXBuffer* pBuffer = NULL;
	    if (ppszMimeTypes)
	    {
		CatStringsCCF(pBuffer, ppszMimeTypes, zm_pszValueSeperator, m_pContext);
	    }
	    m_pValues->SetPropertyCString(PLUGIN_RENDERER_MIME, pBuffer);
	    pBuffer->Release();
	    m_pValues->SetPropertyULONG32(PLUGIN_RENDERER_GRANULARITY, initial_granularity);
	    SetPluginProperty(PLUGIN_RENDERER_TYPE);
	}

	HX_RELEASE(pRenderer);
	nCountInterfaces++;
    }

    // stream description
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXStreamDescription, (void**)&pStreamDescription))
    {
	const char* pszMimeType;
	IHXBuffer* pBuffer;
	if (HXR_OK != pStreamDescription->GetStreamDescriptionInfo(pszMimeType))
	{
	    HX_RELEASE (pStreamDescription);
	    return CANT_GET_FILE_FORMAT_INFO;	// XXXAH Cleanup?
	}
	pStreamDescription->Release();
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszMimeType)
	{
	    pBuffer->Set((UCHAR*)pszMimeType, strlen(pszMimeType)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_STREAMDESCRIPTION, pBuffer);
	pBuffer->Release();

	SetPluginProperty(PLUGIN_STREAM_DESC_TYPE);
	nCountInterfaces++;
    }

    // common class factory
    if(HXR_OK == pHXPlugin->QueryInterface(IID_IHXCommonClassFactory,
					(void**)&pClassFactory))
    {
	SetPluginProperty(PLUGIN_CLASS_FACTORY_TYPE);
	HX_RELEASE (pClassFactory);
	nCountInterfaces++;
    }

    // NO MORE NEW PLUGIN INFORMATION INTERFACES!!!!
    //
    // THIS IS THE LAST ONE!!!!!
    if( SUCCEEDED( pHXPlugin->QueryInterface( IID_IHXPluginProperties, (void**)&pIHXPluginPropertiesThis ) ) )
    {
	IHXValues* pIHXValuesProperties = NULL;

	pHXPlugin->InitPlugin(m_pContext);

	if( SUCCEEDED( pIHXPluginPropertiesThis->GetProperties( pIHXValuesProperties ) ) && pIHXValuesProperties )
	{
	    CHXHeader::mergeHeaders( m_pValues, pIHXValuesProperties );
	}

	HX_RELEASE(pIHXValuesProperties);

	// XXXkshoop Let this coincide with other interfaces.. for now..
	//nCountInterfaces++;
    }

    HX_RELEASE(pIHXPluginPropertiesThis);

    HX_ASSERT(nCountInterfaces<2);
    return NO_ERRORS;
}

void BaseHandler::Plugin::InitializeComponentPlugin( IHXPlugin* pIPlugin, IHXValues* pIValues )
{
    // Setup basic data
    // XXXHP - this is unnecessary information as it is stored on a PER COMPONENT not PER PLUGIN basis in this case.
    // GetBasicValues( pIPlugin );

    // Copy data from pIValues
    CHXHeader::mergeHeaders( m_pValues, pIValues );
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::Plugin::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
BaseHandler::Plugin::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::Plugin::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BaseHandler::Plugin::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::Plugin::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BaseHandler::Plugin::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/**********************************************************************************
***		    BaseHandler::PluginDLL					***
***********************************************************************************/

BaseHandler::PluginDLL::PluginDLL( const char* pszFileName, PluginMountPoint* pMountPoint,
					BaseHandler* pBaseHandler )
    : m_fpCreateInstance(NULL)
    , m_fpShutdown(NULL)
    , m_fCanUnload(NULL)
    , m_pMountPoint( pMountPoint )
    , m_pFileName( NULL )
    , m_pNamespace( NULL )
    , m_nSizeBites(0)
    , m_lRefCount(0)
    , m_NumOfPlugins(0)
    , m_pDLLAccess(NULL)
    , m_bHas_factory(FALSE)
    , m_bLoaded(FALSE)
    , m_nActiveReferences(0)
    , m_pBaseHandler(pBaseHandler)
    , m_bDoesExist(TRUE)
{
    // Always create an IHXBuffer and store the filename there
    CreateBufferCCF(m_pFileName, m_pBaseHandler->m_pContext);
    if (m_pFileName)
    {
	// Make sure there are no path components in the filename
	HX_ASSERT( !strrchr( pszFileName, BaseHandler::zm_cDirectorySeperator ) );

	m_pFileName->Set( (BYTE*) pszFileName, ::strlen( pszFileName ) + 1 );
    }


    m_pDLLAccess = new DLLAccess();
}


BaseHandler::PluginDLL::~PluginDLL()
{
    HX_RELEASE( m_pFileName );
    HX_RELEASE( m_pNamespace );

    if (m_pDLLAccess)
    {
        if (m_bLoaded)
	{
	    if (m_fpShutdown)
	    {
		m_fpShutdown();
		m_fpShutdown = NULL;
	    }

	    m_pDLLAccess->close();
	}

	delete m_pDLLAccess;
	m_pDLLAccess = 0;
    }
}


BaseHandler::Errors
BaseHandler::PluginDLL::Load(IUnknown* pContext)
{
    HX_LOG_BLOCK( "BaseHandler::PluginDLL::Load()" );

    Errors	result	    = NO_ERRORS;

    IUnknown*	pInstance   = NULL;
    IHXPlugin* pPlugin = NULL;
    IHXPluginFactory* pIFactory = NULL;


    if (m_bLoaded)
    {
	return PLUGIN_ALREADY_HAS_MOUNT_POINT; //XXXAH Huh?
    }

    if( m_pFileName->GetSize() <= 1 )
    {
	return PLUGIN_NOT_FOUND;
    }

    // Build complete path for DLL
    CHXString fileNameWithPath;

#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    // we're not statically linked so we must use the mount point path
    IHXBuffer* pBuffer = m_pMountPoint->Path();
    if (pBuffer)
    {
	fileNameWithPath = (char*)pBuffer->GetBuffer();
	UINT32 len = fileNameWithPath.GetLength();
	if (len && fileNameWithPath.GetAt(len - 1) != BaseHandler::zm_cDirectorySeperator)
	{
	    fileNameWithPath += BaseHandler::zm_pszDirectorySeperator;
	}
	HX_RELEASE(pBuffer);
    }
#endif

    fileNameWithPath += (char*) m_pFileName->GetBuffer();

    // 1st load the DLL into memory
    HX_PRIME_ACCUMULATOR( 'pdll', "Total DLL Load Time" );
    int dllLoadResult = m_pDLLAccess->open(fileNameWithPath);
    HX_UPDATE_ACCUMULATOR( 'pdll' );

    if( dllLoadResult != DLLAccess::DLL_OK )
    {
	m_pBaseHandler->ReportError( HXLOG_DEBUG, (char *) m_pFileName->GetBuffer(), m_pDLLAccess->getErrorString() );
	return CANT_OPEN_DLL;
    }

    HX_LOG_CHECKPOINT( "DLL Loaded" );

    // Now look for the HXCreateInstance exported function
    m_fpCreateInstance = (FPCREATEINSTANCE) m_pDLLAccess->getSymbol(HXCREATEINSTANCESTR);
    if (NULL == m_fpCreateInstance)
    {
	m_pBaseHandler->ReportError( HXLOG_DEBUG, "NO HXCreateInstance", NULL );
	return NO_HX_CREATE_INSTANCE;
    }

    // And look for HXShutdown exported function... not required.
    m_fpShutdown    = (FPSHUTDOWN) m_pDLLAccess->getSymbol(HXSHUTDOWNSTR);

    // and look for CanUnload2 exported function
    //JE 3/26/01: look for CanUnload2 instead of CanUnload. This way we will
    //not try and unload any DLLs that may have incorrectly implemented the old
    // CanUnload. If you implement CanUnload2, you better get it right ;)
    m_fCanUnload    = (FPSHUTDOWN) m_pDLLAccess->getSymbol("CanUnload2");

    HX_LOG_CHECKPOINT( "Exported symbols found" );

    // Does this thing support the IHXPlugin Interface
    // Now we will test to see if the DLL contains multiple Plugins.
    HX_PRIME_ACCUMULATOR( 'plmk', "Total Plugin Allocation time" );
    HX_RESULT createResult = m_fpCreateInstance( &pInstance );
    HX_UPDATE_ACCUMULATOR( 'plmk' );

    if( HXR_OK != createResult )
    {
	m_pBaseHandler->ReportError( HXLOG_DEBUG, "HXCreateInstance Failure", NULL );
	result = CREATE_INSTANCHXR_FAILURE;
	goto cleanup;
    }


    HX_LOG_CHECKPOINT( "Plugin instance created" );

    // To be a valid plugin a DLL must support IHXPlugin
    // In addition, it may support IHXPluginFactory
    if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPluginFactory, (void**) &pIFactory ) ) )
    {
	m_bHas_factory = TRUE;
	m_NumOfPlugins = pIFactory->GetNumPlugins();

	HX_RELEASE( pIFactory );
    }
    else if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPlugin, (void**) &pPlugin ) ) )
    {
	m_bHas_factory = FALSE;
	m_NumOfPlugins = 1;

	IHXComponentPlugin* pIPackage = NULL;
	if (SUCCEEDED (pInstance->QueryInterface (IID_IHXComponentPlugin, (void**) &pIPackage)))
	{
#ifndef _WINDOWS // XXXHP TEMPORARY - viper team has requested that this shouldn't assert for now on windows.
	    HX_ASSERT (m_fpShutdown);
#endif
	    pPlugin->InitPlugin (pContext);
	    m_packageName = pIPackage->GetPackageName ();
	    HX_RELEASE (pIPackage);
	}

	HX_RELEASE( pPlugin );
    }
    else
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    // We are now loaded.
    HX_RELEASE(pInstance);
    m_bLoaded = TRUE;

cleanup:

    HX_LOG_CHECKPOINT( "BaseHandler::PluginDLL::Load() exiting" );

    return result;
}


HX_RESULT BaseHandler::PluginDLL::Unload(HXBOOL safe)
{
    if (m_bLoaded)
    {
	if (!safe || ( m_fCanUnload && m_fCanUnload()==HXR_OK ) )
	{
	    if (m_fpShutdown)
	    {
		if (FAILED (m_fpShutdown ())) return HXR_FAIL;
		m_fpShutdown = NULL;
	    }

            if (DLLAccess::DLL_OK == m_pDLLAccess->close())
	    {
		m_bLoaded = FALSE;
		return HXR_OK;
	    }
	}
    }
    return HXR_FAIL;
}

HXBOOL BaseHandler::PluginDLL::IsLoaded()
{
    return m_bLoaded;
}

BaseHandler::Errors	BaseHandler::PluginDLL::CreateInstance(IUnknown** ppUnk,
								 UINT32 uIndex)
{
    if (!m_bLoaded)
	return PLUGIN_NOT_FOUND;

    if (!m_bHas_factory)
    {
	if (HXR_OK != m_fpCreateInstance(ppUnk))
	{
	    return CREATE_INSTANCHXR_FAILURE;
	}
    }
    else
    {
	if (uIndex > (ULONG32)(m_NumOfPlugins-1) && m_NumOfPlugins)
	{
	    return CANT_LOAD_INTERFACE;
	}
	IUnknown*		pUnk;
	IHXPluginFactory*	pPluginFactory;

	m_fpCreateInstance(&pUnk);
	if (HXR_OK != pUnk->QueryInterface
	(IID_IHXPluginFactory, (void**) &pPluginFactory))
	{
	    HX_RELEASE(pUnk);
	    return CREATE_INSTANCHXR_FAILURE;
	}
	else
	{
	    HX_RELEASE(pUnk);
	    if (HXR_OK != pPluginFactory->GetPlugin((UINT16)uIndex, ppUnk))
	    {
		HX_RELEASE(pPluginFactory);
		return CREATE_INSTANCHXR_FAILURE;
	    }
	    HX_RELEASE(pPluginFactory);
	}
    }
    //m_pPlugin2Handler->AddtoLRU(this);
    //m_pPlugin2Handler->UpdateCache();
    return NO_ERRORS;
}

IHXBuffer* BaseHandler::PluginDLL::GetFileName()
{
    m_pFileName->AddRef();
    return m_pFileName;
}

IHXBuffer* BaseHandler::PluginDLL::GetNamespace()
{
    if (m_pNamespace)
    {
	m_pNamespace->AddRef();
    }

    return m_pNamespace;
}

void BaseHandler::PluginDLL::SetNamespace(IHXBuffer* pNamespace)
{
    m_pNamespace = pNamespace;

    if (m_pNamespace)
    {
	m_pNamespace->AddRef();
    }
}

UINT32 BaseHandler::PluginDLL::AddDLLReference()
{
    return ++m_nActiveReferences;
}


UINT32 BaseHandler::PluginDLL::ReleaseDLLReference()
{
    --m_nActiveReferences;
    Unload();	    //XXXAH this should only happen if we have set DLL unloading to true.

    return 0;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::PluginDLL::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
BaseHandler::PluginDLL::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::PluginDLL::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BaseHandler::PluginDLL::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseHandler::PluginDLL::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
BaseHandler::PluginDLL::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/**********************************************************************************
***		    BaseHandler::PluginMountPoint				***
***********************************************************************************/

BaseHandler::PluginMountPoint::PluginMountPoint( IUnknown* pContext, BaseHandler* pHandler, const char* pName,
						 UINT32 majorVersion, UINT32 minorVersion, IHXBuffer* pPath ) :
    m_lRefCount( 0 )
{
#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    m_pPath = pPath;
    if (m_pPath)
    {
	m_pPath->AddRef();
    }
#endif
}


#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
IHXBuffer*
BaseHandler::PluginMountPoint::Path()
{
    if (m_pPath)
    {
	m_pPath->AddRef();
    }
    return m_pPath;
}
#endif

BaseHandler::PluginMountPoint::~PluginMountPoint()
{
#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    HX_RELEASE(m_pPath);
#endif
}


STDMETHODIMP_(ULONG32)
BaseHandler::PluginMountPoint::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
BaseHandler::PluginMountPoint::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// BaseHandler::CheckDirectory
//
///////////////////////////////////////////////////////////////////////////////

BaseHandler::Errors BaseHandler::Stat(const char* pszFilename, struct stat* pStatBuffer)
{
    CHXString	strFileName;

    memset(pStatBuffer,0,sizeof(*pStatBuffer));
#if !defined(_STATICALLY_LINKED)
    if(stat(pszFilename, pStatBuffer) < 0)
	return CANT_OPEN_DLL;
#endif
    pStatBuffer->st_atime = 0;
    return NO_ERRORS ;
}


IHXBuffer* BaseHandler::ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen)
{
    char* pszOut = new char[nBuffLen*2+1];
    char* pszStartOut = pszOut;
    char Nibble;

    IHXBuffer* pOutBuffer = NULL;
    if (HXR_OK == CreateBufferCCF(pOutBuffer, m_pContext))
    {
	for (int i = 0; i<(int)nBuffLen; i++)
	{
	    Nibble = (*pBuffer >> 4) & 15;
	    *pszOut= (Nibble > 9 ) ? Nibble+55 : Nibble +48;
	    pszOut++;
	    Nibble = *pBuffer & 15;
	    *pszOut= (Nibble> 9 ) ? Nibble+ 55 : Nibble+48;
	    pszOut++;
	    pBuffer++;
	}
	*pszOut = 0;
	pOutBuffer->Set((UCHAR*)pszStartOut, strlen(pszStartOut)+1);
    }

    delete[] pszStartOut;
    return pOutBuffer;
}


IHXBuffer* BaseHandler::ChecksumFile(char* pszFileName, IHXBuffer* pPathBuffer)
{
    return ConvertToAsciiString("abc", 3);
}

/********************************************************************
*
*	Plugin Enumeration
*
********************************************************************/

BEGIN_INTERFACE_LIST_NOCREATE( CPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginSearchEnumerator )
END_INTERFACE_LIST

CPluginEnumerator::CPluginEnumerator() :
    m_nIndex(0)
{
}

CPluginEnumerator::~CPluginEnumerator()
{
}


STDMETHODIMP_( UINT32 )
CPluginEnumerator::GetNumPlugins()
{
    return m_ListOfPlugins.GetCount();
}

STDMETHODIMP_( void )
CPluginEnumerator::GoHead()
{
    m_nIndex = 0;
}


STDMETHODIMP
CPluginEnumerator::GetNextPlugin( REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    HX_RESULT res = GetPluginAt( m_nIndex, pIUnkResult, pIUnkOuter );
    m_nIndex++;

    return res;
}

STDMETHODIMP
CPluginEnumerator::GetNextPluginInfo( REF(IHXValues*) pRetValues )
{
    // Initialize out params
    pRetValues = NULL;

    HX_RESULT res = GetPluginInfoAt( m_nIndex, pRetValues );
    m_nIndex++;

    return res;
}


STDMETHODIMP
CPluginEnumerator::GetPluginAt( UINT32 index, REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_ListOfPlugins.FindIndex(index);
    if (pos)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_ListOfPlugins.GetAt(pos);
	if (pPlugin)
	{
	    if (BaseHandler::NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
	    {
		res = HXR_OK;
	    }
	}
    }
    return res;
}


STDMETHODIMP
CPluginEnumerator::GetPluginInfoAt( UINT32 index, REF(IHXValues*) pRetValues )
{
    // Initialize out params
    pRetValues = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_ListOfPlugins.FindIndex(m_nIndex);
    m_nIndex++;
    if (pos)
    {
	BaseHandler::Plugin* pPlugin = (BaseHandler::Plugin*) m_ListOfPlugins.GetAt(pos);
	if (pPlugin)
	{
	    res = pPlugin->GetPluginInfo( pRetValues );
	}
    }

    return res;
}


void CPluginEnumerator::Add(BaseHandler::Plugin* pPlugin)
{
    IHXValues*	    pPluginValues   = NULL;
    IHXBuffer*	    pBuffer	    = NULL;
    HXBOOL	    bAdded	    = FALSE;

    if (HXR_OK == pPlugin->GetPluginInfo(pPluginValues) && pPluginValues)
    {
	if (HXR_OK == pPluginValues->GetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer) &&
	    pBuffer)
	{
	    if (strstr((const char*)pBuffer->GetBuffer(), "RealNetworks"))
	    {
		m_ListOfPlugins.AddHead(pPlugin);
		bAdded = TRUE;
	    }
	}
	HX_RELEASE(pBuffer);
    }
    if (!bAdded)
    {
	m_ListOfPlugins.AddTail(pPlugin);
    }
}

HX_RESULT CPluginEnumerator::GetNext(REF(IUnknown*) pRetUnk)
{
    pRetUnk = NULL;
    return GetNextPlugin( pRetUnk, NULL );
}
