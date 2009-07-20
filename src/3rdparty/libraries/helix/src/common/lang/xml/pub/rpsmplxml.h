/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rpsmplxml.h,v 1.5 2008/02/08 07:44:54 vkathuria Exp $
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
// $Id: rpsmplxml.h,v 1.5 2008/02/08 07:44:54 vkathuria Exp $

#ifndef _RPSMPLXML_H_
#define _RPSMPLXML_H_

#include "hlxclib/ctype.h"

#include "hxtypes.h"
#include "hxassert.h"


//------------------------------------------------------------- Constants	

const int kMaxXMLAttributes = 50;


enum RPXMLTagType
{
    RPXMLTag_Plain,
    RPXMLTag_End
};


enum RPXMLParseResult
{
    RPXMLResult_Tag,
    RPXMLResult_EOF,
    RPXMLResult_Error
};

//------------------------------------------------------------- Types


class CRPSimpleXMLAttr
{
public:
    CRPSimpleXMLAttr() : m_pName( 0 ), m_pValue( 0 ) {}

public:
    char *m_pName;
    char *m_pValue;
};


class CRPSimpleXMLTag
{
public:
    CRPSimpleXMLTag();

    void Init();

    CRPSimpleXMLAttr *GetAttribute( INT32 index ) { HX_ASSERT( index <= m_numAttrs ); return &m_attrs[ index ]; }
    CRPSimpleXMLAttr *FindAttribute( const char *pAttrName );

public:
    RPXMLTagType m_type;
    char *m_pName;

    HXBOOL m_bNeedClose;

    INT32 m_numAttrs;
    CRPSimpleXMLAttr m_attrs[ kMaxXMLAttributes ];
};


class CRPSimpleXMLParser
{
public:
    CRPSimpleXMLParser( char *pData );
    ~CRPSimpleXMLParser();

    RPXMLParseResult NextTag( CRPSimpleXMLTag& tag, HXBOOL bStrict = TRUE );

private:

    HXBOOL _IsValidNameChar( char c );

    char *m_pData;
    char *m_pCurPtr;
};


inline HXBOOL CRPSimpleXMLParser::_IsValidNameChar( char c )
{
    return ( isalnum( c ) || c == '.' || c == '-' || c == '_' );
}


//------------------------------------------------------------- Typedefs
//------------------------------------------------------------- Macros	
	
	
	
#endif // _RPSMPLXML_H_
