/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplgns.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _HXPLGNS_H_
#define _HXPLGNS_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IUnknown			    IUnknown;
typedef _INTERFACE  IHXObjectConfiguration	    IHXObjectConfiguration;
typedef _INTERFACE  IHXPluginProperties	    IHXPluginProperties;
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXValues			    IHXValues;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXObjectConfiguration
 *
 *  Purpose:
 *
 *	Interface for setting context and generic means of plugin 
 *	Configuration.
 *
 *  IHXObjectConfiguration:
 *
 *	{0x00002900-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXObjectConfiguration,   0x00002900, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXObjectConfiguration

DECLARE_INTERFACE_(IHXObjectConfiguration, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXObjectConfiguration::SetContext
     *	Purpose:
     *	    This function is called to set the context for the plugin.
     *	    Either IHXPlugin::InitPlugin or this function must be called 
     *	    before calling any other function on the plugin.
     *	    this is intended to be used as a shortcut for the plugin user.
     *	    If one needs to use SetConfiguration they only need to query
     *	    IHXObjectConfiguration saving them from also querying for 
     *	    IHXPlugin.
     *
     */
    STDMETHOD(SetContext)
    (
	THIS_
	IUnknown*   pIUnknownContext
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXObjectConfiguration::SetConfiguration
     *	Purpose:
     *	    This allows the user of a plugin to supply configuration
     *	    information.  This is often a set of CString properties
     *	    extracted from a list in the config file.  This allows
     *	    each plugin within a class (auth plugin, database plugin, etc..) 
     *	    to require a different set of parameters.
     *
     */
    STDMETHOD(SetConfiguration)
    (
	THIS_
	IHXValues* pIHXValuesConfiguration
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPluginProperties
 *
 *  Purpose:
 *
 *	This allows plugins to return whatever properties they want.
 *
 *  IHXPluginProperties:
 *
 *	{0x00002901-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPluginProperties,   0x00002901, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginProperties

DECLARE_INTERFACE_(IHXPluginProperties, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginProperties::GetProperties
     *	Purpose:
     *	    A plugin will implement this in order to return plugin properties
     *	    that will allow it to be identified uniquely.  (PluginID, 
     *	    AuthenticationProtocol, etc..)
     *
     */
    STDMETHOD(GetProperties)
    (
	THIS_
	REF(IHXValues*) pIHXValuesProperties
    ) PURE;

};

#endif /* !_HXPLGNS_H_ */
