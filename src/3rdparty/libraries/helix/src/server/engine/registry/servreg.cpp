/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servreg.cpp,v 1.11 2009/05/30 19:09:56 atin Exp $ 
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
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "hxpropw.h"

#include "debug.h"

#include "dispatchq.h"
#include "proc.h"
#include "mutex.h"

#include "chxpckts.h"

#include "regkey.h"
#include "watchlst.h"
#include "regprop.h"
#include "regdb_misc.h"
#include "regdb_dict.h"
#include "regdb_dict.h"
#include "hxmon.h"
#include "id.h"
#include "mutex.h"
#include "servreg.h"
#include "base_errmsg.h"
#include "hxstrutl.h"
#include "hxslist.h"
#include "regmem.h"
#include "servbuffer.h"


extern BOOL g_bFastMalloc;

ServerRegistry::ServerRegistry(Process* pProc)
    : m_iPropCount(0)
    , m_nLockedBy(-1)
{
    m_pRootDB = new ServRegDB_dict();
    m_pIdTable = new CHXID(1000);

    m_pMutex = HXMutexCreate();
    HXMutexInit(m_pMutex);

    m_pRootWatchList = new WatchList;

    DPRINTF(D_REGISTRY, ("ServerRegistry::ServerRegistry -- m_pRootDB(%p), "
            "m_pIdTable(%p), m_pRootWatchList(%p)\n", m_pRootDB, m_pIdTable, m_pRootWatchList));

    m_ActiveMap2.InitHashTable(256);
    m_ActiveMap2.SetCaseSensitive(0);

    m_PendingMap2.InitHashTable(256);
    m_PendingMap2.SetCaseSensitive(0);
}

// doesn't actually get called since we never release this object
ServerRegistry::~ServerRegistry()
{
    HX_DELETE(m_pRootDB);
    HX_DELETE(m_pIdTable);
    HX_DELETE(m_pRootWatchList);
    HX_ASSERT (m_nLockedBy == 0);
    HXMutexDestroy (m_pMutex);
}


/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	AddComp
 *  Input Params:   	char* szPropName, Process* pProc
 *  Return Value:   	UINT32
 *  Description:
 *  	function for adding PT_COMPOSITE nodes in the log registry.
 *  PT_COMPOSITE nodes can contain other nodes, this is the structure
 *  demanded by the hierarchical storage.
 */
UINT32
ServerRegistry::AddComp(const char* szPropName, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::AddComp(%s, procnum(%pNode))\n", szPropName,
	Process::get_procnum()));
    
    LOCK(Process::get_proc());
    
    /*
     * call AddComp(), which should in turn call AddDone() response 
     * method with the ServRegDB_dict*, ServRegDB_node* pointers
     */
    UINT32 ulId = AddComp(szPropName);
    UNLOCK(Process::get_proc());
    return ulId;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	AddComp
 *  Input Params:   	const char* szPropName
 *  Return Value:   	UINT32
 *  Description:
 *  	function for adding PT_COMPOSITE nodes in the log database.
 *  PT_COMPOSITE nodes can contain other nodes, this is the structure
 *  demanded by the hierarchical storage.
 */
UINT32
ServerRegistry::AddComp(const char* szPropName)
{

    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::AddComp(%s)\n", szPropName));

    HX_RESULT res            = HXR_OK;

    ServRegDB_dict* pTempDB  = m_pRootDB;
    ServRegDB_node* pNewNode = NULL;
    ServRegDB_node* pNode    = NULL;
    ServRegProperty* pProp   = NULL;
    ServRegKey* pKey         = new ServRegKey(szPropName);
    if (pKey && !pKey->is_valid())
    {
	HX_DELETE(pKey);
	return 0;
    }

    int iLen                 = pKey->size();

    NEW_FAST_TEMP_STR(szCurrKeyStr, 256, iLen);
    BOOL bReadOnly = FALSE;

    /*
     * check if all but the last sub-strings are already present
     * eg.
     *     if foo.bar.moo is to be added then
     *        the following loop checks is "foo" and "bar"
     *        have already been created
     *     after the while loop one final check is made to check
     *     if "moo" NOT already present to avoid adding a DUPLICATE
     */
    *szCurrKeyStr = '\0';
    while(!pKey->last_sub_str())
    {
        pKey->append_sub_str(szCurrKeyStr, iLen);
	if (pProp && pProp->get_type() == PT_COMPOSITE)
	{
	    pProp->get_db_val(&pTempDB);
	}

	if (!pTempDB)
	{
	    DPRINTF(D_INFO, ("%s -- %s has NO Properties under it!\n",
		   szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;
	}
	pNode = pTempDB->find(szCurrKeyStr);
	if (!pNode)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s was NOT FOUND\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;
	}
	pProp = pNode->get_data();
        if (!pProp)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;
	}
	bReadOnly = pProp->is_read_only();
	if (bReadOnly)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;
	}
    }
    if (pProp && pProp->get_type() == PT_COMPOSITE)
    {
	pProp->get_db_val(&pTempDB);
    }
    pKey->append_sub_str(szCurrKeyStr, iLen);
    if (pTempDB->find(szCurrKeyStr))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", pKey->get_key_str()));
	res = HXR_FAILED;
	goto cleanup;
    }

    // everything is alright add the new property
    pNewNode = _addComp(pKey, szCurrKeyStr, pTempDB);

    AddDone(pTempDB, pNewNode);

cleanup:

    DELETE_FAST_TEMP_STR(szCurrKeyStr);

    if (HXR_OK != res)
    {
	if (pKey)
	{
	    delete pKey;
	    pKey = NULL;
	}

	return 0;
    }
    else
    {
	return pNewNode->get_id();
    }
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	AddInt
 *  Input Params:   	const char* szPropName, const INT32 iValue, Process* pProc
 *  Return Value:   	UINT32
 *  Description:
 *  	function to add a property which has an integer value, to
 *  the log registry. this will add all the USER DEFINED properties.
 */

UINT32
ServerRegistry::AddInt(const char* szPropName, const INT32 iValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::AddInt(%s, %ld, procnum(%pNode))\n", szPropName, iValue,
	Process::get_procnum()));

    LOCK(Process::get_proc());

    if (IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return 0;
    }
    UINT32 ulId = AddInt(szPropName, iValue);
    UNLOCK(Process::get_proc());
    return ulId;
}

HX_RESULT
ServerRegistry::GetInt(const char* szPropName, INT32* pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetInt(%s)\n", szPropName));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetInt(szPropName, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetInt(const UINT32 ulId, INT32* pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetInt(%lu)\n", ulId));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetInt(ulId, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetInt
 *  Input Params:   	const char* szPropName, const INT32 iValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	sets the value of an integer property.
 */
HX_RESULT
ServerRegistry::SetInt(const char* szPropName, const INT32 iValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::SetInt(%s, %ld)\n", szPropName, iValue));

    LOCK(Process::get_proc());

    if(IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }
    HX_RESULT res = SetInt(szPropName, iValue);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetInt
 *  Input Params:   	const UINT32 ulId, const INT32 iValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set a new value of the INTEGER property using the HashTree.
 */
HX_RESULT
ServerRegistry::SetInt(const UINT32 ulId, const INT32 iValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::SetInt(%lu, %ld)\n", ulId,
            iValue));
            
    LOCK(Process::get_proc());

    IHXBuffer* pBuffer = NULL;
    if (HXR_OK == GetPropName(ulId, pBuffer))
    {
	if (IsActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }

    HX_RESULT res = SetInt(ulId, iValue);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	AddInt
 *  Input Params:   	const char* szPropName, const INT32 iValue
 *  Return value:   	UINT32
 *  Description:
 *  	function to add a property which has an integer value, to
 *  the log database. this will add all the USER DEFINED properties.
 */

UINT32
ServerRegistry::AddInt(const char* szPropName, const INT32 iValue)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::AddInt(%s, %ld)\n", szPropName, iValue));

    HX_RESULT res            = HXR_OK;
    ServRegDB_node* pNode    = NULL;
    ServRegProperty* pProp   = NULL;
    ServRegDB_dict* pTempDB  = m_pRootDB;
    ServRegDB_node* pNewNode = NULL;

    ServRegKey* pKey         = new ServRegKey(szPropName);
    if (pKey && !pKey->is_valid())
    {
	HX_DELETE(pKey);
	return 0;
    }

    int iLen                 = pKey->size();

    NEW_FAST_TEMP_STR(szCurrKeyStr, 256, iLen);

    BOOL bReadOnly = FALSE;

    *szCurrKeyStr = '\0';
    while(!pKey->last_sub_str())
    {
        pKey->append_sub_str(szCurrKeyStr, iLen);

	if (pProp && pProp->get_type() == PT_COMPOSITE)
	{
	    pProp->get_db_val(&pTempDB);
	}

	if (!pTempDB)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;
	}

	pNode = pTempDB->find(szCurrKeyStr);

	if (!pNode)
	{
	    int iRet = _buildSubstructure4Prop(szCurrKeyStr,
		szPropName);

	    if (iRet)
	    {
		pNode = pTempDB->find(szCurrKeyStr);
	    }
	    if (!iRet || !pNode)
	    {
		res = HXR_FAILED;
		goto cleanup;
	    }
	}

	pProp = pNode->get_data();

        if (!pProp)
	{

	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;

	}
	bReadOnly = pProp->is_read_only();

	if (bReadOnly)
	{

	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         szPropName, szCurrKeyStr));
	    res = HXR_FAILED;
	    goto cleanup;

	}
    }
    if (pProp && pProp->get_type() == PT_COMPOSITE)
    {
	pProp->get_db_val(&pTempDB);
    }

    pKey->append_sub_str(szCurrKeyStr, iLen);

    if (pTempDB->find(szCurrKeyStr))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", pKey->get_key_str()));
	res = HXR_FAILED;
	goto cleanup;
    }

    // everything is alright add the new property
    pNewNode = _addInt(pKey, szCurrKeyStr, iValue, pTempDB);

    AddDone(pTempDB, pNewNode);

cleanup:

    DELETE_FAST_TEMP_STR(szCurrKeyStr);

    if (HXR_OK != res)
    {
	if (pKey)
	{
	    delete pKey;
	    pKey = NULL;
	}
	return 0;
    }
    else
    {
	return pNewNode->get_id();
    }
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetInt
 *  Input Params:   	const char* szPropName, INT32* pValue
 *  Return pValue:   	HX_RESULT
 *  Description:
 *  	retrieve the INTEGER property if it exists.
 */
HX_RESULT
ServerRegistry::GetInt(const char* szPropName, INT32* pValue)
{
    ISLOCKED();

    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (FAILED(res = _find(&pNode, &pProp, szPropName)))
    {
	return res;
    }
   
    if (!pProp)
    {
	DPRINTF(D_REGISTRY, ("GetInt(%s) - No property\n",
		             szPropName));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
	case PT_INTEGER:
	    res = pProp->get_int_val(pValue);
	    break;

	case PT_INTREF:
	    res = pProp->get_int_ref_val(pValue);
	    break;

	default:
	    DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
		                 szPropName));
	    return HXR_PROP_TYPE_MISMATCH;
    }

// Failure result codes take precendence over HXR_PROP_DELETE_PENDING.
    if (SUCCEEDED(res) && pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetInt
 *  Input Params:   	const UINT32 ulId, INT32* pValue
 *  Return pValue:   	HX_RESULT
 *  Description:
 *  	retrieve the INTEGER property using the HashTree.
 */
HX_RESULT
ServerRegistry::GetInt(const UINT32 ulId, INT32* pValue)
{
    ISLOCKED();

    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);
    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("GetInt(%lu) failed\n", ulId));
	return HXR_PROP_NOT_FOUND;
    }
    pProp = pNode->get_data();
    if (pProp)
    {
	switch(pProp->get_type())
	{
	    case PT_INTEGER:
		res = pProp->get_int_val(pValue);
		break;

	    case PT_INTREF:
		res = pProp->get_int_ref_val(pValue);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
		                     ulId));
		return HXR_PROP_TYPE_MISMATCH;
	}

    }

// Failure result codes take precendence over HXR_PROP_DELETE_PENDING.
    if (SUCCEEDED(res) && pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetInt
 *  Input Params:   	const char* szPropName, const INT32 value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	sets the value of an integer property.
 */
HX_RESULT
ServerRegistry::SetInt(const char* szPropName, const INT32 iValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) != HXR_OK)
    {
	return HXR_FAIL;
    }

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }

    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- ServRegProperty is READ ONLY\n",
	                     szPropName));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
        case PT_INTEGER:
            pProp->int_val(iValue);
            break;

        case PT_INTREF:
            DPRINTF(D_REGISTRY, ("cannot set INTREF iValue using SetInt\n"));
            return HXR_FAIL;

        default:
            DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
	                         szPropName));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetInt
 *  Input Params:   	const UINT32 ulId, const INT32 iValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set a new value of the INTEGER property using the HashTree.
 */
HX_RESULT
ServerRegistry::SetInt(const UINT32 ulId, const INT32 iValue)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::SetInt(%lu, %ld)\n", ulId,
                         iValue));
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("SetInt(%lu) failed\n", ulId));
	return HXR_FAIL;
    }
    pProp = pNode->get_data();

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }

    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s(%lu) -- ServRegProperty is READ ONLY\n",
	                     pProp->get_key_str(), ulId));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
        case PT_INTEGER:
            pProp->int_val(iValue);
            break;

        case PT_INTREF:
            DPRINTF(D_REGISTRY, ("cannot set INTREF iValue using SetInt\n"));
            return HXR_FAIL;

        default:
            DPRINTF(D_REGISTRY, ("%s(%lu) -- ServRegProperty<-->Type MISMATCH\n",
	                         pProp->get_key_str(), ulId));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}


