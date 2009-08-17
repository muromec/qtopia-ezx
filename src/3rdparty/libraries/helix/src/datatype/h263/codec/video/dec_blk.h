/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dec_blk.h,v 1.3 2004/07/09 18:32:15 hubbe Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef	_INC_DEC_BLK
#define	_INC_DEC_BLK

//	for the decode_block routine in h261dec.c

#ifndef DO_ASM_DECODE_BLOCK
extern int decode_block( SYMBOL insym[], int * nextsym,
			    BLOCK_DESCR * block, SYMBOL outsym[]);
#endif

#ifdef DO_ASM_DECODE_BLOCK
extern int decode_block( SYMBOL insym[], int * nextsym,
			    BLOCK_DESCR * block, SYMBOL outsym[]);
#pragma aux decode_block parm [eax ebx ecx edx];

void zzpos_err(int zzpos)
#pragma aux zzpos_err parm [eax];
{                              
	char msg[120];
	
	sprintf(msg, "decode_block: Bitstream error, zzpos=%d",zzpos); /* Flawfinder: ignore */
	H261ErrMsg(msg);
}

void checksym_run_err(int run)
#pragma aux checksym_run_err parm [ecx];
{                              
	char msg[120];
	
	sprintf(msg, "PROGRAM ERROR: run = %d in decode_block",run); /* Flawfinder: ignore */
	H261ErrMsg(msg);
}

void checksym_type_err(int type)
#pragma aux checksym_type_err parm [ecx];
{                              
	char msg[120];

	sprintf(msg, "decode_block:  Encountered level = %d  Expected Esc Level (0b2h)", type); /* Flawfinder: ignore */
	H261ErrMsg(msg);
}
#endif
#endif
