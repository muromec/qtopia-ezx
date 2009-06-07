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
#include "hlxosstr.h"
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
#include "tconverter.h"		// CHXTimestampConverter
#include "hxstrutl.h"

#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
#include "mp3format.h"          // MP3 formatter

#ifdef DEMUXER
#include "xmddemuxer.h"         // Demuxer
#include "xmdtypes.h"
#endif

#include "mp3rend.h"            // CRnMp3Ren
#include "pktparse.h"           // CPacketParser
#include "robpktparse.h"        // CRobustPacketParser


CRobustPacketParser::CRobustPacketParser() :
    CPacketParser(),    
    m_pTsConvert(NULL),
    m_ulIndex(0),
    m_bFirstFrame(TRUE)
//    m_dNextFrameTime(0.0),
//    m_dFrameTime(0.0)
{
    m_bReformatted = TRUE;
}

CRobustPacketParser::~CRobustPacketParser()
{
    m_Cycles.DeleteAll();
    HX_DELETE(m_pTsConvert);
}

HX_RESULT
CRobustPacketParser::AddPacket(IHXPacket* pPacket, INT32 streamOffsetTime)
{
    if(pPacket == NULL)
        return HXR_INVALID_PARAMETER;

    // Get the buffer.
    IHXBuffer* pPacketBuf = pPacket->GetBuffer();
    if(pPacketBuf == NULL || pPacketBuf->GetSize() == 0)
    {
        HX_RELEASE(pPacketBuf);
        return HXR_INVALID_PARAMETER;
    }
    double dTime;

    // Get the packet time
    IHXRTPPacket *pRtpPacket = NULL;
    pPacket->QueryInterface(IID_IHXRTPPacket, (void**)&pRtpPacket);

    if (pRtpPacket)
    {
	if (!m_pTsConvert)
	{
	    m_pTsConvert = new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
						     1,
						     90);
	    if (m_pTsConvert)
	    {
		m_pTsConvert->setHXAnchor(pPacket->GetTime());
	    }
	}
	if (m_pTsConvert)
	{
	    dTime = m_pTsConvert->rtp2hxa(pRtpPacket->GetRTPTime());
	}
	else
	{
	    dTime = pRtpPacket->GetRTPTime() / 90.0;
	}
        HX_RELEASE(pRtpPacket);
    }
    else
    {
        //dTime = pPacket->GetTime();
        dTime = pPacket->GetTime();
    }

    if(streamOffsetTime > dTime)
    {
        dTime =  0;
    }
    else
    {
        dTime -= streamOffsetTime;
    }

    // Set the time for the first frame in the packet
    UINT32 ulOffset = GetFrameInfo(pPacketBuf, 0, dTime);
    
    while(ulOffset < pPacketBuf->GetSize())
    {
        ulOffset = GetFrameInfo(pPacketBuf, ulOffset, -1);
    }

    return HXR_OK;
}

