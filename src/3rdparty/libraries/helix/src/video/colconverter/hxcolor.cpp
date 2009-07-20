/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcolor.cpp,v 1.8 2005/03/11 19:58:04 bobclark Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcolor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* component GUID support */
HX_RESULT HXEXPORT ENTRYPOINT(GetHXColorGUID) (UCHAR* guid)
{
    if (!guid)
    {
	    return HXR_FAIL;
    }

    return HXR_OK;
}

/* low-level color converters: */
#include "colorlib.h"

void HXEXPORT ENTRYPOINT(InitColorConverter) ()
{
    /* generate default (CCIR 601-2) conversion tables: */
    SetSrcI420Colors (0, 0, 0, 0);
    SetDestI420Colors (0, 0, 0, 0);
    SetI420ChromaResamplingMode (0);
}

void HXEXPORT ENTRYPOINT(SetColorAdjustments) (float Brightness, float Contrast, float Saturation, float Hue)
{
    /* this will only affect "from I420" converters: */
    SetSrcI420Colors (Brightness, Contrast, Saturation, Hue);
}

void HXEXPORT ENTRYPOINT(GetColorAdjustments) (float* Brightness, float* Contrast, float* Saturation, float* Hue)
{
    /* this will only affect "from I420" converters: */
    GetSrcI420Colors (Brightness, Contrast, Saturation, Hue);
}

#ifdef _FAT_HXCOLOR
int HXEXPORT ENTRYPOINT(SetChromaResamplingMode) (int nNewMode)
{
    return SetI420ChromaResamplingMode (nNewMode);
}

int HXEXPORT ENTRYPOINT(SuggestRGB8Palette) (int nColors, UINT32 *lpRGBVals)
{
    return SuggestDestRGB8Palette (nColors, (unsigned int*) lpRGBVals);
}

int HXEXPORT ENTRYPOINT(SetRGB8Palette) (int nColors, UINT32 *lpRGBVals, int *lpIndices)
{
    SetSrcRGB8Palette (nColors, (unsigned int*) lpRGBVals, lpIndices);
    return SetDestRGB8Palette (nColors, (unsigned int*) lpRGBVals, lpIndices);
}
#endif

/*
 * Currently implemented color Converters (groupped by common input format):
 */
typedef struct {
    int cidOut;                 /* output color format  */
    LPHXCOLORCONVERTER pfnCC;   /* color converter to use */
} CCLINK, *PCCLINK;

typedef struct {
    int cidOut;                 /* output color format  */
    LPHXCOLORCONVERTER2 pfnCC;  /* color converter to use */
} CCLINK2, *PCCLINK2;

/* "I420 to *" converters: */
static CCLINK pcclI420   [] = { {CID_I420,   I420toI420},   
                                {CID_YV12,   I420toYV12},
                                {CID_YUY2,   I420toYUY2},   
                                {CID_UYVY,   I420toUYVY},
                                {CID_RGB32,  I420toRGB32},  
                                {CID_RGB24,  I420toRGB24},
                                {CID_RGB565, I420toRGB565}, 
                                {CID_RGB555, I420toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   I420toRGB8},   
#endif
#ifdef _FAT_HXCOLOR
                                {CID_BGR32,  I420toBGR32},
#endif
                                {CID_ARGB32, I420toRGB32},
                                {CID_UNKNOWN,0} };

/* "I420 to *x" converters: */
static CCLINK2 pcclI420x   [] = {
                {CID_I420,   I420toI420x},
                {CID_YV12,   I420toYV12x},
                {CID_YUY2,   I420toYUY2x},
                {CID_UYVY,   I420toUYVYx},
                {CID_RGB32,  I420toRGB32x},
                {CID_RGB24,  I420toRGB24x},
                {CID_RGB565, I420toRGB565x},
                {CID_RGB555, I420toRGB555x},
#ifdef _8_BIT_SUPPORT
                {CID_RGB8,   I420toRGB8x},
#endif
#ifdef _FAT_HXCOLOR                
                {CID_BGR32,  I420toBGR32x},
#endif
                {CID_ARGB32, I420toRGB32x},
                {CID_UNKNOWN,0} /* end of list */
                };

