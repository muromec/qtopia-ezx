/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rmfftype.h,v 1.13 2006/06/16 09:40:37 jaiswal Exp $
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 UINT32	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/

// realvideo cooment...
// one more RV comment.....
#ifndef _RMFFTYPES_
#define _RMFFTYPES_

#if !(defined REAL_MEDIA_FILE_SERVER_PORT)
#include "hxtypes.h"
#else
#include "machdep.h"
#include "types.h"
#endif /* !(defined REAL_MEDIA_FILE_SERVER_PORT) */
#include <string.h>		// for memcpy's

#include "rule2flg.h" // /For class RuleToFlagMap

#if (!defined(_BEOS))
#define int8	char
#define int16	INT16
#define int32	LONG32
#endif
#define u_int8	UCHAR
#define u_int16 UINT16
#define u_int32 ULONG32


// Event stuff
#define RAE_FILE_VERSION 2
#define RAE_FILE_MAGIC_SIZE 5
#define RAE_FILE_VERSION_SIZE 2     
#define RAE_FILE_HEADER_SIZE (RAE_FILE_MAGIC_SIZE + RAE_FILE_VERSION_SIZE)
#define RAE_EVENT_HEADER_SIZE (sizeof(ULONG32) + sizeof(ULONG32) + sizeof(UINT16))

#define RA_MEDIA_DATA					0x4D444941		// 'MDIA'
#define RA_EVENT_DATA					0x45564E54		//  'EVNT'


#define RM_HEADER_OBJECT				0x2E524D46		// '.RMF' 	
#define RMS_HEADER_OBJECT				0x2E524D53		// '.RMS' 	
#define RM_PROPERTIES_OBJECT			0x50524F50		// 'PROP' 	
#define RM_MEDIA_PROPERTIES_OBJECT		0x4D445052		// 'MDPR' 	
#define RM_CONTENT_OBJECT				0x434F4E54		// 'CONT' 	
#define RM_DATA_OBJECT					0x44415441		// 'DATA' 	
#define RM_INDEX_OBJECT					0x494E4458		// 'INDX' 		// 'DATA' 	
#define RM_MULTIHEADER_OBJECT				0x4D4C5449	// 'MLTI'

#define RM_FILE_OBJECT_VERSION				0
#define RM_PROPERTIES_OBJECT_VERSION		0
#define RM_MEDIAPROPERTIES_OBJECT_VERSION	0
#define RM_CONTENT_OBJECT_VERSION			0
#define RM_DATA_OBJECT_VERSION				0
#define RM_INDEX_OBJECT_VERSION				0
#define RM_PACKET_OBJECT_VERSION			0
#define RM_INDEXRECORD_OBJECT_VERSION		0


#define HX_SAVE_ENABLED					0x0001
#define HX_PERFECT_PLAY_ENABLED			0x0002
#define HX_LIVE_BROADCAST				0x0004
#define HX_MOBILE_PLAY_ENABLED			0x0008

#if (defined( _WIN32 ) || defined( _WINDOWS )) && defined(_M_IX86)
#pragma pack(1)
//	disable warning on: zero-sized array in struct/union
#pragma warning( disable : 4200 )
#endif

#ifdef _MACINTOSH
// modify packing of VideoTypeSpecificData to match 
// hxcodec.h's packing of HX_FORMAT_VIDEO
// cr grobbinsky cf rlovejoy
#pragma options align=mac68k
#endif

#include "hxinline.h"

class RMGenericHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 10;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
};

typedef RMGenericHeader * RMGenericHeaderPtr;
class RMFileHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 18;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT32	file_version;
    UINT32	num_headers;
};

typedef RMFileHeader * RMFileHeaderPtr;
class Properties
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 50;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT32	max_bit_rate;
    UINT32	avg_bit_rate;
    UINT32	max_packet_size;
    UINT32	avg_packet_size;
    UINT32	num_interleave_packets;
    UINT32	duration;
    UINT32	preroll;
    UINT32	index_offset;
    UINT32	data_offset;
    UINT16	num_streams;
    UINT16	flags;
};

typedef Properties * PropertiesPtr;
class MediaProperties
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 46;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT16	stream_number;
    UINT32	max_bit_rate;
    UINT32	avg_bit_rate;
    UINT32	max_packet_size;
    UINT32	avg_packet_size;
    UINT32	start_time;
    UINT32	preroll;
    UINT32	duration;
    UINT8	stream_name_size;
    UINT8	*stream_name;
    UINT8	mime_type_size;
    UINT8	*mime_type;
    UINT32	type_specific_len;
    UINT8	*type_specific_data;
};

typedef MediaProperties * MediaPropertiesPtr;

