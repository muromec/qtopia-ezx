/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: context.cpp,v 1.11 2006/02/24 01:16:00 ping Exp $
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
#include "commreg.h"
#include "debug.h"
#include "ihxpckts.h"
#include "hxcommn.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxpref.h"
#include "hxpreftr.h"
#include "hxnet.h"
#include "hxsched.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "hxerror.h"

#include "plgnhand.h"
#include "dllpath.h"
#include "readpath.h"

#include "sapmgr.h"
#include "sapclass.h"
#include "mcastaddr_mgr.h"

#include "callback_container.h"
#include "timeval.h"
#include "resolvcache.h"
#include "sockimp.h"
#include "remotenetsvc.h"

#include "rscontext.ver"

#include "context.h"
#include "cfgreg.h"
#include "errorcontroller.h"

#if defined(_WIN32)
#include "hxwinsocklib.h"
#endif

const ULONG32 kDefaultSelectTime = 10000; /* default select wait time in usecs. */

/*
 * Global destructor will be called at DLL shutdown and context
 * will be destroyed
 */
DestructContext remotebroadcast_selfDestructor;

Context::Context() :
    m_lRefCount(0),
    m_pCommonClassFactory(NULL),
    m_pPrefs(NULL),
    m_pNetServices(NULL),
    m_pScheduler(NULL),
    m_plugin_handler(NULL),
    m_pSapManager(NULL),
    m_pMulticastAddressPool(NULL),
    m_pRegistry(NULL),
    m_pErrorController(NULL),
    m_pErrorMessages (NULL)
{
#if defined(_WIN32)
    WSADATA WSAData;
    WORD VersionRequested = 0x0202;
    int ErrorCode = WSAStartup(VersionRequested, &WSAData);

    if (ErrorCode != 0)
    {
	HX_ASSERT(0);
    }
#endif
    m_pScheduler = new HXScheduler((IUnknown*)this);
    m_pScheduler->AddRef();
    m_pScheduler->StartScheduler();

    if (GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN))
    {
        HX_RESULT result;

        m_plugin_handler = new PluginHandler();

        m_plugin_handler->AddRef();
        result = m_plugin_handler->Init((IUnknown*)this);
        if(SUCCEEDED(result))
        {
            m_plugin_handler->Refresh();
        }

    }
    else
    {
        PANIC(("Unable to locate plugin directory! Remote Broadcast Services is unable "
               "to load any plugins!\n"));
    }

    m_pSapManager = (IHXSapManager*)(new CSapManager());
    m_pSapManager->AddRef();

    m_pErrorController = new CErrorcontroller;
    if ( m_pErrorController )
    {
	m_pErrorController->AddRef();
	m_pErrorController->QueryInterface(IID_IHXErrorMessages,
	    (void**)&m_pErrorMessages);
    }

#ifdef _UNIX    
    callbacks.init(512);
#endif
    m_pNetServices = new CRemoteNetServicesContext(&callbacks);
    (IHXNetServices*)(m_pNetServices)->AddRef();
    m_pNetServices->Init((IUnknown*)this);
}

Context::~Context()
{
    Close();
}

STDMETHODIMP
Context::Close()
{
#if defined(_WIN32)
    WSACleanup();
#endif
    HX_RELEASE(m_pSapManager);
    HX_RELEASE(m_pMulticastAddressPool);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pNetServices);

    if(m_pCommonClassFactory)
    {
        m_pCommonClassFactory->Close();
        m_pCommonClassFactory->Release();
        m_pCommonClassFactory = NULL;
    }

    if(m_pPrefs)
    {
        m_pPrefs->Release();
        m_pPrefs = NULL;
    }

    if (m_pScheduler)
    {
        m_pScheduler->StopScheduler();
        HX_RELEASE(m_pScheduler);
    }

    /*
     * The plugin handler must be released after everything else has been
     * cleaned up because Close() will cause plugins to be unloaded. And we
     * may have pointers to objects created by these plugins
     */
    if(m_plugin_handler)
    {
        m_plugin_handler->Close();
        m_plugin_handler->Release();
        m_plugin_handler = NULL;
    }

    if(m_pRegistry)
    {
        m_pRegistry->Close();
        m_pRegistry->Release();
        m_pRegistry = NULL;
    }

    if ( m_pErrorController )
    {
        m_pErrorController->Close();
        m_pErrorController->Release();
        m_pErrorController = NULL;
    }

    return HXR_OK;
}

