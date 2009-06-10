/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtptran.h,v 1.57 2008/06/11 22:23:06 amsaleem Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _RTPTRAN_H_
#define _RTPTRAN_H_

// #define RTP_MESSAGE_DEBUG

#include "hxengin.h"
#include "hxmon.h"
#include "transbuf.h"
#include "statinfo.h"
#include "hxsmbw.h"
#include "ntptime.h"
#include "tconverter.h"
#include "chxkeepalive.h"
#include "ihx3gpp.h"

// GCC won't let me forward declare CHXMapLongToObj::Iterator,
// so I have to include this. -JR
#include "hxmap.h"
#include "hxslist.h"
#include "hxqossig.h"
#include "hxqos.h"

#include "packfilt.h"

struct IHXQoSTransportAdaptationInfo;
struct IHXScheduler;
struct IHXUDPSocket;
struct IHXPacket;
struct IHXBuffer;
struct IHXCommonClassFactory;
struct IHXInternalReset;
struct IHXPlayerState;
struct IHXAccurateClock;
class  RTSPResendBuffer;
class  CHXBitset;
class  HX_deque;
class  Timeval;
class  CHXTimestampConverter;

class RTCPBaseTransport;
class RTCPUDPTransport;
class RTCPTCPTransport;
class RTCPPacket;
class ReportHandler;
class CReflectionHandler;

_INTERFACE IHXQoSSignal;
_INTERFACE IHXQoSSignalBus;
_INTERFACE IHXQoSSignalSourceResponse;

HX_RESULT
FixRTPHeader(IHXCommonClassFactory* pCCF,
             IHXBuffer* pOrigBuf,
             REF(IHXBuffer*) pNewBuf,
             UINT16 unSeqNoOffset,
             UINT32 ulRTPTSOffset);

HX_RESULT
FixRTCPSR(IHXCommonClassFactory* pCCF,
          IHXBuffer* pOrigBuf,
          REF(IHXBuffer*) pNewBuf,
          UINT32 ulRTPTSOffset);

/* The largest number of SR records we will keep */
#define LSR_HIST_SZ 64

#define RTP_FILTER_CONSTANT 3

struct LSRRecord
{
    UINT32          m_ulSourceLSR;
    UINT32          m_ulServerLSR;
};

/******************************************************************************
*   RTP RTP RTP RTP RTP
******************************************************************************/
class RTPBaseTransport: public RTSPTransport, public IHXSourceBandwidthInfo
{
public:
    RTPBaseTransport                    (HXBOOL bIsSource);
    ~RTPBaseTransport                   (void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // RTSPTransport
    virtual void Done                   (void);
    virtual void Reset                  (void) {}
    virtual void Restart                (void) {}
    virtual RTSPTransportTypeEnum tag   (void)
            {
                HX_ASSERT(!"don't call this");
                return RTSP_TR_NONE;
            };
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);


    // IHXSourceBandwidthInfo methods
    STDMETHOD(InitBw)                   (THIS_
                                         IHXBandwidthManagerInput* pBwMgr);

    STDMETHOD(SetTransmitRate)          (THIS_
                                         UINT32 ulBitRate);
    HX_RESULT sendPacket                (BasePacket* pPacket)
            {
                HX_ASSERT(!"don't call this");
                return HXR_UNEXPECTED;
            }

    HX_RESULT init                              (void);

    HX_RESULT reflectPacket             (BasePacket* pBasePacket, REF(IHXBuffer*)pSendBuf);
    HX_RESULT makePacket                (BasePacket* pPacket, REF(IHXBuffer*) pPacketBuf);
    void updateQoSInfo                  (UINT32 ulBytesSent);


    HX_RESULT handlePacket              (IHXBuffer* pBuffer);
    HX_RESULT handleMasterSync          (ULONG32 ulHXTime, LONG32 lHXOffsetToMaster);
    HX_RESULT anchorSync                (ULONG32 ulHXTime, ULONG32 ulNTPTime);
    virtual HX_RESULT handleRTCPSync    (NTPTime ntpTime, ULONG32 ulRTPTime);

    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    /* RTP-Info */
    HX_RESULT setFirstSeqNum            (UINT16 streamNumber, UINT16 seqNum, HXBOOL bOnPauseResume = FALSE);
    void notifyRTPInfoProcessed		(HXBOOL bOnPauseResume = FALSE);
    void setFirstTimeStamp              (UINT16 uStreamNumber, UINT32 ulTS,
                                         HXBOOL bIsRaw = FALSE,
					 HXBOOL bOnPauseResume = FALSE);
    void SetFirstTSLive                 (RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw);
    void SetFirstTSStatic               (RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw);

