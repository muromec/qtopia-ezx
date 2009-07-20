/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: static_pushpktflow.h,v 1.29 2007/01/30 00:48:42 jzeng Exp $
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
#ifndef _STATIC_PUSHPKTFLOW_H_
#define _STATIC_PUSHPKTFLOW_H_

#include "basicpcktflow.h"

#define RESEND_QUEUE_SIZE 32 /* MAX 255 */

_INTERFACE IHXRateDescription;

class StaticPushPacketFlow : public BasicPacketFlow,
                             public IHXServerPacketSink,
                             public IHXQoSSignalSink,
                             public IHXStreamAdaptationSetup,
                             public IHXQoSLinkCharSetup
{
public:
    StaticPushPacketFlow(Process* p,
                         IHXSessionStats* pSessionStats,
                         UINT16 unStreamCount,
                         PacketFlowManager* pFlowMgr,
                         IHXServerPacketSource* pSource,
                         BOOL bIsMulticast,
                         BOOL bIsLive);

    ~StaticPushPacketFlow();

    /* IUnknown */
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)();
    STDMETHOD_(ULONG32,Release)();

    /* IHXServerPacketSink */
    STDMETHOD(SetSource)   (THIS_ IHXServerPacketSource* pSource);
    STDMETHOD(PacketReady) (THIS_ ServerPacket* pPacket);
    STDMETHOD(Flush)(THIS);
    STDMETHOD(SourceDone)(THIS);

    /* IHXServerPacketSource */
    STDMETHOD(StartPackets) (THIS);
    STDMETHOD(SinkBlockCleared)(UINT32 ulStream);
    STDMETHOD(EnableTCPMode) (THIS);

    /* IHXPacketResend */
    STDMETHOD(OnPacket)(UINT16 uStreamNumber, BasePacket** ppPacket);

    /* IHXPacketFlowControl Methods */
    STDMETHOD(WantWouldBlock)();

    /* IHXQoSSignalSink */
    STDMETHOD (Signal)(THIS_ IHXQoSSignal* pSignal, IHXBuffer* pSessionId);
    STDMETHOD (ChannelClosed)(THIS_ IHXBuffer* pSessionId);

    /* IHXStreamAdaptationSetup */
    STDMETHOD (GetStreamAdaptationScheme)   (THIS_
                                 REF(StreamAdaptationSchemeEnum) /* OUT */ enumAdaptScheme );
    STDMETHOD (SetStreamAdaptationScheme)   (THIS_
                                 StreamAdaptationSchemeEnum enumAdaptScheme );
    STDMETHOD (SetStreamAdaptationParams) (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams);
    STDMETHOD (GetStreamAdaptationParams) (THIS_
				 UINT16 unStreamNum,
                                 REF(StreamAdaptationParams) /* OUT */ streamAdaptParams);
    STDMETHOD (UpdateStreamAdaptationParams) (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams);
    STDMETHOD (UpdateTargetProtectionTime) (THIS_
                                 StreamAdaptationParams* /* IN */ pStreamAdaptParams);

    /* IHXStreamAdaptationSetup */
    STDMETHOD (SetLinkCharParams) (THIS_
                                 LinkCharParams* /* IN */ pLinkCharParams);
    STDMETHOD (GetLinkCharParams) (THIS_
				 UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams);
protected:
    virtual void HandleResume();

    virtual void InitialStartup();
    virtual void Pause(BOOL bWouldBlock, UINT32 ulPausePoint = 0);

    virtual void SetPlayerInfo(Player* pPlayerControl, 
			       const char* szPlayerSessionId,
                               IHXSessionStats* pSessionStats);

    STDMETHOD(GetNextPacket)(UINT16 unStreamNumber, BOOL bAlwaysGet);

    void Done();

    void JumpStart(UINT16 uStreamNumber);

    BOOL SendPacket(UINT32 ulActualDeliveryRate,
                    REF(UINT32) ulPacketSize);

    BOOL Compare(UINT32 ulMsecSinceLastRecalc,
                 REF(BasicPacketFlow*) pBestFlow);

    STDMETHOD(StartSeek)(UINT32 ulTime);

    STDMETHOD(SeekDone)();

    STDMETHOD(RegisterStream)(Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader);

    STDMETHOD(RegisterStream)(Transport* pTransport,
                              UINT16 uStreamGroupNumber,
			      UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader);

    HX_RESULT SessionPacketReady(HX_RESULT ulStatus, IHXPacket* pPacket);
    
    inline HX_RESULT SendToTransport(ServerPacket* pPacket, PacketStream* pStream, UINT16 unRule);

    STDMETHOD(Activate)();

    STDMETHOD(HandleSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);

    STDMETHOD(HandleUnSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);

    void      TransmitPacket(ServerPacket* pPacket) {return;}

    virtual void RescheduleTSD(UINT16 unStream, float fOldRatio) {}

    void     SendMediaRateSignal(UINT16 unStreamNumber, 
                                 UINT32 ulRate, 
                                 UINT32 ulCumulativeRate,
                                 BOOL bInitialRateReset = FALSE);

    void     UpdateMediaRate(BOOL bInitialRateReset = FALSE);

private:
    IHXServerPacketSource*     m_pSource;
    IHXASMSource*              m_pASMSource;
    IHXServerPacketSink*       m_pBufferVerifier;
    IHXQoSProfileConfigurator* m_pQoSConfig;
    IHXQoSRateShapeAggregator* m_pAggregator;
    IHXQoSClientBufferVerifier* m_pResendVerifier;
    IHXQoSSignalBus*            m_pSignalBus;
    BOOL*                       m_pbPausedStreams;

    /* Resend Support: */
    ServerPacket***            m_ppResendQueue;
    UINT8*                     m_pResendWrite;
    UINT8*                     m_pResendRead;

    ServerPacket**             m_pBlockQueue;

    BOOL                       m_bIsLive;
    UINT32                     m_ulTotalRate;
    UINT32                     m_ulCurrentCumulativeMediaRate;

    StreamAdaptationSchemeEnum      m_enumStreamAdaptScheme;
    LinkCharParams*      m_pSessionAggrLinkCharParams;
    BOOL                        m_bStreamLinkCharSet;

    UINT16		       m_uRegisterStreamGroupNumber;

    //* represents the stream adaptation parameters 
    //*   for aggregate rate adaptation
    //* Stream specific adaptation params are stored in
    //*   m_pStreams
    StreamAdaptationParams     *m_pAggRateAdaptParams;

    inline HX_RESULT SendResendPackets (UINT16 unStreamNumber);
    UINT32 AdjustInitialSendingRate(IHXRateDescription*  pRateDesc, IHXUberStreamManager* pUberStreamManager);
};

#endif // _STATIC_PUSHPCKTFLOW_H_
