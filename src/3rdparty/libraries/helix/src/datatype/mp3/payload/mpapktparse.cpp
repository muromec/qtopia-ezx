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
#include "tconverter.h"
#include "hxstrutl.h"

#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
#include "mp3format.h"          // MP3 formatter

#ifdef DEMUXER
#include "xmddemuxer.h"         // Demuxer
#include "xmdtypes.h"
#endif

#include "mp3rend.h"            // CRnMp3Ren
#include "pktparse.h"           // CPacketParser
#include "mpapktparse.h"        // CMpaPacketParser

CMpaPacketParser::CMpaPacketParser() :    
    CPacketParser(),
    m_pPacket(NULL),    
    m_bPacketLoss(FALSE)
{
    m_bReformatted = FALSE;
}

CMpaPacketParser::~CMpaPacketParser()
{
    HX_RELEASE(m_pPacket);
}

/********************************************************************
 * CMpaPacketParser::AddPacket: Adds a packet. Assumes that all frames 
 * in the packet will be retrieved before AddPacket is called again. 
 * Will not queue packets.
 */
HX_RESULT
CMpaPacketParser::AddPacket(IHXPacket* pPacket, INT32 streamOffsetTime)
{
#ifdef SIM_PKT_LOSS
    static int nPackets = 0;
    if (++nPackets % 10 == 0)
    {
        m_bPacketLoss = TRUE;
        return HXR_FAIL;
    }
#endif //SIM_PKT_LOSS 

    HX_ASSERT(pPacket);
    if(pPacket->IsLost())
    {
        m_bPacketLoss = TRUE;
        return HXR_FAIL;
    }
    else
    {
        // Save this packet        
        HX_RELEASE(m_pPacket);
        m_pPacket = pPacket;
        m_pPacket->AddRef();
        m_lStreamOffsetTime = streamOffsetTime;
    }

    return HXR_OK;
}

HX_RESULT
CMpaPacketParser::RenderAll()
{
    if(!m_pRenderer || !m_pFmt || !m_pPacket)
    {
        return HXR_FAIL;
    }

    UINT32 ulSize;
    UCHAR* pBuffer;    
    IHXBuffer* pBufObj = m_pPacket->GetBuffer();    

    if(!pBufObj)
    {
        // Null buffer?
        return HXR_FAIL;
    }

    pBufObj->Get(pBuffer, ulSize);
    if(!pBuffer || !ulSize)
    {
        // empty packet!
        HX_RELEASE(pBufObj);
        return HXR_FAIL;
    }

    if(!ParseHeaders(pBuffer, ulSize))
    {
        HX_RELEASE(pBufObj);
        return HXR_FAIL;
    }

    if (m_bPacketLoss)
    {
        HandlePacketLoss();
        m_bPacketLoss = FALSE;
    }

    if(m_dNextPts == 0.0)
    {
        m_dNextPts = m_pPacket->GetTime();
        if(m_lStreamOffsetTime > m_dNextPts)
        {
            m_dNextPts =  0;
        }
        else
        {
            m_dNextPts -= m_lStreamOffsetTime;
        }
    }

    HX_RESULT retVal = HXR_OK;
    // Packets may exceed the decode buffer size, so copy the
    // packet iteratively if necessary.
    do
    {
        // Copy packet data to decode buffer to prevent a potential
        // access vioaltion in the mp3 decoder and to deal with rtp
        // packet fragmentation.
        UINT32 ulCopy = HX_MIN(ulSize, DEC_BUFFER_SIZE - m_ulDecBufBytes);

        memcpy(m_pDecBuffer + m_ulDecBufBytes, pBuffer, ulCopy); /* Flawfinder: ignore */
        m_ulDecBufBytes += ulCopy;
    
        pBuffer += ulCopy;
        ulSize -= ulCopy;

        // Use these temp variables to pass to the decoder
        UCHAR   *pDec = m_pDecBuffer;
        UINT32  ulDec = m_ulDecBufBytes;

        int nFrameSize = 0;
        INT32 lScan = 0;

        // Do we need to init our decoder
        if (!m_pDecoder)
        {
            lScan = m_pFmt->ScanForSyncWord(pDec, ulDec, nFrameSize);

            if (lScan >= 0)
            {
                pDec += lScan;
                ulDec -= lScan;
                m_ulDecBufBytes -= lScan;
            }
            else
            {
                // If our buffer is full and we could not find a frame,
                // we have bad data.  So skip 3/4 of buffered data.
                if (DEC_BUFFER_SIZE == m_ulDecBufBytes)
                {
                    pDec += DEC_BUFFER_SIZE * 3 / 4;
                    m_ulDecBufBytes -= DEC_BUFFER_SIZE * 3 / 4;
                }
                
                ulDec = 0;
            }

            if (ulDec && !InitDecoder(pDec, ulDec, FALSE))
            {
                HX_RELEASE(pBufObj);
                return HXR_UNSUPPORTED_AUDIO;
            }
        }

        // Decode all frames in this buffer
        while (ulDec)
        {
            // Make sure we don't have a packet frag
            if (!m_pFmt->CheckValidFrame(pDec, ulDec))
            {
                // Scan through any bad data in the file
                nFrameSize = 0;
                lScan = m_pFmt->ScanForSyncWord(pDec, ulDec, nFrameSize);

                if (lScan >= 0)
                {
                    pDec += lScan;
                    ulDec -= lScan;
                    m_ulDecBufBytes -= lScan;
                }
                else
                {
                    // If our buffer is full and we could not find a frame,
                    // we have bad data.  So skip 3/4 of buffered data.
                    if (DEC_BUFFER_SIZE == m_ulDecBufBytes)
                    {
                        pDec += DEC_BUFFER_SIZE * 3 / 4;
                        m_ulDecBufBytes -= DEC_BUFFER_SIZE * 3 / 4;
                    }
                    break;
                }
            }

            // Decode and render this frame
            ulDec = DecodeAndRender(pDec, ulDec, m_dNextPts, m_bPacketLoss);

            if (ulDec)
            {
                if(ulDec > m_ulDecBufBytes)
                    ulDec = m_ulDecBufBytes;

                pDec += ulDec;
                m_ulDecBufBytes -= ulDec;
                ulDec = m_ulDecBufBytes;

                m_dNextPts += m_dFrameTime;
            }
            else
            {
                retVal = HXR_OUTOFMEMORY;
                break;
            }
        }

        // Copy the leftovers to the start of the buffer
        if (m_ulDecBufBytes)
            memmove(m_pDecBuffer, pDec, m_ulDecBufBytes);

    } while (ulSize);

    HX_RELEASE(pBufObj);
    return retVal;
}

