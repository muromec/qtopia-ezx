/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cache.cpp,v 1.11 2006/02/07 20:49:12 ping Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/windows.h"
#include "hlxosstr.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "ihxpckts.h"
#include "chunkres.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "db.h"
#include "cache.h"

#define  LOG_FILE       "C:/Temp/cache.log"
#include "http_debug.h"

DB*  g_pCacheDbHeader  = NULL;
DB*  g_pCacheDbData    = NULL;
DB*  g_pCacheDbUsage   = NULL;

static char*    GetAbbrevUrl      (char* pData, UINT32 ulSize);
UINT32          GetFreeMbyteCount (const char *pDirectoryName);

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

CCacheEntry::CCacheEntry(IUnknown* pContext)
{
    // This creates a cache object for an existing cache entry
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    LOG ("CCacheEntry::CCacheEntry()    ******* Not expected to be used *******");
}

CCacheEntry::~CCacheEntry()
{   
    if (m_pCacheHeader)               // Free up the dbm data buffer
    {
        delete [] m_pCacheHeader;
        m_pCacheHeader = NULL;
    }
    if (m_pszDbDir)                    // Free the filename storage
        delete [] m_pszDbDir;
    HX_RELEASE(m_pContext);
}

const char* 
CCacheEntry::GetDbBuffer(void)
{
    return((const char *) m_pCacheHeader);
}

const char*
CCacheEntry::GetDataBuffer(void)
{
    const char *pData = (const char *) m_pCacheHeader;

    return(pData + sizeof (CacheHeader));
}

HXBOOL
CCacheEntry::IsExpired(void)
{
    if (m_pCacheHeader == NULL)
        return(TRUE);                   // Be mean if we don't know
        
    return((m_pCacheHeader->m_ulExpiryTime < (UINT32) time(NULL)) ? TRUE : FALSE);
}

UINT32
CCacheEntry::CreateTime(void)
{
    if (m_pCacheHeader == NULL)
        return(0);
        
    return(m_pCacheHeader->m_ulCreateTime);
}

UINT32
CCacheEntry::ExpiryTime(void)
{
    if (m_pCacheHeader == NULL)
        return(0);
        
    return(m_pCacheHeader->m_ulExpiryTime);
}

HXBOOL
CCacheEntry::Evergreen(void)
{
    if (m_pCacheHeader == NULL)
        return(FALSE);
        
    return((m_pCacheHeader->m_ulFlags & CACHE_EVERGREEN) ? TRUE : FALSE);
}

UINT32
CCacheEntry::GetCategory(void)
{
    if (m_pCacheHeader == NULL)
        return(0);
        
    return(m_pCacheHeader->m_ulCategory);
}


        
CCacheEntry::CCacheEntry(IUnknown* pContext, const char* pszDbDir, UINT32 ulMaxCacheSize, const char* pszUrl)
    : m_pszDbDir         (NULL)
    , m_pCacheHeader     (NULL)
{
//  HX_ASSERT (g_pCacheDB == NULL);

    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    INT32     rc          = 0;
    INT32     nMode       = 0;            // A zero value defines the default setting
    char*     pszFilename = NULL;         // Will reference the full filename for each DB
    UINT32    ulFlags     = DB_CREATE;    // DB_CREATE DB_NOMAP DB_THREAD
    DBTYPE    dbtType     = DB_HASH;      // DB_BTREE DB_HASH DB_RECNO DB_UNKNOWN
    DB_INFO   diDbInfo    = { 0 };

    m_ulMaxCacheSize = ulMaxCacheSize;

    pszFilename = new char [strlen(pszDbDir) + 24];

    if (pszFilename)
	{
	    strcpy (pszFilename, pszDbDir); /* Flawfinder: ignore */

	    LOGX ((szDbgTemp, "    [DB] Opening database '%s'", pszDbDir));
	    diDbInfo.ulSize = ulMaxCacheSize;
	    
	    strcpy (pszFilename + strlen(pszDbDir), OS_SEPARATOR_STRING "c_header" OS_SEPARATOR_STRING "db.dat"); /* Flawfinder: ignore */
	    m_pDbHeader = dbopen(pContext, pszFilename, ulFlags, nMode, dbtType, &diDbInfo);
	    if (!m_pDbHeader)
	    {
	        LOGX ((szDbgTemp, "    [DB] Error opening databases '%s'", pszFilename));
	        remove (pszFilename);     // remove the troublesome cache databases and try again
	        m_pDbHeader = dbopen(pContext, pszFilename, DB_CREATE, DB_RDWR, dbtType, &diDbInfo);
	    }
	    
	    strcpy (pszFilename + strlen(pszDbDir), OS_SEPARATOR_STRING "c_data" OS_SEPARATOR_STRING "db.dat"); /* Flawfinder: ignore */
	    m_pDbData = dbopen(pContext, pszFilename, ulFlags, nMode, dbtType, &diDbInfo);
	    if (!m_pDbData)
	    {
	        LOGX ((szDbgTemp, "    [DB] Error opening databases '%s'", pszFilename));
	        remove (pszFilename);     // remove the troublesome cache databases and try again
	        m_pDbData = dbopen(pContext, pszFilename, DB_CREATE, DB_RDWR, dbtType, &diDbInfo);
	    }
	    
	    strcpy (pszFilename + strlen(pszDbDir), OS_SEPARATOR_STRING "c_usage" OS_SEPARATOR_STRING "db.dat"); /* Flawfinder: ignore */
	    m_pDbUsage = dbopen(pContext, pszFilename, ulFlags, nMode, dbtType, &diDbInfo);
	    if (!m_pDbUsage)
	    {
	        LOGX ((szDbgTemp, "    [DB] Error opening databases '%s'", pszFilename));
	        remove (pszFilename);     // remove the troublesome cache databases and try again
	        m_pDbUsage = dbopen(pContext, pszFilename, DB_CREATE, DB_RDWR, dbtType, &diDbInfo);
	    }

	    if (rc || !m_pDbHeader || !m_pDbData || !m_pDbUsage)
	    {
	        LOGX ((szDbgTemp, "    [DB] Error creating new databases in  '%s' [%d]", pszDbDir, rc));
	    }
	    else
	    {
	        LOGX ((szDbgTemp, "    [DB] Created databases in '%s' [%08X]", pszDbDir, m_pDbHeader));
	        g_pCacheDbHeader = m_pDbHeader;
	        g_pCacheDbData   = m_pDbData;
	        g_pCacheDbUsage  = m_pDbUsage;
	        
	        m_pszDbDir = new char[strlen (pszDbDir) + 1];
	        strcpy (m_pszDbDir, pszDbDir); /* Flawfinder: ignore */

	        CleanCache(0);           // purge expired entries
	    }

		delete [] pszFilename;
	}
}

