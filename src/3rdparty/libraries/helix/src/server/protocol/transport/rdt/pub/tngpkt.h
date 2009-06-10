/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id:
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

#ifndef _TNGPKT_H_
#define _TNGPKT_H_

#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

struct buffer 
{
    UINT32      len;
    INT8*       data;
};
#endif/*PMC_PREDEFINED_TYPES*/


/* PMC DEFINITION
packlen struct TNGDataPacket
{
    bit[1]  length_included_flag;   // Inc. length field
    bit[1]  need_reliable_flag;     // Inc. rel. seq no
    bit[5]  stream_id;              // 31 == expansion
    bit[1]  is_reliable;

    u_int16 seq_no;                 // overload for packet type

    if (length_included_flag == 1)
    {
        u_int16 _packlenwhendone;
    }

    bit[1]  back_to_back_packet;
    bit[1]  extension_flag;
    bit[6]  asm_rule_number;        // 63 is expansion

    u_int32 timestamp;

    if (stream_id == 31)
    {
        u_int16 stream_id_expansion;
    }
    if (need_reliable_flag == 1)
    {
        u_int16 total_reliable;
    }
    if (asm_rule_number == 63)
    {
        u_int16 asm_rule_number_expansion;
    }
    if (extension_flag == 1)
    {
        bit[4]  version;             // 4 (0100)
        bit[4]  reserved;
        u_int32 rsid;
    }
    buffer  data;
}
 */

class TNGDataPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 21;}

    UINT8   length_included_flag;
    UINT8   need_reliable_flag;
    UINT8   stream_id;
    UINT8   is_reliable;
    UINT16  seq_no;
    UINT16  _packlenwhendone;
    UINT8   back_to_back_packet;
    UINT8   extension_flag;
    UINT8   asm_rule_number;
    UINT32  timestamp;
    UINT16  stream_id_expansion;
    UINT16  total_reliable;
    UINT16  asm_rule_number_expansion;
    UINT8   version;
    UINT8   reserved;
    UINT32  rsid;
    buffer  data;
};

inline UINT8*
TNGDataPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;
    UINT8* pLen = 0;

    *off &= ~(1<<7); 
    *off |= (length_included_flag&1)<<7;

    *off &= ~(1<<6); 
    *off |= (need_reliable_flag&1)<<6;

    *off &= ~0x3e; 
    *off |= (stream_id&0x1f)<<1;

    *off &= ~1; 
    *off++ |= is_reliable&1;

    *off++ = (UINT8)(seq_no>>8); 
    *off++ = (UINT8)(seq_no);

    if (length_included_flag == 1)
    {
        pLen = off;
        off += 2;
    }

    *off &= ~(1<<7);
    *off |= (back_to_back_packet&1)<<7;

    *off &= ~(1<<6);
    *off |= (extension_flag&1)<<6;

    *off &= ~0x3f; 
    *off++ |= asm_rule_number&0x3f;

    *off++ = (UINT8)(timestamp>>24); 
    *off++ = (UINT8)(timestamp>>16); 
    *off++ = (UINT8)(timestamp>>8); 
    *off++ = (UINT8)(timestamp);

    if (stream_id == 31)
    {
        *off++ = (UINT8)(stream_id_expansion>>8); 
        *off++ = (UINT8)(stream_id_expansion);
    }

    if (need_reliable_flag == 1)
    {
        *off++ = (UINT8)(total_reliable>>8); 
        *off++ = (UINT8)(total_reliable);
    }

    if (asm_rule_number == 63)
    {
        *off++ = (UINT8)(asm_rule_number_expansion>>8); 
        *off++ = (UINT8)(asm_rule_number_expansion);
    }

    if (extension_flag == 1)
    {
        *off &= ~0xf0;
        *off |= (UINT8)(version&0xf)<<4;

        *off &= ~0xf;
        *off++ |= (UINT8)(reserved&0xf);

        *off++ = (UINT8)(rsid>>24); 
        *off++ = (UINT8)(rsid>>16); 
        *off++ = (UINT8)(rsid>>8); 
        *off++ = (UINT8)(rsid);
    }

    if (data.data)
    {
        memcpy(off, data.data, data.len);
    }
    off += data.len;
    len = off-buf;
    if (pLen)
    {
        *pLen++ = (UINT8)(len>>8);
        *pLen++ = (UINT8)(len);
    }

    return off;
}

