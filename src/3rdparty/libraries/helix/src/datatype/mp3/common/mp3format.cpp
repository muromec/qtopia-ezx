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

#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "mp3format.h"
#include "mp3queue.h"
#include "mhead.h"

CMp3Format::CMp3Format()
 :  CAudioInfoBase(),
    m_pMisc(new CMp3Misc),
    m_nLayer(3),
    m_bMpeg25(0),
    m_bTrustPackets(FALSE),
    m_pFnGetData(GetDataOffsetMPEG1)
{
    if (m_pMisc)
        m_pMisc->SetParent(this);
}

CMp3Format::CMp3Format(CMp3Misc* pMisc)
 :  CAudioInfoBase(),
    m_pMisc(pMisc),
    m_nLayer(3),
    m_bMpeg25(0),
    m_pFnGetData(GetDataOffsetMPEG1)
{
    if (m_pMisc)
        m_pMisc->SetParent(this);
}

CMp3Format::~CMp3Format()
{
    HX_DELETE(m_pMisc);
}

HXBOOL CMp3Format::Init(UINT8 *pHeader,
                      UINT32 ulSize)
{
    MPEG_HEAD h;
    memset(&h, 0, sizeof(h));

    if (!head_info(pHeader, ulSize, &h, m_bTrustPackets))
        return 0;

    // Detect MPEG2.5
    if ((pHeader[1] & 0xF0) == 0xE0)
        m_bMpeg25 = 1;

    // Detect MPEG1/MPEG2
    if(h.id)
        m_pFnGetData = GetDataOffsetMPEG1;
    else
        m_pFnGetData = GetDataOffsetMPEG2;

    m_ySyncWord = pHeader[1];

    UINT32   ulTemp;
    int      nTemp;
    int      nPadding = 0;
    GetEncodeInfo(pHeader,
                  ulSize,
                  ulTemp,
                  ulTemp,
                  nTemp,
                  m_nLayer,
                  nTemp,
                  nPadding);

    m_bIsInited = 1;

    return 1;
}

HXBOOL CMp3Format::GetDataOffset(UINT8 *pHeader,
                               UINT32 dwSize,
                               int &nFrameSize,
                               int &nHeaderSize,
                               int &nDataOffset)
{
    return m_pFnGetData(pHeader, dwSize, nFrameSize, nHeaderSize, nDataOffset, m_bTrustPackets);
}

void CMp3Format::ClearMainDataBegin(UINT8 *pHeader)
{
    if (m_nLayer != 3)
        return;

    int nOffset = 4;

    // Check protection bit
    if ((pHeader[1] & 1) == 0)
        nOffset += 2;

    // MPEG1 main_data_begin is 9 bits in MPEG2 it is 8
    pHeader[nOffset] = 0;

    if (GetDataOffsetMPEG1 == m_pFnGetData)
        pHeader[nOffset + 1] &= 0x7F;
}

HXBOOL CMp3Format::GetEncodeInfo(UINT8 *pHeader,
                               UINT32 dwSize,
                               UINT32 &ulBitRate,
                               UINT32 &ulSampleRate,
                               int &nChannels,
                               int &nLayer,
                               int &nSamplesPerFrame,
                               int &nPadding)
{
    MPEG_HEAD h;
    INT32 aSampRate[2][3] =         // MPEG SPEC x NATIVE RATE
    {
	{ 22050,24000,16000 },     // MPEG2
        { 44100,48000,32000 }      // MPEG1
    };

    memset(&h, 0, sizeof(h));

    int nBitRate = 0;
	UINT32 ulTempSampleRate = 0;
	UINT32 ulTempChannels = 0;
	UINT32 ulTempSamplesPerFrame = 0;
	
	if (!head_info2(pHeader, dwSize, &h, &nBitRate, m_bTrustPackets))
		return 0;
    
    if (3 == h.mode)
        ulTempChannels = 1;
    else
        ulTempChannels = 2;

    ulTempSampleRate = aSampRate[h.id][h.sr_index] >> m_bMpeg25;
	if(ulTempSampleRate <= 0)
		return 0;

    // Get the layer so we can work out the bit rate
    switch (h.option)
    {
        case 3:
            nLayer = 1;
            break;

        case 2:
            nLayer = 2;
            break;

        case 1:
            nLayer = 3;
    }

    if((h.option == 1) & (h.id == 0))  // MPEGII Layer III
    {
        ulTempSamplesPerFrame = 576;
        m_pFnGetData = GetDataOffsetMPEG2;
    }
    else if(h.option == 3)
        ulTempSamplesPerFrame = 384;     // Layer I
    else
        ulTempSamplesPerFrame = 1152;

    nPadding = h.pad;

	ulBitRate = nBitRate;
	ulSampleRate = ulTempSampleRate;
	nChannels = ulTempChannels;
	nSamplesPerFrame = ulTempSamplesPerFrame;

    return 1;
}

