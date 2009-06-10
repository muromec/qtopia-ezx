/*
 * This is generated code, do not modify. Look in tngpkt.pm to
 * make modifications
 */
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 UINT32	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/


#ifndef _TNGPKT_H_
#define _TNGPKT_H_

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tngpkt.h,v 1.8 2005/09/27 17:23:33 rayala Exp $ 
 *   
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
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

#include "hxinline.h"

/*
 * XXXSMP Before using any of the packets that aren't used yet,
 * check to make sure the format conforms with the DataPacket's
 * first 5 bytes.
 */
class TNGDataPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 16;}

    UINT8	length_included_flag;
    UINT8	need_reliable_flag;
    UINT8	stream_id;
    UINT8	is_reliable;
    UINT16	seq_no;
    UINT16	_packlenwhendone;
    UINT8	back_to_back_packet;
    UINT8	slow_data;
    UINT8	asm_rule_number;
    UINT32	timestamp;
    UINT16	stream_id_expansion;
    UINT16	total_reliable;
    UINT16	asm_rule_number_expansion;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGDataPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    UINT8* pLen = 0;

    {*off &= (UINT8)(~(1 << 7)); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {*off &= (UINT8)(~(1 << 6)); *off |= (UINT8)((need_reliable_flag & 1) << 6);}
    {
	*off &= (UINT8)(~0x3e); *off |= (UINT8)((stream_id & 0x1f) << 1);
    }
    {*off &= (UINT8)~1; *off++ |= (UINT8)(is_reliable & 1);}
    {*off++ = (UINT8) (seq_no>>8); *off++ = (UINT8) (seq_no);}
    if ((length_included_flag == 1))
    {
      pLen = off;
      off += 2;
      #define PMC_PACK_LENGTH(off,_packlenwhendone) {*off++ = (UINT8) (_packlenwhendone>>8); *off++ = (UINT8) (_packlenwhendone);}
    }
    {*off &= (UINT8)(~(1 << 7)); *off |= (UINT8)((back_to_back_packet & 1) << 7);}
    {*off &= (UINT8)(~(1 << 6)); *off |= (UINT8)((slow_data & 1) << 6);}
    {
	*off &= (UINT8)~0x3f; *off++ |= (UINT8)(asm_rule_number & 0x3f);
    }
    {
	*off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
    }
    if ((stream_id == 31))
    {
      {*off++ = (UINT8) (stream_id_expansion>>8); *off++ = (UINT8) (stream_id_expansion);}
    }
    if ((need_reliable_flag == 1))
    {
      {*off++ = (UINT8) (total_reliable>>8); *off++ = (UINT8) (total_reliable);}
    }
    if ((asm_rule_number == 63))
    {
      {*off++ = (UINT8) (asm_rule_number_expansion>>8); *off++ = (UINT8) (asm_rule_number_expansion);}
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off-buf);
    if (pLen)
    {
        PMC_PACK_LENGTH(pLen,len);
    }
    #undef PMC_PACK_LENGTH
    return off;
}