//
// Can be used to crack open ra4 type specific
// data. Might also work on a portion of ra5
// data as well, but I would not count on it.
// Should be using classes defined over in
// raform4.pm instead.
//
class AudioTypeSpecificData
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 73;}

    UINT32	theID;
    UINT16	AudioSpecificVersion;
    UINT16	AudioSpecificRevision;
    UINT32	raID;
    UINT32	RaChunkSize;
    UINT16	version;
    UINT16	revision;
    UINT16	headerBytesTotal;
    UINT16	compressionType;
    UINT32	granularity;
    UINT32	bytesTotal;
    UINT32	bytesPerMinute;
    UINT32	bytesPerMinute2;
    UINT16	interleaveFactor;
    UINT16	interleaveBlockSize;
    UINT32	userData;
    UINT32	sampleRate;
    UINT16	sampleSize;
    UINT16	numChannels;
    UINT8	InterleaveCode[5];
    UINT8	CompressionCode[5];
    UINT8	isInterleaved;
    UINT8	copyByte;
    UINT8	streamType;
    UINT8	title_len;
    UINT8	*title;
    UINT8	author_len;
    UINT8	*author;
    UINT8	copyright_len;
    UINT8	*copyright;
    UINT8	app_len;
    UINT8	*app;
};

class VideoTypeSpecificData
{
public:
    VideoTypeSpecificData();

    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 26;}

    UINT32	cbLength;
    UINT32	moftag;
    UINT32	submoftag;
    UINT16	uiWidth;
    UINT16	uiHeight;
    UINT16	uiBitCount;
    UINT16	uiPadWidth;
    UINT16	uiPadHeight;
    UINT32	framesPerSecond;
    UINT32      ulSPOExtra;
    UINT32      ulStreamVersion;
};

class MultiStreamHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 8;}

    UINT32	object_id;
    UINT16	num_rules;
    UINT16	*rule_to_header_map;
    UINT16	num_headers;
};

class NameValueProperty
{
public:
    NameValueProperty();
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 13;}

    UINT32	size;
    UINT16	object_version;
    UINT8	name_length;
    UINT8	*name;
    UINT32	type;
    UINT16	value_length;
    UINT8	*value_data;
};

//
// Use this for packing/unpacking
// a single 32 bit integer.
//

class Unsigned32BitInteger
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 4;}

    UINT32	value;
};

//
// enum defining valid values for "type" in
// NameValueProperty structure
//

enum
{
	PROP_INTEGER,
	PROP_BUFFER,
	PROP_STRING
};

class LogicalStream
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 12;}

    UINT32	size;
    UINT16	object_version;
    UINT16	num_physical_streams;
    UINT16	*physical_stream_numbers;
    UINT32	*data_offsets;
    UINT16	num_rules;
    UINT16	*rule_to_physical_stream_number_map;
    UINT16	num_properties;
    NameValueProperty	*properties;
};

class MetaInformation
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 8;}

    UINT32	size;
    UINT16	object_version;
    UINT16	num_properties;
    NameValueProperty	*properties;
};

//
// BUGBUG
// XXX
// PMC can't generate a declaration for this constructor.
// So, the generated .h file will not build until the
// declaration is added by hand.
//
class Content
{
public:
    Content();
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 18;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT16	title_len;
    UINT8	*title;
    UINT16	author_len;
    UINT8	*author;
    UINT16	copyright_len;
    UINT8	*copyright;
    UINT16	comment_len;
    UINT8	*comment;
};

typedef  Content * ContentPtr;
class DataHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 18;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT32	num_interleave_packets;
    UINT32	next_data_header;
};

typedef DataHeader * DataHeaderPtr;
class IndexHeader
{
public:
    IndexHeader ();
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT32	num_indices;
    UINT16	stream_number;
    UINT32	next_index_header;
};

typedef IndexHeader * IndexHeaderPtr;
class IndexRecord
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 14;}

    UINT16	object_version;
    UINT32	timestamp;
    UINT32	offset;
    UINT32	num_interleave_packets;
};

typedef IndexRecord * IndexRecordPtr;

//
// The PacketHeader has changed with version 1.
// So, there are a couple of new structures.
// Use PacketHeaderBase to figure out what
// packet version/size you've got, and then
// create the appropriate PacketHeader version
// struct to really unpack the buffer. We could
// also have done this by embedding if (object_version == xx)
// into the PacketHeader struct, but this lets us
// use the static_size() function to get the
// actual packed size of the struct in question.
//
class PacketHeaderBase
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 4;}

    UINT16	object_version;
    UINT16	length;
};

class PacketHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 12;}

    UINT16	object_version;
    UINT16	length;
    UINT16	stream_number;
    UINT32	timestamp;
    UINT16	flags;
};

typedef PacketHeader * PacketHeaderPtr;
class PacketHeader1
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 13;}

    UINT16	object_version;
    UINT16	length;
    UINT16	stream_number;
    UINT32	timestamp;
    UINT16	asm_rule;
    UINT8	asm_flags;
};

class Profile
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 18;}

    UINT32	object_id;
    UINT32	size;
    UINT16	object_version;
    UINT32	Bandwidth;
    UINT32	CpuPower;
};

typedef Profile * ProfilePtr;

//
// RMEventPacketData is placed directly after the packet header
// for an event stream inside of an RM file.
// event_type_and_string_len should be set to the length of the
// string plus 2 bytes for the event type.
// Also note that start_time and stop_time in here are in
// deciseconds NOT milliseconds.
//
class RMEventPacketData
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 14;}

    UINT16	sequence_number;
    UINT32	start_time_deciseconds;
    UINT32	stop_time_deciseconds;
    UINT16	event_type_and_string_len;
    UINT16	event_type_id;
    UINT8	*event_string;
};