UINT32
CRobustPacketParser::GetFrame(UCHAR*& pFrameBuffer, double& dTime, HXBOOL& bLost)                              
{    
    pFrameBuffer = NULL;
    dTime = 0.0;
    bLost = FALSE;

    // If we don't have any frames yet
    if(m_Cycles.IsEmpty())
    {
        return 0;
    }

    SCycle* pCycle = NULL;
    HXBOOL bNextCycle = TRUE;
    SFrameInfo* pFrameInfo = NULL;
    UINT32 ulIndex = m_ulIndex;

    pCycle = (SCycle*)m_Cycles.GetHead();

    while(bNextCycle)
    {
        // This will never be NULL since we never add NULL
        HX_ASSERT(pCycle != NULL);  

        pFrameInfo = pCycle->pFrames[ulIndex];

        if(pFrameInfo && pFrameInfo->bComplete)
        {
            // we have a frame ready!            
            bNextCycle = FALSE;
        }
        else
        {
            // we haven't got to the next cycle yet, keep waiting
            if(m_Cycles.GetCount() <= 1 && !m_bEndOfPackets)
            {
                m_ulIndex = ulIndex;
                return 0;
            }

            // this frame is lost (we still have more this cycle)
            if(ulIndex < (UINT32)pCycle->nMaxIndex)
            {
                #if defined(WIN32) && defined(_DEBUG)
                char str[256]; /* Flawfinder: ignore */
                SafeSprintf(str, 256, "Known packet loss: Index: %d, Cycle: %d, "
                        "Frag: %c\n", ulIndex, pCycle->nCycleIndex, 
                        pFrameInfo ? 'y' : 'n');
                OutputDebugString(OS_STRING(str));
                #endif // defined(WIN32) && defined(_DEBUG)

                // If we have a partial frame, release it
                HX_DELETE(pCycle->pFrames[ulIndex]);

                if(m_bFirstFrame)
                {
                    #if defined(WIN32) && defined(_DEBUG)
                    OutputDebugString(OS_STRING("First frame missing. Ignoring.\n"));
                    #endif // defined(WIN32) && defined(_DEBUG)                    
   
                    // If this was the first frame, ignore it and keep looking                    
                    while(++ulIndex <= (UINT32)pCycle->nMaxIndex && bNextCycle)
                    {                        
                        pFrameInfo = pCycle->pFrames[ulIndex];
                        if(pFrameInfo && pFrameInfo->bComplete)
                        {
                            // we have a frame ready!                       
                            bNextCycle = FALSE;                            
                            break;
                        }
                        HX_DELETE(pCycle->pFrames[ulIndex]);
                    }                    
                }
                else
                {
                    m_ulIndex++;
                    bLost = TRUE;
                    bNextCycle = FALSE;
                }
            }

            if(bNextCycle)
            {
                // We're done with this cycle, so get rid of it and 
                // go to the next    
                m_Cycles.RemoveHead();
                HX_DELETE(pCycle);

                ulIndex = 0;                
                if(m_Cycles.GetCount() > 0)
                {
                    pCycle = (SCycle*)(m_Cycles.GetHead());
                }
                else if (m_bEndOfPackets)
                {
                    m_ulIndex = 0;
                    return 0;
                }                
            }
        }
    }
    
    // Try to get the frame presentation time
    double dPresTime;

    // If it's lost, go with the default time
    if(bLost)
    {
        dPresTime = m_dNextPts;
    }
    // Else if this frame has a time stamp, use it
    else if(pFrameInfo->dTime >= 0)
    {
        dPresTime = pFrameInfo->dTime;
    }
    // Else if we have calculated the start time of this cycle
    // (based on another frame with a timestamp) calculate this one
    else if(pCycle->dStartTime >= 0)
    {
        dPresTime = pCycle->dStartTime + m_dFrameTime*ulIndex;
    }
    // Else we have no time stamps for any frames in this cycle yet.
    // Check if we have later cycles with timestamps
    else
    {
        dPresTime = -1.0;
        SCycle* pTmpCycle = pCycle;        
        while((pTmpCycle = m_Cycles.GetNext(pTmpCycle)) != 0)
        {
            if(pTmpCycle->dStartTime >= 0)
            {
                // uh-oh, we got a full cycle without any time stamps!
                // either massive packet loss or a really crappy
                // interleaving algorithm. assume it's supposed to go now
                // and any unaccounted for loss is at the end of the cycle
                dPresTime = m_dNextPts;
                pCycle->dStartTime = dPresTime - m_dFrameTime*ulIndex;
            }
        }
    }

    // If we haven't gotten any timestamps yet this cycle, wait for
    // the next packet.
    if(dPresTime < 0)
    {
        if(!m_bEndOfPackets)
        {
            m_ulIndex = ulIndex;
            return 0;
        }
        
        // We don't have a time stamp, but there are no more packets coming
        dPresTime = m_dNextPts;
    }

    // If this is the first frame, set the time accordingly
    if(m_bFirstFrame)
    {
#if defined(WIN32) && defined(_DEBUG)
        char str[256]; /* Flawfinder: ignore */
        SafeSprintf(str, 256, "First decode, setting time to %lf\n", dPresTime);
        OutputDebugString(OS_STRING(str));
#endif // defined(WIN32) && defined(_DEBUG)
        m_dNextPts = dPresTime;
    }

    if(!m_bFirstFrame && dPresTime - m_dNextPts >= m_dFrameTime)
    {
        // We have frames lost before this one
#if defined(WIN32) && defined(_DEBUG)
        char str[256]; /* Flawfinder: ignore */
        SafeSprintf(str, 256, "Apparent packet loss: Index: %d, Cycle: %d, "
                "PresTime: %lf, NextPts: %lf, FrameTime: %lf\n", 
                ulIndex, pCycle->nCycleIndex, dPresTime, m_dNextPts, 
                m_dFrameTime);
        OutputDebugString(OS_STRING(str));
#endif // defined(WIN32) && defined(_DEBUG)
        bLost = TRUE;
    }

    if(pFrameInfo && pFrameInfo->ulADUSize < 2)
    {
        // this frame is too small. skip it and do loss recovery
        HX_DELETE(pCycle->pFrames[ulIndex]);
        m_ulIndex = ulIndex + 1;
        bLost = TRUE;
    }

    // If the frame was lost, we reuse the one already in the buffer,
    // so don't need to copy or anything
    if(!bLost)
    {
        IHXBuffer* pPacketBuf;
        SFrameData* pFrameData;
        UINT32 ulSize;        
        UCHAR* pBuf;
        UCHAR* pDecBuf = m_pDecBuffer;
        m_ulDecBufBytes = 0;

        // Copy the frame into the decode buffer
        for(pFrameData= pFrameInfo->pFrameData; pFrameData != NULL;
            pFrameData = pFrameData->pNext)
        {
            pPacketBuf = pFrameData->pPacketBuf;
            if(pPacketBuf)
            {
                pPacketBuf->Get(pBuf, ulSize);
                if(pBuf && ulSize >= pFrameData->ulOffset + 2)
                {
                    // seek to the frame offset
                    ulSize -= pFrameData->ulOffset;
                    pBuf += pFrameData->ulOffset;
            
                    if(pFrameInfo->ulADUSize < ulSize)
                    {
                        ulSize = pFrameInfo->ulADUSize;
                    }
                    // Make sure it will actually fit
                    if(ulSize > DEC_BUFFER_SIZE - m_ulDecBufBytes)
                    {
                        ulSize = DEC_BUFFER_SIZE - m_ulDecBufBytes;
                    }
            
                    memcpy(pDecBuf, pBuf, ulSize); /* Flawfinder: ignore */
                    m_ulDecBufBytes += ulSize;
                    pDecBuf += ulSize;
                }
            }
        }

        HX_DELETE(pCycle->pFrames[ulIndex]);
        m_ulIndex = ulIndex + 1;
    
        // Clear the main_data_begin (since we're reformatted)
        m_pFmt->ClearMainDataBegin(m_pDecBuffer);
    }

    pFrameBuffer = m_pDecBuffer;
    dTime = dPresTime;

    m_dNextPts = dPresTime + m_dFrameTime;
    m_bFirstFrame = FALSE;   

    return m_ulDecBufBytes;
}

