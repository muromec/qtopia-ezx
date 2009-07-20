/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxclfact.cpp,v 1.11 2006/02/07 19:41:39 ping Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxstring.h"

#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxmon.h"
#include "hxcomm.h"
#include "hxauth.h"
#include "hxsrc.h"
#include "hxvalue.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "ihxlist.h"

#include "chxfgbuf.h"
#include "chxpckts.h"
#include "servbuffer.h"
#include "servpckts.h"
#include "hxclfact.h"
#include "hxreg.h"
#include "proc.h"
#include "fsmanager.h"
#include "servreg.h"
#include "source_finder.h"
#include "hxrquest.h"
#include "server_context.h"
#include "hxgrpenm.h"
#include "hxvalues.h"
#include "servhttppost.h"
#include "mutex.h"
#include "fastfile_factory.h"
#include "imutex.h"
#include "qos_signal.h"
#include "hxlistp.h"

//  Begin:  For PacketReorderShim
#include "servlist.h"
#include "packetreorderqueue.h"
#include "ihxpacketorderer.h"
#include "packetreordershim.h"
//  End:  For PacketReorderShim

extern BOOL g_bFastMalloc;
extern BOOL g_bFastMallocAll;

HXCommonClassFactory::HXCommonClassFactory(Process* proc, MemCache* m)
                    : m_lRefCount(0), m_proc(proc), m_pMemCache(m)
{
}

HXCommonClassFactory::~HXCommonClassFactory()
{
}

