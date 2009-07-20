/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pref.cpp,v 1.16 2008/01/18 04:54:26 vkathuria Exp $
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
   
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxprefs.h"
#include "pref.h"
#include "hxassert.h"
#include "hxbuffer.h"
#include "hxstring.h"
#include "hxstrutl.h"
#include "pckunpck.h"
#include "hlxclib/stdlib.h"

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
#if defined(_CARBON) || defined(_MAC_UNIX)
#include "platform/mac/mac_pref_cf.h"
#else
#include "mac_pref.h"
#endif

#elif defined(_UNIX) && !defined(_MAC_UNIX)
#include "unix_pref.h"

#elif defined(_WIN32) || defined(_WINDOWS)
#include "win_pref.h"

#elif defined(_SYMBIAN)
#include "symbian_pref.h"
#elif defined(_OPENWAVE)
#include "openwave_pref.h"
#elif defined (_BREW)
#include "brew_pref.h" 
#else
#error Undefined platform!
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


// 	call open_pref() to automatically create the correct platform specific preference 
//  object.	If open_pref() returns NULL, an error occurred and the CPref object was 
//  not created. Call last_error to get the error 

CPref * CPref::open_pref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext)
{
    CPref * pPref = NULL;

#if   defined (_CARBON) || defined(_MAC_UNIX)
	pPref = CMacPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, bCommon, pContext);

#elif   defined (_MACINTOSH)
	pPref = CMacPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, TRUE, pContext);

#elif defined( _UNIX)
	pPref = ( CPref *) CUnixPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, pContext);
        
#elif defined(_SYMBIAN)
	pPref = ( CPref *) CSymbianPref::open_pref( pCompanyName,
                                                    pProductName,
                                                    nProdMajorVer,
                                                    nProdMinorVer,
						    pContext);

#elif defined(_OPENWAVE)
	pPref = ( CPref *) COpenwavePref::open_pref( pCompanyName,
                                                     pProductName,
                                                     nProdMajorVer,
                                                     nProdMinorVer,
						     pContext);
#elif defined(_BREW)
	pPref =	( CPref *) CBrewPref::open_pref	(   pCompanyName,
						    pProductName,
						    nProdMajorVer,
						    nProdMinorVer,
						    pContext);
#elif defined( _WIN32 ) || defined( _WINDOWS )
	pPref = CWinPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, bCommon, pContext);
#endif
	if (pPref)
	{
		pPref->m_bIsCommonPref = bCommon;
	}

	return pPref;
}

CPref* CPref::open_shared_pref(const char* pCompanyName, IUnknown* pContext)
{
    // Major and Minor version numbers are not used for a shared preference since it is shared among all products
    return CPref::open_pref(pCompanyName,HX_PRODUCTNAME_SHARED,0,0, TRUE, pContext);
}

CPref* CPref::open_shared_user_pref(const char* pCompanyName, IUnknown* pContext)
{
    // Major and Minor version numbers are not used for a shared preference since it is shared among all products
    return CPref::open_pref(pCompanyName,HX_PRODUCTNAME_SHARED,0,0,FALSE, pContext);
}

// 	Constructor NOTE: use open_pref() to create an instance of this class 
CPref::CPref(IUnknown* pContext) :
    m_pPrefTable(NULL), m_bIsCommonPref(FALSE)
{					
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
    mLastError = HXR_OK;
}   	

//      class destructor 
CPref::~CPref(void)
{
    HX_DELETE(m_pPrefTable);
    HX_RELEASE(m_pContext);
}

void CPref::SetupPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries)
{ 
    // Cleanup existing pref table
    HX_DELETE(m_pPrefTable);

    // Create new pref table
    m_pPrefTable = new CPrefTable(pPrefTable,nTableEntries,this, m_pContext);
    HX_ASSERT(m_pPrefTable);
}

CPrefTable::CPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries,CPref* pPrefs, IUnknown* pContext) :
    m_nTableEntries(nTableEntries),
    m_pPrefTable(pPrefTable),
    m_pCPref(pPrefs),
    m_pIHXPrefs(NULL),
    m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}

CPrefTable::CPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries,IUnknown* pContext) :
    m_nTableEntries(nTableEntries),
    m_pPrefTable(pPrefTable),
    m_pCPref(NULL),
    m_pIHXPrefs(NULL),
    m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    if (m_pContext)
    {
	m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pIHXPrefs);
    }
}