//
// An array of RMImageMapRegionData structures
// is included as part of the raw data for each
// image map packet. The interpretation of num_values
// and values is determined by shape.
//
class RMImageMapRegionData
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 22;}

    UINT32	start_time;
    UINT32	end_time;
    UINT16	shape;
    UINT16	num_values;
    UINT16	*values;
    UINT16	action;
    UINT32	seek_time;
    UINT16	url_len;
    UINT8	*url;
    UINT16	status_len;
    UINT8	*status;
};

//
// RMImageMapPacketData, like RMEventPacketData, is placed
// directly after the packet header for an image map stream
// inside of an RM file.
//
class RMImageMapPacketData
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT16	sequence_number;
    UINT16	num_regions;
    UINT32	start_time;
    UINT32	end_time;
    UINT16	left;
    UINT16	top;
    UINT16	right;
    UINT16	bottom;
    RMImageMapRegionData	*regions;
};

//
// RMBaseImageMapPacketData extracts the basic
// image map data (start, stop, dimensions) without
// actually creating all of the regions on the heap.
//
class RMBaseImageMapPacketData
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT16	sequence_number;
    UINT16	num_regions;
    UINT32	start_time;
    UINT32	end_time;
    UINT16	left;
    UINT16	top;
    UINT16	right;
    UINT16	bottom;
};

class StreamPair
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 4;}

    UINT16	stream1_num;
    UINT16	stream2_num;
};

class StreamPairsHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 10;}

    UINT32	size;
    UINT16	object_version;
    UINT16	num_stream_pairs;
    StreamPair	*stream_pairs;
    UINT8	stream1_property_name_len;
    UINT8	*stream1_property_name;
    UINT8	stream2_property_name_len;
    UINT8	*stream2_property_name;
};

class PhysicalStreamInfo
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 23;}

    UINT32	size;
    UINT16	object_version;
    UINT16	unPhysicalStreamNumber;
    UINT16	unLogicalStreamNumber;
    UINT32	ulDataOffset;
    UINT32	ulBandwidth;
    UINT8	bInterleavedBackwardsCompatible;
    UINT8	bTaggedBackwardsCompatible;
    UINT8	bIncludeAsMultirate;
    UINT8	bOwnsDataSection;
    UINT8	bIgnoreOnWrite;
};

class PhysicalStreamInfoHeader
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 8;}

    UINT32	size;
    UINT16	object_version;
    UINT16	num_physical_streams;
    PhysicalStreamInfo	*physical_streams;
};

void   RMPackUINT32(UINT32 ulValue, UINT8*& rpBuf);
UINT32 RMUnpackUINT32(UINT8*& rpBuf);
void   RMPackUINT16(UINT16 usValue, UINT8*& rpBuf);
UINT16 RMUnpackUINT16(UINT8*& rpBuf);
void   RMPackByteString(UINT8* pStr, UINT8 ucStrSize, UINT8*& rpBuf);
HXBOOL   RMUnpackByteString(UINT8*& rpStr, UINT8& rucStrSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen);
void   RMPackUINT16String(UINT8* pStr, UINT16 usStrSize, UINT8*& rpBuf);
HXBOOL   RMUnpackUINT16String(UINT8*& rpStr, UINT16& rusStrSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen);
void   RMPackUINT32Buffer(UINT8* pBuffer, UINT32 ulSize, UINT8*& rpBuf);
HXBOOL   RMUnpackUINT32Buffer(UINT8*& rpBuffer, UINT32& rulSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen);

#if defined (_DEFINE_INLINE)

HX_INLINE void RMPackUINT32(UINT32 ulValue, UINT8*& rpBuf)
{
    rpBuf[0] = (UINT8) ((ulValue >> 24) & 0x000000FF);
    rpBuf[1] = (UINT8) ((ulValue >> 16) & 0x000000FF);
    rpBuf[2] = (UINT8) ((ulValue >>  8) & 0x000000FF);
    rpBuf[3] = (UINT8) ( ulValue        & 0x000000FF);
    rpBuf   += 4;
}

HX_INLINE UINT32 RMUnpackUINT32(UINT8*& rpBuf)
{
    UINT32 ulRet = (rpBuf[0] << 24) | (rpBuf[1] << 16) | (rpBuf[2] << 8) | rpBuf[3];
    rpBuf       += 4;
    return ulRet;
}

HX_INLINE void RMPackUINT16(UINT16 usValue, UINT8*& rpBuf)
{
    rpBuf[0] = (UINT8) ((usValue >> 8) & 0x00FF);
    rpBuf[1] = (UINT8) ( usValue       & 0x00FF);
    rpBuf   += 2;
}

HX_INLINE UINT16 RMUnpackUINT16(UINT8*& rpBuf)
{
    UINT16 usRet = (rpBuf[0] << 8) | rpBuf[1];
    rpBuf       += 2;
    return usRet;
}

HX_INLINE void RMPackByteString(UINT8* pStr, UINT8 ucStrSize, UINT8*& rpBuf)
{
    *rpBuf++ = ucStrSize;
    memcpy(rpBuf, pStr, ucStrSize);
    rpBuf += ucStrSize;
}

HX_INLINE HXBOOL RMUnpackByteString(UINT8*& rpStr, UINT8& rucStrSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen)
{
    rucStrSize = *rpBuf++;
    if (rpBuf-pBuf+rucStrSize > (int)ulLen) return FALSE;
    rpStr =  rpBuf;
    rpBuf += rucStrSize;
    return TRUE;
}

