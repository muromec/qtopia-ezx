/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstrutl.cpp,v 1.15 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxstrutl.h"
#include "hxassert.h"

#include "hlxclib/stdio.h"      //for vsnprintf

#ifdef _MACINTOSH

int CopyP2CString(ConstStr255Param inSource, char* outDest, int inDestLength)
{
	int outLength = 0;
	HX_ASSERT( inDestLength > 0 );
	if ( inSource )
	{
		outLength = *inSource++;
		outLength = HX_MIN( outLength, ( inDestLength - 1 ) );
		int copiedLength = outLength;
		while ( copiedLength-- )
		{
			*outDest++ = *( char* ) inSource++;
		}
	}
	*outDest = '\0';
	return( outLength );
}

//
//	CopyC2PString converts the source C string to a destination
//	pascal string as it copies. The dest string will
//	be truncated to fit into an Str255 if necessary.
//  If the C String pointer is NULL, the pascal string's length is set to zero
//
void CopyC2PString(const char* inSource, Str255 outDest)
{
#ifdef _CARBON
	if (inSource == NULL) outDest[0] = 0;
	else CopyCStringToPascal(inSource, outDest);  // Carbon TextUtils.h call
#else
	short 	length  = 0;
	
	// handle case of overlapping strings
	if ( (void*)inSource == (void*)outDest )
	{
		unsigned char*		curdst = &outDest[1];
		unsigned char		thisChar;
				
		thisChar = *(const unsigned char*)inSource++;
		while ( thisChar != '\0' ) 
		{
			unsigned char	nextChar;
			
			// use nextChar so we don't overwrite what we are about to read
			nextChar = *(const unsigned char*)inSource++;
			*curdst++ = thisChar;
			thisChar = nextChar;
			
			if ( ++length >= 255 )
				break;
		}
	}
	else if ( inSource != NULL )
	{
		unsigned char*		curdst = &outDest[1];
		short 				overflow = 255;		// count down so test it loop is faster
		register char		temp;
	
		// Can't do the K&R C thing of Òwhile (*s++ = *t++)Ó because it will copy trailing zero
		// which might overrun pascal buffer.  Instead we use a temp variable.
		while ( (temp = *inSource++) != 0 ) 
		{
			*(char*)curdst++ = temp;
				
			if ( --overflow <= 0 )
				break;
		}
		length = 255 - overflow;
	}
	outDest[0] = length;
#endif // !defined _CARBON
}

char
WINToMacCharacter( char inWINChar )
{
	static const unsigned char kXX = 240;
	static const unsigned char kWINToMacHiBitCharConverter[ 128 ] = /* Flawfinder: ignore */
	{
		// Conversion made between Windows MS Sans Serif and Macintosh Geneva.
		kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX,
		kXX, 212, 213, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX, kXX,
		202, 193, 162, 163, 188, 180, kXX, 164, 172, 169, 187, 199, 194, 208, 168, 248,
		251, 177, kXX, kXX, 171, 181, 166, 225, 252, kXX, kXX, 200, kXX, kXX, kXX, 192,
		203, 231, 229, 204, 128, 129, 174, 130, 233, 131, 230, 232, 237, 234, 235, 236,
		kXX, 132, 241, 238, 239, 205, 133, 120, 175, 244, 242, 243, 134, kXX, kXX, 167,
		136, 135, 137, 139, 138, 140, 190, 141, 143, 142, 144, 145, 147, 146, 148, 149,
		182, 150, 152, 151, 153, 155, 154, 214, 191, 157, 156, 158, 159, kXX, kXX, 216
	};
	char outMacChar = ( ( unsigned char ) inWINChar & 0x80 ) ?
					  ( char ) kWINToMacHiBitCharConverter[ ( unsigned char ) inWINChar & ~0x80 ] : inWINChar; /* Flawfinder: ignore */
	return ( outMacChar );
}

