/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_sig_bus.h,v 1.13 2003/08/18 17:46:39 darrick Exp $ 
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

#ifndef _QOS_SIG_BUS_H_
#define _QOS_SIG_BUS_H_

struct IHXQoSTransportAdaptationInfo;
struct IHXQoSSessionAdaptationInfo;
struct IHXQoSApplicationAdaptationInfo;

/* Filter Table Node */
class FilterTableNode 
{
 public:
    FilterTableNode(IHXBuffer* pSessionId);
    ~FilterTableNode();
    
    void Signal(IHXQoSSignal* pSignal);
    void Attach(IHXQoSSignalSink* pSink);
    void Dettach(IHXQoSSignalSink* pSink);

 private:
    HXList         m_Sinks;
    HX_MUTEX       m_NodeLock;
    IHXBuffer*     m_pSessionId;
};

/* Signal Filter: */
class SinkListElem : public HXListElem
{
 public:
    SinkListElem(IHXQoSSignalSink* pSink);
    ~SinkListElem();

    IHXQoSSignalSink* m_pSink;
};

class QoSSignalBus : public IHXQoSSignalBus,
		     public IHXQoSTransportAdaptationInfo, 
		     public IHXQoSSessionAdaptationInfo,
		     public IHXQoSApplicationAdaptationInfo,
		     public IHXQoSProfileConfigurator
{
 public:
    QoSSignalBus (Process* pProc);
    ~QoSSignalBus ();
    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXQoSSignalBus */
    STDMETHOD (Init) (THIS_ IHXBuffer* pSessionId);

    STDMETHOD (Close) (THIS);

    STDMETHOD (Send) (THIS_ IHXQoSSignal* pSignal);

    STDMETHOD (AttachListener)  (THIS_ HX_QOS_SIGNAL ulListenFilter, IHXQoSSignalSink* pListener);

    STDMETHOD (DettachListener) (THIS_ HX_QOS_SIGNAL ulListenFilter, IHXQoSSignalSink* pListener);


    /* IHXIHXQOSTransportAdaptationInfo methods */

    STDMETHODIMP_(UINT32) GetRRFrequency();
    STDMETHODIMP SetRRFrequency(UINT32 ulRRFrequency);

    STDMETHODIMP_(UINT32) GetRTT();
    STDMETHODIMP SetRTT(UINT32 ulRTT);

    STDMETHODIMP_(UINT32) GetPacketLoss();
    STDMETHODIMP SetPacketLoss(UINT32 ulPacketLoss);

    STDMETHODIMP_(UINT32) GetReceivedThroughput();
    STDMETHODIMP SetReceivedThroughput(UINT32 ulReceivedThroughput);

    STDMETHODIMP_(UINT32) GetSuccessfulResends();
    STDMETHODIMP SetSuccessfulResends(UINT32 ulSuccessfulResends);

    STDMETHODIMP_(UINT32) GetFailedResends();
    STDMETHODIMP SetFailedResends(UINT32 ulFailedResends);

    STDMETHODIMP_(IHXBuffer*) GetTxRateRange();
    STDMETHODIMP SetTxRateRange(IHXBuffer* pTxRateRange);

    STDMETHODIMP_(UINT32) GetPacketsSent();
    STDMETHODIMP SetPacketsSent(UINT32 ulPacketsSent);

    STDMETHODIMP_(UINT64) GetBytesSent();
    STDMETHODIMP SetBytesSent(UINT64 ulBytesSent);



    /* IHXQOSSessionAdaptationInfo methods */

    STDMETHODIMP_(UINT32) GetEstimatedPlayerBufferUnderruns();
    STDMETHODIMP SetEstimatedPlayerBufferUnderruns(UINT32 ulEstimatedPlayerBufferUnderruns);

    STDMETHODIMP_(UINT32) GetEstimatedPlayerBufferOverruns();
    STDMETHODIMP SetEstimatedPlayerBufferOverruns(UINT32 ulEstimatedPlayerBufferOverruns);

    STDMETHODIMP_(UINT32) GetBufferDepthTime();
    STDMETHODIMP SetBufferDepthTime(UINT32 ulBufferDepthTime);

    STDMETHODIMP_(UINT32) GetBufferDepthBytes();
    STDMETHODIMP SetBufferDepthBytes(UINT32 ulBufferDepthBytes);


    /* IHXQOSApplicationAdaptationInfo methods */

    STDMETHODIMP_(UINT32) GetTotalBitrateAdaptations();
    STDMETHODIMP SetTotalBitrateAdaptations(UINT32 ulTotalBitrateAdaptations);

    STDMETHODIMP_(UINT32) GetCurrentBitrate();
    STDMETHODIMP SetCurrentBitrate(UINT32 ulCurrentBitrate);
    
    STDMETHODIMP_(UINT32) GetTotalUpshifts();
    STDMETHODIMP SetTotalUpshifts(UINT32 ulTotalUpshifts);

    STDMETHODIMP_(UINT32) GetTotalDownshifts();
    STDMETHODIMP SetTotalDownshifts(UINT32 ulTotalDownshifts);

    STDMETHODIMP_(UINT32) GetASMSubscribes();
    STDMETHODIMP SetASMSubscribes(UINT32 ulASMSubscribes);

    STDMETHODIMP_(UINT32) GetASMUnsubscribes();
    STDMETHODIMP SetASMUnsubscribes(UINT32 ulASMUnsubscribes);


    /* IHXQoSProfileConfiguroatr methods */
    STDMETHOD (SetConfigId)     (THIS_ INT32 lConfigId);
    STDMETHOD (GetConfigId)     (THIS_ REF(INT32) /*OUT*/ lConfigId);
    STDMETHOD (GetConfigInt)    (THIS_ const char* pItemName, REF(INT32) /*OUT*/ lValue); 
    STDMETHOD (GetConfigBuffer) (THIS_ const char* pItemName, REF(IHXBuffer*) /*OUT*/ pValue);

 private:
    LONG32                        m_lRefCount;
    Process*                      m_pProc;

    IHXBuffer*                    m_pSessionId;
    FilterTableNode****           m_pFilterTable;
    INT32                         m_lConfigId;
    char*                         m_pConfigName;

    IHXBuffer*                    m_pTxRateRange;
    UINT32                        m_ulRRFrequency;
    UINT32                        m_ulRTT;
    UINT32                        m_ulPacketLoss;
    UINT32                        m_ulReceivedThroughput;
    UINT64                        m_ulBytesSent;
    UINT32                        m_ulPacketsSent;

    UINT32                        m_ulEstimatedPlayerBufferUnderruns;
    UINT32                        m_ulEstimatedPlayerBufferOverruns;
    UINT32                        m_ulBufferDepthTime;
    UINT32                        m_ulBufferDepthBytes;

    UINT32                        m_ulTotalBitrateAdaptations;
    UINT32                        m_ulCurrentBitrate;
    UINT32                        m_ulTotalUpshifts;
    UINT32                        m_ulTotalDownshifts;
    UINT32                        m_ulASMSubscribes;
    UINT32                        m_ulASMUnsubscribes;
    UINT32                        m_ulSuccessfulResends;
    UINT32                        m_ulFailedResends;

};

#endif /*_QOS_SIG_BUS_H_ */
