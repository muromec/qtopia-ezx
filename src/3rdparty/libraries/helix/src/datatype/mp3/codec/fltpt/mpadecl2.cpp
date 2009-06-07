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
#include "hlxclib/math.h"
#include "mhead.h"
#include "mpadecl2.h"

#include "mpalow.h"     // low level extern C prototypes
///////////////////////////////////////////////////////////////////////////////
// Static Data:
///////////////////////////////////////////////////////////////////////////////
static const int look_joint_L2[16] = {  /* lookup stereo sb's by mode+ext */
64, 64, 64, 64,         /* stereo */
2*4, 2*8, 2*12, 2*16,   /* joint */
64, 64, 64, 64,         /* dual */
32, 32, 32, 32,         /* mono */
};
static const int bat_bit_master_L2[] = {
  0, 5, 7, 9, 10, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48 };
//
// -------- init data -----------------
//
static const int steps[18] = {
0, 3, 5, 7, 9, 15, 31, 63, 127,
255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535 };


/*  ABCD_INDEX = lookqt[mode][sr_index][br_index]  */
/*   -1 = invalid  */
static const signed char lookqt[4][3][16]={ /* Flawfinder: ignore */
  1,-1,-1,-1,2,-1,2,0,0,0,1,1,1,1,1,-1,           /*  44ks stereo */
  0,-1,-1,-1,2,-1,2,0,0,0,0,0,0,0,0,-1,           /*  48ks */
  1,-1,-1,-1,3,-1,3,0,0,0,1,1,1,1,1,-1,           /*  32ks */
  1,-1,-1,-1,2,-1,2,0,0,0,1,1,1,1,1,-1,           /*  44ks joint stereo */
  0,-1,-1,-1,2,-1,2,0,0,0,0,0,0,0,0,-1,           /*  48ks */
  1,-1,-1,-1,3,-1,3,0,0,0,1,1,1,1,1,-1,           /*  32ks */
  1,-1,-1,-1,2,-1,2,0,0,0,1,1,1,1,1,-1,           /*  44ks dual chan */
  0,-1,-1,-1,2,-1,2,0,0,0,0,0,0,0,0,-1,           /*  48ks */
  1,-1,-1,-1,3,-1,3,0,0,0,1,1,1,1,1,-1,           /*  32ks */
// mono extended beyond legal br index
//  1,2,2,0,0,0,1,1,1,1,1,1,1,1,1,-1,          /*  44ks single chan */
//  0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,-1,          /*  48ks */
//  1,3,3,0,0,0,1,1,1,1,1,1,1,1,1,-1,          /*  32ks */
// legal mono
 1,2,2,0,0,0,1,1,1,1,1,-1,-1,-1,-1,-1,          /*  44ks single chan */
 0,2,2,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,          /*  48ks */
 1,3,3,0,0,0,1,1,1,1,1,-1,-1,-1,-1,-1,          /*  32ks */
};

static const int sr_table_L2[8] =
    { 22050, 24000, 16000, 1,
      44100, 48000, 32000, 1 };

/* bit allocation table look up */
/* table per mpeg spec tables 3b2a/b/c/d  /e is mpeg2 */
/* look_bat[abcd_index][4][16]  */
static const unsigned char look_bat[5][4][16] = { /* Flawfinder: ignore */
/* LOOK_BATA */
 0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17,
 0, 1, 2, 3, 4, 5, 6, 17, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 17,  0,0,0,0, 0,0,0,0, 0,0,0,0,
/* LOOK_BATB */
 0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17,
 0, 1, 2, 3, 4, 5, 6, 17, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 17,  0,0,0,0, 0,0,0,0, 0,0,0,0,
/* LOOK_BATC */
 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 4, 5, 6, 7, 8,   0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
/* LOOK_BATD */
 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 4, 5, 6, 7, 8, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
/* LOOK_BATE */
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 4, 5, 6, 7, 8, 0,0,0,0, 0,0,0,0,
 0, 1, 2, 4, 0,0,0,0, 0,0,0,0, 0,0,0,0,
};
/* look_nbat[abcd_index]][4] */
static const unsigned char look_nbat[5][4] = { /* Flawfinder: ignore */
  3, 8, 12, 4,
  3, 8, 12, 7,
  2, 0, 6, 0,
  2, 0, 10, 0,
  4, 0, 7, 19,
};