CPrefTable::~CPrefTable()
{
    HX_RELEASE(m_pIHXPrefs);
    HX_RELEASE(m_pContext);
}

HX_RESULT CPrefTable::RemoveIndexedPref(INT32 nPrefKey)
{
    // Index is out of table range if this is false
    HX_ASSERT(nPrefKey >= 0 && nPrefKey < m_nTableEntries);

    // Need to call SetupPrefTable() if this is false
    HX_ASSERT(m_pPrefTable);

    // Index is out of table range
    if (nPrefKey < 0 || nPrefKey >= m_nTableEntries)
	return HXR_INVALID_PARAMETER;

    // Need to call SetupPrefTable()
    if (!m_pPrefTable || !m_pCPref)
	return HXR_UNEXPECTED;

    return m_pCPref->remove_indexed_pref(m_pPrefTable[nPrefKey].szPrefName);

}

HX_RESULT CPrefTable::RemovePref(INT32 nPrefKey)
{
    // Index is out of table range if this is false
    HX_ASSERT(nPrefKey >= 0 && nPrefKey < m_nTableEntries);

    // Need to call SetupPrefTable() if this is false
    HX_ASSERT(m_pPrefTable);

    // Index is out of table range
    if (nPrefKey < 0 || nPrefKey >= m_nTableEntries)
	return HXR_INVALID_PARAMETER;

    // Need to call SetupPrefTable()
    if (!m_pPrefTable || !m_pCPref)
	return HXR_UNEXPECTED;

    return m_pCPref->remove_pref(m_pPrefTable[nPrefKey].szPrefName);

}

HXBOOL CPrefTable::ReadPoints(const char* pBuffer,HXxPoint* pt,int nNumPoints)
{
    // If this is false something is really messed up
    HX_ASSERT(pBuffer);
    HX_ASSERT(pt);

    const char* szSeperators = ",";
    char	szBufferCopy[MAX_PREF_SIZE]; /* Flawfinder: ignore */
    char*	pToken= NULL;
    
    SafeStrCpy(szBufferCopy, pBuffer, MAX_PREF_SIZE);

   // Establish string and get the first token: 
   pToken = strtok( szBufferCopy, szSeperators );
   int i = 0;
   while( pToken != NULL )
   {
	// Store x value
	pt[i].x = atol(pToken);

	// Get next token and store y value
	pToken = strtok( NULL, szSeperators );
	pt[i].y = atol(pToken);

	// Get next token and continue
	i++;
	pToken = strtok( NULL, szSeperators );
   }

   return (i == nNumPoints) ? TRUE : FALSE;
}

HX_RESULT CPrefTable::ReadPrefColor(INT32 nPrefKey,HXxColor& color,INT32 nIndex)
{
    HX_RESULT theErr = HXR_OK;
    INT32 lValue = 0;

    theErr = ReadPrefInt(nPrefKey,lValue,nIndex);
    color = (HXxColor) lValue;

    return theErr;
}

HX_RESULT CPrefTable::ReadPref(INT32 nPrefKey,INT32 nIndex,IHXBuffer*& pBuffer)
{
    // Index is out of table range if this is false
    HX_ASSERT(nPrefKey >= 0 && nPrefKey < m_nTableEntries);

    // Need to call SetupPrefTable() if this is false
    HX_ASSERT(m_pPrefTable);

    // Index is out of table range
    if (nPrefKey < 0 || nPrefKey >= m_nTableEntries)
	return HXR_INVALID_PARAMETER;

    // Need to call SetupPrefTable()
    if (!m_pPrefTable || (!m_pCPref && !m_pIHXPrefs))
	return HXR_UNEXPECTED;

    // If we have an indexed pref append index number to pref
    if (nIndex > 0)
    {
        char szIndexedPref[MAX_PREF_NAME]; /* Flawfinder: ignore */
	SafeSprintf(szIndexedPref,MAX_PREF_NAME,"%s%ld",m_pPrefTable[nPrefKey].szPrefName,nIndex);
	return (m_pCPref ? m_pCPref->read_pref(szIndexedPref,pBuffer) : m_pIHXPrefs->ReadPref(szIndexedPref,pBuffer));
    }
    // otherwise just read in this pref
    else
	return (m_pCPref ? m_pCPref->read_pref(m_pPrefTable[nPrefKey].szPrefName,pBuffer) : m_pIHXPrefs->ReadPref(m_pPrefTable[nPrefKey].szPrefName,pBuffer));
}