/* "YUV* to *" converters: */
static CCLINK pcclYV12   [] = { {CID_I420,   YV12toI420},   
                                {CID_YV12,   YV12toYV12},
                                {CID_YUY2,   YV12toYUY2},   
                                {CID_UYVY,   YV12toUYVY},
                                {CID_RGB32,  YV12toRGB32},
                                {CID_RGB24,  YV12toRGB24},
                                {CID_RGB565, YV12toRGB565},
                                {CID_RGB555, YV12toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   YV12toRGB8},
#endif
#ifdef _FAT_HXCOLOR                
                                {CID_BGR32,  YV12toBGR32},
#endif
                                {CID_ARGB32, YV12toRGB32},
                                {CID_UNKNOWN,0} /* end of list */
                                };

static CCLINK pcclYVU9   [] = { {CID_I420,   YVU9toI420},
                                {CID_UNKNOWN, 0}};

static CCLINK pcclYUY2 [] = { {CID_YUY2,    YUY2toYUY2},
                              {CID_I420,    YUY2toI420},
                              {CID_UYVY,    YUY2toUYVY},
                              {CID_YV12,    YUY2toYV12},
                              {CID_UNKNOWN, 0}
                            };

static CCLINK pcclYUVU [] = { {CID_I420,    YUVUtoI420},
                              {CID_UNKNOWN, 0}
                            };

static CCLINK pcclUYVY   [] = { {CID_I420,   UYVYtoI420},
                                {CID_YV12,   UYVYtoYV12},
                                {CID_YUY2,   UYVYtoYUY2},
                                {CID_UYVY,   UYVYtoUYVY},
                                {CID_UNKNOWN, 0}};

/* "YUV* to *x" converters: */
static CCLINK2 pcclYV12x [] = { {CID_I420,   YV12toI420x},   
                                {CID_YV12,   YV12toYV12x},
                                {CID_YUY2,   YV12toYUY2x},   
                                {CID_UYVY,   YV12toUYVYx},
                                {CID_RGB32,  YV12toRGB32x},
                                {CID_RGB24,  YV12toRGB24x},
                                {CID_RGB565, YV12toRGB565x},
                                {CID_RGB555, YV12toRGB555x},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   YV12toRGB8x},
#endif
#ifdef _FAT_HXCOLOR                
                                {CID_BGR32,  YV12toBGR32x},
#endif
                                {CID_ARGB32, YV12toRGB32x},
                                {CID_UNKNOWN,0} /* end of list */
                                };


static CCLINK2 pcclYUY2x [] = { {CID_I420,   YUY2toI420x},
                                {CID_YV12,   YUY2toYV12x},
                                {CID_UNKNOWN, 0}};

static CCLINK2 pcclUYVYx [] = { {CID_I420,   UYVYtoI420x},
                                {CID_YV12,   UYVYtoYV12x},
                                {CID_UNKNOWN, 0}};

/* "RGB* to *" converters: */
static CCLINK pcclRGB32  [] = { {CID_I420,    RGB32toI420},
                                {CID_RGB32,   RGB32toRGB32}, 
                                {CID_RGB24,   RGB32toRGB24},
                                {CID_RGB565,  RGB32toRGB565}, 
                                {CID_RGB555,  RGB32toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,    RGB32toRGB8},  
#endif
#ifdef _FAT_HXCOLOR
                                {CID_BGR32,   RGB32toBGR32},
#endif
                                {CID_ARGB32,  RGB32toRGB32},
                                {CID_YUVA,    ARGBtoYUVA},
                                {CID_UNKNOWN, 0} };

static CCLINK pcclARGB32  [] = { {CID_I420,    RGB32toI420},
                                 {CID_RGB32,   RGB32toRGB32},
                                 {CID_ARGB32,  RGB32toRGB32},
                                 {CID_RGB24,   RGB32toRGB24},
                                 {CID_RGB565,  RGB32toRGB565},
                                 {CID_RGB555,  RGB32toRGB555},
#ifdef _8_BIT_SUPPORT
                                 {CID_RGB8,    RGB32toRGB8},
#endif
                                 {CID_BGR32,   RGB32toBGR32},
                                 {CID_RGB32,   RGB32toRGB32},
                                 {CID_YUVA,    ARGBtoYUVA},
                                 {CID_UNKNOWN, 0} };

static CCLINK pcclYUVA  [] = { {CID_UNKNOWN, 0} };


#ifdef _FAT_HXCOLOR
static CCLINK pcclRGB24  [] = { {CID_I420,   RGB24toI420},
                                {CID_RGB32,  RGB24toRGB32}, 
                                {CID_RGB24,  RGB24toRGB24},
                                {CID_RGB565, RGB24toRGB565},
                                {CID_RGB555, RGB24toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   RGB24toRGB8},  
#endif
                                {CID_BGR32,  RGB24toBGR32},
                                {CID_ARGB32,  RGB24toRGB32}, 
                                {CID_UNKNOWN,0}};
