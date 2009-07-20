/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: coloracc.cpp,v 1.12 2007/07/06 20:54:06 jfinnecy Exp $
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

#include "hxtypes.h"

#include "hlxclib/windows.h"

#ifdef _WINDOWS
#ifdef _WIN16
#include <stdlib.h>
#endif
#include <ctype.h>
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#endif

#include "hxresult.h"
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxbuffer.h"
#include "hxprefs.h"
#include "hxplugn.h"
#include "hxerror.h"
#include "hxengin.h"

#include "dllacces.h"
#include "dllpath.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE     
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "coloracc.h"

#include "hxdir.h"

#define WRONG_HXCOLOR_VERSION 0

ColorFuncAccess::ColorFuncAccess(IUnknown* pContext)
    : hStatus(HXR_OK)
    , m_pDllAccess(NULL)
    , m_fpGetHXColorGUID(NULL)
    , m_fpInitColorConverter(NULL)
    , m_fpSetColorAdjustments(NULL)
    , m_fpGetColorAdjustments(NULL)
    , m_SuggestRGB8Palette(NULL)
    , m_SetRGB8Palette(NULL)
    , m_fpSetSharpnessAdjustments(NULL)
    , m_fpGetSharpnessAdjustments(NULL)
    , m_fpConvertRGBtoYUV(NULL)
    , m_fpI420andYUVA(NULL)
    , m_fpI420andI420toI420(NULL)
    , m_fpConvertYUVtoRGB(NULL)
    , m_fpEnhance(NULL)
    , m_fpEnhanceUniform(NULL)
    , m_fpConvertRGB24ToXRGB(NULL)
    , m_fpScanAllCompatibleColorFormats(NULL)
    , m_fpGetColorConverter(NULL)
    , m_fpGetColorConverterx(NULL)
    , m_bLightColorConverter(FALSE)
#ifdef _WINDOWS
    , m_fpInitColorConverter2(NULL)
    , m_fpSetColorAdjustments2(NULL)
    , m_fpGetColorConverter2(NULL)
    , m_fpGetColorConverter2x(NULL)
    , m_fpUpgradeColorConverter(NULL)
    , m_pPlayer(NULL)
#endif
{
    UINT32 uDLLNameLen = MAX_DLL_NAME_LEN;

    m_pContext = pContext;
    if (m_pContext)
    {
        m_pContext->AddRef();
    }

    // If the Codec directory is not currently specified, set it
#ifndef _VXWORKS
    if (!GetDLLAccessPath()->GetPath(DLLTYPE_CODEC))
    {
        SetCodecDir();
    }

    if (GetDLLAccessPath()->GetPath(DLLTYPE_CODEC))
    {
#endif // _VXWORKS
        m_pDllAccess = new DLLAccess;
        if (m_pDllAccess)
        {
            /*
             *  Attempt to load the full version 1st
             */
            DLLAccess::CreateName("colorcvt", "colorcvt", m_pDllName, uDLLNameLen);

            if (DLLAccess::DLL_OK != m_pDllAccess->open(m_pDllName, DLLTYPE_CODEC))
            {
                /*
                 *  Ok, the full version of hxcolor was
                 *  missing. Perhaps we have the lite version?
                 */
                uDLLNameLen = MAX_DLL_NAME_LEN;
                DLLAccess::CreateName("hxltcolor", "hxltcolor", m_pDllName, uDLLNameLen);
                if (DLLAccess::DLL_OK != m_pDllAccess->open(m_pDllName, DLLTYPE_CODEC))
                {
                    hStatus = HXR_FAIL;
                }
                else
                {
                    m_bLightColorConverter = TRUE;
                }
            }
        
            if (hStatus == HXR_FAIL)
            {
                delete m_pDllAccess;
                m_pDllAccess = NULL;
                PANIC(("Unable to load a color conversion library! \n"));
            }
            else
            {
                LoadConversionFunctions();
            }


        }
#ifndef _VXWORKS
    }
    else
    {
        PANIC(("Unable to locate codec directory! Cannot load the"
               "HXColor library!\n"));
    }
#endif

//    HX_RELEASE(pContext);
}

ColorFuncAccess::~ColorFuncAccess()
{
    if (m_pDllAccess)
    {
        m_pDllAccess->close();
        delete m_pDllAccess;
        m_pDllAccess = NULL;
    }

#ifdef _WINDOWS
    HX_RELEASE(m_pPlayer);
#endif

    HX_RELEASE(m_pContext);
}

