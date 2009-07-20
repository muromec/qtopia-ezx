/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsputil.cpp,v 1.11 2007/10/17 15:53:02 praveenkumar Exp $
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

#include "hxtypes.h"
#include "hxstring.h"
#include "dbcs.h"
#include "rtsputil.h"
#include "safestring.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*
 * Table for encoding base64
 */
static const char Base64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Table for encoding URL-safe base64
 */
static const char URLBase64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!*";

#define XX 127
/*
 * Table for decoding base64
 */
static const char Index64[256] = { /* Flawfinder: ignore */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,62,XX,XX, XX,XX,XX,XX, XX,XX,63,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX
};
#define CHAR64(c)  (Index64[(unsigned char)(c)])

static void 
Output64Chunk(int c1, int c2, int c3, int pads, char* pBuf,
    INT32 bufOffset)
{
    pBuf[bufOffset++] = Base64[c1>>2];
    pBuf[bufOffset++] = Base64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)];
    if (pads == 2) 
    {
        pBuf[bufOffset++] = '=';
        pBuf[bufOffset++] = '=';
    }
    else if (pads) 
    {
        pBuf[bufOffset++] = Base64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
        pBuf[bufOffset++] = '=';
    } 
    else 
    {
        pBuf[bufOffset++] = Base64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
        pBuf[bufOffset++] = Base64[c3 & 0x3F];
    }
}

static void 
OutputURL64Chunk(int c1, int c2, int c3, int pads, char* pBuf,
    INT32 bufOffset)
{
    pBuf[bufOffset++] = URLBase64[c1>>2];
    pBuf[bufOffset++] = URLBase64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)];
    if (pads == 2) 
    {
        pBuf[bufOffset++] = '=';
        pBuf[bufOffset++] = '=';
    }
    else if (pads) 
    {
        pBuf[bufOffset++] = URLBase64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
        pBuf[bufOffset++] = '=';
    } 
    else 
    {
        pBuf[bufOffset++] = URLBase64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
        pBuf[bufOffset++] = URLBase64[c3 & 0x3F];
    }
}

INT32 
BinTo64(const BYTE* pInBuf, INT32 len, char* pOutBuf)
{
    int c1, c2, c3;
    //INT32 ct = 0;
    //INT32 written = 0;
    INT32 inOffset = 0;
    INT32 outOffset = 0;

    while(inOffset < len)
    {
	c1 = pInBuf[inOffset++];
	if(inOffset == len)
	{
	    Output64Chunk(c1, 0, 0, 2, pOutBuf, outOffset);
	    outOffset += 4;
	}
	else
	{
	    c2 = pInBuf[inOffset++];
	    if(inOffset == len)
	    {
		Output64Chunk(c1, c2, 0, 1, pOutBuf, outOffset);
		outOffset += 4;
	    }
	    else
	    {
		c3 = pInBuf[inOffset++];
		Output64Chunk(c1, c2, c3, 0, pOutBuf, outOffset);
		outOffset += 4;
	    }
	}
#if XXXBAB
	ct += 4;
	if(ct > 44)	// EOL after 45 chars
	{
	    pOutBuf[outOffset++] = '\n';
	    written += 46;
	    ct = 0;
	}
#endif
    }
#if XXXBAB
    if(ct)
    {
	pOutBuf[outOffset++] = '\n';
	ct++;
    }
#endif
    pOutBuf[outOffset++] = '\0';
    return outOffset;
}

INT32 
BinToURL64(const BYTE* pInBuf, INT32 len, char* pOutBuf)
{
    int c1, c2, c3;
    INT32 inOffset = 0;
    INT32 outOffset = 0;

    while(inOffset < len)
    {
	c1 = pInBuf[inOffset++];
	if(inOffset == len)
	{
	    OutputURL64Chunk(c1, 0, 0, 2, pOutBuf, outOffset);
	    outOffset += 4;
	}
	else
	{
	    c2 = pInBuf[inOffset++];
	    if(inOffset == len)
	    {
		OutputURL64Chunk(c1, c2, 0, 1, pOutBuf, outOffset);
		outOffset += 4;
	    }
	    else
	    {
		c3 = pInBuf[inOffset++];
		OutputURL64Chunk(c1, c2, c3, 0, pOutBuf, outOffset);
		outOffset += 4;
	    }
	}
    }
    pOutBuf[outOffset++] = '\0';
    return outOffset;
}

