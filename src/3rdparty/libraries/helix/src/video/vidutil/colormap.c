/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: colormap.c,v 1.11 2009/05/06 05:55:13 eepaul Exp $
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

#if defined(_WIN32) && !defined(_WINCE)
/* Windows include files: */
#include <vfw.h>
#include <ddraw.h>
#include "hxtypes.h"
#elif defined(_WINCE)
#include "hxwintyp.h"
#include "hxcom.h"
//#undef _WIN32
#include "hxvsurf.h"
#endif

#include "hxcom.h"
#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxvsurf.h"

/* Color IDs:*/
#include "colormap.h"

/*
 * FOURCC codes (in addition to what is defined in wingdi.h):
 */

/* standard YUV formats: */
#define BI_I420         MAKEFOURCC('I','4','2','0') /* planar YCrCb   */
#define BI_YUVA         MAKEFOURCC('Y','U','V','A') /* planar YVU 420 plus alpha plane */
#define BI_YV12         MAKEFOURCC('Y','V','1','2') /* planar YVU 420 */
#define BI_YVU9         MAKEFOURCC('Y','V','U','9') /* planar YVU 420 */
#define BI_YUY2         MAKEFOURCC('Y','U','Y','2') /* packed YVU 422 */
#define BI_UYVY         MAKEFOURCC('U','Y','V','Y') /* packed YVU 422 */
#define BI_DVPF         MAKEFOURCC('D','V','P','F') /* planar YCrCb in structure */

/* Xing decoder's YUV format */
#define BI_XING         MAKEFOURCC('X','I','N','G')

/* MPEG-2 pre-decoded formats: */
#define BI_MC12         MAKEFOURCC('M','C','1','2') /* ATI MPEG-2 M/C */
#define BI_MCAM         MAKEFOURCC('M','C','A','M') /* ATI MPEG-2 M/C */
#define BI_MCS3         MAKEFOURCC('M','C','S','3') /* S3 MPEG-2 M/C  */
#define BI_IGOR         MAKEFOURCC('I','G','O','R') /* Intel i810 M/C */

/* LIBVA */
#define BI_LIBVA         MAKEFOURCC('L','B','V','A') /* using LibVa surface */

/* RMA proprietary formats: */
#ifndef HXCOLOR_RGB3_ID
#define HXCOLOR_RGB3_ID     MAKEFOURCC('3','B','G','R') /* RGB-32 ??      */
#define HXCOLOR_RGB24_ID    MAKEFOURCC('B','G','R',' ') /* top-down RGB-24*/
#define HXCOLOR_RGB565_ID   MAKEFOURCC('6','B','G','R') /* RGB-16 565     */
#define HXCOLOR_RGB555_ID   MAKEFOURCC('5','B','G','R') /* RGB-16 555     */
#define HXCOLOR_8BIT_ID     MAKEFOURCC('T','I','B','8') /* RGB-8 w. pal-e */
#define HXCOLOR_YUV420_ID   MAKEFOURCC('2','V','U','Y') /* planar YCrCb   */
#define HXCOLOR_YUV411_ID   MAKEFOURCC('1','V','U','Y') /* ???            */
#define HXCOLOR_YUVRAW_ID   MAKEFOURCC('R','V','U','Y') /* ???            */
#endif

/*
 * Color format tables:
 */

/* control flags: */
#define _FOURCC   0x01
#define _BITCOUNT 0x02
#define _BITMASK  0x04

/* color format descriptor: */

struct _greg
{
    ULONG32 dwFlags;              /* indicates valid fields       */
    ULONG32 dwFourCC;             /* FourCC code                  */
    ULONG32 dwBitCount;           /* number of bits/pixel         */
    ULONG32 dwBitMask [3];        /* RGB bit masks                */
};

typedef struct _greg  CIDD;
typedef struct _greg* LPCIDD;

