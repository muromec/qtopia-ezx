/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdll.cpp,v 1.15 2007/07/06 21:58:19 jfinnecy Exp $
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

// define all guids here once...
#include "hxver.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxassert.h"
#include "hxresult.h"
#include "hxplugn.h"

#ifdef HELIX_FEATURE_EXTENDED_MEDIAPLATFORM
#include "chxmedpltfmex.h"
#else
#include "chxmedpltfm.h"
#endif

#include "dllpath.h"

#include "hxccf.h"
#include "hxcore.h"
#include "hxslist.h"
#include "pckunpck.h"
#include "hxdir.h"
#include "safestring.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

ENABLE_DLLACCESS_PATHS(HXMediaPlatform);

#if !defined(HELIX_CONFIG_NOSTATICS)
CHXSimpleList*			g_pMediaPlatformList = NULL;
HXBOOL				g_bMediaPlatformOpened = FALSE;
// legacy support
CHXMediaPlatform*		g_pMediaPlatform = NULL;
IHXClientEngine*		g_pEngine = NULL;
UINT16				g_uNumEngines = 0;
#else
#include "globals/hxglobals.h"
const CHXSimpleList*	const	g_pMediaPlatformList = NULL;
const HXBOOL			g_bMediaPlatformOpened = FALSE;
// legacy support
const CHXMediaPlatform*	const	g_pMediaPlatform = NULL;
const IHXClientEngine*	const	g_pEngine = NULL;
const UINT16			g_uNumEngines = 0;
#endif

HX_RESULT 
_HXMediaPlatformOpen(void)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    HXBOOL& g_bMediaPlatformOpened = 
        (HXBOOL&)HXGlobalBool::Get(&::g_bMediaPlatformOpened);
#endif

    // HXMediaPlatform can NOT be initialized in mixed mode(new vs. legacy)
    if (g_uNumEngines)
    {
	return HXR_FAILED;
    }

    g_bMediaPlatformOpened = TRUE;

    return HXR_OK;
}

HX_RESULT
_HXCreateMediaPlatform(IHXMediaPlatform** ppMediaPlatform)
{
    HX_RESULT		rc = HXR_OK;
    CHXMediaPlatform*	pMediaPlatform = NULL;

#if defined(HELIX_CONFIG_NOSTATICS)
    CHXSimpleList*& g_pMediaPlatformList =
        (CHXSimpleList*&)HXGlobalPtr::Get(&::g_pMediaPlatformList);
    HXBOOL& g_bMediaPlatformOpened = 
        (HXBOOL&)HXGlobalBool::Get(&::g_bMediaPlatformOpened);
#endif
    
    *ppMediaPlatform = NULL;

    // HXMediaPlatformOpen needs to be called first
    if (!g_bMediaPlatformOpened)
    {
	return HXR_UNEXPECTED;
    }

    if (!g_pMediaPlatformList)
    {
	g_pMediaPlatformList = new CHXSimpleList();
	if (!g_pMediaPlatformList)
	{
	    return HXR_OUTOFMEMORY;
	}
    }

    pMediaPlatform = CHXMediaPlatform::CreateInstance();

    if (!pMediaPlatform)
    {
	return HXR_OUTOFMEMORY;
    }

    rc = pMediaPlatform->QueryInterface(IID_IHXMediaPlatform, (void**)ppMediaPlatform);
    HX_ASSERT(HXR_OK == rc);

    // internal AddRef() to make sure all instances will be cleaned up in
    // HXMediaPlatformClose()
    pMediaPlatform->AddRef();
    g_pMediaPlatformList->AddTail(pMediaPlatform);
    
    return rc;
}

HX_RESULT
_HXMediaPlatformClose(void)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    HXBOOL& g_bMediaPlatformOpened = 
        (HXBOOL&)HXGlobalBool::Get(&::g_bMediaPlatformOpened);
    CHXSimpleList*& g_pMediaPlatformList =
        (CHXSimpleList*&)HXGlobalPtr::Get(&::g_pMediaPlatformList);