HX_INLINE void RMPackUINT16String(UINT8* pStr, UINT16 usStrSize, UINT8*& rpBuf)
{
    RMPackUINT16(usStrSize, rpBuf);
    memcpy(rpBuf, pStr, usStrSize);
    rpBuf += usStrSize;
}

HX_INLINE HXBOOL RMUnpackUINT16String(UINT8*& rpStr, UINT16& rusStrSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen)
{
    rusStrSize = RMUnpackUINT16(rpBuf);
    if (rpBuf-pBuf+rusStrSize > (int)ulLen) return FALSE;
    rpStr =  rpBuf;
    rpBuf += rusStrSize;
    return TRUE;
}

HX_INLINE void RMPackUINT32Buffer(UINT8* pBuffer, UINT32 ulSize, UINT8*& rpBuf)
{
    RMPackUINT32(ulSize, rpBuf);
    memcpy(rpBuf, pBuffer, ulSize);
    rpBuf += ulSize;
}

HX_INLINE HXBOOL RMUnpackUINT32Buffer(UINT8*& rpBuffer, UINT32& rulSize, UINT8*& rpBuf, UINT8* pBuf, UINT32 ulLen)
{
    rulSize = RMUnpackUINT32(rpBuf);
    if (rpBuf-pBuf+rulSize > ulLen) return FALSE;
    rpBuffer = rpBuf;
    rpBuf   += rulSize;
    return TRUE;
}

HX_INLINE UINT8*
RMGenericHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);

    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMGenericHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);

    return off;
}