/* color format descriptors to use with YUV bitmaps: */
static const CIDD ciddI420      = {_FOURCC,                    BI_I420,       12, {       0,        0,        0}};
static const CIDD ciddYUVA      = {_FOURCC,                    BI_YUVA,       20, {       0,        0,        0}};
static const CIDD ciddYV12      = {_FOURCC,                    BI_YV12,       12, {       0,        0,        0}};
static const CIDD ciddYVU9      = {_FOURCC,                    BI_YVU9,        9, {       0,        0,        0}};
static const CIDD ciddYUY2      = {_FOURCC,                    BI_YUY2,       16, {       0,        0,        0}};
static const CIDD ciddUYVY      = {_FOURCC,                    BI_UYVY,       16, {       0,        0,        0}};
static const CIDD ciddDVPF      = {_FOURCC,                    BI_DVPF,       12, {       0,        0,        0}};
static const CIDD ciddXING      = {_FOURCC,                    BI_XING,       16, {       0,        0,        0}};


/* color format descriptors to use with GDI RGB bitmaps: */
static const CIDD ciddBmpRGB32  = {_FOURCC+_BITCOUNT,          BI_RGB,        32, {       0,        0,        0}};
static const CIDD ciddBmpARGB32 = {_FOURCC+_BITCOUNT,          HX_ARGB,      32, {       0,        0,        0}};
static const CIDD ciddBmpRGB24  = {_FOURCC+_BITCOUNT,          BI_RGB,        24, {       0,        0,        0}};
static const CIDD ciddBmpRGB565 = {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,  16, {  0xF800,   0x07E0,   0x001F}};
static const CIDD ciddBmpRGB555 = {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,  16, {  0x7C00,   0x03E0,   0x001F}};
static const CIDD ciddBmpRGB8   = {_FOURCC+_BITCOUNT,          BI_RGB,         8, {       0,        0,        0}};

/* color format descriptors to use with DirectDraw RGB surfaces: */
static const CIDD ciddDDrRGB32  = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        32, {0xFF0000, 0x00FF00, 0x0000FF}};
static const CIDD ciddDDrARGB32 = {_FOURCC+_BITCOUNT+_BITMASK, HX_ARGB,      32, {0xFF0000, 0x00FF00, 0x0000FF}};
static const CIDD ciddDDrRGB24  = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        24, {0xFF0000, 0x00FF00, 0x0000FF}};
static const CIDD ciddDDrRGB565 = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        16, {  0xF800,   0x07E0,   0x001F}};
static const CIDD ciddDDrRGB555 = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        16, {  0x7C00,   0x03E0,   0x001F}};
static const CIDD ciddDDrRGB8   = {_FOURCC+_BITCOUNT,          BI_RGB,         8, {       0,        0,        0}};

/* color format descriptors to use with BRG bitmaps: */
static const CIDD ciddBGR32     = {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,  32, {0x0000FF, 0x00FF00, 0xFF0000}};
static const CIDD ciddBGR24     = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        24, {0x0000FF, 0x00FF00, 0xFF0000}};

/* color format descriptors to use with QuickTime byte swapped formats: */
static const CIDD ciddRGB32S    = {_FOURCC+_BITCOUNT+_BITMASK, BI_RGB,        32, {0x00FF00, 0xFF0000, 0xFF000000}};

/* color format descriptors to use with embedded device formats: */
static const CIDD ciddRGB444    = {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,  16, {  0x0F00,   0x00F0,   0x000F}};

/* color format descriptors to use with MPEG bitmaps: */
static const CIDD ciddMC12      = {_FOURCC,                    BI_MC12,       12, {       0,        0,        0}};
static const CIDD ciddMCAM      = {_FOURCC,                    BI_MCAM,       12, {       0,        0,        0}};
static const CIDD ciddMCS3      = {_FOURCC,                    BI_MCS3,       12, {       0,        0,        0}};
static const CIDD ciddIGOR      = {_FOURCC,                    BI_IGOR,       12, {       0,        0,        0}};

static const CIDD ciddLIBVA      = {_FOURCC,                    BI_LIBVA,       1, {       0,        0,        0}};

