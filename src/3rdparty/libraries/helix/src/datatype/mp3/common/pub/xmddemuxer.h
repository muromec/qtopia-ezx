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

#ifndef _XMDDEMUXER_H_
#define _XMDDEMUXER_H_

#include "types64.h"

// Pack sizes
enum
{   PACK_SIZE = 2048,
    PACK_HEADER_SIZE = 14,
    PACK_DATA_SIZE = PACK_SIZE - PACK_HEADER_SIZE,

    SYSTEM_HEADER_SIZE = 24
};

// Stat codes
enum
{ 
    PICTURE_START_CODE  = 0x00000100,
    SEQ_START_CODE      = 0x000001b3,
    EXT_DATA_START_CODE = 0x000001b5,
    GROUP_START_CODE    = 0x000001b8,
    PACK_HEADER         = 0x000001ba,
    SYSTEM_HEADER       = 0x000001bb,
    PRIVATE_STREAM_1    = 0x000001bd,
    PRIVATE_STREAM_2    = 0x000001bf,
    VIDEO_PACKET        = 0x000001e0,
    AUDIO_PACKET        = 0x000001c0,
    NO_START_CODE       = 0xffffffff
};        


class CDemuxer
{
public:
    CDemuxer(HXBOOL bMPEG2=TRUE);
    ~CDemuxer();

    UINT32   Demux_ul(UINT8 *pBuffer, UINT32 ulBytes, Packet *pPacket);
    static   INT32 GetStartCode(UINT8 **ppBuffer, UINT32 &ulBytes);
    void     Reset_v();
    
protected:
    UINT8           ProcessPacket(UINT8 **ppBuffer, UINT32 &ulBytes, Packet *pPacket);
    UINT8           ProcessMPEG1Packet(UINT8 **ppBuffer, UINT32 &ulBytes, Packet *pPacket);

    INT64           GetTimeStamp(UINT8 *pBuffer);

    HXBOOL            m_bMPEG2;       // Is this MPEG1 or MPEG2 data
    UINT8           *m_pPack;       // Pointer to the last pack

    INT64           m_llPtsDelta;
};

#endif
