%{
/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rttp2sn.pm,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

/*
 * This is generated code, do not modify. Look in rttp2sn.pm to make
 * modifications.
 */

#ifndef RTTP2SN_H_
#define RTTP2SN_H_
    
#define RTTPSN_URL		    RTTP(SN,0)
#define RTTPSN_STREAM_ID	    RTTP(SN,1)
#define RTTPSN_STREAM_HEADER	    RTTP(SN,2)
#define RTTPSN_TIME		    RTTP(SN,3)
#define RTTPSN_CONTENT		    RTTP(SN,4)
#define RTTPSN_AUDIO_CONTENT	    RTTP(SN,4) // Yes, these are supposed
#define RTTPSN_IMAGE_CONTENT	    RTTP(SN,4) // to be the same!
#define RTTPSN_START		    RTTP(SN,5)
#define RTTPSN_STOP		    RTTP(SN,6)
#define RTTPSN_STREAM_STATUS	    RTTP(SN,7)
#define RTTPSN_DELAY		    RTTP(SN,8)
#define RTTPSN_DUPLICATE_STREAM	    RTTP(SN,9)
#define RTTPSN_KILL_STREAM	    RTTP(SN,10)
#define RTTPSN_KILL_ALL_STREAMS	    RTTP(SN,11)

/* 
 * Status types
 */

#define RTTPSN_ST_NO_ERROR		0
#define RTTPSN_ST_ERROR			1
    
/*
 * Stream types
 */ 
    
#define RTTPSN_ST_REAL_MEDIA	0x2E524D46
#define RTTPSN_ST_REAL_AUDIO 	0x2e7261fd

%}

RTTPSN_url : rttp2_header
{
    Int32		stream_id;
    String		url;
}

RTTPSN_stream_id : rttp2_header
{
    Int32		stream_id;
}

RTTPSN_stream_header : rttp2_header
{
    Int32		stream_type;
    Int32		size;
    Int16		stream_index;
    Bit[14]		reserved : no_access;
    Bit[1]		reliable_check_packets;
    Bit[1]		reliable_stream;
    Int32		bytes_per_minute;
    Int32		max_packet_size;
    Buffer[]		type_specific_data : int32_length;
}

RTTPSN_time : rttp2_header
{
    Int32		sec;
    Int32		usec;
}

//RTTPSN_content : rttp2_header
//{
//  Bit[3]		reserved : no_access;
//  Bit[1]		keyframe;
//  Bit[1]		time_stamp_present;
//  Bit[1]		two_byte_length;
//  Bit[1]		two_byte_stream_index;
//  Bit[1]		reliable_stream;
//  if (two_byte_length == 1)
//  {
//	Int16		data_len;
//  }
//  else
//  {
//	Int8		data_len;
//  }
//  if (two_byte_stream_index == 1)
//  {
//	Int16		stream_index;
//  }
//  else
//  {
//	Int8		stream_index;
//  }
//  if (time_stamp_present == 1)
//  {
//	Int32		time_stamp;
//  }
//  Buffer[]		data : no_length;
//}

RTTPSN_audio_content : rttp2_header
{
    Bit[4]		reserved : no_access;
    Bit[1]		time_stamp_present;
    Bit[1]		two_byte_stream_index;
    Bit[1]		two_byte_length;
    Bit[1]		reliable_stream;
    Int16		data_len;
    Int8		stream_index;
    Buffer[]		data : no_length;
}

RTTPSN_image_content : rttp2_header
{
    Bit[4]		reserved : no_access;
    Bit[1]		time_stamp_present;
    Bit[1]		two_byte_stream_index;
    Bit[1]		two_byte_length;
    Bit[1]		reliable_stream;
    Int16		data_len;
    Int8		stream_index;
    Int32		time_stamp;
    Buffer[]		data : no_length;
}

RTTPSN_start : rttp2_header
{
    Int16		stream_index;
    Int32		sec;
    Int32		usec;
}

