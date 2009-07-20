/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vldecode.h,v 1.3 2004/07/09 18:32:17 hubbe Exp $
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

#ifndef _INC_VLDECODE
#define _INC_VLDECODE

#ifndef DO_ASM_VLDECODE

typedef struct {
    SYMBOL		sym;
    S8			statechange;
    S8			bits;
	} DECTABENTRY;

#define DECTABBITS          8       /* Look up DECTABBITS bits at a time */
#define DECTABSIZE          256     /* 2 ** DECTABBITS */

#define MAX_STRING_VLD          (DECTABBITS + 4)

extern DECTABENTRY dectable[][DECTABSIZE];
extern U8 selectdectab[];

//extern void pascal INCBSPTR( BS_PTR * bs, int incr);
//#pragma aux INCBSPTR parm [eax ebx ecx edx];

#endif

#ifdef DO_ASM_VLDECODE

int overrun_err(void)
{
	char msg[120]; /* Flawfinder: ignore */
	
	sprintf(msg, "VLDecode: Symbol array overrun"); /* Flawfinder: ignore */
	H261ErrMsg(msg);
	return (H261_ERROR);
}

#endif

extern int pascal VLDECODE	(   BS_PTR bs,          
                        int numbits,        
                        int * state,        
                        int * parsed_bits,  
                        int * nsym,         
                        SYMBOL sym[],      
                        int maxsym          
							);
		
#endif
