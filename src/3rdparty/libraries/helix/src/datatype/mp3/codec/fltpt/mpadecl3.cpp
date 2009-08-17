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

#include "statname.h"

#include "hlxclib/string.h"
#include "mhead.h"
#include "mpadecl3.h"

#define min(a,b)  ((((a)>=(b))?(b):(a))) 

#include "mpalowl3.h"     // low level extern C prototypes
///////////////////////////////////////////////////////////////////////////////
// Static Data:
///////////////////////////////////////////////////////////////////////////////
static const int mp_sr20_table_L3[2][4]={441,480,320,-999, 882,960,640,-999};
static const int mp_br_table_L3[2][16]=
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0,    /* mpeg 2 */
     0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};

static const int sr_table_L3[8] =
    { 22050, 24000, 16000, 1,
      44100, 48000, 32000, 1 };

// shorts used to save a few bytes
static const struct  {
short l[23];
short s[14];} sfBandIndexTable[3][3] =   {
/* mpeg-2 */
{
{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,4,8,12,18,24,32,42,56,74,100,132,174,192}},
{{0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,332,394,464,540,576},
 {0,4,8,12,18,26,36,48,62,80,104,136,180,192}},
{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,4,8,12,18,26,36,48,62,80,104,134,174,192}},
},
/* mpeg-1 */
{
{{0,4,8,12,16,20,24,30,36,44,52,62,74,90,110,134,162,196,238,288,342,418,576},
 {0,4,8,12,16,22,30,40,52,66,84,106,136,192}},
{{0,4,8,12,16,20,24,30,36,42,50,60,72,88,106,128,156,190,230,276,330,384,576},
 {0,4,8,12,16,22,28,38,50,64,80,100,126,192}},
{{0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
 {0,4,8,12,16,22,30,42,58,78,104,138,180,192}}
},

/* mpeg-2.5, 11 & 12 KHz seem ok, 8 ok */
{
{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,4,8,12,18,26,36,48,62,80,104,134,174,192}},
{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,4,8,12,18,26,36,48,62,80,104,134,174,192}},
// this 8khz table, and only 8khz, from mpeg123)
{{0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576},
 {0,8,16,24,36,52,72,96,124,160,162,164,166,192}},
},
};


static const SBT_FUNCTION_L3  sbt_table_L3[2][3][2] = {
    sbt_mono_L3,
    sbt_dual_L3,
#ifdef REDUCTION
    sbt16_mono_L3,
    sbt16_dual_L3,
    sbt8_mono_L3,
    sbt8_dual_L3,
#else
  NULL, NULL, NULL, NULL,
#endif

#ifdef EIGHT_BIT  
/*-- 8 bit output -*/
    sbtB_mono_L3,
    sbtB_dual_L3,
#ifdef REDUCTION
    sbtB16_mono_L3,
    sbtB16_dual_L3,
    sbtB8_mono_L3,
    sbtB8_dual_L3,
#else 
  NULL, NULL, NULL, NULL,
#endif
#endif
};
///////////////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////////////
CMpaDecoderL3::CMpaDecoderL3()
 :  CMpaDecoder(),
    mpeg25_flag(0),
    stereo_flag(0),
    igr(0),
    band_limit(576),
    band_limit21(576),
    band_limit12(576),
    band_limit_nsb(32),
    gain_adjust(0),
    id(1),
    ncbl_mixed(0),
    half_outbytes(0),
    zero_level_pcm(0),
    buf_ptr0(0),
    buf_ptr1(0),
    main_pos_bit(0),
#ifdef REFORMAT
    reformat_bytes(0),
    reformat_side_bytes(0),
#endif
    conceal_flag(0)

{
    // memsets not really needed
    //memset(samp_save, 0, sizeof(samp_save));
    memset(nBand, 0, sizeof(nBand));
    memset(sfBandIndex, 0, sizeof(sfBandIndex));
    memset(cb_info, 0, sizeof(cb_info));
    memset(&is_sf_info, 0, sizeof(is_sf_info));
    memset(buf, 0, sizeof(buf));
    memset(&side_info, 0, sizeof(side_info));
    memset(sf, 0, sizeof(sf));
    memset(nsamp, 0, sizeof(nsamp));
    memset(yout, 0, sizeof(yout));
    memset(sample, 0, sizeof(sample));

    conceal[0] = conceal[1] = NULL;

}

CMpaDecoderL3::~CMpaDecoderL3()
{
int i;
    for(i=0;i<2;i++) {
#if defined (MP3_CONCEAL_LOSS)
        if( conceal[i] != NULL ) delete conceal[i];
#endif //MP3_CONCEAL_LOSS
    }
}

//=======================================================================
int CMpaDecoderL3::audio_decode_init(MPEG_HEAD *h,
                                  int framebytes_arg,
                                  int reduction_code,
                                  int transform_code,
                                  int convert_code,
                                  int freq_limit,
                                  int conceal_enable)