UINT32 CRobustPacketParser::GetFrameInfo(IHXBuffer* pPacket, UINT32 ulOffset,
                                         double dTime)
{   
    if(pPacket->GetSize() <= ulOffset)
    {
        return pPacket->GetSize();
    }

    UCHAR*  pBuf = pPacket->GetBuffer() + ulOffset;

    // bit 0: continuation bit
    HXBOOL    bCont = (pBuf[0] & 0x80) == 0 ? FALSE : TRUE; 
    // bit 1: Descriptor type flag
    int nDescSize = (pBuf[0] & 0x40) == 0 ? 1 : 2; 
    
    if(pPacket->GetSize() - ulOffset < (UINT32)nDescSize + 2)
    {
        return pPacket->GetSize();
    }

    UINT32 ulADUSize;
    int nIndex;
    int nCycle;

    // in a one byte descriptor, bits 2-7 are the ADU size
    // in a two byte descriptor, bits 2-15 are the size
    ulADUSize = nDescSize == 1 ? (pBuf[0] & 0x3F) :
                ((UINT32)(pBuf[0] & 0x3F) << 8) + pBuf[1];
    
    pBuf += nDescSize;    
    
    // MPEG audio sync word is the first 12 bits, first 11 always set.
    // if the sync word is in tact, we are not reordered
    if(pBuf[0] == 0xFF && (pBuf[1] & 0xE0) == 0xE0)
    {
        nIndex = 0;
        nCycle = -1;
    }
    // otherwise, the first 11 bits of the sync word are re-used to 
    // identify the frame order    
    else
    {
        // bits 0-7: frame index
        nIndex = pBuf[0];
        // bits 8-10: cycle
        nCycle = (pBuf[1] & 0xE0) >> 5;
    }
    
    SCycle* pCycle = m_Cycles.IsEmpty() ? NULL : (SCycle*)m_Cycles.GetTail();
    if(!pCycle || nCycle != pCycle->nCycleIndex || nCycle < 0)
    {
        // This is a new cycle
        pCycle = new SCycle(nCycle);
        if(pCycle == NULL)
        {
            return pPacket->GetSize();
        }
        pCycle->nMaxIndex = nIndex;
        m_Cycles.AddTail(pCycle);                   
    }
    else if(nIndex > pCycle->nMaxIndex)
    {
        pCycle->nMaxIndex = nIndex;
    }

    SFrameData* pFrameData = new SFrameData();
    SFrameData* pIterator;

    pPacket->AddRef();
    pFrameData->pPacketBuf = pPacket;
    pFrameData->ulOffset = ulOffset + nDescSize;
    pFrameData->bCont = bCont;
    
    SFrameInfo* pInfo = pCycle->pFrames[nIndex];
    if(pInfo != NULL)
    {
        // We have a frag. Note that the spec only accounts for a frame being 
        // split accross two packets when reordered. If one is split across 
        // more than two packets, we will assume the cont frags are in order
        if(bCont || (pCycle->pFrames[nIndex]->pFrameData && 
            !pCycle->pFrames[nIndex]->pFrameData->bCont))
        {
            pIterator = pCycle->pFrames[nIndex]->pFrameData;
            if(pIterator)
            {
                while(pIterator->pNext != NULL)
                    pIterator = pIterator->pNext;

                pFrameData->pNext = NULL;
                pIterator->pNext = pFrameData;
            }
            else
            {
                pFrameData->pNext = pCycle->pFrames[nIndex]->pFrameData;
                pCycle->pFrames[nIndex]->pFrameData = pFrameData;
            }
        }
        // If this is not a continuation frame, it is the first frag
        else
        {
            pFrameData->pNext = pCycle->pFrames[nIndex]->pFrameData;
            pCycle->pFrames[nIndex]->pFrameData = pFrameData;
        }    
    }
    else
    {
        pFrameData->pNext = NULL;
        pInfo = new SFrameInfo();
        pInfo->nIndex = nIndex;
        pInfo->ulADUSize = ulADUSize;
        pInfo->pFrameData = pFrameData;
        pInfo->dTime = dTime;
        pCycle->pFrames[nIndex] = pInfo;
    }

    UINT32 ulSize = 0;
    for(pIterator = pCycle->pFrames[nIndex]->pFrameData; pIterator!= NULL;
        pIterator = pIterator->pNext)
    {        
        HX_ASSERT(pIterator->pPacketBuf);
        ulSize += pIterator->pPacketBuf->GetSize() - pIterator->ulOffset;
    }
    
    pInfo->bComplete = ulSize >= pInfo->ulADUSize ? TRUE : FALSE;

    // Set first 11 bits
    pBuf[0]  = 0xFF;
    pBuf[1] |= 0xE0;

    // Init our decoder if needed
    if(m_pDecoder || InitDecoder(pBuf, pPacket->GetSize() - ulOffset, FALSE))
    {
        // set the first frame's presentation time
        if(dTime >= 0)
        {
            if(pCycle->dStartTime < 0)
            {
                pCycle->dStartTime = dTime - m_dFrameTime*(nIndex);    
            }
        }   
    }    

    return HX_MIN(pFrameData->ulOffset + ulADUSize, pPacket->GetSize());
}

