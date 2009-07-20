/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllpath.cpp,v 1.11 2005/05/05 18:23:24 rggammon Exp $
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

#include "hlxclib/stdlib.h"
#include "hlxclib/stdio.h"

#include "dllpath.h"
#include "hxdir.h"
#ifdef _MACINTOSH
#include "maclibrary.h"
#endif
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif	

const char* const DLLAccessPath::zm_pszDllTypeNames[DLLTYPE_NUMBER] = 
{
    "DT_NotDef",              // Arbitrary DLLs 
    "DT_Plugins",	      // Plug-ins
    "DT_Codecs",	      // Codecs 
    "DT_EncSDK",	      // Encoder SDK DLLs
    "DT_Common",	      // Common libraries	
    "DT_Update_OB",	      // Setup/Upgrade libraries	
    "DT_Objbrokr",	      // Special entry for the object broker
    "DT_RCAPlugins"	      // Gemini plugins
};

DLLAccessPath::DLLAccessPath()
    : m_lRefCount(0)
{
}

DLLAccessPath::~DLLAccessPath()
{
    RestoreEnvironment();
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) DLLAccessPath::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) DLLAccessPath::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT 
DLLAccessPath::SetAccessPaths(const char* pPathDescriptor)
{
    CHXString strNameValue;
    HX_RESULT theError = HXR_OK;
    
    if(pPathDescriptor)
	strNameValue = pPathDescriptor;

    while(theError == HXR_OK && !strNameValue.IsEmpty())
    {
	int nIndex = strNameValue.Find('=');
	if(nIndex != -1)
	{
	    theError = SetPath(strNameValue.Left(nIndex), 
		strNameValue.Right(strNameValue.GetLength() - nIndex - 1));
	}

	pPathDescriptor += strNameValue.GetLength() + 1;
	strNameValue = pPathDescriptor;
    }

    return theError;
}

HX_RESULT 
DLLAccessPath::SetPath(UINT16 nLibType, const char* szPath)
{     
    if(nLibType >= DLLTYPE_NUMBER)
	return HXR_FAILED;

    return SetPath(zm_pszDllTypeNames[nLibType], szPath);
}

HX_RESULT 
DLLAccessPath::SetPath(const char* szLibType, const char* szPath)
{
    if(szPath)
    {
	CHXString strPath = szPath;
	if(!strPath.IsEmpty())
	{
#ifdef _MACINTOSH
    	    ResolveIndependentPath(strPath);
#endif
	    if(strPath.GetAt(strPath.GetLength() - 1) != OS_SEPARATOR_CHAR)
		strPath += OS_SEPARATOR_STRING;
	
	    m_mapPathes.SetAt(szLibType, strPath);
	}
    }
    return HXR_OK;
}

const char* 
DLLAccessPath::GetPath(UINT16 nLibType)
{
    if(nLibType >= DLLTYPE_NUMBER)
	return NULL;

    return GetPath(zm_pszDllTypeNames[nLibType]);
}

const char* 
DLLAccessPath::GetPath(const char* szLibType)
{
    CHXString strTemp;
    if(!m_mapPathes.Lookup(szLibType, strTemp))
	return NULL;

    return(const char*)m_mapPathes[szLibType];
}

HX_RESULT	
DLLAccessPath::PassDLLAccessPath(FPSETDLLACCESSPATH pSetDLLAccessPath)
{
    POSITION pos = m_mapPathes.GetStartPosition();
    UINT32 nBufLength = 0;

    while(pos)
    {
	CHXString strLibType, strPath;
	m_mapPathes.GetNextAssoc(pos, strLibType, strPath);
	nBufLength += strLibType.GetLength() + strPath.GetLength() + 2;
    }

    if(!nBufLength)
	return HXR_OK;

    nBufLength++;

    char* pBuffer = new char[nBufLength];
    if(!pBuffer)
	return HXR_FAILED;

    pos = m_mapPathes.GetStartPosition();
    UINT32 nPosition = 0;
    while(pos)
    {
	CHXString strLibType, strPath;
	m_mapPathes.GetNextAssoc(pos, strLibType, strPath);
	CHXString strEntry = strLibType + "=" + strPath;
        UINT32 ulBytesToCopy = strEntry.GetLength() + 1;
	memcpy(pBuffer + nPosition, (const char*)strEntry, 
            (ulBytesToCopy <= nBufLength - nPosition ? ulBytesToCopy : nBufLength - nPosition));
	nPosition += strEntry.GetLength() + 1;
    }

    pBuffer[nPosition] = 0;
    HX_ASSERT(nPosition + 1 == nBufLength);

    pSetDLLAccessPath(pBuffer);

    delete[] pBuffer;

    return HXR_OK;
}

HX_RESULT 
DLLAccessPath::AddPathToEnvironment(const char* szPath)
{
    HX_RESULT theError = HXR_OK;

#if !defined(_MACINTOSH) && !defined(WIN32_PLATFORM_PSPC)
    if(szPath)
    {
	char* pPathEnvVar = getenv("PATH");
	CHXString strPathEnvVar;
	if(pPathEnvVar)
	    strPathEnvVar = pPathEnvVar;
    
	if(m_strPathEnvVar.IsEmpty())
	    m_strPathEnvVar = "PATH=" + strPathEnvVar;

	CHXString strResultPath = "PATH=";
	strResultPath += szPath;

	if(!strPathEnvVar.IsEmpty())
	{
	    strResultPath += ";";
	    strResultPath += strPathEnvVar;
	}

#if defined (_AIX) || defined (_LINUX) || defined (_FREEBSD)
	{
	  char *ptr = (char *) ((const char *)strResultPath);
	  if(putenv(ptr))
	    theError = HXR_FAILED;
	}
#elif defined (_SOLARIS) || defined(_MAC_UNIX)
	if(putenv((char *)(const char*)strResultPath))
	    theError = HXR_FAILED;
#else
	if(putenv((const char *)strResultPath))
	    theError = HXR_FAILED;
#endif
    }
#endif

    return theError;
}

HX_RESULT 
DLLAccessPath::RestoreEnvironment()
{
    HX_RESULT theError = HXR_OK;

#if !defined(_MACINTOSH) && !defined(WIN32_PLATFORM_PSPC)
    if(!m_strPathEnvVar.IsEmpty())
    {
#if defined(_AIX) || defined(_LINUX) || defined(_FREEBSD)
      char *ptr = (char *)((const char *)m_strPathEnvVar);
	if(putenv(ptr))
	    theError = HXR_FAILED;
#elif defined (_SOLARIS) || defined (_MAC_UNIX)
	if(putenv((char*)(const char *)m_strPathEnvVar))
	    theError = HXR_FAILED;
#else
	if(putenv((const char *)m_strPathEnvVar))
	    theError = HXR_FAILED;
#endif
	m_strPathEnvVar.Empty();
    }
#endif

    return theError;
}

const char* 
DLLAccessPath::GetLibTypeName(UINT16 nLibType)
{
    if(nLibType >= DLLTYPE_NUMBER)
	return NULL;

    return zm_pszDllTypeNames[nLibType];
}
