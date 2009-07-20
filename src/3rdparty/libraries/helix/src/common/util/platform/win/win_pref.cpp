/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win_pref.cpp,v 1.8 2006/02/07 19:21:32 ping Exp $
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

#ifdef _WINDOWS
#ifdef _WIN16
#undef _MT
#endif
#include "hxtypes.h"
#include "hlxclib/windows.h"
#endif

#include "hxcom.h"
#include<string.h>    
#include<shellapi.h> 

#include "hxassert.h"

#include "win_pref.h"  

#include "hxwinver.h"  
#include "dbcs.h"  

#include "ihxpckts.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxstrutl.h"

#ifndef _WIN16
// Not in win16.  Any place that this is used must be ifdefed out.•
#include "hxwinreg.h"
#else 
#include "winreg16.h"//use this instead (in remove_pref function).  Partial functionality
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * call open_pref() to automatically create the correct Windows 
 * specific preference object. 
 */   
CWinPref * CWinPref::open_pref(	const char* pCompanyName, 
				const char* pProductName, 
				int nProdMajorVer, 
				int nProdMinorVer,
				HXBOOL bCommon,
				IUnknown* pContext)
{
    return new CWinPref(pCompanyName, pProductName, 
			nProdMajorVer, nProdMinorVer, bCommon, pContext);
}

/*Constructor NOTE: use open_pref() to create an instance of this class */
CWinPref::CWinPref(const char* pCompanyName, 
		   const char* pProductName, 
		   int nProdMajorVer, 
		   int nProdMinorVer,
		   HXBOOL bCommon,
		   IUnknown* pContext) :
    CPref(pContext),
    m_bCommon(bCommon)
{ 	
    // Assemble registry's key for the preferences 
    SafeStrCpy(RootKeyName, "Software\\", _MAX_KEY);
    SafeStrCat(RootKeyName, pCompanyName, _MAX_KEY);

    char * pComa = (char*) HXFindChar(RootKeyName, ',');
    if(pComa)
    {
      *pComa = 0;
    }

    // If the product name is the special shared product name then we ignore the product name and major and
    // minor version numbers passed in
    if (strcmp(pProductName,HX_PRODUCTNAME_SHARED))
    {
	SafeStrCat(RootKeyName, "\\", _MAX_KEY);  
	SafeStrCat(RootKeyName, pProductName, _MAX_KEY);
	pComa = (char*) HXFindString(RootKeyName, "(tm)");
	if(pComa)
	{
	    memmove(pComa, pComa + 4, 
		    strlen(RootKeyName) - (pComa + 3 - RootKeyName));
	}                           
	    
	if(nProdMajorVer || nProdMinorVer)
	{
	    SafeStrCat(RootKeyName, "\\", _MAX_KEY);  
	    SafeSprintf(RootKeyName + strlen(RootKeyName), _MAX_KEY - strlen(RootKeyName), "%d.%d", 
			nProdMajorVer, nProdMinorVer);
	}
    }
    SafeStrCat(RootKeyName, "\\Preferences", _MAX_KEY); 
    
    // For Windows 3.1 we are using '_' instead of spaces
    DWORD dwPlatformId = HXGetWinVer(NULL);
    if(dwPlatformId == HX_WINVER_16BIT || dwPlatformId == HX_WINVER_32BIT_S)
    {     
        char * pSpace;
	while((pSpace = (char*) HXFindChar(RootKeyName, ' ')) != NULL)
	{
            *pSpace = '_';
	}
    }
}

/*  read_pref reads the preference specified by Key to the Buffer. */	
HX_RESULT CWinPref::read_pref(const char* pPrefKey, IHXBuffer*& pBuffer)
{  
    HX_ASSERT(pPrefKey);

    pBuffer = 0;

    if (!pPrefKey)
    {
	return HXR_INVALID_PARAMETER;
    }
    UINT32 ulBufSize = strlen(RootKeyName) + strlen(pPrefKey) + 2;
    char * pKeyName = new char[ulBufSize];
    if (!pKeyName)
    {
	return HXR_OUTOFMEMORY;
    }

    HKEY hKey;
    HX_RESULT theErr = -1;
    LONG lBufSize = 0;

    SafeStrCpy(pKeyName, RootKeyName, ulBufSize);
    SafeStrCat(pKeyName, "\\", ulBufSize);
    SafeStrCat(pKeyName, pPrefKey, ulBufSize);
	
    if(RegOpenKey(m_bCommon ? HKEY_CLASSES_ROOT : HKEY_CURRENT_USER, pKeyName, &hKey) == ERROR_SUCCESS)
    {
#ifdef _WIN32
        /* First get the key size then allocate big enough buffer */
        if (RegQueryValue(hKey, "", NULL, &lBufSize) == ERROR_SUCCESS && (lBufSize > 0))
#else
	const int BufLen = 9216;
	char *szBuffer = new char[BufLen]; 
        lBufSize = BufLen;
	HX_ASSERT(szBuffer != NULL);
        if (RegQueryValue(hKey, "", szBuffer, &lBufSize) == ERROR_SUCCESS)
#endif                                                                
	{
	    CreateBufferCCF(pBuffer, m_pContext);
	    if (!pBuffer)
	    {
		RegCloseKey(hKey);  
		delete [] pKeyName;
		return HXR_OUTOFMEMORY;
	    }
	    pBuffer->SetSize(lBufSize);
	    RegQueryValue(hKey, "", (LPSTR) pBuffer->GetBuffer(), &lBufSize);
	    theErr = HXR_OK;
	}
        RegCloseKey(hKey);  
#ifdef _WIN32
#else
	if (szBuffer != NULL)
	    delete szBuffer;
#endif                                                                
    } // end if(RegOpenKey(HKEY_CLASSES_ROOT, pKeyName, &hKey) == ERROR_SUCCESS)

    if (pKeyName)
    {
	delete [] pKeyName;
    }
#ifdef _UNICODE
    if (HXR_OK == theErr)
    {
	OS_STRING_TYPE str((OS_TEXT_PTR)( pBuffer->GetBuffer() ));
	char* ascii = (char*)str;

	pBuffer->SetSize( strlen(ascii)+1 );
	strcpy( (char*)pBuffer->GetBuffer(), ascii );
    }
#endif //_UNICODE
    
    return theErr;
}
 
