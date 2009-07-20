/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _AMRPACKETIZER_H_
#define _AMRPACKETIZER_H_

#define AMR_NB_MIME_TYPE        "audio/AMR"
#define AMR_WB_MIME_TYPE        "audio/AMR-WB"

#define AMR_SDP_SIZE            128
#define AMR_DFLT_MAX_PAYLOAD    1440
#define AMR_MAX_PAYLOAD_FRAMES  25  // 25 frames = 500 ms
#define AMR_FRAME_TIME          20  // 20 ms per frame

#define AMR_NB_CFG_MAX_PYLD     "config.Datatypes." AMR_NB_MIME_TYPE \
                                ".MaxPacketSize"
#define AMR_WB_CFG_MAX_PYLD     "config.Datatypes." AMR_WB_MIME_TYPE \
                                ".MaxPacketSize"

#include "amr_frame_info.h"

class AMRPacketizer	: public IHXPayloadFormatObject
{
public:
    AMRPacketizer(AMRFlavor flavor = NarrowBand);
    ~AMRPacketizer();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)             (THIS_ IUnknown* pContext, HXBOOL bPacketize);
    STDMETHOD(Close)            (THIS);
    STDMETHOD(Reset)            (THIS);
    STDMETHOD(SetStreamHeader)  (THIS_ IHXValues* pHeader);
    STDMETHOD(GetStreamHeader)  (THIS_ REF(IHXValues*) pHeader);
    STDMETHOD(SetPacket)        (THIS_ IHXPacket* pPacket);
    STDMETHOD(GetPacket)        (THIS_ REF(IHXPacket*) pPacket);
    STDMETHOD(Flush)            (THIS);

private:
    class SampleInfo;
    struct PacketInfo;

    void HandleMaxPacketSize        (void);
    HX_RESULT AddHeaderMimeType     (void);
    HX_RESULT AddHeaderSDPData      (void);
    HX_RESULT ParseSampleEntry      (void);
    HX_RESULT FindFrameBlocks       (UINT32& ulFrameCount, 
                                     UINT32& ulPayloadSize);
    UINT32 WritePayload             (UINT32 ulFrames, UCHAR* pPayload, 
                                     PacketInfo* pPktInfo);

    static inline int WriteFMTP         (char* pWriter, UINT32 ulSize, 
                                         UINT32 ulRTPPayloadType);
    inline void WritePayloadHeader      (UINT8*& pHeader, UINT8 uILP = 0);
    inline UINT32 PayloadHeaderSize     () { return m_bInterleave ? 2 : 1; }
    inline void GetPacketInfo           (SampleInfo* pSample, 
                                         PacketInfo* pPktInfo);

    INT32                   m_lRefCount;
    IUnknown*               m_pContext;
    IHXCommonClassFactory*  m_pClassFactory;
    IHXValues*              m_pStreamHeader;
    CHXSimpleList           m_InputPackets;
    HXBOOL                    m_bUsesRTPPackets;
    HXBOOL                    m_bRTPPacketTested;
    HXBOOL                    m_bFlushed;
    UINT32                  m_ulMaxPayloadSize;
    UINT32                  m_ulSamplesPerSecond;
    UINT32                  m_ulChannels;

    CTSConverter            m_TSConverter;
    AMRFlavor               m_AMRFlavor;
    UINT32                  m_ulFrameBlocksPerPacket;
    HXBOOL                    m_bInterleave;
    UINT8                   m_uInterleaveLength;

    class SampleInfo
    {
    public:
        SampleInfo(IHXPacket* pPacket, AMRFlavor flavor);
        ~SampleInfo();
        UINT32 GetNumFrames (UINT8* pSample, UINT32 ulSampleSize);

        IHXPacket* m_pPacket;
        IHXBuffer* m_pBufObj;
        UINT8* m_pBuffer;
        UINT32 m_ulSize;
        UINT32 m_ulFrames;
        UINT32 m_ulTotalSize;
        UINT32 m_ulTotalFrames;
        AMRFlavor m_AMRFlavor;
    };

    struct PacketInfo
    {
        UINT32 ulTime;
        UINT32 ulRTPTime;
        UINT16 unStreamNumber;
        UINT8  unASMFlags;
        UINT16 unASMRuleNumber;
    };
};

inline int
AMRPacketizer::WriteFMTP(char* pWriter, UINT32 ulSize, UINT32 ulRTPPayloadType)
{
    return SafeSprintf(pWriter, ulSize, "a=fmtp:%u octet-align=1\r\n",
        ulRTPPayloadType);
}

inline void
AMRPacketizer::WritePayloadHeader(UINT8*& pHeader, UINT8 uILP)
{
    // CMR(4): 0xF since we're not receiving anything
    // reserved(4): 0
    *(pHeader++) = 0xF0;

    if(m_bInterleave)
    {
        // ILL(8), ILP(8)
        pHeader[0] = m_uInterleaveLength;
        pHeader[1] = uILP;
        pHeader += 2;
    }
}

inline void
AMRPacketizer::GetPacketInfo(SampleInfo* pSample, PacketInfo* pPktInfo)
{
    IHXPacket* pInPacket = pSample->m_pPacket;

    // Calculate the time offset for the frame we are starting with
    UINT32 ulTimeDiff = AMR_FRAME_TIME * 
        (pSample->m_ulTotalFrames - pSample->m_ulFrames);

    pPktInfo->ulTime = pInPacket->GetTime() + ulTimeDiff;
    pPktInfo->unStreamNumber = pInPacket->GetStreamNumber();
    pPktInfo->unASMFlags = pInPacket->GetASMFlags();
    pPktInfo->unASMRuleNumber = pInPacket->GetASMRuleNumber();
    pPktInfo->ulRTPTime = m_bUsesRTPPackets ?
        ((IHXRTPPacket*)pInPacket)->GetRTPTime() + 
        m_TSConverter.Convert(ulTimeDiff) : pPktInfo->ulTime;
}

#endif /* _AMRPACKETIZER_H_ */
