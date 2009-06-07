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
 *    Massimo Perga ( massimo.perga@gmail.com <mailto:massimo.perga@gmail.com> )
 *
 * ***** END LICENSE BLOCK ***** */

#include "hxtypes.h"
#include "hlxclib/stdio.h"

#include "hxcom.h"              // IUnknown
#include "hxcomm.h"            // IHXCommonClassFactory
#include "ihxpckts.h"           // IHXBuffer, IHXPacket, IHXValues
#include "hxplugn.h"           // IHXPlugin
#include "hxrendr.h"           // IHXRenderer
#include "hxengin.h"           // IHXInterruptSafe
#include "hxcore.h"            // IHXStream
#include "hxausvc.h"           // Audio Services
#include "hxmon.h"             // IHXStatistics
#include "hxupgrd.h"           // IHXUpgradeCollection
#include "hxslist.h"            // CHXSimpleList
#include "carray.h"             // CHXPtrArray

#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
#include "mp3format.h"          // MP3 formatter

#include "mp3rend.h"            // CRnMp3Ren
#include "pktparse.h"           // CPacketParser

const UINT32 CPacketParser::DEC_BUFFER_SIZE = 4096;

CPacketParser::CPacketParser() :
    m_pBufHead(NULL),
    m_pDecBuffer(NULL),
    m_ulDecBufBytes(0),
    m_pFmt(NULL),
    m_pRenderer(NULL),
    m_pDecoder(NULL),
    m_bTrustPackets(FALSE),
    m_pClassFactory(NULL),
    m_pLastPCMBuffer(NULL),
    m_dLastPCMTime(0.0),
    m_nLayer(0),
    m_ulBitRate(0),
    m_ulChannels(0),
    m_nWaveBufSize(0),
    m_dFrameTime(0.0),
    m_dNextPts(0.0),
    m_bEndOfPackets(FALSE)
{

}

CPacketParser::~CPacketParser()
{
    HX_VECTOR_DELETE(m_pBufHead);
    HX_DELETE(m_pDecoder);
    HX_RELEASE(m_pRenderer);

    HX_RELEASE(m_pLastPCMBuffer);
    HX_RELEASE(m_pClassFactory);
    HX_DELETE(m_pFmt);
}

HX_RESULT
CPacketParser::Init(CRnMp3Ren* pRenderer,
                    IHXCommonClassFactory* pClassFactory)
{
    if(!pRenderer || !pClassFactory)
    {
        return HXR_INVALID_PARAMETER;
    }
    pRenderer->AddRef();
    pClassFactory->AddRef();

    HX_RELEASE(m_pRenderer);
    HX_RELEASE(m_pClassFactory);
    m_pRenderer = pRenderer;
    m_pClassFactory = pClassFactory;

    if(m_pDecBuffer == NULL)
    {
        m_pBufHead = new BYTE[DEC_BUFFER_SIZE + 31];
        m_pDecBuffer = (UCHAR*)((PTR_INT)m_pBufHead+31 & ~31);

        if(m_pDecBuffer == NULL || m_pBufHead == NULL)
            return HXR_OUTOFMEMORY;
    }

    HX_DELETE(m_pFmt);
    m_pFmt = new CMp3Format(NULL);

    m_pFmt->SetTrustPackets(m_bTrustPackets);

    return m_pFmt ? HXR_OK : HXR_OUTOFMEMORY;
}

UINT32
CPacketParser::DecodeAndRender(UCHAR* pDec, UINT32 ulSize, double dTime,
                               HXBOOL bPacketLoss)
{
    // Dynamic format change - must reinit
    if (DidSourceChange(pDec, ulSize) && !InitDecoder(pDec, ulSize, TRUE))
    {
        return 0;
    }

    // Create our PCM buffer
    IHXBuffer *pPCMBuffer = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pPCMBuffer);
    if (!pPCMBuffer)
    {
        return 0;
    }

    HX_RESULT
    retVal = pPCMBuffer->SetSize(m_nWaveBufSize);
    if (FAILED(retVal) || pPCMBuffer->GetSize() != m_nWaveBufSize)
    {
        HX_RELEASE(pPCMBuffer);
        return 0;
    }

    unsigned long dwPCM = pPCMBuffer->GetSize(); 

    // Decode the frame
    unsigned long ulDec = bPacketLoss ? -1 : (unsigned long)ulSize; 
    m_pDecoder->DecodeFrame_v(pDec,&ulDec,
                               pPCMBuffer->GetBuffer(),
                               &dwPCM); 

    if(bPacketLoss)
    {
        ulDec = (unsigned long)ulSize;
    }

    if(ulDec)
    {
    	/*
    	 * If the current packet cannot be decoded, we use the last frame
    	 * to replace it. It can reduce the small noisy claps in music. 
    	 */
		if(dwPCM == 0 && m_pLastPCMBuffer)
		{
			m_pRenderer->Render(m_pLastPCMBuffer, dTime);
		}
		else
		{
			m_pRenderer->Render(pPCMBuffer, dTime);
		} 
	}

    // Store the pcm buffer of the last decoded frame
    HX_RELEASE(m_pLastPCMBuffer);

    m_pLastPCMBuffer = pPCMBuffer;
    m_pLastPCMBuffer->AddRef();
    m_dLastPCMTime = dTime;

    HX_RELEASE(pPCMBuffer);

    return ulDec;
}