HX_RESULT
CRobustPacketParser::RenderAll()
{
    if(!m_pRenderer || !m_pFmt)
    {
        return HXR_FAIL;
    }

    UCHAR* pFrameBuf;
    UINT32 ulSize;
    HXBOOL bPacketLoss;
    double dTime;

    for(;;)
    {
        ulSize = GetFrame(pFrameBuf, dTime, bPacketLoss);
        if(!pFrameBuf || !ulSize)
        {
            return HXR_OK;
        }

        if(!m_pDecoder && !InitDecoder(pFrameBuf, ulSize, TRUE))
        {
            return HXR_FAIL;
        }

        DecodeAndRender(pFrameBuf, ulSize, dTime, bPacketLoss);
    }
}

void
CRobustPacketParser::PreSeek()
{ 
    m_Cycles.DeleteAll();
    m_ulIndex = 0;
    m_bFirstFrame = TRUE;
    CPacketParser::PreSeek();
}

void
CRobustPacketParser::PostSeek(UINT32 time)
{ 
    m_Cycles.DeleteAll();
    m_ulIndex = 0;
    m_bFirstFrame = TRUE;
    CPacketParser::PostSeek(time);
    HX_DELETE(m_pTsConvert);
}

void
CRobustPacketParser::EndOfPackets()
{
    m_bEndOfPackets = TRUE;

    // Decode and render any frames left queued.
    RenderAll();
}
