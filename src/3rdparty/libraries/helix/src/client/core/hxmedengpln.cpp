/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmedengpln.cpp,v 1.8 2007/02/28 05:57:12 gahluwalia Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "dllpath.h"
#include "pckunpck.h"
#include "hxplugn.h"
#include "hxmedengpln.h"
#include "hxcleng.h"
#include "hxver.h"
#include "clntcore.ver"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* const CHXMediaPlaybackEngine::zm_pDescription = "Helix Media Playback Engine Plugin";
const char* const CHXMediaPlaybackEngine::zm_pCopyright   = HXVER_COPYRIGHT;
const char* const CHXMediaPlaybackEngine::zm_pMoreInfoURL  = "http://www.real.com";

ENABLE_DLLACCESS_PATHS(HXMediaPlaybackEngine);

/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CHXMediaPlaybackEngine();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() ? HXR_FAIL : HXR_OK);
}

CHXMediaPlaybackEngine::CHXMediaPlaybackEngine()
		       :m_lRefCount(0)
		       ,m_pCCF(NULL)
{
}

CHXMediaPlaybackEngine::~CHXMediaPlaybackEngine()
{
    HX_RELEASE(m_pCCF);
}

STDMETHODIMP
CHXMediaPlaybackEngine::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXPlugin*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXComponentPlugin))
    {
	AddRef();
	*ppvObj = (IHXComponentPlugin*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXContextUser))
    {
	AddRef();
	*ppvObj = (IHXContextUser*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXMediaPlaybackEngine::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CHXMediaPlaybackEngine::Release()
{ 
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
    
STDMETHODIMP
CHXMediaPlaybackEngine::InitPlugin(IUnknown* pContext)
{
    if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }

    return pContext->QueryInterface(IID_IHXCommonClassFactory, (void**) &m_pCCF);
}

STDMETHODIMP
CHXMediaPlaybackEngine::GetPluginInfo(REF(HXBOOL)      bMultipleLoad,
				      REF(const char*) pDescription,
				      REF(const char*) pCopyright,
				      REF(const char*) pMoreInfoURL,
				      REF(ULONG32)     ulVersionNumber)
{
    bMultipleLoad = TRUE;

    pDescription = zm_pDescription;
    pCopyright   = zm_pCopyright;
    pMoreInfoURL = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CHXMediaPlaybackEngine::GetNumComponents(void)
{
    // CLSID_IHXClientEngine
    return 1;
}

STDMETHODIMP_(char const*)
CHXMediaPlaybackEngine::GetPackageName(void) CONSTMETHOD
{
    return zm_pDescription;
}

STDMETHODIMP
CHXMediaPlaybackEngine::GetComponentInfoAtIndex(UINT32		/*IN*/  nIndex,
						REF(IHXValues*) /*OUT*/ pInfo)
{
    HX_RESULT	    rc = HXR_OK;
    IUnknown*	    pUnknown = NULL;
    GUID  guidTmp = CLSID_IHXClientEngine;
    BYTE* pTmp    = (BYTE *)&guidTmp;

    pInfo = NULL;

    if (nIndex != 0 || !m_pCCF)
    {
	return HXR_FAILED;
    }

    rc = m_pCCF->CreateInstance(CLSID_IHXValues, (void**) &pUnknown);
    if (HXR_OK != rc)
    {
	goto exit;
    }

    rc = pUnknown->QueryInterface(IID_IHXValues, (void**)&pInfo);
    if (HXR_OK != rc)
    {
	goto exit;
    }

    rc = SetBufferPropertyCCF(pInfo,PLUGIN_COMPONENT_CLSID,
			     pTmp, sizeof(guidTmp), m_pCCF);

exit:
    HX_RELEASE(pUnknown);

    return rc;
}

STDMETHODIMP
CHXMediaPlaybackEngine::CreateComponentInstance(REFCLSID	    /*IN*/  rclsid,
						REF(IUnknown*)	    /*OUT*/ ppUnknown,
						IUnknown*	    /*IN*/  pUnkOuter)
{
    HXClientEngine* pEngine = NULL;
    ppUnknown = NULL;

    if (IsEqualCLSID(rclsid, CLSID_IHXClientEngine))
    {
	pEngine = new HXClientEngine();
	if (!pEngine)
	{
	    return HXR_OUTOFMEMORY;
	}

	return pEngine->QueryInterface(IID_IUnknown, (void**)&ppUnknown);
    }
    else
    {
	return HXR_NOTIMPL;
    }
}
