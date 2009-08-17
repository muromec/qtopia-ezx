/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: machine.h,v 1.7 2005/03/14 19:24:47 bobclark Exp $
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

/*-----------------------------------------------------------------------------
 *  machine.h
 *
 *  NAME 
 *   Machine-dependent definitions.
 *   WIN16
 *   __WINDOWS_386__, VXD
 *   FLAT_OS
 *   _NTWIN
 *   _OS2
 *
 *  SYNOPSIS
 *    #include "machine.h"
 *     ...
 *
 *  DESCRIPTION   
 *
 -----------------------------------------------------------------------------*/ 

#ifndef _MACHINE_H_
#define _MACHINE_H_

#ifndef _OS_H_
#include "hxtypes.h"
#include "os.h" //We need it so can tell ifdef WIN16
#endif

#define VvDELETE(OBJ) if (OBJ) { delete(OBJ); OBJ=0; }

// define the VvAssert() macro as appropriate for each platform
#ifdef _DEBUG

#ifdef FOR_MAC
#include <Assert.h>
#define VvAssert(expr)	Assert_(expr)
#else		// other platforms go here
#define VvAssert(expr)
#endif

#else	// #ifdef _DEBUG

#define VvAssert(expr)

#endif	// #ifdef _DEBUG

#ifndef WIN32
#define mmioFOURCC(a,b,c,d) (((U32)((U8)(d) << 24)) | ((U8)(c) << 16) | ((U8)(b) << 8) | ((U8)(a)))
#endif

#ifdef FOR_MAC
#ifdef _MAC_MACHO
#define PATH_SEPARATOR "/"
#else
#define PATH_SEPARATOR ":"
#endif
#endif
#ifdef FOR_WINDOWS
#define PATH_SEPARATOR "\\"
#endif
#ifdef FOR_UNIX
#define PATH_SEPARATOR "/"
#endif

#ifdef FOR_MAC
// Define basic Windows datatypes here
//#define HXBOOL Boolean
#include "../../../common/include/hxtypes.h"

typedef unsigned char * LPBYTE;
#endif

#ifdef FOR_UNIX
// Define basic Windows datatypes here
#define HXBOOL unsigned char

#ifndef NULL
#define NULL (0L)
#endif

#ifndef pascal
#define pascal
#endif

#endif

/* numeric types */
typedef signed char S8;
typedef unsigned char U8;

typedef short int S16;
typedef unsigned short int U16;

typedef long int S32;
typedef unsigned long int U32;
typedef float	FLOAT;

#ifdef FOR_MAC
typedef unsigned short WORD;
//typedef S8 BYTE;
typedef unsigned long DWORD;
typedef S32 LONG;
//typedef unsigned long TIME32;
#endif

typedef S16 Word16;     // used by G723 audio codec
typedef S32 Word32;     // used by G723 audio codec

/* graphic types */
typedef U8       PIXEL; 

/* pointer types (here lie dragons!) */ 

/* Boolean types */
typedef U16 B16;

#define NATURAL_ALIGN 16
#define ALIGN(len,align) ((len)+(align)-1) & (~(align-1)) 

/* Metric types */
typedef U32 TIME32;         /* absolute time */
typedef S32 INTERVAL32;     /* interval time */
                            

#if defined _WIN32
#define FLAT_OS
#elif defined FOR_MAC
#define FLAT_OS
#elif defined FOR_UNIX
#define FLAT_OS
#else
        #HEY, unknown environment type

#endif

#if defined(FOR_MAC) || defined(FOR_UNIX)

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD *LPRGBQUAD;

#endif // MAC / Unix

#endif // _MACHINE_H_
