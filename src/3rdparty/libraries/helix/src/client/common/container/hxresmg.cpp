/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxresmg.cpp,v 1.14 2006/02/09 01:09:52 ping Exp $
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

#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#ifdef _WINDOWS
#include <windows.h>
#endif

#if defined(_MACINTOSH) && !defined(_MAC_MACHO)
#include <stat.h>
#else
#include "hlxclib/sys/stat.h"
#endif
#include "hxxrsmg.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "hxcomm.h"
#include "hxxres.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxprefs.h"
#include "hxmon.h"

#include "hxslist.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "md5.h"
#include "findfile.h"
#include "hxresmg.h"
#include "dllpath.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

// IHXPreferences key defines
#define EXT_RES_DIR		"ExtResources"
#define EXT_RES_DATA_KEY	"ExternalResourcesData"
#define FILE_INFO_KEY		"FileInfo"
#define CHECKSUM_KEY		"XRSCheckSum"
#define LANGUAGE_KEY		"Language"
#define LANG_ID_KEY		"LangID"

// for OS_SEPARATOR_CHAR and OS_SEPARATOR_STRING
#include "hxdir.h"

// static initializations
#if !defined(HELIX_CONFIG_NOSTATICS)
HXExternalResourceManager* HXExternalResourceManager::m_pResourceManager = NULL;
#else
#include "globals/hxglobals.h"
const HXExternalResourceManager* const HXExternalResourceManager::m_pResourceManager = NULL;
#endif


// HXExternalResourceManager methods
HXExternalResourceManager::HXExternalResourceManager(IUnknown* pContext):
     m_lRefCount(0)
    ,m_pResourceList(NULL)
    ,m_pContext(pContext)
    ,m_pHXXResPlugin(NULL)
    ,m_pPrefs(NULL)
    ,m_pRegistry(NULL)
    ,m_ulLanguageID(0x0409)
    ,m_pExternalResDir(NULL)

{
    if(m_pContext)
    {
	m_pContext->AddRef();
    }
    Init();
}

HXExternalResourceManager::~HXExternalResourceManager()
{
    delete[] m_pExternalResDir;

    if(m_pResourceList)
    {
	CHXSimpleList::Iterator i = m_pResourceList->Begin();
	for (; i != m_pResourceList->End(); ++i)
	{
	    IHXXResFile* pRes = (IHXXResFile*)(*i);
	    pRes->Close();
	    pRes->Release();
	}
    }

    HX_DELETE(m_pResourceList);
    HX_RELEASE(m_pPrefs);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pHXXResPlugin);

#if defined(HELIX_CONFIG_NOSTATICS)
    HXExternalResourceManager*& m_pResourceManager =
	(HXExternalResourceManager*&)HXGlobalPtr::Get(
	    &HXExternalResourceManager::m_pResourceManager);
#endif
    m_pResourceManager = NULL;
}

HXExternalResourceManager*
HXExternalResourceManager::Instance(IUnknown* pContext)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    HXExternalResourceManager*& m_pResourceManager =
	(HXExternalResourceManager*&)HXGlobalPtr::Get(
	    &HXExternalResourceManager::m_pResourceManager);
#endif

    if(!m_pResourceManager)
    {
	m_pResourceManager = new HXExternalResourceManager(pContext);
    }

    if (m_pResourceManager)
    {
	m_pResourceManager->AddRef();
    }

    return m_pResourceManager;
}


// IUnknown methods

