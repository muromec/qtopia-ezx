/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: OTTCPWillDial.h,v 1.5 2007/07/06 21:58:01 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

/*
	This code was descended from Apple Sample Code, but the
	code may have been modified.
*/

/*
	File:		OTTCPWillDial.h

	Contains:	Library to determine whether open a TCP endpoint will
				dial the modem.

	Written by:	Quinn "The Eskimo!"

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

	You may incorporate this sample code into your applications without
	restriction, though the sample code has been provided "AS IS" and the
	responsibility for its operation is 100% yours.  However, what you are
	not permitted to do is to redistribute the source as "DSC Sample Code"
	after having made changes. If you're going to re-distribute the source,
	we require that you make it clear in the source that the code was
	descended from Apple Sample Code, but that you've made changes.
*/

/////////////////////////////////////////////////////////////////

//#import "Types.h"
#ifndef _MAC_MACHO
#include <types.h>
#endif
/////////////////////////////////////////////////////////////////

enum {
	kOTTCPDialUnknown = 0,
	kOTTCPDialTCPDisabled,
	kOTTCPDialYes,
	kOTTCPDialNo
};

#ifdef __cplusplus
extern "C" {
#endif


OSStatus OTTCPWillDial(ULONG32 *willDial);
//extern OSStatus OTTCPWillDial(ULONG32 *willDial);
	// This routine returns, in willDial, a flag indicating
	// whether opening a TCP/IP provider will cause the modem 
	// to dial.  You must call InitOpenTransport before calling
	// this routine.

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HXMacNetWillDial
//
// If a destination hostname is provided, it will be used when calling 
// SCNetworkCheckReachabilityByName, otherwise pass NULL and a default address will be used (to simplify
// the calling requirements). Return values are the same as OTTCPWillDial. 
//
// This function is a wrapper of OTTCPWillDial and should be used in its stead. If running on 
// OS X, SystemConfiguration.framework will be used to determine if a connection will dial,
// otherwise the OT calls will be used as a fallback.

OSStatus HXMacNetWillDial( char* remoteHost, ULONG32* willDial );

#ifdef __cplusplus
}
#endif

