/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: activewrap.cpp,v 1.8 2007/07/06 20:39:16 jfinnecy Exp $
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

/****************************************************************************
 * 
 *  activewrap.cpp
 *
 *	Wrapper classes for easy use of ActivePropUser Interface  
 *
 *	Possible TODOs: 
 *	    store list of active property names (set as inactive during destructor)
 *	    add interface to remove active properties
 *	    map active properties to wrapper users (ie multiple users per wrapper)
 *	    let caller set flags about what types of changes are supported, etc
 *
 */

/****************************************************************************
 * Defines
 */


/****************************************************************************
 * Includes
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxerror.h"
#include "hxassert.h"
#include "hxmon.h"
#include "hxslist.h"
#include "hxbuffer.h"
#include "hxstrutl.h"
#include "hxmap.h"

#include "activewrap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

CActivePropWrapper::CActivePropWrapper()
	: m_lRefCount(0)
	, m_pClassFactory(NULL)
	, m_pContext(NULL)
	, m_pUser(NULL)
{
};                             

HX_RESULT
CActivePropWrapper::Init(IUnknown* /*IN*/ pContext, CActivePropWrapperUser * pUser)
{
    if (!(pUser||pContext)) 
    {
	return HXR_FAILED;
    }

    m_pContext = pContext ;
    m_pContext->AddRef();

    m_pUser = pUser;
    m_pUser->AddRef();

    HX_RESULT theErr = HXR_OK;
    
    theErr = m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
	(void**)&m_pClassFactory);
    if (theErr != HXR_OK || m_pClassFactory == NULL)
    {
        theErr = HXR_INVALID_PARAMETER;
        goto bail;
    }

bail:
    return theErr;
};                             


/* Add the registry property pPropName to the list of Active properties
*/

HX_RESULT
CActivePropWrapper::SetAsActive(const char* pPropName)
{
    IHXRegistry* pRegistry = 0;
    IHXActiveRegistry* pActiveReg = 0;

    HX_RESULT theErr = m_pContext->QueryInterface(IID_IHXRegistry, 
	(void**)&pRegistry);
    if (theErr != HXR_OK || pRegistry == NULL)
    {
        theErr = HXR_INVALID_PARAMETER;
        goto bail;
    }

    theErr = pRegistry->QueryInterface(IID_IHXActiveRegistry, 
	(void**) &pActiveReg);
    if (theErr != HXR_OK || pActiveReg == NULL)
    {
        theErr = HXR_INVALID_PARAMETER;
        goto bail;
    }

    theErr = pActiveReg->SetAsActive(pPropName, this);
    
bail:
    HX_RELEASE(pActiveReg);
    HX_RELEASE(pRegistry);

    return theErr ;
}

HX_RESULT
CActivePropWrapper::Done()
{
    if (m_pUser)
    {
        m_pUser->Release();
        m_pUser = NULL;
    }
    return HXR_OK;
}

CActivePropWrapper::~CActivePropWrapper()
{
    HX_RELEASE(m_pContext);
    if (m_pUser)
    {
        m_pUser->Release();
        m_pUser = NULL;
    }
}
    
// IUnknown COM Interface Methods

/****************************************************************************
 *  IUnknown::AddRef                                            ref:  hxcom.h
 *
 *  This routine increases the object reference count in a thread safe
 *  manner. The reference count is used to manage the lifetime of an object.
 *  This method must be explicitly called by the user whenever a new
 *  reference to an object is used.
 */
STDMETHODIMP_(UINT32)
CActivePropWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/****************************************************************************
 *  IUnknown::Release                                           ref:  hxcom.h
 *
 *  This routine decreases the object reference count in a thread safe
 *  manner, and deletes the object if no more references to it exist. It must
 *  be called explicitly by the user whenever an object is no longer needed.
 */
STDMETHODIMP_(UINT32)
CActivePropWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