HX_RESULT
ColorFuncAccess::GetHXColorGUID(UCHAR* pGUID)
{
    HX_RESULT ret = HXR_FAIL;
    UCHAR guid[16];
    if(m_fpGetHXColorGUID && pGUID)
    {
        ret = m_fpGetHXColorGUID(guid);
        if(ret == HXR_OK)
        {
            memcpy(pGUID, guid, sizeof(GUID)); /* Flawfinder: ignore */
        }
    }
    return ret;
}

void
ColorFuncAccess::InitColorConverter()
{
#ifdef _WINDOWS
    if (m_fpInitColorConverter2 && m_pContext)
    {
        m_fpInitColorConverter2(m_pContext);
    }
    else
#endif
        if (m_fpInitColorConverter)
        {
            m_fpInitColorConverter();
        }
}

void
ColorFuncAccess::SetColorAdjustments(float Brightness, float Contrast,
                                     float Saturation, float Hue)
{
#ifdef _WINDOWS
    if (m_fpSetColorAdjustments2 && m_pContext)
    {
        m_fpSetColorAdjustments2(m_pContext, Brightness, Contrast, Saturation, Hue);
    }
    else
#endif
        if (m_fpSetColorAdjustments)
        {
            m_fpSetColorAdjustments(Brightness, Contrast, Saturation, Hue);
        }
}

void
ColorFuncAccess::GetColorAdjustments(float *Brightness, float *Contrast,
                                     float *Saturation, float *Hue)
{
    if (m_fpGetColorAdjustments)
    {
        m_fpGetColorAdjustments(Brightness, Contrast, Saturation, Hue);
    }
}


int ColorFuncAccess::SuggestRGB8Palette (int nColors, UINT32 *lpRGBVals)
{
    return m_SuggestRGB8Palette? m_SuggestRGB8Palette(nColors, lpRGBVals): -1;
}

int ColorFuncAccess::SetRGB8Palette (int nColors, UINT32 *lpRGBVals, int *lpIndices)
{
    return m_SetRGB8Palette? m_SetRGB8Palette(nColors, lpRGBVals, lpIndices): -1;
}

void
ColorFuncAccess::SetSharpnessAdjustments(float Sharpness, INT16 nExpand)
{
    if (m_fpSetSharpnessAdjustments)
    {
        m_fpSetSharpnessAdjustments(Sharpness,nExpand);
    }
}

void
ColorFuncAccess::GetSharpnessAdjustments(float *Sharpness, INT16 *nExpand)
{
    if (m_fpGetSharpnessAdjustments)
    {
        m_fpGetSharpnessAdjustments(Sharpness,nExpand);
    }
}

void
ColorFuncAccess::ConvertRGBtoYUV(UCHAR* pInput, UCHAR* pOutput, INT32 nWidth,
                                 INT32 nHeight, HXBOOL bBGR)
{
    if (m_fpConvertRGBtoYUV)
    {
        m_fpConvertRGBtoYUV(pInput, pOutput, nWidth, nHeight, bBGR);
    }
}



void
ColorFuncAccess::ConvertYUVtoRGB(UCHAR* ySrc, UCHAR* uSrc, UCHAR* vSrc,
                                 INT32 nPitchSrc, UCHAR* Dst, INT32 nWidth,
                                 INT32 nHeight, INT32 nPitchDst, INT16 nFormat,
                                 INT16 nExpand)
{
    if (m_fpConvertYUVtoRGB)
    {
        m_fpConvertYUVtoRGB(ySrc, uSrc, vSrc, nPitchSrc, Dst,
                            nWidth, nHeight, nPitchDst, nFormat, nExpand);
    }
}

void ColorFuncAccess::Enhance(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness)
{
    if (m_fpEnhance)
    {
        m_fpEnhance(yuv420in, inheight, inwidth, pitchSrc, Sharpness);
    }

}

void ColorFuncAccess::EnhanceUniform(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness)
{
    if (m_fpEnhanceUniform)
    {
        m_fpEnhanceUniform(yuv420in, inheight, inwidth, pitchSrc, Sharpness);
    }

}

void
ColorFuncAccess::ConvertRGB24toXRGB( UCHAR* pSrc, UCHAR* pDest, ULONG32 srcSize, ULONG32 destSize, INT32 nWidth, INT32 nHeight)
{
    if (m_fpConvertRGB24ToXRGB)
    {
        m_fpConvertRGB24ToXRGB(pSrc,pDest,srcSize,destSize,nWidth,nHeight);
    }
}