inline UINT8*
TNGDataPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    need_reliable_flag = (*off>>6)&1;

    stream_id = (*off&0x3e)>>1;

    is_reliable = *off++&1;

    seq_no = *off++<<8; 
    seq_no |= *off++;

    if (length_included_flag == 1)
    {
        _packlenwhendone = *off++<<8;
        _packlenwhendone |= *off++;
    }

    back_to_back_packet = (*off>>7)&1;

    extension_flag = (*off>>6)&1;

    asm_rule_number = *off++&0x3f;

    timestamp = ((UINT32)*off++)<<24; 
    timestamp |= ((UINT32)*off++)<<16;
    timestamp |= ((UINT32)*off++)<<8; 
    timestamp |= ((UINT32)*off++);

    if (stream_id == 31)
    {
        stream_id_expansion = *off++<<8; 
        stream_id_expansion |= *off++;
    }

    if (need_reliable_flag == 1)
    {
        total_reliable = *off++<<8; 
        total_reliable |= *off++;
    }

    if (asm_rule_number == 63)
    {
        asm_rule_number_expansion = *off++<<8; 
        asm_rule_number_expansion |= *off++;
    }

    if (extension_flag == 1)
    {
        version = (*off&0xf0)>>4;

        reserved = *off++&0xf;

        rsid = ((UINT32)*off++)<<24;
        rsid |= ((UINT32)*off++)<<16;
        rsid |= ((UINT32)*off++)<<8;
        rsid |= ((UINT32)*off++);
    }

    data.len = len - (off - buf);
    if (off-buf+data.len > len)
    {
        return 0;
    }
    data.data = (INT8*)off;
    off += data.len;

    return off;
}


/* PMC DEFINITION
struct TNGMultiCastDataPacket
{
    bit[1]  length_included_flag;   // Inc. length field
    bit[1]  need_reliable_flag;     // Inc. rel. seq no
    bit[5]  stream_id;              // 31 == expansion
    bit[1]  is_reliable;

    u_int16 seq_no;                 // overload for packet type

    if (length_included_flag == 1)
    {
        u_int16 length;
    }

    bit[1]  back_to_back_packet;
    bit[1]  slow_data;
    bit[6]  asm_rule_number;        // 63 is expansion

    u_int8  group_seq_no;

    u_int32 timestamp;
    if (stream_id == 31)
    {
        u_int16 stream_id_expansion;
    }
    if (need_reliable_flag == 1)
    {
        u_int16 total_reliable;
    }
    if (asm_rule_number == 63)
    {
        u_int16 asm_rule_number_expansion;
    }
    buffer  data;
}
 */

class TNGMultiCastDataPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 17;}

    UINT8   length_included_flag;
    UINT8   need_reliable_flag;
    UINT8   stream_id;
    UINT8   is_reliable;
    UINT16  seq_no;
    UINT16  length;
    UINT8   back_to_back_packet;
    UINT8   slow_data;
    UINT8   asm_rule_number;
    UINT8   group_seq_no;
    UINT32  timestamp;
    UINT16  stream_id_expansion;
    UINT16  total_reliable;
    UINT16  asm_rule_number_expansion;
    buffer  data;
};

inline UINT8*
TNGMultiCastDataPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7);
    *off |= (length_included_flag&1)<<7;

    *off &= ~(1<<6); 
    *off |= (need_reliable_flag&1)<<6;

    *off &= ~0x3e; 
    *off |= (stream_id&0x1f)<<1;

    *off &= ~1; 
    *off++ |= is_reliable&1;

    *off++ = (UINT8)(seq_no>>8); 
    *off++ = (UINT8)(seq_no);

    if (length_included_flag == 1)
    {
        *off++ = (UINT8)(length>>8); 
        *off++ = (UINT8)(length);
    }

    *off &= ~(1<<7);
    *off |= (back_to_back_packet&1)<<7;

    *off &= ~(1<<6);
    *off |= (slow_data&1)<<6;

    *off &= ~0x3f; 
    *off++ |= asm_rule_number&0x3f;

    *off++ = group_seq_no;

    *off++ = (UINT8)(timestamp>>24);
    *off++ = (UINT8)(timestamp>>16); 
    *off++ = (UINT8)(timestamp>>8); 
    *off++ = (UINT8)(timestamp);

    if (stream_id == 31)
    {
        *off++ = (UINT8)(stream_id_expansion>>8); 
        *off++ = (UINT8)(stream_id_expansion);
    }

    if (need_reliable_flag == 1)
    {
        *off++ = (UINT8)(total_reliable>>8); 
        *off++ = (UINT8)(total_reliable);
    }

    if (asm_rule_number == 63)
    {
        *off++ = (UINT8)(asm_rule_number_expansion>>8); 
        *off++ = (UINT8)(asm_rule_number_expansion);
    }

    if (data.data) 
    {
        memcpy(off, data.data, data.len);
    }
    off += data.len;
    len = off-buf;

    return off;
}

inline UINT8*
TNGMultiCastDataPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    need_reliable_flag = (*off>>6)&1;

    stream_id  = (*off&0x3e)>>1;

    is_reliable = *off++&1;

    seq_no = *off++<<8;
    seq_no |= *off++;

    if (length_included_flag == 1)
    {
        length = *off++<<8;
        length |= *off++;
    }

    back_to_back_packet = (*off>>7)&1;

    slow_data = (*off>>6)&1;

    asm_rule_number  = *off++&0x3f;

    group_seq_no = *off++;

    timestamp = ((UINT32)*off++)<<24; 
    timestamp |= ((UINT32)*off++)<<16;
    timestamp |= ((UINT32)*off++)<<8; 
    timestamp |= ((UINT32)*off++);

    if (stream_id == 31)
    {
        stream_id_expansion = *off++<<8; 
        stream_id_expansion |= *off++;
    }

    if (need_reliable_flag == 1)
    {
        total_reliable = *off++<<8; 
        total_reliable |= *off++;
    }

    if (asm_rule_number == 63)
    {
        asm_rule_number_expansion = *off++<<8;
        asm_rule_number_expansion |= *off++;
    }

    data.len = len - (off - buf);
    if (off-buf+data.len > len)
    {
        return 0;
    }
    data.data = (INT8*)off;
    off += data.len;

    return off;
}


