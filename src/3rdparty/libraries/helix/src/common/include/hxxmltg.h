/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxmltg.h,v 1.4 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _HXXMLTG_H
#define _HXXMLTG_H


typedef _INTERFACE	IHXXMLTagObject		IHXXMLTagObject;
typedef _INTERFACE	IHXXMLTagObjectResponse IHXXMLTagObjectResponse;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLTagObjectResponse
 * 
 *  IID_IHXXMLTagObjectResponse:
 * 
 *	{00002C02-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLTagObjectResponse, 0x00002C02, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLTagObjectResponse

DECLARE_INTERFACE_(IHXXMLTagObjectResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /************************************************************************
     *  IHXXMLTagObjectResponse
     */
    STDMETHOD(OnTagDone) (THIS_ UINT32 ulInstance, IHXBuffer* pTag) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLTagHandler
 * 
 *  IID_IHXXMLTagHandler:
 * 
 *	{00002C00-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLTagHandler, 0x00002C03, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
	    0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLTagHandler

DECLARE_INTERFACE_(IHXXMLTagHandler, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /************************************************************************
     *  IHXXMLTagHandler
     */
    STDMETHOD(InitTagHandler) (THIS_ IHXValues* pOptions) PURE;
    STDMETHOD(CreateTagObject) (THIS_ IHXXMLTagObject**  ppObj) PURE;
    STDMETHOD(GetTagHandlerInfo) (THIS_
			    REF(const char*) /*OUT*/ pShortName,
			    REF(const char**) /*OUT*/ pTag,
			    REF(HXBOOL)	    /*OUT*/ bCanHandleAsync) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLTagObject
 * 
 *  IID_IHXXMLTagObject:
 * 
 *	{00002C01-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLTagObject, 0x00002C04, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
	    0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLTagObject

DECLARE_INTERFACE_(IHXXMLTagObject, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /************************************************************************
     *  IHXXMLTagObject
     */
    STDMETHOD(InitTagObject) (IHXXMLTagObjectResponse* pResp) PURE;
    STDMETHOD(OnTag) (THIS_ UINT32 ulInstance, IHXBuffer* pTag) PURE;
    STDMETHOD(Close) (THIS) PURE;

};



#endif