HX_INLINE UINT8*
RMFileHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0) || (object_version == 1))
    {
        RMPackUINT32(file_version, off);
        RMPackUINT32(num_headers, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMFileHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if ((object_version == 0) || (object_version == 1))
    {
        file_version = RMUnpackUINT32(off);
        num_headers  = RMUnpackUINT32(off);
    }
    return off;
}


HX_INLINE UINT8*
Properties::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0))
    {
        RMPackUINT32(max_bit_rate, off);
        RMPackUINT32(avg_bit_rate, off);
        RMPackUINT32(max_packet_size, off);
        RMPackUINT32(avg_packet_size, off);
        RMPackUINT32(num_interleave_packets, off);
        RMPackUINT32(duration, off);
        RMPackUINT32(preroll, off);
        RMPackUINT32(index_offset, off);
        RMPackUINT32(data_offset, off);
        RMPackUINT16(num_streams, off);
        RMPackUINT16(flags, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
Properties::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if ((object_version == 0))
    {
        max_bit_rate           = RMUnpackUINT32(off);
        avg_bit_rate           = RMUnpackUINT32(off);
        max_packet_size        = RMUnpackUINT32(off);
        avg_packet_size        = RMUnpackUINT32(off);
        num_interleave_packets = RMUnpackUINT32(off);
        duration               = RMUnpackUINT32(off);
        preroll                = RMUnpackUINT32(off);
        index_offset           = RMUnpackUINT32(off);
        data_offset            = RMUnpackUINT32(off);
        num_streams            = RMUnpackUINT16(off);
        flags                  = RMUnpackUINT16(off);
    }
    return off;
}


HX_INLINE UINT8*
MediaProperties::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0))
    {
        RMPackUINT16(stream_number, off);
        RMPackUINT32(max_bit_rate, off);
        RMPackUINT32(avg_bit_rate, off);
        RMPackUINT32(max_packet_size, off);
        RMPackUINT32(avg_packet_size, off);
        RMPackUINT32(start_time, off);
        RMPackUINT32(preroll, off);
        RMPackUINT32(duration, off);
        RMPackByteString(stream_name, stream_name_size, off);
        RMPackByteString(mime_type, mime_type_size, off);
        RMPackUINT32Buffer(type_specific_data, type_specific_len, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
MediaProperties::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if ((object_version == 0))
    {
        stream_number   = RMUnpackUINT16(off);
        max_bit_rate    = RMUnpackUINT32(off);
        avg_bit_rate    = RMUnpackUINT32(off);
        max_packet_size = RMUnpackUINT32(off);
        avg_packet_size = RMUnpackUINT32(off);
        start_time      = RMUnpackUINT32(off);
        preroll         = RMUnpackUINT32(off);
        duration        = RMUnpackUINT32(off);
        if (!RMUnpackByteString(stream_name, stream_name_size, off, buf, len)) return 0;
        if (!RMUnpackByteString(mime_type, mime_type_size, off, buf, len)) return 0;
        if (!RMUnpackUINT32Buffer(type_specific_data, type_specific_len, off, buf, len)) return 0;
    }
    return off;
}

HX_INLINE UINT8*
AudioTypeSpecificData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(theID, off);
    RMPackUINT16(AudioSpecificVersion, off);
    RMPackUINT16(AudioSpecificRevision, off);
    RMPackUINT32(raID, off);
    RMPackUINT32(RaChunkSize, off);
    RMPackUINT16(version, off);
    RMPackUINT16(revision, off);
    RMPackUINT16(headerBytesTotal, off);
    RMPackUINT16(compressionType, off);
    RMPackUINT32(granularity, off);
    RMPackUINT32(bytesTotal, off);
    RMPackUINT32(bytesPerMinute, off);
    RMPackUINT32(bytesPerMinute2, off);
    RMPackUINT16(interleaveFactor, off);
    RMPackUINT16(interleaveBlockSize, off);
    RMPackUINT32(userData, off);
    RMPackUINT32(sampleRate, off);
    RMPackUINT16(sampleSize, off);
    RMPackUINT16(numChannels, off);
    {memcpy(off, InterleaveCode, 5); off += 5; }
    {memcpy(off, CompressionCode, 5); off += 5; }
    *off++ = isInterleaved;
    *off++ = copyByte;
    *off++ = streamType;
    RMPackByteString(title, title_len, off);
    RMPackByteString(author, author_len, off);
    RMPackByteString(copyright, copyright_len, off);
    RMPackByteString(app, app_len, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
AudioTypeSpecificData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    theID                 = RMUnpackUINT32(off);
    AudioSpecificVersion  = RMUnpackUINT16(off);
    AudioSpecificRevision = RMUnpackUINT16(off);
    raID                  = RMUnpackUINT32(off);
    RaChunkSize           = RMUnpackUINT32(off);
    version               = RMUnpackUINT16(off);
    revision              = RMUnpackUINT16(off);
    headerBytesTotal      = RMUnpackUINT16(off);
    compressionType       = RMUnpackUINT16(off);
    granularity           = RMUnpackUINT32(off);
    bytesTotal            = RMUnpackUINT32(off);
    bytesPerMinute        = RMUnpackUINT32(off);
    bytesPerMinute2       = RMUnpackUINT32(off);
    interleaveFactor      = RMUnpackUINT16(off);
    interleaveBlockSize   = RMUnpackUINT16(off);
    userData              = RMUnpackUINT32(off);
    sampleRate            = RMUnpackUINT32(off);
    sampleSize            = RMUnpackUINT16(off);
    numChannels           = RMUnpackUINT16(off);
    if (off-buf+5 > (int)len)
	return 0;
    {memcpy(InterleaveCode, off, 5); off += 5; }
    if (off-buf+5 > (int)len)
	return 0;
    {memcpy(CompressionCode, off, 5); off += 5; }
    isInterleaved = *off++;
    copyByte      = *off++;
    streamType    = *off++;
    if (!RMUnpackByteString(title, title_len, off, buf, len)) return 0;
    if (!RMUnpackByteString(author, author_len, off, buf, len)) return 0;
    if (!RMUnpackByteString(copyright, copyright_len, off, buf, len)) return 0;
    if (!RMUnpackByteString(app, app_len, off, buf, len)) return 0;
    return off;
}

HX_INLINE VideoTypeSpecificData::VideoTypeSpecificData() :
    cbLength(0),
    moftag(0),
    submoftag(0),
    uiWidth(0),
    uiHeight(0),
    uiBitCount(0),
    uiPadWidth(0),
    uiPadHeight(0),
    framesPerSecond(0),
    ulSPOExtra(0),
    ulStreamVersion(0)
{
}

HX_INLINE UINT8*
VideoTypeSpecificData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(cbLength, off);
    RMPackUINT32(moftag, off);
    RMPackUINT32(submoftag, off);
    RMPackUINT16(uiWidth, off);
    RMPackUINT16(uiHeight, off);
    RMPackUINT16(uiBitCount, off);
    RMPackUINT16(uiPadWidth, off);
    RMPackUINT16(uiPadHeight, off);
    RMPackUINT32(framesPerSecond, off);
    UINT32 ulBytesPacked = off - buf;
    UINT32 ulBytesLeft   = (len >= ulBytesPacked ? len - ulBytesPacked : 0);
    if (ulBytesLeft >= 8)
    {
        RMPackUINT32(ulSPOExtra, off);
        RMPackUINT32(ulStreamVersion, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
VideoTypeSpecificData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    cbLength        = RMUnpackUINT32(off);
    moftag          = RMUnpackUINT32(off);
    submoftag       = RMUnpackUINT32(off);
    uiWidth         = RMUnpackUINT16(off);
    uiHeight        = RMUnpackUINT16(off);
    uiBitCount      = RMUnpackUINT16(off);
    uiPadWidth      = RMUnpackUINT16(off);
    uiPadHeight     = RMUnpackUINT16(off);
    framesPerSecond = RMUnpackUINT32(off);
    UINT32 ulBytesParsed = off - buf;
    UINT32 ulBytesLeft   = (len >= ulBytesParsed ? len - ulBytesParsed : 0);
    if (ulBytesLeft >= 8)
    {
        ulSPOExtra      = RMUnpackUINT32(off);
        ulStreamVersion = RMUnpackUINT32(off);
    }
    return off;
}

HX_INLINE UINT8*
MultiStreamHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT16(num_rules, off);
    for (int i = 0;  i < num_rules; i++)
        RMPackUINT16(rule_to_header_map[i], off);
    RMPackUINT16(num_headers, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
MultiStreamHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id = RMUnpackUINT32(off);
    num_rules = RMUnpackUINT16(off);
    rule_to_header_map = new UINT16[num_rules];
    if (!rule_to_header_map) return 0;
    for (int i = 0;  i < num_rules; i++)
        rule_to_header_map[i] = RMUnpackUINT16(off);
    num_headers = RMUnpackUINT16(off);
    return off;
}

HX_INLINE NameValueProperty::NameValueProperty()
{
    size           = 0;
    object_version = 0;
    name_length    = 0;
    name           = NULL;
    value_length   = 0;
    value_data     = NULL;
}

HX_INLINE UINT8*
NameValueProperty::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0))
    {
        RMPackByteString(name, name_length, off);
        RMPackUINT32(type, off);
        RMPackUINT16String(value_data, value_length, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
NameValueProperty::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    size = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if ((object_version == 0))
    {
        if (!RMUnpackByteString(name, name_length, off, buf, len)) return 0;
        type = RMUnpackUINT32(off);
        if (!RMUnpackUINT16String(value_data, value_length, off, buf, len)) return 0;
    }
    return off;
}

HX_INLINE UINT8*
Unsigned32BitInteger::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(value, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
Unsigned32BitInteger::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    value = RMUnpackUINT32(off);
    return off;
}

HX_INLINE UINT8*
LogicalStream::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0))
    {
        RMPackUINT16(num_physical_streams, off);
        int i = 0;
        for (i = 0;  i < num_physical_streams; i++)
            RMPackUINT16(physical_stream_numbers[i], off);
        for (i = 0;  i < num_physical_streams; i++)
            RMPackUINT32(data_offsets[i], off);
        RMPackUINT16(num_rules, off);
        for (i = 0;  i < num_rules; i++)
            RMPackUINT16(rule_to_physical_stream_number_map[i], off);
        RMPackUINT16(num_properties, off);
        for (i = 0;  i < num_properties; i++)
            off = properties[i].pack(off, len);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
LogicalStream::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
        return 0;
    UINT8* off = buf;

    UINT32 buf_size = 0;
    size = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if ((object_version == 0))
    {
        num_physical_streams = RMUnpackUINT16(off);

	buf_size = num_physical_streams * sizeof(UINT16);

	// array of num_physical_streams UINT16's following parsing buffer
	// should be less than buffer length.
	if (buf_size > len)
	{
		return 0;
	}

        physical_stream_numbers = new UINT16[num_physical_streams];
        if (!physical_stream_numbers) return 0;
        int i = 0;
        for (i = 0;  i < num_physical_streams; i++)
            physical_stream_numbers[i] = RMUnpackUINT16(off);
        data_offsets = new UINT32[num_physical_streams];
        if (!data_offsets) return 0;
        for (i = 0;  i < num_physical_streams; i++)
            data_offsets[i] = RMUnpackUINT32(off);
        num_rules = RMUnpackUINT16(off);
        rule_to_physical_stream_number_map = new UINT16[num_rules];
        if (!rule_to_physical_stream_number_map) return 0;
        for (i = 0;  i < num_rules; i++)
            rule_to_physical_stream_number_map[i] = RMUnpackUINT16(off);
        num_properties = RMUnpackUINT16(off);
        properties = new NameValueProperty[num_properties];
        if (!properties) return 0;
        for (i = 0;  i < num_properties && off; i++)
            off = properties[i].unpack(off,
                                      ((len > ((UINT32) (off - buf))) ? len - (off - buf) : 0));
    }
    return off;
}

