/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servcallback.h,v 1.3 2004/05/13 18:57:48 tmarshall Exp $ 
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

#ifndef _CALLBACK_H_
#define _CALLBACK_H_


#include "hxassert.h"
#include "hxcom.h"
#include "debug.h"
#include "hxcore.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxmap.h"
#include "timeval.h"
#include "globals.h"

#include "hlxclib/sys/socket.h"

class SocketInfo
{
public:
    SocketInfo()
    {
	cb = 0;
	in = 0;
	ts = FALSE;
    }
    IHXCallback* cb;
    int in;
    BOOL ts;
};

class Callbacks
{
public:
    Callbacks();
    ~Callbacks();
    void    Prepare();
    void    Invoke();
    void    InvokeStart();
    int     InvokeOne();
    int     InvokeOneTS();
    void    Add(SOCKET s, IHXCallback* cb, BOOL bThreadSafe = FALSE);
    void    Remove(SOCKET s);
    void    Enable(SOCKET s, BOOL bThreadSafe = FALSE);
    void    Disable(SOCKET s);
    fd_set* GetSet();

private:
    fd_set store_set;
    fd_set result_set;
    CHXMapLongToObj m_map;
    unsigned i_invoking;
    unsigned i_ts_invoking;
    BOOL m_bCleanFinish;
};

inline
Callbacks::Callbacks()
{
    m_map.InitHashTable(256);
    FD_ZERO(&store_set);
    FD_ZERO(&result_set);
}

inline
Callbacks::~Callbacks()
{
}

inline void
Callbacks::Prepare()
{
    memcpy(result_set.fd_array,
	store_set.fd_array,
	store_set.fd_count * sizeof(SOCKET));
    result_set.fd_count = store_set.fd_count;
}

inline fd_set*
Callbacks::GetSet()
{
    return &result_set;
}

inline void
Callbacks::Add(SOCKET s, IHXCallback* cb, BOOL bThreadSafe)
{
    SocketInfo* pInfo;

    if(m_map.Lookup(s, (void*&)pInfo))
    {
	pInfo->cb = cb;
	pInfo->ts = bThreadSafe;
	if(!pInfo->in)
	{
	    store_set.fd_array[store_set.fd_count] = s;
	    store_set.fd_count++;
	    pInfo->in = 1;
	}
    }
    else
    {
	pInfo = new SocketInfo;
	pInfo->cb = cb;
	pInfo->ts = bThreadSafe;
	m_map.SetAt(s, (void*)pInfo);
	store_set.fd_array[store_set.fd_count] = s;
	store_set.fd_count++;
	pInfo->in = 1;
    }
	
}

inline void
Callbacks::Remove(SOCKET s)
{
    SocketInfo* pInfo;
    if(m_map.Lookup(s, (void*&)pInfo))
    {
	if(pInfo->in)
	{
	    unsigned i;
	    for(i = 0; i < store_set.fd_count; i++)
	    {
		if(store_set.fd_array[i] == s)
		{
		    memmove(&(store_set.fd_array[i]),
			&(store_set.fd_array[i + 1]),
			sizeof(SOCKET) * (store_set.fd_count - i - 1));
		    store_set.fd_count--;
		    break;
		}
	    }
	}
	delete pInfo;
	m_map.RemoveKey(s);
    }
}

inline void
Callbacks::Enable(SOCKET s, BOOL bThreadSafe)
{
    SocketInfo* pInfo;
    if(m_map.Lookup(s, (void*&)pInfo))
    {
	if(!pInfo->in)
	{
	    pInfo->ts = bThreadSafe;
	    store_set.fd_array[store_set.fd_count] =
		s;
	    store_set.fd_count ++;
	    pInfo->in = 1;
	}
    }
}

inline void
Callbacks::Disable(SOCKET s)
{
    SocketInfo* pInfo;
    if(m_map.Lookup(s, (void*&)pInfo))
    {
	if(pInfo->in)
	{
	    unsigned i;
	    for(i = 0; i < store_set.fd_count; i++)
	    {
		if(store_set.fd_array[i] == s)
		{
		    memmove(&(store_set.fd_array[i]),
			&(store_set.fd_array[i + 1]),
			sizeof(SOCKET) * (store_set.fd_count - i - 1));
		    store_set.fd_count--;
		    break;
		}
	    }
	    pInfo->in = 0;
	}
    }
}

inline void
Callbacks::Invoke()
{
    unsigned i;
    SocketInfo* pInfo;
    for (i = 0; i < result_set.fd_count; i++)
    {
	if (m_map.Lookup(result_set.fd_array[i], (void*&)pInfo) &&
	    pInfo->cb)
	{
	    pInfo->cb->Func();
	}
    }
}

inline void
Callbacks::InvokeStart()
{
    i_invoking = 0;
    i_ts_invoking = 0;
    m_bCleanFinish = TRUE;
}

inline int
Callbacks::InvokeOne()
{
    if (i_invoking >= result_set.fd_count)
    {
	return 0;
    }
    SocketInfo* pInfo;
    BOOL bInvoked=FALSE;


    while (!bInvoked && (i_invoking < result_set.fd_count))
    {
        if (m_map.Lookup(result_set.fd_array[i_invoking], (void*&)pInfo) &&
            pInfo->cb && (!pInfo->ts) && pInfo->in)
        {
            bInvoked=TRUE;

            if (m_bCleanFinish)
            {
                m_bCleanFinish = FALSE;
                pInfo->cb->Func();
                m_bCleanFinish = TRUE;
            }
            else
            {
                m_bCleanFinish = TRUE;
            }
        }
        
        i_invoking++;
    }

    if (bInvoked == TRUE)
        return (result_set.fd_count - i_invoking + 1);
    else
        return (result_set.fd_count - i_invoking);
}

inline int
Callbacks::InvokeOneTS()
{
    if (i_ts_invoking >= result_set.fd_count)
    {
	return 0;
    }
    SocketInfo* pInfo;
    BOOL bInvoked=FALSE;

    while (!bInvoked && (i_ts_invoking < result_set.fd_count))
    {
        if (m_map.Lookup(result_set.fd_array[i_ts_invoking], (void*&)pInfo) &&
	    pInfo->cb && pInfo->ts && pInfo->in)
        {
            bInvoked=TRUE;

	    if (m_bCleanFinish)
	    {
	        m_bCleanFinish = FALSE;
	        pInfo->cb->Func();
	        m_bCleanFinish = TRUE;
	    }
	    else
	    {
	        m_bCleanFinish = TRUE;
	    }
        }

        i_ts_invoking++;
    }

    if (bInvoked == TRUE)
        return (result_set.fd_count - i_ts_invoking + 1);
    else
        return (result_set.fd_count - i_ts_invoking);
}

#endif /* _CALLBACK_H_ */
