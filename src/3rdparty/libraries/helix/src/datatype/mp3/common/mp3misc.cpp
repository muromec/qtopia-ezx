/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hxtypes.h"
#include "audinfo.h"
#include "mp3misc.h"
#include "dbcsutil.h"
#include "hxassert.h"

eHeaderType     CMp3Misc::GetHeaderType() {return m_eHeaderType;}

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)

static char* StrNStr(const char* str1,
                     const char* str2,
                     size_t depth1,
                     size_t depth2);

char* StrNStr(const char* str1,
              const char* str2,
              size_t depth1,
              size_t depth2)
{
    const char *tracer1;
    const char *tracer2;
    size_t depth_tracer1;
    size_t depth_tracer2;

    while(depth1 && *str1)
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

	if ((depth_tracer1 == 1) || (*tracer1 == '\0'))
	{
	    return NULL;
	}

	str1++;
	depth1--;
    }

    return NULL;
}

#endif /* #if defined(HELIX_FEATURE_MP3FF_SHOUTCAST) */

CMp3Misc::CMp3Misc()
 :  m_pFmt(NULL)
 ,  m_pTitle(NULL)
 ,  m_pArtist(NULL)
 ,  m_pAlbum(NULL)
 ,  m_pYear(NULL)
 ,  m_pGenre(NULL)
 ,  m_nMetaRepeat(0)
 ,  m_nMetaOffset(0)
 ,  m_eHeaderType(eNone)

{
    PrepareString(m_pTitle);
    PrepareString(m_pArtist);
    PrepareString(m_pAlbum);
    PrepareString(m_pYear);
    PrepareString(m_pGenre);
}

CMp3Misc::~CMp3Misc()
{
    HX_VECTOR_DELETE(m_pTitle);
    HX_VECTOR_DELETE(m_pArtist);
    HX_VECTOR_DELETE(m_pAlbum);
    HX_VECTOR_DELETE(m_pYear);
    HX_VECTOR_DELETE(m_pGenre);
}

HXBOOL CMp3Misc::CheckForHeaders(UINT8 *pBuf,
                               UINT32 dwSize,
                               INT32 &lHeaderSize)
{
    HXBOOL    cRet = 1;

    // Check for ID3v2 tags
    UINT32 ulTag = (pBuf[0] << 16) |
                   (pBuf[1] <<  8) |
                    pBuf[2];
#if defined(HELIX_FEATURE_MP3FF_LENIENT)
    const char* pRIFF    = "RIFF";         // length 4
    const char* pWAVE    = "WAVEfmt";      // length 7
    const char* pData    = "data";         // length 4
#endif /* #if defined(HELIX_FEATURE_MP3FF_LENIENT) */

    if (ulTag == MP3_TAG_ID3)
    {
        m_eHeaderType = eID3v2;

        lHeaderSize = GetTagLength(&pBuf[6], dwSize - 6) + 10;

        GetId3v2Values(pBuf, dwSize, pBuf[3]);
    }

    // Check for ID3v1 tags
    else if (ulTag == MP3_TAG_TAG)
    {
        m_eHeaderType = eID3v1;

        GetId3v1Values(pBuf+3, dwSize-3);
        lHeaderSize = -1;
    }
#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
    // Check for ShoutCast server files
    else
    if ( ulTag == MP3_TAG_ICY ||

          StrNStr((const char*)pBuf,"icy-metaint", dwSize, 11)
       )
    {
        const char* pFull    = "Server Full";  // length 11
        const char* pICYMeta = "icy-metaint:"; // length 12
        const char* pICYName = "icy-name:";    // length 9
        m_eHeaderType = eShoutCast;

        if (!memcmp(&pBuf[8], pFull, 11))
        {
            lHeaderSize = 0;
        }
        else
        {
            INT32 nSize = dwSize;

            // Look for meta data repeat size
            for (INT32 nTemp=0; nSize>=5; --nSize, ++nTemp)
            {
                // If we find the end, set m_nMetaOffset and stop
                if (pBuf[nTemp] == 0x0D && pBuf[nTemp+1] == 0x0A) 
                {
                     if (pBuf[nTemp+2] == 0x0D && pBuf[nTemp+3] == 0x0A)
                     {
                        m_nMetaOffset = nTemp + 4;
                        break;
                     }
                     else if (pBuf[nTemp+3] == 0x0D && pBuf[nTemp+4] == 0x0A)
                     {
                        m_nMetaOffset = nTemp + 5;
                        break;
                     }
                }
                else 
                if (nSize > 12 &&
                    !memcmp(&pBuf[nTemp],pICYMeta,12))
                {
                    nTemp += 12;
                    nSize -= 12;
                    
                    // Skip non-numubers
                    while (nSize && (pBuf[nTemp] < 0x30 || pBuf[nTemp] > 0x39))
                    {
                        ++nTemp;
                        --nSize;
                    }                                    
                    
                    m_nMetaRepeat = atoi((const char*)&pBuf[nTemp]);
                
                    // Find end of metaint data
                    while (pBuf[nTemp+1] != 0x0D && nSize > 4)
                    {
                        ++nTemp;
                        --nSize;
                    }                                    
                }
                else
                if (nSize > 9 &&
                    !memcmp(&pBuf[nTemp],pICYName,9))
                {
                    UINT32 nTitle, nLen;
                    nTemp += 9;
                    nTitle = nTemp;
                    
                    while (pBuf[nTemp+1] != 0x0D && nSize > 4)
                    {
                        ++nTemp;
                        --nSize;
                    }
                    
                    // If the field is too INT32 for our buffer, truncate it.
                    nLen = HX_MIN(nTemp-nTitle+1, 256);

                    dbcsStrCopy((char*)m_pTitle, (const char*)(pBuf+nTitle), nLen);
                    
                    // Skip 0x0A
                    ++nTemp;
                    --nSize;
                }
            }
            
            HX_ASSERT(m_pFmt);

	    if (m_nMetaRepeat > 0 && m_nMetaOffset == 0)
	    {
		// badly formed meta data, offset should be header size
		m_nMetaRepeat = 0;
	    }

            int nFrame = 0;
            lHeaderSize = m_pFmt->ScanForSyncWord(pBuf, dwSize, nFrame);

            if (lHeaderSize < 0)
                cRet = 0;
        }
    }
#endif /* #if defined(HELIX_FEATURE_MP3FF_SHOUTCAST) */
#if defined(HELIX_FEATURE_MP3FF_LENIENT)
    // Check for bogus RIFFWav MP3s
    else
    if (!memcmp(pBuf,pRIFF,4) &&
        !memcmp(&pBuf[8],pWAVE,7))
    {

        m_eHeaderType = eOther;

        // Default value
        lHeaderSize = 72;

        // Search for the 'data' section of the RIFF file.
        // The data is 8 bytes after it.
        UINT8 *pTemp = pBuf;

        while (dwSize >= 4)
        {
            if (!memcmp(pTemp,pData,4))
            {

                lHeaderSize = pTemp - pBuf + 8;
                break;
            }
            else
            {
                ++pTemp;
                --dwSize;
            }
        }
    }
    
    // Check for bogus stuff
    else if (ulTag == MP3_TAG_jpg ||
             ulTag == MP3_TAG_bmp)
    {
        m_eHeaderType = eOther;
        lHeaderSize = -1;
    }
#endif /* #if defined(HELIX_FEATURE_MP3FF_LENIENT) */
    else
    {
        cRet = 0;
        lHeaderSize = 0;
    }

    return cRet;
}

