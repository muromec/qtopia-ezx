/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetif.cpp,v 1.23 2006/03/08 19:17:01 ping Exp $
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
#include <string.h>
#include <stdio.h>

#ifndef _WINCE
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#endif /* _WINCE */

#include "hlxclib/sys/socket.h"
#include "hlxclib/windows.h"
#include "hlxosstr.h"
#include "hxresult.h"
#include "hxslist.h"
#include "netbyte.h"
#include "hxengin.h"
#include "hxerror.h"
#include "errdbg.h"
#include "hxnetif.h"
#include "safestring.h"
#include "pckunpck.h"

#define CHECK_ADDR_CHANGE_INTERVAL  10000

void* NIThreadRoutine(void * pArg);

HXNetInterface::HXNetInterface(IUnknown* pContext)
		: m_lRefCount(0)
		, m_bInitialized(FALSE)
		, m_hIPLib(NULL)
		, m_hWinSockLib(NULL)
		, m_hAddrChangeEvent(NULL)
		, m_pScheduler(NULL)
                , m_pEM(NULL)
		, m_pAddrChangeCallback(NULL)
		, m_pNetInterfaceList(NULL)
		, m_pSinkList(NULL)
		, _pNotifyAddrChange(NULL)
		, _hxWSAStartup(NULL)
		, _hxWSACleanup(NULL)
#if defined HELIX_FEATURE_NETINTERFACES
		, _pGetAdaptersInfo(NULL)
                , _pGetAdaptersAddresses(NULL)
#endif /* HELIX_FEATURE_NETINTERFACES */
#ifndef _WINCE
		, _hxsocket(NULL)
		, _hxclosesocket(NULL)
		, _raWSAIoctl(NULL)
#endif /* _WINCE */
		, m_handle(NULL)
                , m_pThread(NULL)
                , m_pQuitEvent(NULL)
                , m_bIsDone(FALSE)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    if (m_pContext)
    {
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pEM);
    }
}

HXNetInterface::~HXNetInterface()
{
    Close();
}

STDMETHODIMP
HXNetInterface::QueryInterface(REFIID riid, void**ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetInterfaces))
    {
	AddRef();
	*ppvObj = (IHXNetInterfaces*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXNetInterface::AddRef()
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
HXNetInterface::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXNetInterface::UpdateNetInterfaces(void)
{
    HX_RESULT	    rc = HXR_OK;    

    if (!m_bInitialized)
    {
	m_bInitialized = TRUE;

	rc = RetrieveNetInterface0(m_pNetInterfaceList);
	if (HXR_OK != rc)
	{
            Reset(m_pNetInterfaceList);
	    // try the generic method
	    rc = RetrieveNetInterface1(m_pNetInterfaceList);
	}

        DEBUG_OUT(m_pEM, DOL_TRANSPORT, (s,"Active NetworkInterfaces = %lu", m_pNetInterfaceList?m_pNetInterfaceList->GetCount():0));

	if (_pNotifyAddrChange)
	{
	    m_hAddrChangeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	    m_overLapped.Internal = NULL;
	    m_overLapped.InternalHigh = NULL;
	    m_overLapped.Offset = 0;
	    m_overLapped.OffsetHigh = 0;
	    m_overLapped.hEvent = m_hAddrChangeEvent;

	    CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);
	    rc = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pThread, m_pContext);
            if (HXR_OK == rc)
            {
	        rc = m_pThread->CreateThread(NIThreadRoutine, (void*) this, 0);
                if (SUCCEEDED(rc))
                {
                    m_pThread->SetThreadName("Net Interface Change Thread");
                }
            }
        }
        else
        {
	    if (!m_pScheduler)
	    {
		m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
	    }

	    if (m_pScheduler)
	    {
		m_pAddrChangeCallback = new AddrChangeCallback;
		m_pAddrChangeCallback->m_pParent = this;
		m_pAddrChangeCallback->AddRef();

		m_pAddrChangeCallback->m_bIsCallbackPending = TRUE;
		m_pAddrChangeCallback->m_PendingHandle = m_pScheduler->RelativeEnter(m_pAddrChangeCallback, CHECK_ADDR_CHANGE_INTERVAL);
	    }
	}
    }
    else
    {
        IsNetInterfaceChanged(FALSE);
    }

    return rc;
}

