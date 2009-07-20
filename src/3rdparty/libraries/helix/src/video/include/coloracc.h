/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: coloracc.h,v 1.6 2005/03/11 19:58:05 bobclark Exp $
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

#ifndef _COLORACC_H_
#define _COLORACC_H_

#include "hxcolor.h"

#ifdef _WINDOWS
#include "hxcore.h"
#endif

// Forward declarations
class DLLAccess;

const UINT32 MAX_DLL_NAME_LEN = 256;

typedef HX_RESULT (HXEXPORT_PTR FPGETHXCOLORGUID)   (UCHAR* pGUID);
typedef void (HXEXPORT_PTR FPINITCOLORCONVERTER)    (void);
typedef void (HXEXPORT_PTR FPSETCOLORADJUSTMENTS)   (float Brightness,
						    float Contrast,
						    float Saturation,
						    float Hue);

#ifdef _WINDOWS
typedef void (HXEXPORT_PTR FPINITCOLORCONVERTER2)   (IUnknown* pContext);
typedef void (HXEXPORT_PTR FPSETCOLORADJUSTMENTS2)  (IUnknown* pContext,
                                                    float Brightness,
						    float Contrast,
						    float Saturation,
						    float Hue);
typedef void (HXEXPORT_PTR FPUPGRADECOLORCONVERTER) (IUnknown* pContext, IUnknown* pPlayer, INT32 cidIn, INT32 cidOut);
#endif

typedef void (HXEXPORT_PTR FPGETCOLORADJUSTMENTS)   (float *Brightness,
						    float *Contrast,
						    float *Saturation,
						    float *Hue);

typedef int (HXEXPORT_PTR FPSUGGESTRGB8PALETTE) (int nColors, UINT32 *lpRGBVals);
typedef int (HXEXPORT_PTR FPSETRGB8PALETTE) (int nColors, UINT32 *lpRGBVals, int *lpIndices);

typedef void (HXEXPORT_PTR FPSETSHARPNESSADJUSTMENTS)   (float Sharpness, INT16 nExpand);
typedef void (HXEXPORT_PTR FPGETSHARPNESSADJUSTMENTS)   (float* Sharpness, INT16* nExpand);
typedef void (HXEXPORT_PTR FPCONVERTRGBTOYUV)	    (UCHAR* pInput,
						    UCHAR* pOutput,
						    INT32  nWidth,
						    INT32  nHeight,
						    HXBOOL   bBGR);
typedef void (HXEXPORT_PTR FPCONVERTYUVTORGB)	    (UCHAR* ySrc,
						    UCHAR* uSrc,
						    UCHAR* vSrc,
						    INT32  nPitchSrc,
						    UCHAR* Dst,
						    INT32  nWidth,
						    INT32  nHeight,
						    INT32  nPitchDst,
						    INT16  nFormat,
						    INT16  nExpand);
typedef int (HXEXPORT_PTR FPI420ANDYUVA)
      (unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
       int src1_startx, int src1_starty,
       unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
       int src2_startx, int src2_starty,
       unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
       int dest_startx, int dest_starty,
       int width,  int height,  int color_format);

typedef int (HXEXPORT_PTR FPI420ANDI420TOI420)
      (unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
       int src1_startx, int src1_starty,
       unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
       int src2_startx, int src2_starty,
       unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
       int dest_startx, int dest_starty,
       int width,  int height,  int output_format);


typedef void (HXEXPORT_PTR FPENHANCE) (UCHAR* ysrc, INT32 nHeight, INT32 nWidth, INT32 nPitchSrc, float Sharpness);
typedef void (HXEXPORT_PTR FPENHANCEUNIFORM) (UCHAR* ysrc, INT32 nHeight, INT32 nWidth, INT32 nPitchSrc, float Sharpness);


typedef void (HXEXPORT_PTR FPCONVERTRGB24TOXRGB)    (UCHAR* pSrc,
						    UCHAR* pDest,
						    ULONG32 srcSize,
						    ULONG32 destSize,
						    INT32	nWidth,
						    INT32	nHeight);