static const int  out_chans_L2[5] = { 1, 2, 1, 1, 1 };


static const SBT_FUNCTION  sbt_table_L2[2][3][5] = {
// 16 bit (usually short) pcm output
    sbt_mono,   
    sbt_dual,   
    sbt_dual_mono, 
#ifdef REDUCTION
    sbt_dual_left, 
    sbt_dual_right,
    sbt16_mono, 
    sbt16_dual, 
    sbt16_dual_mono, 
    sbt16_dual_left, 
    sbt16_dual_right,
    sbt8_mono,  
    sbt8_dual,  
    sbt8_dual_mono, 
    sbt8_dual_left, 
    sbt8_dual_right,
#else     // no reduction
    NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, 
#endif
// 8 bit pcm output
#ifdef EIGHT_BIT
    sbtB_mono,
    sbtB_dual,
    sbtB_dual_mono,
#ifdef REDUCTION
    sbtB_dual_left,
    sbtB_dual_right,
    sbtB16_mono,
    sbtB16_dual,
    sbtB16_dual_mono,
    sbtB16_dual_left,
    sbtB16_dual_right,
    sbtB8_mono,
    sbtB8_dual,
    sbtB8_dual_mono,
    sbtB8_dual_left,
    sbtB8_dual_right,
#else   // no reduction
    NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, 
#endif
#else       // no eight_bit
    NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, 
#endif
 
};

///////////////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////////////
CMpaDecoderL2::CMpaDecoderL2()
 :  CMpaDecoder()


{
table_init();
}

CMpaDecoderL2::~CMpaDecoderL2()
{
}

#ifdef REFORMAT
IN_OUT  CMpaDecoderL2::audio_decode_reformat(unsigned char *bs,
                         unsigned char *bs_out)
{
    IN_OUT x;
    x.in_bytes = 0;     // signal fail, not relevant for Layer 2
    x.out_bytes = 0;
    return x;
}
#endif

