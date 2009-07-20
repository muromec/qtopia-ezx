/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bitstuff.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

typedef struct TAG_LD {
  /* bit input */
  unsigned char *rdbfr;
  unsigned char *rdptr;
  int incnt;
  int bitcnt;
} LD;

#if defined _AIX || defined _OSF1 || (defined _SOLARIS && !defined __GNUC__)
#define INLINE static inline
#else
#define	INLINE static __inline
#endif

/*
 * Initializes bit-buffer.
 * Use:
 * 	void initbuffer (char *buffer, void *bufinfo);
 * Input:
 *	buffer - pointer to the input buffer to use
 *	bufinfo - pointer to the buffer information
 * Returns:
 * 	none
 */
INLINE void initbuffer (unsigned char *buffer, void *bufinfo)
{
	LD * ld = (LD*)bufinfo;
	ld->rdptr = buffer;
	ld->rdbfr = buffer;
	ld->incnt = 0;
	ld->bitcnt = 0;
}

/*
 * Fills pre-load buffer.
 * Use:
 * 	void fillbfr (void *inst);
 * Input:
 *	inst - decoder instance
 * Returns:
 * 	none
 */
INLINE	void fillbfr (void *inst) {/* obsolete */}

/*
 * Returns next n bits (right adjusted) without advancing.
 * Use:
 *  unsigned int showbits(int n, void *inst);
 * Input:
 *	n - the number of bits to retrieve
 *	inst - decoder instance
 * Note:
 * 	len must be less than 24
 * Returns:
 *	the bit vector
 */
#ifdef _M_IX86
#pragma warning(disable:4035)
INLINE unsigned int showbits (int n, void *inst)
{
	__asm mov	eax,inst
	__asm mov	edx,/* LD */ [eax].rdptr
	__asm mov	ecx,/* LD */ [eax].incnt
	__asm mov	eax,dword ptr [edx]
	__asm mov	edx,32
	__asm bswap	eax
	__asm sub	edx,n
	__asm shl	eax,cl
	__asm mov	ecx,edx
	__asm shr	eax,cl
}
#else
/* I guess it should be portable: */
INLINE unsigned int showbits (int n, void *inst)
{
	LD * ld = (LD*)inst;
	unsigned char *p = ld->rdptr;
	unsigned int a, c = ld->incnt;
	/* load in big-Endian order: */
	a = (unsigned int)(p[0]<<24) + (p[1]<<16) + (p[2]<<8) + (p[3]);
	return (a << c) >> (32-n);
}
#endif

/*
 * Advance by n bits.
 * Use:
 *	void flushbits(int n, void *inst);
 * Input:
 *	n - the number of bits to throw away
 *	inst - decoder instance
 * Returns:
 *	none
 */
INLINE void flushbits (int n, void *inst)
{
	LD * ld = (LD*)inst;
 	ld->incnt += n;
	ld->bitcnt+= n;
	ld->rdptr += ld->incnt >> 3;
	ld->incnt &= 0x07;
}

/*
 * Returns next n bits (right adjusted)
 * Use:
 *  unsigned int getbits(int n, void *inst);
 * Input:
 *	n - the number of bits to retrieve
 *	inst - decoder instance
 * Note:
 * 	len must be less than 24
 * Returns:
 *	the bit vector
 */
INLINE unsigned int getbits(int n, void *inst)
{
	unsigned int l = showbits(n, inst);
    flushbits(n, inst);
	return l;
}

/*
 * Returns next bit from the bitstream.
 * Use:
 *	unsigned int getbits1 (void * inst);
 * Input:
 *	inst - decoder instance
 * Returns:
 *	bit value
 */
#if 1
INLINE unsigned int getbits1 (void * inst)
{
	LD * ld = (LD*)inst;
	unsigned int a = ld->rdptr[0];
	unsigned int c = ld->incnt + 1;
	ld->bitcnt ++;
	ld->incnt = c & 7;
	ld->rdptr += c >> 3;
	return (a >> (8 - c)) & 1;
}
#else
/* 'lazy' version */
INLINE unsigned int getbits1 (void * inst)
{
	return getbits(1, inst);
}
#endif

/*
 * Returns current byte number.
 * Use:
 *	unsigned int getbytes (void *bufinfo);
 *	bufinfo - pointer to the buffer information
 * Returns:
 *	none
 */
INLINE unsigned int getbytes (void *bufinfo)
{
	LD * ld = (LD*)ld;
 	return ld->bitcnt >> 3;
}

/*
 * Seek to a certain position in the bitstream:
 * Use:
 *	unsigned char *gotoByte (int byteInPacket, void *bufinfo);
 * Input:
 *	byteInPacket - target byte position
 *	bufinfo - pointer to the buffer information
 * Returns
 *  pointer to the requested position in the bistream
 */
INLINE unsigned char *gotoByte (int byteInPacket, void *bufinfo)
{
	LD * ld = (LD*)bufinfo;
	unsigned char * newPtrLocation = ld->rdbfr + byteInPacket;
	ld->rdptr = newPtrLocation;
	ld->bitcnt = byteInPacket << 3;
	ld->incnt= 0;
	return newPtrLocation;
}

