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
#include "amr_frame_info.h"
#include "amr_flavor.h"
#include "amr_frame_hdr.h"


// These sizes are for AMR Core bits
const ULONG32 CAMRFrameInfo::zm_frameBits[2][16] = {  
    { 95, 103, 118, 134,  // AMR-NB
     148, 159, 204, 244,
      39,  43,  38,  37,
      0xffffffff,  0xffffffff,  ULONG32(-1),   0 },
    {132, 177, 253, 285,  // AMR-WB
     317, 365, 397, 461, 
     477,  40,  ~0,  0xffffffff, 
      0xffffffff,  0xffffffff,   0,   0}
};

const ULONG32 CAMRFrameInfo::zm_maxFrameBits[2] = {244,  // AMR-NB
                                                   477}; // AMR-WB

const ULONG32 CAMRFrameInfo::zm_frameDuration = 20;

/*
 * Given a buffer which may or may not contain
 * the beginning of an AMR frame, this method
 * finds the beginning of an AMR frame in this 
 * buffer. For a beginning offset to be considered
 * valid, ulMinFrames valid frames must be found
 * from this point.
 */
HXBOOL CAMRFrameInfo::FindFrameBegin(AMRFlavor eFlavor,
                                   BYTE* pBuf, UINT32 ulLen,
                                   UINT32 ulMinFrames,
                                   UINT32& rulFrameBegin)
{
    HXBOOL bRet = FALSE;

    if (pBuf && ulLen && ulMinFrames)
    {
        BYTE*  pBufStart = pBuf;
        BYTE*  pBufLimit = pBuf + ulLen;
        while (pBuf < pBufLimit)
        {
            // Can we find ulMinFrames valid frames from
            // starting with this byte?
            if (TestForConsecutiveFrames(eFlavor,
                                         pBuf,
                                         pBufLimit - pBuf,
                                         ulMinFrames))
            {
                rulFrameBegin = pBuf - pBufStart;
                bRet = TRUE;
                break;
            }
            else
            {
                // No, we didn't, so go to the next byte
                pBuf++;
            }
        }
    }

    return bRet;
}

HXBOOL CAMRFrameInfo::TestForConsecutiveFrames(AMRFlavor eFlavor, BYTE* pBuf,
                                             UINT32 ulLen, UINT32 ulMinFrames)
{
    HXBOOL bRet = FALSE;

    if (pBuf && ulLen)
    {
        BYTE*  pBufLimit   = pBuf + ulLen;
        UINT32 ulNumFrames = 0;
        while (pBuf < pBufLimit)
        {
            // Check if the padding bits are correctly set
            if (!CAMRFrameHdr::ValidHdrByte(*pBuf))
            {
                break;
            }
            // Parse the type and quality
            CAMRFrameHdr cHdr(eFlavor);
            cHdr.Unpack(pBuf); // advances the pointer by 1
            // Make sure the number of data bits is not a ULONG32 -1
            if (cHdr.DataBits() == ((ULONG32) -1))
            {
                break;
            }
            // If we have enough bytes, we'll assume 
            // a valid frame
            if (pBuf + cHdr.DataBytes() <= pBufLimit)
            {
                ulNumFrames++;
            }
            // Advance past the data bytes
            pBuf += cHdr.DataBytes();
        }
        if (ulNumFrames >= ulMinFrames)
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

