/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rdttran.h,v 1.6 2005/03/10 20:59:21 bobclark Exp $ 
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

#ifndef _RDTTRAN_H_
#define _RDTTRAN_H_

#include "hxengin.h"
#include "hxmon.h"
#include "transbuf.h"
#include "statinfo.h"
#include "hxsmbw.h"
#include "hxerror.h"

// GCC won't let me forward declare CHXMapLongToObj::Iterator,
// so I have to include this. -JR
#include "hxmap.h"

#include "packfilt.h"

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
class  TNGLatencyReportPacket;

class TNGDataPacket;
class TNGReportPacket;
class TNGACKPacket;
class TNGRTTRequestPacket;
class TNGRTTResponsePacket;
class TNGCongestionPacket;
class TNGStreamEndPacket;

class ReportHandler;

class TNGUDPTransport: public RTSPTransport, public IHXSourceBandwidthInfo
{
public:
    TNGUDPTransport                     (HXBOOL bIsServer, HXBOOL bIsSource,
                                         RTSPTransportTypeEnum lType,
                                         UINT32 uFeatureLevel = 0,
                                         HXBOOL bDisableResend = FALSE);
    virtual ~TNGUDPTransport            (void);

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    virtual void Done                   (void);
    virtual void Reset                  (void);
    virtual void Restart                (void);
    virtual RTSPTransportTypeEnum tag   (void);
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket);
    virtual HX_RESULT sendToResendBuffer(BasePacket* pPacket);
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer);
    virtual HX_RESULT releasePackets    (void);
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);
    virtual HX_RESULT sendNAKPacket     (UINT16 uStreamNumber,
                                         UINT16 uBeginSeqNo,
                                         UINT16 uEndSeqNo);

    // BCM
    virtual RTSPStreamHandler* GetStreamHandler(void);
    virtual void SetStreamHandler       (RTSPStreamHandler* pHandler);
    virtual void MulticastSetup         (RTSPStreamHandler* pHandler);
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSock = 0);
    virtual HXBOOL isBCM                  (void) { return m_bBCM; }
    // end BCM

    virtual HX_RESULT pauseBuffers      (void);
    virtual HX_RESULT resumeBuffers     (void);
    virtual HXBOOL SupportsPacketAggregation(void) { return (m_uFeatureLevel >= 2); }
    virtual HX_RESULT sendPackets       (BasePacket** pPacket);

    /*
     *  IHXSourceBandwidthInfo methods
     */
    STDMETHOD(InitBw)                   (THIS_
                                        IHXBandwidthManagerInput* pBwMgr);

    STDMETHOD(SetTransmitRate)          (THIS_
                                        UINT32 ulBitRate);

    HX_RESULT init                      (IUnknown* pContext,
                                         IHXSocket* pSocket,
                                         IHXRTSPTransportResponse* pResp);
    void setPeerAddr                    (IHXSockAddr* pAddr);
    HX_RESULT sendPacketFast            (BasePacket* pPacket,
                                         UINT8* pWriteHere = 0,
                                         UINT32* pSizeWritten = NULL);
    HX_RESULT sendMulticastPacket       (BasePacket* pPacket);
    HX_RESULT sendBWReportPacket        (INT32 aveBandwidth, INT32 packetLoss,
                                        INT32 bandwidthWanted);
    HX_RESULT sendRTTRequestPacket      (void);
    HX_RESULT sendRTTResponsePacket     (UINT32 secs, UINT32 uSecs);
    HX_RESULT sendCongestionPacket      (INT32 xmitMultiplier,
                                        INT32 recvMultiplier);
    HX_RESULT sendStreamEndPacket       (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    void setLatencyPeriod               (UINT32 ulMs);

    void setNormalACKInterval           (UINT32 mSecs);
    void setMinimumACKInterval          (UINT32 mSecs);
    void handleACKTimeout               ();

    void setFeatureLevel(UINT32 uFeatureLevel);

    class ACKCallback : public IHXCallback
    {
    public:
        ACKCallback                     (TNGUDPTransport* pTransport);

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
        friend class TNGUDPTransport;
        TNGUDPTransport*                m_pTransport;
        LONG32                          m_lAckRefCount;
                                        ~ACKCallback();
    };
    friend class ACKCallback;

    class SeekCallback : public IHXCallback
    {
    public:
        SeekCallback                    (TNGUDPTransport* pTransport);

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
        friend class TNGUDPTransport;
        TNGUDPTransport*                m_pTransport;
        LONG32                          m_lAckRefCount;
        HXBOOL                            m_bIsCallbackPending;
        CallbackHandle                  m_Handle;
                                        ~SeekCallback();
    };
    friend class SeekCallback;

#if(0) //XXXLCM not used?
    class UDPOutputCallback : public IHXCallback
    {
    public:
        UDPOutputCallback               (TNGUDPTransport* pTransport,
                                         IHXSockAddr* pPeerAddr,
                                         IHXBuffer* pBuffer,
                                         IHXSocket* pUDPSocket);

        virtual ~UDPOutputCallback              (void);

        /* IUnknown methods */
        STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)      (THIS);
        STDMETHOD_(ULONG32,Release)     (THIS);

        /* IHXCallback methods */
        STDMETHOD(Func)                 (THIS);

    private:
        friend class TNGUDPTransport;
        LONG32                          m_lRefCount;
        TNGUDPTransport*                m_pTransport;
        IHXSockAddr*                    m_pPeerAddr;
        IHXBuffer*                      m_pBuffer;
        IHXSocket*                      m_pSocket;
        HXBOOL                            m_bDone;
    };
    friend class UDPOutputCallback;


