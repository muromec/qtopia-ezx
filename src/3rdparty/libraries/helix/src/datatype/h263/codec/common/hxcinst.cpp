/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcinst.cpp,v 1.6 2005/03/14 19:24:46 bobclark Exp $
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


#include "hxvamain.h"
#include "hvenc.h"
#include "h261defs.h"
#include "hvscodes.h"
#include "hxcinst.h"
#include "h263codecf.h"
#include "memory.h"

#ifdef _MACINTOSH
#include <string.h> // for memcpy
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Function Protos      */
U32 VvOpenDecoder ( S16 format );
U32 VvGetDecoderPtr ( S16 index );

U32 VvDecode ( S16 index, U32 PBframeCap, PICTURE_DESCR* picdesc,
                          U32 bsStart, U32 bsEnd, S16* nextGOB, PICTURE* newPic, 
                          VvDecoderStats* status, U32 newBs );
              
U32 VvCloseDecoder (S16 index );
U32 VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline DWORD WIDTHBYTES(DWORD bits) 
{
    return (((bits) + 31) / 32 * 4);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline DWORD ROUNDTO16(DWORD a) 
{
    return ((a+15)>>4)<<4;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HXBOOL IsOpenDecoder(S16 hDecoder)
{
    return 1<=hDecoder && hDecoder<=MAX_NUM_DECODERS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void MarkDecoderClosed(S16& hDecoder)
{
    hDecoder = MAX_NUM_DECODERS+1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline U32 BITNUM(VvBitIndex q) 
{
    return ((((q).l >> 13) &0x7FFF8) + ((q).l&7));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline U32 BITSDIFF(VvBitIndex start,VvBitIndex finish) 
{
    return (BITNUM(finish)-BITNUM(start));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline U32 BYTESDIFF(VvBitIndex start,VvBitIndex finish)
{
    return (BITSDIFF(start,finish) + 7) >> 3;
}




const S16 HXH263CodecInstance::capsCIF(CIF);
const U32 HXH263CodecInstance::effFactor(56);
const U32 HXH263CodecInstance::h263MaxInterFrames(31);
const U32 HXH263CodecInstance::bogusId(1);
const U32 HXH263CodecInstance::cifHeight(288);
const U32 HXH263CodecInstance::cifWidth(352);
const U32 HXH263CodecInstance::qcifHeight(144);
const U32 HXH263CodecInstance::qcifWidth(176);
const U32 HXH263CodecInstance::sqcifHeight(96);
const U32 HXH263CodecInstance::sqcifWidth(128);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HXH263CodecInstance::HXH263CodecInstance() :
m_fccType(0),
m_openFlags(0),
m_vcmVersion(0),
m_lpDecoderPtr(NULL)
{
    MarkDecoderClosed(m_hDecoder);
}

HXBOOL HXH263CodecInstance::Initialize(void)
{
    // make sure we're being opened as a video compressor
    m_fccType = 1;
    m_openFlags = 0;
    m_vcmVersion = 1;

    return TRUE;
}

    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX_RESULT HXH263CodecInstance::DecompressStart( 
	const HXVA_Image_Format &srcFormat,
	HXVA_Image_Format &dstFormat
)
{
    // Somebody called start without end
    //if(IsOpenDecoder(m_hDecoder)) 
	//	DecompressEnd();
    
    // open decoder with CIF capabilities
    // FIX: must check for success here
    m_hDecoder = (S16) VvOpenDecoder(capsCIF); 
    
    if(m_hDecoder > MAX_NUM_DECODERS)     {
        MarkDecoderClosed(m_hDecoder);
        return HXR_OUTOFMEMORY;
    }
    
    // Initialize picture descriptor
    m_PicDesc.rows = ROUNDTO16(srcFormat.dimensions.height);
    m_PicDesc.cols = ROUNDTO16(srcFormat.dimensions.width);
    // Initialize the output picture
    m_PicOut.picLayout = VIVO_YUV12;
    m_PicOut.color = 1;


    // initialize remaining decoder state
    m_nextGob = 0;

    // Grab the decoder pointer
    m_lpDecoderPtr = (LPBYTE) VvGetDecoderPtr(m_hDecoder);

    if(m_lpDecoderPtr==NULL)    {
        DecompressEnd();
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX_RESULT HXH263CodecInstance::DecompressEnd()
{
    if(IsOpenDecoder(m_hDecoder)) 
		VvCloseDecoder(m_hDecoder);
    MarkDecoderClosed(m_hDecoder);
    m_lpDecoderPtr = NULL;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX_RESULT HXH263CodecInstance::DecompressConvert(	
	const HXVA_Image &src,
	HXVA_Image &dst
)
{
    static U32 EOS(0x00FC0000);
    // Cool, the NetShow player seems to send ICM_DECOMPRESS_CONVERT before
    // sending ICM_DECOMPRESS_BEGIN (at least sometimes!)
    if(!IsOpenDecoder(m_hDecoder)) 
	 DecompressStart(src.format, dst.format);

    VvBitIndex begin,end,ret;
    // if biSizeImage == 1 it means we have the second half of a PB frame; set it to 0 and decode
    //if(lpDEX->lpbiSrc->biSizeImage == 1) 
	//	lpDEX->lpbiSrc->biSizeImage = 0;
    begin.l = 0;
    end.ww.byte = (U16)(src.size) + 4; // make room for EOS
    end.l = end.ww.byte * 8 + 1;
    U32 result = 1;
    
    // copy the incoming data to the decoder buffer
    LPBYTE lpData = GetDecoderPtr(); 
    U32 *lpEOS = &EOS; // WIN16 days r over

	if(!lpData) {//decoder was obviously closed already
		return HXR_FAIL;		
	}
    memcpy(lpData, src.GetBasePointer(), src.size); /* Flawfinder: ignore */
    memcpy(lpData+(int)src.size, lpEOS, 4); /* Flawfinder: ignore */
    static const U32 PBFrameCap(1);
    
    m_PicDesc.decodingInProgress = FALSE;

    PICTURE *lpPicOut = &m_PicOut;
    PICTURE_DESCR *lpPicDesc = &m_PicDesc;
    S16 *lpNextGob = &m_nextGob;
    VvDecoderStats *lpDecoderStats = &m_DecoderStats;
    VvBitIndex *lpRet = &ret;
    do  {
        result = 
        VvDecode ( m_hDecoder,          // decoder index
            PBFrameCap,                 // reconstruct B frames
            lpPicDesc,            // frame data
            (U32) begin.l,          // first bit to decode
            (U32) end.l,                // last bit (+1) to decode
            lpNextGob,            // next gob to process
            lpPicOut,         // pointer to output pix
            lpDecoderStats,   // pointer to stats block
            (U32) lpRet );              // pos of first bit
    }
    while(m_PicDesc.decodingInProgress);

    // copy decoded frame to the destination buffer
	// YUV Do something..
	unsigned char* pDst = (unsigned char*)dst.GetBasePointer();
	S32 lumaHeight = dst.format.dimensions.height;
    S32 chromaHeight = (lumaHeight+1)>1;
    // Now turn YUV pointers upside down before passing to DoRGB16
    unsigned char* pY = m_PicOut.y.ptr;
    unsigned char* pCr = m_PicOut.cr.ptr;
    unsigned char* pCb = m_PicOut.cb.ptr;
    int yOffset = m_PicOut.y.hoffset;
    int crOffset = m_PicOut.cr.hoffset;
    int cbOffset = m_PicOut.cb.hoffset;
	int chromaLines = m_PicOut.cb.nvert;
	int chromaBytes = m_PicOut.cb.nhor;
	int memsz = lumaHeight*yOffset;
	if(m_PicOut.picLayout == VVS_LAYOUT_YUV12) memsz += (memsz>>1);
	memcpy(pDst, pY, memsz); /* Flawfinder: ignore */
	if(m_PicOut.picLayout == VVS_LAYOUT_TEE) 
	{
		// optimised layout for color conversion.
		pDst += lumaHeight*yOffset;
		unsigned char * pDst2 = pDst+(chromaLines*chromaBytes);
		for(int i=0;i<chromaLines;i++)
		{
			memcpy(pDst2, pCr, chromaBytes); /* Flawfinder: ignore */
			memcpy(pDst, pCb, chromaBytes); /* Flawfinder: ignore */
			pCr+=crOffset;
			pCb+=cbOffset;
			pDst+=chromaBytes;
			pDst2+=chromaBytes;
		}
	}
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX_RESULT HXH263CodecInstance::DecompressVerifyParams(void)
{
    return HXR_OK;
}
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX_RESULT HXH263CodecInstance::DecompressGetOutputFormat( void)
{
    return HXR_OK;
}



