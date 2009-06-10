/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rpsmplxml.cpp,v 1.5 2007/07/06 20:43:45 jfinnecy Exp $
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
//$Id: rpsmplxml.cpp,v 1.5 2007/07/06 20:43:45 jfinnecy Exp $

#include <ctype.h>
//#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "rpsmplxml.h"


//----------------------------------------------------------- Constants
//----------------------------------------------------------- Types
//----------------------------------------------------------- Functions
//----------------------------------------------------------- Methods


//---------------------------------------------------- CRPSimpleXMLTag

CRPSimpleXMLTag::CRPSimpleXMLTag()
{
    Init();
}


void CRPSimpleXMLTag::Init()
{
    m_type = RPXMLTag_Plain;
    m_pName = 0;
    m_bNeedClose = TRUE;
    m_numAttrs = 0;
}


CRPSimpleXMLAttr *CRPSimpleXMLTag::FindAttribute( const char *pAttrName )
{
    CRPSimpleXMLAttr *pCurAttr = m_attrs;
    for( INT32 i=0; i<m_numAttrs; i++, pCurAttr++ )
    {
	if( !strcmp( pAttrName, pCurAttr->m_pName ) )
	    return pCurAttr;
    }

    return 0;
}



//---------------------------------------------------- CRPSimpleXMLParser


CRPSimpleXMLParser::CRPSimpleXMLParser( char *pData ) :
    m_pData( pData ),
    m_pCurPtr( pData )
{
}


CRPSimpleXMLParser::~CRPSimpleXMLParser()
{
}


RPXMLParseResult CRPSimpleXMLParser::NextTag( CRPSimpleXMLTag& tag, HXBOOL bStrict )
{
    // Initialize the tag to an empty state
    tag.Init();

    // Look for opening bracket
    while( *m_pCurPtr && *m_pCurPtr != '<' )
	m_pCurPtr++;

    // Check for end of file
    if( !*m_pCurPtr )
	return RPXMLResult_EOF;

    // Skip past opening bracket
    m_pCurPtr++;

    // Skip white space
    while( isspace( *m_pCurPtr ) )
	m_pCurPtr++;

    // Check for end tag condition
    if( *m_pCurPtr == '/' )
    {
	tag.m_type = RPXMLTag_End;
	tag.m_bNeedClose = FALSE;
	m_pCurPtr++;
    }


    // Get tag name
    tag.m_pName = m_pCurPtr;
    while( _IsValidNameChar( *m_pCurPtr ) )
	m_pCurPtr++;
    char *pNameEnd = m_pCurPtr;

    // Look for attributes
    while( *m_pCurPtr != '>' )
    {
	// Check for error
	if( !*m_pCurPtr )
	    return RPXMLResult_Error;

	// check for an id
	if( isalpha( *m_pCurPtr ) || *m_pCurPtr == '_' )
	{
	    // found an id.  Check bound condiditons for number of attributes
	    HX_ASSERT( tag.m_numAttrs < kMaxXMLAttributes );
	    if( tag.m_numAttrs >= kMaxXMLAttributes )
		return RPXMLResult_Error;

	    // Parse the name
	    tag.m_attrs[ tag.m_numAttrs ].m_pName = m_pCurPtr;
	    while( _IsValidNameChar( *m_pCurPtr ) )
		m_pCurPtr++;

	    // Check for error
	    if( !*m_pCurPtr )
		return RPXMLResult_Error;

	    char *pIDEnd = m_pCurPtr;

	    // skip the equal sign
	    while( *m_pCurPtr != '=' )
	    {
		if( !*m_pCurPtr )
		    return RPXMLResult_Error;

		m_pCurPtr++;
	    }
	    m_pCurPtr++;

	    // Get the value
	    while( isspace( *m_pCurPtr ) )
		m_pCurPtr++;

	    // Check for error
	    if( !*m_pCurPtr )
		return RPXMLResult_Error;

	    // save quote type
	    char delim = 0;
	    if( *m_pCurPtr == '\"' || *m_pCurPtr == '\'' )
	    {
		delim = *m_pCurPtr;
		m_pCurPtr++;
	    }

	    if( bStrict && !delim )
		return RPXMLResult_Error;

	    tag.m_attrs[ tag.m_numAttrs++ ].m_pValue = m_pCurPtr;

	    while( delim && *m_pCurPtr != delim ||
		   !delim && !isspace(*m_pCurPtr) && *m_pCurPtr != '/')
	    {
		// Check for error
		if( !*m_pCurPtr )
		    return RPXMLResult_Error;

		m_pCurPtr++;
	    }
	    char *pValueEnd = m_pCurPtr;
	    m_pCurPtr++;

	    // If we ran into a forward slash either report an error, or note it
	    if( *pValueEnd == '/' )
	    {
		if( bStrict )
		    return RPXMLResult_Error;
		else
		    tag.m_bNeedClose = FALSE;
	    }

	    // Fix up ends of strings
	    *pIDEnd = '\0';
	    *pValueEnd = '\0';
	}
	else
	    m_pCurPtr++;
    }

    // Check for an end marker
    if( *( m_pCurPtr - 1 ) == '/' )
    {
	tag.m_bNeedClose = FALSE;
    }
    m_pCurPtr++;

    // Fix up ends of strings
    *pNameEnd = '\0';

    return RPXMLResult_Tag;
}


