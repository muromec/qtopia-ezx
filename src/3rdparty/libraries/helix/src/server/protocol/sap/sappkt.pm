%{
#ifndef _SAPPKT_H_
#define _SAPPKT_H_
%}
/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sappkt.pm,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

struct SapPacket
{
    bit[3]		    version;
    bit[3]		    msg_type;
    bit[1]		    encryption_bit;
    bit[1]		    compressed_bit;
    u_int8		    authentication_len;	    
    u_int16		    msg_id_hash;
    u_int32		    orig_src;
    if(authentication_len > 0)
    {
	    /* optional */
	    u_int32[authentication_len]	op_authentication_header;
	}
	if(encryption_bit == 1)
	{   
	    /* optional */
	    int32	    op_key_id;
		int32	    op_timeout;
		bit[1]	    op_encryption_padding;
		bit[31]	    op_random;
	}
	/* sdp */
	buffer		    text_payload;
}

%{
#endif /* _SAPPKT_H_ */
%}
