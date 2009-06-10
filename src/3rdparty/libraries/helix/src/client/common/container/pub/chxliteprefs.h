/* ***** BEGIN LICENSE BLOCK *****
 * 
 * Portions Copyright (c) 1995-2008 RealNetworks, Inc. All Rights Reserved.
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

#ifndef __chxliteprefs_h
#define __chxliteprefs_h


#include "hxtypes.h"
#include "hxmap.h"
#include "unkimp.h"
#include "hxstring.h"
#include "hxprefs.h"
#include "hxengin.h"
#include "ihxcontextuser.h"


class CHXStringList;
class CHXDataFile;
struct IHXCommonClassFactory;
class HXMutex;

// helpers local to this module
namespace LitePrefs
{
void ClearPrefs(CHXMapStringToOb* pPrefs);
CHXDataFile* OpenPrefFile(const char* pPath, UINT16 mode, IHXCommonClassFactory* pFactory);
HX_RESULT WritePrefFile(const char* pPath, CHXStringList& shadows, const CHXMapStringToOb& prefs, IHXCommonClassFactory* pFactory);
HX_RESULT WritePrefs(const CHXMapStringToOb* pPrefs, const char* pPath, IHXCommonClassFactory*	pFactory);
HX_RESULT ReadPrefs(const char* pPath,
			      CHXMapStringToOb* pPrefs, const CHXString& strShadowPath, IHXCommonClassFactory*	pFactory);
HX_RESULT ParsePrefs(CHXDataFile* pFile,
			       CHXMapStringToOb* pPrefs,
			       CHXStringList* pShadows);
HX_RESULT StorePref(CHXMapStringToOb* pPrefs,
			      const char* pName,
			      const char* pValue,
			      HXBOOL bChanged = TRUE,
			      HXBOOL bPersist = TRUE);			      
HX_RESULT RetrievePref(IHXCommonClassFactory* pFactory,
				 const CHXMapStringToOb* pPrefs,
				 const char* pName,
				 REF(IHXBuffer*) pValue);
void FindNewOrAlteredPrefs(const CHXMapStringToOb& memPrefs, 
                  const CHXMapStringToOb& origPrefs, 
                  CHXMapStringToOb& prefsOut);

HXBOOL SkipToken(char*& pos, INT32& nCount, char term);
HXBOOL ParseToken(char*& pos, INT32& nCount, char term, CHXString& token);
CHXString GetBasePath(const CHXString& strPath);
}



enum ParseState
{
    eParsingWhiteSpace,
    eParsingName,
    eParsingValue,
    eParsingShadow,
    eParsingComment
};

// for keeping track of a pref value
class Pref
{
public:
    Pref(const char* pStr): m_bChanged(FALSE), m_bPersist(TRUE), m_strValue(pStr) {}

    HXBOOL HasChanged() const { return m_bChanged;}
    void SetChanged(HXBOOL bChanged) { m_bChanged = bChanged;}
    void SetValue(const char* pStr) { m_strValue = pStr;}
    HXBOOL ToBePersisted() { return m_bPersist;}
    void SetPersistence(HXBOOL bPersist) { m_bPersist = bPersist;}
    const char* Buffer() const { return m_strValue;}
    UINT32 Size() const {return m_strValue.GetLength() + 1;}

private:
    HXBOOL    m_bChanged;
    HXBOOL    m_bPersist;
    CHXString m_strValue;
};
class CHXLitePrefs : public IHXPreferences,
		     public IHXPreferences3,
		     public IHXContextUser,
#if !defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)
		     public IHXCallback,
#endif
		     public CUnknownIMP
{
public:
    CHXLitePrefs(const char* pRootPath = NULL, const char* pShadowPath = NULL);
    virtual ~CHXLitePrefs();

    DECLARE_UNKNOWN(CHXLitePrefs);

    // CHXLitePrefs
    STDMETHOD(Commit)		        (THIS);
    STDMETHOD(SetAutoCommit)            (THIS_ bool bAutoCommit);
    STDMETHOD(ResetPrefs)               (THIS);

    // IHXPreferences
    STDMETHOD(ReadPref)		(THIS_ const char* pName,
					REF(IHXBuffer*) pValue);
    STDMETHOD(WritePref)	(THIS_ const char* pName,
					IHXBuffer* pValue);

    // IHXPreferences3
    STDMETHOD(Open)		(THIS_ const char* pCompanyName,
					const char* pProductName,
					ULONG32 nProdMajorVer,
					ULONG32 nProdMinorVer);
    STDMETHOD(OpenShared)	(THIS_ const char* pCompanyName);
    STDMETHOD(DeletePref)	(THIS_ const char* pPrekKey);

    STDMETHOD(RegisterContext)	(THIS_ IUnknown* pContext);

#if !defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)
    // IHXCallback
    STDMETHOD(Func)		(THIS);
#endif // (HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)

    void                        Close(void);

protected:
    CHXMapStringToOb		m_prefs;
    CHXString               m_strShadowPath;
    IHXCommonClassFactory*	m_pFactory;

private:

    CHXString GetFullPath(const CHXString& strFileName);
    void PossiblyCommitPrefChange();

    

    IHXMutex*			m_pMutex;
    IHXScheduler*		m_pScheduler;
    CallbackHandle		m_hCallback;
    CHXString                   m_strRootPath;
    CHXString                   m_strFileName;
    bool                        m_bAutoCommit;
};


#endif /* #ifndef __chxliteprefs_h */
