/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxrsmg.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _HXXRSMG_H_
#define _HXXRSMG_H_

/****************************************************************************
 *
 * Forward declarations of some interfaces defined/used here-in.
 */
typedef _INTERFACE   IHXExternalResourceManager    IHXExternalResourceManager;
typedef _INTERFACE   IHXExternalResourceReader	    IHXExternalResourceReader;
typedef _INTERFACE   IHXXResource		    IHXXResource;

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXExternalResourceManager
 * 
 *  Purpose:
 *	Platform-independent access to resources
 * 
 * IID_IHXExternalResourceManager:  
 *
 *  {00001200-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXExternalResourceManager, 0x00001200, 0xb4c8, 0x11d0, 
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXExternalResourceManager

DECLARE_INTERFACE_(IHXExternalResourceManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXExternalResourceManager methods
     */
    /************************************************************************
    *  Method:
    *      IHXExternalResourceManager::Init
    *  Purpose:
    *		Reads current language preference and
    *		initializes resource cache
    *		
    */
    STDMETHOD(Init)		    (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXExternalResourceManager::CreateExternalResourceReader
    *  Purpose:
    *		Create an instance of a resource reader for the
    *		"short name" resources in the system
    */
    STDMETHOD(CreateExternalResourceReader)   
				    (THIS_
				    const char* pShortName,
				    REF(IHXExternalResourceReader*) pReader) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXExternalResourceReader
 * 
 *  Purpose:
 *	Platform-independent access to resources
 * 
 * IID_IHXExternalResourceReader:  
 *
 *  {00001201-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXExternalResourceReader, 0x00001201, 0xb4c8, 0x11d0, 
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXExternalResourceReader

DECLARE_INTERFACE_(IHXExternalResourceReader, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXExternalResourceReader methods
     */
    /************************************************************************
    *  Method:
    *      IHXExternalResourceReader::SetDefaultResourceFile
    *  Purpose:
    *		Resource file used if XRS files do not contain
    *		requested resource.
    *		
    */
    STDMETHOD(SetDefaultResourceFile)
				    (THIS_
				    const char* pResourcePath) PURE;

    /************************************************************************
    *  Method:
    *      IHXExternalResourceReader::GetResource
    *  Purpose:
    *		Create an instance of a resource reader for the
    *		"short name" resources in the system
    */
    STDMETHOD_(IHXXResource*, GetResource)   
				    (THIS_
				    UINT32 ulResourceType,
				    UINT32 ulResourceID) PURE;
};

#endif	/* _HXXRSMG_H_ */
