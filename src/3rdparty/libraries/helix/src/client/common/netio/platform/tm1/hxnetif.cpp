/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetif.cpp,v 1.4 2007/07/06 21:57:59 jfinnecy Exp $
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

#include "hxcom.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "hxresult.h"
#include "hxslist.h"
#include "netbyte.h"
#include "hxengin.h"
#include "hxnetif.h"

#include <sys/types.h>
#include <sys/stat.h>
//#include <netinet/in.h>
#include <sys/socket.h>
//#include <net/if.h>
//#include <sys/ioctl.h>

// FIXME
// Can the Trimedia ever have more than one IP address?  If so, then 
// this routine must change.

HXNetInterface::HXNetInterface(IUnknown* pContext)
    : m_lRefCount(0)
    , m_bInitialized(FALSE)
    , m_pNetInterfaceList(NULL)
    , m_pSinkList(NULL)
{
}

HXNetInterface::~HXNetInterface()
{
    Close();
}

STDMETHODIMP
HXNetInterface::QueryInterface(REFIID riid, void**ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetInterfaces))
    {
        AddRef();
        *ppvObj = (IHXNetInterfaces*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::AddRef
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) 
    HXNetInterface::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::Release
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) 
    HXNetInterface::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXNetInterface::UpdateNetInterfaces(void)
{
    return HXR_NOTIMPL;
}

HX_RESULT
HXNetInterface::RetrieveNetInterface(CHXSimpleList*& pNetInterfaceList)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(UINT32)
    HXNetInterface::GetNumOfNetInterfaces()
{
    return m_pNetInterfaceList ? m_pNetInterfaceList->GetCount() : 0;
}

STDMETHODIMP HXNetInterface::GetNetInterfaces(UINT16   lIndex,
                                               REF(NIInfo*) pNIInfo)
{
    pNIInfo = NULL;
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXNetInterface::AddAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT   rc = HXR_OK;

    if (!m_pSinkList)
    {
        m_pSinkList = new CHXSimpleList();
    }

    pSink->AddRef();
    m_pSinkList->AddTail(pSink);

    return rc;
}

STDMETHODIMP
HXNetInterface::RemoveAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT   rc = HXR_OK;

    LISTPOSITION lPosition = m_pSinkList->Find(pSink);

    if (!lPosition)
    {
        rc = HXR_UNEXPECTED;
        goto cleanup;
    }

    m_pSinkList->RemoveAt(lPosition);
    pSink->Release();

  cleanup:

    return rc;
}

HXBOOL HXNetInterface::IsNetInterfaceChanged(void)
{
    return FALSE;
}

void
HXNetInterface::Reset(CHXSimpleList* pNetInterfaceList)
{
    if (pNetInterfaceList)
    {
        while (pNetInterfaceList->GetCount())
        {
            NIInfo* pNIInfo = (NIInfo*)pNetInterfaceList->RemoveHead();
            HX_DELETE(pNIInfo);
        }
    }
}

void
HXNetInterface::Close(void)
{
    Reset(m_pNetInterfaceList);
    HX_DELETE(m_pNetInterfaceList);

    if (m_pSinkList)
    {
        HX_ASSERT(m_pSinkList->GetCount() == 0);
        CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
        for (; ndx != m_pSinkList->End(); ++ndx)
        {
            IHXNetInterfacesAdviseSink* pSink = (IHXNetInterfacesAdviseSink*) (*ndx);
            HX_RELEASE(pSink);
        }
        HX_DELETE(m_pSinkList);
    }
}
