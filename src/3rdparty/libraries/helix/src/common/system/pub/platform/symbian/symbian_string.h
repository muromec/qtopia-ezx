/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_string.h,v 1.4 2009/03/04 00:47:00 girish2080 Exp $
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

#ifndef _SYMBIAN_STRINGS_H_
#define _SYMBIAN_STRINGS_H_

#include <e32std.h>
#include <e32base.h>
#include <charconv.h>
#include <utf.h>

#include "hxassert.h"
#include "hxstring.h"
#include "hxbuffer.h"

#ifdef HELIX_CONFIG_SYMBIAN_GENERATE_MMP
//Symbian MMP file does not support String constant definition using MACRO, 
//so use STRINGIFY to get the string value of HELIX_DEFINE_DLL_NAMESPACE
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x
#else
#define STRINGIFY(x) x
#endif // HELIX_CONFIG_SYMBIAN_GENERATE_MMP

namespace CHXSymbianString
{
    void StringToDes(const CHXString& in, TDes& out);
    void DesToString(const TDesC& in, CHXString& out);
    CHXString DescToString(const TDesC& in);

    CHXString DescToString(const TDesC& desc);
    HBufC* StringToHBuf(const CHXString& s);
    HBufC* AllocTextL(const CHXString& str);
    //Prefix name space to a string descriptor 
    HBufC* PrefixNameSpaceLC(const TDesC& aPath, const char* const aNameSpace);
    void PrefixNameSpace(const TDesC& aPath, const char* const aNameSpace, TDes& aDllNamePath);

	//Prefix HELIX_DEFINE_DLL_NAMESPACE to a string descriptor only if its defined
    HBufC* PrefixNameSpaceLC(const TDesC& aPath);
    void PrefixNameSpace(const TDesC& aPath, TDes& aDllNamePath);
    void PrefixNameSpace(CHXString& aDest);
}


#endif //_SYMBIAN_STRINGS_H_