INT32
BinFrom64(const char* pInBuf, INT32 len, BYTE* pOutBuf)
{
    int c1, c2, c3, c4;
    INT32 offset = 0;
    INT32 outOffset = 0;

    while(offset < len)
    {
	c1 = pInBuf[offset++];
	if(c1 != '=' && CHAR64(c1) == XX)
	{
	    continue;
	}
	else if (offset == len)
	{
	    /* BAD data */
	    return -1;
	}
	do
	{
	    c2 = pInBuf[offset++];
	} 
	while(offset < len && c2 != '=' && CHAR64(c2) == XX);
	if (offset == len)
	{
	    /* BAD data */
	    return -1;
	}
        do
	{
	    c3 = pInBuf[offset++];
	}
	while(offset < len && c3 != '=' && CHAR64(c3) == XX);
	if (offset == len)
	{
	    /* BAD data */
	    return -1;
	}
	do
	{
	    c4 = pInBuf[offset++];
	}
	while(offset < len && c4 != '=' && CHAR64(c4) == XX);
	if (offset == len && c4 != '=' && CHAR64(c4) == XX)
	{
	    /* BAD data */
	    return -1;
	}
	if(c1 == '=' || c2 == '=')
	{
	    continue;
	}
	c1 = CHAR64(c1);
	c2 = CHAR64(c2);
	pOutBuf[outOffset++]  = ((c1 << 2) | ((c2 & 0x30) >> 4));
	if(c3 == '=')
	{
	    continue;
	}
	else
	{
	    c3 = CHAR64(c3);
	    pOutBuf[outOffset++]  = (((c2 & 0x0f) << 4) | ((c3 & 0x3c) >> 2));
	    if(c4 == '=')
	    {
		continue;
	    }
	    else
	    {
		c4 = CHAR64(c4);
		pOutBuf[outOffset++] = (((c3 & 0x03) << 6) | c4);
	    }
	}
    }

    return outOffset;
}

const char*
EncodeCString(const char* pStr)
{
    CHXString tmp;

    for(size_t i=0;i<strlen(pStr);++i)
    {
        switch(pStr[i])
        {
            case '\n':
            {
                tmp += "\\n";
            }
            break;

            case '\t':
            {
                tmp += "\\t";
            }
            break;

            case '\r':
            {
                tmp += "\\r";
            }
            break;

            case '\"':
            {
                tmp += "\\\"";
            }
            break;

            case '\\':
            {
                tmp += "\\\\";
            }
            break;

            default:
            {
                tmp += pStr[i];
            }
        }
    }
    return tmp;
}


#ifdef XXXBAB

void
printbuf(unsigned char* pBuf, int len)
{
    for(int i=0; i<len; i++)
    {
	printf("%02x ", pBuf[i]);
    }
    printf("\n");
}

static unsigned char testBytes[] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
      0x4f, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x77, 0x78,
      0x45, 0x44, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
      0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xb3, 0x5f, 0x78,
      0x4f, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x77, 0x78,
      0x45, 0x44, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
      0x45, 0x44, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
      0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xb3, 0x5f, 0x78,
      0x4f, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x77, 0x78,
      0x45, 0x44, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
      0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xb3, 0x5f, 0x78,
      0x4f, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x77, 0x78,
      0x45, 0x44, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
      0x77, 0x88, 0x00 };

static unsigned char testBytes2[] =
    { 0x7F, 0x63, 0x31, 0xBB, 0x90, 0x21, 0x32, 0x11,
      0xC4 };
void main()
{
    unsigned char encodeBuf[300]; /* Flawfinder: ignore */
    unsigned char decodeBuf[300]; /* Flawfinder: ignore */

    memset(encodeBuf, '\0', sizeof(encodeBuf));
    memset(decodeBuf, '\0', sizeof(decodeBuf));

    int rc = BinTo64(testBytes, sizeof(testBytes), encodeBuf);
    printf("original size: %d, encoded size: %d\n", sizeof(testBytes), rc);
    printf("%s\n", encodeBuf);
    rc = BinFrom64(encodeBuf, rc, decodeBuf);
    printf("decode size: %d\n", rc);
    printbuf(decodeBuf, rc);
}

