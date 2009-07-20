/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cfgreg.cpp,v 1.4 2009/05/14 15:03:37 ehyche Exp $ 
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

#include <stdio.h>
#include <string.h>


#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "ihxpckts.h"
#include "hxmon.h"

#include "watchlst.h"
#include "property.h"
#include "db_dict.h"
#include "commreg.h"
#include "hxpropwclnt.h"
#include "cfgreg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

ConfigRegistry::ConfigRegistry() : 
     m_lRefCount(0)
    , m_pPropDB(NULL)
    , m_pContext(NULL)
{
    m_pPropDB = new CommonRegistry;
}

ConfigRegistry::~ConfigRegistry()
{
//    kill(getpid(), SIGTRAP);
    Close();
}

/* 
 * Users of this class should call Init and the context should
 * expose IHXInterruptState and IHXScheduler if they want to
 * ensure that watches are fired ONLY at non-interrupt time.
 */
void
ConfigRegistry::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    if (m_pPropDB)
    {
        m_pPropDB->Init(pContext);
    }
}

void
ConfigRegistry::Close(void)
{
    HX_DELETE(m_pPropDB);
    HX_RELEASE(m_pContext);
}

/**  IUnknown **/

STDMETHODIMP
ConfigRegistry::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXRegistry))
    {
        AddRef();
        *ppvObj = (IHXRegistry*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXActiveRegistry))
    {
	AddRef();
	*ppvObj = (IHXActiveRegistry*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXActivePropUserResponse))
    {
	AddRef();
	*ppvObj = (IHXActivePropUserResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

STDMETHODIMP_(ULONG32)
ConfigRegistry::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

STDMETHODIMP_(ULONG32)
ConfigRegistry::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

/*************   IHXctiveRegistry ************************/
STDMETHODIMP
ConfigRegistry::SetAsActive(const char* pName,
			    IHXActivePropUser* pUser)
{
    /*
     * Add this new user the list of users for this prop.
     */
    ActivePropUserInfo* p = 0;
    CHXSimpleList* pList = 0;
    if(!m_ActiveMap.Lookup(pName, (void *&)pList))
    {
	pList = new CHXSimpleList;
	m_ActiveMap.SetAt(pName, (void*)pList);
    }

    /*
     * Make sure that this user is not already there.
     */
    CHXSimpleList::Iterator i;
    for (i = pList->Begin(); i != pList->End(); ++i)
    {
	if (((ActivePropUserInfo *)*i)->m_pUser == pUser)
	{
	    return HXR_OK;
	}
    }
    /*
     * Add to list.
     */
    p = new ActivePropUserInfo(pUser);
    pList->AddHead((void*)p);
    return HXR_OK;
}

/************************************************************************
* IHXActiveRegistry::SetAsInactive
*
*	Method to remove an IHXActiveUser from Prop activation.
*/
STDMETHODIMP
ConfigRegistry::SetAsInactive(const char* pName,
			IHXActivePropUser* pUser)
{

    /*
     * Find this user on the list for this prop and
     * remove him.
     */
    ActivePropUserInfo* pCurrent = 0;
    CHXSimpleList* pList = 0;
    if(m_ActiveMap.Lookup(pName, (void*&)pList))
    {
	LISTPOSITION pos;
	if (!pList->IsEmpty())
	{
	    pos = pList->GetHeadPosition();
	    pCurrent = (ActivePropUserInfo*)pList->GetAt(pos);
	    if (pCurrent && pCurrent->m_pUser == pUser)
	    {
		pList->RemoveAt(pos);
		delete pCurrent;
	    };
	    if (pList->IsEmpty())
	    {
		m_ActiveMap.RemoveKey(pName);
		delete pList;
	    }
	}
    }
    return HXR_OK;
}

/************************************************************************
* IHXActiveRegistry::IsActive
*
*     Tells if prop pName has an active user that must be queried to
*   change the value, or if it can just be set.
*/

STDMETHODIMP_(BOOL)
ConfigRegistry::IsActive(const char* pName)
{
    CHXSimpleList* p = 0;
    char* pTemp = new char[strlen(pName) + 1];
    strcpy(pTemp, pName);
    char* pc = pTemp;
    BOOL bEnd = 0;
    while (!bEnd)
    {
	while (*pc && *pc != '.')
	{
	    pc++;
	}
	if (!*pc)
	{
	    bEnd = 1;
	}
	*pc = 0;
	if(m_ActiveMap.Lookup(pTemp, (void*&)p))
	{
	    delete[] pTemp;
	    return 1;
	}
	if (!bEnd)
	{
	    *pc = '.';
	    pc++;
	}
    }
    delete[] pTemp;
    return 0;
}

BOOL
ConfigRegistry::IsDeleteActive(const char* pName)
{
    /*
     * If we are a composite
     */
    HXPropType type;
    BOOL bHadSubComp = 0;
    type = GetTypeByName(pName);
    if (type == PT_COMPOSITE)
    {
	/*
	 * find out if any of out subprops are registered.
	 * If any of our subprops are composites, recurse.
	 */
	IHXValues* pValues = 0;
	GetPropListByName(pName, pValues);
	if (pValues)
	{
	    const char* pSubName;
	    UINT32 ul;
	    HX_RESULT res;
	    res = pValues->GetFirstPropertyULONG32(pSubName, ul);
	    while (res == HXR_OK)
	    {
		/*
		 * If composite, recurse down.
		 */
		type = GetTypeByName(pSubName);
		if (type == PT_COMPOSITE)
		{
		    bHadSubComp = 1;
		    if (IsActive(pSubName))
		    {
			pValues->Release();
			return TRUE;
		    }
		}
		/*
		 * Check for this level (composite or not)
		 */
		CHXSimpleList* p = 0;
		if (m_ActiveMap.Lookup(pSubName, (void*&)p))
		{
		    pValues->Release();
		    return TRUE;
		}
		res = pValues->GetNextPropertyULONG32(pSubName, ul);
	    }
	    pValues->Release();
	}
    }



    /*
     * If this is the deepest composite,
     * find out if any of the base composites of this prop,
     * or this prop itself are registered.
     */
    if (!bHadSubComp)
    {
	CHXSimpleList* p = 0;
	char* pTemp = new char[strlen(pName) + 1];
	strcpy(pTemp, pName);
	char* pc = pTemp;
	BOOL bEnd = 0;
	while (!bEnd)
	{
	    while (*pc && *pc != '.')
	    {
		pc++;
	    }
	    if (!*pc)
	    {
		bEnd = 1;
	    }
	    *pc = 0;
	    if(m_ActiveMap.Lookup(pTemp, (void*&)p))
	    {
		delete[] pTemp;
		return 1;
	    }
	    if (!bEnd)
	    {
		*pc = '.';
		pc++;
	    }
	}
	delete[] pTemp;
    }
    return 0;
}


/*
 * pseudo IHXActivePropUser methods.
 */

/************************************************************************
*    Async request to set int pName to ul.
*/
STDMETHODIMP
ConfigRegistry::SetActiveInt(const char* pName,
			UINT32 ul, 
			IHXActivePropUserResponse* pResponse)
{
    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName))
    {
	if (HXR_OK != m_pPropDB->SetInt(pName, ul))
	{
	    AddInt(pName, ul);
	}
	pResponse->SetActiveIntDone(HXR_OK, pName, ul, 0, 0);
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    
    if (m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
	return HXR_UNEXPECTED;
    }

    CHXSimpleList pListList; //list of apu lists.

    /*
     * _FillListList gives a list of CHXSimpleLists.  Each of the 
     * Lists in the ListList is a List of ActivePropUserInfo*'s
     * for each level in the prop.  a.b.c would have up to 3 lists:
     * the list for a, the list for b, the list for c.
     */
    _FillListList(&pListList, pName);
    CHXSimpleList* pList;
    UINT32 ulNumUsers = 0;
    CHXSimpleList::Iterator i;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	ulNumUsers+= pList->GetCount();
    }

    cb = PendingActiveRegChangesCallback::CreateResponse(
	pResponse,
	PendingActiveRegChangesCallback::PACT_INT,
	ul,
	0,
	pName,
	ulNumUsers);

    m_PendingMap.SetAt(pTemp, (void*)cb);

    /*
     * Send request to each of the users.
     */
    ActivePropUserInfo* pUserInfo = 0;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	CHXSimpleList::Iterator j;
	for (j = pList->Begin(); j != pList->End(); ++j)
	{
	    pUserInfo = (ActivePropUserInfo*)*j;
	    cb = 
		PendingActiveRegChangesCallback::CreateNotify(
		this,
		pUserInfo->m_pUser,
		PendingActiveRegChangesCallback::PACT_INT,
		ul,
		0,
		pName);

	    /*
	     *  XXXDWL  should we schedule this for some time in the future?
	     */
	    cb->Func();
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    return HXR_OK;
}

/************************************************************************
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
ConfigRegistry::SetActiveStr(const char* pName,
			IHXBuffer* pBuffer,
			IHXActivePropUserResponse* pResponse)
{
    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName))
    {
	if (HXR_OK != m_pPropDB->SetStr(pName, pBuffer))
	{
	    AddStr(pName, pBuffer);
	}
	pResponse->SetActiveStrDone(HXR_OK, pName, pBuffer, 0, 0);
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
	return HXR_UNEXPECTED;
    }

    CHXSimpleList pListList; //list of apu lists.

    /*
     * _FillListList gives a list of CHXSimpleLists.  Each of the 
     * Lists in the ListList is a List of ActivePropUserInfo*'s
     * for each level in the prop.  a.b.c would have up to 3 lists:
     * the list for a, the list for b, the list for c.
     */
    _FillListList(&pListList, pName);
    CHXSimpleList* pList;
    UINT32 ulNumUsers = 0;
    CHXSimpleList::Iterator i;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	ulNumUsers+= pList->GetCount();
    }

    cb = PendingActiveRegChangesCallback::CreateResponse(
	pResponse,
	PendingActiveRegChangesCallback::PACT_STR,
	0,
	pBuffer,
	pName,
	ulNumUsers);

    m_PendingMap.SetAt(pTemp, (void*)cb);

    /*
     * Send request to each of the users.
     */
    ActivePropUserInfo* pUserInfo = 0;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	CHXSimpleList::Iterator j;
	for (j = pList->Begin(); j != pList->End(); ++j)
	{
	    pUserInfo = (ActivePropUserInfo*)*j;
	    cb = 
		PendingActiveRegChangesCallback::CreateNotify(
		this,
		pUserInfo->m_pUser,
		PendingActiveRegChangesCallback::PACT_STR,
		0,
		pBuffer,
		pName);
	   
	    /*
	     *  XXXDWL  should we schedule this for some time in the future?
	     */
	    cb->Func();
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    return HXR_OK;
}

