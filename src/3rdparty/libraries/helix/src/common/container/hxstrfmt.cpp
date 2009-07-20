/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstrfmt.cpp,v 1.19 2007/01/26 15:19:18 rrajesh Exp $
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

#include "hxstring.h"
#include "hxassert.h"

#include "hlxclib/string.h"
#include "hlxclib/wchar.h"
#include "safestring.h"
#include "hlxclib/stdarg.h"
#include "hxassert.h"

// Formatting a %s with a null pointer results in (null) being printed
#define FORMAT_NULL_STRING  "(null)"

static const int AlternateFlag   = 0x01;
static const int ZeroPadFlag     = 0x02;
static const int LeftJustifyFlag = 0x04;
static const int AddSpaceFlag    = 0x08;
static const int AddSignFlag     = 0x10;

static const int MaxWidthSize = 12;
static const int WidthParam = -2;
static const int WidthError = -3;

static const int MaxPrecisionSize = 12;
static const int NoPrecision    = -1;
static const int PrecisionParam = -2;
static const int PrecisionError = -3;

static const int NoLength = 0;
static const int ShortLength = 1;
static const int LongLength = 2;

static const int MaxConversionSize = 32;
static const int MaxFormatSize = 11;

static int GetFlags(const char*& pCur)
{
    int ret = 0;

    for (; *pCur && strchr("#0- +", *pCur); pCur++)
    {
	switch(*pCur) {
	case '#':
	    ret |= AlternateFlag;
	    break;

	case '0':
	    ret |= ZeroPadFlag;
	    break;

	case '-':
	    ret |= LeftJustifyFlag;
	    break;

	case ' ':
	    ret |= AddSpaceFlag;
	    break;
	    
	case '+':
	    ret |= AddSignFlag;
	    break;
	};
    }

    return ret;
}

static int GetWidth(const char*& pCur)
{
    int ret = 1;

    if (*pCur)
    {
	if (*pCur == '*')
	{
	    // The width is specified as a parameter
	    pCur++;
	    ret = WidthParam;
	}
	else
	{
	    if (strchr("123456789", *pCur))
	    {
		int i = 0;
		char widthBuf[MaxWidthSize]; /* Flawfinder: ignore */
		
		widthBuf[i++] = *pCur++;

		for(; (i < MaxWidthSize) && 
			*pCur && (strchr("0123456789", *pCur)); i++)
		    widthBuf[i] = *pCur++;

		if (i != MaxWidthSize)
		{
		    widthBuf[i] = '\0';

		    char* pEnd = 0;
		    long int tmp = strtol(widthBuf, &pEnd, 10);
		    
		    if (widthBuf[0] && !*pEnd)
			ret = (int)tmp;
		}
		else
		    ret = WidthError;
	    }
	}
    }

    return ret;
}

static int GetPrecision(const char*& pCur)
{
    int ret = NoPrecision;

    if (*pCur == '.')
    {
	pCur++;

	if (*pCur == '*')
	{
	    // The width is specified as a parameter
	    pCur++;
	    ret = PrecisionParam;
	}
	else
	{
	    int i = 0;
	    char precisionBuf[MaxPrecisionSize]; /* Flawfinder: ignore */
	    
	    for(; (i < MaxPrecisionSize) && 
		    *pCur && (strchr("0123456789", *pCur)); i++)
		precisionBuf[i] = *pCur++;
	    
	    if (i != MaxPrecisionSize)
	    {
		precisionBuf[i] = '\0';
	    
		if (strlen(precisionBuf))
		{
		    char* pEnd = 0;
		    long int tmp = strtol(precisionBuf, &pEnd, 10);
		    
		    if (precisionBuf[0] && !*pEnd)
			ret = (int)tmp;
		}
		else
		    ret = 0;
	    }
	    else
		ret = PrecisionError;
	}
    }

    return ret;
}

static int GetLength(const char*& pCur)
{
    int ret = NoLength;

    switch(*pCur) {
    case 'l':
	ret = LongLength;
	pCur++;
	break;
    case 'h':
	ret = ShortLength;
	pCur++;
	break;
    };

    return ret;
}

static void ConstructFormat(char* fmt, char type, int flags, int length,
			    int precision)
{
    int i = 0;
    fmt[i++] = '%';

    if (flags & AlternateFlag)
	fmt[i++] = '#';

    if (flags & LeftJustifyFlag)
	fmt[i++] = '-';

    if (flags & AddSpaceFlag)
	fmt[i++] = ' ';

    if (flags & AddSignFlag)
	fmt[i++] = '+';

    if (flags & ZeroPadFlag)
	fmt[i++] = '0';

    fmt[i++] = '*';

    if (precision != NoPrecision)
    {
	fmt[i++] = '.';
	fmt[i++] = '*';
    }

    if (length == ShortLength)
	fmt[i++] = 'h';
    
    if (length == LongLength)
	fmt[i++] = 'l';


    fmt[i++] = type;
    fmt[i] = '\0';

    HX_ASSERT(i < MaxFormatSize);
}