{
int i, j, k;
int samprate;
int limit;
int bit_code;
int out_chans;

// could have multiple inits, free error conceal class if any
for(i=0;i<2;i++) {
    if( conceal[i] != NULL ) {
#if defined (MP3_CONCEAL_LOSS)
        delete conceal[i];
        conceal[i] = NULL;
#endif //MP3_CONCEAL_LOSS
    }
}
conceal_flag = 0;
if( conceal_enable ) conceal_flag = 1;


buf_ptr0 = 0;
buf_ptr1 = 0;

/* check if code handles */
if( h->option != 1 ) return 0;         /* layer III only */

if( h->id ) ncbl_mixed = 8;  /* mpeg-1 */
else        ncbl_mixed = 6;  /* mpeg-2 */

m_bMpeg1 = h->id;
m_nSampsPerFrame = 1152;

if (!m_bMpeg1)
    m_nSampsPerFrame >>=1;

framebytes = framebytes_arg;

transform_code = transform_code;    /* not used, asm compatability */
bit_code = 0;

#ifndef REDUCTION
    reduction_code = 0;
#endif
#ifdef EIGHT_BIT
if( convert_code & 8 ) bit_code = 1;
#endif

convert_code = convert_code & 3;    /* higher bits used by dec8 freq cvt */
if( reduction_code < 0 ) reduction_code = 0;
if( reduction_code > 2 ) reduction_code = 2;
if( freq_limit < 1000 ) freq_limit = 1000;


samprate = sr_table_L3[4*h->id + h->sr_index];
if( (h->sync & 1) == 0 ) samprate = samprate/2;  // mpeg 2.5 
/*----- compute nsb_limit --------*/
nsb_limit = (freq_limit*64L + samprate/2)/samprate;   /*- caller limit -*/
limit = (32>>reduction_code);
if( limit > 8 ) limit--;
if( nsb_limit > limit )  nsb_limit = limit;
limit = 18*nsb_limit;
k = h->id;
if( (h->sync & 1) == 0 ) k = 2;  // mpeg 2.5 
if( k == 1 ) {
band_limit12 = 3*sfBandIndexTable[k][h->sr_index].s[13];
band_limit = band_limit21 = sfBandIndexTable[k][h->sr_index].l[22];
}
else {
band_limit12 = 3*sfBandIndexTable[k][h->sr_index].s[12];
band_limit = band_limit21 = sfBandIndexTable[k][h->sr_index].l[21];
}
band_limit += 8;        /* allow for antialias */
if( band_limit > limit ) band_limit = limit;

if( band_limit21 > band_limit ) band_limit21 = band_limit;
if( band_limit12 > band_limit ) band_limit12 = band_limit;


band_limit_nsb = (band_limit+17)/18;  /* limit nsb's rounded up */
/*----------------------------------------------*/
gain_adjust = 0;     /* adjust gain e.g. cvt to mono sum channel */
if( (h->mode != 3) && (convert_code == 1) ) gain_adjust = -4;

outvalues = 1152 >> reduction_code;
if( h->id == 0 ) outvalues /= 2;

out_chans = 2;
if( h->mode == 3 ) out_chans = 1;
if( convert_code ) out_chans = 1;

sbt_L3 = sbt_table_L3[bit_code][reduction_code][out_chans-1];
k = 1 + convert_code;
if( h->mode == 3 ) k = 0;
// Store the proper xform offset in Xform.  We will need to
// do a switch on Xform and call the appropriate member function.
iXform = k;

//Xform = xform_table[k];

outvalues *= out_chans;

if( bit_code )  outbytes = outvalues;
else            outbytes = sizeof(short)*outvalues;
if( bit_code ) zero_level_pcm = 128;   /* 8 bit output */
else           zero_level_pcm = 0;


decinfo.channels  = out_chans;
decinfo.outvalues = outvalues;
decinfo.samprate =  samprate >> reduction_code;
if( bit_code ) decinfo.bits     = 8;
else           decinfo.bits     = sizeof(short)*8;
decinfo.framebytes = framebytes;
decinfo.type = 0;

half_outbytes = outbytes/2;
/*------------------------------------------*/

/*- init band tables --*/


k = h->id;
if( (h->sync & 1) == 0 ) k = 2;  // mpeg 2.5 
for(i=0;i<22;i++) 
    sfBandIndex[0][i] = sfBandIndexTable[k][h->sr_index].l[i+1];
for(i=0;i<13;i++) 
    sfBandIndex[1][i] = 3*sfBandIndexTable[k][h->sr_index].s[i+1];
for(i=0;i<22;i++) nBand[0][i] = 
    sfBandIndexTable[k][h->sr_index].l[i+1] 
        - sfBandIndexTable[k][h->sr_index].l[i];
for(i=0;i<13;i++) nBand[1][i] = 
    sfBandIndexTable[k][h->sr_index].s[i+1] 
        - sfBandIndexTable[k][h->sr_index].s[i];


/*--- clear buffers --*/
for(i=0;i<576;i++) yout[i] = 0.0f;
for(j=0;j<2;j++) {
    for(k=0;k<2;k++) {
        for(i=0;i<576;i++) {
            sample[j][k][i].x = 0.0f;
            sample[j][k][i].s = 0;
        }
    }
}

// h_id used to select xform routine
h_id = h->id;

// conceal class
//conceal[0] = new CConcealment(band_limit21);
//if( h->mode != 3 ) {   // need two channels of concealment
//    conceal[1] = new CConcealment(band_limit21);
//}

if( conceal_flag ) {

#if defined (MP3_CONCEAL_LOSS)    
    conceal[0] = new CConcealment(576);
    if( h->mode != 3 ) {   // need two channels of concealment
        conceal[1] = new CConcealment(576);
    }
#endif //MP3_CONCEAL_LOSS
}


return 1;
}
//=========================================================================
IN_OUT CMpaDecoderL3::audio_decode(unsigned char *bs, unsigned char *pcm)
{

    if( h_id )  {
        return L3audio_decode_MPEG1(bs, pcm);
    }
    else {
        return L3audio_decode_MPEG2(bs, pcm);
    }
}
///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------*/
IN_OUT CMpaDecoderL3::L3audio_decode_MPEG1(unsigned char *bs, 
                                           unsigned char *pcm)
{
    int sync;
    IN_OUT in_out;
    int side_bytes;
    int nbytes;

    iframe++;

    bitget_init(&bitdat, bs); /* initialize bit getter */
    /* test sync */
    in_out.in_bytes = 0;      /* assume fail */
    in_out.out_bytes = 0;
    sync = bitget(&bitdat, 12);

    if( sync != 0xFFF ) return in_out;       /* sync fail */
    /*-----------*/

    /*-- unpack side info --*/
    side_bytes = unpack_side_MPEG1();
    if( framebytes <= 0 ) return in_out;  // fail bad sr or br index

    padframebytes = framebytes+pad;

    in_out.in_bytes = padframebytes;

    /*-- load main data and update buf pointer --*/
    /*------------------------------------------- 
      if start point < 0, must just cycle decoder 
      if jumping into middle of stream, 
      ---------------------------------------------*/
    buf_ptr0 = buf_ptr1 - side_info.main_data_begin;   /* decode start point */
    if( buf_ptr1 > BUF_TRIGGER ) {          /* shift buffer */
        memmove(buf, buf+buf_ptr0, side_info.main_data_begin);
        buf_ptr0 = 0;
        buf_ptr1 = side_info.main_data_begin;
    }
    nbytes = padframebytes - side_bytes - crcbytes;
    if( nbytes>0 )
    {
        memmove(buf+buf_ptr1, bs+side_bytes+crcbytes, nbytes);
        buf_ptr1 += nbytes;
    }
    
    /*-----------------------*/
 
    if( buf_ptr0 >= 0 ) {
        main_pos_bit = buf_ptr0 << 3;
        unpack_main(buf, 0);
        Xform(pcm, 0);
        unpack_main(buf, 1);
        Xform(pcm+half_outbytes, 1);
        in_out.out_bytes = outbytes;
    }
    else {
        memset(pcm, zero_level_pcm, outbytes);  /* fill out skipped frames */
        in_out.out_bytes = outbytes;
    }

    return in_out;
}
//=========================================================================
IN_OUT CMpaDecoderL3::L3audio_decode_MPEG2(unsigned char *bs, unsigned char *pcm)
{
int sync;
IN_OUT in_out;
int side_bytes;
int nbytes;

iframe++;

bitget_init(&bitdat, bs);          /* initialize bit getter */
/* test sync */
in_out.in_bytes = 0;     /* assume fail */
in_out.out_bytes = 0;
sync = bitget(&bitdat, 12);

//if( sync != 0xFFF ) return in_out;       /* sync fail */
mpeg25_flag = 0;
if( sync != 0xFFF ) {
    mpeg25_flag = 1;       /* mpeg 2.5 sync */
    if( sync != 0xFFE ) return in_out;  /* sync fail */
}
/*-----------*/


/*-- unpack side info --*/
side_bytes = unpack_side_MPEG2(igr);
if( framebytes <= 0 ) return in_out;  // fail bad sr or br index

padframebytes = framebytes+pad;

in_out.in_bytes = padframebytes;

buf_ptr0 = buf_ptr1 - side_info.main_data_begin;   /* decode start point */
if( buf_ptr1 > BUF_TRIGGER ) {          /* shift buffer */
    memmove(buf, buf+buf_ptr0, side_info.main_data_begin);
    buf_ptr0 = 0;
    buf_ptr1 = side_info.main_data_begin;
}
nbytes = padframebytes - side_bytes - crcbytes;
memmove(buf+buf_ptr1, bs+side_bytes+crcbytes, nbytes);
buf_ptr1 += nbytes;
/*-----------------------*/

if( buf_ptr0 >= 0 ) {
    main_pos_bit = buf_ptr0 << 3;
    unpack_main(buf, igr);
    Xform(pcm, igr);
    in_out.out_bytes = outbytes;
}
else {
    memset(pcm, zero_level_pcm, outbytes);  /* fill out skipped frames */
    in_out.out_bytes = outbytes;
}

igr = igr^1;
return in_out;
}
/*====================================================================*/
int CMpaDecoderL3::unpack_side_MPEG1()
{
int prot;
int br_index;
int igr, ch;
int side_bytes;

/* decode partial header plus initial side info */
/* at entry bit getter points at id, sync skipped by caller */

id = bitget(&bitdat, 1);   /* id */
bitget(&bitdat, 2);        /* skip layer */
prot = bitget(&bitdat, 1);     /* bitget prot bit */
br_index = bitget(&bitdat, 4);
sr_index = bitget(&bitdat, 2);
pad = bitget(&bitdat, 1);
bitget(&bitdat, 1);            /* skip to mode */
side_info.mode = bitget(&bitdat, 2);        /* mode */
side_info.mode_ext = bitget(&bitdat, 2);    /* mode ext */

if( side_info.mode != 1 ) side_info.mode_ext = 0;

/* adjust global gain in ms mode to avoid having to mult by 1/sqrt(2) */
ms_mode = side_info.mode_ext >> 1;
is_mode = side_info.mode_ext & 1;


crcbytes = 0;
if( prot ) bitget(&bitdat, 4);    /* skip to data */
else {
    bitget(&bitdat, 20);          /* skip crc */
    crcbytes = 2;
}

if( br_index > 0 )      /* framebytes fixed for free format */
    framebytes =
        2880 * mp_br_table_L3[id][br_index]/mp_sr20_table_L3[id][sr_index];

side_info.main_data_begin = bitget(&bitdat, 9);
if( side_info.mode == 3 ) {
    side_info.private_bits  = bitget(&bitdat, 5);
    nchan = 1;
    stereo_flag = 0;
    side_bytes = (4+17);   /*-- with header --*/
}
else {                      
    side_info.private_bits  = bitget(&bitdat, 3);
    nchan = 2;
    stereo_flag = 1;
    side_bytes = (4+32);   /*-- with header --*/
}
for(ch=0;ch<nchan;ch++) side_info.scfsi[ch] = bitget(&bitdat, 4);
/* this always 0 (both igr) for short blocks */


for(igr=0;igr<2;igr++) {
for(ch=0;ch<nchan;ch++) {
    side_info.gr[igr][ch].part2_3_length   = bitget(&bitdat, 12);
    side_info.gr[igr][ch].big_values       = bitget(&bitdat, 9);
    side_info.gr[igr][ch].global_gain      = bitget(&bitdat, 8) + gain_adjust;
    if( ms_mode ) side_info.gr[igr][ch].global_gain -= 2;
    side_info.gr[igr][ch].scalefac_compress      = bitget(&bitdat, 4);
    side_info.gr[igr][ch].window_switching_flag  = bitget(&bitdat, 1);
    if( side_info.gr[igr][ch].window_switching_flag ) {
        side_info.gr[igr][ch].block_type          = bitget(&bitdat, 2);
        side_info.gr[igr][ch].mixed_block_flag    = bitget(&bitdat, 1);
        side_info.gr[igr][ch].table_select[0]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[1]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].subblock_gain[0]    = bitget(&bitdat, 3);
        side_info.gr[igr][ch].subblock_gain[1]    = bitget(&bitdat, 3);
        side_info.gr[igr][ch].subblock_gain[2]    = bitget(&bitdat, 3);
        /* region count set in terms of long block cb's/bands */
        /* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
        /* if(window_switching_flag) always 36 samples in region0 */
        side_info.gr[igr][ch].region0_count = (8-1);    /* 36 samples */
        side_info.gr[igr][ch].region1_count = 20-(8-1);
    }
    else {
        side_info.gr[igr][ch].mixed_block_flag    = 0;
        side_info.gr[igr][ch].block_type          = 0;
        side_info.gr[igr][ch].table_select[0]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[1]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[2]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].region0_count       = bitget(&bitdat, 4);
        side_info.gr[igr][ch].region1_count       = bitget(&bitdat, 3);
    }
    side_info.gr[igr][ch].preflag             = bitget(&bitdat, 1);
    side_info.gr[igr][ch].scalefac_scale      = bitget(&bitdat, 1);
    side_info.gr[igr][ch].count1table_select  = bitget(&bitdat, 1);
}
}    


