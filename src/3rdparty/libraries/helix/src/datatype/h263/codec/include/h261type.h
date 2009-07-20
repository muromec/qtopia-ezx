/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h261type.h,v 1.3 2007/07/06 22:00:32 jfinnecy Exp $
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
 
#ifndef _INC_H261TYPE
#define _INC_H261TYPE   1
#include "machine.h"

//typedef signed char       S8;         // 8 bits signed
//typedef unsigned char     U8;         // 8 bits unsigned
//typedef short int         S16;        // 16 bits signed
//typedef unsigned short int U16;       // 16 bits unsigned
//typedef long int          S32;        // 32 bits signed
//typedef unsigned long int U32;        // 32 bits unsigned
//typedef unsigned char     PIXEL;      // 8 bits unsigned

/* SYMBOL - Symbol descriptor */
typedef struct {
    S8              value;
    S8              type;
} SYMBOL;

/* BS_PTR - Pointer to current position in bitstream */
typedef struct {
    U8          *byteptr;    // Points to current byte
    int          bitptr;     // Number of bits already consumed/processed in
                                // current byte, i.e., points at bit (7-bitptr)
} BS_PTR;

/* BLOCK_DESCR - Descriptor for symbols in a block */
typedef struct {
    SYMBOL      *sym;        // Points to array of run-level pairs
    int          nsym;       // Number of run-level pairs in array
    int          intraVar;   // Block variance
    int          predErr;    // Prediction error (undefined for INTRA blocks)
} BLOCK_DESCR;

/* MACROBLOCK_DESCR - 256-byte descriptor for MB, that consists of 6 blocks */
/*** NOTE! This structure MUST be 256 bytes in size for efficiency.  ***/
typedef struct {
    BLOCK_DESCR     block[6];
    BLOCK_DESCR     Bblock[6];  // Descriptors for B-blocks
    U32             codedCount; // Number of times macroblock has been coded
    int             lumaVar;    // Average of 4 luminance block variances (intraVar)
    int             dquant;     // H.263: differential value for quant
    int             quality;    // Quality index used to select "quant" and HF attenuation
    U8              mtype;
    U8              active;     // Indicates active part of picture for INTRA blocks;
                                // Undefined for other MTYPEs
    U8              quant;		// Quantizer step
    U8              cbp;		// Coded block pattern
    U8              x;          // mb_column
    U8              y;          // mb_row
    S8              mv_x;       // Hor. component of motion vector
    S8              mv_y;       // Vert. component of motion vector
    S8              mvd_x;      // Hor. motion vector difference
    S8              mvd_y;      // Vert. motion vector difference
    U8              modB;       // Mode for B-blocks; defined for PB frames only    
    U8              intra_mode; // Intra prediction mode; defined for advanced intra mode only
    S8              blkMvX[4];  // Hor. motion for 8x8 blocks; defined when mtype=INTER4V
    S8              blkMvY[4];  // Vert. motion for 8x8 blocks
    S8              blkDiffX[4]; // Hor. mv diff. for 8x8 blocks (mtype=INTER4V)
    S8              blkDiffY[4]; // Vert. mv difference for 8x8 blocks
    U8              Bquant;     // quant for B-frame
    U8              cbpB;       // Coded block pattern for B-blocks; defined if modB > 1
    S8              mvdB_x;     // Mv data for B-macroblock; defined if modB > 0
    S8              mvdB_y;
    S8              blkMvFx[4]; // Hor. motion for forward prediction of B-frame
    S8              blkMvFy[4]; // Vert. motion for forward prediction of B-frame
    S8              blkMvBx[4]; // Hor. motion for backward prediction of B-frame
    S8              blkMvBy[4]; // Vert. motion for backward prediction of B-frame
} MACROBLOCK_DESCR;

/* GOB_DESCR - Descriptor for Group Of Block */
typedef struct {
    int                     gn;         // GOB Number
    int                     next_gn;    // Next GOB Number
    int                     gquant;
    int                     gei;
    int                     num_mb;     // Number of macroblocks
    MACROBLOCK_DESCR        *mb;       // Origin of macroblock array
    int                     first_col;  // First MB in GOB
    int                     first_row;
    int                     mb_width;   // MBs per row
    int                     mb_offset;  // Row offset
    int                     num_gspare;
    S8                      *gspare;
} GOB_DESCR;

/*  ENCTABENTRY - Variable length code */
typedef struct {
    U32             code;   // Variable length code, starting in MSB; max 25 bits
    int             len;    // of bits
} ENCTABENTRY;

#define DCT_8X8_SCALE       2                       /* DCT_SCALE**2 */
#define QTABLE_MAX_MAGN     (2048*DCT_8X8_SCALE)    /* 2048 * DCT_SCALE**2 */
#define MAX_Q_INDEX         127
                     /* Largest qz output */
/* Dimensions for Qtab */
#define QUALITY_STEPS       (31)
#define MASKING_STEPS       (1)

/* Structures for table-based quantization */
typedef struct QTab {
    S8  qVal[(1+2*QTABLE_MAX_MAGN)];
} QTab;
typedef struct QTab *pQTab;
                 
typedef struct QMatrix {
    int     maxCoeff;           // last coeff to quantize [1,64]
    pQTab   qCoeffTabs[64];     // tables
} QMatrix;
typedef struct QMatrix *pQMatrix;


// typedef for region-of-interest rectangle
typedef struct {
    S32 left;
    S32 top;
    S32 right;
    S32 bottom;
} ROI_RECT;

#endif
