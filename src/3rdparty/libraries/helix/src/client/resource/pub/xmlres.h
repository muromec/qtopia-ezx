/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlres.h,v 1.3 2007/07/06 21:58:27 jfinnecy Exp $
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

#ifndef _XMLRES_H_
#define _XMLRES_H_

#include "resid.h"

#define IDS_ERR_XML_GENERALERROR    HX_XML_RES_INIT_ID +   0
#define IDS_ERR_XML_BADENDTAG	    HX_XML_RES_INIT_ID +   1
#define IDS_ERR_XML_NOCLOSE	    HX_XML_RES_INIT_ID +   2
#define IDS_ERR_XML_BADATTRIBUTE    HX_XML_RES_INIT_ID +   3
#define IDS_ERR_XML_NOVALUE	    HX_XML_RES_INIT_ID +   4
#define IDS_ERR_XML_MISSINGQUOTE    HX_XML_RES_INIT_ID +   5
#define IDS_ERR_XML_NOTAGTYPE	    HX_XML_RES_INIT_ID +   6
#define IDS_ERR_XML_ILLEGALID	    HX_XML_RES_INIT_ID +   7
#define IDS_ERR_XML_DUPATTRIBUTE    HX_XML_RES_INIT_ID +   8
#define IDS_ERR_XML_COMMENT_B4_PROCINST HX_XML_RES_INIT_ID +   9

#define IDS_ERR_XML_SYNTAX				HX_XML_RES_INIT_ID +    11
#define IDS_ERR_XML_NO_ELEMENTS				HX_XML_RES_INIT_ID +    12
// replaced INVALID_TOKEN WITH specific errors in 100s below
//#define IDS_ERR_XML_INVALID_TOKEN			HX_XML_RES_INIT_ID +    13
#define IDS_ERR_XML_UNCLOSED_TOKEN			HX_XML_RES_INIT_ID +    14
#define IDS_ERR_XML_PARTIAL_CHAR			HX_XML_RES_INIT_ID +    15
#define IDS_ERR_XML_TAG_MISMATCH			HX_XML_RES_INIT_ID +    16
//use DUPATTRIBUTE instead
//#define IDS_ERR_XML_DUPLICATE_ATTRIBUTE			HX_XML_RES_INIT_ID +    17
#define IDS_ERR_XML_JUNK_AFTER_DOC_ELEMENT		HX_XML_RES_INIT_ID +    18
#define IDS_ERR_XML_PARAM_ENTITY_REF			HX_XML_RES_INIT_ID +    19
#define IDS_ERR_XML_UNDEFINED_ENTITY			HX_XML_RES_INIT_ID +    20
#define IDS_ERR_XML_RECURSIVE_ENTITY_REF		HX_XML_RES_INIT_ID +    21
#define IDS_ERR_XML_ASYNC_ENTITY			HX_XML_RES_INIT_ID +    22
#define IDS_ERR_XML_BAD_CHAR_REF			HX_XML_RES_INIT_ID +    23
#define IDS_ERR_XML_BINARY_ENTITY_REF			HX_XML_RES_INIT_ID +    24
#define IDS_ERR_XML_ATTRIBUTE_EXTERNAL_ENTITY_REF	HX_XML_RES_INIT_ID +    25
#define IDS_ERR_XML_MISPLACED_XML_PI			HX_XML_RES_INIT_ID +    26
#define IDS_ERR_XML_UNKNOWN_ENCODING			HX_XML_RES_INIT_ID +    27
#define IDS_ERR_XML_INCORRECT_ENCODING			HX_XML_RES_INIT_ID +    28
#define IDS_ERR_XML_UNCLOSED_CDATA_SECTION		HX_XML_RES_INIT_ID +    29
#define IDS_ERR_XML_EXTERNAL_ENTITY_HANDLING		HX_XML_RES_INIT_ID +    30
#define IDS_ERR_XML_NOT_STANDALONE			HX_XML_RES_INIT_ID +    31

