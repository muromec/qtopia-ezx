/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxabdutil.cpp,v 1.5 2005/04/28 23:40:17 ehyche Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxassert.h"
#include "protdefs.h"
#include "hxabdutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define BinWidth            0.000100

double
CalculateABD_1(int mode, TransportMode transport, 
               struct gapinfo** pGapInfo, int nGaps)
{
    double dwResult = 0;
    double dwTotalGap = 0;
    double dwTotalRead = 0;    

    for (int i = 0; i < nGaps; i++)
    {
        dwTotalRead += pGapInfo[i]->size;
        dwTotalGap += pGapInfo[i]->gap;
    }

    if (dwTotalGap)
    {
        // Calculate bandwidth: use 1000 bits = 1kbit
        dwResult = dwTotalRead / (dwTotalGap * 125.0);
    }

    return dwResult;
}

typedef struct  
{
    int value;
    int count;
} bin_item;

double
CalculateABD_2(int mode, TransportMode transport, 
               struct gapinfo** pGapInfo, int nGaps)
{
    bin_item bin[MAX_ABD_PROBPKT];

    double dwResult = 0;
    int bin_count;   
    double lower_time, upper_time;
    int max_index, max_count, max_value, cur_value;
    int i, j;
    double gap_sum;
    int sum_count;

    UINT32 dwTotalRead = 0;

    // use 100us bins to go through all the gaps
    bin_count = 0;
    for (i = 0; i < nGaps; i++) 
    {
        dwTotalRead += pGapInfo[i]->size;

	cur_value = (int)(pGapInfo[i]->gap/BinWidth);

	j = 0;
	while ((j < bin_count) && (cur_value != bin[j].value))  
        {
	    j++;
	}

	if (j == bin_count) 
        {
	    bin[bin_count].value = cur_value;
	    bin[bin_count].count = 1;
	    bin_count ++;
	}
	else 
        {
	    bin[j].count ++;
	}
    }

    // find out the biggest bin
    max_index = 0; max_count = bin[0].count;
    for (i = 1; i < bin_count; i++) 
    {
        if (bin[i].count > max_count) 
        {
	    max_count = bin[i].count;
	    max_index = i;
        }
    }

    max_value = bin[max_index].value;
    lower_time = max_value * BinWidth;
    upper_time = (max_value + 1) * BinWidth;

    // see whether the adjacent two bins also have some items
    for (i = 0; i < bin_count; i++) 
    {
	if (bin[i].value == max_value + 1) 
	    upper_time += BinWidth;
	if (bin[i].value == max_value - 1) 
	    lower_time -= BinWidth;
    }

    // average them
    gap_sum = 0;
    sum_count = 0;
    for (i = 0; i < nGaps; i++) 
    {
	if ((pGapInfo[i]->gap >= lower_time) && (pGapInfo[i]->gap <= upper_time)) 
        {
	    gap_sum += pGapInfo[i]->gap;
	    sum_count ++;
	}
    }

    if (sum_count)
    {
        dwResult = (((dwTotalRead / nGaps) * sum_count) / (gap_sum * 125));
    }

    return dwResult;
}

double
CalculateABD(int mode, TransportMode transport, 
             struct ABD_PROBPKT_INFO** pProbPktInfo, int nProbPktInfo)
{
    double      dwResult = 0;
    UINT32      ulGaps = 0;
    double      dwGap = 0;
    gapinfo*    pGaps[MAX_ABD_PROBPKT];
    int         i;

    memset(pGaps, 0, sizeof(gapinfo*) * MAX_ABD_PROBPKT);

    for (i = 1; i < nProbPktInfo; i++)
    {
        if ((pProbPktInfo[i]->seq - pProbPktInfo[i-1]->seq) == 1) 
        {
            double recvGap = (double)(pProbPktInfo[i]->recvTime - pProbPktInfo[i-1]->recvTime)/1000000.0;
            
            dwGap = recvGap;

            pGaps[ulGaps] = new gapinfo;
            pGaps[ulGaps]->gap = dwGap;
            pGaps[ulGaps]->size = pProbPktInfo[i]->dwSize;
            
            ulGaps++;
        }
    }

    if (ulGaps)
    {
        // XXX HP, 1 or 2?
        dwResult = CalculateABD_1(mode, transport, &pGaps[0], ulGaps);
    }

    for (i = 0; i < MAX_ABD_PROBPKT; i++)
    {
        if (!pGaps[i])
        {
            break;
        }
        delete pGaps[i];
    }

    return dwResult;
}
