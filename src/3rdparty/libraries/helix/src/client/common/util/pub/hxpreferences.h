/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpreferences.h,v 1.6 2006/05/18 18:25:00 jeffl Exp $
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

#ifndef _HXPREFERENCES_H_
#define _HXPREFERENCES_H_

// Inherit
#include "unkimp.h"             // CUnknownIMP
#include "hxcontextusermixin.h" // CHXContextUserMixin
#include "hxprefs.h"            // IHXPreferences#
#include "ihxpreferencesfile.h" // IHXPreferencesFile

// Member variables
#include "hxstring.h"                   // CHXString
#include "hxthreadsync.h"               // HXLockKey
#include "hxtime.h"                     // HXTime
#include "chxmapstringtoob.h"           // CHXMapStringToOb

//
// CHXPreferences Class Declaration
//
class CHXPreferences: public CUnknownIMP
                    , public CHXContextUserMixin
                    , public IHXPreferences
                    , public IHXPreferences2
                    , public IHXPreferences3
                    , public IHXPreferencesFile
{
    DECLARE_UNKNOWN(CHXPreferences)

public:
    typedef enum KeyStatus
    {
        HXPreferencesKeyStatus_Unmodified = 0,
        HXPreferencesKeyStatus_Modified,
        HXPreferencesKeyStatus_Removed
    } KeyStatus;

    class StatusValuePair
    {
    public:
        StatusValuePair(KeyStatus status, IHXBuffer* pBuffer) :
            first(status),
            second(pBuffer)
        {}

        KeyStatus first;
        SPIHXBuffer second;
    };

    typedef CHXMapStringToOb PrefRootMap; // PrefRootMap contain PrefKeyValMap objects
    typedef CHXMapStringToOb PrefKeyValMap; // HXPreferences_PrefKeyValueMap contain PrefKeyValMapValue
    typedef StatusValuePair PrefKeyValMapValue;

public:
    CHXPreferences();
    virtual ~CHXPreferences();

    //
    // IHXPreferences
    //
    STDMETHOD(ReadPref)     (THIS_ const char* pPrefKey, REF(IHXBuffer*) pBuffer);
    STDMETHOD(WritePref)    (THIS_ const char* pPrefKey, IHXBuffer* pBuffer);

    //
    // IHXPreferences2
    //
    STDMETHOD(GetPreferenceEnumerator)  (THIS_ REF(IHXPreferenceEnumerator*) pEnum);
    STDMETHOD(ResetRoot)                (THIS_ const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer);

    //
    // IHXPreferences3
    //
    STDMETHOD(Open)         (THIS_ const char* pCompanyName, const char* pProductName, ULONG32 nProdMajorVer, ULONG32 nProdMinorVer);
    STDMETHOD(OpenShared)   (THIS_ const char* pCompanyName);
    STDMETHOD(DeletePref)   (THIS_ const char* pPrefKey);

    //
    // IHXPreferencesFile
    //
    STDMETHOD(SetFilename)  (THIS_ const char* szFilePath, HXBOOL bUseDefaultPreferences = TRUE);
    STDMETHOD(Flush)        (THIS);

    // Public Functions
    // These functions use the pref key and do NOT attach a prefix to the key, unlike
    // the functions in the IHXPreferences and IHXPreferences3
    HX_RESULT GetPrefNoPrefix       (const char* pPrefKey, UINT32 nIndex, REF(IHXBuffer*) pBuffer);
    HX_RESULT ReadPrefNoPrefix      (const char* pPrefKey, REF(IHXBuffer*) pBuffer);
    HX_RESULT WritePrefNoPrefix     (const char* pPrefKey, IHXBuffer* pBuffer, KeyStatus ksStatus = HXPreferencesKeyStatus_Modified, HXBOOL bOnlyChangeUnmodifiedPrefs = FALSE);
    HX_RESULT DeletePrefNoPrefix    (const char* pPrefKey);
    void Close(void);

protected:
    CHXString _AppendPrefix                 (const char* szPrefKey);
    void _SetFilenameHelper                 (PrefRootMap* pReplacementKeyMap, HXLockKey* pReplacementKeyMapLock, CHXString* pStrFilePath, HXTime* pTimeStamp);
    inline HXBOOL _IsUsingDefaultPreferences  ();
    inline void _Clear                      (HXBOOL bClearLock);

    // Member Variables
    CHXString       m_strRootKey;

private:
    HX_RESULT _LoadFile         (const char* szFileFullPath, HXTime* pTime);
    HX_RESULT _ParseXML         (IHXBuffer* pBuffer, HXTime* pTime);
    PrefKeyValMap* _GetValueMap (const char* szPref, HXBOOL bCreate = FALSE);
    void _ClearData             ();

    inline CHXString _GetPrefRootMapIterKey                 (PrefRootMap::Iterator& iter);
    inline PrefKeyValMap* _GetPrefRootMapIterValue          (PrefRootMap::Iterator& iter);
    inline CHXString _GetPrefKeyValMapIterKey               (PrefKeyValMap::Iterator& iter);
    inline PrefKeyValMapValue* _GetPrefKeyValMapIterValue   (PrefKeyValMap::Iterator& iter);

    // These variables can either point to the static variables OR point to a new object
    // that has no relations with the static variables.  So any access to these variables
    // should be LOCKED with m_pMemberLock
    HXLockKey*      m_pMemberLock;
    CHXString*      m_pStrFilePath;
    HXTime*         m_pTimeStamp;
    PrefRootMap*    m_pKeyMap;

    // Static Member Variables
    // These member variables are static because of legacy issues.  Previously prefs
    // were tightly tied with the windows registry.  As a result, we need to use a
    // static map so that if this object is created without using the objectbroker,
    // then they would not share the same map.  This simulates a windows registry
    // per process.
    static HXLockKey    m_DefaultLock;
    static CHXString    m_strDefaultFilePath;
    static HXTime       m_DefaultTimeStamp;
    static PrefRootMap  m_DefaultKeyMap;
    static INT32        m_lDefaultKeyMapRefCount;
};


#endif // _HXPREFERENCES_H_
