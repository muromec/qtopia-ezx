/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxres.h,v 1.5 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _IHXXRES_H_
#define _IHXXRES_H_



// Includes for this file...
#include "hxcom.h"
#include "hxtypes.h"
#include "hxplugncompat.h"

//
//	CONSTANTS
//

//
//	Used to keep track of the end of the resource.  Useful for debugging as well.
//
#define	  kEndOfResourceMarker		0x52454e44  //'REND'


//
//	Standard resource types.
//
enum 
{
	HX_RT_CURSOR = 1,
	HX_RT_BITMAP,
	HX_RT_ICON,
	HX_RT_MENU,
	HX_RT_DIALOG,
	HX_RT_STRING,
	HX_RT_FONTDIR,
	HX_RT_FONT,
	HX_RT_ACCELERATOR,
	HX_RT_RCDATA,
	HX_RT_MESSAGETABLE,
	HX_RT_GROUPCURSOR=12,
	HX_RT_GROUPICON=14,
	HX_RT_VERSION=16
};


//
//	CLASSES
//

// Forward declarations of some interfaces defined or used here-in.
typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE  IHXXResource		IHXXResource;




/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance
 * 
 *  Purpose:
 * 
 *	Create the instance of the resource manager.
 */
STDAPI HXCreateInstance(IUnknown** /*OUT*/ ppManager);





#undef	    INTERFACE
#define	    INTERFACE	IHXXResFile;

DEFINE_GUID(IID_IHXXResFile, 		0x00000A00, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXXResFile, IUnknown)
{
	// IUnknown methods...
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, void **ppvObj) PURE;
	STDMETHOD_(ULONG32, AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG32, Release)			(THIS) PURE;

	//
	//	IHXXRes methods
	//

	STDMETHOD_(HX_RESULT,Open) 			(THIS_ const char* path) PURE;
	STDMETHOD_(HX_RESULT,Close)			(THIS) PURE;
	
	STDMETHOD_(HX_RESULT,SetLanguage) 		(THIS_ ULONG32 id) PURE;
	STDMETHOD_(HX_RESULT,GetResource)		(THIS_ ULONG32 type, 
							ULONG32 ID, 
							IHXXResource** resource) PURE;

	
	//
	//	Special High level functions for checking if this is a resource.
	//
	STDMETHOD_(IHXXResource*,GetString)		(THIS_ ULONG32 ID) PURE;	
	STDMETHOD_(IHXXResource*,GetBitmap)		(THIS_ ULONG32 ID) PURE;
	STDMETHOD_(IHXXResource*,GetDialog)		(THIS_ ULONG32 ID) PURE;
	STDMETHOD_(IHXXResource*,GetVersionInfo)	(THIS) PURE;
	
	//
	//      Iteration functions
	//
	STDMETHOD(GetFirstResourceLanguage)		(REF(ULONG32)) PURE;
	STDMETHOD(GetNextResourceLanguage)		(REF(ULONG32)) PURE;

	//	Short name support
	STDMETHOD_(HXBOOL, IncludesShortName)				(THIS_ const char* pShortName) PURE;
	
	STDMETHOD_(HX_RESULT,FlushCache) 		(THIS) PURE;
	STDMETHOD_(HX_RESULT,SetCacheLimit)		(THIS_ ULONG32 MaxCachedData) PURE;

    	// if the resource file actually resides as a resource, pass in the file ref of the resource fork.
    	// Macintosh only
	STDMETHOD(UseResourceFile)			(THIS_
	 						INT16   /*IN*/  nResourceFileRef) PURE;
	
};







#undef	    INTERFACE
#define	    INTERFACE	IHXXResource;

DEFINE_GUID(IID_IHXXResource, 		0x00000A01, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXXResource, IUnknown)
{
public:
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj) PURE;
	STDMETHOD_(ULONG32, AddRef) (THIS) PURE;
	STDMETHOD_(ULONG32, Release) (THIS) PURE;

	//
	//	Functions for determining information from a loaded resource.
	//
	STDMETHOD_(ULONG32,ID)			(THIS) PURE;
	
	//
	//	Returns the type of the resource.
	//
	STDMETHOD_(ULONG32,Type)		(THIS) PURE; 
	
	//
	//	Returns the length of the resource data.
	//
	STDMETHOD_(ULONG32,Length)		(THIS) PURE;
	
	//
	//	Return the Language of the resource.
	//
	
	STDMETHOD_(ULONG32,Language)		(THIS) PURE;
	
	//
	//	Returns an interface to the file that this resource was loaded from.
	//
	STDMETHOD_(IHXXResFile*,ResFile) (THIS) PURE;
	
	//
	//	Data accessors
	//

	STDMETHOD_(void* ,ResourceData) (THIS) PURE;

};


#endif // _IHXXRES_H_