/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::AddStr
 *  Input Params:   	const char* szPropName, const char* const pValue,
 *                      Process* pProc
 *  Return Value:   	UINT32
 *  Description:
 *  	adds a STRING ServRegProperty ot the registry.
 */
UINT32
ServerRegistry::AddStr(const char* szPropName, IHXBuffer* pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::AddStr(%s, pValue(%pProp), proc(%pProp))\n",
	    szPropName, pValue, Process::get_proc()));

    LOCK(Process::get_proc());

    if (IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return 0;
    }

    UINT32 ulId = AddStr(szPropName, pValue);
    UNLOCK(Process::get_proc());
    return ulId;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetStr
 *  Input Params:   	const char* szPropName, IHXBuffer* const pValue,
 *                      Process* pProc
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING ServRegProperty given a new value and its
 *  CORRECT length.
 */
HX_RESULT
ServerRegistry::SetStr(const char* szPropName, IHXBuffer* pValue, Process* pProc)
{
    LOCK(Process::get_proc());

    if (IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }

    HX_RESULT res = SetStr(szPropName, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetStr(const char* szPropName, REF(IHXBuffer*) pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetStr(%s)\n", szPropName));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetStr(szPropName, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetStr(const UINT32 ulId, REF(IHXBuffer*) pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetStr(%lu)\n", ulId));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetStr(ulId, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetStr
 *  Input Params:   	const UINT32 ulId, IHXBuffer* pValue,
 *                      Process* pProc
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING ServRegProperty given a new value 
 */
HX_RESULT
ServerRegistry::SetStr(const UINT32 ulId, IHXBuffer* pValue, 
                       Process* pProc)
{
    LOCK(Process::get_proc());

    IHXBuffer* pBuffer = NULL;
    if (HXR_OK == GetPropName(ulId, pBuffer))
    {
	if (IsActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }
    HX_RESULT res = SetStr(ulId, pValue);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::AddStr
 *  Input Params:   	const char* szPropName, IHXBuffer* pValue,
 *  Return Value:   	UINT32
 *  Description:
 *  	adds a STRING ServRegProperty ot the registry.
 */
UINT32
ServerRegistry::AddStr(const char* szPropName, IHXBuffer* pValue)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::AddStr(%s, pValue(%pProp)\n",
                         szPropName, pValue));

    ServRegDB_dict* pOwnerDB = m_pRootDB;
    ServRegDB_node* pNode    = NULL;
    ServRegProperty* pProp   = NULL;

    ServRegKey* pKey         = new ServRegKey(szPropName);
    if (pKey && !pKey->is_valid())
    {
	HX_DELETE(pKey);
	return 0;
    }

    int iLen = pKey->size();
    NEW_FAST_TEMP_STR(szCurrKeyStr, 256, iLen);
    BOOL bReadOnly = FALSE;

    *szCurrKeyStr = '\0';
    while(!pKey->last_sub_str())
    {
        pKey->append_sub_str(szCurrKeyStr, iLen);

	if (pProp && pProp->get_type() == PT_COMPOSITE)
	{
	    pProp->get_db_val(&pOwnerDB);
	}

	if (!pOwnerDB)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	            szPropName, szCurrKeyStr));
	    delete pKey;
            DELETE_FAST_TEMP_STR(szCurrKeyStr);
	    return 0;
	}
	pNode = pOwnerDB->find(szCurrKeyStr);
	if (!pNode)
	{
	    if (!_buildSubstructure4Prop(szCurrKeyStr, szPropName))
	    {
		delete pKey;
                DELETE_FAST_TEMP_STR(szCurrKeyStr);
		return 0;
	    }
	    pNode = pOwnerDB->find(szCurrKeyStr);
	    if (!pNode)
	    {
		delete pKey;
                DELETE_FAST_TEMP_STR(szCurrKeyStr);
		return 0;
	    }
	}
	pProp = pNode->get_data();
        if (!pProp)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         szPropName, szCurrKeyStr));
	    delete pKey;
            DELETE_FAST_TEMP_STR(szCurrKeyStr);
	    return 0;
	}
	bReadOnly = pProp->is_read_only();
	if (bReadOnly)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         szPropName, szCurrKeyStr));
	    delete pKey;
            DELETE_FAST_TEMP_STR(szCurrKeyStr);
	    return 0;
	}
    }
    if (pProp && pProp->get_type() == PT_COMPOSITE)
    {
	pProp->get_db_val(&pOwnerDB);
    }
    pKey->append_sub_str(szCurrKeyStr, iLen);
    if (pOwnerDB->find(szCurrKeyStr))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", pKey->get_key_str()));
	delete pKey;
        DELETE_FAST_TEMP_STR(szCurrKeyStr);
	return 0;
    }

    // everything is alright add the new property
    ServRegDB_node* pNewNode = _addBuf(pKey, szCurrKeyStr, pValue, pOwnerDB, PT_STRING);

    DELETE_FAST_TEMP_STR(szCurrKeyStr);

    AddDone(pOwnerDB, pNewNode);
    return pNewNode->get_id();
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetStr
 *  Input Params:   	const char* szPropName, REF(IHXBuffer*) pValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieves a STRING ServRegProperty value from the registry given its
 *  property name.
 */

HX_RESULT
ServerRegistry::GetStr(const char* szPropName, REF(IHXBuffer*) pValue)
{
    ISLOCKED();
    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (FAILED(res = _find(&pNode, &pProp, szPropName)))
    {
	return res;
    }

    if (!pProp)
    {
	DPRINTF(D_REGISTRY, ("GetStr(%s) -- no property!", szPropName));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
	case PT_STRING:
	    res = pProp->get_buf_val(&pValue, PT_STRING);
	    break;

	default:
	    DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
		                 szPropName));
	    return HXR_PROP_TYPE_MISMATCH;
    }

// Failure result codes take precendence over HXR_PROP_DELETE_PENDING.
    if (SUCCEEDED(res) && pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetStr
 *  Input Params:   	const UINT32 ulId, REF(IHXBuffer*) pValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieves a STRING ServRegProperty value from the registry given its
 *  id.
 */
HX_RESULT
ServerRegistry::GetStr(const UINT32 ulId, REF(IHXBuffer*) pValue)
{
    ISLOCKED();
    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("GetStr(%lu) -- no db node\n", ulId));
	return HXR_PROP_NOT_FOUND;
    }

    pProp = pNode->get_data();

    if (!pProp)
    {
	DPRINTF(D_REGISTRY, ("GetStr(%lu) -- no property\n", ulId));
	return HXR_FAIL;
    }


    switch(pProp->get_type())
    {
	case PT_STRING:
	    res = pProp->get_buf_val(&pValue, PT_STRING);
	    break;

	default:
	    DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
		                 ulId));
	    return HXR_PROP_TYPE_MISMATCH;
    }

    if (pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;

}


/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetStr
 *  Input Params:   	const char* szPropName, const IHXBuffer* pValue,
 *                      size_t val_len
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING ServRegProperty given a new value 
 */
HX_RESULT
ServerRegistry::SetStr(const char* szPropName, IHXBuffer* pValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) != HXR_OK)
    {
	return HXR_FAIL;
    }

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }

    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- ServRegProperty is READ ONLY\n",
	                     szPropName));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
        case PT_STRING:
            pProp->buf_val(pValue, PT_STRING);
            break;

        default:
            DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
	                         szPropName));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetStr
 *  Input Params:   	const UINT32 ulId, const IHXBuffer* pValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING ServRegProperty given a new value
 */
HX_RESULT
ServerRegistry::SetStr(const UINT32 ulId, IHXBuffer* pValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("SetStr(%lu) failed - no db node\n", ulId));
	return HXR_FAIL;
    }

    pProp = pNode->get_data();

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }

    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty is READ ONLY\n",
	                     ulId));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
        case PT_STRING:
            pProp->buf_val(pValue, PT_STRING);
            break;

        default:
            DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
	                         ulId));
            return HXR_PROP_TYPE_MISMATCH;
    }

// dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}

UINT32
ServerRegistry::AddBuf(const char* szPropName, IHXBuffer* pBuffer, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::AddBuf(%s, pBuffer(%pProp), proc(%pProp))\n",
	    szPropName, pBuffer, Process::get_proc()));

    LOCK(Process::get_proc());

    if (IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return 0;
    }
    UINT32 ulRet = AddBuf(szPropName, pBuffer);
    UNLOCK(Process::get_proc());
    return ulRet;
}

HX_RESULT
ServerRegistry::GetBuf(const char* szPropName, IHXBuffer** ppBuffer, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetBuf(%s)\n", szPropName));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetBuf(szPropName, ppBuffer);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetBuf(const UINT32 ulId, IHXBuffer** ppBuffer, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetBuf(%lu)\n", ulId));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetBuf(ulId, ppBuffer);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::SetBuf(const char* szPropName, IHXBuffer* pBuffer, Process* pProc)
{
    LOCK(Process::get_proc());

    if(IsActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }
    
    HX_RESULT res = SetBuf(szPropName, pBuffer);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::SetBuf(const UINT32 ulId, IHXBuffer* pBuffer, Process* pProc)
{
    LOCK(Process::get_proc());

    IHXBuffer* pTmpBuf = NULL;
    if (HXR_OK == GetPropName(ulId, pTmpBuf))
    {
	if (IsActive((const char*)pTmpBuf->GetBuffer(), Process::get_proc()))
	{
	    pTmpBuf->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pTmpBuf->Release();
    }
    HX_RESULT res = SetBuf(ulId, pBuffer);
    UNLOCK(Process::get_proc());
    return res;
}

UINT32
ServerRegistry::AddBuf(const char* szPropName, IHXBuffer* pBuffer)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::AddBuf(%s, pBuffer(%pProp)\n",
                         szPropName, pBuffer));

    ServRegDB_dict* pTempDB = m_pRootDB;
    ServRegDB_node* pNode   = NULL;
    ServRegProperty* pProp  = NULL;

    ServRegKey* pKey        = new ServRegKey(szPropName);
    if (pKey && !pKey->is_valid())
    {
	HX_DELETE(pKey);
	return 0;
    }

    int iLen                = pKey->size();

    NEW_FAST_TEMP_STR(pCurrKeyStr, 256, iLen);
    BOOL bReadOnly = FALSE;

    *pCurrKeyStr = '\0';
    while(!pKey->last_sub_str())
    {
        pKey->append_sub_str(pCurrKeyStr, iLen);

	if (pProp && pProp->get_type() == PT_COMPOSITE)
	{
	    pProp->get_db_val(&pTempDB);
	}

	if (!pTempDB)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    delete pKey;
	    return 0;
	}

	pNode = pTempDB->find(pCurrKeyStr);

	if (!pNode)
	{
	    if (!_buildSubstructure4Prop(pCurrKeyStr, szPropName))
	    {
		delete pKey;
                DELETE_FAST_TEMP_STR(pCurrKeyStr);
		return 0;
	    }

	    pNode = pTempDB->find(pCurrKeyStr);

	    if (!pNode)
	    {
		delete pKey;
                DELETE_FAST_TEMP_STR(pCurrKeyStr);
		return 0;
	    }
	}

	pProp = pNode->get_data();

        if (!pProp)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    delete pKey;
	    return 0;
	}

	bReadOnly = pProp->is_read_only();

	if (bReadOnly)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    delete pKey;
	    return 0;
	}
    }

    if (pProp && pProp->get_type() == PT_COMPOSITE)
    {
	pProp->get_db_val(&pTempDB);
    }

    pKey->append_sub_str(pCurrKeyStr, iLen);

    if (pTempDB->find(pCurrKeyStr))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", pKey->get_key_str()));
        DELETE_FAST_TEMP_STR(pCurrKeyStr);
	delete pKey;
	return 0;
    }

    // everything is alright add the new property
    ServRegDB_node* pNewNode = _addBuf(pKey, pCurrKeyStr, pBuffer, pTempDB);

    DELETE_FAST_TEMP_STR(pCurrKeyStr);

    AddDone(pTempDB, pNewNode);
    return pNewNode->get_id();
}

HX_RESULT
ServerRegistry::GetBuf(const char* szPropName, IHXBuffer** ppBuffer)
{
    ISLOCKED();
    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (FAILED(res = _find(&pNode, &pProp, szPropName)))
    {
	return res;
    }

    if (!pProp)
    {
	return HXR_FAIL;
    }

    if (pProp->get_type() == PT_BUFFER)
    {
	res = pProp->get_buf_val(ppBuffer);
    }
    else if (pProp->_alternate_string_access_ok)
    {
	res = pProp->get_buf_val(ppBuffer, PT_STRING);
    }
    else
    {
	DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
			     szPropName));
	return HXR_PROP_TYPE_MISMATCH;
    }

// Failure result codes take precendence over HXR_PROP_DELETE_PENDING.
    if (SUCCEEDED(res) && pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;
}

HX_RESULT
ServerRegistry::GetBuf(const UINT32 ulId, IHXBuffer** ppBuffer)
{
    ISLOCKED();

    HX_RESULT res = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("GetBuf(%lu) failed\n", ulId));
	return HXR_PROP_NOT_FOUND;
    }

    pProp = pNode->get_data();

    if (!pProp)
    {
        return HXR_FAIL;
    }

    if (pProp->get_type() == PT_BUFFER)
    {
	res = pProp->get_buf_val(ppBuffer);
    }
    else if (pProp->_alternate_string_access_ok)
    {
	res = pProp->get_buf_val(ppBuffer, PT_STRING);
    }
    else
    {
	DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
			     ulId));
	return HXR_PROP_TYPE_MISMATCH;
    }