/* non-standard descriptors we should support: */
struct _stColors
{
    int cid;                    /* image format ID              */
    CIDD  cidd;                 /* color format descriptor      */
};

static const struct _stColors ciddlOtherColors [] = {
    {CID_I420, {_FOURCC,                    HXCOLOR_YUV420_ID, 12, {         0,          0,          0}}},
    {CID_YUVA,              {_FOURCC,                    HXCOLOR_YUVA_ID,   20, {         0,          0,          0}}},
    {CID_RGB32,             {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,      32, {0xFF000000, 0x00FF0000, 0x0000FF00}}}, 
    {CID_RGB565,            {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,      16, {0x0000f800, 0x000007c0, 0x0000003e}}}, 
    {CID_RGB32,             {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,      32, {  0xFF0000,   0x00FF00,   0x0000FF}}},
    {CID_RGB32,             {_FOURCC+_BITCOUNT,          HXCOLOR_RGB3_ID,   32, {         0,          0,          0}}},
    {CID_RGB24,             {_FOURCC+_BITCOUNT+_BITMASK, BI_BITFIELDS,      24, {  0xFF0000,   0x00FF00,   0x0000FF}}},
    {CID_RGB24,             {_FOURCC+_BITCOUNT,          HXCOLOR_RGB3_ID,   24, {         0,          0,          0}}},
    {CID_RGB24,             {_FOURCC,                    HXCOLOR_RGB24_ID,  24, {         0,          0,          0}}},
    {CID_RGB565,            {_FOURCC,                    HXCOLOR_RGB565_ID, 16, {         0,          0,          0}}},
    {CID_RGB555,            {_FOURCC+_BITCOUNT,          BI_RGB,            16, {         0,          0,          0}}},
    {CID_RGB555,            {_FOURCC,                    HXCOLOR_RGB555_ID, 16, {         0,          0,          0}}},
    {CID_RGB8,              {_FOURCC,                    HXCOLOR_8BIT_ID,    8, {         0,          0,          0}}},
    {CID_UNKNOWN,           {0      ,                    0,                  0, {         0,          0,          0}}}
};

/* color format attributes: */
#define _BITMAP         0x0001  /* has bitmap descriptor        */
#define _DIRECTDRAW     0x0002  /* has DirectDraw descriptor    */
#define _RGB            0x0100  /* normal RGB format            */
#define _BGR            0x0200  /* B/R flipped RGB format       */
#define _YUV            0x0400  /* YUV format                   */
#define _MPEG           0x0800  /* MPEG pre-decoded format      */
#define _RGBS           0x1000  /* Byte swapped RGB format      */

/* the main color descriptors' table: */
struct _stColorTable
{
    ULONG32 dwFlags;               /* general format properties    */
    const struct _greg * lpBitmapCIDD;     /* bitmap format descriptor     */
    const struct _greg * lpDirectDrawCIDD; /* DirectDraw format descriptor */
    int     nBPP;                  /* bytes per pixel              */
};

