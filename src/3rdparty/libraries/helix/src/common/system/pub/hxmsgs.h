/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmsgs.h,v 1.6 2005/08/19 02:39:04 rggammon Exp $
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

#ifndef _HXMSGS_
#define _HXMSGS_

#ifdef _WIN32
#include "hlxclib/windows.h"
#define HXMSG_BASE WM_USER
#define HXMSG_TIMER WM_TIMER
#else
#define HXMSG_BASE 0
#define HXMSG_TIMER 1
#endif


//
// deprecated
//
#define HXMSG_ASYNC_TIMER	(HXMSG_TIMER)	        /*Async Timer Notification */
#define HXMSG_ASYNC_CALLBACK	(HXMSG_BASE + 100)      /*Async DNS Notification*/
#define HXMSG_ASYNC_DNS	        (HXMSG_BASE + 101)      /*Async DNS Notification*/
#define HXMSG_ASYNC_CONNECT	(HXMSG_BASE + 102)      /*Async Connect Notification*/
#define HXMSG_ASYNC_READ	(HXMSG_BASE + 103)      /*Async Read Notification*/
#define HXMSG_ASYNC_WRITE	(HXMSG_BASE + 104)	/*Async Write Notification*/
#define HXMSG_ASYNC_DETACH	(HXMSG_BASE + 105)	/*Async Detach Notification*/
#define HXMSG_ASYNC_NETWORKIO	(HXMSG_BASE + 106)	/*Read/Write*/
#define HXMSG_ASYNC_RESUME	(HXMSG_BASE + 107)	/* Resume network thread timer*/
#define HXMSG_ASYNC_STOP	(HXMSG_BASE + 108)	/* Stop network thread timer*/
#define HXMSG_ASYNC_ACCEPT	(HXMSG_BASE + 109)	/*Async Accept Notification */
#define HXMSG_ASYNC_SETREADER_CONNECTION	(HXMSG_BASE + 111)	/*local loopback reader accept completed */
#define HXMSG_ASYNC_START_READERWRITER	        (HXMSG_BASE + 112)	/*local loopback starting */

// dtdr messages
#define HXMSG_ASYNC_FILE_HEADER   (HXMSG_BASE + 113) 
#define HXMSG_ASYNC_STREAM_HEADER (HXMSG_BASE + 114) 
#define HXMSG_ASYNC_STREAM_DONE   (HXMSG_BASE + 115) 
#define HXMSG_ASYNC_PACKET        (HXMSG_BASE + 116) 
#define HXMSG_ASYNC_TERMINATION   (HXMSG_BASE + 117) 

// HXSocketSelector
#define HXMSG_SOCKETSELECT   (HXMSG_BASE + 113)

// HXTaskManager (each instance needs a unique message)
#define HXMSG_TASKDONE_1     (HXMSG_BASE + 114) // resolver
#define HXMSG_TASKDONE_2     (HXMSG_BASE + 115) // test

// HXNetThreadSocketProxy/Stub
#define HXMSG_THREADEDSOCK   (HXMSG_BASE + 116) 

//
// Generic
//
#define HXMSG_QUIT	     (HXMSG_BASE + 200)	/* Exit from the thread */


#endif
