/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win32_dll.cpp,v 1.6 2004/07/09 18:19:14 hubbe Exp $
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

#include "hlxclib/windows.h"

#include "hxassert.h"

#if !defined _DEBUG && (defined WIN32 || defined _WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
#define AVOID_LOADLIBRARY_FAILURE
#endif

static const char ErrLoadFailed[] = "Unable to load library";
static const char ErrSymbolNotFound[] = "Symbol not found";

class Win32DLLAccess : public DLLAccessImp
{
public:
    Win32DLLAccess();
    virtual ~Win32DLLAccess();
    virtual int Open(const char* dllName);
    virtual int Close() ;
    virtual void* GetSymbol(const char* symbolName);
    virtual const char* GetErrorStr() const;
    virtual char* CreateVersionStr(const char* dllName) const;
    virtual void CreateName(const char* short_name, const char* long_name, char* out_buf,
		      UINT32& out_buf_len, UINT32 nMajor, UINT32 nMinor);

private:
    HINSTANCE m_handle;
    const char* m_pErrorStr;
};

Win32DLLAccess::Win32DLLAccess() : 
    m_handle(0)
{}

Win32DLLAccess::~Win32DLLAccess()
{
    if (m_handle)
    {
	Close();
    }
}

int Win32DLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;

    if (m_handle)
    {
	Close();
    }

#ifdef AVOID_LOADLIBRARY_FAILURE
    UINT nOldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
#endif /* AVOID_LOADLIBRARY_FAILURE */

    m_handle = LoadLibrary(OS_STRING(dllName));

#ifdef AVOID_LOADLIBRARY_FAILURE
    ::SetErrorMode(nOldMode);
#endif /* AVOID_LOADLIBRARY_FAILURE */

    if (m_handle)
    {
	ret = DLLAccess::DLL_OK;
    }
    else
    {
	m_pErrorStr = ErrLoadFailed;
    }

#if defined _FLIGHTRECORDING && defined _WIN32
    if (m_handle)
    {
        FlightRecorderPassContextToDll(m_handle);
    }
#endif    

    return ret;
}

int Win32DLLAccess::Close()
{
#if defined(_DEBUG)
    // if debug build, don't free library if "HXMemoryLeak" is enabled.
    // set  \\HKEY_CLASSES_ROOT\HXMemoryLeak  to "1" for mem leak debugging
    if (::HXDebugOptionEnabled("HXMemoryLeak")==FALSE)
	FreeLibrary(m_handle);
#else
    FreeLibrary(m_handle);
#endif

    m_handle = 0;

    return DLLAccess::DLL_OK;
}

void* Win32DLLAccess::GetSymbol(const char* symbolName)
{
    void* ret = GetProcAddress(m_handle, OS_STRING(symbolName));

    if (!ret)
    {
	m_pErrorStr = ErrSymbolNotFound;
    }

    return ret;
}

const char* Win32DLLAccess::GetErrorStr() const
{
    return m_pErrorStr;
}

char* Win32DLLAccess::CreateVersionStr(const char* dllName) const
{
    return 0;
}
    
void
Win32DLLAccess::CreateName(const char* short_name, const char* long_name, char* out_buf,
		      UINT32& out_buf_len, UINT32 nMajor, UINT32 nMinor)
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
    return new Win32DLLAccess();
}
