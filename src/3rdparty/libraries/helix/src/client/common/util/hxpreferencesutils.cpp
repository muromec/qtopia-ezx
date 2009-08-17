/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxpreferencesutils.cpp,v 1.1 2006/03/22 17:51:58 stanb Exp $
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

#include "hxpreferencesutils.h"
#include "hxbufferutils.h"          // HXBufferUtils
#include "hxprefs.h"


namespace HXPreferencesUtils
{

HX_RESULT
SplitKey(const char* szFullKey, CHXString* /*OUT*/ pKeyPrefix, CHXString* /*OUT*/ pKeyName)
{
    // TEST: strFullKey.GetLength() > nLastOccurence+1
    REQUIRE_RETURN_QUIET(szFullKey, HXR_INVALID_PARAMETER);

    // Finding the last separator, and if there is, then set the prefix key
    CHXString strFullKey = szFullKey;
    INT32 nLastOccurence = strFullKey.ReverseFind(czPrefKeySeparator);
    if (pKeyPrefix)
    {
        pKeyPrefix->Empty();
        if (nLastOccurence >= 0)
        {
            *pKeyPrefix = strFullKey.Left(nLastOccurence);
        }
    }

    // Setting the key name
    if (pKeyName)
    {
        pKeyName->Empty();
        if (strFullKey.GetLength() > nLastOccurence+1)
        {
            *pKeyName = strFullKey.Mid(nLastOccurence+1);
        }
    }

    return HXR_OK;
}

HX_RESULT
WritePrefString(IHXCommonClassFactory* pCCF, const char* szPrefName, const char* szString)
{
    if (!pCCF || !szPrefName || !szString)
    {
        return HXR_INVALID_PARAMETER;
    }
    HX_RESULT res = HXR_FAIL;

    SPIHXPreferences spPrefs(pCCF, CLSID_IHXPreferences, &res);
    SPIHXBuffer spBuffer;
    if (spPrefs)
        res = HXBufferUtils::CreateBufferFromString(pCCF, szString, spBuffer.AsInOutParam());

    if (spBuffer)   
        res = spPrefs->WritePref(szPrefName,spBuffer.Ptr());
    
    return res;
}

HX_RESULT
ReadPrefString(IHXCommonClassFactory* pCCF, const char* szPrefName, CHXString& theString)
{
    SPIHXBuffer spBuffer;
    HX_RESULT res = ReadPrefBuffer(pCCF, szPrefName, spBuffer.AsInOutParam());

    if(spBuffer)   
        theString = (const char*)spBuffer->GetBuffer();

    return res;
}

HX_RESULT
WritePrefINT32(IHXCommonClassFactory* pCCF, const char* szPrefName, INT32 nValue)
{
    CHXString strPref;
    strPref.Format("%ld", nValue);            
    return WritePrefString(pCCF,szPrefName,strPref);
}

HX_RESULT
ReadPrefINT32(IHXCommonClassFactory* pCCF, const char* szPrefName, INT32& nValue)
{    
    SPIHXBuffer spBuffer;
    HX_RESULT res = ReadPrefBuffer(pCCF, szPrefName, spBuffer.AsInOutParam());
    if(spBuffer)   
        nValue = atol((const char*) spBuffer->GetBuffer());
        
    return res;
}

HX_RESULT
WritePrefUINT32(IHXCommonClassFactory* pCCF, const char* szPrefName, UINT32 unValue)
{
    CHXString strPref;
    strPref.AppendULONG(unValue);   
    return WritePrefString(pCCF,szPrefName,strPref);
}

HX_RESULT
ReadPrefUINT32(IHXCommonClassFactory* pCCF, const char* szPrefName, UINT32& unValue)
{    
    SPIHXBuffer spBuffer;
    HX_RESULT res = ReadPrefBuffer(pCCF, szPrefName, spBuffer.AsInOutParam());
    if(spBuffer)   
        unValue = (UINT32)atol((const char*) spBuffer->GetBuffer());
        
    return res;
}

HX_RESULT
DeletePref(IHXCommonClassFactory* pCCF, const char* szPrefName )
{
    if (!pCCF || !szPrefName)
    {
        return HXR_INVALID_PARAMETER;
    }
    HX_RESULT res = HXR_FAIL;

    SPIHXPreferences3 spPrefs(pCCF, CLSID_IHXPreferences, &res);
    if (spPrefs)
    {
        res = spPrefs->DeletePref(szPrefName);
    }

    return res;
}

HX_RESULT
ReadPrefBuffer(IHXCommonClassFactory* pCCF, const char* szPrefName, IHXBuffer** ppBuffer)
{
    if (!pCCF || !szPrefName || !ppBuffer)
    {
        return HXR_INVALID_PARAMETER;
    }
    HX_RESULT res = HXR_FAIL;

    SPIHXPreferences spPrefs(pCCF, CLSID_IHXPreferences, &res);

    if (spPrefs)
    {
        res = spPrefs->ReadPref(szPrefName, *ppBuffer);
    }

    return res;
}

HX_RESULT
WritePrefBuffer(IHXCommonClassFactory* pCCF, const char* szPrefName, IHXBuffer* pBuffer)
{
    if (!pCCF || !szPrefName || !pBuffer)
    {
        return HXR_INVALID_PARAMETER;
    }
    HX_RESULT res = HXR_FAIL;

    SPIHXPreferences spPrefs(pCCF, CLSID_IHXPreferences, &res);

    if (spPrefs)
    {
        res = spPrefs->WritePref(szPrefName, pBuffer);
    }

    return res;
}

};

//Leave a CR/LF before EOF to prevent CVS from getting angry