void
CMpaPacketParser::RestartStream(void)
{
    m_bPacketLoss = FALSE;
    m_ulDecBufBytes = 0;

    HX_RELEASE(m_pLastPCMBuffer);
    m_dLastPCMTime = 0.0;
}

C2250PacketParser::~C2250PacketParser()
{
    HX_DELETE(m_pTsConvert);
}

void C2250PacketParser::PostSeek(UINT32 time)
{
    CMpaPacketParser::PostSeek(time);
    HX_DELETE(m_pTsConvert);
}

HXBOOL
C2250PacketParser::ParseHeaders(UCHAR*& pBuffer, UINT32& ulSize)
{
    // Skip rtp payload header
    // Check for bad or unknown packets
    if(pBuffer[0] || pBuffer[1])
        return FALSE;

    // Only extract the rtp pts for a packet containing the start of a frame
    if (!pBuffer[2] && !pBuffer[3] && !m_pPacket->IsLost())
    {
        IHXRTPPacket *pRtpPacket = NULL;
        m_pPacket->QueryInterface(IID_IHXRTPPacket, (void**)&pRtpPacket);

        if (pRtpPacket)
        {
            if (!m_pTsConvert)
            {
                m_pTsConvert = new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
							 1,
							 90);
		if (m_pTsConvert)
		{
		    m_pTsConvert->setHXAnchor(m_pPacket->GetTime());
		}
            }

            if (m_pTsConvert)
                m_dNextPts = m_pTsConvert->rtp2hxa(pRtpPacket->GetRTPTime());
            else
                m_dNextPts = pRtpPacket->GetRTPTime()/90.0;

            HX_RELEASE(pRtpPacket);
            
	    if(m_lStreamOffsetTime > m_dNextPts)
                m_dNextPts =  0;
            else
                m_dNextPts -= m_lStreamOffsetTime;
        }        
    }
    // If we lost packets, resync with the start of a frame
    else if (m_bPacketLoss)
    {
        return FALSE;
    }

    pBuffer += 4;
    ulSize -= 4;

    return TRUE;
}

void C2250PacketParser::HandlePacketLoss()
{
    // Render the last PCM buffer for the number of frames we lost
    if (m_pLastPCMBuffer)
    {
        double dTime = m_dLastPCMTime + m_dFrameTime;
        int nLostFrames = (int)((m_dNextPts - dTime) / m_dFrameTime + .5);

        for (int i=0; i<nLostFrames; i++)
        {
            m_pRenderer->Render(m_pLastPCMBuffer, dTime);
            dTime += m_dFrameTime;
        }
    }

    // Remove any partial audio frames in the decode buffer
    m_ulDecBufBytes = 0;
}

