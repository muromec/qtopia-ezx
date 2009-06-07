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
#include "hxtypes.h"
#include "audinfo.h"

CAudioInfoBase::CAudioInfoBase()
 :  m_ySyncWord(0),
    m_bIsInited(0)
{
}

CAudioInfoBase::~CAudioInfoBase()
{
}

HXBOOL CAudioInfoBase::IsInited()
{
    return m_bIsInited;
}

INT16 CAudioInfoBase::GetSyncWord()
{
    return m_ySyncWord;
}

UINT32 CAudioInfoBase::GetAudioRtpPacket(UINT8 *pRead,
                                         UINT32 ulRead,
                                         tRtpPacket *pRtp)
{
    UINT8   bCont = 1,
            *pStart = pRead,
            *pLastFrame = NULL;

    UINT32  ulBytes = ulRead,
            ulFrameSize = 0;

    pRtp->ulHeaderSize = 4;

    // Finish fragmented packet
    if (pRtp->bPacketFrag)
    {
        if (ulRead >= pRtp->ulFragBytesRem)
        {
            // Set offset header
            pRtp->aHeader[3] = (pRtp->wFragBytes & 0xFF00)>>8;
            pRtp->aHeader[4] = pRtp->wFragBytes & 0x00FF;

            pRtp->bPacketReady = 1;
            pRtp->bPacketFrag = 0;
            pRtp->wFragBytes = 0;
            
            ulBytes = pRtp->ulFragBytesRem;
            pRtp->ulFragBytesRem = 0;

            return ulBytes;
        }
        else
            return 0;
    }
    
    while (bCont)
    {
        ulFrameSize = (UINT32)CheckValidFrame(pRead, ulRead);

        // Check for space in our rtp packet
        if (ulFrameSize > pRtp->ulBytesFree)
        {
            // The frame is bigger than our rtp packet size
            if (!pRtp->ulDataChunks)
            {
                pRtp->bPacketFrag = 1;

                // Set rtp packet to max size (send as much of
                // the slice as we can).
                pRead += pRtp->ulBytesFree;

                pRtp->wFragBytes = (INT16)(pRead - pStart);
                pRtp->ulFragBytesRem = ulFrameSize - pRtp->wFragBytes;
            }

            pRtp->bPacketReady = 1;
            bCont = 0;
        }
        else if (ulFrameSize)
        {
            pLastFrame = pRead;

            ++pRtp->ulDataChunks;
            pRead += ulFrameSize;
            ulRead -= ulFrameSize;

            pRtp->ulBytesFree -= ulFrameSize;
        }
        else
        {
            if (pRtp->ulDataChunks)
            {
                pRtp->bPacketReady = 1;
                bCont = 0;
            }
            // On a seek, we need to find an audio frame
            else
            {
                int nFrameSize = 0;
                INT32 lScan = 0;
                lScan = ScanForSyncWord(pRead, ulRead, nFrameSize);

                if (lScan >= 0)
                {
                    pRead += lScan;
                    ulRead -= lScan;
                }
                else
                {
                    bCont = 0;                    
                }
            }
        }
    }

    ulBytes = (UINT32)(pRead - pStart);

    // Bump scr and pts
    if (!pRtp->bPacketReady)
    {
        memset(pRtp->aHeader, 0, sizeof(pRtp->aHeader));
        pRtp->bPacketReady = 0;
        pRtp->ulHeaders = 0;
        pRtp->ulDataChunks = 0;
        pRtp->ulBytesFree = RTP_PACKET_SIZE;
    }

    return ulBytes;
}
