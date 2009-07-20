/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resstr.h,v 1.6 2007/07/06 20:39:28 jfinnecy Exp $
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

#if !defined( _INC_WINDOWS )
#include "hlxclib/windows.h"
#endif


#if !defined( _INC_STDLIB )
#include <stdlib.h>
#endif

// #define DEBUG_CLASS

class ResStr
{
public:

	/*
	 *	Class constructors & destructor
	 */
	//	Default constructor
	ResStr( void );
	//	Initialized contstructor
	ResStr( UINT32	iResNum );
	//	Copy constructor
	ResStr( const ResStr &theRes );

	~ResStr( void );

	/*
	 *	Class public methods
	 */

	//	Assignment operator
	ResStr &operator=( const ResStr &s );

	//	Returns our resource number
	UINT32 GetResNum( void )
	{
		return( m_iResNum );
	}

	//	Returns our resource strlen
	UINT32	strlen( void )
	{
		return( m_iLen );
	}

	//	Loads a new string into the ResStr object
	HXBOOL LoadStr( UINT32 iResNum );

	//	Provide access to the resource string
	LPSTR operator&( void )
	{
		return( m_pcString ? LPSTR( m_pcString ) : LPSTR( m_pcNULL ) );
	}

	//	Combination of loading a new resource string,
	//	and providing a pointer to the string
	LPSTR	operator()( UINT32 iResNum = 0 );

protected:
	UINT32		m_iLen;
	UINT32		m_iResNum;
	char		*m_pcString;

private:
	static char			*m_pcNULL;		//	Pointer to an empty string

	//	Isolate functions which actually access resources

	static UINT32	GetResSize( UINT32 iRes );
	static HXBOOL	LoadRes( UINT32 iRes, char *pcBuffer, UINT32 iMax );
};
