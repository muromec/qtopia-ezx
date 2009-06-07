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

#include "l3.h"


#ifdef _MSC_VER
#pragma warning(disable: 4505)
#endif

#ifdef __cplusplus
extern "C" {
#endif
void unpack_huff(int xy[][2], int n, int ntable, BITDAT *bitdat);
int unpack_huff_quad(int vwxy[][4], int n, int nbits, int ntable, BITDAT *bitdat);
#ifdef __cplusplus
}
#endif


/*===============================================================*/

/* max bits required for any lookup - change if htable changes */
/* quad required 10 bit w/signs  must have (MAXBITS+2) >= 10   */
#define MAXBITS 9

static const HUFF_ELEMENT huff_table_0[] = { 0, 0, 0, 64 }; /* dummy must not use */

#include "htable.h"

/*-- 6 bit lookup (purgebits, value) --*/
static const unsigned char quad_table_a[][2] = {	/* JR - made const */
6, 11,  6, 15,  6, 13,  6, 14,  6,  7,  6,  5,  5,  9,
5,  9,  5,  6,  5,  6,  5,  3,  5,  3,  5, 10,  5, 10,  
5, 12,  5, 12,  4,  2,  4,  2,  4,  2,  4,  2,  4,  1,  
4,  1,  4,  1,  4,  1,  4,  4,  4,  4,  4,  4,  4,  4,  
4,  8,  4,  8,  4,  8,  4,  8,  1,  0,  1,  0,  1,  0,  
1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  
1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  
1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  
1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  
1,  0,  
};

typedef struct {
    const HUFF_ELEMENT* table;	
    int linbits;
    int ncase;
} HUFF_SETUP;

#define no_bits       0
#define one_shot      1
#define no_linbits    2
#define have_linbits  3
#define quad_a        4
#define quad_b        5


/* JR - made const */
static const HUFF_SETUP table_look[] = {
huff_table_0,  0,  no_bits,
huff_table_1,  0,  one_shot,
huff_table_2,  0,  one_shot,
huff_table_3,  0,  one_shot,
huff_table_0,  0,  no_bits,
huff_table_5,  0,  one_shot,
huff_table_6,  0,  one_shot,
huff_table_7,  0,  no_linbits,
huff_table_8,  0,  no_linbits,
huff_table_9,  0,  no_linbits,
huff_table_10, 0,  no_linbits,
huff_table_11, 0,  no_linbits,
huff_table_12, 0,  no_linbits,
huff_table_13, 0,  no_linbits,
huff_table_0,  0,  no_bits,
huff_table_15, 0,  no_linbits,
huff_table_16, 1,  have_linbits,
huff_table_16, 2,  have_linbits,
huff_table_16, 3,  have_linbits,
huff_table_16, 4,  have_linbits,
huff_table_16, 6,  have_linbits,
huff_table_16, 8,  have_linbits,
huff_table_16, 10, have_linbits,
huff_table_16, 13, have_linbits,
huff_table_24, 4,  have_linbits,
huff_table_24, 5,  have_linbits,
huff_table_24, 6,  have_linbits,
huff_table_24, 7,  have_linbits,
huff_table_24, 8,  have_linbits,
huff_table_24, 9,  have_linbits,
huff_table_24, 11, have_linbits,
huff_table_24, 13, have_linbits,
huff_table_0,  0,  quad_a,
huff_table_0,  0,  quad_b,
};
/*========================================================*/

/*------------- get n bits from bitstream -------------*/
static unsigned int bitget(BITDAT *bitdat, int n)
{
unsigned int x;
if( bitdat->bits < n ) {    /* refill bit buf if necessary */
          while( bitdat->bits <= 24 ) {
             bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++;
             bitdat->bits += 8;
          }
}
bitdat->bits -= n;
x = bitdat->bitbuf >> bitdat->bits;
bitdat->bitbuf -= x << bitdat->bits;
return x;
}

/*----- get n bits  - checks for n+2 avail bits (linbits+sign) -----*/
static unsigned int bitget_lb(BITDAT *bitdat, int n)
{
unsigned int x;
if( bitdat->bits < (n+2) ) {    /* refill bit buf if necessary */
          while( bitdat->bits <= 24 ) {
             bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++;
             bitdat->bits += 8;
          }
}
bitdat->bits -= n;
x = bitdat->bitbuf >> bitdat->bits;
bitdat->bitbuf -= x << bitdat->bits;
return x;
}