static const struct _stColorTable  ciddTbl[NFORMATS] = {
    {_BITMAP+_YUV,              &ciddI420,      (const struct _greg*)NULL,    1}, /* CID_I420   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddYV12,      &ciddYV12,      1}, /* CID_YV12   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddYVU9,      &ciddYVU9,      1}, /* CID_YVU9   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddYUY2,      &ciddYUY2,      2}, /* CID_YUY2   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddUYVY,      &ciddUYVY,      2}, /* CID_UYVY   */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpRGB32,  &ciddDDrRGB32,  4}, /* CID_RGB32  */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpRGB24,  &ciddDDrRGB24,  3}, /* CID_RGB24  */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpRGB565, &ciddDDrRGB565, 2}, /* CID_RGB565 */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpRGB555, &ciddDDrRGB555, 2}, /* CID_RGB555 */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpRGB8,   &ciddDDrRGB8,   1}, /* CID_RGB8   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddXING,      &ciddXING,      1}, /* CID_XING   */
    {_BITMAP+_DIRECTDRAW+_RGB,  &ciddBmpARGB32, &ciddDDrARGB32, 4}, /* CID_ARGB32 */
    {_BITMAP+_YUV,              &ciddYUVA,      NULL,           1}, /* CID_YUVA   */
    {_BITMAP+_YUV,              &ciddYUY2,      &ciddYUY2,      2}, /* CID_YUYU   */
    {0,                         NULL,           NULL,           0}, /* CID_UNKNOWN*/
    {_BITMAP+_BGR,              &ciddBGR32,     NULL,           4}, /* CID_BGR32  */
    {_BITMAP+_BGR,              &ciddBGR24,     NULL,           3}, /* CID_BGR24  */
    {_BITMAP+_RGBS,             &ciddRGB32S,    NULL,           4}, /* CID_RGB32S */
    {_BITMAP+_RGB,              &ciddRGB444,    NULL,           2}, /* CID_RGB444 */
    {_BITMAP+_DIRECTDRAW+_MPEG, &ciddMC12,      &ciddMC12,      1}, /* CID_MC12   */
    {_BITMAP+_DIRECTDRAW+_MPEG, &ciddMCAM,      &ciddMCAM,      1}, /* CID_MCAM   */
    {_BITMAP+_DIRECTDRAW+_MPEG, &ciddMCS3,      &ciddMCS3,      1}, /* CID_MCS3   */
    {_BITMAP+_DIRECTDRAW+_MPEG, &ciddIGOR,      &ciddIGOR,      1}, /* CID_IGOR   */
    {_BITMAP+_DIRECTDRAW+_YUV,  &ciddDVPF,      &ciddDVPF,      1}, /* CID_DVPF   */
    {_BITMAP,                   &ciddLIBVA,     &ciddLIBVA,     1}  /* CID_LIBVA   */
};

                                                 
/*********************************
 *  _TEXT segment :-)
 *********************************/
int MapFourCCtoCID(ULONG32 dwFourCC)
{
    int i;

    //for (i=0; i<CID_UNKNOWN; i++)
    for (i=0; i<NFORMATS; i++)
    {
        if (ciddTbl[i].lpBitmapCIDD &&
            ciddTbl[i].lpBitmapCIDD->dwFourCC == dwFourCC)
	    {
	        return i;
	    }
    }

    return CID_UNKNOWN;
}

int MapCIDtoFourCC(ULONG32 CID)
{
    if (CID < NFORMATS)
        return ciddTbl[CID].lpBitmapCIDD->dwFourCC;
    else
        return 0;
}


/*
 * Check color format:
 */
static int ChkColor (const struct _greg* pcidd, ULONG32 dwFourCC, ULONG32 dwBitCount, ULONG32 *lpBitMask)
{
    /* clear match flags: */
    ULONG32 dwMatch = 0;

    /* check FourCC: */
    if ((pcidd->dwFlags & _FOURCC) && pcidd->dwFourCC == dwFourCC)
        dwMatch |= _FOURCC;

    /* check BitCount: */
    if ((pcidd->dwFlags & _BITCOUNT) && pcidd->dwBitCount == dwBitCount)
        dwMatch |= _BITCOUNT;

    /* check BitMasks: */
    if ((pcidd->dwFlags & _BITMASK) && lpBitMask &&
        pcidd->dwBitMask[0] == lpBitMask[0] &&
        pcidd->dwBitMask[1] == lpBitMask[1] &&
        pcidd->dwBitMask[2] == lpBitMask[2])
        dwMatch |= _BITMASK;

    /* combine results: */
    return dwMatch == pcidd->dwFlags;
}

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
int GetBitmapColor (LPBITMAPINFO lpbi)
#else
int GetBitmapColor (HXBitmapInfo* lpHXbi)
#endif
{
    ULONG32 dwFourCC, dwBitCount, *lpColors;
    register int i;

#ifdef _WIN32
    HXBitmapInfo* lpHXbi;

    if (!lpbi)
    {
	return CID_UNKNOWN;
    }
    lpHXbi = (HXBitmapInfo*) lpbi;
#endif

    /* check bitmap pointer/size... */
    if (lpHXbi == NULL)
        return CID_UNKNOWN;

    /* get bitmap parameters to compare: */
    dwFourCC = lpHXbi->bmiHeader.biCompression;
    dwBitCount = lpHXbi->bmiHeader.biBitCount;
    lpColors = (ULONG32*)&lpHXbi->un.dwBitMask[0];

    /* scan standard bitmap formats: */
    for (i=0; i<NFORMATS; i++)
        if ((ciddTbl[i].dwFlags & _BITMAP) && ciddTbl[i].lpBitmapCIDD &&
            ChkColor (ciddTbl[i].lpBitmapCIDD, dwFourCC, dwBitCount, lpColors))
            return i;
    
    /* scan our proprietary formats: */
    for (i=0; ; i++)
        if (ChkColor (&(ciddlOtherColors[i].cidd), dwFourCC, dwBitCount, lpColors))
            break;

    /* return color ID: */
    return ciddlOtherColors[i].cid;
}

