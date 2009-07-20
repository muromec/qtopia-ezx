/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpreferences.cpp,v 1.6 2006/05/25 09:44:03 pankajgupta Exp $
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

// Local
#include "hxpreferences.h"
#include "hxpreferencesenumerator.h" // CHXPreferencesEnumerator

// Interfaces / Smart Pointers
#include "ihxpckts.h"   // SPIHXBuffer

// Objects
#include "filespec.h"       // CHXFileSpecifier

// Utility
#include "filespecutils.h"      // CHXFileSpecUtils
#include "hxbufferutils.h"      // HXBufferUtils
#include "hxpreferencesutils.h" // HXPreferencesUtils

// XML Parser
#include "rpsmplxml.h"  // CRPSimpleXMLParser

//
// Interface List
//
BEGIN_INTERFACE_LIST(CHXPreferences)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXContextUser)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferences)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferences2)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferences3)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferencesFile)
END_INTERFACE_LIST

//
// Constants
//
namespace HXPreferences_Constants
{
    const char* g_szAttributeName       = "name";
    const char* g_szAttributeValue      = "value";
    const char* g_szTagPref             = "pref";
    const char* g_szConfigFileVersion   = "1.0";

    const char* g_szTagConfig           = "config";
    const char* g_szAttributeTimeStamp  = "ts";
    const char* g_szTimeStampSeparator  = ":";

    const UINT32 g_uEscapeCharacters = 2;
    const char* g_EscapeCharacters[HXPreferences_Constants::g_uEscapeCharacters][2] = 
        {
            {"&amp;", "&"}, // This must be the first 
            {"&quot;", "\""}
        }; // The number of escape characters must correlate with g_uEscapeCharacters
};

//
// Private Utility Declaration
// NOTE: These utility should be put somewhere else that anyone dealing with XML can access
//
namespace CHXPreferencesPrivateUtils
{
    // Declaration
    void EncodeEscapeCharacters                (REF(CHXString) /* in/out */ str);
    void DecodeEscapeCharacters                (REF(CHXString) /* in/out */ str);
};

//
// CHXPreferences static member variable initialization
//
HXLockKey CHXPreferences::m_DefaultLock;
CHXString CHXPreferences::m_strDefaultFilePath;
HXTime CHXPreferences::m_DefaultTimeStamp;
CHXPreferences::PrefRootMap CHXPreferences::m_DefaultKeyMap;
INT32 CHXPreferences::m_lDefaultKeyMapRefCount = 0;

//
// Constructor / Destructor
//
CHXPreferences::CHXPreferences()
    : m_strRootKey()
    , m_pMemberLock(&m_DefaultLock)
    , m_pStrFilePath(&m_strDefaultFilePath)
    , m_pTimeStamp(&m_DefaultTimeStamp)
    , m_pKeyMap(&m_DefaultKeyMap)
{
    HXAutoLock autoLock(&m_DefaultLock);
    m_lDefaultKeyMapRefCount++;
}

CHXPreferences::~CHXPreferences()
{
    if (m_pStrFilePath != &m_strDefaultFilePath)
    {
        Flush();
    }

    HXAutoLock autoLock(&m_DefaultLock);
    m_lDefaultKeyMapRefCount--;
    if (!_IsUsingDefaultPreferences())
    {
        _Clear(TRUE);
    }
    
    if (m_lDefaultKeyMapRefCount <= 0)
    {
        HX_ASSERT(m_lDefaultKeyMapRefCount >= 0 && "Why is the ref count less than 0?");
        Flush();
        _Clear(TRUE);
    }
}

//
// IHXPreferences
//
STDMETHODIMP
CHXPreferences::ReadPref(THIS_ const char* pPrefKey, REF(IHXBuffer*) pBuffer)
{
    return ReadPrefNoPrefix(_AppendPrefix(pPrefKey), pBuffer);
}

STDMETHODIMP
CHXPreferences::WritePref(THIS_ const char* pPrefKey, IHXBuffer* pBuffer)
{
    return WritePrefNoPrefix(_AppendPrefix(pPrefKey), pBuffer, HXPreferencesKeyStatus_Modified, FALSE);
}