HXBOOL
CPacketParser::DidSourceChange(UCHAR *pHeader, UINT32 ulSize)
{
    unsigned long lPCMSampleRate = 0;
    int nChannels = 0;
    int nBitsPerSample = 0;

    // Get existing header info
    m_pDecoder->GetPCMInfo_v(lPCMSampleRate,
                              nChannels,
                              nBitsPerSample);

    UINT32  ulBitRate = 0;
    UINT32  ulSampRate = 0;

    int     nSrcChannels = 0;
    int     nLayer = 0;
    int     nSamplesPerFrame = 0;
    int     nPadding = 0;

    // Get new header values
    m_pFmt->GetEncodeInfo(pHeader, ulSize,
                          ulBitRate, ulSampRate,
                          nSrcChannels, nLayer, nSamplesPerFrame, nPadding);

    // Allow layer3 bitrates to change
    if (nChannels != nSrcChannels ||
        lPCMSampleRate != ulSampRate ||
        m_pDecoder->GetSamplesPerFrame_n() != nSamplesPerFrame ||
        (m_ulBitRate != ulBitRate && (nLayer != 3 || m_nLayer != 3)))
        return TRUE;
    else
        return FALSE;
}

HXBOOL
CPacketParser::InitDecoder(UCHAR *pBuffer, UINT32 ulSize, HXBOOL bReInit)
{
    if (!m_pDecoder)
    {
        // Create and init the decoder
        m_pDecoder = new CMpaDecObj();
        if (m_pDecoder) m_pDecoder->SetTrustPackets(m_bTrustPackets);
    }

    if (!m_pDecoder->Init_n(pBuffer, ulSize, m_bReformatted))
    {
        HX_DELETE(m_pDecoder);
        return FALSE;
    }

    // Create and init the PCM device
    unsigned long lPCMSampRate = 0;

    int     nPCMChannels = 0,
            nBitsPerSample = 0;

    m_pDecoder->GetPCMInfo_v(lPCMSampRate,
                              nPCMChannels,
                              nBitsPerSample);

    m_nWaveBufSize = m_pDecoder->GetSamplesPerFrame_n() *
                     (nBitsPerSample>>3) * nPCMChannels;

    // Get the time per frame
    m_dFrameTime = m_pDecoder->GetSamplesPerFrame_n() * 1000.0 / lPCMSampRate;

    m_ulChannels = nPCMChannels;

    // Init the audio stream
    HXAudioFormat audioFmt;
    audioFmt.uChannels = nPCMChannels;
    audioFmt.uBitsPerSample = nBitsPerSample;
    audioFmt.ulSamplesPerSec = lPCMSampRate;
    audioFmt.uMaxBlockSize = m_nWaveBufSize;

    HX_ASSERT(m_pRenderer);
    if(bReInit ? !m_pRenderer->ReInitAudioStream(audioFmt) :
       !m_pRenderer->InitAudioStream(audioFmt))
    {
        return FALSE;
    }

    HX_ASSERT(m_pFmt);
    m_pFmt->Init(pBuffer, ulSize);

    UINT32 ulSampRate = 0;
    int nChannels = 0;
    int nSamplesPerFrame = 0;
    int nPadding = 0;

    m_pFmt->GetEncodeInfo(pBuffer, ulSize,
                          m_ulBitRate, ulSampRate,
                          nChannels, m_nLayer, nSamplesPerFrame, nPadding);

    return TRUE;
}

void
CPacketParser::OverrideFactory(IHXCommonClassFactory* pCommonClassFactory)
{
    HX_RELEASE(m_pClassFactory);
    m_pClassFactory = pCommonClassFactory;
    m_pClassFactory->AddRef();
}
