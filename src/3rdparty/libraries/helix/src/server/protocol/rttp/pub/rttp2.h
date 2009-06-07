/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rttp2.h,v 1.4 2004/06/02 21:16:18 tmarshall Exp $
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
#ifndef _RTTP_H_
#define _RTTP_H_

/*
 * RealTime Transport Protocol
 *
 * NB: this file *must* be usable from C files.
 */

#include "hlxclib/sys/socket.h"

#ifdef _UNIX
#include <arpa/inet.h>
#endif /* _UNIX */

#include "hxmarsh.h"

#define RTTP_VERSION         1
#define RTTP_REVISION        0

const int RBP_MAJOR_VERSION = 0;
const int RBP_MINOR_VERSION = 0;

/* RTTP speaking entity ids */
#define RTTPE_UNKNOWN                   0
#define RTTPE_SERVER                    1
#define RTTPE_MONITOR                   2
#define RTTPE_ENCODER                   3
#define RTTPE_REMOTE_LICENSE            4
#define RTTPE_FIREWALL                  5
#define RTTPE_EVENTGEN                  6
#define RTTPE_RESOLVER                  7
#define RTTPE_LIVESRC                   8
#define RTTPE_PLAYER                    9
#define RTTPE_REMOTE_LICENSE_CLIENT     10
#define RTTPE_RESOLVER_CLIENT           11
#define RTTPE_MIXER                     12
#define RTTPE_LICENSE                   13
#define RTTPE_SPLITTER_PLAYER           14
#define RTTPE_BROWSER                   15

/* Increment this when you add an entity to the above list. */
#define RTTPE_NUM_ENTITIES              16

/* Sub-protocol ids */
#define RTTP_C          0               /* Common protocol -- spoken by all */
#define RTTP_SP         1               /* Server <-> Player */
#define RTTP_SL         2               /* Server <-> Resolver */
#define RTTP_SN         3               /* Server <-> Encoder */
#define RTTP_SR         4               /* Server <-> Relay */
#define RTTP_SE         5               /* Server <-> Event generator */
#define RTTP_RP         6               /* Relay <-> Player */
#define RTTP_PF         7               /* Player <-> Firewall */
#define RTTP_SM         8               /* Server <-> Monitor */
#define RTTP_SK         9               /* Server <-> License manager */
#define RTTP_SS         10              /* Server <-> Server */

#define RTTP_SUB_SHIFT  10
#define RTTP(sub,code)  (((RTTP_##sub)<<RTTP_SUB_SHIFT)+code)

/* RTTP message field composing/decomposing macros */
#define RTTP_len(code)          (((code) < 0? (code)&0xffff : ((code)&0xf))*4)
#define RTTP_op(code)           (((code)>>16)&0x7fff)
#define RTTP_code(op,len)       (((op)<<16)|((len)>63? 0x80000000:0)|((len)/4))
#define RTTP_setcode(op,str)    ((str).code = RTTP_code(op,sizeof (str) - 4))

/* RTTP message constants */
#define RTTP2_COMMON_HEADER_LEN         4
#define RTTP2_BYTE_BOUNDARY             4
#define RTTP2_OPCODE_LEN                2

struct RTTP_msg {
        UINT32                code;
};

/*
 * Common protocol
 */
#define RTTPC_ERROR             RTTP(C,0)
#define RTTPC_HELLO             RTTP(C,1)
#define OLD_RTTPC_HELLO         RTTP(C,1)
#define RTTPC_ID                RTTP(C,2)
#define RTTPC_ID_RESP           RTTP(C,3)
#define RTTPC_REDIRECT          RTTP(C,4)
#define RTTPC_EVENT             RTTP(C,5)


#define RTTP_VERSION_SHIFT      12
#define RTTP_TYPE_SHIFT         6

struct RTTP_hello: RTTP_msg {
        UINT32        key[2];
                        RTTP_hello(int type, int version);
};

inline
RTTP_hello::RTTP_hello(int type, int version) {
        code = htonl(RTTP_code(RTTPC_HELLO, sizeof *this - 4) |
                     (type<<RTTP_TYPE_SHIFT) | (version<<RTTP_VERSION_SHIFT));
}

/*
 * Other subprotocols should be defined in their own header file.
 * This will allow upgrading individual subprotocols without
 * affecting unrelated entities.
 */

/*
 * Data types and lengths for the protocol.
 *
 */

const short RTTP_SHORT_LEN = 2;
const short RTTP_LONG_LEN  = 4;

/*
 *
 * The base class for the new RTTP2 implementation. When this is completely
 * phased in, a lot of the above stuff needs to be removed.
 *
 */

class Protocol_msg
{
public:
    void                init(Byte* new_buf);
    void                init(Byte* new_buf, UINT32 new_len);

    Byte*               msg;
    UINT32            msg_len;
};

class RTTP2_msg : public Protocol_msg
{
public:
    void                set_length(UINT32 new_len);
    INT32               get_length();
    INT32               get_na_length();
    void                set_opcode(INT16 new_opcode);
    INT16               get_opcode();
    void                msg_align(INT32& length);
};

inline
void Protocol_msg::init(Byte* new_buf)
{
    msg = new_buf;
}

inline
void Protocol_msg::init(Byte* new_buf, UINT32 new_len)
{
    msg = new_buf;
    msg_len = new_len;
}

inline
void RTTP2_msg::msg_align(INT32& length)
{
    if ( (length % 4) != 0)
      {
        length += (4 - (length % 4));
      }
}

inline
void RTTP2_msg::set_length(UINT32 new_len)
{
    char        padding_len;

    padding_len = (char) (4 - (new_len % 4));
    if (padding_len < 4)
    {
        new_len += padding_len;
        msg[new_len] = padding_len;
    }
    new_len >>= 2;
    putshort(msg+2, (unsigned short)new_len);
}

inline
INT32 RTTP2_msg::get_length()
{
    return (getshort(msg+2) << 2);
}

inline
INT32 RTTP2_msg::get_na_length()
{
    INT16   opcode = (INT16) getshort(msg);
    if (opcode & 0x8000)
    {
        Byte    padding_len;
        INT32   length;
        length = get_length();
        padding_len = *(msg + length);
        return (length - padding_len);
    }

    return get_length();
}

inline
void RTTP2_msg::set_opcode(INT16 new_opcode)
{
   // INT32* opcode = (INT32*) msg;
   // (*opcode) = htonl( (ntohl(*opcode) & 0x0000ffff) | (new_opcode << 16));
    putshort(msg, new_opcode);
}

inline
INT16 RTTP2_msg::get_opcode()
{
    INT32  opcode = ntohl(*(INT32*) msg);
    return ((INT16)(opcode >> 16)) & 0x7fff;
}



#endif/*_RTTP_H_*/
