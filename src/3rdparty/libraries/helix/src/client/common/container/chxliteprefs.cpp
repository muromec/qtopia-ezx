#include "chxliteprefs.h"
#include "hlxclib/fcntl.h"
#include "chxdataf.h"
#include "hxslist.h"
#include "hxstring.h"
#include "hxdir.h"
#include "hxccf.h"
#include "ihxpckts.h"
#include "hxmutex.h"
#include <ctype.h>
#include "hlxclib/stdlib.h"
#include "pathutil.h"
#include "hxthread.h"
#include "pckunpck.h"


BEGIN_INTERFACE_LIST(CHXLitePrefs)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferences)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferences3)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXContextUser)
END_INTERFACE_LIST


// for keeping track of a pref value
class Pref
{
public:
    Pref(const char* pStr): m_bChanged(FALSE), m_strValue(pStr){}

    HXBOOL HasChanged() const { return m_bChanged;}
    void SetChanged(HXBOOL bChanged) { m_bChanged = bChanged;}
    void SetValue(const char* pStr) { m_strValue = pStr;}
    
    const char* Buffer() const { return m_strValue;}
    UINT32 Size() const {return m_strValue.GetLength() + 1;}

private:
    HXBOOL    m_bChanged;
    CHXString m_strValue;
};



//
// get base path part of full filename path (strip off filename)
//
inline
CHXString LitePrefs::GetBasePath(const CHXString& strPath)
{
    CHXString strBase;
    INT32 idxSep = strPath.ReverseFind(OS_SEPARATOR_CHAR);
    if( idxSep != -1 )
    {
        strBase = strPath.Left(idxSep + 1);
    }
    return strBase;
}

//
// go through current prefs (in memory) and determine which entries
// are: a) set in a shadow but now have a different value; or b) not
// in a shadow to begin with
//
inline void
LitePrefs::FindNewOrAlteredPrefs(const CHXMapStringToOb& memPrefs, 
                  const CHXMapStringToOb& origPrefs, 
                  CHXMapStringToOb& prefsOut)
{
    // for each current pref setting (cast away const to work around deficiency in map inteface)...
    CHXMapStringToOb& memPrefs_ = (CHXMapStringToOb&)memPrefs;
    CHXMapStringToOb::Iterator iterEnd = memPrefs_.End();
    for(CHXMapStringToOb::Iterator iter = memPrefs_.Begin(); iter != iterEnd; ++iter)
    {
        Pref* pPref = (Pref*)*iter;
        const char* pKey = iter.get_key();
        HX_ASSERT(pPref);
        HX_ASSERT(pKey);
    
        // transfer this pref if is new or altered
        if( pPref->HasChanged() || (0 != origPrefs.Lookup(pKey)) )
        {
            prefsOut.SetAt(pKey, pPref);
        }
    }
}

//
// Prefs are read from primary file, then shadows. Settings in primary file override
// those in shadows (which are essentially defaults). Therefore the first value that 
// is found for a given pref is the value that remains in effect.
//
HX_RESULT 
LitePrefs::ReadPrefs(const char* pPath,
			CHXMapStringToOb* pPrefs, const CHXString& strShadowPath, IHXCommonClassFactory* pFactory)
{
    HX_RESULT res = HXR_FAIL;
    CHXDataFile* pFile = LitePrefs::OpenPrefFile(pPath, O_RDONLY, pFactory);
    if (pFile)
    {
        CHXStringList shadows;
        res = LitePrefs::ParsePrefs(pFile, pPrefs, &shadows);
        pFile->Close();

        // use base path from current pref file to locate regerenced shadow file
        // only if shadow path is NULL. shadow files are the linked files.
        CHXString strBase = strShadowPath;
        if(strBase.IsEmpty())
        {
            strBase = LitePrefs::GetBasePath(pPath);
        }
    
        LISTPOSITION i = shadows.GetHeadPosition();
        while(i)
        {
	    const CHXString& strShadowFile = *((CHXString*) shadows.GetNext(i));
            CHXString strPath = HXPathUtil::CombinePath(strBase, strShadowFile);
	    res = LitePrefs::ReadPrefs(strPath, pPrefs, strBase, pFactory);
        }
	    
        HX_DELETE(pFile);
    }
    return res;
}


