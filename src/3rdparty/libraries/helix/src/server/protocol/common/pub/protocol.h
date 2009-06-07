/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: protocol.h,v 1.4 2006/10/03 00:34:08 tknox Exp $
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

#include "tcpio.h"
#include "hxmarsh.h"
#include "debug.h"

#define MESSAGE_HEADER_SIZE     4
#define MESSAGE_SIZE            100

inline void
make_message_header(Byte* cp, unsigned short opcode, int length)
{
    length >>= 2;
    putshort(cp, opcode);
    putshort(cp+2, length);
}

inline int
message_opcode(Byte* header)
{
    return getshort(header) & 0x7fff;
}

inline int
message_length(Byte* header)
{
    return getshort(header+2) << 2;
}

inline int
message_length_round(int length)
{
    return (length + 3) & ~3;
}

inline Byte*
message_data(Byte* buf)
{
    return buf + MESSAGE_HEADER_SIZE;
}

inline Byte*
message_align(Byte* end, Byte* start)
{
    return ((end - start) % 4) ? end + 4 - ((end - start) % 4) : end;
}
