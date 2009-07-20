/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_prof_mgr.cpp,v 1.7 2003/08/06 19:24:16 damonlan Exp $ 
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
#include "safestring.h"
#include "servreg.h"
#include "qos_prof_mgr.h"

#define QOS_USER_AGENT_PROFILE_ROOT "config.MediaDelivery.UserAgentProfiles"
#define DEFAULT_USER_AGENT          "Default"
#define DEFAULT_USER_AGENT_LEN       7

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
    m_pProfiles (NULL),
    m_ulNumProfiles (0),
    m_ulProfileRoot(0),
    m_ulDefaultProfile (0)
{
    HX_ASSERT(m_pProc);
    m_ulProfileRoot = m_pProc->pc->registry->GetId((const char*)QOS_USER_AGENT_PROFILE_ROOT,
						   m_pProc);
    UpdateProfiles();
}

QoSProfileSelector::~QoSProfileSelector()
{
    for (UINT32 i = 0; i < m_ulNumProfiles; i++)
    {
	HX_DELETE(m_pProfiles [i]);
    }
    HX_VECTOR_DELETE(m_pProfiles);
}

void
QoSProfileSelector::UpdateProfiles()
{
    if (!m_ulProfileRoot)
    {
	return;
    }

    if (m_ulNumProfiles = m_pProc->pc->registry->Count(m_ulProfileRoot, m_pProc))
    {
	UINT32 ulProfIdx = 0;
	ULONG32 ulConfigID = 0;
	IHXValues* pList = NULL;
	const char* pName = NULL;
	HX_RESULT hResult = HXR_FAIL;
	
	m_pProfiles = new UserAgentProfile* [m_ulNumProfiles];
	memset(m_pProfiles, 0, sizeof(UserAgentProfile*) * m_ulNumProfiles);
	
	m_pProc->pc->registry->GetPropList(m_ulProfileRoot, pList, m_pProc);
	
	HX_ASSERT(pList);
	
	hResult = pList->GetFirstPropertyULONG32(pName, ulConfigID);
	while (pName && SUCCEEDED(hResult))
	{
	    if (PT_COMPOSITE == m_pProc->pc->registry->GetType(ulConfigID, m_pProc))
	    {
		IHXBuffer* pPropName = NULL;
		
		if (SUCCEEDED(m_pProc->pc->registry->GetPropName(ulConfigID, pPropName, m_pProc)))
		{
		    IHXBuffer* pUserAgent = NULL;
		    char* pUserAgentName = new char [11 + pPropName->GetSize()];
		    
		    SafeSprintf(pUserAgentName, 11 + pPropName->GetSize(), "%s.UserAgent", 
				pPropName->GetBuffer());
		    
		    if (SUCCEEDED(m_pProc->pc->registry->GetStr(pUserAgentName, pUserAgent, 
								m_pProc)))
		    {
			if ((!strncmp((const char*)pUserAgent->GetBuffer(), DEFAULT_USER_AGENT,
				      DEFAULT_USER_AGENT_LEN)))
			{
			    m_ulDefaultProfile = ulConfigID;
			}

			m_pProfiles [ulProfIdx] = 
			    new UserAgentProfile((const char*)pUserAgent->GetBuffer(), ulConfigID);
			ulProfIdx++;
		    }
		    
		    HX_RELEASE(pUserAgent);
		    HX_DELETE(pUserAgentName);
		}
	    }
	    hResult = pList->GetNextPropertyULONG32(pName, ulConfigID);
	}
	HX_RELEASE(pList);
    }
}
    
/* IHXQoSProfileSelector */
STDMETHODIMP
QoSProfileSelector::SelectProfile(IHXBuffer* pUserAgent,
				  IHXBuffer* pTransportMime,
				  IHXBuffer* pMediaMime,
				  REF(INT32) /*OUT*/ ulConfigID)
{
    ulConfigID = 0;

    if (!pUserAgent)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!(pUserAgent->GetBuffer()))
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!m_pProfiles)
    {
	return HXR_NOT_INITIALIZED;
    }

    for (UINT32 i = 0; i < m_ulNumProfiles; i++)
    {
	if ((m_pProfiles [i]) && (!strncmp((const char*)pUserAgent->GetBuffer(), 
					   m_pProfiles [i]->m_pUserAgent,
					   m_pProfiles [i]->m_ulUserAgentLen)))
	{
	    ulConfigID = m_pProfiles [i]->m_ulConfigId;
	    return HXR_OK;
	}
    }

    ulConfigID = (ulConfigID) ? ulConfigID : m_ulDefaultProfile;
    
    return (ulConfigID) ? HXR_OK : HXR_FAIL;
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

