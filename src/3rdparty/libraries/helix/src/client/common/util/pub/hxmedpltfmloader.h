/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmedpltfmloader.h,v 1.3 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _CHXMEDPLTFMLOADER_H_
#define _CHXMEDPLTFMLOADER_H_

#if defined(HELIX_FEATURE_MEDIAPLATFORM_STATIC_LINK)
inline HX_RESULT HXLoadMediaPlatform(const char* pszPath = NULL)
{
    return HXR_OK;
}
inline void HXUnloadMediaPlatform()
{
    // empty
}
#else
#include "dllacces.h"
#include "ihxmedpltfm.h"

class CHXMediaPlatformLoader
{
public:
    // singleton access only
    static CHXMediaPlatformLoader& Loader();
    ~CHXMediaPlatformLoader();

    HX_RESULT Load(const char* pszPath);
    void Unload();

    HX_RESULT	HXMediaPlatformOpen();
    HX_RESULT	HXCreateMediaPlatform(IHXMediaPlatform** ppMediaPlatform);
    HX_RESULT	HXMediaPlatformClose();
    HX_RESULT	CreateEngine(IHXClientEngine** ppEngine);
    HX_RESULT	CloseEngine(IHXClientEngine*  pEngine);
    HX_RESULT	SetDLLAccessPath(const char* pszPath);

private:
    // ctor 
    CHXMediaPlatformLoader();

    FPHXMEDIAPLATFORMOPEN   m_fpHXMediaPlatformOpen;
    FPHXCREATEMEDIAPLATFORM m_fpHXCreateMediaPlatform;
    FPHXMEDIAPLATFORMCLOSE  m_fpHXMediaPlatformClose;
    FPRMCREATEENGINE	    m_fpCreateEngine;
    FPRMCLOSEENGINE	    m_fpCloseEngine;
    FPRMSETDLLACCESSPATH    m_fpSetDLLAccessPath;

    HXBOOL		    m_bLoaded;
    DLLAccess*              m_pDLLAccess;
};

inline HX_RESULT HXLoadMediaPlatform(const char* pszPath = NULL)
{
    return CHXMediaPlatformLoader::Loader().Load(pszPath);
}

inline void HXUnloadMediaPlatform()
{
    CHXMediaPlatformLoader::Loader().Unload();
}

inline
HX_RESULT CHXMediaPlatformLoader::HXMediaPlatformOpen()
{
    return m_fpHXMediaPlatformOpen();
}

inline
HX_RESULT CHXMediaPlatformLoader::HXCreateMediaPlatform(IHXMediaPlatform** ppMediaPlatform)
{
    return m_fpHXCreateMediaPlatform(ppMediaPlatform);
}

inline
HX_RESULT CHXMediaPlatformLoader::HXMediaPlatformClose()
{
    return m_fpHXMediaPlatformClose();
}

inline
HX_RESULT CHXMediaPlatformLoader::CreateEngine(IHXClientEngine** ppEngine)
{
    return m_fpCreateEngine(ppEngine);
}

inline
HX_RESULT CHXMediaPlatformLoader::CloseEngine(IHXClientEngine*  pEngine)
{
    return m_fpCloseEngine(pEngine);
}

inline
HX_RESULT CHXMediaPlatformLoader::SetDLLAccessPath(const char* pszPath)
{
    return m_fpSetDLLAccessPath(pszPath);
}

#define DEF_MEDPLTFM(api) CHXMediaPlatformLoader::Loader().##api
// Not using DEF_MEDPLTFM Macro because it gives compilation error with GCCE tool chain
#define HXMediaPlatformOpen	CHXMediaPlatformLoader::Loader().HXMediaPlatformOpen
#define HXCreateMediaPlatform	CHXMediaPlatformLoader::Loader().HXCreateMediaPlatform
#define HXMediaPlatformClose	 CHXMediaPlatformLoader::Loader().HXMediaPlatformClose
#define CreateEngine		CHXMediaPlatformLoader::Loader().CreateEngine
#define CloseEngine		CHXMediaPlatformLoader::Loader().CloseEngine
#define SetDLLAccessPath	 CHXMediaPlatformLoader::Loader().SetDLLAccessPath

#endif /* HELIX_FEATURE_MEDIAPLATFORM_STATIC_LINK */

#endif /* _CHXMEDPLTFMLOADER_H_ */
