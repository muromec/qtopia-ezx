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
#include "mpadecl1.h"

#include "mpalow.h"     // low level extern C prototypes
///////////////////////////////////////////////////////////////////////////////
// Static Data:
///////////////////////////////////////////////////////////////////////////////
static const int sr_table_L1[8] =
    { 22050, 24000, 16000, 1,
      44100, 48000, 32000, 1 };

static const int  out_chans_L1[5] = { 1, 2, 1, 1, 1 };

static const int look_joint_L1[16] = {  /* lookup stereo sb's by mode+ext */
64, 64, 64, 64,         /* stereo */
2*4, 2*8, 2*12, 2*16,   /* joint */
64, 64, 64, 64,         /* dual */
32, 32, 32, 32,         /* mono */
};

static const int bat_bit_master_L1[] = {
0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

static const SBT_FUNCTION  sbt_table_L1[2][3][5] = {
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
CMpaDecoderL1::CMpaDecoderL1()
 :  CMpaDecoder()
{
    table_init();
}

CMpaDecoderL1::~CMpaDecoderL1()
{
}

//IN_OUT CMpaDecoderL1::audio_decode(unsigned char *bs, unsigned char *pcm, int size)
//{
//return audio_decode(bs, pcm);
//}

#ifdef REFORMAT
IN_OUT  CMpaDecoderL1::audio_decode_reformat(unsigned char *bs,
                         unsigned char *bs_out)
{
    IN_OUT x;
    x.in_bytes = 0;     // signal fail, not relevant Layer 1
    x.out_bytes = 0;
    return x;
}
#endif
//=======================================================================
int CMpaDecoderL1::audio_decode_init(MPEG_HEAD *h,
                                  int framebytes_arg,
                                  int reduction_code,
                                  int transform_code,
                                  int convert_code,
                                  int freq_limit,
                                  int conceal_enable)
{
int i,k;
int samprate;
int limit;
int bit_code;

/* check if code handles */
if( h->option != 3 )  return 0;  /* layer I only */

#ifndef REDUCTION
    reduction_code = 0;
#endif
#ifndef EIGHT_BIT
    convert_code = 0;
#endif

m_bMpeg1 = h->id;
m_nSampsPerFrame = 384;

transform_code = transform_code;    /* not used, asm compatability */

bit_code = 0;
if( convert_code & 8 ) bit_code = 1;
convert_code = convert_code & 3;    /* higher bits used by dec8 freq cvt */
if( reduction_code < 0 ) reduction_code = 0;
if( reduction_code > 2 ) reduction_code = 2;
if( freq_limit < 1000 ) freq_limit = 1000;


framebytes = framebytes_arg;
/* check if code handles */
if( h->option != 3 ) return 0;         /* layer I only */

nbatL1 = 32;
max_sb = nbatL1;
/*----- compute nsb_limit --------*/
samprate = sr_table_L1[4*h->id + h->sr_index];
nsb_limit = (freq_limit*64L + samprate/2)/samprate;   /*- caller limit -*/
/*---- limit = 0.94*(32>>reduction_code);  ----*/
limit = (32>>reduction_code);
if( limit > 8 ) limit--;
if( nsb_limit > limit )  nsb_limit = limit;
if( nsb_limit > max_sb ) nsb_limit = max_sb;

outvalues = 384 >> reduction_code;
if( h->mode != 3 ) {         /* adjust for 2 channel modes */
          nbatL1    *= 2;
          max_sb    *= 2;
          nsb_limit *= 2;
}

/* set sbt function */
k = 1 + convert_code;
if( h->mode == 3 ) {
          k = 0;
          }
sbt = sbt_table_L1[bit_code][reduction_code][k];
outvalues *= out_chans_L1[k];

if( bit_code ) outbytes = outvalues;
else           outbytes = sizeof(short)*outvalues;
decinfo.channels  = out_chans_L1[k];
decinfo.outvalues = outvalues;
decinfo.samprate =  samprate >> reduction_code;
if( bit_code ) decinfo.bits     = 8;
else           decinfo.bits     = sizeof(short)*8;
decinfo.framebytes = framebytes;
decinfo.type = 0;

/* clear sample buffer, unused sub bands must be 0 */
for(i=0;i<768;i++) sample[i] = 0.0F;

return 1;
}
//=========================================================================
IN_OUT CMpaDecoderL1::audio_decode(unsigned char *bs, unsigned char *pcm, int size)
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
pad = (load(1)) << 2;
load(1);            /* skip to mode */
stereo_sb = look_joint_L1[load(4)];
if( prot ) load(4);           /* skip to data */
else       load(20);          /* skip crc */

unpack_baL1();        /* unpack bit allocation */
unpack_sfL1();        /* unpack scale factor */
unpack_sampL1();      /* unpack samples */

sbt(sample, pcm, 12, vbuf, vb_ptr);
/*-----------*/
in_out.in_bytes = framebytes + pad;
in_out.out_bytes = outbytes;

return in_out;
}
///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////
void CMpaDecoderL1::table_init()
{
int i;
int step;

/*--  c_values (dequant) --*/
for(step=4, i=1; i<16; i++, step<<=1)
                   look_c_valueL1[i] = (float)(2.0/(step-1));

/*--  scale factor table, scale by 32768 for 16 pcm output  --*/
for(i=0;i<64;i++) sf_table[i] = (float)(32768.0*2.0*pow(2.0,-i/3.0));


}
/*---------------------------------------------------------*/
/*---------------------------------------------------------*/
/*------------- bit getter --------------------------------*/
/*---------------------------------------------------------*/
void CMpaDecoderL1::load_init( unsigned char *buf)
{
bs_ptr = buf;
bits = 0;
bitbuf = 0;
}
/*------------- get n bits from bitstream -------------*/
int CMpaDecoderL1::load( int n)
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
void CMpaDecoderL1::skip( int n)
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
/*======================================================================*/
void CMpaDecoderL1::unpack_baL1()
{
int j;
int nstereo;

bit_skip = 0;
nstereo = stereo_sb;

for(j=0;j<nbatL1; j++) {
  mac_load_check(4);
  ballo[j] = samp_dispatch[j] =  mac_load(4);
  if( j >= nsb_limit )  bit_skip += bat_bit_master_L1[samp_dispatch[j]];
  c_value[j] = look_c_valueL1[samp_dispatch[j]];
  if( --nstereo < 0 ) {
      ballo[j+1] = ballo[j];
      samp_dispatch[j] += 15;  /* flag as joint */
      samp_dispatch[j+1] = samp_dispatch[j];  /* flag for sf */
      c_value[j+1] = c_value[j];
      j++;
  }
}
/*-- terminate with bit skip and end --*/
samp_dispatch[nsb_limit] = 31;
samp_dispatch[j]         = 30;
}
/*-------------------------------------------------------------------------*/
void CMpaDecoderL1::unpack_sfL1(void)    /* unpack scale factor */
{                          /* combine dequant and scale factors */
int i;

for(i=0;i<nbatL1;i++) {
  if( ballo[i] ) {
    mac_load_check(6);
    cs_factorL1[i] = c_value[i]*sf_table[mac_load(6)];
  }
}
/*-- done --*/
}
/*-------------------------------------------------------------------------*/
#define UNPACKL1_N(n)                                        \
    s[k]     =  cs_factorL1[k]*(load(n)-((1 << n-1) -1));    \
    goto dispatch;
#define UNPACKL1J_N(n)                                       \
    tmp        =  (load(n)-((1 << n-1) -1));                 \
    s[k]       =  cs_factorL1[k]*tmp;                        \
    s[k+1]     =  cs_factorL1[k+1]*tmp;                      \
    k++;                                                     \
    goto dispatch;
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
void CMpaDecoderL1::unpack_sampL1()    /* unpack samples */
{
int j, k;
float *s;
long tmp;

s = sample;
for(j=0;j<12;j++)  {
k = -1;
dispatch:   switch (samp_dispatch[++k]) {
case 0:
    s[k]     =  0.0F;
    goto dispatch;
case 1:     UNPACKL1_N(2)     /*  3 levels */
case 2:     UNPACKL1_N(3)     /*  7 levels */
case 3:     UNPACKL1_N(4)     /* 15 levels */
case 4:     UNPACKL1_N(5)     /* 31 levels */
case 5:     UNPACKL1_N(6)     /* 63 levels */
case 6:     UNPACKL1_N(7)     /* 127 levels */
case 7:     UNPACKL1_N(8)     /* 255 levels */
case 8:     UNPACKL1_N(9)     /* 511 levels */
case 9:     UNPACKL1_N(10)    /* 1023 levels */
case 10:    UNPACKL1_N(11)    /* 2047 levels */
case 11:    UNPACKL1_N(12)    /* 4095 levels */
case 12:    UNPACKL1_N(13)    /* 8191 levels */
case 13:    UNPACKL1_N(14)    /* 16383 levels */
case 14:    UNPACKL1_N(15)    /* 32767 levels */
/* -- joint ---- */
case 15+0:
    s[k+1] = s[k] =  0.0F;
    k++;       /* skip right chan dispatch */
    goto dispatch;
/* -- joint ---- */
case 15+1:     UNPACKL1J_N(2)     /*  3 levels */
case 15+2:     UNPACKL1J_N(3)     /*  7 levels */
case 15+3:     UNPACKL1J_N(4)     /* 15 levels */
case 15+4:     UNPACKL1J_N(5)     /* 31 levels */
case 15+5:     UNPACKL1J_N(6)     /* 63 levels */
case 15+6:     UNPACKL1J_N(7)     /* 127 levels */
case 15+7:     UNPACKL1J_N(8)     /* 255 levels */
case 15+8:     UNPACKL1J_N(9)     /* 511 levels */
case 15+9:     UNPACKL1J_N(10)    /* 1023 levels */
case 15+10:    UNPACKL1J_N(11)    /* 2047 levels */
case 15+11:    UNPACKL1J_N(12)    /* 4095 levels */
case 15+12:    UNPACKL1J_N(13)     /* 8191 levels */
case 15+13:    UNPACKL1J_N(14)     /* 16383 levels */
case 15+14:    UNPACKL1J_N(15)     /* 32767 levels */

/* -- end of dispatch -- */
case 31:
   skip(bit_skip);
case 30:
   s += 64;
} /* end switch */
} /* end j loop */

/*-- done --*/
}
/*-------------------------------------------------------------------*/