HXBOOL CPrefTable::IsPrefSet(INT32 nPrefKey,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Don't need buffer anymore
    HX_RELEASE(pBuffer);

    return SUCCEEDED(result) ? TRUE : FALSE;
}

HX_RESULT CPrefTable::ReadPrefInt(INT32 nPrefKey,INT32& nValue,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    // Default return value
    nValue = 0;

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Convert the preference to an integer 
    if (result == HXR_OK)
	nValue = atol((const char*)pBuffer->GetBuffer());
    // Pref doesn't exist convert default value to integer
    // Added the invalid parameter check to insure the index in valid range RBP 8/20/01 
    else if (HXR_INVALID_PARAMETER != result && HXR_UNEXPECTED != result)
    {
	if(NULL != m_pPrefTable[nPrefKey].pDefaultValue)
	{
	    nValue = atol(m_pPrefTable[nPrefKey].pDefaultValue);
	}
	else
	{
	    return result;
	}
    }
    else
	return HXR_FAIL;

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return HXR_OK;
}

HX_RESULT CPrefTable::ReadPrefRect(INT32 nPrefKey,HXxRect& rect,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Convert the preference to a rect
    HXxPoint ptArray[2];
    if (result == HXR_OK)
    {
	if (ReadPoints((const char*)pBuffer->GetBuffer(),ptArray,2))
	{
	    rect.left = ptArray[0].x;
	    rect.top = ptArray[0].y;
	    rect.right = ptArray[1].x;
	    rect.bottom = ptArray[1].y;

	    // Don't need buffer anymore
	    if (pBuffer)
		pBuffer->Release();
	    return HXR_OK;
	}
    }

    if (m_pPrefTable[nPrefKey].pDefaultValue)
    {
	ReadPoints(m_pPrefTable[nPrefKey].pDefaultValue,ptArray,2);
	rect.left = ptArray[0].x;
	rect.top = ptArray[0].y;
	rect.right = ptArray[1].x;
	rect.bottom = ptArray[1].y;
    }
    else
	return HXR_FAIL;

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return HXR_OK;
}

HX_RESULT CPrefTable::ReadPrefSize(INT32 nPrefKey,HXxSize& size,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Convert the preference to a size
    HXxPoint ptArray;
    if (result == HXR_OK)
    {
	if (ReadPoints((const char*)pBuffer->GetBuffer(),&ptArray,1))
	{
	    size.cx = ptArray.x;
	    size.cy = ptArray.y;

	    // Don't need buffer anymore
	    if (pBuffer)
		pBuffer->Release();
	    return HXR_OK;
	}
    }

    if (m_pPrefTable[nPrefKey].pDefaultValue)
    {
	ReadPoints(m_pPrefTable[nPrefKey].pDefaultValue,&ptArray,1);
	size.cx = ptArray.x;
	size.cy = ptArray.y;
    }
    else
	return HXR_FAIL;

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return HXR_OK;
}

HX_RESULT CPrefTable::ReadPrefPoint(INT32 nPrefKey,HXxPoint& pt,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Convert the preference to a size
    HXxPoint ptArray;
    if (result == HXR_OK)
    {
	if (ReadPoints((const char*)pBuffer->GetBuffer(),&ptArray,1))
	{
	    pt.x = ptArray.x;
	    pt.y = ptArray.y;

	    // Don't need buffer anymore
	    if (pBuffer)
		pBuffer->Release();
	    return HXR_OK;
	}
    }

    if (m_pPrefTable[nPrefKey].pDefaultValue)
    {
	ReadPoints(m_pPrefTable[nPrefKey].pDefaultValue,&ptArray,1);
	pt.x = ptArray.x;
	pt.y = ptArray.y;
    }
    else
	return HXR_FAIL;

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return HXR_OK;
}

HX_RESULT CPrefTable::ReadPrefString(INT32 nPrefKey,char* szString,INT32 nStrLen,INT32 nIndex)
{
    CHXBuffer* pBuffer = NULL;
    HX_RESULT result;

    if (!szString || !nStrLen)
	return HXR_INVALID_PARAMETER;

    // Default return value
    szString[0] = '\0';

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,(IHXBuffer*&)pBuffer);

    // Convert the preference to a string
    if (result == HXR_OK)
    {
	strncpy(szString,(const char*)pBuffer->GetBuffer(),(int)nStrLen);
        szString[nStrLen-1] = '\0';
    }
    // Pref doesn't exist convert default value to integer
    else if (m_pPrefTable[(int)nPrefKey].pDefaultValue)
    {
	strncpy(szString,m_pPrefTable[nPrefKey].pDefaultValue,(int)nStrLen);
        szString[nStrLen-1] = '\0';
    }
    else
	return HXR_FAIL;

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return HXR_OK;
}
    
