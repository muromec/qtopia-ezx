/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rttp2sn.h,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

class RTTPSN_url : public RTTP2_msg
{
public:
    void		get(Int32& stream_id, char*& url);
    void		set(Int32 stream_id, const char* url);
};

inline
void RTTPSN_url::get(Int32& stream_id, char*& url)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    stream_id = getlong(msg+msg_offset);
    msg_offset += 4;
    url = (char*) msg+msg_offset;
    msg_offset += strlen(url) + 1;
}

inline
void RTTPSN_url::set(Int32 stream_id, const char* url)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    putlong(msg+msg_offset, stream_id);
    msg_offset += 4;
    strcpy((char*)msg+msg_offset, url);
    msg_offset += strlen(url) + 1;
    set_opcode((Int16) RTTPSN_URL);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSN_stream_id : public RTTP2_msg
{
public:
    void		get(Int32& stream_id);
    void		set(Int32 stream_id);
};

inline
void RTTPSN_stream_id::get(Int32& stream_id)
{
    stream_id = getlong(msg+4);
}

inline
void RTTPSN_stream_id::set(Int32 stream_id)
{
    memset(msg, 0, 4);
    putlong(msg+4, stream_id);
    set_opcode((Int16) RTTPSN_STREAM_ID);
    set_length(4);
    msg_len = 8;
}

class RTTPSN_stream_header : public RTTP2_msg
{
public:
    void		get(Int32& stream_type, Int32& size, Int16& stream_index, Int16& reliable_check_packets, Int16& reliable_stream, Int32& bytes_per_minute, Int32& max_packet_size, Byte*& type_specific_data, Int32& type_specific_data_len);
    void		set(Int32 stream_type, Int32 size, Int16 stream_index, Int16 reliable_check_packets, Int16 reliable_stream, Int32 bytes_per_minute, Int32 max_packet_size, Byte* type_specific_data, Int32 type_specific_data_len);
};

inline
void RTTPSN_stream_header::get(Int32& stream_type, Int32& size, Int16& stream_index, Int16& reliable_check_packets, Int16& reliable_stream, Int32& bytes_per_minute, Int32& max_packet_size, Byte*& type_specific_data, Int32& type_specific_data_len)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    stream_type = getlong(msg+msg_offset);
    msg_offset += 4;
    size = getlong(msg+msg_offset);
    msg_offset += 4;
    stream_index = (INT16) getshort(msg+msg_offset);
    msg_offset += 2;
    Int16 bit_field;
    bit_field = (INT16) getshort(msg+msg_offset);
    msg_offset += 2;
    reliable_check_packets = (bit_field >> 1) & 1;
    reliable_stream = (bit_field >> 0) & 1;
    bytes_per_minute = getlong(msg+msg_offset);
    msg_offset += 4;
    max_packet_size = getlong(msg+msg_offset);
    msg_offset += 4;
    type_specific_data_len = getlong(msg+msg_offset);
    type_specific_data = (Byte*)msg+msg_offset+ 4;
    msg_offset += type_specific_data_len + 4;
}

inline
void RTTPSN_stream_header::set(Int32 stream_type, Int32 size, Int16 stream_index, Int16 reliable_check_packets, Int16 reliable_stream, Int32 bytes_per_minute, Int32 max_packet_size, Byte* type_specific_data, Int32 type_specific_data_len)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    putlong(msg+msg_offset, stream_type);
    msg_offset += 4;
    putlong(msg+msg_offset, size);
    msg_offset += 4;
    putshort(msg+msg_offset, stream_index);
    msg_offset += 2;
    Int16 bit_field = 0;
    bit_field = bit_field | (reliable_check_packets << 1);
    bit_field = bit_field | (reliable_stream << 0);
    putshort(msg+msg_offset, bit_field);
    msg_offset += 2;
    putlong(msg+msg_offset, bytes_per_minute);
    msg_offset += 4;
    putlong(msg+msg_offset, max_packet_size);
    msg_offset += 4;
    putlong(msg+msg_offset, type_specific_data_len);
    memcpy(msg+msg_offset + 4, type_specific_data, type_specific_data_len);
    msg_offset += type_specific_data_len + 4;
    set_opcode((Int16) RTTPSN_STREAM_HEADER);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSN_time : public RTTP2_msg
{
public:
    void		get(Int32& sec, Int32& usec);
    void		set(Int32 sec, Int32 usec);
};

