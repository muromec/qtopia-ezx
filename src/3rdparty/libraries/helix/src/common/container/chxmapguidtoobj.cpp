/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxmapguidtoobj.cpp,v 1.6 2007/07/06 20:34:58 jfinnecy Exp $
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

#include "chxmapguidtoobj.h"

#define MAP_TYPE CHXMapGUIDToObj

#define MAP_ITERATOR_KEY_TYPE const key_type*
#define MAP_ITERATOR_KEY_GET(k) &k
#define MAP_OVERRIDE_GETNEXTASSOC
#include "chxmapcommon_inl.h"

DECLARE_ITEMVEC_IMP(CHXMapGUIDToObj,ItemVec_t,Item,Item(),0,GUID_NULL);

#if defined(_DEBUG) || defined(_DEBUG_HASH)
#include "hxguid.h"
#endif

void CHXMapGUIDToObj::GetNextAssoc (POSITION& pos, key_type*& key, value_type& value) const
{
    int item = Pos2Item(pos);

    HX_ASSERT (item >= 0 && item < m_items.size());

    const Item* pItem = &m_items[item];
    key = (key_type*)(&pItem->key);
    value = pItem->val;

    // go to next valid item
    int s = m_items.size();
    ++item; ++pItem;
    while (item < s && pItem->bFree) ++item, ++pItem;

    if (item >= s)
    {
        // Hit the end...
        pos = 0;
    }
    else
    {
        // Convert back to 1-based POSITION
        pos = Item2Pos(item);
    }
}

// ==================== Code specific to the type of key and/or value goes below here.

void CHXMapGUIDToObj::ConstructTypeSpecifics()
{
}

ULONG32 CHXMapGUIDToObj::DefaultHashFunc (key_arg_type key)
{
#ifdef _DEBUG_HASH
    CHXGUID guid(key);
    CHXString strGUID;
    guid.Get(strGUID);
#endif /* _DEBUG_HASH */

    // XXXSAB: Need something to reasonably distribute GUIDs...anyone have
    //         an idea?
    ULONG32 ret = 0;

    // XOR together sizeof(ULONG32) chunks of the key...
    static const int wordSize = sizeof(ULONG32);

    const char* keySrc = (const char*)&key;
    int byteLen = sizeof(GUID);
    int wordLen = byteLen / wordSize;
        
    ULONG32 intWord;
    char* pCharWord = (char*)&intWord;

    for (int i = 0; i < wordLen; ++i)
    {
        strncpy (pCharWord, keySrc, wordSize); /* Flawfinder: ignore */
        keySrc += wordSize; byteLen -= wordSize;

#ifdef _DEBUG_HASH
        printf ("CHXMapGUIDToObj::DefaultHashFunc(\"%s\"): %lx ^ %lx\n",
                (const char*)strGUID, ret, intWord);
#endif /* _DEBUG_HASH */
        ret ^= intWord;
    }

    if (byteLen > 0)
    {
        HX_ASSERT (byteLen < wordSize);
        intWord = 0;
        strncpy (pCharWord, keySrc, byteLen); /* Flawfinder: ignore */

#ifdef _DEBUG_HASH
        printf ("CHXMapGUIDToObj::DefaultHashFunc(\"%s\"): %lx ^ %lx\n",
                (const char*)strGUID, ret, intWord);
#endif /* _DEBUG_HASH */
        ret ^= intWord;
    }

#ifdef _DEBUG_HASH
    printf ("CHXMapGUIDToObj::DefaultHashFunc(\"%s\") -> %d (0x%lx)\n",
            (const char*)strGUID, ret);
#endif /* _DEBUG_HASH */

    return ret;
}

void CHXMapGUIDToObj::Dump() const
{
#ifdef _DEBUG
    int i;
    CHXGUID guid;
    CHXString strGUID;

    printf("(CHXMapGUIDToObj*)%p:\n", this);
    printf("   items[sz=%d/%d]", m_items.size(), m_items.capacity());
    for (i = 0; i < m_items.size(); ++i)
    {
        guid.Set(m_items[i].key);
        guid.Get(strGUID);
        printf("%s{%s,%p%s}",
               (i % 2) ? " " : "\n      ",
               (const char*)strGUID, m_items[i].val,
               m_items[i].bFree ? ",FREE" : "");
    }
    printf("\n   free[sz=%d/%d]", m_free.size(), m_free.capacity());
    for (i = 0; i < m_free.size(); ++i)
    {
        printf("%s%d",
               (i % 10) ? " " : "\n      ",
               m_free[i]);
    }
    printf("\n   buckets[sz=%d]", m_buckets.size());
    for (i = 0; i < m_buckets.size(); ++i)
    {
        if (! m_buckets[i].empty())
        {
            printf("\n  %6d[sz=%d/%d]:",
                   i, m_buckets[i].size(), m_buckets[i].capacity());
            for (int j = 0; j < m_buckets[i].size(); ++j)
                printf(" %d", m_buckets[i][j]);
        }
    }
    printf("\n   chunkSize=%ld; bucketChunkSize=%ld; hf=%p;",
           (long)m_chunkSize, (long)m_bucketChunkSize, m_hf);
    printf("\n   defChunkSize=%ld; defBucketChunkSize=%ld; defNumBuckets=%ld;\n",
           (long)z_defaultChunkSize, (long)z_defaultBucketChunkSize,
           (long)z_defaultNumBuckets);
#endif /* _DEBUG */
}
