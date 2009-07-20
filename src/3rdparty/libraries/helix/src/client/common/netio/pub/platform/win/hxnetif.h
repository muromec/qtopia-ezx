/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetif.h,v 1.17 2007/07/06 21:58:03 jfinnecy Exp $
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

#ifndef _HXNETIF_H_
#define _HXNETIF_H_

#define MAX_INTERFACES	20

#include "hlxclib/sys/socket.h"
#include "hxthread.h"
#include "hxerror.h"

#include <iprtrmib.h>

#if defined HELIX_FEATURE_NETINTERFACES
#include <iptypes.h>
typedef DWORD (PASCAL FAR   *GETADAPTERSINFO) (PIP_ADAPTER_INFO, PULONG);
typedef DWORD (PASCAL FAR   *GETADAPTERSADDRESSES) (ULONG, DWORD, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
#endif /* HELIX_FEATURE_NETINTERFACES */

typedef int (PASCAL FAR	*WSASTARTUP)( WORD, LPWSADATA );
typedef int (PASCAL FAR *WSACLEANUP)( void );

#ifndef _WINCE
typedef SOCKET (PASCAL FAR *HXSOCKET)( int, int, int );
typedef int (PASCAL FAR *CLOSESOCKET)( SOCKET );
typedef int (PASCAL FAR *WSAIOCTL)( SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
#endif /* _WINCE */

typedef DWORD (PASCAL FAR   *NOTIFYADDRCHANGE)(PHANDLE, LPOVERLAPPED);


class AddrChangeCallback;

class HXNetInterface : public IHXNetInterfaces
{
private:
    LONG32		m_lRefCount;

    HXBOOL		m_bInitialized;
    CHXSimpleList*	m_pNetInterfaceList;
    CHXSimpleList*	m_pSinkList;

    WSASTARTUP		_hxWSAStartup;
    WSACLEANUP		_hxWSACleanup;

#if defined HELIX_FEATURE_NETINTERFACES
    GETADAPTERSINFO         _pGetAdaptersInfo;
    GETADAPTERSADDRESSES    _pGetAdaptersAddresses;

    HX_RESULT           HXGetAdaptersAddresses(CHXSimpleList*& pNetInterfaceList);
    HX_RESULT           HXGetAdaptersInfo(CHXSimpleList*& pNetInterfaceList);
#endif /* HELIX_FEATURE_NETINTERFACES */

#ifndef _WINCE
    HXSOCKET		_hxsocket;
    CLOSESOCKET		_hxclosesocket;
    WSAIOCTL		_raWSAIoctl;
#endif /* _WINCE */

    HINSTANCE		m_hIPLib;
    HINSTANCE		m_hWinSockLib;

    AddrChangeCallback*	m_pAddrChangeCallback;

    IUnknown*		m_pContext;
    IHXScheduler*	m_pScheduler;
    IHXErrorMessages*   m_pEM;

    HX_RESULT		RetrieveNetInterface0(CHXSimpleList*& pNetInterfaceList);
    HX_RESULT		RetrieveNetInterface1(CHXSimpleList*& pNetInterfaceList);
    void		Reset(CHXSimpleList* pNetInterfaceList);

    friend class AddrChangeCallback;

    ~HXNetInterface();


public:
    HXNetInterface(IUnknown* pContext);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXNetworkInterfaces methods
     */
    STDMETHOD(UpdateNetInterfaces)		(THIS);

    STDMETHOD_(UINT32, GetNumOfNetInterfaces)	(THIS);

    STDMETHOD(GetNetInterfaces)			(THIS_
						UINT16		lIndex,
						REF(NIInfo*)	pNIInfo);

    STDMETHOD(AddAdviseSink)			(THIS_
						IHXNetInterfacesAdviseSink* pSink);

    STDMETHOD(RemoveAdviseSink)			(THIS_
						IHXNetInterfacesAdviseSink* pSink);

    HXBOOL    IsNetInterfaceChanged(HXBOOL bCallback);
    void    Close(void);

    IHXThread*          m_pThread;
    IHXEvent*	        m_pQuitEvent;
    HANDLE		m_hAddrChangeEvent;
    HANDLE		m_handle;
    OVERLAPPED		m_overLapped;
    HXBOOL                m_bIsDone;
    NOTIFYADDRCHANGE	_pNotifyAddrChange;
};

class AddrChangeCallback : public IHXCallback, 
			   public IHXInterruptSafe
{
public:
    HXNetInterface*	m_pParent;
    CallbackHandle	m_PendingHandle;
    HX_BITFIELD		m_bIsCallbackPending : 1;

			AddrChangeCallback();
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    STDMETHOD(Func)		(THIS);

    STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS);

protected:
			~AddrChangeCallback();

    LONG32		m_lRefCount;
};

#endif /* _HXNETIF_H_ */
