/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sio.cpp,v 1.8 2007/07/06 20:35:11 jfinnecy Exp $
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

#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "debug.h"
#include "bio.h"
#include "sio.h"

#include "hxheap.h"
#include "hxassert.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

SIO::Region*
SIO::Region_list::find(Byte* buf, SIO::Region**& regp)
{
    Region* r;
    Region **rp;
    for (rp = &regs; (r = *rp) != 0; rp = &r->next)
    {
	if (r->base <= buf && buf < r->limit)
	{
	    regp = rp;
	    return r;
	}
    }
    regp = rp;
    return 0;
}

SIO::Region*
SIO::Region_list::find(off_t offt, SIO::Region**& regp)
{
    Region* r;
    Region **rp;
    for (rp = &regs; (r = *rp) != 0; rp = &r->next)
    {
	if (offt < r->off)
	    break;
	if (r->off + r->limit - r->base < offt)
	    continue;
	regp = rp;
	return r;
    }
    regp = rp;
    return 0;
}

Byte*
SIO::Region_list::remove(Region* reg)
{
    Region* r;
    for (Region** rp = &regs; (r = *rp) != 0; rp = &r->next)
	if (r == reg)
	{
	    *rp = r->next;
	    Byte* b = r->base;
	    delete r;
	    return b;
	}
    return 0;
}

int
SIO::write_flush_count()
{
    Region* r = writer.regs;
    int count = 0;
    
    while (r && r->refcount == 0 && r != writer.creg)
    {
	count += HX_SAFEINT(r->limit - r->base - (r->flush_off - r->off));
	r = r->next;
    }

    return count;
}

HXBOOL
SIO::write_flush_needed()
{
    Region* r = writer.regs;
    
    while (r && r->refcount == 0 && r != writer.creg)
    {
	if (HX_SAFEINT(r->limit - r->base - (r->flush_off - r->off)))
            return TRUE;
	r = r->next;
    }
    
    return FALSE;
}
