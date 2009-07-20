/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_resource.h,v 1.3 2003/06/27 02:41:30 dcollins Exp $ 
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

#ifndef _SERVER_RESOURCE_H
#define _SERVER_RESOURCE_H

/* 
 * Some of these values are for server_engine.h
 */
#define DESCRIPTOR_START_VALUE		15
#define SOCK_START_VALUE		4
#define RESOLVER_CAPACITY_VALUE		3
#define MAX_RESOLVERS			20
#define DESCRIPTOR_CAPACITY_VALUE (*g_pDescriptorCapacityValue)
#define SOCK_CAPACITY_VALUE (*g_pSockCapacityValue)

extern UINT32* volatile g_pDescriptorCapacityValue;
extern UINT32* volatile g_pSockCapacityValue;

inline void
InitServerResourceValues()
{
    g_pDescriptorCapacityValue = new UINT32;
    g_pSockCapacityValue = new UINT32;


#if defined _WIN32
    DESCRIPTOR_CAPACITY_VALUE =	10000000;
    SOCK_CAPACITY_VALUE	= 10978; // 0.67 (slop) * 16384
#endif

#if defined _UNIX
/* Unix only has descriptors.  Claim to have an insane amount of sockets */
    SOCK_CAPACITY_VALUE	= 10000000;

#if defined _FREEBSD || defined _OPENBSD || defined _NETBSD
    DESCRIPTOR_CAPACITY_VALUE = 16000;

#elif defined (_SOLARIS) || defined (_SUN)
/*
 * on Solaris there seems to be a limit to the max num of descriptors
 * that select() can handle -- 1024
 * there is content out there which uses SMIL where each connection
 * can potentially have upto 10 descriptors, so as a overly conservative
 * estimate we set each streamer to handle only 100 connections.
 * it is better to have more streamers than have the server choke because
 * of too many file descriptors open.
 */
    DESCRIPTOR_CAPACITY_VALUE = 680;

#elif defined (_LINUX)

    DESCRIPTOR_CAPACITY_VALUE = 3000;

#elif defined (_IRIX)
    DESCRIPTOR_CAPACITY_VALUE = (int)sysconf(_SC_OPEN_MAX);
#else

    DESCRIPTOR_CAPACITY_VALUE = 1024;

#endif
#endif /* ifdef _UNIX */

}


#endif
