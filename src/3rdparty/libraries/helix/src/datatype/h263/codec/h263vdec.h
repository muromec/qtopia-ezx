/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263vdec.h,v 1.5 2009/04/16 17:15:13 gahluwalia Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef __H263VDEC_H__
#define __H263VDEC_H__

#include "hxtypes.h"
//#include "hxcodec.h"


#if defined(_MSC_VER)
///////////////////////////////////
//
// Begin 1-byte structure alignment
//
///////////////////////////////////
// RealVideo front end files use 1-byte structure alignment under MSVC,
// but the backend is compiled with 8-byte alignment.
// This pragma keeps the structure alignment independent of compiler options,
// for all structures declared in this file.
#pragma pack(push, 1)
#endif

// Byte alignment settings for Mac
#if defined( _MACINTOSH )
#pragma options align=mac68k
#endif


#ifdef __cplusplus
extern "C" {	/* Assume C declarations for C++ */
#endif /* __cplusplus */


#define RV_DECODE_MORE_FRAMES   	    0x00000001
#define RV_DECODE_DONT_DRAW     	    0x00000002
#define RV_DECODE_KEY_FRAME				0x00000004
    // Indicates that the decompressed image is a key frame.
    // Note that enhancement layer EI frames are not key frames, in the
    // traditional sense, because they have dependencies on lower layer
    // frames.

#define RV_DECODE_B_FRAME				0x00000008
    // Indicates that the decompressed image is a B frame.
    // At most one of PIA_DDN_KEY_FRAME and PIA_DDN_B_FRAME will be set.

#ifndef TAG_HXCODEC_SEGMENTINFO
#define TAG_HXCODEC_SEGMENTINFO
typedef struct tag_HXCODEC_SEGMENTINFO
{
	LONG32 bIsValid;
	ULONG32 ulSegmentOffset;
} HXCODEC_SEGMENTINFO;
#define HXCODEC_SEGMENTINFO_SIZE	(8)
#endif

typedef struct tag_H263DecoderInParams
{
	ULONG32 dataLength;
	LONG32	bInterpolateImage;
	ULONG32 numDataSegments;
	HXCODEC_SEGMENTINFO *pDataSegments;
	ULONG32 flags;
        // 'flags' should be initialized by the front-end before each
        // invocation to decompress a frame.  It is not updated by the decoder.
        //
        // If it contains RV_DECODE_MORE_FRAMES, it informs the decoder
        // that it is being called to extract the second or subsequent
        // frame that the decoder is emitting for a given input frame.
        // The front-end should set this only in response to seeing
        // an RV_DECODE_MORE_FRAMES indication in H263DecoderOutParams.
        //
        // If it contains RV_DECODE_DONT_DRAW, it informs the decoder
        // that it should decode the image (in order to produce a valid
        // reference frame for subsequent decoding), but that no image
        // should be returned.  This provides a "hurry-up" mechanism.
	ULONG32 timestamp;
} H263DecoderInParams;

typedef struct tag_H263DecoderOutParams
{
	ULONG32 numFrames;
	ULONG32 notes;
        // 'notes' is assigned by the transform function during each call to
        // decompress a frame.  If upon return the notes parameter contains
        // the indication RV_DECODE_MORE_FRAMES, then the front-end
        // should invoke the decoder again to decompress the same image.
        // For this additional invocation, the front-end should first set
        // the RV_DECODE_MORE_FRAMES bit in the 'H263DecoderInParams.flags'
        // member, to indicate to the decoder that it is being invoked to
        // extract the next frame.
        // The front-end should continue invoking the decoder until the
        // RV_DECODE_MORE_FRAMES bit is not set in the 'notes' member.
        // For each invocation to decompress a frame in the same "MORE_FRAMES"
        // loop, the front-end should send in the same input image.
        //
        // If the decoder has no frames to return for display, 'numFrames' will
        // be set to zero.  To avoid redundancy, the decoder does *not* set
        // the RV_DECODE_DONT_DRAW bit in 'notes' in this case.


	ULONG32 timestamp;
        // The 'temporal_offset' parameter is used in conjunction with the
        // RV_DECODE_MORE_FRAMES note, to assist the front-end in
        // determining when to display each returned frame.
        // If the decoder sets this to T upon return, the front-end should
        // attempt to display the returned image T milliseconds relative to
        // the front-end's idea of the presentation time corresponding to
        // the input image.
        // Be aware that this is a signed value, and will typically be
        // negative.

	ULONG32 width;
	ULONG32 height;
		// Width and height of the returned frame.
		// This is the width and the height as signalled in the bitstream.

} H263DecoderOutParams;


typedef struct tagHXV10_INIT
{
	UINT16 outtype;
	UINT16 pels;
	UINT16 lines;
	UINT16 nPadWidth;	/* number of columns of padding on right to get 16 x 16 block*/
	UINT16 nPadHeight;	/* number of rows of padding on bottom to get 16 x 16 block*/

	UINT16 pad_to_32;   // to keep struct member alignment independent of
	                    // compiler options
	ULONG32 ulInvariants;
	    // ulInvariants specifies the invariant picture header bits
	LONG32 packetization;
	ULONG32 ulStreamVersion;
	IUnknown* pContext;
} HXV10_INIT;

typedef HX_RESULT (HXEXPORT_PTR FPTRANSFORMINIT)(void * pH263Init,void **global);
typedef HX_RESULT (HXEXPORT_PTR FPTRANSFORMFREE)(void *global);
typedef HX_RESULT (HXEXPORT_PTR FPTRANSFORMHXV10TOYUV)(UCHAR *pH263Packets, 
	UCHAR *pDecodedFrameBuffer, void *pInputParams, 
	void *pOutputParams,void *global);


HX_RESULT HXEXPORT ENTRYPOINT(HXVtoYUV420Transform) (UCHAR *pRV10Packets, 
	UCHAR *pDecodedFrameBuffer, void *pInputParams, 
	void *pOutputParams,void *global);
HX_RESULT HXEXPORT ENTRYPOINT(HXVtoYUV420Init) (HXV10_INIT * pHxv10Init,void **global);
HX_RESULT HXEXPORT ENTRYPOINT(HXVtoYUV420Free) (void *global);


#ifdef __cplusplus
}		/* Assume C declarations for C++ */
#endif	/* __cplusplus */


#if defined(_MSC_VER)
#pragma pack(pop)
///////////////////////////////////
//
// End 1-byte structure alignment
//
///////////////////////////////////
#endif

// Byte alignment settings for Mac
#if defined( _MACINTOSH )
#pragma options align=reset
#endif

#endif // __H263VDEC_H__

