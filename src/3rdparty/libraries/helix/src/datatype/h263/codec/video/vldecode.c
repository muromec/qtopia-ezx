/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vldecode.c,v 1.2 2004/07/09 18:32:17 hubbe Exp $
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

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "vldstate.h"
#include "vldecode.h"
#include "vldtabs.h"

// forward definition of function
void IncBsPtr2( BS_PTR * bs, int incr);


//  VLDecode - Decode bitstream
extern int pascal VLDECODE(   BS_PTR bs,          // Bitstream pointer */
                        int numbits,        // Max # bits to decode */
                        int * state,        // Initial decoder state;
                                            // returns updated state
                        int * parsed_bits,  // Returns # decoded bits */
                        int * nsym,         // Returns # decoded symbols */
                        SYMBOL sym[],       // Returns nsym symbols */
                        int maxsym          // Dimension of sym array
)
{
    register DECTABENTRY * entry;
    char msg[120];

    //printf( "Entered VLDecode: bitstream = %2x %2x %2x %2x  bitptr = %d\n",
    //        *bs.byteptr, *(bs.byteptr + 1), *(bs.byteptr + 2),
    //        *(bs.byteptr + 3), bs.bitptr);
            
    *parsed_bits = 0, *nsym = 0;
    while (*parsed_bits < numbits  &&  *nsym < maxsym) {
        entry = &dectable [selectdectab[ *state ]] [Get8Bits( bs )];
        while (entry->bits < 0) {   /* Long codeword; sym.value indicates table */
            *parsed_bits -= entry->bits;
            IncBsPtr2( &bs, -entry->bits);
            *state += entry->statechange;
            //printf("VLDecode: "); printsym( entry->sym );
            //printf("  State: "); printstate( *state ); printf("\n");
            entry = &dectable [ entry->sym.value ] [Get8Bits( bs )];
        }
        *parsed_bits += entry->bits;
        IncBsPtr2( &bs, entry->bits);
        *state += entry->statechange;
/*        {
            int input;
            printf("VLDecode: "); printsym( entry->sym );
            printf("  State: "); printstate( *state );
            printf("  Cont? (<0 to exit): ");
            scanf("%d", &input);
            if (input < 0) exit(0);
        }*/
        if (entry->sym.type  ==  SYM_EXIT) { /* Premature exit */
//#define VTEL_M261
#ifdef VTEL_M261
			/* aw talked to staffan: H.261 needs a picture startcode at the end, which comes right after the end of the bitstream.
			However, we are not able to find out the end exactly - only in 8 bit granularity.
			The EOS which we append for H.263 results in an error which we catch here. 
			Also in H.263, EOS is bytealigned and not at the end of the bitstream 
			*/
			if(entry->sym.value  ==  ILLEGAL_SYMBOL) { 
				static const SYMBOL startsymbol = {0, SYM_STARTCODE};
			   *parsed_bits = numbits;
				*state = ST_AFTER_STARTCODE	;
				//entry = &dectable [ entry->sym.value ] [Get8Bits( bs )];
				sym [(*nsym)++] = startsymbol;
				return( OK );
			} else 
#endif
			{
				if (*parsed_bits > numbits) {
					return (OUT_OF_BITS);
				} else {
					return (entry->sym.value);
				}
			}
		}
        sym [(*nsym)++] = entry->sym;
    }
    if (*nsym >= maxsym) {
        //sprintf( msg, "VLDecode: Symbol array overrun");
        //H261ErrMsg( msg );
        return( H261_ERROR );
    }
    if (*parsed_bits > numbits) {
        return (OUT_OF_BITS);
    }
    return( OK );
}

void IncBsPtr2( BS_PTR * bs, int incr)
{
    bs->bitptr += incr;
    while (bs->bitptr > 7) {
        ++(bs->byteptr);
        bs->bitptr -= 8;
    }
    while (bs->bitptr < 0) {
        --(bs->byteptr);
        bs->bitptr += 8;
    }
    return;
}
