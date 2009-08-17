/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: buffer.cpp,v 1.6 2004/07/09 18:21:35 hubbe Exp $
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
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hxtypes.h"

#include "hxassert.h"
#include "buffer.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


Buffer::Buffer() {
    refcount = 0;
}

Buffer::~Buffer() 
{
    if (base) delete[] base;
    HX_ASSERT(refcount == 0);
}

/*
 * Ensure that required number of contiguous bytes is
 * available (in preparation for storing a large object,
 * part of which may have already been buffered).
 */

void
WrapBuffer::ensure(int required) {
    int count;

    if (limit - data >= required)
	return;
    count = space - data;
    if (limit < base + required) {
	Byte* newbase = new Byte[required];
	memmove(newbase, data, count);
	delete[] base;
	base = newbase;
	limit = base + required;
    } else {
	memmove(base, data, count);
    }
    data = base;
    space = data + count;
}

void
WrapBuffer::ensure_space(int required) {
    if (limit - space >= required)
	return;
    ensure(required + count());
}

void
GrowBuffer::grow(int s) {
    Byte* newbase;
    int count;

    count = used();
    newbase = new Byte[s];
    memcpy(newbase, base, (count <= s ? count : s)); /* Flawfinder: ignore */
    delete[] base;
    base = newbase;
    limit = base + s;
    data = base;
    space = base + count;
} 

void
GrowBuffer::ensure_space(int required) {
    if (limit - space >= required)
	return;

    int s = used() + required;

    /*
     * Try to reduce the amount of copying required
     */

    grow(s*2);
}
