/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: metadllaccess.cpp,v 1.5 2006/10/18 23:00:23 ping Exp $
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
#include "hxcom.h"
#include "hlxclib/string.h"
#include "hlxclib/stdio.h"
#include "hxassert.h"
#include "dllacces.h"
#include "dllpath.h"
#include "hxstrutl.h"
#include "hxdir.h" // for OS_SEPARATOR_CHAR


#if !defined(_STATICALLY_LINKED) || !defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#error this file should exist only for consolidated core builds!
#endif

class MetaDLLAccess : public DLLAccessImp
{
public:
    MetaDLLAccess();
    virtual ~MetaDLLAccess();
    virtual int Open(const char* dllName);
    virtual int Close() ;
    virtual void* GetSymbol(const char* symbolName);
    virtual const char* GetErrorStr() const;
    virtual char* CreateVersionStr(const char* dllName) const;
    virtual DLLAccessPath* GetDLLAccessPath();
    virtual void CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor);
private:
    DLLAccessImp* m_pActualDLLAccessImp;
};

MetaDLLAccess::MetaDLLAccess()
  : m_pActualDLLAccessImp(NULL)
{
    // default to platform-specific implementation, although
    // if Open() fails it will fall back to static implementation.
    m_pActualDLLAccessImp = DLLAccess::CreatePlatformDLLImp();
}

MetaDLLAccess::~MetaDLLAccess()
{
    HX_DELETE(m_pActualDLLAccessImp);
}

int
MetaDLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;
    
    ret = m_pActualDLLAccessImp->Open(dllName);
    if (ret != DLLAccess::DLL_OK)
    {
        // couldn't find it in platform-specific (dynamic)
        // DLL so look in the statically-linked implementation
        HX_DELETE(m_pActualDLLAccessImp);
        
        // we may (probably do) need to strip the name from the
        // full pathname.
        const char* justName = strrchr(dllName, OS_SEPARATOR_CHAR);
        if (justName)
        {
            justName++; // get past the separator character
        }
        else
        {
            justName = dllName;
        }
        
        m_pActualDLLAccessImp = DLLAccess::CreateStaticDLLImp();

        ret = m_pActualDLLAccessImp->Open(justName);
        
        if (ret != DLLAccess::DLL_OK)
        {
            HX_DELETE(m_pActualDLLAccessImp);
        }
    }
   
    return ret;
}

int
MetaDLLAccess::Close()
{
    int ret = DLLAccess::DLL_OK;
    
    if (m_pActualDLLAccessImp)
    {
        ret = m_pActualDLLAccessImp->Close();
    }
    return ret;
}

void*
MetaDLLAccess::GetSymbol(const char* symbolName)
{
    HX_ASSERT(m_pActualDLLAccessImp);
    if (m_pActualDLLAccessImp)
    {
        return m_pActualDLLAccessImp->GetSymbol(symbolName);
    }
    else
    {
        return NULL;
    }
}

const char*
MetaDLLAccess::GetErrorStr() const
{
    if (m_pActualDLLAccessImp)
    {
        return m_pActualDLLAccessImp->GetErrorStr();
    }
    else
    {
        return NULL;
    }
}

char*
MetaDLLAccess::CreateVersionStr(const char* dllName) const
{
    HX_ASSERT(m_pActualDLLAccessImp);
    if (m_pActualDLLAccessImp)
    {
        return m_pActualDLLAccessImp->CreateVersionStr(dllName);
    }
    else
    {
        return NULL;
    }
}

DLLAccessPath*
MetaDLLAccess::GetDLLAccessPath()
{
    HX_ASSERT(m_pActualDLLAccessImp);
    if (m_pActualDLLAccessImp)
    {
        return m_pActualDLLAccessImp->GetDLLAccessPath();
    }
    else
    {
        return 0;
    }
}

void
MetaDLLAccess::CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
{
    DLLAccessImp* pDLLAccessImp = NULL;
    if (1)
    {
        pDLLAccessImp = DLLAccess::CreateStaticDLLImp();
    }
    else
    {
        pDLLAccessImp = DLLAccess::CreatePlatformDLLImp();
    }
    pDLLAccessImp->CreateName(short_name, long_name, out_buf, out_buf_len, nMajor, nMinor);
    HX_DELETE(pDLLAccessImp);
}

DLLAccessImp* DLLAccess::CreateMetaDLLImp()
{
    return new MetaDLLAccess();
}