typedef LPHXCOLORCONVERTER (HXEXPORT_PTR FPGETCOLORCONVERTER)
					    (INT32 cidIn, INT32 cidOut);
typedef LPHXCOLORCONVERTER2 (HXEXPORT_PTR FPGETCOLORCONVERTERx)
					    (INT32 cidIn, INT32 cidOut);

#ifdef _WINDOWS
typedef LPHXCOLORCONVERTER (HXEXPORT_PTR FPGETCOLORCONVERTER2)
					    (IUnknown* pContext, INT32 cidIn, INT32 cidOut);
typedef LPHXCOLORCONVERTER2 (HXEXPORT_PTR FPGETCOLORCONVERTER2x)
					    (IUnknown* pContext, INT32 cidIn, INT32 cidOut);

#endif

typedef HXBOOL (HXEXPORT_PTR FPSCANCOMPATIBLECOLORFORMATS)
		(INT32 cidIn, INT32 cidOutMask, void *pParam,
		    HXBOOL (*pfnTryIt) (void *pParam,
		    INT32 cidOut, LPHXCOLORCONVERTER pfnCC));

typedef HXBOOL (HXEXPORT_PTR FPSCANALLCOMPATIBLECOLORFORMATS)
		(INT32 cidIn, void *pParam,
		    HXBOOL (*pfnTryIt) (void *pParam,
		    INT32 cidOut, LPHXCOLORCONVERTER pfnCC));


class ColorFuncAccess
{
public:
    ColorFuncAccess(IUnknown* pContext);
    ~ColorFuncAccess();

    HX_RESULT hStatus;
    HX_RESULT GetHXColorGUID	(UCHAR* pGUID);
    void InitColorConverter	();
    void SetColorAdjustments    (float Brightness, float Contrast,
				float Saturation, float Hue);
    void GetColorAdjustments    (float *Brightness, float *Contrast,
				float *Saturation, float *Hue);
    int SuggestRGB8Palette (int nColors, UINT32 *lpRGBVals);
    int SetRGB8Palette (int nColors, UINT32 *lpRGBVals, int *lpIndices);

    void SetSharpnessAdjustments    (float Sharpness, INT16 nExpand);
    void GetSharpnessAdjustments    (float* Sharpness, INT16* nExpand);
    void ConvertRGBtoYUV	(UCHAR* pInput, UCHAR* pOutput,
				INT32 nWidth, INT32 nHeight, HXBOOL bBGR);
    void ConvertYUVtoRGB	(UCHAR* ySrc, UCHAR* uSrc, UCHAR* vSrc,
				INT32 nPitchSrc, UCHAR* Dst, INT32 nWidth,
				INT32 nHeight, INT32 nPitchDst, INT16 nFormat,
				INT16 nExpand);
    void Enhance(UCHAR* ysrc, INT32 nHeight, INT32 nWidth, INT32 nPitchSrc, float Sharpness);
    void EnhanceUniform(UCHAR* ysrc, INT32 nHeight, INT32 nWidth, INT32 nPitchSrc, float Sharpness);
    void ConvertRGB24toXRGB( UCHAR* pSrc, UCHAR* pDest,
	            ULONG32 srcSize, ULONG32 destSize, INT32 nWidth, INT32 nHeight);

    HXBOOL CheckColorConverter (INT32 cidIn, INT32 cidOut);
    HXBOOL CheckColorConverter2 (INT32 cidIn, INT32 cidOut);

    virtual
    int  ColorConvert(INT32 cidOut,
                      unsigned char *dest_ptr, int dest_width, int dest_height,
                      int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                      INT32 cidIn,
                      unsigned char *src_ptr, int src_width, int src_height, int src_pitch,
                      int src_x, int src_y, int src_dx, int src_dy);

