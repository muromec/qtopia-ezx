/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hpux_dll.cpp,v 1.5 2004/12/02 23:32:47 nhart Exp $
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

#include <dlfcn.h>
#include <signal.h>

#include "platform/unix/unix_dll_common.h"

class HPUXDLLAccess : public DLLAccessImp
{
public:
    HPUXDLLAccess();
    virtual ~HPUXDLLAccess();
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

HPUXDLLAccess::HPUXDLLAccess() : 
    m_handle(0)
{}

HPUXDLLAccess::~HPUXDLLAccess()
{
    if (m_handle)
    {
	Close();
    }
}

int HPUXDLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;
	char *err = NULL;

    if (m_handle)
    {
	Close();
    }

	sigset_t newset, oldset;
	// block all signals, avoiding thread switch
	sigfillset(&newset);
	sigprocmask(SIG_SETMASK, &newset, &oldset);

    m_handle = dlopen(dllName, RTLD_LAZY);

	sigprocmask(SIG_SETMASK, &oldset, &newset);

    if (m_handle)
    {
        ret = DLLAccess::DLL_OK;
    } else {
		err = dlerror();
	}

    HandleHXStopOnLoad(dllName);

    return ret;
}

int HPUXDLLAccess::Close()
{
    int ret = DLLAccess::NO_LOAD;

    if (m_handle)
    {
        if (!dlclose(m_handle))
        {
            ret = DLLAccess::DLL_OK;
        }
        m_handle = 0;
    }

    return ret;
}

void* HPUXDLLAccess::GetSymbol(const char* symbolName)
{
    return dlsym(m_handle, symbolName);
}

const char* HPUXDLLAccess::GetErrorStr() const
{
    return dlerror();
}

char* HPUXDLLAccess::CreateVersionStr(const char* dllName) const
{
    return UnixFindDLLVersion(dllName);
}

DLLAccessImp* DLLAccess::CreatePlatformDLLImp()
{
    return new HPUXDLLAccess();
}
