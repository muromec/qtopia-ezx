/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dec_blk.c,v 1.4 2004/07/09 18:32:15 hubbe Exp $
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

#include "hxtypes.h"

#include "machine.h"
#ifdef FOR_MAC
#include <stdlib.h>	// For exit function
#endif

#include "dllindex.h"
#include "h261defs.h"
#include "vldstate.h"
#include "h261func.h"
#include "hvscodes.h"
#include "dec_blk.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

extern S16 Recon [QUANT_MAX - QUANT_MIN + 1] [N_SYM_INDICES];

//#define BS_ERR_EXIT(a)  a     // Terminate on bitstream error (i.e., illegal 
                                // bitstream syntax) to simplify debugging
#define BS_ERR_EXIT(a)          // Handle bitstream errors without terminating 
                                // (to cope with a real channel)

//#define CHECKSYM_ON             // To avoid unreferenced params

#ifdef CHECKSYM_ON
#include <stdio.h>
#define CHECKSYM(a)     a       // Check symbol types to verify decoder state tables
#else
#define CHECKSYM(a)             // Don't check symbol types
#endif

#ifdef CHECKSYM_ON
static int  checksym1( SYMBOL sym, int type, char routine[] )
{
    char msg[120], csym[80], ctype[80];
    
    UNREFERENCED_PARAMETER( routine );

    if (sym.type == type) {
        return (OK);
    } else {    // Not expected type
        sprintsym( sym , csym, 80);
        sprinttype( type, ctype, 80);
        SafeSprintf(msg, 120, "%s: Encountered %s  Expected %s", routine, csym, ctype);
        H261ErrMsg( msg );
        return (H261_ERROR);
    }
}
#endif


extern int decode_block( SYMBOL insym[], int * nextsym,
                            BLOCK_DESCR * block, SYMBOL outsym[])
{
    int isym, zzpos, run;
CHECKSYM(char msg[120]); /* Flawfinder: ignore */

    isym = 0;
    zzpos = 0;
    //printf("decode_block first: "); printsym( insym[*nextsym] ); printf("\n");
    while (insym[*nextsym].type != SYM_EOB) {
        if (insym[*nextsym].type == SYM_ESC_RUN) {
            run = insym[(*nextsym)++].value;
            outsym[isym].type = run;

            CHECKSYM( if (checksym1( insym[*nextsym], SYM_ESC_LEVEL, "decode_block") != OK) exit(0); )

            outsym[isym++].value = insym[(*nextsym)++].value;
        } else {
            run = insym[*nextsym].type;
            CHECKSYM( if (run < 0) {
                sprintf( msg, "PROGRAM ERROR: run = %d in decode_block", run); /* Flawfinder: ignore */
                H261ErrMsg( msg );
                return( H261_ERROR );
                } )
            outsym[isym++] = insym[(*nextsym)++];
        }
        zzpos += run + 1;
        //printf("decode_block next: "); printsym( insym[*nextsym] );
        //printf("   zzpos = %d\n", zzpos);
        if (zzpos > 64) {   // If we decoded coeff. 63, we will now have zzpos=64
            BS_ERR_EXIT( sprintf( msg, "decode_block: Bitstream error, zzpos=%d", zzpos); /* Flawfinder: ignore */
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
    }
    (*nextsym)++;   /* Advance pointer to symbol after EOB */
    block->sym = outsym;
    block->nsym = isym;
    return (isym);
}

