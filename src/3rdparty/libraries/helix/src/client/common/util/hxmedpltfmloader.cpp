/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxstrutl.h"
//#include "hxtlogutil.h"
#include "hxmedpltfmloader.h"

#include "hxheap.h"

#if defined(HELIX_CONFIG_NOSTATICS)
	#include "globals/hxglobals.h"
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(HELIX_FEATURE_MEDIAPLATFORM_STATIC_LINK)
#error should not be included in build
#endif

#if defined(HELIX_CONFIG_NOSTATICS)

// function used to register with global mgr
// for deleting the loader
void DeleteMediaPlatformLoader(void *pLoader)
{
    if(pLoader != NULL)
    {
        delete pLoader;
    }
}
#endif

CHXMediaPlatformLoader& 
CHXMediaPlatformLoader::Loader()
{
#if !defined(HELIX_CONFIG_NOSTATICS)	
    static CHXMediaPlatformLoader instance;
    return instance;
#else
    static const UINT32 ulContext = 0;
    CHXMediaPlatformLoader** ppLoader = NULL;
    CHXMediaPlatformLoader* pRetInstance = NULL;

    HXGlobalManager* pGM = HXGlobalManager::Instance();
    if(pGM != NULL)
    {
        ppLoader = (CHXMediaPlatformLoader**) (pGM->Get((GlobalID)&ulContext));
        if(ppLoader == NULL)
        {
            pRetInstance = new CHXMediaPlatformLoader();
            pGM->Add( (GlobalID)&ulContext, (GlobalType)pRetInstance, &DeleteMediaPlatformLoader);
        }
        else
        {
            pRetInstance = *ppLoader;
        }
    }
    
    HX_ASSERT(pRetInstance);
   return *pRetInstance;
#endif    
}

CHXMediaPlatformLoader::CHXMediaPlatformLoader()
		       :m_pDLLAccess(NULL)
		       ,m_bLoaded(FALSE)
		       ,m_fpHXMediaPlatformOpen(NULL)
		       ,m_fpHXCreateMediaPlatform(NULL)
		       ,m_fpHXMediaPlatformClose(NULL)
		       ,m_fpCreateEngine(NULL)
		       ,m_fpCloseEngine(NULL)
		       ,m_fpSetDLLAccessPath(NULL)
{
}

CHXMediaPlatformLoader::~CHXMediaPlatformLoader()
{
}

HX_RESULT 
CHXMediaPlatformLoader::Load(const char* pszPath)
{
    HX_RESULT	rc = HXR_OK;
    char        staticLibPath[_MAX_PATH] = {0};

    if(m_bLoaded)
    {
        return rc;
    }

#ifdef _MACINTOSH
    char    dllhome[_MAX_PATH]       = {'\0'};	    /* Flawfinder: ignore */
#elif defined(_SYMBIAN)
    char    dllhome[_MAX_PATH]       = "c:";	    /* Flawfinder: ignore */
#else
    char    dllhome[_MAX_PATH]       = {'.','\0'};   /* Flawfinder: ignore */
#endif

    if (pszPath)
    {
        SafeStrCpy(dllhome,  pszPath, _MAX_PATH);
    }

#if defined(_MAC_UNIX)
    SafeSprintf(staticLibPath, _MAX_PATH, "%s/%s", dllhome, "hxmedpltfm.bundle");
#elif defined(_UNIX)
    SafeSprintf(staticLibPath, _MAX_PATH, "%s/%s", dllhome, "hxmedpltfm.so");
#elif defined(_WINDOWS) || defined(_SYMBIAN)
    SafeSprintf(staticLibPath, _MAX_PATH, "%s\\%s", dllhome, "hxmedpltfm.dll");
#elif defined(_MACINTOSH)
#ifdef _MAC_MACHO
    SafeSprintf(staticLibPath, _MAX_PATH, "hxmedpltfm.bundle");
#else
    if (strlen(dllhome) > 0 )
    {
        SafeSprintf(staticLibPath, _MAX_PATH, "%s:%s", dllhome, "hxmedpltfm.shlb");
    }
    else
    {
        SafeSprintf(staticLibPath, _MAX_PATH, "%s", "hxmedpltfm.shlb");
    }
#endif
#endif

    HX_ASSERT(!m_pDLLAccess);
    m_pDLLAccess = new DLLAccess();

    if (!m_pDLLAccess)
    {
	rc = HXR_OUTOFMEMORY;
	goto exit;
    }

    if (DLLAccess::DLL_OK != m_pDLLAccess->open(staticLibPath))
    {
        const char* pErrorString = m_pDLLAccess->getErrorString();
	rc = HXR_FAILED;
	goto exit;
    }

    m_fpHXMediaPlatformOpen = (FPHXMEDIAPLATFORMOPEN) m_pDLLAccess->getSymbol("HXMediaPlatformOpen");
    m_fpHXCreateMediaPlatform = (FPHXCREATEMEDIAPLATFORM) m_pDLLAccess->getSymbol("HXCreateMediaPlatform");
    m_fpHXMediaPlatformClose = (FPHXMEDIAPLATFORMCLOSE) m_pDLLAccess->getSymbol("HXMediaPlatformClose");

    m_fpCreateEngine = (FPRMCREATEENGINE) m_pDLLAccess->getSymbol("CreateEngine");
    m_fpCloseEngine  = (FPRMCLOSEENGINE)  m_pDLLAccess->getSymbol("CloseEngine");
    m_fpSetDLLAccessPath = (FPRMSETDLLACCESSPATH) m_pDLLAccess->getSymbol("SetDLLAccessPath");

    if (!m_fpHXMediaPlatformOpen    ||
	!m_fpHXCreateMediaPlatform  ||
	!m_fpHXMediaPlatformClose   ||
	!m_fpCreateEngine	    ||
	!m_fpCloseEngine	    ||
	!m_fpSetDLLAccessPath)
    {
	rc = HXR_FAILED;
	goto exit;
    }

    m_bLoaded = TRUE;

exit:

    if (HXR_OK != rc)
    {
	Unload();
    }

    return rc;
}

void CHXMediaPlatformLoader::Unload()
{
    HX_ASSERT(m_pDLLAccess);
    if(m_pDLLAccess) 
    {
	m_pDLLAccess->close();
	HX_DELETE(m_pDLLAccess);

	m_bLoaded = FALSE;
    }
}
