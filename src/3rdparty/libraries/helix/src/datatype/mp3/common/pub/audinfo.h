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

#ifndef _AUDINFO_H_
#define _AUDINFO_H_

// RTP Stuff
typedef struct
{
    UINT8       aHeader[8];     // Payload header

    UINT32      ulBytesFree,    // Number of bytes available in packet
                ulBytesSent,    // Number of bytes sent in packets
                ulFirstPts,     // First pts in play sequence (90 hz)
                ulFirstScr,     // First scr in play sequence (ms)
                ulFragBytesRem, // Number of bytes needed for full packet
                ulHeaders,      // Number of headers in this packet
                ulHeaderSize,   // Number of bytes in the payload header
                ulPendingReq,   // Number of packet requests pending
                ulDataChunks;   // Number of rtp data seqments in this packet

#ifdef SKIP_TEST
    UINT32      ulPicSkip;
#endif

    double      dScr,           // Time for packet delivery
                dPts,           // Time for packet presentation
                dPtsDelta;      // Delta of file ts and stream ts

    HXBOOL        bDiscontinuity, // Set marker bit to 1
                bPacketReady,   // Is this packet ready for the wire
                bPacketFrag,    // Fragmentation on this packet
                bServiceReq;    // Send packets for old requests

    INT16       wFragBytes;     // Number of bytes in frag packet

} tRtpPacket;

enum            {RTP_PACKET_SIZE=1450};

class CAudioInfoBase
{
public:

    CAudioInfoBase();
    virtual ~CAudioInfoBase();

    virtual HXBOOL Init(UINT8 *pHeader,
                      UINT32 ulSize) = 0;

    virtual HXBOOL GetEncodeInfo(UINT8 *pHeader,
                               UINT32 dwSize,
                               UINT32 &ulBitRate,
                               UINT32 &ulSampRate,
                               int &nChannels,
                               int &nLayer,
                               int &nSamplesPerFrame,
                               int &nPadding) = 0;

    virtual int CheckValidFrame(UINT8 *pBuf,
                                UINT32 dwSize) = 0;

    virtual INT32 ScanForSyncWord(UINT8 *pBuf,
                                  INT32 lSize,
                                  int &nFrameSize) = 0;

    virtual HXBOOL    IsInited();
    virtual INT16   GetSyncWord();

    UINT32 GetAudioRtpPacket(UINT8 *pRead,
                             UINT32 ulRead,
                             tRtpPacket *pRtp);
protected:

	UINT8   m_ySyncWord;    // Last byte of the sync word
	HXBOOL    m_bIsInited;    // Has Init_c been called
};
#endif
