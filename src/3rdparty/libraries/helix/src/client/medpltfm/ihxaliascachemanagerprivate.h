/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxaliascachemanagerprivate.h,v 1.2 2007/07/06 21:58:19 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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


#ifndef _IHXALIASCACHEMANPRIV_H
#define _IHXALIASCACHEMANPRIV_H


#include "hxcom.h"
#include "hxcomptr.h"

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAliasCacheManagerPrivate
 *
 *  Purpose:
 *
 *  IID_IHXAliasCacheManagerPrivate
:
 *
 *  {13196F87-56A4-48cd-9631-699EF3077B5E}

 *
 */
DEFINE_GUID(IID_IHXAliasCacheManagerPrivate, 
    0x13196f87, 0x56a4, 0x48cd, 0x96, 0x31, 0x69, 0x9e, 0xf3, 0x7, 0x7b, 0x5e);


#undef  INTERFACE
#define INTERFACE   IHXAliasCacheManagerPrivate


DECLARE_INTERFACE_(IHXAliasCacheManagerPrivate, IUnknown)
{
    /*
     *	IHXAliasCacheManagerPrivate methods
     */

    STDMETHOD( IsCLSIDAliasedPrivate )(THIS_ REFCLSID clsid, CLSID* pAliasCLSID, IUnknown **ppIParentContext) PURE;
    STDMETHOD_( void, AddAliasCacheObserverPrivate ) ( THIS_ REFCLSID alias, IUnknown *pIChildObserver ) PURE;
    STDMETHOD_( void, RemoveAliasCacheObserverPrivate ) ( THIS_ REFCLSID alias, IUnknown *pIChildObserver ) PURE;
    STDMETHOD_( void, OnAliasRemovedFromInstanceCachePrivate ) ( THIS_ REFCLSID alias ) PURE;
};

DEFINE_SMART_PTR(IHXAliasCacheManagerPrivate);

#endif //_IHXALIASCACHEMANPRIV_H