#ifdef DEMUXER
CSysPacketParser::CSysPacketParser(HXBOOL bMPEG2, IHXRegistry* pRegistry) : 
    CMpaPacketParser(),
    m_pRegistry(pRegistry),
    m_llLastPts(0),
    m_llFirstPts(0),
    m_ulPlayTime(0),
    m_bCheckVcdBug(TRUE),
    m_bVcdBug(FALSE)
{
    m_pDemuxer = new CDemuxer(bMPEG2);
}

HXBOOL
CSysPacketParser::Demux(UCHAR*& pBuffer, UINT32& ulSize, UINT32 ulFragSize)
{
    if(!m_pDemuxer)
    {
        return TRUE;
    }

     // Get the es data
    Packet packet;

    m_pDemuxer->Demux_ul(pBuffer, ulSize, &packet);

    pBuffer = packet.pData;
    ulSize = packet.lBytes;

    // Sync clock with the first stream time stamp
    if (m_dNextPts == 0.0)
    {
        if (!packet.cHasPts)
        {
            return FALSE;
        }

        m_llFirstPts = packet.llPts;
        m_llLastPts = packet.llPts;

        INT32 lTime = m_pPacket->GetTime();

        // Set the first timestamp in the registry (don't include the stream
        // offset in this value since the video renderer needs the offset
        // in its timestamps for proper a/v sync).
        INT32 nDelta = (INT32)(packet.llPts/UNITS_CONVERSION_MS) - lTime;

	// This fixes 108347, dont synchronize against a bad packet when starting playback
	if( nDelta < 0 && lTime == 0 )
	{
	    nDelta = 0;
	    packet.llPts = 0;
	    m_llFirstPts = m_llLastPts = 0;
	}

        if(m_pRegistry)
        {
	    m_pRegistry->SetIntByName("FirstPts", nDelta);
        }
        
        if(m_lStreamOffsetTime > lTime)
        {
            lTime =  0;
        }
        else
        {
            lTime -= m_lStreamOffsetTime;
        }
        
        m_dNextPts = lTime;

        #if defined _DEBUG && defined _WIN32
        char szTmp[256]; /* Flawfinder: ignore */
        SafeSprintf(szTmp, 256, "Audio Pts %ld\n", (UINT32)m_dNextPts);
        //OutputDebugString(szTmp);
        #endif

        m_ulPlayTime = lTime;
    }
    else if (packet.llPts)
    {
        // Make sure we inited our decoder before doing ts manipulation
        if (m_ulBitRate)
        {
            // Update the pts (stream ts - 1st stream ts + playtime)
            double dNewTs = (double)(packet.llPts - m_llFirstPts) / 
                            (double)UNITS_CONVERSION_MS + m_ulPlayTime;

            // Pts refer to the first frame in this packet, so if we have
            // a partial frame in our decode buffer, the new pts is not
            // for that frame.  Decrement the new pts by on frame time to
            // sync with the partial frame.
            if (ulFragSize)
            {
                // Ok, some vcds generate erroneous time stamps. The pts
                // refers to the first byte in the packet not instead of
                // the first frame.  Try and catch this bug here and adjust.
                
                if (m_bCheckVcdBug)
                {
                    m_bCheckVcdBug = FALSE;

                    if ((dNewTs - ulFragSize * 1000 / (m_ulBitRate>>3)) - 
                        m_dNextPts <= 1)
                    {
                        m_bVcdBug = TRUE;
                    }
                }
                
                if (m_bVcdBug)
                    dNewTs -= ulFragSize * 1000 / (m_ulBitRate>>3);
                else
                    dNewTs -= m_dFrameTime;
            }

            // Dynamic timestamp change
            // Look for timestamps that go backwards or are less than 1000.
            // An encoder is supposed to send timestamps at least every .7 sec
            INT32 lDelta = dNewTs - m_dNextPts;
            if (abs(lDelta) > 1000)
            {
                if(m_pRegistry)
                {
                    m_pRegistry->SetIntByName("FirstPts", 
                        (INT32)(packet.llPts/UNITS_CONVERSION_MS) - 
                        (INT32)m_dNextPts);
                }
                m_llFirstPts = packet.llPts - 
                                (INT64)m_dNextPts*UNITS_CONVERSION_MS + 
                                (INT64)m_ulPlayTime*UNITS_CONVERSION_MS;
            }
            else
                m_dNextPts = dNewTs;
        }
        
        m_llLastPts = packet.llPts;
    }

    return TRUE;
}
#endif // DEMUXER
