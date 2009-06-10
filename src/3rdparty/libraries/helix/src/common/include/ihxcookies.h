/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxcookies.h,v 1.8 2008/08/04 17:55:04 ping Exp $
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

#ifndef _IHXCOOKIES_H_
#define _IHXCOOKIES_H_

#define HX_HELIX_COOKIES                0x00000000      // Helix Cookie DB
#define HX_EXTERNAL_COOKIES_IE          0x00000001      // Internet Explorer
#define HX_EXTERNAL_COOKIES_NS          0x00000002      // Netscape
#define HX_EXTERNAL_COOKIES_FF_TEXT     0x00000004      // Firefox Text based(cookies.txt), FireFox 2.0 and below
#define HX_EXTERNAL_COOKIES_FF_SQLITE   0x00000008      // Firefox SQLite based(cookies.sqlite), FireFox 3.0 and above
#define HX_EXTERNAL_COOKIES_SF          0x000000010     // Safari

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXBuffer			IHXBuffer;
typedef _INTERFACE	IHXValues			IHXValues;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCookies
 *
 *  Purpose:
 *
 *	Interface to manage Cookies database
 *
 *  IID_IHXCookies:
 *
 *	{00003200-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXCookies, 0x00003200, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			     0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXCookies, IUnknown)
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
     * IHXCookies methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCookies::SetCookies
     *	Purpose:
     *	    Set cookies
     */
    STDMETHOD(SetCookies)	(THIS_
				 const char*	pHost,
				 const char*	pPath,
				 IHXBuffer*	pCookies) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCookies::GetCookies
     *	Purpose:
     *	    Get cookies
     */
    STDMETHOD(GetCookies)	(THIS_
				 const char*	    pHost,
				 const char*	    pPath,
				 REF(IHXBuffer*)   pCookies) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCookiesHelper
 *
 *  Purpose:
 *
 *	Interface to format cookies
 *
 *  IID_IHXCookiesHelper
 *
 *	{00003201-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXCookiesHelper, 0x00003201, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				    0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCookiesHelper

DECLARE_INTERFACE_(IHXCookiesHelper, IUnknown)
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
     * IHXCookiesHelper methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCookiesHelper::Pack
     *	Purpose:
     *	    convert cookies into IHXValues format
     */
    STDMETHOD(Pack)		(THIS_
				 IHXBuffer*	    pCookies,
				 REF(IHXValues*)   pCookiesHeader) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCookiesHelper::UnPack
     *	Purpose:
     *	    convert cookies into raw data(string) format
     */
    STDMETHOD(UnPack)		(THIS_
				 IHXValues*	    pCookiesHeader,
				 REF(IHXBuffer*)   pCookies) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCookies3
 *
 *  Purpose:
 *
 *	Interface the method SyncRMCookies from HXCookies
 *
 *  IID_IHXCookies3
 *
 *	{CBECD0D8-8F68-4f07-917C-B972E7FCF530}
 *
 */
DEFINE_GUID(IID_IHXCookies3, 0xcbecd0d8, 0x8f68, 0x4f07, 0x91, 0x7c, 0xb9,
				    0x72, 0xe7, 0xfc, 0xf5, 0x30);
#undef  INTERFACE
#define INTERFACE   IHXCookies3

DECLARE_INTERFACE_(IHXCookies3, IUnknown)
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
     * IHXCookies3 methods
     */
    STDMETHOD(SyncRMCookies)	(THIS_ HXBOOL bSave) PURE;
};

#endif /* _IHXCOOKIES_H_ */
