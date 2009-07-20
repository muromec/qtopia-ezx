/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef _HXPLATFORMDATA_H_
#define _HXPLATFORMDATA_H_

#include "hxtypes.h"

/*
    Standard component packages
 */
#define COMPONENT_PACKAGE_UISYSTEM	"Gemini"
#define COMPONENT_PACKAGE_RMASDK	"RealMediaSDK"

/*
 *  Maximum size for the windows
 */

static const UINT32 z_WINMAX = 16384;

/*
    Standard namespace support
 */
#define NAMESPACE_SEPARATOR ':'
#define DEFAULT_NAMESPACE "http://ns.real.com/gemini.v1"

static const char* z_pDefaultNamespace = DEFAULT_NAMESPACE;

/*
 * Note these macros use CHXString
 */

#ifdef __cplusplus

#include "hxstring.h"

inline const CHXString& ADD_DEFAULT_NAMESPACE( CHXString& fullTagName, const char* pTagname )
{
    fullTagName = z_pDefaultNamespace;
    fullTagName += NAMESPACE_SEPARATOR;
    fullTagName += pTagname;
    
    return fullTagName;
}

inline void STRIP_OFF_DEFAULT_NAMESPACE( CHXString& fullTagName )
{
    INT32 index = fullTagName.ReverseFind( NAMESPACE_SEPARATOR );

    if ( index > -1 )
    {
	CHXString theNamespace( fullTagName.Left( index ) );

	if ( z_pDefaultNamespace == theNamespace )
	{
	    fullTagName = fullTagName.Right( fullTagName.GetLength() - ( index + 1 ) );
	}
    }
}

inline void STRIP_OFF_NAMESPACE( const char* pNamespace, CHXString& fullTagName )
{
    INT32 index = fullTagName.ReverseFind( NAMESPACE_SEPARATOR );

    if ( index > -1 )
    {
	CHXString theNamespace( fullTagName.Left( index ) );

	if ( theNamespace == pNamespace )
	{
	    fullTagName = fullTagName.Right( fullTagName.GetLength() - ( index + 1 ) );
	}
    }
}

inline void STRIP_OFF_ANY_NAMESPACE( CHXString& fullTagName )
{
    INT32 index = fullTagName.ReverseFind( NAMESPACE_SEPARATOR );

    if ( index > -1 )
    {
	fullTagName = fullTagName.Right( fullTagName.GetLength() - ( index + 1 ) );
    }
}

inline HXBOOL HAS_NAMESPACE( const char* pTag )
{
    CHXString fullTagName = pTag;
    INT32 index = fullTagName.ReverseFind( NAMESPACE_SEPARATOR );

    if ( index > -1 )
    {
	return TRUE;
    }
    return FALSE;
}

#endif


#endif // _HXPLATFORMDATA_H_
