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

#ifndef _MP3FORMAT_H_
#define _MP3FORMAT_H_

#include "hxtypes.h"
#include "audinfo.h"
#include "mp3misc.h"

class CMp3Queue;

class CMp3Format : public CAudioInfoBase
{
public:
    CMp3Format();
    CMp3Format(CMp3Misc* pMis);
    ~CMp3Format();

    ///////////////////////////////////////////////////////////////////////////
    // Function:    Init
    // Purpose:     Inits the reformat for the specified audio type.
    // Params:      pHeader is pointer to an MP3 header
    //              ulSize is the size of pHeader
    //
    // Notes:       This must be called once per audio type.
    // Return:      1 if successful 0 otherwise
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    HXBOOL Init(UINT8 *pHeader,
              UINT32 ulSize);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    GetDataOffset
    // Purpose:     Extract the frame size, header size, and main_data_begin
    //              from an MP3 sync word.
    //
    // Params:      pHeader is a pointer to an MP3 sync word
    //              dwSize is the size of the pHeader buffer
    //              nFrameSize will store the size of the sync
    //              nHeaderSize will store size of the sync word, CRC, and side
    //              info nDataOffset will store main_data_begin.
    //
    // Return:      1 if successful, 0 otherwise (not enough data)
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    HXBOOL GetDataOffset(UINT8 *pHeader,
                       UINT32 dwSize,
                       int &nFrameSize,
                       int &nHeaderSize,
                       int &nDataOffset);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    ClearMainDataBegin
    // Purpose:     Sets the main_data_begin bits of the mp3 frame to 0
    //
    // Params:      pHeader is a pointer to an mp3 frame
    //
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    void ClearMainDataBegin(UINT8 *pHeader);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    GetEncodeInfo
    // Purpose:     Gets encode info on the mpeg audio stream
    //
    // Params:      pHeader is a pointer to an MPEG audio sync word
    //              dwSize is the size of the pHeader buffer
    //              ulBitRate will store the encoded bitrate of the stream
    //              ulSampRate will store the encoded sample rate of the stream
    //              nChannels will store the number of channels in the stream
    //              nSamplesPerFrame will store the MPEG samples in each frame
    //              nPadding will store the padding bit in each frame
    //
    // Return:      1 if successful, 0 otherwise (not enough data)
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    HXBOOL GetEncodeInfo(UINT8 *pHeader,
                       UINT32 dwSize,
                       UINT32 &ulBitRate,
                       UINT32 &ulSampRate,
                       int &nChannels,
                       int &nLayer,
                       int &nSamplesPerFrame,
                       int &nPadding);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    ReformatMP3Frame
    // Purpose:     Reformat an MP3 frame by moving its sync word before its
    //              main data.
    //
    // Params:      pFrame is a pointer to 2 consecutive MP3 sync words
    //              dwBytes is the number of bytes in the buffer *ppFrame
    //              dwPrevBytes is the size of any bytes before ppFrame.
    //
    // Return:      The size of the modified MP3 frame, or 0 for an error
    //
    // Note:        main_data_begin does not include any bytes in sync words or
    //              side info, so no book keeping is necessary of the offsets.
    //              Also, the caller must ensure that the main_data_begin bytes
    //              are before pFrame.  The maximum value of main_data_begin is 512.
    //
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    int ReformatMP3Frame(UINT8 **ppFrame,
                         UINT32 dwBytes,
                         UINT32 dwPrevBytes);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    UnformatMP3Frame
    // Purpose:     Unformats a formatted mp3 to its origonal form
    //
    // Params:      pQueue is a queue that buffers formatted frames until
    //              the queue has enough data to unformat.
    //              pDest is a pointer to the buffer to copy the
    //              unformatted frame
    //              ulTime will contain the time stamp of the unformatted frame
    //
    // Return:      The size of the unformatted MP3 frame, 0 if the queue
    //              needs more data to finish the unformat.
    //
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    int UnformatMP3Frame(CMp3Queue* pQueue,
                         UINT8* pDest,
                         UINT32 &ulTime);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    CheckValidFrame
    // Purpose:     Checks if pBuf points to a valid MPEG audio frame of the
    //              type we have been decoding (passed to in Init_c).
    // Params:      pBuf is a buffer of data
    //              dwSize is the size of pBuf
    // Returns:     Size of the frame, or 0 if it is not a frame
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    int CheckValidFrame(UINT8 *pBuf,
                        UINT32 dwSize);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    ScanForSyncWord
    // Purpose:     Looks for a sync word ensuring there is a full frame in pBuf
    // Params:      pBuf is a buffer to scan for the sync word
    //              lSize is the size of pBuf
    //              nFrameSize will cotain the size of the frame associated with
    //              the sync word.
    // Returns:     The number of bytes scanned, or -1 for no sync word
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    INT32 ScanForSyncWord(UINT8 *pBuf,
                          INT32 lSize,
                          int &nFrameSize);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    CheckForHeaders
    // Purpose:     Checks for one of the many headers, legit and bogus, people
    //              put in mp3 streams.
    // Params:      pBuf is a buffer of mp3 data
    //              dwSize is the size of pBuf
    //              lHeaderSize will cotain the size of the header.  -1 means
    //              the rest of the stream is the header.
    // Returns:     1 if it found a header, otherwise 0
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    HXBOOL CheckForHeaders(UINT8 *pBuf,
                         UINT32 dwSize,
                         INT32 &lHeaderSize);

    // Status Functions
    eHeaderType     GetHeaderType();
    INT16           GetSyncWord() {return 0xFF00 + m_ySyncWord;}

    // Check syncword
    HXBOOL            IsValidSyncWord(BYTE cSync);
    void            SetTrustPackets(HXBOOL bTrust) { m_bTrustPackets = bTrust; }

    // Functions to access the Id3 header values
    UINT8*  GetId3Title(int &nLen);
    UINT8*  GetId3Artist(int &nLen);
    UINT8*  GetId3Album(int &nLen);
    UINT8*  GetId3Genre(int &nLen);
    int     GetMetaOffset();
    int     GetMetaRepeat();


private:
    typedef HXBOOL (*GET_DATA_OFFSET)(UINT8 *,
                                    UINT32,
                                    int &,
                                    int &,
                                    int &,
                                    int);

    static HXBOOL GetDataOffsetMPEG1(UINT8 *pHeader,
                                   UINT32 dwSize,
                                   int &nFrameSize,
                                   int &nHeaderSize,
                                   int &nDataOffset,
                                   int trustPackets);

    static HXBOOL GetDataOffsetMPEG2(UINT8 *pHeader,
                                   UINT32 dwSize,
                                   int &nFrameSize,
                                   int &nHeaderSize,
                                   int &nDataOffset,
                                   int trustPackets);

    CMp3Misc*   m_pMisc;

    int         m_nLayer;       // Layer of MPEG Audio stream
    HXBOOL        m_bMpeg25;      // Is this MPEG 2.5
    HXBOOL        m_bTrustPackets;

    GET_DATA_OFFSET m_pFnGetData;   // GetDataOffset Function
};


inline HXBOOL CMp3Format::IsValidSyncWord(BYTE cSync)
{
    // We check here everything except the
    // least-significant bit (the redundancy bit)
    return (cSync & 0xFE) == (m_ySyncWord & 0xFE);
}

#endif
