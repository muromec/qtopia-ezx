/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: colormap.h,v 1.3 2007/03/05 17:38:03 ping Exp $
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

#ifndef __COLORMAP_H__
#define __COLORMAP_H__   1

#ifdef __cplusplus
extern "C" {
#endif

    
#include "ciddefs.h"

// this is needed here primarily for UNIX where mmioFOURCC is not present
// this is also defined in /src/comlib/win/ddraw.h - which is also not
// used with UNIXes
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((ULONG32)(BYTE)(ch0) | ((ULONG32)(BYTE)(ch1) << 8) |   \
                ((ULONG32)(BYTE)(ch2) << 16) | ((ULONG32)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)

/*
 * Some useful format-classification macros:
 */
#define IsGDI(cid)          ((cid) >= CID_RGB32 && (cid) <= CID_RGB8)
#define IsPalettized(cid)   ((cid) == CID_RGB8)
#define IsRGB(cid)          (((cid) >= CID_RGB32 && (cid) <= CID_RGB8) || ((cid) >= CID_BGR32 && (cid) <= CID_RGB444) || ((cid)==CID_ARGB32) )
#define IsYUV(cid)          ((cid)==CID_YUVA || (cid) == CID_XING || (cid) == CID_DVPF || ((cid) >= CID_I420 && (cid) <= CID_UYVY))
#define IsYUVPlanar(cid)    ( (cid)==CID_YUVA || (cid)==CID_XING || ((cid) >= CID_I420 && (cid) <= CID_YVU9))
#define IsYUVPacked(cid)    ((cid) >= CID_YUY2 && (cid) <= CID_UYVY)
#define IsMotionComp(cid)   ((cid) >= CID_MC12 && (cid) <= CID_IGOR)
#define IsHWPostfilter(cid) ((cid) == CID_DVPF)
#define IsStructured(cid)   ((cid) >= CID_MC12 && (cid) <= CID_DVPF)

/*
 * Get color format ID.
 * Use:
 *  int GetBitmapColor (LPBITMAPINFO lpbi);
 *  int GetDirectDrawColor (LPDDPIXELFORMAT lpddpf);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure containing image format
 *  lpddpf - pointer to a DirectDraw DDPIXELFORMAT structure
 * Returns:
 *  ID of the specified format, if found;
 *  CID_UNKNOWN, if not supported.
 */
#ifdef _WIN32
#include <ddraw.h>

    int GetBitmapColor (LPBITMAPINFO lpbi);

#if !defined(WIN32_PLATFORM_PSPC)
    int GetDirectDrawColor (LPDDPIXELFORMAT lpddpf);
#endif /* !defined(WIN32_PLATFORM_PSPC) */
#else
    int GetBitmapColor (HXBitmapInfo* lpbi);
#endif

/*
 * Set color format.
 * Use:
 *  int SetBitmapColor (LPBITMAPINFO lpbi, int cid);
 *  int SetDirectDrawColor (LPDDPIXELFORMAT lpddpf, int cid);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure containing image format
 *  lpddpf - pointer to a DirectDraw DDPIXELFORMAT structure
 *  cid - color format to set
 * Returns:
 *  0, if success,
 *  !0 if wrong color format ID passed
 */
#ifdef _WIN32
int SetBitmapColor (LPBITMAPINFO lpbi, int cid);

#if !defined(WIN32_PLATFORM_PSPC)
int SetDirectDrawColor (LPDDPIXELFORMAT lpddpf, int cid);
#endif /* !defined(WIN32_PLATFORM_PSPC) */
#else
int SetBitmapColor (HXBitmapInfo* lpbi, int cid);
#endif

/*
 * Get pitch of the bitmap image.
 * Use:
 *  int GetBitmapPitch (LPBITMAPINFO lpbi);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure containing image format
 * Returns:
 *  !0 -- pitch of the bitmap image; <0 if bottom-up bitmap
 *  0 - unrecognized bitmap format
 */
#ifdef _WIN32    
int GetBitmapPitch (LPBITMAPINFO lpbi);
#else    
int GetBitmapPitch (HXBitmapInfo* lpbi);
#endif


    
/*
 * Get size of the bitmap image.
 * Use:
 *  int GetBitmapImageSize (LPBITMAPINFO lpbi);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure containing image format
 * Returns:
 *  !0 -- size of the bitmap image
 *  0 - unrecognized bitmap format
 */
#ifdef _WIN32    
int GetBitmapImageSize (LPBITMAPINFO lpbi);
#else    
int GetBitmapImageSize (HXBitmapInfo* lpbi);
#endif

#ifdef _WIN32   
int MakeBitmap (LPBITMAPINFO lpbi, int nBISize,
    int cid, ULONG32 dwWidth, ULONG32 dwHeight, LPPALETTEENTRY lppe, int nColors);
#else
int MakeBitmap( HXBitmapInfo* lpbi, int nBISize, int cid,
                ULONG32 dwWidth, ULONG32 dwHeight, void* lppe, int nColors);
#endif   


/*
 * Maps a FourCC to CID don't use this for RGB values.
 * Use:
 *  int MapFourCCtoCID (ULONG32 dwFourCC);
 * Input:
 *  dwFourCC - teh fourCC you want to map to a CID
 * Returns:
 *  teh CID, if found
 */
int MapFourCCtoCID(ULONG32 dwFourCC);

/*
 * Maps a CID to a FourCC
 * Use:
 *  int MapCIDtoFourCC (ULONG32 CID);
 * Returns:
 *  the FourCC, if found
 *  else 0.
 */

int MapCIDtoFourCC(ULONG32 CID);

#ifdef _WIN32

/*
 * Get bitmap palette.
 * Use:
 *  int GetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure
 *  lppe - pointer to a buffer to contain palette entries
 * Returns:
 *  the number of colors in palette
 *  0, if bitmap does not use palette.
 */
int GetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe);

/*
 * Set bitmap palette.
 * Use:
 *  int SetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe, int n);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure
 *  lppe - pointer to a buffer containing palette entries
 *  n    - the total number of colors in palette
 * Returns:
 *  the number of colors set
 *  0, if bitmap does not use palette.
 */
int SetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe, int n);


/*
 * Check the validity of a bitmap structure.
 * Use:
 *  int CheckBitmap (LPBITMAPINFO lpbi);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure to check
 * Returns:
 *  !0 - bitmap parameters are correct
 *  0 - otherwise
 */
int CheckBitmap (LPBITMAPINFO lpbi);

#endif // _WIN32
#ifdef __cplusplus
}
#endif

#endif /* __COLORMAP_H__ */

