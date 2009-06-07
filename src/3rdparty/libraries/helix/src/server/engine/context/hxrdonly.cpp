/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxrdonly.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include <stdio.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxrdonly.h"
#include "proc.h"
#include "servreg.h"

HXISetPropReadOnly::HXISetPropReadOnly(ServerRegistry* pRegistry,
    Process* pProc)
    : m_lRefCount(0)
    , m_pRegistry(pRegistry)
    , m_pProc(pProc)
{
}

STDMETHODIMP
HXISetPropReadOnly::QueryInterface(REFIID riid, void** ppvObj)
{
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HXISetPropReadOnly::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HXISetPropReadOnly::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

STDMETHODIMP
HXISetPropReadOnly::SetReadOnlyByName(const char* pName)
{
    return m_pRegistry->SetReadOnly(pName, TRUE, m_pProc);
}

STDMETHODIMP
HXISetPropReadOnly::SetReadOnlyById(UINT32 ulRegId)
{
    return m_pRegistry->SetReadOnly(ulRegId, TRUE, m_pProc);
}

STDMETHODIMP
HXISetPropReadOnly::SetWritableByName(const char* pName)
{
    return m_pRegistry->SetReadOnly(pName, FALSE, m_pProc);
}

STDMETHODIMP
HXISetPropReadOnly::SetWritableById(UINT32 ulRegId)
{
    return m_pRegistry->SetReadOnly(ulRegId, FALSE, m_pProc);
}