/* PMC DEFINITION
struct TNGASMActionPacket
{
    bit[1]  length_included_flag;
    bit[5]  stream_id;
    bit[1]  dummy0;                 // 0
    bit[1]  dummy1;                 // 0
    u_int16 packet_type;            // 0xff00
    u_int16 reliable_seq_no;
    if (length_included_flag == 1)
    {
        u_int16 length;
    }
    if (stream_id == 31)
    {
        u_int16 stream_id_expansion;
    }
    buffer  data;
}
 */

class TNGASMActionPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 9;}

    UINT8   length_included_flag;
    UINT8   stream_id;
    UINT8   dummy0;
    UINT8   dummy1;
    UINT16  packet_type;
    UINT16  reliable_seq_no;
    UINT16  length;
    UINT16  stream_id_expansion;
    buffer  data;
};

inline UINT8*
TNGASMActionPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (length_included_flag&1)<<7;

    *off &= ~0x7c; 
    *off |= (stream_id&0x1f)<<2;

    *off &= ~(1<<1);
    *off |= (dummy0&1)<<1;
    *off &= ~1; 
    *off++ |= dummy1&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);
    *off++ = (UINT8)(reliable_seq_no>>8);
    *off++ = (UINT8)(reliable_seq_no);

    if (length_included_flag == 1)
    {
        *off++ = (UINT8)(length>>8); 
        *off++ = (UINT8)(length);
    }

    if (stream_id == 31)
    {
        *off++ = (UINT8)(stream_id_expansion>>8); 
        *off++ = (UINT8)(stream_id_expansion);
    }

    if (data.data) 
    {
        memcpy(off, data.data, data.len);
    }
    off += data.len;
    len = off-buf;

    return off;
}

inline UINT8*
TNGASMActionPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    stream_id  = (*off&0x7c)>>2;

    dummy0 = (*off>>1)&1;
    dummy1 = *off++&1;

    packet_type = *off++<<8;
    packet_type |= *off++;

    reliable_seq_no = *off++<<8; 
    reliable_seq_no |= *off++;

    if (length_included_flag == 1)
    {
        length = *off++<<8;
        length |= *off++;
    }

    if (stream_id == 31)
    {
        stream_id_expansion = *off++<<8; 
        stream_id_expansion |= *off++;
    }

    data.len = len - (off - buf);
    if (off-buf+data.len > len)
    {
        return 0;
    }
    data.data = (INT8*)off; 
    off += data.len;

    return off;
}


/* PMC DEFINITION
struct TNGACKPacket
{
    bit[1]  length_included_flag;
    bit[1]  lost_high;
    bit[5]  dummy;                 // 0
    bit[1]  extension_flag;
    u_int16 packet_type;            // 0xff02
    if (length_included_flag == 1)
    {
        u_int16 length;
    }
    if (extension_flag == 1)
    {
        bit[4]  version;            // 4 (0100)
        bit[4]  reserved;
        u_int32 rsid;
    }
    
    // data consists of header + bitmap repeated for each stream: 
    //   u_int16 stream_number
    //   u_int16 last_seq_no
    //   u_int16 bit_count
    //   u_int8  bitmap_length
    //   buffer bitmap
    buffer  data;
}
 */

class TNGACKPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 10;}

    UINT8   length_included_flag;
    UINT8   lost_high;
    UINT8   dummy;
    UINT8   extension_flag;
    UINT16  packet_type;
    UINT16  length;
    UINT8   version;
    UINT8   reserved;
    UINT32  rsid;
    buffer  data;
};

inline UINT8*
TNGACKPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (length_included_flag&1)<<7;

    *off &= ~(1<<6);
    *off |= (lost_high&1)<<6;

    *off &= ~0x3e; 
    *off |= (dummy&0x1f)<<1;

    *off &= ~1; 
    *off++ |= extension_flag&1;

    *off++ = (UINT8)(packet_type>>8);
    *off++ = (UINT8)(packet_type);

    if (length_included_flag == 1)
    {
        *off++ = (UINT8)(length>>8); 
        *off++ = (UINT8)(length);
    }

    if (extension_flag == 1)
    {
        *off &= ~0xf0;
        *off |= (UINT8)(version&0xf)<<4;

        *off &= ~0xf;
        *off++ |= (UINT8)(reserved&0xf);

        *off++ = (UINT8)(rsid>>24); 
        *off++ = (UINT8)(rsid>>16); 
        *off++ = (UINT8)(rsid>>8); 
        *off++ = (UINT8)(rsid);
    }

    if (data.data) 
    {
        memcpy(off, data.data, data.len);
    }
    off += data.len;
    len = off-buf;

    return off;
}

