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

#ifndef _MPADECL2_H_
#define _MPADECL2_H_

#include "mpadec.h"

typedef void (*SBT_FUNCTION)(float *sample, unsigned char *pcm, 
                             int n, float vbuf[][512], int vb_ptr[]);
///////////////////////////////////////////////////////////////////////////////
// CMpaDecoderL2 is the layer1 MPEG audio decoder
///////////////////////////////////////////////////////////////////////////////
class CMpaDecoderL2 : public CMpaDecoder
{
public:
    CMpaDecoderL2();
    ~CMpaDecoderL2();

    //IN_OUT      audio_decode(unsigned char *bs,
    //                           unsigned char *pcm);
    
    IN_OUT      audio_decode(unsigned char *bs,
                               unsigned char *pcm,
                               int size);

    int         audio_decode_init(MPEG_HEAD *h,
                                    int framebytes_arg,
                                    int reduction_code = 0,
                                    int transform_code = 0,
                                    int convert_code = 0,
                                    int freq_limit = 24000,
                                    int conceal_enable = 0);

#ifdef REFORMAT
    // dummy
    IN_OUT  audio_decode_reformat(unsigned char *bs,
                         unsigned char *bs_out);
#endif

private:

    float look_c_value[18];          /* built by init */
    /*----------------*/
    int max_sb;
    int stereo_sb;
    /*----------------*/
    int bit_skip;
    /*----------------*/
    int nbat[4];
    int bat[4][16];
    int ballo[64];                   /* set by unpack_ba */
    unsigned int samp_dispatch[66];  /* set by unpack_ba */
    float c_value[64];               /* set by unpack_ba */
    /*----------------*/
    unsigned int sf_dispatch[66];    /* set by unpack_ba */
    float sf_table[64];
    float cs_factor[3][64];
    /*----------------*/
    float sample[2304];
    signed char  group3_table[32][3]; /* Flawfinder: ignore */
    signed char  group5_table[128][3]; /* Flawfinder: ignore */
    signed short group9_table[1024][3];
    /*----------------*/
    SBT_FUNCTION sbt;

    //--- bit getter data
    unsigned char *bs_ptr;
    unsigned int bitbuf;
    int bits;
    int bitval;

    //------ private functions ----------
    void table_init();
    // bit getter
    void load_init( unsigned char *buf);
    int  load( int n);
    void skip( int n);

    void unpack_ba();
    void unpack_sfs(); 
    void unpack_sf();

    void unpack_samp();    /* unpack samples */


};
#endif      // _MPADECL2_H_