// helper
CHXDataFile*
LitePrefs::OpenPrefFile(const char* pPath, UINT16 mode, IHXCommonClassFactory*	pFactory)
{
    CHXDataFile* pFile = CHXDataFile::Construct((IUnknown*) pFactory);
    if (pFile)
    {
	HX_RESULT res = pFile->Open(pPath, mode, TRUE);
	if (FAILED(res))
	{
            HX_DELETE(pFile);
	}
    }
    return pFile;
}

//
// write out the given file in our pref file format
//
HX_RESULT
LitePrefs::WritePrefFile(const char* pPath, CHXStringList& shadows, const CHXMapStringToOb& prefs, IHXCommonClassFactory* pFactory)
{
    HX_RESULT hr = HXR_FAIL;
    CHXDataFile* pFile = LitePrefs::OpenPrefFile(pPath, O_WRONLY | O_CREAT | O_TRUNC, pFactory);
    if (pFile)
    {
        //
        // write the shadow pref file references
        //
        LISTPOSITION j = shadows.GetHeadPosition();
        while(j)
	{
	    const CHXString& fileName = *((CHXString*) shadows.GetNext(j));
	    // format shadow, write to file
	    pFile->Write("[", 1);
	    pFile->Write(fileName, fileName.GetLength());
	    pFile->Write("]\n", 2);
	}

        //
        // write out preference name/value entries
        //

        // cast away const to work around deficiency in map interface...
        CHXMapStringToOb& prefs_ = (CHXMapStringToOb&)prefs;

        CHXMapStringToOb::Iterator iterEnd = prefs_.End();
        for(CHXMapStringToOb::Iterator iter = prefs_.Begin(); iter != iterEnd; ++iter)
        {
            const char* pPrefKey = iter.get_key();
            Pref* pPref = (Pref*)*iter;
            
	    pFile->Write(pPrefKey, strlen(pPrefKey));
	    pFile->Write("=", 1);

	    if (pPref->Size() > 1)
	    {
		// don't write the null terminator
		pFile->Write(pPref->Buffer(), pPref->Size()-1);
	    }
	    
	    pFile->Write("\n", 1);
	}
        HX_DELETE(pFile);
        hr = HXR_OK;
    }
    return hr;
}




HX_RESULT
LitePrefs::WritePrefs(const CHXMapStringToOb* pPrefs, const char* pPath, IHXCommonClassFactory*	pFactory)
{
    HX_RESULT res = HXR_FAIL;
    CHXDataFile* pFile = LitePrefs::OpenPrefFile(pPath, O_RDONLY, pFactory);
    if (pFile)
    {
	// read current preference settings from files
	CHXStringList shadows;
	CHXMapStringToOb origPrefs;
	res = LitePrefs::ParsePrefs(pFile, &origPrefs, &shadows);
        HX_DELETE(pFile);
	if (SUCCEEDED(res))
	{
            // determine altered or new preferences
            CHXMapStringToOb newPrefs;
            FindNewOrAlteredPrefs(*pPrefs, origPrefs, newPrefs);

            // write out the prefs with our up-to-date values
            LitePrefs::WritePrefFile(pPath, shadows, newPrefs, pFactory);
	}
        
	LitePrefs::ClearPrefs(&origPrefs);
    }
	
    return res;
}

HX_RESULT 
LitePrefs::StorePref(CHXMapStringToOb* pPrefs,
	const char* pName,
	const char* pValue,
	HXBOOL bChanged)
{
    Pref* pPref = NULL;
    if (pPrefs->Lookup(pName, (void*&)pPref))
    {
        // update existing pref
        pPref->SetValue(pValue);
    }
    else
    {   
        // create and add new pref
        pPref = new Pref(pValue);
        if(!pPref)
        {
            return HXR_OUTOFMEMORY;
        }
        pPrefs->SetAt(pName, pPref);
    }
    
    pPref->SetChanged(bChanged);
    return HXR_OK;
}


