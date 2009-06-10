/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxcom.h"
#include "hxtdirconv.h"

#include "hxdir.h"

// For heap checking
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////
// Method:
//	CHXTDirConv::GetNativePathname
// Purpose:
//	Converts szPathname into a pathname in the platform's native format (for example,
//	converts forward slashes to backward slashes on Windows)
HX_RESULT CHXTDirConv::GetNativePathname(const char* szPathname, CHXString& strNativePathname)
{
    HX_RESULT res = HXR_OK;
    
    // Validate params
    if (!szPathname || *szPathname == '\0')
    {
	    HX_ASSERT(FALSE);
	    return HXR_POINTER;
	}
    
    strNativePathname = szPathname;

    // Replace path separators with native platform's path separators
	for (UINT32 i=0; i < strNativePathname.GetLength(); i++)
	{
		char c = strNativePathname.GetAt(i);
		if (c == '\\' || c == '/')
		{
			strNativePathname.SetAt(i, OS_SEPARATOR_CHAR);
		}
	}
	   
    return res;
}




/////////////////////////////////////////////////////////////////////////
// Method:
//	CHXTDirConv::GetInternalPathname
// Purpose:
//	Only meaningful on the Mac.  Converts a POSIX-style pathname to an HFS-style pathname
HX_RESULT CHXTDirConv::GetInternalPathname(const char* szPathname, CHXString& strInternalPathname)
{
    HX_RESULT res = HXR_OK;
    
    // Validate params
    if (!szPathname || *szPathname == '\0')
    {
	    HX_ASSERT(FALSE);
	    return HXR_POINTER;
	}
    
    strInternalPathname = szPathname;
	
	HX_ASSERT(SUCCEEDED(res));   
    return res;
}