#endif // XXXLCM not used

private:


    // HX_RESULT m_udpOutputStatus; XXXLCM not used (for UDPOutputCallback)
    void StartAck(void);
    void KillAck(void);
    typedef struct _BitmapDataItem
    {
        BYTE* m_pData;
        UINT32 m_dataLen;
    } BitmapDataItem;

    typedef struct _BwDetectionData
    {
        UINT32      m_ulSize;
        UINT32      m_ulTimeStamp;
        UINT32      m_ulATime;
    } BwDetectionData;

    HXBOOL createLatencyReportPacket(TNGLatencyReportPacket* pRpt);
    void startScheduler                 (void);
    HX_RESULT sendACKPacket             (void);

    HX_RESULT handleDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                               UINT32* pLen, UINT32 ulTimeStamp);
    HX_RESULT handleMulticastDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen, UINT32 ulTimeStamp);
    HX_RESULT handleASMActionPacket     (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleBWReportPacket      (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleACKPacket           (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleRTTRequestPacket    (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleRTTResponsePacket   (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleCongestionPacket    (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleStreamEndPacket     (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleBandwidthReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                          UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                           UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                            UINT32* pLen);
    HX_RESULT handleAutoBWDetectionPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen);

    HX_RESULT writePacket               (BYTE* pData, UINT32 dataLen);

    INT32                               m_lRefCount;
    IHXSocket*                          m_pUDPSocket;
    IHXFastPathNetWrite*                m_pFastPathNetWrite;
    IHXSockAddr*                        m_pPeerAddr;
    UINT32                              m_ulNormalACKInterval;
    UINT32                              m_ulMinimumACKInterval;
    Timeval*                            m_pLastACKTime;
    CallbackHandle                      m_ackTimeoutID;
    ACKCallback*                        m_pACKCallback;
    SeekCallback*                       m_pSeekCallback;
    HXBOOL                                m_bCallbackPending;
    IHXBandwidthManagerInput*           m_pBwMgrInput;
    UINT32                              m_lastLatencyReportTime;
    UINT32                              m_LatencyReportStartTime;
    UINT32                              m_ulBackToBackTime;
    UINT32                              m_ulLatencyReportPeriod;
    RTSPTransportTypeEnum               m_lTransportType;
    IHXMulticastSocket*                 m_pMulticastSocket;
    IHXSockAddr*                        m_pMulticastAddr;
    HXBOOL                                m_bIsServer;
    UINT32                              m_uFeatureLevel;
    IHXAccurateClock*                   m_pAccurateClock;
    HXBOOL                                m_bDisableResend;

    HXBOOL                                m_bBCM;
    IHXErrorMessages*                   m_pErrMsg;
};


class TNGTCPTransport: public RTSPTransport, public IHXSourceBandwidthInfo
{
public:
    TNGTCPTransport                     (HXBOOL bIsSource,
                                         HXBOOL bNoLostPackets = FALSE,
                                         UINT32 uFeatureLevel = 0);
    virtual ~TNGTCPTransport            (void);

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    virtual void Done                   (void);
    virtual void Reset                  (void) {};  //XXXLCM need implement?
    virtual void Restart                (void) {};
    virtual RTSPTransportTypeEnum tag   (void);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket);
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer);
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    /* IHXSourceBandwidthInfo */
    STDMETHOD(InitBw)           (THIS_
                                IHXBandwidthManagerInput* pBwMgr);

    STDMETHOD(SetTransmitRate)  (THIS_
                                UINT32 ulBitRate);

    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        INT8 interleave,
                                        IHXRTSPTransportResponse* pResp);
    HX_RESULT sendPackets               (BasePacket** pPacket);
    HX_RESULT sendStreamEndPacket       (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);
    virtual HX_RESULT startPackets      (UINT16 uStreamNumber);
    virtual HX_RESULT stopPackets       (UINT16 uStreamNumber);

    void setTransportInterleave         (INT8 interleave);

    void setFeatureLevel(UINT32 uFeatureLevel);

private:
    HXBOOL createLatencyReportPacket(TNGLatencyReportPacket* pRpt);
    HX_RESULT handleDataPacket          (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen, UINT32 ulTimeStamp);
    HX_RESULT handleStreamEndPacket     (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                           UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                            UINT32* pLen);
    HX_RESULT handleAutoBWDetectionPacket(IHXBuffer* pBuffer, UINT32* pPos, UINT32* pLen);

    HX_RESULT writePacket               (BYTE* pData, UINT32 dataLen);
    HX_RESULT writePacket               (IHXBuffer* pBuffer);

    INT32                               m_lRefCount;
    IHXSocket*                          m_pTCPSocket;
    IHXBufferedSocket*                  m_pFastSocket; //XXXLCM unused/deprecated?
    INT8                                m_tcpInterleave;
    HXBOOL                                m_bNoPacketBuffering;
    HXBOOL                                m_bPacketsStarted;

    IHXBandwidthManagerInput*           m_pBwMgrInput;
    UINT32                              m_lastLatencyReportTime;
    UINT32                              m_LatencyReportStartTime;
    UINT32                              m_uFeatureLevel;
    IHXAccurateClock*                   m_pAccurateClock;
    HXBOOL                                m_bNoLostPackets;
};

#endif /* ndef _RDTTRAN_H_ */
