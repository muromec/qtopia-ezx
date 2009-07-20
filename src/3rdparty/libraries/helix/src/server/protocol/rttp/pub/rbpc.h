/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rbpc.h,v 1.3 2003/03/04 06:13:06 dcollins Exp $ 
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
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

#include "rttp2.h"
#include "hxassert.h"

#ifndef _AIX
typedef char int8;
typedef short int16;
typedef long int32;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned long u_int32;
#endif
typedef char*	pmc_string;

struct buffer {
	 int16	len;
	 int8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/



/*
 * This is generated code, do not modify. Look in rttp2c.pm to make
 * modifications.
 */

#ifndef RBPC_H_
#define RBPC_H_

#define RBPC_ERROR			RTTP(C,0)
#define RTTP2_HELLO			RTTP(C,1)
#define RBPC_HELLO			RTTP(C,2)
#define RBPC_ID				RTTP(C,3)
#define RBPC_ID_RESP			RTTP(C,4)

const int DEFAULT_SMALL_RBP_BUF_SIZE	= 256;
const int RTTP_KT_PN_CHALLENGE_SIZE	= 32;


enum Key_types
{
    RTTP_KT_NONE	= 0,
    RTTP_KT_PASSWORD,
    RTTP_KT_PN_CHALLENGE
};  

class Header
{
public:
    u_int8*	pack(u_int8* buf, int &len);
    u_int8*	unpack(u_int8* buf, unsigned int len);
    const int	static_size() {return 4;}

    int16 opcode;
    int16 length;
};

inline u_int8*
Header::pack(u_int8* buf, int &len)
{
    u_int8* off = buf+4;

    buf[0] = opcode>>8; buf[1] = opcode&0xff;
    int _len = off - buf - 4;
    if (_len&3)
    {
	*buf |= 0x80; off += 4 - (_len&3); off[-1] = 4 - (_len&3);
	_len += 4 - (_len&3);
	ASSERT ((_len + 4) <= len);
    }
    buf[2] = _len>>10; buf[3] = (_len>>2)&0xff;
    len = off-buf;
    return off;
}

inline u_int8*
Header::unpack(u_int8* buf, unsigned int len)
{
    if (len <= 0 || len < (unsigned)static_size())
	return 0;
    u_int8* off = buf+4;

    opcode = (*buf&0x7f)<<8; opcode |= buf[1];
    unsigned int _len = buf[2]<<8; _len |= buf[3]; _len <<= 2;
    length = _len;
    if (*buf & 0x80)
    {
	if (_len+4 > len)
	    return 0;
	int padbytes = off[_len-1];
	if (padbytes < 1 || padbytes > 3)
	    return 0;
	_len -= padbytes;
    }
    len = _len+4;
    off = buf + (((off-buf)+3)&~3);return off;
}
class RBPC_hello
{
public:
    u_int8*	pack(u_int8* buf, int &len);
    u_int8*	unpack(u_int8* buf, unsigned int len);
    const int	static_size() {return 7;}

    int16 opcode;
    int16 length;
    int8	subprotocol;
    u_int8	major_version;
    u_int8	minor_version;
    buffer	id;
};

inline u_int8*
RBPC_hello::pack(u_int8* buf, int &len)
{
    u_int8* off = buf+4;

    opcode = RBPC_HELLO;
    buf[0] = opcode>>8; buf[1] = opcode&0xff;
    *off++ = subprotocol;
    {
	*off &= ~0xc0; *off |= (major_version&0x3)<<6;
    }
    {
	*off &= ~0x3f; *off++ |= minor_version&0x3f;
    }
    {
	*off++ = 0;
	*off++ = 0;
    }
    {
	if (id.len < 0)
	    return 0;
	memcpy(off, id.data, id.len); off += id.len;
    }
    int _len = off - buf - 4;
    if (_len&3)
    {
	*buf |= 0x80; off += 4 - (_len&3); off[-1] = 4 - (_len&3);
	_len += 4 - (_len&3);
	ASSERT ((_len + 4) <= len);
    }
    buf[2] = _len>>10; buf[3] = (_len>>2)&0xff;
    len = off-buf;
    return off;
}

inline u_int8*
RBPC_hello::unpack(u_int8* buf, unsigned int len)
{
    if (len <= 0 || len < (unsigned)static_size())
	return 0;
    u_int8* off = buf+4;

    opcode = (*buf&0x7f)<<8; opcode |= buf[1];
    unsigned int _len = buf[2]<<8; _len |= buf[3]; _len <<= 2;
    length = _len;
    if (*buf & 0x80)
    {
	if (_len+4 > len)
	    return 0;
	int padbytes = off[_len-1];
	if (padbytes < 1 || padbytes > 3)
	    return 0;
	_len -= padbytes;
    }
    len = _len+4;
    subprotocol = *off++;
    {
	major_version  = (*off&0xc0)>>6;
    }
    {
	minor_version  = *off++&0x3f;
    }
    {
	off++;
	off++;
    }
    {
	id.len = len - (off - buf);
	if (id.len < 0 || off-buf+id.len > (int)len)
	    return 0;
	id.data = (int8 *)off; off += id.len;
    }
    off = buf + (((off-buf)+3)&~3);return off;
}
class RBPC_id
{
public:
    u_int8*	pack(u_int8* buf, int &len);
    u_int8*	unpack(u_int8* buf, unsigned int len);
    const int	static_size() {return 6;}

