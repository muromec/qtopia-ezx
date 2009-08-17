/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win_fwctlmgr.cpp,v 1.8 2006/11/30 17:35:52 ping Exp $
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

#define _WIN32_DCOM 

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxassert.h"
#include "hxtick.h"
#include "hxslist.h"
#include "hlxosstr.h"
#include "win_fwctlmgr.h"

typedef struct
{
    LONG                lPort;
    NET_FW_IP_PROTOCOL  protocol;
} HXFWPorts;

// copied from netfw_i.c in MS Platform SDK
const IID IID_INetFwOpenPort = {0xE0483BA0,0x47FF,0x4D9C,{0xA6,0xD6,0x77,0x41,0xD0,0xB1,0x95,0xF7}};
const IID IID_INetFwAuthorizedApplication = {0xB5E64FFA,0xC2C5,0x444E,{0xA3,0x01,0xFB,0x5E,0x00,0x01,0x80,0x50}};

#define HX_FWPORT_IDENTIFIER    L"HelixDNAClient"

HXWinFirewallControlManager::HXWinFirewallControlManager()
		            :HXFirewallControlManager()
                            ,m_lastError(HXR_OK)
                            ,m_fwMgr(NULL)
                            ,m_fwProfile(NULL)
                            ,m_fwBstrName(NULL)
                            ,m_lLan(1024)
{
    HRESULT hr = S_OK;

    // Initialize COM.
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (S_OK == hr ||               // The COM library was initialized successfully on the calling thread
        S_FALSE ==  hr ||           // The COM library is already initialized on the calling thread.
        RPC_E_CHANGED_MODE == hr)   // A previous call to CoInitializeEx specified a different concurrency
                                    // model for the calling thread
    {
        if (HXR_OK == Init())
        {
            // create the identifier for the ports we modify
            m_fwBstrName = SysAllocString((const OLECHAR *)HX_FWPORT_IDENTIFIER);
            if (SysStringLen(m_fwBstrName) == 0)
            {
                m_lastError = HXR_OUTOFMEMORY;
            }
            else
            {
                // remove any left open ports by us from previous session
                ResetPorts();
                ResetApps();
                m_bInitialized = TRUE;
            }
        }
    }    
}

HXWinFirewallControlManager::~HXWinFirewallControlManager()
{    
    Close();
}

void
HXWinFirewallControlManager::Close()
{
    // remove any left open ports by us from this session
    ResetPorts();
    ResetApps();

    SysFreeString(m_fwBstrName);
    HX_RELEASE(m_fwMgr);
    HX_RELEASE(m_fwProfile);

    CoUninitialize();
    m_bInitialized = FALSE;

    return;
}

STDMETHODIMP_(HXBOOL)
HXWinFirewallControlManager::IsFirewallOn()
{
    HXBOOL    fwOn = FALSE;
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;

    if (!m_bInitialized)
    {
        goto exit;
    }

    // Get the current state of the firewall.
    hr = m_fwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Check to see if the firewall is on.
    if (fwEnabled == VARIANT_TRUE)
    {
        fwOn = TRUE;
    }

exit:

    return fwOn;
}

STDMETHODIMP
HXWinFirewallControlManager::TurnOnFirewall()
{
    HRESULT hr = S_OK;

    // no-op if firewall is on
    if (IsFirewallOn())
    {
        return HXR_OK;
    }

    if (!m_bInitialized)
    {
        return HXR_FAILED;
    }

    // Turn the firewall on.
    hr = m_fwProfile->put_FirewallEnabled(VARIANT_TRUE);
    if (SUCCEEDED(hr))
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAILED;
    }
}

STDMETHODIMP
HXWinFirewallControlManager::TurnOffFirewall()
{
    HRESULT hr = S_OK;

    // no-op if firewall is off
    if (!IsFirewallOn())
    {
        return HXR_OK;
    }

    HX_ASSERT(m_fwProfile);

    // Turn the firewall off.
    hr = m_fwProfile->put_FirewallEnabled(VARIANT_FALSE);
    if (SUCCEEDED(hr))
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAILED;
    }
}