HX_RESULT
CCacheEntry::WriteCache(UINT32       nContentRead,
                        UINT32       ulExpiryTime,
                        char *       pszUrl,
                        const char * pszMimeType,
                        IHXBuffer*  pHttpHeaders,
                        CChunkyRes*  pChunkyRes,
                        HXBOOL         bMirroredServers)
{
    char*       pszAbbrevUrl    = NULL;
    UINT32      ulActual        = 0;
    UINT32      ulFlags         = 0;
    
    LOG ("WriteCache()");

    if (m_pDbHeader == NULL || m_pDbData == NULL)
    {
        return(HXR_FAIL);
    }

    // If the flag m_bMirroredServers is set, truncate the leftmost domain label
    if (bMirroredServers)
    {
        pszAbbrevUrl = GetAbbrevUrl (pszUrl, strlen(pszUrl));
        if (pszAbbrevUrl)
        {
            pszUrl = pszAbbrevUrl;
        }
        else
        {
            LOG ("    Failed to get abbreviated URL");
        }
    }

    // Save the header in the cache with the URL as the key
    UINT32  ulHeaderSize  = sizeof (CacheHeader) + pHttpHeaders->GetSize();
    char *  pHeader       = new char[ulHeaderSize];

    LOGX((szDbgTemp,">   Caching %lu + %lu bytes under the key '%s', Type is '%s'",
          ulHeaderSize, nContentRead, pszUrl, NULLOK(pszMimeType)));

    if (pHeader)
    {
        // Read directly into the buffer
        CacheHeader* pCacheHeader    = (CacheHeader*) pHeader;

        memset (pCacheHeader, 0, sizeof (CacheHeader));
        pCacheHeader->m_ulCreateTime        = time (NULL);
        pCacheHeader->m_ulExpiryTime        = ulExpiryTime;
        pCacheHeader->m_ulFlags             = 0;
        pCacheHeader->m_ulHttpHeaderSize    = pHttpHeaders->GetSize();

        strncpy(pCacheHeader->m_szMimeType, pszMimeType, sizeof pCacheHeader->m_szMimeType); /* Flawfinder: ignore */
        pCacheHeader->m_szMimeType[sizeof(pCacheHeader->m_szMimeType)-1] = '\0';

        // Add the HTTP Header data after the Cache Header
        memcpy (pCacheHeader + 1, pHttpHeaders->GetBuffer(), pHttpHeaders->GetSize()); /* Flawfinder: ignore */
    }
    DBT key  = { pszUrl, strlen(pszUrl) };
    DBT data = { pHeader, ulHeaderSize };

    int rc = m_pDbHeader->put(m_pDbHeader, &key, &data, ulFlags);

    if (rc == 0)
    {
        m_bCached = TRUE;
        LOG("    Cache header entry written");

        // Create usage info
        CacheUsage   usage          = { 0, 0, 0, 0, 0, 0 };
        DBT          dbtUsage       = { &usage, sizeof (CacheUsage) };
        UINT32       ulNow          = time (NULL);
        
        usage.m_ulCreateTime   = time (NULL);
        usage.m_ulLastUsedTime = time (NULL);
        usage.m_ulExpiryTime   = ulExpiryTime;
        usage.m_ulUseCount     = 1;
        usage.m_ulSize         = nContentRead;
        
        if ((rc = m_pDbUsage->put (m_pDbUsage, &key, &dbtUsage, ulFlags)) == DB_OK)
        {
            LOG ("    Put of usage info done");
            LOGX((szDbgTemp, "    %lu Create=%ld Last=%ld Expiry=%ld", usage.m_ulUseCount,
                                                    ulNow - usage.m_ulCreateTime,
                                                    ulNow - usage.m_ulLastUsedTime,
                                                    usage.m_ulExpiryTime - ulNow));
        }
        else
        {
            LOG ("*** Put of usage failed");
        }
    }
    else
    {
        if (rc == DB_KEYEXIST)
        {
            LOG("    Duplicate header entry, not written");
            m_bCached = TRUE;
        }
        else
        {
            LOGX((szDbgTemp, "*** Error putting header entry into cache [%lu]", rc));
        }
    }
/**********/

    /**** Now write the content ****/
    // Add the URL contents
    char* pContent = new char [nContentRead];
    if (pContent)
    {
        pChunkyRes->GetData(0, pContent, nContentRead, &ulActual);

        data.size = ulActual;
        data.data = pContent;

        int rc = m_pDbData->put(m_pDbData, &key, &data, 0);
        if (rc == 0)
        {
            m_bCached = TRUE;
            LOG("    Cache content entry written");
        }
        else
        {
            if (rc == DB_KEYEXIST)
            {
                LOG("    Duplicate content entry, not written");
                m_bCached = TRUE;
            }
            else
            {
                LOGX((szDbgTemp, "*** Error putting content entry into cache [%lu]", rc));
            }
        }
        delete [] pContent;
    }
    else
    {
        LOG ("*** Error allocating buffer for content");
    }

    m_pDbHeader->sync(m_pDbHeader, 0);

    HX_VECTOR_DELETE(pHeader);
    return(HXR_OK);
}