/* return  bytes in header + side info */
return side_bytes;
}
/*====================================================================*/
int CMpaDecoderL3::unpack_side_MPEG2(int igr)
{
int prot;
int br_index;
int ch;
int side_bytes;

/* decode partial header plus initial side info */
/* at entry bit getter points at id, sync skipped by caller */

id = bitget(&bitdat, 1);   /* id */
bitget(&bitdat, 2);        /* skip layer */
prot = bitget(&bitdat, 1);     /* bitget prot bit */
br_index = bitget(&bitdat, 4);
sr_index = bitget(&bitdat, 2);
pad = bitget(&bitdat, 1);
bitget(&bitdat, 1);            /* skip to mode */
side_info.mode = bitget(&bitdat, 2);        /* mode */
side_info.mode_ext = bitget(&bitdat, 2);    /* mode ext */

if( side_info.mode != 1 ) side_info.mode_ext = 0;

/* adjust global gain in ms mode to avoid having to mult by 1/sqrt(2) */
ms_mode = side_info.mode_ext >> 1;
is_mode = side_info.mode_ext & 1;

crcbytes = 0;
if( prot ) bitget(&bitdat, 4);    /* skip to data */
else {
    bitget(&bitdat, 20);          /* skip crc */
    crcbytes = 2;
}

if( br_index > 0 )  {       /* framebytes fixed for free format */
  if( mpeg25_flag == 0 ) {
    framebytes =
        1440 * mp_br_table_L3[id][br_index]/mp_sr20_table_L3[id][sr_index];
  }
  else {
    framebytes =
        2880 * mp_br_table_L3[id][br_index]/mp_sr20_table_L3[id][sr_index];
    //if( sr_index == 2 ) return 0;  // fail mpeg25 8khz
  }
}
side_info.main_data_begin = bitget(&bitdat, 8);
if( side_info.mode == 3 ) {
    side_info.private_bits  = bitget(&bitdat, 1);
    nchan = 1;
    stereo_flag = 0;
    side_bytes = (4+9);   /*-- with header --*/
}
else {                      
    side_info.private_bits  = bitget(&bitdat, 2);
    nchan = 2;
    stereo_flag = 1;
    side_bytes = (4+17);   /*-- with header --*/
}
side_info.scfsi[1] = side_info.scfsi[0] = 0;


for(ch=0;ch<nchan;ch++) {
    side_info.gr[igr][ch].part2_3_length   = bitget(&bitdat, 12);
    side_info.gr[igr][ch].big_values       = bitget(&bitdat, 9);
    side_info.gr[igr][ch].global_gain      = bitget(&bitdat, 8) + gain_adjust;
    if( ms_mode ) side_info.gr[igr][ch].global_gain -= 2;
    side_info.gr[igr][ch].scalefac_compress      = bitget(&bitdat, 9);
    side_info.gr[igr][ch].window_switching_flag  = bitget(&bitdat, 1);
    if( side_info.gr[igr][ch].window_switching_flag ) {
        side_info.gr[igr][ch].block_type          = bitget(&bitdat, 2);
        side_info.gr[igr][ch].mixed_block_flag    = bitget(&bitdat, 1);
        side_info.gr[igr][ch].table_select[0]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[1]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].subblock_gain[0]    = bitget(&bitdat, 3);
        side_info.gr[igr][ch].subblock_gain[1]    = bitget(&bitdat, 3);
        side_info.gr[igr][ch].subblock_gain[2]    = bitget(&bitdat, 3);
        /* region count set in terms of long block cb's/bands  */
        /* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
        /* bt=1 or 3       54 samples */
        /* bt=2 mixed=0    36 samples */
        /* bt=2 mixed=1    54 (8 long sf) samples? or maybe 36 */
        /* region0 discussion says 54 but this would mix long */
        /* and short in region0 if scale factors switch */
        /* at band 36 (6 long scale factors) */
        if( (side_info.gr[igr][ch].block_type == 2) ) {
            side_info.gr[igr][ch].region0_count = (6-1); /* 36 samples */
            side_info.gr[igr][ch].region1_count = 20-(6-1);
        }
        else {  /* long block type 1 or 3 */
            side_info.gr[igr][ch].region0_count = (8-1); /* 54 samples */
            side_info.gr[igr][ch].region1_count = 20-(8-1);
        }
    }
    else {
        side_info.gr[igr][ch].mixed_block_flag    = 0;
        side_info.gr[igr][ch].block_type          = 0;
        side_info.gr[igr][ch].table_select[0]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[1]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].table_select[2]     = bitget(&bitdat, 5);
        side_info.gr[igr][ch].region0_count       = bitget(&bitdat, 4);
        side_info.gr[igr][ch].region1_count       = bitget(&bitdat, 3);
    }
    side_info.gr[igr][ch].preflag             = 0;
    side_info.gr[igr][ch].scalefac_scale      = bitget(&bitdat, 1);
    side_info.gr[igr][ch].count1table_select  = bitget(&bitdat, 1);
}

