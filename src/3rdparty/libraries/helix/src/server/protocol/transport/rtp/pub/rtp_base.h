/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtp_base.h,v 1.8 2006/12/21 19:16:13 tknox Exp $
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

#ifndef _RTP_BASE_H_
#define _RTP_BASE_H_

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

#include "transport.h"

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

class ServerRTCPBaseTransport;
class ServerRTCPUDPTransport;
class ServerRTCPTCPTransport;
class RTCPPacket;
class ReportHandler;
class CReflectionHandler;

_INTERFACE IHXQoSSignal;
_INTERFACE IHXQoSSignalBus;
_INTERFACE IHXQoSSignalSourceResponse;

HX_RESULT
ServerFixRTPHeader(IHXCommonClassFactory* pCCF,
             IHXBuffer* pOrigBuf,
             REF(IHXBuffer*) pNewBuf,
             UINT16 unSeqNoOffset,
             UINT32 ulRTPTSOffset);

HX_RESULT
ServerFixRTCPSR(IHXCommonClassFactory* pCCF,
          IHXBuffer* pOrigBuf,
          REF(IHXBuffer*) pNewBuf,
          UINT32 ulRTPTSOffset);

/* The largest number of SR records we will keep */
#define LSR_HIST_SZ 64

#define RTP_FILTER_CONSTANT 3

struct ServerLSRRecord
{
    UINT32          m_ulSourceLSR;
    UINT32          m_ulServerLSR;
};

/******************************************************************************
*   RTP RTP RTP RTP RTP
******************************************************************************/
class ServerRTPBaseTransport: public Transport
{
public:
    ServerRTPBaseTransport                    (HXBOOL bIsSource);
    ~ServerRTPBaseTransport                   (void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    void Reset(void){}
    void Restart(void){}

    // Transport
    STDMETHOD_(void, Done)              (THIS);
    virtual RTSPTransportTypeEnum tag   (void)
            {
                HX_ASSERT(!"don't call this");
                return RTSP_TR_NONE;
            };
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);

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
    //HX_RESULT handleMasterSync          (ULONG32 ulHXTime, LONG32 lHXOffsetToMaster);
    //HX_RESULT anchorSync                (ULONG32 ulHXTime, ULONG32 ulNTPTime);
    virtual HX_RESULT handleRTCPSync    (NTPTime ntpTime, ULONG32 ulRTPTime);

    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    /* RTP-Info */
    HX_RESULT setFirstSeqNum            (UINT16 streamNumber, UINT16 seqNum);
    //void notifyRTPInfoProcessed        (void);
    void setFirstTimeStamp              (UINT16 uStreamNumber, UINT32 ulTS,
                                         HXBOOL bIsRaw = FALSE);
    void SetFirstTSLive                 (RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw);
    void SetFirstTSStatic               (RTSPStreamData* pStreamData, UINT32 ulTS, HXBOOL bIsRaw);

    //void setPlayRange                   (UINT32 ulFrom, UINT32 ulTo);
    HX_RESULT setFirstPlayTime          (Timeval* pTv);
    void OnPause            (Timeval* pTV);

    void setLegacyRTPLive               (void);

    void setRTCPTransport               (ServerRTCPBaseTransport* pRTCPTran);
    STDMETHOD_(HXBOOL,isRTP)		(THIS) { return TRUE; }
    STDMETHOD_(HXBOOL,isReflector)	(THIS) { return (m_ulPayloadWirePacket==1); }

    void SyncTimestamp                  (IHXPacket* pPacket);

    STDMETHOD (getRTCPRule)	        (THIS_ REF(UINT16) unRTCPRule);

    inline HXBOOL isRTCPRule              (UINT16 unRuleNo)
    {
        return (m_bHasRTCPRule && (unRuleNo == m_RTCPRuleNumber));
    }

    /* XXXMC
     * Special-case handling for PV clients
     */
    void setPVEmulationMode     (HXBOOL bEmulatePVSession);
    
    HX_RESULT getSSRC                        (UINT32& uSSRC);

protected:
#ifdef RTP_MESSAGE_DEBUG
    void messageFormatDebugFileOut(const char* fmt, ...);
#endif  // RTP_MESSAGE_DEBUG

    /*
     * Marker Bit Handling Routine
     */
    typedef void (ServerRTPBaseTransport::*HandleMBitFunc)(REF(UINT8),IHXPacket*,UINT16);
    inline void MBitRTPPktInfo (REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo);
    inline void MBitASMRuleNo  (REF(UINT8)bMBit, IHXPacket* pPkt, UINT16 unRuleNo);


