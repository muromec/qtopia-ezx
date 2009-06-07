/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dmofuncs.cpp,v 1.1 2004/08/17 21:32:21 kross Exp $ 
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

#include "stdafx.h"
#include "dllacces.h"

DLLAccessPath * GetDLLAccessPath()
{
    return NULL;
}

static void *_getSymbol(const char *pszSymbol)
{
    static DLLAccess dllAccess;

    if(!dllAccess.isOpen())
    {
	if(dllAccess.open("msdmo.dll") != DLLAccess::DLL_OK)
	    return NULL;
    }
    return dllAccess.getSymbol(pszSymbol);
}


typedef HRESULT (STDAPICALLTYPE *DMOREGISTER)(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
);

STDAPI DMORegister(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
)
{
    static DMOREGISTER fpDMORegister = NULL;
    if(!fpDMORegister)
    {
	fpDMORegister = (DMOREGISTER) _getSymbol("DMORegister");
	if(!fpDMORegister)
	    return E_FAIL;
    }

    return fpDMORegister(szName, clsidDMO, guidCategory, dwFlags, cInTypes, pInTypes, cOutTypes, pOutTypes);
}


typedef HRESULT (STDAPICALLTYPE *DMOUNREGISTER)(REFCLSID clsidDMO, REFGUID guidCategory);

STDAPI DMOUnregister(REFCLSID clsidDMO, REFGUID guidCategory)
{
    static DMOUNREGISTER fpDMOUnregister = NULL;
    if(!fpDMOUnregister)
    {
	fpDMOUnregister = (DMOUNREGISTER) _getSymbol("DMOUnregister");
	if(!fpDMOUnregister)
	    return E_FAIL;
    }

    return fpDMOUnregister(clsidDMO, guidCategory);
}

typedef HRESULT (STDAPICALLTYPE *MOINITMEDIATYPE)(DMO_MEDIA_TYPE *pmt, DWORD cbFormat);
STDAPI MoInitMediaType(DMO_MEDIA_TYPE *pmt, DWORD cbFormat)
{
    static MOINITMEDIATYPE fpMoInitMediaType = NULL;
    if(!fpMoInitMediaType)
    {
	fpMoInitMediaType = (MOINITMEDIATYPE) _getSymbol("MoInitMediaType");
	if(!fpMoInitMediaType)
	    return E_FAIL;
    }

    return fpMoInitMediaType(pmt, cbFormat);
}

typedef HRESULT (STDAPICALLTYPE *MOFREEMEDIATYPE)(DMO_MEDIA_TYPE *pmt);
STDAPI MoFreeMediaType(DMO_MEDIA_TYPE *pmt)
{
    static MOFREEMEDIATYPE fpMoFreeMediaType = NULL;
    if(!fpMoFreeMediaType)
    {
	fpMoFreeMediaType = (MOFREEMEDIATYPE) _getSymbol("MoFreeMediaType");
	if(!fpMoFreeMediaType)
	    return E_FAIL;
    }

    return fpMoFreeMediaType(pmt);
}

typedef HRESULT (STDAPICALLTYPE *MOCOPYMEDIATYPE)(DMO_MEDIA_TYPE *pmtDest, const DMO_MEDIA_TYPE *pmtSrc);
STDAPI MoCopyMediaType(DMO_MEDIA_TYPE *pmtDest, const DMO_MEDIA_TYPE *pmtSrc)
{
    static MOCOPYMEDIATYPE fpMoCopyMediaType = NULL;
    if(!fpMoCopyMediaType)
    {
	fpMoCopyMediaType = (MOCOPYMEDIATYPE) _getSymbol("MoCopyMediaType");
	if(!fpMoCopyMediaType)
	    return E_FAIL;
    }

    return fpMoCopyMediaType(pmtDest, pmtSrc);
}
