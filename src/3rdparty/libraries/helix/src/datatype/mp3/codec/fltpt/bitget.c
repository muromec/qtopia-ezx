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

#include "bitget.h"




/*====================================================================*/
/* get bits from bitstream in endian independent way */

/*------------- initialize bit getter -------------*/
void bitget_init(BITDAT *bitdat, unsigned char *buf)
{
bitdat->bs_ptr0 = bitdat->bs_ptr = buf;
bitdat->bits = 0;
bitdat->bitbuf = 0;
}
/*------------- initialize bit getter -------------*/
void bitget_init_end(BITDAT *bitdat, unsigned char *buf_end)
{
bitdat->bs_ptr_end = buf_end;
}
/*------------- data overrun test  -------------*/
int bitget_overrun(BITDAT *bitdat)
{
return  (bitdat->bs_ptr > bitdat->bs_ptr_end);
}
/*------------- get n bits from bitstream -------------*/
int bitget_bits_used(BITDAT *bitdat)
{
int n;      /* compute bits used from last init call */
n = ((bitdat->bs_ptr-bitdat->bs_ptr0) << 3) - bitdat->bits;
return n;
}
/*------------- check for n bits in bitbuf -------------*/
void bitget_check(BITDAT *bitdat, int n)
{
if( bitdat->bits < n ) {
    while( bitdat->bits <= 24 ) {
         bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++;
         bitdat->bits += 8;
    }
}
}
/*------------- get n bits from bitstream -------------*/
unsigned int bitget(BITDAT *bitdat, int n)
{
unsigned int x;
if( bitdat->bits < n ) {           /* refill bit buf if necessary */
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
/*------------- get 1 bit from bitstream -------------*/
unsigned int bitget_1bit(BITDAT *bitdat)
{
unsigned int x;
if( bitdat->bits <= 0 ) {           /* refill bit buf if necessary */
          while( bitdat->bits <= 24 ) {
             bitdat->bitbuf = (bitdat->bitbuf << 8) | *bitdat->bs_ptr++;
             bitdat->bits += 8;
          }
}
bitdat->bits--;
x = bitdat->bitbuf >> bitdat->bits;
bitdat->bitbuf -= x << bitdat->bits;
return x;
}
/*====================================================================*/