STDMETHODIMP 
HXExternalResourceManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXExternalResourceManager), (IHXExternalResourceManager*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXExternalResourceManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXExternalResourceManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXExternalResourceManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

// IHXExternalResourceManager methods

STDMETHODIMP
HXExternalResourceManager::Init()
{
    HX_RESULT rc = GetHXXResPlugin();

    if(HXR_OK == rc)
    {
	// get language and res directory preferences/reg entries
	if (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences,
						(void**)&m_pPrefs) &&
	    HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, 
						(void**)&m_pRegistry))
	{
	    IHXBuffer* pBuffer = NULL;
	    CHXString strTemp;
	    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME, "LangID");
	    if(HXR_OK == m_pRegistry->GetStrByName(strTemp, pBuffer))
	    {
		const char* pActualLangID = (const char*)pBuffer->GetBuffer();
		m_ulLanguageID = strtol(pActualLangID, NULL, 10);
		HX_RELEASE(pBuffer);
	    }

            const char* pPath = NULL;
            // Get the plugin directory from the Dll Access Paths
            if (GetDLLAccessPath() &&
                (pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN)) &&
                *pPath)
            {
                UINT32 ulBufLen = strlen(pPath) + 1 + strlen(EXT_RES_DIR) + 1;
                m_pExternalResDir = new char[ulBufLen];
                SafeStrCpy(m_pExternalResDir, pPath, ulBufLen);
                if (m_pExternalResDir[strlen(m_pExternalResDir)-1] != OS_SEPARATOR_CHAR)
                {
                    SafeStrCat(m_pExternalResDir, OS_SEPARATOR_STRING, ulBufLen);
                }

                SafeStrCat(m_pExternalResDir, EXT_RES_DIR, ulBufLen);
                rc = LoadResourceFiles();
            }

	}
    }

    return rc;
}

STDMETHODIMP
HXExternalResourceManager::CreateExternalResourceReader(
				    const char* pShortName,
				    REF(IHXExternalResourceReader*) pReader)
{
    HX_RESULT rc = HXR_OK;

    HXExternalResourceReader* pHXReader = new HXExternalResourceReader(this);
    if(!pHXReader)
    {
	return HXR_OUTOFMEMORY;
    }
    pHXReader->AddRef();
    pHXReader->Init(pShortName, m_pResourceList);

    pReader = pHXReader;
    return rc;
}


// HXExternalResourceManager private methods

HX_RESULT
HXExternalResourceManager::GetHXXResPlugin()
{
    HX_RESULT rc = HXR_FAILED;

    IUnknown* pUnk = NULL;
    IUnknown* pUnknownInstance = NULL;
    IHXCommonClassFactory* pCommonClassFactory = NULL;
    IHXPluginGroupEnumerator* pGroupEnumerator = NULL;
    UINT32 ulNumPlugins = 0;

    // enumerate the plugins
    if (HXR_OK != m_pContext->QueryInterface(
				IID_IHXCommonClassFactory,
				(void**)&pCommonClassFactory))
    {
	goto cleanup;
    }

    if (HXR_OK != pCommonClassFactory->CreateInstance(
				CLSID_IHXPluginGroupEnumerator,
				(void**)&pUnk))
    {
	goto cleanup;
    }

    if (HXR_OK != pUnk->QueryInterface(IID_IHXPluginGroupEnumerator,
				    (void**)&pGroupEnumerator))
    {
	goto cleanup;
    }

    if (HXR_OK != pGroupEnumerator->Init(IID_IHXXResFile))
    {
	goto cleanup;
    }

    ulNumPlugins = pGroupEnumerator->GetNumOfPlugins();
    if(ulNumPlugins > 0)
    {
	if (HXR_OK == pGroupEnumerator->GetPlugin(0, pUnknownInstance))
	{
	    m_pHXXResPlugin = pUnknownInstance;
	    m_pHXXResPlugin->AddRef();
	    rc = HXR_OK;
	}
    }

cleanup:
    HX_RELEASE(pUnk);
    HX_RELEASE(pUnknownInstance);
    HX_RELEASE(pCommonClassFactory);
    HX_RELEASE(pGroupEnumerator);

    return rc;
}