STDMETHODIMP_(UINT32)
HXNetInterface::GetNumOfNetInterfaces()
{
    if (!m_bInitialized)
    {
	UpdateNetInterfaces();
    }

    return m_pNetInterfaceList ? m_pNetInterfaceList->GetCount() : 0;
}

STDMETHODIMP
HXNetInterface::GetNetInterfaces(UINT16	lIndex,
				 REF(NIInfo*)	pNIInfo)
{
    HX_RESULT	rc = HXR_OK;
    int		i = 0;
    CHXSimpleList::Iterator iter;

    pNIInfo = NULL;

    if (!m_bInitialized)
    {
	UpdateNetInterfaces();
    }

    if (m_pNetInterfaceList)
    {
	iter = m_pNetInterfaceList->Begin();
	for (; iter != m_pNetInterfaceList->End(); ++iter, ++i)
	{
	    NIInfo* pInfo = (NIInfo*)(*iter);
	    if (i == lIndex)
	    {
		pNIInfo = pInfo;
		break;
	    }
	}
    }

    if (!pNIInfo)
    {
	rc = HXR_FAILED;
    }

    return rc;
}

STDMETHODIMP
HXNetInterface::AddAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pSinkList)
    {
	m_pSinkList = new CHXSimpleList();
    }

    pSink->AddRef();
    m_pSinkList->AddTail(pSink);

    return rc;
}

STDMETHODIMP
HXNetInterface::RemoveAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT	rc = HXR_OK;

    LISTPOSITION lPosition = m_pSinkList->Find(pSink);

    if (!lPosition)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_pSinkList->RemoveAt(lPosition);
    pSink->Release();

cleanup:

    return rc;
}

HX_RESULT
HXNetInterface::RetrieveNetInterface0(CHXSimpleList*& pNetInterfaceList)
{
#if defined HELIX_FEATURE_NETINTERFACES
    HX_RESULT		rc = HXR_OK;

    if (!m_hIPLib)
    {
	m_hIPLib = LoadLibrary(OS_STRING("IPHLPAPI.DLL"));
	if (m_hIPLib)
	{
            _pGetAdaptersInfo = (GETADAPTERSINFO)::GetProcAddress(m_hIPLib, OS_STRING("GetAdaptersInfo"));
            _pGetAdaptersAddresses = (GETADAPTERSADDRESSES)::GetProcAddress(m_hIPLib, OS_STRING("GetAdaptersAddresses"));
            _pNotifyAddrChange = (NOTIFYADDRCHANGE)::GetProcAddress(m_hIPLib, OS_STRING("NotifyAddrChange"));
	}
    }

    // _pGetAdaptersAddresses is only supported on XP
    // _pGetAdaptersInfo is the fallback method in iphlpapi.dll for the rest of Windows
    if ((_pGetAdaptersAddresses || _pGetAdaptersInfo) &&
	_pNotifyAddrChange)
    {
        rc = HXGetAdaptersAddresses(pNetInterfaceList);
        if (HXR_OK != rc)
        {
            rc = HXGetAdaptersInfo(pNetInterfaceList);
        }
    }

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_NETINTERFACES */
}

