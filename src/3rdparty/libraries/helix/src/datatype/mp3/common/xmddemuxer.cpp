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
#include "hlxclib/string.h"
#include "types64.h"
#include "xmdtypes.h"
#include "xmddemuxer.h"
#include "hxassert.h"

CDemuxer::CDemuxer(HXBOOL bMPEG2)
 :  m_bMPEG2(bMPEG2),
    m_pPack(NULL),
    m_llPtsDelta(0)
{
}

CDemuxer::~CDemuxer()
{
}


///////////////////////////////////////////////////////////////////////////////
// Function:    Demux_ul
// Returns:     The number of bytes processed
///////////////////////////////////////////////////////////////////////////////
UINT32 CDemuxer::Demux_ul(UINT8 *pBuffer,
                             UINT32 ulBytes,
                             Packet *pPacket)
{
    UINT8       *pTemp = (UINT8*)pBuffer;
    UINT32      ulTemp = ulBytes;
    INT32       lCode = 0;

    memset(pPacket, 0, sizeof(Packet));

    UINT8 bCont = TRUE;

    while (ulTemp >= 4 && bCont)
    {
        lCode = GetStartCode(&pTemp, ulTemp);

        switch (lCode)
        {
            case -1:
                bCont = FALSE;
                break;

            case PACK_HEADER:
            {
	            m_pPack = pTemp;

                pTemp+=4;
	            ulTemp-=4;

                // Is this MPEG1 or MPEG2
                UINT8 yNext = *pTemp & 0xF0;

                // Check for MPEG1 or MPEG2.
                // In MPEG2 files, '0100' follows pack_start_code and
                // in MPEG1 files, '0010' follows pack_start_code.
                if ((yNext & 0xC0) == 0x40)
                    m_bMPEG2 = TRUE;
                else
                    m_bMPEG2 = FALSE;

                break;
            }

            default:
                // PRIVATE_STREAM_1
                // VIDEO_PACKET
                // AUDIO_PACKET
                if ( PRIVATE_STREAM_1 == lCode ||
                    (lCode & 0x000000F0) == 0xE0 ||
                    (lCode & 0x000000E0) == 0xC0)
                {
                    if (ProcessPacket(&pTemp, ulTemp, pPacket))
                    {
                        bCont = FALSE;
                    }
                }
                else
                {
                    pTemp+=4;
	                ulTemp-=4;
                }

                break;
        }
	}


    return (UINT32)(PTR_INT)(pTemp - pBuffer);
}


INT32 CDemuxer::GetStartCode(UINT8 **ppBuffer,
                                UINT32 &ulBytes)
{
    if ((long)ulBytes < 4)
        return -1;

    UINT8 *pStart = *ppBuffer + 2,
                  *pEnd = *ppBuffer + ulBytes - 1;

    INT32 ulCode = -1;
    
    while (pStart < pEnd)
    {
        // Look for a 1
        pStart = (UINT8*)memchr(pStart, 1, pEnd-pStart);

        if (!pStart)
            return -1;

        // If the previous 2 bytes are 0's assume it is a start code
        if (pStart[-1] || pStart[-2])
            ++pStart;
        else
        {
            ulCode = 0x00000100 + pStart[1];
            pStart -= 2;

            break;
            
        }
    }

    ulBytes -= pStart - *ppBuffer;
    *ppBuffer = pStart;

    return ulCode;
}