/************************************************************************
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
ConfigRegistry::SetActiveBuf(const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse)
{
    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName))
    {
	if (HXR_OK != m_pPropDB->SetBuf(pName, pBuffer))
	{
	    AddBuf(pName, pBuffer);
	}
	//XXXPM check return code.
	pResponse->SetActiveBufDone(HXR_OK, pName, pBuffer, 0, 0);
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
	return HXR_UNEXPECTED;
    }

    CHXSimpleList pListList; //list of apu lists.

    /*
     * _FillListList gives a list of CHXSimpleLists.  Each of the 
     * Lists in the ListList is a List of ActivePropUserInfo*'s
     * for each level in the prop.  a.b.c would have up to 3 lists:
     * the list for a, the list for b, the list for c.
     */
    _FillListList(&pListList, pName);
    CHXSimpleList* pList;
    UINT32 ulNumUsers = 0;
    CHXSimpleList::Iterator i;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	ulNumUsers+= pList->GetCount();
    }

    cb = PendingActiveRegChangesCallback::CreateResponse(
	pResponse,
	PendingActiveRegChangesCallback::PACT_BUF,
	0,
	pBuffer,
	pName,
	ulNumUsers);

    m_PendingMap.SetAt(pTemp, (void*)cb);

    /*
     * Send request to each of the users.
     */
    ActivePropUserInfo* pUserInfo = 0;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	CHXSimpleList::Iterator j;
	for (j = pList->Begin(); j != pList->End(); ++j)
	{
	    pUserInfo = (ActivePropUserInfo*)*j;
	    cb = 
		PendingActiveRegChangesCallback::CreateNotify(
		this,
		pUserInfo->m_pUser,
		PendingActiveRegChangesCallback::PACT_BUF,
		0,
		pBuffer,
		pName);

	    /*
	     *  XXXDWL should we schedule this for some time in the future?
	     */
	    cb->Func();
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
ConfigRegistry::DeleteActiveProp(const char* pName,
			    IHXActivePropUserResponse* pResponse)
{
    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName))
    {
	m_pPropDB->Del(pName);
	//XXXPM check return code
	pResponse->DeleteActivePropDone(HXR_OK, pName, 0, 0);
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
	return HXR_UNEXPECTED;
    }

    CHXSimpleList pListList; //list of apu lists.

    /*
     * _FillListList gives a list of CHXSimpleLists.  Each of the 
     * Lists in the ListList is a List of ActivePropUserInfo*'s
     * for each level in the prop.  a.b.c would have up to 3 lists:
     * the list for a, the list for b, the list for c.
     */
    _FillListList(&pListList, pName);

    /*
     * _AppendListList appends to our list lists of apus for all
     * props in all the subtrees from this prop.  This only happens
     * on delete since delete is the only case where the sub props
     * need to get notified.
     */
    _AppendListList(&pListList, pName);


    CHXSimpleList* pList;
    UINT32 ulNumUsers = 0;
    CHXSimpleList::Iterator i;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	ulNumUsers+= pList->GetCount();
    }

    cb = PendingActiveRegChangesCallback::CreateResponse(
	pResponse,
	PendingActiveRegChangesCallback::PACT_DELETE,
	0,
	0,
	pName,
	ulNumUsers);

    /*
     * Since for this one, we only succeed if they all succeed, start with
     * status of ok.
     */
    cb->m_res = HXR_OK;
    m_PendingMap.SetAt(pTemp, (void*)cb);


    /*
     * Send request to each of the users.
     */
    ActivePropUserInfo* pUserInfo = 0;
    for (i = pListList.Begin(); i != pListList.End(); ++i)
    {
	pList = (CHXSimpleList*)*i;
	CHXSimpleList::Iterator j;
	for (j = pList->Begin(); j != pList->End(); ++j)
	{
	    pUserInfo = (ActivePropUserInfo*)*j;
	    cb = 
		PendingActiveRegChangesCallback::CreateNotify(
		this,
		pUserInfo->m_pUser,
		PendingActiveRegChangesCallback::PACT_DELETE,
		0,
		0,
		pName);

	    /*
	     *  XXDWL should we schedule this for some time in the future? 
	     */
	    cb->Func();
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    return HXR_OK;
}

/*************** IHXActivePropUserResponse *******/

STDMETHODIMP
ConfigRegistry::SetActiveIntDone(HX_RESULT res, const char* pName, UINT32 ul,
				 IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }
    
    /*
     * If any succeeded, then we better set the var.
     */
    if (res == HXR_OK)
    {
	cb->m_res = HXR_OK;
    }
    /*
     * Add any result info.
     */
    cb->AddResInfo(pInfo, ulNumInfo);
    /*
     * decrement number of outstanding sets request.  If this is
     * the last one, then do the set and send the callback.
     */
    cb->m_ulSetsOutstanding--;
    if (cb->m_ulSetsOutstanding)
    {
	delete[] pTemp;
	return HXR_OK;
    }

    m_PendingMap.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	if (HXR_OK != m_pPropDB->SetInt(pName, ul))
	{
	    m_pPropDB->AddInt(pName, ul);
	}
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
    
    //XXXDWL schedule this for some time in the future ?
    cb->Func();
    
    return HXR_OK;

}

