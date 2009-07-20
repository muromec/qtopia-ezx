/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: passfd.cpp,v 1.8 2005/02/17 01:34:12 atin Exp $ 
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

#if defined(_SOLARIS) && defined(__GNUC__)
#undef _XOPEN_SOURCE
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "hxtypes.h"
#include "passfd.h"
#include "debug.h"

// FreeBSD 4.1 requires this for the ALIGN macro
#if defined(_FREEBSD4) || defined(_FREEBSD5) || defined(_OPENBSD) || defined(_NETBSD)
#include <machine/param.h>
#endif

/* Begin unix OS Independent fd passing */

#ifdef __alpha
#define _SOCKADDR_LEN
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <string.h>
#if defined __linux && !defined _RED_HAT_5_X_
	#include <linux/un.h>
	#define CMSG_DATA(cmsg)         ((u_char *)((cmsg) + 1))
#endif

#if defined __hpux  || defined __sun__ || defined __sgi__ || (defined __alpha && !defined _OSF1) || (defined _SOLARIS && defined _NATIVE_COMPILER)
#define OLDBSD
#endif

#ifndef OLDBSD
#define CONTROLLEN (sizeof(struct cmsghdr) + sizeof(int))
union _control_un
{
    struct cmsghdr cm;
    char control[CONTROLLEN];
};
typedef union _control_un control_un;
#endif

int
send_connection(int port, int fd)
{
    struct iovec    iov[1];
    struct msghdr   msg;
    char            buf[2];
#ifndef OLDBSD
 control_un hdrPlusData;
 struct cmsghdr *cmptr = &(hdrPlusData.cm);
#endif
    iov[0].iov_base = buf;
    iov[0].iov_len = 2;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    if (fd < 0)
    {
#ifndef OLDBSD
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
#else
        msg.msg_accrights = NULL;
        msg.msg_accrightslen = 0;
#endif
        buf[1] = -fd;
        if (buf[1] == 0)
            buf[1] = 1;
    } else
    {
#ifndef OLDBSD
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        cmptr->cmsg_len = CONTROLLEN;
        msg.msg_control = (caddr_t) cmptr;
        msg.msg_controllen = CONTROLLEN;
        *(int *) CMSG_DATA(cmptr) = fd;
#else
        msg.msg_accrights = (caddr_t) & fd;
        msg.msg_accrightslen = sizeof(int);
#endif
        buf[1] = 0;
    }
    buf[0] = 0;

    if (sendmsg(port, &msg, 0) != 2)
        return -1;

    return 0;
}

int
recv_connection(int port)
{
    int             nread, status;
    int             newfd = -1;
    char            *ptr, buf[32000];
    struct iovec    iov[1];
    struct msghdr   msg;
#ifndef OLDBSD
    control_un hdrPlusData;
    struct cmsghdr *cmptr = &(hdrPlusData.cm);
#endif

    status = -1;
    for (;;)
    {
        iov[0].iov_base = buf;
        iov[0].iov_len = sizeof(buf);
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
#ifndef OLDBSD
        msg.msg_control = (caddr_t) cmptr;
        msg.msg_controllen = CONTROLLEN;
#else
        msg.msg_accrights = (caddr_t) & newfd;
        msg.msg_accrightslen = sizeof(int);
#endif

        if ((nread = recvmsg(port, &msg, 0)) < 0)
        {
            perror("recvmsg");
            return -1;
        } else if (nread == 0)
        {
            return -1;
        }
        for (ptr = buf; ptr < &buf[nread];)
        {
            if (*ptr++ == 0)
            {
                if (ptr != &buf[nread - 1])
                {
                    return -1;
                }
                status = *ptr & 255;
                if (status == 0)
                {
#ifndef OLDBSD
                    if (msg.msg_controllen != CONTROLLEN)
                    {
#else
                    if (msg.msg_accrightslen != sizeof(int))
                    {
#endif
                        return -1;
                    }
#ifndef OLDBSD
                    newfd = *(int *) CMSG_DATA(cmptr);
#endif
                } else
                    newfd = -status;
                nread -= 2;
            }
        }
        if (nread > 0)
            return -1;
        if (status >= 0)
            return newfd;
    }
}


/* Machine Independant fd passing - test program */
#if 0
main()
{
    int             s[2];
    int             f;
    char            buffer[1024];

    socketpair(AF_UNIX, SOCK_STREAM, 0, s);

    if (fork())
    {
        f = open("/etc/passwd", O_RDONLY);
        read(f, buffer, 10);

        send_fd(s[0], f);
    } else
    {
        f = recv_fd(s[1]);
        read(f, buffer, 512);
        write(0, buffer, 512);
    }
}
#endif //0
