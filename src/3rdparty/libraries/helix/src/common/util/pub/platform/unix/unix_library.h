/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_library.h,v 1.5 2007/07/06 20:39:28 jfinnecy Exp $
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

#ifndef __UNIX_LIBRARY_H
#define __UNIX_LIBRARY_H

#include "hxtypes.h"

#define HMODULE ULONG32
#define HINSTANCE ULONG32

// just a large number used to identify bad handles
#define DL_MAX_NUM_LIBS 9000000L
#define DL_BAD_LIB_ID DL_MAX_NUM_LIBS + 1


/* Functions to load & release Shared Libraries */

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		LoadLibrary()
//
//	Purpose:
//
//		Called to load a Unix Shared Library given the library's name
//
//	Parameters:
//
//		char* dllname
//
//	Return:
//
//		ULONG32
//		Returns a handle to the library which is used to identify which 
//		library is being accessed in subsequent calls to these functions.
//
ULONG32 LoadLibrary(const char* dllname);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		FreeLibrary()
//
//	Purpose:
//
//		Called to free a Shared Library. If this is not called the library
//		will be freed when the application quits.
//
//	Parameters:
//
//		HMODULE lib
//		This is an opaque handle identifying the library.
//
//	Return:
//
//		none
//
void FreeLibrary(HMODULE lib);

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		GetProcAddress()
//
//	Purpose:
//
//		Called to get a function pointer in a Shared Library.
//
//	Parameters:
//
//		HMODULE lib
//
//		char* function
//		The function name
//
//	Return:
//
//		void*
//		The address of the function.
//

void* GetProcAddress(HMODULE lib, char* function);

#if defined RAFreeLibrary
#undef RAFreeLibrary
#endif
#define RAFreeLibrary(x) FreeLibrary(x)

#endif // ifndef