HX_INLINE UINT8*
TNGDataPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    need_reliable_flag = (UINT8)((*off >> 6) & 1);
    {
	stream_id  = (UINT8)((*off & 0x3e) >> 1);
    }
    is_reliable = (UINT8)(*off++ & 1);
    {seq_no = (UINT16)(*off++ << 8); seq_no |= *off++;}
    if ((length_included_flag == 1))
    {
      {_packlenwhendone = (UINT16)(*off++ << 8); _packlenwhendone |= *off++;}
    }
    back_to_back_packet = (UINT8)((*off >> 7) & 1);
    slow_data = (UINT8)((*off >> 6) & 1);
    {
	asm_rule_number  = (UINT8)(*off++ & 0x3f);
    }
    {
	timestamp = GetDwordFromBufAndInc(off);
    }
    if ((stream_id == 31))
    {
      {stream_id_expansion = (UINT16)(*off++ << 8); stream_id_expansion |= *off++;}
    }
    if ((need_reliable_flag == 1))
    {
      {total_reliable = (UINT16)(*off++ << 8); total_reliable |= *off++;}
    }
    if ((asm_rule_number == 63))
    {
      {asm_rule_number_expansion = (UINT16)(*off++ << 8); asm_rule_number_expansion |= *off++;}
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGMultiCastDataPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 17;}

    UINT8	length_included_flag;
    UINT8	need_reliable_flag;
    UINT8	stream_id;
    UINT8	is_reliable;
    UINT16	seq_no;
    UINT16	length;
    UINT8	back_to_back_packet;
    UINT8	slow_data;
    UINT8	asm_rule_number;
    UINT8	group_seq_no;
    UINT32	timestamp;
    UINT16	stream_id_expansion;
    UINT16	total_reliable;
    UINT16	asm_rule_number_expansion;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGMultiCastDataPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {*off &= (UINT8)~(1 << 6); *off |= (UINT8)((need_reliable_flag & 1) << 6);}
    {
	*off &= (UINT8)~0x3e; *off |= (UINT8)((stream_id & 0x1f) << 1);
    }
    {*off &= (UINT8)~1; *off++ |= (UINT8)(is_reliable & 1);}
    {*off++ = (UINT8) (seq_no>>8); *off++ = (UINT8) (seq_no);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((back_to_back_packet & 1) << 7);}
    {*off &= (UINT8)~(1 << 6); *off |= (UINT8)((slow_data & 1) << 6);}
    {
	*off &= (UINT8)~0x3f; *off++ |= (UINT8)(asm_rule_number & 0x3f);
    }
    *off++ = group_seq_no;
    {
	*off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
    }
    if ((stream_id == 31))
    {
      {*off++ = (UINT8) (stream_id_expansion>>8); *off++ = (UINT8) (stream_id_expansion);}
    }
    if ((need_reliable_flag == 1))
    {
      {*off++ = (UINT8) (total_reliable>>8); *off++ = (UINT8) (total_reliable);}
    }
    if ((asm_rule_number == 63))
    {
      {*off++ = (UINT8) (asm_rule_number_expansion>>8); *off++ = (UINT8) (asm_rule_number_expansion);}
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
TNGMultiCastDataPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    need_reliable_flag = (UINT8)((*off >> 6) & 1);
    {
	stream_id  = (UINT8)((*off & 0x3e) >> 1);
    }
    is_reliable = (UINT8)(*off++ & 1);
    {seq_no = (UINT16)(*off++ << 8); seq_no |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    back_to_back_packet = (UINT8)((*off >> 7) & 1);
    slow_data = (UINT8)((*off >> 6) & 1);
    {
	asm_rule_number  = (UINT8)(*off++ & 0x3f);
    }
    group_seq_no = *off++;
    {
	timestamp = GetDwordFromBufAndInc(off);
    }
    if ((stream_id == 31))
    {
      {stream_id_expansion = (UINT16)(*off++ << 8); stream_id_expansion |= *off++;}
    }
    if ((need_reliable_flag == 1))
    {
      {total_reliable = (UINT16)(*off++ << 8); total_reliable |= *off++;}
    }
    if ((asm_rule_number == 63))
    {
      {asm_rule_number_expansion = (UINT16)(*off++ << 8); asm_rule_number_expansion |= *off++;}
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGASMActionPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 9;}

    UINT8	length_included_flag;
    UINT8	stream_id;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT16	packet_type;
    UINT16	reliable_seq_no;
    UINT16	length;
    UINT16	stream_id_expansion;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGASMActionPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((stream_id & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1<<1); *off |= (UINT8)((dummy0 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy1 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    {*off++ = (UINT8) (reliable_seq_no>>8); *off++ = (UINT8) (reliable_seq_no);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    if ((stream_id == 31))
    {
      {*off++ = (UINT8) (stream_id_expansion>>8); *off++ = (UINT8) (stream_id_expansion);}
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
TNGASMActionPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    {
	stream_id  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy0 = (UINT8)((*off >> 1) & 1);
    dummy1 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    {reliable_seq_no = (UINT16)(*off++ << 8); reliable_seq_no |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    if ((stream_id == 31))
    {
      {stream_id_expansion = (UINT16)(*off++ << 8); stream_id_expansion |= *off++;}
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGBandwidthReportPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 12;}

    UINT8	length_included_flag;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT16	packet_type;
    UINT16	length;
    UINT16	interval;
    UINT32	bandwidth;
    UINT8	sequence;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGBandwidthReportPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy0 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1<<1); *off |= (UINT8)((dummy1 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy2 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    {*off++ = (UINT8) (interval>>8); *off++ = (UINT8) (interval);}
    {
	*off++ = (UINT8) (bandwidth>>24); *off++ = (UINT8) (bandwidth>>16); *off++ = (UINT8) (bandwidth>>8); *off++ = (UINT8) (bandwidth);
    }
    *off++ = sequence;
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGBandwidthReportPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    {
	dummy0  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy1 = (UINT8)((*off >> 1) & 1);
    dummy2 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    {interval = (UINT16)(*off++ << 8); interval |= *off++;}
    {
	bandwidth = GetDwordFromBufAndInc(off);
    }
    sequence = *off++;
    return off;
}
#endif //_DEFINE_INLINE

class TNGReportPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 5;}

    UINT8	length_included_flag;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT16	packet_type;
    UINT16	length;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGReportPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy0 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1<<1); *off |= (UINT8)((dummy1 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy2 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGReportPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    {
	dummy0  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy1 = (UINT8)((*off >> 1) & 1);
    dummy2 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGACKPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 5;}

    UINT8	length_included_flag;
    UINT8	lost_high;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT16	packet_type;
    UINT16	length;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGACKPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {*off &= (UINT8)~(1<<6); *off |= (UINT8)((lost_high & 1) << 6);}
    {
	*off &= (UINT8)~0x3e; *off |= (UINT8)((dummy0 & 0x1f) << 1);
    }
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy1 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGACKPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    lost_high = (UINT8)((*off >> 6) & 1);
    {
	dummy0  = (UINT8)((*off & 0x3e) >> 1);
    }
    dummy1 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGRTTRequestPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 3;}

    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT8	dummy3;
    UINT16	packet_type;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGRTTRequestPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((dummy0 & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy1 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1<<1); *off |= (UINT8)((dummy2 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy3 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGRTTRequestPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    dummy0 = (UINT8)((*off >> 7) & 1);
    {
	dummy1  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy2 = (UINT8)((*off >> 1) & 1);
    dummy3 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    return off;
}
#endif //_DEFINE_INLINE

class TNGRTTResponsePacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 11;}

    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT8	dummy3;
    UINT16	packet_type;
    UINT32	timestamp_sec;
    UINT32	timestamp_usec;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGRTTResponsePacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((dummy0 & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy1 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1<<1); *off |= (UINT8)((dummy2 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy3 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    {
	*off++ = (UINT8) (timestamp_sec>>24); *off++ = (UINT8) (timestamp_sec>>16); *off++ = (UINT8) (timestamp_sec>>8); *off++ = (UINT8) (timestamp_sec);
    }
    {
	*off++ = (UINT8) (timestamp_usec>>24); *off++ = (UINT8) (timestamp_usec>>16); *off++ = (UINT8) (timestamp_usec>>8); *off++ = (UINT8) (timestamp_usec);
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGRTTResponsePacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    dummy0 = (UINT8)((*off >> 7) & 1);
    {
	dummy1  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy2 = (UINT8)((*off >> 1) & 1);
    dummy3 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    {
	timestamp_sec = GetDwordFromBufAndInc(off);
    }
    {
	timestamp_usec = GetDwordFromBufAndInc(off);
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGCongestionPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 11;}

    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT8	dummy3;
    UINT16	packet_type;
    INT32	xmit_multiplier;
    INT32	recv_multiplier;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGCongestionPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((dummy0 & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy1 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((dummy2 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy3 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    {
	*off++ = (UINT8) (xmit_multiplier>>24); *off++ = (UINT8) (xmit_multiplier>>16); *off++ = (UINT8) (xmit_multiplier>>8); *off++ = (UINT8) (xmit_multiplier);
    }
    {
	*off++ = (UINT8) (recv_multiplier>>24); *off++ = (UINT8) (recv_multiplier>>16); *off++ = (UINT8) (recv_multiplier>>8); *off++ = (UINT8) (recv_multiplier);
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
TNGCongestionPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    dummy0 = (UINT8)((*off >> 7) & 1);
    {
	dummy1  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy2 = (UINT8)((*off >> 1) & 1);
    dummy3 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    {
	xmit_multiplier = (INT32)GetDwordFromBufAndInc(off);
    }
    {
	recv_multiplier = (INT32)GetDwordFromBufAndInc(off);
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGStreamEndPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT8	need_reliable_flag;
    UINT8	stream_id;
    UINT8	packet_sent;
    UINT8	ext_flag;
    UINT16	packet_type;
    UINT16	seq_no;
    UINT32	timestamp;
    UINT16	stream_id_expansion;
    UINT16	total_reliable;
    UINT8	reason_dummy[3];
    UINT32	reason_code;
    pmc_string	reason_text;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGStreamEndPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((need_reliable_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((stream_id & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((packet_sent & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(ext_flag & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    {*off++ = (UINT8) (seq_no>>8); *off++ = (UINT8) (seq_no);}
    {
	*off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
    }
    if ((stream_id == 31))
    {
      {*off++ = (UINT8) (stream_id_expansion>>8); *off++ = (UINT8) (stream_id_expansion);}
    }
    if ((need_reliable_flag == 1))
    {
      {*off++ = (UINT8) (total_reliable>>8); *off++ = (UINT8) (total_reliable);}
    }
    if (ext_flag)
    {
      {memcpy(off, reason_dummy, 3); off += 3; }
      {
	  *off++ = (UINT8) (reason_code>>24); *off++ = (UINT8) (reason_code>>16); *off++ = (UINT8) (reason_code>>8); *off++ = (UINT8) (reason_code);
      }
      {
	  int _len = (int)strlen(reason_text); *off++ = (UINT8)(_len >> 8); *off++ = (UINT8)_len;
	  memcpy(off, reason_text, (size_t)_len); off += _len;
      }
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGStreamEndPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    need_reliable_flag = (UINT8)((*off >> 7) & 1);
    {
	stream_id  = (UINT8)((*off & 0x7c) >> 2);
    }
    packet_sent = (UINT8)((*off >> 1) & 1);
    ext_flag = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    {seq_no = (UINT16)(*off++ << 8); seq_no |= *off++;}
    {
	timestamp = GetDwordFromBufAndInc(off);
    }
    if ((stream_id == 31))
    {
      {stream_id_expansion = (UINT16)(*off++ << 8); stream_id_expansion |= *off++;}
    }
    if ((need_reliable_flag == 1))
    {
      {total_reliable = (UINT16)(*off++ << 8); total_reliable |= *off++;}
    }
    if (ext_flag)
    {
      if (off-buf+3 > (int)len)
	  return 0;
      {memcpy(reason_dummy, off, 3); off += 3; }
      {
	  reason_code = ((UINT32)*off++)<<24; reason_code |= ((UINT32)*off++)<<16;
	  reason_code |= ((UINT32)*off++)<<8; reason_code |= ((UINT32)*off++);
      }
      {
	  int _len = *off++>>8; _len |= *off++;
	  if (off-buf+_len > (int)len)
	      return 0;
	  reason_text = new char[_len+1];
	  memcpy(reason_text, off, (size_t)_len); off += _len; reason_text[_len] = 0;
      }
     }
    return off;
}
#endif //_DEFINE_INLINE

class TNGLatencyReportPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 9;}

    UINT8	length_included_flag;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT16	packet_type;
    UINT16	length;
    UINT32	server_out_time;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGLatencyReportPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy0 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((dummy1 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy2 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    {
	*off++ = (UINT8) (server_out_time>>24); *off++ = (UINT8) (server_out_time>>16); *off++ = (UINT8) (server_out_time>>8); *off++ = (UINT8) (server_out_time);
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGLatencyReportPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    {
	dummy0  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy1 = (UINT8)((*off >> 1) & 1);
    dummy2 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    {
	server_out_time = GetDwordFromBufAndInc(off);
    }
    return off;
}
#endif //_DEFINE_INLINE

    /*
     * RDTFeatureLevel 3 packets
     */

class RDTTransportInfoRequestPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 7;}

    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	request_rtt_info;
    UINT8	request_buffer_info;
    UINT16	packet_type;
    UINT32	request_time_ms;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
RDTTransportInfoRequestPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((dummy0 & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy1 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((request_rtt_info & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(request_buffer_info & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((request_rtt_info == 1))
    {
      {
	  *off++ = (UINT8) (request_time_ms>>24); *off++ = (UINT8) (request_time_ms>>16); *off++ = (UINT8) (request_time_ms>>8); *off++ = (UINT8) (request_time_ms);
      }
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
RDTTransportInfoRequestPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    dummy0 = (UINT8)((*off >> 7) & 1);
    {
	dummy1  = (UINT8)((*off & 0x7c) >> 2);
    }
    request_rtt_info = (UINT8)((*off >> 1) & 1);
    request_buffer_info = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((request_rtt_info == 1))
    {
      {
	  request_time_ms = GetDwordFromBufAndInc(off);
      }
    }
    return off;
}
#endif //_DEFINE_INLINE

class RDTBufferInfo
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 14;}

    UINT16	stream_id;
    UINT32	lowest_timestamp;
    UINT32	highest_timestamp;
    UINT32	bytes_buffered;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
RDTBufferInfo::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off++ = (UINT8) (stream_id>>8); *off++ = (UINT8) (stream_id);}
    {
	*off++ = (UINT8) (lowest_timestamp>>24); *off++ = (UINT8) (lowest_timestamp>>16); *off++ = (UINT8) (lowest_timestamp>>8); *off++ = (UINT8) (lowest_timestamp);
    }
    {
	*off++ = (UINT8) (highest_timestamp>>24); *off++ = (UINT8) (highest_timestamp>>16); *off++ = (UINT8) (highest_timestamp>>8); *off++ = (UINT8) (highest_timestamp);
    }
    {
	*off++ = (UINT8) (bytes_buffered>>24); *off++ = (UINT8) (bytes_buffered>>16); *off++ = (UINT8) (bytes_buffered>>8); *off++ = (UINT8) (bytes_buffered);
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
RDTBufferInfo::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    {stream_id = (UINT16)(*off++ << 8); stream_id |= *off++;}
    {
	lowest_timestamp = GetDwordFromBufAndInc(off);
    }
    {
	highest_timestamp = GetDwordFromBufAndInc(off);
    }
    {
	bytes_buffered = GetDwordFromBufAndInc(off);
    }
    return off;
}
#endif //_DEFINE_INLINE

class RDTTransportInfoResponsePacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 13;}

    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	has_rtt_info;
    UINT8	is_delayed;
    UINT8	has_buffer_info;
    UINT16	packet_type;
    UINT32	request_time_ms;
    UINT32	response_time_ms;
    UINT16	buffer_info_count;
    RDTBufferInfo	*buffer_info;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
RDTTransportInfoResponsePacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((dummy0 & 1) << 7);}
    {
	*off &= (UINT8)~0x78; *off |= (UINT8)((dummy1 & 0xf) << 3);
    }
    {*off &= (UINT8)~(1 << 2); *off |= (UINT8)((has_rtt_info & 1) << 2);}
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((is_delayed & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(has_buffer_info & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((has_rtt_info == 1))
    {
      {
	  *off++ = (UINT8) (request_time_ms>>24); *off++ = (UINT8) (request_time_ms>>16); *off++ = (UINT8) (request_time_ms>>8); *off++ = (UINT8) (request_time_ms);
      }
      if (is_delayed)
      {
	{
	    *off++ = (UINT8) (response_time_ms>>24); *off++ = (UINT8) (response_time_ms>>16); *off++ = (UINT8) (response_time_ms>>8); *off++ = (UINT8) (response_time_ms);
	}
      }
    }
    if ((has_buffer_info == 1))
    {
      {*off++ = (UINT8) (buffer_info_count>>8); *off++ = (UINT8) (buffer_info_count);}
      {for (int i = 0;  i < buffer_info_count; i++)
	  off = buffer_info[i].pack(off, len);
      }
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
RDTTransportInfoResponsePacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    dummy0 = (UINT8)((*off >> 7) & 1);
    {
	dummy1  = (UINT8)((*off & 0x78) >> 3);
    }
    has_rtt_info = (UINT8)((*off >> 2) & 1);
    is_delayed = (UINT8)((*off >> 1) & 1);
    has_buffer_info = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((has_rtt_info == 1))
    {
      {
	  request_time_ms = GetDwordFromBufAndInc(off);
      }
      if (is_delayed)
      {
	{
	    response_time_ms = GetDwordFromBufAndInc(off);
	}
      }
    }
    if ((has_buffer_info == 1))
    {
      {buffer_info_count = (UINT16)(*off++ << 8); buffer_info_count |= *off++;}
      {
	  buffer_info = new RDTBufferInfo[buffer_info_count];
	  for (int i = 0;  i < buffer_info_count; i++)
	  off = buffer_info[i].unpack(off, len);
      }
    }
    return off;
}
#endif //_DEFINE_INLINE

class TNGBWProbingPacket
{
public:
    UINT8*	    pack(UINT8* buf, UINT32 &len);
    UINT8*	    unpack(UINT8* buf, UINT32 len);
    const UINT32    static_size() {return 10;}

    UINT8	length_included_flag;
    UINT8	dummy0;
    UINT8	dummy1;
    UINT8	dummy2;
    UINT16	packet_type;
    UINT16	length;    
    UINT8	seq_no;
    UINT32	timestamp;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
TNGBWProbingPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off &= (UINT8)~(1 << 7); *off |= (UINT8)((length_included_flag & 1) << 7);}
    {
	*off &= (UINT8)~0x7c; *off |= (UINT8)((dummy0 & 0x1f) << 2);
    }
    {*off &= (UINT8)~(1 << 1); *off |= (UINT8)((dummy1 & 1) << 1);}
    {*off &= (UINT8)~1; *off++ |= (UINT8)(dummy2 & 1);}
    {*off++ = (UINT8) (packet_type>>8); *off++ = (UINT8) (packet_type);}
    if ((length_included_flag == 1))
    {
      {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    }
    *off++ = seq_no;
    {
	*off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
    }
    {
	if (data.data) memcpy(off, data.data, data.len); off += data.len;
    }
    len = (UINT32)(off - buf);
    return off;
}

HX_INLINE UINT8*
TNGBWProbingPacket::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    length_included_flag = (UINT8)((*off >> 7) & 1);
    {
	dummy0  = (UINT8)((*off & 0x7c) >> 2);
    }
    dummy1 = (UINT8)((*off >> 1) & 1);
    dummy2 = (UINT8)(*off++ & 1);
    {packet_type = (UINT16)(*off++ << 8); packet_type |= *off++;}
    if ((length_included_flag == 1))
    {
      {length = (UINT16)(*off++ << 8); length |= *off++;}
    }
    seq_no = *off++;
    {
	timestamp = ((UINT32)*off++)<<24; timestamp |= ((UINT32)*off++)<<16;
	timestamp |= ((UINT32)*off++)<<8; timestamp |= ((UINT32)*off++);
    }
    {
	data.len = len - (off - buf);
	if (off-buf+data.len > len)
	    return 0;
	data.data = (INT8 *)off; off += data.len;
    }
    return off;
}
#endif //_DEFINE_INLINE

#endif /* _TNGPKT_H_ */