inline UINT8*
TNGACKPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    lost_high = (*off>>6)&1;

    dummy = (*off&0x3e)>>1;

    extension_flag = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    if (length_included_flag == 1)
    {
        length = *off++<<8; 
        length |= *off++;
    }

    if (extension_flag == 1)
    {
        version = (*off&0xf0)>>4;

        reserved = *off++&0xf;

        rsid = ((UINT32)*off++)<<24;
        rsid |= ((UINT32)*off++)<<16;
        rsid |= ((UINT32)*off++)<<8;
        rsid |= ((UINT32)*off++);
    }

    data.len = len - (off - buf);
    if (off-buf+data.len > len)
    {
        return 0;
    }
    data.data = (INT8*)off;
    off += data.len;

    return off;
}


/* PMC DEFINITION
struct TNGRTTRequestPacket
{
    bit[1]  dummy0;                 // 0
    bit[5]  dummy1;                 // 0
    bit[1]  dummy2;                 // 0
    bit[1]  dummy3;                 // 0
    u_int16 packet_type;            // 0xff03
}
 */

class TNGRTTRequestPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 3;}

    UINT8   dummy0;
    UINT8   dummy1;
    UINT8   dummy2;
    UINT8   dummy3;
    UINT16  packet_type;
};

inline UINT8*
TNGRTTRequestPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7);
    *off |= (dummy0&1)<<7;
    *off &= ~0x7c; 
    *off |= (dummy1&0x1f)<<2;
    *off &= ~(1<<1); 
    *off |= (dummy2&1)<<1;
    *off &= ~1; 
    *off++ |= dummy3&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    len = off-buf;

    return off;
}

inline UINT8*
TNGRTTRequestPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    dummy0 = (*off>>7)&1;
    dummy1 = (*off&0x7c)>>2;
    dummy2 = (*off>>1)&1;
    dummy3 = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    return off;
}


/*
struct TNGRTTResponsePacket
{
    bit[1]  dummy0;                 // 0
    bit[5]  dummy1;                 // 0
    bit[1]  dummy2;                 // 0
    bit[1]  dummy3;                 // 0
    u_int16 packet_type;            // 0xff04
    u_int32 timestamp_sec;
    u_int32 timestamp_usec;
}
 */

class TNGRTTResponsePacket
{
public:
    UINT8*    pack(UINT8* buf, UINT32 &len);
    UINT8*    unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 11;}

    UINT8    dummy0;
    UINT8    dummy1;
    UINT8    dummy2;
    UINT8    dummy3;
    UINT16    packet_type;
    UINT32    timestamp_sec;
    UINT32    timestamp_usec;
};

inline UINT8*
TNGRTTResponsePacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7);
    *off |= (dummy0&1)<<7;
    *off &= ~0x7c; 
    *off |= (dummy1&0x1f)<<2;
    *off &= ~(1<<1);
    *off |= (dummy2&1)<<1;
    *off &= ~1; 
    *off++ |= dummy3&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    *off++ = (UINT8)(timestamp_sec>>24); 
    *off++ = (UINT8)(timestamp_sec>>16); 
    *off++ = (UINT8)(timestamp_sec>>8); 
    *off++ = (UINT8)(timestamp_sec);


    *off++ = (UINT8)(timestamp_usec>>24); 
    *off++ = (UINT8)(timestamp_usec>>16); 
    *off++ = (UINT8)(timestamp_usec>>8); 
    *off++ = (UINT8)(timestamp_usec);

    len = off-buf;

    return off;
}

inline UINT8*
TNGRTTResponsePacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    dummy0 = (*off>>7)&1;
    dummy1 = (*off&0x7c)>>2;
    dummy2 = (*off>>1)&1;
    dummy3 = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    timestamp_sec = ((UINT32)*off++)<<24;
    timestamp_sec |= ((UINT32)*off++)<<16;
    timestamp_sec |= ((UINT32)*off++)<<8; 
    timestamp_sec |= ((UINT32)*off++);

    timestamp_usec = ((UINT32)*off++)<<24;
    timestamp_usec |= ((UINT32)*off++)<<16;
    timestamp_usec |= ((UINT32)*off++)<<8;
    timestamp_usec |= ((UINT32)*off++);

    return off;
}


/* PMC DEFINITION
struct TNGCongestionPacket
{
    bit[1]  dummy0;                 // 0
    bit[5]  dummy1;                 // 0
    bit[1]  dummy2;                 // 0
    bit[1]  dummy3;                 // 0
    u_int16 packet_type;            // 0xff05
    int32   xmit_multiplier;
    int32   recv_multiplier;
}
 */

class TNGCongestionPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 11;}

    UINT8   dummy0;
    UINT8   dummy1;
    UINT8   dummy2;
    UINT8   dummy3;
    UINT16  packet_type;
    INT32   xmit_multiplier;
    INT32   recv_multiplier;
};

