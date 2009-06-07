/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bwe.h,v 1.3 2004/06/02 17:18:29 tmarshall Exp $
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

#ifndef _BWE_H_
#define _BWE_H_

const int BWE_COMPRESSION_CODE_LEN = 4;
const int BWE_FILE_NAME_LEN = 4;
const UINT32 MAX_BANDWIDTH = 0xffffffff;

struct BW_encoding
{
    UINT16      bw;             // bandwidth in units of 100 bytes/sec
    char        comp[4];        // compression to be used for this bw
    char        file[4];        // file name to be used for this bw
};

void fill_bwe(BW_encoding* bwe, int bw, char* compression_code);
void copy_bwe(BW_encoding*& to, const BW_encoding* from);
UINT32 get_max_bw(const BW_encoding* bwe);

#endif/*_BWE_H_*/
