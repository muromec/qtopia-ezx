/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: set_debugger_thread_name.cpp,v 1.2 2007/07/06 20:41:58 jfinnecy Exp $
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

#include "hlxclib/windows.h"
#include "set_debugger_thread_name.h"

void SetDebuggerThreadName(UINT32 ulThreadID, const char* pszThreadName)
{
    if (ulThreadID && pszThreadName)
    {
        // This method sets the thread name in the VC debugger.
        // It does this by throwing and catching a special exception
        // which is used to pass the thread name to the debugger.
        // This is only defined for Visual C, which sets the
        // _MSC_VER define.
#if defined(_MSC_VER) && defined(_DEBUG)
        struct 
        {
            DWORD dwType;        // must be 0x1000
            LPCSTR szName;       // pointer to name (in same addr space)
            DWORD dwThreadID;    // thread ID (-1 caller thread)
            DWORD dwFlags;       // reserved for future use, most be zero
        } SetThreadNameInfo = {0x1000, pszThreadName, ulThreadID, 0};

        __try
        {
            RaiseException(0x406d1388, 0, sizeof (SetThreadNameInfo) / sizeof (DWORD), (DWORD*) &SetThreadNameInfo);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
#endif /* #if defined(_MSC_VER) */
    }
}
