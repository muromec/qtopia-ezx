/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resstr.cpp,v 1.8 2005/03/14 19:36:40 bobclark Exp $
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

#include "hxcom.h"
#include "hxassert.h"
#include "ResStr.h"
#include "hxstrutl.h"
#include "hxmullan.h"		// Multipl-Language Support API
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif		

/*
 *	Class static data
 */
char	*ResStr::m_pcNULL = "";

/*
 *	Class Constructors
 */
//	Default constructor
ResStr::ResStr( void )
{
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	DEBUGOUTSTR( "Called default contructor\r\n" );
#endif
	m_iLen = 0;
	m_iResNum = 0;
	m_pcString = m_pcNULL;
}


//	Initialized constructor
ResStr::ResStr( UINT32	iResNum )
{
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	char	acBuffer[100]; /* Flawfinder: ignore */

	SafeSprintf( acBuffer, 100, "Called contructor w/ ResStr( %d )\r\n", iResNum );
	DEBUGOUTSTR( acBuffer );
#endif

	//	Initialize some instance variables
	m_iLen = 0;
	m_iResNum = 0;
	m_pcString = m_pcNULL;

	//	This member will try to load the resource, but won't
	//	disturb our member data if it fails.
	LoadStr( iResNum );
}


//	Copy constructor
ResStr::ResStr( const ResStr &theRes )
{
	//	Copy the data from theRes into the new ResStr
	m_iLen = theRes.m_iLen;
	m_iResNum = theRes.m_iResNum;
	m_pcString = new char[ m_iLen + 1 ];

	//	This is a deep copy, so we duplicate the character data
	if (m_pcString)
	{
	    SafeStrCpy( m_pcString, theRes.m_pcString, m_iLen+1 );
	}
}


/*
 *	Class destructor
 *
 */
ResStr::~ResStr( void )
{
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	DEBUGOUTSTR( "Called desructor\r\n" );
#endif

	if (m_pcString && (m_pcString != m_pcNULL))
	{
		delete [] m_pcString;
	}
	m_iLen = 0;
	m_iResNum = 0;
	m_pcString = NULL;
	return;
}


/*
 *	Class public methods
 */
//	Assignment operator
ResStr &ResStr::operator=( const ResStr &theRes )
{
	char	*pcBuffer;

#ifndef _WIN32
	if (this == &theRes)
	{
		return( *this );
	}
#endif

	pcBuffer = new char[ theRes.m_iLen + 1 ];
	if (m_pcString && (m_pcString != m_pcNULL))
	{
		delete [] m_pcString;
	}
	m_iLen = theRes.m_iLen;
	m_iResNum = theRes.m_iResNum;
	m_pcString = pcBuffer;
	SafeStrCpy( m_pcString, theRes.m_pcString, m_iLen+1 );
	return( *this );
}


//	This method is used to get the object to load a Resource String
HXBOOL ResStr::LoadStr( UINT32 iResNum )
{
	UINT32		iLen;
	HXBOOL	btRes;
	char	*pcBuff;

	pcBuff = NULL;

#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	char	acBuffer[100]; /* Flawfinder: ignore */
#endif

	//	Get the resource size and allocate a buffer for it
	iLen = ResStr::GetResSize( iResNum );
	pcBuff = new char[ iLen ];
	if (!pcBuff)
	{
		goto FatalOut;
	}

	//	Actually load the resource
	btRes = ResStr::LoadRes( iResNum, pcBuff, iLen );
	if (btRes)
	{
		m_iLen = ::strlen( pcBuff );
		m_iResNum = iResNum;
		if (m_pcString && (m_pcString != m_pcNULL))
		{
			delete [] m_pcString;
		}
		m_pcString = pcBuff;
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
		SafeSprintf( acBuffer, 100, "Succeeded in loading resource as: *\'%s\'*\r\n", (LPSTR)pcBuff );
		DEBUGOUTSTR( acBuffer );
#endif
	}
	else
	{
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
		DEBUGOUTSTR( "Failed resource load\r\n" );
#endif
		goto FatalOut;
	}
	return( TRUE );

FatalOut:
	if (pcBuff && (pcBuff != m_pcNULL))
	{
		delete [] pcBuff;
	}
	return( FALSE );
}


//	This method initializes the object to load a Resource String, &
//	returns a pointer to the string.
LPSTR ResStr::operator()( UINT32 iResNum )
{
	if (iResNum && (iResNum != m_iResNum))
	{
		LoadStr( iResNum );
	}
	return( (LPSTR)m_pcString );
}


/*
 *	Class Private functions
 *	These functions are all used in the implementation, however we don't
 *	want to expose the implementation to the caller.  Hence they are Private.
 */
//	Returns an integer size guaranteed to be able to hold the string.
UINT32 ResStr::GetResSize( UINT32 iRes )
{
	//	Turns out that we can't lookup the size of string resources
	//	for some reason.  So since we DO have a promise that these 
	//	strings are less than 255 bytes, we just return 256 to be safe.
	return( 256 );
}


//	At the lowest level, this is what's used to load all resource
//	strings.
HXBOOL ResStr::LoadRes( UINT32 iRes, char *pcBuffer, UINT32 iMax )
{
	int	iLoad;

#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	char	acBuffer[100]; /* Flawfinder: ignore */

	SafeSprintf( acBuffer, 100, "Attempting to load resource %d\r\n", iRes );
	DEBUGOUTSTR( acBuffer );
#endif

	iLoad = HXLoadString(HX_SAFEUINT(iRes), (LPSTR)pcBuffer, HX_SAFEINT(iMax));
#if defined( _DEBUG ) && defined( DEBUG_CLASS )
	if (!iLoad)
	{
		DEBUGOUTSTR( "Failed string load\r\n" );
	}
#endif

	return( !!iLoad );
}
