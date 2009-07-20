/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpclk.h,v 1.3 2007/07/06 20:51:33 jfinnecy Exp $
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
 * This is generated code, do not modify. Look in httpclk.pm to
 * make modifications
 */
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 INT16	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/


#ifndef _HTTPCLK_H_
#define _HTTPCLK_H_

enum HTTPOpcodes
{
    HTTP_RV_RESPONSE		= 'r',
    HTTP_RESPONSE		= 'H',
    HTTP_OPTION_RESPONSE	= 'O',
    HTTP_OPT_RESPONSE_END	= 'e',
    HTTP_POSTDONE		= 'h', /*pna_http_post_done*/
    HTTP_DONE			= 'D'  /*pna_http_done*/

};

enum HTTPOptionOpcodes
{
    HTTP_OPTION_SHORT_PADDING	= 1
};

class CloakedHTTPResponse
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 4;}

    UINT8	opcode;
    UINT8	length;
    UINT8	status;
    UINT8	padding;
};

inline UINT8*
CloakedHTTPResponse::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    opcode = HTTP_RESPONSE;
    *off++ = opcode;
    length = 2;
    *off++ = length;
    *off++ = status;
    padding = 0;
    *off++ = padding;
    len = off-buf;
    return off;
}

inline UINT8*
CloakedHTTPResponse::unpack(UINT8* buf, UINT32 len)
{
    if (len <= 0)
	return 0;
    UINT8* off = buf;

    opcode = *off++;
    length = *off++;
    status = *off++;
    padding = *off++;
    return off;
}
class CloakedHTTPRVResponse
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 4;}

    UINT8	opcode;
    UINT16	length;
    UINT8	status;
};

inline UINT8*
CloakedHTTPRVResponse::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    opcode = HTTP_RV_RESPONSE;
    *off++ = opcode;
    length = 1;
    {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    *off++ = status;
    len = off-buf;
    return off;
}

inline UINT8*
CloakedHTTPRVResponse::unpack(UINT8* buf, UINT32 len)
{
    if (len <= 0)
	return 0;
    UINT8* off = buf;

    opcode = *off++;
    {length = *off++<<8; length |= *off++;}
    status = *off++;
    return off;
}

#endif /* _HTTPCLK_H_ */
