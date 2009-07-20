/* 
 * $Id: cloaked_guid_dict.h,v 1.3 2005/05/12 17:53:51 atin Exp $
 */
#ifndef	_CLOAKED_GUID_DICT_H_
#define	_CLOAKED_GUID_DICT_H_

UINT32 cgde_strhash(const char *key);

class CloakedGUIDDict;
class CloakedHTTPProtocol;
class CloakConn;

class CloakedGUIDDictEntry
{
public:
    CloakedGUIDDictEntry(const char* key, UINT32 hash, CloakConn* obj, CloakedGUIDDictEntry* next);

    INT32 AddRef();
    INT32 Release();

    void* GetObj() { return _obj; }

protected:
    ~CloakedGUIDDictEntry();

private:
    friend class		CloakedGUIDDict;

    const char *		_key;
    UINT32			_key_len;
    UINT32			_hash;
    CloakConn*			_obj;
    CloakedGUIDDictEntry*	_next;
    UINT32			_ref_count;
};

class CloakedGUIDDict
{
public:
			CloakedGUIDDict(UINT32 nbuckets=512);
			CloakedGUIDDict(int(*comp)(const char *, const char *),
			     UINT32(*hash)(const char*) = cgde_strhash,
			     UINT32 nbuckets=512);
			~CloakedGUIDDict();

    UINT32		enter(const char * key, CloakConn* obj);
    void		remove(const char * key, CloakConn*& obj);
    void		find(const char * key, CloakConn*& obj);
    int			size();

private:
    UINT32		_count;
    UINT32		_nbuckets;
    CloakedGUIDDictEntry**	_table;
    HX_MUTEX 		_mutex;

    int			(*_compare)(const char *, const char *);
    UINT32		(*_hash)(const char *);
    void		init();
};

inline int
CloakedGUIDDict::size()
{
    return _count;
}

#endif /*_CLOAKED_GUID_DICT_H_*/
