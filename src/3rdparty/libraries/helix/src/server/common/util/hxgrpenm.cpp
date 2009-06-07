/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxgrpenm.cpp,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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


#include "hxtypes.h"
#include "hxcom.h"
#include "hxplugn.h"
#include "hxgrpenm.h"
#include "carray.h"
#include "plgnhand.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

CHXPluginGroupEnumerator::CHXPluginGroupEnumerator(
    PluginHandler* pPluginHandler) :
    m_lRefCount(0)
{
    m_pPluginHandler = pPluginHandler;
    m_pPlugins = NULL;
    m_nPluginCount = 0;
}

CHXPluginGroupEnumerator::~CHXPluginGroupEnumerator()
{
    HX_DELETE(m_pPlugins);
}

STDMETHODIMP
CHXPluginGroupEnumerator::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPluginGroupEnumerator*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPluginGroupEnumerator))
    {
        AddRef();
        *ppvObj = (IHXPluginGroupEnumerator*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
CHXPluginGroupEnumerator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
CHXPluginGroupEnumerator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHXPluginGroupEnumerator::Init(REFIID iid)
{
    PluginHandler::Basic* general = m_pPluginHandler->m_general_handler;
    CHXSimpleList::Iterator i;

    m_nPluginCount = 0;
    if(general->GetNumOfPlugins() == 0)
	return HXR_FAIL;

    m_pPlugins = new CHXPtrArray;
    m_pPlugins->SetSize(1);

    for(i = general->m_pPlugins->Begin();
	i != general->m_pPlugins->End();
	++i)
    {
	PluginHandler::Plugin* pPlugin = (PluginHandler::Plugin*)(*i);
	IUnknown* pInstance;
	pPlugin->GetInstance(&pInstance);
	
	if(pInstance)
	{
	    IUnknown* pQuery;
	    if(HXR_OK == pInstance->QueryInterface(iid, (void**)&pQuery))
	    {
		m_pPlugins->SetAtGrow((int)m_nPluginCount, pPlugin);
		pQuery->Release();
		m_nPluginCount++;
	    }
	    pInstance->Release();
	    pPlugin->ReleaseInstance();
	}
    }
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CHXPluginGroupEnumerator::GetNumOfPlugins()
{
    return m_nPluginCount;
}

STDMETHODIMP
CHXPluginGroupEnumerator::GetPlugin(UINT32 ulIndex, REF(IUnknown*) pPlugin)
{
    PluginHandler::Plugin* pHandlerPlugin = NULL;
    IUnknown* pInstance = NULL;

    HX_ASSERT(ulIndex < m_nPluginCount);

    pHandlerPlugin = (PluginHandler::Plugin*)m_pPlugins->GetAt((int)ulIndex);

    HX_ASSERT(pHandlerPlugin);

    pHandlerPlugin->GetInstance(&pInstance);
    pHandlerPlugin->ReleaseInstance();

    if(pInstance)
    {
	pPlugin = pInstance;
	return HXR_OK;
    }
    else
    {
	return HXR_NOINTERFACE;
    }
}