STDMETHODIMP
Context::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(this);
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCommonClassFactory))
    {
        if(!m_pCommonClassFactory)
        {
            m_pCommonClassFactory = new HXCommonClassFactory((IUnknown*)this);
            m_pCommonClassFactory->AddRef();
        }
        m_pCommonClassFactory->AddRef();
        *ppvObj = (IHXCommonClassFactory*)m_pCommonClassFactory;
        return HXR_OK;
    }
    else if (m_pScheduler && IsEqualIID(riid, IID_IHXScheduler))
    {
        m_pScheduler->AddRef();
        *ppvObj = (IHXScheduler*)m_pScheduler;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
        if(!m_pPrefs)
        {
            m_pPrefs = new HXPreferences();
            m_pPrefs->AddRef();
            m_pPrefs->Open("RealNetworks", "RemoteBroadcastSDK",
                           TARVER_MAJOR_VERSION, TARVER_MINOR_VERSION);

            if (NULL == m_pCommonClassFactory)
            {
                m_pCommonClassFactory = new HXCommonClassFactory((IUnknown*)this);
                m_pCommonClassFactory->AddRef();
            }
        }
        m_pPrefs->AddRef();
        *ppvObj = (IHXPreferences*)m_pPrefs;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetServices))
    {
	(IHXNetServices*)m_pNetServices->AddRef();
        *ppvObj = (IHXNetServices*)m_pNetServices;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSapManager))
    {
        if(m_pSapManager)
        {
            m_pSapManager->AddRef();
            *ppvObj = m_pSapManager;
            return HXR_OK;
        }
    }
    else if (IsEqualIID(riid, IID_IHXMulticastAddressPool))
    {
        if(!m_pMulticastAddressPool)
        {
            m_pMulticastAddressPool = new MulticastAddressPool();
            m_pMulticastAddressPool->AddRef();
        }
        m_pMulticastAddressPool->AddRef();
        *ppvObj = (IHXMulticastAddressPool*)m_pMulticastAddressPool;
        return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXRegistry)) || (IsEqualIID(riid, IID_IHXActiveRegistry)) )
    {
        if(!m_pRegistry)
        {
            m_pRegistry = new ConfigRegistry();
            if(m_pRegistry)
            {
                m_pRegistry->AddRef();
                m_pRegistry->Init((IUnknown*)this);
            }
            else
            {
                *ppvObj = NULL;
                return HXR_FAIL;
            }
        }
        return (m_pRegistry->QueryInterface(riid, ppvObj) );
    }
    else if ( (IsEqualIID(riid, IID_IHXErrorMessages)) || (IsEqualIID(riid, IID_IHXErrorSinkControl)) )
    {
        if (!m_pErrorController )
        {
            m_pErrorController = new CErrorcontroller;
            if ( m_pErrorController )
            {
                    m_pErrorController->AddRef();
            }
            else
            {
                *ppvObj = NULL;
                return HXR_FAIL;
            }
        }
        return m_pErrorController->QueryInterface( riid, ppvObj );
    }
    else if(m_plugin_handler &&
            IsEqualIID(riid, IID_IHXPluginHandler))
    {
        m_plugin_handler->AddRef();
        *ppvObj = (void*)m_plugin_handler;
        return HXR_OK;
    }
    else if (m_plugin_handler &&
             m_plugin_handler->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
Context::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
Context::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
Context::Process(void)
{
    HX_RESULT hRes = HXR_OK;
    BOOL bMoreReaderOrWriter = FALSE;
    BOOL bMoreTSReaderOrWriter = FALSE;
    ULONG32 ulNextSchedulerTime = 0;
    UINT32 ulWriters = 0;
    UINT32 ulReaders = 0;
    Timeval*    selecttimeoutp;
    Timeval     eventtimeout;
    Timeval	defaultSelectTime((LONG32)kDefaultSelectTime);
    int n;

    do
    {
	if ((m_pScheduler) &&
	    (m_pScheduler->GetNextEventDueTimeDiff(ulNextSchedulerTime)))
	{
	    eventtimeout = Timeval((LONG32)ulNextSchedulerTime*1000);
	}
	else
	{
	    eventtimeout = defaultSelectTime;
	}

	if ( eventtimeout > defaultSelectTime )
	{
	    selecttimeoutp = &defaultSelectTime;
	}
	else
	{
	    selecttimeoutp = &eventtimeout;
	}

	n = callbacks.Select((struct timeval*)selecttimeoutp);

	/* n = 0 means select has timed out */
	/* *selecttimeoutp is 0, when next event is due. */
    }while ((n == 0 ) && (*selecttimeoutp > 0));
    
    if (m_pScheduler)
    {
	hRes = m_pScheduler->OnTimeSync(FALSE);
    }

    if (n < 0)
    {
	bMoreReaderOrWriter = FALSE;
	bMoreTSReaderOrWriter = FALSE;
	
#ifdef _UNIX
	if (errno == EBADF)
	{
	    callbacks.HandleBadFds(m_pErrorMessages);
	}
#endif
    }
    else if (n == 0)
    {
	bMoreReaderOrWriter = FALSE;
	bMoreTSReaderOrWriter = FALSE;
    }
    else
    {
	bMoreReaderOrWriter = TRUE;
	bMoreTSReaderOrWriter = TRUE;
	callbacks.invoke_start();
    }

    while ((bMoreTSReaderOrWriter) ||
	   (bMoreReaderOrWriter))
    {
	if (bMoreTSReaderOrWriter)
	{
	    bMoreTSReaderOrWriter = callbacks.invoke_n_ts(
		10, 10);
	}
	
	if (bMoreReaderOrWriter)
	{
	    bMoreReaderOrWriter = callbacks.invoke_n(
		10, 10, &ulReaders, &ulWriters);
	}
    }

    return hRes;
}

void
Context::DestroyGlobals(void)
{
    return;
}