// Failure result codes take precendence over HXR_PROP_DELETE_PENDING.
    if (SUCCEEDED(res) && pProp->is_deleted())
    {
	res = HXR_PROP_DELETE_PENDING;
    }

    return res;
}

HX_RESULT
ServerRegistry::SetBuf(const char* szPropName, IHXBuffer* pBuffer)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) != HXR_OK)
    {
	return HXR_FAIL;
    }

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }
   
    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- ServRegProperty is READ ONLY\n",
	                     szPropName));
	return HXR_FAIL;
    }

    if (pProp->get_type() == PT_BUFFER)
    {
	pProp->buf_val(pBuffer);
    }
    else if (pProp->_alternate_string_access_ok)
    {
	pProp->buf_val(pBuffer, PT_STRING);
    }
    else
    {
	DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
			     szPropName));
	return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}

HX_RESULT
ServerRegistry::SetBuf(const UINT32 ulId, IHXBuffer* pBuffer)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);
    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("GetBuf(%lu) failed\n", ulId));
	return HXR_FAIL;
    }
    pProp = pNode->get_data();

    if (!pProp || pProp->is_deleted())
    {
        return HXR_FAIL;
    }    

    if (pProp->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty is READ ONLY\n",
	                     ulId));
	return HXR_FAIL;
    }

    switch(pProp->get_type())
    {
        case PT_BUFFER:
	    pBuffer->AddRef();
            pProp->buf_val(pBuffer);
	    pBuffer->Release();
            break;

        default:
            DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
	                         ulId));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(pNode, pProp);
}

UINT32
ServerRegistry::AddIntRef(const char* szPropName, INT32* pValue, Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::AddIntRef(%s, %ld)\n", 
            szPropName, *pValue));

    LOCK(Process::get_proc());

    UINT32 ulId = AddIntRef(szPropName, pValue);
    UNLOCK(Process::get_proc());
    return ulId;
}

UINT32
ServerRegistry::AddIntRef(const char* szPropName, INT32* pValue)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("ServerRegistry::AddIntRef(%s, %ld)\n", szPropName,
                         *pValue));
    ServRegDB_dict* pTempDB = m_pRootDB;
    ServRegDB_node* pNode   = NULL;
    ServRegProperty* pProp  = NULL;

    ServRegKey* pKey        = new ServRegKey(szPropName);
    if (pKey && !pKey->is_valid())
    {
	HX_DELETE(pKey);
	return 0;
    }

    int iLen                = pKey->size();

    NEW_FAST_TEMP_STR(pCurrKeyStr, 256, iLen);
    BOOL bReadOnly = FALSE;

    *pCurrKeyStr = '\0';
    while(!pKey->last_sub_str())
    {
        pKey->append_sub_str(pCurrKeyStr, iLen);
	if (pProp && pProp->get_type() == PT_COMPOSITE)
	    pProp->get_db_val(&pTempDB);
	if (!pTempDB)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    return 0;
	}
	pNode = pTempDB->find(pCurrKeyStr);
	if (!pNode)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s was NOT FOUND\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    return 0;
	}
	pProp = pNode->get_data();
        if (!pProp)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    return 0;
	}
	bReadOnly = pProp->is_read_only();
	if (bReadOnly)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         szPropName, pCurrKeyStr));
            DELETE_FAST_TEMP_STR(pCurrKeyStr);
	    return 0;
	}
    }
    if (pProp && pProp->get_type() == PT_COMPOSITE)
    {
	pProp->get_db_val(&pTempDB);
    }

    pKey->append_sub_str(pCurrKeyStr, iLen);

    if (pTempDB->find(pCurrKeyStr))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", pKey->get_key_str()));
        DELETE_FAST_TEMP_STR(pCurrKeyStr);
	return 0;
    }

    // everything is alright add the new property
    ServRegDB_node* pNewNode = _addIntRef(pKey, pCurrKeyStr, pValue, pTempDB);

    DELETE_FAST_TEMP_STR(pCurrKeyStr);

    AddDone(pTempDB, pNewNode);
    return pNewNode->get_id();
}

HX_RESULT
ServerRegistry::SetReadOnly(const char* szPropName, BOOL bValue, Process* pProc)
{
    LOCK(Process::get_proc());

    HX_RESULT res = SetReadOnly(szPropName, bValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::SetReadOnly(UINT32 ulRegId, BOOL bValue, Process* pProc)
{
    LOCK(Process::get_proc());

    HX_RESULT res = SetReadOnly(ulRegId, bValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::SetReadOnly(const char* szPropName, BOOL bValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) != HXR_OK)
    {
	return HXR_FAIL;
    }

    if (!pProp)
    {
        return HXR_FAIL;
    }

    _setReadOnly(pProp, bValue);

    return HXR_OK;
}

HX_RESULT
ServerRegistry::SetReadOnly(UINT32 ulRegId, BOOL bValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulRegId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("SetReadOnly(%lu) failed\n", ulRegId));
	return HXR_FAIL;
    }

    pProp = pNode->get_data();

    if (!pProp)
    {
        return HXR_FAIL;
    }

    _setReadOnly(pProp, bValue);

    return HXR_OK;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	Del
 *  Input Params:   	const char* szPropName
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (ServRegDB_node too) from the registry given
 *  the key string.
 */
UINT32
ServerRegistry::Del(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());

    if (IsDeleteActive(szPropName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return 0;
    }
    UINT32 ulRet = Del(szPropName);
    UNLOCK(Process::get_proc());
    return ulRet;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	Del
 *  Input Params:   	const UINT32 ulId
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (ServRegDB_node too) from the registry given
 *  the key string.
 */
UINT32
ServerRegistry::Del(const UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());

    IHXBuffer* pBuffer;

    if (HXR_OK == GetPropName(ulId, pBuffer))
    {
	if (IsDeleteActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return (UINT32)HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }
    UINT32 ulRet = Del(ulId);
    UNLOCK(Process::get_proc());
    return ulRet;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	Del
 *  Input Params:   	const char* szPropName
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (ServRegDB_node too) from the database given
 *  the key string.
 */
UINT32
ServerRegistry::Del(const char* szPropName)
{
    ISLOCKED();
    ServRegDB_node* pNode     = NULL;
    ServRegProperty* pProp    = NULL;
    ServRegDB_dict* pParentDB = NULL;

    if (_find(&pNode, &pProp, szPropName) != HXR_OK)
    {
	return 0;
    }

    if (!pProp)
    {
	DPRINTF(D_REGISTRY, ("%s -- has NO DATA\n", szPropName));
        return 0;
    }

    // XXXDPS - Currently we do not check if any of the children are read-
    // only before deleting the key. Thus, a user could delete a read-only
    // node by deleting its parent, grandparent, etc. This would slow down
    // the Del() call significantly in the general case because we would
    // have to do a search first, and abort the entire operation if any child
    // is read-only. So it is currently safest to write-protect a key that
    // has no parents (a root key, like "license" or "config").
    if (pProp->is_read_only())
    {
	DPRINTF(D_REGISTRY, ("%s -- is READ ONLY\n", szPropName));
        return 0;
    }

    // find the DB that contains this node
    pParentDB = pNode->get_db();

    if (!pParentDB)
    {
	DPRINTF(D_REGISTRY, ("%s -- has NO IMPLEMENTATION\n", szPropName));
        return 0;
    }

    // here the response method doesn't exactly end this method
    DeleteDone(pParentDB, pNode, pProp);

    UINT32 ulId = pNode->get_id();

    if (pProp->m_lWatchCount)
    {
	/*
	 * Wait for the clear watches to delete the node
	 */
	pProp->set_deleted(pParentDB, pNode, ulId);

	return ulId;
    }

    return _Del(pParentDB, pNode, pProp, ulId);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	Del
 *  Input Params:   	const UINT32 ulId
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (ServRegDB_node too) from the database given
 *  the key string.
 */
UINT32
ServerRegistry::Del(const UINT32 ulId)
{
    ISLOCKED();
    ServRegDB_node* pNode     = NULL;
    ServRegProperty* pProp    = NULL;
    ServRegDB_dict* pParentDB = m_pRootDB;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("%lu -- was NOT FOUND\n", ulId));
	return 0;
    }

    pProp = pNode->get_data();

    if (!pProp)
    {
	DPRINTF(D_REGISTRY, ("%lu -- has NO DATA\n", ulId));
	return 0;
    }

    if (pProp->is_read_only())
    {
	DPRINTF(D_REGISTRY, ("%lu -- is READ ONLY\n", ulId));
        return 0;
    }

    // find the DB that contains this node
    pParentDB = pNode->get_db();

    if (!pParentDB)
    {
	DPRINTF(D_REGISTRY, ("%lu -- has NO IMPLEMENTATION\n", ulId));
        return 0;
    }

    // here the response method doesn't exactly end this method
    DeleteDone(pParentDB, pNode, pProp);

    if (pProp->m_lWatchCount)
    {
	/*
	 * Wait for the clear watches to delete the node
	 */

	pProp->set_deleted(pParentDB, pNode, ulId);

	return ulId;
    }

    return _Del(pParentDB, pNode, pProp, ulId);
}

HXPropType
ServerRegistry::GetType(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());
    HXPropType type = GetType(szPropName);
    UNLOCK(Process::get_proc());
    return type;
}



HXPropType
ServerRegistry::GetType(const UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());
    HXPropType type = GetType(ulId);
    UNLOCK(Process::get_proc());
    return type;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetType
 *  Input Params:   	const char* szPropName
 *  Return Value:   	HXPropType
 *  Description:
 *      returns the Datatype of the ServRegProperty.
 */
HXPropType
ServerRegistry::GetType(const char* szPropName)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) == HXR_OK)
    {
	if (pProp)
	{
	    return pProp->get_type();
	}
    }

    return PT_UNKNOWN;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetType
 *  Input Params:   	const UINT32 ulId
 *  Return Value:   	HXPropType
 *  Description:
 *      returns the Datatype of the ServRegProperty.
 */
HXPropType
ServerRegistry::GetType(const UINT32 ulId)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;
    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);
    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("Get Type (%lu) failed\n", ulId));
	return PT_UNKNOWN;
    }
    pProp = pNode->get_data();

    if (pProp)
    {
	return pProp->get_type();
    }

    return PT_UNKNOWN;
}

HX_RESULT
ServerRegistry::GetPropList(REF(IHXValues*) pValues, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropList(pValues);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetPropList(const char* szPropName, REF(IHXValues*) pValues, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropList(szPropName, pValues);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetPropList(const UINT32 ulId, REF(IHXValues*) pValues, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropList(ulId, pValues);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetPropList const
 *  Input Params:	REF(IHXValues*) pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	it returns back a list of triples one for each property under 
 *  highest level of the database. each of these properties consists of 
 *  name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */

HX_RESULT
ServerRegistry::GetPropList(REF(IHXValues*) pValues)
{
    return _getPropList(m_pRootDB, pValues);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetPropList const
 *  Input Params:   	const char* szPropName, REF(IHXValues*) pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */

HX_RESULT
ServerRegistry::GetPropList(const char* szPropName, REF(IHXValues*) pValues)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) == HXR_OK)
    {
	if (pProp)
	{
	    switch(pProp->get_type())
	    {
		case PT_COMPOSITE:
		{
		    ServRegDB_dict* pChildDB = NULL;

		    pProp->get_db_val(&pChildDB);
		    return _getPropList(pChildDB, pValues);
		}
		default:
		    DPRINTF(D_REGISTRY, ("%s -- is NOT a COMPOSITE property\n", 
		                         szPropName));
		    break;
	    }
	}
    }
    return HXR_FAIL;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetPropList const
 *  Input Params:   	const UINT32 ulId, REF(IHXValues*) pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */

HX_RESULT
ServerRegistry::GetPropList(const UINT32 ulId, REF(IHXValues*) pValues)
{
    ISLOCKED();

    if (!ulId)
    {
	return _getPropList(m_pRootDB, pValues);
    }

    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("could not get PropList for %lu -- no data\n",
	                     ulId));
	return HXR_FAIL;
    }

    pProp = pNode->get_data();

    if (pProp)
    {
	switch(pProp->get_type())
	{
	    case PT_COMPOSITE:
	    {
		ServRegDB_dict* pChildDB = NULL;

		pProp->get_db_val(&pChildDB);
		return _getPropList(pChildDB, pValues);
	    }
	    default:
		DPRINTF(D_REGISTRY, ("%s(%lu) -- is NOT a COMPOSITE property\n",
		                     pProp->get_key_str(), ulId));
		break;
	}
    }
    return HXR_FAIL;
}

HX_RESULT
ServerRegistry::GetChildIdList(const char* szPropName, REF(UINT32*) pChildIds, REF(UINT32) ulCount, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetChildIdList(szPropName, pChildIds, ulCount);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetChildIdList(const UINT32 ulId, REF(UINT32*) pChildIds, REF(UINT32) ulCount, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetChildIdList(ulId, pChildIds, ulCount);
    UNLOCK(Process::get_proc());
    return res;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetChildIdList const
 *  Input Params:   	const char* szPropName, REF(UINT32*) pChildIds
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */

HX_RESULT
ServerRegistry::GetChildIdList(const char* szPropName, REF(UINT32*) pChildIds, REF(UINT32) ulCount)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) == HXR_OK)
    {
	if (pProp)
	{
	    switch(pProp->get_type())
	    {
		case PT_COMPOSITE:
		{
		    ServRegDB_dict* pChildDB = NULL;

		    pProp->get_db_val(&pChildDB);
		    return _getChildIdList(pChildDB, pChildIds, ulCount);
		}
		default:
		    DPRINTF(D_REGISTRY, ("%s -- is NOT a COMPOSITE property\n", 
		                         szPropName));
		    break;
	    }
	}
    }
    return HXR_FAIL;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetChildIdList const
 *  Input Params:   	const UINT32 ulId, REF(UINT32) pChildIds
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */

HX_RESULT
ServerRegistry::GetChildIdList(const UINT32 ulId, REF(UINT32*) pChildIds, REF(UINT32) ulCount)
{
    ISLOCKED();

    if (!ulId)
    {
	return _getChildIdList(m_pRootDB, pChildIds, ulCount);
    }

    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);

    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("could not get PropList for %lu -- no data\n",
	                     ulId));
	return HXR_FAIL;
    }

    pProp = pNode->get_data();

    if (pProp)
    {
	switch(pProp->get_type())
	{
	    case PT_COMPOSITE:
	    {
		ServRegDB_dict* pChildDB = NULL;

		pProp->get_db_val(&pChildDB);
		return _getChildIdList(pChildDB, pChildIds, ulCount);
	    }
	    default:
		DPRINTF(D_REGISTRY, ("%s(%lu) -- is NOT a COMPOSITE property\n",
		                     pProp->get_key_str(), ulId));
		break;
	}
    }
    return HXR_FAIL;
}


HX_RESULT
ServerRegistry::Copy(const char* szFrom, const char* szTo, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = Copy(szFrom, szTo);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::Copy(const char* szFrom, const char* szTo)
{
    ISLOCKED();
    HXPropType type;
    type = GetType(szFrom);
    char szBuf[256];
    INT32 iIntVal          = 0;
    UINT32 ulId            = 0;
    IHXBuffer* pBuffer    = NULL;
    HX_RESULT res          = HXR_OK;
    IHXValues* pValues    = NULL;
    const char* szPropName = NULL;

    switch (type)
    {
    case PT_COMPOSITE:

    // Get all props and recurse copy all of them.

    // The value of res will get replaced below by the return
    // value of recursive Copy() call, if call is made.
        if (AddComp(szTo))
        {
            res = HXR_OK;
        }
        else
        {
            res = HXR_FAIL;
        }

	if (HXR_OK != (res = GetPropList(szFrom, pValues)))
	{
	    break;
	}
	if (HXR_OK == pValues->GetFirstPropertyULONG32(szPropName, ulId))
	{
	    strcpy(szBuf, szTo);
	    strcat(szBuf, szPropName + strlen(szFrom));
	    res = Copy(szPropName, szBuf);
	    while (HXR_OK == pValues->GetNextPropertyULONG32(szPropName, ulId))
	    {
		strcpy(szBuf, szTo);
		strcat(szBuf, szPropName + strlen(szFrom));
		res = Copy(szPropName, szBuf);
	    }
	    HX_RELEASE(pValues);
	}
	break;

    case PT_INTEGER:
	if (HXR_OK != (res = GetInt(szFrom, &iIntVal)))
	{
	    break;
	}
	if (AddInt(szTo, iIntVal))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	break;

    case PT_STRING:
	if (HXR_OK != (res = GetStr(szFrom, pBuffer)))
	{
	    break;
	}
	if (AddStr(szTo, pBuffer))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	HX_RELEASE(pBuffer);
	break;

    case PT_BUFFER:
	if (HXR_OK != (res = GetBuf(szFrom, &pBuffer)))
	{
	    break;
	}
	if (AddBuf(szTo, pBuffer))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	HX_RELEASE(pBuffer);
	break;
    
    default:
	res = HXR_FAIL;
    }

    return res;
}

HX_RESULT
ServerRegistry::SetStringAccessAsBufferById(UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT ret = SetStringAccessAsBufferById(ulId);
    UNLOCK(Process::get_proc());
    return ret;
}

HX_RESULT
ServerRegistry::SetStringAccessAsBufferById(UINT32 hash_key)
{
    ISLOCKED();
    ServRegDB_node*  d = 0;
    ServRegProperty* p = 0;

    d = (ServRegDB_node *)m_pIdTable->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("SetStringAccessAsBufferById(%lu) failed\n",
	    hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_STRING:
		p->SetStringAccessAsBufferById();
		return HXR_OK;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
		                     hash_key));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

UINT32
ServerRegistry::FindParentKey(const char* pName, Process* pProc)
{
    LOCK(Process::get_proc());
    UINT32 ulRet = FindParentKey(pName);
    UNLOCK(Process::get_proc());
    return ulRet;
}

UINT32
ServerRegistry::FindParentKey(const UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());
    UINT32 ulRet = FindParentKey(ulId);
    UNLOCK(Process::get_proc());
    return ulRet;
}

UINT32
ServerRegistry::FindParentKey(const char* prop_name)
{
    ISLOCKED();
    ServRegDB_node* d = 0;
    ServRegProperty* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
    {
	if (d)
	{
	    // find the DB that contains this node
	    ServRegDB_dict* ldb = d->get_db();
	    if (ldb)
	    {
		// find the node that owns this DB
		d = ldb->owner_node();
		if (d)
		    return d->get_id();
	    }
	}
    }
    return 0;
}

UINT32
ServerRegistry::FindParentKey(const UINT32 hash_key)
{
    ISLOCKED();
    ServRegDB_node* d = (ServRegDB_node *)m_pIdTable->get(hash_key);

    // find the node with the hash key
    if (d)
    {
	// find the DB that contains this node
	ServRegDB_dict* ldb = d->get_db();
	if (ldb)
	{
	    // find the node that owns this DB
	    d = ldb->owner_node();
	    if (d)
		return d->get_id();
	}
    }
    return 0;
}

INT32
ServerRegistry::Count(Process* pProc)
{
    LOCK(Process::get_proc());
    INT32 nRet = m_iPropCount;
    UNLOCK(Process::get_proc());
    return nRet;
}

INT32
ServerRegistry::Count(const char* pName, Process* pProc)
{
    LOCK(Process::get_proc());
    INT32 nRet = Count(pName);
    UNLOCK(Process::get_proc());
    return nRet;
}

INT32
ServerRegistry::Count(const UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());
    INT32 nRet = Count(ulId);
    UNLOCK(Process::get_proc());
    return nRet;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::Count
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the number of Properties below the COMPOSITE whose
 *  name has been specified. if the ServRegProperty is not a COMPOSITE then
 *  ZERO is returned.
 */
