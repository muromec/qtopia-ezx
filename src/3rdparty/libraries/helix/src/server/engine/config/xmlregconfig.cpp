/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: xmlregconfig.cpp,v 1.3 2003/03/10 17:42:01 dcollins Exp $ 
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
#include "ihxpckts.h"
#include "proc.h"
#include "base_errmsg.h"
#include "hxstrutl.h"
#include "servreg.h"
#include "server_version.h"
#ifdef _WIN32
#include "reg.h"
#endif

#include "xmlregconfig.h"

XMLServerConfig::XMLServerConfig(Process* proc, ServerRegistry* registry)
     :m_proc(proc), m_lRefCount(0)
{
#ifdef _WIN32
    m_winregkey = 0; 
#endif
    m_registry = registry;
    m_registry -> AddRef();
  
    m_pHXRegistry = new HXRegistry(registry, proc);
    m_pHXRegistry -> AddRef();

    IHXRegistry2* pReg2 = NULL;
    m_pHXRegistry->QueryInterface(IID_IHXRegistry2, (void**)&pReg2);

    XMLConfig::init(pReg2, proc->pc->error_handler,
                    ServerVersion::ProductName(), ServerVersion::MajorVersion(),
                    ServerVersion::MinorVersion());

    HX_RELEASE(pReg2);
}

XMLServerConfig::~XMLServerConfig()
{
     HX_RELEASE(m_pHXRegistry);
     HX_RELEASE(m_registry);

#ifdef _WIN32
    if (m_winregkey)
    {
	delete[] m_winregkey;
    }
#endif     
}

STDMETHODIMP
XMLServerConfig::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXRegConfig*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRegConfig))
    {
	AddRef();
	*ppvObj = (IHXRegConfig*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
XMLServerConfig::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);    
}

STDMETHODIMP_(UINT32)
XMLServerConfig::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT XMLServerConfig::Read(char* filename, char* pWinRegKey,
                          char* pServRegKey)
{ 
     HX_RESULT hResult = HXR_OK;
 
#ifdef _WIN32
    if (strlen(filename) == 0)
    {
	return HXR_FAIL;
    }
    else if (strlen(filename) > 9 && !memcmp(filename, "registry:", 9))
    {
	WinRegistry winreg(m_proc, m_registry);
	HX_RESULT hResult = winreg.Read(&filename[9], pServRegKey);
	if (HXR_OK != hResult)
	{
	    ERRMSG(m_proc->pc->error_handler,
		   "%s: registry configuraton key not found", &filename[9]);
	}
	m_winregkey = new_string(&filename[9]);
	return hResult;
    }
#endif

    hResult = XMLConfig::Read(filename, pServRegKey);

#ifdef _WIN32
    if (hResult == HXR_OK &&
        pWinRegKey && pWinRegKey[0] &&
	pServRegKey && pServRegKey[0])
    {
	WinRegistry winreg(m_proc, m_registry);
	winreg.Nuke(pWinRegKey);
	hResult = winreg.Import(pServRegKey, pWinRegKey);
    }
#endif
    return hResult;
}

STDMETHODIMP XMLServerConfig::WriteKey(THIS_ const char* pKeyName)
{
    HX_RESULT hr = XMLConfig::WriteKey(pKeyName);
#ifdef _WIN32
    /*
     * If they gave us a winregkey, then write to the windows
     * registry.
     */
    if (hr != HXR_OK && m_winregkey)
    {
	WinRegistry winreg(m_proc, m_registry);
	winreg.Nuke(m_winregkey);
	return winreg.Import(pKeyName, m_winregkey);
    }
#endif
    return hr;
}
