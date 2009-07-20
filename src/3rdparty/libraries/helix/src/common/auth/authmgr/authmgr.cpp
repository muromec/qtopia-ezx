/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: authmgr.cpp,v 1.6 2007/07/06 20:34:54 jfinnecy Exp $
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

#include <stdio.h>

#include "authmgr.ver"

#include "hxtypes.h"

#define INITGUID

#include "hxcom.h"
#include "hxcomm.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxplugn.h"
#include "hxprefs.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxmon.h"
#include "hxauth.h"
#include "hxauthn.h"
#include "hxplgns.h"
#include "hxdb.h"
#include "hxvalue.h"
#include "ihxfgbuf.h"
#undef INITGUID

#include "smartptr.h"
#include "hxathsp.h"
#include "hxplnsp.h"
#include "miscsp.h"

#include "cliauth.h"
#include "svrauth.h"

#include "authmgr.h"

#include "hxver.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _AIX
#include "hxtbuf.h"
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Authmgr);
#endif

const char* CHXAuthFactory::zm_pDescription	= 
(
    "Helix Authentication Manager"
);
const char* CHXAuthFactory::zm_pCopyright	= HXVER_COPYRIGHT;
const char* CHXAuthFactory::zm_pMoreInfoURL	= HXVER_MOREINFO;

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
 *	NOTE: Aggregation is never used. Therefore an outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 *
 */
STDAPI ENTRYPOINT(HXCreateInstance)
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    HX_RESULT hxrRet = CHXAuthFactory::CreateInstance(ppIUnknown);
    return hxrRet;
}

/****************************************************************************
 *
 *  Function:
 *
 *	HXShutdown()
 *
 *  Purpose:
 *
 *	Function implemented by all plugin DLL's to free any *global*
 *	resources. This method is called just before the DLL is unloaded.
 *
 */
STDAPI ENTRYPOINT(HXShutdown)(void)
{
    return HXR_OK;
}

BEGIN_INTERFACE_LIST(CHXAuthFactory)
    INTERFACE_LIST_ENTRY(IID_IHXPlugin, IHXPlugin)
    INTERFACE_LIST_ENTRY(IID_IHXCommonClassFactory, IHXCommonClassFactory)
END_INTERFACE_LIST

CHXAuthFactory::CHXAuthFactory()
    : m_pContext(NULL)
{
}

CHXAuthFactory::~CHXAuthFactory()
{
    HX_RELEASE(m_pContext);
}

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP 
CHXAuthFactory::InitPlugin(IUnknown* /*IN*/ pContext)
{
    /* This plugin does not need any context */
    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    if (m_pContext)
    {
	m_pContext->AddRef();
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    unInterfaceCount	the number of standard HX interfaces
 *			supported by this plugin DLL.
 *    pIIDList		array of IID's for standard HX interfaces
 *			supported by this plugin DLL.
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP 
CHXAuthFactory::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP 
CHXAuthFactory::CreateInstance
(
    REFCLSID	/*IN*/  rclsid,
    void**	/*OUT*/ ppUnknown
)
{
    DECLARE_SMART_POINTER(IHXObjectConfiguration)  spocObject;
    HX_RESULT					    hxrRet;

    *ppUnknown = NULL;

    if (IsEqualCLSID(rclsid, CLSID_CHXClientAuthenticator))
    {
	hxrRet = CHXClientAuthenticator::CreateInstance
	(
	    (IUnknown**)ppUnknown
	);

	spocObject = (IUnknown*)(*ppUnknown);
	spocObject->SetContext(m_pContext);
	
	return hxrRet;
    }
    else if (IsEqualCLSID(rclsid, CLSID_CHXServerAuthenticator))
    {
	hxrRet = CServerAuthenticator::CreateInstance
	(
	    (IUnknown**)ppUnknown
	);

	spocObject = (IUnknown*)(*ppUnknown);
	spocObject->SetContext(m_pContext);
	
	return hxrRet;
    }

    return HXR_NOINTERFACE;
}

STDMETHODIMP
CHXAuthFactory::CreateInstanceAggregatable
(
    REFCLSID	    /*IN*/  rclsid,
    REF(IUnknown*)  /*OUT*/ ppUnknown,
    IUnknown*	    /*IN*/  pUnkOuter
)
{
    return HXR_NOINTERFACE;
}