#else
static CCLINK pcclRGB24  [] = { {CID_UNKNOWN,0} };
#endif


#ifdef _FAT_HXCOLOR
static CCLINK pcclRGB565 [] = { {CID_I420,   RGB565toI420},
                                {CID_RGB32,  RGB565toRGB32},
                                {CID_RGB24,  RGB565toRGB24},
                                {CID_RGB565, RGB565toRGB565},
                                {CID_RGB555,RGB565toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   RGB565toRGB8}, 
#endif
                                {CID_BGR32,  RGB565toBGR32},
                                {CID_ARGB32,  RGB565toRGB32},
                                {CID_UNKNOWN,0}};
#else
static CCLINK pcclRGB565 [] = { {CID_UNKNOWN,0} };
#endif

#ifdef _FAT_HXCOLOR
static CCLINK pcclRGB555 [] = { {CID_I420,   RGB555toI420},
                                {CID_RGB32,  RGB555toRGB32},
                                {CID_RGB24,  RGB555toRGB24},
                                {CID_RGB565, RGB555toRGB565},
                                {CID_RGB555,RGB555toRGB555},
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8,   RGB555toRGB8}, 
#endif
                                {CID_BGR32,  RGB555toBGR32},
                                {CID_ARGB32,  RGB555toRGB32},
                                {CID_UNKNOWN,0}};
#else
static CCLINK pcclRGB555 [] = { {CID_UNKNOWN,0} };
#endif

#ifdef _8_BIT_SUPPORT

static CCLINK pcclRGB8   [] = { {CID_I420,   RGB8toI420},
                                {CID_RGB32,  RGB8toRGB32},  
                                {CID_RGB24,  RGB8toRGB24},
                                {CID_RGB565, RGB8toRGB565}, 
                                {CID_RGB555, RGB8toRGB555},
                                {CID_RGB8,   RGB8toRGB8},   
                                {CID_BGR32,  RGB8toBGR32},
                                {CID_ARGB32,  RGB8toRGB32},
                                {CID_UNKNOWN,0}};
#else

static CCLINK pcclRGB8   [] = { {CID_UNKNOWN, 0} };

#endif

/* "XING to *" converters: */
static CCLINK pcclXing   [] = { {CID_YV12,   XINGtoYV12}, 
                                {CID_YUY2,   XINGtoYUY2},
                                {CID_UYVY,   XINGtoUYVY},
                                {CID_RGB32, XINGtoRGB32}, 
                                {CID_RGB24, XINGtoRGB24},
                                {CID_RGB565, XINGtoRGB565}, 
#ifdef _8_BIT_SUPPORT
                                {CID_RGB8, XINGtoRGB8},
#endif
                                {CID_ARGB32, XINGtoRGB32}, 
                                {CID_UNKNOWN,0} 

};

/* "BGR* to *" converters: */
static CCLINK pcclBGR32  [] = { {CID_I420,    BGR_32toI420},
                                {CID_UNKNOWN,0} 
								};

static CCLINK pcclBGR24  [] = { {CID_I420,    BGR24toI420},
                                {CID_UNKNOWN,0} 
								};




/* unsupported input formats: */
static CCLINK pcclUNKNOWN [] = {{CID_UNKNOWN, 0}};

/*
 * Color formats/Converters map:
 */
static PCCLINK ppcclColorMap[] =
{
    pcclI420, 
    pcclYV12, 
    pcclYVU9, 
    pcclYUY2, 
    pcclUYVY,
    pcclRGB32, 
    pcclRGB24, 
    pcclRGB565, 
    pcclRGB555, 
    pcclRGB8, 
    pcclXing,
    pcclARGB32, 
    pcclYUVA,
	pcclYUVU,
    pcclUNKNOWN,
    pcclBGR32,
    pcclBGR24
};