HX_RESULT
HXNetInterface::RetrieveNetInterface1(CHXSimpleList*& pNetInterfaceList)
{
#ifdef _WINCE
    return HXR_NOT_SUPPORTED;
#else

    HX_RESULT	    rc = HXR_OK;
    int		    iStructures = 0;
    int		    iPosition = 0;
    long	    lFlags = 0;
    UINT32	    ulBytes = 0; 
    UINT32	    ulNetAddress = 0;
    UINT32	    ulNetMask = 0;
    char            szValue[MAX_IPv4_ADDRESS_LENGTH];
    SOCKET	    s = 0;
    WORD	    wVersionRequested = 0;
    NIType	    type = NI_UNKNOWN;
    NIInfo*	    pNIInfo = NULL;
    WSADATA	    wsaData;
    IHXBuffer*      pBuffer = NULL;
    INTERFACE_INFO* pInfo = NULL;

    if (!m_hWinSockLib)
    {
	m_hWinSockLib = LoadLibrary(OS_STRING("ws2_32.dll"));

	if (m_hWinSockLib)
	{
	    _hxWSAStartup = (WSASTARTUP)GetProcAddress(m_hWinSockLib, OS_STRING("WSAStartup"));
	    _hxWSACleanup = (WSACLEANUP)GetProcAddress(m_hWinSockLib, OS_STRING("WSACleanup"));
	    _hxsocket = (HXSOCKET)GetProcAddress(m_hWinSockLib, OS_STRING("socket"));
	    _hxclosesocket = (CLOSESOCKET)GetProcAddress(m_hWinSockLib, OS_STRING("closesocket"));
	    _raWSAIoctl = (WSAIOCTL)GetProcAddress(m_hWinSockLib, OS_STRING("WSAIoctl"));
	}
    }

    if (!_hxsocket	||
	!_hxclosesocket ||
	!_hxWSAStartup  ||
	!_hxWSACleanup  ||
	!_raWSAIoctl)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    wVersionRequested = (1 << 8) + 1;	
    _hxWSAStartup(wVersionRequested, &wsaData);

    s = _hxsocket(AF_INET, SOCK_DGRAM, 0);
    
    if (s == INVALID_SOCKET)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pInfo = new INTERFACE_INFO[MAX_INTERFACES];

    // get interfaces list
    if (_raWSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, NULL, pInfo,
		    sizeof(INTERFACE_INFO)*MAX_INTERFACES, &ulBytes, NULL, NULL))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    // caculate # of interfaces we have
    iStructures = ulBytes / sizeof(INTERFACE_INFO);

    // iterate through the interface list
    for (iPosition = 0; iPosition < iStructures; iPosition++)
    {
	lFlags = pInfo[iPosition].iiFlags;

        if ((lFlags & IFF_POINTTOPOINT) && (lFlags & IFF_UP))
	{
	    type = NI_PPP;
	}
	else if ((lFlags & IFF_BROADCAST) && (lFlags & IFF_UP))
	{
	    type = NI_ETHERNET;
	}
	else
	{
	    continue;
	}

	pNIInfo = new NIInfo;
        if (!pNIInfo)
        {
            rc = HXR_OUTOFMEMORY;
            goto cleanup;
        }

	pNIInfo->type = type;
        pNIInfo->status = NI_OPER_STATUS_OPERATIONAL;

        pNIInfo->pAddressInfo = new NIAddressInfo;
        pNIInfo->pAddressInfo->type = NI_ADDR_IPv4;
        
        memset(szValue, 0, MAX_IPv4_ADDRESS_LENGTH);        
        const struct sockaddr_in* psa = reinterpret_cast<const struct sockaddr_in*>(&pInfo[iPosition].iiAddress);
        const u_char* pin = &psa->sin_addr.S_un.S_un_b.s_b1;
        sprintf(szValue, "%u.%u.%u.%u", pin[0], pin[1], pin[2], pin[3]);

        pBuffer = NULL;
        if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
        {
            pBuffer->Set((const unsigned char*)szValue, strlen(szValue) + 1);
            pNIInfo->pAddressInfo->pAddress = pBuffer;
        }
        
        memset(szValue, 0, MAX_IPv4_ADDRESS_LENGTH);        
        psa = reinterpret_cast<const struct sockaddr_in*>(&pInfo[iPosition].iiNetmask);
        pin = &psa->sin_addr.S_un.S_un_b.s_b1;
        sprintf(szValue, "%u.%u.%u.%u", pin[0], pin[1], pin[2], pin[3]);

        pBuffer = NULL;
        if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
        {
            pBuffer->Set((const unsigned char*)szValue, strlen(szValue) + 1);
            pNIInfo->pAddressInfo->pSubnet = pBuffer;
            pNIInfo->pAddressInfo->ulSubnetPrefix = CalculateIPv4Prefix((UINT8*)&pin);
        }

	if (!pNetInterfaceList)
	{
	    pNetInterfaceList = new CHXSimpleList();
	}

	pNetInterfaceList->AddTail(pNIInfo);
    }

