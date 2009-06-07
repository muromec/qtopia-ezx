/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bcngtran.h,v 1.10 2007/03/30 19:08:38 tknox Exp $
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

#ifndef _BCNGTRAN_H_
#define _BCNGTRAN_H_

#include "transport.h"

#define BCNG_SESSION_ID_ENTRY                   "BCNGTransport.SessionID"
#define BCNG_DESTRUCT_RESEND_DELAY              10000
#define DEFAULT_PING_TIMEOUT                    30000
#define DEFAULT_PING_CHECKS_FAIL_TILL_TIMEOUT   2

class BCNGTransport;
class BCNGTransportDestructTimeout;
class BCNGSessionTimeout;
struct BdstStatistics;
class SenderStats;
class ConfigWatcher;

class BCNGTransport : public Transport,
                     public IHXSocketResponse
{
public:
    BCNGTransport(BOOL bIsSource, IUnknown* pContext);
    virtual ~BCNGTransport(void);

    /* IUnknown */
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    /* IHXSocketResponse */
    STDMETHOD(EventPending)     (THIS_ UINT32 uEvent, HX_RESULT status);

    /* IHXPropWatchResponse */
    STDMETHODIMP AddedProp    (const UINT32            ulId,
                               const HXPropType       propType,
                               const UINT32            ulParentID);

    STDMETHODIMP ModifiedProp (const UINT32            ulId,
                               const HXPropType       propType,
                               const UINT32            ulParentID);

    STDMETHODIMP DeletedProp  (const UINT32            ulId,
                               const UINT32            ulParentID);


    /* Transport */
    void addStreamInfo(RTSPStreamInfo* pStreamInfo, UINT32 ulBufferDepth);
    void setSessionID(const char* pSessionID);
    STDMETHOD_(void, Done)              (THIS);

    HX_RESULT sendPacket(BasePacket* pPacket);
    HX_RESULT sendPackets(BasePacket** pPacket);
    HX_RESULT handlePacket(IHXBuffer* pBuffer);
    HX_RESULT streamDone(UINT16 streamNumber,
                                        UINT32 uReasonCode = 0,
                                        const char* pReasonText = NULL);

    HX_RESULT startPackets(UINT16 uStreamNumber);
    HX_RESULT stopPackets(UINT16 uStreamNumber);

    RTSPTransportTypeEnum tag(void);

    void Reset(void)              { /*NO-OP*/ }
    void Restart(void)            { /*NO-OP*/ }
    HX_RESULT pauseBuffers(void)  { /*NO-OP*/ return HXR_OK;}
    HX_RESULT resumeBuffers(void) { /*NO-OP*/ return HXR_OK;}

    /* BCNGTransport */

    HX_RESULT init (IHXRTSPTransportResponse* pResponse,
                    /* transport params */
                    IHXSockAddr* pAddr,
                    UINT16 nPorts,
                    ProtocolType::__PType Protocol,
                    BOOL bResendSupported,
                    UINT8 ucFECLevel,
                    UINT8 ucTTL,
                    UINT8 ucSecurityType,
                    buffer* pAuthenticationData,
                    UINT32 ulPullSplitID,
                    /* TCP specific params */
                    IHXSocket* pTCPSocket,
                    INT8 ucInterleave);

    void Destroy();
    void SwitchTransportProtocolToTCP();

    void HandleFEC(IHXBuffer* pOutputPacket, UINT32 ulPacketSeqNo);
    void QueueResendPkt(IHXBuffer* pBuffer, UINT32 ulPktSeqNo);
    void ResendReceived(buffer pData);
    void PingReceived() { m_ucNumChecksWithoutPing = 0; }
    void PingTimeoutFunc();
    void OutputPacket(IHXBuffer* pBuffer);
    void SendDisconnectPacket();

private:
    IHXSockAddr*            m_pAddr;
    UINT32                  m_ulPullSplitID;
    UINT32                  m_ulRedundancySendInterval;
    UINT32                  m_ulWouldBlockTimeout;
    UINT32                  m_ulReconnectTimeout;
    UINT32                  m_ulStartTime;
    UINT32                  m_ulSecuritySalt;
    UINT32                  m_ulSessionID;
    UINT32                  m_ulFECSizeXOR;
    UINT32                  m_ulFECCounter;
    UINT32                  m_ulFECLargestSizeSeen;
    UINT32                  m_ulProtectedSequenceStart;
    UINT32                  m_ulDestructTimeoutHandle;
    UINT32                  m_ulSequenceNo;
    UINT32                  m_ulPacketSequenceNo;
    UINT32                  m_ulSecurityKey;
    UINT32                  m_ulSessionTimeoutHandle;
    UINT32                  m_ulSessionTimeoutCheckInterval;

    /* ids for debug parameters stored in registry */
    UINT32                  m_ulBroadcastDistRoot_id;
    UINT32                  m_ulDebugLevel_id;
    UINT32                  m_ulDebugStatsInterval_id;
    UINT32                  m_ulDebugFileName_id;

    UINT16                  m_unNumStreams;
    UINT16                  m_unStreamDonesSeen;

    UINT8                   m_ucFECLevel;
    UINT8                   m_ucTTL;
    UINT8                   m_ucTCPInterleave;
    UINT8                   m_ucNumChecksWithoutPing;
    UINT8                   m_ucChecksWithoutPingTillTimeout;

    BOOL                    m_bPacketsStarted;
    BOOL                    m_bResendSupported;
    BOOL                    m_bDNSDone;
    BOOL                    m_bNeedsRemoval;
    BOOL                    m_bReadyForInit;
    BOOL                    m_bConfigurationChanged;

    IHXSocket*              m_pUDPOutput;
    IHXBufferedSocket*      m_pFastTCPOutput;
    IHXSocket*              m_pTCPOutput;
    IHXNetServices*         m_pNetSvc;
    IHXBuffer*              m_pFECBuffer;
    IHXBuffer*              m_pFECPacketBuffer;
    UINT32*                 m_pPacketStreamSequenceNo;

    ProtocolType::__PType   m_Protocol;
    BroadcastSecurity       m_Security;
    HXList                  m_ResendRawPacketQueue;
    AvgBandwidthCalc        m_AvgBandwidth;
    buffer                  m_AuthData;

    char*                         m_pPathPrefix;
    char*                         m_pRTSPSessionID;
    BCNGTransportDestructTimeout* m_pDestructTimeout;
    BCNGSessionTimeout*           m_pSessionTimeout;
    SenderStats*                  m_pStats;
    ConfigWatcher*                m_pConfigWatcher;

    BdstStatistics*               m_pSharedStats;

    friend class RTSPServerProtocol;
    friend class BCNGTransportDestructTimeout;
    friend class BCNGSessionTimeout;
    friend class CUTBCNGTransportTestDriver;
};

class BCNGTransportDestructTimeout : public IHXCallback
{
private:
    ULONG32                 m_RefCount;
    BCNGTransport*          m_pTransport;
public:
    BCNGTransportDestructTimeout(BCNGTransport* pTransport);
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    STDMETHOD(Func)             (THIS);
};

class BCNGSessionTimeout : public IHXCallback
{
private:
    ULONG32                 m_RefCount;
    BCNGTransport*          m_pTransport;
public:
    BCNGSessionTimeout(BCNGTransport* pTransport);
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    STDMETHOD(Func)             (THIS);
};

#endif /* _BCNGTRAN_H_ */