HX_RESULT
LitePrefs::ParsePrefs(CHXDataFile* pFile,
	 CHXMapStringToOb* pPrefs,
	 CHXStringList* pShadows)
{
    HX_ASSERT(pFile && pPrefs && pShadows);

    #define BUF_SZ	0x0400	// read file in 1k chunks

    ParseState eState = eParsingWhiteSpace;
    INT32 nCount = 0;
    CHXString strBuf;
    char* pos = 0;
    char* buf = strBuf.GetBuffer(BUF_SZ);

    CHXString sName;
    CHXString sValue;
    for (;;)
    {
	// read more data
	if (nCount == 0)
	{
	    nCount = (INT32)pFile->Read(buf, BUF_SZ);
	    if (nCount <= 0)
            {
                // end of file
		break;
            }
	    pos = buf;
	}
        switch(eState)
        {
        case eParsingValue:
        {
            if (LitePrefs::ParseToken(pos, nCount, '\n', sValue))
            {
                eState = eParsingWhiteSpace;
            
                // don't add this name value if name already exists in prefs
                if(0 ==  pPrefs->Lookup(sName))
                {
                    HX_RESULT res = LitePrefs::StorePref(pPrefs, sName, sValue, FALSE);
                    if (FAILED(res))
                    {
                        return res;
                    }
                }
                sName.Empty();
                sValue.Empty();
            }
            
        }
        break;
        case eParsingComment:
        {
            // skip to end of line
            if (LitePrefs::SkipToken(pos, nCount, '\n'))
            {
                eState = eParsingWhiteSpace;
            }
        }
        break;
        case eParsingName:
        {
            // name is everything up to '='
            if (LitePrefs::ParseToken(pos, nCount, '=', sName))
            {
                eState = eParsingValue;
            }
        }
        break;
        case eParsingShadow:
        {
            // shadow reference is everything up to closing ']'
            if (LitePrefs::ParseToken(pos, nCount, ']', sName))
            {
                eState = eParsingComment;
          
                // queue this up for parsing
                pShadows->AddTailString(sName);
                sName.Empty();
            }
        }
        break;
        case eParsingWhiteSpace:
        {
                // we're looking for something to parse
                switch (*pos)
                {
                    case '[':
                    {
                        eState = eParsingShadow;
                    }
                    break;

                    case '#':
                    case ';':
                    {
                        eState = eParsingComment;
                    }
                    break;

                    default:
                    {
                        eState = eParsingName;
                        sName = *pos; // don't lose this char
                    }
                }
                ++pos;
                --nCount;
        }
        break;
        default:
            HX_ASSERT(false);
            break;
        }
    }

    return HXR_OK;
}



void
LitePrefs::ClearPrefs(CHXMapStringToOb* pPrefs)
{
    POSITION pos = pPrefs->GetStartPosition();
    while(pos != NULL)
    {
        Pref * pPref = NULL;
        char * szPrefKey = NULL;
        pPrefs->GetNextAssoc( pos, (const char*&) szPrefKey, (void*&) pPref );
        HX_ASSERT(pPref);
        delete pPref;
    }
    pPrefs->RemoveAll();
}

HX_RESULT 
LitePrefs::RetrievePref(IHXCommonClassFactory* pFactory,
			   const CHXMapStringToOb* pPrefs,
			   const char* pName,
			   REF(IHXBuffer*) pValue)
{
    HX_RESULT res = HXR_FAIL;

    Pref* pPref = NULL;
    if (pPrefs->Lookup(pName, (void*&)pPref))
    {
	res = pFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pValue);
	if (SUCCEEDED(res))
	{
	    res = pValue->Set((const UCHAR*)pPref->Buffer(), 
				   pPref->Size());
	}
    }

    return res;
}

