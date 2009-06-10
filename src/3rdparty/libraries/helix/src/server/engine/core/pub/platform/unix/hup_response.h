/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hup_response.h,v 1.3 2003/01/23 23:42:55 damonlan Exp $ 
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

#ifndef _HUP_RESPONSE_H
#define _HUP_RESPONSE_H

class HUPResponse : public IHXReconfigServerResponse
{
public:
    HUPResponse();
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /************************************************************************
     * IHXReconfigServerResponse::ReconfigServerDone
     *
     * Purpose:
     *
     *      Notification that reconfiguring the server is done.
     */
    STDMETHOD(ReconfigServerDone)   (THIS_
                                    HX_RESULT res,
                                    IHXBuffer** pInfo,
                                    UINT32 ulNumInfo);
private:
    INT32	m_lRefCount;
};

HUPResponse::HUPResponse()
: m_lRefCount(0)
{
}

STDMETHODIMP_(ULONG32)
HUPResponse::AddRef()
{   
    return InterlockedIncrement(&m_lRefCount);
}   
    
STDMETHODIMP_(ULONG32)
HUPResponse::Release()
{   
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
HUPResponse::QueryInterface(REFIID riid,
				void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXReconfigServerResponse*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXReconfigServerResponse))
    {
	AddRef();
	*ppvObj = (IHXReconfigServerResponse*)this;
	return HXR_OK;
    }
    *ppvObj = 0;
    return HXR_NOINTERFACE;
}

STDMETHODIMP
HUPResponse::ReconfigServerDone(HX_RESULT res,
					    IHXBuffer** pInfo,
					    UINT32 ulNumInfo)
{
    printf("Server reconfigured.\n");
    return HXR_OK;
}

#endif