HXBOOL
HXExternalResourceManager::ContainsCurrentLanguage(IHXXResFile* pResFile)
{
    HXBOOL rc = FALSE;

    if(!pResFile)
    {
	return rc;
    }

    UINT32 ulLangID = 0;
    rc = pResFile->GetFirstResourceLanguage(ulLangID);
    while(HXR_OK == rc)
    {
	if(m_ulLanguageID == ulLangID)
	{
	    rc = TRUE;
	    break;
	}
	rc = pResFile->GetNextResourceLanguage(ulLangID);
    };

    return rc;
}

IHXXResFile*
HXExternalResourceManager::MakeResFileObject(const char* pPath)
{
    IHXXResFile* pResFile = NULL;

    if(m_pHXXResPlugin &&
	HXR_OK == m_pHXXResPlugin->QueryInterface(IID_IHXXResFile, 
						(void**)&pResFile))
    {
	// open res file and see if it contains any resources with the
	// current language ID

	pResFile->Open(pPath);
	if(ContainsCurrentLanguage(pResFile))
	{
	    pResFile->SetLanguage(m_ulLanguageID);
	}
	else
	{
	    HX_RELEASE(pResFile);   // don't want this one
	}
    }
    return pResFile;
}

HX_RESULT
HXExternalResourceManager::Stat(const char* pPath, struct stat* pStatBuffer)
{
    HX_RESULT rc = HXR_OK;

    memset(pStatBuffer,0,sizeof(*pStatBuffer));
	
    if(stat((const char*)pPath, pStatBuffer) < 0)
    {
	rc = HXR_FAIL;
    }
    return rc;
}

IHXBuffer* 
HXExternalResourceManager::ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen)
{
    char* pOut = new char[nBuffLen*2+1];
    char* pStartOut = pOut;
    char Nibble;

    IHXBuffer* pOutBuffer = NULL;
    if (HXR_OK == CreateBufferCCF(pOutBuffer, m_pContext))
    {
	for (int i = 0; i < (int)nBuffLen; i++)
	{
	    Nibble = (*pBuffer >> 4) & 15;
	    *pOut= (Nibble > 9 ) ? Nibble + 55 : Nibble + 48;
	    pOut++;
	    Nibble = *pBuffer & 15;
	    *pOut= (Nibble > 9 ) ? Nibble + 55 : Nibble + 48;
	    pOut++;
	    pBuffer++;
	}
	*pOut = 0;
	pOutBuffer->Set((UCHAR*)pStartOut, strlen(pStartOut)+1);
    }
    delete[] pStartOut;
    return pOutBuffer;
}

IHXBuffer* 
HXExternalResourceManager::ChecksumFile(const char* pPath)
{
    struct stat	    stat_struct;
    md5_state_t	    MD5_data;
    UCHAR	    tempbuf[16];

    md5_init(&MD5_data);
    if (HXR_OK != Stat(pPath, &stat_struct))
    {
        md5_finish(tempbuf, &MD5_data);
	return NULL;
    } 
    md5_append(&MD5_data,(UCHAR*)&(stat_struct),sizeof(stat_struct)); 
    md5_finish(tempbuf, &MD5_data);
    return ConvertToAsciiString((char*)tempbuf, sizeof(tempbuf));
}

