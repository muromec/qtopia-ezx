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

/* bitstream reader functions. No checking for end-of-bits included! */

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

/** The bitstream structure.
 *  The idea of the bitstream reader is to keep a cache word that has the machine's
 *  largest native size. This word keeps the next-to-read bits left-aligned so that
 *  on a read, one shift suffices.
 *  The cache word is only refilled if it does not contain enough bits to satisy a
 *  a read request. Because the refill only happens in multiple of 8 bits, the maximum
 *  read size that is guaranteed to be always fulfilled is the number of bits in a long
 *  minus 8 (or the number of bits in a byte).
 */

struct BITSTREAM ;

/** read nBits bits from bitstream
  * @param pBitstream the bitstream to read from
  * @param nBits the number of bits to read. nBits must be <= 32, currently.
  * @return the bits read, right-justified
  */
unsigned int readBits(struct BITSTREAM *pBitstream, int nBits) ;

/** push bits back into the bitstream.
  * This call is here to make look-ahead possible, where after reading the client
  * may realize it has read too far ahead. It is guaranteed to succeed as long as
  * you don't push more bits back than have been read in the last readBits() call.
  * @param pBitstream the bitstream to push back into
  * @param bits the bits to push back
  * @param nBits the number of bits to push back.
  * @return an error code, signalling success or failure.
  */
int unreadBits(struct BITSTREAM *pBitstream, int bits, int nBits) ;

/** byte-align the bitstream read pointer. */
void byteAlign(struct BITSTREAM *pBitstream) ;

/** allocate memory for a new bitstream structure.
  * @param ppBitstream a pointer to a bitstream handle, to be initialized on
  *        successfull return
  * @param nBits the maximum number of bits this bitstream must be able to hold.
  *        nBits must be divisible by 32.
  * @return an error code, signalling success or failure.
  * @see reverseBitstream
  */
int newBitstream(struct BITSTREAM **ppBitstream, int nBits) ;

/** free memory associated with a bitstream structure.
  * @param pBitstream a bitstream handle
  */
void deleteBitstream(struct BITSTREAM *pBitstream) ;

/** feed nbits bits to the bitstream, byte-wise.
  * @param pBitstream the bitstream into which to feed the bytes
  * @param input the input from which to read the bytes
  * @param nbits the number of bits in the input. nbits must be divisible by 32
  *  for reverseBitstream() to work.
  * @return an error code, signalling success or failure.
  * @see reverseBitstream
  */

int feedBitstream(struct BITSTREAM *pBitstream, const unsigned char *input, int nbits) ;

/** set bitstream position, relative to origin defined through feedBitstream().
  * @param pBitstream the bitstream
  * @param position the position in bits (must be multiple of 8, currently).
  * Always measured from beginning, regardless of direction.
  * @param direction the direction of reading (+1/-1)
  */

int setAtBitstream(struct BITSTREAM *pBitstream, int position, int direction) ;

/** return the number of bits left until end-of-stream.
  * @param pBitstream the bitstream
  * @return the number of bits left
  */
int bitsLeftInBitstream(struct BITSTREAM *pBitstream) ;

#ifdef __cplusplus
}
#endif

#endif /* _BITSTREAM_H_ */
