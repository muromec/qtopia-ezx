/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxhurl.h,v 1.4 2007/07/06 20:43:42 jfinnecy Exp $
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

/*
 *
 * IHXHurl interface : Encapsulates the process of 'hurling'
 * the user to a web page. The URL to hurl to can be a script
 * returing a redirect URL. Data can be attached to the URL
 * before the hurling.
 *
 *
 */


#ifndef _IHXHURL_H
#define _IHXHURL_H

#include "hxcom.h"


// these are common parameter definitions when using SetData()
// ADD ALPHABETICALLY by value, we don't want to duplicate a field name

#define HXHURL_AUTHFAILEDCOMP 	    "AC"
#define HXHURL_AUTHFAILEDVERS 	    "AV"

#define HXHURL_PRIMARYBROWSER	    "BR"
#define HXHURL_PRIMARYBROWSERVER    "BV"

#define HXHURL_CPU 				    "CP"
#define HXHURL_CONNECTSPEED		    "CS"

#define HXHURL_DISTCODE			    "DC"
#define HXHURL_DDRAWVER 		    "DR"

#define HXHURL_EXTRAINFO 		    "EI"
#define HXHURL_ERROR 			    "ER"

#define HXHURL_FREERAM 			    "FM"

#define HXHURL_GUID 			    "GU"

#define HXHURL_INSTCOMPONENTS	    "IC"
#define HXHURL_ITEMID 			    "ID"

#define HXHURL_LANGPREF 		    "LP"
#define HXHURL_PRODLANG 		    "LI"

#define HXHURL_USERERRSTRING	    "MI" // user-supplied error text
#define HXHURL_MOREINFOURL		    "MU" // user-supplied URL 

#define HXHURL_ORIGCODE 		    "OC"
#define HXHURL_OS 				    "OS"
#define HXHURL_OSLANG 			    "OL"

#define HXHURL_PRODNAME 		    "PN"
#define HXHURL_PRODTYPE 		    "PT"
#define HXHURL_PRODVER 			    "PV"
#define HXHURL_PHYSRAM 			    "PR"

#define HXHURL_SNDCARD 			    "SC"
#define HXHURL_SNDCARD_DRVVER	    "SV"
#define HXHURL_PRODSERIAL 		    "SN"

#define HXHURL_USERCODE			    "UC"
#define HXHURL_LASTURL 			    "UR"

#define HXHURL_VIRTRAM 			    "VR"
#define HXHURL_VIDCARD 			    "VC"
#define HXHURL_VIDCARD_DRVVER		"VV"

#define HXHURL_COUNTRY			    "CO"
#define HXHURL_REGION 			    "RGN"


// {A3028581-DB10-11d1-8F39-0060083BE561}
DEFINE_GUID(IID_IHXHurlResponse, 
0xa3028581, 0xdb10, 0x11d1, 0x8f, 0x39, 0x0, 0x60, 0x8, 0x3b, 0xe5, 0x61);

#undef  INTERFACE
#define INTERFACE IHXHurlResponse

DECLARE_INTERFACE_(IHXHurlResponse, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32, Release) (THIS) PURE;

    // Callback to say whether the hurl suceeded or not.
    STDMETHOD(OnHurlDone) (THIS_ HX_RESULT status) PURE;
};


// {B16C0330-B2EC-11d1-8EFD-0060083BE561}
DEFINE_GUID(IID_IHXHurl, 0xb16c0330, 0xb2ec, 0x11d1, 0x8e, 0xfd, 0x0, 0x60, 0x8, 0x3b, 0xe5, 0x61);

#undef  INTERFACE
#define INTERFACE IHXHurl

DECLARE_INTERFACE_(IHXHurl, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32, Release) (THIS) PURE;

	// add name/value pairs to the hurl.
	STDMETHOD(SetData) (THIS_
						const char* szName,
						const char* szValue
						) PURE;

	STDMETHOD(SetDataUINT32) (THIS_
						const char* szName,
						UINT32 nValue
						) PURE;

	// After Setting any additional data you want sent with the
	// hurl, perform the hurl.
	STDMETHOD(Hurl) (THIS) PURE;

	// set the URL to which to hurl  ;-)..
	STDMETHOD(SetUrl) ( THIS_ const char* szUrl ) PURE;
	STDMETHOD(GetUrl) ( THIS_ const char** szUrl) PURE;

	// Get the Hurl ready to do another hurl, cancels any pending
	// hurl that this object is managing.
	STDMETHOD(Reset) (THIS) PURE;

	// Register response object if needed.
	STDMETHOD(SetResponse) ( THIS_ IHXHurlResponse* pIResponse) PURE;

};


// {7F108C4B-A684-4cad-A97A-E3AC2898ABC5}
DEFINE_GUID(IID_IHXHurl2, 0x7f108c4b, 0xa684, 0x4cad, 0xa9, 0x7a, 0xe3, 0xac, 0x28, 0x98, 0xab, 0xc5);

DECLARE_INTERFACE_(IHXHurl2, IHXHurl)
{
    //sets the target window for hurling
    STDMETHOD(SetTarget) ( THIS_ const char* szTarget) PURE;
};

#endif


