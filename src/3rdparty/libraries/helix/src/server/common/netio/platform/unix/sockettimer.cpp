/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockettimer.cpp,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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
#ifdef PAULM_SOCKTIMING
#include <stdio.h>
#include "sockettimer.h"

sinfo::sinfo(int sock, int p, int cloak)
: fd(sock)
, port(p)
, used(1)
, cloaked(cloak)
{
    time(&intime);
}

void
sinfo::Set(int sock, int p, int cloak)
{
    fd = sock;
    port = p;
    used = 1;
    cloaked = cloak;
    time(&intime);
}

void
sinfo::SetCloak(int cloak)
{
    cloaked = cloak;
}

void
sinfo::Unset()
{
    used = 0;
}

SocketTimer::SocketTimer()
: size(0)
, table(0)
, fill(0)
{
    table = new sinfo*[8192];
    size = 8192;
}

SocketTimer::~SocketTimer()
{
    delete[] table;
}

void
SocketTimer::Add(int fd, int port, int cloak)
{
    int i;
    for (i = 0; i < fill; i++)
    {
	if (!table[i]->used)
	{
	    table[i]->Set(fd,port, cloak);
	    return;
	}
    }
    if (i < size)
    {
	table[i] = new sinfo(fd, port, cloak);
	fill = i + 1;
	return;
    }
    printf("Ran out of socket accounting space!!!!!!!\n");
}

void
SocketTimer::Remove(int fd)
{
    int i;
    for (i = 0; i < fill; i++)
    {
	if (table[i] && table[i]->used && table[i]->fd == fd)
	{
	    table[i]->Unset();
	}
    }
}

void
SocketTimer::SetCloak(int s, int c)
{
    int i;
    for (i = 0; i < fill; i++)
    {
	if (table[i] && table[i]->used && table[i]->fd == s)
	{
	    table[i]->SetCloak(c);
	}
    }
}

void
SocketTimer::Report()
{
    int i;
    time_t now;
    time(&now);
    printf("Socket times:\n");
    for (i = 0; i < fill; i++)
    {
	if (table[i] && table[i]->used)
	{
	    printf("fd: %d\tport: %d\tsecs: %d",
		table[i]->fd, table[i]->port,
		now - table[i]->intime);
	    if (table[i]->cloaked)
	    {
		printf("  <--  CLOAKED\n");
	    }
	    else
	    {
		printf("\n");
	    }
	}
    }
    printf("Done.\n");
    fflush(stdout);
}
#endif

