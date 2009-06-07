/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: urlutil.cpp,v 1.6 2004/07/09 19:14:47 jgordon Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hlxclib/stdlib.h"
#include "hlxclib/ctype.h"
#include "hlxclib/string.h"

#include "hxassert.h"
#include "urlutil.h"

char x2c(char *what)
{
  register char digit;

  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
  return(digit);
}

void unescape_url_without_plus(char *url)
{
  register int x,y;

  for (x=0,y=0; url[y]; ++x,++y) {
    if((url[x] = url[y]) == '%') {
      url[x] = x2c(&url[y+1]);
      y+=2;
    }
  }
  url[x] = '\0';
}


/*
 * An (hopefully) efficient url decoder
 * In:  pEnc    = encoded url buffer
 *      nEncLen = length of encoded url buffer, not including terminator
 * Out: szDec   = filled in with decoded url and null terminated
 *
 * Note that in the worst case (which is also the normal case), no encoding
 * will be used and pDec will require one more byte of space than pEnc (for
 * the terminator).
 */
void
DecodeURL(const BYTE* pEnc, size_t nEncLen, char* szDec)
{
    BYTE* pDec = (BYTE*)szDec;
    const BYTE* pEnd = pEnc+nEncLen;
    while (pEnc < pEnd)
    {
        if (*pEnc == '%' && pEnc + 2 < pEnd && isxdigit(pEnc[1]) && 
            isxdigit(pEnc[2]))
        {
            unsigned int c1, c2;
            c1 = isdigit(pEnc[1]) ? pEnc[1]-'0' : 10+tolower(pEnc[1])-'a';
            c2 = isdigit(pEnc[2]) ? pEnc[2]-'0' : 10+tolower(pEnc[2])-'a';
            *pDec = c1*16 + c2;
            pEnc += 3;
        }
        else
        {
            *pDec = *pEnc;
            pEnc++;
        }
        if (*pDec < 0x20)
        {
            // Somebody is playing games, convert CTLs to dots
            *pDec = '.';
        }
        pDec++;
    }
    *pDec = '\0';
}

void
DecodeURL(const char* pEnc, char* szDec)
{
    DecodeURL((const BYTE*)pEnc, strlen(pEnc), szDec);
}

static BOOL
ParseStreamId(const char* pBuf, const char** ppStreamId)
{
    if (pBuf == NULL)
    {
        HX_ASSERT(FALSE);
        return FALSE;
    }

    // [A-Za-z]+
    if (!isalpha(*pBuf))
    {
        return FALSE;
    }
    while (isalpha(*pBuf))
    {
        pBuf++;
    }

    // '='
    if (*pBuf != '=')
    {
        return FALSE;
    }
    pBuf++;

    // [0-9]+
    if (!isdigit(*pBuf))
    {
        return FALSE;
    }
    *ppStreamId = pBuf;
    while (isdigit(*pBuf))
    {
        pBuf++;
    }

    // Must be end of url or query params
    if (*pBuf != '\0' && *pBuf != '?')
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsStreamId(const char* pBuf)
{
    const char* pStreamId;
    return ParseStreamId(pBuf, &pStreamId);
}

BOOL
GetStreamId(const char* pBuf, UINT32* puStreamId)
{
    BOOL rc;
    const char* pStreamId;
    if ((rc = ParseStreamId(pBuf, &pStreamId)))
    {
        *puStreamId = (UINT32)strtoul(pStreamId, NULL, 10);
    }
    return rc;
}