cleanup:

    if (_hxclosesocket)
    {
	_hxclosesocket(s);
    }

    if (_hxWSACleanup)
    {
	_hxWSACleanup();
    }

    HX_VECTOR_DELETE(pInfo);

    return rc;
#endif
}	

#if defined HELIX_FEATURE_NETINTERFACES
HX_RESULT           
HXNetInterface::HXGetAdaptersAddresses(CHXSimpleList*& pNetInterfaceList)
{
    HX_RESULT                   rc = HXR_OK;
    UINT32                      ulSize = 0;
    DWORD                       dwResult = NO_ERROR;
    NIInfo*		        pNIInfo = NULL;
    NIAddressInfo*              pCurAddressInfo = NULL;
    NIAddressInfo*              pPreAddressInfo = NULL;
    IHXBuffer*                  pBuffer = NULL;
    PIP_ADAPTER_ADDRESSES       pAddressInfo = NULL;
    PIP_ADAPTER_ADDRESSES       pAddressInfoIter = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pAddress = NULL;
    PIP_ADAPTER_PREFIX          pPrefix = NULL;

    if (!_pGetAdaptersAddresses)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    dwResult = _pGetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, 0, &ulSize);
    if (ERROR_BUFFER_OVERFLOW != dwResult)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    pAddressInfo = (PIP_ADAPTER_ADDRESSES)(new char[ulSize]);
    if (!pAddressInfo)
    {
        rc = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    dwResult = _pGetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, pAddressInfo, &ulSize);
    if (ERROR_SUCCESS != dwResult)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pAddressInfoIter = pAddressInfo;
    while (pAddressInfoIter != NULL)
    {
        // sanity check, pAddressInfo may not return all the info we need, bail out.
        if (pAddressInfoIter->Length != sizeof(IP_ADAPTER_ADDRESSES))
        {
            rc = HXR_FAILED;
            goto cleanup;
        }

        // we're interested in the active ones
        if (IfOperStatusUp == pAddressInfoIter->OperStatus)
        {
	    pNIInfo = new NIInfo;
	    if (!pNIInfo)
	    {
		rc = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

            switch (pAddressInfoIter->IfType)
            {
            case IF_TYPE_ETHERNET_CSMACD:
            case IF_TYPE_ETHERNET_3MBIT:
            case IF_TYPE_GIGABITETHERNET:
                pNIInfo->type = NI_ETHERNET;
                break;
            case IF_TYPE_ISO88025_TOKENRING:
                pNIInfo->type = NI_TOKENRING;
                break;
            case IF_TYPE_FDDI:
                pNIInfo->type = NI_FDDI;
            case IF_TYPE_PPP:
                pNIInfo->type = NI_PPP;
                break;
            case IF_TYPE_SOFTWARE_LOOPBACK:
                pNIInfo->type = NI_LOOPBACK;
                break;
            case IF_TYPE_SLIP:
                pNIInfo->type = NI_SLIP;
                break;
            default:
                pNIInfo->type = NI_UNKNOWN;
                break;
            }

	    pNIInfo->status = NI_OPER_STATUS_OPERATIONAL;
            pNIInfo->ulIPv4Index = pAddressInfoIter->IfIndex;
            pNIInfo->ulIPv6Index = pAddressInfoIter->Ipv6IfIndex;
            pNIInfo->ulMTU = pAddressInfoIter->Mtu;

            if (pAddressInfoIter->Description)
            {
                pBuffer = NULL;
                if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                {
                    pBuffer->Set((const unsigned char*)pAddressInfoIter->Description, 
                                 strlen((const char*)pAddressInfoIter->Description) + 1);
                }
                pNIInfo->pDescription = pBuffer;
            }

            pAddress = pAddressInfoIter->FirstUnicastAddress;
            pPrefix = pAddressInfoIter->FirstPrefix;
            pPreAddressInfo = NULL;
            while (pAddress)
            {
                pCurAddressInfo = new NIAddressInfo;;

                struct sockaddr* pa = (struct sockaddr*)pAddress->Address.lpSockaddr;
                switch (pa->sa_family)
                {
                case AF_INET:
                    pCurAddressInfo->type = NI_ADDR_IPv4;
                    break;
                case AF_INET6:
                    pCurAddressInfo->type = NI_ADDR_IPv6;
                    break;
                default:
                    HX_ASSERT(FALSE);
                    break;
                }

                // IP
                if (NI_ADDR_UNKNOWN != pCurAddressInfo->type)
                {
                    pBuffer = NULL;
                    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                    {
                        if (0 == NetAddressToAscii(pa, pBuffer))
                        {
                            pCurAddressInfo->pAddress = pBuffer;
                        }
                        else
                        {
                            HX_RELEASE(pBuffer);
                        }
                    }

                    // Subnet
                    if (pPrefix)
                    {
                        pa = (struct sockaddr*)pPrefix->Address.lpSockaddr;
                        pBuffer = NULL;
                        if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                        {
                            if (0 == NetAddressToAscii(pa, pBuffer))
                            {
                                pCurAddressInfo->pSubnet = pBuffer;
                            }
                            else
                            {
                                HX_RELEASE(pBuffer);
                            }
                        }
                        pCurAddressInfo->ulSubnetPrefix = pPrefix->PrefixLength;
                    }
                }

                if (!pPreAddressInfo)
                {
                    pNIInfo->pAddressInfo = pPreAddressInfo = pCurAddressInfo;
                }
                else
                {
                    pPreAddressInfo->next = pCurAddressInfo;
                    pPreAddressInfo = pCurAddressInfo;
                }

                pAddress = pAddress->Next;
                pPrefix = pPrefix ? pPrefix->Next : NULL;
            }

	    if (!pNetInterfaceList)
	    {
		pNetInterfaceList = new CHXSimpleList();
		if (!pNetInterfaceList)
		{
		    rc = HXR_OUTOFMEMORY;
		    goto cleanup;
		}
	    }

	    pNetInterfaceList->AddTail(pNIInfo);
        }
        pAddressInfoIter = pAddressInfoIter->Next;
    }

cleanup:

    HX_VECTOR_DELETE(pAddressInfo);

    return rc;
}