inline
void RTTPSN_time::get(Int32& sec, Int32& usec)
{
    sec = getlong(msg+4);
    usec = getlong(msg+8);
}

inline
void RTTPSN_time::set(Int32 sec, Int32 usec)
{
    memset(msg, 0, 4);
    putlong(msg+4, sec);
    putlong(msg+8, usec);
    set_opcode((Int16) RTTPSN_TIME);
    set_length(8);
    msg_len = 12;
}

class RTTPSN_audio_content : public RTTP2_msg
{
public:
    void		get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Byte*& data);
    void		set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Byte* data);
};

inline
void RTTPSN_audio_content::get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Byte*& data)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field;
    bit_field = *(msg+msg_offset);
    msg_offset += 1;
    time_stamp_present = (bit_field >> 3) & 1;
    two_byte_stream_index = (bit_field >> 2) & 1;
    two_byte_length = (bit_field >> 1) & 1;
    reliable_stream = (bit_field >> 0) & 1;
    data_len = (INT16) getshort(msg+msg_offset);
    msg_offset += 2;
    stream_index = *(msg+msg_offset);
    msg_offset += 1;
    data = (Byte*)msg+msg_offset;
    msg_offset += data_len;
}

inline
void RTTPSN_audio_content::set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Byte* data)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field = 0;
    bit_field = bit_field | (time_stamp_present << 3);
    bit_field = bit_field | (two_byte_stream_index << 2);
    bit_field = bit_field | (two_byte_length << 1);
    bit_field = bit_field | (reliable_stream << 0);
    *(msg+msg_offset) = bit_field;
    msg_offset += 1;
    putshort(msg+msg_offset, data_len);
    msg_offset += 2;
    *(msg+msg_offset) = stream_index;
    msg_offset += 1;
    memcpy(msg+msg_offset, data, data_len);
    msg_offset += data_len;
    set_opcode((Int16) RTTPSN_AUDIO_CONTENT);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSN_image_content : public RTTP2_msg
{
public:
    void		get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data);
    void		set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data);
};

inline
void RTTPSN_image_content::get(Int16& time_stamp_present, Int16& two_byte_stream_index, Int16& two_byte_length, Int16& reliable_stream, Int16& data_len, Int8& stream_index, Int32& time_stamp, Byte*& data)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field;
    bit_field = *(msg+msg_offset);
    msg_offset += 1;
    time_stamp_present = (bit_field >> 3) & 1;
    two_byte_stream_index = (bit_field >> 2) & 1;
    two_byte_length = (bit_field >> 1) & 1;
    reliable_stream = (bit_field >> 0) & 1;
    data_len = (INT16) getshort(msg+msg_offset);
    msg_offset += 2;
    stream_index = *(msg+msg_offset);
    msg_offset += 1;
    time_stamp = getlong(msg+msg_offset);
    msg_offset += 4;
    data = (Byte*)msg+msg_offset;
    msg_offset += data_len;
}

inline
void RTTPSN_image_content::set(Int16 time_stamp_present, Int16 two_byte_stream_index, Int16 two_byte_length, Int16 reliable_stream, Int16 data_len, Int8 stream_index, Int32 time_stamp, Byte* data)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    Int8 bit_field = 0;
    bit_field = bit_field | (time_stamp_present << 3);
    bit_field = bit_field | (two_byte_stream_index << 2);
    bit_field = bit_field | (two_byte_length << 1);
    bit_field = bit_field | (reliable_stream << 0);
    *(msg+msg_offset) = bit_field;
    msg_offset += 1;
    putshort(msg+msg_offset, data_len);
    msg_offset += 2;
    *(msg+msg_offset) = stream_index;
    msg_offset += 1;
    putlong(msg+msg_offset, time_stamp);
    msg_offset += 4;
    memcpy(msg+msg_offset, data, data_len);
    msg_offset += data_len;
    set_opcode((Int16) RTTPSN_IMAGE_CONTENT);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSN_start : public RTTP2_msg
{
public:
    void		get(Int16& stream_index, Int32& sec, Int32& usec);
    void		set(Int16 stream_index, Int32 sec, Int32 usec);
};

inline
void RTTPSN_start::get(Int16& stream_index, Int32& sec, Int32& usec)
{
    stream_index = (INT16) getshort(msg+4);
    sec = getlong(msg+6);
    usec = getlong(msg+10);
}

