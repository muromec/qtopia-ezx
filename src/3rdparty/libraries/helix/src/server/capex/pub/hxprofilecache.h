/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxprofilecache.h,v 1.4 2004/05/21 19:53:26 jgordon Exp $
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _HXPROFILECACHE_H_
#define _HXPROFILECACHE_H_

#include "hxpssprofile.h"

_INTERFACE IUnknown;
_INTERFACE IHXValues;
_INTERFACE IHXPSSProfileData;
_INTERFACE IHXProfileCache;


DEFINE_GUID_ENUM(IID_IHXProfileCache, 0x00004430, 0x901, 0x11d1,
                 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXProfileCache IID_IHXProfileCache

#undef  INTERFACE
#define INTERFACE IHXProfileCache 

DECLARE_INTERFACE_(IHXProfileCache, IUnknown)
{
    // IUnknown methods
    STDMETHOD (QueryInterface)      (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXProfileCache methods
    STDMETHOD (InitCache)           (THIS_ UINT32 ulMaxSize = 0) PURE;
    STDMETHOD (GetProfile)          (THIS_ const char* szKey,
                                     REF(IHXPSSProfileData*) pProfile,
                                     REF(UINT32) ulMergeRule) PURE;
    STDMETHOD (AddProfile)          (THIS_ const char* szKey,
                                     IHXPSSProfileData* pProfile,
                                     UINT32 ulMergeRule = HX_CP_CMR_NORMAL)
                                     PURE;
    STDMETHOD (RemoveProfile)       (THIS_ const char* szKey) PURE;
};

#endif /* _HXPROFILECACHE_H_ */
