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

#ifdef __MWERKS__
#pragma once
#endif

#ifndef _RACODEC
#define _RACODEC

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"

/*
 *  CONSTANTS
 */

#define MAX_CODEC_ID_LENGTH 	5
#define MAX_CODEC_NAME_LENGTH	32

#define HX_MAX_AUDIO_QUALITY  	0
#define HX_MIN_AUDIO_QUALITY  	5

// generic error return value from codec
#define HX_CODEC_ERROR	-1

/*
 *  DATA TYPES
 */

// set struct alignment for Mac
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#pragma options align=mac68k
#endif


// for Cookie API
typedef void* RACODEC;

//
//  Define a typedef to get around armcc compiler error
//
typedef void* VOID_PTR;

typedef struct audio_format
{
	UINT16  FormatTag;
	UINT32  SamplesPerSec;
	UINT16  BitsPerSample;
	UINT16  Channels;

} HX_AUDIO_FORMAT;

typedef enum
{
    CONTENT_VOICE_ONLY,
    CONTENT_MUSIC_ONLY,
//  $Comstrip:	IF SDK
//    CONTENT_VOICE_AND_MUSIC
//  $Comstrip:	ELSE
    CONTENT_VOICE_AND_MUSIC,
    CONTENT_VIDEO_ONLY
//  $Comstrip:	ENDIF SDK
} HX_CONTENT_TYPE;

typedef enum
{                                 /* Property Type           */
//    FLV_PROP_NAME,                /* char[]                  */
//    FLV_PROP_DESCRIPTION,         /* char[]                  */
//    FLV_PROP_STATUS_TEXT,         /* char[]                  */
//    FLV_PROP_BIT_RATE,            /* UINT32                  */
//    FLV_PROP_INPUT_AUDIO_FORMAT,  /* HX_AUDIO_FORMAT         */
//    FLV_PROP_OUTPUT_AUDIO_FORMAT, /* HX_AUDIO_FORMAT         */
//    FLV_PROP_CONTENT_TYPE,        /* HX_CONTENT_TYPE (UINT16)*/
//    FLV_PROP_GRANULARITY,         /* UINT32                  */
//    FLV_PROP_AUDIO_BANDWIDTH,
//    FLV_PROP_COUPLING_BEGIN,
//    FLV_PROP_FRAME_SIZE,
//    FLV_PROP_SAMPLES_IN,
//    FLV_PROP_INTERLEAVE_FACTOR,
//    FLV_PROP_INTERLEAVE_TYPE,
//    FLV_PROP_INTERLEAVE_PATTERN,
//    FLV_PROP_OPAQUE_DATA,
//    FLV_PROP_USER = 1000
    FLV_PROP_NAME,                /* char[]                  */
    FLV_PROP_BIT_RATE,            /* ULONG32                 */
    FLV_PROP_INPUT_AUDIO_FORMAT,  /* HX_AUDIO_FORMAT         */
    FLV_PROP_OUTPUT_AUDIO_FORMAT, /* HX_AUDIO_FORMAT         */
    FLV_PROP_DESCRIPTION,         /* char[]                  */
    FLV_PROP_CONTENT_TYPE,        /* HX_CONTENT_TYPE (UINT16)*/
    FLV_PROP_GRANULARITY,         /* ULONG32                 */
    FLV_PROP_STATUS_TEXT,         /* char[]                  */
    FLV_PROP_RV_NAME,             /* char[]                  */
    FLV_PROP_RV_DESCRIPTION,      /* char[]                  */
    FLV_PROP_RV_STATUS_TEXT,      /* char[]                  */
    FLV_PROP_AUDIO_BANDWIDTH,
    FLV_PROP_COUPLING_BEGIN,
    FLV_PROP_FRAME_SIZE,
    FLV_PROP_SAMPLES_IN,
    FLV_PROP_INTERLEAVE_FACTOR,
    FLV_PROP_INTERLEAVE_TYPE,
    FLV_PROP_INTERLEAVE_PATTERN,
    FLV_PROP_OPAQUE_DATA,
    FLV_PROP_FREQ_RESPONSE,       /* UINT32                  */
    FLV_PROP_BITS_PER_FRAME,
    FLV_PROP_USER = 1000

} HX_FLV_PROPERTY;

// Decoder API

typedef struct radecoder_init_params
{
	UINT32		sampleRate;
	UINT16		bitsPerSample;
	UINT16		channels;
	UINT16		audioQuality;
	UINT32		bitsPerFrame;
	UINT32		granularity;
	UINT32		opaqueDataLength;
	BYTE*		opaqueData;

} RADECODER_INIT_PARAMS;


// Encoder API

typedef struct raencoder_info
{
	UINT32		granularity;
	UINT32		sampleRate;
	UINT16		bitsPerSample;
	UINT16		channels;
	UINT16		compressionType;
	char		compressionCode[MAX_CODEC_ID_LENGTH]; /* Flawfinder: ignore */
	char		displayName[MAX_CODEC_NAME_LENGTH]; /* Flawfinder: ignore */

} RAENCODER_INFO;

typedef struct raencoder_init_params
{
	INT32		numSamplesIn;
	INT32		numBytesOut;
	UINT32		sampleRate;
	UINT16		bitsPerSample;
	UINT16		channels;
	UINT16		flvIndex;

} RAENCODER_INIT_PARAMS;

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#pragma options align=reset
#endif

/*
 *  PROTOTYPES
 */

extern "C"
{
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAOpenCodec) (RACODEC* pCodecRef);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAOpenCodec2) (RACODEC* pCodecRef, const char* pCodecPath);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RACloseCodec) (RACODEC codecRef);

    STDAPI_(void) ENTRYPOINTCALLTYPE ENTRYPOINT(RASetPwd) (RACODEC codecRef, const char* pPwd);

    STDAPI_(UINT16) ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetNumberOfFlavors)(RACODEC codecRef);
    STDAPI_(UINT16) ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetNumberOfFlavors2)(RACODEC codecRef);

    STDAPI_(VOID_PTR) ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetFlavorProperty)(RACODEC codecRef, UINT16 flvIndex, UINT16 propIndex, UINT16* pSize);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RASetFlavor)(RACODEC codecRef, UINT16 flvIndex);

    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAInitEncoder) (RACODEC codecRef, void* pParam );
    STDAPI_(void) ENTRYPOINTCALLTYPE ENTRYPOINT(RAEncode) (RACODEC codecRef, UINT16* inBuf, Byte* outBuf);
    STDAPI_(void) ENTRYPOINTCALLTYPE ENTRYPOINT(RAFreeEncoder) (RACODEC codecRef);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetBackend) (RACODEC codecRef, void*** pFuncList);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetGUID) (UCHAR* pGUID);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetDecoderBackendGUID) (RACODEC codecRef, UCHAR* pGUID);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RASetComMode) (RACODEC codecRef);

    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAInitDecoder) (RACODEC codecRef, void* pParam);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RADecode) (RACODEC codecRef, Byte* in, UINT32 inLength, Byte* out, UINT32* pOutLength, UINT32 userData);
    STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAFlush) (RACODEC codecRef, Byte* outBuf, UINT32* pOutLength);
    STDAPI_(void) ENTRYPOINTCALLTYPE ENTRYPOINT(RAFreeDecoder) (RACODEC codecRef);
}

#endif // #ifndef _RACODEC