UINT8* CMp3Misc::GetId3Title(int &nLen)
{
    return GetId3String(m_pTitle, nLen);
}

UINT8* CMp3Misc::GetId3Artist(int &nLen)
{
    return GetId3String(m_pArtist, nLen);
}

UINT8* CMp3Misc::GetId3Album(int &nLen)
{
    return GetId3String(m_pAlbum, nLen);
}

UINT8* CMp3Misc::GetId3Genre(int &nLen)
{
    return GetId3String(m_pGenre, nLen);
}

UINT8* CMp3Misc::GetId3String(UINT8* pStr, int& nLen)
{
    nLen = 0;

    if (!pStr)
        return NULL;

    nLen = strlen((const char*)pStr);
    return pStr;
}

void CMp3Misc::PrepareString(UINT8*& rpStr)
{
    rpStr = new UINT8[257];
    if (rpStr)
        rpStr[0] = '\0';
}

void CMp3Misc::GetId3v1Values(UINT8 *pBuf,
                                UINT32 ulSize)
{
    // Get Title
    GetId3EntryAndAdvance(m_pTitle, eTitleLen, pBuf, ulSize);

    // Get Artist
    GetId3EntryAndAdvance(m_pArtist, eArtistLen, pBuf, ulSize);

    // Get Album
    GetId3EntryAndAdvance(m_pAlbum, eAlbumLen, pBuf, ulSize);
}

void CMp3Misc::GetId3EntryAndAdvance(UINT8*  pStr,
                                     UINT32  ulLen,
                                     UINT8*& rpBuf,
                                     UINT32& rulSize)
{
    if (rulSize >= ulLen)
    {
        GetId3Entry(rpBuf, pStr, ulLen);
        rpBuf   += ulLen;
        rulSize -= ulLen;
    }
}

void CMp3Misc::GetId3Entry(UINT8 *pBuf,
                           UINT8 *pEntry,
                           int nEntryLen)
{	
    dbcsStrCopy((char*)pEntry, (const char*)pBuf, nEntryLen);
}