#endif

    if (g_pMediaPlatformList)
    {
	CHXSimpleList::Iterator i;
	for (i = g_pMediaPlatformList->Begin(); i != g_pMediaPlatformList->End(); ++i)
	{
	    CHXMediaPlatform* pMediaPlatform = (CHXMediaPlatform*) (*i);
	    pMediaPlatform->Close();
	    HX_RELEASE(pMediaPlatform);
	}

	HX_DELETE(g_pMediaPlatformList);
    }

    g_bMediaPlatformOpened = FALSE;

    return HXR_OK;
}

IHXBuffer*  
GetDefaultPluginDir(IUnknown* pContext)
{
    IHXBuffer*	lpBuffer	= NULL;
    char mask_name[_MAX_PATH + 1] = ""; /* Flawfinder: ignore */

#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    if (!GetSystemDirectory(mask_name, _MAX_PATH))
    {
	strcpy(mask_name, ""); /* Flawfinder: ignore */
    }

    if (strlen(mask_name) > 0 && mask_name[strlen(mask_name) - 1] != OS_SEPARATOR_CHAR)
    {
	SafeStrCat(mask_name,  OS_SEPARATOR_STRING, _MAX_PATH+1);
    }

    SafeStrCat(mask_name, "Real", _MAX_PATH+1);
#elif defined (_UNIX) && !defined(_MAC_UNIX)
    SafeStrCpy(mask_name, getenv("HOME"), _MAX_PATH+1);
    SafeStrCat(mask_name, "/Real", _MAX_PATH+1);
#elif defined (_MACINTOSH) || defined(_MAC_UNIX)
    FSSpec extSpec;
    extSpec.name[0] = 0;

    // if Sys 8.5 or greater, use Application Support folder, else Extensions
    INT32	sysVersion;
    char*	bytes=(char*)&sysVersion;
    OSType 	folderType;

    ::Gestalt(gestaltSystemVersion,&sysVersion);
    if (bytes[2]>8 || ((bytes[2]==8) && (bytes[3] >= 0x50)))
    	folderType = kApplicationSupportFolderType;
    else
	folderType = kExtensionFolderType;

    if (noErr == ::FindFolder (-1, folderType, kDontCreateFolder,
				&extSpec.vRefNum, &extSpec.parID))
    {
	CHXString str_path;
	str_path = extSpec;
	SafeStrCpy(mask_name, (char*)(const char*)str_path, _MAX_PATH);
    }
    else
	SafeStrCpy(mask_name, ":System Folder:Extensions:", _MAX_PATH+1 );

    SafeStrCat(mask_name, "Real", _MAX_PATH+1);

#if defined(_CARBON) || defined(_MAC_UNIX)
    if (bytes[2] >= 0x10) // OS X
    {
#ifdef _MAC_MACHO
        CFBundleRef mainBundle;
        CFURLRef mainBundleURL;
        CFURLRef updirURL;
        CFBundleRef myBundle;

        // get the main bundle for the app
        mainBundle = ::CFBundleGetMainBundle();

        // look for a resource in the main bundle by name
        mainBundleURL = ::CFBundleCopyBundleURL( mainBundle );
        updirURL = ::CFURLCreateCopyDeletingLastPathComponent(NULL, mainBundleURL);

        CFStringRef urlString = CFURLCopyPath(updirURL);
        CFStringGetCString(urlString, mask_name, _MAX_PATH, kCFStringEncodingMacRoman);

#else
    	ProcessSerialNumber psn;
    	ProcessInfoRec pir;

    	GetCurrentProcess(&psn);
    	pir.processName = NULL;
    	pir.processAppSpec = &extSpec;
    	pir.processInfoLength = sizeof(pir);

    	GetProcessInformation(&psn, &pir);

    	extSpec.name[0] = '\0';

    	CHXString str_path;
    	str_path = extSpec;
    	SafeStrCpy(mask_name, (char*)(const char*)str_path, _MAX_PATH);
#endif
    }
#endif


#elif defined(_WINCE)
    strcpy(mask_name, "\\"); /* Flawfinder: ignore */
#endif //defined (_WINDOWS) || defined (_WIN32)

    CreateAndSetBufferCCF(lpBuffer, (UCHAR*)mask_name, strlen(mask_name)+1, pContext);
    return lpBuffer;
}

