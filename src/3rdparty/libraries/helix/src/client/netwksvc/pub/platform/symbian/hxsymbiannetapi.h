/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbiannetapi.h,v 1.5 2005/04/01 21:21:19 ehyche Exp $
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

#ifndef HXSYMBIANNETAPI_H
#define HXSYMBIANNETAPI_H

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxpnets.h"

class HXNetworkServices : public IHXNetworkServices,
			  public IHXCloakedNetworkServices
{
public:
    HXNetworkServices(IUnknown* pContext);
    ~HXNetworkServices();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXNetworkServices methods
     */

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateTCPSocket
     *	Purpose:
     *	    Create a new TCP socket.
     */
    STDMETHOD(CreateTCPSocket)	(THIS_
				IHXTCPSocket**    /*OUT*/  ppTCPSocket);

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateUDPSocket
     *	Purpose:
     *	    Create a new UDP socket.
     */
    STDMETHOD(CreateUDPSocket)	(THIS_
				IHXUDPSocket**    /*OUT*/  ppUDPSocket);

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateListenSocket
     *	Purpose:
     *	    Create a new TCP socket that will listen for connections on a
     *	    particular port.
     */
    STDMETHOD(CreateListenSocket)   (THIS_
				    IHXListenSocket** /*OUT*/ ppListenSocket
				    );

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateResolver
     *	Purpose:
     *	    Create a new resolver that can lookup host names
     */
    STDMETHOD(CreateResolver)  	(THIS_
			    	IHXResolver**    /*OUT*/     ppResolver);

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
			IHXTCPSocket**    /*OUT*/     ppTCPSocket);

    /************************************************************************
     *	Method:
     *	    IHXCloakedNetworkServices::CreateServerCloakedSocket
     *	Purpose:
     *	    Create a new HTTP cloaked TCP socket that will listen for 
     *      connections on a particular port.
     */
    STDMETHOD(CreateServerCloakedSocket)	(THIS_
			IHXListenSocket**    /*OUT*/     ppListenSocket);

    void Close();

private:
    ULONG32 m_lRefCount;
    IUnknown* m_pContext;
    IUnknown* m_pAccessPointMan;
};

#endif /* HXSYMBIANNETAPI_H */