STDMETHODIMP
HXWinFirewallControlManager::OpenApp(const char* pszAppName,
                                     const char* pszAppPath)
{
    HRESULT     hr = S_OK;
    HX_RESULT   rc = HXR_OK;
    wchar_t     pOutWideBuf[MAX_PATH];    
    BSTR        appBstrName = NULL;
    BSTR	appDisplayBstrName = NULL;
    INetFwAuthorizedApplication*    fwAuthorizedApp = NULL;
    INetFwAuthorizedApplications*   fwAuthorizedApps = NULL;

    if (!m_bInitialized)
    {
        rc = HXR_FAILED;
        goto exit;
    }

    if (!pszAppPath)
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    hr = m_fwProfile->get_AuthorizedApplications(&fwAuthorizedApps);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create an instance of an open port.
    hr = CoCreateInstance(
            __uuidof(NetFwAuthorizedApplication),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwAuthorizedApplication),
            reinterpret_cast<void**>(static_cast<INetFwAuthorizedApplication**>(&fwAuthorizedApp))
            );
    if (FAILED(hr))
    {
        goto exit;
    }

    if (pszAppName)
    {
	// convert string to unicode
	MultiByteToWideChar(0, 0, pszAppName, strlen(pszAppName) + 1, pOutWideBuf, MAX_PATH);
	appDisplayBstrName = SysAllocString((const OLECHAR *)pOutWideBuf);
	hr = fwAuthorizedApp->put_Name(appDisplayBstrName);
    }
    else
    {
        hr = fwAuthorizedApp->put_Name(m_fwBstrName);
    }

    if (FAILED(hr))
    {
        goto exit;
    }

    // convert string to unicode
    MultiByteToWideChar(0, 0, pszAppPath, strlen(pszAppPath) + 1, pOutWideBuf, MAX_PATH);
    appBstrName = SysAllocString((const OLECHAR *)pOutWideBuf);

    hr = fwAuthorizedApp->put_ProcessImageFileName(appBstrName);
    if (FAILED(hr))
    {
        goto exit;
    }

    // add app to exception list
    hr = fwAuthorizedApps->Add(fwAuthorizedApp);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:

    SysFreeString(appBstrName);
    SysFreeString(appDisplayBstrName);
    HX_RELEASE(fwAuthorizedApp);
    HX_RELEASE(fwAuthorizedApps);

    if (S_OK != hr && E_UNEXPECTED != hr)
    {
        rc = HXR_FAILED;
    }

    return rc;        
}

STDMETHODIMP
HXWinFirewallControlManager::CloseApp(const char* pszAppName,
                                      const char* pszAppPath)
{
    HRESULT     hr = S_OK;
    HX_RESULT   rc = HXR_OK;
    wchar_t     pOutWideBuf[MAX_PATH];    
    BSTR        appBstrName = NULL;
    INetFwAuthorizedApplications*   fwAuthorizedApps = NULL;

    if (!m_bInitialized)
    {
        rc = HXR_FAILED;
        goto exit;
    }

    if (!pszAppPath)
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    hr = m_fwProfile->get_AuthorizedApplications(&fwAuthorizedApps);
    if (FAILED(hr))
    {
        goto exit;
    }

    // convert string to unicode
    MultiByteToWideChar(0, 0, pszAppPath, strlen(pszAppPath) + 1, pOutWideBuf, MAX_PATH);
    appBstrName = SysAllocString((const OLECHAR *)pOutWideBuf);

    hr = fwAuthorizedApps->Remove(appBstrName);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:

    SysFreeString(appBstrName);
    HX_RELEASE(fwAuthorizedApps);

    if (FAILED(hr))
    {
        rc = HXR_FAILED;
    }

    return rc;        
}