/*------------- get n bits but DO NOT remove from bitstream --*/
static unsigned int bitget2(BITDAT *bitdat, int n)
{
unsigned int x;
if( bitdat->bits < (MAXBITS+2) ) {    /* refill bit buf if necessary */
          while( bitdat->bits <= 24 ) {
             bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++;
             bitdat->bits += 8;
          }
}
x = bitdat->bitbuf >> (bitdat->bits-n);
return x;
}
/*------------- remove n bits from bitstream ---------*/
static void bitget_purge(BITDAT *bitdat, int n)
{
bitdat->bits -= n;
bitdat->bitbuf -= (bitdat->bitbuf >> bitdat->bits) << bitdat->bits;
}
/*------------- get 1 bit from bitstream NO CHECK -------------*/
static unsigned int bitget_1bit(BITDAT *bitdat)
{
unsigned int x;
bitdat->bits--;
x = bitdat->bitbuf >> bitdat->bits;
bitdat->bitbuf -= x << bitdat->bits;
return x;
}
/*========================================================*/
/*========================================================*/
#define mac_bitget_check(bitdat, n)                 \
if( bitdat->bits < (n) ) {                   \
    while( bitdat->bits <= 24 ) {            \
        bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++; \
        bitdat->bits += 8;                   \
    }                                       \
}
/*---------------------------------------------------------*/
#define mac_bitget2(bitdat, n)                \
    (bitdat->bitbuf >> (bitdat->bits-n));
/*---------------------------------------------------------*/
#define mac_bitget(bitdat, n)                \
       ( bitdat->bits -= n,           \
         code  = bitdat->bitbuf >> bitdat->bits,     \
         bitdat->bitbuf -= code << bitdat->bits,     \
         code )
/*---------------------------------------------------------*/
#define mac_bitget_purge(bitdat, n)                \
    bitdat->bits -= n,                    \
    bitdat->bitbuf -= (bitdat->bitbuf >> bitdat->bits) << bitdat->bits;
/*---------------------------------------------------------*/
#define mac_bitget_1bit(bitdat)                         \
       ( bitdat->bits--,                           \
         code  = bitdat->bitbuf >> bitdat->bits,    \
         bitdat->bitbuf -= code << bitdat->bits,  \
         code )
