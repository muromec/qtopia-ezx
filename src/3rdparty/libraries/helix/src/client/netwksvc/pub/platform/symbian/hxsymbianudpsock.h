/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianudpsock.h,v 1.5 2007/07/06 21:58:24 jfinnecy Exp $
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

#ifndef HXSYMBIANUDPSOCK_H
#define HXSYMBIANUDPSOCK_H

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxccf.h"
#include "hxslist.h"

#include <es_sock.h>

class HXSymbianUDPWriter;
class HXSymbianUDPReader;

class HXSymbianUDPSocket : public IHXUDPSocket,
			   public IHXSetSocketOption
{
public:
    HXSymbianUDPSocket(IHXCommonClassFactory* pCCF);
    ~HXSymbianUDPSocket();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXUDPSocket methods
     *
     *  Network addresses and ports are in native byte order
     */

    STDMETHOD(Init)		(THIS_
				ULONG32			ulAddr,
				UINT16			nPort,
				IHXUDPResponse*	pUDPResponse);

    STDMETHOD(Bind)		(THIS_
				UINT32			    ulLocalAddr,
				UINT16 			    nPort);

    STDMETHOD(Read)		(THIS_
				UINT16			Size);

    STDMETHOD(Write)		(THIS_
				IHXBuffer*		pBuffer);

    STDMETHOD(WriteTo)		(THIS_
    				ULONG32			ulAddr,
				UINT16			nPort,
				IHXBuffer*		pBuffer);

    STDMETHOD(GetLocalPort)	(THIS_
    				REF(UINT16)		port);

    STDMETHOD(JoinMulticastGroup)	(THIS_
    					ULONG32	    ulMulticastAddr,
    					ULONG32	    ulInterfaceAddr);
    
    STDMETHOD(LeaveMulticastGroup)	(THIS_
    					ULONG32	    ulMulticastAddr,
    					ULONG32	    ulInterfaceAddr);


    /*
     *	IHXSetSocketOption methods
     */

    STDMETHOD(SetOption)		(THIS_ 
					 HX_SOCKET_OPTION option,
					 UINT32 ulValue);
protected:
    friend class HXSymbianUDPWriter;
    friend class HXSymbianUDPReader;

    void OnWriteDone(HX_RESULT status);
    void OnReadDone(HX_RESULT status, 
		    ULONG32 ulAddr, UINT16 nPort, 
		    IHXBuffer* pBuffer);

private:    
    enum UDPState {udpNotOpen,
		   udpOpen,
		   udpInitialized,
		   udpBound};

    HX_RESULT DoRead(UINT16 size);

    void CloseSocket(HX_RESULT status);

    ULONG32 m_lRefCount;
    IHXUDPResponse* m_pResponse;
    UDPState m_state;

    UINT32 m_ulRemoteAddr;
    UINT16 m_nRemotePort;

    RSocketServ m_socketServ;
    RSocket m_socket;

    CHXSimpleList m_writeList;
    HXSymbianUDPWriter* m_pWriter;

    HXSymbianUDPReader* m_pReader;
};

#endif /* HXSYMBIANUDPSOCK_H */