#if defined(_WIN32) && !defined(WINCE)
int GetDirectDrawColor (LPDDPIXELFORMAT lpddpf)
{
    /* check bitmap pointer... */
    if (lpddpf != NULL && lpddpf->dwSize >= sizeof(DDPIXELFORMAT)) {

        /* scan DirectDraw formats: */
        register int i;
        for (i=0; i<NFORMATS; i++)
            if ((ciddTbl[i].dwFlags & _DIRECTDRAW) && ciddTbl[i].lpDirectDrawCIDD &&
                ChkColor (ciddTbl[i].lpDirectDrawCIDD, lpddpf->dwFourCC, lpddpf->dwRGBBitCount, &(lpddpf->dwRBitMask)))
                return i;
    }
    return CID_UNKNOWN;
}
#endif


/*
 * Set color format.
 * Use:
 *  void SetBitmapColor (LPBITMAPINFO lpbi, int cid);
 *  void SetDirectDrawColor (LPDDPIXELFORMAT lpddpf, int cid);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure containing image format
 *  lpddpf - pointer to a DirectDraw DDPIXELFORMAT structure
 *  cid - color format to set
 * Returns:
 *  0, if success,
 *  !0 if wrong color format ID passed
 */
#ifdef _WIN32
int SetBitmapColor (LPBITMAPINFO lpbi, int cid)
#else
int SetBitmapColor (HXBitmapInfo* lpHXbi, int cid)
#endif
{
    const struct _greg* lpcidd;

#ifdef _WIN32
    HXBitmapInfo* lpHXbi;

    if (!lpbi)
    {
	return CID_UNKNOWN;
    }
    lpHXbi = (HXBitmapInfo*) lpbi;
#endif

    /* check input parameters: */
    if (lpHXbi == NULL ||
        cid < 0 || cid > NFORMATS || !(ciddTbl[cid].dwFlags & _BITMAP) ||
        (lpcidd = ciddTbl[cid].lpBitmapCIDD) == NULL)
        return -1;

    /* set color-format releated fields in BITMAPINFO: */
    lpHXbi->bmiHeader.biCompression = lpcidd->dwFourCC;
    lpHXbi->bmiHeader.biBitCount    = (UINT16)lpcidd->dwBitCount;

    /* set color masks: */
    if (lpcidd->dwFlags & _BITMASK) {

        /* get offset of color masks in the bitmap: */
        ULONG32 *lpColors = (ULONG32*)&lpHXbi->un.dwBitMask[0];
        lpColors[0] = lpcidd->dwBitMask[0];
        lpColors[1] = lpcidd->dwBitMask[1];
        lpColors[2] = lpcidd->dwBitMask[2];
    }

    /* success: */
    return 0;
}



/*
 * Calculates the size of an image with a given color format.
 */