STDMETHODIMP
HXWinFirewallControlManager::OpenPort(UINT32 ulPort, HX_NET_FW_IP_PROTOCOL protocol)
{
    HRESULT     hr = S_OK;
    HX_RESULT   rc = HXR_OK;
    int i = 0;
    HXBOOL bOpened = FALSE;
    VARIANT fwAllowed;
    VARIANT fwRestricted;
    BSTR fwBstrName = NULL;
    NET_FW_IP_PROTOCOL winprotocol;
    INetFwOpenPort* fwOpenPort = NULL;
    INetFwOpenPorts* fwOpenPorts = NULL;

    if (!m_bInitialized)
    {
        rc = HXR_FAILED;
        goto exit;
    }

    // Retrieve the globally open ports collection.
    hr = m_fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Attempt to retrieve the globally open port.
    if (HX_NET_FW_IP_PROTOCOL_UDP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_UDP;
    }
    else if (HX_NET_FW_IP_PROTOCOL_TCP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_TCP;
    }
    else
    {
        HX_ASSERT(FALSE);
        goto exit;
    }

    // Create an instance of an open port.
    hr = CoCreateInstance(
            __uuidof(NetFwOpenPort),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwOpenPort),
            reinterpret_cast<void**>(static_cast<INetFwOpenPort**>(&fwOpenPort))
            );
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the port number.
    hr = fwOpenPort->put_Port(ulPort);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the IP protocol.
    hr = fwOpenPort->put_Protocol(winprotocol);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the friendly name of the port.
    hr = fwOpenPort->put_Name(m_fwBstrName);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Opens the port and adds it to the collection.
    hr = fwOpenPorts->Add(fwOpenPort);
    if (FAILED(hr))
    {
        goto exit;
    }

    // There is an issue with Add(), Add() is async which means the port might not be 
    // added to the exception list even after the call returns success.
    //
    // To work around this, we call IsPortAllowed() to manually check the port status. 
    // We will attempt 10 times, if somehow the port is still not added, then we'll bail out
    // anyway. This prevents us from stucking in this loop too long.
    //
    // Microsoft has acknowledged this issue and will fix this in their future SDK release.
    // The case# is SRX041011609235
    for (i = 0; i< 10; i++)
    {
        UINT32 foo = HX_GET_TICKCOUNT();

	Sleep(10);   // relinquish the time slice in order to give firewall kernel 
                     // a chance to synchronize the exception list

        UINT32 bar = HX_GET_TICKCOUNT() - foo;

        hr = m_fwMgr->IsPortAllowed(NULL,
                                    NET_FW_IP_VERSION_ANY,
                                    ulPort,
                                    NULL,
                                    winprotocol,
                                    &fwAllowed,
                                    &fwRestricted);

        if (SUCCEEDED(hr) && fwAllowed.boolVal == VARIANT_TRUE)
        {
            bOpened = TRUE;
            break;
        }
    }

exit:

    // Release the globally open port.
    // Release the globally open ports collection.
    HX_RELEASE(fwOpenPort);
    HX_RELEASE(fwOpenPorts);

    if (S_OK != hr && E_UNEXPECTED != hr)
    {
        rc = HXR_FAILED;
    }

    return rc;
}

STDMETHODIMP
HXWinFirewallControlManager::ClosePort(UINT32 ulPort, HX_NET_FW_IP_PROTOCOL protocol)
{
    HRESULT     hr = S_OK;
    HX_RESULT   rc = HXR_OK;
    NET_FW_IP_PROTOCOL winprotocol;
    INetFwOpenPorts* fwOpenPorts = NULL;

    if (!m_bInitialized)
    {
        rc = HXR_FAILED;
        goto exit;
    }

    // Retrieve the globally open ports collection.
    hr = m_fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Attempt to retrieve the globally open port.
    if (HX_NET_FW_IP_PROTOCOL_UDP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_UDP;
    }
    else if (HX_NET_FW_IP_PROTOCOL_TCP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_TCP;
    }
    else
    {
        HX_ASSERT(FALSE);
        goto exit;
    }

    hr = fwOpenPorts->Remove(ulPort, winprotocol);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:

    // Release the globally open port.
    // Release the globally open ports collection.
    HX_RELEASE(fwOpenPorts);

    if (FAILED(hr))
    {
        rc = HXR_FAILED;
    }

    return rc;
}

STDMETHODIMP_(HXBOOL)
HXWinFirewallControlManager::IsAppExempted(const char* pszAppPath)
{
    HRESULT     hr = S_OK;
    HXBOOL        bExempted = FALSE;
    wchar_t     pOutWideBuf[MAX_PATH];
    BSTR        bstrName = NULL;
    BSTR        appBstrName = NULL;
    IUnknown*   pUnk = NULL;
    IDispatch*  pdispApps = NULL;
    VARIANT     varCurPorts;
    IEnumVARIANT*                   pEnumVar = NULL;
    INetFwAuthorizedApplication*    fwAuthorizedApp = NULL;
    INetFwAuthorizedApplications*   fwAuthorizedApps = NULL;

    if (!m_bInitialized || !pszAppPath)
    {
        goto exit;
    }

    hr = m_fwProfile->get_AuthorizedApplications(&fwAuthorizedApps);
    if (SUCCEEDED(hr))
    {
        // convert string to unicode
        MultiByteToWideChar(0, 0, pszAppPath, strlen(pszAppPath) + 1, pOutWideBuf, MAX_PATH);

        appBstrName = SysAllocString((const OLECHAR *)pOutWideBuf);

        hr = fwAuthorizedApps->get__NewEnum(&pUnk);
        if (SUCCEEDED(hr))
        {
            hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVar);
            if (SUCCEEDED(hr))
            {
                VariantInit(&varCurPorts);
                hr = pEnumVar->Reset();
        
                // Loop through each port in the collection
                while (S_OK == pEnumVar->Next(1, &varCurPorts, NULL))
                {
                    pdispApps = V_DISPATCH(&varCurPorts);

                    if (SUCCEEDED(pdispApps->QueryInterface(IID_INetFwAuthorizedApplication, 
                                                            (void **)&fwAuthorizedApp)))
                    {
                        // Get the name associated with the port
                        if (SUCCEEDED(fwAuthorizedApp->get_ProcessImageFileName(&bstrName)))
                        {
                            // Retrieve the port created by us(m_fwBstrName)
                            if (VARCMP_EQ == VarBstrCmp(bstrName, appBstrName, m_lLan, NORM_IGNORECASE))
                            {
                                bExempted = TRUE;

                                SysFreeString(bstrName);
                                HX_RELEASE(fwAuthorizedApp);
                                VariantClear(&varCurPorts);
                                break;
                            }
                            SysFreeString(bstrName);
                        }
                        HX_RELEASE(fwAuthorizedApp);
                    }
                    VariantClear(&varCurPorts);
                }
                HX_RELEASE(pEnumVar);
            }
            HX_RELEASE(pUnk);
        }
        SysFreeString(appBstrName);
    }