/* return  bytes in header + side info */
return side_bytes;
}
/*====================================================================*/
void CMpaDecoderL3::unpack_main(unsigned char *buf, int igr)
{
int ch;
int bit0;
int n1, n2, n3, n4, nn2, nn3;
int nn4;
int qbits;
int m0;

for(ch=0;ch<nchan;ch++) {
    bitget_init(&bitdat, buf + (main_pos_bit >> 3));
    bit0 = (main_pos_bit & 7);
    if( bit0 ) bitget(&bitdat, bit0);
    main_pos_bit +=  side_info.gr[igr][ch].part2_3_length;
    bitget_init_end(&bitdat, buf + ((main_pos_bit+39) >> 3));
    /*-- scale factors --*/
    if( id ) 
        unpack_sf_sub_MPEG1(&sf[igr][ch], 
            &side_info.gr[igr][ch], side_info.scfsi[ch],  igr, &bitdat);
    else 
        unpack_sf_sub_MPEG2(&sf[igr][ch], 
            &side_info.gr[igr][ch], is_mode & ch, &is_sf_info, &bitdat);
    /*--- huff data ---*/
    n1 = sfBandIndex[0][side_info.gr[igr][ch].region0_count];
    n2 = sfBandIndex[0][side_info.gr[igr][ch].region0_count
                        + side_info.gr[igr][ch].region1_count+1];
    n3 =  side_info.gr[igr][ch].big_values;
    n3 = n3 + n3;

    
    if( n3 > band_limit ) n3 = band_limit;
    if( n2 > n3 ) n2 = n3;
    if( n1 > n3 ) n1 = n3;
    nn3 = n3 - n2;
    nn2 = n2 - n1;
    unpack_huff(sample[ch][igr],     n1, side_info.gr[igr][ch].table_select[0], &bitdat);
    unpack_huff(sample[ch][igr]+n1, nn2, side_info.gr[igr][ch].table_select[1], &bitdat);
    unpack_huff(sample[ch][igr]+n2, nn3, side_info.gr[igr][ch].table_select[2], &bitdat);
    qbits = side_info.gr[igr][ch].part2_3_length - (bitget_bits_used(&bitdat)-bit0);
    nn4 = unpack_huff_quad(sample[ch][igr]+n3, band_limit - n3, qbits, 
            side_info.gr[igr][ch].count1table_select, &bitdat);
    n4 = n3 + nn4;
    nsamp[igr][ch] = n4;
    //limit n4 or allow deqaunt to sf band 22
    if( side_info.gr[igr][ch].block_type == 2 ) n4 = min(n4,band_limit12);
    else  n4 = min(n4,band_limit21);
    if( n4 < 576 ) memset(sample[ch][igr]+n4, 0, sizeof(SAMPLE)*(576-n4));
    if(  bitget_overrun(&bitdat) ) { // bad data overrun
        memset(sample[ch][igr], 0, sizeof(SAMPLE)*(576));
    }
}


/*--- dequant ---*/
for(ch=0;ch<nchan;ch++) {
    dequant(sample[ch][igr], 
        &nsamp[igr][ch],    /* nsamp updated for shorts */
        &sf[igr][ch], &side_info.gr[igr][ch], 
        &cb_info[igr][ch], ncbl_mixed, nBand, re_buf);
}


/*--- ms stereo processing  ---*/
if( ms_mode ) {
    if( is_mode == 0 ) {
        m0 = nsamp[igr][0];    /* process to longer of left/right */
        if( m0 < nsamp[igr][1] ) m0 = nsamp[igr][1];
    }
    else {  /* process to last cb in right */
        m0  = sfBandIndex[cb_info[igr][1].cbtype][cb_info[igr][1].cbmax];
    }
    ms_process(sample[0][igr], m0);    
}

/*--- is stereo processing  ---*/
if( is_mode ) {
    if( id ) 
        is_process_MPEG1(sample[0][igr], &sf[igr][1], 
            cb_info[igr], nsamp[igr][0], ms_mode, nBand, sfBandIndex);
    else
        is_process_MPEG2(sample[0][igr], &sf[igr][1], 
            cb_info[igr], &is_sf_info,
            nsamp[igr][0], ms_mode, nBand, sfBandIndex);
}

/*-- adjust ms and is modes to max of left/right */
if( side_info.mode_ext ) {
    if(nsamp[igr][0] < nsamp[igr][1]) nsamp[igr][0] = nsamp[igr][1];
    else  nsamp[igr][1] = nsamp[igr][0];
}

/*--- antialias ---*/
for(ch=0;ch<nchan;ch++) {
    if( cb_info[igr][ch].ncbl == 0 ) continue; /* have no long blocks */
    if( side_info.gr[igr][ch].mixed_block_flag ) n1 = 1; /* 1 -> 36 samples */
    else n1 = (nsamp[igr][ch]+7)/18;
    if( n1 > 31 ) n1 = 31;
    antialias(sample[ch][igr], n1);
    n1 = 18*n1+8;       /* update number of samples */
    if( n1 > nsamp[igr][ch] ) nsamp[igr][ch] = n1;
}


//Xform(pcm, igr);


/*-- done --*/
}
/*====================================================================*/
void CMpaDecoderL3::Xform(unsigned char *pcm, int igr)
{
    switch(iXform) {
    default:
    case 0:
        Xform_mono(pcm, igr);
        break;
    case 1:
        Xform_dual(pcm,igr);
        break;
    case 2:
        Xform_dual_mono(pcm,igr);
        break;
    case 3:
        Xform_mono(pcm,igr);
        break;
    case 4:
        Xform_dual_right(pcm,igr);
        break;
    }
}    
/*====================================================================*/
void CMpaDecoderL3::Xform_mono(unsigned char *pcm, int igr)
{
int igr_prev, n1, n2;

/*--- hybrid + sbt ---*/
n1 = n2 = nsamp[igr][0];   /* total number bands */
if( side_info.gr[igr][0].block_type == 2 ) { /* long bands */
    n1 = 0;     
    if( side_info.gr[igr][0].mixed_block_flag ) 
                n1 = sfBandIndex[0][ncbl_mixed-1];
}
if( n1 > band_limit ) n1 = band_limit;
if( n2 > band_limit ) n2 = band_limit;
igr_prev = igr^1;

nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev], 
     yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0], band_limit_nsb);