/*
 * Retrieve the list of color formats that can be converted to, given an input
 * format
 * Use:
 *  HX_RESULT GetCompatibleColorFormats (INT32 cidIn, INT32* pcidOut, UINT32* pnSize);
 * Input:
 *  cidIn - input color format ID (!CID_UNKNOWN)
 *  pcidOut - in - empty array, out - filled array of supported output formats
 *  pnSize - in - size of array, out - number of elements in array
 * Returns:
 *  result
 */HX_RESULT HXEXPORT ENTRYPOINT(GetCompatibleColorFormats)(
    INT32 cidIn /* in */, 
    INT32* pcidOut /* in/out */, 
    UINT32* pnSize /* in/out */)
{
    HX_RESULT res = HXR_FAIL;

        /* check parameters: */
    int nConversions = sizeof(ppcclColorMap)/sizeof(PCCLINK);
    if(cidIn >= 0 && cidIn < nConversions && pcidOut && pnSize) 
    {
	UINT32 nFormats = 0;
        /* scan color map: */
        PCCLINK pccl = ppcclColorMap [cidIn];
        while(pccl && pccl->cidOut != CID_UNKNOWN && nFormats < *pnSize) 
	{
	    pcidOut[nFormats] = pccl->cidOut;

            /* try next element in the list: */
            pccl++;
	    nFormats++;
        }

	res = HXR_OK;
	*pnSize = nFormats;
    }

    return res;
}

/*
 * Find Converter to trasform data in format X to format Y.
 * Use:
 *  LPHXCOLORCONVERTER GetColorConverter (INT32 cidIn, INT32 cidOut);
 * Input:
 *  cidIn - input color format ID (!CID_UNKNOWN)
 *  cidOut - desirable output color format ID (!CID_UNKNOWN)
 * Returns:
 *  pointer to an appropriate color conversion routine, if success;
 *  NULL - conversion is not supported.
 */
LPHXCOLORCONVERTER HXEXPORT ENTRYPOINT(GetColorConverter) (INT32 cidIn, INT32 cidOut)
{
    /* check parameters: */
    int nConversions = sizeof(ppcclColorMap)/sizeof(PCCLINK);
    if (cidIn >= 0 && cidIn < nConversions &&
        cidOut >= 0 && cidOut <= NFORMATS) {

        /* scan color map: */
        PCCLINK pccl = ppcclColorMap [cidIn];
        while (pccl &&
               pccl->cidOut != CID_UNKNOWN) {

            /* check output format:  */
            if (pccl->cidOut == cidOut)
                return pccl->pfnCC;

            /* try next element in the list: */
            pccl ++;
        }
    }
    return NULL;
}

LPHXCOLORCONVERTER2 HXEXPORT ENTRYPOINT(GetColorConverter2) (INT32 cidIn, INT32 cidOut)
{
    CCLINK2 *pTemp = NULL;

    /* check parameters: */
    if (cidIn == CID_I420)
    {
        pTemp = pcclI420x;
    }
    else if (cidIn == CID_YV12)
    {
        pTemp = pcclYV12x;
    }
    else if (cidIn == CID_YUY2)
    {
        pTemp = pcclYUY2x;
    }
    else if (cidIn == CID_UYVY)
    {
        pTemp = pcclUYVYx;
    }

    if (pTemp)
    {
        /* scan color map: */
        for (int i=0; pTemp[i].cidOut != CID_UNKNOWN; i++)
        {
            /* check output format:  */
            if (pTemp[i].cidOut == cidOut)
                return pTemp[i].pfnCC;
        }
    }

    return NULL;
}


/*
 * Try selected compatible color formats.
 * Use:
 *  HXBOOL ScanCompatibleColorFormats (INT32 cidIn, INT32 cidOutMask, void *pParam,
 *      HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC));
 * Input:
 *  cidIn - input color format ID (!CID_UNKNOWN)
 *  cidOutMask - masks output formats to try (use ~0 to scan all formats)
 *  pParam - pointer to a parameter block to pass to fpTryIt ()
 *  pfnTryIt - pointer to a function, which will be called for each
 *          compatible output format;
 * Returns:
 *  TRUE, if fpTryIt() has exited with TRUE status;
 *  FALSE, if non of the compatible formats has been accepted by fpTryIt().
 */
HXBOOL HXEXPORT ENTRYPOINT(ScanCompatibleColorFormats) (INT32 cidIn, INT32 cidOutMask, void *pParam,
    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC), INT32)
{
    /* check parameters: */
    int nConversions = sizeof(ppcclColorMap)/sizeof(PCCLINK);
    if (cidIn >= 0 && cidIn < nConversions && pfnTryIt != NULL) {

        /* scan color map: */
        PCCLINK pccl = ppcclColorMap [cidIn];
        while (pccl->cidOut != CID_UNKNOWN) {

            /* try this format: */
            if ((cidOutMask & (1U << pccl->cidOut)) &&
                (* pfnTryIt) (pParam, pccl->cidOut, pccl->pfnCC))
                return TRUE;

            /* try next element in the list: */
            pccl ++;
        }
    }
    return FALSE;
}

