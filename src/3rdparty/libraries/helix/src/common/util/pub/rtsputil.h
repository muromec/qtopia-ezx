/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsputil.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _RTSPUTIL_H_
#define _RTSPUTIL_H_

/*
 * Function: 
 * 		BinTo64
 *
 * Purpose:
 *		Encode binary data into text using 64 characters from
 *		the ANSI character set a base 64 values. Essentially
 *		encodes 24 bits (3 bytes) into 4 6-bit base64 characters.
 *
 * Parameters:	
 *		pInBuf - pointer to buffer of binary data to be encoded
 *              len - length of data to be encoded
 *              pOutBuf - pointer to output buffer
 *
 * Returns:
 *		length of encoded buffer
 *
 * Notes: 
 *		The output buffer should be about 33% larger than the input
 *        	buffer to hold the encoded data. Text is broken up into
 *              72 character lines.
 */
INT32 BinTo64(const BYTE* pInBuf, INT32 len, char* pOutBuf);

/*
 * Function: 
 * 		BinToURL64
 *
 * Purpose:
 *		Encode binary data into text using 64 characters from
 *		the ANSI character set a base 64 values. Essentially
 *		encodes 24 bits (3 bytes) into 4 6-bit base64 characters.
 *
 *              Note: This function differs from BinTo64 in that the '*' and
 *              '!' characters are used instead of the "/" and "+" chars. 
 *              This is useful if the encoded string is going to be used as
 *              a URL parameter. The same BinFrom64 function can be used to
 *              decode both encodings.
 *
 * Parameters:	
 *		pInBuf - pointer to buffer of binary data to be encoded
 *              len - length of data to be encoded
 *              pOutBuf - pointer to output buffer
 *
 * Returns:
 *		length of encoded buffer
 *
 * Notes: 
 *		The output buffer should be about 33% larger than the input
 *        	buffer to hold the encoded data. Text is broken up into
 *              72 character lines.
 */
INT32 BinToURL64(const BYTE* pInBuf, INT32 len, char* pOutBuf);

/*
 * Function:
 *		BinFrom64
 *
 * Purpose:
 *		Decode base 64 text back into binary.
 *
 * Parameters:
 *		pInBuf - pointer to buffer of encoded data
 *		len - length of data to be decoded
 *		pOutBuf - pointer to output buffer
 *
 * Returns:
 *		length of decoded buffer or -1 if invalid input
 *
 * Notes:
 *
 */
INT32 BinFrom64(const char* pInBuf, INT32 len, BYTE* pOutBuf);

/*
 * Function:
 *		EncodeCString
 *
 * Purpose:
 *		Add escape ('\') characters so strings containing
 *		control characters can be used as data without
 *		losing the control character interpretation.
 *
 * Parameters:
 *		pInString - C-style zero terminated string to encode
 *
 * Returns:
 *		Encoded string
 *
 */		
const char* EncodeCString(const char* pInString);

/*
 * Function: 
 * 		URLEscapeBuffer
 *
 * Purpose:
 *		URL-Escapes the input buffer, translating illegal URL
 *		characters into a % sign followed by the hex value of
 *		that character.
 *
 * Parameters:	
 *		pInBuf - pointer to buffer of printable data to be encoded
 *              len - length of data to be encoded
 *              pOutBuf - pointer to output buffer
 *
 * Returns:
 *		length of encoded buffer
 *
 * Notes: 
 *		The output buffer should be 3 times larger than the input
 *        	buffer to hold the encoded data. Each illegal URL character
 *              will be translated to 3 characters (i.e. @ -> %40). The
 *		output will not be NULL terminated.
 */
INT32 URLEscapeBuffer(const char* pInBuf, INT32 len, char* pOutBuf);

/*
 * Function: 
 * 		URLEscapeBuffer2
 *
 * Purpose:
 * 		See RFC 1738.  This function should do proper url escaping.
 *
 * Parameters:	
 *		pInBuf - pointer to buffer of printable data to be encoded
 *              len - length of data to be encoded
 *              pOutBuf - pointer to output buffer
 *
 *              bReserved - Should reserved values be escaped in this string?
 *
 * Returns:
 *		length of encoded buffer
 *
 * Notes:
 * 		See RFC1738
 *		"Thus, only alphanumerics, the special characters "$-_.+!*'(),",
 *		and reserved characters used for their reserved purposes may be
 *		used unencoded within a URL."
 *
 *		Reserved characters: ";", "/", "?", ":", "@", "=" and "&" 
 *
 */
INT32 URLEscapeBuffer2(const char* pInBuf, INT32 len, char* pOutBuf, 
		       HXBOOL bReserved);

/*
 * Function:
 *		URLUnescapeBuffer
 *
 * Purpose:
 *		URL-Unescapes the input buffer, translating escaped 
 *		characters to their normal representation (i.e. %40 -> @).
 *
 * Parameters:
 *		pInBuf - pointer to buffer of escaped data
 *		len - length of data to be decoded
 *		pOutBuf - pointer to output buffer
 *
 * Returns:
 *		length of decoded buffer or -1 if invalid input
 *
 * Notes:
 *		The output buffer must be at least the same size as the 
 *		input buffer, to handle the case in which the input buffer
 *		contains no escaped characters. The output will not be NULL
 *		terminated.
 */
INT32 URLUnescapeBuffer(const char* pInBuf, INT32 len, char* pOutBuf);

#endif /* _RTSPUTIL_H_ */
