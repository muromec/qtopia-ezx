/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clip1.c,v 1.2 2004/07/09 18:32:15 hubbe Exp $
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
//#include <string.h>
//#include <stdlib.h>
//#include <math.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "clip.h"

#define FRACBITS        6   /* Fractional bits in IDCT computation */
#define PIXEL_MIN       0
#define PIXEL_MAX       255
#define CLIPMARGIN      300
#define CLIPMIN         (PIXEL_MIN - CLIPMARGIN)
#define CLIPMAX         (PIXEL_MAX + CLIPMARGIN)
extern PIXEL   clip[(CLIPMAX-CLIPMIN+1)];

extern void idct2_clip(PIXEL x[], int xdim, S32 idct_out[8][4], int idct_class)
{
    int     i, temp;

#ifdef LITTLE_ENDIAN
    int     temp1, temp2, temp3;
	PIXEL	hi_clip0, hi_clip1, hi_clip2, hi_clip3;
	PIXEL	lo_clip0, lo_clip1, lo_clip2, lo_clip3;
	U32		*y;


	switch (idct_class)
	{
	//	DC only case	
	case DC_ONLY:
		{
			temp = idct_out[0][0];
			lo_clip0 = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
			hi_clip0 = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];

			temp1 = 0x0000ffff & (int)hi_clip0;
			temp1 = temp1 << 8 | lo_clip0;
			temp2 = temp1 << 16 | temp1; 

			temp1 = 0x0000ffff & (int)lo_clip0;
			temp1 = temp1 << 8 | hi_clip0;
			temp3 = temp1 << 16 | temp1;
			
			y = (U32 *)&x[0];
			
			for (i = 0; i < 8; i++)
				{
					y[i*xdim/4] = temp2;
					y[i*xdim/4+1] = temp3;
				}
			break;
		}
	//	DC + 1 horizontal AC case
	case DC_AC_H:
		{
			temp = idct_out[0][0];
			lo_clip0 = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
			hi_clip0 = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];

			temp = idct_out[0][1];
			lo_clip1 = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
			hi_clip1 = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];

			temp = idct_out[0][2];
			lo_clip2 = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
			hi_clip2 = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];

			temp = idct_out[0][3];
			lo_clip3 = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
			hi_clip3 = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];

			temp1 = 0x0000ffff & (int)hi_clip0;
			temp1 = temp1 << 8 | lo_clip0; 

			temp2 = 0x0000ffff & (int)hi_clip1;
			temp2 = temp2 << 8 | lo_clip1;
			temp2 = temp2 << 16 | temp1; 

			temp3 = 0x0000ffff & (int)lo_clip2;
			temp3 = temp3 << 8 | hi_clip2;

			temp = 0x0000ffff & (int)lo_clip3;
			temp = temp << 8 | hi_clip3;
			temp3 = temp3 << 16 | temp; 
			
			y = (U32 *)&x[0];
			
			for (i = 0; i < 8; i++)
				{
					y[i*xdim/4] = temp2;
					y[i*xdim/4+1] = temp3;
				}
			break;
		}		
	case GENERAL:
		{
    	for (i = 0; i < 8; i++) 
    		{
        		temp = idct_out[i][0];
        		x[i*xdim] = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        		x[i*xdim+1] = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];
        		temp = idct_out[i][1];
        		x[i*xdim+2] = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        		x[i*xdim+3] = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];
        		temp = idct_out[i][2];
        		x[i*xdim+7] = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        		x[i*xdim+6] = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];
        		temp = idct_out[i][3];
        		x[i*xdim+5] = clip[-CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        		x[i*xdim+4] = clip[-CLIPMIN + (temp >> (FRACBITS + 16))];
	        }
		break;	        
		}
		default:
			break;
	}			
#else

    for (i = 0; i < 8; i++)
        {
            int     j;

            // Notice the funky casts of idct_out, which needs to be treated
            // as an S16 [8][8].
            for (j = 0; j < 4; j++) {
                temp = ((S16 (*)[8]) idct_out)[i][j] >> FRACBITS;
                x[i*xdim+j] = clip[-CLIPMIN + temp];
                temp = ((S16 (*)[8]) idct_out)[i][j+4] >> FRACBITS;
                x[i*xdim+7-j] = clip[-CLIPMIN + temp];
            }
        }
#endif
}
