/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: llcrtsup.h,v 1.3 2004/05/13 18:57:48 tmarshall Exp $ 
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

#if !defined _LLCRTSUP_H_ || defined LLCRTSUP_HERE
#define _LLCRTSUP_H_

#include "hlxclib/windows.h"

typedef void (* _UserHeapFreeType)(void* pBlock); 
typedef void* (* _UserHeapAllocType)(size_t size);
typedef void* (* _UserHeapCallocType)(size_t num, size_t size);
typedef void* (* _UserHeapReAllocType)(void *memblock, size_t size);
typedef int (* _UserHeapValidateType)(void* memblock);
typedef int (* _UserHeapSizeType)(void* memblock);

typedef _UserHeapFreeType (*fSetUserHeapFree)(_UserHeapFreeType pFunc);
typedef _UserHeapAllocType (*fSetUserHeapAlloc)(_UserHeapAllocType pFunc);
typedef _UserHeapCallocType (*fSetUserHeapCalloc)(_UserHeapCallocType pFunc);
typedef _UserHeapReAllocType (*fSetUserHeapReAlloc)(_UserHeapReAllocType pFunc);
typedef _UserHeapValidateType (*fSetUserHeapValidate)(_UserHeapValidateType pFunc);
typedef _UserHeapSizeType (*fSetUserHeapSize)(_UserHeapSizeType pFunc);
typedef void* (*f_CrtHeapAlloc)(size_t);

#ifdef LLCRTSUP_HERE
fSetUserHeapFree SetUserHeapFree;
fSetUserHeapAlloc SetUserHeapAlloc;
fSetUserHeapCalloc SetUserHeapCalloc;
fSetUserHeapReAlloc SetUserHeapReAlloc;
fSetUserHeapValidate SetUserHeapValidate;
fSetUserHeapSize SetUserHeapSize;
f_CrtHeapAlloc _CrtHeapAlloc;

_UserHeapFreeType default_SetUserHeapFree(_UserHeapFreeType pFunc){return NULL;}
_UserHeapAllocType default_SetUserHeapAlloc(_UserHeapAllocType pFunc){return NULL;}
_UserHeapCallocType default_SetUserHeapCalloc(_UserHeapCallocType pFunc){return NULL;}
_UserHeapReAllocType default_SetUserHeapReAlloc(_UserHeapReAllocType pFunc){return NULL;}
_UserHeapValidateType default_SetUserHeapValidate(_UserHeapValidateType pFunc){return NULL;}
_UserHeapSizeType default_SetUserHeapSize(_UserHeapSizeType pFunc){return NULL;}
void* default_CrtHeapAlloc(size_t){return NULL;}

BOOL
CRTSupInit()
{
    BOOL bGood = TRUE;
    HINSTANCE hLib = 0;
#ifdef _DEBUG
    hLib = LoadLibrary("pncrtd.dll");
#else
    hLib = LoadLibrary("pncrt.dll");
#endif
    if (!(SetUserHeapFree = (fSetUserHeapFree)GetProcAddress(hLib,
	"SetUserHeapFree")))
    {
	bGood = FALSE;
	SetUserHeapFree = default_SetUserHeapFree;
    }
    if (!(SetUserHeapAlloc = (fSetUserHeapAlloc)GetProcAddress(hLib,
	"SetUserHeapAlloc")))
    {
	bGood = FALSE;
	SetUserHeapAlloc = default_SetUserHeapAlloc;
    }
    if (!(SetUserHeapCalloc = (fSetUserHeapCalloc)GetProcAddress(hLib,
	"SetUserHeapCalloc")))
    {
	bGood = FALSE;
	SetUserHeapCalloc = default_SetUserHeapCalloc;
    }
    if (!(SetUserHeapReAlloc = (fSetUserHeapReAlloc)GetProcAddress(hLib,
	"SetUserHeapReAlloc")))
    {
	bGood = FALSE;
	SetUserHeapReAlloc = default_SetUserHeapReAlloc;
    }
    if (!(SetUserHeapValidate = (fSetUserHeapValidate)GetProcAddress(hLib,
	"SetUserHeapValidate")))
    {
	bGood = FALSE;
	SetUserHeapValidate = default_SetUserHeapValidate;
    }
    if (!(SetUserHeapSize = (fSetUserHeapSize)GetProcAddress(hLib,
	"SetUserHeapSize")))
    {
	bGood = FALSE;
	SetUserHeapSize = default_SetUserHeapSize;
    }
    if (!(_CrtHeapAlloc = (f_CrtHeapAlloc)GetProcAddress(hLib,
	"_CrtHeapAlloc")))
    {
	bGood = FALSE;
	_CrtHeapAlloc = default_CrtHeapAlloc;
    }
    FreeLibrary(hLib);
    return bGood;
}
#else
extern fSetUserHeapFree SetUserHeapFree;
extern fSetUserHeapAlloc SetUserHeapAlloc;
extern fSetUserHeapCalloc SetUserHeapCalloc;
extern fSetUserHeapReAlloc SetUserHeapReAlloc;
extern fSetUserHeapValidate SetUserHeapValidate;
extern fSetUserHeapSize SetUserHeapSize;
extern f_CrtHeapAlloc _CrtHeapAlloc;
BOOL CRTSupInit();

#endif

#endif /* _LLCRTSUP_H_ */