INT32
ServerRegistry::Count(const char* prop_name)
{
    ISLOCKED();
    ServRegDB_node* d = 0;
    ServRegProperty* p = 0;
    ServRegDB_dict* ldb = 0;
    UINT32 h = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return 0;

    if (p)
    {
	if (p->get_type() == PT_COMPOSITE)
	{
	    p->get_db_val(&ldb);
	    if (ldb)
		return ldb->count();
	}
    }
    return 0;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::Count
 *  Input Params:   	const UINT32 hash_key
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the number of Properties below the COMPOSITE whose
 *  hash key has been specified. if the ServRegProperty is not a COMPOSITE
 *  then ZERO is returned.
 */
INT32
ServerRegistry::Count(const UINT32 hash_key)
{
    ISLOCKED();
    ServRegDB_node* d = 0;
    ServRegProperty* p = 0;
    ServRegDB_dict* ldb = 0;

    d = (ServRegDB_node *)m_pIdTable->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("%lu -- does not exist\n", hash_key));
	return HXR_FAIL;
    }

    p = d->get_data();
    if (p)
    {
	if (p->get_type() == PT_COMPOSITE)
	{
	    p->get_db_val(&ldb);
	    if (ldb)
		return ldb->count();
	}
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2002 RealNetworks
//
//  Function Name:  	ServerRegistry::GetPropStatus
//  Input Params:   	[in] const UINT32 ulId - id of property.
//                      [in] Process* pProc - ptr to process information.
//
//  Return Value:   	HXR_OK if property exists
//                      HXR_PROP_DELETE_PENDING if prop exists, but is in 
//                        process of being deleted.
//                      HXR_FAIL if property does not exist.
//  Description:
//    Queries for the existence/status of a registry property. 
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ServerRegistry::GetPropStatus(const UINT32 ulId, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropStatus(ulId);
    UNLOCK(Process::get_proc());
    return res;
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2002 RealNetworks
//
//  Function Name:  	ServerRegistry::GetPropStatus
//  Input Params:   	[in] const char* szPropName - name of property.
//                      [in] Process* pProc - ptr to process information.
//
//  Return Value:   	HXR_OK if property exists
//                      HXR_PROP_DELETE_PENDING if prop exists, but is in 
//                        process of being deleted.
//                      HXR_FAIL if property does not exist.
//  Description:
//    Queries for the existence/status of a registry property. 
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ServerRegistry::GetPropStatus(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropStatus(szPropName);
    UNLOCK(Process::get_proc());
    return res;
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2002 RealNetworks
//
//  Function Name:  	ServerRegistry::GetPropStatus
//  Input Params:   	[in] const UINT32 ulId - id of property.
//
//  Return Value:   	HXR_OK if property exists
//                      HXR_PROP_DELETE_PENDING if prop exists, but is in 
//                        process of being deleted.
//                      HXR_FAIL if property does not exist.
//  Description:
//    Queries for the existence/status of a registry property. 
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ServerRegistry::GetPropStatus(const UINT32 ulId)
{
    ISLOCKED();
    ServRegDB_node* pNode  = NULL;
    ServRegProperty* pProp = NULL;
    
    pNode = (ServRegDB_node*)m_pIdTable->get(ulId);
    if (!pNode)
    {
	return HXR_PROP_NOT_FOUND;
    }

    pProp = pNode->get_data();
    if (!pProp)
    {
	return HXR_FAIL;
    }

    if (pProp->is_deleted())
    {
	return HXR_PROP_DELETE_PENDING;
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2002 RealNetworks
//
//  Function Name:  	ServerRegistry::GetPropStatus
//  Input Params:   	[in] const char* szPropName - name of property.
//
//  Return Value:   	HXR_OK if property exists
//                      HXR_PROP_DELETE_PENDING if prop exists, but is in 
//                        process of being deleted.
//                      HXR_FAIL if property does not exist.
//  Description:
//    Queries for the existence/status of a registry property. 
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ServerRegistry::GetPropStatus(const char* szPropName)
{
    ISLOCKED();
    HX_RESULT res          = HXR_OK;
    ServRegDB_node* pNode  = NULL;
    ServRegProperty* pProp = NULL;

    if (FAILED(res = _find(&pNode, &pProp, szPropName)))
    {
	return res;
    }
    
    if (!pNode || !pProp)
    {
	return HXR_FAIL;
    }

    if (pProp->is_deleted())
    {
	return HXR_PROP_DELETE_PENDING;
    }

    return HXR_OK;
}


HX_RESULT
ServerRegistry::GetPropName(const UINT32 ulId, REF(IHXBuffer*) pPropName, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT res = GetPropName(ulId, pPropName);
    UNLOCK(Process::get_proc());
    return res;
}


/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::GetPropName const
 *  Input Params:   	const UINT32 ulId, REF(IHXBuffer*) pPropName
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	return a pointer to the key string of the ServRegProperty whose
 *  id is given.
 */
HX_RESULT
ServerRegistry::GetPropName(const UINT32 ulId, REF(IHXBuffer*) pPropName)
{
    ISLOCKED();
    ServRegDB_node* pNode = (ServRegDB_node*)m_pIdTable->get(ulId);
    if (pNode)
    {
	ServRegProperty* pProp = pNode->get_data();
	if (pProp)
	{
	    pPropName = new ServerBuffer();
	    pPropName->Set((const unsigned char*)pProp->get_key_str(), 
			   pProp->get_key_str_len());
	    pPropName->AddRef();
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}

UINT32
ServerRegistry::GetId(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());
    UINT32 ulRet = GetId(szPropName);
    UNLOCK(Process::get_proc());
    return ulRet;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetId
 *  Input Params:   	const char* szPropName
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the ID value for the ServRegProperty name (szPropName) passed
 *  as a parameter. the main CONDITION is that the ServRegProperty MUST EXIST,
 *  otherwise it returns a ZERO (0).
 */
UINT32
ServerRegistry::GetId(const char* szPropName)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY&D_ENTRY, ("ServerRegistry::GetId(%s)\n", szPropName));
    ServRegDB_node* pNode  = NULL;
    ServRegProperty* pProp = NULL;

    if (_find(&pNode, &pProp, szPropName) == HXR_OK)
    {
	if (pNode)
	{
	    return pNode->get_id();
	}
    }
    return 0;
}

UINT32
ServerRegistry::SetWatch(ServerPropWatch* cb)
{
    /*
     * XXXAAK -- need to do some checking to disallow duplicate watchpoints
     */
    LOCK(cb->proc);
    WListElem* wle = new WListElem;
    wle->data = cb;
    m_pRootWatchList->insert(wle);
    UNLOCK(cb->proc);

    return 1;
}

UINT32
ServerRegistry::SetWatch(const char* szName, ServerPropWatch* cb)
{
    LOCK(cb->proc);
    ServRegDB_node*  d = 0;
    ServRegProperty* p = 0;

    if (_find(&d, &p, szName) == HXR_OK)
	if (p)
	{
	    WListElem* wle = new WListElem;
	    wle->data = cb;
	    p->m_pWatchList->insert(wle);
	    DPRINTF(0x00800000, ("SR::SetWatch(%s, %lu) -- "
		    "p->m_pWatchList(%p), watchCount(%ld)\n", szName,
		    p->_id, cb, p->m_lWatchCount));
	    p->m_lWatchCount++;
            UNLOCK(cb->proc);
	    return d->get_id();
	}
    UNLOCK(cb->proc);
    return 0;
}

UINT32
ServerRegistry::SetWatch(const UINT32 hash_key, ServerPropWatch* cb)
{
    LOCK(cb->proc);
    ServRegDB_node*  d = 0;
    ServRegProperty* p = 0;

    d = (ServRegDB_node *)m_pIdTable->get(hash_key);
    if (!d)
    {
        UNLOCK(cb->proc);
	return 0;
    }
    p = d->get_data();

    if (p)
    {
	    WListElem* wle = new WListElem;
	    wle->data = cb;
	    p->m_pWatchList->insert(wle);
	    DPRINTF(0x00800000, ("SR::SetWatch(%s, %lu) -- "
		    "p->m_pWatchList(%p), watchCount(%ld)\n", p->get_key_str(),
		    hash_key, cb, p->m_lWatchCount));
	    p->m_lWatchCount++;
            UNLOCK(cb->proc);
	    return hash_key;
    }
    UNLOCK(cb->proc);
    return 0;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::ClearWatch
 *  Input Params:   	IHXPropWatchResponse* response, Process* pProc
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear a watchpoint from the global watchlist at the root
 *  level of the registry hierarchy.
 */
HX_RESULT
ServerRegistry::ClearWatch(IHXPropWatchResponse* response, Process* pProc)
{
    LOCK(Process::get_proc());
    HX_RESULT ret = _clearWatch(response);
    UNLOCK(Process::get_proc());
    return ret;
}

HX_RESULT
ServerRegistry::ClearWatch(const char* szName, 
			   IHXPropWatchResponse* response,
			   Process* pProc)
{
    ServRegDB_node*  d = 0;
    ServRegProperty* p = 0;

    LOCK(Process::get_proc());

    HX_RESULT ret = _find(&d, &p, szName);
    if (ret == HXR_OK)
        ret = _clearWatch(p, response);
    UNLOCK(Process::get_proc());
    return ret;
}

HX_RESULT
ServerRegistry::ClearWatch(const UINT32 hash_key, 
			   IHXPropWatchResponse* response,
			   Process* pProc)
{
    DPRINTF(D_REGISTRY, ("SR::ClearWatch(%lu)\n", hash_key));
    ServRegDB_node*  d = 0;
    ServRegProperty* p = 0;

    LOCK(Process::get_proc());

    d = (ServRegDB_node *)m_pIdTable->get(hash_key);
    if (!d)
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_NOT_FOUND;
    }

    HX_RESULT ret = _clearWatch(d->get_data(), response);
    UNLOCK(Process::get_proc());
    return ret;
}

HX_RESULT
ServerRegistry::DeleteWatch(ServRegProperty* p, WListElem* wle)
{
    ISLOCKED();
    p->m_pWatchList->removeElem(wle);
    delete wle;
    p->m_lWatchCount--;

    if (p->is_deleted() && !p->m_lWatchCount)
    {
	_Del(p->_owner_db, p->_owner_node, p, p->_id);
    }

    return HXR_OK;
}



    /*
     * IHXRegistry2 methods
     */
HX_RESULT
ServerRegistry::ModifyInt(const char* szName,
                          INT32       nDelta,
                          INT32*      pValue,
                          Process*    pProc)
{
    DPRINTF(D_REGISTRY, ("SR::ModifyInt(%s, %ld)\n", szName, nDelta));

    LOCK(Process::get_proc());

    if(IsActive(szName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }

    UINT32 ulId = GetId(szName);
    HX_RESULT ulRet = GetInt(ulId, pValue);
    if (SUCCEEDED(ulRet))
    {
        *pValue += nDelta;
        ulRet = SetInt(ulId, *pValue);
    }

    UNLOCK(Process::get_proc());
    return ulRet;
}


HX_RESULT
ServerRegistry::ModifyInt(const UINT32 id,
                          INT32        nDelta,
                          INT32*       pValue,
                          Process*     pProc)
{
    DPRINTF(D_REGISTRY, ("SR::ModifyInt(%lu, %ld)\n", id, nDelta));

    LOCK(Process::get_proc());

    IHXBuffer* pBuffer = 0;
    if (HXR_OK == GetPropName(id, pBuffer))
    {
	if (IsActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }

    HX_RESULT ulRet = GetInt(id, pValue);
    if (SUCCEEDED(ulRet))
    {
        *pValue += nDelta;
        ulRet = SetInt(id, *pValue);
    }

    UNLOCK(Process::get_proc());
    return ulRet;
}

HX_RESULT
ServerRegistry::BoundedModifyInt(const char* szName,
			         INT32       nDelta,
			         INT32*      pValue,
			         Process*    pProc,
			         INT32	     nMin,
			         INT32	     nMax)
{
    DPRINTF(D_REGISTRY, ("SR::BoundedModifyInt(%s, %ld, %ld, %ld)\n",
			szName, nDelta, nMin, nMax));

    LOCK(Process::get_proc());

    if(IsActive(szName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }

    UINT32 ulId = GetId(szName);
    HX_RESULT ulRet = GetInt(ulId, pValue);
    HX_RESULT ulRet2 = HXR_OK;
    if (SUCCEEDED(ulRet))
    {
	if (nDelta)
	{
	    INT32 nOrigVal = *pValue;
	    *pValue += nDelta;
	    if (nDelta < 0)
	    {
		// underflow occured
		if (*pValue > nOrigVal)
		{
		    DPRINTF(D_REGISTRY, ("SR::BMIBN(%s, <%ld, %ld> %ld, %ld)"
			" - underflow occured!\n",
			szName, nOrigVal, *pValue, nMin, nMax));
		    *pValue = nMin;
		    ulRet = HXR_PROP_VAL_UNDERFLOW;
		}
		else if (*pValue < nMin)
		{
		    *pValue = nMin;
		    ulRet = HXR_PROP_VAL_LT_LBOUND;
		}
	    }
	    else
	    {
		// overflow occured
		if (*pValue < nOrigVal)
		{
		    DPRINTF(D_REGISTRY, ("SR::BMIBN(%s, <%ld, %ld> %ld, %ld)"
			" - overflow occured!\n",
			szName, nOrigVal, *pValue, nMin, nMax));
		    *pValue = nMax;
		    ulRet = HXR_PROP_VAL_OVERFLOW;
		}
		else if (*pValue > nMax)
		{
		    *pValue = nMax;
		    ulRet = HXR_PROP_VAL_GT_UBOUND;
		}
	    }
	}
        ulRet2 = SetInt(ulId, *pValue);
    }
    else if (!nDelta)
        ulRet2 = SetInt(ulId, *pValue);

    UNLOCK(Process::get_proc());
    return (ulRet2 != HXR_OK ? ulRet2 : ulRet);
}

HX_RESULT
ServerRegistry::BoundedModifyInt(const UINT32 id,
			         INT32        nDelta,
			         INT32*       pValue,
			         Process*     pProc,
			         INT32	      nMin,
			         INT32	      nMax)
{
    DPRINTF(D_REGISTRY, ("SR::BoundedModifyInt(%lu, %ld, %ld, %ld)\n",
			id, nDelta, nMin, nMax));

    LOCK(Process::get_proc());

    IHXBuffer* pBuffer = 0;
    if (HXR_OK == GetPropName(id, pBuffer))
    {
	if (IsActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }

    HX_RESULT ulRet = GetInt(id, pValue);
    HX_RESULT ulRet2 = HXR_OK;
    if (SUCCEEDED(ulRet))
    {
	if (nDelta)
	{
	    INT32 nOrigVal = *pValue;
	    *pValue += nDelta;
	    if (nDelta < 0)
	    {
		// underflow occured
		if (*pValue > nOrigVal)
		{
		    DPRINTF(D_REGISTRY, ("SR::BMIBI(%lu, <%ld, %ld> %ld, %ld)"
			" -- underflow occured!\n",
			id, nOrigVal, *pValue, nMin, nMax));
		    *pValue = nMin;
		    ulRet = HXR_PROP_VAL_UNDERFLOW;
		}
		else if (*pValue < nMin)
		{
		    *pValue = nMin;
		    ulRet = HXR_PROP_VAL_LT_LBOUND;
		}
	    }
	    else
	    {
		// overflow occured
		if (*pValue < nOrigVal)
		{
		    DPRINTF(D_REGISTRY, ("SR::BMIBI(%lu, <%ld, %ld> %ld, %ld)"
			" -- overflow occured!\n",
			id, nOrigVal, *pValue, nMin, nMax));
		    *pValue = nMax;
		    ulRet = HXR_PROP_VAL_OVERFLOW;
		}
		else if (*pValue > nMax)
		{
		    *pValue = nMax;
		    ulRet = HXR_PROP_VAL_GT_UBOUND;
		}
	    }
	}
        ulRet2 = SetInt(id, *pValue);
    }

    UNLOCK(Process::get_proc());
    return (ulRet2 != HXR_OK ? ulRet2 : ulRet);
}

HX_RESULT
ServerRegistry::SetAndReturnInt(const char* szName,
                                INT32       nValue,
                                INT32*      pOldValue,
                                Process*    pProc)
{
    DPRINTF(D_REGISTRY, ("SR::SetAndReturnInt(%s, %ld)\n", szName, nValue));

    LOCK(Process::get_proc());

    if(IsActive(szName, Process::get_proc()))
    {
        UNLOCK(Process::get_proc());
	return HXR_PROP_ACTIVE;
    }

    UINT32 ulId = GetId(szName);
    HX_RESULT ulRet = GetInt(ulId, pOldValue);
    if (SUCCEEDED(ulRet))
    {
        ulRet = SetInt(ulId, nValue);
    }

    UNLOCK(Process::get_proc());
    return ulRet;
}

HX_RESULT
ServerRegistry::SetAndReturnInt(const UINT32 id,
                                INT32        nValue,
                                INT32*       pOldValue,
                                Process*     pProc)
{
    DPRINTF(D_REGISTRY, ("SR::SetAndReturnInt(%lu, %ld)\n", id, nValue));

    LOCK(Process::get_proc());

    IHXBuffer* pBuffer = 0;
    if (HXR_OK == GetPropName(id, pBuffer))
    {
	if (IsActive((const char*)pBuffer->GetBuffer(), Process::get_proc()))
	{
	    pBuffer->Release();
            UNLOCK(Process::get_proc());
	    return HXR_PROP_ACTIVE;
	}
	pBuffer->Release();
    }

    HX_RESULT ulRet = GetInt(id, pOldValue);
    if (SUCCEEDED(ulRet))
    {
        ulRet = SetInt(id, nValue);
    }

    UNLOCK(Process::get_proc());
    return ulRet;
}

HX_RESULT
ServerRegistry::GetIntRef(const char* szPropName,
                          INT32**     ppValue,
                          Process*    pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetIntRef(%s)\n", szPropName));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetIntRef(szPropName, ppValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetIntRef(const char* szPropName,
                          INT32**     ppValue)
{
    ISLOCKED();
    HX_RESULT res          = HXR_OK;
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    if (FAILED(res = _find(&pNode, &pProp, szPropName)))
    {
        return res;
    }

    if (pProp)
    {
        switch(pProp->get_type())
        {
            case PT_INTREF:
                return pProp->get_int_ref(ppValue);
                break;

            default:
                DPRINTF(D_REGISTRY, ("%s -- ServRegProperty<-->Type MISMATCH\n",
                                     szPropName));
                return HXR_PROP_TYPE_MISMATCH;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
ServerRegistry::GetIntRef(const UINT32 ulId,
                          INT32**      ppValue,
                          Process*     pProc)
{
    DPRINTF(D_REGISTRY, ("SR::GetIntRef(%lu)\n", ulId));
    
    LOCK(Process::get_proc());

    HX_RESULT res = GetIntRef(ulId, ppValue);
    UNLOCK(Process::get_proc());
    return res;
}

HX_RESULT
ServerRegistry::GetIntRef(const UINT32 ulId,
                          INT32**      ppValue)
{
    ISLOCKED();
    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;

    pNode = (ServRegDB_node *)m_pIdTable->get(ulId);
    if (!pNode)
    {
	DPRINTF(D_REGISTRY, ("GetIntRef(%lu) failed\n", ulId));
	return HXR_PROP_NOT_FOUND;
    }
    pProp = pNode->get_data();
    if (pProp)
    {
	switch(pProp->get_type())
	{
	    case PT_INTREF:
		return pProp->get_int_ref(ppValue);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- ServRegProperty<-->Type MISMATCH\n",
		                     ulId));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

UINT32
ServerRegistry::AddInt64(const char* szName,
                         const INT64 nValue,
                         Process*    pProc)
{
    return 0;
}

HX_RESULT
ServerRegistry::GetInt64(const char* szName,
                         INT64*      pValue,
                         Process*    pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::GetInt64(const UINT32 ulId,
                         INT64*       pValue,
                         Process*     pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::SetInt64(const char* szName,
                         const INT64 nValue,
                         Process*    pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::SetInt64(const UINT32 id,
                         const INT64  nValue,
                         Process*     pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::ModifyInt64(const char* szName,
                            INT64       nDelta,
                            INT64*      pValue,
                            Process*    pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::ModifyInt64(const UINT32 id,
                            INT64        nDelta,
                            INT64*       pValue,
                            Process*     pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::SetAndReturnInt64(const char* szName,
                                  INT64       nValue,
                                  INT64*      pOldValue,
                                  Process*    pProc)
{
    return HXR_NOTIMPL;
}


HX_RESULT
ServerRegistry::SetAndReturnInt64(const UINT32 id,
                                  INT64        nValue,
                                  INT64*       pOldValue,
                                  Process*     pProc)
{
    return HXR_NOTIMPL;
}

UINT32
ServerRegistry::AddInt64Ref(const char* szName,
                            INT64*      pValue,
                            Process*    pProc)
{
    return 0;
}

HX_RESULT
ServerRegistry::GetInt64Ref(const char* szName,
                            INT64**     ppValue,
                            Process*    pProc)
{
    return HXR_NOTIMPL;
}

HX_RESULT
ServerRegistry::GetInt64Ref(const UINT32 id,
                            INT64**     ppValue,
                            Process*     pProc)
{
    return HXR_NOTIMPL;
}



HX_RESULT
ServerRegistry::AddDone(ServRegDB_dict* pParentDB, ServRegDB_node* pNode)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::AddDone()\n"));
        _dispatchParentCallbacks(pParentDB, pNode, DBE_ADDED);
    return HXR_OK;
}

HX_RESULT
ServerRegistry::SetDone(ServRegDB_node* pNode, ServRegProperty* pProp)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::SetDone()\n"));
        _dispatchCallbacks(pNode, pProp, DBE_MODIFIED);
    return HXR_OK;
}

HX_RESULT
ServerRegistry::DeleteDone(ServRegDB_dict* pParentDB, ServRegDB_node* pNode,
                           ServRegProperty* pProp)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::DeleteDone(db(%pProp), node(%pProp), prop(%pProp))\n",
	pParentDB, pNode, pProp));
        _dispatchCallbacks(pNode, pProp, DBE_DELETED);
        _dispatchParentCallbacks(pParentDB, pNode, DBE_DELETED);
    return HXR_OK;
}

ServRegDB_node*
ServerRegistry::_addComp(ServRegKey* pKey, char* szPropName, ServRegDB_dict* pOwnerDB)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_addComp()\n"));

    ServRegProperty* pNewProp =
        new ServRegProperty(pKey, PT_COMPOSITE);

    ServRegDB_node* pNewNode = pOwnerDB->add(szPropName, pNewProp);

    if (!pNewNode)
    {
	delete pNewProp;
	return 0;
    }

    pNewProp->db_val(new ServRegDB_dict(pNewNode));

    UINT32 ulId = m_pIdTable->create((void *)pNewNode);
    pNewNode->id(ulId);
    m_iPropCount++;

    return pNewNode;
}

ServRegDB_node*
ServerRegistry::_addInt(ServRegKey* pKey, 
			char* szPropName, 
			INT32 iValue, 
			ServRegDB_dict* pOwnerDB)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_addInt()\n"));

    ServRegProperty* pNewProp =
        new ServRegProperty(pKey, PT_INTEGER);
    pNewProp->int_val(iValue);

    ServRegDB_node* pNewNode = pOwnerDB->add(szPropName, pNewProp);
    if (!pNewNode)
    {
	delete pNewProp;
	return 0;
    }

    UINT32 ulId = m_pIdTable->create((void *)pNewNode);
    pNewNode->id(ulId);
    m_iPropCount++;

    return pNewNode;
}

ServRegDB_node*
ServerRegistry::_addBuf(ServRegKey* pKey, 
			char* szPropName, 
			IHXBuffer* pBuffer, 
			ServRegDB_dict* pOwnerDB,
                        HXPropType type)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_addBuf()\n"));

    ServRegProperty* pNewProp =
        new ServRegProperty(pKey, type);
    // AddRef gets called within the buf_val() method
    pNewProp->buf_val(pBuffer, type);

    ServRegDB_node* pNewNode = pOwnerDB->add(szPropName, pNewProp);
    if (!pNewNode)
    {
	delete pNewProp;
	return 0;
    }

    UINT32 ulId = m_pIdTable->create((void *)pNewNode);
    pNewNode->id(ulId);
    m_iPropCount++;

    return pNewNode;
}

ServRegDB_node*
ServerRegistry::_addIntRef(ServRegKey* pKey, 
			   char* szPropName, 
			   INT32* pValue, 
			   ServRegDB_dict* pOwnerDB)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_addIntRef()\n"));

    ServRegProperty* pNewProp =
        new ServRegProperty(pKey, PT_INTREF);
    pNewProp->int_ref_val(pValue);

    ServRegDB_node* pNewNode = pOwnerDB->add(szPropName, pNewProp);
    if (!pNewNode)
    { 
	return 0;
    }

    UINT32 ulId = m_pIdTable->create((void *)pNewNode);
    pNewNode->id(ulId);
    m_iPropCount++;

    return pNewNode;
}

UINT32
ServerRegistry::_Del(ServRegDB_dict* pOwnerDB, ServRegDB_node* pNode, ServRegProperty* pProp, UINT32 ulId)
{
    ISLOCKED();

    if (pProp->get_type() == PT_COMPOSITE)
    {
	ServRegDB_dict* pChildDB;
	pProp->get_db_val(&pChildDB);
	if (_del(pChildDB) == HXR_FAIL)
	{
	    return 0;
	}
    }

    m_pIdTable->destroy(ulId);
    pOwnerDB->del(pNode);

    m_iPropCount--;
    return ulId;
}

HX_RESULT
ServerRegistry::_del(ServRegDB_dict* pOwnerDB)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("_del(pOwnerDB(%p)\n", pOwnerDB));
    ServRegDB_node* pNode = pOwnerDB->first();

    while (pNode)
    {
        ServRegProperty* pProp = pNode->get_data();
	DPRINTF(D_REGISTRY, ("_del(pOwnerDB(%p)) -- %s(%lu)\n", pOwnerDB, 
		pProp->get_key_str(), pNode->get_id()));
	if (pProp)
	{
	    if (pProp->get_type() == PT_COMPOSITE)
	    {
		ServRegDB_dict* pChildDB;
		pProp->get_db_val(&pChildDB);
		if (!pChildDB)
		{
		    DPRINTF(D_REGISTRY, ("invalid ServRegProperty(%lu) not deleted\n",
		                         pNode->get_id()));
		    return HXR_FAIL;
		}
		_del(pChildDB);
	    }
	    // fire off the callbacks if someone is watching this prop
	    DeleteDone(pOwnerDB, pNode, pProp);

	    m_pIdTable->destroy(pNode->get_id());
	    pOwnerDB->del(pNode);
	    
	    m_iPropCount--;
	}
	else
	{
	    DPRINTF(D_REGISTRY, ("data corrputed for ServRegProperty(%lu)\n",
	                         pNode->get_id()));
	    return HXR_FAIL;
	}

	pNode = pOwnerDB->first();
    }
    return HXR_OK;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::_getPropList const
 *  Input Params:   	ServRegDB_dict* pOwnerDB, REF(IHXValues*) pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *    pValues is assigned to an IHXValues containing name-registry id pairs of
 *    all properties in db provided by pOwnerDB. 
 */
HX_RESULT
ServerRegistry::_getPropList(ServRegDB_dict* pOwnerDB, REF(IHXValues*) pValues)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("_getPropList(pOwnerDB(%pProp))\n", pOwnerDB));
    ServRegDB_node* pNode = NULL;

    pValues = new CHXHeader;
    pValues->AddRef();

    pNode = pOwnerDB->first();
    while (pNode)
    {
        ServRegProperty* pProp = pNode->get_data();

	if (!pProp || pProp->is_deleted())
	{
	    DPRINTF(D_REGISTRY, ("%ld is an empty Registry pNode, skipping it\n",
	                         pNode->get_id()));
	}
	else
	{
	    pValues->SetPropertyULONG32(pProp->get_key_str(), pNode->get_id());
	}

	ServRegDB_node* pNextNode = pOwnerDB->next(pNode);
	pNode = pNextNode;
    }
    return HXR_OK;
}

/*
 *  Copyright (c) 2002 RealNetworks
 *
 *  Function Name:  	ServerRegistry::_getChildIdList const
 *  Input Params:   	ServRegDB_dict* pOwnerDB, REF(UINT32*) pChildren
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	return back an array of IDs consisting of
 *  all the properties in the database level specified by "pOwnerDB". 
 */

HX_RESULT
ServerRegistry::_getChildIdList(ServRegDB_dict* pOwnerDB, REF(UINT32*) pChildIds, REF(UINT32) ulCount)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("_getPropList(pOwnerDB(%pProp))\n", pOwnerDB));
    ServRegDB_node*  pNode = NULL;

    ulCount = 0;

    pChildIds = new UINT32[pOwnerDB->count()];
   
    pNode = pOwnerDB->first();
    while (pNode)
    {
        ServRegProperty* pProp = pNode->get_data();

	if (!pProp || pProp->is_deleted())
	{

	// We should never see this.
	    DPRINTF(D_REGISTRY, ("%ld is an empty Registry pNode, skipping it\n",
	                         pNode->get_id()));
	}
	else
	{
	    pChildIds[ulCount++] = pNode->get_id();
	}

	ServRegDB_node* pNextNode = pOwnerDB->next(pNode);
	pNode = pNextNode;
    }
    return HXR_OK;
}


/*
 *  Copyright (c) 1996, 1997, 1998 RealNetworks
 *
 *  Function Name:  	ServerRegistry::_setReadOnly
 *  Input Params:   	ServRegProperty* pProp, BOOL bValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the read_only flag of this node, and all nodes beneath
 *	this node (if this node is a composite) to bValue.
 */
HX_RESULT
ServerRegistry::_setReadOnly(ServRegProperty* pProp, BOOL bValue)
{
    ISLOCKED();
    ServRegDB_dict* pChildDB  = NULL;
    ServRegDB_node* pNode     = NULL;

    // Set read_only flag on pProp
    pProp->set_read_only(bValue);

    if (pProp->get_type() == PT_COMPOSITE)
    {
	// Get pChildDB of pProp
	pProp->get_db_val(&pChildDB);
	if (pChildDB)
	{
	    // Set read_only flag on each child
	    pNode = pChildDB->first();
	    while (pNode)
	    {
		ServRegProperty* pChildProp = pNode->get_data();

		if (!pChildProp)
		{
		    DPRINTF(D_REGISTRY, ("%ld is an empty Registry pNode, "
			"skipping it\n", pNode->get_id()));
		}
		// Recurse...
		_setReadOnly(pChildProp, bValue);

		ServRegDB_node* pNextNode = pChildDB->next(pNode);
		pNode = pNextNode;
	    }
	}
    }

    return HXR_OK;
}

UINT32			
ServerRegistry::_buildSubstructure4Prop(const char* pFailurePoint,
					const char* pProp)
{
    /*
     * A lower composite was not there.
     * Add all of the composites up the to prop.
     */
    ISLOCKED();
    UINT32 len = strlen(pProp) + 1;
    ServRegKey* lame = new ServRegKey(pProp);
    if (lame && !lame->is_valid())
    {
	HX_DELETE(lame);
	return 0;
    }
    NEW_FAST_TEMP_STR(temp_key_str, 256, len);
    *temp_key_str = 0;
    while (strlen(temp_key_str) < strlen(pFailurePoint))
    {
	lame->append_sub_str(temp_key_str, len);
    }
    int ret;
    while ((ret = AddComp(temp_key_str)) != 0)
    {
	if (lame->last_sub_str())
	{
	    break;
	}
	lame->append_sub_str(temp_key_str, len);
    }
    DELETE_FAST_TEMP_STR(temp_key_str);
    delete lame;
    lame = 0;
    return ret;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	_dispatchParentCallbacks
 *  Input Params:   	ServRegDB_dict* db, ServRegDB_node* currNode, ServRegDB_Event e
 *  Return Value:   	void
 *  Description:
 *      fires off the callbacks of the Parent of the
 *  ServRegProperty in "currNode". it is used only when a ServRegProperty gets
 *  added or deleted.
 */
void
ServerRegistry::_dispatchParentCallbacks(ServRegDB_dict* db, ServRegDB_node* currNode, 
                                         ServRegDB_Event e)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_dispatchParentCallback(db(%p), ServRegDB_node(%p), "
            "ServRegDB_event(%d), procnum(%d))\n", db, currNode, e,
            Process::get_procnum()));
    ServRegDB_node*	   parNode = 0;		// parent's node
    ServRegProperty* 	   parProp = 0;		// parent's property
    UINT32 	   par_hash_key = 0;

    if (m_pRootDB == db)
    {
	if (!m_pRootWatchList || m_pRootWatchList->empty())
        {
	    return;
        }

	ServerPropWatch* 	cb;
	UINT32 hash_key = currNode->get_id();

	for (WatchList_iterator wli(m_pRootWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    cb = (ServerPropWatch *)wle->data;
	    DPRINTF(D_REGISTRY, ("SR::_dPC -- m_pRootWatchList(%p)\n", cb));
	    PropWatchCallback* pwcb = new PropWatchCallback;
	    pwcb->m_pPlugin = cb->m_pResponse;
	    pwcb->m_proc = cb->proc;
	    pwcb->m_procnum = cb->procnum;
	    pwcb->m_hash = hash_key;
	    pwcb->m_parentHash = 0;
	    pwcb->m_bParentBeingNotified = TRUE;
	    pwcb->m_event = e;
	    pwcb->m_type = currNode->get_data()->get_type();
	    pwcb->m_registry = this;
            pwcb->m_pKey = 0;
            pwcb->m_pDeletedPropCB = 0;
	    pwcb->m_pPlugin->AddRef();
            if (pwcb->m_type == PT_COMPOSITE && e == DBE_DELETED)
            {   //Used by IHXDeletedProp; for now we only support composite XXX
                if (HXR_OK == pwcb->m_pPlugin->QueryInterface(
                    IID_IHXDeletedPropResponse,
                    (void**)&(pwcb->m_pDeletedPropCB)))
                {
                    GetPropName(hash_key, pwcb->m_pKey);
                }
            }
            DPRINTF(0x00800000, ("SR::_dPC -- 1. pwcb(%p)->procnum(%d)\n", 
		pwcb, pwcb->m_procnum));
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), pwcb, pwcb->m_procnum);
            DPRINTF(0x00800000, 
		("SR::_dPC -- 1. after send(pwcb(%p)->procnum(%d)\n", 
		pwcb, pwcb->m_procnum));
	}
    }
    else
    {
	// find the node that owns this DB
	parNode = db->owner_node();
	if (parNode)
	{
	    par_hash_key = parNode->get_id();
	    // get the parent's ServRegProperty
	    parProp = parNode->get_data();
	    if (!parProp)
	    {
		ERRMSG(Process::get_proc()->pc->error_handler,
		       "%s has an INVALID parent ServRegProperty\n",
		       currNode->get_data()->get_key_str());
		return;
	    }
	}
	else
	{
	    ERRMSG(Process::get_proc()->pc->error_handler,
	           "%s has an INVALID parent ServRegDB_node\n",
		   currNode->get_data()->get_key_str());
	    return;
	}

	if (!parProp->m_pWatchList || parProp->m_pWatchList->empty())
        {
	    return;
        }

	UINT32 hash_key = currNode->get_id();
	for (WatchList_iterator wli(parProp->m_pWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    ServerPropWatch* cb = (ServerPropWatch *)wle->data;
	    DPRINTF(D_REGISTRY, ("SR::_dPC -- parProp(%s)->m_pWatchList(%p)\n",
		    parProp->get_key_str(), cb));
	    PropWatchCallback* pwcb = new PropWatchCallback;
	    pwcb->m_pPlugin = cb->m_pResponse;
	    pwcb->m_proc = cb->proc;
	    pwcb->m_procnum = cb->procnum;
	    pwcb->m_hash = hash_key;
	    pwcb->m_parentHash = par_hash_key;
	    pwcb->m_bParentBeingNotified = TRUE;
	    pwcb->m_event = e;
	    pwcb->m_type = currNode->get_data()->get_type();
	    pwcb->m_registry = this;
            pwcb->m_pKey = 0;
            pwcb->m_pDeletedPropCB = 0;
	    pwcb->m_pPlugin->AddRef();
            if (pwcb->m_type == PT_COMPOSITE && e == DBE_DELETED)
            {   //Used by IHXDeletedProp; for now we only support composite XXX
                if (HXR_OK == pwcb->m_pPlugin->QueryInterface(
                    IID_IHXDeletedPropResponse,
                    (void**)&(pwcb->m_pDeletedPropCB)))
                {
                    GetPropName(hash_key, pwcb->m_pKey);
                }
            }
	    DPRINTF(D_REGISTRY, ("_dPC -- hash(%lu) - par_hash(%lu)\n",
		    hash_key, par_hash_key));
            DPRINTF(0x00800000, ("SR::_dPC -- 2. pwcb(%p)->procnum(%d)\n",
		pwcb, pwcb->m_procnum));
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), pwcb, pwcb->m_procnum);
            DPRINTF(0x00800000,
		("SR::_dpC -- 2. after send(pwcb(%p)->procnum(%d))\n",
		pwcb, pwcb->m_procnum));
	}
    }
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	_dispatchCallbacks
 *  Input Params:   	UINT32 hash_key, ServRegProperty* p, ServRegDB_Event e
 *  Return Value:   	void
 *  Description:
 *  	it fires off the watch callbacks of the ServRegProperty "p" whenever
 *  it gets modified or deleted.
 */
void
ServerRegistry::_dispatchCallbacks(ServRegDB_node* d, ServRegProperty* p, ServRegDB_Event e)
{
    ISLOCKED();
    DPRINTF(0x00800000, ("SR::_dispatchCallbacks(ServRegDB_node(%p), "
	    "Property(%p), ServRegDB_event(%d), procnum(%d))\n", d, p, e,
            Process::get_procnum()));
    if (p->m_lWatchCount <= 0)
    {
	return;
    }

    UINT32 hash_key = d->get_id();
    UINT32 par_id = 0;

    // find the DB that contains this node
    ServRegDB_dict* ldb = d->get_db();
    if (ldb)
    {
	// find the node that owns this DB
	ServRegDB_node* par_node = ldb->owner_node();
	if (par_node)
	    par_id = par_node->get_id();
    }

    for (WatchList_iterator wli(p->m_pWatchList); *wli != 0; ++wli)
    {
	WListElem* wle = *wli;
	ServerPropWatch* cb = (ServerPropWatch *)wle->data;
	DPRINTF(0x00800000, ("SR::_dC -- p(%s)->m_pWatchList(%p)\n",
		p->get_key_str(), cb));
	PropWatchCallback* pwcb = new PropWatchCallback;
	pwcb->m_pPlugin = cb->m_pResponse;
	pwcb->m_proc = cb->proc;
	pwcb->m_procnum = cb->procnum;
	pwcb->m_hash = hash_key;
	pwcb->m_parentHash = par_id;
	pwcb->m_event = e;
	pwcb->m_type = p->get_type();
	pwcb->m_registry = this;
        pwcb->m_pKey = 0;
        pwcb->m_pDeletedPropCB = 0;
        pwcb->m_pPlugin->AddRef();
        if (pwcb->m_type == PT_COMPOSITE && e == DBE_DELETED)
        {   //Used by IHXDeletedProp; for now we only support composite XXX
            if (HXR_OK == pwcb->m_pPlugin->QueryInterface(
                IID_IHXDeletedPropResponse, (void**)&(pwcb->m_pDeletedPropCB)))
            {
                GetPropName(hash_key, pwcb->m_pKey);
            }
        }
        DPRINTF(0x00800000, ("SR::_dC -- pwcb(%p)->procnum(%d)\n", pwcb,
                pwcb->m_procnum));
	Process::get_proc()->pc->dispatchq->send(Process::get_proc(), pwcb, pwcb->m_procnum);
    }
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServerRegistry::_clearWatch
 *  Input Params:   	IHXPropWatchResponse* response
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear watch at the root level of the registry.
 */
HX_RESULT
ServerRegistry::_clearWatch(IHXPropWatchResponse* response)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_clearWatch(resp(%p))\n", response));
    for (WatchList_iterator wli(m_pRootWatchList); *wli != 0; ++wli)
    {
	WListElem* wle = *wli;
	ServerPropWatch* pw = (ServerPropWatch *)wle->data;
	DPRINTF(D_REGISTRY, ("_clearWatch -- m_pRootWatchList(%p)\n", pw));
	if (pw->m_pResponse == response)
	{
	    m_pRootWatchList->removeElem(wle);
	    delete wle;
	    delete pw;
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	_clearWatch
 *  Input Params:   	ServRegProperty* p, IHXPropWatchResponse* response
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clears a watch callback (watchpoint) from a ServRegProperty based on 
 *  the process number. if the ServRegProperty is not specified, then the
 *  watchpoints at the highgest level are cleared.
 */
HX_RESULT
ServerRegistry::_clearWatch(ServRegProperty* p, 
			    IHXPropWatchResponse* response)
{
    ISLOCKED();
    DPRINTF(0x00800000, ("SR::_clearWatch(prop(%p), resp(%p))\n",
	    p, response));
    if (p)
    {
	for (WatchList_iterator wli(p->m_pWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    ServerPropWatch* pw = (ServerPropWatch *)wle->data;
	    DPRINTF(0x00800000, ("SR::_clearWatch -- p->m_pWatchList(%p)\n",
		    pw));
	    if (pw->m_pResponse == response)
	    {
		delete pw;
		HX_RESULT ret = DeleteWatch(p, wle);
		return ret;
	    }
	}
    }
    return HXR_FAIL;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	_find
 *  Input Params:   	ServRegDB_node** ppNode, ServRegProperty** ppProp, 
 *                      const char* szPropName
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	find a ServRegDB_node and its corresponding ServRegProperty based on the name
 *  or the hash key, whichever is given.
 */
HX_RESULT
ServerRegistry::_find(ServRegDB_node** ppNode, ServRegProperty** ppProp, const char* szPropName)
{
    ISLOCKED();
    DPRINTF(D_REGISTRY, ("SR::_find(szPropName(%s))\n", szPropName));

    ServRegKey key(szPropName);
    if (!key.is_valid())
	return 0;

    int iLen = key.size();
    NEW_FAST_TEMP_STR(szCurrKeyStr, 256, iLen);
    ServRegDB_dict* pTempDB = m_pRootDB;
    HX_RESULT res = HXR_OK;

    // find the node that contains the "szPropName"
    *szCurrKeyStr = '\0';
    while(key.append_sub_str(szCurrKeyStr, iLen))
    {
	if (!pTempDB)
	{
	    DPRINTF(D_REGISTRY, ("searching for %s -- "
		    "%s has NO Properties under it!\n",
		    szPropName, szCurrKeyStr));
	    res = HXR_FAIL;
	    goto cleanup;
	}
	*ppNode = pTempDB->find(szCurrKeyStr);
	if (!(*ppNode))
	{
	    DPRINTF(D_REGISTRY, ("searching for %s -- %s was NOT FOUND\n",
		    szPropName, szCurrKeyStr));
	    res = HXR_PROP_NOT_FOUND;
	    goto cleanup;
	}

	*ppProp = (*ppNode)->get_data();
        if (!(*ppProp))
	{
	    DPRINTF(D_REGISTRY, ("searching for %s -- %s has NO DATA\n",
		    szPropName, szCurrKeyStr));
            res = HXR_FAIL;
	    goto cleanup;
	}
	if ((*ppProp)->get_type() == PT_COMPOSITE)
        {
	    (*ppProp)->get_db_val(&pTempDB);
        }
    }

    if (*ppNode && *ppProp)
    {
	res = HXR_OK;
    }
    else
    {
	res = HXR_FAIL;
    }

cleanup:
    DELETE_FAST_TEMP_STR(szCurrKeyStr);
    return res;
}

/*
 * Com stuff.  There is only one of these objects and he should
 * never die, so forget about ref counting.
 */
STDMETHODIMP_(ULONG32)
ServerRegistry::AddRef()
{
    return 0xffffffff;
}   

STDMETHODIMP_(ULONG32)
ServerRegistry::Release()
{
    return 0xffffffff;
}

STDMETHODIMP
ServerRegistry::QueryInterface(REFIID riid, void** ppvObj)
{
    ISLOCKED();
    HX_RESULT ret = HXR_OK;

    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXActivePropUserResponse*)this;
	ret = HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXActivePropUserResponse))
    {
	AddRef();
	*ppvObj = (IHXActivePropUserResponse*)this;
	ret = HXR_OK;
    }
    else
    {
        *ppvObj = 0;
        ret = HXR_NOINTERFACE;
    }

    return ret;

}


/*
 * IHXActiveRegistry stuff.
 */

/************************************************************************
* IHXActiveRegistry::SetAsActive
*
*     Method to set prop pName to active and register pUser as
*   the active prop user.
*/
STDMETHODIMP
ServerRegistry::SetAsActive(const char* pName,
			IHXActivePropUser* pUser,
			Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * Add this new user the list of users for this prop.
     */
    ActivePropUserInfo* p = 0;
    CHXSimpleList* pList = 0;
    if(!m_ActiveMap2.Lookup(pName, (void *&)pList))
    {
	pList = new CHXSimpleList;
	m_ActiveMap2.SetAt(pName, (void*)pList);
    }

    /*
     * Make sure that this user is not already there.
     */
    CHXSimpleList::Iterator i;
    for (i = pList->Begin(); i != pList->End(); ++i)
    {
	if (((ActivePropUserInfo *)*i)->m_pUser == pUser)
	{
            UNLOCK(Process::get_proc());
	    return HXR_OK;
	}
    }
    /*
     * Add to list.
     */
    p = new ActivePropUserInfo(pUser, Process::get_proc());
    pList->AddHead((void*)p);
    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/************************************************************************
* IHXActiveRegistry::SetAsInactive
*
*	Method to remove an IHXActiveUser from Prop activation.
*/
STDMETHODIMP
ServerRegistry::SetAsInactive(const char* pName,
			      IHXActivePropUser* pUser,
                              Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * Find this user on the list for this prop and
     * remove him.
     */
    ActivePropUserInfo* pCurrent = 0;
    CHXSimpleList* pList = 0;
    if(m_ActiveMap2.Lookup(pName, (void*&)pList))
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
		m_ActiveMap2.RemoveKey(pName);
		delete pList;
	    }
	}
    }
    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/************************************************************************
* IHXActiveRegistry::IsActive
*
*     Tells if prop szPropName has an active user that must be queried to
*   change the value, or if it can just be set.
*/

STDMETHODIMP_(BOOL)
ServerRegistry::IsActive(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());

    CHXSimpleList* pList = NULL;

    BOOL bEnd            = FALSE;

    NEW_FAST_TEMP_STR(szNameTemp, 256, strlen(szPropName) + 1);
    strcpy(szNameTemp, szPropName);

    char* pTemp          = szNameTemp;

    while (!bEnd)
    {
	while (*pTemp && *pTemp != '.')
	{
	    pTemp++;
	}
	if (!*pTemp)
	{
	    bEnd = TRUE;
	}
	*pTemp = 0;
	if(m_ActiveMap2.Lookup(szNameTemp, (void*&)pList))
	{
            DELETE_FAST_TEMP_STR(szNameTemp);
            UNLOCK(Process::get_proc());
	    return TRUE;
	}
	if (!bEnd)
	{
	    *pTemp = '.';
	    pTemp++;
	}
    }

    DELETE_FAST_TEMP_STR(szNameTemp);
    UNLOCK(Process::get_proc());
    return FALSE;
}

BOOL
ServerRegistry::IsDeleteActive(const char* szPropName, Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * If we are a composite
     */
    HXPropType type       = PT_UNKNOWN;

    BOOL bHadSubComp       = 0;
    UINT32 ulId            = 0;

    const char* szSubName  = NULL;
    UINT32 ulSubId         = 0;
    HX_RESULT hr           = HXR_FAIL;

    UINT32* pChildList     = NULL;
    UINT32 ulChildListSize = 0;

    UINT32 i               = 0;

    ServRegDB_node*  pNode = NULL;
    ServRegProperty* pProp = NULL;


    ulId = GetId(szPropName);

    type = GetType(ulId);

    if (type == PT_COMPOSITE)
    {
	// find out if any of out subprops are registered.
	// If any of our subprops are composites, recurse.

        GetChildIdList(ulId, pChildList, ulChildListSize);

        for (i = 0; i < ulChildListSize; i++)
        {
	    type = GetType(pChildList[i]);

            pNode = (ServRegDB_node*)m_pIdTable->get(pChildList[i]);

            if (!pNode)
            {
	        continue;
            }

            pProp = pNode->get_data();

            if (!pProp)
            {
                continue;
            }

	    if (type == PT_COMPOSITE)
	    {
		// If composite, recurse down.

		bHadSubComp = TRUE;
		if (IsActive(pProp->get_key_str(), Process::get_proc()))
		{
		    HX_VECTOR_DELETE(pChildList);
                    UNLOCK(Process::get_proc());
		    return TRUE;
		}
	    }
	    /*
	     * Check for this level (composite or not)
	     */
	    CHXSimpleList* pList = NULL;
	    if (m_ActiveMap2.Lookup(pProp->get_key_str(), (void*&)pList))
	    {
		HX_VECTOR_DELETE(pChildList);
                UNLOCK(Process::get_proc());
		return TRUE;
	    }
        }

        HX_VECTOR_DELETE(pChildList);

    }

    // If this is the deepest composite,
    // find out if any of the base composites of this prop,
    // or this prop itself are registered.

    if (!bHadSubComp)
    {
	CHXSimpleList* pList = NULL;
        NEW_FAST_TEMP_STR(szNameTemp, 256, strlen(szPropName) + 1);

	strcpy(szNameTemp, szPropName);

        char* pTemp          = szNameTemp;

	BOOL bEnd = FALSE;
	while (!bEnd)
	{
	    while (*pTemp && *pTemp != '.')
	    {
		pTemp++;
	    }
	    if (!*pTemp)
	    {
		bEnd = TRUE;
	    }

	    *pTemp = '\0';

	    if(m_ActiveMap2.Lookup(szNameTemp, (void*&)pList))
	    {
                DELETE_FAST_TEMP_STR(szNameTemp);
                UNLOCK(Process::get_proc());
		return TRUE;
	    }
	    if (!bEnd)
	    {
		*pTemp = '.';
		pTemp++;
	    }
	}
        DELETE_FAST_TEMP_STR(szNameTemp);
    }

    UNLOCK(Process::get_proc());
    return FALSE;
}


/*
 * pseudo IHXActivePropUser methods.
 */

/************************************************************************
*    Async request to set int pName to ul.
*/
STDMETHODIMP
ServerRegistry::SetActiveInt(const char* pName,
                             UINT32 ul, 
                             IHXActivePropUserResponse* pResponse,
                             Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName, Process::get_proc()))
    {
	HX_RESULT hr = SetInt(pName, ul, Process::get_proc());

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (hr != HXR_OK && hr != HXR_PROP_TYPE_MISMATCH)
	{
	    hr = AddInt(pName, ul, Process::get_proc()) ? HXR_OK : HXR_FAIL;
	}

	pResponse->SetActiveIntDone(hr, pName, ul, 0, 0);
        UNLOCK(Process::get_proc());
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap2.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
        UNLOCK(Process::get_proc());
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
	ulNumUsers, //num users for this prop.
	Process::get_proc());

    m_PendingMap2.SetAt(pTemp, (void*)cb);

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
	     * Schedule on the dq for the process who registered the object
	     * for active notification.
	     */
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, pUserInfo->m_pProc->procnum());
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/************************************************************************
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
ServerRegistry::SetActiveStr(const char* pName,
                             IHXBuffer* pBuffer,
                             IHXActivePropUserResponse* pResponse,
                             Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName, Process::get_proc()))
    {
	HX_RESULT hr = SetStr(pName, pBuffer, Process::get_proc());

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (hr != HXR_OK && hr != HXR_PROP_TYPE_MISMATCH)
	{
	    hr = AddStr(pName, pBuffer, Process::get_proc()) ? HXR_OK : HXR_FAIL;
	}

	pResponse->SetActiveStrDone(hr, pName, pBuffer, 0, 0);
        UNLOCK(Process::get_proc());
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap2.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
        UNLOCK(Process::get_proc());
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
	ulNumUsers, //num users for this prop.
	Process::get_proc());

    m_PendingMap2.SetAt(pTemp, (void*)cb);

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
	    * Schedule on the dq for the process who registered the object
	    * for active notification.
	    */
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, pUserInfo->m_pProc->procnum());
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/************************************************************************
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
ServerRegistry::SetActiveBuf(const char* pName,
                             IHXBuffer* pBuffer,
                             IHXActivePropUserResponse* pResponse,
                             Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName, Process::get_proc()))
    {
	HX_RESULT hr = SetBuf(pName, pBuffer, Process::get_proc());

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (hr != HXR_OK && hr != HXR_PROP_TYPE_MISMATCH)
	{
	    hr = AddBuf(pName, pBuffer, Process::get_proc()) ? HXR_OK : HXR_FAIL;
	}

	pResponse->SetActiveBufDone(hr, pName, pBuffer, 0, 0);

        UNLOCK(Process::get_proc());
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap2.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
        UNLOCK(Process::get_proc());
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
	ulNumUsers, //num users for this prop.
	Process::get_proc());

    m_PendingMap2.SetAt(pTemp, (void*)cb);

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
	     * Schedule on the dq for the process who registered the object
	     * for active notification.
	     */
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, pUserInfo->m_pProc->procnum());
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
ServerRegistry::DeleteActiveProp(const char* pName,
                                 IHXActivePropUserResponse* pResponse,
                                 Process* pProc)
{
    LOCK(Process::get_proc());

    /*
     * If it is not active, just set it / create it, send the
     * response, and return ok.
     */
    if(!IsActive(pName, Process::get_proc()))
    {
	Del(pName, Process::get_proc());
	//XXXPM check return code
	pResponse->DeleteActivePropDone(HXR_OK, pName, 0, 0);
        UNLOCK(Process::get_proc());
	return HXR_OK;
    }

    /*
     * If there is already one outstanding, then fail.  Maybe later
     * I should create a callback to try again?
     */
    char* pTemp = new_string(pName);
    strlwr(pTemp);
    PendingActiveRegChangesCallback* cb;
    if (m_PendingMap2.Lookup(pTemp, (void*&)cb))
    {
	delete[] pTemp;
	UNLOCK(Process::get_proc());
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
	ulNumUsers, //num users for this prop.
	Process::get_proc());

    /*
     * Since for this one, we only succeed if they all succeed, start with
     * status of ok.
     */
    cb->m_res = HXR_OK;
    m_PendingMap2.SetAt(pTemp, (void*)cb);


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
	     * Schedule on the dq for the process who registered the object
	     * for active notification.
	     */
	    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, pUserInfo->m_pProc->procnum());
	}
    }

    while (!pListList.IsEmpty())
    {
	pListList.RemoveHead();
    }
    delete[] pTemp;

    UNLOCK(Process::get_proc());
    return HXR_OK;
}

