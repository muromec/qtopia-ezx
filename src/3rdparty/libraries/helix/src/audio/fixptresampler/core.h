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
/*
 * Fixed-point sampling rate conversion library
 * Developed by Ken Cooke (kenc@real.com)
 * May 2003
 *
 * Internal state.
 */

typedef unsigned int uint;

typedef struct state_t {
	int up;
	int dn;
	int nchans;
	int nwing;
	int nhist;
	int nstart;
	int offset;
	uint phasef;
	uint stepf;
	short *histbuf;
	short *filter;
	short *rwing;
	short *lwing;
	short *stepNptr;
	int stepNfwd[3];
	int stepNbak[3];
	short *step1ptr;
	int step1fwd[3];
	int step1bak[3];
	short *(*ResampleCore)(short *, short *, short *, struct state_t *);
} state_t;

#ifdef __GCCE__
extern "C"
{
#endif
/* core functions */
short *RATCoreMono(short *pcmptr, short *pcmend, short *outptr, state_t *s);
short *RATCoreStereo(short *pcmptr, short *pcmend, short *outptr, state_t *s);
short *ARBCoreMono(short *pcmptr, short *pcmend, short *outptr, state_t *s);
short *ARBCoreStereo(short *pcmptr, short *pcmend, short *outptr, state_t *s);
#ifdef __GCCE__
}
#endif