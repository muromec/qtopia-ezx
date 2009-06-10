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

#ifndef _PKTPARSE_H_
#define _PKTPARSE_H_

class CRnMp3Ren;

/***********************************************************
 * CPacketParser: Base class packet parser and decoder
 */

class CPacketParser
{
public:
    CPacketParser();
    virtual ~CPacketParser();
    
    virtual HX_RESULT   Init            (CRnMp3Ren* pRenderer, 
                                         IHXCommonClassFactory* pClassFactory);
    virtual HX_RESULT   AddPacket       (IHXPacket* pPacket, 
                                         INT32 streamOffsetTime) PURE;
    virtual HX_RESULT   RenderAll       (void) PURE; 

    inline virtual void     EndOfPackets    (void) { m_bEndOfPackets = TRUE; }
    inline virtual UINT32   GetChannels     (void) { return m_ulChannels; }
    inline virtual double   GetTimePerPkt   (void) { return m_dFrameTime; }
    inline virtual double   GetLastPCMTime  (void) { return m_dLastPCMTime; }

    inline virtual void     Begin           (UINT32 time) { m_dNextPts = 0.0; }
    inline virtual void     PreSeek         (void)
    {
        HX_DELETE(m_pDecoder);
        m_ulDecBufBytes = 0;
    }
    inline virtual void     PostSeek        (UINT32 time) { HX_DELETE(m_pDecoder);
							    m_ulDecBufBytes = 0;
							    m_dNextPts = 0.0;
                                                            HX_RELEASE(m_pLastPCMBuffer); }
    void OverrideFactory(IHXCommonClassFactory* pCommonClassFactory);
    void SetTrustPackets(HXBOOL bTrust) { m_bTrustPackets = bTrust; }
    
protected:
    virtual HXBOOL InitDecoder(UCHAR *pBuffer, UINT32 ulSize, HXBOOL bReInit);

    UINT32  DecodeAndRender(UCHAR* pDec, UINT32 ulSize, double dTime, 
                            HXBOOL bPacketLoss);
    HXBOOL    DidSourceChange(UCHAR *pHeader, UINT32 ulSize);    

    UCHAR*          m_pBufHead;
    UCHAR*          m_pDecBuffer;
    UINT32          m_ulDecBufBytes;
    
    CMp3Format*     m_pFmt;
    CRnMp3Ren*      m_pRenderer;
    CMpaDecObj*     m_pDecoder;

    HXBOOL            m_bReformatted;
    HXBOOL            m_bTrustPackets;

    IHXCommonClassFactory* m_pClassFactory;
    
    IHXBuffer*     m_pLastPCMBuffer;
    double          m_dLastPCMTime;

    int             m_nLayer;
    UINT32          m_ulBitRate;
    UINT32          m_ulChannels;    

    UINT16          m_nWaveBufSize;

    double          m_dFrameTime;
    double          m_dNextPts;
    HXBOOL            m_bEndOfPackets;

    static const UINT32 DEC_BUFFER_SIZE;
};

#endif // _PKTPARSE_H_
