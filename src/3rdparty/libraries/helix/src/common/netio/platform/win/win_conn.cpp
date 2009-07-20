/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win_conn.cpp,v 1.5 2006/02/07 19:21:25 ping Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#include "conn.h"

#include "platform/win/win_net.h"

conn*
conn::actual_new_socket(IUnknown* pContext, UINT16 type)
{
    return win_net::new_socket(pContext, type);
}

/* 	call init_drivers() to do any platform specific initialization of the network drivers
	before calling any other functions in this class. params is a pointer to an optional
	platform specific structure needed to initialize the drivers. Simply typecast it to 
	the correct struct in your platform specific version of init_drivers(). The function
	will return HXR_OK if an error occurred otherwise it will return the platform
	specific error */

HX_RESULT
conn::init_drivers (void *params)
{
    return win_net::init_drivers(params);
}

HX_RESULT
conn::close_drivers (void *params)
{
    return win_net::close_drivers(params);
}


HX_RESULT
conn::host_to_ip_str(char *host, char *ip, UINT32 ulIPBufLen)
{
    return win_net::host_to_ip_str(host,ip, ulIPBufLen);
}


/* IHXNetworkInterfaceEnumerator support */
HX_RESULT 
conn::get_host_name(char *name, int namelen)
{
    HX_ASSERT(name && namelen);

    return win_net::get_host_name(name, namelen);
}


HX_RESULT
conn::get_host_by_name(char *name, REF(struct hostent*) hostent)
{
    HX_ASSERT(name);

    return win_net::get_host_by_name(name, hostent);
}