HX_INLINE UINT8*
MetaInformation::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if ((object_version == 0))
    {
        RMPackUINT16(num_properties, off);
        for (int i = 0;  i < num_properties; i++)
            off = properties[i].pack(off, len);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
MetaInformation::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        num_properties = RMUnpackUINT16(off);
        properties = new NameValueProperty[num_properties];
        if (!properties) return 0;
        for (int i = 0;  i < num_properties; i++)
            off = properties[i].unpack(off, len);
    }
    return off;
}

HX_INLINE UINT8*
Content::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT16String(title, title_len, off);
        RMPackUINT16String(author, author_len, off);
        RMPackUINT16String(copyright, copyright_len, off);
        RMPackUINT16String(comment, comment_len, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
Content::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        if (!RMUnpackUINT16String(title, title_len, off, buf, len)) return 0;
        if (!RMUnpackUINT16String(author, author_len, off, buf, len)) return 0;
        if (!RMUnpackUINT16String(copyright, copyright_len, off, buf, len)) return 0;
        if (!RMUnpackUINT16String(comment, comment_len, off, buf, len)) return 0;
    }
    return off;
}


//
// BUGBUG
// XXX
// PMC can't generate a declaration for this constructor.
// So, the generated .h file will not build until the
// declaration is added by hand.
//
HX_INLINE
Content::Content()
{
	object_id = 0;
	size = 0;
	object_version = 0;
	title_len = 0;
	title = NULL;
	author_len = 0;
	author = NULL;
	copyright_len = 0;
	copyright = NULL;
	comment_len = 0;
	comment = NULL;
}

HX_INLINE UINT8*
DataHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT32(num_interleave_packets, off);
        RMPackUINT32(next_data_header, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
DataHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        num_interleave_packets = RMUnpackUINT32(off);
        next_data_header       = RMUnpackUINT32(off);
    }
    return off;
}

HX_INLINE
IndexHeader::IndexHeader()
{
    object_id = 0;
    size = 0;
    object_version = 0;
    num_indices = 0;
    stream_number = 0;
    next_index_header = 0;
}

