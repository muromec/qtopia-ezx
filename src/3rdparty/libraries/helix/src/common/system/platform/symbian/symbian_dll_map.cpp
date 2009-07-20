/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_dll_map.cpp,v 1.6 2005/11/17 23:31:52 rrajesh Exp $
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

#include "platform/symbian/symbian_dll_map.h"
#include "hxassert.h"

class DLLMapInfo
{
public:
    DLLMapInfo(void* pEntryPoint);
    ~DLLMapInfo();
    void* EntryPoint() const { return m_pEntryPoint;}
    void OnLoad() {m_ulCount++;}
    HXBOOL OnUnload();

private:
    void DoCleanup();

    void* m_pEntryPoint;
    UINT32 m_ulCount;
};

DLLMapInfo::DLLMapInfo(void* pEntryPoint) :
    m_pEntryPoint(pEntryPoint),
    m_ulCount(0)
{}

DLLMapInfo::~DLLMapInfo()
{
    if (m_ulCount != 0)
    {
	DoCleanup();
    }
}

HXBOOL DLLMapInfo::OnUnload()
{
    HXBOOL bRet = FALSE;

    HX_ASSERT(m_ulCount);

    if (m_ulCount > 0)
    {
	m_ulCount--;

	if (m_ulCount == 0)
	{
	    DoCleanup();
	    bRet = TRUE;
	}
    }

    return bRet;
}

void DLLMapInfo::DoCleanup()
{
#if !defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
#if defined(_ARM)
    // Configuration options for Symbian_OS_v*.*
    // Make the device behave like the emulator.
    // The emulator calls E32Dll(EDllProcessDetach) when 
    // Close() is called on the library handle. This does
    // not occur on the device so we've added this code
    // to do it.
    TLibraryEntry pEntry = (TLibraryEntry)m_pEntryPoint;
    
    if (pEntry)
    {
	pEntry(EDllProcessDetach);
    }
#endif
#else
    // The EntryPoint stores the function pointer for FreeGlobals.
    // Symbian 3.0 deprecated the EntryPoint API. So FreeGlobal
    // api is added into dlls for cleaning global maps.
    if(m_pEntryPoint != NULL)
    {
        TLibraryEntry pFreeGlobal = (TLibraryEntry)m_pEntryPoint;
        pFreeGlobal(1);
    }
#endif // End of #if !defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)

    
}

SymbianDLLMapImp::SymbianDLLMapImp()
{}

SymbianDLLMapImp::~SymbianDLLMapImp()
{
    HX_ASSERT(m_libList.IsEmpty());

    while(!m_libList.IsEmpty())
    {
	DLLMapInfo* pTmp = (DLLMapInfo*)m_libList.RemoveHead();

	HX_DELETE(pTmp);
    }
}

void SymbianDLLMapImp::OnLoad(void* pDLLEntryPoint)
{
    DLLMapInfo* pInfo = FindInfo(pDLLEntryPoint);

    if (!pInfo)
    {
	pInfo = new DLLMapInfo(pDLLEntryPoint);

	if (pInfo && !m_libList.AddHead(pInfo))
	{
	    // The AddHead() failed
	    HX_DELETE(pInfo);
	}
    }

    if (pInfo)
    {
	pInfo->OnLoad();
    }
}

void SymbianDLLMapImp::OnUnload(void* pDLLEntryPoint)
{
    DLLMapInfo* pInfo = FindInfo(pDLLEntryPoint);

    HX_ASSERT(pInfo);
    if (pInfo)
    {
	if (pInfo->OnUnload())
	{
	    // This was the last unload for this DLL.
	    // Remove it from our list and destroy the info
	    
	    LISTPOSITION pos = m_libList.GetHeadPosition();
	    while(pInfo && pos)
	    {
		if (pInfo == (DLLMapInfo*)m_libList.GetAt(pos))
		{
		    m_libList.RemoveAt(pos);
		    HX_DELETE(pInfo);
		}
		else
		{
		    m_libList.GetNext(pos);
		}
	    }
	}
    }
}

DLLMapInfo* SymbianDLLMapImp::FindInfo(void* pDLLEntryPoint)
{
    DLLMapInfo* pRet = NULL;

    CHXSimpleList::Iterator itr = m_libList.Begin();

    for(; !pRet && (itr != m_libList.End()); ++itr)
    {
	DLLMapInfo* pTmp = (DLLMapInfo*)*itr;

	if (pTmp && (pTmp->EntryPoint() == pDLLEntryPoint))
	{
	    pRet = pTmp;
	}
    }
    return pRet;
}
