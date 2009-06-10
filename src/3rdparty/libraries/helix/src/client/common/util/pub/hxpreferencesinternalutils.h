/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxpreferencesinternalutils.h,v 1.3 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _HXPREFERENCESINTERNALUTILS_H_
#define _HXPREFERENCESINTERNALUTILS_H_

// Member variables
#include "hxstring.h"       // CHXString
#include "hxthreadsync.h"   // HXLockKey
#include "ihxpckts.h"       // SPIHXBuffer
#include "hxtime.h"         // HXTime
#include <map>

namespace HXPreferencesInternalUtils
{
    typedef enum
    {
        KeyStatus_Unmodified = 0,
        KeyStatus_Modified,
        KeyStatus_Removed
    } KeyStatus;

    typedef std::pair<KeyStatus, SPIHXBuffer> StatusValuePair;

    typedef std::map<CHXString, StatusValuePair> StrToStatusValuePairMap;
    typedef std::pair<CHXString, StatusValuePair> StrToStatusValuePairMapPair;

    typedef std::map<CHXString, StrToStatusValuePairMap*> StrToMapPtrMap;
    typedef std::pair<CHXString, StrToStatusValuePairMap*> StrToMapPtrMapPair;

    HX_RESULT LoadFile                      (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* szFileFullPath, HXTime* pTime);
    HX_RESULT FlushMapToDisk                (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* szFileFullPath, HXTime* pTime);
    HX_RESULT GetPrefNoPrefix               (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* pPrefKey, UINT32 nIndex, REF(IHXBuffer*) pBuffer);
    HX_RESULT ReadPrefNoPrefix              (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* pPrefKey, REF(IHXBuffer*) pBuffer);
    HX_RESULT WritePrefNoPrefix             (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* pPrefKey, IHXBuffer* pBuffer, KeyStatus ksStatus, HXBOOL bOnlyChangeUnmodifiedPrefs);
    HX_RESULT DeletePrefNoPrefix            (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* pPrefKey);
    HX_RESULT ParseXML                      (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, IHXBuffer* pBuffer, HXTime* pTime);
    StrToStatusValuePairMap* GetValueMap    (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock, const char* szPref, HXBOOL bCreate = FALSE);
    void ClearData                          (IHXCommonClassFactory* pCCF, StrToMapPtrMap* pKeyMap, HXLockKey* pLock);

    void EncodeEscapeCharacters(REF(CHXString) /* in/out */ str);
    void DecodeEscapeCharacters(REF(CHXString) /* in/out */ str);
};


#endif // _HXPREFERENCESINTERNALUTILS_H_
//Leave a CR/LF before EOF to prevent CVS from getting angry
