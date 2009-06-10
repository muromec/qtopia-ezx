/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#define INITGUID

#include "hxcom.h"
#include <stdio.h>
#ifndef _WINCE
#include <signal.h>
#endif

#include "hxtypes.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxvalue.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxrendr.h"
#include "hxplugn.h"
#include "hxupgrd.h"
#include "hxausvc.h"
#include "netbyte.h"
#include "hxheap.h"
#include "hxstrutl.h"
#include "hxslist.h"
#include "hxformt.h"
#include "hxpends.h"
#include "hxwin.h"
#include "hxvalue.h"
#include "baseobj.h"
#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"
#include "hxdllaccess.h"
#include "hxplgns.h"
#include "ihxfgbuf.h"

#if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU) || defined(HELIX_FEATURE_AUDIO_AIFF)
#include "audrend.h"
#include "pcmrend.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU)
              || defined(HELIX_FEATURE_AUDIO_AIFF) */

#if defined(HELIX_FEATURE_AUDIO_WAVE)
#include "wvffplin.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) */

#if defined(HELIX_FEATURE_AUDIO_AU)
#include "auffplin.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_AU) */

#if defined(HELIX_FEATURE_AUDIO_AIFF)
#include "aiffplin.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_AIFF) */

#if defined(HELIX_FEATURE_PLAYBACK_LOCAL)
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB) || defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)
#define INCLUDE_AMRFF
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB) || defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB) */
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_LOCAL) */
#if defined(INCLUDE_AMRFF)
#include "amrff.h"
#endif /* #if defined(INCLUDE_AMRFF) */
#if defined(HELIX_FEATURE_AUDIO_MPEG4)
#include "hxacodec.h"
#include "mp4audio.h"
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4) */
#include "audplin.h"

#if !defined(HELIX_FEATURE_DLLACCESS_CLIENT)
#include "dllpath.h"
ENABLE_DLLACCESS_PATHS(audplin);
#endif	// HELIX_FEATURE_DLLACCESS_CLIENT

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

HX_RESULT (STDAPICALLTYPE  * const AudioPluginFactory::m_fpEntryArray[])(IUnknown**)=
{
#if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU) || defined(HELIX_FEATURE_AUDIO_AIFF)
    CPCMAudioRenderer::HXCreateInstance,
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU)
              || defined(HELIX_FEATURE_AUDIO_AIFF) */
#if defined(HELIX_FEATURE_AUDIO_WAVE)
    CWaveFileFormat::HXCreateInstance,
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) */
#if defined(HELIX_FEATURE_AUDIO_AU)
    CAUFileFormat::HXCreateInstance,
#endif /* #if defined(HELIX_FEATURE_AUDIO_AU) */
#if defined(HELIX_FEATURE_AUDIO_AIFF)
    AIFFFileFormat::HXCreateInstance,
#endif /* #if defined(HELIX_FEATURE_AUDIO_AIFF) */
#if defined(INCLUDE_AMRFF)
    CAMRFileFormat::HXCreateInstance,
#endif /* #if defined(INCLUDE_AMRFF) */
#if defined(HELIX_FEATURE_AUDIO_MPEG4)
    CMP4AudioRenderer::HXCreateInstance,
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4) */
    NULL
};

HX_RESULT (STDAPICALLTYPE* const AudioPluginFactory::m_fpUnloadArray[])()={
#if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU) || defined(HELIX_FEATURE_AUDIO_AIFF)
    CPCMAudioRenderer::CanUnload2,
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) || defined(HELIX_FEATURE_AUDIO_AU)
              || defined(HELIX_FEATURE_AUDIO_AIFF)*/
#if defined(HELIX_FEATURE_AUDIO_WAVE)
    CWaveFileFormat::CanUnload2,
#endif /* #if defined(HELIX_FEATURE_AUDIO_WAVE) */
#if defined(HELIX_FEATURE_AUDIO_AU)
    CAUFileFormat::CanUnload2,
#endif /* #if defined(HELIX_FEATURE_AUDIO_AU) */
#if defined(HELIX_FEATURE_AUDIO_AIFF)
    AIFFFileFormat::CanUnload2,
#endif /* #if defined(HELIX_FEATURE_AUDIO_AIFF) */
#if defined(INCLUDE_AMRFF)
    CAMRFileFormat::CanUnload2,
#endif /* #if defined(INCLUDE_AMRFF) */
#if defined(HELIX_FEATURE_AUDIO_MPEG4)
    CMP4AudioRenderer::CanUnload2,
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4) */
    NULL
};

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppIUnknown)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new AudioPluginFactory();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    for( int i=0; AudioPluginFactory::m_fpUnloadArray[i]; i++ )
    {
        if( (AudioPluginFactory::m_fpUnloadArray[i])() != HXR_OK )
        {
            return HXR_FAIL;
        }
    }
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory
//  Purpose:
//      Constructor. Counts the number of functions within the
//      Entry Array. Would have liked to use:
//      return (sizeof(m_fpEntryArray)/sizeof(m_fpEntryArray[0]))-1;
//      But for some strange reason the complier spits at it...
//

AudioPluginFactory::AudioPluginFactory() :
	m_lRefCount(0)
{
    for(m_usNumOfPlugins=0;m_fpEntryArray[m_usNumOfPlugins];m_usNumOfPlugins++) ;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP AudioPluginFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown *)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPluginFactory))
    {
	AddRef();
	*ppvObj = (IHXPluginFactory *)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}



/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) AudioPluginFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) AudioPluginFactory::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory::GetNumPlugins
//  Purpose:
//      Returns an int, the number of plugins in this DLL.
//
//

STDMETHODIMP_(UINT16) AudioPluginFactory::GetNumPlugins()
{
    return m_usNumOfPlugins;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory::GetPlugin
//  Purpose:
//      Returns an IUnknown Reference to the specified Interface
//
//

STDMETHODIMP AudioPluginFactory::GetPlugin(UINT16 uindex, IUnknown** pPlugin)
{
    if (uindex<m_usNumOfPlugins)
	return m_fpEntryArray[uindex](pPlugin);
    *pPlugin=0;
    return HXR_NOINTERFACE;
}
