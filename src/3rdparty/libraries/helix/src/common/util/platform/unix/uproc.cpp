/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: uproc.cpp,v 1.5 2004/07/09 18:23:15 hubbe Exp $
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

#include <unistd.h>
#include <string.h>

#include "hxtypes.h"

#include "hxproc.h"

#if defined(_FREEBSD)
#include <osreldate.h>
#if (__FreeBSD_version >= 199512)
#include "libutil.h"
#endif
#endif

UINT32
process_id()
{
    return getpid();
}

#if (defined _SGI || defined _IRIX || defined _HPUX || defined _SOLARIS || defined _BEOS)
INT32
Killpg(pid_t pgrp, int sig)
{
    return kill(pgrp, sig);
}
#else
INT32
Killpg(pid_t pgrp, int sig)
{
    return killpg(pgrp, sig);
}
#endif /* (_SGI || _IRIX || _HPUX || _SOLARIS || _BEOS) */ 

void
setprocstr(char *src, char **origargv, int origargc, char **environ)
{
    char *ptr, *ptr2;
    int count;

#if (__FreeBSD_version >= 199512)
    setproctitle (src);
    return;
#endif

    if (strlen(src) <= strlen(origargv[0]))
    {
        for (ptr = origargv[0]; *ptr; *(ptr++) = '\0');

        strcpy(origargv[0], src); /* Flawfinder: ignore */
    }
    else
    {
        ptr = origargv[0] + strlen(origargv[0]);
        for (count = 1; count < origargc; count++) {
            if (origargv[count] == ptr + 1)
                ptr += strlen(++ptr);
        }
        if (environ[0] == ptr + 1) {
            for (count = 0; environ[count]; count++)
                if (environ[count] == ptr + 1)
                    ptr += strlen(++ptr);
        }
        count = 0;
        for (ptr2 = origargv[0]; ptr2 <= ptr; ptr2++) {
            *ptr2 = '\0';
            count++;
        }
        strncpy(origargv[0], src, count); /* Flawfinder: ignore */
    }
}