// This could be handled with a single template function, but for now
// we can't use templates. :( 
// I'll use a macro instead.

#define CONVERT_FUNC_DEF(funcName, convertType)                    \
static int funcName(const char* fmt,                               \
                    int width, int precision,                      \
                    convertType value)                             \
{                                                                  \
    int ret = 0;                                                   \
    int bufSize = width + MaxConversionSize;                       \
                                                                   \
    if (precision != NoPrecision)                                  \
        bufSize += precision;                                      \
    char* pBuf = new char[bufSize];                                \
    if (precision == NoPrecision)                                  \
	ret = SafeSprintf(pBuf, bufSize, fmt, width, value);       \
    else                                                           \
	ret = SafeSprintf(pBuf, bufSize, fmt, width, precision, value); \
    HX_ASSERT(ret < bufSize);                                      \
    delete [] pBuf;                                                \
    return ret;                                                    \
}

CONVERT_FUNC_DEF(ConvertInt, int)
CONVERT_FUNC_DEF(ConvertShort, short int)
CONVERT_FUNC_DEF(ConvertLong, long int)
CONVERT_FUNC_DEF(ConvertUInt, unsigned int)
CONVERT_FUNC_DEF(ConvertUShort, unsigned short int)
CONVERT_FUNC_DEF(ConvertULong, unsigned long int)
CONVERT_FUNC_DEF(ConvertDouble, double)
CONVERT_FUNC_DEF(ConvertChar, char)
CONVERT_FUNC_DEF(ConvertWChar, wchar_t)
CONVERT_FUNC_DEF(ConvertPtr, void*)
CONVERT_FUNC_DEF(ConvertUInt64, UINT64)
CONVERT_FUNC_DEF(ConvertInt64, INT64)

