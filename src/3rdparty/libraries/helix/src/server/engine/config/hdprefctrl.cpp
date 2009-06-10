/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hdprefctrl.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "hxerror.h"
#include "hxresult.h"
#include "hxmon.h"
#include "proc.h"
#include "servreg.h"
#include "hdprefctrl.h"
#include "hxstrutl.h"

/*
 * HTTPDeliverablePrefController stuff.
 */
HTTPDeliverablePrefController::HTTPDeliverablePrefController(Process* p, char* pCharListName, char*** HTTP_paths)
    : m_proc(p)
    , m_pCharListName(pCharListName)
    , m_HTTP_paths(HTTP_paths)
{
    m_proc->pc->registry->SetAsActive(m_pCharListName, this, m_proc);
}

HTTPDeliverablePrefController::~HTTPDeliverablePrefController()
{
}



/*
 * IHXActivePropUser stuff.
 */


/************************************************************************
* IHXActivePropUser::SetActiveStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
HTTPDeliverablePrefController::SetActiveStr(const char* pName,
			IHXBuffer* pBuffer,
			IHXActivePropUserResponse* pResponse)
{

    /*
     * Get everything under HTTPDeliverable.
     */
    IHXValues* pHTTPPath = 0;
    IHXBuffer* pBuf = 0;
    char** pNewHTTPD;
    HX_RESULT res;
    const char* name;
    UINT32 ul;
    int i;

    /*
     * If the list is already in the reg, then we have to merge
     * our new stuff to set with the stuff from the reg.
     */
    if (HXR_OK == m_proc->pc->registry->GetPropList(
	m_pCharListName, pHTTPPath, m_proc))
    {
	UINT32 ulNumPaths = 0;
	res = pHTTPPath->GetFirstPropertyULONG32(name, ul);
	while (res == HXR_OK)
	{
	    ulNumPaths++;
	    res = pHTTPPath->GetNextPropertyULONG32(name, ul);
	}
	/*
	 * If we are changing one that is already there...
	 */
	if (HXR_OK == pHTTPPath->GetPropertyULONG32(
	    pName, ul))
	{
	    pNewHTTPD = new char* [ulNumPaths+1];
	    i = 0;
	    res = pHTTPPath->GetFirstPropertyULONG32(name, ul);
	    while (res == HXR_OK)
	    {
		pBuf = 0;
		/*
		 * If this is the one we are changing...
		 */
		if (!strcasecmp(name, pName))
		{
		    pNewHTTPD[i] = new_string(
			(const char*)pBuffer->GetBuffer());
		}
		else
		{
		    m_proc->pc->registry->GetStr(name, pBuf, m_proc);
		    if (!pBuf)
		    {
			i--;
		    }
		    else
		    {
			pNewHTTPD[i] = new_string(
			    (const char*)pBuf->GetBuffer());
			pBuf->Release();
		    }
		}
		i++;
		res = pHTTPPath->GetNextPropertyULONG32(name, ul);
	    }
	    pNewHTTPD[i] = 0;
	}
	/*
	 * We must be adding one...
	 */
	else
	{
	    pNewHTTPD = new char* [ulNumPaths+2];
	    i = 0;
	    res = pHTTPPath->GetFirstPropertyULONG32(name, ul);
	    while (res == HXR_OK)
	    {
		pBuf = 0;
		m_proc->pc->registry->GetStr(name, pBuf, m_proc);
		if (pBuf)
		{
		    pNewHTTPD[i] = new_string(
			(const char*)pBuf->GetBuffer());
		    pBuf->Release();
		    i++;
		}
		res = pHTTPPath->GetNextPropertyULONG32(name, ul);
	    }
	    pNewHTTPD[i++] = new_string((const char*)pBuffer->GetBuffer());
	    pNewHTTPD[i] = 0;

	}
    }
    /*
     * It was not in the reg, so this is the only one.
     */
    else
    {
	pNewHTTPD = new char* [2];
	pNewHTTPD[0] = new_string((const char*)pBuffer->GetBuffer());
	pNewHTTPD[1] = 0;
    }

    /*
     * Clean up the old one.
     */
    if (m_HTTP_paths && *m_HTTP_paths)
    {
	i = 0;
	while ((*m_HTTP_paths)[i])
	{
	    delete[] (*m_HTTP_paths)[i];
	    i++;
	}
	delete[] (*m_HTTP_paths);
    }
    *m_HTTP_paths = pNewHTTPD;

    pResponse->SetActiveStrDone(HXR_OK, pName, pBuffer, 0, 0);
    return HXR_OK;
}


/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
HTTPDeliverablePrefController::DeleteActiveProp(const char* pName,
			    IHXActivePropUserResponse* pResponse)
{
    IHXValues* pHTTPPath;
    char** pNewHTTPD = 0;
    const char* name;
    UINT32 ul;
    UINT32 ulNumPaths = 0;
    HX_RESULT res;
    int i;
    IHXBuffer* pBuf = 0;
    if (HXR_OK == m_proc->pc->registry->GetPropList(
	m_pCharListName, pHTTPPath, m_proc))
    {
	if (HXR_OK == pHTTPPath->GetPropertyULONG32(pName, ul))
	{
	    /*
	     * Count the number of current paths.
	     */
	    res = pHTTPPath->GetFirstPropertyULONG32(name, ul);
	    while (res == HXR_OK)
	    {
		ulNumPaths++;
		res = pHTTPPath->GetNextPropertyULONG32(name, ul);
	    }

	    pNewHTTPD = new char* [ulNumPaths];
	    i = 0;
	    res = pHTTPPath->GetFirstPropertyULONG32(name, ul);
	    while (res == HXR_OK)
	    {
		pBuf = 0;
		/*
		 * If this is the one we want to delete, then just 
		 * skip over it in the copy.
		 */
		if (strcasecmp(name, pName))
		{
		    m_proc->pc->registry->GetStr(name, pBuf, m_proc);
		    if (pBuf)
		    {
			pNewHTTPD[i] = new_string(
			    (const char*)pBuf->GetBuffer());
			pBuf->Release();
			i++;
		    }
		}
		res = pHTTPPath->GetNextPropertyULONG32(name, ul);
	    }
	    pNewHTTPD[i] = 0;
	    /*
	     * Clean up the old one and set it to the new one.
	     */
	    if (m_HTTP_paths && *m_HTTP_paths)
	    {
		i = 0;
		while ((*m_HTTP_paths)[i])
		{
		    delete[] (*m_HTTP_paths)[i];
		    i++;
		}
		delete[] (*m_HTTP_paths);
	    }
	    *m_HTTP_paths = pNewHTTPD;
	    pNewHTTPD = 0;
	}
    }
    pResponse->DeleteActivePropDone(HXR_OK, pName, 0, 0);
    return HXR_OK;
}
