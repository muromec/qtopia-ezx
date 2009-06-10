/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rttp2sm.h,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

#define RTTPSM_CLIENT_ADDED		'I'
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
#define RTTPSM_STRING_REQUEST			RTTP(SM,19)
#define RTTPSM_STRING_RESPONSE			RTTP(SM,20)
#define RTTPSM_INTEGER_REQUEST			RTTP(SM,21)
#define RTTPSM_INTEGER_RESPONSE			RTTP(SM,22)
#define RTTPSM_BUFFER_REQUEST			RTTP(SM,23)
#define RTTPSM_BUFFER_RESPONSE			RTTP(SM,24)

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

class RTTPSM_server_id_request : public RTTP2_msg
{
public:
    void		get();
    void		set();
};

inline
void RTTPSM_server_id_request::get()
{
}

inline
void RTTPSM_server_id_request::set()
{
    memset(msg, 0, 4);
    set_opcode((Int16) RTTPSM_SERVER_ID_REQUEST);
    set_length(0);
    msg_len = 4;
}

class RTTPSM_server_id : public RTTP2_msg
{
public:
    void		get(char*& server_id);
    void		set(const char* server_id);
};

inline
void RTTPSM_server_id::get(char*& server_id)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    server_id = (char*) msg+msg_offset;
    msg_offset += strlen(server_id) + 1;
}

inline
void RTTPSM_server_id::set(const char* server_id)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    strcpy((char*)msg+msg_offset, server_id);
    msg_offset += strlen(server_id) + 1;
    set_opcode((Int16) RTTPSM_SERVER_ID);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSM_password : public RTTP2_msg
{
public:
    void		get(Byte*& password, Int16& password_len);
    void		set(Byte* password, Int16 password_len);
};

inline
void RTTPSM_password::get(Byte*& password, Int16& password_len)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    password_len = getshort(msg+msg_offset);
    password = (Byte*)msg+msg_offset+ 2;
    msg_offset += password_len + 2;
}

inline
void RTTPSM_password::set(Byte* password, Int16 password_len)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    putshort(msg+msg_offset, password_len);
    memcpy(msg+msg_offset + 2, password, password_len);
    msg_offset += password_len + 2;
    set_opcode((Int16) RTTPSM_PASSWORD);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSM_license_request : public RTTP2_msg
{
public:
    void		get();
    void		set();
};

inline
void RTTPSM_license_request::get()
{
}

inline
void RTTPSM_license_request::set()
{
    memset(msg, 0, 4);
    set_opcode((Int16) RTTPSM_LICENSE_REQUEST);
    set_length(0);
    msg_len = 4;
}

class RTTPSM_license : public RTTP2_msg
{
public:
    void		get(char*& license);
    void		set(const char* license);
};

inline
void RTTPSM_license::get(char*& license)
{
    Int32 msg_offset = 0;
    msg_offset += 2;
    msg_offset += 2;
    license = (char*) msg+msg_offset;
    msg_offset += strlen(license) + 1;
}

inline
void RTTPSM_license::set(const char* license)
{
    Int32 msg_offset = 0;
    memset(msg, 0, 4);
    msg_offset += 2;
    msg_offset += 2;
    strcpy((char*)msg+msg_offset, license);
    msg_offset += strlen(license) + 1;
    set_opcode((Int16) RTTPSM_LICENSE);
    msg_align(msg_offset);
    set_length(msg_offset-4);
    msg_len = msg_offset;
}

class RTTPSM_request_peak_time : public RTTP2_msg
{
public:
    void		get(Int8& do_reset);
    void		set(Int8 do_reset);
};

inline
void RTTPSM_request_peak_time::get(Int8& do_reset)
{
    do_reset = *(msg+4);
}

inline
void RTTPSM_request_peak_time::set(Int8 do_reset)
{
    memset(msg, 0, 4);
    *(msg+4) = do_reset;
    set_opcode((Int16) RTTPSM_REQUEST_PEAK_TIME);
    set_length(4);
    msg_len = 8;
}

class RTTPSM_current_peak_time : public RTTP2_msg
{
public:
    void		get(Int32& time_since_last_reset);
    void		set(Int32 time_since_last_reset);
};

inline
void RTTPSM_current_peak_time::get(Int32& time_since_last_reset)
{
    time_since_last_reset = getlong(msg+4);
}

inline
void RTTPSM_current_peak_time::set(Int32 time_since_last_reset)
{
    memset(msg, 0, 4);
    putlong(msg+4, time_since_last_reset);
    set_opcode((Int16) RTTPSM_CURRENT_PEAK_TIME);
    set_length(4);
    msg_len = 8;
}

#endif /* _RTTP2SM_H_ */
