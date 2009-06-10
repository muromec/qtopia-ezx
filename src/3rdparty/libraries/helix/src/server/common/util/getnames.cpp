/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: getnames.cpp,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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


#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hxtypes.h"
#include "sockio.h"
#include "debug.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


#ifdef	MSDOS
#define SEP	'\\'
#else
#define SEP	'/'
#endif

#ifdef _WINCE
#include "wincestr.h"
#define strerror(x)
#endif

const char*
getprogname(const char* arg) 
{
    const char* progname = strrchr(arg, SEP);
    return progname ? progname + 1 : arg;
}

    char*
getservername() 
{
    char* local_host = new char[80];
    int result = 0;

    result = SocketIO::gethostname(local_host, 80);
    if (result != 0)
    {
	//DPRINTF(D_INFO, ("gethostname failed: %s\n", strerror(result)));
	delete[] local_host;
	return 0;
    }

    return local_host;
}