HX_RESULT
HXNetInterface::HXGetAdaptersInfo(CHXSimpleList*& pNetInterfaceList)
{
    HX_RESULT		rc = HXR_OK;
    UINT32		ulSize = 0;
    UINT32		ulNetAddress = 0;
    UINT32		ulNetMask = 0;
    DWORD               dwResult = NO_ERROR;
    PIP_ADAPTER_INFO    pAdapterInfo = NULL;
    PIP_ADDR_STRING     pAddrList = NULL;
    IHXBuffer*          pBuffer = NULL;

    if (!_pGetAdaptersInfo)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    // first make a call with ulSize = 0 to get the exact size needed
    dwResult = _pGetAdaptersInfo(pAdapterInfo, &ulSize);
    if (ERROR_BUFFER_OVERFLOW != dwResult)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    // allocate right amount of space for adapter info
    pAdapterInfo = (PIP_ADAPTER_INFO)(new char[ulSize]);
    if (!pAdapterInfo)
    {
	rc = HXR_OUTOFMEMORY;
	goto cleanup;
    }

    // fill up adapters info
    dwResult= _pGetAdaptersInfo(pAdapterInfo, &ulSize);
    if (ERROR_SUCCESS != dwResult)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    // step thru the adapters list that we received
    while (pAdapterInfo != NULL)
    {
        NIInfo*		pNIInfo = NULL;
        NIAddressInfo*  pCurAddressInfo = NULL;
        NIAddressInfo*  pPreAddressInfo = NULL;

	// step thru all IP addresses for each adapter
	pAddrList = &(pAdapterInfo->IpAddressList);
	do
	{
	    ulNetAddress = HXinet_addr(pAddrList->IpAddress.String);
	    ulNetMask = HXinet_addr(pAddrList->IpMask.String);

	    // ignore the net interface with invalid IP
	    if (INADDR_NONE == ulNetAddress || 0 == ulNetAddress)
            {
                break;
            }

            if (!pNIInfo)
            {
	        pNIInfo = new NIInfo;
	        if (!pNIInfo)
	        {
		    rc = HXR_OUTOFMEMORY;
		    goto cleanup;
	        }

                switch (pAdapterInfo->Type)
                {
                case MIB_IF_TYPE_ETHERNET:
                    pNIInfo->type = NI_ETHERNET;
                    break;
                case MIB_IF_TYPE_TOKENRING:
                    pNIInfo->type = NI_TOKENRING;
                    break;
                case MIB_IF_TYPE_FDDI:
                    pNIInfo->type = NI_FDDI;
                case MIB_IF_TYPE_PPP:
                    pNIInfo->type = NI_PPP;
                    break;
                case MIB_IF_TYPE_LOOPBACK:
                    pNIInfo->type = NI_LOOPBACK;
                    break;
                case MIB_IF_TYPE_SLIP:
                    pNIInfo->type = NI_SLIP;
                    break;
                default:
                    pNIInfo->type = NI_UNKNOWN;
                    break;
                }

	        pNIInfo->status = NI_OPER_STATUS_OPERATIONAL;
                pNIInfo->ulIPv4Index = pAdapterInfo->Index;

                if (pAdapterInfo->Description)
                {
                    pBuffer = NULL;
                    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                    {
                        pBuffer->Set((const unsigned char*)pAdapterInfo->Description, strlen(pAdapterInfo->Description) + 1);
                    }

                    pNIInfo->pDescription = pBuffer;
                }
            }

            // IP
            if (pAddrList->IpAddress.String)
            {
                pCurAddressInfo = new NIAddressInfo;;                   
                pCurAddressInfo->type = NI_ADDR_IPv4;

                pBuffer = NULL;
                if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                {
                    pBuffer->Set((const unsigned char*)pAddrList->IpAddress.String, strlen(pAddrList->IpAddress.String) + 1);
                    pCurAddressInfo->pAddress = pBuffer;
                }

                // Subnet
                if (pAddrList->IpMask.String)
                {
                    UINT32 ulSubnet = ulNetAddress & ulNetMask;

                    pBuffer = NULL;
                    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
                    {
                        NetLongToAscii(DwToHost(ulSubnet), pBuffer);
                        pCurAddressInfo->pSubnet = pBuffer;
                    }

                    UINT8 pin[4];
                    pin[3] = (UINT8)((ulNetMask & 0xFF000000) >> 24);
                    pin[2] = (UINT8)((ulNetMask & 0x00FF0000) >> 16);
                    pin[1] = (UINT8)((ulNetMask & 0x0000FF00) >> 8);
                    pin[0] = (UINT8)(ulNetMask & 0x000000FF);

                    pCurAddressInfo->ulSubnetPrefix = CalculateIPv4Prefix((UINT8*)&pin);
                }

                if (!pPreAddressInfo)
                {
                    pNIInfo->pAddressInfo = pPreAddressInfo = pCurAddressInfo;
                }
                else
                {
                    pPreAddressInfo->next = pCurAddressInfo;
                    pPreAddressInfo = pCurAddressInfo;
                }
            }

	    pAddrList = pAddrList->Next;
        } while (pAddrList != NULL);

	if (pNIInfo)
        {
            if (!pNetInterfaceList)
	    {
	        pNetInterfaceList = new CHXSimpleList();
	        if (!pNetInterfaceList)
	        {   
		    rc = HXR_OUTOFMEMORY;
		    goto cleanup;
	        }
	    }   
	    pNetInterfaceList->AddTail(pNIInfo);
        }

	pAdapterInfo = pAdapterInfo->Next;
    }

cleanup:

    HX_VECTOR_DELETE(pAdapterInfo);

    return rc;
}
#endif /* HELIX_FEATURE_NETINTERFACES */