void CMp3Misc::GetId3v2Values(UINT8 *pBuf,
                              UINT32 ulSize,
                              int nVersion)
{
    UINT32 count        = 0;
    UINT32 tagLength    = 0;
    UINT32 ulMinVer     = pBuf[3];
    UINT32 ulNotDone    = 0x1F;
    UINT8* ppStrArr[5]  = {m_pTitle,     m_pArtist,    m_pAlbum,     m_pYear,      m_pGenre};
    UINT32 ulVer2Tag[5] = {MP3_TAG_TT2,  MP3_TAG_TP1,  MP3_TAG_TAL,  MP3_TAG_TYE,  MP3_TAG_TCO};
    UINT32 ulVer3Tag[5] = {MP3_TAG_TIT2, MP3_TAG_TPE1, MP3_TAG_TALB, MP3_TAG_TYER, MP3_TAG_TCON};
    UINT32 ulVer4Tag[5] = {MP3_TAG_TIT2, MP3_TAG_TPE1, MP3_TAG_TALB, MP3_TAG_TDRC, MP3_TAG_TCON};

    if (ulMinVer == 2 || ulMinVer == 3 || ulMinVer == 4)
    {
        // Get the tag lookup table
        UINT32* pTag;
        if (ulMinVer == 2)
        {
            pTag = &ulVer2Tag[0];
        }
        else if (ulMinVer == 3)
        {
            pTag = &ulVer3Tag[0];
        }
        else if (ulMinVer == 4)
        {
            pTag = &ulVer4Tag[0];
        }
        // a tag's size is given in a 4-byte field of the following form:
        // 0xxxxxxx0xxxxxxx0xxxxxxx0xxxxxxx where x is a value of 0 or 1
        // the 0's are there to avoid potential sync problems
        // this means we have bitshift in multiples of 7 rather than 8
        tagLength = GetTagLength(&pBuf[count+6], ulSize - count - 6);
        // tagLength gives the length of the tag from the point it's specified, so we need
        // to add 10 to make it the total from the start of the file
        count     += 10;
        tagLength += 10;
        tagLength  = HX_MIN(tagLength, ulSize - 4);

        while (count < tagLength && ulNotDone)
        {
            // Create a 32-bit tag
            UINT32 ulTag = (pBuf[count]   << 24) |
                           (pBuf[count+1] << 16) |
                           (pBuf[count+2] <<  8) |
                            pBuf[count+3];
            // If we are v3.2, then we only check three bytes
            if (ulMinVer == 2) ulTag &= 0xFFFFFF00;
            // Loop through and check for tags
            for (UINT32 i = 0; i < 5; i++)
            {
                if (ulTag == pTag[i])
                {
                    if (ulMinVer == 2)
                    {
                        GetId3v2_2Entry(pBuf, ppStrArr[i], count);
                    }
                    else if (ulMinVer == 3)
                    {
                        GetId3v2_3Entry(pBuf, ppStrArr[i], count);
                    }
                    else if (ulMinVer == 4)
                    {
                        GetId3v2_4Entry(pBuf, ppStrArr[i], count);
                    }
                    ulNotDone &= ~(1 << i);
                    break;
                }
            }
            count++;
        }
    }
}

void CMp3Misc::GetId3v2_2Entry(UINT8 *pBuf,
                               UINT8 *pEntry,
                               UINT32 &count)
{
    // Data size is at byte 5 and includes the language encoding byte, so subtract one
    // Data starts at byte 6 with lang byte, which we ignore
    GetId3v2_XEntry(pBuf, pEntry, count, 5, 7, 6);
}

void CMp3Misc::GetId3v2_3Entry(UINT8 *pBuf,
                                 UINT8 *pEntry,
                                 UINT32 &count)
{
    // Data size is at byte 7 and includes the language encoding byte, so subtract one
    // Data starts at byte 10 with lang byte, which we ignore
    GetId3v2_XEntry(pBuf, pEntry, count, 7, 11, 10);
}

void CMp3Misc::GetId3v2_4Entry(UINT8 *pBuf,
                                 UINT8 *pEntry,
                                 UINT32 &count)
{
    GetId3v2_XEntry(pBuf, pEntry, count, 7, 11, 10);
}

void CMp3Misc::GetId3v2_XEntry(UINT8* pBuf, UINT8* pEntry, UINT32& count,
                               INT32 a, INT32 b, INT32 c)
{
    // Data size is at byte a and includes the language encoding byte, so subtract one
    UINT8 nFieldLen = pBuf[count + a]-1;
    // Data starts at byte c with lang byte, which we ignore
    count += dbcsStrCopy((char*)pEntry, (const char*)(pBuf+count+b), nFieldLen) + c;
}

UINT32 CMp3Misc::GetTagLength(UINT8* pBuf, UINT32 ulLen)
{
    UINT32 ulRet = 0;

    if (ulLen >= 4)
    {
        ulRet = ((pBuf[0] & 0x7F) << 21) +
                ((pBuf[1] & 0x7F) << 14) + 
                ((pBuf[2] & 0x7F) <<  7) +
                 (pBuf[3] & 0x7F);
    }

    return ulRet;
}