inline UINT8*
TNGCongestionPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (dummy0&1)<<7;
    *off &= ~0x7c; 
    *off |= (dummy1&0x1f)<<2;
    *off &= ~(1<<1); 
    *off |= (dummy2&1)<<1;
    *off &= ~1; 
    *off++ |= dummy3&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    *off++ = (UINT8)(xmit_multiplier>>24);
    *off++ = (UINT8)(xmit_multiplier>>16); 
    *off++ = (UINT8)(xmit_multiplier>>8); 
    *off++ = (UINT8)(xmit_multiplier);

    *off++ = (UINT8)(recv_multiplier>>24); 
    *off++ = (UINT8)(recv_multiplier>>16); 
    *off++ = (UINT8)(recv_multiplier>>8); 
    *off++ = (UINT8)(recv_multiplier);

    len = off-buf;

    return off;
}

inline UINT8*
TNGCongestionPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    dummy0 = (*off>>7)&1;
    dummy1 = (*off&0x7c)>>2;
    dummy2 = (*off>>1)&1;
    dummy3 = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    xmit_multiplier = ((INT32)*off++)<<24; 
    xmit_multiplier |= ((INT32)*off++)<<16;
    xmit_multiplier |= ((INT32)*off++)<<8;
    xmit_multiplier |= ((INT32)*off++);


    recv_multiplier = ((INT32)*off++)<<24; 
    recv_multiplier |= ((INT32)*off++)<<16;
    recv_multiplier |= ((INT32)*off++)<<8;
    recv_multiplier |= ((INT32)*off++);

    return off;
}


/* PMC DEFINITION
struct TNGStreamEndPacket
{
    bit[1]  need_reliable_flag;
    bit[5]  stream_id;
    bit[1]  packet_sent;            // 0
    bit[1]  ext_flag;
    u_int16 packet_type;            // 0xff06 or 0xff0c
    u_int16 seq_no;
    u_int32 timestamp;
    if (packet_type == 0xff0c)
    {
        bit[4]  version;             // 4 (0100)
        bit[4]  reserved;
        u_int32 rsid;
    }
    if (stream_id == 31)
    {
        u_int16 stream_id_expansion;
    }
    if (need_reliable_flag == 1)
    {
        u_int16 total_reliable;
    }
    // reason for stream end (optional): requires RDT v3
    if (ext_flag)
    {
        u_int8[3]   reason_dummy;   // 0xff 0xff 0xff
        u_int32 reason_code;
        string  reason_text;
    }
}
 */

class TNGStreamEndPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 25;}

    UINT8   need_reliable_flag;
    UINT8   stream_id;
    UINT8   packet_sent;
    UINT8   ext_flag;
    UINT16  packet_type;
    UINT16  seq_no;
    UINT32  timestamp;
    UINT8   version;
    UINT8   reserved;
    UINT32  rsid;
    UINT16  stream_id_expansion;
    UINT16  total_reliable;
    UINT8   reason_dummy[3];
    UINT32  reason_code;
    char*   reason_text;
};

inline UINT8*
TNGStreamEndPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (need_reliable_flag&1)<<7;

    *off &= ~0x7c; 
    *off |= (stream_id&0x1f)<<2;

    *off &= ~(1<<1); 
    *off |= (packet_sent&1)<<1;

    *off &= ~1; 
    *off++ |= ext_flag&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    *off++ = (UINT8)(seq_no>>8); 
    *off++ = (UINT8)(seq_no);

    *off++ = (UINT8)(timestamp>>24); 
    *off++ = (UINT8)(timestamp>>16); 
    *off++ = (UINT8)(timestamp>>8);
    *off++ = (UINT8)(timestamp);

    if (packet_type == 0xff0c)
    {
        *off &= ~0xf0;
        *off |= (UINT8)(version&0xf)<<4;

        *off &= ~0xf;
        *off++ |= (UINT8)(reserved&0xf);

        *off++ = (UINT8)(rsid>>24); 
        *off++ = (UINT8)(rsid>>16); 
        *off++ = (UINT8)(rsid>>8); 
        *off++ = (UINT8)(rsid);
    }

    if (stream_id == 31)
    {
       *off++ = (UINT8)(stream_id_expansion>>8); 
       *off++ = (UINT8)(stream_id_expansion);
    }

    if (need_reliable_flag == 1)
    {
       *off++ = (UINT8)(total_reliable>>8); 
       *off++ = (UINT8)(total_reliable);
    }

    if (ext_flag)
    {
        memcpy(off, reason_dummy, 3); 
        off += 3; 

        *off++ = (UINT8)(reason_code>>24);
        *off++ = (UINT8)(reason_code>>16); 
        *off++ = (UINT8)(reason_code>>8); 
        *off++ = (UINT8)(reason_code);

        int _len = strlen(reason_text); 
        *off++ = _len>>8; 
        *off++ = _len;
        memcpy(off, reason_text, _len); 
        off += _len;
    }

    len = off-buf;

    return off;
}