    int16 opcode;
    int16 length;
    int16	key_type;
    buffer	key;
};

inline u_int8*
RBPC_id::pack(u_int8* buf, int &len)
{
    u_int8* off = buf+4;

    opcode = RBPC_ID;
    buf[0] = opcode>>8; buf[1] = opcode&0xff;
    {*off++ = (u_int8) (key_type>>8); *off++ = (u_int8) (key_type);}
    {*off++ = 0; *off++ = 0;}
    {
	if (key.len < 0)
	    return 0;
	memcpy(off, key.data, key.len); off += key.len;
    }
    int _len = off - buf - 4;
    if (_len&3)
    {
	*buf |= 0x80; off += 4 - (_len&3); off[-1] = 4 - (_len&3);
	_len += 4 - (_len&3);
	ASSERT ((_len + 4) <= len);
    }
    buf[2] = _len>>10; buf[3] = (_len>>2)&0xff;
    len = off-buf;
    return off;
}

inline u_int8*
RBPC_id::unpack(u_int8* buf, unsigned int len)
{
    if (len <= 0 || len < (unsigned)static_size())
	return 0;
    u_int8* off = buf+4;

    opcode = (*buf&0x7f)<<8; opcode |= buf[1];
    int _len = buf[2]<<8; _len |= buf[3]; _len <<= 2;
    if (_len < 0)
	return 0;
    length = _len;
    if (*buf & 0x80)
    {
	int padbytes = off[_len-1];
	if (padbytes < 1 || padbytes > 3)
	    return 0;
	_len -= padbytes;
    }
    len = _len+4;
    {key_type = *off++<<8; key_type |= *off++;}
    off += 2;
    {
	key.len = len - (off - buf);
	if (key.len < 0 || off-buf+key.len > (int)len)
	    return 0;
	key.data = (int8 *)off; off += key.len;
    }
    off = buf + (((off-buf)+3)&~3);return off;
}
class RBPC_id_resp
{
public:
    u_int8*	pack(u_int8* buf, int &len);
    u_int8*	unpack(u_int8* buf, unsigned int len);
    const int	static_size() {return 6;}

    int16 opcode;
    int16 length;
    int16	key_type;
    buffer	key;
};

inline u_int8*
RBPC_id_resp::pack(u_int8* buf, int &len)
{
    u_int8* off = buf+4;

    opcode = RBPC_ID_RESP;
    buf[0] = opcode>>8; buf[1] = opcode&0xff;
    {*off++ = (u_int8) (key_type>>8); *off++ = (u_int8) (key_type);}
    {*off++ = 0; *off++ = 0;}
    {
	if (key.len < 0)
	    return 0;
	memcpy(off, key.data, key.len); off += key.len;
    }
    int _len = off - buf - 4;
    if (_len&3)
    {
	*buf |= 0x80; off += 4 - (_len&3); off[-1] = 4 - (_len&3);
	_len += 4 - (_len&3);
	ASSERT ((_len + 4) <= len);
    }
    buf[2] = _len>>10; buf[3] = (_len>>2)&0xff;
    len = off-buf;
    return off;
}

inline u_int8*
RBPC_id_resp::unpack(u_int8* buf, unsigned int len)
{
    if (len <= 0 || len < (unsigned)static_size())
	return 0;
    u_int8* off = buf+4;

    opcode = (*buf&0x7f)<<8; opcode |= buf[1];
    int _len = buf[2]<<8; _len |= buf[3]; _len <<= 2;
    if (_len < 0)
	return 0;
    length = _len;
    if (*buf & 0x80)
    {
	int padbytes = off[_len-1];
	if (padbytes < 1 || padbytes > 3)
	    return 0;
	_len -= padbytes;
    }
    len = _len+4;
    {key_type = *off++<<8; key_type |= *off++;}
    off += 2;
    {
	key.len = len - (off - buf);
	if (key.len < 0 || off-buf+key.len > (int)len)
	    return 0;
	key.data = (int8 *)off; off += key.len;
    }
    off = buf + (((off-buf)+3)&~3);return off;
}

#endif /* RBPC_H_ */
