/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvenc.h,v 1.3 2007/07/06 22:00:32 jfinnecy Exp $
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

#ifndef _INC_HVENC
#define _INC_HVENC      1

#include "machine.h"
#include "h261type.h" 

//#define VvDbgMsg( msg )       VvDbg( msg )
#define VvDbgMsg( msg )
extern void VvDbg ( char *msg );
 
/* Temporal filter function id.s        */
enum { Identity, NonLinear };

/* MbInfo */
typedef struct {
    S16         predBits;               // Number of predicted bits per macroblock
    S16         actYBits;               // Number of actual bits per macroblock (Y component)
    S16         actCbBits;              // Number of actual bits per macroblock (Cb component)
    S16         actCrBits;              // Number of actual bits per macroblock (Cr component)
    S32         mse;                    // mse or var measure
    S16         qStep;                  // Quantizer step used
    S16         codingType;             // Intra, Inter, etc.
    S16         cFlag;                  // True if any luma blocks to be coded
    S16         dummy;
} MbInfo;
 
/* FrameInfo */
typedef struct {
    S32         logSum;                 // Sum of variance or mse for coded blocks
    S32         codedBlkCnt;            // Number of macroblocks to be coded
    S32         qTargetBits;            // Goal Bits for q-step selection
} FrameInfo; 

/* COMPONENT - Image component descriptor */
typedef struct {
    S32         nhor;           // Active pels per row
    S32         nvert;          // Number of rows
    S32         hoffset;        // Bytes between rows i.e. active + non-active
    S32         pOffset;        // Bytes between adjacent pels. If this is not 0 or 1,
                                // then the array is not packed. This is here to handle
                                // pictures at the application level. 
    U32         ptrAlias;       // 16:16 alias to ptr (decoder only)
    PIXEL       *ptr;    // Data
} COMPONENT;

/* PICTURE - Frame descriptor */
typedef struct {
    S32             color;          // 1 = Color is valid, 0 = luma only
    S32             vXoffset;       // X offset for placing video data 0,8,16..
    S32             vYoffset;       // Y offset for placing video data
    S32             xSize   ;       // Active pels per row
    S32             ySize;          // Active rows
    S16             picLayout;      // Picture layout e.g. S422
    S16             dummy;          // Dummy for alignment
    COMPONENT       y;
    COMPONENT       cb;
    COMPONENT       cr;
    TIME32          tTimestamp;     // Timestamp
} PICTURE;

/* PICTURE_DESCR - Descriptor for Picture layer information */
#define MAX_PEI_COUNT       4
typedef struct {
    S32             codingMethod;   // 0 = H.261, 1 = H.263
    S32             splitscreen;
    S32             doccamera;
    S32             fp_release;
    S32             format;
    S32             rows;
    S32             cols;
    S32             hi_res;
    S32             tr;
    S32             trPrev;         // Temp Reference for prev. P-frame
    S32             ptype;
    S32             interFrame;     // 0 = INTRA picture, otherwise INTER picture
    S32             unrestrictedMv; // 0 = off, otherwise on
    S32             advancedPred;   // 0 = off, otherwise on
    S32             syntax_basedAC; // 0 = off, otherwise on
    S32             PBframeMode;    // 0 = off, otherwise on
    S32             reducedResUpdate;// 0 = off, otherwise QUANT for Reduced-res. Update mode
    S32             deblockingFilterMode;// 0 = off, otherwise on
    S32             advancedIntraMode;// 0 = off, otherwise on
    S32             tempRefBframe;  // Present if PB-frame; add these 3 bits to TR for
                                    // prev. P-frame to get TR for B-frame
    S32             dbQuant;        // quantization info for B-pictures (2 bits)
    S32             peiCount;
    S32             cpm;            // continuous presence multipoint.
    S32             decodingInProgress; // Set to FALSE if feeding new bitstream to decoder
                                    // VvDecode will set it to TRUE if decoder "timed out",
                                    // i.e., didn't finish decoding.  VvDecode needs to be
                                    // called repeatedly until returning FALSE
    U8              pSpare[MAX_PEI_COUNT]; // ;;;
} PICTURE_DESCR;