/****************************************************************************
 *  IUnknown::QueryInterface                                    ref:  hxcom.h
 *
 *  This routine indicates which interfaces this object supports. If a given
 *  interface is supported, the object's reference count is incremented, and
 *  a reference to that interface is returned. Otherwise a NULL object and
 *  error code are returned. This method is called by other objects to
 *  discover the functionality of this object.
 */
STDMETHODIMP
CActivePropWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this },
		{ GET_IIDHANDLE(IID_IHXActivePropUser), (IHXActivePropUser*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
} 

/************************************************************************
* IHXActivePropUser::SetActiveInt
*
*    Async request to set int pName to ul.
*/
STDMETHODIMP
CActivePropWrapper::SetActiveInt(const char* pName,
                        UINT32 ul,
                        IHXActivePropUserResponse* pResponse)
{
    //printf("SetActiveInt: %s = %d\n", pName, ul);
    if (!m_pUser) return HXR_OK;

    CHXString strName(pName);
    CHXString strErr;
    
    HX_RESULT res = m_pUser->PropUpdated(strName, ul, strErr);
    IHXBuffer* pBuf = NULL;
	
    if (! strErr.IsEmpty())
    {
	pBuf = MakeBufString((const char*) strErr);
    }
    pResponse->SetActiveIntDone(res,
	pName, 
	ul,
	pBuf ? &pBuf : 0,
	pBuf ? 1 : 0);

    HX_RELEASE(pBuf);

    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
CActivePropWrapper::SetActiveStr(const char* pName,
                        IHXBuffer* pBuffer,
                        IHXActivePropUserResponse* pResponse)
{
    //printf("SetActiveStr: %s = %s\n", pName, (const char *)pBuffer->GetBuffer());
    if (!m_pUser) return HXR_OK;

    CHXString strName(pName);
    CHXString strVal((const char *)pBuffer->GetBuffer());
    CHXString strErr;

    HX_RESULT res = m_pUser->PropUpdated(strName, strVal, strErr);
    IHXBuffer* pBuf = NULL;
	
    if (! strErr.IsEmpty())
    {
        pBuf = MakeBufString((const char*) strErr);
    }
    
    if ((res == HXR_OK) &&
	 (strVal != (const char *)pBuffer->GetBuffer()))
    {
        pBuffer->Set((const UCHAR *)(const char *)strVal, strVal.GetLength());
    }
	
    pResponse->SetActiveStrDone(res,
	pName, 
	pBuffer, 
	pBuf ? &pBuf : 0,
	pBuf ? 1 : 0);

    HX_RELEASE(pBuf);
	
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveBuf
*
*    Async request to set buffer pName to buffer in pBuffer.
*	 reUse SetActiveStr
*/
STDMETHODIMP
CActivePropWrapper::SetActiveBuf(const char* pName,
                            IHXBuffer* pBuffer,
                            IHXActivePropUserResponse* pResponse)
{
    //printf("SetActiveBuf is Calling ");
    return SetActiveStr(pName, pBuffer, pResponse);
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*       Async request to delete the active property.
*/
STDMETHODIMP
CActivePropWrapper::DeleteActiveProp(const char* pName,
                        IHXActivePropUserResponse* pResponse)
{
    //printf("DeleteActiveProp: %s\n", pName);
    if (!m_pUser) return HXR_OK;

    CHXString strName(pName);
    CHXString strErr;
    
    HX_RESULT res = m_pUser->PropDeleted(strName, strErr);
    IHXBuffer* pBuf = NULL;
	
    if (! strErr.IsEmpty())
    {
        pBuf = MakeBufString((const char*) strErr);
    }
    
    pResponse->DeleteActivePropDone(res,
	pName, 
	pBuf ? &pBuf : 0,
	pBuf ? 1 : 0);

    HX_RELEASE(pBuf);
	
    return HXR_OK;
}


IHXBuffer*
CActivePropWrapper::MakeBufString(const char* pText)
{
    IHXBuffer* pRet;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pRet);
    pRet->Set((const unsigned char*)pText, strlen(pText) + 1);
    return pRet;
}