static int ImageSize (int cid, ULONG32 dwWidth, ULONG32 dwHeight)
{
    int pitch, size;

    /* do it in a lazy way: */
    switch (cid) {
        /* planar YUV 4:2:0 formats: */
        case CID_I420:
        case CID_YV12:
            size = dwHeight * dwWidth * 3 / 2;
            break;
        /* YUV 9 format: */
       case CID_YUVA:
           size = (dwHeight * dwWidth) * 5 / 2;
           break;
        case CID_YVU9:
            size = dwHeight * dwWidth * 9 / 8;
            break;
        /* packet YUV 4:2:2 formats: */
        case CID_YUY2:
        case CID_UYVY:
            size = dwHeight * dwWidth * 2;
            break;
        /* RGB formats: */
        case CID_RGB32:
        case CID_ARGB32:
        case CID_RGB24:
        case CID_RGB565:
        case CID_RGB555:
        case CID_RGB8:
        case CID_BGR32:
        case CID_BGR24:
        case CID_RGB32S:
        case CID_RGB444:
            pitch = dwWidth * ciddTbl[cid].nBPP;
            pitch = (pitch + 3) & ~3;
            size = dwHeight * pitch;
			break;
        /* the other formats: */
        default:
            size = 0; /* no idea what size should be??? */
    }
    return size;
}

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
int GetBitmapPitch (LPBITMAPINFO lpbi)
#else      
int GetBitmapPitch (HXBitmapInfo* lpbi)
#endif      
{
    register int cid, pitch;

    /* check bitmap pointer & format: */
    cid = GetBitmapColor (lpbi);
    if (cid == CID_UNKNOWN || !(ciddTbl[cid].dwFlags & _BITMAP))
        return 0;

    if (cid == CID_XING)
        return 768;
    
    /* calculate image pitch: */
    pitch = lpbi->bmiHeader.biWidth * ciddTbl[cid].nBPP;
    if (ciddTbl[cid].dwFlags & (_RGB|_BGR))
#if defined(_MACINTOSH) || defined(_UNIX)
        pitch = ((pitch + 3) & ~3);
#else
        pitch = -((pitch + 3) & ~3);
#endif

    /* return pitch: */
    return pitch;
}

/*
* Get pitch of the bitmap image.
* Use:
*  int GetBitmapPitch2 (int cid, int biWidth);
* Input:
*  cid
*  biWidth
* Returns:
*  !0 -- pitch of the bitmap image; <0 if bottom-up bitmap
*  0 - unrecognized bitmap format
*/
int GetBitmapPitch2 (int cid, int biWidth)
{
	register int pitch;
	
	if (cid == CID_UNKNOWN || !(ciddTbl[cid].dwFlags & _BITMAP))
		return 0;

	if (cid == CID_XING)
		return 768;

	/* calculate image pitch: */
	pitch = biWidth * ciddTbl[cid].nBPP;
	if (ciddTbl[cid].dwFlags & (_RGB|_BGR))
#if defined(_MACINTOSH) || defined(_UNIX)
		pitch = ((pitch + 3) & ~3);
#else
		pitch = -((pitch + 3) & ~3);
#endif

	/* return pitch: */
	return pitch;
}

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
int GetBitmapImageSize (LPBITMAPINFO lpbi)
#else      
int GetBitmapImageSize (HXBitmapInfo* lpbi)
#endif      
{
    /* check bitmap pointer & format: */
    register int cid = GetBitmapColor (lpbi);
    if (cid == CID_UNKNOWN ||
        !(ciddTbl[cid].dwFlags & _BITMAP) ||
        lpbi->bmiHeader.biWidth <= 0 ||
        lpbi->bmiHeader.biHeight <= 0 ||
        lpbi->bmiHeader.biPlanes != 1)
        return 0;

    return ImageSize (cid, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight);
}


/*
 * Build a bitmap structure.
 * Use:
 *  int MakeBitmap (LPBITMAPINFO lpbi, int nBISize,
 *   int cid, ULONG32 dwWidth, ULONG32 dwHeight, LPPALETTEENTRY lppe, int nColors);
 * Input:
 *  lpbi - pointer to BITMAPINFO structure to contain image format
 *  nBISize - size of memory block allocated for BITMAPINFO structure
 *  cid - color format to use for bitmap
 *  dwWidth, dwHeight - image width/height
 *  lppe, nColors - palette info
 * Returns:
 *  !0 - bitmap has been successfully created
 *  0 - invalid bitmap parameters
 */