HX_INLINE UINT8*
IndexHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT32(num_indices, off);
        RMPackUINT16(stream_number, off);
        RMPackUINT32(next_index_header, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
IndexHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        num_indices       = RMUnpackUINT32(off);
        stream_number     = RMUnpackUINT16(off);
        next_index_header = RMUnpackUINT32(off);
    }
    return off;
}

HX_INLINE UINT8*
IndexRecord::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT32(timestamp, off);
        RMPackUINT32(offset, off);
        RMPackUINT32(num_interleave_packets, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
IndexRecord::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        timestamp              = RMUnpackUINT32(off);
        offset                 = RMUnpackUINT32(off);
        num_interleave_packets = RMUnpackUINT32(off);
    }
    return off;
}

HX_INLINE UINT8*
PacketHeaderBase::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(object_version, off);
    RMPackUINT16(length, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
PacketHeaderBase::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_version = RMUnpackUINT16(off);
    length         = RMUnpackUINT16(off);
    return off;
}

HX_INLINE UINT8*
PacketHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT16(length, off);
        RMPackUINT16(stream_number, off);
        RMPackUINT32(timestamp, off);
        RMPackUINT16(flags, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
PacketHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        length        = RMUnpackUINT16(off);
        stream_number = RMUnpackUINT16(off);
        timestamp     = RMUnpackUINT32(off);
        flags         = RMUnpackUINT16(off);
    }
    return off;
}

HX_INLINE UINT8*
PacketHeader1::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(object_version, off);
    if (object_version == 1)
    {
        RMPackUINT16(length, off);
        RMPackUINT16(stream_number, off);
        RMPackUINT32(timestamp, off);
        RMPackUINT16(asm_rule, off);
        *off++ = asm_flags;
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
PacketHeader1::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_version = RMUnpackUINT16(off);
    if (object_version == 1)
    {
        length        = RMUnpackUINT16(off);
        stream_number = RMUnpackUINT16(off);
        timestamp     = RMUnpackUINT32(off);
        asm_rule      = RMUnpackUINT16(off);
        asm_flags     = *off++;
    }
    return off;
}

HX_INLINE UINT8*
Profile::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(object_id, off);
    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    if (object_version == 0)
    {
        RMPackUINT32(Bandwidth, off);
        RMPackUINT32(CpuPower, off);
    }
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
Profile::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    object_id      = RMUnpackUINT32(off);
    size           = RMUnpackUINT32(off);
    object_version = RMUnpackUINT16(off);
    if (object_version == 0)
    {
        Bandwidth = RMUnpackUINT32(off);
        CpuPower  = RMUnpackUINT32(off);
    }
    return off;
}

HX_INLINE UINT8*
RMEventPacketData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(sequence_number, off);
    RMPackUINT32(start_time_deciseconds, off);
    RMPackUINT32(stop_time_deciseconds, off);
    RMPackUINT16(event_type_and_string_len, off);
    RMPackUINT16(event_type_id, off);
    memcpy(off, event_string, event_type_and_string_len - 2);
    off += event_type_and_string_len - 2;
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMEventPacketData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    sequence_number           = RMUnpackUINT16(off);
    start_time_deciseconds    = RMUnpackUINT32(off);
    stop_time_deciseconds     = RMUnpackUINT32(off);
    event_type_and_string_len = RMUnpackUINT16(off);
    event_type_id             = RMUnpackUINT16(off);
    if (off-buf+(event_type_and_string_len - 2) > (int)len)
	return 0;
    event_string = (UINT8*) off;
    off += event_type_and_string_len - 2;
    return off;
}

HX_INLINE UINT8*
RMImageMapRegionData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(start_time, off);
    RMPackUINT32(end_time, off);
    RMPackUINT16(shape, off);
    RMPackUINT16(num_values, off);
    for (int i = 0;  i < num_values; i++)
        RMPackUINT16(values[i], off);
    RMPackUINT16(action, off);
    RMPackUINT32(seek_time, off);
    RMPackUINT16String(url, url_len, off);
    RMPackUINT16String(status, status_len, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMImageMapRegionData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    start_time = RMUnpackUINT32(off);
    end_time   = RMUnpackUINT32(off);
    shape      = RMUnpackUINT16(off);
    num_values = RMUnpackUINT16(off);
    values     = new UINT16[num_values];
    if (!values) return 0;
    for (int i = 0;  i < num_values; i++)
        values[i] = RMUnpackUINT16(off);
    action    = RMUnpackUINT16(off);
    seek_time = RMUnpackUINT32(off);
    if (!RMUnpackUINT16String(url, url_len, off, buf, len)) return 0;
    if (!RMUnpackUINT16String(status, status_len, off, buf, len)) return 0;
    return off;
}

