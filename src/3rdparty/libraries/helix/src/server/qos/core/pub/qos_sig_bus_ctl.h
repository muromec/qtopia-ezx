/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_sig_bus_ctl.h,v 1.8 2003/09/05 18:31:34 jc Exp $ 
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

#ifndef _QOS_SIG_BUS_CTL_H_
#define _QOS_SIG_BUS_CTL_H_

#define NOTIFY_MAP_SIZE 1024

typedef _INTERFACE	IHXQoSSignalBus		IHXQoSSignalBus;
typedef _INTERFACE	IHXQoSSignalSource	IHXQoSSignalSource;
class Process;

class NotifyMapEntry
{
 public:
    NotifyMapEntry (IHXBuffer* pId);
    ~NotifyMapEntry ();

    void AddResponse     (IHXQoSSignalSourceResponse* pResp);
    void RemoveResponse  (IHXQoSSignalSourceResponse* pResp);
    void Notify          (IHXQoSSignalBus* pBus);

    IHXBuffer*       m_pId;
    HXList           m_NotifyList;
};

class NotifyListElem : public HXListElem
{
 public:
    NotifyListElem(IHXQoSSignalSourceResponse* pResp);
    ~NotifyListElem();

    IHXQoSSignalSourceResponse* m_pResp;
};

class QoSSignalBusController : public IUnknown
{
 public:
    QoSSignalBusController ();
    ~QoSSignalBusController ();
    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    /* QoSSignalBusController */
    HX_RESULT CreateSignalBus  (Process* pProc, IHXBuffer* pSessionId);

    HX_RESULT DestroySignalBus (Process* pProc, IHXBuffer* pSessionId);

    HX_RESULT GetSignalBus     (Process* pProc, IHXBuffer* pSessionId, 
				IHXQoSSignalSourceResponse* pResp);

    HX_RESULT ReleaseResponseObject (Process* pProc, IHXBuffer* pSessionId, 
				IHXQoSSignalSourceResponse* pResp);

    HX_RESULT AddStreamer      (Process* pProc);

 private:
    LONG32                        m_lRefCount;
    
    Process**                     m_pStreamerProcs;

    CHXMapStringToOb              m_ResponseMap;
    HX_MUTEX                      m_MapLock;
};

/* Public proxy for QoS Signal Bus Controller */ 
class QoSSignalSource : public IHXQoSSignalSource
{
 public:
    QoSSignalSource(Process* pProc, QoSSignalBusController* pCtl);
    ~QoSSignalSource();

    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXQoSSignalSource */
    STDMETHOD (GetSignalBus) (THIS_ IHXBuffer* pSessionId, 
			      IHXQoSSignalSourceResponse* pResp);

    STDMETHOD (ReleaseResponseObject) (THIS_ IHXBuffer* pSessionId, 
			      IHXQoSSignalSourceResponse* pResp);

 private:
    LONG32                        m_lRefCount;
    QoSSignalBusController*       m_pCtl;
    Process*                      m_pProc;
};

#endif /* _QOS_SIG_BUS_CTL_H_*/