/* Temporary definition */
typedef struct {
    long    rows;               // Number of rows in a frame    
    long    cols;               // Number of columns in a frame. For Spigot this should
                                // include y,u,v  e.g. 320 for 1/4
    short   topSkip;            // Number of lines to skip on top of video
    short   botSkip;            // Number of lines to skip on bottom of video.
    short   vidType;            // Indicates video capture board type.
    short   dummy;
    S32     bNewDIB;
    S32     xCur;
    S32     yCur;
    U32     histVal;            // Xth percentile (X is specified by H261EncoderParams.percentile).
    S32     histCount;          // Number of samples in lumaHist.
    S32     lumaHist[256];      // Holds histogram of raw luma data
    unsigned char *dataPtr1; // Points to start of pixel data (not S422 marker)
    unsigned char *dataPtr; // Points to start of pixel data (not S422 marker)
} VIDEOFrame;

/* Length of statistics arrays */
#define NUM_TIMERS          7
#define NUM_DCT_STATS       4


/* Label time array */
#define TFLT        0
#define IVAR        1
#define MEST        2
#define PSEL        3
#define QSEL        4
#define DCTC        5
#define VLCC        6

/* Label DCT stats array */
#define DCT_STAT_SKIPPED    0
#define DCT_STAT_ZERO       1
#define DCT_STAT_1ST_THREE  2
#define DCT_STAT_MORE       3

/* Macroblock "map" types and definition */
#define DECODER_MAP     1   // decMB.type 
#define MEST_MAP_X      2   // Motion estimation map type
#define MEST_MAP_Y      3   // Motion estimation map type

typedef struct {
    S32     format;         // QCIF of CIF. Set by user.   
    S32     type;           // 1 = decoder map (others TBD). Set by user.
    U8      data[396];      // Array of data. 1 byte per macroblock. Declare
                            // for CIF and use subset for QCIF.  
} MBMap;
 
/*
 * bit index -- 16:16 byte offset:bit number 
 */
typedef union VvBitIndex *lpVvBitIndex;
typedef union VvBitIndex {
    U32 l;
    struct ww {
        U16 bit;
        U16 byte;
    } ww;
} VvBitIndex;

/* Decoder statistics */
typedef struct VvDecoderStats *lpVvDecoderStats;
typedef struct VvDecoderStats {
    S32 dwStartcodes;
    S32 dwGobs;
    S32 dwBadgobs;
    S32 dwFirstBadgob;
    S32 dwUpdateGobs;
    S32 dwIntraFrame;
    S32 dwPsc;
    VvBitIndex pscIndex;    /* picture start code index */
    S32 dwBframe;           /* Report whether B-frame is returned from VvDecode */
} VvDecoderStats;

/* Possible video board vendors types */ 
enum { S422, YVU9, VLV3, VLV4, DIB };   // S422 = Video Spigot,  YVU9 = Intel Smart Recorder
                                        // VLV3 = Captivator 4:1:1, VLV4 = Captivator Mono
                                        // DIB = Device Independent Bitmap

/* Misc defines*/
#ifndef TRUE
#define FALSE                       0
#define TRUE                        1
#endif

#define NO_ERROR                    0L
#define UNKNOWN_VIDTYPE             1
#define VIDTYPE_NOT_SUPPORTED_YET   2
//#define MAX_BITSTR_SIZE             32768
#define MAX_BITSTR_SIZE             49512

/* VvOpenEncoder and VvCloseEncoder     defines */
#define MAX_NUM_ENCODERS    2

#ifndef AP_VIDEORESIZING
#define MAX_NUM_DECODERS    2
#else
#define MAX_NUM_DECODERS    20
#endif

/* VvOpenEncoder and VvCloseEncoder     error codes */
#define MAX_ENCODERS_OPEN       MAX_NUM_ENCODERS + 10
#define FAILED_MALLOC           MAX_NUM_ENCODERS + 11
#define NO_ENCODERS_OPEN        MAX_NUM_ENCODERS + 12
#define MAX_DECODERS_OPEN       MAX_NUM_ENCODERS + 13
#define NO_DECODERS_OPEN        MAX_NUM_ENCODERS + 14
#define UNKNOWN_PICTURE_FORMAT  MAX_NUM_ENCODERS + 15
        
#endif
