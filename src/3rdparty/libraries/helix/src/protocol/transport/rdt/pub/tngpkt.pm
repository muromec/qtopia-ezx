%{
#ifndef _TNGPKT_H_
#define _TNGPKT_H_

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tngpkt.pm,v 1.3 2005/01/05 22:49:52 gwright Exp $ 
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

/*
 * XXXSMP Before using any of the packets that aren't used yet,
 * check to make sure the format conforms with the DataPacket's
 * first 5 bytes.
 */
%}

packlen struct TNGDataPacket
{
    bit[1]			length_included_flag;	/* Inc. length field */
    bit[1]			need_reliable_flag;	/* Inc. rel. seq no */
    bit[5]			stream_id;		/* 31 == expansion */
    bit[1] 			is_reliable;

    u_int16			seq_no;         /* overload for packet type*/

    if(length_included_flag == 1)
    {
	u_int16			_packlenwhendone;
    }

    bit[1]			back_to_back_packet;
    bit[1]			slow_data;
    bit[6]			asm_rule_number;	/* 63 is expansion */

    u_int32			timestamp;
    if(stream_id == 31)
    {
	u_int16			stream_id_expansion;
    }
    if(need_reliable_flag == 1)
    {
	u_int16			total_reliable;
    }
    if(asm_rule_number == 63)
    {
	u_int16			asm_rule_number_expansion;
    }
    buffer			data;
}

struct TNGMultiCastDataPacket
{
    bit[1]			length_included_flag;	/* Inc. length field */
    bit[1]			need_reliable_flag;	/* Inc. rel. seq no */
    bit[5]			stream_id;		/* 31 == expansion */
    bit[1] 			is_reliable;

    u_int16			seq_no;         /* overload for packet type*/

    if(length_included_flag == 1)
    {
	u_int16			length;
    }

    bit[1]			back_to_back_packet;
    bit[1]			slow_data;
    bit[6]			asm_rule_number;	/* 63 is expansion */

    u_int8			group_seq_no;

    u_int32			timestamp;
    if(stream_id == 31)
    {
	u_int16			stream_id_expansion;
    }
    if(need_reliable_flag == 1)
    {
	u_int16			total_reliable;
    }
    if(asm_rule_number == 63)
    {
	u_int16			asm_rule_number_expansion;
    }
    buffer			data;
}

struct TNGASMActionPacket
{
    bit[1]			length_included_flag;
    bit[5]			stream_id;
    bit[1]			dummy0;	/* 0 */
    bit[1]			dummy1; /* 1 */
    u_int16			packet_type; /* 0xff00 */
    u_int16			reliable_seq_no;
    if(length_included_flag == 1)
    {
	u_int16			length;
    }
    if(stream_id == 31)
    {
	u_int16			stream_id_expansion;
    }
    buffer			data;
}
    
struct TNGBandwidthReportPacket
{
    bit[1]			length_included_flag;
    bit[5]			dummy0;	/* 0 */
    bit[1]			dummy1;	/* 0 */
    bit[1]			dummy2; /* 0 */
    u_int16			packet_type; /* 0xff01 */
    if(length_included_flag == 1)
    {
	u_int16			length;
    }
    u_int16			interval;
    u_int32			bandwidth;
    u_int8			sequence;
}

struct TNGReportPacket
{
    bit[1]                      length_included_flag;
    bit[5]                      dummy0; /* 0 */
    bit[1]                      dummy1; /* 0 */
    bit[1]                      dummy2; /* 0 */
    u_int16                     packet_type; /* 0xff07 */
    if(length_included_flag == 1)
    {
	u_int16                   length;
    }
    buffer                      data;
}

struct TNGACKPacket
{
    bit[1]			length_included_flag;
    bit[1]			lost_high;
    bit[5]			dummy0; /* 0 */
    bit[1]			dummy1; /* 0 */
    u_int16			packet_type; /* 0xff02 */
    if(length_included_flag == 1)
    {
	u_int16			length;
    }
    /*
     * data consists of header + bitmap repeated for each stream: 
     *   u_int16 stream_number
     *   u_int16 last_seq_no
     *   u_int16 bit_count
     *   u_int8  bitmap_length
     *   buffer bitmap
     */
    buffer			data;
}