int ColorFuncAccess::I420andYUVA(
    unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
    int src1_startx, int src1_starty,
    unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
    int src2_startx, int src2_starty,
    unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
    int dest_startx, int dest_starty,
    int width,  int height,  int color_format)
{
    if( m_fpI420andYUVA )
    {
        return m_fpI420andYUVA( src1_ptr, src1_pels, src1_lines, src1_pitch,
                                src1_startx, src1_starty,
                                src2_ptr, src2_pels, src2_lines, src2_pitch,
                                src2_startx, src2_starty,
                                dest_ptr, dest_pels, dest_lines, dest_pitch,
                                dest_startx, dest_starty, width, height,
                                color_format);
    }
    return HXR_FAIL;
}


int ColorFuncAccess::I420andI420toI420(
    unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
    int src1_startx, int src1_starty,
    unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
    int src2_startx, int src2_starty,
    unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
    int dest_startx, int dest_starty,
    int width,  int height,  int alpha )
{
    if( m_fpI420andI420toI420 )
    {
        return m_fpI420andI420toI420( src1_ptr, src1_pels, src1_lines, src1_pitch,
                                      src1_startx, src1_starty,
                                      src2_ptr, src2_pels, src2_lines, src2_pitch,
                                      src2_startx, src2_starty,
                                      dest_ptr, dest_pels, dest_lines, dest_pitch,
                                      dest_startx, dest_starty, width, height,
                                      alpha);
    }
    return HXR_FAIL;
}


LPHXCOLORCONVERTER ColorFuncAccess::GetColorConverter (INT32 cidIn, INT32 cidOut)
{
#ifdef _WINDOWS
    if (m_fpGetColorConverter2 && m_pContext)
    {
        LPHXCOLORCONVERTER colorConverter = m_fpGetColorConverter2(m_pContext, cidIn, cidOut);
        if (!colorConverter)
            m_fpUpgradeColorConverter(m_pContext,m_pPlayer, cidIn, cidOut);
        else
            return colorConverter;
    }
    else
#endif
        if (m_fpGetColorConverter)
        {
            return m_fpGetColorConverter(cidIn, cidOut);
        }
    return NULL;
}

LPHXCOLORCONVERTER2 ColorFuncAccess::GetColorConverter2 (INT32 cidIn, INT32 cidOut)
{
#ifdef _WINDOWS
    if (m_fpGetColorConverter2x && m_pContext)
    {
        LPHXCOLORCONVERTER2 colorConverter = m_fpGetColorConverter2x(m_pContext, cidIn, cidOut);
        if (!colorConverter)
            m_fpUpgradeColorConverter(m_pContext,m_pPlayer, cidIn, cidOut);
        else
            return colorConverter;
    }
    else
#endif
        if (m_fpGetColorConverterx)
        {
            return m_fpGetColorConverterx(cidIn, cidOut);
        }
    return NULL;
}


HXBOOL ColorFuncAccess::ScanCompatibleColorFormats(INT32 cidIn, INT32 cidOutMask, void *pParam,
                                                 HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC))
{
    if (m_fpScanCompatibleColorFormats)
    {
        return m_fpScanCompatibleColorFormats(cidIn, cidOutMask, pParam, pfnTryIt);
    }
    return FALSE;
}

HXBOOL ColorFuncAccess::ScanAllCompatibleColorFormats(INT32 cidIn, void *pParam,
                                                    HXBOOL (*pfnTryIt) (void *pParam, INT32 cidOut, LPHXCOLORCONVERTER pfnCC))
{
    if (m_fpScanAllCompatibleColorFormats)
    {
        return m_fpScanAllCompatibleColorFormats(cidIn, pParam, pfnTryIt);
    }
    return FALSE;
}