HXBOOL
HXNetInterface::IsNetInterfaceChanged(HXBOOL bCallback)
{
    HX_RESULT       rc = HXR_OK;
    HXBOOL	    bResult = FALSE;
    CHXSimpleList*  pTempNetInterfaceList = NULL;

    // manually poll the network interfaces and compare them
    rc = RetrieveNetInterface0(pTempNetInterfaceList);
    if (HXR_OK != rc)
    {
	// try the generic method
        Reset(pTempNetInterfaceList);
	rc = RetrieveNetInterface1(pTempNetInterfaceList);
    }

    // we switched from no-network to network or vice versa
    if ((!m_pNetInterfaceList && pTempNetInterfaceList) ||
        (m_pNetInterfaceList && !pTempNetInterfaceList))
    {
        bResult = TRUE;
    }
    else if (m_pNetInterfaceList && pTempNetInterfaceList)
    {
	if (pTempNetInterfaceList->GetCount() != m_pNetInterfaceList->GetCount())
	{
	    bResult = TRUE;
	}
	else
	{
	    CHXSimpleList::Iterator ndx0 = pTempNetInterfaceList->Begin();
	    CHXSimpleList::Iterator ndx1 = m_pNetInterfaceList->Begin();
	    for (; ndx0 != pTempNetInterfaceList->End() && ndx1 != m_pNetInterfaceList->End(); ++ndx0, ++ndx1)
	    {
		NIInfo* pInfo0 = (NIInfo*)(*ndx0);
		NIInfo* pInfo1 = (NIInfo*)(*ndx1);

                if (pInfo0->pAddressInfo && pInfo1->pAddressInfo)
                {
                    if (pInfo0->pAddressInfo->pAddress &&
                        pInfo1->pAddressInfo->pAddress)
                    {
                        if (0 != strcmp((const char*)pInfo0->pAddressInfo->pAddress->GetBuffer(),
                                        (const char*)pInfo1->pAddressInfo->pAddress->GetBuffer()))
                        {
                            bResult = TRUE;
                            break;
                        }
                    }
                    else if (pInfo0->pAddressInfo->pAddress != pInfo1->pAddressInfo->pAddress)
                    {
                        bResult = TRUE;    
                        break;
                    }
                }
                else if (pInfo0->pAddressInfo != pInfo1->pAddressInfo)
                {
                    bResult = TRUE;
                    break;
                }
	    }
	}

	if (bResult)
	{
	    Reset(m_pNetInterfaceList);	
	    HX_DELETE(m_pNetInterfaceList);
	}
	else
	{
	    Reset(pTempNetInterfaceList);
	    HX_DELETE(pTempNetInterfaceList);
	}
    }

    if (bResult)
    {   
        m_pNetInterfaceList = pTempNetInterfaceList;

        DEBUG_OUT(m_pEM, DOL_TRANSPORT, (s,"Active NetworkInterfaces = %lu", m_pNetInterfaceList?m_pNetInterfaceList->GetCount():0));

        if (bCallback)
        {
            if (m_pSinkList)
            {
	        CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
	        for (; ndx != m_pSinkList->End(); ++ndx)
	        {
	            IHXNetInterfacesAdviseSink* pSink = (IHXNetInterfacesAdviseSink*) (*ndx);
	            pSink->NetInterfacesUpdated();
	        }
            }

            if (m_pAddrChangeCallback)
            {
                HX_ASSERT(!m_pAddrChangeCallback->m_bIsCallbackPending);

	        m_pAddrChangeCallback->m_bIsCallbackPending = TRUE;
	        m_pAddrChangeCallback->m_PendingHandle = m_pScheduler->RelativeEnter(m_pAddrChangeCallback, 
                                                                                     CHECK_ADDR_CHANGE_INTERVAL);
            }
        }
    }

    return bResult;
}

