/* ***** BEGIN LICENSE BLOCK *****
 *
 * Version: $Id: wchar.h,v 1.8 2009/04/09 23:01:40 cbailey Exp $
 * 
 * Copyright Notices: 
 *  
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
 *  
 * Patent Notices: This file may contain technology protected by one or  
 * more of the patents listed at www.helixcommunity.org 
 *  
 * 1.   The contents of this file, and the files included with this file, 
 * are protected by copyright controlled by RealNetworks and its  
 * licensors, and made available by RealNetworks subject to the current  
 * version of the RealNetworks Public Source License (the "RPSL")  
 * available at  http://www.helixcommunity.org/content/rpsl unless  
 * you have licensed the file under the current version of the  
 * RealNetworks Community Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply.  You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *  
 * 2.  Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above.  Please note that RealNetworks and its  
 * licensors disclaim any implied patent license under the GPL.   
 * If you wish to allow use of your version of this file only under  
 * the terms of the GPL, and not to allow others 
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting Paragraph 1 above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete Paragraph 1 above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
 *  
 * This file is part of the Helix DNA Technology.  RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created.   Copying, including reproducing, storing,  
 * adapting or translating, any or all of this material other than  
 * pursuant to the license terms referred to above requires the prior  
 * written consent of RealNetworks and its licensors 
 *  
 * This file, and the files included with this file, is distributed 
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributors: Nokia Inc
 *
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef HLXLIB_WCHAR_H
#define HLXLIB_WCHAR_H

// the header has the inclusion for the wchar string operations

#ifdef _SYMBIAN
    #include "platform/symbian/symbian_wchar_functions.h"
    #include <libc/string.h>
#elif _BREW
#elif ANDROID
    //Android has no wide-char support
#ifdef ANDROID_RELEASE_1
    //Android release 1.0  has a stub wchar.h that re-defines wchar_t to char
    //this conflicts with gcc 4 builtin wchar_t
    #define wchar_t _wchar_t_
#endif
    #include <wchar.h>
    #define mbstowcs(w,m,x) mbsrtowcs(w,(const char**)(& #m),x,NULL)
#else
    #include <wchar.h>
#endif // End of #ifdef _SYMBIAN

#if defined(_MAC_UNIX)
    #include <wctype.h>
    #include "hlxclib/string.h"
	
HLX_INLINE 
int wcsncasecmp (const wchar_t *str1, const wchar_t *str2, size_t n)
{
    INT32 l1  = 0;
    INT32 l2  = 0;
    INT32 diff = 0;

    while (n--)
    {
	l1 = towlower (*str1);
	l2 = towlower (*str2);

	diff = l1 - l2;
	if (diff)
	    return diff;

	if (!l1)
	    return 0;

	++str1;
	++str2;
    }
    return diff;
}
#endif

#endif // End of #ifndef HLXLIB_WCHAR_H

