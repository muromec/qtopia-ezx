/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bsd_dll.cpp,v 1.10 2007/07/06 20:41:57 jfinnecy Exp $
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

#include "dllacces.h"

#include <dlfcn.h>

#include "hxstrutl.h"
#include "platform/unix/unix_dll_common.h"

class FreeBSDDLLAccess : public DLLAccessImp
{
public:
    FreeBSDDLLAccess();
    virtual ~FreeBSDDLLAccess();
    virtual int Open(const char* dllName);
    virtual int Close() ;
    virtual void* GetSymbol(const char* symbolName);
    virtual const char* GetErrorStr() const;
    virtual char* CreateVersionStr(const char* dllName) const;
    virtual void CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
		      {
		          UnixCreateName(short_name, long_name, out_buf, out_buf_len,
		          nMajor, nMinor);
		      }

private:
    void* m_handle;
};

FreeBSDDLLAccess::FreeBSDDLLAccess() : 
    m_handle(0)
{}

FreeBSDDLLAccess::~FreeBSDDLLAccess()
{
    if (m_handle)
    {
        Close();
    }
}

int FreeBSDDLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;

    if (m_handle)
    {
        Close();
    }
    
    m_handle = dlopen(dllName, 1);

    if (m_handle)
    {
        ret = DLLAccess::DLL_OK;
    }

    HandleHXStopOnLoad(dllName);

    return ret;
}

int FreeBSDDLLAccess::Close()
{
    int ret = DLLAccess::NO_LOAD;

    if(m_handle)
    {
        //GCC 2.95 does not clean up any function scope statics until
        //the atexit handler is called. This raises the problem that
        //if you have such a static defined in a DLL, and the DLL is
        //unloaded before the end of the program, you will get a seg
        //fault when the atexit handler fires as it is trying to clean
        //up memory that is no longer around. GCC 3.2 does not have
        //this problem. So, for any GCC<3.2 we will not do a
        //dlclose(). Be aware that this will make your runtime memory
        //requirment higher as all DLLs can be queried and freed at
        //startup of different apps.
#if defined(__GNUC__)
#if (__GNUC__>3 || (__GNUC__==3 && __GNUC_MINOR__>=2))
        if (!dlclose(m_handle))
        {
            ret = DLLAccess::DLL_OK;
        }
# else // GCC versin < 3
        ret = DLLAccess::DLL_OK;
# endif
#else // Not using gnu C compiler...
        if (!dlclose(m_handle))
        {
            ret = DLLAccess::DLL_OK;
        }
#endif        
        m_handle = 0;
    }

    return ret;
}

void* FreeBSDDLLAccess::GetSymbol(const char* symbolName)
{
    void* ret = 0;

    UINT32 bufSize = strlen(symbolName) + 1;
    char* pNewName = new char[bufSize + 1]; //add extra room for "_"

    if (pNewName)
    {
#if defined(_FREEBSD2) || defined(_OPENBSD)
        pNewName[0] = '_';
        SafeStrCpy(&pNewName[1], symbolName, bufSize);
#else
        SafeStrCpy(pNewName, symbolName, bufSize);
#endif  
        ret = dlsym(m_handle, pNewName);

        delete [] pNewName;
    }

    return ret;
}

const char* FreeBSDDLLAccess::GetErrorStr() const
{
    return dlerror();
}

char* FreeBSDDLLAccess::CreateVersionStr(const char* dllName) const
{
    return UnixFindDLLVersion(dllName);
}

DLLAccessImp* DLLAccess::CreatePlatformDLLImp()
{
    return new FreeBSDDLLAccess();
}
