/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllacces.cpp,v 1.11 2005/03/14 19:35:25 bobclark Exp $
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

#include "hxtypes.h"

#include "hxassert.h"
#include "dllpath.h"
#include "dllacces.h"
#include "hxstrutl.h"

#include "system.ver"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

const UINT32 DLLAccess::EXTRA_BUF_LEN = 32;

#if !defined(HELIX_CONFIG_NOSTATICS)
HXBOOL g_bReportHXCreateInstancePointer = FALSE;
#endif

DLLAccess::DLLAccess():
    m_dllImp(0),
    m_curError(0),
    m_curErrorString(0),
    m_isOpen(0),
    m_dllName(0),
    m_version(0)
{
}

DLLAccess::DLLAccess(const char* dllName, UINT16 nLibType):
    m_dllImp(0),
    m_curError(0),
    m_curErrorString(0),
    m_isOpen(0),
    m_dllName(0),
    m_version(0)
{
    open(dllName, nLibType);
}

//
//  NOTE:
//  PLEASE PLEASE PLEASE PLEASE PLEASE
//  Check that pointer before deleting it, AND ABSOLUTELY SET THAT PTR TO NULL when 
//  you have deleted it!  If you don't it is possible for double deletions to happen,
//  which can cause enormous problems.    
//
//  If I find you not doing this I am going to come talk you about why you feel so
//  compelled to cause crashes.
//
DLLAccess::~DLLAccess()
{
    if(m_isOpen) // SEH 3/4/99: Added check to avoid allocating an error string.
    {
    	close();
    }

    delete [] m_curErrorString;
    m_curErrorString=NULL;

    delete [] m_dllName;
    m_dllName=NULL;
    
    delete [] m_version;
    m_version=NULL;

    delete m_dllImp;
    m_dllImp = 0;
}

int DLLAccess::open(const char* dllName, UINT16 nLibType)
{
    HX_ASSERT(dllName);

    if(!dllName)
    {
	m_curError = NO_LOAD;
	setErrorString("Invalid DLL name");
	return m_curError;
    }

    if(m_isOpen)
    {
	m_curError = NO_LOAD;
	setErrorString("DLL already open");
	return m_curError;
    }

    delete m_dllImp;

    // Create DLLAccess implementation object
    m_dllImp = CreateDLLImp();
        
    if (m_dllImp)
    {
	CHXString strDllPath;
	
	DLLAccessPath* pDLLAccessPath = m_dllImp->GetDLLAccessPath();

	if ((nLibType != DLLTYPE_NOT_DEFINED) && pDLLAccessPath)
	{
	    if(pDLLAccessPath->GetPath(nLibType))
		strDllPath = pDLLAccessPath->GetPath(nLibType);
	}
	strDllPath += dllName;

	m_curError = m_dllImp->Open((const char*)strDllPath);

	if (m_curError == DLL_OK)
	{
	    m_isOpen = 1;
	    setErrorString("");
	    setDLLName(strDllPath);

	    delete [] m_version;
	    m_version = m_dllImp->CreateVersionStr(strDllPath);

	    FPSETDLLACCESSPATH pSetDLLAccessPath = 
		(FPSETDLLACCESSPATH)getSymbol("SetDLLAccessPath");

	    if(pSetDLLAccessPath && pDLLAccessPath)
	    {
		pDLLAccessPath->PassDLLAccessPath(pSetDLLAccessPath);
	    }

	    // Reset m_curError to DLL_OK since the getSymbol() call
	    // could have changed it's value
	    m_curError = DLL_OK;
	}
	else
	{
	    setErrorString(m_dllImp->GetErrorStr());
	}
    }
    else
    {
	m_curError = NO_LOAD;
	setErrorString("Not enough memory");
    }

    return m_curError;
}

int DLLAccess::close()
{
    if(m_isOpen)
    {
	m_curError = m_dllImp->Close();

	if (m_curError == DLL_OK)
	{
	    setErrorString("");
	}
	else
	{
	    setErrorString(m_dllImp->GetErrorStr());
	}

	m_isOpen = 0;
	setDLLName("");

	delete m_dllImp;
	m_dllImp = 0;
    }
    else
    {
	m_curError = NO_LOAD;
	setErrorString("DLL not loaded");
    }

    return m_curError;
}

void* DLLAccess::getSymbol(const char* symName)
{
    void* ret = 0;

    if(m_isOpen)
    {
	HX_ASSERT(m_dllImp);
	ret = m_dllImp->GetSymbol(symName);

	if (!ret)
	{
	    m_curError = BAD_SYMBOL;
	    setErrorString(m_dllImp->GetErrorStr());
	}
    }
    else
    {
	m_curError = BAD_SYMBOL;
	setErrorString("DLL not loaded");
    }

    return ret;
}

void DLLAccess::setErrorString(const char* str)
{
    if (str)
    {
	delete [] m_curErrorString;

	UINT32 bufSize = strlen(str)+1;
	m_curErrorString = new char[bufSize];
    
	HX_ASSERT(m_curErrorString);
	if (m_curErrorString)
	{
	    SafeStrCpy(m_curErrorString, str, bufSize);
	}
    }
}

void DLLAccess::setDLLName(const char* str)
{
    if (str)
    {
	delete [] m_dllName;    

	UINT32 bufSize = strlen(str)+1;
	m_dllName = new char[bufSize];
    
	HX_ASSERT(m_dllName);
	if (m_dllName)
	{
	    SafeStrCpy(m_dllName, str, bufSize);
	}
    }
}

void
DLLAccess::CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len)
{
    CreateName(short_name, long_name, out_buf, out_buf_len, 
	       TARVER_MAJOR_VERSION, TARVER_MINOR_VERSION);
}

void
DLLAccess::CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
{
    DLLAccessImp* pDLLAccessImp = CreateDLLImp();
    
    pDLLAccessImp->CreateName(short_name, long_name, out_buf, out_buf_len, nMajor, nMinor);
    
    HX_DELETE(pDLLAccessImp);
}

DLLAccessPath* DLLAccessImp::GetDLLAccessPath()
{
    // The default behavior is to call the
    // global GetDLLAccessPath() function
    return ::GetDLLAccessPath();
}

DLLAccessImp* DLLAccess::CreateDLLImp()
{
#if defined(_STATICALLY_LINKED) && defined(HELIX_CONFIG_CONSOLIDATED_CORE)
    return DLLAccess::CreateMetaDLLImp();
#elif defined(_STATICALLY_LINKED)
    return DLLAccess::CreateStaticDLLImp();
#else
    return DLLAccess::CreatePlatformDLLImp();
#endif
}