/*
 * Try all compatible color formats.
 */
HXBOOL HXEXPORT ENTRYPOINT(ScanAllCompatibleColorFormats) (INT32 cidIn,void *pParam,
    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC), INT32)
{
    return ENTRYPOINT(ScanCompatibleColorFormats) (cidIn, (INT32)~0, pParam, pfnTryIt);
}


/***********************
 * Old HXCOLOR.DLL interface:
 ****************************************************/

#ifdef _FAT_HXCOLOR
/*
 * Converts a YUV buffer into an RGB buffer in the specified format.
 */
void HXEXPORT ENTRYPOINT(ConvertYUVtoRGB) (UCHAR* ySrc, UCHAR* uSrc, UCHAR* vSrc,
                                           INT32  nPitchSrc,
                                           UCHAR* Dst, INT32  nWidth, INT32 nHeight,
                                           INT32  nPitchDst, INT16 nFormat, INT16 nExpand)
{
#if defined(_WINDOWS) || defined(_UNIX)

    static void (*cc []) (unsigned char *, unsigned char *, unsigned char *, int, unsigned char *, int, int, int) =
    {
        oldI420toRGB24,   oldI420toRGB555,   oldI420toRGB565,
        oldI420toRGB24x2, oldI420toRGB555x2, oldI420toRGB565x2
    };

    /* get function index: */
    register int idx = (nFormat - T_RGB888);
    if (idx < 0 || idx > 2) return;         /* bad format */
    if (nExpand) idx += 3;

    /* call color converter/interpolator: */
    (* cc [idx]) (ySrc, uSrc, vSrc, nPitchSrc, Dst, nWidth, nHeight, nPitchDst);

#elif defined (_MACINTOSH)

    /* Mac version uses big endian RGB32 format only.
     * Note, that our *->RGB32 converters will generate correct output
     * on both LE & BE machines: */
    (nExpand? oldI420toRGB32x2: oldI420toRGB32)
        (ySrc, uSrc, vSrc, nPitchSrc, Dst, nWidth, nHeight, nPitchDst);
#endif
}

void HXEXPORT ENTRYPOINT(ConvertYUVtoMacRGB32) (UCHAR* ySrc, UCHAR* uSrc, UCHAR* vSrc, INT32  nPitchSrc,
        UCHAR* Dst, INT32  nWidth, INT32  nHeight, INT32  nPitchDst, INT16  nFormat, INT16  nExpand)
{
    /* Mac version uses big endian RGB32 format only.
     * Note, that our *->RGB32 converters will generate correct output
     * on both LE & BE machines: */
    (nExpand? oldI420toRGB32x2: oldI420toRGB32)
        (ySrc, uSrc, vSrc, nPitchSrc, Dst, nWidth, nHeight, nPitchDst);
}

/*
 * Converts a 24bit RGB Buffer into a 32bit RGB buffer, used mainly on the Macintosh.
 */
void HXEXPORT ENTRYPOINT(ConvertRGB24toXRGB) (UCHAR* pSrc, UCHAR* pDest,
    ULONG32 srcSize, ULONG32 destSize, INT32 nWidth, INT32 nHeight)
{
    /* this shall generate a correct padded RGB32 on both LE & BE machines: */
    RGB24toRGB32 (pDest, nWidth, nHeight, (nWidth*3+3)&~3, 0, 0, nWidth, nHeight,
                 pSrc, nWidth, nHeight, nWidth * 4, 0, 0, nWidth, nHeight);
}

/*
 * Converts a RGB3 buffer into a YUV buffer in Y'CbCr 4:2:0 format.
 */
void HXEXPORT ENTRYPOINT(ConvertRGBtoYUV) (UCHAR* pInput, UCHAR* pOutput,
    INT32 nWidth, INT32 nHeight, HXBOOL bBGR)
{
    /* ignore bBRG for now: */
    RGB24toI420 (pOutput, nWidth, nHeight, (nWidth*3+3)&~3, 0, 0, nWidth, nHeight,
                pInput, nWidth, nHeight, nWidth, 0, 0, nWidth, nHeight);
}

#endif


#ifdef __cplusplus
}
#endif
