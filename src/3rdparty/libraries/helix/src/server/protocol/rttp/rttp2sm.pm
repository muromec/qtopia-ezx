%{
/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rttp2sm.pm,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

/*
 * This is generated code, do not modify. Look in rttp2sm.pm to make
 * modifications.
 */

#ifndef _RTTP2SM_H_
#define _RTTP2SM_H_

/*
 * Opcodes for the server<->monitor protocol. 
 */

#define RTTPSM_UNKNOWN			0x00
#define RTTPSM_NEWCLIENT		0x01
#define RTTPSM_SPLITTER_CLIENT		0x02
#define RTTPSM_PLAYER			0x04
#define RTTPSM_SPLITTER			0x08
#define RTTPSM_RESOLVER			0x10
#define RTTPSM_MONITOR			0x20
#define RTTPSM_LIVESRC			0x40
#define RTTPSM_SUBSERVER		0x80
#define RTTPSM_RESOLVER_CLIENT		0x100

#define RTTPSM_CLIENT_ADDED			'I'
#define RTTPSM_CLIENT_REMOVED		'S'
#define RTTPSM_CLIENT_UPDATED		'U'
#define RTTPSM_CLIENT_CONNECTED		'A'

#define RTTPSM_REQUEST_SERVER_CONFIG		RTTP(SM,0)
#define RTTPSM_TRANS_SERVER_CONFIG		RTTP(SM,1)
#define RTTPSM_SET_CONFIG_VAR			RTTP(SM,2)
#define RTTPSM_START_CURRENT_COUNTS		RTTP(SM,3)
#define RTTPSM_START_SERVER_STRUCTURE		RTTP(SM,4)
#define RTTPSM_SERVER_STRUCTURE			RTTP(SM,5)
#define RTTPSM_CURRENT_COUNTS			RTTP(SM,6)
#define RTTPSM_CLIENT_UPDATE			RTTP(SM,7)
#define RTTPSM_UPDATE_CONFIG			RTTP(SM,8)
#define RTTPSM_ERROR_MESSAGE			RTTP(SM,9)
#define RTTPSM_PASSWORD				RTTP(SM,10)
#define RTTPSM_PASSWORD_REPLY			RTTP(SM,11)
#define RTTPSM_REQUEST_FORK_INFO		RTTP(SM,12)
#define RTTPSM_SERVER_ID_REQUEST		RTTP(SM,13)
#define RTTPSM_SERVER_ID			RTTP(SM,14)
#define RTTPSM_LICENSE_REQUEST			RTTP(SM,15)
#define RTTPSM_LICENSE				RTTP(SM,16)
#define RTTPSM_REQUEST_PEAK_TIME		RTTP(SM,17)
#define RTTPSM_CURRENT_PEAK_TIME		RTTP(SM,18)

/*
 * Constants for server<->monitor messages.
 */

enum RTTPSM_error_opcode 
{
	RTTPSM_ERROR_PASSWORD_OPCODE,
	RTTPSM_ERROR_INVALID_PASSWORD,
	RTTPSM_ERROR_CONFIG,
	RTTPSM_ERROR_INVALID_LICENSE
};

const short RTTP2_MAX_CFG_VAR_NAME_LEN		    = 256;
const short RTTP2SM_MAX_MONITOR_PASSWORD_LEN	    = 256;
const short RTTP2SM_MAX_HOSTNAME_LEN		    = 1024;
const short RTTP2SM_MAX_URL_LEN			    = 1024;
const short RTTP2SM_MAX_ERROR_STRING_LEN	    = 1024;
const short RTTP2SM_CURRENT_PEAK_TIME_STATIC_SIZE   = 16;
const short RTTP2SM_REQUEST_PEAK_TIME_STATIC_SIZE   = 16;

%}

RTTPSM_server_id_request : rttp2_header
{
}

RTTPSM_server_id : rttp2_header
{
    String	    server_id;
}

RTTPSM_password : rttp2_header
{
    Buffer[]	    password;
}

RTTPSM_license_request : rttp2_header
{
}

RTTPSM_license : rttp2_header
{
    String	    license;
}

RTTPSM_request_peak_time : rttp2_header
{
    Int8	    do_reset;
}

RTTPSM_current_peak_time : rttp2_header
{
    Int32	    time_since_last_reset;
}

%{
#endif /* _RTTP2SM_H_ */
%}