#define IDS_ERR_XML_INVALID_NAME			HX_XML_RES_INIT_ID +    100
#define IDS_ERR_XML_INVALID_CHAR_IN_DOC			HX_XML_RES_INIT_ID +    101
#define IDS_ERR_XML_TWO_DASHES_NOT_ALLOWED_IN_COMMENT	HX_XML_RES_INIT_ID +    102
#define IDS_ERR_XML_INVALID_DECL			HX_XML_RES_INIT_ID +    103
#define IDS_ERR_XML_INVALID_PI				HX_XML_RES_INIT_ID +    104
#define IDS_ERR_XML_INVALID_PI_TARGET			HX_XML_RES_INIT_ID +    105
#define IDS_ERR_XML_INVALID_CDATA			HX_XML_RES_INIT_ID +    106
#define IDS_ERR_XML_NO_CLOSING_GT			HX_XML_RES_INIT_ID +    107
#define IDS_ERR_XML_INVALID_HEX_CHAR_REF		HX_XML_RES_INIT_ID +    108
#define IDS_ERR_XML_INVALID_CHAR_REF			HX_XML_RES_INIT_ID +    109
#define IDS_ERR_XML_INVALID_REF				HX_XML_RES_INIT_ID +    110
#define IDS_ERR_XML_MISSING_EQUALS			HX_XML_RES_INIT_ID +    111
// use MISSINGQUOTE instead
//#define IDS_ERR_XML_MISSING_QUOT_APOS			HX_XML_RES_INIT_ID +    112
#define IDS_ERR_XML_MISSING_REQ_SPACE			HX_XML_RES_INIT_ID +    113
#define IDS_ERR_XML_LT_NOT_ALLOWED			HX_XML_RES_INIT_ID +    114
#define IDS_ERR_XML_EXPECTED_GT				HX_XML_RES_INIT_ID +    115
#define IDS_ERR_XML_INVALID_GT_AFFT_2_RSQB_IN_CONTENT	HX_XML_RES_INIT_ID +    116
#define IDS_ERR_XML_INVALID_COMMENT			HX_XML_RES_INIT_ID +    117


#define ERRSTR_XML_GENERALERROR     "XML: Unknown error near line %d: %s"
#define ERRSTR_XML_BADENDTAG	    "XML: Bad end tag near line %d: %s"
#define ERRSTR_XML_NOCLOSE	    "XML: No close near line %d: %s"
#define ERRSTR_XML_BADATTRIBUTE	    "XML: Bad attribute near line %d: %s"
#define ERRSTR_XML_NOVALUE	    "XML: No value near line %d: %s"
#define ERRSTR_XML_MISSINGQUOTE	    "XML: Missing quote near line %d: %s"
#define ERRSTR_XML_NOTAGTYPE	    "XML: No tag type near line %d: %s"
#define ERRSTR_XML_ILLEGALID	    "XML: Illegal id near line %d: %s"
#define ERRSTR_XML_DUPATTRIBUTE	    "XML: Duplicate attribute near line %d: %s"
#define ERRSTR_XML_COMMENT_B4_PROCINST "XML: Comment precedes processing instruction before line %d: %s"


