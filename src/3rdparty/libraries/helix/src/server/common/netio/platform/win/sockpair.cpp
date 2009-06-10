/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockpair.cpp,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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

#include <winsock2.h>
#include "sockpair.h"
#include "hxassert.h"

SocketPair::SocketPair()
: m_bBlocking(TRUE)
, m_r(INVALID_SOCKET)
, m_w(INVALID_SOCKET)
{
}

SocketPair::~SocketPair()
{
    Close();
}

int
SocketPair::Init()
{
    SOCKET ear;
    sockaddr_in addr;
    int iSize;

    /* listen */
    ear = socket(AF_INET, SOCK_STREAM, 0);
    if (ear == INVALID_SOCKET)
    {
	return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (SOCKET_ERROR == bind(ear, (sockaddr*)&addr, sizeof(addr)))
    {
	closesocket(ear);
	return -1;
    }
    DWORD dwNoBlock = 1;
    if (SOCKET_ERROR == listen(ear, 1))
    {
	closesocket(ear);
	return -1;
    }
    iSize = sizeof(addr);
    if (SOCKET_ERROR == getsockname(ear, (sockaddr*)&addr, &iSize))
    {
	closesocket(ear);
	return -1;
    }
    ioctlsocket(ear, FIONBIO, &dwNoBlock);

    /* connect */
    m_w = socket(AF_INET, SOCK_STREAM, 0);
    if (m_w == INVALID_SOCKET)
    {
	closesocket(ear);
	return -1;
    }
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dwNoBlock = 1;
    ioctlsocket(m_w, FIONBIO, &dwNoBlock);
    connect(m_w, (sockaddr*)&addr, sizeof(addr));

    /* accept */
    while (1)
    {
	iSize = sizeof(addr);
	m_r = accept(ear, (sockaddr*)&addr, &iSize);
	if (m_r != INVALID_SOCKET)
	{
	    break;
	}
	Sleep(0);
    }

    dwNoBlock = 0;
    ioctlsocket(m_w, FIONBIO, &dwNoBlock);
    closesocket(ear);
    return 0;
}

int
SocketPair::Close()
{
    if (m_r != INVALID_SOCKET)
    {
	closesocket(m_r);
    }
    if (m_w != INVALID_SOCKET)
    {
	closesocket(m_w);
    }
    m_bBlocking = TRUE;
    return 0;
}


int
SocketPair::SetBlocking(BOOL bBlocking)
{
    HX_ASSERT(m_r != INVALID_SOCKET && m_w != INVALID_SOCKET);
    if (bBlocking != m_bBlocking)
    {
	DWORD dwNoBlock = !bBlocking;
	if (SOCKET_ERROR == ioctlsocket(m_w, FIONBIO, &dwNoBlock))
	{
	    return -1;
	}
	if (SOCKET_ERROR == ioctlsocket(m_r, FIONBIO, &dwNoBlock))
	{
	    dwNoBlock = m_bBlocking;
	    ioctlsocket(m_w, FIONBIO, &dwNoBlock);
	    return -1;
	}
	m_bBlocking = bBlocking;
	return 0;
    }
    return 0;
}

int
SocketPair::Read(unsigned char* pOut, int size)
{
    HX_ASSERT(m_r != INVALID_SOCKET && m_w != INVALID_SOCKET);
    return recv(m_r, (char*)pOut, size, 0);
}

int
SocketPair::Write(unsigned char* pIn, int size)
{
    HX_ASSERT(m_r != INVALID_SOCKET && m_w != INVALID_SOCKET);
    return send(m_w, (char*)pIn, size, 0);
}

int
SocketPair::GetReadFD()
{
    HX_ASSERT(m_r != INVALID_SOCKET && m_w != INVALID_SOCKET);
    return m_r;
}

int
SocketPair::GetWriteFD()
{
    HX_ASSERT(m_r != INVALID_SOCKET && m_w != INVALID_SOCKET);
    return m_w;
}
