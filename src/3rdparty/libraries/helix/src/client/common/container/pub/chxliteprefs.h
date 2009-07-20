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
			      HXBOOL bChanged = TRUE);
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



#ifdef _SYMBIAN
class CHXLitePrefsExt : public CHXLitePrefs
{
    
    public:
    CHXLitePrefsExt(const char* pRootPath = NULL, const char* pShadowPath = NULL);
    virtual ~CHXLitePrefsExt();
    STDMETHOD(OpenPrefFile)     (THIS_ const char* pFullFilepath);
};
#endif

#endif /* #ifndef __chxliteprefs_h */