#ifdef _WIN32
int MakeBitmap (LPBITMAPINFO lpbi, int nBISize,
    int cid, ULONG32 dwWidth, ULONG32 dwHeight, LPPALETTEENTRY lppe, int nColors)
#else
int MakeBitmap(HXBitmapInfo* lpbi, int nBISize, int cid, ULONG32 dwWidth,
               ULONG32 dwHeight, void* lppe, int nColors)
#endif      
{
    const struct _greg* lpcidd;
    int setPalette, bitmapinfoSize;
    /* check input parameters: */
    if (lpbi == NULL ||
        cid < 0 || cid > NFORMATS || !(ciddTbl[cid].dwFlags & _BITMAP) ||
        (lpcidd = ciddTbl[cid].lpBitmapCIDD) == NULL ||
        (int)dwWidth <= 0 || (int)dwHeight <= 0)
        return 0;

    /* calculate bitmapinfo size: */
    setPalette = 0;
#ifdef _WIN32    
    bitmapinfoSize = sizeof(BITMAPINFOHEADER);
#else
    bitmapinfoSize = sizeof(HXBitmapInfoHeader);
#endif    
    if (lpcidd->dwFlags & _BITMASK)
        bitmapinfoSize += 3 * sizeof(ULONG32);
    else
    if ((lpcidd->dwFlags & (_FOURCC|_BITCOUNT)) == (_FOURCC|_BITCOUNT) &&
        lpcidd->dwFourCC == BI_RGB && lpcidd->dwBitCount <= 8 &&
        nColors) {
        /* check palette parameters: */
        if (lppe == NULL || nColors < 0 || nColors > 256)
            return 0;
#ifdef _WIN32        
        bitmapinfoSize += nColors * sizeof(PALETTEENTRY);
        setPalette = 1;
#endif        
    }

    /* check if we have sufficient amount of memory: */
    if (nBISize < bitmapinfoSize)
        return 0;

    /* initialize bitmapinfo structure: */
    memset((void *)lpbi, 0, bitmapinfoSize);
#ifdef _WIN32    
    lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
#else
    lpbi->bmiHeader.biSize = sizeof(HXBitmapInfoHeader);
#endif
    
    /* set image parameters: */
    lpbi->bmiHeader.biWidth = dwWidth;
    lpbi->bmiHeader.biHeight = dwHeight;
    lpbi->bmiHeader.biPlanes = 1;
    lpbi->bmiHeader.biSizeImage = ImageSize (cid, dwWidth, dwHeight);

    /* set color format: */
    SetBitmapColor (lpbi, cid);

#ifdef _WIN32
    /* set palette: */
    if (setPalette)
        SetBitmapPalette (lpbi, lppe, nColors);
#endif
    return bitmapinfoSize;  /* the number of bytes written */
}