void
HXNetInterface::Reset(CHXSimpleList* pNetInterfaceList)
{
    if (pNetInterfaceList)
    {
	while (pNetInterfaceList->GetCount())
	{
	    NIInfo* pNIInfo = (NIInfo*)pNetInterfaceList->RemoveHead();
	    HX_DELETE(pNIInfo);
	}
    }
}

void
HXNetInterface::Close(void)
{
    m_bIsDone = TRUE;

    if (m_pAddrChangeCallback)
    {
	if (m_pAddrChangeCallback->m_bIsCallbackPending)
	{
	    m_pAddrChangeCallback->m_bIsCallbackPending = FALSE;
	    m_pScheduler->Remove(m_pAddrChangeCallback->m_PendingHandle);
	    m_pAddrChangeCallback->m_PendingHandle = 0;
	}

	HX_RELEASE(m_pAddrChangeCallback);
    }

    if (m_hAddrChangeEvent)
    {
        if (m_pThread)
        {
            SetEvent(m_hAddrChangeEvent);
	    
            m_pQuitEvent->Wait(ALLFS);
	    m_pThread->Exit(0);
            
            HX_RELEASE(m_pThread);
            HX_RELEASE(m_pQuitEvent);  
        }

	CloseHandle(m_hAddrChangeEvent);
	m_hAddrChangeEvent = NULL;
    }

    if (m_handle)
    {
#ifndef _WINCE
	CancelIo(m_handle);
#else
	CloseHandle(m_handle);
#endif /* _WINCE */
	// XXX HP
	// Confirmed with MS TS that the caller doesn't need to
	// call CloseHandle() on m_handle, it's taken care of by
	// the FreeLibrary()
	m_handle = NULL;
    }

    Reset(m_pNetInterfaceList);
    HX_DELETE(m_pNetInterfaceList);

    if (m_pSinkList)
    {
	HX_ASSERT(m_pSinkList->GetCount() == 0);
	CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
	for (; ndx != m_pSinkList->End(); ++ndx)
	{
	    IHXNetInterfacesAdviseSink* pSink = (IHXNetInterfacesAdviseSink*) (*ndx);
	    HX_RELEASE(pSink);
	}
	HX_DELETE(m_pSinkList);
    }


    if (m_hIPLib)
    {
	FreeLibrary(m_hIPLib);
	m_hIPLib = NULL;
    }

    if (m_hWinSockLib)
    {
	FreeLibrary(m_hWinSockLib);
	m_hWinSockLib = NULL;
    }

    HX_RELEASE(m_pEM);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pContext);
}