STDMETHODIMP
ConfigRegistry::SetActiveStrDone(HX_RESULT res, const char* pName, IHXBuffer* pBuffer,
				 IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }

    /*
     * If any succeeded, then we better set the var.
     */
    if (res == HXR_OK)
    {
	cb->m_res = HXR_OK;
    }
    /*
     * Add any result info.
     */
    cb->AddResInfo(pInfo, ulNumInfo);

    /*
     * decrement number of outstanding sets request.  If this is
     * the last one, then do the set and send the callback.
     */
    cb->m_ulSetsOutstanding--;
    if (cb->m_ulSetsOutstanding)
    {
	delete[] pTemp;
	return HXR_OK;
    }

    m_PendingMap.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	if (HXR_OK != m_pPropDB->SetStr(pName, pBuffer))
	{
	    m_pPropDB->AddStr(pName, pBuffer);
	}
    }

    /*
     * Schedule on the dq.
     */
    delete[] pTemp;

    //XXXDWL schedule this for some time in the future ?
    cb->Func();
    
    return HXR_OK;
}

STDMETHODIMP
ConfigRegistry::SetActiveBufDone(HX_RESULT res, const char* pName, IHXBuffer* pBuffer,
				 IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }

    /*
     * If any succeeded, then we better set the var.
     */
    if (res == HXR_OK)
    {
	cb->m_res = HXR_OK;
    }
    /*
     * Add any result info.
     */
    cb->AddResInfo(pInfo, ulNumInfo);

    /*
     * decrement number of outstanding sets request.  If this is
     * the last one, then do the set and send the callback.
     */
    cb->m_ulSetsOutstanding--;
    if (cb->m_ulSetsOutstanding)
    {
	delete[] pTemp;
	return HXR_OK;
    }

    m_PendingMap.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	if(HXR_OK != m_pPropDB->SetBuf(pName, pBuffer))
	{
	    m_pPropDB->AddBuf(pName, pBuffer);
	}
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;

     //XXXDWL schedule this for some time in the future ?
    cb->Func();
    
    return HXR_OK;
}

