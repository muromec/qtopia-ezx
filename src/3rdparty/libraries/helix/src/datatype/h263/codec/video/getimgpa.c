/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: getimgpa.c,v 1.3 2007/07/06 22:00:33 jfinnecy Exp $
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

/*  Header File Includes        */
#include "dllindex.h"
#include "h261func.h"
#include "h261defs.h"

short getImgParms( short formatCap, short *numGOBs, short *numMBs, long *imgSize,
                   long *lumaSize, long *chromaLineLength, long *chromaRows, long *maxsym )
{
    switch ( formatCap )
    {
    case SQCIF:
        *chromaLineLength = (long)64;
        *chromaRows = (long)48;
        *numGOBs  = 6;
        *numMBs   = (8*6);
        break;
    case QCIF:
        *chromaLineLength = (long)88;
        *chromaRows = (long)72;
        *numGOBs  = 9;
        *numMBs   = (11*9);
        break;
    case CIF:
        *chromaLineLength = (long)176;
        *chromaRows = (long)144;
        *numGOBs  = 18;
        *numMBs   = (22 * 18);
        break;
    case CIF4:
        *chromaLineLength = (long)352;
        *chromaRows = (long)288;
        *numGOBs  = 18;
        *numMBs   = 4 * 22*18;
        break;
    case CIF16:
        *chromaLineLength = (long)704;
        *chromaRows = (long)576;
        *numGOBs  = 18;
        *numMBs   = 16 * 22*18;
        break;
    default:
        return ( UNKNOWN_PICTURE_FORMAT );
        break;
    }
    *imgSize = *numMBs * (256 + 2 * 64);
    *lumaSize = *numMBs * 256;
    *maxsym = *imgSize;
    return ( 0 );
}