inline UINT8*
TNGStreamEndPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    need_reliable_flag = (*off>>7)&1;

    stream_id  = (*off&0x7c)>>2;

    packet_sent = (*off>>1)&1;

    ext_flag = *off++&1;

    packet_type = *off++<<8;
    packet_type |= *off++;

    seq_no = *off++<<8;
    seq_no |= *off++;

    timestamp = ((UINT32)*off++)<<24; 
    timestamp |= ((UINT32)*off++)<<16;
    timestamp |= ((UINT32)*off++)<<8;
    timestamp |= ((UINT32)*off++);

    if (packet_type == 0xff0c)
    {
        version = (*off&0xf0)>>4;

        reserved = *off++&0xf;

        rsid = ((UINT32)*off++)<<24;
        rsid |= ((UINT32)*off++)<<16;
        rsid |= ((UINT32)*off++)<<8;
        rsid |= ((UINT32)*off++);
    }

    if (stream_id == 31)
    {
        stream_id_expansion = *off++<<8; 
        stream_id_expansion |= *off++;
    }

    if (need_reliable_flag == 1)
    {
        total_reliable = *off++<<8;
        total_reliable |= *off++;
    }

    if (ext_flag)
    {
        if ((UINT32)(off-buf+3) > len)
        {
            return 0;
        }

        memcpy(reason_dummy, off, 3); 
        off += 3; 

        reason_code = ((UINT32)*off++)<<24; 
        reason_code |= ((UINT32)*off++)<<16;
        reason_code |= ((UINT32)*off++)<<8;
        reason_code |= ((UINT32)*off++);

        int _len = *off++>>8;
        _len |= *off++;

        if (off-buf+_len > len)
        {
            return 0;
        }

        reason_text = new char[_len+1];
        memcpy(reason_text, off, _len); 
        off += _len; 
        reason_text[_len] = 0;
    }

    return off;
}


/* PMC DEFINITION
struct TNGLatencyReportPacket
{
    bit[1]  length_included_flag;
    bit[5]  dummy0;                 // 0
    bit[1]  dummy1;                 // 0
    bit[1]  dummy2;                 // 0
    u_int16 packet_type;            // 0xff08
    if (length_included_flag == 1)
    {
        u_int16 length;
    }
    u_int32 server_out_time;
}
 */

class TNGLatencyReportPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 9;}

    UINT8   length_included_flag;
    UINT8   dummy0;
    UINT8   dummy1;
    UINT8   dummy2;
    UINT16  packet_type;
    UINT16  length;
    UINT32  server_out_time;
};

inline UINT8*
TNGLatencyReportPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (length_included_flag&1)<<7;

    *off &= ~0x7c; 
    *off |= (dummy0&0x1f)<<2;
    *off &= ~(1<<1); 
    *off |= (dummy1&1)<<1;
    *off &= ~1;
    *off++ |= dummy2&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    if (length_included_flag == 1)
    {
        *off++ = (UINT8)(length>>8); 
        *off++ = (UINT8)(length);
    }

    *off++ = (UINT8)(server_out_time>>24); 
    *off++ = (UINT8)(server_out_time>>16); 
    *off++ = (UINT8)(server_out_time>>8); 
    *off++ = (UINT8)(server_out_time);

    len = off-buf;

    return off;
}

inline UINT8*
TNGLatencyReportPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    dummy0 = (*off&0x7c)>>2;
    dummy1 = (*off>>1)&1;
    dummy2 = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    if (length_included_flag == 1)
    {
        length = *off++<<8;
        length |= *off++;
    }

    server_out_time = ((UINT32)*off++)<<24;
    server_out_time |= ((UINT32)*off++)<<16;
    server_out_time |= ((UINT32)*off++)<<8; 
    server_out_time |= ((UINT32)*off++);

    return off;
}


/*
 * RDTFeatureLevel 3 packets
 */


/* PMC DEFINITION
struct RDTTransportInfoRequestPacket
{
    bit[1]                 // 0
    bit[5]                 // 0
    bit[1]  request_rtt_info;
    bit[1]  request_buffer_info;
    u_int16 packet_type;   // 0xff09
 
    if (request_rtt_info == 1)
    {
        u_int32 request_time_ms;
    }
 }
 */

class RDTTransportInfoRequestPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 7;}

    UINT8   dummy0;
    UINT8   dummy1;
    UINT8   request_rtt_info;
    UINT8   request_buffer_info;
    UINT16  packet_type;
    UINT32  request_time_ms;
};

inline UINT8*
RDTTransportInfoRequestPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7);
    *off |= (dummy0&1)<<7;
    *off &= ~0x7c; 
    *off |= (dummy1&0x1f)<<2;

    *off &= ~(1<<1); 
    *off |= (request_rtt_info&1)<<1;

    *off &= ~1; 
    *off++ |= request_buffer_info&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    if (request_rtt_info == 1)
    {    
        *off++ = (UINT8)(request_time_ms>>24); 
        *off++ = (UINT8)(request_time_ms>>16); 
        *off++ = (UINT8)(request_time_ms>>8); 
        *off++ = (UINT8)(request_time_ms);
    }

    len = off-buf;

    return off;
}

