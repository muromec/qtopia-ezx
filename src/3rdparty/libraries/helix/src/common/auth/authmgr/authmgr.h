/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: authmgr.h,v 1.5 2007/07/06 20:34:54 jfinnecy Exp $
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

#ifndef _AUTHMGR_H_
#define _AUTHMGR_H_

#include "unkimp.h"

/* forward decl. */
struct IUnknown;
struct IHXCommonClassFactory;
struct IHXRegistry;
struct IHXPlugin;

class CHXString;
class CHXSimpleList;


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
STDAPI HXCreateInstance(IUnknown**  /*OUT*/	ppIUnknown);


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
STDAPI HXShutdown(void);


/////////////////////////////////////////////////////////////////////////////
// 
//  Class:
//
//  	CHXAuthFactory
//
//  Purpose:
//
//  	Authenticator Factory.
//

class CHXAuthFactory 
    : public CUnknownIMP
    , public IHXPlugin
    , public IHXCommonClassFactory
{
    DECLARE_UNKNOWN(CHXAuthFactory)

public:
    CHXAuthFactory();

    ~CHXAuthFactory();

    /*** IHXPlugin methods ***/

    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    unInterfaceCount	the number of standard HX interfaces 
     *				supported by this plugin DLL.
     *	    pIIDList		array of IID's for standard HX interfaces
     *				supported by this plugin DLL.
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)        /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    /************************************************************************
     *	Method:
     *	    IHXCommonClassFactory::CreateInstance
     *	Purpose:
     *	    Creates instances of CClientAuthManager and CServerAuthManager.
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE: Aggregation is never used. Therefore and outer unknown is
     *	    not passed to this function, and you do not need to code for this
     *	    situation.
     */
    STDMETHOD(CreateInstance)
    (
	REFCLSID    /*IN*/  rclsid,
	void**	    /*OUT*/ ppUnknown
    );

    /************************************************************************
     *  Method:
     *	    IHXController::CreateInstanceAggregatable
     *  Purpose:
     *	    Creates instances of common objects that can be aggregated
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE 1: Unlike CreateInstance, this method will create internal
     *		    objects that support Aggregation.
     *
     *	    NOTE 2: The output interface is always the non-delegating 
     *		    IUnknown.
     */
    STDMETHOD(CreateInstanceAggregatable)
    (
	REFCLSID	    /*IN*/  rclsid,
	REF(IUnknown*)  /*OUT*/ ppUnknown,
	IUnknown*	    /*IN*/  pUnkOuter
    );

private:
    IUnknown*			m_pContext;
    static const char*		zm_pDescription;
    static const char*		zm_pCopyright;
    static const char*		zm_pMoreInfoURL;
};

#endif // !_AUTHMGR_H_
