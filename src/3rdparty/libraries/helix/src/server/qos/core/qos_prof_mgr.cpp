/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_prof_mgr.cpp,v 1.15 2007/12/05 10:55:47 vijendrakumara Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxerror.h"
#include "hxqos.h"
#include "hxassert.h"
#include "proc.h"
#include "server_context.h"
#include "safestring.h"
#include "servreg.h"
#include "hxreg.h"
#include "qos_prof_mgr.h"
#include "qos_cfg_names.h"

/* UserAgentProfile */
UserAgentProfile::UserAgentProfile (const char* pUserAgent, INT32 ulConfigId) :
    m_pUserAgent (NULL),
    m_ulConfigId(ulConfigId)
{
    HX_ASSERT(pUserAgent);

    if (pUserAgent)
    {
        m_pUserAgent = new_string(pUserAgent);
        m_ulUserAgentLen = strlen(pUserAgent);
    }
}

UserAgentProfile::~UserAgentProfile ()
{
    HX_VECTOR_DELETE(m_pUserAgent);
}

/* QoS Profile Selector*/
QoSProfileSelector::QoSProfileSelector(Process* pProc) :
    m_pProc(pProc),
    m_pDefaultUAS(NULL),
    m_lRefCount(0)
{
    HX_ASSERT(m_pProc);

    IHXRegistry2* pReg2 = NULL;
    m_pProc->pc->server_context->QueryInterface(IID_IHXRegistry2, (void**)&pReg2);
    m_pUASConfigTree = new UASConfigTree(pProc->pc->error_handler, pReg2);
    HX_RELEASE(pReg2);

    UpdateProfiles();
}

QoSProfileSelector::~QoSProfileSelector()
{
    HX_DELETE(m_pUASConfigTree);
}

void
QoSProfileSelector::UpdateProfiles()
{
    m_pUASConfigTree->Update();
    m_pDefaultUAS = m_pUASConfigTree->GetUserAgentSettings(DEFAULT_USER_AGENT);
}
    
/* IHXQoSProfileSelector */
STDMETHODIMP
QoSProfileSelector::SelectProfile(IHXBuffer* pUserAgent,
                                  IHXBuffer* pTransportMime,
                                  IHXBuffer* pMediaMime,
                                  REF(IHXUserAgentSettings*) /*OUT*/ pUAS)
{
    pUAS = NULL;

    if (!pUserAgent || !(pUserAgent->GetBuffer()))
    {
        if (!m_pDefaultUAS)
        {
            return HXR_NOT_INITIALIZED;
        }
        pUAS = (IHXUserAgentSettings*)m_pDefaultUAS;
        HX_ADDREF(pUAS);
        return HXR_OK;
    }

    UserAgentSettings* pSelectedUAS = m_pUASConfigTree->SelectUserAgentSettings((char *)pUserAgent->GetBuffer());
    if(pSelectedUAS)
    {
        pUAS = (IHXUserAgentSettings*)pSelectedUAS;
    }
    else
    {
        if (!m_pDefaultUAS)
        {
            return HXR_NOT_INITIALIZED;
        }
        pUAS = (IHXUserAgentSettings*)m_pDefaultUAS; //Return Detault UAS
    }
    HX_ADDREF(pUAS);
    return HXR_OK;
}

STDMETHODIMP_(CHXMapStringToOb::Iterator)
QoSProfileSelector::GetBegin()
{
    return m_pUASConfigTree->Begin();
}

STDMETHODIMP_(CHXMapStringToOb::Iterator)
QoSProfileSelector::GetEnd()
{
    return m_pUASConfigTree->End();
}

/* IUnknown */
STDMETHODIMP
QoSProfileSelector::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSProfileSelector*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSProfileSelector))
    {
        AddRef();
        *ppvObj = (IHXQoSProfileSelector*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSProfileSelector::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSProfileSelector::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