int CMp3Format::ReformatMP3Frame(UINT8 **ppFrame,
                                 UINT32 dwBytes,
                                 UINT32 dwPrevBytes)
{
    // Get the sync info
    int nFrameSize = 0,
        nHeaderSize = 0,
        nDataOffset = 0;

    UINT8   bAcceptFrame = 1;
    UINT8   *pFrame = *ppFrame;

    if (!GetDataOffset(pFrame,
                       dwBytes,
                       nFrameSize,
                       nHeaderSize,
                       nDataOffset))
        return 0;

    if (m_nLayer != 3)
        return nFrameSize;

    // nDataOffset points to data before our buffer.
    // We must reformat what we can for the next frame.
    if ((UINT32)nDataOffset > dwPrevBytes)
    {
        nDataOffset = dwPrevBytes;
        bAcceptFrame = 0;
    }

    // Calculate the modified frame size
    UINT8 *pNextFrame = pFrame + nFrameSize;
    int nDataOffset2 = 0,
        nTemp = 0;

    // Last frame has just enough data
    if ((int)dwBytes > nFrameSize)
    {
        if (!GetDataOffset(pNextFrame,
                           dwBytes - nFrameSize,
                           nTemp,
                           nTemp,
                           nDataOffset2))
            return 0;
    }

    // If this is a self contained frame, do nothing special
    if (!nDataOffset && !nDataOffset2)
        return nFrameSize;

    // Clear main_data_begin offset of current sync
    nTemp = 4;

    // Check protection bit
    if ((pFrame[1] & 1) == 0)
        nTemp += 2;

    //pFrame[nTemp] = 0;
    //
    // MPEG1 main_data_begin is 9 bits in MPEG2 it is 8
    //if (GetDataOffsetMPEG1 == m_pFnGetData)
    //    pFrame[nTemp + 1] &= 0x7F;

    // Store the header of current sync
    UINT8 *pHeader = new UINT8[nHeaderSize];

    memcpy(pHeader, pFrame, nHeaderSize); /* Flawfinder: ignore */

    // Move offset data forward
    memmove(pFrame - nDataOffset + nHeaderSize,
            pFrame - nDataOffset,
            nDataOffset);

    // Copy header data before offset data
    memcpy(pFrame - nDataOffset, /* Flawfinder: ignore */
           pHeader,
           nHeaderSize);

    delete [] pHeader;

    *ppFrame -= nDataOffset;

    // Calculate the new frame size
    nFrameSize = (pNextFrame - nDataOffset2) -
                 *ppFrame;
    if ( nFrameSize < 0)
    {
        return 0;
    }
    return nFrameSize * bAcceptFrame;

}


int CMp3Format::CheckValidFrame(UINT8 *pBuf,
                                UINT32 dwSize)
{
    // Are there enough bytes
    if (dwSize < 4)
        return 0;

    // Is this a sync word
    if ((pBuf[0] != 0xFF) |
        !IsValidSyncWord(pBuf[1]))
        return 0;

    MPEG_HEAD       head;
    memset(&head, 0, sizeof(head));

    int nRet = head_info(pBuf, dwSize, &head, m_bTrustPackets);

    if (nRet)
    {
        nRet += head.pad;

        if (nRet > (int)dwSize)
            nRet = 0;
    }

    return nRet;
}