HX_RESULT
HXExternalResourceManager::SaveFileInfo(const char* pFileName,
				         const char* pPath)
{
    HX_RESULT rc = HXR_OK;

   // set/reset cache info
    if(!m_pPrefs)
    {
	return HXR_FAIL;
    }

    IHXBuffer* pStatInfo = ChecksumFile(pPath);
    if(pStatInfo)
    {
	CHXString prefString;
	prefString.Format("%s\\%s\\%s\\%s",
			    EXT_RES_DATA_KEY,
			    FILE_INFO_KEY,
			    pFileName,
			    CHECKSUM_KEY);
	m_pPrefs->WritePref(prefString, pStatInfo);
	HX_RELEASE(pStatInfo);
    }

    // now write out the langages supported in the resource file
    IHXXResFile* pResFile;
    if(HXR_OK == m_pHXXResPlugin->QueryInterface(IID_IHXXResFile, 
						(void**)&pResFile))
    {
	HX_RESULT rc = HXR_OK;

	// open res file and iterate through all of the languages
	// it supports
	pResFile->Open(pPath);

	UINT32 ulLangID = 0;
	rc = pResFile->GetFirstResourceLanguage(ulLangID);
	while(HXR_OK == rc)
	{
	    CHXString prefString;
	    IHXBuffer* pBuffer = NULL;
	    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	    {
		prefString.Format("%s\\%s\\%s\\%s\\%ld",
				    EXT_RES_DATA_KEY,
				    FILE_INFO_KEY,
				    pFileName,
				    LANGUAGE_KEY,
				    ulLangID);
		pBuffer->Set((BYTE*)"", 1);
		m_pPrefs->WritePref(prefString, pBuffer);
		HX_RELEASE(pBuffer);
	    }

	    rc = pResFile->GetNextResourceLanguage(ulLangID);
	};
	HX_RELEASE(pResFile);
    }

    return rc;
}

HX_RESULT
HXExternalResourceManager::LoadResourceFile(const char* pPath)
{
    HX_RESULT rc = HXR_OK;

    // MakeResFileObject only returns an IHXXResFile
    // if there are any resources in the current language
    IHXXResFile* pResFile = MakeResFileObject(pPath);
    if (pResFile)
    {
	if(!m_pResourceList)
	{
	    m_pResourceList = new CHXSimpleList;
	}
    	m_pResourceList->AddTail(pResFile);
    }

    return rc;
}

HXBOOL
HXExternalResourceManager::FileInfoCurrent(const char* pFileName, 
					    const char* pPath)
{
    HXBOOL rc = FALSE;

    IHXBuffer* pStatInfo = ChecksumFile(pPath);
    if(pStatInfo)
    {
	IHXBuffer* pPrefValue = NULL;
	CHXString prefString;
	prefString.Format("%s\\%s\\%s\\%s",
			    EXT_RES_DATA_KEY,
			    FILE_INFO_KEY,
			    pFileName,
			    CHECKSUM_KEY);
	if(HXR_OK == m_pPrefs->ReadPref(prefString, pPrefValue) &&
	    pPrefValue)
	{
	    const char* pActualPrefValue = (const char*)pPrefValue->GetBuffer();
	    const char* pActualStatInfo = (const char*)pStatInfo->GetBuffer();
	    if(strcmp(pActualPrefValue, pActualStatInfo) == 0)
	    {
		rc = TRUE;
	    }
	    HX_RELEASE(pPrefValue);
	}
	HX_RELEASE(pStatInfo);
    }

    return rc;
}

HXBOOL
HXExternalResourceManager::ContainsCurrentLanguage(const char* pFileName,
						   const char* pPath)
{
#if defined (_MACINTOSH) || defined (_UNIX)
    /* Temporary - Mac Installer does not write this pref yet -- XXXRA */	
    return TRUE;
#endif

    HXBOOL rc = FALSE;

    IHXBuffer* pLangInfo = NULL;
    CHXString prefString;
    prefString.Format("%s\\%s\\%s\\%s\\%ld",
			EXT_RES_DATA_KEY,
			FILE_INFO_KEY,
			pFileName,
			LANGUAGE_KEY,
			m_ulLanguageID);
    if(HXR_OK == m_pPrefs->ReadPref(prefString, pLangInfo))
    {
	rc = TRUE;
	HX_RELEASE(pLangInfo);
    }

    return rc;
}