HX_INLINE UINT8*
RMImageMapPacketData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(sequence_number, off);
    RMPackUINT16(num_regions, off);
    RMPackUINT32(start_time, off);
    RMPackUINT32(end_time, off);
    RMPackUINT16(left, off);
    RMPackUINT16(top, off);
    RMPackUINT16(right, off);
    RMPackUINT16(bottom, off);
    for (int i = 0;  i < num_regions; i++)
        off = regions[i].pack(off, len);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMImageMapPacketData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    sequence_number = RMUnpackUINT16(off);
    num_regions     = RMUnpackUINT16(off);
    start_time      = RMUnpackUINT32(off);
    end_time        = RMUnpackUINT32(off);
    left            = RMUnpackUINT16(off);
    top             = RMUnpackUINT16(off);
    right           = RMUnpackUINT16(off);
    bottom          = RMUnpackUINT16(off);
    regions = new RMImageMapRegionData[num_regions];
    if (!regions) return 0;
    for (int i = 0;  i < num_regions; i++)
        off = regions[i].unpack(off, len);
    return off;
}

HX_INLINE UINT8*
RMBaseImageMapPacketData::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(sequence_number, off);
    RMPackUINT16(num_regions, off);
    RMPackUINT32(start_time, off);
    RMPackUINT32(end_time, off);
    RMPackUINT16(left, off);
    RMPackUINT16(top, off);
    RMPackUINT16(right, off);
    RMPackUINT16(bottom, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
RMBaseImageMapPacketData::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    sequence_number = RMUnpackUINT16(off);
    num_regions     = RMUnpackUINT16(off);
    start_time      = RMUnpackUINT32(off);
    end_time        = RMUnpackUINT32(off);
    left            = RMUnpackUINT16(off);
    top             = RMUnpackUINT16(off);
    right           = RMUnpackUINT16(off);
    bottom          = RMUnpackUINT16(off);
    return off;
}

HX_INLINE UINT8*
StreamPair::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT16(stream1_num, off);
    RMPackUINT16(stream2_num, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
StreamPair::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    stream1_num = RMUnpackUINT16(off);
    stream2_num = RMUnpackUINT16(off);
    return off;
}

HX_INLINE UINT8*
StreamPairsHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    RMPackUINT16(num_stream_pairs, off);
    for (int i = 0;  i < num_stream_pairs; i++)
        off = stream_pairs[i].pack(off, len);
    RMPackByteString(stream1_property_name, stream1_property_name_len, off);
    RMPackByteString(stream2_property_name, stream2_property_name_len, off);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
StreamPairsHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    size             = RMUnpackUINT32(off);
    object_version   = RMUnpackUINT16(off);
    num_stream_pairs = RMUnpackUINT16(off);
    stream_pairs = new StreamPair[num_stream_pairs];
    if (!stream_pairs) return 0;
    for (int i = 0;  i < num_stream_pairs; i++)
        off = stream_pairs[i].unpack(off, len);
    if (!RMUnpackByteString(stream1_property_name, stream1_property_name_len, off, buf, len)) return 0;
    if (!RMUnpackByteString(stream2_property_name, stream2_property_name_len, off, buf, len)) return 0;
    return off;
}

HX_INLINE UINT8*
PhysicalStreamInfo::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    RMPackUINT16(unPhysicalStreamNumber, off);
    RMPackUINT16(unLogicalStreamNumber, off);
    RMPackUINT32(ulDataOffset, off);
    RMPackUINT32(ulBandwidth, off);
    *off++ = bInterleavedBackwardsCompatible;
    *off++ = bTaggedBackwardsCompatible;
    *off++ = bIncludeAsMultirate;
    *off++ = bOwnsDataSection;
    *off++ = bIgnoreOnWrite;
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
PhysicalStreamInfo::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    size                   = RMUnpackUINT32(off);
    object_version         = RMUnpackUINT16(off);
    unPhysicalStreamNumber = RMUnpackUINT16(off);
    unLogicalStreamNumber  = RMUnpackUINT16(off);
    ulDataOffset           = RMUnpackUINT32(off);
    ulBandwidth            = RMUnpackUINT32(off);
    bInterleavedBackwardsCompatible = *off++;
    bTaggedBackwardsCompatible      = *off++;
    bIncludeAsMultirate             = *off++;
    bOwnsDataSection                = *off++;
    bIgnoreOnWrite                  = *off++;
    return off;
}

HX_INLINE UINT8*
PhysicalStreamInfoHeader::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    RMPackUINT32(size, off);
    RMPackUINT16(object_version, off);
    RMPackUINT16(num_physical_streams, off);
    for (int i = 0;  i < num_physical_streams; i++)
        off = physical_streams[i].pack(off, len);
    len = off-buf;
    return off;
}

HX_INLINE UINT8*
PhysicalStreamInfoHeader::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    size                 = RMUnpackUINT32(off);
    object_version       = RMUnpackUINT16(off);
    num_physical_streams = RMUnpackUINT16(off);
    physical_streams = new PhysicalStreamInfo[num_physical_streams];
    if (!physical_streams) return 0;
    for (int i = 0;  i < num_physical_streams; i++)
        off = physical_streams[i].unpack(off, len);
    return off;
}


HX_INLINE u_int32
get_rm_object_id(Byte* buffer, int buffer_len)
{
    RMGenericHeader	gen_header;
    if (gen_header.unpack(buffer, buffer_len) == 0)
    {
	return 0;
    }
    return gen_header.object_id;
}
#endif //_DEFINE_INLINE


#if (defined( _WIN32 ) || defined( _WINDOWS )) && defined(_M_IX86)
#pragma pack()

//	Restore warnings
#pragma warning( default : 4200 )
#endif
#ifdef _MACINTOSH
#pragma options align=reset
#endif

#endif