/*write_pref writes (saves) the preference specified by Key from the Buffer.*/
HX_RESULT CWinPref::write_pref(const char* pPrefKey, IHXBuffer* pBuffer)
{   
    HX_ASSERT(pPrefKey && pBuffer);

    if (!pPrefKey || !pBuffer)
    {
	return HXR_INVALID_PARAMETER;
    }
    UINT32 ulBufLen = strlen(RootKeyName) + strlen(pPrefKey) + 2;
    char * pKeyName = new char[ulBufLen];
    if (!pKeyName)
    {
	return HXR_OUTOFMEMORY;
    }

    HKEY hKey;
    HX_RESULT theErr = -1;

    SafeStrCpy(pKeyName, RootKeyName, ulBufLen);
    SafeStrCat(pKeyName, "\\", ulBufLen);
    SafeStrCat(pKeyName, pPrefKey, ulBufLen);
	
    if(RegCreateKey(m_bCommon ? HKEY_CLASSES_ROOT : HKEY_CURRENT_USER, pKeyName, &hKey) == ERROR_SUCCESS )
    {	
	RegSetValue(hKey, "", REG_SZ, (const char*) pBuffer->GetBuffer(), strlen( (char*)pBuffer->GetBuffer() ));
	RegCloseKey(hKey);
	theErr = HXR_OK;
    }

    if (pKeyName)
    {
	delete [] pKeyName;
    }

    return theErr;
}

HX_RESULT CWinPref::delete_pref(const char* pPrefKey)
{
    return remove_pref(pPrefKey);
}

/*  remove_indexed_pref removes indexed preference specified by Key */       
HX_RESULT CWinPref::remove_indexed_pref(const char* pPrefKey)
{
    HKEY hKey;

    // Remove old entries for that preference
    if(RegOpenKey(m_bCommon ? HKEY_CLASSES_ROOT : HKEY_CURRENT_USER, RootKeyName, &hKey) == ERROR_SUCCESS)
    {   
	UINT16 ind = 1;
	char TmpKeyName[_MAX_KEY]; /* Flawfinder: ignore */
                
	while(1)
	{
	    SafeSprintf(TmpKeyName, _MAX_KEY, "%s%d", pPrefKey, ind);
	    ind++;
	    if(RegDeleteKey(hKey, OS_STRING(TmpKeyName)) != ERROR_SUCCESS)
		break;
	}
    
	RegCloseKey(hKey);                                    
    }

    return HXR_OK;
}

HX_RESULT CWinPref::remove_pref(const char* pPrefKey)
{
    CWinRegKey PrefKey;
    PrefKey.SetRootKey(m_bCommon ? HKEY_CLASSES_ROOT : HKEY_CURRENT_USER);
    PrefKey.SetRelativePath(RootKeyName);
    if(PrefKey.Open() == HXR_OK)
    {
	PrefKey.DeleteSubKey(pPrefKey);
	PrefKey.Close();
    }

    return HXR_OK;
}

HX_RESULT CWinPref::BeginSubPref(const char* szSubPref)
{
    // Current buffer RootKeyName + NULL ... New addition szSubPref + '\\'
    HX_ASSERT((strlen(RootKeyName) + 1 + strlen(szSubPref) + 1) <= _MAX_KEY);

    // Just add sub-pref to the root key name
    SafeStrCat(RootKeyName, "\\", _MAX_KEY);
    SafeStrCat(RootKeyName, szSubPref, _MAX_KEY);

    return HXR_OK;
}

HX_RESULT CWinPref::EndSubPref()
{
    // Find the \ character from the end of the string
    char* pSlash = HXReverseFindChar(RootKeyName,'\\');

    // if we find the \ character we NULL terminate at this point removing the last sub-pref added
    if (pSlash)
    {
	*pSlash = '\0';
	return HXR_OK;
    }
    else
	return HXR_FAIL;

}

HX_RESULT CWinPref::GetPrefKey(UINT32 nIndex,IHXBuffer*& pBuffer)
{
    HX_RESULT result = HXR_FAIL;

    // Init output
    pBuffer = NULL;

    // Open the current RootKey that we are at, may be a subpreference
    HKEY hKey = NULL;
    if(RegOpenKey(m_bCommon ? HKEY_CLASSES_ROOT : HKEY_CURRENT_USER,RootKeyName,&hKey) == ERROR_SUCCESS)
    {
	// Create a buffer to store the pref key
	CreateBufferCCF(pBuffer, m_pContext);
	if (!pBuffer)
	{
	    RegCloseKey(hKey);  
	    return HXR_OUTOFMEMORY;
	}

	// AddRef buffer and setup size
	pBuffer->SetSize(MAX_PATH+1);

	// If we get a value back we are ok
	if (RegEnumKey(hKey,nIndex,(char*)pBuffer->GetBuffer(),MAX_PATH+1) == ERROR_SUCCESS)
	    result = HXR_OK;
	// No subkeys so clear buffer
	else
	{
	    pBuffer->Release();
	    pBuffer = NULL;
	}

	RegCloseKey(hKey);
    }

    return result;
}
