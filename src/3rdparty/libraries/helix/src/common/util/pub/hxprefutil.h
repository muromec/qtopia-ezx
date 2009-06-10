/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprefutil.h,v 1.16 2007/09/13 18:50:51 ping Exp $
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

#ifndef _HXPREFUTIL_
#define _HXPREFUTIL_

/*
 *
 * These all guarantee that the passed in reference will not be modified
 * if the function fails, so you can pass in a default value.
 * 
 * */

class CHXString;
_INTERFACE IHXPreferences;
_INTERFACE IUnknown;

HX_RESULT ReadPrefUINT32(IHXPreferences* pPreferences, const char* pszName,
                        UINT32& ulValue);

HX_RESULT ReadPrefUINT16(IHXPreferences* pPreferences, const char* pszName,
                       UINT16& ulValue);

HX_RESULT ReadPrefUINT8(IHXPreferences* pPreferences, const char* pszName, 
                       UINT8& nValue);

HX_RESULT ReadPrefFLOAT(IHXPreferences* pPreferences, const char* pszName, 
                        HXFLOAT& fValue);

HX_RESULT ReadPrefBOOL(IHXPreferences* pPreferences, const char* pszName, 
                       HXBOOL& ulValue);

HX_RESULT ReadPrefCSTRING(IHXPreferences* pPreferences, const char* pszName, CHXString& strValue);

HX_RESULT ReadPrefUINT32Array(IHXPreferences* pPrefs,
                              const char*     pszBaseName,
                              UINT32          ulNumValues,
                              UINT32*         pulValue);

HX_RESULT ReadPrefBOOL(IUnknown* pUnk, const char* pszName, HXBOOL& bValue);
HX_RESULT ReadPrefUINT32(IUnknown* pUnk, const char* pszName, UINT32& unValue);
HX_RESULT ReadPrefUINT16(IUnknown* pUnk, const char* pszName, UINT16& unValue);
HX_RESULT ReadPrefUINT8(IUnknown* pUnk, const char* pszName, UINT8& nValue);
HX_RESULT ReadPrefFLOAT(IUnknown* pUnk, const char* pszName, HXFLOAT& fValue);
HX_RESULT ReadPrefCSTRING(IUnknown* pUnk, const char* pszName, CHXString& strValue);
HX_RESULT ReadPrefStringBuffer(IUnknown* pUnk, const char* pszName, IHXBuffer*& rpStr);
HX_RESULT ReadPrefUINT32Array(IUnknown* pUnk,
                              const char*     pszBaseName,
                              UINT32          ulNumValues,
                              UINT32*         pulValue);

HX_RESULT WritePrefUINT32(IUnknown* pUnk, const char* pszName, UINT32 ulValue);
HX_RESULT WritePrefCSTRING(IUnknown* pUnk, const char* pszName, 
                           const CHXString& strValue);

HX_RESULT DeletePref(IUnknown* pUnk, const char* pszName);
#endif /* _HXPREFUTIL_ */
