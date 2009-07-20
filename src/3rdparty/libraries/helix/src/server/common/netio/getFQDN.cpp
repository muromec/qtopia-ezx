/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: getFQDN.cpp,v 1.3 2008/07/03 21:54:16 dcollins Exp $ 
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

#include "hxstrutl.h"
#include "getnames.h"

#ifndef _WINDOWS
#include <unistd.h>
#include <strings.h>
#if defined _SOLARIS
#include <sys/systeminfo.h>
#endif
#endif
#include <stdio.h>


char* GetFullyQualifiedDomainName()
{
    /* POSIX?! we don't need no stinkin' POSIX! */
#ifdef _SOLARIS
    INT32 lHostNameLen = 0;
    INT32 lDomainNameLen = 0;
    char pNodeName[257]; /* max IPv4 nodename len, plus null */
    char pDomainName[2048];
   
    lHostNameLen = sysinfo(SI_HOSTNAME, pNodeName, 257);
    lDomainNameLen = sysinfo(SI_SRPC_DOMAIN, pDomainName, 2048);

    if ( (lHostNameLen > 0) && (lDomainNameLen > 0) )
    {
	UINT16 len = lHostNameLen + lDomainNameLen + 1; // add one for the "."
	char* pFullyQualifiedDomainName = new char [len + 1]; // plus the null
	sprintf(pFullyQualifiedDomainName, "%s.%s", pNodeName, pDomainName);
	pFullyQualifiedDomainName[len] = '\0';
	return pFullyQualifiedDomainName;
    }
    else
    {
	return 0;
    }

#elif defined _LINUX
    char pHostName[2048];
    char pDomainName[2048];
    
    if ((0 == ::gethostname(pHostName, 2048)))
    {
	if (strchr(pHostName, '.'))
	{
	    return new_string(pHostName);
	}
#if !defined(_LSB)
	else if (0 == ::getdomainname(pDomainName, 2048))
	{
	    UINT16 len = strlen(pDomainName) + strlen(pHostName) + 1; 
	    char* pFullyQualifiedDomainName = new char [len+1];
	    sprintf(pFullyQualifiedDomainName, "%s.%s", pHostName, pDomainName);
	    pFullyQualifiedDomainName[len] = '\0';
	    return pFullyQualifiedDomainName;
	}
#endif
	else
	{
	    return 0;
	}
    }
    else
    {
	return 0;
    }
#else
    return getservername();
#endif
}
