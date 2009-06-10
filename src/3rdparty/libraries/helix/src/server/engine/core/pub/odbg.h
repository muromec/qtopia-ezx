/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: odbg.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _ODBG_H
#define _ODBG_H

#include "objdbg.h"

class ObjDebugger : public IHXObjDebugger
{
public:
    ObjDebugger();

    void NotifyAddRef();
    void NotifyRelease();
    void DumpState();
 
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(AddRefPoint)      (THIS_
                                ULONG32 Num);

    STDMETHOD(ReleasePoint)     (THIS_
                                ULONG32 Num);

protected:
    UINT32 m_AddRefs[256];
    UINT32 m_Releases[256];
    UINT32 m_ulNextAddRef;
    UINT32 m_ulNextRelease;
    UINT32 m_ulHighestAddRef;
    UINT32 m_ulHighestRelease;
};

inline
ObjDebugger::ObjDebugger()
: m_ulNextAddRef(0)
, m_ulNextRelease(0)
, m_ulHighestAddRef(0)
, m_ulHighestRelease(0)
{
    memset(m_AddRefs, 0, sizeof(UINT32) * 256);
    memset(m_Releases, 0, sizeof(UINT32) * 256);
}

inline void
ObjDebugger::NotifyAddRef()
{
    if (m_ulHighestAddRef < m_ulNextAddRef)
    {
	m_ulHighestAddRef = m_ulNextAddRef;
    }
    m_AddRefs[m_ulNextAddRef]++;
    m_ulNextAddRef  = 0;
}    

inline void
ObjDebugger::NotifyRelease()
{
    if (m_ulHighestRelease < m_ulNextRelease)
    {
	m_ulHighestRelease = m_ulNextRelease;
    }
    m_Releases[m_ulNextRelease]++;
    m_ulNextRelease = 0;
}

inline STDMETHODIMP
ObjDebugger::AddRefPoint(UINT32 Num)
{
    m_ulNextAddRef = Num;
    return HXR_OK;
}

inline STDMETHODIMP
ObjDebugger::ReleasePoint(UINT32 Num)
{
    m_ulNextRelease = Num;
    return HXR_OK;
} 

inline void
ObjDebugger::DumpState()
{
    printf("\tAddRefs:\t");
    int i;
    for (i = 0; i <= m_ulHighestAddRef; i++)
    {
	printf("%d-", m_AddRefs[i]);
    }
    printf("\n\tReleases:\t");
    for (i = 0; i <= m_ulHighestRelease; i++)
    {
	printf("%d-", m_Releases[i]);
    }
    printf("\n");
}

#endif