int CMp3Format::UnformatMP3Frame(CMp3Queue* pQueue,
                                 UINT8* pDest,
                                 UINT32 &ulTime)
{
    tFrameInfo* pInfo = pQueue->GetHead();
    ulTime = pInfo->ulTime;

    // Do we have enough bytes for the frame
    if (pInfo->ulFrameSize - pInfo->ulHdrSize <= pQueue->GetDataBytes())
    {
        int nFrameSize = pInfo->ulFrameSize,
            nRet = pInfo->ulFrameSize;

        memcpy(pDest, pInfo->pHdr, pInfo->ulHdrSize); /* Flawfinder: ignore */
        pDest += pInfo->ulHdrSize;

        memcpy(pDest, pInfo->pData, pInfo->ulDataSize); /* Flawfinder: ignore */
        nFrameSize -= pInfo->ulDataSize + pInfo->ulHdrSize;

        pQueue->RemoveDataBytes(pInfo->ulDataSize);

        pInfo->pData += pInfo->ulDataSize;
        pDest += pInfo->ulDataSize;
        pInfo->ulDataSize = 0;

        int nCopy;

        // Copy data from future frames until we comple this frame
        while (nFrameSize)
        {
            pInfo = pQueue->Next();

            nCopy = HX_MIN(pInfo->ulDataSize, (UINT32)nFrameSize);
            memcpy(pDest, pInfo->pData, nCopy); /* Flawfinder: ignore */

            pDest += nCopy;
            pInfo->pData += nCopy;

            pInfo->ulDataSize -= nCopy;
            nFrameSize -= nCopy;

            pQueue->RemoveDataBytes(nCopy);
        }

        pQueue->RemoveHead();
        return nRet;
    }
    else
        return 0;
}

INT32 CMp3Format::ScanForSyncWord(UINT8 *pBuf,
                                  INT32 lSize,
                                  int &nFrameSize)
{
    UINT8       *pStart = pBuf,
                *pEnd = pBuf + lSize - 4,
                *pTemp = NULL,
                ySyncCheck = 0;
    MPEG_HEAD   head;
    int         iFrameSize = 0;

    if (lSize < 4)
        return -1;

    nFrameSize = 0;

    for (; pBuf < pEnd; pBuf++)
    {
        if (pBuf[0] != 0xFF)
            continue;

        if (m_ySyncWord)
        {
            if (!IsValidSyncWord(pBuf[1]))
                continue;
        }
        else if ((pBuf[1] & 0xE0) != 0xE0)
            continue;

        ySyncCheck = pBuf[1];

        memset(&head, 0, sizeof(head));
        iFrameSize = head_info(pBuf, pEnd-pBuf, &head, m_bTrustPackets);

        // Looks like a valid frame
        if (iFrameSize)
        {
            iFrameSize += head.pad;

            // Check for another frame
            if (pBuf+iFrameSize < pEnd+3)
            {
                pTemp = pBuf + iFrameSize;

                if((pTemp[0] == 0xFF) && (pTemp[1] & 0xE0 == 0xE0))
                {
                    nFrameSize = iFrameSize;
                }
                // We do not have consequitve frames..what to do, what to do?
                else if ((pTemp[0] != 0xFF) |
                    (pTemp[1] != ySyncCheck))
                {
                    INT32 lHeaderSize = 0;

                    // First, check for know non-mp3 data that may
                    // reside in an mp3 stream and terminate a frame.
                    HXBOOL bFoundHdr = FALSE;

                    if (m_pMisc)
                        bFoundHdr = m_pMisc->CheckForHeaders(pTemp,
                                                             pEnd-pTemp+4,
                                                             lHeaderSize);
                    if (bFoundHdr)
                        nFrameSize = iFrameSize;

                    // Next, check if the padding bit was incorrectly set
                    else if ((pTemp[-1] == 0xFF) &
                             (pTemp[0] == ySyncCheck))
                        nFrameSize = iFrameSize;

                    // Finally, check if the padding bit was not set and
                    // should have been.
                    else if ((pTemp[1] == 0xFF) &
                             (pTemp[2] == ySyncCheck))
                        nFrameSize = iFrameSize;

                    // Did not find consequtive frames.  Keep looking
                    else
                        continue;
                }
                else
                {
                    // We found a frame
                    nFrameSize = iFrameSize;
                }
            }
            // There is one full frame in this buffer
            else if (pBuf+iFrameSize == pEnd+4)
            {
                // We found a frame
                nFrameSize = iFrameSize;
            }

            break;
        }
    }

    if (nFrameSize)
        return pBuf - pStart;
    else
        return -1;
}