HX_RESULT
CCacheEntry::ReadCache (DBT* pdbtKey, DBT* pdbtHeader, DBT* pdbtContent, HXBOOL bMirroredServers)
{
    HX_RESULT   rc      = HXR_FAIL;
    UINT32      ulFlags = 0;

    LOG ("ReadCache()");
    if (m_pDbHeader == NULL || m_pDbData == NULL)
    {
        return(HXR_FAIL);
    }

    // if bMirroredServers is set, try an abbreviated domain name
    if (bMirroredServers)
    {
        LOG ("    Reading abbreviated URL");

        DBT dbtMirror = { NULL, 0 };

        dbtMirror.data = GetAbbrevUrl ((char*)pdbtKey->data, pdbtKey->size);
        dbtMirror.size = strlen ((char*)dbtMirror.data);

        if ((rc = m_pDbHeader->get (m_pContext, m_pDbHeader, &dbtMirror, pdbtHeader, ulFlags)) != HXR_OK)
        {
            LOG ("    No header entry in cache (Abbreviated URL)");
        }
        else
        {
            rc = m_pDbData->get (m_pContext, m_pDbData, &dbtMirror, pdbtContent, ulFlags);
        }
        delete [] (char*)dbtMirror.data;
    }

    if (rc != HXR_OK)
    {
        if ((rc = m_pDbHeader->get (m_pContext, m_pDbHeader, pdbtKey, pdbtHeader, ulFlags)) != HXR_OK)
        {
            LOG ("    No header entry in cache");
        }
        else
        {
            rc = m_pDbData->get (m_pContext, m_pDbData, pdbtKey, pdbtContent, ulFlags);
        }
    }
    
    // Record usage info
    if (rc == HXR_OK)
    {
        DBT   dbtUsage = { NULL, 0 };
        
        if ((rc = m_pDbUsage->get (m_pContext, m_pDbUsage, pdbtKey, &dbtUsage, ulFlags)) != 0)
        {
            LOG ("*** Get of usage failed");
        }
        else
        {
            PCacheUsage pUsage    = (PCacheUsage) dbtUsage.data;
            CacheUsage  usage     = { 0, 0, 0, 0, 0, 0 };
            DBT         dbtUsage2 = { &usage, sizeof (CacheUsage) };
            UINT32      ulNow     = time (NULL);
        
            usage.m_ulSize         = pUsage->m_ulSize;
            usage.m_ulCreateTime   = pUsage->m_ulCreateTime;
            usage.m_ulExpiryTime   = pUsage->m_ulExpiryTime;
            usage.m_ulLastUsedTime = time (NULL);
            usage.m_ulUseCount     = pUsage->m_ulUseCount + 1;
        
            if ((rc = m_pDbUsage->put (m_pDbUsage, pdbtKey, &dbtUsage2, ulFlags)) == DB_OK)
            {
                LOG ("    Put of usage info done");
                LOGX((szDbgTemp, "    Count=%lu Created=%ld Last=%ld Expiry=%ld", usage.m_ulUseCount,
                                                        ulNow - usage.m_ulCreateTime,
                                                        ulNow - usage.m_ulLastUsedTime,
                                                        usage.m_ulExpiryTime - ulNow));
                LOGX((szDbgTemp, "    Count=%ld Created=%08X Last=%08X Expiry=%08X", usage.m_ulUseCount,
                                                        usage.m_ulCreateTime,
                                                        usage.m_ulLastUsedTime,
                                                        usage.m_ulExpiryTime));
            }
            else
            {
               LOG ("$$$ Put failed (usage)");
            }

	    free(dbtUsage.data);
        }
        rc = HXR_OK;    // Use cache even if usage table has errors
    }
    
    LOGX ((szDbgTemp, "    %s: %s", rc ? "Miss" : "Hit", pdbtKey->data));

    return(rc);
}