UINT8 CDemuxer::ProcessPacket(UINT8 **ppBuffer,
                                   UINT32 &ulBytes,
                                   Packet *pPacket)
{
    if (!m_bMPEG2)
        return ProcessMPEG1Packet(ppBuffer, ulBytes, pPacket);

    pPacket->pHeader = *ppBuffer;

    if (ulBytes < 6)
        return 1;

    // Move beyond the packet start code
    *ppBuffer += 3;
    UINT8 yStream = **ppBuffer;
    *ppBuffer += 1;

  	// Extract the packet length (including the PES header)
  	int nLength = **ppBuffer << 8;
    *ppBuffer += 1;
    nLength += **ppBuffer;
	
    if ((UINT32)nLength > ulBytes)
        return 1;
    
    pPacket->lHeaderBytes = nLength + 6;

    // Move beyond the packet length field to the PES header
	*ppBuffer += 1;
	ulBytes -= 6;

	// PES Header
	if ((**ppBuffer & 0xC0) == 0x80)
	{
        // Check for an encryped pack
        if (**ppBuffer & 0x30)
            HX_ASSERT(FALSE);
		        
        // Check for pts and/or dts
        UINT8 yPtsDtsFlags = *(*ppBuffer+1) & 0xC0;

        // Extract the length of the PES extra header
		UINT8 cSize = *(*ppBuffer+2);

		// Move beyond 3 byte header
        *ppBuffer += 3;
		nLength -= 3;
		ulBytes -= 3;

        // Extract pts/dts
        if ( ((yPtsDtsFlags & 0xC0) == 0x80) || ((yPtsDtsFlags & 0xC0) == 0xC0) ) 
        { 
            // "0010"                    4 bits
            // presentation_time_stamp   3 bits
            // marker_bit                1 bit 
            // presentation_time_stamp  15 bits  
            // marker_bit                1 bit 
            // presentation_time_stamp  15 bits  
            // marker_bit                1 bit 

            pPacket->cHasPts = 1;
            pPacket->llPts = GetTimeStamp(*ppBuffer) + m_llPtsDelta;
                        
            *ppBuffer +=5;
		    nLength -= 5;
            ulBytes -=5;
            cSize -= 5;

            // Extract dts
            if ( (yPtsDtsFlags & 0xC0) == 0xC0 ) 
            { 
                // " 0001 "                  4 bits
                // decode_time_stamp         3 bits 
                // marker_bit                1 bit 
                // decoding_time_stamp      15 bits  
                // marker_bit                1 bit 
                // decoding_time_stamp      15 bits  
                // marker_bit                1 bit 
            
                pPacket->cHasDts = 1;
                pPacket->llDts = GetTimeStamp(*ppBuffer) + m_llPtsDelta;
                
                *ppBuffer += 5;
		        nLength -= 5;
                ulBytes -=5;
                cSize -= 5;
            }
        }

        // Move beyond the remaining PES header
		if (cSize)
        {
            *ppBuffer += cSize;
		    nLength -= cSize;
		    ulBytes -= cSize;
        }

		// PrivateStream 1 (AC3/LPCM/DTS/Subpicture)
        if (yStream == 0xbd)
        {
            // Check for AC3 data
		    if ((**ppBuffer & 0xF8) == 0x80)
		    {
                // Mark the stream id
                pPacket->cStreamId = **ppBuffer & 0x07;

		        // Store private data
                pPacket->pPrivateData = *ppBuffer;
                pPacket->lPrivBytes = 4;

		        // Move beyond 4 byte header
                *ppBuffer += 4;
		        nLength -= 4;
		        ulBytes -= 4;
		    
		        // Fill pPacket
		        pPacket->pData = *ppBuffer;
                pPacket->lBytes = nLength;
                pPacket->ePacket = eAudio;
                pPacket->eSubtype = eAC3;

                return 1;
		    }
		    // LPCM
            else if ((**ppBuffer & 0xF8) == 0xA0)
		    {
                // Mark the stream id
                pPacket->cStreamId = **ppBuffer & 0x07;                        

		        // Store private data
                pPacket->pPrivateData = *ppBuffer;
                pPacket->lPrivBytes = 7;

                // Move beyond  byte header
                *ppBuffer += 7;
		        nLength -= 7;
		        ulBytes -= 7;
		    
		        // Fill pPacket
		        pPacket->pData = *ppBuffer;
                pPacket->lBytes = nLength;
                pPacket->ePacket = eAudio;
                pPacket->eSubtype = eLPCM;

                return 1;
		    }
            // DTS
            else if ((**ppBuffer & 0xF8) == 0x88)
		    {
                // Mark the stream id
                pPacket->cStreamId = **ppBuffer & 0x07;                        

		        // Move beyond 4 byte header
                *ppBuffer += 4;
		        nLength -= 4;
		        ulBytes -= 4;
		    
		        // Fill pPacket
		        pPacket->pData = *ppBuffer;
                pPacket->lBytes = nLength;
                pPacket->ePacket = eAudio;
                pPacket->eSubtype = eDTS;

                return 1;
		    }
            // Subpicture
            else if ((**ppBuffer & 0xE0) == 0x20)
            {
                // Mark the stream id
                pPacket->cStreamId = **ppBuffer & 0x1F;                        

		        // Move beyond id header
                *ppBuffer += 1;
		        nLength -= 1;
		        ulBytes -= 1;
		    
		        // Fill pPacket
		        pPacket->pData = *ppBuffer;
                pPacket->lBytes = nLength;
                pPacket->ePacket = eSubpicture;

                return 1;
            }
        }
        // Video
        else if ((yStream & 0xF0) == 0xe0)
        {
            // Mark the stream id
            pPacket->cStreamId = yStream & 0x0F;

		    // Fill pPacket
		    pPacket->pData = *ppBuffer;
            pPacket->lBytes = nLength;
            pPacket->ePacket = eVideo;
            pPacket->eSubtype = eMPEG2;

            return 1;
        }
        // Audio
        else if ((yStream & 0xE0) == 0xc0)
        {
            // Mark the stream id
            pPacket->cStreamId = yStream & 0x1F;

		    // Fill pPacket
		    pPacket->pData = *ppBuffer;
            pPacket->lBytes = nLength;
            pPacket->ePacket = eAudio;
            pPacket->eSubtype = eMPEG1;

            return 1;
        }
	}

	// Skip the packet
    *ppBuffer += nLength;
    ulBytes -= nLength;

    return 0;
}