static char
MacToWinCharacter( char inMacChar )
{
	static const unsigned char kXX = 240;
	static const unsigned char kMacToWinHiBitCharConverter[ 128 ] = /* Flawfinder: ignore */
	{
		// Conversion made between  Macintosh Geneva and Windows MS Sans Serif
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xE1, 0xE0, 0xE2, 0xE4, 0xE3, 0xE5, 0xE7, 0xE9, 0xE8,
		0xEA, 0xEB, 0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF3, 0xF2, 0xF4, 0xF6, 0xF5, 0xFA, 0xF9, 0xFB, 0x9F,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA7, 0xA5, 0xB6, 0xDF, 0xAE, 0xA9, 0xAA, 0xB4, 0xA8, 0xAD, 0xC6, 0xD8,
		0xB0, 0xB1, 0xB2, 0xB3, 0xA5, 0xB5, 0xF0, 0xB7, 0xB8, 0xB9, 0xBA, 0xAA, 0xA4, 0xBD, 0xE6, 0xF8,
		0xBF, 0xA1, 0xAC, 0xC3, 0xC4, 0xC5, 0xC6, 0xAB, 0xBB, 0xC6, 0xA0, 0xC0, 0xC3, 0xD5, 0xCE, 0xCF,
		0xAD, 0xD1, 0xD2, 0xD3, 0x91, 0x92, 0xF7, 0xD7, 0xFF, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
		0xE0, 0xB7, 0xE2, 0xE3, 0xE4, 0xC2, 0xCA, 0xC1, 0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0xD3, 0xD4,
		0xF0, 0xD2, 0xDA, 0xDB, 0xD9, 0xF5, 0xF6, 0xF7, 0xAF, 0xF9, 0xFA, 0xB0, 0xB8, 0xFF, 0xFF, 0xFF
	};
	char outWinChar = ( ( unsigned char ) inMacChar & 0x80 ) ?
					  ( char ) kMacToWinHiBitCharConverter[ ( unsigned char ) inMacChar & ~0x80 ] : inMacChar; /* Flawfinder: ignore */
	return ( outWinChar );
}

// these functions are used to convert Windows extended chars (used in non-English Roman languages)
// to Mac extended chars & vice-versa
void
StripMacChars( char* pChars)
{
	// don't bother if double-byte language
#ifdef _CARBON
	if (CharacterByteType(pChars,0, smRoman) == smSingleByte) /* CharByte can't be executed at interrupt time! */
#else
	if (CharByte(pChars,0) == smSingleByte) /* CharByte can't be executed at interrupt time! */
#endif
	{
		int theStringLength = strlen(pChars);
		for ( int index = 0; index < theStringLength; ++index )
		{
			pChars[index] = MacToWinCharacter( pChars[ index ] ) ;
		}
	}
}

void
StripWinChars( char* pChars)
{
	// don't bother if double-byte language
#ifdef _CARBON
	if (CharacterByteType(pChars,0, smRoman) == smSingleByte) /* CharByte can't be executed at interrupt time! */
#else
	if (CharByte(pChars,0) == smSingleByte) /* CharByte can't be executed at interrupt time! */
#endif
	{
		int theStringLength = strlen(pChars);
		for ( int index = 0; index < theStringLength; ++index )
		{
			pChars[index] = WINToMacCharacter( pChars[ index ] ) ;
		}
	}
}
#endif

char *
StripLine (char *string)
{
    char * ptr = NULL;

    // remove leading blanks
    while(*string=='\t' || *string==' ' || *string=='\r' || *string=='\n')
    {
	string++;    
    }

    for(ptr=string; *ptr; ptr++)
    {
	;   // NULL BODY; Find end of string
    }

    // remove trailing blanks
    for(ptr--; ptr >= string; ptr--)	
    {
	if(*ptr=='\t' || *ptr==' ' || *ptr=='\r' || *ptr=='\n')
	{
	    *ptr = '\0'; 
	}
        else 
	{
	    break;
	}
    }

    return string;
}

#ifdef __cplusplus

void
StrAllocCopy(char*& pDest, const char* pSrc)
{
    HX_VECTOR_DELETE(pDest);

    if (pSrc)
    {
        pDest = new char[strlen(pSrc) + 1];
        if (pDest)
	{
	    strcpy(pDest, pSrc); /* Flawfinder: ignore */
	}
    }
}

#else /* #ifdef __cplusplus */

void
StrAllocCopy(char** pDest, const char* pSrc)
{
    if (pDest)
    {
        HX_VECTOR_DELETE(*pDest);

        if (pSrc)
        {
            *pDest = new char[strlen(pSrc) + 1];
            if (*pDest)
	    {
	        strcpy(*pDest, pSrc); /* Flawfinder: ignore */
	    }
        }
    }
}

#endif /* #ifdef __cplusplus #else */

