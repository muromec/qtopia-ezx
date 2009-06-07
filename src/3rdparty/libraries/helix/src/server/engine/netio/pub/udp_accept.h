/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: udp_accept.h,v 1.2 2003/01/23 23:42:56 damonlan Exp $ 
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

#ifndef _UDP_ACCEPT_H_
#define _UDP_ACCEPT_H_

#include "base_callback.h"

class Process;
class Engine;
class UDPIO;

class UDPAcceptor
{
public:
			    UDPAcceptor(Process* _proc, Engine* _engine);
			    ~UDPAcceptor();
			    
    int			    Init(UINT16 first_port = 0,
				 UINT16 last_port = 0);
    int			    Init(UINT16 port);
    void		    Enable();
    void		    Disable();
    int			    Error();
    virtual void	    MessageReceived() = 0;

    UINT16		    m_port;

protected:
    Process*		    m_proc;
    Engine*		    m_engine;
    UDPIO*		    m_conn;

private:
    class MessageReceivedCallback: public BaseCallback
    {
    public:
    	STDMETHOD(Func)     (THIS);
	UDPAcceptor*        m_udp_acceptor;
    };
    MessageReceivedCallback* m_message_received_cb;
};

#endif /* _UDP_ACCEPT_H_ */