STDMETHODIMP
HXCommonClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCommonClassFactory))
    {
        AddRef();
        *ppvObj = (IHXCommonClassFactory*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HXCommonClassFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HXCommonClassFactory::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXCommonClassFactory::CreateInstance
(
    REFCLSID    /*IN*/  rclsid,
    void**      /*OUT*/ ppUnknown
)
{
    if (IsEqualCLSID(rclsid, CLSID_IHXBuffer))
    {
        *ppUnknown = (IUnknown*)(IHXBuffer*)(new ServerBuffer(TRUE));
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXPacket))
    {
        if (!g_bFastMallocAll && !g_bFastMalloc)
            *ppUnknown = (IUnknown*)(IHXPacket*)(new ServerPacket(TRUE));
        else
            *ppUnknown = (IUnknown*)(IHXPacket*)(new(m_pMemCache)
                ServerPacket(TRUE));
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXRTPPacket))
    {
        if (!g_bFastMallocAll && !g_bFastMalloc)
            *ppUnknown = (IUnknown*)(IHXRTPPacket*)(new ServerRTPPacket(TRUE));
        else
            *ppUnknown = (IUnknown*)(IHXRTPPacket*)(new(m_pMemCache)
                ServerRTPPacket(TRUE));
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues))
    {
        *ppUnknown = (IUnknown*)(IHXValues*)(new CHXHeader());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues2))
    {
        *ppUnknown = (IUnknown*)(IHXValues2*)(new CHXHeader());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXFileSystemManager))
    {
        *ppUnknown = (IUnknown*)(IHXFileSystemManager*)(new FSManager(m_proc));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if(IsEqualCLSID(rclsid, CLSID_IHXPluginGroupEnumerator))
    {
        *ppUnknown = (IUnknown*)(IHXPluginGroupEnumerator*)
            (new CHXPluginGroupEnumerator(m_proc->pc->plugin_handler));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXFragmentedBuffer))
    {
	*ppUnknown = (IUnknown*)(IHXFragmentedBuffer*)(new CHXFragmentedBuffer((IUnknown*)(IHXCommonClassFactory*)this));
	((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXKeyValueList))
    {
        *ppUnknown = (IUnknown*)(IHXKeyValueList*)(new CKeyValueList());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualIID(rclsid, CLSID_IHXSourceFinderObject))
    {
        *ppUnknown = (IUnknown*)(IHXSourceFinderObject*)(new CServerSourceFinder(m_proc));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXHTTPPostObject))
    {
        CHXHTTPPostObject* pObject = CHXHTTPPostObject::CreateObject();
        *ppUnknown = (IUnknown*) (IHXHTTPPostObject*) pObject;
        ((IUnknown*)*ppUnknown)->AddRef();
        pObject->InitObject(m_proc->pc->server_context);
        return HXR_OK;
    }
    else if(IsEqualCLSID(rclsid, CLSID_IHXRequest))
    {
        *ppUnknown = (IUnknown*)(IHXRequest*)(new CHXRequest());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXMutex))
    {
        *ppUnknown = (IUnknown*)(IHXMutex*)(new Mutex());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXFastFileFactory))
    {
        *ppUnknown = (IUnknown*)(IHXFastFileFactory*)
            (new FastFileFactory(m_proc->pc->server_context));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXFastFileFactory2))
    {
        *ppUnknown = (IUnknown*)(IHXFastFileFactory2*)
            (new FastFileFactory(m_proc->pc->server_context));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXQoSSignal))
    {
        *ppUnknown = (IUnknown*)(IHXQoSSignal*)(new QoSSignal());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXList))
    {
        IHXFastAlloc* pAlloc = NULL;
        m_proc->pc->server_context->QueryInterface(IID_IHXFastAlloc,
            (void**)&pAlloc);

        *ppUnknown = (IUnknown*)(IHXList*)(new CHXList(pAlloc));
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else if(IsEqualCLSID(rclsid, IID_IHXPacketOrderer))
    {
        *ppUnknown = (IUnknown*)(IHXPacketOrderer*)(new CPacketOrderShim());
        ((IUnknown*)*ppUnknown)->AddRef();
        return HXR_OK;
    }
    else
    {
        // Try the factory plugins
        PluginHandler* pPluginHandler = m_proc->pc->plugin_handler;

        *ppUnknown = NULL;

        if(pPluginHandler)
        {
            PluginHandler::Factory* pFactories;
            PluginHandler::Plugin*  pPlugin;

            CHXSimpleList::Iterator i;

            pFactories = pPluginHandler->m_factory_handler;
            for(i = pFactories->m_pPlugins->Begin();
                i != pFactories->m_pPlugins->End();
                ++i)
            {
                IUnknown* pInstance = 0;
                pPlugin = (PluginHandler::Plugin*)(*i);
                pPlugin->GetInstance(&pInstance);

                if(pInstance)
                {
                    HX_RESULT res;
                    IHXPlugin* pFPlugin = 0;
                    res = pInstance->QueryInterface(IID_IHXPlugin,
                                                    (void**)&pFPlugin);
                    if(res == HXR_OK)
                    {
                        IHXCommonClassFactory* pClassFactory;
                        IUnknown* pContext;

                        m_proc->pc->server_context->QueryInterface(
                            IID_IUnknown,
                            (void**)&pContext);

                        pFPlugin->InitPlugin(pContext);
                        pFPlugin->Release();
                        pContext->Release();
                        res = pInstance->QueryInterface(
                            IID_IHXCommonClassFactory,
                            (void**)&pClassFactory);
                        if(HXR_OK == res)
                        {
                            res = pClassFactory->CreateInstance(rclsid,
                                                               ppUnknown);
                            if(HXR_OK != res)
                                *ppUnknown = NULL;
                            pClassFactory->Release();
                        }
                    }
                    pInstance->Release();
                    pPlugin->ReleaseInstance();
                    if(*ppUnknown)
                    {
                        return HXR_OK;
                    }
                }
            }
        }
    }

    *ppUnknown = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP
HXCommonClassFactory::CreateInstanceAggregatable
(
    REFCLSID    /*IN*/  rclsid,
    REF(IUnknown *) /*OUT*/ ppUnknown,
    IUnknown*   /*IN*/  pUnkOuter
)
{
    return HXR_NOTIMPL;
}
