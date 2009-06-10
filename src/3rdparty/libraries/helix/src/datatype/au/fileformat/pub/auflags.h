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

/* defines for the AU file format */

#ifndef _AUFLAGS_H_
#define _AUFLAGS_H_

#define SUN_MAGIC   	0x2e736e64      /* Really '.snd' */
#define SUN_INV_MAGIC   0x646e732e      /* '.snd' upside-down */
#define DEC_MAGIC   	0x2e736400      /* Really '\0ds.' (for DEC) */
#define DEC_INV_MAGIC   0x0064732e      /* '\0ds.' upside-down */
#define SUN_HDRSIZE 	24          /* Size of minimal header */
#define SUN_UNSPEC  	((unsigned)(~0))    /* Unspecified data size */
#define SUN_ULAW    1           /* u-law encoding */
#define SUN_LIN_8   2           /* Linear 8 bits */
#define SUN_LIN_16  3           /* Linear 16 bits */
#define SUN_LIN_24  4           /* Linear 24 bits */
#define SUN_LIN_32  5           /* Linear 32 bits */
#define SUN_FLOAT   6           /* IEEE FP 32 bits */
#define SUN_DOUBLE  7           /* IEEE FP 64 bits */
#define SUN_G721    23          /* CCITT G.721 4-bits ADPCM - G726-32 80 byte pkts (20ms 8khz) */  
#define SUN_G722    24		/* CCITT g.722 ADPCM */
#define SUN_G726_24 25          /* CCITT G.723 3-bits ADPCM - G726-24 60 byte pkts (20ms 8khz) */
#define SUN_G726_40 26          /* CCITT G.723 5-bits ADPCM - G726-40 100 byte pkts (20ms 8khz) */
#define SUN_ALAW_8  27		/* 8-bit ISDN A-law */

#endif
