/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlreslt.h,v 1.5 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _XMLRESLT_H_
#define _XMLRESLT_H_

#define HXR_XML_GENERALERROR  	MAKE_HX_RESULT(1,SS_XML,0)                        // 80040480
#define HXR_XML_BADENDTAG	MAKE_HX_RESULT(1,SS_XML,1)                        // 80040481
#define HXR_XML_NOCLOSE		MAKE_HX_RESULT(1,SS_XML,2)                        // 80040482
#define HXR_XML_BADATTRIBUTE	MAKE_HX_RESULT(1,SS_XML,3)                        // 80040483
#define HXR_XML_NOVALUE		MAKE_HX_RESULT(1,SS_XML,4)                        // 80040484
#define HXR_XML_MISSINGQUOTE	MAKE_HX_RESULT(1,SS_XML,5)                        // 80040485
#define HXR_XML_NOTAGTYPE	MAKE_HX_RESULT(1,SS_XML,6)                        // 80040486
#define HXR_XML_ILLEGALID	MAKE_HX_RESULT(1,SS_XML,7)                        // 80040487
#define HXR_XML_DUPATTRIBUTE	MAKE_HX_RESULT(1,SS_XML,8)                        // 80040488
#define HXR_XML_COMMENT_B4_PROCINST MAKE_HX_RESULT(1,SS_XML,9)                    // 80040489

/* There are a couple of dupplicate errors here... We will keep them separate
   to make management of the expat parser simpler. */

#define HXR_XML_SYNTAX				MAKE_HX_RESULT(1,SS_XML,11)       // 8004048b
#define HXR_XML_NO_ELEMENTS			MAKE_HX_RESULT(1,SS_XML,12)       // 8004048c
/* We removed HXR_XML_INVALID_TOKEN ... replaced it with the errors in the 100's
   down below. - when josh updates code to use new errors */
#define HXR_XML_INVALID_TOKEN			MAKE_HX_RESULT(1,SS_XML,13)       // 8004048d
#define HXR_XML_UNCLOSED_TOKEN			MAKE_HX_RESULT(1,SS_XML,14)       // 8004048e
#define HXR_XML_PARTIAL_CHAR			MAKE_HX_RESULT(1,SS_XML,15)       // 8004048f
#define HXR_XML_TAG_MISMATCH			MAKE_HX_RESULT(1,SS_XML,16)       // 80040490
// will use HXR_XML_DUPATTRIBUTE instead - when josh updates code to use new errors
#define HXR_XML_DUPLICATE_ATTRIBUTE		MAKE_HX_RESULT(1,SS_XML,17)       // 80040491
#define HXR_XML_JUNK_AFTER_DOC_ELEMENT		MAKE_HX_RESULT(1,SS_XML,18)       // 80040492
#define HXR_XML_PARAM_ENTITY_REF		MAKE_HX_RESULT(1,SS_XML,19)       // 80040493
#define HXR_XML_UNDEFINED_ENTITY		MAKE_HX_RESULT(1,SS_XML,20)       // 80040494
#define HXR_XML_RECURSIVE_ENTITY_REF		MAKE_HX_RESULT(1,SS_XML,21)       // 80040495
#define HXR_XML_ASYNC_ENTITY			MAKE_HX_RESULT(1,SS_XML,22)       // 80040496
#define HXR_XML_BAD_CHAR_REF			MAKE_HX_RESULT(1,SS_XML,23)       // 80040497
#define HXR_XML_BINARY_ENTITY_REF		MAKE_HX_RESULT(1,SS_XML,24)       // 80040498
#define PNR_XML_ATTRIBUTE_EXTEHXAL_ENTITY_REF	MAKE_HX_RESULT(1,SS_XML,25)       // 80040499
#define HXR_XML_MISPLACED_XML_PI		MAKE_HX_RESULT(1,SS_XML,26)       // 8004049a
#define HXR_XML_UNKNOWN_ENCODING		MAKE_HX_RESULT(1,SS_XML,27)       // 8004049b
#define HXR_XML_INCORRECT_ENCODING		MAKE_HX_RESULT(1,SS_XML,28)       // 8004049c
#define HXR_XML_UNCLOSED_CDATA_SECTION		MAKE_HX_RESULT(1,SS_XML,29)       // 8004049d
#define PNR_XML_EXTEHXAL_ENTITY_HANDLING	MAKE_HX_RESULT(1,SS_XML,30)       // 8004049e
#define HXR_XML_NOT_STANDALONE			MAKE_HX_RESULT(1,SS_XML,31)       // 8004049f

#define HXR_XML_INVALID_NAME			  MAKE_HX_RESULT(1,SS_XML,100)    // 800404e4
#define HXR_XML_INVALID_CHAR_IN_DOC		  MAKE_HX_RESULT(1,SS_XML,101)    // 800404e5
#define HXR_XML_TWO_DASHES_NOT_ALLOWED_IN_COMMENT MAKE_HX_RESULT(1,SS_XML,102)    // 800404e6
#define HXR_XML_INVALID_DECL			  MAKE_HX_RESULT(1,SS_XML,103)    // 800404e7
#define HXR_XML_INVALID_PI			  MAKE_HX_RESULT(1,SS_XML,104)    // 800404e8
#define HXR_XML_INVALID_PI_TARGET		  MAKE_HX_RESULT(1,SS_XML,105)    // 800404e9
#define HXR_XML_INVALID_CDATA			  MAKE_HX_RESULT(1,SS_XML,106)    // 800404ea
#define HXR_XML_NO_CLOSING_GT			  MAKE_HX_RESULT(1,SS_XML,107)    // 800404eb
#define HXR_XML_INVALID_HEX_CHAR_REF		  MAKE_HX_RESULT(1,SS_XML,108)    // 800404ec
#define HXR_XML_INVALID_CHAR_REF		  MAKE_HX_RESULT(1,SS_XML,109)    // 800404ed
#define HXR_XML_INVALID_REF			  MAKE_HX_RESULT(1,SS_XML,110)    // 800404ee
#define HXR_XML_MISSING_EQUALS			  MAKE_HX_RESULT(1,SS_XML,111)    // 800404ef
// will use HXR_XML_MISSINGQUOTE
//#define HXR_XML_MISSING_QUOT_APOS		  MAKE_HX_RESULT(1,SS_XML,112)    // 800404f0
#define HXR_XML_MISSING_REQ_SPACE		  MAKE_HX_RESULT(1,SS_XML,113)    // 800404f1
#define HXR_XML_LT_NOT_ALLOWED			  MAKE_HX_RESULT(1,SS_XML,114)    // 800404f2
#define HXR_XML_EXPECTED_GT			  MAKE_HX_RESULT(1,SS_XML,115)    // 800404f3
#define HXR_XML_INVALID_GT_AFFT_2_RSQB_IN_CONTENT MAKE_HX_RESULT(1,SS_XML,116)    // 800404f4
#define HXR_XML_INVALID_COMMENT			  MAKE_HX_RESULT(1,SS_XML,117)    // 800404f5

#endif	/* _XMLRESLT_H */
