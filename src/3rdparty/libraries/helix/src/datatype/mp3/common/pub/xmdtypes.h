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

#ifndef _XMDTYPES_H_
#define _XMDTYPES_H_

#include "hxtypes.h"

// Colorspace types supported by Colorspace converter
typedef enum XMDColorSpaceTag
{
    eXMDColorSpaceMCS3,
    eXMDColorSpaceMC12,
    eXMDColorSpaceYV12,
    eXMDColorSpaceYUY2,
    eXMDColorSpaceUYVY,
    eXMDColorSpaceVBPL,
    eXMDColorSpaceCLPL,
    eXMDColorSpaceCLJR,
    eXMDColorSpaceRGB8,
    eXMDColorSpaceRGB555,
    eXMDColorSpaceRGB565,
    eXMDColorSpaceRGB24,
    eXMDColorSpaceRGB32,
    eXMDColorSpaceNone
} XMDColorSpace;


// The order (value) of these frame type enums must be this way.
// This maps directly to the MPEG spec
typedef enum XMDFrameTypeTag
{
    eXMDFrameTypeNone   = 0,
    eXMDFrameTypeI      = 1,
    eXMDFrameTypeP      = 2,
    eXMDFrameTypeB      = 3,
    eXMDFrameTypeD      = 4
} XMDFrameType;

// Used as return values for API calls
typedef enum XMDErrorTag
{
    eXMDErrorOk,                    // No error
    eXMDErrorError,                 // Generic error
    eXMDErrorInvalidParameter,      // Invalid parameter to function
    eXMDErrorDecoderNotFound,       // Decoder DLL not found or not ready
    eXMDErrorInvalidWindow,         // Invalid window handle
    eXMDErrorDecodeError,           // Error decoding
    eXMDErrorInvalidDecoderVersion, // Incorrect decoder dll
    eXMDErrorCurrentlyOpen,         // Already open
    eXMDErrorDecoderNotReady,       // Decoder probably busy
    eXMDErrorNotEnoughData,         // Ran out of data
    eXMDErrorHardware,              // Hardware failure
    eXMDErrorFileRead,              // File read error
    eXMDErrorInvalidFile,           // Bad MPEG file
    eXMDErrorOutOfRange             // Parameter out of range
} XMDError;

// Used to describe the state of the YUV buffers during decode
typedef struct XMDBufferDescriptionTag
{
    BYTE            *pDestBufy;
    BYTE            *pDestBufuv;
    BYTE            *pRefForwardBufy;
    BYTE            *pRefForwardBufuv;
    BYTE            *pRefBackBufy;
    BYTE            *pRefBackBufuv;
    XMDFrameType    eDestBufType;
    XMDFrameType    eForwardBufType;
    XMDFrameType    eBackBufType;
    INT16           iDestBufNum;
    LONG32          lDestBufOrd;
    INT16           iForwardBufNum;
    LONG32          lForwardBufOrd;
    INT16           iBackBufNum;
    LONG32          lBackBufOrd;
    ULONG32         dwNumI;
    ULONG32         dwNumP;
    ULONG32         dwNumB;
    INT16           iShowBufNum;
    LONG32          lShowBufOrd;
    INT16           iWidth;
    INT16           iHeight;
}XMDBufferDescription;

typedef enum
{
    NO_CODE                 = 0xFFFFFFFF,
    SEQUENCE_HEADER_CODE    = 0x000001B3,
    SEQUENCE_END_CODE       = 0x000001B7,
    GOP_START_CODE          = 0x000001B8,
    USER_DATA_START_CODE    = 0x000001B2,
    EXTENSION_START_CODE    = 0x000001B5,
    PACK_START_CODE         = 0x000001BA,
    PIC_START_CODE          = 0x00000100,
    FIRST_SLICE_CODE        = 0x00000101,
    START_CODE_SIGNTR       = 0x00000001
}eStartCode;

enum
{
    SEQUENCE_DISPLAY_EXT = 0x2,
    PICTURE_DISPLAY_EXT  = 0x7
};