#define ERRSTR_XML_SYNTAX				"XML: Syntax error near line %d: %s"
#define ERRSTR_XML_NO_ELEMENTS				"XML: No element found near line %d: %s"
// removed, repaced with specific error messages below.
//#define ERRSTR_XML_INVALID_TOKEN			"XML: Not well-formed near line %d: %s"
#define ERRSTR_XML_UNCLOSED_TOKEN			"XML: Unclosed token near line %d: %s"
#define ERRSTR_XML_PARTIAL_CHAR				"XML: Unclosed token near line %d: %s"
#define ERRSTR_XML_TAG_MISMATCH				"XML: Mismatched tag near line %d: %s"
// use DUPATRIBUTE instead
//#define ERRSTR_XML_DUPLICATE_ATTRIBUTE			"XML: Duplicate attribute near line %d: %s"
#define ERRSTR_XML_JUNK_AFTER_DOC_ELEMENT		"XML: Junk after document element near line %d: %s"
#define ERRSTR_XML_PARAM_ENTITY_REF			"XML: Illegal parameter entity reference near line %d: %s"
#define ERRSTR_XML_UNDEFINED_ENTITY			"XML: Undefined entity near line %d: %s"
#define ERRSTR_XML_RECURSIVE_ENTITY_REF			"XML: Recursive entity reference near line %d: %s"
#define ERRSTR_XML_ASYNC_ENTITY				"XML: Asynchronous entity near line %d: %s"
#define ERRSTR_XML_BAD_CHAR_REF				"XML: Reference to invalid character number near line %d: %s"
#define ERRSTR_XML_BINARY_ENTITY_REF			"XML: Reference to binary entity near line %d: %s"
#define ERRSTR_XML_ATTRIBUTE_EXTERNAL_ENTITY_REF	"XML: Reference to external entity in attribute near line %d: %s"
#define ERRSTR_XML_MISPLACED_XML_PI			"XML: Xml processing instruction not at start of external entity near line %d: %s"
#define ERRSTR_XML_UNKNOWN_ENCODING			"XML: Unknown encoding near line %d: %s"
#define ERRSTR_XML_INCORRECT_ENCODING			"XML: Encoding specified in xml declaration is incorrect near line %d: %s"
#define ERRSTR_XML_UNCLOSED_CDATA_SECTION		"XML: Unclosed CDATA section near line %d: %s"
#define ERRSTR_XML_EXTERNAL_ENTITY_HANDLING		"XML: Error in processing external entity reference near line %d: %s"
#define ERRSTR_XML_NOT_STANDALONE			"XML: Document is not standalone near line %d: %s"

#define ERRSTR_XML_INVALID_NAME				"XML: Name is invalid near line %d: %s"
#define ERRSTR_XML_INVALID_CHAR_IN_DOC			"XML: Character not allowed in doc near line %d: %s"
#define ERRSTR_XML_TWO_DASHES_NOT_ALLOWED_IN_COMMENT	"XML: Comments can not have '--' in them near line %d: %s"
#define ERRSTR_XML_INVALID_DECL				"XML: Invalid document type declaration syntax near line %d: %s"
#define ERRSTR_XML_INVALID_PI				"XML: Invalid processing instructions syntax near line %d: %s"
#define ERRSTR_XML_INVALID_PI_TARGET			"XML: XML processing instruction target must be lower case near line %d: %s"
#define ERRSTR_XML_INVALID_CDATA			"XML: Error in CDATA near line %d: %s"
#define ERRSTR_XML_NO_CLOSING_GT			"XML: End tag requires a closing > near line %d: %s"
#define ERRSTR_XML_INVALID_HEX_CHAR_REF			"XML: Invalid hexadecimal character reference near line %d: %s"
#define ERRSTR_XML_INVALID_CHAR_REF			"XML: Invalid character reference near line %d: %s"
#define ERRSTR_XML_INVALID_REF				"XML: Invalid entity reference near line %d: %s (try changing '&' to '&amp;')"
#define ERRSTR_XML_MISSING_EQUALS			"XML: Attribute names must be followed by an equal sign near line %d: %s"
// use MISSINGQUOTE instead
//#define ERRSTR_XML_MISSING_QUOT_APOS			"XML: Attribute values must start with a \" or ' near line %d: %s"
#define ERRSTR_XML_MISSING_REQ_SPACE			"XML: Attribute values must be followed by a whitespace near line %d: %s"
#define ERRSTR_XML_LT_NOT_ALLOWED			"XML: The < character must be escaped in an attribute value near line %d: %s"
#define ERRSTR_XML_EXPECTED_GT				"XML: / character must be followed by > to end empty element near line %d: %s"
#define ERRSTR_XML_INVALID_GT_AFFT_2_RSQB_IN_CONTENT	"XML: Content can not have the ']]>' string in it near line %d: %s"
#define ERRSTR_XML_INVALID_COMMENT			"XML: Invalid beginning of comment near line %d: %s"

#endif	/*_XMLRES_H_*/
