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

#ifndef _MP3MISC_H_
#define _MP3MISC_H_

typedef enum
{
    eNone,
    eID3v1,
    eID3v2,
    eShoutCast,
    eIceCast,
    eOther
} eHeaderType;

class CAudioInfoBase;

class CMp3Misc
{
public:
    CMp3Misc();
    virtual ~CMp3Misc();

    void        SetParent(CAudioInfoBase* pFmt) {m_pFmt = pFmt;}

    virtual 
    HXBOOL        CheckForHeaders(UINT8 *pBuf,
                                UINT32 dwSize,
                                INT32 &lHeaderSize);

    void        GetId3v1Values(UINT8 *pBuf,
                               UINT32 ulSize);

    void        GetId3Entry(UINT8 *pBuf,
                            UINT8 *pEntry,
                            int nEntryLen);
    void        GetId3EntryAndAdvance(UINT8*  pStr,
                                      UINT32  ulLen,
                                      UINT8*& rpBuf,
                                      UINT32& rulSize);
    
    void        GetId3v2Values(UINT8 *pBuf,
                               UINT32 ulSize,
                               int nVersion);
    void        GetId3v2_4Entry(UINT8 *pBuf,
                                UINT8 *pEntry,
                                UINT32 &count);
    void        GetId3v2_3Entry(UINT8 *pBuf,
                                UINT8 *pEntry,
                                UINT32 &count);
    void        GetId3v2_2Entry(UINT8 *pBuf,
                                UINT8 *pEntry,
                                UINT32 &count);
    void        GetId3v2_XEntry(UINT8* pBuf, UINT8* pEntry, UINT32& count,
                                INT32 a, INT32 b, INT32 c);

    UINT8*      GetId3Title(int &nLen);
    UINT8*      GetId3Artist(int &nLen);
    UINT8*      GetId3Album(int &nLen);
    UINT8*      GetId3Genre(int &nLen);
    UINT8*      GetId3String(UINT8* pStr, int& nLen);
    void        PrepareString(UINT8*& rpStr);
    int         GetMetaOffset() {return m_nMetaOffset;}
    int         GetMetaRepeat() {return m_nMetaRepeat;}
    
    void        SetGenreInfo(INT32 ID3v1genre,
                             char *pGenre);

    eHeaderType GetHeaderType();
protected:

    // Offsets to m_aId3Values
    enum
    {
        eTitleLen   = 30,
        eArtistLen  = 30,
        eAlbumLen   = 30,
        eYearLen    = 4,
        eCommentLen = 30,
        eGenreLen   = 1
    };

    CAudioInfoBase* m_pFmt;

    UINT8       *m_pTitle,
                *m_pArtist,
                *m_pAlbum,
                *m_pYear,
                *m_pGenre;

    INT32       m_nMetaRepeat,
                m_nMetaOffset;
    
    eHeaderType m_eHeaderType;  // Type of mp3 header in stream

    UINT32 GetTagLength(UINT8* pBuf, UINT32 ulLen);
};

#define MP3_TAG_ID3  0x00494433  // 'I' 'D' '3'
#define MP3_TAG_TAG  0x00544147  // 'T' 'A' 'G'
#define MP3_TAG_ICY  0x00494359  // 'I' 'C' 'Y'
#define MP3_TAG_jpg  0x006A7067  // 'j' 'p' 'g'
#define MP3_TAG_bmp  0x00626D70  // 'b' 'm' 'p'
#define MP3_TAG_TT2  0x54543200  // 'T' 'T' '2'
#define MP3_TAG_TP1  0x54503100  // 'T' 'P' '1'
#define MP3_TAG_TAL  0x54414C00  // 'T' 'A' 'L'
#define MP3_TAG_TYE  0x54594500  // 'T' 'Y' 'E'
#define MP3_TAG_TCO  0x54434F00  // 'T' 'C' 'O'
#define MP3_TAG_TIT2 0x54495432  // 'T' 'I' 'T' '2'
#define MP3_TAG_TPE1 0x54504531  // 'T' 'P' 'E' '1'
#define MP3_TAG_TALB 0x54414C42  // 'T' 'A' 'L' 'B'
#define MP3_TAG_TYER 0x54594552  // 'T' 'Y' 'E' 'R'
#define MP3_TAG_TCON 0x54434F4E  // 'T' 'C' 'O' 'N'
#define MP3_TAG_TDRC 0x54445243  // 'T' 'D' 'R' 'C'

#endif
