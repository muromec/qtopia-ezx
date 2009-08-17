/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxaccesspoint.h,v 1.4 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef IHXACCESSPOINT_H
#define IHXACCESSPOINT_H

#include "hxcom.h"

typedef _INTERFACE IHXValues IHXValues;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAccessPointConnectResponse
 *
 *  Purpose:
 *
 *      Notification inferface for signalling when the access point connect
 *      has completed.
 *
 *  IID_IHXAccessPointConnectResponse:
 *
 *	{9E9CA2D6-CBFE-40f8-94FD-38F4EB5DF80F}
 *
 */
DEFINE_GUID(IID_IHXAccessPointConnectResponse, 0x9e9ca2d6, 0xcbfe, 0x40f8, 0x94, 
	    0xfd, 0x38, 0xf4, 0xeb, 0x5d, 0xf8, 0xf);

#undef  INTERFACE
#define INTERFACE   IHXAccessPointConnectResponse

DECLARE_INTERFACE_(IHXAccessPointConnectResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAccessPointConnectResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXAccessPointConnectResponse::ConnectDone
     *	Purpose:
     *	    Called when the connection to the access point is complete.
     *       The status parameter equals HXR_OK if the connect was successful.
     *
     */
    STDMETHOD(ConnectDone)(THIS_ HX_RESULT status) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAccessPointSelectorResponse
 *
 *  Purpose:
 *      Used by the IHXAccessPointSelector to return information about
 *      the selected access point
 *
 *  IID_IHXAccessPointSelectorResponse:
 *
 *	{9E9CA2D6-CBFE-40f8-94FD-38F4EB5DF811}
 *
 */
DEFINE_GUID(IID_IHXAccessPointSelectorResponse, 
	    0x9e9ca2d6, 0xcbfe, 0x40f8, 0x94, 
	    0xfd, 0x38, 0xf4, 0xeb, 0x5d, 0xf8, 0x11);

#undef  INTERFACE
#define INTERFACE   IHXAccessPointSelectorResponse

DECLARE_INTERFACE_(IHXAccessPointSelectorResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAccessPointSelectorResponse methods
     */


    /************************************************************************
     *	Method:
     *	    IHXAccessPointSelectorResponse::SelectAccessPointDone
     *	Purpose:
     *      Returns the selected access point info
     */
    STDMETHOD(SelectAccessPointDone)(THIS_ HX_RESULT status, 
				     IHXValues* pInfo) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAccessPointSelector
 *
 *  Purpose:
 *      Handles requests for selecting an access point
 *
 *  IID_IHXAccessPointSelector:
 *
 *	{9E9CA2D6-CBFE-40f8-94FD-38F4EB5DF812}
 *
 */
DEFINE_GUID(IID_IHXAccessPointSelector, 0x9e9ca2d6, 0xcbfe, 0x40f8, 0x94, 
	    0xfd, 0x38, 0xf4, 0xeb, 0x5d, 0xf8, 0x12);

#undef  INTERFACE
#define INTERFACE   IHXAccessPointSelector

DECLARE_INTERFACE_(IHXAccessPointSelector, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAccessPointSelector methods
     */

    /************************************************************************
     *	Method:
     *	    IHXAccessPointSelector::SelectAccessPoint
     *	Purpose:
     *      The core uses this to get information about the preferred
     *      access point
     */
    STDMETHOD(SelectAccessPoint)(THIS_ 
				 IHXAccessPointSelectorResponse* pResponse) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAccessPointManager
 *
 *  Purpose:
 *      Manages the network access point
 *
 *  IID_IHXAccessPointManager:
 *
 *	{9E9CA2D6-CBFE-40f8-94FD-38F4EB5DF810}
 *
 */
DEFINE_GUID(IID_IHXAccessPointManager, 0x9e9ca2d6, 0xcbfe, 0x40f8, 0x94, 
	    0xfd, 0x38, 0xf4, 0xeb, 0x5d, 0xf8, 0x10);

#undef  INTERFACE
#define INTERFACE   IHXAccessPointManager

DECLARE_INTERFACE_(IHXAccessPointManager, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAccessPointManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::Connect
     *	Purpose:
     *	    Notifies the access point manager that an object wants the access
     *      point to connect to it's ISP.
     *
     */
    STDMETHOD(Connect) (THIS_ IHXAccessPointConnectResponse* pResp) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::RegisterSelector
     *	Purpose:
     *      Provides the IHXAccessPointManager with an IHXAccessPointSelector 
     *      to use when it needs information about the desired access point.
     *
     */
    STDMETHOD(RegisterSelector)(THIS_ IHXAccessPointSelector* pSelector) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::UnregisterSelector
     *	Purpose:
     *      Unregisters a previously registered IHXAccessPointSelector
     *
     */
    STDMETHOD(UnregisterSelector)(THIS_ IHXAccessPointSelector* pSelector) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::GetActiveAccessPointInfo
     *	Purpose:
     *      Returns information about the access point we are currently 
     *      connected to. This function returns an error if we are 
     *      not connected to an access point.
     *
     */
    STDMETHOD(GetActiveAccessPointInfo)(THIS_ REF(IHXValues*) pInfo) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::GetPreferredAccessPointInfo
     *	Purpose:
     *      Returns information about the access point we want to connect to.
     */
    STDMETHOD(GetPreferredAccessPointInfo)(THIS_ REF(IHXValues*) pInfo) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAccessPointManager::SetPreferredAccessPointInfo
     *	Purpose:
     *      Tells the access point manager about the access 
     *      point we would like it to connect to.
     */
    STDMETHOD(SetPreferredAccessPointInfo)(THIS_ IHXValues* pInfo) PURE;
};


#endif /* IHXACCESSPOINT_H */