STDMETHODIMP
ConfigRegistry::DeleteActivePropDone(HX_RESULT res, const char* pName,
				     IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap.Lookup(pTemp, (void*&)cb))
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }

    /*
     * If any failed, then we better not remove the var.
     */
    if (res != HXR_OK)
    {
	cb->m_res = res;
    }
    /*
     * Add any result info.
     */
    cb->AddResInfo(pInfo, ulNumInfo);

    /*
     * decrement number of outstanding sets request.  If this is
     * the last one, then do the set and send the callback.
     */
    cb->m_ulSetsOutstanding--;
    if (cb->m_ulSetsOutstanding)
    {
	delete[] pTemp;
	return HXR_OK;
    }

    m_PendingMap.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	m_pPropDB->Del(pName);
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
     
    //XXXDWL schedule this for some time in the future ?
    cb->Func();
       
    return HXR_OK;
}


/*************** IHXRegistry *******************/
STDMETHODIMP
ConfigRegistry::CreatePropWatch(IHXPropWatch*& ppObj)
{
    if (!m_pPropDB)
    {
	return HXR_FAIL;
    }

    ppObj = (IHXPropWatch *)new HXClientPropWatch(m_pPropDB, m_pContext);
    if (ppObj)
    {
        ppObj->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

STDMETHODIMP_(UINT32)
ConfigRegistry::AddComp(const char * new_prop)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddComp(new_prop);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::AddInt(const char* new_prop, const INT32 val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddInt(new_prop, val);
}

STDMETHODIMP
ConfigRegistry::GetIntByName(const char* prop_name, INT32& val) const
{
    return m_pPropDB->GetInt(prop_name, &val);
}

STDMETHODIMP
ConfigRegistry::GetIntById(const UINT32 hash_key, INT32& val) const
{
    return m_pPropDB->GetInt(hash_key, &val);
}

STDMETHODIMP
ConfigRegistry::SetIntByName(const char* prop_name, const INT32 val)
{
    return m_pPropDB->SetInt(prop_name, val);
}

STDMETHODIMP
ConfigRegistry::SetIntById(const UINT32 hash_key, const INT32 val)
{
    return m_pPropDB->SetInt(hash_key, val);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::AddStr(const char* new_prop, IHXBuffer* val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddStr(new_prop, val);
}

STDMETHODIMP
ConfigRegistry::GetStrByName(const char* prop_name, REF(IHXBuffer*) val) const
{
    val = NULL;
    return m_pPropDB->GetStr(prop_name, val);
}

STDMETHODIMP
ConfigRegistry::GetStrById(const UINT32 hash_key, REF(IHXBuffer*) val) const
{
    val = NULL;

    return m_pPropDB->GetStr(hash_key, val);
}

STDMETHODIMP
ConfigRegistry::SetStrByName(const char* prop_name, IHXBuffer* val)
{
    return m_pPropDB->SetStr(prop_name, val);
}

STDMETHODIMP
ConfigRegistry::SetStrById(const UINT32 hash_key, IHXBuffer* val)
{
    return m_pPropDB->SetStr(hash_key, val);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::AddBuf(const char* new_prop, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddBuf(new_prop, p_buf);
}

STDMETHODIMP
ConfigRegistry::GetBufByName(const char* prop_name, IHXBuffer*& pp_buf) const
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->GetBuf(prop_name, &pp_buf);
}

STDMETHODIMP
ConfigRegistry::GetBufById(const UINT32 hash_key, IHXBuffer*& pp_buf) const
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->GetBuf(hash_key, &pp_buf);
}

STDMETHODIMP
ConfigRegistry::SetBufByName(const char* prop_name, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->SetBuf(prop_name, p_buf);
}

STDMETHODIMP
ConfigRegistry::SetBufById(const UINT32 hash_key, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->SetBuf(hash_key, p_buf);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::AddIntRef(const char* new_prop, INT32* val)
{
    return m_pPropDB->AddIntRef(new_prop, val);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::DeleteByName(const char* prop_name)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(prop_name);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::DeleteById(const UINT32 hash_key)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(hash_key);
}

STDMETHODIMP_(HXPropType)
ConfigRegistry::GetTypeByName(const char* prop_name) const
{
    return m_pPropDB->GetType(prop_name);
}

STDMETHODIMP_(HXPropType)
ConfigRegistry::GetTypeById(const UINT32 hash_key) const
{
    return m_pPropDB->GetType(hash_key);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::FindParentIdByName(const char* prop_name) const
{
    return m_pPropDB->FindParentKey(prop_name);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::FindParentIdById(const UINT32 hash_key) const
{
    return m_pPropDB->FindParentKey(hash_key);
}

STDMETHODIMP
ConfigRegistry::GetPropName(const UINT32 id, IHXBuffer*& prop_name) const
{
    return m_pPropDB->GetPropName(id, prop_name);
}

STDMETHODIMP_(UINT32)
ConfigRegistry::GetId(const char* prop_name) const
{
    return m_pPropDB->GetId(prop_name);
}

STDMETHODIMP
ConfigRegistry::GetPropListOfRoot(IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(pValues);
}

STDMETHODIMP
ConfigRegistry::GetPropListByName(const char* prop_name, 
                                    IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(prop_name, pValues);
}

STDMETHODIMP
ConfigRegistry::GetPropListById(const UINT32 hash_key,
                                  IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(hash_key, pValues);
}

STDMETHODIMP_(INT32)
ConfigRegistry::GetNumPropsAtRoot() const
{
    return m_pPropDB->Count();
}

STDMETHODIMP_(INT32)
ConfigRegistry::GetNumPropsByName(const char* prop_name) const
{
    return m_pPropDB->Count(prop_name);
}

STDMETHODIMP_(INT32)
ConfigRegistry::GetNumPropsById(const UINT32 hash_key) const
{
    return m_pPropDB->Count(hash_key);
}


/*
 * PendingActiveRegChangesCallback
 */

STDMETHODIMP
ConfigRegistry::PendingActiveRegChangesCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

STDMETHODIMP_(ULONG32)
ConfigRegistry::PendingActiveRegChangesCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

STDMETHODIMP_(ULONG32)
ConfigRegistry::PendingActiveRegChangesCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

STDMETHODIMP
ConfigRegistry::PendingActiveRegChangesCallback::Func()
{
    if(m_PacReason == PACR_RESPONSE)
    {
	switch(m_PacType)
	{
	case PACT_STR:
	    m_pResponse->SetActiveStrDone(m_res,
		m_keyname, m_pPendingBuffer,
		m_res_info,
		m_ulNumResInfo);
	    break;
	    
	case PACT_INT:
	    m_pResponse->SetActiveIntDone(m_res,
		m_keyname, m_ulPendingInt,
		m_res_info,
		m_ulNumResInfo);
	    break;

	case PACT_BUF:
	    m_pResponse->SetActiveBufDone(m_res,
		m_keyname, m_pPendingBuffer,
		m_res_info,
		m_ulNumResInfo);
	    break;

	case PACT_DELETE:
	    m_pResponse->DeleteActivePropDone(m_res,
		m_keyname, m_res_info,
		m_ulNumResInfo);
	    break;
	}
	for (UINT32 i = 0; i < m_ulNumResInfo; i++)
	{
	    m_res_info[i]->Release();
	}
	m_ulNumResInfo = 0;
    }
    else if(m_PacReason == PACR_NOTIFY)
    {
	switch(m_PacType)
	{
	case PACT_STR:
	    m_pUser->SetActiveStr(m_keyname,
		m_pPendingBuffer,
		m_pResponse);
	    break;

	case PACT_INT:
	    m_pUser->SetActiveInt(m_keyname,
		m_ulPendingInt,
		m_pResponse);
	    break;

	case PACT_BUF:
	    m_pUser->SetActiveBuf(m_keyname,
		m_pPendingBuffer,
		m_pResponse);
	    break;

	case PACT_DELETE:
	    m_pUser->DeleteActiveProp(m_keyname,
		m_pResponse);
	    break;
	}
    }

    delete this;
    return HXR_OK;
}

/*
 * Adds lists of apus for all of the super props.
 */
void
ConfigRegistry::_FillListList(CHXSimpleList* pList, const char* pName)
{
    CHXSimpleList* p;
    char* pTemp = new char[strlen(pName) + 1];
    strcpy(pTemp, pName);
    char* pc = pTemp;
    BOOL bEnd = 0;
    while (!bEnd)
    {
	while (*pc && *pc != '.')
	{
	    pc++;
	}
	if (!*pc)
	{
	    bEnd = 1;
	}
	*pc = 0;
	if (m_ActiveMap.Lookup(pTemp, (void*&)p))
	{
	    pList->AddHead((void*)p);
	}
	if (!bEnd)
	{
	    *pc = '.';
	    pc++;
	}
    }
    delete[] pTemp;
    return;
}

/*
 * Adds lists of apus for all of the sub props.
 */
void
ConfigRegistry::_AppendListList(CHXSimpleList* pList, const char* pName)
{
    CHXSimpleList* p;
    HXPropType type;

    /*
     * If this is not a composite or has no sub-props then
     * we can just go home now.
     */
    type = GetTypeByName(pName);
    if (type != PT_COMPOSITE)
    {
	return;
    }

    IHXValues* pValues = 0;
    GetPropListByName(pName, pValues);
    if (!pValues)
    {
	return;
    }

    /*
     * For each of the sub props, get its list, and if it 
     * is a composite, recurse.
     */
    UINT32 ul;
    HX_RESULT res;
    const char* pSubName;
    res = pValues->GetFirstPropertyULONG32(pSubName, ul);
    while (res == HXR_OK)
    {
	type = GetTypeByName(pSubName);
	if (type == PT_COMPOSITE)
	{
	    _AppendListList(pList, pSubName);
	}
	if (m_ActiveMap.Lookup(pSubName, (void*&)p))
	{
	    pList->AddHead((void*)p);
	}
	res = pValues->GetNextPropertyULONG32(pSubName, ul);
    }
    pValues->Release();
}