// MPEG2 Bit Flags
enum
{
    PIC_EXT_TOP_FIELD_FIRST = 0x80,
    PIC_EXT_FRAME_PRED_FRAME_DCT = 0x40,
    PIC_EXT_CONCEALMENT_MOTION_VECTORS = 0x20,
    PIC_EXT_Q_SCALE_TYPE = 0x10,
    PIC_EXT_INTRA_VIC_FORMAT = 0x08,
    PIC_EXT_ALTERNATE_SCAN = 0x04,
    PIC_EXT_REPEAT_FIRST_FIELD = 0x02,
    PIC_EXT_PROGRESSIVE_FRAME = 0x01,

    PIC_INTRA_DC_PREC =         0x0C000,
    PIC_STRUCT =                0x03000,
    PIC_TYPE =                  0x00C00,
    PIC_DCT_TYPE =              0x00100,

    PIC_STRUCT_TOP_FIELD =      0x01000,
    PIC_STRUCT_BOTTOM_FIELD =   0x02000
};

enum
{
    TIME_STAMP_UNITS =          10000000,
    UNITS_CONVERSION_MS =       10000
};

enum
{
    MPEG_BITS_PER_PEL =         12
};


#define ALIGNED_HEIGHT(h) ((h + 15) & 0xFFF0)
#define UNITS_TO_MS(x)  (x) / UNITS_CONVERSION_MS

const INT64 PictureTimes[16] =
{
    0,
    (INT64)((double)TIME_STAMP_UNITS / 23.976),
    (INT64)((double)TIME_STAMP_UNITS / 24),
    (INT64)((double)TIME_STAMP_UNITS / 25),
    (INT64)((double)TIME_STAMP_UNITS / 29.97),
    (INT64)((double)TIME_STAMP_UNITS / 30),
    (INT64)((double)TIME_STAMP_UNITS / 50),
    (INT64)((double)TIME_STAMP_UNITS / 59.94),
    (INT64)((double)TIME_STAMP_UNITS / 60)
};

struct tMPEG_SEQ_HDR
{
    INT32           display_pels_width;
    INT32           display_pels_height;
    INT32           pel_aspect_ratio;
    INT32           picture_rate;
    UINT32          bit_rate;
    INT32           vbv_buf_size;
    INT32           constrained_stream_flag;
    INT32           intra_q_matrix_flag;
    UINT8           intra_q_matrix[64];
    INT32           non_intra_q_matrix_flag;
    UINT8           non_intra_q_matrix[64];
    INT32           ext_data_flag;          // not part of MPEG spec
    UINT32          ext_buf_size;           // not part of MPEG spec
    INT8            ext_data[32];
    UINT32          ext_data_size;          // not part of MPEG spec
    UINT32          user_buf_size;          // not part of MPEG spec
    INT32           user_data_flag;
    INT8            user_data[32];
    UINT32          user_data_size;         // not part of MPEG spec
    UINT8           bEncrypted;             // Is this an encrypted ReelMagic file?
    INT64           picture_time;           // In 90 Nano Seconds
};

typedef struct
{
    UINT8           *pSurface,
                    *pY, *pUV,
                    *pYDisplay,
                    *pUVDisplay,
                    bDecodeSuccess;
    INT32           lPitch;
    UINT32          lPts;
    INT16           nBufferIndex;           // The buffer index of this buffer
    INT16           nShowBufferIndex;       // If this buffer decoded successfully,
                                            //   the number of the buffer that should be shown
} tDisplayObj;

typedef struct
{
    XMDFrameType    eFrame;
    
    UCHAR           *pData;

    UINT32          ulFrameSize,
                    ulWidth,
                    ulHeight,
		    ulTimeStamp;

    UINT8           bRepeatField,
                    bDynamicChange,
                    bSequenceEnd,
                    bSequenceHeader,
                    bStillFrame;

    tMPEG_SEQ_HDR   seqHdr;

} tFrameObj;

typedef enum 
{
    eNoError,
    eOutOfData,
    eOutOfMemory,
    eInvalidData,
    eDecoderNotInitialized,
    eBufferNotAvailable,
    eUnknown
} eErr;

#endif
