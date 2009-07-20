/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxmapcommon_inl.h,v 1.11 2005/03/14 19:33:47 bobclark Exp $
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

#ifndef MAP_TYPE
#error "Should only include this from specific CHXMap implementation!"
#endif

#ifndef MAP_ITERATOR_KEY_TYPE
#define MAP_ITERATOR_KEY_TYPE key_type
#endif

#ifndef MAP_ITERATOR_KEY_GET
#define MAP_ITERATOR_KEY_GET(k) k
#endif

#include "debug.h"
#include "hxassert.h"

#undef HX_ASSERT
#define HX_ASSERT(x)

// Implementation notes...
//
// * Data are stored in the m_items vector.
// * The hash buckets/slots are stored in m_buckets.
// * Each bucket is a vector of integer indices into the m_items vector.
// * Rather than copying data around in m_items when an item is removed,
//   the item is just marked as "free" and its index is added to the
//   m_free vector.
// * The returned POSITION values are 1-based indices into m_items.
//   XXXSAB: This might be easier if it just passed out ptrs into the
//           m_items data, but this method could survive a reallocation of
//           the m_items vector (e.g. is something were added during
//           iteration, but I don't think that's really supported).
//

#if defined(HELIX_CONFIG_LOW_HEAP_HASH_TABLE)
#define HASH_TABLE_NUM_BUCKETS 1
#define HASH_TABLE_BUCKET_SIZE 1
#elif defined(HELIX_CONFIG_NOSTATICS)
//XXXgfw revisit these numbers when we do symbian size reduction.
//XXXgfw trade off between speed of lookups and size/fragmentation.
#define HASH_TABLE_NUM_BUCKETS 10
#define HASH_TABLE_BUCKET_SIZE 1
#else
#define HASH_TABLE_NUM_BUCKETS 101
#define HASH_TABLE_BUCKET_SIZE 5
#endif
#if defined(HELIX_CONFIG_NOSTATICS)
const ULONG32 MAP_TYPE::z_defaultNumBuckets = HASH_TABLE_NUM_BUCKETS;
const ULONG32 MAP_TYPE::z_defaultChunkSize = 0;
const ULONG32 MAP_TYPE::z_defaultBucketChunkSize = HASH_TABLE_BUCKET_SIZE;
#else
ULONG32 MAP_TYPE::z_defaultNumBuckets = HASH_TABLE_NUM_BUCKETS;
ULONG32 MAP_TYPE::z_defaultChunkSize = 0;
ULONG32 MAP_TYPE::z_defaultBucketChunkSize = HASH_TABLE_BUCKET_SIZE;
#endif


MAP_TYPE::Iterator::Iterator (ItemVec_t* pItems, int item) :
    m_pItems(pItems), m_item(item),
    m_key(MAP_ITERATOR_KEY_GET(key_nil())), m_val(val_nil())
{
    if (item < 0) m_item = pItems ? pItems->size() : 0;
	
	if (m_pItems)
	{
		GotoValid();

		if (m_item < m_pItems->size())
		{
			m_key = MAP_ITERATOR_KEY_GET((*m_pItems)[m_item].key);
			m_val = (*m_pItems)[m_item].val;
		}
	}
}

MAP_TYPE::Iterator& MAP_TYPE::Iterator::operator++()
{
    HX_ASSERT (m_pItems);
    HX_ASSERT (m_item >= 0);

    int s = m_pItems->size();
    if (m_item < s)
    {
        ++m_item;
        GotoValid();

        if (m_item < s)
        {
            m_key = MAP_ITERATOR_KEY_GET((*m_pItems)[m_item].key);
            m_val = (*m_pItems)[m_item].val;
        }
        else
        {
            m_key = MAP_ITERATOR_KEY_GET(key_nil());
            m_val = val_nil();
        }
    }
    return *this;
}

MAP_TYPE::Iterator MAP_TYPE::Iterator::operator++(int)
{
    HX_ASSERT (m_pItems);
    HX_ASSERT (m_item >= 0);

    Iterator ret (*this);

    int s = m_pItems->size();
    if (m_item < s)
    {
        ++m_item;
        GotoValid();

        if (m_item < s)
        {
            m_key = MAP_ITERATOR_KEY_GET((*m_pItems)[m_item].key);
            m_val = (*m_pItems)[m_item].val;
        }
        else
        {
            m_key = MAP_ITERATOR_KEY_GET(key_nil());
            m_val = val_nil();
        }
    }

    return ret;
}

HXBOOL MAP_TYPE::Iterator::operator==(const Iterator& other) const
{
    return (m_pItems == other.m_pItems && m_item == other.m_item);
}