HXBOOL CMp3Format::CheckForHeaders(UINT8 *pBuf,
                                 UINT32 dwSize,
                                 INT32 &lHeaderSize)
{
    if (m_pMisc)
        return m_pMisc->CheckForHeaders(pBuf, dwSize, lHeaderSize);
    else
        return FALSE;
}

eHeaderType CMp3Format::GetHeaderType()
{
    if (m_pMisc)
        return m_pMisc->GetHeaderType();
    else
        return eNone;
}

// Functions to access the Id3 header values
UINT8* CMp3Format::GetId3Title(int &nLen)
{
    if (m_pMisc)
        return m_pMisc->GetId3Title(nLen);
    else
        return NULL;
}

UINT8* CMp3Format::GetId3Artist(int &nLen)
{
    if (m_pMisc)
        return m_pMisc->GetId3Artist(nLen);
    else
        return NULL;
}

UINT8* CMp3Format::GetId3Album(int &nLen)
{
    if (m_pMisc)
        return m_pMisc->GetId3Album(nLen);
    else
        return NULL;
}

UINT8* CMp3Format::GetId3Genre(int &nLen)
{
    if (m_pMisc)
        return m_pMisc->GetId3Genre(nLen);
    else
        return NULL;
}

int CMp3Format::GetMetaOffset()
{
    if (m_pMisc)
        return m_pMisc->GetMetaOffset();
    else
        return 0;
}

int CMp3Format::GetMetaRepeat()
{
    if (m_pMisc)
        return m_pMisc->GetMetaRepeat();
    else
        return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Private Functions

HXBOOL CMp3Format::GetDataOffsetMPEG1(UINT8 *pHeader,
                                    UINT32 dwSize,
                                    int &nFrameSize,
                                    int &nHeaderSize,
                                    int &nDataOffset,
                                    int trustPackets)
{
    // Get header info
    MPEG_HEAD h;
    memset(&h, 0, sizeof(h));
    nFrameSize = head_info(pHeader, dwSize, &h, trustPackets);

    if (nFrameSize)
        nFrameSize += h.pad;

    // HeaderSize is the sync word, CRC, and side info
    nHeaderSize = 4;

    // Check protection bit
    if ((pHeader[1] & 1) == 0)
        nHeaderSize += 2;

    // Not enough data to check main_data_begin
    if (dwSize < (unsigned int)nHeaderSize + 2)
        return 0;

    // Extract main_data_begin offset (9 bits)
    nDataOffset = (pHeader[nHeaderSize] << 1) +
                  (pHeader[nHeaderSize + 1] >> 7);

    // Side info (256 bits for stereo, 136 for mono)
    if (3 == h.mode)
        nHeaderSize += 17;
    else
        nHeaderSize += 32;

    return 1;
}

HXBOOL CMp3Format::GetDataOffsetMPEG2(UINT8 *pHeader,
                                    UINT32 dwSize,
                                    int &nFrameSize,
                                    int &nHeaderSize,
                                    int &nDataOffset,
                                    int trustPackets)
{
    // Get header info
    MPEG_HEAD h;
    memset(&h, 0, sizeof(h));
    nFrameSize = head_info(pHeader, dwSize, &h, trustPackets);

    if (nFrameSize)
        nFrameSize += h.pad;

    // HeaderSize is the sync word, CRC, and side info
    nHeaderSize = 4;

    // Check protection bit
    if ((pHeader[1] & 1) == 0)
        nHeaderSize += 2;

    // Not enough data to check main_data_begin
    if (dwSize < (unsigned int)nHeaderSize + 1)
        return 0;

    // Extract main_data_begin offset (8 bits)
    nDataOffset = pHeader[nHeaderSize];

    // Side info (136 bits for stereo, 72 for mono)
    if (3 == h.mode)
        nHeaderSize += 9;
    else
        nHeaderSize += 17;

    return 1;
}