IHXBuffer* 
GetPluginDir(IUnknown* pContext)
{
    IHXBuffer* pPluginDir = NULL;

#ifdef _STATICALLY_LINKED
    CreateBufferCCF(pPluginDir, pContext);
    pPluginDir->Set((const UCHAR *)"",1);
#else
    const char* pPath = NULL;

    // Get the plugin directory from the Dll Access Paths
    pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
    if (!pPath || !pPath[0])
    {
	pPluginDir = GetDefaultPluginDir(pContext);
	GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN,
	    (const char*)pPluginDir->GetBuffer());
    }
    else
    {
	CreateBufferCCF(pPluginDir, pContext);
	pPluginDir->Set((const UCHAR*)pPath, strlen(pPath) + 1);

	//	Validate this path.
	//

#ifdef _MAC_CFM // XXXSEH: Revisit validation under Mach-O.
	//
	// Couldn't find a cross platform path validator, so I'll do it just for the Macintosh.
	// That's where this is most important anyways.
	//

	char	tempPath[1024]; /* Flawfinder: ignore */
	FSSpec	tempSpec;
	OSErr	err=0;
        UINT32      ulBytesToCopy = (m_pPluginDir->GetSize() > 1023 ? 1023 : m_pPluginDir->GetSize());
	memcpy(tempPath,m_pPluginDir->GetBuffer(),ulBytesToCopy); /* Flawfinder: ignore */
	tempPath[ulBytesToCopy]=0;

	err = FSSpecFromPathName(tempPath,&tempSpec);

	//
	//	Uhoh the Macintosh path validator could not resolve this
	//	path, thus we must refresh it.  Strange how we store this
	//  path but never expect it to change.
	//
	if (err != noErr)
	{
	    HX_RELEASE(m_pPluginDir);

	    m_pPluginDir = GetDefaultPluginDir();

	    GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN,
		(const char*)m_pPluginDir->GetBuffer());
	}
#endif
    }

    // CFindFile is kind of brain dead in that it will append a OS Seperator
    // after the path regardless of what is there currently (bad CFindFile bad!)
    // so we will strip it off if it is the last character,
    // Also all functions within the plugin handler assume that the plugin
    // directory will have not have an OS seperator at the end of it.

    char*	    pszPluginDir    = NULL;
    ULONG32	    nPluginDirLen   = 0;

    pPluginDir->Get((UCHAR*&)pszPluginDir, nPluginDirLen);

    // now we COULD (and should for speed) use nPluginDirLen-1 as the
    // length of the string. However, it is SLIGHTLY safer to use strlen

    if ( *(pszPluginDir+(strlen(pszPluginDir)-1)) == OS_SEPARATOR_CHAR)
    {
	*(pszPluginDir+(strlen(pszPluginDir)-1)) = 0;
    }
#endif // _STATICALLY_LINKED

    return pPluginDir;
}

STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(HXMediaPlatformOpen)(void)
{
    return _HXMediaPlatformOpen();
}

STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(HXCreateMediaPlatform)(IHXMediaPlatform** ppMediaPlatform)
{
    return _HXCreateMediaPlatform(ppMediaPlatform);
}

STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(HXMediaPlatformClose)(void)
{
    return _HXMediaPlatformClose();
}

STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(HXGetRootMediaPlatform)(IHXMediaPlatform**	ppMediaPlatform)
{
    *ppMediaPlatform = NULL;
    if(!g_pMediaPlatformList || g_pMediaPlatformList->IsEmpty())
	return HXR_FAILED;

    *ppMediaPlatform = (IHXMediaPlatform*)g_pMediaPlatformList->GetHead();
    if(*ppMediaPlatform)
    {
	(*ppMediaPlatform)->AddRef();
	return HXR_OK;
    }

    return HXR_FAILED;
}

STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(CreateEngine)(IHXClientEngine** ppEngine)
{
    HX_RESULT		    rc = HXR_OK;
    IHXBuffer*		    pIPluginDir = NULL;
    IHXCommonClassFactory*  pCCF = NULL;

#if defined(HELIX_CONFIG_NOSTATICS)
    HXBOOL& g_bMediaPlatformOpened = 
        (HXBOOL&)HXGlobalBool::Get(&::g_bMediaPlatformOpened);
    UINT16& g_uNumEngines = (UINT16&)HXGlobalInt16::Get(&::g_uNumEngines);
    CHXSimpleList*& g_pMediaPlatformList =
        (CHXSimpleList*&)HXGlobalPtr::Get(&::g_pMediaPlatformList);
    CHXMediaPlatform*& g_pMediaPlatform =
        (CHXMediaPlatform*&)HXGlobalPtr::Get(&::g_pMediaPlatform);
    IHXClientEngine*& g_pEngine =
        (IHXClientEngine*&)HXGlobalPtr::Get(&::g_pEngine);
#endif

    *ppEngine = NULL;

    // HXMediaPlatform can NOT be initialized in mixed mode(new vs. legacy)
    if (g_bMediaPlatformOpened)
    {
	return HXR_FAILED;
    }

    g_uNumEngines++;
    if (g_pEngine)
    {
        *ppEngine = g_pEngine;
        return HXR_OK;
    }
   
    HX_ASSERT(!g_pMediaPlatform);
    g_pMediaPlatform = new CHXMediaPlatform();
    if (!g_pMediaPlatform)
    {
	rc = HXR_OUTOFMEMORY;
	goto exit;
    }

    g_pMediaPlatform->AddRef();

    pIPluginDir = GetPluginDir((IUnknown*)(IHXMediaPlatform*)g_pMediaPlatform);
    if (!pIPluginDir)
    {
	rc = HXR_FAILED;
	goto exit;
    }

    if (HXR_OK != g_pMediaPlatform->AddPluginPath(HXVER_SDK_PRODUCT, 
						  (const char*)pIPluginDir->GetBuffer()))
    {
	rc = HXR_FAILED;
	goto exit;
    }

    if (HXR_OK != g_pMediaPlatform->Init(NULL))
    {
	rc = HXR_FAILED;
	goto exit;
    }

    if (HXR_OK != g_pMediaPlatform->QueryInterface(IID_IHXCommonClassFactory,
						   (void**)&pCCF))
    {
	rc = HXR_FAILED;
	goto exit;
    }

    if (HXR_OK != pCCF->CreateInstance(CLSID_IHXClientEngine, (void**)&g_pEngine))
    {
	rc = HXR_FAILED;
	goto exit;
    }

    *ppEngine = g_pEngine;

exit:

    HX_RELEASE(pIPluginDir);
    HX_RELEASE(pCCF);

    if (HXR_OK != rc)
    {
	g_pMediaPlatform->Close();
	HX_RELEASE(g_pMediaPlatform);
    }

    return rc;
}


STDAPI ENTRYPOINTCALLTYPE
ENTRYPOINT(CloseEngine)(IHXClientEngine* pEngine)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    HXBOOL& g_bMediaPlatformOpened = 
        (HXBOOL&)HXGlobalBool::Get(&::g_bMediaPlatformOpened);
    UINT16& g_uNumEngines = (UINT16&)HXGlobalInt16::Get(&::g_uNumEngines);
    CHXSimpleList*& g_pMediaPlatformList =
        (CHXSimpleList*&)HXGlobalPtr::Get(&::g_pMediaPlatformList);
    CHXMediaPlatform*& g_pMediaPlatform =
        (CHXMediaPlatform*&)HXGlobalPtr::Get(&::g_pMediaPlatform);
    IHXClientEngine*& g_pEngine = (IHXClientEngine*&)HXGlobalPtr::Get(&::g_pEngine);
#endif

    if (g_pEngine && (g_pEngine == pEngine))
    {
	if (g_uNumEngines)
	{
	    g_uNumEngines--;
	}

	if (0 == g_uNumEngines)
	{
	    IHXClientEngine2* pEngine2 = NULL;
	    if (HXR_OK == g_pEngine->QueryInterface(IID_IHXClientEngine2, (void**)&pEngine2))
	    {
		pEngine2->Close();
	    }
	    HX_RELEASE(pEngine2);
	    HX_RELEASE(g_pEngine);

	    g_pMediaPlatform->Close();
	    HX_RELEASE(g_pMediaPlatform);
	}
    }

    return HXR_OK;
}
