/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstrutl.h,v 1.13 2009/03/04 00:47:42 girish2080 Exp $
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

#ifndef _HXSTRUTL_H_
#define _HXSTRUTL_H_

#include "hlxclib/string.h" /* for strxxx functions */
#include "hlxclib/stdlib.h" /* for atoi64() and itoa() functionallity */

#include "safestring.h"

#if !defined(_VXWORKS)
#ifdef _UNIX
#include <strings.h>
#include <ctype.h>
#endif
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#endif

#include "hxresult.h"

#if defined (_MACINTOSH) 

#define isascii isprint

inline const char *AnsiNext(const char* pcPtr) { return(	 pcPtr + 1 ); }
inline const char *AnsiPrev(const char * /* pcStart */, const char* pcPtr) { return (pcPtr - 1 ); }

int CopyP2CString(ConstStr255Param inSource, char* outDest, int inDestLength);
void CopyC2PString(const char* inSource, Str255 outDest);
char WINToMacCharacter( char inWINChar );
// these functions are used to convert Windows extended chars (used in non-English Roman languages)
// to Mac extended chars & vice-versa
void StripWinChars( char* pChars);
void StripMacChars( char* pChars);

inline void pstrcpy(Str255 dst, ConstStr255Param src) { BlockMoveData(src, dst, 1+src[0]); }

#ifndef _CARBON
inline void PStrCopy(StringPtr dest, ConstStr255Param src) { BlockMoveData(src, dest, 1+src[0]); }
inline void p2cstrcpy(char *dst, ConstStr255Param src) { CopyP2CString(src, dst, 255); }
inline void c2pstrcpy(Str255 dst, const char * src) { CopyC2PString(src, dst); }
#endif

#endif /* _MACINTOSH */

#define CR		(CHAR) '\r'
#define LF              (CHAR) '\n'
#define CRLF            "\r\n"

#ifdef _WIN32
    #define LINEBREAK	    "\015\012"
    #define LINEBREAK_LEN   2
#else
    #define LINEBREAK	    "\012"
    #define LINEBREAK_LEN   1
#endif /* _WIN32 */

#define LINE_BUFFER_SIZE	4096
#define MAX_BYTES_PER_COOKIE	4096
#define MAX_NUMBER_OF_COOKIES	300
#define MAX_COOKIES_PER_SERVER	20

/*
According to C99 7.4/1:
---------------
The header <ctype.h> declares several functions useful for
classifying and mapping characters. In all cases the argument is an
int, the value of which shall be representable as an unsigned char or
shall equal the value of the macro EOF. If the argument has any other
value, the behavior is undefined.
---------------
Typecast the value to an (unsigned char) before passing it to isspace() to ensure that
if the value is a signed char it doesn't get bit extended on certain (VC) compilers.
*/
#define IS_SPACE(x)	(isspace((unsigned char) x))

#ifdef __cplusplus
void	    StrAllocCopy(char*& pDest, const char* pSrc);
#else
void	    StrAllocCopy(char** pDest, const char* pSrc);
#endif
char*	    StripLine(char* pLine);

#include "hxtypes.h"
#include "hxcom.h"
typedef _INTERFACE IHXValues IHXValues;
HX_RESULT   SaveStringToHeader(IHXValues* /* IN OUT */     pHeader, 
			       const char*  /* IN */	   pszKey, 
			       const char* /* IN */	   pszValue,
			       IUnknown*		   pContext);

char* StrStrCaseInsensitive(const char* str1, const char* str2);
char* StrNStr(const char* str1, const char* str2, size_t depth1, size_t depth2);
char *StrNChr(const char *str, int c, size_t depth);
char *StrNRChr(const char *str, int c, size_t depth);
size_t StrNSpn(const char *str1, const char *str2, size_t depth1, size_t depth2);
size_t StrNCSpn(const char *str1, const char *str2, size_t depth1, size_t depth2);

char* StrToUpper(char *pString);

#if defined( _SYMBIAN)
#define NEW_FAST_TEMP_STR(NAME, EstimatedBiggestSize, LenNeeded)	\
    char*   NAME = new char[(LenNeeded)];		        

#define DELETE_FAST_TEMP_STR(NAME)					\
    delete[] NAME;							

#else
/* XXXSMP We can use alloca() on platforms that support it for more speed! */
#define NEW_FAST_TEMP_STR(NAME, EstimatedBiggestSize, LenNeeded)	\
    char    __##NAME##__StaticVersion[EstimatedBiggestSize];		\
    char*   NAME;							\
    UINT32  ulNeeded##NAME##Len = (LenNeeded);				\
        								\
    if (ulNeeded##NAME##Len <= EstimatedBiggestSize)			\
    {   								\
        NAME = __##NAME##__StaticVersion;				\
    }       								\
    else								\
    {									\
        NAME = new char[ulNeeded##NAME##Len];				\
    }

#define DELETE_FAST_TEMP_STR(NAME)					\
    if (NAME != __##NAME##__StaticVersion)				\
    { 									\
        delete[] NAME;							\
    }
#endif /* defined(_SYMBIAN) */

#ifdef HELIX_CONFIG_SYMBIAN_GENERATE_MMP
// Symbian MMP file does not support String constant definition using MACRO, 
// so use STRINGIFY to get the string value of a MACRO
// STRINGIFY(HX_MACRO) should return "helix" if HX_MACRO is defined as:
// #define HX_MACRO helix
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x
#else
#define STRINGIFY(x) x
#endif // HELIX_CONFIG_SYMBIAN_GENERATE_MMP

#endif /* _HXSTRUTL_H_ */
