/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cache.h,v 1.5 2006/02/07 20:49:12 ping Exp $
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

#include "db.h"

#define DEFAULT_MAX_CACHE_SIZE              (4  * 1024 * 1024)
#define MININUM_MAX_CACHE_SIZE              (256 * 1024)
#define SECS_IN_YEAR                        (366 * 24 * 60 * 60)

class  CHXString;
class  CChunkyRes;
class  CHXSimpleList;

struct IHXBuffer;

#ifdef _WINDOWS
typedef HXBOOL (PASCAL FAR	*GETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
#endif /* _WINDOWS */

/****************************************************************************
 * 
 *  Class: CCacheEntry
 * 
 *  Purpose:
 * 
 *    This is a helper class to allow a header to be easily placed before the
 *    data stored in a URL's cache entry.  It allows the expiry time and other
 *    out-of-band information to be stored with the data. 
 */

typedef struct _CacheHeader
{
    UINT32      m_ulCreateTime;         // Seconds since 1970-01-01T00:00:00Z
    UINT32      m_ulExpiryTime;
    UINT32      m_ulCategory;           // NEWS, COOL, SPORTS, etc.
    UINT32      m_ulFlags;              // EVERGREEN, MIRRORED_SERVERS
#define CACHE_EVERGREEN         1
#define CACHE_MIRRORED_SERVERS  2
    UINT32      m_ulHttpHeaderSize;     // Number of bytes in HTTP Header
    CHAR        m_szMimeType[64];
} CacheHeader, *PCacheHeader;

typedef struct _CacheUsage
{
    UINT32      m_ulCreateTime;         // Seconds since 1970-01-01T00:00:00Z
    UINT32      m_ulExpiryTime;
    UINT32      m_ulLastUsedTime;
    UINT32      m_ulUseCount;		// Number of 'gets'
    UINT32      m_ulFlags;              // EVERGREEN, MIRRORED_SERVERS
    UINT32      m_ulSize ;              // Size of content
} CacheUsage, *PCacheUsage;

class CCacheEntry
{
public:
    CCacheEntry(IUnknown* pContext);
    CCacheEntry(IUnknown* pContext, const char* pszDbFile, UINT32 ulMaxCacheSize, const char* pszUrl);

#ifdef DELETE_THIS
    CCacheEntry(UINT32 ulExpiry, const char* pszMimeType, IHXBuffer *pHeader, IHXBuffer *pContent);
#endif // DELETE_THIS
     
    ~CCacheEntry(); 

public:
    const char*   GetDbBuffer   (void);    // Returns a pointer to a data buffer to feed DBM
    const char*   GetDataBuffer (void);    // Returns pointer to the streamed HTTP data
    UINT32        GetDataSize   (void);    // Returns the size of the HTTP data
    HXBOOL        IsExpired     (void);    // True if cache entry has expired
    UINT32        CreateTime    (void);    // Time entry created, in seconds since 1970-01-01
    UINT32        ExpiryTime    (void);    // Time entry expires, in seconds since 1970-01-01
    HXBOOL          Evergreen     (void);    // True if the content should not be purged
    UINT32        GetCategory   (void);    // NEWS, COOL, SPORTS, etc.

    HX_RESULT     CleanCache    (UINT32       ulCutOffTime, UINT32 ulSlackSeconds = (5 * 60));

#ifdef DELETE_THIS
    UINT32        CreateEntry   (UINT32       ulExpiry,
                                 const char*  pszMimeType,
                                 IHXBuffer*  pBuffer);
#endif // DELETE

    HX_RESULT     WriteCache    (UINT32       nContentRead,
                                 UINT32       ulExpiryTime,
                                 char*        pFilename,
                                 const char*  pszMimeType,
                                 IHXBuffer*  pHttpHeader,
                                 CChunkyRes*  pChunkyRes,
                                 HXBOOL         bMirroredServers);

    HX_RESULT     ReadCache     (DBT* key, DBT* dbtHeader, DBT* dbtContent, HXBOOL bMirroredServers = FALSE);

    HX_RESULT     get           (DBT *pdbtKey, DBT *pdbtHeader, DBT *pdbtContent, DB_TXN* pTxnid, UINT32 ulFlags = 0);
    HX_RESULT     put           (DB* pDb, DBT *pdbtKey, DBT *pdbtHeader, DBT *pdbtContent, DB_TXN* pTxnid, UINT32 ulFlags = 0);
    UINT32        del           (DBT* key, DB_TXN* pTxnid = NULL, UINT32 ulFlags = 0);
    UINT32        close         (UINT32 ulFlags = 0);
    UINT32        sync          (UINT32 ulFlags = 0);

private:
    char*         m_pszDbDir;               // The name of the database directory
    HXBOOL        m_bCached;                // TRUE if item is cached
    DB*           m_pDbHeader;              // Pointer to the database handle
    DB*           m_pDbData;                // Pointer to the database handle
    DB*           m_pDbUsage;               // Pointer to the database handle
    UINT32        m_ulMaxCacheSize;         // Maximum size of the cache
    PCacheHeader  m_pCacheHeader;	    // Pointer to cache header structure
    UINT8*        m_pCacheData;		    // Pointer to cached HTTP content
    IUnknown*	  m_pContext;
}; 
