/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxloader.h,v 1.4 2005/03/14 19:35:28 bobclark Exp $
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

#if !defined( _HXLOADER_H )
#define	_HXLOADER_H

#if !defined( _WIN32 )

#if !defined( _HXTYPES )
#include "hxtypes.h"
#endif	//	!defined( _HXTYPES )

/*
 *	Private extensions to HXTYPES
 *	These could be included in HXTYPES, but I'm not sure how the rest of the
 *	team likes them.
 */

typedef UCHAR					*PUCHAR;
typedef UINT16					*PUINT16;
typedef ULONG32					*PULONG32;
typedef const char* 			PCSTR;

extern "C"
{

/*
 *	PARAMSIG - Param signatures are formed by a string of tokens from the
 *	enum below.  The first token in the string corresponds to the function 
 *	return value.  All the rest of the tokens correspond directly to the
 *	function parameters as listed left to right in the function declaration.
 *
 *	The parameter signature is terminated by a 'END_PARAM' token.
 *
 *	Note that the for now we assume the PASCAL calling convention as it makes
 *	things simplest.  We can't support the generality of the 'C' calling
 *	convention anyway (variable number of args).
 *
 *	The simplest function: void Foo( void );
 *	Would be defined by: { VOID_RET, END_PARAM };
 *
 *	Another example: UINT16 Bar( UINT16, ULONG32, char * );
 *	Would be defined by: { UINT16_PARAM, UINT16_PARAM, ULONG32_PARAM, FPTR_PARAM }
 *
 *	Note that these token streams are read at runtime, and assembled into the correct
 *	assembler code to do the paramter conversions.
 *		
 */
 
typedef enum paramSIG
{
	END_PARAM	= 0,		//	Last or NULL parameter
	CHAR8_PARAM,			//	Signed char param (sign extended)
	UCHAR8_PARAM,			//	Unsigned char param (clear upper bits)
	INT16_PARAM,			//	Signed 16 bit int (sign extended)
	UINT16_PARAM,			//	Unsigned 16 bit int (clear upper bits)
	LONG32_PARAM,			//	Signed 32 bit int (direct copy)
	ULONG32_PARAM,			//	Unsigned 32 bit int (direct copy)
	FLOAT_PARAM,			//	IEEE single prec. float (32 bit - ???)
	DOUBLE_PARAM,			//	IEEE double prec. float (64 bit - ???)

	FPTR_PARAM,				//	16:16 far pointer (Mapped to correct address space)

	VOID_RET,				//	Void return type

	PARAMSIG_BOGUSVAL		//	Never use this guy
} PARAMSIG, *PPARAMSIG;


//	disable warning on: zero-sized array in struct/union
#pragma warning( disable : 4200 )

typedef struct thunkSIG
{
	char		*pcProcName;	//	Function name
	PARAMSIG	atParams[];	//	This is a null terminated array of PARAMSIGs.
} THUNKSIG;

//	Restore warnings
#pragma warning( default : 4200 )

/*
 *	Returns a ULONG32 that is either an hModule, or
 *	a pointer to a CPEModule object that encapsulates
 *	the loaded module.
 */
ULONG32 __export FAR PASCAL RALoadLibrary( LPCSTR lpLibName, THUNKSIG **pSignatures, ULONG32 dwFlags );


/*
 *	Takes a dwWord that is either an hModule,
 *	or a pointer to a CPEModule object that encapsulates
 *	the loaded module.
 */
HXBOOL __export FAR PASCAL RAFreeLibrary( ULONG32 dwhLib );


/*
 *	Takes a dwWord that is either an hModule,
 *	or a  pointer to a CPEModule object that encapsulates
 *	the loaded module.
 */
FARPROC __export FAR PASCAL RAGetProcAddress( ULONG32 dwhLib, LPCSTR lpProcName );


/*
 *	Takes a dwWord that is either an hModule,
 *	or a pointer to a CPEModule object that encapsulates
 *	the loaded module.
 */
ULONG32 __export FAR PASCAL RAGetModuleFileName( ULONG32 dwhModule, LPSTR lpFilenameBuff, ULONG32 dwBuffSize );


/*
 *	Takes the name of an executable (16 or 32 bit) and returns a block of memory containing
 *	the file version info.  This works for 16 or 32 bit executables.
 *
 *	NOTE:	The caller will need to free the block with RAVerDeleteFileVersionInfo().
 */
LPCSTR __export FAR PASCAL RAVerLockFileVersionInfo( LPCSTR lpszFileName, ULONG32 *lpSizeOut, ULONG32 dwFlags );

/*
 *	Takes an extracted file resource and returns selected information from it.
 *	This operates w/ the same semantics as ::VerQueryValue().
 */
HXBOOL __export FAR PASCAL RAVerQueryValue( LPCSTR lpvBlock, LPCSTR lpszSubBlock, void FAR* FAR* lplpBuffer, ULONG32 *lpdwCount );

/*
 *	Frees version info for a 16 or 32 bit stamped executable, that was extracted with
 *	RAVerLockFileVersionInfo().
 *
 *	NOTE:	This method is intended to be used with RAVerLockFileVersionInfo().
 */
void __export FAR PASCAL RAVerDeleteFileVersionInfo( LPCSTR lpvVersionInfo );
}
#else	//	This is the 32 bit else clause for a #if !defined( _WIN32 ) prepro directive
		//	The above functions don't exist for 32 bit.  So we just call the original
		//	WinAPI functions when building 32 bit
#define RALoadLibrary( lpLibName, pSignatures, dwFlags ) \
			((ULONG32)LoadLibrary( lpLibName ))

#define RAFreeLibrary( dwhLibInst ) \
			(FreeLibrary( (HINSTANCE)dwhLibInst ))

#define RAGetProcAddress( dwhDecoder, lpstrFunctionName )	\
			((FARPROC)GetProcAddress( (HINSTANCE)dwhDecoder, lpstrFunctionName ))

#define	RAVerQueryValue( lpvBlock, lpszSubBlock, lplpBuffer, lpdwCount ) \
			(VerQueryValue((const LPVOID)lpvBlock, lpszSubBlock, lplpBuffer, (UINT FAR *)lpdwCount ))

#endif	//	#if !defined( _WIN32 )

#endif	//	#if !defined( _HXLOADER_H )


