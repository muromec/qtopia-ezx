/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h261defs.h,v 1.3 2005/06/15 01:48:43 rggammon Exp $
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

#ifndef _INC_H261DEFS
#define _INC_H261DEFS   1

#include "machine.h"

// Set up endianness.  We assume Mac is big endian, we assume 
// Windows is little endian and we assume Unix must specify
// endianness externally
#if defined(FOR_MAC)
#define BIG_ENDIAN
#elif !defined(FOR_UNIX)
#define LITTLE_ENDIAN           // Intel X86
#else
#if defined(_LITTLE_ENDIAN) && !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif
#if defined(_BIG_ENDIAN) && !defined(BIG_ENDIAN)
#define BIG_ENDIAN
#endif
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#error Unix systems must define LITTLE_ENDIAN or BIG_ENDIAN in its makefile
#endif
#endif	


#define YES                 (1)
#define NO                  (0)
#define OK                  (1)
#define H261_ERROR          (-1)    // Temporary fix

#ifndef ERROR
#define ERROR               (-1)
#endif
// Replace the previous three lines with the following once ERROR has been replaced by H261_ERROR everywhere
//#undef ERROR

// Coding method
#define H261_CODING         (0)
#define H263_CODING         (1)
 
// Resolution 
#define SQCIF               (2)
#define CIF                 (1)
#define QCIF                (0)
#define	CIF4				(3)
#define	CIF16				(4)
#define	ANYSIZE				(5)


// Decoder's non-standard output DIB types.
// BI_RGB is defined in windows.h
#define VIVO_RGB555 	(0)
#define VIVO_RGB565		(1)
#define VIVO_RGB888		(2)
#define	VIVO_YUV9		(3)		//YUV 4:1:1 Format - Intel YUV9
#define VIVO_YUV16		(4)		//YUV 4:2:2 Format
#define VIVO_YUV32		(5)
#define	VIVO_YVU9		(6)		// YVU Planar
#define VIVO_YUY2		(7)		// YUYV interleaved
#define VIVO_YUV12		(8)		// yuv 420

// Masks for Temporal Reference
#define H263_TR_MASK    (0xff)  // Mask off 8 LSBs
#define H261_TR_MASK    (0x1f)  // Mask off 5 LSBs
#define TRB_MASK    (0x7)   // Mask off 3 LSBs

// Define bits in PTYPE (13 bits) for H.263
#define PTYPE263_BLANK1             (0x1000)    // Bit 1 always "1".
#define PTYPE263_BLANK2             (0x000)     // Bit 2 always "0".
#define PTYPE263_SPLITSCREEN        (0x400)     // Bit 3: "0"=split screen off, "1"=on.
#define PTYPE263_DOCCAMERA          (0x200)     // Bit 4: "0"=document camera off, "1"=on.
#define PTYPE263_FP_RELEASE         (0x100)     // Bit 5: "0"=Freeze Picture Release off, "1"=on.
#define PTYPE263_RESERVED           (0xc0)      // Bits 6-8 are source format:"111" reserved
#define PTYPE263_EPTYPE             (0xe0)      // "110" = Extended PTYPE (reserved in H.263)
#define PTYPE263_16CIF              (0xa0)      // "101" = 16CIF
#define PTYPE263_4CIF               (0x80)      // "100" = 4CIF
#define PTYPE263_CIF                (0x60)      // "011" = CIF
#define PTYPE263_QCIF               (0x40)      // "010" = QCIF
#define PTYPE263_SQCIF              (0x20)      // "001" = sub-QCIF
#define PTYPE263_INTER              (0x10)      // Bit 9: "0"=INTRA picture, "1"=INTER.
#define PTYPE263_UNRESTRICTED_MV    (0x8)       // Bit 10: "0"=unrestricted MV mode off, "1"=on.
#define PTYPE263_SYNTAX_BASED_AC    (0x4)       // Bit 11: "0"=arithmetic coding off, "1"=on.
#define PTYPE263_ADVANCED_PRED      (0x2)       // Bit 12: "0"=advanced pred off, "1"=on.
#define PTYPE263_PB_FRAME           (0x1)       // Bit 13: "0"=normal I-orP picture, "1"=PB frame.

// Define bits in PTYPE
#define FLAG_SPLITSCREEN    (0x20)
#define FLAG_DOCCAMERA      (0x10)
#define FLAG_FP_RELEASE     (0x8)
#define FLAG_CIF_FORMAT     (0x4)
#define FLAG_HI_RES         (0x2)   // 0 = on, 1 = off
#define FLAG_SPARE          (0x1)

// Macroblock types
#define MTYPE_SKIP          (0)
#define MTYPE_INTRA         (1)
#define MTYPE_INTRA_MQUANT  (2)
#define MTYPE_INTER         (3)
#define MTYPE_INTER_MQUANT  (4)
#define MTYPE_MC_NOCBP      (5)
#define MTYPE_MC_CBP        (6)
#define MTYPE_MC_MQUANT     (7)
#define MTYPE_MCFILT_NOCBP  (8)
#define MTYPE_MCFILT_CBP    (9)
#define MTYPE_MCFILT_MQUANT (10)
#define MTYPE_MIN           (0)
#define MTYPE_MAX           (10)