void* NIThreadRoutine(void * pArg)
{
    HXNetInterface* pHXNetInterface = (HXNetInterface*) pArg;

    if (!pHXNetInterface || !pHXNetInterface->_pNotifyAddrChange)
    {
        return (void*) 0;
    }

    while (!pHXNetInterface->m_bIsDone)
    {
#ifndef _WINCE
	if (ERROR_IO_PENDING == pHXNetInterface->_pNotifyAddrChange(&pHXNetInterface->m_handle, 
                                                                    &pHXNetInterface->m_overLapped))
#else
	// Overlapped param is not supported in wince 3.0 in the second param to NotifyAddrChange	
	if(NO_ERROR == pHXNetInterface->_pNotifyAddrChange(&pHXNetInterface->m_hAddrChangeEvent, NULL))
#endif /* _WINCE */    
        {
            if (WaitForSingleObject(pHXNetInterface->m_hAddrChangeEvent, INFINITE) == WAIT_OBJECT_0 )
            {
                if (!pHXNetInterface->m_bIsDone)
                {
                    pHXNetInterface->IsNetInterfaceChanged(TRUE);
                }
            }
        }
        else
        {
            break;
        }
    }

    pHXNetInterface->m_pQuitEvent->SignalEvent();

    return (void*) 0;
}

AddrChangeCallback::AddrChangeCallback() :
     m_lRefCount (0)
    ,m_pParent (0)
    ,m_PendingHandle (0)
    ,m_bIsCallbackPending (FALSE)
{
}

AddrChangeCallback::~AddrChangeCallback()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP AddrChangeCallback::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
		{ GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) AddrChangeCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) AddrChangeCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP AddrChangeCallback::Func(void)
{
    m_PendingHandle         = 0;
    m_bIsCallbackPending    = FALSE;

    if (m_pParent)
    {
	m_pParent->IsNetInterfaceChanged(TRUE);
    }

    return HXR_OK;
}


STDMETHODIMP_(HXBOOL) AddrChangeCallback::IsInterruptSafe()
{
    return FALSE;
}
