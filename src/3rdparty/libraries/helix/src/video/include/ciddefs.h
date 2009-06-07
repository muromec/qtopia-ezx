/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ciddefs.h,v 1.2 2004/07/09 18:36:01 hubbe Exp $
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

#ifndef _CIDDEFS_H_
#define _CIDDEFS_H_

// $Private:

/* CCIR 601 YCrCb format: */
#ifndef CID_I420
#define CID_I420        0   /* planar YCrCb 4:2:0 format (CCIR)     */
/* DirectDraw YUV formats: */
#define CID_YV12        1   /* planar YVU 4:2:0 (ATI)               */
#define CID_YVU9        2   /* planar YVU 16:2:0 (Intel,ATI)        */
#define CID_YUY2        3   /* packed YVU 4:2:2 (ATI,S3,etc.)       */
#define CID_UYVY        4   /* yet another packed 4:2:2 (3Dfx)      */
/* Windows RGB formats: */
#define CID_RGB32       5   /* 32-bit RGB                           */
#define CID_RGB24       6   /* 24-bit RGB                           */
#define CID_RGB565      7   /* 16-bit RGB 565                       */
#define CID_RGB555      8   /* 16-bit RGB 555                       */
#define CID_RGB8        9   /* 8-bit paletized RGB                  */
#define CID_XING        10  /* XING Format			    */
#define CID_ARGB32      11  /* RGB32 with Alpha Channel.            */
#define CID_YUVA        12  /* planar YCrCbA 4:2:0                  */    
#define CID_YUVU        13  /* yet another packed 4:2:2	(signed u&v)*/

/* reserved ID for unknown/bad formats: */
#define CID_UNKNOWN     14  /* any other format                     */
/* X-server B/R flipped RGB formats: */
#define CID_BGR32       15  /* 32-bit BGR (0x00BBGGRR)              */
#define CID_BGR24       16  /* 24-bit BGR                           */

/* QuickTime byte swapped formats: */
#define CID_RGB32S      17  /* 32-bit RGB byte swapped              */

/* Embeded device formats: */
#define CID_RGB444      18  /* 12-bit RGB in 16-bit array           */

/* MPEG-2 pre-decoded formats: */
#define CID_MC12        19  /* ATI Rage-Pro MPEG-2 M/C surface      */
#define CID_MCAM        20  /* ATI Rage-128 MPEG-2 IDCT&M/C surface */
#define CID_MCS3        21  /* S3 Savage3D+ MPEG-2 M/C engine       */
#define CID_IGOR        22  /* Intel i810 'Whitney' M/C interface   */
#define CID_DVPF        23

/* the number of formats: */
#define NFORMATS        (CID_DVPF+1)
#endif


// $EndPrivate.

#endif