// H.263 macroblock types
#define MTYPE263_INTER      (3 << 2)
#define MTYPE263_INTER_Q    (4 << 2)
#define MTYPE263_INTER4V    (5 << 2)
#define MTYPE263_INTRA      (6 << 2)
#define MTYPE263_INTRA_Q    (7 << 2)
#define MTYPE263_MIN        (3 << 2)
#define MTYPE263_MAX        (7 << 2)
#define MTYPE263_INTRA_MIN  (6 << 2)
#define MTYPE263_INTRA_MAX  (7 << 2)
#define MTYPE263_STUFFING   (8 << 2)
#define MTYPE263_STARTCODE  (9 << 2)

/* Symbol types */
#define SYM_EXIT            (-1)
#define SYM_ESCAPE          (-2)
#define SYM_EOB             (-64)
#define SYM_INTRA_DC        (-65)
#define SYM_MBA             (-66)
#define SYM_STARTCODE       (-67)
#define SYM_MBA_STUFFING    (-68)
#define SYM_MTYPE           (-69)
#define SYM_MVD             (-70)
#define SYM_CBP             (-71)
#define SYM_QUANT_TR        (-72)
#define SYM_GEI_PEI         (-73)
#define SYM_SPARE           (-74)
#define SYM_GN              (-75)
#define SYM_PTYPE           (-76)
#define SYM_ESC_RUN         (-77)
#define SYM_ESC_LEVEL       (-78)
// Symbol types for H.263
#define SYM_MCBPC           (-79)
#define SYM_MCBPC_STUFFING  (-80)
#define SYM_MODB            (-81)
#define SYM_CBPY            (-82)
#define SYM_DQUANT          (-83)
// Symbol types for H.263+
#define SYM_INTRA_MODE      (-84)

// Limits for Macroblock address
#define MBA_MIN             (1)
#define MBA_MAX             (33)

// Limits for motion vectors
#define MV_MIN          (-15)
#define MV_MAX          15

// Limits for motion vector differences; larger differences (up to +/- 30)
// are represented as MVD +/- MV_WRAP
#define MVD_MIN         (-16)
#define MVD_MAX         (15)
#define MV_WRAP         (32)
#define MVD263_MIN      (-32)
#define MVD263_MAX      (31)
#define MV263_WRAP      (64)
// Limits for motion vectors in UMV mode (1 fraactional bit)
#define UMV_MIN         (-63)
#define UMV_MAX         (63)
#define UMV_NEG_THRESH  (-31)
#define UMV_POS_THRESH  (32)

// Limits for Coded block Pattern
#define CBP_MIN             (0)
#define CBP_MAX             (63)
#define CBPY_MIN            (0)
#define CBPY_MAX            (15)

// Limits for QUANT
#define QUANT_MIN           (1)
#define QUANT_MAX           (31)
#define DQUANT_MIN          (-2)
#define DQUANT_MAX          (2)
#define DBQUANT_MIN         (0)
#define DBQUANT_MAX         (3)

#define N_SYM_INDICES       256     // Number of "level" symbols
#define FORCED_UPDATE       (33)    // Forced update interval
#define FORCED_THRESH       (50)   // Forced update threshold


/* BCH defines  */
#define BCH_WORDS_PER_BLK       16  // Each block is 16 words * 32 b/word = 512 bits
#define BCH_BYTES_PER_BLK       (4 * BCH_WORDS_PER_BLK)
#define BCH_INFO_BITS           492
#define BCH_PARITY_BITS         18
#define BCH_FRAMING_BIT         0x80
#define BCH_FILL_INDICATOR      0x40
#define BCH_INFO_BITS_1ST_BYTE  6
#define BCH_INFO_BYTES          ((BCH_INFO_BITS + 7 + 8 - BCH_INFO_BITS_1ST_BYTE) / 8)
#define BCH_PARBITS_LAST_INFO_BYTE  (BCH_PARITY_BITS % 8)
#define BCH_LAST_INFO_MASK      (0xfc)  // Mask out bits 7 to 2
#define BCH_G_POLY              0x25724000L
#define BCH_FRAME_PATTERN       (0x1b)  // "0001 1011"

/* From vlc.h 10/29/93 md   */
#define U32_BITS            (32)
#define MAX_CODELENGTH      (U32_BITS - 7)
#define MAX_STRING_VLC      (MAX_CODELENGTH + 7)

#define N_RL_PAIRS          (128)
#define RUN_DIM             (64)
#define N_TCOEFF            (N_RL_PAIRS + RUN_DIM)
#define N_RL263_PAIRS       (116)
#define N_TCOEF263          (N_RL263_PAIRS + RUN_DIM)
#define N_LAST_RL263_PAIRS  (88)
#define N_LAST_TCOEF263     (N_LAST_RL263_PAIRS + RUN_DIM)

#define LEVEL_DIM           (256)
#define DIM_DCT_FIRST       (2)

// Defines for DCT type
#define DCT_TYPE_8x8            0
#define DCT_TYPE_4x4            1
#define DCT_TYPE_MAHKEENAC      2
#define DCT_TYPE_NONE           (-1)
#define QUANT_8x8_COEFFS        64
#define QUANT_4x4_COEFFS        10
#define QUANT_MAHKEENAC_COEFFS  3

// Enumerate blocks for H.263 Advanced Prediction mode (4 motion vectors per MB)
#define UPPER_LEFT_BLK          (0)
#define UPPER_RIGHT_BLK         (1)
#define LOWER_LEFT_BLK          (2)
#define LOWER_RIGHT_BLK         (3)
#define WHOLE_MACROBLOCK        (4)

#endif
