/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockpair.cpp,v 1.3 2003/08/17 14:41:46 dcollins Exp $ 
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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "hxtypes.h"
#include "sockpair.h"

SocketPair::SocketPair()
: m_bBlocking(TRUE)
, m_r(-1)
, m_w(-1)
{
}

SocketPair::~SocketPair()
{
    Close();
}

int
SocketPair::Init()
{
    int sv[2];
#if !defined _BEOS
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
#endif
    m_r = sv[0];
    m_w = sv[1];
    return 0;
}

int
SocketPair::Close()
{
    if (m_r != -1)
    {
	close(m_r);
    }
    if (m_w != -1)
    {
	close(m_w);
    }
    m_bBlocking = TRUE;
    return 0;
}


int
SocketPair::SetBlocking(BOOL bBlocking)
{
    fcntl(m_r, F_SETFL, O_NONBLOCK);
    fcntl(m_w, F_SETFL, O_NONBLOCK);
    return 0;
}

int
SocketPair::Read(unsigned char* pOut, int size)
{
    return read(m_r, (char*)pOut, size);
}

int
SocketPair::Write(unsigned char* pIn, int size)
{
    return write(m_w, (char*)pIn, size);
}

int
SocketPair::GetReadFD()
{
    return m_r;
}

int
SocketPair::GetWriteFD()
{
    return m_w;
}