HX_RESULT
SaveStringToHeader(IHXValues* pHeader, const char* pszKey, const char* pszValue, IUnknown* pContext)
{
    HX_RESULT	hr = HXR_OK;
    IHXBuffer*	pBuffer = NULL;
    
    hr = CreateBufferCCF(pBuffer, pContext);
    if (HXR_OK == hr)
    {
	pBuffer->Set((const UCHAR*)pszValue, strlen(pszValue)+1);    

	if (pHeader) pHeader->SetPropertyBuffer(pszKey, pBuffer);

	pBuffer->Release();
    }
    
    return hr;
}

char* 
StrStrCaseInsensitive(const char* str1, const char* str2)
{
    char *cp = (char*) str1;
    char *s1, *s2, c1, c2;

    if (!*str2)
        return((char *)str1);

    while (*cp)
    {
        s1 = cp;
        s2 = (char*) str2;

	s1--, s2--;        
	do
	{
            s1++, s2++;

	    /* convert both chars to lower case */
	    c1 = *s1;
	    c2 = *s2;
	    if ((c1 <= 'Z') && (c1 >= 'A'))
		c1 += ('a' - 'A');
	    if ((c2 <= 'Z') && (c2 >= 'A'))
		c2 += ('a' - 'A');
	} while (*s1 && *s2 && !(c1-c2));

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}

char* StrNStr(const char* str1, const char* str2, size_t depth1, size_t depth2)
{
    const char *tracer1;
    const char *tracer2;
    size_t depth_tracer1;
    size_t depth_tracer2;

    while(*str1)
    {
	for (tracer1 = str1, 
	     tracer2 = str2, 
	     depth_tracer1 = depth1, 
	     depth_tracer2 = depth2;
	     (*tracer1 == *tracer2) && depth_tracer1 && (*tracer1 != '\0');
	     tracer1++, depth_tracer1--)
	{
	    tracer2++;
	    depth_tracer2--;
	    if ((depth_tracer2 == 0) || (*tracer2 == '\0'))
	    {
		return (char *) str1;
	    }
	}

	if ((depth_tracer1 == 0) || (*tracer1 == '\0'))
	{
	    return NULL;
	}

	str1++;
	depth1--;
    }

    return NULL;
}

char *StrNChr(const char *str, int c, size_t depth)
{
    do
    {
	if (depth--)
	{
	    if (*str == c)
	    {
		return (char *) str;
	    }
	}
	else
	{
	    return NULL;
	}
    } while (*(str++) != '\0');

    return NULL;
}

char *StrNRChr(const char *str, int c, size_t depth)
{
    const char *o_ptr = NULL;

    do
    {
	if (depth--)
	{
	    if (*str == c)
	    {
		o_ptr = str;
	    }
	}
	else
	{
	    return (char *) o_ptr;
	}
    } while (*(str++) != '\0');

    return (char *) o_ptr;
}

size_t StrNSpn(const char *str1, const char *str2, size_t depth1, size_t depth2)
{
    const char *tracer;
    size_t depth_tracer;
    size_t depth_match = 0;

    while(depth1 && (*str1 != '\0'))
    {
	for (tracer = str2, depth_tracer = depth2;
	    (*str1 != *tracer) && depth_tracer && (*tracer != '\0');
	    tracer++, depth_tracer--)
	{
	    ;
	}
	
	if ((depth_tracer == 0) || (*tracer == '\0'))
	{
	    return depth_match;
	}
	
	depth_match++;
	
	str1++;
	depth1--;
    }

    return depth_match;
}

size_t StrNCSpn(const char *str1, const char *str2, size_t depth1, size_t depth2)
{
    const char *tracer;
    size_t depth_tracer;
    size_t depth_match = 0;

    while(depth1 && (*str1 != '\0'))
    {
	for (tracer = str2, depth_tracer = depth2;
	     (*str1 != *tracer) && depth_tracer && (*tracer != '\0');
	     tracer++, depth_tracer--)
	{
	    ;
	}

	if ((depth_tracer != 0) && (*tracer != '\0'))
	{
	    return depth_match;
	}

	depth_match++;

	str1++;
	depth1--;
    }

    return depth_match;
}


char* StrToUpper(char *pString)
{
    char* pTracer = pString;

    while (*pTracer)
    {
	*pTracer = toupper(*pTracer);
	pTracer++;
    }

    return pString;
}