#endif /* XXXBAB */


const char escapeChars[] =
{       
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,1,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/*
 * Function: URLEscapeBuffer
 */
INT32 
URLEscapeBuffer(const char* pInBuf, INT32 len, char* pOutBuf)
{
    const char* pInputPos = pInBuf;
    char* pOutputPos = pOutBuf;
    char pTemp[3]; /* Flawfinder: ignore */
    
    if (!pInBuf || !len || !pOutBuf)
    {
	return -1;
    }

    // Iterate through the input buffer
    while (pInputPos < pInBuf + len)
    {
	HXBOOL bIsIllegal = FALSE;

	if (HXIsDBCSEnabled())
	{
	    // If an illegal URL character is found, translate it
	    bIsIllegal = 
	        HXIsEqual(pInputPos, 0x7f) ||
    		HXIsEqual(pInputPos, '%')  ||
    		HXIsEqual(pInputPos, '"')  ||
    		HXIsEqual(pInputPos, '+')  ||
    		HXIsEqual(pInputPos, '<')  ||
		HXIsEqual(pInputPos, '>')  ||
    		HXIsEqual(pInputPos, ',')  ||
    		HXIsEqual(pInputPos, '#')  ||
    		HXIsEqual(pInputPos, 0x01) ||
    		HXIsEqual(pInputPos, 0x02) ||
    		HXIsEqual(pInputPos, 0x03) ||
    		HXIsEqual(pInputPos, 0x04) ||
    		HXIsEqual(pInputPos, 0x05) ||
    		HXIsEqual(pInputPos, 0x06) ||
    		HXIsEqual(pInputPos, 0x07) ||
    		HXIsEqual(pInputPos, 0x08) ||
    		HXIsEqual(pInputPos, 0x09) ||
    		HXIsEqual(pInputPos, 0x0a) ||
    		HXIsEqual(pInputPos, 0x0b) ||
    		HXIsEqual(pInputPos, 0x0c) ||
    		HXIsEqual(pInputPos, 0x0d) ||
    		HXIsEqual(pInputPos, 0x0e) ||
    		HXIsEqual(pInputPos, 0x0f) ||
    		HXIsEqual(pInputPos, 0x10) ||
    		HXIsEqual(pInputPos, 0x11) ||
    		HXIsEqual(pInputPos, 0x12) ||
    		HXIsEqual(pInputPos, 0x13) ||
    		HXIsEqual(pInputPos, 0x14) ||
    		HXIsEqual(pInputPos, 0x15) ||
    		HXIsEqual(pInputPos, 0x16) ||
    		HXIsEqual(pInputPos, 0x17) ||
    		HXIsEqual(pInputPos, 0x18) ||
    		HXIsEqual(pInputPos, 0x19) ||
    		HXIsEqual(pInputPos, 0x1a) ||
    		HXIsEqual(pInputPos, 0x1b) ||
    		HXIsEqual(pInputPos, 0x1c) ||
    		HXIsEqual(pInputPos, 0x1d) ||
    		HXIsEqual(pInputPos, 0x1e) ||
    		HXIsEqual(pInputPos, 0x1f);
	}
	else
	{
	    // Use a lookup table for improved performance
	    bIsIllegal = (HXBOOL)escapeChars[(UCHAR)*pInputPos];
	}

	if (bIsIllegal)
	{
	    SafeSprintf(pTemp, sizeof(pTemp), "%02x", *pInputPos);

	    *pOutputPos++ = '%';
	    *pOutputPos++ = pTemp[0];
	    *pOutputPos++ = pTemp[1];
	}
	else if (HXIsEqual(pInputPos, ' '))
	{
	    *pOutputPos++ = '+';
	}
	else
	{
	    *pOutputPos++ = *pInputPos;

	    if (HXIsLeadByte(*pInputPos))
	    {
	    	*pOutputPos++ = *(pInputPos + 1);
	    }
	}

	pInputPos = HXGetNextChar(pInputPos);
    }

    // Return the length of the escaped content
    return (pOutputPos - pOutBuf);
}

const char escapeCharsNoReserved[] =
{ //8,9,A,B,C,D,E,F
    0,1,1,1,1,1,1,1, // 0x00
    1,1,1,1,1,1,1,1, // 0x08
    1,1,1,1,1,1,1,1, // 0x10
    1,1,1,1,1,1,1,1, // 0x18
    1,0,1,1,0,1,0,0, // 0x20
    0,0,0,0,0,0,0,0, // 0x28
    0,0,0,0,0,0,0,0, // 0x30
    0,0,0,0,1,0,1,0, // 0x38
    0,0,0,0,0,0,0,0, // 0x40
    0,0,0,0,0,0,0,0, // 0x48
    0,0,0,0,0,0,0,0, // 0x50
    0,0,0,1,1,1,1,0, // 0x58
    1,0,0,0,0,0,0,0, // 0x60
    0,0,0,0,0,0,0,0, // 0x68
    0,0,0,0,0,0,0,0, // 0x70
    0,0,0,1,1,1,1,1, // 0x78
    1,1,1,1,1,1,1,1, // 0x80
    1,1,1,1,1,1,1,1, // 0x88
    1,1,1,1,1,1,1,1, // 0x90
    1,1,1,1,1,1,1,1, // 0x98
    1,1,1,1,1,1,1,1, // 0xA0
    1,1,1,1,1,1,1,1, // 0xA8
    1,1,1,1,1,1,1,1, // 0xB0
    1,1,1,1,1,1,1,1, // 0xB8
    1,1,1,1,1,1,1,1, // 0xC0
    1,1,1,1,1,1,1,1, // 0xC8
    1,1,1,1,1,1,1,1, // 0xD0
    1,1,1,1,1,1,1,1, // 0xD8
    1,1,1,1,1,1,1,1, // 0xE0
    1,1,1,1,1,1,1,1, // 0xE8
    1,1,1,1,1,1,1,1, // 0xF0
    1,1,1,1,1,1,1,1  // 0xF8
};


const char escapeCharsReserved[] =
{ //8,9,A,B,C,D,E,F     
    0,1,1,1,1,1,1,1, // 0x00
    1,1,1,1,1,1,1,1, // 0x08
    1,1,1,1,1,1,1,1, // 0x10
    1,1,1,1,1,1,1,1, // 0x18
    1,0,1,1,0,1,1,0, // 0x20
    0,0,0,0,0,0,0,1, // 0x28
    0,0,0,0,0,0,0,0, // 0x30
    0,0,1,1,1,1,1,1, // 0x38
    1,0,0,0,0,0,0,0, // 0x40
    0,0,0,0,0,0,0,0, // 0x48
    0,0,0,0,0,0,0,0, // 0x50
    0,0,0,1,1,1,1,0, // 0x58
    1,0,0,0,0,0,0,0, // 0x60
    0,0,0,0,0,0,0,0, // 0x68
    0,0,0,0,0,0,0,0, // 0x70
    0,0,0,1,1,1,1,1, // 0x78
    1,1,1,1,1,1,1,1, // 0x80
    1,1,1,1,1,1,1,1, // 0x88
    1,1,1,1,1,1,1,1, // 0x90
    1,1,1,1,1,1,1,1, // 0x98
    1,1,1,1,1,1,1,1, // 0xA0
    1,1,1,1,1,1,1,1, // 0xA8
    1,1,1,1,1,1,1,1, // 0xB0
    1,1,1,1,1,1,1,1, // 0xB8
    1,1,1,1,1,1,1,1, // 0xC0
    1,1,1,1,1,1,1,1, // 0xC8
    1,1,1,1,1,1,1,1, // 0xD0
    1,1,1,1,1,1,1,1, // 0xD8
    1,1,1,1,1,1,1,1, // 0xE0
    1,1,1,1,1,1,1,1, // 0xE8
    1,1,1,1,1,1,1,1, // 0xF0
    1,1,1,1,1,1,1,1  // 0xF8
};

/*
 * Function: URLEscapeBufferReserved  this function will escape all potentially unsafe charactors.
 */
INT32 
URLEscapeBuffer2(const char* pInBuf, INT32 len, char* pOutBuf, HXBOOL bReserved)
{
    const char* pInputPos = pInBuf;
    char* pOutputPos = pOutBuf;
    char pTemp[3];
    
    if (!pInBuf || !len || !pOutBuf)
    {
	return -1;
    }

    const char* lookupTable = NULL;
    if (bReserved)
    {
	lookupTable = (const char*)escapeCharsReserved;
    }
    else
    {
	lookupTable = (const char*)escapeCharsNoReserved;
    }
    
 

    // Iterate through the input buffer
    while (pInputPos < pInBuf + len)
    {
	// Use a lookup table for improved performance
	HXBOOL bIsIllegal = (HXBOOL)lookupTable[(UCHAR)*pInputPos];

	if (bIsIllegal)
	{
	    SafeSprintf(pTemp, sizeof(pTemp), "%02x", *pInputPos);

	    *pOutputPos++ = '%';
	    *pOutputPos++ = pTemp[0];
	    *pOutputPos++ = pTemp[1];
	}
	else
	{
	    *pOutputPos++ = *pInputPos;
	}

	++pInputPos;
    }

    // Return the length of the escaped content
    return (pOutputPos - pOutBuf);
}


/*
 * Function: URLUnescapeBuffer
 */
INT32 
URLUnescapeBuffer(const char* pInBuf, INT32 len, char* pOutBuf)
{
    char* pOutputPos = pOutBuf;
    char pTemp[3];

    if (!pInBuf || !len || !pOutBuf)
    {
	return -1;
    }

    // Iterate through the input buffer
    for (INT32 i = 0; i < len; i++)
    {
	// Ignore whitespace
	if ((UCHAR)pInBuf[i] < 21)
	{
	    continue;
	}
	// If an escaped character is found, translate it
	else if (pInBuf[i] == '%')
	{
	    if (len < i + 3)
	    {
		// Incomplete %xx representation
		return -1;
	    }
    
	    // Ignore whitespace
	    while (pInBuf[i + 1] < 21) 
	    { 
		i++;
		if (len < i + 3)
		{
		    return -1;
		}
	    }
	    pTemp[0] = pInBuf[i + 1];

	    // Ignore whitespace
	    while (pInBuf[i + 2] < 21) 
	    { 
		i++;
		if (len < i + 3)
		{
		    return -1;
		}
	    }
	    pTemp[1] = pInBuf[i + 2];

	    pTemp[2] = '\0';
	    *pOutputPos++ = (char)strtol(pTemp, NULL, 16);

	    i += 2;
	}
	else if (pInBuf[i] == '+')
	{
	    *pOutputPos++ = ' ';
	}
	else
	{
	    *pOutputPos++ = pInBuf[i];
	}
    }

    // Return the length of the escaped content
    return (pOutputPos - pOutBuf);
}


#ifdef XXXDPS

static const char* pTestTable[] =
{
    "this is a test, this is only a test",
    "abcdefghijklmnopqrstuvwxyz0123456789/NoneOfThisShouldGetEscaped...",
    "\r\n\t,#><+\"%All Of The First 10 Chars Should Get Escaped",
    "Insert a DBCS test string here."
};

void main()
{
    unsigned char encodeBuf[300]; /* Flawfinder: ignore */
    unsigned char decodeBuf[300]; /* Flawfinder: ignore */

    for (int i = 0; i < 4; i++)
    {
	memset(encodeBuf, '\0', sizeof(encodeBuf));
	memset(decodeBuf, '\0', sizeof(decodeBuf));

	printf("Escaping >>>%s<<<\n", pTestTable[i]);
	int rc = URLEscapeBuffer((char*)pTestTable[i], strlen(pTestTable[i]), (char*)encodeBuf);
	printf("original size: %d, encoded size: %d\n", strlen(pTestTable[i]), rc);
	printf("%s\n", encodeBuf);
	rc = URLUnescapeBuffer((char*)encodeBuf, rc, (char*)decodeBuf);
	printf("decode size: %d\n", rc);
	printf("%s\n", decodeBuf);
	printf("\n");
    }
}

#endif /* XXXDPS */