HXBOOL
LitePrefs::SkipToken(char*& pos, INT32& nCount, char term)
{
    HXBOOL bRet = FALSE;

    while (*pos != term && nCount != 0)
    {
        ++pos;
        --nCount;
    }

    if( nCount != 0 )
    {
        // skip term
        ++pos;
        --nCount;
        bRet = TRUE;
    }

    return bRet;
}

HXBOOL
LitePrefs::ParseToken(char*& pos, INT32& nCount, char term, CHXString& token)
{
    HXBOOL bRet = FALSE;

    while (*pos != term && nCount != 0)
    {
        token += *pos;
        ++pos;
        --nCount;
    }

    if( nCount != 0 )
    {
        // skip term
        ++pos;
        --nCount;

        // trim white space around token
        token.TrimLeft();
        token.TrimRight();

        bRet = TRUE;
    }

    return bRet;
}



//
// class CHXLitePrefs
//
CHXLitePrefs::CHXLitePrefs(const char* pRootPath, const char *pShadowPath)
: m_pMutex(NULL)
, m_pFactory(NULL)
, m_pScheduler(NULL)
, m_hCallback((CallbackHandle)NULL)
, m_strRootPath(pRootPath)
, m_strShadowPath(pShadowPath)
, m_bAutoCommit(true)
{
    m_prefs.SetCaseSensitive(FALSE);
}


CHXLitePrefs::~CHXLitePrefs()
{
    Close();
}

inline
CHXString CHXLitePrefs::GetFullPath(const CHXString& strFileName)
{
    return HXPathUtil::CombinePath(m_strRootPath, strFileName);
}

void
CHXLitePrefs::Close()
{
#if !defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)
    // remove pending callback
    if (m_hCallback)
    {
	m_pScheduler->Remove(m_hCallback);
        m_hCallback = (CallbackHandle)NULL;
    }
#endif // (HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)

    // commit changes
    Commit();

    // cleanup
    LitePrefs::ClearPrefs(&m_prefs);
    HX_RELEASE(m_pFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pMutex);
}


// IHXPreferences

STDMETHODIMP
CHXLitePrefs::ReadPref(const char* pName, REF(IHXBuffer*) pValue)
{
    HX_RESULT res = HXR_FAIL;

    if (pName && m_pFactory)
    {
	m_pMutex->Lock();

	res = LitePrefs::RetrievePref(m_pFactory, &m_prefs, pName, pValue);

	m_pMutex->Unlock();
    }

    return res;
}

STDMETHODIMP
CHXLitePrefs::SetAutoCommit(bool bAutoCommit)
{
    // if false, client must explicitly call Commit() to save changes after write
    m_bAutoCommit = bAutoCommit;
    return HXR_OK;
}

void
CHXLitePrefs::PossiblyCommitPrefChange()
{
    if (m_bAutoCommit)
    {
#if defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)      
        HX_VERIFY(SUCCEEDED(Commit()));
#else
        // schedule a callback and commit the changes
        // so that multiple WritePref calls result in only one write to file
	if (!m_hCallback && m_pScheduler)
	{
	    m_hCallback = m_pScheduler->RelativeEnter(this, 0);
	    HX_ASSERT(m_hCallback);
	}
	else if (!m_pScheduler)
	{
	    // lame... no scheduler!
	    HX_VERIFY(SUCCEEDED(Commit()));
	}
#endif
    }
}

STDMETHODIMP
CHXLitePrefs::WritePref(const char* pName, IHXBuffer* pValue)
{
    HX_RESULT res = HXR_FAIL;

    if (pName && pValue)
    {
	m_pMutex->Lock();
	res = LitePrefs::StorePref(&m_prefs, pName, (const char*)pValue->GetBuffer());
        if(SUCCEEDED(res))
        {
            PossiblyCommitPrefChange();
        }
        m_pMutex->Unlock();
    }

    return res;
}