static char *
GetAbbrevUrl (char* pData, UINT32 ulSize)
{
    UINT32      i               = 0;
    UINT32      j               = 0;
    UINT32      ulDotCount      = 0;
    UINT32      ulSlashCount    = 0;
    char*       pReturn         = NULL;

    if (pData == NULL)
    {
        LOG ("    NULL URL cannot be abbreviated");
        return(NULL);
    }

    // there should be three or more dots in the domain
    for (i = 0; i < ulSize; i++)
    {
        if (pData[i] == '\0')
        {
            break;
        }
        if ((pData[i] == '/' || pData[i] == '\\') && (++ulSlashCount > 2))
        {
            break;
        }
        if (pData[i] == '.')
        {
            ulDotCount++;
        }
    }
    if (ulSlashCount < 3 || ulDotCount < 3)
    {
        LOG ("    URL cannot be abbreviated");
        return(NULL);
    }   

    // allocate storage
    pReturn = new char[ulSize];

    // copy the protocol string
    for (i = 0; i < ulSize; i++)
    {
        if (pData[i] == '\0')
            break;

        pReturn[j++] = pData[i];

        if (i >= 2)
        {
            if (pData[i-2] == ':' && pData[i-1] == '/' && pData[i] == '/')
                break;
        }
    }

    // skip the first domain label
    do
    {
        if (pData[i] != '.')
            break;
    }
    while (++i < ulSize);

    // copy the rest of the URL
    do
    {
        pReturn[j++] = pData[i];
    }
    while ((++i < ulSize) && (pData[i] != '\0'));

    return (pReturn);
}

#define ENFORCE_SIZE_LIMIT
#define SECONDS_IN_UNIT     (10 * 60)    /* ten minutes */
#define BUMP_LIMIT          (16)

typedef struct _BUMPLIST {
    UINT32      ulRank;
    UINT32      ulSize;
    char*       pszUrl;
} BUMPLIST, *PBUMPLIST;

