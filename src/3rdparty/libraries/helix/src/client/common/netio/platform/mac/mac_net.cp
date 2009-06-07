/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_net.cp,v 1.4 2004/07/09 18:43:19 hubbe Exp $
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

#include "hxcom.h"
#include "Mac_net.h"
#include "OT_net.h"

#include "macsockets.h"

#ifdef _CARBON
#include "hx_moreprocesses.h"
#endif

Boolean Mac_net::sHasOT = FALSE;
Boolean	Mac_net::sNetInitialized = FALSE;

Mac_net*
Mac_net::new_socket(UINT16 type)
{
Mac_net 	*c;
HX_RESULT	theErr = HXR_OK;

	if(!sNetInitialized)
		return(NULL);
	
	if(sHasOT) 				// create Open Transport socket
	{
		c = (Mac_net *) OT_net::new_socket(type);
	}
	else 
	{
		c = NULL;
	}	
	return(c);
}

// Mac_net should set the socket reference to a value indicating the socket is not open
Mac_net::Mac_net (void)

{
	mLastError	= noErr;
	m_lRefCount	= 0;
}

// ~Mac_net should close the socket if it is open
Mac_net::~Mac_net(void)
{

}

Boolean 
Mac_net::CheckForCancel(void)
{
#ifdef _CARBON
    if (IsMacInCooperativeThread())
    {
	return ::CheckEventQueueForUserCancel();
    }
    else
    {
	return false;
    }
#else

EventRecord theEvent;
Boolean 	Cancel;
char 		c;

	Cancel = FALSE;

	if(WaitNextEvent(everyEvent, &theEvent, 0L,NULL))		/* check for cancel */
	{
	
		AEProcessAppleEvent(&theEvent);
	
		if(theEvent.what == keyDown)
		{
			if (theEvent.modifiers & cmdKey)	
			{
			
				
			
				c = theEvent.message & charCodeMask;
				if(c == '.')
					Cancel = TRUE;
			}
		}
	}
	return(Cancel);
#endif // _CARBON
}	


// init_drivers() should do any network driver initialization here
// params is a pointer to a platform specfic defined struct that 
// contains an required initialization data

HX_RESULT
Mac_net::init_drivers(void *params)
{

HX_RESULT theErr = HXR_OK;

	if(sNetInitialized)
		return HXR_OK;

	// check if OT can be initialized
	theErr = OT_net::init_drivers(params);
	if(theErr == HXR_OK)
	{
		sHasOT = TRUE;
		sNetInitialized = TRUE;
	}
	else 
	{		
		// no longer uses MacTCP
	}
	
	if(theErr)
		theErr = HXR_NET_CONNECT;
		
	return(theErr);
}

/* 	close_drivers() should close any network drivers used by the program
 	NOTE: The program MUST not make any other calls to the network drivers
 	until init_drivers() has been called */


HX_RESULT
Mac_net::close_drivers(void *params)
{

HX_RESULT theErr = HXR_OK;

	if(sNetInitialized)
	{
		if(sHasOT) 
			theErr = OT_net::close_drivers(params);

	}
	
	return(theErr);
}

HX_RESULT
Mac_net::host_to_ip_str(char *host, char *ip, UINT32 ulBufLen)
{
	HX_RESULT theErr = HXR_OK;
	
	if(sNetInitialized)
	{
		if(sHasOT) 
			theErr = OT_net::host_to_ip_str(host,ip, ulBufLen);
	}
	else
		theErr = HXR_SOCKET_CREATE;

	return(theErr);
}


INT16
getsockname(int sock, sockaddr* addr, INT16* addr_len)
{
	return 0;	// shouldn't these really do more?
}

ULONG32	Mac_net::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 Mac_net::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

