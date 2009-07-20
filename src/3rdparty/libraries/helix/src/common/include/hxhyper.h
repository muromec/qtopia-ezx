/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxhyper.h,v 1.5 2007/04/14 04:42:06 ping Exp $
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

#ifndef _HXHYPER_H_
#define _HXHYPER_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE   IUnknown				IUnknown;
typedef _INTERFACE   IHXValues				IHXValues;
typedef _INTERFACE   IHXHyperNavigate			IHXHyperNavigate;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHyperNavigate
 * 
 *  Purpose:
 * 
 *	Allows you to perform simple "Go to URL" operations.
 * 
 *  IID_IHXHyperNavigate:
 * 
 *	{00000900-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXHyperNavigate, 0x00000900, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXHyperNavigate

DECLARE_INTERFACE_(IHXHyperNavigate, IUnknown)
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
     *	IHXHyperNavigate methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate::GoToURL
     *	Purpose:
     *	    Performs a simple Go To URL operation.
     *	Parameters:
     *      pURL: fully qualified URL such as http://www.real.com
     *	    pTarget: target frame.  To not use a frame, set this to NULL
     */
    STDMETHOD(GoToURL)	    (THIS_
			    const char* pURL,
			    const char* pTarget) PURE;

};

//	$Private:
#define URL_COMMAND "command:"
//	$EndPrivate.

//	$Private:
/* This will be made public post Redstone Beta 1 XXXRA */

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHyperNavigate2
 * 
 *  Purpose:
 * 
 *	Allows you to perform advanced operations for a given URL.
 * 
 *  IID_IHXHyperNavigate2:
 * 
 *	{00000901-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXHyperNavigate2, 0x00000901, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXHyperNavigate2

DECLARE_INTERFACE_(IHXHyperNavigate2, IUnknown)
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
     *	IHXHyperNavigate2 methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate2::Execute
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:	    URL (absolute or relative)
     *	    pTargetInstance:	
     *	    pTargetApplication: 
     *	    pTargetRegion:
     *	    pParams:
     */
    STDMETHOD(Execute)	    (THIS_
			    const char* pURL,
			    const char* pTargetInstance,
			    const char* pTargetApplication,
			    const char* pTargetRegion,
			    IHXValues* pParams) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHyperNavigateWithContext
 * 
 *  Purpose:
 * 
 *	Allows you to perform advanced operations for a given URL.
 * 
 *  IID_IHXHyperNavigateWithContext:
 * 
 *	{00000902-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXHyperNavigateWithContext, 0x00000902, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXHyperNavigateWithContext

DECLARE_INTERFACE_(IHXHyperNavigateWithContext, IUnknown)
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
     *	IHXHyperNavigateWithContext methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigateWithContext::ExecuteWithContext
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:		    URL (absolute or relative)
     *	    pTargetInstance:	
     *	    pTargetApplication: 
     *	    pTargetRegion:
     *	    pParams:
     *	    pContext:
     */
    STDMETHOD(ExecuteWithContext)   (THIS_
				    const char* pURL,
				    const char* pTargetInstance,
				    const char* pTargetApplication,
				    const char* pTargetRegion,
				    IHXValues* pParams,
				    IUnknown*	pContext) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHyperNavigateHint
 * 
 *  Purpose:
 * 
 *	Passes future URLs which *may* be hurled
 * 
 *  IID_IHXHyperNavigateHint:
 * 
 *	{D6507709-F344-4011-94EE-5737D378EC4A}
 * 
 */

DEFINE_GUID(IID_IHXHyperNavigateHint, 0xd6507709, 0xf344, 0x4011, 0x94, 0xee, 0x57,
                        0x37, 0xd3, 0x78, 0xec, 0x4a);

#undef  INTERFACE
#define INTERFACE   IHXHyperNavigateHint

DECLARE_INTERFACE_(IHXHyperNavigateHint, IUnknown)
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
     *	IHXHyperNavigateHint methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigateHint::Hint
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:	    URL (absolute or relative)
     *	    pTarget:	    target for URL
     *	    pParams:        parameters on URL. Currently supported keys include:
     *                           "width":  desired width of target window (ULONG32)
     *                           "height": desired height of target window (ULONG32)
     *                           "begin":  time at which URL will be fired (ULONG32)
     *                                     (0xFFFFFFFF if not known)
     */
    STDMETHOD(Hint) (THIS_ const char* pURL,
                           const char* pTarget,
                           IHXValues*  pParams) PURE;
};

//	$EndPrivate.

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXHyperNavigate)
DEFINE_SMART_PTR(IHXHyperNavigate2)
DEFINE_SMART_PTR(IHXHyperNavigateWithContext)
DEFINE_SMART_PTR(IHXHyperNavigateHint)

#endif /* _HXHYPER_H_ */
