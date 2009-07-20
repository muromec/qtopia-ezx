/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdptools.cpp,v 1.11 2005/08/02 18:00:37 albertofloyd Exp $
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

/****************************************************************************
 *  Defines
 */
#define AUDIO_MIMETYPE_PREFIX	    "audio/"
#define AUDIO_MIMETYPE_PREFIX_SIZE  (sizeof(AUDIO_MIMETYPE_PREFIX) - 1)
#define VIDEO_MIMETYPE_PREFIX	    "video/"
#define VIDEO_MIMETYPE_PREFIX_SIZE  (sizeof(VIDEO_MIMETYPE_PREFIX) - 1)
#define APP_MIMETYPE_PREFIX	    "application/"
#define APP_MIMETYPE_PREFIX_SIZE    (sizeof(APP_MIMETYPE_PREFIX) - 1)


/****************************************************************************
 *  Includes
 */
//#include "hlxclib/stdlib.h"

#include "sdptools.h"

#include "hxstrutl.h"
#include "hxassert.h"
#include "hxstring.h"
#include "hxinline.h"


/****************************************************************************
 *  Locals
 */
HX_INLINE HX_RESULT _HexCharPairToByte(UINT8* pByte, const char* pCharPair);
HX_INLINE void _ByteToHexCharPair(const UINT8 pByte, char* pCharPair);


/****************************************************************************
 *  Tools
 */
HX_RESULT HexStringToBinary(UINT8* pOutBuffer, const char* pString)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pOutBuffer);
    HX_ASSERT(pString);

    while (SUCCEEDED(retVal) && ((*pString) != '\0'))
    {
	retVal = _HexCharPairToByte(pOutBuffer, pString);

	pOutBuffer++;
	pString += 2;
    }

    return retVal;
}


HX_RESULT HexCharPairToByte(UINT8* pByte, const char* pCharPair)
{
    return _HexCharPairToByte(pByte, pCharPair);
}

HX_INLINE HX_RESULT _HexCharPairToByte(UINT8* pByte, const char* pCharPair)
{
    UINT8 uIdx = 2;
    UINT8 uByte = 0;
    UINT8 uAddVal;
    char cBits;

    do
    {
	uIdx--;

	cBits = *pCharPair;

	if ((cBits >= '0') && (cBits <= '9'))
	{
	    uAddVal = (UINT8)(cBits - '0');
	}
	else if ((cBits >= 'a') && (cBits <= 'f'))
	{
	    uAddVal = (UINT8)(cBits - 'a' + 10);
	}
	else if ((cBits >= 'A') && (cBits <= 'F'))
	{
	    uAddVal = (UINT8)(cBits - 'A' + 10);
	}
	else
	{
	    return HXR_FAIL;
	}

	uByte += (UINT8)(uAddVal << (4 * uIdx));
	pCharPair++;
    } while ((uIdx != 0) && (*pCharPair != '\0'));

    *pByte = uByte;

    return HXR_OK;
}

void BinaryToHexString(const UINT8* pBuffer, UINT32 ulSize, char* pString)
{
    HX_ASSERT(pBuffer);
    HX_ASSERT(pString);

    for(UINT32 i = 0; i < ulSize; i++)
    {
	_ByteToHexCharPair(pBuffer[i], &pString[i*2]);
    }

    pString[ulSize*2] = '\0';
}

void ByteToHexCharPair(const UINT8 uByte, char* pCharPair)
{
    _ByteToHexCharPair(uByte, pCharPair);
}

HX_INLINE void _ByteToHexCharPair(const UINT8 uByte, char* pCharPair)
{
    HX_ASSERT(pCharPair);

    char c = (char)((uByte >> 4) & 0x0f);
    pCharPair[0] = (char)(c < 0xa ? c + '0' : c - 0xa + 'a');
    c = (char)(uByte & 0x0f);
    pCharPair[1] = (char)(c < 0xa ? c + '0' : c - 0xa + 'a');
}

RTSPMediaType SDPMapMimeToMediaType(const char* pMimeType)
{
    RTSPMediaType eMediaType = RTSPMEDIA_TYPE_UNKNOWN;

    if (pMimeType)
    {
	if (strncasecmp(AUDIO_MIMETYPE_PREFIX, 
			pMimeType, 
			AUDIO_MIMETYPE_PREFIX_SIZE) == 0)
	{
	    eMediaType = RTSPMEDIA_TYPE_AUDIO;
	}
	else if (strncasecmp(VIDEO_MIMETYPE_PREFIX, 
			pMimeType, 
			VIDEO_MIMETYPE_PREFIX_SIZE) == 0)
	{
	    eMediaType = RTSPMEDIA_TYPE_VIDEO;
	}
	else if (strncasecmp(APP_MIMETYPE_PREFIX, 
			pMimeType, 
			APP_MIMETYPE_PREFIX_SIZE) == 0)
	{
	    eMediaType = RTSPMEDIA_TYPE_APP;
	}
    }

    return eMediaType;
}

/*
 * Copies SDPData from pSDP tp pNewSDP, removing any tokens matching pTokens
 * pTokens is a NULL terminated list of strings
 */
UINT32 RemoveSDPTokens(const char* pTokens[], const UINT32 ulTokenLen[],
                      const char* pSDP, UINT32 ulSDPLen, 
                      char* pNewSDP, UINT32 ulNewSDPSize)
{
    UINT32 i = 0;
    UINT32 ulEnd;
    UINT32 ulCopySize;
    char* pWriter = pNewSDP;

    while(pSDP[i] != '\0' && i < ulSDPLen)
    {
        // ignore trailing white space
        while( i < ulSDPLen && isspace(pSDP[i]) ) 
        { 
            i++;
        }

        // Find the end of the line
        for(ulEnd = i; ulEnd < ulSDPLen && pSDP[ulEnd] != '\0'; ulEnd++)
        {
            if(pSDP[ulEnd] == '\r' && ulEnd + 1 < ulSDPLen && 
                pSDP[ulEnd + 1] == '\n')
            {
                ulEnd +=2;
                break;
            }
        }

        // See if this one is to be removed
        HXBOOL bRemove = FALSE;
        for(int j=0; pTokens[j] && !bRemove; j++)
        {
            if(ulTokenLen[j] < ulSDPLen - i &&
                strncasecmp(&pSDP[i], pTokens[j], ulTokenLen[j]) == 0)
            {
                bRemove = TRUE;
            }
        }

        // copy it if it doesn't match a remove token
        if(!bRemove)
        {
            HX_ASSERT(strncasecmp(&pSDP[i], "a=", 2) == 0);

            ulCopySize = ulEnd - i;
            if(ulCopySize < ulNewSDPSize)
            {
                memcpy(pWriter, &pSDP[i], ulCopySize);
                ulNewSDPSize -= ulCopySize;
                pWriter += ulCopySize;
            }
        }
        i = ulEnd;
    }

    return (UINT32)(pWriter - pNewSDP);
}