//================================================================
int CMpaDecoderL2::audio_decode_init(MPEG_HEAD *h,
                                       int framebytes_arg,
                                       int reduction_code,
                                       int transform_code,
                                       int convert_code,
                                       int freq_limit,
                                       int conceal_enable)
{
int i, j, k;
int abcd_index;
long samprate;
int limit;
int bit_code;

/* check if code handles */
if( h->option != 2 )  return 0;  /* layer II only */

#ifndef REDUCTION
    reduction_code = 0;
#endif
#ifndef EIGHT_BIT
    convert_code = 0;
#endif


m_bMpeg1 = h->id;
m_nSampsPerFrame = 1152;

transform_code = transform_code;    /* not used, asm compatability */
bit_code = 0;
if( convert_code & 8 ) bit_code = 1;
convert_code = convert_code & 3;    /* higher bits used by dec8 freq cvt */
if( reduction_code < 0 ) reduction_code = 0;
if( reduction_code > 2 ) reduction_code = 2;
if( freq_limit < 1000 ) freq_limit = 1000;


framebytes = framebytes_arg;
/* check if code handles */
if( h->sr_index == 3 ) return 0;       /* reserved */

/* compute abcd index for bit allo table selection */
if( h->id )                /* mpeg 1 */
   abcd_index = lookqt[h->mode][h->sr_index][h->br_index];
else  abcd_index = 4;      /* mpeg 2 */

if( abcd_index < 0 ) return 0;  // fail invalid Layer II bit rate index

for(i=0;i<4;i++) {
    for(j=0;j<16;j++) bat[i][j] = look_bat[abcd_index][i][j];
}
for(i=0;i<4;i++) nbat[i] = look_nbat[abcd_index][i];
max_sb = nbat[0] + nbat[1] + nbat[2] + nbat[3];
/*----- compute nsb_limit --------*/
samprate = sr_table_L2[4*h->id + h->sr_index];
nsb_limit = (freq_limit*64L + samprate/2)/samprate;   /*- caller limit -*/
/*---- limit = 0.94*(32>>reduction_code);  ----*/
limit = (32>>reduction_code);
if( limit > 8 ) limit--;
if( nsb_limit > limit )  nsb_limit = limit;
if( nsb_limit > max_sb ) nsb_limit = max_sb;

outvalues = 1152 >> reduction_code;
if( h->mode != 3 ) {         /* adjust for 2 channel modes */
          for(i=0;i<4;i++) nbat[i] *= 2;
          max_sb *= 2;
          nsb_limit *= 2;
}

/* set sbt function */
k = 1 + convert_code;
if( h->mode == 3 ) {
          k = 0;
          }
sbt = sbt_table_L2[bit_code][reduction_code][k];
outvalues *= out_chans_L2[k];
if( bit_code )  outbytes = outvalues;
else            outbytes = sizeof(short)*outvalues;
decinfo.channels  = out_chans_L2[k];
decinfo.outvalues = outvalues;
decinfo.samprate =  samprate >> reduction_code;
if( bit_code ) decinfo.bits     = 8;
else           decinfo.bits     = sizeof(short)*8;
decinfo.framebytes = framebytes;
decinfo.type = 0;

/* clear sample buffer, unused sub bands must be 0 */
for(i=0;i<2304;i++) sample[i] = 0.0F;

return 1;
}
/*---------------------------------------------------------*/
IN_OUT CMpaDecoderL2::audio_decode(unsigned char *bs,
                                   unsigned char *pcm,
                                   int size)
{
int sync, prot;
IN_OUT in_out;

load_init(bs);          /* initialize bit getter */
/* test sync */
in_out.in_bytes = 0;     /* assume fail */
in_out.out_bytes = 0;
sync = load(12);
if( sync != 0xFFF ) return in_out;       /* sync fail */

load(3);   /* skip id and option (checked by init) */
prot = load(1);     /* load prot bit */
load(6);            /* skip to pad */
pad = load(1);
load(1);            /* skip to mode */
stereo_sb = look_joint_L2[load(4)];
if( prot ) load(4);           /* skip to data */
else       load(20);          /* skip crc */

unpack_ba();        /* unpack bit allocation */
unpack_sfs();       /* unpack scale factor selectors */
unpack_sf();        /* unpack scale factor */
unpack_samp();      /* unpack samples */


sbt(sample, pcm, 36, vbuf, vb_ptr);
/*-----------*/
in_out.in_bytes = framebytes + pad;
in_out.out_bytes = outbytes;

return in_out;
}
///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////
void CMpaDecoderL2::table_init()
{
int i, j;
int code;

/*--  c_values (dequant) --*/
for(i=1;i<18;i++) look_c_value[i] = 2.0F/steps[i];

/*--  scale factor table, scale by 32768 for 16 pcm output  --*/
for(i=0;i<64;i++) sf_table[i] = (float)(32768.0*2.0*pow(2.0,-i/3.0));

/*--  grouped 3 level lookup table 5 bit token --*/
for(i=0;i<32;i++) {
   code = i;
   for(j=0;j<3;j++) {
      group3_table[i][j] = (char)(( code % 3 ) - 1);
      code /= 3;
  }
}
/*--  grouped 5 level lookup table 7 bit token --*/
for(i=0;i<128;i++) {
   code = i;
   for(j=0;j<3;j++) {
      group5_table[i][j] = (char)(( code % 5 ) - 2);
      code /= 5;
  }
}
/*--  grouped 9 level lookup table 10 bit token --*/
for(i=0;i<1024;i++) {
   code = i;
   for(j=0;j<3;j++) {
      group9_table[i][j] = (short)(( code % 9 ) - 4);
      code /= 9;
  }
}


}
/*---------------------------------------------------------*/
/*------------- bit getter --------------------------------*/
/*---------------------------------------------------------*/
void CMpaDecoderL2::load_init( unsigned char *buf)
{
bs_ptr = buf;
bits = 0;
bitbuf = 0;
}
/*------------- get n bits from bitstream -------------*/
int CMpaDecoderL2::load( int n)
{
unsigned int x;
if( bits < n ) {           /* refill bit buf if necessary */
          while( bits <= 24 ) {
             bitbuf = (bitbuf << 8) | *bs_ptr++;
             bits += 8;
          }
}
bits -= n;
x = bitbuf >> bits;
bitbuf -= x << bits;
return x;
}
/*------------- skip over n bits in bitstream -------------*/
void CMpaDecoderL2::skip( int n)
{
int k;
if( bits < n ) {
          n -= bits;
          k = n >> 3;     /*--- bytes = n/8 --*/
          bs_ptr+=k;
          n -= k << 3;
          bitbuf =  *bs_ptr++;
          bits = 8;
          }
bits -= n;
bitbuf -= (bitbuf >> bits) << bits;
}
/*--------------------------------------------------------------*/
#define mac_load_check(n)                     \
   if( bits < (n) ) {                           \
          while( bits <= 24 ) {               \
             bitbuf = (bitbuf << 8) | *bs_ptr++;  \
             bits += 8;                       \
          }                                   \
   }
