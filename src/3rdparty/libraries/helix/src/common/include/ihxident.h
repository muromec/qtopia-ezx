/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxident.h,v 1.6 2005/04/22 01:01:13 rggammon Exp $
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

#ifndef _IHXIDENT_H
#define _IHXIDENT_H

DEFINE_CONSTANT_STRING(kRetailerCode,"RetailerCode")
DEFINE_CONSTANT_STRING(kOrigCode,    "OrigCode") // Original distribution code

typedef void* HXxWindowID;

// RealPlayer features
#define RPF_PLAYERPLUS		0x00000001	// is it a plus player?
#define RPF_REALPLAYER		0x00000002	// is it the RealPlayer?
#define RPF_EMBEDPLAYER		0x00000004	// is it an embedded player?

// {AE7EB8A0-32DC-11d2-8AC0-00C04FEE3A97}
DEFINE_GUID(IID_IHXProductIdentity, 
0xae7eb8a0, 0x32dc, 0x11d2, 0x8a, 0xc0, 0x0, 0xc0, 0x4f, 0xee, 0x3a, 0x97);

DECLARE_INTERFACE_(IHXProductIdentity, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    //********************************************************************
    // Method:	IHXProductIdentity::GetMajorVersion
    // Purpose:	return the major version of the product
    //********************************************************************
    STDMETHOD_(UINT32,GetMajorVersion) (THIS) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetMinorVersion
    // Purpose:	return the minor version of the product
    //********************************************************************
    STDMETHOD_(UINT32,GetMinorVersion) (THIS) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetReleaseNumber
    // Purpose:	return the release number of the product
    //********************************************************************
    STDMETHOD_(UINT32,GetReleaseNumber) (THIS) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetBuildNumber
    // Purpose:	return the build number of the product
    //********************************************************************
    STDMETHOD_(UINT32,GetBuildNumber) (THIS) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetLanguageId
    // Purpose:	return the language id of this product
    //********************************************************************
    STDMETHOD_(UINT32,GetLanguageId) (THIS) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetProductName
    // Purpose:	return the name of this product
    //********************************************************************
    STDMETHOD(GetProductName) (THIS_ char* pName, UINT16 len) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetLanguageString
    // Purpose:	return the language string of this product
    //********************************************************************
    STDMETHOD(GetLanguageString) (THIS_ char* pLanguage, UINT16 len) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetVersionString
    // Purpose:	return the version string of this product
    //********************************************************************
    STDMETHOD(GetVersionString) (THIS_ char* pVersion, UINT16 len) PURE;
    //********************************************************************
    // Method:	IHXProductIdentity::GetDistributionCode
    // Purpose:	return the distribution code string of this product
    //********************************************************************
    STDMETHOD(GetDistributionCode) (THIS_ char* pDistCode, UINT16 len) PURE;
};

// {AE7EB8A0-32DC-11d2-7AC0-00C04FEE3A98}
DEFINE_GUID(IID_IHXProductIdentity2,
0xae7eb8a0, 0x32dc, 0x11d2, 0x7a, 0xc0, 0x0, 0xc0, 0x4f, 0xee, 0x3a, 0x98);

DECLARE_INTERFACE_(IHXProductIdentity2, IHXProductIdentity)
{
    //********************************************************************
    // Method:	IHXProductIdentity2::GetPath
    // Purpose:	return the full path to the product's main executable
    //********************************************************************
    STDMETHOD(GetPath) (THIS_ char* pPath, UINT16 len) PURE;

    //********************************************************************
    // Method:	IHXProductIdentity2::GetActiveWindow
    // Purpose:	get active window of the client
    //********************************************************************
    STDMETHOD(GetActiveWindow) (THIS_ REF(HXxWindowID) window) PURE;

    //********************************************************************
    // Method:	IHXProductIdentity2::GetProductTitle
    // Purpose:	return the title of this product
    //********************************************************************
    STDMETHOD(GetProductTitle) (THIS_ char* pTitle, UINT16 len) PURE;

    //********************************************************************
    // Method:	IHXProductIdentity2::HasFeatures
    // Purpose:	The possible values for ulFeatures are listed at the top of 
    //		this file.  These values can be combined with the | operator
    //		if desired to ask for multiple features in one call.  TRUE
    //		is returned only if ALL the requested features are supported.
    //********************************************************************
    STDMETHOD_(HXBOOL,HasFeatures) (THIS_ UINT32 ulFeatures) PURE;

    //********************************************************************
    // Method:	IHXProductIdentity2::GetGUID
    // Purpose:	return the GUID of product's user
    //********************************************************************
    STDMETHOD(GetGUID) (THIS_ char* pGUID, UINT16 len) PURE;
};

// {145CAAA7-7173-4287-8106-9A1573FDA85E}
DEFINE_GUID(IID_IHXProductIdentity3, 
0x145caaa7, 0x7173, 0x4287, 0x81, 0x6, 0x9a, 0x15, 0x73, 0xfd, 0xa8, 0x5e);

DECLARE_INTERFACE_(IHXProductIdentity3, IHXProductIdentity2)
{
    //********************************************************************
    // Method:	IHXProductIdentity3::GetProductValue
    // Purpose:	return the retailer code string of this product
    //********************************************************************
    STDMETHOD(GetProductValue) (THIS_ const char* pProduct, char* pReturnValue, UINT16 len) PURE;
};

// {716D4362-26E0-4beb-92FA-E83EECF5822E}
DEFINE_GUID(CLSID_TheProduct, 
0x716d4362, 0x26e0, 0x4beb, 0x92, 0xfa, 0xe8, 0x3e, 0xec, 0xf5, 0x82, 0x2e);

#endif