HX_RESULT
CCacheEntry::CleanCache(UINT32 ulCutOffDate, UINT32 ulSlackSeconds)
{
    /*
     * clean out expired entries
     */
    LOGX ((szDbgTemp, "CleanCache(%ld)", ulCutOffDate));

    UINT32          rc           = 0;
    DBT             dbtKey       = { NULL, 0 };
    DBT             dbtData      = { NULL, 0 };
    UINT32          ulIndex      = 0;
    UINT32          ulMbytesFree = 0;
    static UINT32   ulLowRank    = 0;            //FNH  Why static?
    static UINT32   ulLastTime   = 0;
    
    if (m_pDbHeader == NULL || m_pDbHeader == NULL || m_pDbHeader == NULL)
    {
        LOG ("*** NULL database handle");
        return(HXR_FAIL);
    }
    
    ulMbytesFree = GetFreeMbyteCount(m_pszDbDir);
    if (ulMbytesFree > DISK_FREE_LOW)
    {
        if (time(NULL) - ulLastTime < ulSlackSeconds)
            // Disk space is not low, and we checked recently
            return(HXR_OK);
    }

    ulLastTime = time(NULL);

    UINT32      ulDbSize                = 0;
    BUMPLIST    bumpList[BUMP_LIMIT]    = {{ 0, 0, NULL }}; // Bump up to this many entries

    /* Initialize the key/data pair so the flags aren't set. */
    memset(&dbtKey, 0, sizeof(dbtKey));
    memset(&dbtData, 0, sizeof(dbtData));
    /* Walk through the database and look at the key/data pairs. */

    HXBOOL firstTimeThrough = TRUE;
    while (TRUE)
    {
	rc = m_pDbUsage->seq(m_pContext, m_pDbUsage, &dbtKey, &dbtData, (firstTimeThrough)? R_FIRST : R_NEXT);
	firstTimeThrough = FALSE;
	if (rc != 0) break;

        PCacheUsage    pCacheUsage = (PCacheUsage) dbtData.data;

        LOGX ((szDbgTemp, "    Cache entry: (%7d bytes) UseCount=%ld TTL=%lds, LastUsed=%ld, %.*s",
             pCacheUsage->m_ulSize, pCacheUsage->m_ulUseCount,
             (INT32)pCacheUsage->m_ulExpiryTime - (INT32)time(NULL),
             (INT32)time(NULL) - (INT32)pCacheUsage->m_ulLastUsedTime,
             (int)dbtKey.size, (char *)dbtKey.data));

        // if expiry time is in the past or cutoff time is in the future
        HXBOOL  fDeleteEntry = TRUE;
        if (rc == 0 && pCacheUsage)
        {
             if (pCacheUsage->m_ulExpiryTime < (UINT32)time(NULL))
             {
                 LOG ("$$$ Cache entry has expired");
             }
             else  if(ulCutOffDate && pCacheUsage->m_ulCreateTime < ulCutOffDate)
             {
                 LOG ("$$$ Cache entry was created before cut-off date");
             }
             else  if(pCacheUsage->m_ulCreateTime < (UINT32)time(NULL) - SECS_IN_YEAR)
             {
                 LOG ("$$$ Cache entry more than a year old");
             }
             else  if(pCacheUsage->m_ulExpiryTime > (UINT32)time(NULL) + SECS_IN_YEAR)
             {
                 LOG ("$$$ Cache entry expires more than a year from now");
             }
             else
             {
                 fDeleteEntry = FALSE;
             }
        }
        // if expiry time is in the past or cutoff time is in the future
        if (rc == 0 && pCacheUsage && fDeleteEntry)
        {
             m_pDbData->del(m_pDbData, &dbtKey, 0);        // Remove expired entry from cache
             m_pDbUsage->del(m_pDbUsage, &dbtKey, 0);      // Remove expired entry from cache
             m_pDbHeader->del(m_pDbHeader, &dbtKey, 0);    // Remove expired entry from cache
        }
#ifdef ENFORCE_SIZE_LIMIT
        else if (pCacheUsage && !(pCacheUsage->m_ulFlags & CACHE_EVERGREEN))
        {
             ulDbSize += pCacheUsage->m_ulSize;

             // construct a list of entries and sizes so cache size limits can be enforced
             UINT32 ulRank  = (pCacheUsage->m_ulSize * 1000) / m_ulMaxCacheSize;
             ulRank += (time(NULL) - pCacheUsage->m_ulLastUsedTime) / SECONDS_IN_UNIT;
             ulRank /= pCacheUsage->m_ulUseCount;

             LOGX ((szDbgTemp, "    Rank=%3lu, %1.*s", ulRank, (int)dbtKey.size, (char *)dbtKey.data));

             // Add entry to list of items to possibly purge
             if (ulRank > ulLowRank)
             {
                 // The entry will bump the lowest ranked entry on the list
                 for (ulIndex = 0; ulIndex < BUMP_LIMIT; ulIndex++)
                 {
                  UINT32  ulNewLowRank = ulRank;

                  if (bumpList[ulIndex].ulRank == ulLowRank)
                  {
                      bumpList[ulIndex].ulRank = ulRank;
                      bumpList[ulIndex].ulSize = pCacheUsage->m_ulSize;

                      if (bumpList[ulIndex].pszUrl)
                           delete [] bumpList[ulIndex].pszUrl;
                      bumpList[ulIndex].pszUrl = new char [dbtKey.size + 1];
                      strncpy (bumpList[ulIndex].pszUrl, (char *)dbtKey.data, dbtKey.size); /* Flawfinder: ignore */
                      bumpList[ulIndex].pszUrl[dbtKey.size] = '\0';

                      ulLowRank = ulNewLowRank;
                  }
                  else    // check for next lowest ranked entry
                  {
                      if (ulNewLowRank > bumpList[ulIndex].ulRank)
                           ulNewLowRank = bumpList[ulIndex].ulRank;
                  }
                 }
                 // Finish the search for the next lowest ranked entry
                 while (++ulIndex < BUMP_LIMIT)
                 {
                  if (ulLowRank > bumpList[ulIndex].ulRank)
                      ulLowRank = bumpList[ulIndex].ulRank;
                 }
             }
        }
        else if (pCacheUsage)
        {
             LOG ("    Evergreen content");
             LOGX ((szDbgTemp, "    ulFlags=0x%08X", pCacheUsage->m_ulFlags));
        }
        else
        {
             LOG ("    No usage stats");
        }
#endif // ENFORCE_SIZE_LIMIT

    }

    if (rc != DB_NOTFOUND)
    {
        LOGX ((szDbgTemp, "*** DB sequential get failed *** [%d]", (int)rc));
    }

    #ifdef ENFORCE_SIZE_LIMIT
    // The ranked list of deletion candidates already exists, use it
    if (ulDbSize < m_ulMaxCacheSize)
    {
        LOGX ((szDbgTemp, "$$$ The cache size is within bounds [%lu vs %lu]", ulDbSize, m_ulMaxCacheSize));
    }
    else    // the array needs to be sorted
    {
        //FNH Find PN routine to sort list
        for (INT32 ulOne = 0; ulOne < BUMP_LIMIT; ulOne++)
        {
             for (INT32 ulTwo = ulOne + 1; ulTwo < BUMP_LIMIT; ulTwo++)
             {
                 if (bumpList[ulOne].ulRank < bumpList[ulTwo].ulRank)
                 {
                  UINT32 ulTempRank      = bumpList[ulOne].ulRank;
                  UINT32 ulTempSize      = bumpList[ulOne].ulSize;
                  char*  pszTempUrl      = bumpList[ulOne].pszUrl;

                  bumpList[ulOne].ulRank = bumpList[ulTwo].ulRank;
                  bumpList[ulOne].ulSize = bumpList[ulTwo].ulSize;
                  bumpList[ulOne].pszUrl = bumpList[ulTwo].pszUrl;

                  bumpList[ulTwo].ulRank = ulTempRank;
                  bumpList[ulTwo].ulSize = ulTempSize;
                  bumpList[ulTwo].pszUrl = pszTempUrl;
                 }
             }
        }
    }
    // start purging entries until the size is right
    for (INT32 ulVictim = 0; ulVictim < BUMP_LIMIT; ulVictim++)
    {
        if (bumpList[ulVictim].pszUrl == NULL)
             break;
        if (ulDbSize < m_ulMaxCacheSize - (m_ulMaxCacheSize / 20)) 
             break;

        LOGX ((szDbgTemp, "$$$ Bumping cache entry ranked %6d, Size = %6d, '%s'",
                bumpList[ulVictim].ulRank, bumpList[ulVictim].ulSize, bumpList[ulVictim].pszUrl));

        ulDbSize -= bumpList[ulVictim].ulSize;

        DBT dbtDelKey   = { bumpList[ulVictim].pszUrl, strlen(bumpList[ulVictim].pszUrl) };

        m_pDbData->del(m_pDbData, &dbtDelKey, 0);        // Remove expired entry from cache
        m_pDbUsage->del(m_pDbUsage, &dbtDelKey, 0);      // Remove expired entry from cache
        m_pDbHeader->del(m_pDbHeader, &dbtDelKey, 0);    // Remove expired entry from cache
    }
    #endif // ENFORCE_SIZE_LIMIT

    sync(0);  // Update disk file

    // delete any urls still left in the bumpList
    for (ulIndex = 0; ulIndex < BUMP_LIMIT; ulIndex++)
    {
	HX_VECTOR_DELETE(bumpList[ulIndex].pszUrl);
    }

    LOG ("... CleanCache() returns OK");
    return(HXR_OK);
}

