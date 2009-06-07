/* 
 * $Id: cloaked_guid_dict.cpp,v 1.2 2005/05/10 23:44:52 atin Exp $
 */
#include <string.h>
#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "mutex.h"
#include "hxresult.h"
#include "hxassert.h"
#include "cloaked_guid_dict.h"
#include "http_demux.h"
#include "cloak_common.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


UINT32
cgde_strhash(const char* key)
{
    // Chris Torek's hash function
    UINT32 h = 0x31fe;

    while (*key)
	h = h*33 + *key++;
    return h;
}

CloakedGUIDDict::CloakedGUIDDict(UINT32 nbuckets):
    _count(0), _nbuckets(nbuckets),
    _compare(strcasecmp), _hash(cgde_strhash)
{
    init();
}

CloakedGUIDDict::CloakedGUIDDict(
    int (*compare)(const char*, const char*)
    , UINT32 (*hash)(const char*), UINT32 nbuckets)
    : _count(0) 
    , _nbuckets(nbuckets)
    , _compare(compare)
    , _hash(cgde_strhash)
{
    init();
}

CloakedGUIDDict::~CloakedGUIDDict()
{
    HXMutexLock(_mutex);
    for (UINT32 i = 0; i < _nbuckets; i++)
    {
	CloakedGUIDDictEntry* nexte;
	for (CloakedGUIDDictEntry* e = _table[i]; e != 0; e = nexte)
	{
	    nexte = e->_next;
	    e->Release();
	}
    }
    delete[] _table;
    HXMutexUnlock(_mutex);

    HXDestroyMutex(_mutex);
    _mutex = NULL;
}

void
CloakedGUIDDict::init()
{
    _mutex = HXCreateMutex();
    _table = new CloakedGUIDDictEntry*[_nbuckets];
    memset(_table, 0, _nbuckets * sizeof(CloakedGUIDDictEntry *));
}

UINT32
CloakedGUIDDict::enter(const char* key, CloakConn* obj)
{
    HXMutexLock(_mutex);
    UINT32 h = _hash(key);
    CloakedGUIDDictEntry* e;
    CloakedGUIDDictEntry* nexte;

    _count++;

    // Grow the table if 66% full
    UINT32 nb = _count*3;
    if (nb > _nbuckets*2)
    {
	CloakedGUIDDictEntry** tab = new CloakedGUIDDictEntry*[nb];
	UINT32 i;

	memset(tab, 0, nb * sizeof(CloakedGUIDDictEntry *));
	for (i = 0; i < _nbuckets; i++)
	{
	    for (e = _table[i]; e != 0; e = nexte)
	    {
		nexte = e->_next;
		e->_next = tab[e->_hash%nb];
		tab[e->_hash%nb] = e;
	    }
	}
	delete[] _table;
	_table = tab;
	_nbuckets = nb;
    }
    int idx = h % _nbuckets;
    CloakedGUIDDictEntry* cgde = new CloakedGUIDDictEntry(key, h, obj, _table[idx]);
    cgde->AddRef();
    _table[idx] = cgde;
    HXMutexUnlock(_mutex);

    return idx;
}

void
CloakedGUIDDict::remove(const char* key, CloakConn*& obj)
{
    HX_ASSERT(key);

    HXMutexLock(_mutex);
    CloakedGUIDDictEntry* e, **ep;
    UINT32 h = _hash(key);
    for (ep = &_table[h%_nbuckets]; (e = *ep) != 0; ep = &e->_next)
    {
	if (_compare(key, e->_key) != 0)
	    continue;
	*ep = e->_next;
	obj = e->_obj;
	obj->AddRef();
	e->Release();
	--_count;
	HXMutexUnlock(_mutex);
	return;
    }
    HXMutexUnlock(_mutex);
    obj = 0;
    return;
}

void
CloakedGUIDDict::find(const char* key, CloakConn*& obj)
{
    HXMutexLock(_mutex);
    UINT32 h = _hash(key);
    for (CloakedGUIDDictEntry* e = _table[h%_nbuckets]; e != 0; e = e->_next)
    {
	if (_compare(key, e->_key) == 0)
	{
	    obj = e->_obj;
	    obj->AddRef();
	    HXMutexUnlock(_mutex);
	    return;
	}
    }
    HXMutexUnlock(_mutex);
    obj = 0;
    return;
}

CloakedGUIDDictEntry::CloakedGUIDDictEntry(const char* k, UINT32 h, CloakConn* obj, CloakedGUIDDictEntry* n)
    : _key(0)
    , _key_len(0)
    , _hash(h)
    , _obj(obj)
    , _next(n)
    , _ref_count(0)
{
    _key_len = strlen(k)+1;
    _key = new char[_key_len]; 
    strcpy((char *)_key, k);
    _obj->AddRef();
    // fprintf(stderr, "cgde::cgde(%p) -- mutex(%p) created\n", this, m_pMutex);
}

CloakedGUIDDictEntry::~CloakedGUIDDictEntry()
{
    HX_DELETE(_key);
    HX_RELEASE(_obj);
    _key_len = 0;
    // fprintf(stderr, "cgde::~cgde(%p)\n", this);
}

INT32
CloakedGUIDDictEntry::AddRef()
{
    return InterlockedIncrement(&_ref_count);
}

INT32
CloakedGUIDDictEntry::Release()
{
    if (InterlockedDecrement(&_ref_count) > 0)
    {
	return _ref_count;
    }
    delete this;
    return 0;
}
