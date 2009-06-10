/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: logtrans_plin.cpp,v 1.4 2007/07/06 21:58:17 jfinnecy Exp $
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
#define INITGUID

#include "logtrans_plin.h"
#include "hxtlogutil.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxstring.h"

#include "hxclsnk.h"
#include "hxslist.h"
#include "sinkctl.h"

const char CHXLogTranslatorPlugin::zm_pDescription[] = 
"Error Log Translator Plugin";

const char CHXLogTranslatorPlugin::zm_pCopyright[] = "Helix Community 2004";
const char CHXLogTranslatorPlugin::zm_pMoreInfoURL[] = "";

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppIUnknown)   
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (ppIUnknown)
    {
        *ppIUnknown = (IHXPlugin*)new CHXLogTranslatorPlugin;

        if (*ppIUnknown)
        {
            (*ppIUnknown)->AddRef();
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}   


CHXLogTranslatorPlugin::CHXLogTranslatorPlugin() :
    m_ulRefCount(0),
    m_pErrorSink(NULL)
{}



CHXLogTranslatorPlugin::~CHXLogTranslatorPlugin()
{
    HX_RELEASE(m_pErrorSink);
}

/*
 *	IUnknown methods
 */
STDMETHODIMP 
CHXLogTranslatorPlugin::QueryInterface(THIS_
                                        REFIID riid,
                                        void** ppvObj)
{
    QInterfaceList qiList[] = {
        { GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*)this },
        { GET_IIDHANDLE(IID_IHXGenericPlugin), (IHXGenericPlugin*)this },
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*)this }
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
CHXLogTranslatorPlugin::AddRef(THIS)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
CHXLogTranslatorPlugin::Release(THIS)
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    if(m_ulRefCount == 0)
    {
        delete this;
    }
    return 0;
}

/*
 *	IHXPlugin methods
 */
STDMETHODIMP
CHXLogTranslatorPlugin::GetPluginInfo(THIS_
                                       REF(HXBOOL)	/*OUT*/ bMultipleLoad,
                                       REF(const char*) /*OUT*/ pDescription,
                                       REF(const char*) /*OUT*/ pCopyright,
                                       REF(const char*) /*OUT*/ pMoreInfoURL,
                                       REF(ULONG32)	/*OUT*/ ulVersionNumber)
{
    bMultipleLoad   = FALSE;
    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = 1;

    return HXR_OK;
}

STDMETHODIMP
CHXLogTranslatorPlugin::InitPlugin(THIS_
                                    IUnknown*   /*IN*/  pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if(pContext)
    {
        HX_ENABLE_LOGGING(pContext);

        HX_RELEASE(m_pErrorSink);
        m_pErrorSink = new CHXErrorSinkTranslator;
        if (m_pErrorSink)
        {
            m_pErrorSink->AddRef();
        }

        IHXPlayerSinkControl* pSinkCtl = NULL;
        
        if (HXR_OK == pContext->QueryInterface(IID_IHXPlayerSinkControl,
                                               (void**)&pSinkCtl))
        {
            pSinkCtl->AddSink(this);
        }
        HX_RELEASE(pSinkCtl);
    }

    return res;
}

/*
 *	IHXGenericPlugin methods
 */

STDMETHODIMP 
CHXLogTranslatorPlugin::IsGeneric(THIS_
                                   REF(HXBOOL)	 /*OUT*/ bIsGeneric)
{
    bIsGeneric = TRUE;
    return HXR_OK;
}


STDMETHODIMP
CHXLogTranslatorPlugin::PlayerCreated(THIS_ IHXPlayer* pPlayer)
{

    if (pPlayer && m_pErrorSink)
    {
        IHXErrorSinkControl* pControl = NULL;
        if (HXR_OK == pPlayer->QueryInterface(IID_IHXErrorSinkControl, 
                                              (void**) &pControl))
        {
            pControl->AddErrorSink(m_pErrorSink, HXLOG_DEBUG, HXLOG_DEBUG);
        }
        HX_RELEASE(pControl);
    }

    return HXR_OK;
}

STDMETHODIMP
CHXLogTranslatorPlugin::PlayerClosed(THIS_ IHXPlayer* pPlayer)
{
    if (pPlayer && m_pErrorSink)
    {
        IHXErrorSinkControl* pControl = NULL;
        if (HXR_OK == pPlayer->QueryInterface(IID_IHXErrorSinkControl, 
                                              (void**) &pControl))
        {
            pControl->RemoveErrorSink(m_pErrorSink);
        }
        HX_RELEASE(pControl);
    }

    return HXR_OK;
}
