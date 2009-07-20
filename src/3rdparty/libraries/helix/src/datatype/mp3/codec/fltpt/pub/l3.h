/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _L3_H_
#define _L3_H_



/****  L3.h  ***************************************************

  Layer III structures

  *** Layer III is 32 bit only          ***
  *** Layer III code assumes 32 bit int ***

Copyright 1996-97 Xing Technology Corp.
******************************************************************/

#define GLOBAL_GAIN_SCALE (4*15)
/* #define GLOBAL_GAIN_SCALE 0 */


#if defined _M_IX86 || defined __alpha__ || defined __i386__ || defined __i386 || defined i386 || defined _LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#else
#define LITTLE_ENDIAN 0
#endif

#ifndef LITTLE_ENDIAN
#error Layer III LITTLE_ENDIAN must be defined 0 or 1
#endif

/*-----------------------------------------------------------*/
/*---- huffman lookup tables ---*/
/* endian dependent !!! */
#if LITTLE_ENDIAN
    typedef union {
        int ptr;
        struct {
            unsigned char signbits;
            unsigned char x;
            unsigned char y;
            unsigned char purgebits;   // 0 = esc
        } b;
    } HUFF_ELEMENT;
#else           /* big endian machines */
    typedef union {
        int ptr;        /* int must be 32 bits or more */
        struct {
            unsigned char purgebits;   // 0 = esc
            unsigned char y;
            unsigned char x;
            unsigned char signbits;
        } b;
    } HUFF_ELEMENT;
#endif
/*--------------------------------------------------------------*/
typedef struct {
    unsigned int bitbuf;
    int bits;
    unsigned char *bs_ptr;
    unsigned char *bs_ptr0;
    unsigned char *bs_ptr_end;      // optional for overrun test
} BITDAT;

/*-- side info ---*/
typedef struct {
    int part2_3_length;
    int big_values;
    int global_gain;
    int scalefac_compress;
    int window_switching_flag;
    int block_type;
    int mixed_block_flag;
    int table_select[3];
    int subblock_gain[3];
    int region0_count;
    int region1_count;
    int preflag;
    int scalefac_scale;
    int count1table_select;
}   GR;
typedef struct {
    int mode;
    int mode_ext;
    /*---------------*/
    int main_data_begin;    /* beginning, not end, my spec wrong */
    int private_bits;
    /*---------------*/
    int scfsi[2];          /* 4 bit flags [ch] */
    GR  gr[2][2];          /* [gran][ch] */
}   SIDE_INFO;
/*-----------------------------------------------------------*/
/*-- scale factors ---*/
// check dimensions - need 21 long, 3*12 short
// plus extra for implicit sf=0 above highest cb
typedef struct {
	int l[23];            /* [cb] */
	int s[3][13];         /* [window][cb] */
	} SCALEFACT;  
/*-----------------------------------------------------------*/
typedef struct {
    int cbtype;     /* long=0 short=1 */
    int cbmax;      /* max crit band */
    int lb_type;    /* long block type 0 1 3 */
    int cbs0;       /* short band start index 0 3 12 (12=no shorts */
    int ncbl;       /* number long cb's 0 8 21 */
    int cbmax_s[3]; /* cbmax by individual short blocks */
} CB_INFO;
/*-----------------------------------------------------------*/
/* scale factor infor for MPEG2 intensity stereo  */
typedef struct {
    int nr[3];
    int slen[3];
    int intensity_scale;
} IS_SF_INFO;       
/*-----------------------------------------------------------*/
typedef union {
    int s;
    float x;
} SAMPLE;
/*-----------------------------------------------------------*/


#endif //_L3_H_