/*
 * These are wrappers for the Berkeley DB C API.  They are fairly thin, but do
 * have defaults for several parameters.
 *
 * *** Note ***
 * In order to make the DB_TXN* argument optional, its location had to be moved
 * to be after the DBT* arguments.  This means the functions are not drop-in
 * replacements for Berkeley DB calls, the arguments are in a different order.
 * (Of course, the lack of a DB* argument already changed the arg list.)
 */

HX_RESULT
CCacheEntry::get (DBT *pdbtKey, DBT *pdbtHeader, DBT *pdbtContent, DB_TXN* pTxnid, UINT32 ulFlags)
{
    HX_RESULT rc = HXR_OK;
    
    LOG ("get()");
    if (m_pDbHeader == NULL || m_pDbData == NULL)
    {
        rc = HXR_FAIL;
    }
    else if ((rc = m_pDbHeader->get (m_pContext, m_pDbHeader, pdbtKey, pdbtHeader, ulFlags)) != 0)
    {
        LOG ("*** Get of header failed");
    }
    else if ((rc = m_pDbData->get (m_pContext, m_pDbData, pdbtKey, pdbtContent, ulFlags)) != 0)
    {
        LOG ("*** Get of content failed");
    }
    else
    {
        // This may not be needed
        m_pCacheHeader = (PCacheHeader) pdbtHeader->data;
    }
    
    LOG (rc ? "    Miss" : "    Hit");

    return(rc);
}

