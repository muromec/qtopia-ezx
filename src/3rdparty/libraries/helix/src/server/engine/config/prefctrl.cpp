/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: prefctrl.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "prefctrl.h"

PrefController::PrefController()
: m_ulRefCount(0)
{
}

PrefController::~PrefController()
{
}

/*
 * Com stuff.
 */
STDMETHODIMP
PrefController::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXActivePropUser))
    {
	AddRef();
	*ppvObj = (IHXActivePropUser*)this;
	return HXR_OK;
    }
    *ppvObj = 0;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
PrefController::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
PrefController::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }
    delete this;
    return 0;
}

/*
 * IHXActivePropUser stuff.
 */

/************************************************************************
* IHXActivePropUser::SetActiveInt
*
*    Async request to set int pName to ul.
*/
STDMETHODIMP
PrefController::SetActiveInt(const char* pName,
			UINT32 ul,
			IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveIntDone(HXR_FAIL, pName, ul, 0, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
PrefController::SetActiveStr(const char* pName,
			IHXBuffer* pBuffer,
			IHXActivePropUserResponse* pResponse)
{

    pResponse->SetActiveStrDone(HXR_FAIL, pName, pBuffer, 0, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveBuf
*
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
PrefController::SetActiveBuf(const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveBufDone(HXR_FAIL, pName, pBuffer, 0, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
PrefController::DeleteActiveProp(const char* pName,
			    IHXActivePropUserResponse* pResponse)
{
    pResponse->DeleteActivePropDone(HXR_FAIL, pName, 0, 0);
    return HXR_OK;
}