FreqInvert(yout, nsamp[igr][0]);
sbt_L3(yout, pcm, 0, vbuf, vb_ptr);

}
/*--------------------------------------------------------------------*/
void CMpaDecoderL3::Xform_dual_right(unsigned char *pcm, int igr)
{
int igr_prev, n1, n2;

/*--- hybrid + sbt ---*/
n1 = n2 = nsamp[igr][1];   /* total number bands */
if( side_info.gr[igr][1].block_type == 2 ) { /* long bands */
    n1 = 0;     
    if( side_info.gr[igr][1].mixed_block_flag ) 
                n1 = sfBandIndex[0][ncbl_mixed-1];
}
if( n1 > band_limit ) n1 = band_limit;
if( n2 > band_limit ) n2 = band_limit;
igr_prev = igr^1;
nsamp[igr][1] = hybrid(sample[1][igr], sample[1][igr_prev], 
      yout, side_info.gr[igr][1].block_type, n1, n2, nsamp[igr_prev][1], band_limit_nsb);
FreqInvert(yout, nsamp[igr][1]);
sbt_L3(yout, pcm, 0, vbuf, vb_ptr);

}
/*--------------------------------------------------------------------*/
void CMpaDecoderL3::Xform_dual(unsigned char *pcm, int igr)
{
int ch;
int igr_prev, n1, n2;


/*--- hybrid + sbt ---*/
igr_prev = igr^1;
for(ch=0;ch<nchan;ch++) {
    n1 = n2 = nsamp[igr][ch];   /* total number bands */
    if( side_info.gr[igr][ch].block_type == 2 ) { /* long bands */
        n1 = 0;     
        if( side_info.gr[igr][ch].mixed_block_flag ) 
                    n1 = sfBandIndex[0][ncbl_mixed-1];
    }
    if( n1 > band_limit ) n1 = band_limit;
    if( n2 > band_limit ) n2 = band_limit;
    nsamp[igr][ch] = hybrid(sample[ch][igr], sample[ch][igr_prev], 
        yout, side_info.gr[igr][ch].block_type, n1, n2, nsamp[igr_prev][ch], band_limit_nsb);
    FreqInvert(yout, nsamp[igr][ch]);
    sbt_L3(yout, pcm, ch, vbuf, vb_ptr);
}

}
/*--------------------------------------------------------------------*/
void CMpaDecoderL3::Xform_dual_mono(unsigned char *pcm, int igr)
{
int igr_prev, n1, n2, n3;

/*--- hybrid + sbt ---*/
igr_prev = igr^1;
if( (side_info.gr[igr][0].block_type == side_info.gr[igr][1].block_type)
      && (side_info.gr[igr][0].mixed_block_flag == 0)
      && (side_info.gr[igr][1].mixed_block_flag == 0) ) 
{

    n2 = nsamp[igr][0];   /* total number bands max of L R */
    if( n2 < nsamp[igr][1] ) n2 = nsamp[igr][1];
    if( n2 > band_limit ) n2 = band_limit;
    n1 = n2;  /* n1 = number long bands */
    if( side_info.gr[igr][0].block_type == 2 ) n1 = 0;
    sum_f_bands(sample[0][igr], sample[1][igr], n2);
    n3 = nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev], 
        yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0], band_limit_nsb);
}
else 
{       /* transform and then sum (not tested - never happens in test)*/
    /*-- left chan --*/
    n1 = n2 = nsamp[igr][0];   /* total number bands */
    if( side_info.gr[igr][0].block_type == 2 ) { 
        n1 = 0;     /* long bands */
        if( side_info.gr[igr][0].mixed_block_flag ) 
                    n1 = sfBandIndex[0][ncbl_mixed-1];
    }
    n3 = nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev], 
        yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0], band_limit_nsb);
    /*-- right chan --*/
    n1 = n2 = nsamp[igr][1];   /* total number bands */
    if( side_info.gr[igr][1].block_type == 2 ) { 
        n1 = 0;     /* long bands */
        if( side_info.gr[igr][1].mixed_block_flag ) 
                    n1 = sfBandIndex[0][ncbl_mixed-1];
    }
    nsamp[igr][1] = hybrid_sum(sample[1][igr], sample[0][igr], 
        yout, side_info.gr[igr][1].block_type, n1, n2);
    if( n3 < nsamp[igr][1] ) n1 = nsamp[igr][1];
}