    virtual
    int  ColorConvert2(INT32 cidOut,
                       unsigned char *dest_ptr, int dest_width, int dest_height,
                       int dest_pitch, int dest_x, int dest_y, int dest_dx, int dest_dy,
                       INT32 cidIn,
                       unsigned char *pY, unsigned char *pU, unsigned char *pV,
                       int src_width, int src_height, int yPitch, int uPitch, int vPitch,
                       int src_x, int src_y, int src_dx, int src_dy);
    int I420andYUVA(
	unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
        int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
        int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
        int dest_startx, int dest_starty,
	int width,  int height,  int color_format);
    
    int I420andI420toI420(
        unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
        int src1_startx, int src1_starty,
	unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
        int src2_startx, int src2_starty,
	unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
        int dest_startx, int dest_starty,
	int width,  int height,  int alpha);
    
    
    HXBOOL ScanCompatibleColorFormats (INT32 cidIn, INT32 cidOutMask, void *pParam,
		    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC));
    HXBOOL ScanAllCompatibleColorFormats (INT32 cidIn, void *pParam,
		    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC));

    virtual void Terminate() {};

#ifdef _WINDOWS
    void SetPlayer(IHXPlayer* pPlayer);
#endif

protected:
    DLLAccess* m_pDllAccess;
    char       m_pDllName[MAX_DLL_NAME_LEN]; /* Flawfinder: ignore */

    IUnknown*  m_pContext;

    FPGETHXCOLORGUID	    m_fpGetHXColorGUID;
    FPINITCOLORCONVERTER    m_fpInitColorConverter;
    FPSETCOLORADJUSTMENTS   m_fpSetColorAdjustments;
    FPGETCOLORADJUSTMENTS   m_fpGetColorAdjustments;
    FPSUGGESTRGB8PALETTE    m_SuggestRGB8Palette;
    FPSETRGB8PALETTE        m_SetRGB8Palette;
    FPSETSHARPNESSADJUSTMENTS m_fpSetSharpnessAdjustments;
    FPGETSHARPNESSADJUSTMENTS m_fpGetSharpnessAdjustments;
    FPCONVERTRGBTOYUV	    m_fpConvertRGBtoYUV;
    FPI420ANDYUVA           m_fpI420andYUVA;
    FPI420ANDI420TOI420     m_fpI420andI420toI420;
    FPCONVERTYUVTORGB	    m_fpConvertYUVtoRGB;
    FPENHANCE               m_fpEnhance;
    FPENHANCEUNIFORM	    m_fpEnhanceUniform;
    FPCONVERTRGB24TOXRGB    m_fpConvertRGB24ToXRGB;
    FPSCANCOMPATIBLECOLORFORMATS m_fpScanCompatibleColorFormats;
    FPSCANALLCOMPATIBLECOLORFORMATS m_fpScanAllCompatibleColorFormats;
    FPGETCOLORCONVERTER	    m_fpGetColorConverter;
    FPGETCOLORCONVERTERx    m_fpGetColorConverterx;
    HXBOOL                    m_bLightColorConverter;

#ifdef _WINDOWS
    IHXPlayer*             m_pPlayer;
    FPINITCOLORCONVERTER2   m_fpInitColorConverter2;
    FPGETCOLORCONVERTER2    m_fpGetColorConverter2;
    FPGETCOLORCONVERTER2x   m_fpGetColorConverter2x;
    FPSETCOLORADJUSTMENTS2  m_fpSetColorAdjustments2;
    FPUPGRADECOLORCONVERTER m_fpUpgradeColorConverter;
#endif
    void LoadConversionFunctions();
    void WrongHXColorVersion();
    HX_RESULT SetCodecDir();

    LPHXCOLORCONVERTER GetColorConverter (INT32 cidIn, INT32 cidOut);
    LPHXCOLORCONVERTER2 GetColorConverter2 (INT32 cidIn, INT32 cidOut);

};

#endif // _COLORACC_H_