    virtual void  setPlayRange          (UINT32 ulFrom, UINT32 ulTo, HXBOOL bOnPauseResume = FALSE);
    HX_RESULT setFirstPlayTime          (Timeval* pTv);
    void OnPause			(Timeval* pTV);

    void setLegacyRTPLive               (void);

    void setRTCPTransport               (RTCPBaseTransport* pRTCPTran);
    HXBOOL isRTP(void)                        { return TRUE; }
    HXBOOL isReflector(void)                  { return (m_ulPayloadWirePacket==1); }

    void SyncTimestamp                  (IHXPacket* pPacket);

    HX_RESULT getRTCPRule (REF(UINT16) unRTCPRule);

    inline HXBOOL isRTCPRule              (UINT16 unRuleNo)
    {
        return (m_bHasRTCPRule && (unRuleNo == m_RTCPRuleNumber));
    }

    /* XXXMC
     * Special-case handling for PV clients
     */
    void setPVEmulationMode     (HXBOOL bEmulatePVSession);

protected:
#ifdef RTP_MESSAGE_DEBUG
    void messageFormatDebugFileOut(const char* fmt, ...);
#endif  // RTP_MESSAGE_DEBUG

    void resetStartInfoWaitQueue        (void);

    /*
     * Marker Bit Handling Routine
     */
    typedef void (RTPBaseTransport::*HandleMBitFunc)(REF(UINT8),IHXPacket*,UINT16);
    inline void MBitRTPPktInfo (REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo);
    inline void MBitASMRuleNo  (REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo);


    IHXBandwidthManagerInput*           m_pBwMgrInput;
    IHXTransportSyncServer*             m_pSyncServer;
    UINT16                              m_streamNumber;
    INT32                               m_lRefCount;
    UINT8                               m_rtpPayloadType;
    HXBOOL                                m_bHasMarkerRule;
    UINT16                              m_markerRuleNumber;
    HXBOOL                                m_bIsSyncMaster;
    HXBOOL                              m_bDone;

#ifdef RTP_MESSAGE_DEBUG
    HXBOOL                                m_bMessageDebug;
    CHXString                           m_messageDebugFileName;
#endif // RTP_MESSAGE_DEBUG

     /*
      * Reflection support
      */
     HXBOOL                               m_bHasRTCPRule;
     UINT32                             m_ulPayloadWirePacket;
     UINT16                             m_RTCPRuleNumber;

    UINT16                              m_uFirstSeqNum;
    UINT32                              m_ulFirstRTPTS;
    HXBOOL                                m_bFirstSet;
    HXBOOL                                m_bWeakStartSync;

    INT32                               m_lTimeOffsetHX;
    INT32                               m_lTimeOffsetRTP;
    INT32                               m_lOffsetToMasterHX;
    INT32                               m_lOffsetToMasterRTP;
    INT32                               m_lSyncOffsetHX;
    INT32                               m_lSyncOffsetRTP;
    INT32                               m_lNTPtoHXOffset;
    HXBOOL                                m_bNTPtoHXOffsetSet;

    ULONG32                             m_ulLastRTPTS;
    ULONG32                             m_ulLastHXTS;
    ULONG32                             m_ulLastRawRTPTS;
    HXBOOL                                m_bLastTSSet;

    CHXSimpleList                       m_StartInfoWaitQueue;
    UINT32				m_ulWaitQueueBytes;
    HXBOOL				m_bWaitForStartInfo;
    HXBOOL				m_bAbortWaitForStartInfo;
    HXBOOL				m_bFirstSeqNumLocked;

    HXBOOL                                m_bSSRCDetermined;
    UINT32                              m_ulSSRCDetermined;

    UINT32                              m_ulAvgPktSz;
    CReflectionHandler*                m_pReflectionHandler;

    /*
     * RTP-Info:  According to RFC2326, it is possible not to have one of them,
     * so if it is missing, take care of it right here in transport
     */
    HXBOOL                                m_bSeqNoSet;
    /* m_bRTPTimeSet is used differently on the server and the client */
    HXBOOL                                m_bRTPTimeSet;