/*
 * IHXActivePropUserResponse methods.
 */

/************************************************************************
* Called with status result on completion of set request.
*/
STDMETHODIMP
ServerRegistry::SetActiveIntDone(HX_RESULT res, const char* pName, UINT32 ul,
				 IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    ISLOCKED();
    /*
     * Lookup the id and get the callback.
     */
    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap2.Lookup(pTemp, (void*&)cb))
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

    m_PendingMap2.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	cb->m_res = SetInt(pName, ul);

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (cb->m_res != HXR_OK && cb->m_res != HXR_PROP_TYPE_MISMATCH)
	{
	    cb->m_res = AddInt(pName, ul) ? HXR_OK : HXR_FAIL;
	}
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, cb->m_pProc->procnum());
    
    return HXR_OK;

}

STDMETHODIMP
ServerRegistry::SetActiveStrDone(HX_RESULT res, const char* pName,
                                 IHXBuffer* pBuffer, IHXBuffer* pInfo[],
                                 UINT32 ulNumInfo)
{
    ISLOCKED();

    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap2.Lookup(pTemp, (void*&)cb))
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

    m_PendingMap2.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	cb->m_res = SetStr(pName, pBuffer);

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (cb->m_res != HXR_OK && cb->m_res != HXR_PROP_TYPE_MISMATCH)
	{
	    cb->m_res = AddStr(pName, pBuffer) ? HXR_OK : HXR_FAIL;
	}
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, cb->m_pProc->procnum());
    
    return HXR_OK;
}

