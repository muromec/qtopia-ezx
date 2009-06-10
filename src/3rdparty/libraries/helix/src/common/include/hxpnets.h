/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpnets.h,v 1.5 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _HXPNETS_H_
#define _HXPNETS_H_

/*
 * Forward declarations of some interfaces used here-in.
 */

typedef _INTERFACE	IHXTCPSocket			IHXTCPSocket;
typedef _INTERFACE	IHXListenSocket		IHXListenSocket;
typedef _INTERFACE	IHXHTTPProxy			IHXHTTPProxy;
typedef _INTERFACE	IHXCloakedNetworkServices	IHXCloakedNetworkServices;
typedef _INTERFACE	IHXValues      		IHXValues;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCloakedNetworkServices
 * 
 *  Purpose:
 * 
 *	This is a factory interface for the various types of HTTP cloaked 
 *      networking interfaces described above.
 * 
 *  IID_IHXCloakedNetworkServices:
 *
 *  	{00000600-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXCloakedNetworkServices, 0x00000600, 0xb4c8, 0x11d0, 0x99, 
			0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXCloakedNetworkServices

DECLARE_INTERFACE_(IHXCloakedNetworkServices, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXNetworkCloakedServices methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCloakedNetworkServices::CreateClientCloakedSocket
     *	Purpose:
     *	    Create a new HTTP cloaked TCP socket for the client.
     */
    STDMETHOD(CreateClientCloakedSocket)	(THIS_
			IHXTCPSocket**    /*OUT*/     ppTCPSocket) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCloakedNetworkServices::CreateServerCloakedSocket
     *	Purpose:
     *	    Create a new HTTP cloaked TCP socket that will listen for 
     *      connections on a particular port.
     */
    STDMETHOD(CreateServerCloakedSocket)	(THIS_
			IHXListenSocket**    /*OUT*/     ppListenSocket) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHTTPProxy
 * 
 *  Purpose:
 * 
 *	Provides the user with an asynchronous TCP networking interface.
 * 
 *  IID_IHXHTTPProxy:
 * 
 *  	{00000601-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXHTTPProxy, 0x00000601, 0xb4c8, 0x11d0, 0x99, 0x95, 
			0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXHTTPProxy

DECLARE_INTERFACE_(IHXHTTPProxy, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXHTTPProxy methods
     *
     *  Network addresses and ports are in native byte order
     *  
     */

    /************************************************************************
     *	Method:
     *	    IHXHTTPProxy::SetProxy
     *	Purpose:
     *	    This method is called when to set the HTTP proxy (if required)
     *	    while using HTTP cloaked socket behind the firewall. This 
     *	    interface is only supported by IHXTCPSocket returned by the 
     *	    CreateCloakedSocket.
     */
    STDMETHOD(SetProxy)		(THIS_
				const char* /*IN*/  pProxyHostName,
				UINT16	    /*IN*/  nPort) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCloakedTCPSocket
 * 
 *  Purpose:
 * 
 *	interfaces specific to cloaked TCP sockets
 * 
 *  IID_IHXCloakedTCPSocket:
 * 
 *  	{00000602-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXCloakedTCPSocket, 0x00000602, 0xb4c8, 0x11d0, 0x99, 0x95, 
			0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXCloakedTCPSocket

DECLARE_INTERFACE_(IHXCloakedTCPSocket, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXCloakedTCPSocket methods
     *
     */

    /************************************************************************
     *	Method:
     *	    IHXCloakedTCPSocket::InitCloak
     */
    STDMETHOD(InitCloak)		(THIS_
					IHXValues* /*IN*/  pValues,
					IUnknown* pUnknown) PURE;
};

#endif /* _HXPNETS_H_ */

