/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: shared_udp.h,v 1.4 2004/05/03 19:02:49 tmarshall Exp $ 
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

#ifndef _SHARED_UDP_H_
#define _SHARED_UDP_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "servlist.h"

#define SHARED_UDP_PORT_REGISTRY_NAME "config.UDPResendPortRange"
#define MIN_SHARED_UDP_PORT    0
#define MAX_SHARED_UDP_PORT    32000

class CHXMapLongToObj;
class ServerContext;

class SharedUDPPortReader : public IUnknown
{
public:
				SharedUDPPortReader(ServerContext* context);

    HX_RESULT			RegisterForeignAddress(UINT32 ulAddr, 
						       UINT16 sPort,
						       IHXUDPResponse* pUDPResponse,
						       UINT16 sPortEnum);

    HX_RESULT			UnregisterForeignAddress(UINT32 ulAddr, 
							 UINT16 sPort,
							 UINT16 sPortEnum);

    UINT16			GetPort(UINT16 sPortEnum);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // Listener objects are HXList objects that tie a port number to 
    // a callback obj.  (The addr is superfluous, but stored anyway.)
    // That is, each listener object is keeping track of one callback,
    // with the foreign address it's interested in.

    class Listener : public HXListElem
    {
    public:
	    	 			Listener() : m_ulAddr(0),
						     m_sPort(0),
						     m_pObj(0) {}
	virtual				~Listener();

	inline void			SetAddr(UINT32 ulAddr)
						{ m_ulAddr = ulAddr; }
	inline void			SetPort(UINT16 sPort)
						{ m_sPort = sPort; }
	void				SetObj(IHXUDPResponse *p);

	inline IHXUDPResponse*		GetObj()
						{ return m_pObj; }
	inline UINT16			GetPort()
						{ return m_sPort; }

	inline void			UnsetObj()
					    { 
						if (m_pObj) m_pObj->Release(); 
						m_pObj = 0; 
					    }

    private:
	UINT32				m_ulAddr;
	UINT16				m_sPort;
	IHXUDPResponse*		m_pObj;
    };

    // One receiver per shared port.

    class Receiver : public IHXUDPResponse
    {
    public:
					Receiver();
					~Receiver();

	HX_RESULT			Init(IHXUDPSocket* socket);

	HX_RESULT			RegisterForeignAddress(UINT32 ulAddr, 
							       UINT16 sPort,
							       IHXUDPResponse* pUDPResponse);

	HX_RESULT			UnregisterForeignAddress(UINT32 ulAddr, 
								 UINT16 sPort);

	inline UINT16			GetPort() { return m_sPort; }

	/*
	 * IHXUDPResponse methods
	 */
	STDMETHOD(ReadDone)		(THIS_
					 HX_RESULT status,
					 IHXBuffer* pBuffer,
					 UINT32 ulAddr,
					 UINT16 nPort);

	/*
	 *	IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
					 REFIID riid,
					 void** ppvObj);
	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

    private:
	LONG32				m_lRefCount;
	CHXMapLongToObj*		m_pAddrMap;
	IHXUDPSocket*			m_pUDPSocket;
	UINT16				m_sPort;

	UINT32				m_lastAddr;
	UINT16				m_lastPort;
	Listener*			m_lastListener;
    };


private:
    virtual			~SharedUDPPortReader();

    LONG32			m_lRefCount;
    Receiver*			m_ppReceiver[2];
    BOOL			m_bSharedAvailable;

};

#endif /* _SHARED_UDP_H_ */
