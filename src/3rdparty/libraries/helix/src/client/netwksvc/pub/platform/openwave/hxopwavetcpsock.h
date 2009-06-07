/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxopwavetcpsock.h,v 1.5 2005/03/14 20:32:08 bobclark Exp $
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


#ifndef HXOPWAVETCPSOCK_H
#define HXOPWAVETCPSOCK_H

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxccf.h"
#include "hxslist.h"

#include "hx_opsocket.h"

class HXOpwaveTCPConnector;
class HXOpwaveTCPWriter;
class HXOpwaveTCPReader;

class HXOpwaveTCPSocket : public IHXTCPSocket,
                          public IHXSetSocketOption,
			  public OpSocket
{

public:

    HXOpwaveTCPSocket(IHXCommonClassFactory* pCCF,
		  			  IHXResolver* pResolver);
    ~HXOpwaveTCPSocket();


    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
									 REFIID riid,
									 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXTCPSocket methods
     *
     *  Network addresses and ports are in native byte order
     *  
     */
    STDMETHOD(Init)		(THIS_
		  				IHXTCPResponse*    /*IN*/  pTCPResponse);

    STDMETHOD(SetResponse)	(THIS_
		    				IHXTCPResponse*	    pTCPResponse);

    STDMETHOD(Bind)		(THIS_
			  			 UINT32			    ulLocalAddr,
		 				 UINT16 			    nPort);

    /*
     * pDestination is a string containing host name or dotted-ip notation
     */
    STDMETHOD(Connect)		(THIS_
							 const char*		    pDestination,
			 				 UINT16 			    nPort);

    STDMETHOD(Read)		(THIS_
		 				 UINT16			    Size);

    STDMETHOD(Write)		(THIS_
			 				 IHXBuffer*		    pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXTCPSocket::WantWrite
     *	Purpose:
     *	    This method is called when you wish to write a large amount of
     *	    data.  If you are only writing small amounts of data, you can
     *	    just call Write (all data not ready to be transmitted will be
     *	    buffered on your behalf).  When the TCP channel is ready to be
     *	    written to, the response interfaces WriteReady method will be 
     *	    called.
     */
    STDMETHOD(WantWrite)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXTCPSocket::GetForeignAddress
     *	Purpose:
     *	    Returns the address of the other end of the TCP socket as a
     *	    ULONG32 in local host order
     */
    STDMETHOD(GetForeignAddress)	(THIS_
				    				 REF(ULONG32) lAddress);

    STDMETHOD(GetLocalAddress)		(THIS_
			    					 REF(ULONG32) lAddress);

    /************************************************************************
     *	Method:
     *	    IHXTCPSocket::GetForeignPort
     *	Purpose:
     *	    Returns the port of the other end of the TCP socket in local
     *      host order.
     */
    STDMETHOD(GetForeignPort)		(THIS_
			     					 REF(UINT16) port);


    STDMETHOD(GetLocalPort)		(THIS_
		      					 REF(UINT16) port);

    /*
     *	IHXSetSocketOption methods
     */
    STDMETHOD(SetOption)		(THIS_ 
								 HX_SOCKET_OPTION option,
								 UINT32 ulValue);

   /*
    *  Override partial OpSocket methods for handling event sink methods
    */
    virtual void onReadable(OpSocketEvent *pSocketEvent);
    virtual void onWritable(OpSocketEvent *pSocketEvent);
    virtual void onException(OpSocketEvent *pSocketEvent);
    //virtual bool onEvent(OpEvent& ev);

protected:

    friend class HXOpwaveTCPResolvResp;

    HX_RESULT GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr);

    void OnConnect(HX_RESULT status);

    void OnWriteDone(HX_RESULT status);

    void OnReadDone(HX_RESULT status, IHXBuffer* pBuffer);

    HX_RESULT DoWrite();
    HX_RESULT DoRead();

private:    

    enum TCPState {tcpNotInitialized,
				   tcpInitialized,
				   tcpBound,
				   tcpResolving,
				   tcpConnecting,
				   tcpConnected};

    void CloseConnection(HX_RESULT status);

    ULONG32 m_lRefCount;
    IHXTCPResponse* m_pResponse;
    IHXCommonClassFactory* m_pCCF;
	IHXResolver* m_pResolver;
    
	TCPState m_state;
    UINT16 m_nConnectPort;
	UINT16 m_nLocalPort;
	UINT32 m_ulLocalAddr;
    CHXSimpleList m_writeList;
    HXBOOL m_bWantWrite;
    HXBOOL m_bWritable;
    HXBOOL m_bReadable;

    ipv4_t m_ipDest;
    IHXBuffer* m_pReadBuffer;
    UINT32     m_ulReadSize;
    IHXBuffer* m_pWriteBuffer;
    UINT32 m_ulBytesLeftToWrite;

};



#endif /* HXOPWAVETCPSOCK_H */

