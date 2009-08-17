/* ***** BEGIN LICENSE BLOCK *****
 *
 * Source last modified: $Id:
 *
 * Copyright Notices:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 *
 * Patent Notices: This file may contain technology protected by one or
 * more of the patents listed at www.helixcommunity.org
 *
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its
 * licensors, and made available by RealNetworks subject to the current
 * version of the RealNetworks Public Source License (the "RPSL")
 * available at  * http://www.helixcommunity.org/content/rpsl unless
 * you have licensed the file under the current version of the
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its
 * licensors disclaim any implied patent license under the GPL.
 * If you wish to allow use of your version of this file only under
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 *
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing,
 * adapting or translating, any or all of this material other than
 * pursuant to the license terms referred to above requires the prior
 * written consent of RealNetworks and its licensors
 *
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
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
#include "tonefmt.h"
#include "tonerend.h"
#include "tonerend.ver"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* const CTONEAudioRenderer::m_pszDescription        = "Helix TONE Audio Renderer Plugin";
const char* const CTONEAudioRenderer::m_pszRendererName       = "Tone Generation Audio";
const char* const CTONEAudioRenderer::m_ppszStreamMimeTypes[] = { "audio/x-hx-tonesequence", NULL };

CTONEAudioRenderer::CTONEAudioRenderer(void)
{
	m_bInSeekMode = FALSE;
}

CTONEAudioRenderer::~CTONEAudioRenderer()
{
	HX_RELEASE(m_pCommonClassFactory);
}

STDMETHODIMP CTONEAudioRenderer::GetPluginInfo(REF(BOOL)        bLoadMultiple,
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

STDMETHODIMP CTONEAudioRenderer::GetRendererInfo(REF(const char**) pStreamMimeTypes,
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

CAudioFormat* CTONEAudioRenderer::CreateFormatObject(IHXValues* pHeader)
{
	m_pTONEAudioFormat = new CTONEAudioFormat(m_pCommonClassFactory, this);
	return m_pTONEAudioFormat;
}

const char* CTONEAudioRenderer::GetRendererName(void)
{
    return (const char*) m_pszRendererName;
}

HX_RESULT STDAPICALLTYPE CTONEAudioRenderer::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppIUnknown)
    {
        // Set default
        *ppIUnknown = NULL;
        // Create the object
        CTONEAudioRenderer *pObj = new CTONEAudioRenderer();
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

HX_RESULT STDAPICALLTYPE CTONEAudioRenderer::CanUnload2()
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

STDMETHODIMP
CTONEAudioRenderer::OnPostSeek(UINT32 timeBeforeSeek,
                      UINT32 timeAfterSeek)
{
    m_pMutex->Lock();
	m_pTONEAudioFormat->SetSeekPktDur(timeAfterSeek);
	m_bInSeekMode = FALSE;
	m_ulLastWriteTime = timeAfterSeek;

	m_pMutex->Unlock();
    return HXR_OK;
}