inline UINT8*
RDTTransportInfoRequestPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    dummy0 = (*off>>7)&1;
    dummy1 = (*off&0x7c)>>2;

    request_rtt_info = (*off>>1)&1;

    request_buffer_info = *off++&1;

    packet_type = *off++<<8;
    packet_type |= *off++;

    if (request_rtt_info == 1)
    {
        request_time_ms = ((UINT32)*off++)<<24;
        request_time_ms |= ((UINT32)*off++)<<16;
        request_time_ms |= ((UINT32)*off++)<<8;
        request_time_ms |= ((UINT32)*off++);
    }

    return off;
}


/* PMC DEFINITION
struct RDTBufferInfo
{
   u_int16  stream_id;
   u_int32  lowest_timestamp;
   u_int32  highest_timestamp;
   u_int32  bytes_buffered;
}
 */

class RDTBufferInfo
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 14;}

    UINT16  stream_id;
    UINT32  lowest_timestamp;
    UINT32  highest_timestamp;
    UINT32  bytes_buffered;
};

inline UINT8*
RDTBufferInfo::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off++ = (UINT8)(stream_id>>8); 
    *off++ = (UINT8)(stream_id);

    *off++ = (UINT8)(lowest_timestamp>>24); 
    *off++ = (UINT8)(lowest_timestamp>>16); 
    *off++ = (UINT8)(lowest_timestamp>>8); 
    *off++ = (UINT8)(lowest_timestamp);

    *off++ = (UINT8)(highest_timestamp>>24); 
    *off++ = (UINT8)(highest_timestamp>>16); 
    *off++ = (UINT8)(highest_timestamp>>8); 
    *off++ = (UINT8)(highest_timestamp);

    *off++ = (UINT8)(bytes_buffered>>24); 
    *off++ = (UINT8)(bytes_buffered>>16); 
    *off++ = (UINT8)(bytes_buffered>>8); 
    *off++ = (UINT8)(bytes_buffered);

    len = off-buf;

    return off;
}

inline UINT8*
RDTBufferInfo::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    stream_id = *off++<<8; 
    stream_id |= *off++;

    lowest_timestamp = ((UINT32)*off++)<<24; 
    lowest_timestamp |= ((UINT32)*off++)<<16;
    lowest_timestamp |= ((UINT32)*off++)<<8; 
    lowest_timestamp |= ((UINT32)*off++);

    highest_timestamp = ((UINT32)*off++)<<24;
    highest_timestamp |= ((UINT32)*off++)<<16;
    highest_timestamp |= ((UINT32)*off++)<<8; 
    highest_timestamp |= ((UINT32)*off++);

    bytes_buffered = ((UINT32)*off++)<<24;
    bytes_buffered |= ((UINT32)*off++)<<16;
    bytes_buffered |= ((UINT32)*off++)<<8; 
    bytes_buffered |= ((UINT32)*off++);

    return off;
}


/* PMC DEFINITION
struct RDTTransportInfoResponsePacket
{
   bit[1]   extension_flag;
   bit[4]   dummy;                  // 0
   bit[1]   has_rtt_info;
   bit[1]   is_delayed; 
   bit[1]   has_buffer_info;
   u_int16  packet_type;            // 0xff0a

   if (has_rtt_info == 1)
   {
       u_int32  request_time_ms;

       if (is_delayed)
       {
           u_int32  response_time_ms;
       }
   }
   if (extension_flag == 1)
   {
       bit[4]   version;             // 4 (0100)
       bit[4]   reserved;
       u_int32  rsid;
   }
       if (has_buffer_info == 1)
       {
       u_int16 buffer_info_count;
       RDTBufferInfo[buffer_info_count] buffer_info;
   }
}
 */

class RDTTransportInfoResponsePacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 18;}

    UINT8   extension_flag;
    UINT8   dummy;
    UINT8   has_rtt_info;
    UINT8   is_delayed;
    UINT8   has_buffer_info;
    UINT16  packet_type;
    UINT32  request_time_ms;
    UINT32  response_time_ms;
    UINT8   version;
    UINT8   reserved;
    UINT32  rsid;
    UINT16  buffer_info_count;
    RDTBufferInfo    *buffer_info;
};

inline UINT8*
RDTTransportInfoResponsePacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (extension_flag&1)<<7;

    *off &= ~0x78; 
    *off |= (dummy&0xf)<<3;

    *off &= ~(1<<2); 
    *off |= (has_rtt_info&1)<<2;

    *off &= ~(1<<1); 
    *off |= (is_delayed&1)<<1;

    *off &= ~1; 
    *off++ |= has_buffer_info&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    if (has_rtt_info == 1)
    {
        *off++ = (UINT8)(request_time_ms>>24); 
        *off++ = (UINT8)(request_time_ms>>16); 
        *off++ = (UINT8)(request_time_ms>>8); 
        *off++ = (UINT8)(request_time_ms);

        if (is_delayed)
        {
            *off++ = (UINT8)(response_time_ms>>24); 
            *off++ = (UINT8)(response_time_ms>>16); 
            *off++ = (UINT8)(response_time_ms>>8); 
            *off++ = (UINT8)(response_time_ms);
        }
    }

    if (extension_flag == 1)
    {
        *off &= ~0xf0;
        *off |= (UINT8)(version&0xf)<<4;

        *off &= ~0xf;
        *off++ |= (UINT8)(reserved&0xf);

        *off++ = (UINT8)(rsid>>24); 
        *off++ = (UINT8)(rsid>>16); 
        *off++ = (UINT8)(rsid>>8); 
        *off++ = (UINT8)(rsid);
    }

        if (has_buffer_info == 1)
        {
        *off++ = (UINT8)(buffer_info_count>>8); 
        *off++ = (UINT8)(buffer_info_count);

        for (int i = 0;  i < buffer_info_count; i++)
        {
            off = buffer_info[i].pack(off, len);
        }
    }

    len = off-buf;

    return off;
}