/*--------------------------------------------------------------*/
#define mac_load(n)                    \
       ( bits -= n,                    \
         bitval = bitbuf >> bits,      \
         bitbuf -= bitval << bits,     \
         bitval )

static const int nbit[4] = { 4, 4, 3, 2 };

/*======================================================================*/
void CMpaDecoderL2::unpack_ba()
{
int i, j, k;
int nstereo;

bit_skip = 0;
nstereo = stereo_sb;
k = 0;
for(i=0;i<4;i++) {
   for(j=0;j<nbat[i]; j++, k++) {
   mac_load_check(4);
   ballo[k] = samp_dispatch[k] = bat[i][mac_load(nbit[i])];
   if( k >= nsb_limit )  bit_skip += bat_bit_master_L2[samp_dispatch[k]];
   c_value[k] = look_c_value[samp_dispatch[k]];
   if( --nstereo < 0 ) {
       ballo[k+1] = ballo[k];
       samp_dispatch[k] += 18;  /* flag as joint */
       samp_dispatch[k+1] = samp_dispatch[k];  /* flag for sf */
       c_value[k+1] = c_value[k];
       k++;
       j++;
   }
   }
}
samp_dispatch[nsb_limit] = 37;   /* terminate the dispatcher with skip */
samp_dispatch[k] = 36;           /* terminate the dispatcher */

}
/*-------------------------------------------------------------------------*/
void CMpaDecoderL2::unpack_sfs()    /* unpack scale factor selectors */
{
int i;

for(i=0;i<max_sb;i++) {
  mac_load_check(2);
  if( ballo[i] ) sf_dispatch[i] = mac_load(2);
  else           sf_dispatch[i] = 4;           /* no allo */
}
sf_dispatch[i] = 5;    /* terminate dispatcher */
}
/*-------------------------------------------------------------------------*/
void CMpaDecoderL2::unpack_sf()    /* unpack scale factor */
{                          /* combine dequant and scale factors */
int i;
i = -1;
dispatch:   switch (sf_dispatch[++i]) {
case 0:              /* 3 factors 012 */
  mac_load_check(18);
  cs_factor[0][i] = c_value[i]*sf_table[mac_load(6)];
  cs_factor[1][i] = c_value[i]*sf_table[mac_load(6)];
  cs_factor[2][i] = c_value[i]*sf_table[mac_load(6)];
  goto dispatch;
case 1:             /* 2 factors 002 */
  mac_load_check(12);
  cs_factor[1][i] = cs_factor[0][i] = c_value[i]*sf_table[mac_load(6)];
  cs_factor[2][i] = c_value[i]*sf_table[mac_load(6)];
  goto dispatch;
case 2:             /* 1 factor 000 */
  mac_load_check(6);
  cs_factor[2][i] = cs_factor[1][i] = cs_factor[0][i] =
                                       c_value[i]*sf_table[mac_load(6)];
  goto dispatch;
case 3:             /* 2 factors 022 */
  mac_load_check(12);
  cs_factor[0][i] = c_value[i]*sf_table[mac_load(6)];
  cs_factor[2][i] = cs_factor[1][i] = c_value[i]*sf_table[mac_load(6)];
  goto dispatch;
case 4:             /* no allo */
  /*-- cs_factor[2][i] = cs_factor[1][i] = cs_factor[0][i] = 0.0;  --*/
  goto dispatch;
case 5:             /* all done */
;
} /* end switch */


}
/*-------------------------------------------------------------------------*/
#define UNPACK_N(n)                                          \
    s[k]     =  cs_factor[i][k]*(load(n)-((1 << n-1) -1));   \
    s[k+64]  =  cs_factor[i][k]*(load(n)-((1 << n-1) -1));   \
    s[k+128] =  cs_factor[i][k]*(load(n)-((1 << n-1) -1));   \
    goto dispatch;