HX_RESULT
CCacheEntry::put (DB* pDb, DBT *pdbtKey, DBT *pdbtHeader, DBT *pdbtContent, DB_TXN* pTxnid, UINT32 ulFlags)
{
    HX_RESULT rc = HXR_FAIL;
    
    if (m_pDbHeader == NULL || m_pDbData == NULL)
    {
        LOG ("*** Error putting cache entry (NULL database handles)");
        return(HXR_FAIL);
    }
    if (pdbtHeader == NULL || pdbtContent == NULL)
    {
        LOG ("*** Error putting cache entry (NULL parameters)");
        return(HXR_FAIL);
    }
    if ((rc = m_pDbHeader->put (m_pDbHeader, pdbtKey, pdbtHeader, ulFlags)) == HXR_OK)
    {
        rc = m_pDbData->put (m_pDbData, pdbtKey, pdbtContent, ulFlags);
    }

    if (rc != HXR_OK)    
    {
        LOG ("*** Put failed");
    }
    sync (0);
    
    return(rc);
}

#if 0
        int (*close)    __P((struct __db *));
        int (*del)      __P((const struct __db *, const DBT *, UINT32));
        int (*get)      __P((const struct __db *, const DBT *, DBT *, UINT32));
        int (*put)      __P((const struct __db *, DBT *, const DBT *, UINT32));
        int (*seq)      __P((const struct __db *, DBT *, DBT *, UINT32));
        int (*sync)     __P((const struct __db *, UINT32));
#endif

UINT32
CCacheEntry::del (DBT* pdbtKey, DB_TXN* pTxnid, UINT32 ulFlags)
{   
    if (m_pDbHeader)
	m_pDbHeader->del (m_pDbHeader, pdbtKey, ulFlags);
    if (m_pDbData)
	m_pDbData->del (m_pDbData, pdbtKey, ulFlags);
    if (m_pDbUsage)
	m_pDbUsage->del (m_pDbUsage, pdbtKey, ulFlags);

    return HXR_OK;
}

UINT32
CCacheEntry::close (UINT32 ulFlags)
{
    if (m_pDbHeader)
    {
	m_pDbHeader->close (m_pDbHeader);
	m_pDbHeader = NULL;
    }

    if (m_pDbData)
    {
	m_pDbData->close (m_pDbData);
	m_pDbData = NULL;
    }

    if (m_pDbUsage)
    {
	m_pDbUsage->close (m_pDbUsage);
	m_pDbUsage = NULL;
    }

    return HXR_OK;
}

UINT32
CCacheEntry::sync (UINT32 ulFlags)
{
    if (m_pDbHeader)
	m_pDbHeader->sync (m_pDbHeader, ulFlags);
    if (m_pDbData)
	m_pDbData->sync (m_pDbData, ulFlags);
    if (m_pDbUsage)
	m_pDbUsage->sync (m_pDbUsage, ulFlags);

    return HXR_OK;
}

