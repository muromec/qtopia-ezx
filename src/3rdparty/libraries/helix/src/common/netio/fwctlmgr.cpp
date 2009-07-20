/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fwctlmgr.cpp,v 1.5 2007/07/06 20:43:53 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxassert.h"
#include "fwctlmgr.h"

#if defined(_WIN32)
#include "platform/win/win_fwctlmgr.h"
#endif /* _WIN32 */

HXFirewallControlManager::HXFirewallControlManager()
		         :m_lRefCount(0)
                         ,m_bInitialized(FALSE)
{
}

HXFirewallControlManager::~HXFirewallControlManager()
{
}

STDMETHODIMP
HXFirewallControlManager::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXFirewallControlManager), (IHXFirewallControlManager*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFirewallControlManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFirewallControlManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFirewallControlManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HXFirewallControlManager* HXFirewallControlManager::Create()
{
    HXFirewallControlManager*	pFWCtlMgr = NULL;

#if defined(_WIN32)
    pFWCtlMgr = new HXWinFirewallControlManager();
#endif /* _WIN32 */

    return pFWCtlMgr;
}

void 
HXFirewallControlManager::Close()
{
    return;
}

STDMETHODIMP_(HXBOOL)
HXFirewallControlManager::IsFirewallOn()
{
    return FALSE;
}

STDMETHODIMP
HXFirewallControlManager::TurnOnFirewall()
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXFirewallControlManager::TurnOffFirewall()
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXFirewallControlManager::OpenApp(const char* pszAppName,
                                  const char* pszAppPath)
{
    return HXR_NOTIMPL;
}
STDMETHODIMP
HXFirewallControlManager::CloseApp(const char* pszAppName,
                                   const char* pszAppPath)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXFirewallControlManager::OpenPort(UINT32 ulPort, 
                                   HX_NET_FW_IP_PROTOCOL protocol)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXFirewallControlManager::ClosePort(UINT32 ulPort, 
                                    HX_NET_FW_IP_PROTOCOL protocol)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(HXBOOL)
HXFirewallControlManager::IsAppExempted(const char* pszAppPath)
{
    return FALSE;
}

STDMETHODIMP_(HXBOOL)
HXFirewallControlManager::IsPortExempted(UINT32 ulPort, 
                                         HX_NET_FW_IP_PROTOCOL protocol)
{
    return FALSE;
}
