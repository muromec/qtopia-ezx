/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxccf.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _HXCCF_H_
#define _HXCCF_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */

typedef _INTERFACE	IHXCommonClassFactory		IHXCommonClassFactory;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCommonClassFactory
 * 
 *  Purpose:
 * 
 *	RMA interface that manages the creation of common RMA classes.
 * 
 *  IID_IHXCommonClassFactory:
 * 
 *	{00000000-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCommonClassFactory, 0x00000000, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCommonClassFactory

DECLARE_INTERFACE_(IHXCommonClassFactory, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXCommonClassFactory methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCommonClassFactory::CreateInstance
     *	Purpose:
     *	    Creates instances of common objects supported by the system,
     *	    like IHXBuffer, IHXPacket, IHXValues, etc.
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE: Aggregation is never used. Therefore and outer unknown is
     *	    not passed to this function, and you do not need to code for this
     *	    situation.
     */
    STDMETHOD(CreateInstance)		(THIS_
					REFCLSID    /*IN*/  rclsid,
					void**	    /*OUT*/ ppUnknown) PURE;

    /************************************************************************
     *  Method:
     *	    IHXController::CreateInstanceAggregatable
     *  Purpose:
     *	    Creates instances of common objects that can be aggregated
     *	    supported by the system, like IHXSiteWindowed
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
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter) PURE;
};


#endif /*_HXCCF_H_*/
