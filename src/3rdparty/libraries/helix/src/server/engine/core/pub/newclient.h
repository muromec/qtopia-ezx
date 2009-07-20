/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: newclient.h,v 1.5 2004/07/31 15:17:41 dcollins Exp $ 
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

#ifndef _NEWCLIENT_H_
#define _NEWCLIENT_H_

class CByteQueue;
class Client;
class NewClientState;
class HXProtocol;
class SIO;

class NewClientProtocol: public HXProtocol
{
public:
			NewClientProtocol(Client* pClient);
		     	~NewClientProtocol();

    STDMETHOD_(UINT32,AddRef)	(THIS);
    STDMETHOD_(UINT32,Release)	(THIS);

    void 		Done(HX_RESULT status);
    void		init();
    INT32 		handleInput(IHXBuffer* pBuf);
    void 		setState(NewClientState* pState);
    void 		setNRequired(int nRequired);
    int 		nRequired();
    virtual const char*	versionStr();
    void		setClientAlreadyConnected(BOOL isConnectedFlag);
    BOOL		isClientAlreadyConnected();
    virtual void	setSwitchProtocol(HXProtocol* pProt);
    void		setAlreadyReadData(Byte* pAlreadyReadData, 
				           UINT32 alreadyReadDataLen);
protected:
    LONG32		m_lRefCount;
    HXProtocol*		m_pSwitchProtocol;
    NewClientState*  	m_pState;
    int              	m_nRequired;
    BOOL		m_bClientAlreadyConnected;
    CByteQueue*		m_pInQueue;
};

inline void
NewClientProtocol::Done(HX_RESULT status)
{
    if (m_pClient)
	HX_RELEASE(m_pClient);
}

inline void
NewClientProtocol::setState(NewClientState* pState)
{
    m_pState = pState;
}

inline void
NewClientProtocol::setNRequired(int nRequired)
{
    m_nRequired = nRequired;
}

inline int
NewClientProtocol::nRequired()
{
    return m_nRequired;
}

inline void
NewClientProtocol::setSwitchProtocol(HXProtocol* pProt)
{
    m_pSwitchProtocol = pProt;
}

inline const char*
NewClientProtocol::versionStr()
{
    return "newclient";
}

inline void
NewClientProtocol::setClientAlreadyConnected(BOOL connectedFlag)
{
    m_bClientAlreadyConnected = connectedFlag;
}

inline BOOL
NewClientProtocol::isClientAlreadyConnected()
{
    return m_bClientAlreadyConnected;
}

class NewClientState
{
public:
    virtual int handleInput(Byte* pMsg, int nMsgLen,
	NewClientProtocol* pProt) = 0;
    void DispatchClient(Client*);
};

class NewClientInitState: public NewClientState
{
public:
    static NewClientInitState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientInitState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMsgLen, 
	NewClientProtocol* pProt);

protected:
    NewClientInitState() {};

private:
    static NewClientInitState* m_pState;
};

class NewClientAlivePingState: public NewClientState
{
public:
    static NewClientAlivePingState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientAlivePingState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMstLen, 
	NewClientProtocol* pProt);

protected:
    NewClientAlivePingState() {};

private:
    static NewClientAlivePingState* m_pState;
};

class NewClientRTSPState: public NewClientState
{
public:
    static NewClientRTSPState* instance()
    {
        if(!m_pState)
            m_pState = new NewClientRTSPState;
        return m_pState;
    }

    int handleInput(Byte* pMsg, int nMstLen,
        NewClientProtocol* pProt);

protected:
    NewClientRTSPState() {};

private:
    static NewClientRTSPState* m_pState;
};

class NewClientIsCloakingState : public NewClientState
{
public:
    static NewClientIsCloakingState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientIsCloakingState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMsgLen,
	NewClientProtocol* pProt);

protected:
    NewClientIsCloakingState() {};

private:
    static NewClientIsCloakingState* m_pState;
};

class NewClientIsQTCloakingState : public NewClientState
{
public:
    static NewClientIsQTCloakingState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientIsQTCloakingState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMsgLen,
	NewClientProtocol* pProt);

protected:
    NewClientIsQTCloakingState() {};

private:
    static NewClientIsQTCloakingState* m_pState;
};

class NewClientHTTPState: public NewClientState
{
public:
    static NewClientHTTPState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientHTTPState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMsgLen,
	NewClientProtocol* pProt);

protected:
    NewClientHTTPState() {};

private:
    static NewClientHTTPState* m_pState;
};

class NewClientRBPState: public NewClientState
{
public:
    static NewClientRBPState* instance()
    {
	if(!m_pState)
	    m_pState = new NewClientRBPState;
	return m_pState;
    }

    int handleInput(Byte* pMsg, int nMsgLen, 
	NewClientProtocol* pProt);

protected:
    NewClientRBPState() {};

private:
    static NewClientRBPState* m_pState;
};

#endif/*_NEWCLIENT_H_*/