#ifdef _WIN32
UINT32
GetFreeMbyteCount (const char *pDirectoryName)
{
    HXBOOL	bOK                           = FALSE;
    HXBOOL	bUseExtFunc		      = FALSE;
    ULONG32	ulFreeMbytes                  = 0;
    ULONGLONG	ullFreeBytesAvailableToCaller = 0;
    ULONGLONG	ullTotalNumberOfBytes         = 0;
    ULONGLONG	ullTotalNumberOfFreeBytes     = 0;
    DWORD	dwSectorsPerCluster = 0;	    // pointer to sectors per cluster
    DWORD	dwBytesPerSector = 0;		    // pointer to bytes per sector
    DWORD	dwNumberOfFreeClusters = 0;	    // pointer to number of free clusters
    DWORD	dwTotalNumberOfClusters = 0;	    // pointer to total number of clusters	
    LPVOID	lpMsgBuf = NULL;

    CHAR	pucDisk[4]                    = { 0 };
    
    GETDISKFREESPACEEX	_getDiskFreeSpaceEx = NULL;
    HINSTANCE		hLib		    = NULL;
    
    /* 
     * If there is an error creating the database, we do not store the 
     * directory name in m_pszDbDir in CCacheEntry constructor
     */
    HX_ASSERT(pDirectoryName != NULL);
    if (pDirectoryName == NULL)
    {
	return 0;
    }

    strncpy (pucDisk, pDirectoryName, 3); /* Flawfinder: ignore */
    pucDisk[3] = '\0';

    // Windows 95 OSR2: The GetDiskFreeSpaceEx function is available on Windows 95 
    // systems beginning with OEM Service Release 2 (OSR2). 
    //
    // To determine whether GetDiskFreeSpaceEx is available, call the LoadLibrary 
    // load the KERNEL32.DLL file, then call the GetProcAddress function to obtain 
    // an address for GetDiskFreeSpaceEx. If GetProcAddress fails, use the 
    // GetDiskFreeSpace function instead of GetDiskFreeSpaceEx.
    //
    // NOTE: if we don't do so, the httpfsys will not be loaded by the plugin handler!!
    if (!(hLib = LoadLibrary(OS_STRING("kernel32.dll"))))
    {
	goto cleanup;
    }    

    _getDiskFreeSpaceEx = (GETDISKFREESPACEEX)GetProcAddress(hLib, OS_STRING("GetDiskFreeSpaceExA"));

    if (_getDiskFreeSpaceEx)
    {
	bOK = _getDiskFreeSpaceEx(OS_STRING(pucDisk),
				  (PULARGE_INTEGER) &ullFreeBytesAvailableToCaller,
				  (PULARGE_INTEGER) &ullTotalNumberOfBytes,
				  (PULARGE_INTEGER) &ullTotalNumberOfFreeBytes);

	if (bOK)
	{
	    ulFreeMbytes = (ULONG32)(ullTotalNumberOfFreeBytes >> 20);
	    goto cleanup;
	}
    }

#ifndef _WINCE
    bOK = GetDiskFreeSpace(OS_STRING(pucDisk),
                          (LPDWORD) &dwSectorsPerCluster,
                          (LPDWORD) &dwBytesPerSector,
                          (LPDWORD) &dwNumberOfFreeClusters,
			  (LPDWORD) &dwTotalNumberOfClusters);

    if (bOK)
    {
        ulFreeMbytes = (ULONG32)(dwBytesPerSector * dwSectorsPerCluster * dwNumberOfFreeClusters);
    }
#endif /* _WINCE */

cleanup:

    if (hLib)
    {
	FreeLibrary(hLib);
    }

    return(ulFreeMbytes);
}            
#elif defined(_MACINTOSH)
UINT32
GetFreeMbyteCount (const char *pDirectoryName)
{
    ULONG32     ulFreeMbytes                = 0;
    HParamBlockRec	volPB;
    OSErr	error;
    UCHAR volName[256];
    ULONG32 ullTotalNumberOfFreeBytes     = 0;
    size_t len;
    
    /* 
     * If there is an error creating the database, we do not store the 
     * directory name in m_pszDbDir in CCacheEntry constructor
     */
    HX_ASSERT(pDirectoryName != NULL);
    if (pDirectoryName == NULL)
    {
      return 0;
    }

	/*
	 * Volume name is length delimited! (Note that this is not used as an input parameter.)
	 * Note also that this routine always returns the free byte of the first mounted volume
	 * (default volume, don't confuse the directory with the volume name).  --wy 2/9/00 
	 */
	len = strlen(pDirectoryName);
	if(len > 255)
		len = 255;
	memcpy(volName+1, pDirectoryName, len); /* Flawfinder: ignore */
	volName[0] = (UCHAR)len;

    volPB.volumeParam.ioNamePtr = volName;
    volPB.volumeParam.ioVRefNum = 0;
    volPB.volumeParam.ioVolIndex = 1;	
    
    error = PBHGetVInfo(&volPB,false);
    
    if(error == noErr)
    {
	ullTotalNumberOfFreeBytes = volPB.volumeParam.ioVFrBlk * volPB.volumeParam.ioVAlBlkSiz;
        ulFreeMbytes = (ULONG32)(ullTotalNumberOfFreeBytes >> 20);
    }
    return(ulFreeMbytes);
} 
#else
UINT32
GetFreeMbyteCount (const char *pDirectoryName)
{
    return(DISK_FREE_LOW * 2);
}
#endif