    UINT32				m_ulSeekCount;


    Timeval*                            m_pFirstPlayTime;
    Timeval*				m_pLastPauseTime;

    LONG32                              m_lRTPOffset; //Reflects RTP session duration.
     


    /* this class does everything for RTCP */
    ReportHandler*                      m_pReportHandler;

    RTCPBaseTransport*                  m_pRTCPTran;

    HXBOOL                                m_bIsLive;
    UINT32                              m_ulExtensionSupport;

    /*
     *  This stream may not have been SETUPed.
     */
    HXBOOL                                m_bActive;

    /* XXXMC
     * Support for PV Emulation
     */
    HXBOOL                m_bEmulatePVSession;

    /*
     * Markerbit Handling
     */
    HandleMBitFunc      m_pMBitHandler;

    /*
     * Reflector RTP-Info
     */
    struct ReflectorInfo
    {
        ReflectorInfo(void)
            : m_unSeqNoOffset(0)
            , m_ulRTPTSOffset(0)
            , m_unNextSeqNo(0)
            , m_bNeedTSOffset(TRUE)

        {}
        UINT16          m_unSeqNoOffset;
        UINT32          m_ulRTPTSOffset;

        /*
         * After PAUSE:
         *  seq needs to be continuous
         *  rtptime needs to reflect time of the initial rtptime
         */
        UINT16          m_unNextSeqNo;
        HXBOOL          m_bNeedTSOffset;
    };
    ReflectorInfo*     m_pReflectorInfo;


    UINT8           m_cLSRRead;
    UINT8           m_cLSRWrite;
    LSRRecord       m_LSRHistory [LSR_HIST_SZ];
    UINT32          MapLSR(UINT32 ulSourceLSR);

    IHXQoSTransportAdaptationInfo*      m_pQoSInfo;

    friend class RTCPBaseTransport;
    friend class RTCPUDPTransport;
    friend class RTCPTCPTransport;

private:
    HX_RESULT _handlePacket(IHXBuffer* pBuffer, HXBOOL bIsRealTime);
    INT16 CompareRTPTimestamp(ULONG32 x, ULONG32 y);
};

/*
*   NOTE:
*   Each RTP transport represents a RTP session because transport is bounded to a pair
*   of addr and ports.

*   As it is currently implemented, RTP transport can NOT have more than one stream.

*   Currently, we do NOT support RTSP/RTP multicast.  so we don't need to do a number
*   of things that should be done if we were to support multicast.
*   1. Maintain a table of members (instread we have just one member)
*   2. Don't really need to calculate RTCP intervals (instead every 5 sec)

*   There are two assumptions due to the nature of 1 to 1 or 1 to many session
*   1. A server (server) never receives RTP.
*   2. There is only one sender (server) in a session.
*/

class RTPUDPTransport : public RTPBaseTransport
{
public:
    RTPUDPTransport                     (HXBOOL bIsSource);
    virtual ~RTPUDPTransport            (void);

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);


    // RTSPTransport
    virtual void Done                   (void);
    virtual RTSPTransportTypeEnum tag   (void);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket);
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer);
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0);

    virtual HX_RESULT init              (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        IHXRTSPTransportResponse* pResp);
    void setPeerAddr                    (IHXSockAddr* pPeerAddr);

    /* XXXMC
     * Special-case handling for PV clients
     */
    HX_RESULT sendPVHandshakeResponse(UINT8* pPktPayload);

protected:
    class KeepAliveCB : public IHXCallback
    {
    public:
        KeepAliveCB(RTPUDPTransport* pTransport);

        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        /*
         *      IHXCallback methods
         */
        STDMETHOD(Func)                 (THIS);

    private:
        RTPUDPTransport*                m_pTransport;
        LONG32                          m_lRefCount;
        ~KeepAliveCB();
    };
    friend class KeepAliveCB;

    HX_RESULT onNATKeepAlive(void);

protected:
    IHXSocket*                          m_pUDPSocket;
    IHXSockAddr*                        m_pPeerAddr;

private:
    HX_RESULT writePacket               (IHXBuffer* pSendBuffer);

    IHXSockAddr*                        m_pMulticastAddr;
    IHXMulticastSocket*                 m_pMulticastSocket;

    CHXKeepAlive m_keepAlive;
    UINT16       m_keepAliveSeq;  // Sequence # for keepalive packets

    friend class RTCPBaseTransport;
    friend class RTCPUDPTransport;
};



