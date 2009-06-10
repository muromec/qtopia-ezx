/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: engine_desc_reg.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _ENGINE_DESC_REG_H_
#define _ENGINE_DESC_REG_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxspriv.h"
#include "server_resource.h"

class ServerEngineDescReg : public IHXDescriptorRegistration
{
public:
    ServerEngineDescReg();
    ~ServerEngineDescReg();

    void RegisterDesc();
    void UnRegisterDesc();
    void RegisterSock();
    void UnRegisterSock();

    UINT32 GetDescCapacity();
    UINT32 GetSockCapacity();

    STDMETHOD(QueryInterface)   	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef) 	(THIS);
    STDMETHOD_(ULONG32, Release)	(THIS);
    STDMETHOD(RegisterDescriptors)      (THIS_
					UINT32 ulCount);

    STDMETHOD(UnRegisterDescriptors)    (THIS_
					UINT32 ulCount);

    STDMETHOD(RegisterSockets)		(THIS_
					UINT32 ulCount);

    STDMETHOD(UnRegisterSockets)	(THIS_
					UINT32 ulCount);
    
    UINT32		NumFDs();

protected:
    UINT32              m_ulDescCount;
    UINT32              m_ulSockCount;
    virtual void	OnSocketsChanged(){}
    virtual void	OnDescriptorsChanged(){}

private:
    ULONG32             m_ulRefCount;
};

inline
ServerEngineDescReg::ServerEngineDescReg()
{
    m_ulRefCount = 1;
    m_ulDescCount = DESCRIPTOR_START_VALUE;
    m_ulSockCount = SOCK_START_VALUE;
}

inline
ServerEngineDescReg::~ServerEngineDescReg()
{
}

#endif /* _ENGINE_DESC_REG_H_ */
