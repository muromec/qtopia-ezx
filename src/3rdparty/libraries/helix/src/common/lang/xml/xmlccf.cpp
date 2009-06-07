/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlccf.cpp,v 1.9 2006/11/21 18:29:13 ping Exp $
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
//  $Id: xmlccf.cpp,v 1.9 2006/11/21 18:29:13 ping Exp $

/****************************************************************************
 *  hxxml plugin
 */

#define INITGUID 1

#include "hxcom.h"
#include "hxtypes.h"

#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxcomm.h"
#include "hxxml.h"
#include "hxver.h"
#include "hxerror.h"
#include "hxassert.h"
#include "hxperf.h"

#undef INITGUID


#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "xmlparse.h"
#include "expatprs.h"
#include "xmlccf.h"
#include "hxxml.ver"

#if !defined(_SYMBIAN)
HX_ENABLE_CHECKPOINTS_FOR_MODULE( "Rnxmllib", "RnxmllibPerf.log" )
#endif

/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown**  /*OUT*/	ppIUnknown)   
{   
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new HXXMLCCFPlugin();
    if (*ppIUnknown)	
    {	
	(*ppIUnknown)->AddRef();    
	return HXR_OK;	
    }
    return HXR_OUTOFMEMORY;
}   

/****************************************************************************
 * 
 *  Function:
 * 
 *	CanUnload2()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's.  If it returns HXR_OK, 
 *	then the pluginhandler can unload the DLL.
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK );
}

/****************************************************************************
 * 
 *  Function:
 * 
 *	CanUnload()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's if it returns HXR_OK 
 *	then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
    return ENTRYPOINT(CanUnload2)();
}


HXXMLCCFPlugin::HXXMLCCFPlugin()
: m_lRefCount(0)
, m_pContext(NULL)
, m_pClassFactory(NULL)
{
}

HXXMLCCFPlugin::~HXXMLCCFPlugin()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
}

/************************************************************************
 *  IUnknown COM Interface Methods                          ref:  hxcom.h
 */
STDMETHODIMP
HXXMLCCFPlugin::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCommonClassFactory))
    {
	AddRef();
	*ppvObj = (IHXCommonClassFactory*)this;
	return HXR_OK;
    }
    *ppvObj = 0;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HXXMLCCFPlugin::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HXXMLCCFPlugin::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/************************************************************************
 *  IHXPlugin Interface Methods                         ref:  hxplugn.h
 */
STDMETHODIMP
HXXMLCCFPlugin::GetPluginInfo(
	  REF(HXBOOL)        bLoadMultiple,
	  REF(const char*) pDescription,
	  REF(const char*) pCopyright,
	  REF(const char*) pMoreInfoURL,
	  REF(UINT32)      versionNumber
	)
{
    bLoadMultiple = TRUE;
    pDescription = "RealNetworks XML Parser Plugin";
    pCopyright = HXVER_COPYRIGHT;
    pMoreInfoURL = HXVER_MOREINFO;
    versionNumber = TARVER_ULONG32_VERSION;
    return HXR_OK;
}

STDMETHODIMP
HXXMLCCFPlugin::InitPlugin(IUnknown* pHXCore)
{
    HX_RELEASE(m_pContext);
    m_pContext = pHXCore;
    m_pContext->AddRef();
    return HXR_OK;
}

/* IHXCommonClassFactory */

STDMETHODIMP
HXXMLCCFPlugin::CreateInstance( REFCLSID /*IN*/ rclsid,
			      void**   /*OUT*/ ppUnknown)
{
    if (IsEqualCLSID(rclsid, CLSID_IHXXMLParser))
    {
	*ppUnknown = (IUnknown*)(IHXXMLParser*)(new HXExpatXMLParser(m_pContext));
	((IUnknown*)*ppUnknown)->AddRef();
	return HXR_OK;
    }
    *ppUnknown = NULL;
    return HXR_NOINTERFACE;
}
    
STDMETHODIMP
HXXMLCCFPlugin::CreateInstanceAggregatable(
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter)
{
     return HXR_NOINTERFACE;
}
