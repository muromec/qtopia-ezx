/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cloaksock.h,v 1.3 2003/09/04 22:39:09 dcollins Exp $ 
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

#ifndef _CLOAK_SOCK_H_
#define _CLOAK_SOCK_H_

#include "hxcom.h"
#include "hxengin.h"
#include "sockio.h"
#include "base_callback.h"
#include "server_inetwork.h"

class CByteQueue;
class Client;
class IHXTCPSocketContext;
class Process;
class SIO;
class TCPIO;

struct IHXTCPResponse;

class CloakedStreamTCPSocketContext : public IHXTCPSocketContext
{
public:
    CloakedStreamTCPSocketContext(Process* proc, IHXResolver* pResolver);
    CloakedStreamTCPSocketContext(Process* proc,
	IHXResolver* pResolver, TCPIO* sock, 
	UINT32 lForeignAddr, UINT16 sForeignPort);
    ~CloakedStreamTCPSocketContext();

    /*
     * IHXBufferedSocket methods
     */
    STDMETHOD(BufferedWrite)		(THIS_
					IHXBuffer*);

    STDMETHOD(FlushWrite)		(THIS);

    STDMETHOD(SetDesiredPacketSize)	(THIS_
					UINT32);


    STDMETHOD(Read)		(THIS_
				UINT16		    	Size);
    STDMETHOD(Write)		(THIS_
				IHXBuffer*	    	pBuffer);
    STDMETHOD(WantWrite)        (THIS);

    STDMETHOD(AddSender)	(THIS_
				IHXTCPSocketContext*	pToClient,
				IHXTCPResponse*	pToClientResponse,
				Client* 		pClient);
    STDMETHOD(AddReceiver)	(THIS_
				IHXTCPSocketContext*	pFromClient,
				IHXTCPResponse*	pFromClientResponse,
				Byte*			pData,
				UINT32			dataLen,
				Client* 		pClient);

    void 			disableRead();
    void 			enableRead();
    int 			readUndo(BYTE* pMsg, UINT32 nBytes);
    void 			disconnect();
    void 			reconnect(Engine* pEngine);
    int 			write_flush();

    TCPIO*			getReadTCPIO();
    TCPIO*			getWriteTCPIO();
    SIO*			getReadSIO();
    SIO*			getWriteSIO();

    IHXTCPSocketContext*	m_pReceiver;
    IHXTCPSocketContext*	m_pToClient;
    IHXTCPResponse*	        m_pToClientResponse;
    Client*	        	m_pClient;

    HX_RESULT			DoRead();
    void			DoWrite();

private:
    void                   	RotateReceiver();
    IHXTCPSocketContext*    CurrentReceiver();
    TCPSocketReadCallback* 	CurrentReadCallback();
    TCPSocketReadCallback*	m_pToClientReadCb;

    class StoredReceiver
    {
        friend class              CloakedStreamTCPSocketContext;

                            StoredReceiver(TCPSocketReadCallback* m_pReadCb,
                                           IHXTCPSocketContext* rcvr,
                                           IHXTCPResponse* rcvrResponse,
                                           Byte* already_read_data,
                                           UINT32 already_read_data_len);
                            ~StoredReceiver();
        UINT32              RemainingDataLen();

        TCPSocketReadCallback* m_pReadCb;
        IHXTCPSocketContext*  m_pReceiver;
        IHXTCPResponse*    m_pReceiverResp;
        Byte*               m_pAlreadyReadData;
        UINT32              m_lAlreadyReadDataLen;
        Byte*               m_pRemainingData;
    }; 

    Process*			m_proc;
    int                         m_bWriteCbPending;
    CHXSimpleList*              m_pReceiverQueue;
    Byte*                       m_pEncodedBuf;
    int                         m_BufLen;
    int                         m_LeftoverLen;
    Byte*                       m_pDecodedBuf;
    int                         m_DecodedLeftoverLen;
    int				m_nNumPosts;
    IHXTCPSocketContext*	GetNextTCPSocketContext()
				{
				    return m_pToClient;
				}
};

#endif /*_CLOAK_SOCK_H_*/