void
ColorFuncAccess::LoadConversionFunctions()
{
    if (m_pDllAccess)
    {
        m_fpGetHXColorGUID                = (FPGETHXCOLORGUID) m_pDllAccess->getSymbol("GetHXColorGUID");
        m_fpInitColorConverter            = (FPINITCOLORCONVERTER) m_pDllAccess->getSymbol("InitColorConverter");
        m_fpSetColorAdjustments           = (FPSETCOLORADJUSTMENTS)  m_pDllAccess->getSymbol("SetColorAdjustments");
        m_fpGetColorAdjustments           = (FPGETCOLORADJUSTMENTS) m_pDllAccess->getSymbol("GetColorAdjustments");
        m_SuggestRGB8Palette              = (FPSUGGESTRGB8PALETTE) m_pDllAccess->getSymbol("SuggestRGB8Palette");
        m_SetRGB8Palette                  = (FPSETRGB8PALETTE) m_pDllAccess->getSymbol("SetRGB8Palette");
        m_fpSetSharpnessAdjustments       = (FPSETSHARPNESSADJUSTMENTS) m_pDllAccess->getSymbol("SetSharpnessAdjustments");
        m_fpGetSharpnessAdjustments       = (FPGETSHARPNESSADJUSTMENTS) m_pDllAccess->getSymbol("GetSharpnessAdjustments");
        m_fpConvertRGBtoYUV               = (FPCONVERTRGBTOYUV) m_pDllAccess->getSymbol("ConvertRGBtoYUV");
        m_fpConvertYUVtoRGB               = (FPCONVERTYUVTORGB) m_pDllAccess->getSymbol("ConvertYUVtoRGB");
        m_fpEnhance                       = (FPENHANCE) m_pDllAccess->getSymbol("Enhance");
        m_fpEnhanceUniform                = (FPENHANCEUNIFORM) m_pDllAccess->getSymbol("EnhanceUniform");
        m_fpConvertRGB24ToXRGB            = (FPCONVERTRGB24TOXRGB) m_pDllAccess->getSymbol("ConvertRGB24toXRGB");
        m_fpScanCompatibleColorFormats    = (FPSCANCOMPATIBLECOLORFORMATS) m_pDllAccess->getSymbol("ScanCompatibleColorFormats");
        m_fpScanAllCompatibleColorFormats = (FPSCANALLCOMPATIBLECOLORFORMATS) m_pDllAccess->getSymbol("ScanAllCompatibleColorFormats");
        m_fpGetColorConverter             = (FPGETCOLORCONVERTER) m_pDllAccess->getSymbol("GetColorConverter");
        m_fpGetColorConverterx            = (FPGETCOLORCONVERTERx) m_pDllAccess->getSymbol("GetColorConverter2");
        
        m_fpI420andYUVA                   = (FPI420ANDYUVA) m_pDllAccess->getSymbol("I420andYUVA");
        m_fpI420andI420toI420             = (FPI420ANDI420TOI420) m_pDllAccess->getSymbol("I420andI420toI420");

#if _WINDOWS
#if defined(XXXX_NEW_COLOR_CONVERTER)
        m_fpInitColorConverter2 = (FPINITCOLORCONVERTER2)  m_pDllAccess->getSymbol("InitColorConverter");
        m_fpGetColorConverter2  = (FPGETCOLORCONVERTER2) m_pDllAccess->getSymbol("GetColorConverter");
        m_fpGetColorConverter2x = (FPGETCOLORCONVERTER2x) m_pDllAccess->getSymbol("GetColorConverter2x");
        m_fpSetColorAdjustments2  = (FPSETCOLORADJUSTMENTS2) m_pDllAccess->getSymbol("SetColorAdjustments");
        m_fpUpgradeColorConverter = (FPUPGRADECOLORCONVERTER) m_pDllAccess->getSymbol("UpgradeColorConverter");
#endif
#endif
    }

    if (!m_fpInitColorConverter  ||
        !m_fpSetColorAdjustments ||
        !m_fpSetSharpnessAdjustments ||
        !m_fpEnhance ||
        !m_fpEnhanceUniform ||
#ifdef _WINDOWS 
        !m_fpGetColorAdjustments ||
        !m_fpGetSharpnessAdjustments ||
#endif
        //!m_fpConvertRGB24ToXRGB ||
        !m_fpScanAllCompatibleColorFormats ||
        !m_fpGetColorConverter
        )
    {
        WrongHXColorVersion();
    }
    else
    {
        if (!m_bLightColorConverter && (
                !m_SuggestRGB8Palette    ||
                !m_SetRGB8Palette        ||
                !m_fpConvertRGBtoYUV     ||
                !m_fpConvertYUVtoRGB)
            )
        {
            WrongHXColorVersion();
        }
    }
}