UINT8 CDemuxer::ProcessMPEG1Packet(UINT8 **ppBuffer,
                                      UINT32 &ulBytes,
                                      Packet *pPacket)
{
    UINT8 *pHeader = *ppBuffer;

    if (ulBytes < 6)
        return 1;
    
    // Move beyond the packet start code
    *ppBuffer += 3;
    UINT8 yStream = **ppBuffer;
    *ppBuffer += 1;

  	// Extract the packet length (including the PES header)
  	int nLength = **ppBuffer << 8;
    *ppBuffer += 1;
    nLength += **ppBuffer;
	
    INT32   lHeaderBytes = nLength + 6;

    // Move beyond the packet length to data
	*ppBuffer += 1;
	ulBytes -= 6;

    if ((UINT32)nLength > ulBytes)
        return 1;

    pPacket->pHeader = pHeader;
    pPacket->lHeaderBytes = lHeaderBytes;

    // Remove stuffing bytes
    while (**ppBuffer == 0xFF)
	{			
        if (!ulBytes)
            return 1;

        *ppBuffer += 1;
        --ulBytes;
        --nLength;
	}

    if ( (**ppBuffer & 0xC0) == 0x40 )
    {
        // "01"                    2 bits
        // STD_buffer_scale        1 bit
        // STD_buffer_size        13 bits
        
        if (ulBytes < 2)
            return 1;
		
        *ppBuffer += 2;
		ulBytes -= 2;
        nLength -= 2;
    }

    // Extract PTS and DTS
    UINT8 bNextByte = **ppBuffer;
    if ( ((bNextByte & 0xF0) == 0x20) || ((bNextByte & 0xF0) == 0x30) ) 
    {
        if (ulBytes < 5)
            return 1;
        
        pPacket->cHasPts = 1;
        pPacket->llPts = GetTimeStamp(*ppBuffer) + m_llPtsDelta;
        
        *ppBuffer +=5;
		nLength -= 5;
        ulBytes -=5;

        // DTS
        if ( (bNextByte & 0xF0) == 0x30 )
        {
            if (ulBytes < 5)
                return 1;

            pPacket->cHasDts = 1;
            pPacket->llDts = GetTimeStamp(*ppBuffer) + m_llPtsDelta;

            *ppBuffer += 5;
		    nLength -= 5;
            ulBytes -=5;
        }
    }
    else 
    {
        if (!ulBytes)
            return 1;

        // "0000 1111"
        *ppBuffer += 1;
        --ulBytes;
        --nLength;
    }
    
    // Extract video and audio data
    if ((yStream & 0xF0) == 0xe0)
    {
        // Mark the stream id
        pPacket->cStreamId = yStream & 0x0F;

		// Fill pPacket
		pPacket->pData = *ppBuffer;
        pPacket->lBytes = nLength;
        pPacket->ePacket = eVideo;
        pPacket->eSubtype = eMPEG1;

        return 1;
    }
    // Audio
    else if ((yStream & 0xE0) == 0xc0)
    {
        // Mark the stream id
        pPacket->cStreamId = yStream & 0x1F;

		// Fill pPacket
		pPacket->pData = *ppBuffer;
        pPacket->lBytes = nLength;
        pPacket->ePacket = eAudio;
        pPacket->eSubtype = eMPEG1;

        return 1;
    }

    return 0;
}

INT64 CDemuxer::GetTimeStamp(UINT8 *pBuffer)
{
    INT64 llPts = 0;

    llPts  = (*pBuffer & 0x0E) << 29;
    llPts |=  *(pBuffer+1)  << 22;
    llPts |= (*(pBuffer+2) & 0xFE) << 14;
    llPts |=  *(pBuffer+3)  << 7;
    llPts |= (*(pBuffer+4) & 0xFE) >> 1;

    return (INT64)(llPts / 90.0 * UNITS_CONVERSION_MS);
}

void CDemuxer::Reset_v()
{
    m_llPtsDelta = 0;

    m_pPack = NULL;
}
