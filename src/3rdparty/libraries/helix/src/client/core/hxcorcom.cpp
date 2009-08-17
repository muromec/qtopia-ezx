/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcorcom.cpp,v 1.8 2006/01/13 21:46:57 ping Exp $
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

#ifdef _WIN16
#include "hlxclib/windows.h"
#endif /* _WIN16 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxcleng.h"
#include "hxcorcom.h"
#include "hxassert.h"
#include "hxplugn.h"
#include "hxplugn.h"
#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#include "basehand.h"
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
#include "hxpluginmanager.h"
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else /* #if defined(_STATICALLY_LINKED) */
#include "plghand2.h"
#endif /* #if defined(_STATICALLY_LINKED) #else */

#ifdef _WINDOWS
#include "platform/win/wcorecom.h"
#endif


#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif



// --------------------------------------------------------------------------
//  Create
//
//  Create an instance of an HXCoreComm object.  This function will create
//  the appropriate platform-specific object.
// --------------------------------------------------------------------------

HXCoreComm*
HXCoreComm::Create(HXClientEngine* pEngine)
{
    HXCoreComm* pComm = NULL;

    HX_ASSERT(pEngine);

#ifdef _WINDOWS
    pComm = new WinCoreComm(pEngine);
#endif

    return pComm;
}


// --------------------------------------------------------------------------
//  constructor
// --------------------------------------------------------------------------

HXCoreComm::HXCoreComm(HXClientEngine* pEngine)
    : m_pEngine(pEngine)
{
    HX_ASSERT(m_pEngine);
    m_pEngine->AddRef();
}



// --------------------------------------------------------------------------
//  destructor
// --------------------------------------------------------------------------

HXCoreComm::~HXCoreComm()
{
    HX_RELEASE(m_pEngine);
}



// --------------------------------------------------------------------------
//  StopAudioPlayback
//
//  This method is called when the HXCoreComm object is asked to stop
//  audio playback.  It should tell the engine to stop playback of any
//  presentation that uses audio.
// --------------------------------------------------------------------------

STDMETHODIMP
HXCoreComm::StopAudioPlayback()
{
    HX_ASSERT(m_pEngine);

    return m_pEngine->StopAudioPlayback();
}

// --------------------------------------------------------------------------
//  UnloadPlugins
//
//  This method is called when the HXCoreComm object is asked to unload
//  plugins.  
// --------------------------------------------------------------------------

STDMETHODIMP
HXCoreComm::UnloadPlugins()
{
    HX_ASSERT(m_pEngine);

    m_pEngine->ShutDown();

    IHXPlugin2Handler* pPluginHandler = NULL;
    if (HXR_OK == m_pEngine->QueryInterface(IID_IHXPlugin2Handler, (void**) &pPluginHandler))
    {
	pPluginHandler->FlushCache();
	HX_RELEASE(pPluginHandler);
    }

    return HXR_OK;
}

// --------------------------------------------------------------------------
//  ReloadPlugins
//
//  This method is called when the HXCoreComm object is asked to reload
//  plugins.  
// --------------------------------------------------------------------------

STDMETHODIMP
HXCoreComm::ReloadPlugins()
{
    IHXPluginReloader* pPluginReloader = NULL;

    HX_ASSERT(m_pEngine);
    HX_ASSERT(m_pEngine->m_pPlugin2Handler);

    if (HXR_OK == m_pEngine->m_pPlugin2Handler->QueryInterface(IID_IHXPluginReloader,
							       (void**)&pPluginReloader))
    {
	pPluginReloader->ReloadPlugins();
    }
    HX_RELEASE(pPluginReloader);

    return HXR_OK;
}