STDMETHODIMP
ServerRegistry::SetActiveBufDone(HX_RESULT res, const char* pName,
                                 IHXBuffer* pBuffer, IHXBuffer* pInfo[],
                                 UINT32 ulNumInfo)
{
    ISLOCKED();
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap2.Lookup(pTemp, (void*&)cb))
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

    m_PendingMap2.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	cb->m_res = SetBuf(pName, pBuffer);

    // SetInt returns three possible values, HXR_OK, HXR_FAILED,
    // HXR_PROP_TYPE_MISMATCH. Only bother to AddInt() if fail and 
    // not a type mismatch.
	if (cb->m_res != HXR_OK && cb->m_res != HXR_PROP_TYPE_MISMATCH)
	{
	    cb->m_res = AddBuf(pName, pBuffer) ? HXR_OK : HXR_FAIL;
	}
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, cb->m_pProc->procnum());
    
    return HXR_OK;
}

STDMETHODIMP
ServerRegistry::DeleteActivePropDone(HX_RESULT res, const char* pName,
				     IHXBuffer* pInfo[], UINT32 ulNumInfo)
{
    ISLOCKED();
    /*
     * Lookup the id and get the callback.
     */

    char* pTemp = new_string(pName);
    PendingActiveRegChangesCallback* cb = 0;
    strlwr(pTemp);
    if(!m_PendingMap2.Lookup(pTemp, (void*&)cb))
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

    m_PendingMap2.RemoveKey(pTemp);

    /*
     * If status is cool, then set the data.
     */
    if(cb->m_res == HXR_OK)
    {
	Del(pName);
    }


    /*
     * Schedule on the dq.
     */
    delete[] pTemp;
    Process::get_proc()->pc->dispatchq->send(Process::get_proc(), cb, cb->m_pProc->procnum());
    
    return HXR_OK;
}

