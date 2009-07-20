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

#ifndef AMR_FRAME_HDR_H
#define AMR_FRAME_HDR_H

#include "hxtypes.h"
#include "amr_frame_info.h"

// This class represents the AMR frame header
// This header is defined in 
// 3GPP TS26.235 v5.0.0 Annex B Section B.5.3
//   7   6   5   4   3   2   1   0
// +---+---------------+---+---+---+
// | P |       FT      | Q | P | P |
// +---+---------------+---+---+---+
//
// P  = padding
// FT = frame type
// Q  = quality indicator
//
// According to RFC3267, the three P bits
// MUST be set to 0.
//

class CAMRFrameHdr
{
public:
    CAMRFrameHdr(AMRFlavor flavor);
    CAMRFrameHdr(UINT8 type, UINT8 quality);

    void Pack(UINT8*& pBuf) const;
    void Unpack(UINT8*& pBuf);

    UINT8 Type() const;
    UINT8 Quality() const;

    static ULONG32 HdrBits();
    static ULONG32 HdrBytes();
    static HXBOOL ValidHdrByte(BYTE ucByte);
    ULONG32 DataBits() const;
    ULONG32 DataBytes() const;

private:
    UINT8 m_type;
    UINT8 m_quality;
    AMRFlavor m_flavor;
};

inline
CAMRFrameHdr::CAMRFrameHdr(AMRFlavor flavor) :
    m_type(15),
    m_quality(0),
    m_flavor(flavor)
{}

inline
CAMRFrameHdr::CAMRFrameHdr(UINT8 type, UINT8 quality) :
    m_type(type),
    m_quality(quality)
{}

inline
void CAMRFrameHdr::Pack(UINT8*& pBuf) const
{
    *pBuf++ = ((m_type & 0xf) << 3) | ((m_quality & 0x1) << 2);
}

inline
void CAMRFrameHdr::Unpack(UINT8*& pBuf)
{
    m_type = (*pBuf >> 3) & 0x0f;
    m_quality = (*pBuf >> 2) & 0x01;

    pBuf++;
}

inline
UINT8 CAMRFrameHdr::Type() const
{
    return m_type;
}

inline
UINT8 CAMRFrameHdr::Quality() const
{
    return m_quality;
}

inline
ULONG32 CAMRFrameHdr::HdrBits()
{
    return 8;
}

inline
ULONG32 CAMRFrameHdr::HdrBytes()
{
    return 1;
}

inline HXBOOL CAMRFrameHdr::ValidHdrByte(BYTE ucByte)
{
    // According to RFC3267, the three padding
    // bits must be set to 0. Therefore, if ucByte
    // has any one of these bits set to 1, then 
    // we know that this could not be a valid
    // AMR header byte.
    //
    // Bit mask for padding bits is 0x83.
    return ((ucByte & 0x83) ? FALSE : TRUE);
}

inline
ULONG32 CAMRFrameHdr::DataBits() const
{
    return CAMRFrameInfo::FrameBits(m_flavor, m_type);
}

inline
ULONG32 CAMRFrameHdr::DataBytes() const
{
    return (DataBits() + 7) >> 3;
}

#endif // AMR_FRAME_HDR_H