HX_RESULT CPrefTable::ReadPrefBuffer(INT32 nPrefKey,IHXBuffer*& pBuffer,INT32 nIndex)
{
    HX_RESULT result;
    // read_pref creates an IHXBuffer so this should be NULL
    HX_ASSERT(!pBuffer);

    // Try to read the pref
    result = ReadPref(nPrefKey,nIndex,pBuffer);

    // If the pref read failed try to create our own IMRABuffer and copy in the default pref
    if (result != HXR_OK)
    {
	if (m_pPrefTable[nPrefKey].pDefaultValue)
	{
	    return CreateAndSetBufferCCF(pBuffer, (UCHAR*)m_pPrefTable[nPrefKey].pDefaultValue,
					 strlen(m_pPrefTable[nPrefKey].pDefaultValue)+1, m_pContext);
	}
	else
	    return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT CPrefTable::WritePref(INT32 nPrefKey,INT32 nIndex,IHXBuffer* pBuffer)
{
    // Index is out of table range if this is false
    HX_ASSERT(nPrefKey >= 0 && nPrefKey < m_nTableEntries);

    // Need to call SetupPrefTable() if this is false
    HX_ASSERT(m_pPrefTable);

    // Index is out of table range
    if (nPrefKey < 0 || nPrefKey >= m_nTableEntries)
	return HXR_INVALID_PARAMETER;

    // Need to call SetupPrefTable()
    if (!m_pPrefTable || (!m_pCPref && !m_pIHXPrefs))
	return HXR_UNEXPECTED;

    // If we have an indexed pref append index number to pref
    if (nIndex > 0)
    {
        char szIndexedPref[MAX_PREF_NAME]; /* Flawfinder: ignore */
	SafeSprintf(szIndexedPref,MAX_PREF_NAME,"%s%ld",m_pPrefTable[nPrefKey].szPrefName,nIndex);
	return (m_pCPref ? m_pCPref->write_pref(szIndexedPref,pBuffer) : m_pIHXPrefs->WritePref(szIndexedPref,pBuffer));
    }
    else
	return (m_pCPref ? m_pCPref->write_pref(m_pPrefTable[nPrefKey].szPrefName,pBuffer) : m_pIHXPrefs->WritePref(m_pPrefTable[nPrefKey].szPrefName,pBuffer));
}

IHXBuffer* CPrefTable::CreateIHXBuffer(const char* szString)
{
    IHXBuffer* pBuffer = NULL;
    CreateAndSetBufferCCF(pBuffer, (UCHAR*)szString,
			  strlen(szString)+1, m_pContext);

    return pBuffer;
}

HX_RESULT CPrefTable::WritePrefColor(INT32 nPrefKey,const HXxColor& color,INT32 nIndex)
{
    INT32 lValue = (INT32)color;
    return WritePrefInt(nPrefKey,lValue,nIndex);
}

HX_RESULT CPrefTable::WritePrefInt(INT32 nPrefKey,INT32 nValue,INT32 nIndex)
{
    IHXBuffer* pBuffer = NULL;
    char szBuff[MAX_INT_BUFFER]; /* Flawfinder: ignore */
    
    SafeSprintf(szBuff, sizeof(szBuff), "%ld", nValue);

    // Create buffer and write pref
    pBuffer = CreateIHXBuffer(szBuff);
    HX_RESULT result = WritePref(nPrefKey,nIndex,pBuffer);

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return result;
}

HX_RESULT CPrefTable::WritePrefRect(INT32 nPrefKey,const HXxRect& rect,INT32 nIndex)
{
    IHXBuffer* pBuffer = NULL;
    char szBuff[MAX_RECT_BUFFER]; /* Flawfinder: ignore */
    
    SafeSprintf(szBuff, sizeof(szBuff), "%ld,%ld,%ld,%ld",
                rect.left, rect.top, rect.right, rect.bottom);

    // Create buffer and write pref
    pBuffer = CreateIHXBuffer(szBuff);
    HX_RESULT result = WritePref(nPrefKey,nIndex,pBuffer);

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return result;
}

HX_RESULT CPrefTable::WritePrefSize(INT32 nPrefKey,const HXxSize& size,INT32 nIndex)
{
    IHXBuffer* pBuffer = NULL;
    char szBuff[MAX_SIZE_BUFFER]; /* Flawfinder: ignore */
    
    SafeSprintf(szBuff, sizeof(szBuff), "%ld,%ld", size.cx, size.cy);

    // Create buffer and write pref
    pBuffer = CreateIHXBuffer(szBuff);
    HX_RESULT result = WritePref(nPrefKey,nIndex,pBuffer);

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return result;
}

HX_RESULT CPrefTable::WritePrefPoint(INT32 nPrefKey,const HXxPoint& pt,INT32 nIndex)
{
    IHXBuffer* pBuffer = NULL;
    char szBuff[MAX_POINT_BUFFER]; /* Flawfinder: ignore */
    
    SafeSprintf(szBuff, sizeof(szBuff), "%ld,%ld", pt.x, pt.y);

    // Create buffer and write pref
    pBuffer = CreateIHXBuffer(szBuff);
    HX_RESULT result = WritePref(nPrefKey,nIndex,pBuffer);

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return result;
}

HX_RESULT CPrefTable::WritePrefString(INT32 nPrefKey,const char* szString,INT32 nIndex)
{
    IHXBuffer* pBuffer = NULL;

    // Create buffer and write pref
    pBuffer = CreateIHXBuffer(szString);
    HX_RESULT result = WritePref(nPrefKey,nIndex,pBuffer);

    // Don't need buffer anymore
    if (pBuffer)
	pBuffer->Release();

    return result;
}

HX_RESULT CPrefTable::WritePrefBuffer(INT32 nPrefKey,IHXBuffer* pBuffer,INT32 nIndex)
{
    return WritePref(nPrefKey,nIndex,pBuffer);
}

HX_RESULT CPrefTable::BeginSubPref(INT32 nPrefKey)
{
    // Index is out of table range if this is false
    HX_ASSERT(nPrefKey >= 0 && nPrefKey < m_nTableEntries);

    // Need to call SetupPrefTable() if this is false
    HX_ASSERT(m_pPrefTable);

    // Index is out of table range
    if (nPrefKey < 0 || nPrefKey >= m_nTableEntries)
	return HXR_INVALID_PARAMETER;

    // Need to call SetupPrefTable()
    if (!m_pPrefTable || !m_pCPref)
	return HXR_UNEXPECTED;

    // Call BeginSubPref with the actual string
    return m_pCPref->BeginSubPref(m_pPrefTable[nPrefKey].szPrefName);
}

///////////////////////////////////////////////
#ifdef PREFTESTING

#include <stdio.h>

#define INITGUID

#include "hxcom.h"
#include "ihxpckts.h"

#include "pref.h"
#include "playpref.h"
#include "chxpckts.h"

int main(int argc, char **argv)
{
	CPref *mPref = 0;
	CHXBuffer* pinBuffer = 0;
	CHXBuffer* poutBuffer = 0;

#ifdef _WINDOWS
	const char* pCompanyName = HXVER_COMPANY;
	const char* pProductName = HXVER_SDK_PRODUCT;
	int			nMajVer = TARVER_MAJOR_VERSION;
	int			nMinVer = TARVER_MINOR_VERSION;
#else
	const char* pCompanyName = HXVER_COMPANY;
	const char* pProductName = HXVER_SDK_PRODUCT;
	int			nMajVer = 6;
	int			nMinVer = 0;
#endif

	char * pComa = HXFindChar(pCompanyName, ',');
	if(pComa)
        *pComa = 0;

	mPref = CPlayerPref::open_pref(pCompanyName,pProductName,nMajVer,nMinVer);

	pinBuffer = new CHXBuffer;
	if ( !pinBuffer )
	{
		printf("can't create CHXBuffer\n");
		exit(0);
	}
	char *svalue = 0;
	svalue = (char*) new char [ 10 ];
	strcpy(svalue, "IsADog"); /* Flawfinder: ignore */
	pinBuffer->AddRef();
	pinBuffer->Set((const unsigned char*) svalue, strlen(svalue) + 1);
	mPref->write_pref("Pushkin",pinBuffer);
	mPref->read_pref("Pushkin",poutBuffer);
//	printf("Pushkin is: %s\n",(char*) (poutBuffer->GetBuffer()));

	if ( mPref ) 
		delete mPref;

	exit(0);
}
#endif