/*
 * PendingActiveRegChangesCallback
 */

void
ServerRegistry::PendingActiveRegChangesCallback::func(Process* p)
{
    /*
     * we should now be in the correct process.
     */
    BOOL bLocked = p->pc->registry->MutexLockIfNeeded(p);

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

    if (bLocked) p->pc->registry->MutexUnlock(p);
    delete this;
}

/*
 * Adds lists of apus for all of the super props.
 */
void
ServerRegistry::_FillListList(CHXSimpleList* pList, const char* pName)
{
    ISLOCKED();
    CHXSimpleList* p;
    NEW_FAST_TEMP_STR(pTemp, 256, strlen(pName) + 1);
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
	if (m_ActiveMap2.Lookup(pTemp, (void*&)p))
	{
	    pList->AddHead((void*)p);
	}
	if (!bEnd)
	{
	    *pc = '.';
	    pc++;
	}
    }
    DELETE_FAST_TEMP_STR(pTemp);
    return;
}

/*
 * Adds lists of apus for all of the sub props.
 */
void
ServerRegistry::_AppendListList(CHXSimpleList* pList, const char* pName)
{
    ISLOCKED();
    CHXSimpleList* p;
    HXPropType type;

    /*
     * If this is not a composite or has no sub-props then
     * we can just go home now.
     */
    type = GetType(pName);
    if (type != PT_COMPOSITE)
    {
	return;
    }

    IHXValues* pValues = 0;
    GetPropList(pName, pValues);
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
	type = GetType(pSubName);
	if (type == PT_COMPOSITE)
	{
	    _AppendListList(pList, pSubName);
	}
	if (m_ActiveMap2.Lookup(pSubName, (void*&)p))
	{
	    pList->AddHead((void*)p);
	}
	res = pValues->GetNextPropertyULONG32(pSubName, ul);
    }
    pValues->Release();
}
