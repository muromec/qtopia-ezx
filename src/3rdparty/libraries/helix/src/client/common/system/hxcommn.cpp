/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcommn.cpp,v 1.31 2007/07/06 21:58:03 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxstring.h"

#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxmon.h"
#include "hxplugn.h"
#if defined(HELIX_FEATURE_KEYVALUELIST)
#include "hxvalue.h"
#endif /* #if defined(HELIX_FEATURE_KEYVALUELIST) */
#include "hxcache.h"
#include "hxtbuf.h"
#include "hxpref.h"
#include "cachobj.h"

#include "smartptr.h"
#include "miscsp.h"
#include "unkimp.h"
//#include "rncont.h"
#include "hxlist.h"
//#include "rnmap.h"

#include "chxfgbuf.h"
#include "chxpckts.h"
#include "watchlst.h"
#include "hxclreg.h"
#include "hxrquest.h"
#include "hxvalues.h"
#include "recognizer.h"

#if defined(_WINDOWS) && !defined(_WINCE)
#ifdef _WIN32
#include <vfw.h>
#else
#include "hlxclib/windows.h"
#include <drawdib.h>
#endif	/* _WIN32 */
#endif	/* _WINDOWS */

// Needed for CHXSiteWindowed
#include "hxwintyp.h"
#include "chxxtype.h"
#include "hxwin.h"
#include "hxengin.h"
#include "hxsite2.h"
#include "hxslist.h"
//#include "hxcodec.h"
#include "hxvsurf.h"
#include "hxvctrl.h"
#include "dllacces.h"
#include "dllaccesserver.h"
#include "dllpath.h"

#if defined(HELIX_FEATURE_FILESYSTEMMGR)
#include "hxfsmgr.h"
#endif

#if defined(HELIX_FEATURE_LITEPREFS)
#include "chxliteprefs.h"
#endif

#if defined(HELIX_FEATURE_HTTP_SERVICE)
#include "chxhttp.h"
#include "hxhttp.h"
#endif

#include "hxmisus.h"
#include "hxcommn.h"

#include "hxplugn.h"
#include "hxgrpen2.h"
#include "chxuuid.h"
#include "hxmutex.h"
#include "chxthread.h"
#include "chxminiccf.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXCommonClassFactory::HXCommonClassFactory(IUnknown* pContext) :
     m_lRefCount (0)
    ,m_pContext(pContext)
    ,m_pMiniCCF(new CHXMiniCCF())
{
    if (m_pContext)
    {
	m_pContext->AddRef();
    }

    if (m_pMiniCCF)
    {
	m_pMiniCCF->AddRef();
    }
}

HXCommonClassFactory::~HXCommonClassFactory()
{ 
    Close();
}