HXBOOL MAP_TYPE::Iterator::operator!=(const Iterator& other) const
{
    return (m_pItems != other.m_pItems || m_item != other.m_item);
}

MAP_TYPE::value_type MAP_TYPE::Iterator::operator*()
{
    return m_val;
}

MAP_TYPE::Iterator::iter_key_type MAP_TYPE::Iterator::get_key()
{
    return m_key;
}

void MAP_TYPE::Iterator::GotoValid()
{
    // Skip to the next item that is in use (i.e. not free)
    while (m_item < m_pItems->size() && (*m_pItems)[m_item].bFree) ++m_item;
}

POSITION MAP_TYPE::Item2Pos(int item) const
{
    return (item >= 0 && item < m_items.size()) ? (POSITION)(item+1) : 0;
}

int MAP_TYPE::Pos2Item(POSITION pos) const
{
    return pos ? ((int)(PTR_INT)pos - 1) : m_items.size();
}

MAP_TYPE::MAP_TYPE(int chunkSize) :
    m_hf(NULL),
    m_numBuckets(z_defaultNumBuckets),
    m_chunkSize(chunkSize),
    m_bucketChunkSize(z_defaultBucketChunkSize)
{
    m_items.SetChunkSize(chunkSize);
    ConstructTypeSpecifics();
}

MAP_TYPE::~MAP_TYPE()
{
}

HXBOOL MAP_TYPE::LookupInBucket(
    ULONG32 bucket, key_arg_type key, int& retItem) const
{
    const HlxMap::IntVec_t& rBucket = m_buckets[bucket];
    int len = rBucket.size();

    const int* pCur = &rBucket[0];
    for (int i = 0; i < len; ++i, ++pCur)
    {
        if (IsKeyMatch(m_items[*pCur].key, key))
        {
            retItem = *pCur;
            return TRUE;
        }
    }
    return FALSE;
}

HXBOOL MAP_TYPE::Lookup(key_arg_type key, int& retItem) const
{
    if (m_buckets.empty()) return FALSE;

    return LookupInBucket (HashKey(key) % m_buckets.size(), key, retItem);
}

MAP_TYPE::Item* MAP_TYPE::LookupItem(
    ULONG32 bucket, key_arg_type key)
{
    if (!m_buckets.empty())
    {
        const HlxMap::IntVec_t& rBucket = m_buckets[bucket];
        int len = rBucket.size();

        const int* pCur = &rBucket[0];
        for (int i = 0; i < len; ++i, ++pCur)
        {
            Item& item = m_items[*pCur];
            if (IsKeyMatch(item.key, key))
            {
                return &item;
            }
        }
    }

    return NULL;
}

HXBOOL MAP_TYPE::Lookup(key_arg_type key, value_ref_type value) const
{
    if (m_buckets.empty()) return FALSE;

    const Item* pItem = LookupItem(HashKey(key) % m_buckets.size(), key);
    if (!pItem) return FALSE;

    value = pItem->val;
    return TRUE;
}

POSITION MAP_TYPE::Lookup(key_arg_type key) const
{
    POSITION ret = 0;
    int item;
    if (Lookup(key, item)) ret = Item2Pos(item);

    return ret;
}

HXBOOL MAP_TYPE::AddToBucket(
    ULONG32 bucket, key_arg_type key, value_arg_type value, int& retItem)
{
    int idx = m_items.size();
    if (m_free.empty())
    {
        m_items.push_back(Item(key, value, false));
    }            
    else
    {
        idx = m_free.back();
        m_free.pop_back();

        Item& rItem = m_items[idx];
        rItem.key = key;
        rItem.val = value;
        rItem.bFree = false;
    }

    m_buckets[bucket].push_back(idx);

    retItem = idx;
    return TRUE;
}

MAP_TYPE::key_ref_type MAP_TYPE::GetKeyAt(POSITION pos) const
{
    int item = Pos2Item(pos);
    HX_ASSERT (item >= 0 && item < m_items.size());

    if (item > 0 && item < m_items.size()) return m_items[item].key;
    return key_nil();
}

MAP_TYPE::value_const_ref_type MAP_TYPE::GetAt(POSITION pos) const
{
    int item = Pos2Item(pos);
    HX_ASSERT (item >= 0 && item < m_items.size());

    if (item > 0 && item < m_items.size()) return m_items[item].val;
    return val_nil();
}

MAP_TYPE::value_ref_type MAP_TYPE::GetAt(POSITION pos)
{
    int item = Pos2Item(pos);
    HX_ASSERT (item >= 0 && item < m_items.size());

    if (item > 0 && item < m_items.size()) return m_items[item].val;
    return val_nil();
}

