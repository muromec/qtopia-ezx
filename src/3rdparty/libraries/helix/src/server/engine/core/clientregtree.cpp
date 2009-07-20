/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: clientregtree.cpp,v 1.6 2005/07/20 21:47:53 dcollins Exp $ 
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

/*
 * ClientRegTree: Implements a persistent client tree in the registry.
 *
 * Implemented to allow multiple GETs from an HTTP client (such as WMP) to
 * run against as the same "client"/session-id for statistics gathering.
 *
 * m_pProc should always be the current proc, m_pContext the current proc's
 * context, and so forth.  When the regtree is taken over by a new client, 
 * make sure these are updated properly.
 *
 * Client  ->  ClientGUIDEntry  ->  ClientRegTree
 * 
 * Every Client needs a RegTree, but not necessarily a GUIDEntry in the 
 * GUIDTable.  But for simplicity, the callback/destruct logic is all in one
 * place, the GUIDEntry, so Client objects create a shell GUIDEntry for every
 * RegTree, whether it's in the global table or not.  I therefore also got rid
 * of refcounting on the RegTree, because there should be one and only one for
 * per GUIDEntry and it can be deleted when the GUIDEntry is deleted.
 * In fact, I've decided to make the RegTree a static data member of the entry
 * for efficiency.  You can make it a pointer again if you need to.
 *
 */

#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "dict.h"
#include "debug.h"
#include "config.h"
#include "netbyte.h"

#include "server_context.h"
#include "server_info.h"
#include "hxstats.h"
#include "server_stats.h"
#include "clientregtree.h"
#include "client.h"


const int ClientRegTree::MAX_TIME_STRING_LEN = 80;

/*
 * ClientRegTree implementation
 */

/*
 * ClientRegTree::RegTreeInit()
 *
 * This object is meant to be passed from client to client as the sequential
 * WMT HTTP requests come in.  Therefore, only one client at a time is 
 * accessing it (so no mutex needed), but we must ensure that this object has
 * the context and registry for the current proc.  Thus, it must be init'ed
 * every time a new client picks it up.
 */
HX_RESULT
ClientRegTree::RegTreeInit(Client* pClient, IHXBuffer* pStartTime)
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pClassFactory);

    m_pProc = pClient->proc;        // don't addref/release
    m_pProc->pc->server_context->
        QueryInterface(IID_IUnknown, (void**) &m_pContext);

    m_pContext->QueryInterface(IID_IHXRegistry, (void**) &m_pRegistry);
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                               (void**) &m_pClassFactory);

    // The seqnum (session id) is persistent across calls.
    // Only set up the registry tree once.

    if (!m_ulRegistryConnId)
    {
        m_ulRegistryConnId = m_pProc->pc->server_info->
            IncrementTotalClientCount(m_pProc);

        InitRegistry(pStartTime);
    }

    return HXR_OK;
}

ClientRegTree::ClientRegTree()
    : m_lRefCount(0)
      , m_pContext(NULL)
      , m_pClassFactory(NULL)
      , m_pProc(NULL)
      , m_pRegistry(NULL)
      , m_ulRegistryConnId(0)
{
    // initialize the id array that stores the properties' ids.
    memset(m_ulRegId, 0, sizeof(UINT32)*MAX_FIELDS);
}

ClientRegTree::~ClientRegTree()
{
    DeleteRegEntry();

    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pContext);
}

void
ClientRegTree::InitRegistry(IHXBuffer* pStartTime)
{
    IHXBuffer* pValue;
    char str[128];



    sprintf(str, "client.%ld", m_ulRegistryConnId);
    m_ulRegId[CLIENT] = m_pRegistry->AddComp(str);
    sprintf(str, "client.%ld.ConnID", m_ulRegistryConnId);
    // should this be conn_id from client obj??
    m_ulRegId[CONN_ID] = m_pRegistry->AddInt(str, m_ulRegistryConnId);

    if (pStartTime) 
    {
        m_ulRegId[TIME] = m_pRegistry->AddStr(str, pStartTime);
    }
    else // Proxy.
    {
        time_t     start_time;
        char       start_time_string[MAX_TIME_STRING_LEN]; //ok 
        struct tm  start_time_struct;

        time(&start_time);
        hx_localtime_r(&start_time, &start_time_struct);
        strftime(start_time_string, MAX_TIME_STRING_LEN, "%d/%b/%Y:%H:%M:%S",
                &start_time_struct);
        sprintf(str, "client.%ld.StartTime", m_ulRegistryConnId);
        m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pValue);
        pValue->Set((const BYTE*)start_time_string, strlen(start_time_string)+1);

        m_ulRegId[TIME] = m_pRegistry->AddStr(str, pValue);
        pValue->Release();
    }


    sprintf(str, "client.%ld.Session", m_ulRegistryConnId);
    m_ulRegId[SESSION] = m_pRegistry->AddComp(str);

    sprintf(str, "client.%ld.SessionCount", m_ulRegistryConnId);
    m_ulRegId[SESSION_COUNT] = m_pRegistry->AddInt(str, 0);

    sprintf(str, "client.%ld.ControlBytesSent", m_ulRegistryConnId);
    m_ulRegId[CTRL_BYTES_SENT] = m_pRegistry->AddInt(str, 0);

}