void
HXCommonClassFactory::Close()
{ 
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pMiniCCF);
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXCommonClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCommonClassFactory), (IHXCommonClassFactory*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCommonClassFactory*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXCommonClassFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXCommonClassFactory::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/************************************************************************
 *	Method:
 *		IHXController::CreateInstance
 *	Purpose:
 *		Creates instances of common objects supported by the system,
 *		like IHXBuffer, IHXPacket, IHXValues, etc.
 *
 *		This method is similar to Window's CoCreateInstance() in its 
 *		purpose, except that it only creates objects of a well known
 *		types.
 *
 *		NOTE: Aggregation is never used. Therefore and outer unknown is
 *		not passed to this function, and you do not need to code for this
 *		situation.
 */
STDMETHODIMP HXCommonClassFactory::CreateInstance
(
    REFCLSID	/*IN*/		rclsid,
    void**	/*OUT*/		ppUnknown
)
{
    if (m_pMiniCCF)
    {
	HX_RESULT res = m_pMiniCCF->CreateInstance(rclsid, ppUnknown);

	if (HXR_NOINTERFACE != res)
	{
	    return res;
	}
    }

    if (IsEqualCLSID(rclsid, CLSID_IHXPacket))
    {
	*ppUnknown = (IUnknown*)(IHXPacket*)(new CHXPacket());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXRTPPacket))
    {
	*ppUnknown = (IUnknown*)(IHXRTPPacket*)(new CHXRTPPacket());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXMultiPayloadPacket))
    {
	*ppUnknown = (IUnknown*)(IHXPacket*)(new CHXMultiPayloadPacket());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if(IsEqualCLSID(rclsid, CLSID_IHXRequest))
    {
	*ppUnknown = (IUnknown*)(IHXRequest*)(new CHXRequest());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_FIFOCACHE)
    else if(IsEqualCLSID(rclsid, CLSID_IHXFIFOCache))
    {
	*ppUnknown = (IUnknown*)(IHXFIFOCache*)(new HXFIFOCache((IUnknown*)this));
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_FIFOCACHE */
#if defined(HELIX_FEATURE_REGION)
#ifdef _WIN32
    else if(IsEqualCLSID(rclsid, CLSID_IHXRegion))
    {
	HXRegion *pRegion = new HXRegion();
        if( !pRegion )
        {
            return HXR_OUTOFMEMORY;
        }
	return pRegion->QueryInterface(IID_IUnknown, (void**) ppUnknown);
    }
#endif
#endif /* HELIX_FEATURE_REGION */
#if defined(HELIX_FEATURE_FRAGMENTBUFFER)
    else if (IsEqualCLSID(rclsid, CLSID_IHXFragmentedBuffer))
    {
	return CHXFragmentedBuffer::CreateInstance((IUnknown**)ppUnknown);
    }
#endif /* HELIX_FEATURE_FRAGMENTBUFFER */
#if defined(HELIX_FEATURE_KEYVALUELIST)
    else if(IsEqualCLSID(rclsid, CLSID_IHXKeyValueList))
    {
	*ppUnknown = (IUnknown*)(IHXKeyValueList*)(new CKeyValueList());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_KEYVALUELIST) */
#if defined(HELIX_FEATURE_FILESYSTEMMGR)
    else if (IsEqualCLSID(rclsid, CLSID_IHXFileSystemManager))
    {
	*ppUnknown = (IUnknown*)(IHXFileSystemManager*)(new HXFileSystemManager(m_pContext));
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_FILESYSTEMMGR */
    else if (IsEqualCLSID(rclsid, CLSID_IHXMutex))
    {
        CHXMutex *pMutex = new CHXMutex();
        if (pMutex)
        {
            return pMutex->QueryInterface(IID_IUnknown, (void**) ppUnknown);
        }
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXEvent))
    {
	*ppUnknown = (IUnknown*)(IHXEvent*)(new CHelixEvent());
	if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXThread))
    {
	*ppUnknown = (IUnknown*)(IHXThread*)(new CHXThread());
	if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXAsyncTimer))
    {
	*ppUnknown = (IUnknown*)(IHXAsyncTimer*)(new CHXAsyncTimer());
	if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_PREFERENCES) && !defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
    else if (IsEqualCLSID(rclsid, CLSID_IHXPreferences))
    {
#if defined(HELIX_FEATURE_LITEPREFS)
	*ppUnknown = (IUnknown*)(IHXPreferences*) CHXLitePrefs::CreateObject();
#else	// HELIX_FEATURE_LITEPREFS
	*ppUnknown = (IUnknown*)(IHXPreferences*) new HXPreferences;
#endif	// HELIX_FEATURE_LITEPREFS
	if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_PREFERENCES && !HELIX_FEATURE_NO_INTERNAL_PREFS */

// Unix now implements this as a factory plugin.  Windows and Mac may
// do so someday too.
#if (defined (_WINDOWS) || defined (_MACINTOSH)) && !defined(_WINCE)
    else if(IsEqualCLSID(rclsid, CLSID_IHXSiteWindowed))
    {
	{
	    IHXPreferences*	pPreferences	= NULL;
	    IHXBuffer*		pBuffer		= NULL;
	    HXBOOL		bUseNewSite	= TRUE;

	    if (m_pContext &&
	        (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences,
                                                      (void**)&pPreferences)))
	    {   
		if (pPreferences->ReadPref("UseHXVideo", pBuffer) == HXR_OK)
		{
		    bUseNewSite = (::atoi((char*) pBuffer->GetBuffer()) == 1);
		    HX_RELEASE(pBuffer);
		}
		HX_RELEASE(pPreferences);
	    }

	    if (bUseNewSite)
	    {
		goto classFactory;
	    }
	}

#ifdef _WIN32

        // Can not use the plugin handler to create the 
        // site, since it uses the same class ID as 
        // pnvideo, and the plugin handler gets REALLY
        // confused.
        // futher we can not use DLL access, since we WANT to keep
        // a reference on the DLL object created.

        UINT32 uDLLNameLen = 256;
        char    szDllName[256]; /* Flawfinder: ignore */
        DLLAccess::CreateName("pnol", "pnoldvideo", szDllName, uDLLNameLen);

        const char* pPath = NULL;
        CHXString fileName;

        pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
        fileName = pPath;
        fileName += "\\";
        fileName += szDllName;

        HINSTANCE hinst = LoadLibrary(fileName);
        FPCREATEINSTANCE fpCreateInstance = (FPCREATEINSTANCE) GetProcAddress(hinst, HXCREATEINSTANCESTR);
        *ppUnknown = 0;
        if (fpCreateInstance)
        {
            fpCreateInstance((IUnknown**) ppUnknown);
            IHXPlugin* pPlug = NULL;
            if (HXR_OK == (*(IUnknown**)ppUnknown)->QueryInterface(IID_IHXPlugin, (void**)&pPlug))
            {
                pPlug->InitPlugin(m_pContext);
                HX_RELEASE(*(IUnknown**)ppUnknown);
                fpCreateInstance((IUnknown**) ppUnknown);
            }
            return HXR_OK;
        }
        return HXR_FAIL;
    }