inline UINT8*
RDTTransportInfoResponsePacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    extension_flag = (*off>>7)&1;

    dummy = (*off&0x78)>>3;

    has_rtt_info = (*off>>2)&1;

    is_delayed = (*off>>1)&1;

    has_buffer_info = *off++&1;

    packet_type = *off++<<8;
    packet_type |= *off++;

    if (has_rtt_info == 1)
    {
        request_time_ms = ((UINT32)*off++)<<24;
        request_time_ms |= ((UINT32)*off++)<<16;
        request_time_ms |= ((UINT32)*off++)<<8;
        request_time_ms |= ((UINT32)*off++);

        if (is_delayed)
        {
            response_time_ms = ((UINT32)*off++)<<24; 
            response_time_ms |= ((UINT32)*off++)<<16;
            response_time_ms |= ((UINT32)*off++)<<8;
            response_time_ms |= ((UINT32)*off++);
        }
    }

    if (extension_flag == 1)
    {
        version = (*off&0xf0)>>4;

        reserved = *off++&0xf;

        rsid = ((UINT32)*off++)<<24;
        rsid |= ((UINT32)*off++)<<16;
        rsid |= ((UINT32)*off++)<<8;
        rsid |= ((UINT32)*off++);
    }

        if (has_buffer_info == 1)
        {
        buffer_info_count = *off++<<8;
        buffer_info_count |= *off++;

        buffer_info = new RDTBufferInfo[buffer_info_count];

        for (int i = 0;  i < buffer_info_count; i++)
        {
            off = buffer_info[i].unpack(off, len);
        } 
    }
    return off;
}


/* PMC DEFINITION
struct TNGBWProbingPacket
{
    bit[1]  length_included_flag;
    bit[5]  dummy0;                 // 0
    bit[1]  dummy1;                 // 0
    bit[1]  dummy2;                 // 0
    u_int16 packet_type;            // 0xff0b
    if (length_included_flag == 1)
    {
       u_int16  length;
    }
    u_int8  seq_no;
    u_int32 timestamp;
    buffer  data;
}
 */

class TNGBWProbingPacket
{
public:
    UINT8*  pack(UINT8* buf, UINT32 &len);
    UINT8*  unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 10;}

    UINT8   length_included_flag;
    UINT8   dummy0;
    UINT8   dummy1;
    UINT8   dummy2;
    UINT16  packet_type;
    UINT16  length;
    UINT8   seq_no;
    UINT32  timestamp;
    buffer  data;
};

inline UINT8*
TNGBWProbingPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off &= ~(1<<7); 
    *off |= (length_included_flag&1)<<7;

    *off &= ~0x7c;
    *off |= (dummy0&0x1f)<<2;
    *off &= ~(1<<1); 
    *off |= (dummy1&1)<<1;
    *off &= ~1; 
    *off++ |= dummy2&1;

    *off++ = (UINT8)(packet_type>>8); 
    *off++ = (UINT8)(packet_type);

    if (length_included_flag == 1)
    {
        *off++ = (UINT8)(length>>8);
        *off++ = (UINT8)(length);
    }

    *off++ = seq_no;

    *off++ = (UINT8)(timestamp>>24);
    *off++ = (UINT8)(timestamp>>16);
    *off++ = (UINT8)(timestamp>>8); 
    *off++ = (UINT8)(timestamp);


    if (data.data)
    {
        memcpy(off, data.data, data.len);
    }
    off += data.len;
    len = off-buf;

    return off;
}

inline UINT8*
TNGBWProbingPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
    {
        return 0;
    }

    UINT8* off = buf;

    length_included_flag = (*off>>7)&1;

    dummy0  = (*off&0x7c)>>2;
    dummy1 = (*off>>1)&1;
    dummy2 = *off++&1;

    packet_type = *off++<<8; 
    packet_type |= *off++;

    if (length_included_flag == 1)
    {
        length = *off++<<8; 
        length |= *off++;
    }

    seq_no = *off++;

    timestamp = ((UINT32)*off++)<<24; 
    timestamp |= ((UINT32)*off++)<<16;
    timestamp |= ((UINT32)*off++)<<8;
    timestamp |= ((UINT32)*off++);

    data.len = len - (off - buf);
    if (off-buf+data.len > len)
    {
        return 0;
    }
    data.data = (INT8*)off; 
    off += data.len;

    return off;
}

#endif /* _TNGPKT_H_ */