inline
void RTTPSN_start::set(Int16 stream_index, Int32 sec, Int32 usec)
{
    memset(msg, 0, 4);
    putshort(msg+4, stream_index);
    putlong(msg+6, sec);
    putlong(msg+10, usec);
    set_opcode((Int16) RTTPSN_START);
    set_length(12);
    msg_len = 16;
}

class RTTPSN_stop : public RTTP2_msg
{
public:
    void		get(Int16& stream_index, Int32& sec, Int32& usec);
    void		set(Int16 stream_index, Int32 sec, Int32 usec);
};

inline
void RTTPSN_stop::get(Int16& stream_index, Int32& sec, Int32& usec)
{
    stream_index = (INT16) getshort(msg+4);
    sec = getlong(msg+6);
    usec = getlong(msg+10);
}

inline
void RTTPSN_stop::set(Int16 stream_index, Int32 sec, Int32 usec)
{
    memset(msg, 0, 4);
    putshort(msg+4, stream_index);
    putlong(msg+6, sec);
    putlong(msg+10, usec);
    set_opcode((Int16) RTTPSN_STOP);
    set_length(12);
    msg_len = 16;
}


const int RTTPSN_STREAM_STATUS_STATIC_SIZE = 6;
class RTTPSN_stream_status : public RTTP2_msg
{
public:
    void		get(Int16& status_type, char*& status_message);
    void		set(Int16 status_type, const char* status_message);
};

inline
void RTTPSN_stream_status::get(Int16& status_type, char*& status_message)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    status_type = (INT16) getshort(msg+msg_offset);
    msg_offset += 2;
    status_message = (char*) msg+msg_offset;
    msg_offset += strlen(status_message) + 1;
}

inline
void RTTPSN_stream_status::set(Int16 status_type, const char* status_message)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    putshort(msg+msg_offset, status_type);
    msg_offset += 2;
    strcpy((char*)msg+msg_offset, status_message);
    msg_offset += strlen(status_message) + 1;
    set_opcode((Int16) RTTPSN_STREAM_STATUS);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSN_delay : public RTTP2_msg
{
public:
    void		get(Int16& stream_index);
    void		set(Int16 stream_index);
};

inline
void RTTPSN_delay::get(Int16& stream_index)
{
    stream_index = (INT16) getshort(msg+4);
}

inline
void RTTPSN_delay::set(Int16 stream_index)
{
    memset(msg, 0, 4);
    putshort(msg+4, stream_index);
    set_opcode((Int16) RTTPSN_DELAY);
    set_length(4);
    msg_len = 8;
}

class RTTPSN_duplicate_stream : public RTTP2_msg
{
public:
    void		get(Int16& stream_number);
    void		set(Int16 stream_number);
};

inline
void RTTPSN_duplicate_stream::get(Int16& stream_number)
{
    stream_number = (INT16) getshort(msg+4);
}

inline
void RTTPSN_duplicate_stream::set(Int16 stream_number)
{
    memset(msg, 0, 4);
    putshort(msg+4, stream_number);
    set_opcode((Int16) RTTPSN_DUPLICATE_STREAM);
    set_length(4);
    msg_len = 8;
}

class RTTPSN_kill_stream : public RTTP2_msg
{
public:
    void		get(Int16& stream_number);
    void		set(Int16 stream_number);
};

inline
void RTTPSN_kill_stream::get(Int16& stream_number)
{
    stream_number = (INT16) getshort(msg+4);
}

inline
void RTTPSN_kill_stream::set(Int16 stream_number)
{
    memset(msg, 0, 4);
    putshort(msg+4, stream_number);
    set_opcode((Int16) RTTPSN_KILL_STREAM);
    set_length(4);
    msg_len = 8;
}

class RTTPSN_kill_all_streams : public RTTP2_msg
{
public:
    void		get();
    void		set();
};

inline
void RTTPSN_kill_all_streams::get()
{
}

inline
void RTTPSN_kill_all_streams::set()
{
    memset(msg, 0, 4);
    set_opcode((Int16) RTTPSN_KILL_ALL_STREAMS);
    set_length(0);
    msg_len = 4;
}

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
	data_len = (INT16) getshort(msg+msg_offset);
	msg_offset += 2;
    }
    else
    {
	data_len = getbyte(msg+msg_offset);
	msg_offset += 1;
    }
    if (two_byte_stream_index == 1)
    {
	stream_index = (INT8) getshort(msg+msg_offset);
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