// IHXPreferences3
STDMETHODIMP
CHXLitePrefs::Open(const char* pCompanyName,
		   const char* pProductName,
		   ULONG32 nProdMajorVer,
		   ULONG32 nProdMinorVer)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pProductName)
    {
        m_strFileName.Format("%s_%lu_%lu.cfg",pProductName, nProdMajorVer, nProdMinorVer); 

	LitePrefs::ClearPrefs(&m_prefs);
    
        CHXString strPath = GetFullPath(m_strFileName);
        res = LitePrefs::ReadPrefs(strPath, &m_prefs, m_strShadowPath, m_pFactory);
    }
    return res;
}


STDMETHODIMP
CHXLitePrefs::OpenShared(const char* pCompanyName)
{
    return HXR_NOTIMPL; //Open(pCompanyName, "Shared", 0, 0);
}


STDMETHODIMP
CHXLitePrefs::DeletePref(const char* pPrefKey)
{
    HX_RESULT hr = HXR_FAIL;
    Pref* pPref = NULL;

    m_pMutex->Lock();
    if (m_prefs.Lookup(pPrefKey, (void*&)pPref))
    {
	// delete the pref
        m_prefs.RemoveKey(pPrefKey);
	delete pPref;

        PossiblyCommitPrefChange();
        hr = HXR_OK;
    }
    m_pMutex->Unlock();

    return hr;
}

//
// Restore primary pref file so it contains no pref
// settings (and update current prefs accordingly) 
//
STDMETHODIMP
CHXLitePrefs::ResetPrefs()
{
    m_pMutex->Lock();
    // force re-write of file so only shadow references remain.
    LitePrefs::ClearPrefs(&m_prefs);
    Commit();

    // re-build memory prefs (so default values from shadows are set)
    CHXString strPrimaryFilePath = GetFullPath(m_strFileName);
    HX_RESULT hr = LitePrefs::ReadPrefs(strPrimaryFilePath, &m_prefs, m_strShadowPath, m_pFactory);
    m_pMutex->Unlock();

    return hr;
}


STDMETHODIMP
CHXLitePrefs::RegisterContext(IUnknown* pContext)
{
    HX_RESULT res = HXR_FAIL;

    if (!m_pFactory && pContext)
    {
	res = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pFactory);
	HX_ASSERT(SUCCEEDED(res));

#if !defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)
	res = pContext->QueryInterface(IID_IHXScheduler,
				       (void**)&m_pScheduler);
	HX_ASSERT(SUCCEEDED(res));
#endif // (HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)

	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);	
    }

    return res;
}


STDMETHODIMP
CHXLitePrefs::Commit()
{
    HX_RESULT res = HXR_NOT_INITIALIZED;
    
    if (m_strFileName)
    {
	m_pMutex->Lock();

        CHXString strPath = GetFullPath(m_strFileName);
	res = LitePrefs::WritePrefs(&m_prefs, strPath, m_pFactory);

	m_pMutex->Unlock();
    }

    return res;
}

// IHXCallback
#if !defined(HELIX_CONFIG_LITEPREFS_SLUGGISH_OUT)
STDMETHODIMP
CHXLitePrefs::Func()
{
    m_hCallback = (CallbackHandle)NULL;

    HX_VERIFY(SUCCEEDED(Commit()));

    return HXR_OK;
}
#endif


#ifdef _SYMBIAN
CHXLitePrefsExt::CHXLitePrefsExt(const char *pRootPath, const char *pShadowPath):
            CHXLitePrefs(pRootPath, pShadowPath)
{
    //empty
}

CHXLitePrefsExt::~CHXLitePrefsExt()
{
    //empty
}

STDMETHODIMP
CHXLitePrefsExt::OpenPrefFile(const char * pPrefFile)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pPrefFile)
    {

	    LitePrefs::ClearPrefs(&m_prefs);
        CHXString strPath = pPrefFile;
        res = LitePrefs::ReadPrefs(strPath, &m_prefs, m_strShadowPath, m_pFactory);
    }

    return res;
}
#endif