void ColorFuncAccess::WrongHXColorVersion()
{
    m_fpInitColorConverter  = NULL;
    m_fpSetColorAdjustments = NULL;
    m_fpGetColorAdjustments = NULL;
    m_SuggestRGB8Palette    = NULL;
    m_SetRGB8Palette        = NULL;
    m_fpSetSharpnessAdjustments = NULL;
    m_fpGetSharpnessAdjustments = NULL;
    m_fpConvertRGBtoYUV = NULL;
    m_fpConvertYUVtoRGB = NULL;
    m_fpEnhance=NULL;
    m_fpEnhanceUniform=NULL;
    m_fpConvertRGB24ToXRGB = NULL;
    m_fpScanCompatibleColorFormats = NULL;
    m_fpScanAllCompatibleColorFormats = NULL;
    m_fpGetColorConverter = NULL;

    delete m_pDllAccess;
    m_pDllAccess = NULL;
    hStatus = HXR_FAIL;

    HX_ASSERT(WRONG_HXCOLOR_VERSION);
}

HX_RESULT
ColorFuncAccess::SetCodecDir()
{
#ifndef _VXWORKS
    const char* pPath = NULL;
    CHXString codecDir;

    pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
    HX_ASSERT(pPath && *pPath);

    codecDir = pPath;

#ifndef _MACINTOSH
    if (strcmp((const char*)codecDir.Right(1), OS_SEPARATOR_STRING))
    {
        codecDir += OS_SEPARATOR_STRING;
    }
    codecDir += "Codecs";
#endif

    GetDLLAccessPath()->SetPath(DLLTYPE_CODEC, (const char*)codecDir);
#endif // _VXWORKS
    return HXR_OK;
}

#ifdef _WINDOWS
void
ColorFuncAccess::SetPlayer(IHXPlayer* pPlayer)
{
    if (m_pPlayer)
    {
        m_pPlayer->Release();
    }

    m_pPlayer = pPlayer;

    if (m_pPlayer)
    {
        m_pPlayer->AddRef();
    }
}
#endif


HXBOOL ColorFuncAccess::CheckColorConverter (INT32 cidIn, INT32 cidOut)
{
    LPHXCOLORCONVERTER fpConvert = GetColorConverter(cidIn, cidOut);
    return fpConvert != NULL;
}

HXBOOL ColorFuncAccess::CheckColorConverter2 (INT32 cidIn, INT32 cidOut)
{
    LPHXCOLORCONVERTER2 fpConvert = GetColorConverter2(cidIn, cidOut);
    return fpConvert != NULL;
}

int ColorFuncAccess::ColorConvert(INT32 cidOut, unsigned char *dest_ptr,
                                  int dest_width, int dest_height,
                                  int dest_pitch, int dest_x, int dest_y,
                                  int dest_dx, int dest_dy,
                                  INT32 cidIn, unsigned char *src_ptr,
                                  int src_width, int src_height,
                                  int src_pitch, int src_x, int src_y,
                                  int src_dx, int src_dy)
{
    int nRet = -1;

    LPHXCOLORCONVERTER fpConvert = GetColorConverter (cidIn, cidOut);

    if (fpConvert)
    {
        nRet = fpConvert(dest_ptr, dest_width, dest_height,
                         dest_pitch, dest_x, dest_y,
                         dest_dx, dest_dy,
                         src_ptr, src_width, src_height,
                         src_pitch, src_x, src_y,
                         src_dx, src_dy);
    }

    return nRet;
}

int ColorFuncAccess::ColorConvert2(INT32 cidOut, unsigned char *dest_ptr,
                                   int dest_width, int dest_height,
                                   int dest_pitch, int dest_x, int dest_y,
                                   int dest_dx, int dest_dy,
                                   INT32 cidIn, unsigned char *pY,
                                   unsigned char *pU, unsigned char *pV,
                                   int src_width, int src_height,
                                   int yPitch, int uPitch, int vPitch,
                                   int src_x, int src_y,
                                   int src_dx, int src_dy)
{
    int nRet = -1;

    LPHXCOLORCONVERTER2 fpConvert = GetColorConverter2 (cidIn, cidOut);

    if (fpConvert)
    {
        nRet = fpConvert(dest_ptr, dest_width, dest_height,
                         dest_pitch, dest_x, dest_y,
                         dest_dx, dest_dy,
                         pY, pU, pV, src_width, src_height,
                         yPitch, uPitch, vPitch,
                         src_x, src_y,
                         src_dx, src_dy);
    }

    return nRet;
}