RTTPSN_stop : rttp2_header
{
    Int16		stream_index;
    Int32		sec;
    Int32		usec;
}

%{

const int RTTPSN_STREAM_STATUS_STATIC_SIZE = 6;
%}
RTTPSN_stream_status : rttp2_header
{
    Int16		status_type;
    String		status_message;
}

RTTPSN_delay : rttp2_header
{
    Int16		stream_index;
}

RTTPSN_duplicate_stream : rttp2_header
{
    Int16		stream_number;
}

RTTPSN_kill_stream : rttp2_header
{
    Int16		stream_number;
}

RTTPSN_kill_all_streams : rttp2_header
{
}

%{  
/*
 * protgen isn't smart enough to generate the content message correctly
 * so I've helped it along.
 */
    
class RTTPSN_content : public RTTP2_msg
{
public:
#ifdef REAL_MEDIA_FILE_SERVER_PORT
    void		get(Int16& keyframe, Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data);
    void		set(Int16 keyframe, Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data);
#else
    void		get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data);
    void		set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data);
#endif
};
   
inline
#ifdef REAL_MEDIA_FILE_SERVER_PORT
void RTTPSN_content::get(Int16& keyframe, Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data)
#else
void RTTPSN_content::get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data)
#endif
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field;
    bit_field = *(msg+msg_offset);
    msg_offset += 1;
#ifdef REAL_MEDIA_FILE_SERVER_PORT
    keyframe = (bit_field >> 4) & 1;
#endif
    time_stamp_present = (bit_field >> 3) & 1;
    two_byte_stream_index = (bit_field >> 2) & 1;
    two_byte_length = (bit_field >> 1) & 1;
    reliable_stream = (bit_field >> 0) & 1;
    if (two_byte_length == 1)
    {
	data_len = getshort(msg+msg_offset);
	msg_offset += 2;
    }
    else
    {
	data_len = getbyte(msg+msg_offset);
	msg_offset += 1;
    }
    if (two_byte_stream_index == 1)
    {
	stream_index = getshort(msg+msg_offset);
	msg_offset += 2;
    }
    else
    {
	stream_index = *(msg+msg_offset);
	msg_offset += 1;
    }
    if (time_stamp_present == 1)
    {
	time_stamp = getlong(msg+msg_offset);
	msg_offset += 4;
    }
    data = msg+msg_offset;
    msg_offset += data_len;
}

#ifdef REAL_MEDIA_FILE_SERVER_PORT
inline
void RTTPSN_content::set(Int16 keyframe, Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data)
#else
inline
void RTTPSN_content::set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data)
#endif /* REAL_MEDIA_FILE_SERVER_PORT */
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field = 0;
#ifdef REAL_MEDIA_FILE_SERVER_PORT
    bit_field = bit_field | (keyframe << 4);
#endif /* REAL_MEDIA_FILE_SERVER_PORT */
    bit_field = bit_field | (time_stamp_present << 3);
    bit_field = bit_field | (two_byte_stream_index << 2);
    bit_field = bit_field | (two_byte_length << 1);
    bit_field = bit_field | (reliable_stream << 0);
    *(msg+msg_offset) = bit_field;
    msg_offset += 1;
    if (two_byte_length == 1)
    {
	putshort(msg+msg_offset, data_len);
	msg_offset += 2;
    }
    else
    {
	putbyte(msg+msg_offset, (char)data_len);
	msg_offset += 1;
    }
    if (two_byte_stream_index == 1)
    {
	putshort(msg+msg_offset, stream_index);
	msg_offset += 2;
    }
    else
    {
	*(msg+msg_offset) = stream_index;
	msg_offset += 1;
    }
    if (time_stamp_present == 1)
    {
	putlong(msg+msg_offset, time_stamp);
	msg_offset += 4;
    }
    memcpy(msg+msg_offset, data, data_len);
    msg_offset += data_len;
    set_opcode((Int16) (RTTPSN_CONTENT| 0x8000));
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

#endif /* RTTP2SN_H_ */
%}
