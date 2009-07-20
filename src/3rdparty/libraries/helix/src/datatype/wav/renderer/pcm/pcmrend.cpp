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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxrendr.h"
#include "hxplugn.h"
#include "baseobj.h"
#include "hxver.h"
#include "audrend.h"
#include "audrendf.h"
#include "pcmfmt.h"
#include "pcmrend.h"
#include "pcmrend.ver"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* const CPCMAudioRenderer::m_pszDescription        = "Helix PCM Audio Renderer Plugin";
const char* const CPCMAudioRenderer::m_pszRendererName       = "PCM Audio";
const char* const CPCMAudioRenderer::m_ppszStreamMimeTypes[] = { "audio/L8", "audio/L16", "audio/x-pn-wav", "audio/PCMA","audio/pcma","audio/PCMU",NULL };


CPCMAudioRenderer::CPCMAudioRenderer(void)
{
}

CPCMAudioRenderer::~CPCMAudioRenderer()
{    
}

STDMETHODIMP CPCMAudioRenderer::GetPluginInfo(REF(HXBOOL)        bLoadMultiple,
                                              REF(const char*) pDescription,
                                              REF(const char*) pCopyright,
                                              REF(const char*) pMoreInfoURL,
                                              REF(ULONG32)     ulVersionNumber)
{
    HX_RESULT retVal = CAudioRenderer::GetPluginInfo(bLoadMultiple,
                                                     pDescription,
                                                     pCopyright,
                                                     pMoreInfoURL,
                                                     ulVersionNumber);
    if (SUCCEEDED(retVal))
    {
        pDescription    = (const char*) m_pszDescription;
        ulVersionNumber = TARVER_ULONG32_VERSION;
    }
    
    return retVal;
}

STDMETHODIMP CPCMAudioRenderer::GetRendererInfo(REF(const char**) pStreamMimeTypes,
                                                REF(UINT32)       unInitialGranularity)
{
    HX_RESULT retVal = CAudioRenderer::GetRendererInfo(pStreamMimeTypes,
                                                       unInitialGranularity);
    if (SUCCEEDED(retVal))
    {
        pStreamMimeTypes = (const char**) m_ppszStreamMimeTypes;
    }
    
    return retVal;
}

CAudioFormat* CPCMAudioRenderer::CreateFormatObject(IHXValues* pHeader)
{
    return new CPCMAudioFormat(m_pCommonClassFactory, this);
}

const char* CPCMAudioRenderer::GetRendererName(void)
{
    return (const char*) m_pszRendererName;
}

HX_RESULT STDAPICALLTYPE CPCMAudioRenderer::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppIUnknown)
    {
        // Set default
        *ppIUnknown = NULL;
        // Create the object
        CPCMAudioRenderer *pObj = new CPCMAudioRenderer();
        if (pObj)
        {
            // QI for IUnknown
            retVal = pObj->QueryInterface(IID_IUnknown, (void**) ppIUnknown);
        }
        if (FAILED(retVal))
        {
            HX_DELETE(pObj);
        }
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CPCMAudioRenderer::CanUnload2()
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

