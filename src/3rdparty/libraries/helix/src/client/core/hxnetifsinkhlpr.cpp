/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetifsinkhlpr.cpp,v 1.2 2005/03/14 20:31:01 bobclark Exp $
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

#include "hxnetifsinkhlpr.h"

HXNetInterfaceAdviseSinkHelper::HXNetInterfaceAdviseSinkHelper() :
    m_pNetInterfaces(NULL),
    m_pNetIFSink(NULL)
{}

HXNetInterfaceAdviseSinkHelper::~HXNetInterfaceAdviseSinkHelper()
{
    clear();    
}

HX_RESULT 
HXNetInterfaceAdviseSinkHelper::Init(IUnknown* pContext, 
                                     void* pObj, 
                                     NetInterfaceUpdatedFunc pFunc)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext && pObj && pFunc)
    {
        clear();

        res = pContext->QueryInterface(IID_IHXNetInterfaces,
                                       (void**)&m_pNetInterfaces);

        if (m_pNetInterfaces)
        {            
            m_pNetIFSink = new AdviseSink(pObj, pFunc);

            if (m_pNetIFSink)
            {
                m_pNetIFSink->AddRef();
                res = m_pNetInterfaces->AddAdviseSink(m_pNetIFSink);
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }

            if (HXR_OK != res)
            {
                HX_RELEASE(m_pNetIFSink);
                HX_RELEASE(m_pNetInterfaces);
            }
        }
    }

    return res;
}

HXBOOL HXNetInterfaceAdviseSinkHelper::NeedsInit() const
{
    return (m_pNetInterfaces != NULL) ? TRUE : FALSE;
}

void HXNetInterfaceAdviseSinkHelper::clear()
{
    if (m_pNetInterfaces && m_pNetIFSink)
    {        
        m_pNetInterfaces->RemoveAdviseSink(m_pNetIFSink);
    }
    HX_RELEASE(m_pNetIFSink);
    HX_RELEASE(m_pNetInterfaces);
}

HXNetInterfaceAdviseSinkHelper::AdviseSink::AdviseSink(void* pObj, 
                                                       NetInterfaceUpdatedFunc pFunc) :
    m_lRefCount(0),
    m_pObj(pObj),
    m_pFunc(pFunc)
{}

/*
 *  IUnknown methods
 */
STDMETHODIMP
HXNetInterfaceAdviseSinkHelper::AdviseSink::QueryInterface(THIS_
                                                           REFIID riid,
                                                           void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IHXNetInterfacesAdviseSink), (IHXNetInterfacesAdviseSink*)this },
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetInterfacesAdviseSink*)this },
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXNetInterfaceAdviseSinkHelper::AdviseSink::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HXNetInterfaceAdviseSinkHelper::AdviseSink::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXNetInterfacesAdviseSink methods
 */
STDMETHODIMP
HXNetInterfaceAdviseSinkHelper::AdviseSink::NetInterfacesUpdated(THIS)
{
    HX_RESULT res = HXR_OK;

    if (m_pFunc)
    {
        res = m_pFunc(m_pObj);
    }

    return res;
}