#endif
#ifdef _MACINTOSH
    goto classFactory;
/*
    {
	    *ppUnknown = (IUnknown*)(IHXSiteWindowed*)(new CHXSiteWindowed(m_pContext));
	}
	return ((IUnknown*)*ppUnknown)->QueryInterface(IID_IHXSiteWindowed,
							(void**)ppUnknown);
*/
    }
#endif
#endif
#if defined(HELIX_FEATURE_VIDEO) && defined (HELIX_FEATURE_MISU)
    else if(IsEqualCLSID(rclsid, CLSID_IHXMultiInstanceSiteUserSupplier))
    {
	*ppUnknown = (IUnknown*)(IHXMultiInstanceSiteUserSupplier*)(new CHXMultiInstanceSiteUserSupplier());
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }
	return ((IUnknown*)*ppUnknown)->
		    QueryInterface(IID_IHXMultiInstanceSiteUserSupplier,
				(void**)ppUnknown);
    }
#endif /* HELIX_FEATURE_VIDEO */
#if !defined(_STATICALLY_LINKED)
    else if(IsEqualCLSID(rclsid, CLSID_IHXPluginGroupEnumerator))
    {
	IHXPlugin2Handler* pPlugin2Handler = NULL;

        if (m_pContext)
        {
            m_pContext->QueryInterface(IID_IHXPlugin2Handler,
                                       (void**)&pPlugin2Handler);
        }

	*ppUnknown = (IUnknown*)(IHXPluginGroupEnumerator*)
	(
	    new CHXPlugin2GroupEnumerator(pPlugin2Handler)
	);
        if( !*ppUnknown )
        {
            return HXR_OUTOFMEMORY;
        }

	HX_RELEASE(pPlugin2Handler);

	return ((IUnknown*)*ppUnknown)->QueryInterface
	(
	    IID_IHXPluginGroupEnumerator,
	    (void**)ppUnknown
	);
    }

#endif /* _STATICALLY_LINKED */
#if defined(HELIX_FEATURE_DLLACCESS_SERVER)
    else if (IsEqualCLSID(rclsid, CLSID_IHXDllAccess))
    {
	*ppUnknown = (IUnknown*)(IHXDllAccess*)(new DLLAccessServer);

	if (*ppUnknown)
	{
	    ((IUnknown*)*ppUnknown)->AddRef();
	    return HXR_OK;
	}

	return HXR_OUTOFMEMORY;
    }