//
// IHXPreferences2 methods
//
STDMETHODIMP
CHXPreferences::GetPreferenceEnumerator(THIS_ REF(IHXPreferenceEnumerator*) pEnum)
{
    HX_ASSERT(!pEnum && "Why are you passing in a valid enumerator?  It should be null");
    pEnum = new CHXPreferencesEnumerator(this, m_strRootKey);
    HX_ADDREF(pEnum);
    return HXR_OK;
}

STDMETHODIMP
CHXPreferences::ResetRoot(THIS_ const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer)
{
    return Open(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer);
}

//
// IHXPreferences3
//
STDMETHODIMP
CHXPreferences::Open(THIS_ const char* pCompanyName, const char* pProductName, ULONG32 nProdMajorVer, ULONG32 nProdMinorVer)
{
    HX_ASSERT(pCompanyName && pProductName && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(pCompanyName && pProductName, HXR_INVALID_PARAMETER);
    m_strRootKey.Format("%s_%s_%u.%u", static_cast<const char*>(pCompanyName), static_cast<const char*>(pProductName), nProdMajorVer, nProdMinorVer);
    return HXR_OK;
}

STDMETHODIMP
CHXPreferences::OpenShared(THIS_ const char* pCompanyName)
{
    HX_ASSERT(pCompanyName && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(pCompanyName, HXR_INVALID_PARAMETER);
    m_strRootKey = pCompanyName;
    return HXR_OK;
}

STDMETHODIMP
CHXPreferences::DeletePref(THIS_ const char* pPrefKey)
{
    return DeletePrefNoPrefix(_AppendPrefix(pPrefKey));
}

//
// IHXPreferencesFile
//
STDMETHODIMP
CHXPreferences::SetFilename(THIS_ const char* szFilePath, HXBOOL bUseDefaultPreferences)
{
    HX_ASSERT(m_pKeyMap && "Member variable is NULL");
    HX_ASSERT(m_pStrFilePath && "Member variable is NULL");
    HX_ASSERT(m_pMemberLock && "Why is the Lock null?");
    HX_ASSERT(m_pTimeStamp && "Member variable is NULL");
    REQUIRE_RETURN_QUIET(m_pKeyMap && m_pStrFilePath && m_pTimeStamp, HXR_FAIL);

    // If this object has already been set as a non-default preference and if the user
    // tries to change the filename again, then the data will be flushed
    HXLockKey* pOldLock = m_pMemberLock;
    HXBOOL bWasUsingDefaultPrefs = TRUE;
    {
        HXAutoLock autoLock(m_pMemberLock);
        if (!_IsUsingDefaultPreferences() && m_pStrFilePath && !m_pStrFilePath->IsEmpty() && *m_pStrFilePath != szFilePath)
        {
            Flush();
        }

        bWasUsingDefaultPrefs = _IsUsingDefaultPreferences();
        if (bUseDefaultPreferences && !bWasUsingDefaultPrefs)
        {
            _SetFilenameHelper(&m_DefaultKeyMap, &m_DefaultLock, &m_strDefaultFilePath, &m_DefaultTimeStamp);
        }
        else if (!bUseDefaultPreferences && bWasUsingDefaultPrefs)
        {
            HXLockKey* pNewLock = new HXLockKey;
            PrefRootMap* pNewMap = new PrefRootMap();
            CHXString* pNewFilename = new CHXString;
            HXTime* pNewTimeStamp = new HXTime;
            _SetFilenameHelper(pNewMap, pNewLock, pNewFilename, pNewTimeStamp);
        }
        else if (!bUseDefaultPreferences && !bWasUsingDefaultPrefs)
        {
            if (m_pKeyMap)
            {
                _ClearData();
                m_pKeyMap->RemoveAll();
            }
            if (m_pStrFilePath)
            {
                m_pStrFilePath->Empty();
            }
            if (m_pTimeStamp)
            {
                m_pTimeStamp->tv_sec = 0;
                m_pTimeStamp->tv_usec = 0;
            }
            pOldLock = NULL;
        }
    }

    if (!bWasUsingDefaultPrefs)
    {
        HX_DELETE(pOldLock);
    }

    if (szFilePath)
    {
        HXAutoLock autoLock(m_pMemberLock); // Write Lock
        // XXXPBC Issue: This function allows opening multiple file, but it will only keep
        // track of the last opened file for this instance of the object.
        *m_pStrFilePath = szFilePath;

        return _LoadFile(szFilePath, m_pTimeStamp);
    }
    return HXR_OK;
}

STDMETHODIMP
CHXPreferences::Flush(THIS)
{
    HX_ASSERT(m_pKeyMap && "Member variable is NULL");
    HX_ASSERT(m_pStrFilePath && "Member variable is NULL");
    HX_ASSERT(m_pMemberLock && "Why is the lock NULL?");
    HX_ASSERT(m_pTimeStamp && "Member variable is NULL");
    REQUIRE_RETURN_QUIET(m_pKeyMap && m_pStrFilePath && m_pTimeStamp, HXR_FAIL);

    HX_RESULT hxres = HXR_OK;
    CHXString strFilename = *m_pStrFilePath;

    if (!strFilename.IsEmpty())
    {
        HXAutoLock autoLock(m_pMemberLock); // Read/Write Lock
        // Reloading the preference file to make sure it gets the updated data.
        _LoadFile(strFilename, m_pTimeStamp);
        HXTime time;
#if defined (_WINDOWS) || defined (_WIN32) || defined(_UNIX) || defined (_MACINTOSH) || defined(_VXWORKS) || defined(_OPENWAVE)
        gettimeofday(&time, NULL);
#else
        HX_ASSERT(!"Did not implement this part for the current system you're using.  Please contact pcho for more info.");
#endif
        CHXString sString, strKey, strXMLOutput;
        strXMLOutput.Format("<xml version=\"1.0\">\n\t<%s version=\"%s\" %s=\"%d:%d\">\n", HXPreferences_Constants::g_szTagConfig, HXPreferences_Constants::g_szConfigFileVersion, HXPreferences_Constants::g_szAttributeTimeStamp, time.tv_sec, time.tv_usec);

        for (PrefRootMap::Iterator iterKey = m_pKeyMap->Begin(); iterKey != m_pKeyMap->End(); iterKey++)
        {
            if (!_GetPrefRootMapIterValue(iterKey))
            {
                continue;
            }
            PrefKeyValMap::Iterator newIterValue = _GetPrefRootMapIterValue(iterKey)->Begin();
            while (newIterValue != _GetPrefRootMapIterValue(iterKey)->End())
            {
                PrefKeyValMap::Iterator iterValue = newIterValue;
                newIterValue++;
                if (_GetPrefKeyValMapIterValue(iterValue)->first != HXPreferencesKeyStatus_Removed)
                {
                    CHXString strFirstKey = _GetPrefRootMapIterKey(iterKey);
                    CHXString strSecondKey = _GetPrefKeyValMapIterKey(iterValue);
                    if (!(strFirstKey.IsEmpty()))
                    {
                        strKey = strFirstKey;
                        strKey += HXPreferencesUtils::czPrefKeySeparator;
                    }
                    if (!(strSecondKey.IsEmpty()))
                    {
                        strKey += strSecondKey;
                    }

                    HX_ASSERT(_GetPrefKeyValMapIterValue(iterValue)->second.IsValid() && "This should never be null");
                    if (_GetPrefKeyValMapIterValue(iterValue)->second.IsValid())
                    {
                        // XXXPBC Issue: Encoding escape characters this way is very inefficient because
                        // even if there are no characters to encode, it will copy the buffer into a string
                        // and end up doing nothing to it.
                        CHXString strValue = _GetPrefKeyValMapIterValue(iterValue)->second->GetBuffer();
                        CHXPreferencesPrivateUtils::EncodeEscapeCharacters(strValue);
                        CHXPreferencesPrivateUtils::EncodeEscapeCharacters(strKey);
                        sString.Format("\t\t<%s %s=\"%s\" %s=\"%s\"/>\n", HXPreferences_Constants::g_szTagPref, HXPreferences_Constants::g_szAttributeName, strKey.IsEmpty() ? "" : (const char*)strKey, HXPreferences_Constants::g_szAttributeValue, strValue.IsEmpty() ? "" : (const char*)strValue);
                        strXMLOutput += sString;
                        _GetPrefKeyValMapIterValue(iterValue)->first = HXPreferencesKeyStatus_Unmodified;
                    }
                    strKey.Empty();
                }
                else
                {
                    _GetPrefRootMapIterValue(iterKey)->Erase(iterValue);
                    if (_GetPrefKeyValMapIterValue(iterValue))
                    {
                        delete _GetPrefKeyValMapIterValue(iterValue);
                    }
                }
            }
        }
        strXMLOutput += "\t</";
        strXMLOutput += HXPreferences_Constants::g_szTagConfig;
        strXMLOutput += ">\n</xml>\n";

        // Writing the preference file
        // XXXPBC Issue: We should actually use CHXFileSpecUtils::WriteTextFile, but since we're
        // using ReadBinaryFile, we have to use its counterpart.  Look at the note for reading
        // the config file to see why I chose to use ReadBinaryFile.
        CHXFileSpecifier xmlFile(strFilename);
        SPIHXBuffer spBuffer(GetCommonClassFactoryNoAddRef(), CLSID_IHXBuffer);
        spBuffer->Set((const UCHAR*)static_cast<const char*>(strXMLOutput), strXMLOutput.GetLength() + 1);
        hxres = CHXFileSpecUtils::WriteBinaryFile(xmlFile, spBuffer.Ptr(), TRUE);

        if (m_pTimeStamp)
        {
            *m_pTimeStamp = time;
        }
    }

    return hxres;
}

//
// Public Functions
//
HX_RESULT
CHXPreferences::GetPrefNoPrefix(const char* pPrefKey, UINT32 nIndex, REF(IHXBuffer*) pBuffer)
{
    HX_ASSERT(m_pMemberLock && "Why is the lock NULL?");

    HXAutoLock autoLock(m_pMemberLock); // Read Lock
    PrefKeyValMap* pPrefValueMap = _GetValueMap(pPrefKey, FALSE);
    if (pPrefValueMap)
    {
        PrefKeyValMap::Iterator mIter;
        UINT32 nCounter = 0;
        for (nCounter = 0, mIter = pPrefValueMap->Begin(); (nCounter < nIndex) && (mIter != pPrefValueMap->End()); nCounter++, mIter++);

        if ((nCounter == nIndex) && (mIter != pPrefValueMap->End()))
        {
            pBuffer = _GetPrefKeyValMapIterValue(mIter)->second.Ptr();
            HX_ADDREF(pBuffer);
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
CHXPreferences::ReadPrefNoPrefix(const char* pPrefKey, REF(IHXBuffer*) pBuffer)
{
    HX_ASSERT(pPrefKey && "Invalid Parameter");
    HX_ASSERT(m_pMemberLock && "Why is the lock NULL?");
    REQUIRE_RETURN_QUIET(pPrefKey, HXR_INVALID_PARAMETER);

    CHXString strKeyPrefix, strKeyName;
    HX_RESULT hxres = HXPreferencesUtils::SplitKey(pPrefKey, &strKeyPrefix, &strKeyName);
    if (SUCCEEDED(hxres))
    {
        hxres = HXR_FAIL;
        HXAutoLock autoLock(m_pMemberLock); // Read Lock
        PrefKeyValMap* pPrefValueMap = _GetValueMap(strKeyPrefix, FALSE);
        if (pPrefValueMap)
        {
            PrefKeyValMap::Iterator mcIter = pPrefValueMap->Find(strKeyName);
            if (mcIter != pPrefValueMap->End() && _GetPrefKeyValMapIterValue(mcIter)->first != HXPreferencesKeyStatus_Removed)
            {
		SPIHXBuffer spBuffer(GetCommonClassFactoryNoAddRef(), CLSID_IHXBuffer);
		if(spBuffer.IsValid())
		{
		    spBuffer->Set(_GetPrefKeyValMapIterValue(mcIter)->second->GetBuffer(), _GetPrefKeyValMapIterValue(mcIter)->second->GetSize());
		    spBuffer.AsPtr(&pBuffer);
		    hxres = HXR_OK;
		}
		else
		{
		    hxres = HXR_OUTOFMEMORY;
		}
            }
        }
    }
    return hxres;
}

HX_RESULT
CHXPreferences::WritePrefNoPrefix(const char* pPrefKey, IHXBuffer* pBuffer, KeyStatus ksStatus, HXBOOL bOnlyChangeUnmodifiedPrefs)
{
    HX_ASSERT(pPrefKey && "Invalid Parameter");
    HX_ASSERT(pBuffer && "Invalid Parameter");
    HX_ASSERT(m_pMemberLock && "Why is the lock NULL?");
    REQUIRE_RETURN_QUIET(pPrefKey && pBuffer, HXR_INVALID_PARAMETER);

    CHXString strKeyPrefix, strKeyName;
    HX_RESULT hxres = HXPreferencesUtils::SplitKey(pPrefKey, &strKeyPrefix, &strKeyName);

    if (SUCCEEDED(hxres))
    {
        hxres = HXR_FAIL;
        HXAutoLock autoLock(m_pMemberLock); // Write Lock
        PrefKeyValMap* pPrefValueMap = _GetValueMap(strKeyPrefix, TRUE);
        HX_ASSERT(pPrefValueMap && "Why was it not created?");
        if (pPrefValueMap)
        {
            PrefKeyValMap::Iterator mIter = pPrefValueMap->Find(strKeyName);

            // XXXPBC Issue: For every write, we are creating a new copy of the input buffer.
            // Obviously this is a huge performance hit, but it was neccessary because when
            // mountpoints are added, the code actually reuses the buffer it passes in.  Therefore
            // the data is corrupted.
            SPIHXBuffer spBuffer;
            HXBufferUtils::CopyBuffer(GetCommonClassFactoryNoAddRef(), pBuffer, spBuffer.AsInOutParam());
            if (mIter != pPrefValueMap->End())
            {
                if (!bOnlyChangeUnmodifiedPrefs ||
                    (bOnlyChangeUnmodifiedPrefs && _GetPrefKeyValMapIterValue(mIter)->first == HXPreferencesKeyStatus_Unmodified))
                {
                    // overwrite it.
                    _GetPrefKeyValMapIterValue(mIter)->first = ksStatus;
                    _GetPrefKeyValMapIterValue(mIter)->second = spBuffer.Ptr();
                }
            }
            else
            {
                pPrefValueMap->SetAt(strKeyName, new StatusValuePair(ksStatus, spBuffer.Ptr()));
            }
            hxres = HXR_OK;
        }
    }

    return hxres;
}

HX_RESULT
CHXPreferences::DeletePrefNoPrefix(const char* pPrefKey)
{
    HX_ASSERT(pPrefKey && "Invalid Parameter");
    HX_ASSERT(m_pMemberLock && "Why is the lock NULL?");
    REQUIRE_RETURN_QUIET(pPrefKey, HXR_INVALID_PARAMETER);

    CHXString strKeyPrefix, strKeyName;
    HX_RESULT hxres = HXPreferencesUtils::SplitKey(pPrefKey, &strKeyPrefix, &strKeyName);
    if (SUCCEEDED(hxres))
    {
        HXAutoLock autoLock(m_pMemberLock); // Write Lock
        PrefKeyValMap* pPrefValueMap = _GetValueMap(strKeyPrefix, FALSE);
        if (pPrefValueMap)
        {
            PrefKeyValMap::Iterator mIter = pPrefValueMap->Find(strKeyName);
            if (mIter != pPrefValueMap->End())
            {
                // if the pair exist, then set it as deleted.
                _GetPrefKeyValMapIterValue(mIter)->first = HXPreferencesKeyStatus_Removed;
            }
        }
    }
    return HXR_OK;
}

void
CHXPreferences::Close(void)
{
    Flush();
}

CHXString
CHXPreferences::_AppendPrefix(const char* szPrefKey)
{
    CHXString strRet = szPrefKey ? szPrefKey : "";
    if (!strRet.IsEmpty())
    {
        if (!m_strRootKey.IsEmpty())
        {
            strRet = m_strRootKey + HXPreferencesUtils::czPrefKeySeparator + strRet;
        }
    }
    else
    {
        return m_strRootKey;
    }
    return strRet;
}

void
CHXPreferences::_SetFilenameHelper(PrefRootMap* pReplacementKeyMap, HXLockKey* pReplacementKeyMapLock, CHXString* pStrFilePath, HXTime* pTimeStamp)
{
    HXLockKey* pPrevLock = m_pMemberLock;
    HXBOOL bWasUsingDefaultPrefs = _IsUsingDefaultPreferences();
    HXAutoLock autoLock(pReplacementKeyMapLock); // Write Lock
    {
        HXAutoLock autoLock2(m_pMemberLock); // Write Lock

        if (!bWasUsingDefaultPrefs)
        {
            // Not deleting the old lock because it is up to the callee to delete it
            _Clear(FALSE);
        }

        m_pMemberLock = pReplacementKeyMapLock;
        m_pKeyMap = pReplacementKeyMap;
        m_pStrFilePath = pStrFilePath;
        m_pTimeStamp = pTimeStamp;
    }
}

HXBOOL
CHXPreferences::_IsUsingDefaultPreferences()
{
    return m_pKeyMap == &m_DefaultKeyMap;
}

void
CHXPreferences::_Clear(HXBOOL bClearLock)
{    
    HXBOOL bUsingDefaultPrefs = TRUE;
    {
        HXAutoLock autoLock(m_pMemberLock);
        bUsingDefaultPrefs = _IsUsingDefaultPreferences();
        _ClearData();
        if (!bUsingDefaultPrefs)
        {
            HX_DELETE(m_pKeyMap);
            HX_DELETE(m_pTimeStamp);
            HX_DELETE(m_pStrFilePath);
        }
    }

    if (bClearLock && !bUsingDefaultPrefs)
    {
        HX_DELETE(m_pMemberLock);
    }
}

//
// Private Functions
//
HX_RESULT
CHXPreferences::_LoadFile(const char* szFileFullPath, HXTime* pTime)
{
    HX_RESULT hxres = HXR_OK; // Set to HXR_OK because it is ok if the file does not exist
    CHXString strFilename = szFileFullPath;

    HX_ASSERT(!strFilename.IsEmpty() && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(!strFilename.IsEmpty(), HXR_INVALID_PARAMETER);

    CHXFileSpecifier xmlFile(strFilename);
    if (CHXFileSpecUtils::FileExists(xmlFile, GetCommonClassFactoryNoAddRef()))
    {
        // Reading the preference file
        // XXXPBC Issue: We should actually use CHXFileSpecUtils::ReadTextFile, but ReadTextFile
        // only takes in a CHXString.  If we used that, then it would result in a huge performance
        // hit when the player is loaded.  As a result, we are using binary xml files.
        // Also ReadBinaryFile takes in an allocated buffer, so the only way to do by following
        // the current stand was to do create a SPIHXBuffer and get the Ptr() to that, then pass
        // it in.
        SPIHXBuffer spBuffer(GetCommonClassFactoryNoAddRef(), CLSID_IHXBuffer);
        IHXBuffer* pBuffer = spBuffer.Ptr();
        hxres = CHXFileSpecUtils::ReadBinaryFile(xmlFile, pBuffer);
        if (SUCCEEDED(hxres))
        {
            hxres = _ParseXML(spBuffer.Ptr(), pTime);
        }
    }
    return hxres;
}

// If the timestamps are equal, then this function will just overwrite the preference file. Otherwise,
// this function will only overwrite the preferences in the map that have NOT been modified
HX_RESULT
CHXPreferences::_ParseXML(IHXBuffer* pBuffer, HXTime* pTime)
{
    HX_ASSERT(m_pKeyMap && "Invalid Parameter");
    HX_ASSERT(pBuffer && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(m_pKeyMap && pBuffer, HXR_INVALID_PARAMETER);

    // Parsing the preference file
    // CRPSimpleXMLParser actually modifies the buffer, but since the buffer will
    // only exist until this function exists, we do not care if the buffer is modified
    HXTime timeStamp;
    CRPSimpleXMLParser xmlParser(const_cast<char*>(HELIX::STR_BUF(pBuffer)));
    CRPSimpleXMLTag xmlTag;
    RPXMLParseResult xmlRes;
    HXAutoLock autoLock(m_pMemberLock); // No one should be able to read/write to the keymap or timestamp while parsing the xml
    while ((xmlRes = xmlParser.NextTag(xmlTag)) != RPXMLResult_EOF)
    {
        // XXXPBC Issue: Semantic checks for a properly written config file should be done.
        // Also checks for the config and xml version should be done.  
        // TODO: 1) handle unicode strings
        if ((xmlRes == RPXMLResult_Tag) && (xmlTag.m_type == RPXMLTag_Plain))
        {
            if (strcmp(xmlTag.m_pName, HXPreferences_Constants::g_szTagPref) == 0)
            {
                // XXXPBC Issue: Implementation of additonal attributes need to be taken care of
                CRPSimpleXMLAttr* pAttrName = xmlTag.FindAttribute(HXPreferences_Constants::g_szAttributeName);
                CRPSimpleXMLAttr* pAttrValue = xmlTag.FindAttribute(HXPreferences_Constants::g_szAttributeValue);

                HX_ASSERT(pAttrName && pAttrValue && pAttrValue->m_pValue && pAttrName->m_pValue && "Something went wrong with the XML Parser");
                if (pAttrName && pAttrValue && pAttrName->m_pValue && pAttrValue->m_pValue)
                {
                    // XXXPBC Issue: Decoding escape characters this way is very inefficient because
                    // even if there are no characters to decode, it will copy the buffer into a string
                    // and end up doing nothing to it.
                    CHXString strStr = pAttrValue->m_pValue;
                    CHXPreferencesPrivateUtils::DecodeEscapeCharacters(strStr);
                    SPIHXBuffer spValue(GetCommonClassFactoryNoAddRef(), CLSID_IHXBuffer);
                    spValue->Set((const UCHAR*)(const char*)strStr, strStr.GetLength() + 1);
                    WritePrefNoPrefix(pAttrName->m_pValue, spValue.Ptr(), HXPreferencesKeyStatus_Unmodified, TRUE);
                }
            }
            else if (strcmp(xmlTag.m_pName, HXPreferences_Constants::g_szTagConfig) == 0)
            {
                CRPSimpleXMLAttr* pAttrTimeStamp = xmlTag.FindAttribute(HXPreferences_Constants::g_szAttributeTimeStamp);
                HX_ASSERT(pAttrTimeStamp && pAttrTimeStamp->m_pValue && "No timestamp?");
                if (pAttrTimeStamp && pAttrTimeStamp->m_pValue)
                {
                    CHXString strTimeStamp = pAttrTimeStamp->m_pValue;
                    HX_ASSERT(strTimeStamp.Find(HXPreferences_Constants::g_szTimeStampSeparator) != -1 && "Timestamp is malformed");
                    if (strTimeStamp.Find(HXPreferences_Constants::g_szTimeStampSeparator) != -1)
                    {
                        timeStamp.tv_sec = atoi(strTimeStamp.Left(strTimeStamp.Find(HXPreferences_Constants::g_szTimeStampSeparator))); // seconds
                        timeStamp.tv_usec = atoi(strTimeStamp.Right(strTimeStamp.Find(HXPreferences_Constants::g_szTimeStampSeparator) + 1)); // milliseconds
                        HX_ASSERT(timeStamp.tv_sec && timeStamp.tv_usec && "Bad timestamp");
                    }

                    if (pTime &&
                        timeStamp.tv_sec == pTime->tv_sec &&
                        timeStamp.tv_usec == pTime->tv_usec &&
                        (timeStamp.tv_sec != 0 || pTime->tv_usec != 0))
                    {
                        return HXR_OK;
                    }
                }
            }
        }
    }

    if (pTime)
    {
        *pTime = timeStamp;
    }

    return HXR_OK;
}

CHXPreferences::PrefKeyValMap*
CHXPreferences::_GetValueMap(const char* szPref, HXBOOL bCreate)
{
    HX_ASSERT(m_pKeyMap && "Invalid Parameter");
    HX_ASSERT(szPref && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(m_pKeyMap && szPref, NULL);

    PrefKeyValMap* pRetPrefValueMap = NULL;
    HXAutoLock autoLock(m_pMemberLock); // Read / Write Lock
    PrefRootMap::Iterator mcIter = m_pKeyMap->Find(szPref ? szPref : "");
    if (mcIter != m_pKeyMap->End())
    {
        pRetPrefValueMap = _GetPrefRootMapIterValue(mcIter);
    }
    else if (bCreate)
    {
        pRetPrefValueMap = new PrefKeyValMap;
        if (pRetPrefValueMap)
        {
            m_pKeyMap->SetAt(szPref, pRetPrefValueMap);
        }
    }
    return pRetPrefValueMap;
}

void
CHXPreferences::_ClearData()
{
    if (m_pKeyMap)
    {
        HXAutoLock autoLock(m_pMemberLock); // Write Lock
        for (PrefRootMap::Iterator mIter = m_pKeyMap->Begin(); mIter != m_pKeyMap->End(); mIter++)
        {
            PrefKeyValMap* pCurrentMap = _GetPrefRootMapIterValue(mIter);
            if (pCurrentMap)
            {
                for (PrefKeyValMap::Iterator mIter2 = pCurrentMap->Begin(); mIter2 != pCurrentMap->End(); mIter2++)
                {
                    PrefKeyValMapValue* pCurrentValue = _GetPrefKeyValMapIterValue(mIter2);
                    HX_DELETE(pCurrentValue);
                }
                pCurrentMap->RemoveAll();
            }
            HX_DELETE(pCurrentMap);
        }
        m_pKeyMap->RemoveAll();
    }
}

CHXString
CHXPreferences::_GetPrefRootMapIterKey(PrefRootMap::Iterator& iter)
{
    return iter.get_key();
}

CHXPreferences::PrefKeyValMap*
CHXPreferences::_GetPrefRootMapIterValue(PrefRootMap::Iterator& iter)
{
    return ((PrefKeyValMap*) *iter);
}

CHXString
CHXPreferences::_GetPrefKeyValMapIterKey(PrefKeyValMap::Iterator& iter)
{
    return iter.get_key();
}

CHXPreferences::PrefKeyValMapValue*
CHXPreferences::_GetPrefKeyValMapIterValue(PrefKeyValMap::Iterator& iter)
{
    return ((PrefKeyValMapValue*) *iter);
}

//
// Private Utility Implementation
//
namespace CHXPreferencesPrivateUtils
{
    void
    EncodeEscapeCharacters(REF(CHXString) /* in/out */ str)
    {
        for (INT32 i = 0; i < HXPreferences_Constants::g_uEscapeCharacters; i++)
        {
            str.FindAndReplace(HXPreferences_Constants::g_EscapeCharacters[i][1], HXPreferences_Constants::g_EscapeCharacters[i][0], TRUE);
        }
    }

    void
    DecodeEscapeCharacters(REF(CHXString) /* in/out */ str)
    {
        for (INT32 i = HXPreferences_Constants::g_uEscapeCharacters - 1; i >= 0; i--)
        {
            str.FindAndReplace(HXPreferences_Constants::g_EscapeCharacters[i][0], HXPreferences_Constants::g_EscapeCharacters[i][1], TRUE);
        }
    }
};
//
// End of CHXPreferencesPrivateUtils namespace
//
