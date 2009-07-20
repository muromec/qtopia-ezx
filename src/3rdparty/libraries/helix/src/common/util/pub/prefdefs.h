/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: prefdefs.h,v 1.3 2004/07/09 18:23:37 hubbe Exp $
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

#ifndef __PREFDEFS_H__
#define __PREFDEFS_H__

#define DEF_SEEK_TIME        	10
#define DEF_AUDIO_QUALITY    	0     
#define DEF_SYNC_MULTIMEDIA  	1
#define DEF_RECEIVE_TCP      	0           
#define DEF_USE_UDP_PORT     	0
#define DEF_UDP_PORT         	7070
#define DEF_SERVER_TIMEOUT   	90    
#define DEF_SEND_STATISTICS  	1
#define DEF_LOSS_CORRECTION  	1      
#define DEF_USE_PROXY        	0  
#define DEF_PROXY_PORT       	1090   
#define MIN_PROXY_PORT			1000
#define MAX_PROXY_PORT			9999
#define DEF_HTTP_PROXY_PORT  	80
#define MIN_HTTP_PROXY_PORT		0
#define MAX_HTTP_PROXY_PORT		65535
#define DEF_USE_8BIT         	0 
#define DEF_USE_11KHZ        	0 
#define DEF_ACCEPT_LOSS      	10      
#define DEF_SCAN_TIME	     	10000 // milli-seconds...
#define MIN_SCAN_TIME			1000
#define MAX_SCAN_TIME			1200000 //20 min
#define DEF_BANDWIDTH        	28800
#define DEF_MAXCLIPS         	4
#define MAX_PERF_TIME			300 // seconds
#define MIN_PERF_TIME			10 // seconds
#define DEF_PERF_TIME        	60 // seconds

#define DEF_SETTINGS_TIMEOUT	2000
#define MIN_SETTINGS_TIMEOUT	1
#define MAX_SETTINGS_TIMEOUT	60000
#define	DEF_AUTO_TRANSPORT		1
#define	DEF_PREFPLAY_ENTIRECLIP 0
#define DEF_ATTEMPT_MULTICAST	1
#define DEF_ATTEMPT_UDP		1
#define DEF_ATTEMPT_TCP		1
#define DEF_ATTEMPT_HTTPCLOAK	1

#define DEF_HTTP_PORT		80
#define DEF_HTTP_PORT_STR	":80/"
#endif // __PREFDEFS_H__