/*--------*/
FreqInvert(yout, n3);
sbt_L3(yout, pcm, 0, vbuf, vb_ptr);

}
/*------------------------------------------------------------------*/
/*==================================================================*/
/*====================concealment/reformatted frames================*/
/*==================================================================*/
#ifdef REFORMAT
// test routine - reformats frame
IN_OUT CMpaDecoderL3::audio_decode_reformat(unsigned char *bs, 
                                            unsigned char *bs_out)
{
    if( h_id )  {
        return L3reformat_MPEG1(bs, bs_out);
    }
    else {
        return L3reformat_MPEG2(bs, bs_out);
    }
}
//============================================================
// test routine - reformats frame
IN_OUT CMpaDecoderL3::L3reformat_MPEG1(unsigned char *bs, 
                                       unsigned char *bs_out)
{
int sync;
IN_OUT in_out;
int side_bytes;
int nbytes, nbytes_r;

//iframe++;


bitget_init(bs);          /* initialize bit getter */
/* test sync */
in_out.in_bytes = 0;     /* assume fail */
in_out.out_bytes = 0;
sync = bitget(&bitdat, 12);

if( sync != 0xFFF ) return in_out;       /* sync fail */
/*-----------*/

/*-- unpack side info --*/
side_bytes = unpack_side_MPEG1();
if( framebytes <= 0 ) return in_out;  // fail bad sr or br index

padframebytes = framebytes+pad;

in_out.in_bytes = padframebytes;

/*-- load main data and update buf pointer --*/
/*------------------------------------------- 
   if start point < 0, must just cycle decoder 
   if jumping into middle of stream, 
---------------------------------------------*/

nbytes_r = reformat_side_bytes + reformat_bytes - side_info.main_data_begin;
if( nbytes_r > 0 ) {
    memmove(bs_out, reformat_buf, nbytes_r);
    in_out.out_bytes = nbytes_r;
}


buf_ptr0 = buf_ptr1 - side_info.main_data_begin;   /* decode start point */
if( buf_ptr1 > BUF_TRIGGER ) {          /* shift buffer */
    memmove(buf, buf+buf_ptr0, side_info.main_data_begin);
    buf_ptr0 = 0;
    buf_ptr1 = side_info.main_data_begin;
}
nbytes = padframebytes - side_bytes - crcbytes;
memmove(buf+buf_ptr1, bs+side_bytes+crcbytes, nbytes);
buf_ptr1 += nbytes;
/*-----------------------*/

if( buf_ptr0 >= 0 ) {
    reformat_side_bytes = side_bytes+crcbytes;
    memmove(reformat_buf, bs, reformat_side_bytes);
    reformat_buf[4+crcbytes]    = 0;      // set main data begin = 0
    reformat_buf[4+crcbytes+1] &= 0x7F;   // set main data begin = 0
    reformat_bytes = buf_ptr1 - buf_ptr0;
    if( reformat_bytes > 0 ) {
        memmove(reformat_buf+reformat_side_bytes, 
            buf+buf_ptr0, reformat_bytes);
    }
}
else {
    reformat_side_bytes = 0;
    reformat_bytes = 0;
}

return in_out;
}
/*=========================================================*/
// test routine - reformats frame
IN_OUT CMpaDecoderL3::L3reformat_MPEG2(unsigned char *bs, 
                                       unsigned char *bs_out)
{
int sync;
IN_OUT in_out;
int side_bytes;
int nbytes, nbytes_r;

//iframe++;

bitget_init(bs);          /* initialize bit getter */
/* test sync */
in_out.in_bytes = 0;     /* assume fail */
in_out.out_bytes = 0;
sync = bitget(&bitdat, 12);

//if( sync != 0xFFF ) return in_out;       /* sync fail */
mpeg25_flag = 0;
if( sync != 0xFFF ) {
    mpeg25_flag = 1;       /* mpeg 2.5 sync */
    if( sync != 0xFFE ) return in_out;  /* sync fail */
}

/*-- unpack side info --*/
side_bytes = unpack_side_MPEG2(igr);
if( framebytes <= 0 ) return in_out;  // fail bad sr or br index

padframebytes = framebytes+pad;

in_out.in_bytes = padframebytes;

nbytes_r = reformat_side_bytes + reformat_bytes - side_info.main_data_begin;
if( nbytes_r > 0 ) {
    memmove(bs_out, reformat_buf, nbytes_r);
    in_out.out_bytes = nbytes_r;
}


buf_ptr0 = buf_ptr1 - side_info.main_data_begin;   /* decode start point */
if( buf_ptr1 > BUF_TRIGGER ) {          /* shift buffer */
    memmove(buf, buf+buf_ptr0, side_info.main_data_begin);
    buf_ptr0 = 0;
    buf_ptr1 = side_info.main_data_begin;
}
nbytes = padframebytes - side_bytes - crcbytes;
memmove(buf+buf_ptr1, bs+side_bytes+crcbytes, nbytes);
buf_ptr1 += nbytes;
/*-----------------------*/

if( buf_ptr0 >= 0 ) {
    reformat_side_bytes = side_bytes+crcbytes;
    memmove(reformat_buf, bs, reformat_side_bytes);
    reformat_buf[4+crcbytes]    = 0;      // set main data begin = 0
    reformat_bytes = buf_ptr1 - buf_ptr0;
    if( reformat_bytes > 0 ) {
        memmove(reformat_buf+reformat_side_bytes, 
            buf+buf_ptr0, reformat_bytes);
    }
}
else {
    reformat_side_bytes = 0;
    reformat_bytes = 0;
}

//igr = igr^1;   //  must be done by decode_reformatted

return in_out;
}
#endif  // REFORMAT 
//=========================================================================
//=========================================================================
IN_OUT CMpaDecoderL3::audio_decode(unsigned char *bs, unsigned char *pcm, int size)
{
    if (!size)
        return audio_decode(bs, pcm);

// overloaded for reformatted frames.   main_data_begin == 0

    if( h_id )  {
        return L3audio_decode_reformattedMPEG1(bs, pcm, size);
    }
    else {
        return L3audio_decode_reformattedMPEG2(bs, pcm, size);
    }
}
/*====================================================================*/
IN_OUT CMpaDecoderL3::L3audio_decode_reformattedMPEG1(unsigned char *bs, 
                           unsigned char *pcm, int size)
{
int sync;
IN_OUT in_out;
int side_bytes;

// decode reformatted (self contained) frame 
// fails if main_data_begin != 0


iframe++;

in_out.in_bytes  = 0;   // assume fail  e.g. sync fail
in_out.out_bytes = 0;

if( size > 0 ) {

    bitget_init(&bitdat, bs);          /* initialize bit getter */
    /* test sync */
    sync = bitget(&bitdat, 12);
    if( sync != 0xFFF ) return in_out;       /* sync fail */
    /*-----------*/

    /*-- unpack side info --*/
    side_bytes = unpack_side_MPEG1();
    if( framebytes <= 0 ) return in_out;  // fail bad sr or br index
    if( side_info.main_data_begin ) return in_out;  // fail if data not self contained

    in_out.in_bytes = size;
    in_out.out_bytes = outbytes;

    main_pos_bit = (side_bytes+crcbytes) << 3;  // set for unpack_main
    unpack_main(bs, 0);          // first granule
    conceal_insert(0);
    Xform(pcm, 0);
    unpack_main(bs, 1);          // second granule
    conceal_insert(1);
    Xform(pcm+half_outbytes, 1);

}
else {      // size <= 0, missing frame 
    in_out.in_bytes = size;
    in_out.out_bytes = outbytes;
    conceal_fixup(0);
    Xform(pcm, 0);
    conceal_fixup(1);
    Xform(pcm+half_outbytes, 1);
}




return in_out;
}
//=========================================================================
IN_OUT CMpaDecoderL3::L3audio_decode_reformattedMPEG2(unsigned char *bs, 
                             unsigned char *pcm, int size)
{
int sync;
IN_OUT in_out;
int side_bytes;

// decode reformatted (self contained) frame 
// fails if main_data_begin != 0

iframe++;

in_out.in_bytes = 0;     /* assume fail */
in_out.out_bytes = 0;

if( size > 0 ) {
    bitget_init(&bitdat, bs);          /* initialize bit getter */
    /* test sync */
    sync = bitget(&bitdat, 12);
    //if( sync != 0xFFF ) return in_out;       /* sync fail */
    mpeg25_flag = 0;
    if( sync != 0xFFF ) {
        mpeg25_flag = 1;       /* mpeg 2.5 sync */
        if( sync != 0xFFE ) return in_out;  /* sync fail */
    }

    /*-- unpack side info --*/
    side_bytes = unpack_side_MPEG2(igr);
    if( framebytes <= 0 ) return in_out;  // fail bad sr or br index
    if( side_info.main_data_begin ) return in_out;  // fail if data not self contained

    in_out.in_bytes = size;
    in_out.out_bytes = outbytes;

    main_pos_bit = (side_bytes+crcbytes) << 3;  // set for unpack_main
    unpack_main(bs, igr);
    conceal_insert(igr);
    Xform(pcm, igr);
}
else {      // size <= 0, missing frame 
    in_out.in_bytes = size;
    in_out.out_bytes = outbytes;
    conceal_fixup(igr);
    Xform(pcm, igr);
}

igr = igr^1;
return in_out;
}
/*====================================================================*/
void CMpaDecoderL3::conceal_insert(int igr)
{
int ch, nlines, blockType, i;

if( conceal_flag ) {
    for(ch=0;ch<nchan;ch++) {

#if defined (MP3_CONCEAL_LOSS)        
        conceal[ch]->insert(&sample[ch][igr][0].x, 
            side_info.gr[igr][ch].block_type, nsamp[igr][ch]);
        nlines = conceal[ch]->retrieve(&sample[ch][igr][0].x, blockType);
#endif //MP3_CONCEAL_LOSS
        
        side_info.gr[igr][ch].mixed_block_flag      = 0;
        side_info.gr[igr][ch].block_type            = blockType;
        side_info.gr[igr][ch].window_switching_flag = (blockType != 0) ? 1 : 0;
        nsamp[igr][ch] = nlines;
        for(i=nlines;i<576;i++) sample[ch][igr][i].x = 0.0f;
    }
}
}
/*====================================================================*/
void CMpaDecoderL3::conceal_fixup(int igr)
{
int ch, nlines, blockType, i;

if( conceal_flag ) {
    for(ch=0;ch<nchan;ch++) {

#if defined (MP3_CONCEAL_LOSS)        
        conceal[ch]->insert();
        nlines = conceal[ch]->retrieve(&sample[ch][igr][0].x, blockType);
#endif //MP3_CONCEAL_LOSS
        
        side_info.gr[igr][ch].mixed_block_flag      = 0;
        side_info.gr[igr][ch].block_type            = blockType;
        side_info.gr[igr][ch].window_switching_flag = (blockType != 0) ? 1 : 0;
        nsamp[igr][ch] = nlines;
        for(i=nlines;i<576;i++) sample[ch][igr][i].x = 0.0f;
    }
}
else {       // mute
    for(ch=0;ch<nchan;ch++) {
        side_info.gr[igr][ch].mixed_block_flag      = 0;
        side_info.gr[igr][ch].block_type            = 0;
        side_info.gr[igr][ch].window_switching_flag = 0;
        for(i=0;i<576;i++) sample[ch][igr][i].x = 0.0f;
    }

}

}
/*====================================================================*/