exit:
   
    return bExempted;
}

STDMETHODIMP_(HXBOOL)
HXWinFirewallControlManager::IsPortExempted(UINT32 ulPort, 
                                            HX_NET_FW_IP_PROTOCOL protocol)
{
    HRESULT hr = S_OK;
    HXBOOL bExempted = FALSE;
    VARIANT_BOOL fwEnabled;
    NET_FW_IP_PROTOCOL winprotocol;
    INetFwOpenPort* fwOpenPort = NULL;
    INetFwOpenPorts* fwOpenPorts = NULL;

    if (!m_bInitialized)
    {
        goto exit;
    }

    // Retrieve the globally open ports collection.
    hr = m_fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Attempt to retrieve the globally open port.
    if (HX_NET_FW_IP_PROTOCOL_UDP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_UDP;
    }
    else if (HX_NET_FW_IP_PROTOCOL_TCP == protocol)
    {
        winprotocol = NET_FW_IP_PROTOCOL_TCP;
    }
    else
    {
        HX_ASSERT(FALSE);
        goto exit;
    }

    hr = fwOpenPorts->Item(ulPort, winprotocol, &fwOpenPort);
    if (SUCCEEDED(hr))
    {
        // Find out if the globally open port is enabled.
        hr = fwOpenPort->get_Enabled(&fwEnabled);
        if (FAILED(hr))
        {
            goto exit;
        }

        if (fwEnabled == VARIANT_TRUE)
        {
            bExempted = TRUE;
            goto exit;
        }
    }

exit:

    HX_RELEASE(fwOpenPort);
    HX_RELEASE(fwOpenPorts);

    return bExempted;
}

HX_RESULT
HXWinFirewallControlManager::Init()
{
    HX_RESULT       rc = HXR_OK;
    HRESULT         hr = S_OK;
    INetFwPolicy*   fwPolicy = NULL;

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(
            __uuidof(NetFwMgr),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwMgr),
            reinterpret_cast<void**>(static_cast<INetFwMgr**>(&m_fwMgr))
            );
    if (FAILED(hr))
    {
        rc = HXR_FAILED;
        goto exit;
    }

    // Retrieve the local firewall policy.
    hr = m_fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr))
    {
        rc = HXR_FAILED;
        goto exit;
    }

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->get_CurrentProfile(&m_fwProfile);
    if (FAILED(hr))
    {
        rc = HXR_FAILED;
        goto exit;
    }

exit:

    // Release the local firewall policy.
    HX_RELEASE(fwPolicy);

    return rc;
}