class RTPTCPTransport: public RTPBaseTransport
{
public:
    RTPTCPTransport                     (HXBOOL bIsSource);
    ~RTPTCPTransport                    (void);
    void Done                           (void);

    RTSPTransportTypeEnum tag           (void);
    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        IHXRTSPTransportResponse* pResp);
    void setInterleaveChannel           (INT8 tcpInterleave)
    {
        m_tcpInterleave = tcpInterleave;
    }
    HX_RESULT sendPacket                (BasePacket* pPacket);

 protected:
    IHXSocket*                          m_pTCPSocket;
    INT8                                m_tcpInterleave;
    virtual HX_RESULT writePacket      (IHXBuffer* pBuf);

    friend class RTCPBaseTransport;
    friend class RTCPTCPTransport;
};


/******************************************************************************
*   RTCP RTCP RTCP RTCP RTCP
******************************************************************************/
class RTCPBaseTransport: public RTSPTransport,
                         public IHXQoSSignalSourceResponse
{
public:
    RTCPBaseTransport                   (HXBOOL bIsSender);
    ~RTCPBaseTransport                  (void);
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);
    void Done                           (void);

    RTSPTransportTypeEnum tag           (void)
    {
        HX_ASSERT(!"don't call this");
        return RTSP_TR_NONE;
    }
    HX_RESULT sendPacket                (BasePacket* pPacket)
    {
        HX_ASSERT(!"don't call this");
        return HXR_UNEXPECTED;
    }
    HX_RESULT handlePacket              (IHXBuffer* pBuffer);
    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL)
    {
        HX_ASSERT(!"don't call this");
        return HXR_UNEXPECTED;
    }

    void addStreamInfo(RTSPStreamInfo* pStreamInfo,
                       UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);

    void setSSRC(UINT32 ulSSRC);

    void getProtocolOverhead(IHXSocket* pSocket);

    HX_RESULT init                              (void);
    void setSessionID                   (const char* pSessionID);

    void Reset(void){}
    void Restart(void){}

    HX_RESULT SetTSConverter(CHXTimestampConverter::ConversionFactors conversionFactors);

    CHXTimestampConverter* GetTSConverter(void)    { return m_pTSConverter; }

    class ReportCallback : public IHXCallback
    {
    public:
        ReportCallback                  (RTCPBaseTransport* pTransport);

        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        /*
         *      IHXCallback methods
         */
        STDMETHOD(Func)                 (THIS);

    private:
        RTCPBaseTransport*              m_pTransport;
        LONG32                          m_lReportRefCount;
        ~ReportCallback                 ();
    };
    friend class ReportCallback;

    virtual HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf) = 0;
    virtual HX_RESULT sendSenderReport          (void) = 0;
    virtual HX_RESULT sendReceiverReport        (void) = 0;
    virtual HX_RESULT sendBye                   (void) = 0;

    // IHXQoSSignalSourceResponse
    STDMETHOD (SignalBusReady)(THIS_ HX_RESULT hResult, IHXQoSSignalBus* pBus,
                               IHXBuffer* pSessionId);