HX_RESULT
HXExternalResourceManager::LoadResourceFiles()
{
    CFindFile*	pFileFinder	= NULL;
    char*	pResFile	= NULL;
    const char*	pFileExtension	= "*.xrs";
    HX_RESULT   rc		= HXR_OK;

    // find plugins with specified pattern
    pFileFinder = CFindFile::CreateFindFile(m_pExternalResDir, 0, 
					    pFileExtension);
    if (NULL == pFileFinder)
    {
	rc = HXR_FAIL;
	goto cleanup;
    }

    // collect plugin info.
    pResFile = pFileFinder->FindFirst();
    while (pResFile)
    {
	const char* pResPath = pFileFinder->GetCurFilePath();
	// only load the file if it has resources that reflect
	// the current system language ID
	if(!FileInfoCurrent(pResFile, pResPath))
	{
	    SaveFileInfo(pResFile, pResPath);
	}
	    
	if(ContainsCurrentLanguage(pResFile, pResPath))
	{
	    rc = LoadResourceFile(pResPath);
	}
	if(HXR_OK != rc)
	{
	    // error - log it and keep on?
	}
	pResFile = pFileFinder->FindNext();
    }
    delete pFileFinder;

cleanup:
    return rc;
}

// HXExternalResourceReader methods

HXExternalResourceReader::HXExternalResourceReader(
				     HXExternalResourceManager* pMgr):
     m_lRefCount(0)
    ,m_pResourceList(NULL)
    ,m_pResManager(pMgr)
    ,m_pDefaultRes(NULL)
{
}

HXExternalResourceReader::~HXExternalResourceReader()
{
    HX_DELETE(m_pResourceList);
    HX_RELEASE(m_pDefaultRes);
}

// IUnknown methods

STDMETHODIMP 
HXExternalResourceReader::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXExternalResourceReader), (IHXExternalResourceReader*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXExternalResourceReader*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXExternalResourceReader::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXExternalResourceReader::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
HXExternalResourceReader::SetDefaultResourceFile(
			    const char* pResourcePath)
{
    HX_RESULT rc = HXR_OK;

    m_pDefaultRes = m_pResManager->MakeResFileObject(pResourcePath);

    return rc;
}

IHXXResource*
HXExternalResourceReader::GetResource(IHXXResFile* pResFile,
				       UINT32 ulResourceType,
				       UINT32 ulResourceID)
{
    IHXXResource* pRes = NULL;
    switch(ulResourceType)
    {
	case HX_RT_STRING:
	{
	    pRes = pResFile->GetString(ulResourceID);
	}
	break;

	case HX_RT_DIALOG:
	{
	    pRes = pResFile->GetDialog(ulResourceID);
	}
	break;

	case HX_RT_BITMAP:
	{
	    pRes = pResFile->GetBitmap(ulResourceID);
	}
	break;

	default:
	break;
    }
    return pRes;
}

STDMETHODIMP_(IHXXResource*)
HXExternalResourceReader::GetResource(UINT32 ulResourceType,
				    UINT32 ulResourceID)
{
    IHXXResource* pRes = NULL;

    if(m_pResourceList)
    {
	CHXSimpleList::Iterator i = m_pResourceList->Begin();
	for (; i != m_pResourceList->End(); ++i)
	{
	    IHXXResFile* pResFile = (IHXXResFile*)(*i);
	    pRes = GetResource(pResFile, ulResourceType, ulResourceID);
	    if(pRes)
	    {
		break;
	    }
	}
    }

    if(!pRes &&
	m_pDefaultRes)	// check default
    {
	pRes = GetResource(m_pDefaultRes, ulResourceType, ulResourceID);
    }

    return pRes;
}


HX_RESULT
HXExternalResourceReader::Init(const char* pShortName,
				CHXSimpleList* pResList)
{
    HX_RESULT rc = HXR_OK;

    if(pResList)
    {
	CHXSimpleList::Iterator i = pResList->Begin();
	for (; i != pResList->End(); ++i)
	{
	    IHXXResFile* pResFile = (IHXXResFile*)(*i);
	    if(pResFile->IncludesShortName(pShortName))
	    {
		if(!m_pResourceList)
		{
		    m_pResourceList = new CHXSimpleList;
		}
		m_pResourceList->AddTail(pResFile);
	    }
	}
    }

    return rc;
}