HX_RESULT
HXWinFirewallControlManager::ResetPorts()
{
    HRESULT             hr = S_OK;
    CHXSimpleList*      pPortsRemoveList = NULL;
    CHXSimpleList::Iterator i;
    IUnknown*           pUnk = NULL;
    INetFwOpenPorts*    fwOpenPorts = NULL;
    IEnumVARIANT*       pEnumVar = NULL;

    // nothing to be reset if we are failed to initialize
    if (!m_bInitialized)
    {
        goto exit;
    }

    // Retrieve the globally open ports collection.
    hr = m_fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = fwOpenPorts->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVar);
        if (SUCCEEDED(hr))
        {
            VARIANT varCurPorts;
            VariantInit(&varCurPorts);
            hr = pEnumVar->Reset();
            
            // Loop through each port in the collection
            while (S_OK == pEnumVar->Next(1, &varCurPorts, NULL))
            {
                INetFwOpenPort* fwOpenPort = NULL;
                IDispatch*      pdispPorts = V_DISPATCH(&varCurPorts);

                if (SUCCEEDED(pdispPorts->QueryInterface(IID_INetFwOpenPort, (void **)&fwOpenPort)))
                {
                    // Get the name associated with the port
                    BSTR bstrName = NULL;
                    if (SUCCEEDED(fwOpenPort->get_Name(&bstrName)))
                    {
                        // Retrieve the port created by us(m_fwBstrName)
                        if (VARCMP_EQ == VarBstrCmp(bstrName, m_fwBstrName, m_lLan, NULL))
                        {
                            if (!pPortsRemoveList)
                            {
                                pPortsRemoveList = new CHXSimpleList();
                            }

                            if (pPortsRemoveList)
                            {
                                HXFWPorts* pPort = new HXFWPorts;                               
                                fwOpenPort->get_Port(&(pPort->lPort));
                                fwOpenPort->get_Protocol(&(pPort->protocol));

                                pPortsRemoveList->AddTail(pPort);
                            }
                        }
                        SysFreeString(bstrName);
                    }
                    HX_RELEASE(fwOpenPort);
                }
                VariantClear(&varCurPorts);
            }
            HX_RELEASE(pEnumVar);
        }
        HX_RELEASE(pUnk);
    }

    // Close the ports opened by us
    if (pPortsRemoveList)
    {
        for (i = pPortsRemoveList->Begin(); i != pPortsRemoveList->End(); ++i)
        {
	    HXFWPorts* pPort = (HXFWPorts*)(*i);
            fwOpenPorts->Remove(pPort->lPort, pPort->protocol);
            HX_DELETE(pPort);
        }
        HX_DELETE(pPortsRemoveList);
    }

exit:

    HX_RELEASE(fwOpenPorts);

    return HXR_OK;
}

HX_RESULT
HXWinFirewallControlManager::ResetApps()
{
    HRESULT             hr = S_OK;
    CHXSimpleList*      pAppsRemoveList = NULL;
    CHXSimpleList::Iterator i;
    IUnknown*           pUnk = NULL;
    IEnumVARIANT*       pEnumVar = NULL;
    INetFwAuthorizedApplication*    fwAuthorizedApp = NULL;
    INetFwAuthorizedApplications*   fwAuthorizedApps = NULL;

    // nothing to be reset if we are failed to initialize
    if (!m_bInitialized)
    {
        goto exit;
    }

    hr = m_fwProfile->get_AuthorizedApplications(&fwAuthorizedApps);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = fwAuthorizedApps->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVar);
        if (SUCCEEDED(hr))
        {
            VARIANT varCurPorts;
            VariantInit(&varCurPorts);
            hr = pEnumVar->Reset();
            
            // Loop through each port in the collection
            while (S_OK == pEnumVar->Next(1, &varCurPorts, NULL))
            {
                INetFwAuthorizedApplication*    fwAuthorizedApp = NULL;
                IDispatch*      pdispApps = V_DISPATCH(&varCurPorts);

                if (SUCCEEDED(pdispApps->QueryInterface(IID_INetFwAuthorizedApplication, 
                                                        (void **)&fwAuthorizedApp)))
                {
                    // Get the name associated with the port
                    BSTR    bstrDisplayName = NULL;
                    BSTR*   bstrAppName = NULL;
                    if (SUCCEEDED(fwAuthorizedApp->get_Name(&bstrDisplayName)))
                    {
                        // Retrieve the port created by us(m_fwBstrName)
                        if (VARCMP_EQ == VarBstrCmp(bstrDisplayName, m_fwBstrName, m_lLan, NULL))
                        {
                            if (!pAppsRemoveList)
                            {
                                pAppsRemoveList = new CHXSimpleList();
                            }

                            if (pAppsRemoveList)
                            {
                                fwAuthorizedApp->get_ProcessImageFileName(bstrAppName);
                                pAppsRemoveList->AddTail(bstrAppName);
                            }
                        }
                        SysFreeString(bstrDisplayName);
                    }
                    HX_RELEASE(fwAuthorizedApp);
                }
                VariantClear(&varCurPorts);
            }
            HX_RELEASE(pEnumVar);
        }
        HX_RELEASE(pUnk);
    }

    // Close the apps opened by us
    if (pAppsRemoveList)
    {
        for (i = pAppsRemoveList->Begin(); i != pAppsRemoveList->End(); ++i)
        {
	    BSTR* pAppName = (BSTR*)(*i);
            fwAuthorizedApps->Remove(*pAppName);
            SysFreeString(*pAppName);
        }
        HX_DELETE(pAppsRemoveList);
    }

exit:

    HX_RELEASE(fwAuthorizedApps);

    return HXR_OK;
}