#endif /* #if defined(HELIX_FEATURE_DLLACCESS_SERVER) */
#ifdef HELIX_FEATURE_HTTP_SERVICE
    else if (IsEqualCLSID(rclsid, CLSID_IHXHttp))
    {
	*ppUnknown = (IUnknown*)(IHXHttp*)(new CHXHttp);

	if (*ppUnknown)
	{
	    ((IUnknown*)*ppUnknown)->AddRef();
	    return HXR_OK;
	}

	return HXR_OUTOFMEMORY;
    }
#endif
#ifdef HELIX_FEATURE_FILE_RECOGNIZER
    else if(IsEqualCLSID(rclsid, CLSID_IHXFileRecognizer))
    {
	*ppUnknown = (IUnknown*)(IHXFileRecognizer*)(new CHXFileRecognizer(this));

	if (*ppUnknown)
	{
	    ((IUnknown*)*ppUnknown)->AddRef();
	    return HXR_OK;
	}

    }
#endif /* HELIX_FEATURE_FILE_RECOGNIZER */
    else
    {
#if ((defined (_WINDOWS) || defined (_MACINTOSH)) && !defined(_WINCE)) || (_MACINTOSH)
classFactory:
#endif
	// Try the factory plugins
	IHXPlugin2Handler* pPlugin2Handler = NULL;

        if (m_pContext)
        {
            m_pContext->QueryInterface(IID_IHXPlugin2Handler,
                                       (void**)&pPlugin2Handler);
        }

	*ppUnknown = NULL;
	IUnknown* pIUnknownInstance = NULL;




	if(pPlugin2Handler)
	{
	    if 
	    (
		SUCCEEDED
		(
		    pPlugin2Handler->FindImplementationFromClassID
		    (
			rclsid, 
			pIUnknownInstance
		    )
		)
	    )
	    {
		*ppUnknown = pIUnknownInstance;
		HX_RELEASE(pPlugin2Handler);
		return HXR_OK;
	    }
	}
#if defined(HELIX_FEATURE_PLUGINHANDLER1)
	else
	{   // The encoder still uses the old PluginHandler..

	    // Try the factory plugins
	    PluginHandler* pPluginHandler = NULL;

            if (m_pContext)
            {
                m_pContext->QueryInterface(IID_IHXPluginHandler,
                                           (void**)&pPluginHandler);
	    }

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

			    pFPlugin->InitPlugin(m_pContext);
			    pFPlugin->Release();
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
	    HX_RELEASE(pPluginHandler);
	}
#endif /* HELIX_FEATURE_PLUGINHANDLER1 */
	HX_RELEASE(pPlugin2Handler);
    }
 
    *ppUnknown = NULL;
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *	IHXController::CreateInstanceAggregatable
 *  Purpose:
 *	Creates instances of common objects supported by the system,
 *	like IHXBuffer, IHXPacket, IHXValues, etc.
 *
 *	This method is similar to Window's CoCreateInstance() in its 
 *	purpose, except that it only creates objects of a well known
 *	types.
 *
 *	NOTE 1: Unlike CreateInstance, this method will create internal
 *		objects that support Aggregation.
 *
 *	NOTE 2: The output interface is always the non-delegating 
 *		IUnknown.
 */
STDMETHODIMP HXCommonClassFactory::CreateInstanceAggregatable
(
    REFCLSID	    /*IN*/	rclsid,
    REF(IUnknown*)  /*OUT*/	pUnknown,
    IUnknown*	    /*IN*/	pUnkOuter
)
{
    HX_RESULT res = HXR_NOINTERFACE;
    pUnknown = NULL;

    if (m_pMiniCCF)
    {
	res = m_pMiniCCF->CreateInstanceAggregatable(rclsid, pUnknown, 
                                                     pUnkOuter);
    }

    return res;
}

