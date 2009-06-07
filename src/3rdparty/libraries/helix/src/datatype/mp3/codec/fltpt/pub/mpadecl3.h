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

#ifndef _MPADECl3_H_
#define _MPADECL3_H_

#include "mpadec.h"
#include "sconceal.h"   // error concealment (for lost frames)

typedef void (*SBT_FUNCTION_L3)(float *sample, unsigned char *pcm, 
                             int ch, float vbuf[][512], int vb_ptr[]);
#define NBUF (8*1024)
#define BUF_TRIGGER (NBUF-1500)


///////////////////////////////////////////////////////////////////////////////
// CMpaDecoderL3 is the layer1 MPEG audio decoder
///////////////////////////////////////////////////////////////////////////////
class CMpaDecoderL3 : public CMpaDecoder
{
public:
    CMpaDecoderL3();
    ~CMpaDecoderL3();

    // initialize Layer3 audio decoder
    // conceal_enable determines missing frame handling
    // for reformatted frames
    //   if conceal_enable != 0, conceal by Wolfgang's method
    //   if conceal_enable == 0, handle by null spectrum
    int         audio_decode_init(MPEG_HEAD *h,
                                 int framebytes_arg,
                                 int reduction_code = 0,
                                 int transform_code = 0,
                                 int convert_code = 0,
                                 int freq_limit = 24000,
                                 int conceal_enable = 0);

    // decode standard Layer3 frame
    // no error concealment
    IN_OUT      audio_decode(unsigned char *bs,
                             unsigned char *pcm);

    // decode reformatted frames (self contained, main_data_begin=0)
    // call fails if main_data_begin != 0
    // optional error concealment for missing frames
    // missing frame indicated to decoder by size <= 0 
    //   if conceal_enable != 0, conceal by Wolfgang's method
    //   if conceal_enable == 0, handle by null spectrum
    //     
    IN_OUT      audio_decode(unsigned char *bs,
                             unsigned char *pcm,
                             int size);


#ifdef REFORMAT
    // test code, reformats to self contained frame, main_data_begin = 0
    IN_OUT audio_decode_reformat(unsigned char *bs, unsigned char *bs_out);
private:
    unsigned char reformat_buf[2400]; /* Flawfinder: ignore */
    int reformat_bytes;
    int reformat_side_bytes;
#endif


private:    
    IN_OUT L3audio_decode_MPEG1(unsigned char *bs, 
                                unsigned char *pcm);
    IN_OUT L3audio_decode_MPEG2(unsigned char *bs, 
                                unsigned char *pcm); 

    void Xform(unsigned char *pcm, int igr);
    void Xform_mono(unsigned char *pcm, int igr);
    void Xform_dual(unsigned char *pcm, int igr);
    void Xform_dual_mono(unsigned char *pcm, int igr);
    void Xform_dual_right(unsigned char *pcm, int igr);

    int unpack_side_MPEG1();
    int unpack_side_MPEG2(int igr);
//    void unpack_main(unsigned char *buf, unsigned char *pcm, int igr);
    void unpack_main(unsigned char *buf, int igr);

    /*-- short portion is 3*x !! --*/
    int     nBand[2][22]; /* [long/short][cb] */
    int     sfBandIndex[2][22]; /* [long/short][cb] */
	BITDAT bitdat;			/* JR - new (no longer C global) */
	float re_buf[192][3];	/* JR - new (no longer C global) */

    /*------------*/
    int mpeg25_flag;
    int stereo_flag;
    int igr;    
    int band_limit;
    int band_limit21;   // limit for sf band 21
    int band_limit12;   // limit for sf band 12 short
    int band_limit_nsb;  /* copy to global for hybrid */
    int gain_adjust;     /* adjust gain e.g. cvt to mono */
    int id;
    int ncbl_mixed;   /* number of long cb's in mixed block 8 or 6 */
    int half_outbytes;
    unsigned int zero_level_pcm;
    /* cb_info[igr][ch], compute by dequant, used by joint */
    CB_INFO cb_info[2][2]; 
    IS_SF_INFO is_sf_info;   /* MPEG-2 intensity stereo */
    /*---------------------------------*/
    /* main data bit buffer */
    unsigned char buf[NBUF]; /* Flawfinder: ignore */
    int buf_ptr0;
    int buf_ptr1;
    int main_pos_bit;
    /*---------------------------------*/
    SIDE_INFO side_info;

    SCALEFACT sf[2][2];   /* [gr][ch] */

    int nsamp[2][2];   /* must start = 0, for nsamp[igr_prev] */

    float yout[576];     /* hybrid out, sbt in */

    /* sample buffer */
    /*- sample union of int/float  sample[ch][gr][576] */
    /* static SAMPLE sample[2][2][576]; */
    SAMPLE sample[2][2][576];

    int iXform;

    SBT_FUNCTION_L3 sbt_L3;

//======= concealment =========
    int conceal_flag;
    CConcealment *conceal[2];

    IN_OUT L3audio_decode_reformattedMPEG1(unsigned char *bs, 
                           unsigned char *pcm, int size);
    IN_OUT L3audio_decode_reformattedMPEG2(unsigned char *bs, 
                           unsigned char *pcm, int size);
    
    void conceal_insert(int igr);
    void conceal_fixup(int igr);

#ifdef REFORMAT
    // test code,  reformats frame
    IN_OUT L3reformat_MPEG1(unsigned char *bs, unsigned char *bs_out);
    // test code,  reformats frame
    IN_OUT L3reformat_MPEG2(unsigned char *bs, unsigned char *bs_out);
#endif

};


#endif