    IHXTransportSyncServer*             m_pSyncServer;
    UINT16                              m_streamNumber;
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

    INT32                               m_lTimeOffsetRTP;
    INT32                               m_lSyncOffsetHX;
    INT32                               m_lSyncOffsetRTP;
    INT32                               m_lNTPtoHXOffset;
    HXBOOL                                m_bNTPtoHXOffsetSet;

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


    Timeval*                            m_pFirstPlayTime;
    Timeval*                m_pLastPauseTime;

    LONG32                              m_lRTPOffset; //Reflects RTP session duration.
     


    /* this class does everything for RTCP */
    ReportHandler*                      m_pReportHandler;

    ServerRTCPBaseTransport*                  m_pRTCPTran;

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
         * After PAUSE: seq needs to be continuous rtptime needs to reflect
         * time of the initial rtptime
         */
        UINT16          m_unNextSeqNo;
        HXBOOL          m_bNeedTSOffset;
    };
    ReflectorInfo*     m_pReflectorInfo;


    UINT8           m_cLSRRead;
    UINT8           m_cLSRWrite;
    ServerLSRRecord m_LSRHistory [LSR_HIST_SZ];
    UINT32          MapLSR(UINT32 ulSourceLSR);

    IHXQoSTransportAdaptationInfo*      m_pQoSInfo;

    friend class ServerRTCPBaseTransport;
    friend class ServerRTCPUDPTransport;
    friend class ServerRTCPTCPTransport;

    friend class CUTServerRTPUDPTransportTestDriver; // For unit testing
    friend class CUTServerRTPTCPTransportTestDriver; // For unit testing

private:
    HX_RESULT _handlePacket(IHXBuffer* pBuffer, HXBOOL bIsRealTime);
};

/******************************************************************************
*   RTCP RTCP RTCP RTCP RTCP
******************************************************************************/
class ServerRTCPBaseTransport: public Transport,
                         public IHXQoSSignalSourceResponse
{
public:
    ServerRTCPBaseTransport                   (HXBOOL bIsSender);
    ~ServerRTCPBaseTransport                  (void);
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);
    STDMETHOD_(void, Done)              (THIS);

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
    virtual HX_RESULT handlePacketList  (CHXSimpleList* pList, IHXBuffer* pBuffer);
    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL)
    {
        HX_ASSERT(!"don't call this");
        return HXR_UNEXPECTED;
    }

    void addStreamInfo(RTSPStreamInfo* pStreamInfo,
                       UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);

    void getProtocolOverhead(IHXSocket* pSocket);

    HX_RESULT init                              (void);
    void setSessionID                   (const char* pSessionID);

    HX_RESULT getSSRC                        (UINT32& uSSRC);

    void Reset(void){}
    void Restart(void){}

    HX_RESULT SetTSConverter(CHXTimestampConverter::ConversionFactors conversionFactors);

    CHXTimestampConverter* GetTSConverter(void)    { return m_pTSConverter; }

    class ReportCallback : public IHXCallback
    {
    public:
        ReportCallback                  (ServerRTCPBaseTransport* pTransport);

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
        ServerRTCPBaseTransport*              m_pTransport;
        LONG32                          m_lReportRefCount;
        ~ReportCallback                 ();
    };
    friend class ReportCallback;

    virtual HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf) = 0;
    virtual HX_RESULT sendSenderReport          (void) = 0;
    virtual HX_RESULT sendReceiverReport        (void) { return HXR_NOTIMPL; }
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

    ServerRTPBaseTransport*                   m_pDataTransport;
    HXBOOL                              m_bDone;

    // XXXGo
    // make sure stream number that RTCP is using is the same as the one
    // in RTP
    UINT16                              m_streamNumber;
    HXBOOL                                m_bSendBye;

    HXBOOL                                m_bSendReport;
    ReportCallback*                     m_pReportCallback;
    HXBOOL                                m_bCallbackPending;
    CallbackHandle                      m_reportTimeoutID;
    HXBOOL                                m_bSchedulerStarted;
    HXBOOL                                m_bSendRTCP;

    UINT32                              m_ulProtocolOverhead;

    // a random number for CNAME...
    BYTE*                               m_pcCNAME;

    /* this class does everything for RTCP */
    /* and pointing to the same instance that ServerRTPBaseTransport has */
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

    friend class ServerRTPBaseTransport;
    friend class ServerRTPUDPTransport;
    friend class ServerRTPTCPTransport;
    friend class CUTServerRTPTCPTransportTestDriver;
    friend class CUTServerRTPUDPTransportTestDriver;
};

#endif /* ndef _RTP_BASE_H_ */