HX_RESULT
ClientRegTree::SetAddrs(IHXBuffer* pLocalAddr, IHXBuffer* pForeignAddr, 
                        UINT16 usForeignPort, BOOL is_cloak)
{
    HX_RESULT ret = HXR_OK;
    IHXBuffer* pValue;
    char str[128];

    if (pForeignAddr)
    {
        if (m_ulRegId[ADDR])
        {
            ret = m_pRegistry->SetStrById(m_ulRegId[ADDR], pForeignAddr);
        }
        else
        {
            sprintf(str, "client.%ld.Addr", m_ulRegistryConnId);
            m_ulRegId[ADDR] = m_pRegistry->AddStr(str, pForeignAddr);
            ret = m_ulRegId[ADDR] ? HXR_OK : HXR_FAIL;
        }
    }

    if (pLocalAddr && SUCCEEDED(ret))
    {
        DPRINTF(0x5d000000, ("LocalAddr = %s\n", 
                (const char *)pLocalAddr->GetBuffer()));
        if (m_ulRegId[INTERFACE_ADDR])
        {
            ret = m_pRegistry->SetStrById(m_ulRegId[INTERFACE_ADDR], pLocalAddr);
        }
        else
        {
            sprintf(str, "client.%ld.InterfaceAddr", m_ulRegistryConnId);
            m_ulRegId[INTERFACE_ADDR] = m_pRegistry->AddStr(str, pLocalAddr);
            ret = m_ulRegId[INTERFACE_ADDR] ? HXR_OK : HXR_FAIL;
        }
    }

    if (SUCCEEDED(ret))
    {
        if (m_ulRegId[IN_PORT])
        {
            ret = m_pRegistry->SetIntById(m_ulRegId[IN_PORT], 
                                          (INT32)WToNet(usForeignPort));
        }
        else
        {
            sprintf(str, "client.%ld.Port", m_ulRegistryConnId);
            m_ulRegId[IN_PORT] = m_pRegistry->
                AddInt(str, (INT32)WToNet(usForeignPort));
            ret = m_ulRegId[IN_PORT] ? HXR_OK : HXR_FAIL;
        }
    }

    if (SUCCEEDED(ret))
    {
        if (m_ulRegId[IS_CLOAK])
        {
            ret = m_pRegistry->SetIntById(m_ulRegId[IS_CLOAK], is_cloak);
        }
        else
        {
            sprintf(str, "client.%ld.IsCloaked", m_ulRegistryConnId);
            m_ulRegId[IS_CLOAK] = m_pRegistry->AddInt(str, is_cloak);
            ret = m_ulRegId[IS_CLOAK] ? HXR_OK : HXR_FAIL;
        }
    }

    HX_ASSERT(SUCCEEDED(ret));
    return ret;
}

HX_RESULT
ClientRegTree::SetProtocol(IHXBuffer* pValue)
{
    HX_RESULT ret;

    if (m_ulRegId[PROTOCOL])
    {
        ret = m_pRegistry->SetStrById(m_ulRegId[PROTOCOL], pValue);
    }
    else
    {
        char str[128];
        sprintf(str, "client.%ld.Protocol", m_ulRegistryConnId);
        m_ulRegId[PROTOCOL] = m_pRegistry->AddStr(str, pValue);
        ret = m_ulRegId[PROTOCOL] ? HXR_OK : HXR_FAIL;
    }

    return ret;
}

HX_RESULT
ClientRegTree::SetVersion(IHXBuffer* pValue)
{
    HX_RESULT ret;

    if (m_ulRegId[VERSION])
    {
        ret = m_pRegistry->SetStrById(m_ulRegId[VERSION], pValue);
    }
    else
    {
        char str[128];
        sprintf(str, "client.%ld.Version", m_ulRegistryConnId);
        m_ulRegId[VERSION] = m_pRegistry->AddStr(str, pValue);
        ret = m_ulRegId[VERSION] ? HXR_OK : HXR_FAIL;
    }

    return ret;
}

HX_RESULT
ClientRegTree::SetControlBytesSent(UINT32 ulControlBytesSent)
{
    return m_pRegistry->SetIntById(m_ulRegId[CTRL_BYTES_SENT], 
                                   ulControlBytesSent);
}

HX_RESULT
ClientRegTree::DeleteRegEntry()
{
    if (m_ulRegId[CLIENT])
    {
        m_pRegistry->DeleteById(m_ulRegId[CLIENT]);
        m_ulRegId[CLIENT] = 0;
    }
    return HXR_OK;
}
