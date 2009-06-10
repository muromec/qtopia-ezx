/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: engine.cpp,v 1.4 2007/06/27 14:40:52 srao Exp $ 
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

#include <errno.h>
#include <process.h>
#include "hxtypes.h"
#include "debug.h"
#include "engine.h"
#include "trycatch.h"



extern BOOL	terminated;
extern BOOL g_bServerGoingDown;
extern Engine** volatile g_ppLastEngine;

void
Engine::KillSelect()
{
    char x = 0;
    if (send(m_sock, &x, 1, 0) < 0)
    {
        if (!g_bServerGoingDown)
        {
            printf("KillSelect: -- send() failed - error(%d)\n",
                WSAGetLastError());
        }
    }
}

void
Engine::InitKillSelect()
{
    SOCKET trigger[3];
    sockaddr_in addr;
    int iSize;

    /*
     * Listen side.
     */
    trigger[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (trigger[0] == INVALID_SOCKET)
    {
	printf("InitKillSelect: trigger 0 failed\n");
	return;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (SOCKET_ERROR == bind(trigger[0], (sockaddr*)&addr, sizeof(addr)))
    {
	printf("InitKillSelect: bind failed - error(%d)\n",
	    WSAGetLastError());
	return ;
    }
    DWORD dwNoBlock = 1;
    if (SOCKET_ERROR == listen(trigger[0], 1))
    {
	printf("InitKillSelect: listen failed - error(%d)\n",
	    WSAGetLastError());
    }
    iSize = sizeof(addr);
    if (SOCKET_ERROR == getsockname(trigger[0], (sockaddr*)&addr, &iSize))
    {
	printf("InitKillSelect: getsockname failed - error(%d)\n",
	    WSAGetLastError());
	return;
    }
    ioctlsocket(trigger[0], FIONBIO, &dwNoBlock);


    /*
     * connect side
     */
    trigger[1] = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dwNoBlock = 1;
    ioctlsocket(trigger[1], FIONBIO, &dwNoBlock);
    connect(trigger[1], (sockaddr*)&addr, sizeof(addr));

    /*
     * Accept side.
     */
    while (1)
    {
	iSize = sizeof(addr);
	trigger[2] = accept(trigger[0], (sockaddr*)&addr, &iSize);
	if (trigger[2] != INVALID_SOCKET)
	{
	    break;
	}
	Sleep(0);
    }

    closesocket(trigger[0]);

    CatchKillSelect* pCatch = new CatchKillSelect;
    pCatch->m_sock = trigger[2];
    m_sock = trigger[1];
    callbacks.add(HX_READERS, pCatch->m_sock, pCatch, TRUE);
}

STDMETHODIMP
Engine::CatchKillSelect::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    
    *ppvObj = 0;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
Engine::CatchKillSelect::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG32)
Engine::CatchKillSelect::Release()
{
    return 1;
}

STDMETHODIMP
Engine::CatchKillSelect::Func()
{
    char x;
    recv(m_sock, &x, 1, 0);
    return HXR_OK;
}


Engine::Engine()
    : m_pSharedUDPReader(0)
{
    //jmp_start_flag = 0;
    //memset(&jb, 0, sizeof(jmp_buf));

    /*
     * Normalize now for NT
     */

    (void)gettimeofday(&now, 0);
    now.tv_usec = (now.tv_usec / 1000) * 1000;
    m_ulMainloopIterations = 0;

    InitKillSelect();
}