/*========================================================*/
/*========================================================*/
void unpack_huff(int xy[][2], int n, int ntable, BITDAT *bitdat)
{
int i;
const HUFF_ELEMENT *t;	/* JR - made const ptr */
const HUFF_ELEMENT *t0;	/* JR - made const ptr */
int linbits;
int bits;
int code;
int x, y;

if( n <= 0 ) return;
n = n >> 1;         /* huff in pairs */
/*-------------*/
t0      = table_look[ntable].table;
linbits = table_look[ntable].linbits;
switch (table_look[ntable].ncase) {
default:
/*------------------------------------------*/
case no_bits:       /*- table 0, no data, x=y=0--*/
for(i=0;i<n;i++) {
    xy[i][0] = 0;
    xy[i][1] = 0;
}
return;
/*------------------------------------------*/
case one_shot:       /*- single lookup, no escapes -*/
for(i=0;i<n;i++) {
    mac_bitget_check(bitdat, (MAXBITS+2));
    bits = t0[0].b.signbits;
    code = mac_bitget2(bitdat, bits);
    mac_bitget_purge(bitdat, t0[1+code].b.purgebits);
    x = t0[1+code].b.x;
    y = t0[1+code].b.y;
    if( x ) if( mac_bitget_1bit(bitdat) ) x = -x;
    if( y ) if( mac_bitget_1bit(bitdat) ) y = -y;
    xy[i][0] = x;
    xy[i][1] = y;
    if( bitdat->bs_ptr > bitdat->bs_ptr_end ) break;  // bad data protect
}
return;
/*------------------------------------------*/
case no_linbits:
for(i=0;i<n;i++) {
    t = t0;
    for(;;) {
        mac_bitget_check(bitdat, (MAXBITS+2));
        bits = t[0].b.signbits;
        code = mac_bitget2(bitdat, bits);
        if( t[1+code].b.purgebits ) break;
        t += t[1+code].ptr;  /* ptr include 1+code */
        mac_bitget_purge(bitdat, bits);
    }
    mac_bitget_purge(bitdat, t[1+code].b.purgebits);
    x = t[1+code].b.x;
    y = t[1+code].b.y;
    if( x ) if( mac_bitget_1bit(bitdat) ) x = -x;
    if( y ) if( mac_bitget_1bit(bitdat) ) y = -y;
    xy[i][0] = x;
    xy[i][1] = y;
    if( bitdat->bs_ptr > bitdat->bs_ptr_end ) break;  // bad data protect
}
return;
/*------------------------------------------*/
case have_linbits:
for(i=0;i<n;i++) {
    t = t0;
    for(;;) {
        bits = t[0].b.signbits;
        code = bitget2(bitdat, bits);
        if( t[1+code].b.purgebits ) break;
        t += t[1+code].ptr;  /* ptr includes 1+code */
        mac_bitget_purge(bitdat, bits);
    }
    mac_bitget_purge(bitdat, t[1+code].b.purgebits);
    x = t[1+code].b.x;
    y = t[1+code].b.y;
    if( x == 15 ) x += bitget_lb(bitdat, linbits);
    if( x ) if( mac_bitget_1bit(bitdat) ) x = -x;
    if( y == 15 ) y += bitget_lb(bitdat, linbits);
    if( y ) if( mac_bitget_1bit(bitdat) ) y = -y;
    xy[i][0] = x;
    xy[i][1] = y;
    if( bitdat->bs_ptr > bitdat->bs_ptr_end ) break;  // bad data protect
}
return;
}  /*--- end switch ---*/

}
/*==========================================================*/
int unpack_huff_quad(int vwxy[][4], int n, int nbits, int ntable, BITDAT *bitdat)
{
int i;
int code;
int x, y, v, w;
int tmp;
int i_non_zero, tmp_nz;

tmp_nz = 15;
i_non_zero = -1;

n = n >> 2;     /* huff in quads */

if( ntable ) goto case_quad_b;

/* case_quad_a: */
for(i=0;i<n;i++) {
    if( nbits <= 0 ) break;
    mac_bitget_check(bitdat, 10);
    code = mac_bitget2(bitdat, 6);
    nbits -= quad_table_a[code][0];
    mac_bitget_purge(bitdat, quad_table_a[code][0]);
    tmp = quad_table_a[code][1];
if( tmp ) { i_non_zero = i; tmp_nz = tmp; }
    v = (tmp>>3) & 1;
    w = (tmp>>2) & 1;
    x = (tmp>>1) & 1;
    y =  tmp & 1;
    if( v ) { if( mac_bitget_1bit(bitdat) ) v = -v; nbits--; }
    if( w ) { if( mac_bitget_1bit(bitdat) ) w = -w; nbits--; }
    if( x ) { if( mac_bitget_1bit(bitdat) ) x = -x; nbits--; }
    if( y ) { if( mac_bitget_1bit(bitdat) ) y = -y; nbits--; }
    vwxy[i][0] = v;
    vwxy[i][1] = w;
    vwxy[i][2] = x;
    vwxy[i][3] = y;
    if( bitdat->bs_ptr > bitdat->bs_ptr_end ) break;  // bad data protect
}
if( nbits < 0 ) {
    i--;
    vwxy[i][0] = 0;
    vwxy[i][1] = 0;
    vwxy[i][2] = 0;
    vwxy[i][3] = 0;
}

i_non_zero = (i_non_zero+1) << 2;

if( (tmp_nz & 3) == 0 ) i_non_zero -= 2;

return i_non_zero;

/*--------------------*/
case_quad_b:
for(i=0;i<n;i++) {
    if( nbits < 4 ) break;
    nbits -= 4;
    mac_bitget_check(bitdat, 8);
    tmp = mac_bitget(bitdat, 4) ^ 15;   /* one's complement of bitstream */
    if( tmp ) { i_non_zero = i; tmp_nz = tmp; }
    v = (tmp>>3) & 1;
    w = (tmp>>2) & 1;
    x = (tmp>>1) & 1;
    y =  tmp & 1;
    if( v ) { if( mac_bitget_1bit(bitdat) ) v = -v; nbits--; }
    if( w ) { if( mac_bitget_1bit(bitdat) ) w = -w; nbits--; }
    if( x ) { if( mac_bitget_1bit(bitdat) ) x = -x; nbits--; }
    if( y ) { if( mac_bitget_1bit(bitdat) ) y = -y; nbits--; }
    vwxy[i][0] = v;
    vwxy[i][1] = w;
    vwxy[i][2] = x;
    vwxy[i][3] = y;
    if( bitdat->bs_ptr > bitdat->bs_ptr_end ) break;  // bad data protect
}
if( nbits < 0 ) {
    i--;
    vwxy[i][0] = 0;
    vwxy[i][1] = 0;
    vwxy[i][2] = 0;
    vwxy[i][3] = 0;
}

i_non_zero = (i_non_zero+1) << 2;

if( (tmp_nz & 3) == 0 ) i_non_zero -= 2;

return i_non_zero;  /* return non-zero sample (to nearest pair) */

}
/*-----------------------------------------------------*/
