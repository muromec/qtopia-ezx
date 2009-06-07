/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_dll.cpp,v 1.14 2005/11/17 23:31:52 rrajesh Exp $
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

#include "dllacces.h"

#include <e32std.h>
#include <stdio.h>
#include <string.h>

#include "platform/symbian/isymbian_dll_map.h"
#include "platform/symbian/symbian_dll_map_inst.h"

#include "hxassert.h"

const char ErrLoadFailed[] = "DLL not loaded";
const char ErrNoOrd1[] = "Function ordinal 1 no present\n";
const char ErrSymbolNotFound[] = "Symbol not found";
const char ErrInvalidOrdinal[] = "Invalid function ordinal";

typedef int (*GetSym2Ord)(const char* pSymbolName);

class SymbianDLLAccess : public DLLAccessImp
{
public:
    SymbianDLLAccess();
    virtual ~SymbianDLLAccess();
    virtual int Open(const char* dllName);
    virtual int Close() ;
    virtual void* GetSymbol(const char* symbolName);
    virtual const char* GetErrorStr() const;
    virtual char* CreateVersionStr(const char* dllName) const;
    virtual void CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor);

private:
    RLibrary m_handle;
    const char* m_pErrorStr;
};

static HBufC* CreateNameDesc(const char* dllName)
{
    TInt length = strlen(dllName);
    HBufC* pRet = HBufC::NewMax(length);

    if (pRet)
    {
	for (TInt i = 0;i < length ; i++)
	{
	    pRet->Des()[i] = dllName[i];
	}
    }

    return pRet;
}

SymbianDLLAccess::SymbianDLLAccess()
{}

SymbianDLLAccess::~SymbianDLLAccess()
{
    Close();
}

int SymbianDLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;

    Close();

    HBufC* pDllName = CreateNameDesc(dllName);
    TInt res = m_handle.Load(pDllName->Des());

    delete pDllName;

    if (res == KErrNone)
    {
	ISymbianDLLMap* pDLLMap = HXSymbianDLLMapInstance::GetInstance();

#ifndef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY  // Configuration options for Symbian_OS_v*.*
	void* pEntrypoint = (void*)m_handle.EntryPoint();
#else
	void* pEntrypoint = (void*)m_handle.Lookup(2);
#endif /* HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY */

	HX_ASSERT(pDLLMap);
	if (pDLLMap && pEntrypoint)
	{
	    pDLLMap->OnLoad(pEntrypoint);
	}

	ret = DLLAccess::DLL_OK;
    }
    else
    {
	if (res == KErrNoMemory)
	{
	    ret = DLLAccess::OUT_OF_MEMORY;
	}
	m_pErrorStr = ErrLoadFailed;
    }


    return ret;
}

int SymbianDLLAccess::Close()
{
    if (m_handle.Handle())
    {
	ISymbianDLLMap* pDLLMap = HXSymbianDLLMapInstance::GetInstance();

#ifndef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY  // Configuration options for Symbian_OS_v*.*
	void* pEntrypoint = (void*)m_handle.EntryPoint();
#else
       void* pEntrypoint = (void*)m_handle.Lookup(2);
#endif /* HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY */
	
	HX_ASSERT(pDLLMap);
	if (pDLLMap && pEntrypoint)
	{
	    pDLLMap->OnUnload(pEntrypoint);
	}
	m_handle.Close();
    }

    return DLLAccess::DLL_OK;
}

void* SymbianDLLAccess::GetSymbol(const char* symbolName)
{
    void* pRet = 0;

    // Get the GetSymbol2Ord function from the DLL.
    // This should always be the function in ordinal 1
    GetSym2Ord getSymbol2Ord = (GetSym2Ord)m_handle.Lookup(1);
    
    if (getSymbol2Ord)
    {
	// Get the ordinal value for the specified symbol
	int symbolOrdinal = getSymbol2Ord(symbolName);
	
	if (symbolOrdinal > 1)
	{
	    // Use the ordinal to get the pointer to the symbol
	    pRet = (void*)m_handle.Lookup(symbolOrdinal);

	    if (!pRet)
		m_pErrorStr = ErrInvalidOrdinal;
	}
	else
	    m_pErrorStr = ErrSymbolNotFound;
    }
    else
	m_pErrorStr = ErrNoOrd1;

    return pRet;
}

const char* SymbianDLLAccess::GetErrorStr() const
{
    return m_pErrorStr;
}

char* SymbianDLLAccess::CreateVersionStr(const char* dllName) const
{
    return 0;
}

void
SymbianDLLAccess::CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
{
    UINT32  long_name_len;

    out_buf[0] = 0;

    long_name_len = strlen(long_name);

    if (long_name_len + DLLAccess::EXTRA_BUF_LEN > out_buf_len)
    {
	ASSERT(0);
	out_buf_len = 0;
	return;
    }

    out_buf_len = sprintf(out_buf, "%s.dll", long_name); /* Flawfinder: ignore */
}

DLLAccessImp* DLLAccess::CreatePlatformDLLImp()
{
    return new SymbianDLLAccess();
}
