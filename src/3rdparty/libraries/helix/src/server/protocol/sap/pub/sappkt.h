/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sappkt.h,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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
 * This is generated code, do not modify. Look in sappkt.pm to
 * make modifications
 */
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 UINT32	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/


#ifndef _SAPPKT_H_
#define _SAPPKT_H_
class SapPacket
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT8	version;
    UINT8	msg_type;
    UINT8	encryption_bit;
    UINT8	compressed_bit;
    UINT8	authentication_len;
    UINT16	msg_id_hash;
    UINT32	orig_src;
    UINT32	*op_authentication_header;
    INT32	op_key_id;
    INT32	op_timeout;
    UINT8	op_encryption_padding;
    UINT32	op_random;
    buffer	text_payload;
};

inline UINT8*
SapPacket::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {
	*off &= ~0xe0; *off |= (version&0x7)<<5;
    }
    {
	*off &= ~0x1c; *off |= (msg_type&0x7)<<2;
    }
    {*off &= ~(1<<1); *off |= (encryption_bit&1)<<1;}
    {*off &= ~1; *off++ |= compressed_bit&1;}
    *off++ = authentication_len;
    {*off++ = (UINT8) (msg_id_hash>>8); *off++ = (UINT8) (msg_id_hash);}
    {
	*off++ = (UINT8) (orig_src>>24); *off++ = (UINT8) (orig_src>>16);
	*off++ = (UINT8) (orig_src>>8); *off++ = (UINT8) (orig_src);
    }
    if ((authentication_len > 0))
    {
      {for (int i = 0;  i < authentication_len; i++)
	  {
	      *off++ = (UINT8) (op_authentication_header[i]>>24); *off++ = (UINT8) (op_authentication_header[i]>>16);
	      *off++ = (UINT8) (op_authentication_header[i]>>8); *off++ = (UINT8) (op_authentication_header[i]);
	  }
      }
    }
    if ((encryption_bit == 1))
    {
      {
	  *off++ = (UINT8) (op_key_id>>24); *off++ = (UINT8) (op_key_id>>16);
	  *off++ = (UINT8) (op_key_id>>8); *off++ = (UINT8) (op_key_id);
      }
      {
	  *off++ = (UINT8) (op_timeout>>24); *off++ = (UINT8) (op_timeout>>16);
	  *off++ = (UINT8) (op_timeout>>8); *off++ = (UINT8) (op_timeout);
      }
      {*off &= ~(1<<7); *off |= (op_encryption_padding&1)<<7;}
      {
	  *off &= ~0x7f; *off++ |= (op_random>>24)&0x7f;
	  *off++ = (UINT8) (op_random>>16);
	  *off++ = (UINT8) (op_random>>8);
	  *off++ = (UINT8) (op_random);
      }
    }
    {
	if (text_payload.len < 0)
	    return 0;
	memcpy(off, text_payload.data, text_payload.len); off += text_payload.len;
    }
    len = off-buf;
    return off;
}

inline UINT8*
SapPacket::unpack(UINT8* buf, UINT32 len)
{
    if (len <= 0)
	return 0;
    UINT8* off = buf;

    {
	version  = (*off&0xe0)>>5;
    }
    {
	msg_type  = (*off&0x1c)>>2;
    }
    encryption_bit = (*off>>1)&1;
    compressed_bit = *off++&1;
    authentication_len = *off++;
    {msg_id_hash = *off++<<8; msg_id_hash |= *off++;}
    {
	orig_src = ((UINT32)*off++)<<24; orig_src |= ((UINT32)*off++)<<16;
	orig_src |= ((UINT32)*off++)<<8; orig_src |= ((UINT32)*off++);
    }
    if ((authentication_len > 0))
    {
      {
	  op_authentication_header = new UINT32[authentication_len];
	  for (int i = 0;  i < authentication_len; i++)
	  {
	      op_authentication_header[i] = ((UINT32)*off++)<<24; op_authentication_header[i] |= ((UINT32)*off++)<<16;
	      op_authentication_header[i] |= ((UINT32)*off++)<<8; op_authentication_header[i] |= ((UINT32)*off++);
	  }
      }
    }
    if ((encryption_bit == 1))
    {
      {
	  op_key_id = ((INT32)*off++)<<24; op_key_id |= ((INT32)*off++)<<16;
	  op_key_id |= ((INT32)*off++)<<8; op_key_id |= ((INT32)*off++);
      }
      {
	  op_timeout = ((INT32)*off++)<<24; op_timeout |= ((INT32)*off++)<<16;
	  op_timeout |= ((INT32)*off++)<<8; op_timeout |= ((INT32)*off++);
      }
      op_encryption_padding = (*off>>7)&1;
      {
	  op_random  = (*off++&0x7f)<<24;
	  op_random |= *off++<<16;
	  op_random |= *off++<<8;
	  op_random |= *off++;
      }
    }
    {
	text_payload.len = len - (off - buf);
	if (text_payload.len < 0 || off-buf+text_payload.len > (int)len)
	    return 0;
	text_payload.data = (INT8 *)off; off += text_payload.len;
    }
    return off;
}

#endif /* _SAPPKT_H_ */