MAP_TYPE::value_ref_type MAP_TYPE::operator[](key_arg_type key)
{
    if (m_buckets.empty())
    {
        if( HXR_OUTOFMEMORY == InitHashTable (m_numBuckets) )
        {
            // Don't know if caller knows to check for val_nil.
            return val_nil();
        }
    }

    ULONG32 bucket = HashKey(key) % m_buckets.size();
    Item* pItem = LookupItem(bucket, key);
    if (!pItem)
    {
        int itemIdx;
        if (AddToBucket (bucket, key, val_nil(), itemIdx))
        {
            return m_items[itemIdx].val;
	}
        else
        {
            // Now what?  Adding it failed...
            return val_nil();
        }
    }

    return pItem->val;
}
POSITION MAP_TYPE::SetAt(key_arg_type key, value_arg_type value)
{
    if (m_buckets.empty())
    {
        if( HXR_OUTOFMEMORY == InitHashTable (m_numBuckets) )
        {
            // Don't know if caller knows to check for 0.
            return 0;
        }
    }

    ULONG32 bucket = HashKey(key) % m_buckets.size();
    int item;
    if (LookupInBucket(bucket, key, item)) m_items[item].val = value;
    else AddToBucket(bucket, key, value, item);
    return Item2Pos(item);
}

POSITION MAP_TYPE::Remove(key_arg_type key)
{
    if (m_buckets.empty()) return 0;

    int item = -1;

    ULONG32 bucket = HashKey(key) % m_buckets.size();
    HlxMap::IntVec_t& rBucket = m_buckets[bucket];
    int len = rBucket.size();

    int* pCur = &rBucket[0];
    for (int i = 0; i < len; ++i, ++pCur)
    {
        if (IsKeyMatch(m_items[*pCur].key, key))
        {
            item = *pCur;
            rBucket.zap(i);
            m_free.push_back(item);
            m_items[item].bFree = true;
        }
    }

    // key not found
    if (item < 0) return 0;

    // Go to next valid item
    int s = m_items.size();
    ++item;
    while (item < s && m_items[item].bFree) ++item;

    if (item >= s) return 0;
    return Item2Pos(item);
}

HXBOOL MAP_TYPE::RemoveKey(key_arg_type key)
{
    int countBefore = GetCount();
    Remove(key);
    return GetCount() < countBefore;
}

MAP_TYPE::Iterator MAP_TYPE::Erase(Iterator it)
{
    if (it.m_pItems && it.m_item >= 0 && it.m_item < it.m_pItems->size())
    {
        // POSITION is a 1-based index to m_items.
        POSITION pos = Remove ((*it.m_pItems)[it.m_item].key);
        if (pos) return Iterator (&m_items, Pos2Item(pos));
    }
    return End();
}

MAP_TYPE::Iterator MAP_TYPE::Find(key_arg_type key)
{
    int item;
    if (Lookup(key, item)) return Iterator(&m_items, item);
    return End();
}

void MAP_TYPE::RemoveAll()
{
    m_free.resize(0);
    m_items.resize(0);

    int len = m_buckets.size();
    for (int i = 0; i < len; ++i) m_buckets[i].resize(0);

    return;
}

POSITION MAP_TYPE::GetStartPosition() const
{
    if (GetCount() <= 0) return 0;

    int s = m_items.size();
    int item = 0;
    const Item* pItem = &m_items[item];
    while (item < s && pItem->bFree) ++item, ++pItem;

    // Since GetCount() is > 0, we HAVE to find something here...
    HX_ASSERT (item < s);

    return Item2Pos(item);
}

#ifndef MAP_OVERRIDE_GETNEXTASSOC
void MAP_TYPE::GetNextAssoc (POSITION& pos, key_type& key, value_type& value) const
{
    int item = Pos2Item(pos);

    HX_ASSERT (item >= 0 && item < m_items.size());

    const Item* pItem = &m_items[item];
    key = pItem->key;
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
#endif /* MAP_OVERRIDE_GETNEXTASSOC */

MAP_TYPE::Iterator MAP_TYPE::Begin()
{
    return Iterator (&m_items, 0);
}

MAP_TYPE::Iterator MAP_TYPE::End()
{
    return Iterator(&m_items, m_items.size());
}

HX_RESULT MAP_TYPE::InitHashTable(ULONG32 numBuckets, HXBOOL bAlloc)
{
    RemoveAll();

    m_numBuckets = numBuckets;
    HX_RESULT retVal = HXR_OK;
    if (bAlloc)
    {
        retVal = m_buckets.Init((UINT16)numBuckets);
        if( retVal != HXR_OUTOFMEMORY )
        {
            for (ULONG32 i = 0; i < numBuckets; ++i)
                m_buckets[i].SetChunkSize(m_bucketChunkSize);
        }
    }
    return retVal;
}
