/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: factory.cpp,v 1.13 2008/05/23 15:49:51 ehyche Exp $
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


#define INITGUID

#include "hxcom.h"
#include "hlxclib/stdio.h"
#ifndef _WINCE
#include "hlxclib/signal.h"
#endif

#include "hxtypes.h"
#include "hxcomm.h"

#if defined( _WIN32 )
#include "mmreg.h"
#include "msacm.h"
#endif

#include "hxcore.h"     // IHXPlayer
#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "ihxlist.h" // XXXAJC remove when CHXURL is no longer used
#include "hxlistp.h" // XXXAJC remove when CHXURL is no longer used
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxrendr.h"
#include "hxpends.h"
#include "hxauthn.h"
#include "hxtbuf.h"
#include "hxtset.h"
#include "ihxcookies.h"
#include "hxpxymgr.h"
#include "ihxfgbuf.h"
#include "ihxident.h"
#include "ihxperplex.h"

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

#include "hxtlogutil.h"

#undef INITGUID

#include "httppars.h"
#include "httpmsg.h"
#include "cache.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif
#if defined(HELIX_CONFIG_HTTP_INCLUDE_DATAFSYS)
#include "datafsys.h"
#endif
#include "httpfsys.h"
#include "factory.h"
#include "ihxcookies2.h"

HX_RESULT (STDAPICALLTYPE  * const HTTPPluginFactory::m_fpEntryArray[])(IUnknown**)={
	CHTTPFileSystem::HXCreateInstance,
#if defined(HELIX_CONFIG_HTTP_INCLUDE_DATAFSYS)
	DataFileSystem::HXCreateInstance,
#endif	
	0};

HX_RESULT (STDAPICALLTYPE  * const HTTPPluginFactory::m_fpExitArray[])()={
	CHTTPFileSystem::HXShutdown,
#if defined(HELIX_CONFIG_HTTP_INCLUDE_DATAFSYS)
	DataFileSystem::HXShutdown,
#endif	
	0};

HX_RESULT (* const HTTPPluginFactory::m_fpUnloadArray[])()={
	CHTTPFileSystem::CanUnload,
#if defined(HELIX_CONFIG_HTTP_INCLUDE_DATAFSYS)
	DataFileSystem::CanUnload,
#endif	
	0};


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
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)
(   
    IUnknown**  /*OUT*/	ppIUnknown  
)   
{   
    *ppIUnknown = (IUnknown*)(IHXPluginFactory*)new HTTPPluginFactory();  
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
 *	HXShutdown()
 *
 *  Purpose:
 *
 *	Function implemented by all plugin DLL's to free any *global* 
 *	resources. This method is called just before the DLL is unloaded. 
 *
 */

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXSHUTDOWN)()	
{   
   int i = 0;

   while(HTTPPluginFactory::m_fpExitArray[i])
   {
      HTTPPluginFactory::m_fpExitArray[i]();                  
      i++;
   }
    
   return HXR_OK;  
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
    int i = 0;

    while(HTTPPluginFactory::m_fpUnloadArray[i])
    {
       if (HXR_OK != HTTPPluginFactory::m_fpUnloadArray[i]())
       {
          return HXR_FAIL;
       }
       i++;
    }
    
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HTTPPluginFactory
//  Purpose:
//      Constructor. Counts the number of functions within the
//      Entry Array. Would have liked to use:
//      return (sizeof(m_fpEntryArray)/sizeof(m_fpEntryArray[0]))-1;
//      But for some strange reason the complier spits at it...
//

HTTPPluginFactory::HTTPPluginFactory() :
	m_lRefCount(0)
{
    for(m_usNumOfPlugins=0;m_fpEntryArray[m_usNumOfPlugins];m_usNumOfPlugins++) {};
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP HTTPPluginFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown *)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPluginFactory))
    {
	AddRef();
	*ppvObj = (IHXPluginFactory *)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}



/////////////////////////////////////////////////////////////////////////
//  Method:
//      HTTPPluginFactory::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) HTTPPluginFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HTTPPluginFactory::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) HTTPPluginFactory::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   


/////////////////////////////////////////////////////////////////////////
//  Method:
//      HTTPPluginFactory::GetNumPlugins
//  Purpose:
//      Returns an int, the number of plugins in this DLL.
//      
//

STDMETHODIMP_(UINT16) HTTPPluginFactory::GetNumPlugins()
{
    return m_usNumOfPlugins;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      AudioPluginFactory::GetPlugin
//  Purpose:
//      Returns an IUnknown Reference to the specified Interface
//      
//

STDMETHODIMP HTTPPluginFactory::GetPlugin(UINT16 uindex, IUnknown** pPlugin)
{
    if (uindex<m_usNumOfPlugins)
	return m_fpEntryArray[uindex](pPlugin);
    *pPlugin=0;
    return HXR_NOINTERFACE;
}
