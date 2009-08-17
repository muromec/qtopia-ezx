/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rights_msg.cpp,v 1.5 2003/11/06 21:24:05 atin Exp $ 
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

#if defined(_SOLARIS) && defined(__GNUC__)
#undef _XPG4_2
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "rights_msg.h"


rights_msg::rights_msg()
{
    m_buf = 0;
    m_size = 0;
    m_pNextFD = 0;
}

rights_msg::~rights_msg()
{
    delete[] m_buf;
}

void
rights_msg::set_num_fd(int n)
{
    /*
     * buffer layout:
     * (msghdr)(iovec)(buf)(rights....)
     */
    int _size;
    struct iovec* piov;
    unsigned char* pbuf;

    _size = sizeof(struct msghdr) + sizeof(struct iovec) +
	2 + sizeof(int) * n;

    /*
     * Move the size up to something word aligned.
     */
    while (_size & 3)
    {
	_size++;
    }

    if (_size > m_size)
    {
	delete[] m_buf;
	m_size = _size;
	m_buf = new unsigned char[m_size];
	m_pmh = (struct msghdr*)m_buf;
	piov = (struct iovec*)(m_buf + sizeof(struct msghdr));
	pbuf = (unsigned char*)(m_buf + sizeof(struct msghdr) +
				sizeof(struct iovec));
	pbuf[0] = 0;
	pbuf[1] = 0;
	piov->iov_base = (char*)pbuf;
	piov->iov_len = 2;
	m_pmh->msg_iov = piov;
	m_pmh->msg_iovlen = 1;
	m_pmh->msg_name = NULL;
	m_pmh->msg_name = 0;
    }
    else
    {
	m_pmh = (struct msghdr*)m_buf;
    }
    m_pmh->msg_accrightslen = sizeof(int) * n;
    unsigned char* p = (unsigned char *)(m_buf + sizeof(struct msghdr) +
        sizeof(struct iovec) + 2);

    /*
     * word align m_pNextFD
     */
    while ((PTR_INT)p & 3)
    {
	p++;
    }
    m_pNextFD = (int *)p;
    m_pmh->msg_accrights = (char*)m_pNextFD;
}

void
rights_msg::reset_num_fd(int n)
{
    /*
     * buffer layout:
     * (msghdr)(iovec)(buf)(rights....)
     */
    int _size;
    struct msghdr* pmh;
    struct iovec* piov;
    unsigned char* pbuf;
    unsigned char* buf;

    _size = sizeof(struct msghdr) + sizeof(struct iovec) +
	2 + sizeof(int) * n;

    /*
     * Move the size up to something word aligned.
     */
    while (_size & 3)
    {
	_size++;
    }

    m_size = _size;
    buf = new unsigned char[m_size];
    pmh = (struct msghdr*)buf;
    piov = (struct iovec*)(buf + sizeof(struct msghdr));
    pbuf = (unsigned char*)(buf + sizeof(struct msghdr) +
			    sizeof(struct iovec));
    pbuf[0] = 0;
    pbuf[1] = 0;
    piov->iov_base = (char*)pbuf;
    piov->iov_len = 2;
    pmh->msg_iov = piov;
    pmh->msg_iovlen = 1;
    pmh->msg_name = NULL;
    pmh->msg_name = 0;
    pmh->msg_accrightslen = sizeof(int) * n;
    unsigned char* p = (unsigned char *)(buf + sizeof(struct msghdr) +
        sizeof(struct iovec) + 2);
    unsigned char* oldfds = (unsigned char *)(m_buf + sizeof(struct msghdr) +
        sizeof(struct iovec) + 2);

    /*
     * word align m_pNextFD
     */
    while ((PTR_INT)oldfds & 3)
    {
	oldfds++;
    }
    memcpy(p, oldfds, sizeof(int) * n);
    m_pNextFD = (int *)p;
    pmh->msg_accrights = (char*)m_pNextFD;
    delete [] m_buf;
    m_buf = buf;
    m_pmh = pmh;
}

void
rights_msg::add_fd(int fd)
{
    *m_pNextFD++ = fd;
}

const unsigned char* 
rights_msg::buf()
{
    return m_buf;
}

int
rights_msg::size()
{
    return m_size;
}

