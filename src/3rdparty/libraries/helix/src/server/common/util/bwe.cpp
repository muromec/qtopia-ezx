/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bwe.cpp,v 1.3 2004/06/02 17:18:29 tmarshall Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"

#include "debug.h"

#include "bwe.h"

void
fill_bwe(BW_encoding* bwe, int bw, char* compression_code)
{
    bwe->bw = bw;
    memcpy(bwe->comp, compression_code, 4);
    if (!memcmp(bwe->comp, "lpcJ", 4))
    {
        memcpy(bwe->file, "14_4", 4);
    }
    else
    {
        memcpy(bwe->file, bwe->comp, 4);
    }
}

void
copy_bwe(BW_encoding*& to, const BW_encoding* from)
{
    if (from == 0)
    {
        to = 0;
        return;
    }

    int i = 0;
    const BW_encoding* bwe = from;
    while(bwe->bw)
    {
        i++;
        bwe++;
    }
    to = new BW_encoding[i+1];
    memcpy(to, from, sizeof(BW_encoding) * i);
    to[i].bw = 0;
}

UINT32
get_max_bw(const BW_encoding* bwe)
{
    const BW_encoding*  current_bw = bwe;
    UINT32            max_bw = 0;

    while(current_bw->bw)
    {
        if (memcmp("lpcJ", current_bw->comp, 4) == 0)
        {
            if (10 > max_bw)
            {
                max_bw = 10;
            }
        }
        else if (memcmp("28_8", current_bw->comp, 4) == 0)
        {
            if (20 > max_bw)
            {
                max_bw = 20;
            }
        }
        else if (current_bw->bw > max_bw)
        {
            max_bw = current_bw->bw;
        }
        current_bw++;
    }
    return max_bw;
}