struct TNGRTTRequestPacket
{
    bit[1]			dummy0; /* 0 */
    bit[5]			dummy1;	/* 0 */
    bit[1]			dummy2;	/* 0 */
    bit[1]			dummy3; /* 0 */
    u_int16			packet_type; /* 0xff03 */
}

struct TNGRTTResponsePacket
{
    bit[1]			dummy0; /* 0 */
    bit[5]			dummy1;	/* 0 */
    bit[1]			dummy2;	/* 0 */
    bit[1]			dummy3; /* 0 */
    u_int16			packet_type; /* 0xff04 */
    u_int32			timestamp_sec;
    u_int32			timestamp_usec;
}

struct TNGCongestionPacket
{
    bit[1]			dummy0; /* 0 */
    bit[5]			dummy1;	/* 0 */
    bit[1]			dummy2;	/* 0 */
    bit[1]			dummy3; /* 0 */
    u_int16			packet_type; /* 0xff05 */
    int32			xmit_multiplier;
    int32			recv_multiplier;
}

struct TNGStreamEndPacket
{
    bit[1]			need_reliable_flag;
    bit[5]			stream_id;
    bit[1]			packet_sent; /* 0 */
    bit[1]			ext_flag;
    u_int16			packet_type; /* 0xff06 */
    u_int16			seq_no;
    u_int32			timestamp;
    if(stream_id == 31)
    {
	u_int16			stream_id_expansion;
    }
    if(need_reliable_flag == 1)
    {
	u_int16			total_reliable;
    }
    /* reason for stream end (optional): requires RDT v3 */
    if(ext_flag == 1)
    {
	u_int8[3]		reason_dummy; /* 0xff 0xff 0xff */
	u_int32			reason_code;
	string			reason_text;
    }
}

struct TNGLatencyReportPacket
{
    bit[1]                     length_included_flag;
    bit[5]                     dummy0; /* 0 */
    bit[1]                     dummy1; /* 0 */
    bit[1]                     dummy2; /* 0 */
    u_int16                      packet_type; /* 0xff08 */
    if(length_included_flag == 1)
    {
       u_int16                   length;
    }
    u_int32                    server_out_time;
}

%{

    /*
     * RDTFeatureLevel 3 packets
     */

%}
 
struct RDTTransportInfoRequestPacket
{
    bit[1]                     dummy0; /* 0 */
    bit[5]                     dummy1; /* 0 */
    bit[1]                     request_rtt_info;
    bit[1]                     request_buffer_info;
    u_int16                    packet_type; /* 0xff09 */
 
    if (request_rtt_info == 1)
    {
        u_int32                     request_time_ms;
     }
 }

struct RDTBufferInfo
{
   u_int16                    stream_id;
   u_int32                    lowest_timestamp;
   u_int32                    highest_timestamp;
   u_int32                    bytes_buffered;
}
 
struct RDTTransportInfoResponsePacket
{
   bit[1]                     dummy0; /* 0 */
   bit[4]                     dummy1; /* 0 */
   bit[1]                     has_rtt_info;
   bit[1]                     is_delayed; 
   bit[1]                     has_buffer_info;
   u_int16                    packet_type; /* 0xff0a */
 
   if (has_rtt_info == 1)
   {
       u_int32                     request_time_ms;
       
       if (is_delayed == 1)
       {
	   u_int32                     response_time_ms;
       }
   }
   if (has_buffer_info == 1)
   {
       u_int16 buffer_info_count;
       RDTBufferInfo[buffer_info_count] buffer_info;
   }
}

struct TNGBWProbingPacket
{
    bit[1]                     length_included_flag;
    bit[5]                     dummy0; /* 0 */
    bit[1]                     dummy1; /* 0 */
    bit[1]                     dummy2; /* 0 */
    u_int16                    packet_type; /* 0xff0b */
    if(length_included_flag == 1)
    {
       u_int16                 length;
    }
    u_int8		       seq_no;
    u_int32		       timestamp;
    buffer		       data;
}

%{
#endif /* _TNGPKT_H_ */
%}