protected:
    /* caller is responsible for freeing pSendBuf */
    HX_RESULT makeSenderReport          (REF(IHXBuffer*) pSendBuf);
    HX_RESULT makeReceiverReport        (REF(IHXBuffer*) pSendBuf);
    HX_RESULT makeBye                   (REF(IHXBuffer*) pSendBuf);

    HX_RESULT startScheduler            (void);
    HX_RESULT stopScheduler             (void);
    HXBOOL      isShedulerStarted         (void)  { return m_bSchedulerStarted; }
    void      scheduleNextReport        (void);

    HX_RESULT HandleNADUPacket(RTCPPacket *pNADUPacket);

    RTPBaseTransport*                   m_pDataTransport;
    HXBOOL                              m_bDone;

    // XXXGo
    // make sure stream number that RTCP is using is the same as the one
    // in RTP
    UINT16                              m_streamNumber;
    INT32                               m_lRefCount;
    HXBOOL                                m_bSendBye;

    HXBOOL                                m_bSendReport;
    ReportCallback*                     m_pReportCallback;
    HXBOOL                                m_bCallbackPending;
    CallbackHandle                      m_reportTimeoutID;
    HXBOOL                                m_bSchedulerStarted;
    HXBOOL                                m_bSendRTCP;

    HXBOOL                                m_bSSRCDetermined;
    UINT32                              m_ulSSRCDetermined;
    UINT32                              m_ulProtocolOverhead;

    // a random number for CNAME...
    BYTE*                               m_pcCNAME;

    /* this class does everything for RTCP */
    /* and pointing to the same instance that RTPBaseTransport has */
    ReportHandler*                      m_pReportHandler;

    CHXTimestampConverter*              m_pTSConverter;

    IHXThreadSafeScheduler*             m_pTSScheduler;

    /* For placing recevier reports on QoS Signal Bus */
    IHXQoSSignalBus*                    m_pSignalBus;
    IHXQoSSignal*                       m_pQoSSignal_RR;
    IHXQoSSignal*                       m_pQoSSignal_APP;
    IHXQoSSignal*                       m_pQoSSignal_NADU;
    IHXBuffer*                          m_pSessionId;

    /* Timing of RTCP RR Interval: */
    UINT32                              m_ulLastRR;
    UINT32                              m_ulLastSeq;
    UINT32                              m_ulLastLoss;
    UINT32                              m_ulLastRate;
    UINT32                              m_ulRRIntvl;

    IHX3gppNADU*                        m_pNADU;
    UINT32                              m_ulNADURRCount;

    friend class RTPBaseTransport;
    friend class RTPUDPTransport;
    friend class RTPTCPTransport;
};

class RTCPUDPTransport : public RTCPBaseTransport
{
public:
    RTCPUDPTransport                    (HXBOOL bIsSender);
    ~RTCPUDPTransport                   (void);
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    virtual void Done                   (void);
    virtual RTSPTransportTypeEnum tag   (void);
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    virtual HX_RESULT init              (IUnknown* pContext,
                                         IHXSocket* pSocket,
                                         RTPUDPTransport* pDataTransport,
                                         IHXRTSPTransportResponse* pResp,
                                         UINT16 streamNumber);
    void setPeerAddr                    (IHXSockAddr* pPeerAddr);

    HX_RESULT handlePacket(IHXBuffer* pBuffer);

    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0);

protected:
    class KeepAliveCB : public IHXCallback
    {
    public:
        KeepAliveCB(RTCPUDPTransport* pTransport);

        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        /*
         *      IHXCallback methods
         */
        STDMETHOD(Func)                 (THIS);

    private:
        RTCPUDPTransport*               m_pTransport;
        LONG32                          m_lRefCount;
        ~KeepAliveCB();
    };
    friend class KeepAliveCB;

    HX_RESULT onNATKeepAlive(void);

protected:
    IHXSocket*      m_pUDPSocket;
    IHXSockAddr*    m_pPeerAddr;

private:

    HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf);
    HX_RESULT sendSenderReport          (void);
    HX_RESULT sendReceiverReport        (void);
    HX_RESULT sendBye                   (void);


    IHXMulticastSocket*     m_pMulticastSocket;
    IHXSockAddr*            m_pMulticastAddr;

    CHXKeepAlive            m_keepAlive;

    friend class RTPBaseTransport;
    friend class RTPUDPTransport;
};

class RTCPTCPTransport: public RTCPBaseTransport
{
public:
    RTCPTCPTransport                    (HXBOOL bIsSender);
    ~RTCPTCPTransport                   (void);
    void Done                           (void);

    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        RTPTCPTransport* pDataTransport,
                                        IHXRTSPTransportResponse* pResp,
                                        UINT16 streamNumber);
    void setInterleaveChannel           (INT8 tcpInterleave)
    {
        m_tcpInterleave = tcpInterleave;
    }
    RTSPTransportTypeEnum tag           (void);
    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

private:
    HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf);
    HX_RESULT sendSenderReport          (void);
    HX_RESULT sendReceiverReport        (void);
    HX_RESULT sendBye                   (void);
    HX_RESULT writePacket               (IHXBuffer* pBuf);

    IHXSocket*                          m_pTCPSocket;
    INT8                                m_tcpInterleave;
//    friend class RTPBaseTransport;
//    friend class RTPTCPTransport;
};

#endif /* ndef _RTPTRAN_H_ */