#define UNPACK_N2(n)                                             \
    mac_load_check(3*n);                                         \
    s[k]     =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    s[k+64]  =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    s[k+128] =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    goto dispatch;
#define UNPACK_N3(n)                                             \
    mac_load_check(2*n);                                         \
    s[k]     =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    s[k+64]  =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    mac_load_check(n);                                           \
    s[k+128] =  cs_factor[i][k]*(mac_load(n)-((1 << n-1) -1));   \
    goto dispatch;
#define UNPACKJ_N(n)                                         \
    tmp        =  (load(n)-((1 << n-1) -1));                 \
    s[k]       =  cs_factor[i][k]*tmp;                       \
    s[k+1]     =  cs_factor[i][k+1]*tmp;                     \
    tmp        =  (load(n)-((1 << n-1) -1));                 \
    s[k+64]    =  cs_factor[i][k]*tmp;                       \
    s[k+64+1]  =  cs_factor[i][k+1]*tmp;                     \
    tmp        =  (load(n)-((1 << n-1) -1));                 \
    s[k+128]   =  cs_factor[i][k]*tmp;                       \
    s[k+128+1] =  cs_factor[i][k+1]*tmp;                     \
    k++;       /* skip right chan dispatch */                \
    goto dispatch;
/*-------------------------------------------------------------------------*/
void CMpaDecoderL2::unpack_samp()    /* unpack samples */
{
int i, j, k;
float *s;
int n;
long tmp;

s = sample;
for(i=0;i<3;i++)  {           /* 3 groups of scale factors */
for(j=0;j<4;j++)  {
k = -1;
dispatch:   switch (samp_dispatch[++k]) {
case 0:
   s[k+128] = s[k+64] = s[k] =  0.0F;
   goto dispatch;
case 1:                     /* 3 levels grouped 5 bits */
    mac_load_check(5);
    n = mac_load(5);
    s[k]     =  cs_factor[i][k]*group3_table[n][0];
    s[k+64]  =  cs_factor[i][k]*group3_table[n][1];
    s[k+128] =  cs_factor[i][k]*group3_table[n][2];
    goto dispatch;
case 2:                     /* 5 levels grouped 7 bits */
    mac_load_check(7);
    n = mac_load(7);
    s[k]     =  cs_factor[i][k]*group5_table[n][0];
    s[k+64]  =  cs_factor[i][k]*group5_table[n][1];
    s[k+128] =  cs_factor[i][k]*group5_table[n][2];
    goto dispatch;
case 3:     UNPACK_N2(3)     /* 7 levels */
case 4:                     /* 9 levels grouped 10 bits */
    mac_load_check(10);
    n = mac_load(10);
    s[k]     =  cs_factor[i][k]*group9_table[n][0];
    s[k+64]  =  cs_factor[i][k]*group9_table[n][1];
    s[k+128] =  cs_factor[i][k]*group9_table[n][2];
    goto dispatch;
case 5:     UNPACK_N2(4)     /* 15 levels */
case 6:     UNPACK_N2(5)     /* 31 levels */
case 7:     UNPACK_N2(6)     /* 63 levels */
case 8:     UNPACK_N2(7)     /* 127 levels */
case 9:     UNPACK_N2(8)     /* 255 levels */
case 10:    UNPACK_N3(9)     /* 511 levels */
case 11:    UNPACK_N3(10)    /* 1023 levels */
case 12:    UNPACK_N3(11)    /* 2047 levels */
case 13:    UNPACK_N3(12)    /* 4095 levels */
case 14:    UNPACK_N(13)    /* 8191 levels */
case 15:    UNPACK_N(14)    /* 16383 levels */
case 16:    UNPACK_N(15)    /* 32767 levels */
case 17:    UNPACK_N(16)    /* 65535 levels */
/* -- joint ---- */
case 18+0:
    s[k+128+1] = s[k+128] = s[k+64+1] = s[k+64] = s[k+1] = s[k] =  0.0F;
    k++;       /* skip right chan dispatch */
    goto dispatch;
case 18+1:                    /* 3 levels grouped 5 bits */
    n = load(5);
    s[k]       =  cs_factor[i][k]*group3_table[n][0];
    s[k+1]     =  cs_factor[i][k+1]*group3_table[n][0];
    s[k+64]    =  cs_factor[i][k]*group3_table[n][1];
    s[k+64+1]  =  cs_factor[i][k+1]*group3_table[n][1];
    s[k+128]   =  cs_factor[i][k]*group3_table[n][2];
    s[k+128+1] =  cs_factor[i][k+1]*group3_table[n][2];
    k++;       /* skip right chan dispatch */
    goto dispatch;
case 18+2:                     /* 5 levels grouped 7 bits */
    n = load(7);
    s[k]       =  cs_factor[i][k]*group5_table[n][0];
    s[k+1]     =  cs_factor[i][k+1]*group5_table[n][0];
    s[k+64]    =  cs_factor[i][k]*group5_table[n][1];
    s[k+64+1]  =  cs_factor[i][k+1]*group5_table[n][1];
    s[k+128]   =  cs_factor[i][k]*group5_table[n][2];
    s[k+128+1] =  cs_factor[i][k+1]*group5_table[n][2];
    k++;       /* skip right chan dispatch */
    goto dispatch;
case 18+3:     UNPACKJ_N(3)     /* 7 levels */
case 18+4:                     /* 9 levels grouped 10 bits */
    n = load(10);
    s[k]      =  cs_factor[i][k]*group9_table[n][0];
    s[k+1]    =  cs_factor[i][k+1]*group9_table[n][0];
    s[k+64]   =  cs_factor[i][k]*group9_table[n][1];
    s[k+64+1] =  cs_factor[i][k+1]*group9_table[n][1];
    s[k+128]  =  cs_factor[i][k]*group9_table[n][2];
    s[k+128+1]=  cs_factor[i][k+1]*group9_table[n][2];
    k++;       /* skip right chan dispatch */
    goto dispatch;
case 18+5:     UNPACKJ_N(4)     /* 15 levels */
case 18+6:     UNPACKJ_N(5)     /* 31 levels */
case 18+7:     UNPACKJ_N(6)     /* 63 levels */
case 18+8:     UNPACKJ_N(7)     /* 127 levels */
case 18+9:     UNPACKJ_N(8)     /* 255 levels */
case 18+10:    UNPACKJ_N(9)     /* 511 levels */
case 18+11:    UNPACKJ_N(10)    /* 1023 levels */
case 18+12:    UNPACKJ_N(11)    /* 2047 levels */
case 18+13:    UNPACKJ_N(12)    /* 4095 levels */
case 18+14:    UNPACKJ_N(13)    /* 8191 levels */
case 18+15:    UNPACKJ_N(14)    /* 16383 levels */
case 18+16:    UNPACKJ_N(15)    /* 32767 levels */
case 18+17:    UNPACKJ_N(16)    /* 65535 levels */
/* -- end of dispatch -- */
case 37:
   skip(bit_skip);
case 36:
   s += 3*64;
} /* end switch */
} /* end j loop */
} /* end i loop */


}
/*-------------------------------------------------------------------------*/
