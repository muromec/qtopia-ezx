/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tagparse.h,v 1.6 2005/03/14 19:36:33 bobclark Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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
 *      XML tag parser
 *
 *	Not to be confused with the full XML parser, this parser will only
 *      tell you what tags appear in a document, and will parse their
 *      attributes for you. It will not perform any validation.
 */
#ifndef _TAGPARSE_H_
#define _TAGPARSE_H_


#include "carray.h"
#include "hxstack.h"


class XMLTagParser
{
private:
    UINT32	m_comment_state;
    UINT32	m_comment_get_arg;
    UINT16	m_comment_pos;
    HXBOOL	m_comment_start;
    char	m_comment_arg[1024]; /* Flawfinder: ignore */
    char	m_comment_command[20]; /* Flawfinder: ignore */
    char*	m_pEncoding;

    GetStringResult GetString	(const char*&, const char*, char*&, UINT32);
    void FindCommentClose	(const char*&, const char*, const char*);
    char GetEscapeMacro		(const char*&, const char*);

public:
    XMLTagParser(const char* pEncoding = NULL);
    ~XMLTagParser();

    XMLParseResult Parse    	(const char*& buf, 
				UINT32 len, 
				XMLTag*& tag);
    XMLParseResult ParseTag	(const char* open, 
				const char* close, 
				XMLTagType ulTagType, 
				XMLTag*& tag);
};

#endif // _TAGPARSE_H_