#ifdef __amd64__
static bool ParseFormat(const char*& pCur, int& charCount, va_list args)
#else
static bool ParseFormat(const char*& pCur, int& charCount, va_list& args)
#endif
{
    bool ret = true;

    const char* pTmp = pCur;

    int flags = GetFlags(pTmp);
    int width = 1;
    int precision = NoPrecision;
    int convertSize = 0;

    if ((width = GetWidth(pTmp)) == WidthError)
    {
	HX_ASSERT(!fprintf(stderr, "Width field too long '%s'\n", pCur));
	ret = false;
    }
    else if ((precision = GetPrecision(pTmp)) == PrecisionError)
    {
	HX_ASSERT(!fprintf(stderr, "Precision field too long '%s'\n", pCur));
	ret = false;
    }
    else
    {
	int length = GetLength(pTmp);
	char type = *pTmp++;

	if (width == WidthParam)
	{
	    width = va_arg(args, int);
	    if (width < 0)
	    {
		width = -width;
		flags |= LeftJustifyFlag;
	    }
	}

	if (precision == PrecisionParam)
	{
	    precision = va_arg(args, int);

	    if (precision < 0)
		precision = 0;
	}

	switch (type) {
	case 's':
        {
            const char* pVal = va_arg(args, const char*);
            if (length == LongLength)
            {
                HX_ASSERT(!"Wide characters not supported");
                
                // Make up something large and hope that it's big enough
                convertSize = 512;
            }
            else
            {
                
                if(precision > 0)
                {
                    // precision is the max value formatted irrespective of
                    // string len being smaller or larger than preceision
                    convertSize += precision;
                }
                else
                {
                    if(pVal != NULL)
                    {
                        convertSize = strlen(pVal);
                    }
                    else
                    {
                        // format will result in string "(null)"
                        // hence adjust size to the null str len
                        convertSize = strlen(FORMAT_NULL_STRING); 
                    }
                } // End of if(precision > 0)
                
            }
            
        }break;

	case 'd':
	case 'i':
	{
	    char fmt[MaxFormatSize]; /* Flawfinder: ignore */

	    ConstructFormat(fmt, type, flags, length, precision);

	    if (length == LongLength)
	    {
		long int val = va_arg(args, long int);
		convertSize = ConvertLong(fmt, width, precision, val);
	    }
	    else if (length == ShortLength)
	    {
		short int val = va_arg(args, int);
		convertSize = ConvertShort(fmt, width, precision, val);
	    }
	    else
	    {
		int val = va_arg(args, int);
		convertSize = ConvertInt(fmt, width, precision, val);;
	    }
	}break;

	case 'u':
	case 'o':
	case 'x':
	case 'X':
	{
	    char fmt[MaxFormatSize]; /* Flawfinder: ignore */

	    ConstructFormat(fmt, type, flags, length, precision);

	    if (length == LongLength)
	    {
		unsigned long int val = va_arg(args, unsigned long int);
		convertSize = ConvertULong(fmt, width, precision, val);
	    }
	    else if (length == ShortLength)
	    {
		unsigned short int val = va_arg(args, unsigned int);
		convertSize = ConvertUShort(fmt, width, precision, val);
	    }
	    else
	    {
		unsigned int val = va_arg(args, unsigned int);
		convertSize = ConvertUInt(fmt, width, precision, val);
	    }
	}break;

	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
	{
	    char fmt[MaxFormatSize]; /* Flawfinder: ignore */

	    ConstructFormat(fmt, type, flags, length, precision);

	    double val = va_arg(args, double);
	    convertSize = ConvertDouble(fmt, width, precision, val);
	}break;

	case 'c':
	{
	    char fmt[MaxFormatSize]; /* Flawfinder: ignore */

	    ConstructFormat(fmt, type, flags, length, precision);

	    if (length == LongLength)
	    {
		wchar_t val = va_arg(args, int);
		convertSize = ConvertWChar(fmt, width, precision, val);
	    }
	    else
	    {
		char val = va_arg(args, int);
		convertSize = ConvertChar(fmt, width, precision, val);
	    }
	}break;

	case 'p':
	{
	    char fmt[MaxFormatSize]; /* Flawfinder: ignore */

	    ConstructFormat(fmt, type, flags, length, precision);

	    void* val = va_arg(args, void*);
	    convertSize = ConvertPtr(fmt, width, precision, val);
	}break;

	case '%':
	    convertSize = 1;
	    break;

    case 'S':
        {
            
            const wchar_t* pVal = va_arg(args, const wchar_t*);
            if(precision > 0)
            {
                // precision is the max value formatted irrespective of
                // string len being smaller or larger than preceision
                convertSize += precision;
            }
            else
            {
                if(pVal != NULL)
                {
                    convertSize = wcslen(pVal);
                }
                else
                {
                    // format will result in string "(null)"
                    // hence adjust size to the null str len
                    convertSize = strlen(FORMAT_NULL_STRING); 
                }
            } // End of if(precision > 0)
            break;
        }
        
    case 'I' :
        {
            char fmt[MaxFormatSize]; /* Flawfinder: ignore */
            ConstructFormat(fmt, type, flags, length, precision);
            
            //Assumption: syntax I64d or I64u
            strncat(fmt, pTmp, 3);
            
            if(*pTmp == 'd')
            {
                INT64 val = va_arg(args, INT64);
                
                convertSize = ConvertInt64(fmt, width, precision, val);
            }
            else 
            {
                UINT64 val = va_arg(args, UINT64);
                convertSize = ConvertUInt64(fmt, width, precision, val);
            }
            
            if(convertSize <= 0)
            {
                // Just in case, if a platform did not support
                convertSize = 512;
            }
            break;
        }

	default:
	{
	    HX_ASSERT(!"Unknown format type");
	    // Make up something large and hope that it's big enough
	    convertSize = 512;
	}break;
	};
	
    }

    if (ret)
    {
	charCount += (convertSize > width) ? convertSize : width;

	pCur = pTmp;
    }

    return ret;
}

#ifdef __amd64__
static int GuessSize(const char* pFormat, va_list args)
#else
static int GuessSize(const char* pFormat, va_list& args)
#endif
{
    int ret = 1;

    const char* pCur = pFormat;

    while(*pCur && ret != -1)
    {
	switch(*pCur) {
	case '%':
	    pCur++;

	    // Handle format characters
	    if (!ParseFormat(pCur, ret, args))
		ret = -1;
	    break;
	default:
	    ret++;
	    pCur++;
	    break;
	};
    }

    return ret;
}

void CHXString::Format(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    FormatV(pFmt, args);
    va_end(args);
}

// This fudge factor is added to the guess to protect us
// from guessing wrong
static const int FormatFudgeFactor = 128;

void CHXString::FormatV(const char* pFmt, va_list args)
{
#ifdef __amd64__
    va_list argList;

    va_copy(argList, args);
#else
    va_list argList = args;
#endif
    
    //Guess the size
    int estimatedSize = GuessSize(pFmt, args);

    if (m_pRep)
	m_pRep->Resize(estimatedSize + FormatFudgeFactor);
    else
	m_pRep = new CHXStringRep(estimatedSize + FormatFudgeFactor);

    int actualSize = vsnprintf(m_pRep->GetBuffer(), m_pRep->GetBufferSize(),
                               pFmt, argList);

    HX_ASSERT(actualSize < estimatedSize);

    m_pRep->SetStringSize(actualSize);

    FreeExtra();

    va_end(argList);
}

void CHXString::AppendFormat(const char* pFmt, ...)
{
    CHXString   strFormat;
    va_list     args;

    va_start(args, pFmt);
    strFormat.FormatV(pFmt, args);
    va_end(args);

    *this += strFormat;
}