#ifdef _WIN32
#ifndef WINCE
int SetDirectDrawColor (LPDDPIXELFORMAT lpddpf, int cid)
{
    LPCIDD lpcidd;

    /* check input parameters: */
    if (lpddpf == NULL ||
        lpddpf->dwSize < sizeof(DDPIXELFORMAT) ||
        cid < 0 || cid > NFORMATS || !(ciddTbl[cid].dwFlags & _DIRECTDRAW) ||
        (lpcidd = ciddTbl[cid].lpDirectDrawCIDD) == NULL)
        return -1;

    /* set color-format releated fields in DDPIXELFORMAT: */
    lpddpf->dwFourCC      = lpcidd->dwFourCC;
    lpddpf->dwRGBBitCount = lpcidd->dwBitCount;
    lpddpf->dwRBitMask    = lpcidd->dwBitMask[0];
    lpddpf->dwGBitMask    = lpcidd->dwBitMask[1];
    lpddpf->dwBBitMask    = lpcidd->dwBitMask[2];

    /* set control flags: */
    lpddpf->dwFlags = (lpddpf->dwFourCC == BI_RGB)? DDPF_RGB: DDPF_FOURCC;

    /* success: */
    return 0;
}
#endif
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
int GetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe)
{
    int i, n = 0;

    /* check input parameters: */
    if (lpbi != NULL && lppe != NULL &&
        lpbi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) &&
        lpbi->bmiHeader.biCompression == BI_RGB &&
        lpbi->bmiHeader.biBitCount <= 8) {

        /* get pointer to palette entries: */
        RGBQUAD *prgb = (RGBQUAD*)((BYTE*)lpbi + lpbi->bmiHeader.biSize);

        /* get number of entries to process: */
        n = lpbi->bmiHeader.biClrUsed;
        if (!n) n = 1U << lpbi->bmiHeader.biBitCount; /* !!! */

        /* a DIB color table has its colors stored BGR not RGB
         * so flip them around: */
        for (i = 0; i < n; i ++) {
            lppe[i].peRed   = prgb[i].rgbRed;
            lppe[i].peGreen = prgb[i].rgbGreen;
            lppe[i].peBlue  = prgb[i].rgbBlue;
            lppe[i].peFlags = 0;
        }
    }

    /* return # of colors extracted */
    return n;
}

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
int SetBitmapPalette (LPBITMAPINFO lpbi, LPPALETTEENTRY lppe, int nColors)
{
    int i, m, n = 0;

    /* check input parameters: */
    if (lpbi != NULL && lppe != NULL && nColors > 0 && nColors <= 256 &&
        lpbi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) &&
        lpbi->bmiHeader.biCompression == BI_RGB &&
        lpbi->bmiHeader.biBitCount <= 8) {

        /* get pointer to palette entries: */
        RGBQUAD *prgb = (RGBQUAD*)((BYTE*)lpbi + lpbi->bmiHeader.biSize);

        /* check the number of entries to copy: */
        m = 1U << lpbi->bmiHeader.biBitCount; /* !!! */
        n = nColors; if (n > m) n = m;

        /* a DIB color table has its colors stored BGR not RGB
         * so flip them around: */
        for (i = 0; i < n; i ++) {
            prgb[i].rgbRed      = lppe[i].peRed;
            prgb[i].rgbGreen    = lppe[i].peGreen;
            prgb[i].rgbBlue     = lppe[i].peBlue;
            prgb[i].rgbReserved = 0;
        }

        /* set number of palette entries copied: */
        if (i == m) i = 0;  /* !!! */
        lpbi->bmiHeader.biClrUsed = i;
        lpbi->bmiHeader.biClrImportant = i;
    }

    /* return # of colors set */
    return n;
}


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
int CheckBitmap (LPBITMAPINFO lpbi)
{
    /* check bitmap pointer & format: */
    register int cid = GetBitmapColor (lpbi);
    if (cid == CID_UNKNOWN ||
        !(ciddTbl[cid].dwFlags & _BITMAP) ||
        lpbi->bmiHeader.biWidth <= 0 ||
        lpbi->bmiHeader.biHeight <= 0 ||
        lpbi->bmiHeader.biPlanes != 1 ||
        lpbi->bmiHeader.biSizeImage != ImageSize (cid, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight))
        return 0;

    /* check if image should contain a palette: */
    if (lpbi->bmiHeader.biCompression == BI_RGB &&
        lpbi->bmiHeader.biBitCount <= 8) {
        /* check ## of palette entries: */
        unsigned int m = 1U << lpbi->bmiHeader.biBitCount;
        if (lpbi->bmiHeader.biClrUsed > m ||
            lpbi->bmiHeader.biClrImportant > lpbi->bmiHeader.biClrUsed)
            return 0;
    } else /* no palette: */
    if (lpbi->bmiHeader.biClrUsed != 0 ||
        lpbi->bmiHeader.biClrImportant != 0)
        return 0;

    /* all tests passed... */
    return 1;
}
#endif //_WIN32
/* colormap.c -- end of file */

















