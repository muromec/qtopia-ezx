/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fsdb.cpp,v 1.11 2006/02/07 20:49:12 ping Exp $
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

#include "hxtypes.h"

#ifndef _MACINTOSH
# include "hlxclib/sys/types.h"
//# include <sys/stat.h>
#endif

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"

#include "safestring.h"
#include "hxdir.h"      // for mkdir()
#include "hxcom.h"
#include "db.h"
#include "findfile.h"
#include "chxdataf.h"
#include "hxstrutl.h"

#if defined( _WIN32) && !defined(_WINCE)
# include <io.h>
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define LOG_FILE    "C:/Temp/db.log"
#include "http_debug.h"

#define MAX_CACHE_ENTRY_SIZE    (8 * 1024 * 1024)   /* 8 MBytes limit on entry size */

#ifdef _WIN32
# define BINARY_FILE "b"
#else
# define BINARY_FILE ""
#endif

int db_func_close (DB *);
int db_func_del   (const DB *, const DBT *, UINT32);
int db_func_get   (IUnknown*, const DB *, const DBT *, DBT *, UINT32);
int db_func_put   (const DB *, DBT *, const DBT *, UINT32);
int db_func_seq   (IUnknown*, const DB *, DBT *, DBT *, UINT32);
int db_func_sync  (const DB *, UINT32);
int db_func_fd    (const DB *);

typedef struct _FileList {
    char*             pFilename;
    struct _FileList* pNext;
}  FileList; 

static HX_RESULT GrabMutex       (const DB* pDb);
static HX_RESULT FreeMutex       (const DB* pDb);
static HX_RESULT CloseMutex      (const DB* pDb);
static HX_RESULT createDatabase  (DB* pDb, const char* pszFilename);
static void      createPath      (const char* pszFilename);
static HX_RESULT destroyDatabase (const DB* pDb, const char* pszFilename);
static HX_RESULT verifyDatabase  (IUnknown* pContext, const char* pszFilename);
static HX_RESULT GetFilename     (const DB* pDb, const DBT* pKey, UINT8** ppszFilename, FILE** ppFile);
static HX_RESULT GetKeyAndData   (char* pFilename, DBT* pKey, DBT* pData);
static HXBOOL      CompareKey      (const DBT* pKey, FILE* pFile);
static HX_RESULT WriteSection    (FILE* pFile, const DBT* pDbThang);
static void      CreateFileList  (const DB* pDb, FileList* pListHead);
static void      FreeFileList    (FileList* pList);

extern UINT32    GetFreeMbyteCount (const char *pDirectoryName);

#if defined (_WINDOWS ) || defined (_WIN32)
#define OS_SEPARATOR_CHAR       '\\'
#define OS_SEPARATOR_STRING     "\\"
#elif defined (_UNIX)
#define OS_SEPARATOR_CHAR       '/'
#define OS_SEPARATOR_STRING     "/"
#elif defined (__MWERKS__)
#define OS_SEPARATOR_CHAR       ':'
#define OS_SEPARATOR_STRING     ":"
#endif // defined (_WINDOWS ) || defined (WIN32)

UINT32
get_hash (const DBT* pKey)
{
    UINT32  hash    = 5381;
    UINT8*  pStr    = (UINT8*) pKey->data;
    UINT32  ulSize  = pKey->size;

    while (ulSize--)
        hash = ((hash << 5) + hash) + *pStr++;

    return hash;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifndef O_RDONLY
#define O_RDONLY 001
#endif

DB *
dbopen(IUnknown*   pContext,
       const char* pszFilename, 
       int         ulFlags,
       int         nMode,
       DBTYPE      type,
       DB_INFO*    pInfo)
{
    DB*       pDb                = (DB*) calloc (1, sizeof (DB));
    UINT32    rc                 = 0;
    UINT32    i                  = 0;
    char*     pLastSlash         = NULL;
    
    LOG ("dbopen()");

    if (pDb == NULL)
    {
        LOG ("*** Cannot allocate space for database reference");
        return(NULL);
    }
    if (type != DB_HASH)
    {
        LOG ("*** Only hashed database entries are supported\n");
        // Go ahead and treat it as a hashed database anyway
    }
        
    // Record the directory path
    pDb->pDir = (char*)malloc (strlen(pszFilename) + 1);
    strcpy (pDb->pDir, pszFilename); /* Flawfinder: ignore */
    pLastSlash = strrchr (pDb->pDir, OS_SEPARATOR_CHAR);
    if (pLastSlash)
    {
        *pLastSlash = '\0';
    }
    else
    {
        *pDb->pDir = '\0';
    }
    LOGX ((szDbgTemp, "    Base directory = '%s'", pDb->pDir));
    
    // Set up the function references
    pDb->close = db_func_close;
    pDb->del   = db_func_del;
    pDb->get   = db_func_get;
    pDb->put   = db_func_put;
    pDb->seq   = db_func_seq;
    pDb->sync  = db_func_sync;
    pDb->fd    = db_func_fd;
    
    // Make sure we are the only ones accessing the database
    if (GrabMutex(pDb) == HXR_FAIL)
    {
        LOG ("    Could not get mutex to open file");
        return(NULL);
    }
    
    if (ulFlags & DB_TRUNCATE) 
    {
        LOG ("    DB Truncate flag, destroying existing database");
        destroyDatabase(pDb, pszFilename);
    }
    
    // Verify the database structure
    rc = verifyDatabase (pContext, pszFilename);
    
    if ((rc != HXR_OK) && !(ulFlags & DB_CREATE))
    {
        LOG ("    DB does not exist, create flag not present");
        FreeMutex (pDb);
        return (NULL);
    }
    
#if 0
    if ((rc == HXR_OK) && (ulFlags & DB_NOOVERWRITE))
    {
        LOG ("    DB exists: DB_NOOVERWRITE flag prohibits success");
        FreeMutex (pDb);
        return (NULL);
    }
#endif // 0

    if ((rc != HXR_OK) && (ulFlags & DB_CREATE))
    {
        LOG ("    Creating new database");
	    UINT32          ulMbytesFree = 0;

	    ulMbytesFree = GetFreeMbyteCount(pDb->pDir);
        if (ulMbytesFree < DISK_FREE_LOW)
        {
            LOG ("*** Not enough disk space to safely create cache");
            FreeMutex (pDb);
            return(NULL);
        }
        rc = createDatabase(pDb, pszFilename);
        if (rc == HXR_OK)
        {
            rc = verifyDatabase (pContext, pszFilename);
        }
    }
    
    if (rc != HXR_OK)
    {

        LOG ("*** Verify failed, closing database");
        db_func_close (pDb);
        pDb = NULL;
    }
    FreeMutex (pDb);
    
    return(pDb);
}

#ifdef __cplusplus
}
#endif

static HX_RESULT
createDatabase(DB* pDb, const char* pszFilename)
{
    FILE*  pDbFile  = NULL;
    
    createPath (pszFilename);
    
#ifdef _WINCE
	::DeleteFile(OS_STRING(pszFilename));
#else
    unlink (pszFilename);
#endif /* _WINCE */
    pDbFile = fopen (pszFilename, "w");
    if (pDbFile)
    {
        UINT32  ulTime = time(NULL);
        char szBuf[26];
        fprintf (pDbFile, "REALFSDB\n"
                          "%s"
                          "Partitioning=%d\n",
                          hx_ctime_r((time_t *)&ulTime, szBuf),
                          0);
        fclose (pDbFile);      
    }
    return(pDbFile ? HXR_OK : HXR_FAIL);
}

static void
createPath (const char* pszFilename)
{
    UINT32        ulIndex  = 0;
    UINT8*        pDir     = NULL;
	CHXDirectory  Dir;
    
    if (pszFilename == NULL)
        return;
        
    pDir = (UINT8*)calloc (strlen(pszFilename) + 1, 1);
    
    for (ulIndex = 0; ulIndex < MAX_FILENAME; ulIndex++)
    {
        pDir[ulIndex] = pszFilename[ulIndex];
        
        if (pszFilename[ulIndex] == '\0')
            break;
        if (pszFilename[ulIndex + 1] == OS_SEPARATOR_CHAR)   
        {
	    Dir.SetPath((char*)pDir);
	    if(!Dir.IsValid())
	    {
	        Dir.Create();
	    }
	}
    }
    free (pDir);
}

static HX_RESULT
destroyDatabase(const DB* pDb, const char* pszFilename)
{
    char       pBaseDir[MAX_FILENAME] = { 0 }; /* Flawfinder: ignore */
    FileList   fileList           = { 0, 0 };
    FileList*  pEntry                 = NULL;
    FileList*  pNext                  = NULL;
    
#ifdef _WINCE
	::DeleteFile(OS_STRING(pszFilename));
#else
    unlink (pszFilename);
#endif /* _WINCE */

    // Delete all of the hash filenames
    CreateFileList (pDb, &fileList);

    pEntry = fileList.pNext;
    while (pEntry)
    {
        pNext = pEntry->pNext;
        remove (pEntry->pFilename);
        free (pEntry->pFilename);
        free (pEntry);
        pEntry = pNext;
    }
    
    return(HXR_OK);
}


static HX_RESULT
verifyDatabase (IUnknown* pContext, const char* pszFilename)
{
    HX_RESULT     rc      = HXR_FAIL;
    CHXDataFile*  pPnFile = CHXDataFile::Construct(pContext);
    HX_ASSERT(pPnFile);

    if (pPnFile == NULL)
    {
        LOG ("    HXR_OUTOFMEMORY");
	    rc = HXR_OUTOFMEMORY;
    }
    else        
    {
        if (pPnFile->Open (pszFilename, O_RDONLY) == HXR_OK)
        {
            ULONG32  ulFileSize = 0;
            
            ulFileSize = pPnFile->GetSize();
            if (ulFileSize > 4)
            {
               rc = HXR_OK;
            }
            else   
            {
                LOGX((szDbgTemp, "    Size too small: %lu [%s]", ulFileSize, pszFilename));
            }
            pPnFile->Close();
        }
        else       
        {
            LOG ("*** Cannot open pPnFile");
        }
	delete pPnFile;
    }
    
    // Check the sub-directory structure
    
    LOG ("    Database verified");
    return(rc);
}

int
db_func_close (DB *pDb)
{
    LOG ("db_func_close()");
    
    if (pDb) {
        CloseMutex (pDb);
        
        if (pDb->pDir)
        {
            free (pDb->pDir);
        }    
        free (pDb);
    }
    
    return(HXR_OK);
}

int
db_func_del (const DB *pDb, const DBT *pKey, UINT32 ulFlags)
{
    UINT8*          pszFilename     = 0;
    FILE*           pFile           = NULL;

    LOG ("db_func_del()");

    // Grab control of the database
    if (GrabMutex(pDb) != HXR_OK)
    {
        return(HXR_FAIL);
    }
    
    GetFilename (pDb, pKey, &pszFilename, &pFile);
    if (pszFilename && pFile)
    {
        LOGX((szDbgTemp, "    Deleting entry %s", pszFilename));
        // We have found the entry, delete it
        fclose (pFile);
        pFile = NULL;
        remove ((char*)pszFilename);
    }
    else
    {
        LOG ("    Nothing deleted");
    }
    if (pFile)
    {
        fclose(pFile);
    }
    if (pszFilename)
    {
        free (pszFilename);
    }
    
    FreeMutex (pDb);
    
    // Return the result
    return(HXR_OK);
}

int
db_func_get (IUnknown* pContext, const DB *pDb, const DBT *pKey, DBT *pData, UINT32 ulFlags)
{
    UINT32          ulEntryIndex    = 0;
    char*           pszFilename     = NULL;
    FILE*           pFile           = NULL;
    static  UINT8*  pucData         = NULL;
    
    LOG ("db_funct_get()");
    
    pData->size = 0;
    pData->data = NULL;
    
    // Grab control of the database
    if (GrabMutex(pDb) != HXR_OK)
    {
        return(HXR_FAIL);
    }
    
    GetFilename (pDb, pKey, (UINT8**)&pszFilename, &pFile);
    if (pszFilename && pFile)
    {
        UINT32  ulFileSize  = 0;
        UINT32  ulSize      = 0;
        UINT8   pSizeStr[4] = { 0 };
        
        LOGX((szDbgTemp, "    Found entry %s", pszFilename));
        // We have found the entry, get the data
        if (fread(pSizeStr, 1, sizeof pSizeStr, pFile) == sizeof pSizeStr)
        {
            ulSize = pSizeStr[0];                   // Architecture independent
            ulSize = (ulSize << 8) | pSizeStr[1]; 
            ulSize = (ulSize << 8) | pSizeStr[2]; 
            ulSize = (ulSize << 8) | pSizeStr[3];
            
            // Get the file's actual size to verify data length
            CHXDataFile* pPnFile  = CHXDataFile::Construct(pContext);
            HX_ASSERT(pFile);
            if (pPnFile != NULL)
            {
                if (pPnFile->Open (pszFilename, O_RDONLY) == HXR_OK)
                {
                    ulFileSize = pPnFile->GetSize();
                    pPnFile->Close();
                }       
		delete pPnFile;
            }
            
            if (sizeof pKey->size + pKey->size + sizeof ulSize + ulSize != ulFileSize)
            {
                LOGX((szDbgTemp, "*** Incorrect file length (on get!): %lu + %lu + %lu + %lu vs %lu",
                                   sizeof pKey->size, pKey->size, sizeof ulSize, ulSize, ulFileSize));
                fclose (pFile);
                pFile = NULL;
                remove (pszFilename);
                free (pszFilename);
                FreeMutex (pDb);
                return(HXR_FAIL);
            }
            LOGX((szDbgTemp, "    Data size is %lu bytes", ulSize));
            pData->size = ulSize;
            pData->data = malloc (ulSize);
            
            fread (pData->data, 1, ulSize, pFile);
        }
        else    
        {
            LOGX((szDbgTemp, "    fread failed  [%s]", strerror(errno)));
        }
        fclose (pFile);
        pFile = NULL;
        free (pszFilename);
        pszFilename = NULL;
    }
    else
    {
        LOG ("    Miss");
    }
    
    if (pFile)
    {
        fclose(pFile);
    }
    if (pszFilename)
    {
        free (pszFilename);
    }
    
    pucData = (UINT8*)pData->data;

    FreeMutex (pDb);

    // Return the result
    LOG (pucData ? "...db_get succeeded" : "...db_get failed");
    return(pucData ? HXR_OK : HXR_FAIL);
}

int
db_func_put (const DB *pDb, DBT *pKey, const DBT *pData, UINT32 ulFlags)
{
    HXBOOL        bFailed      = FALSE;
    UINT32      ulBlockCount = 0;
    UINT32      i            = 0;
    char*       pszFilename  = NULL;
    FILE*       pFile        = NULL;
    
    LOG ("db_funct_put()");
    
    // Grab control of the database
    if (GrabMutex(pDb) != HXR_OK)
    {
        return(DB_PUT_ERROR);
    }
    
    // If overwrite mode selected, zap any existing entries
    if (!(ulFlags & DB_NOOVERWRITE))
        db_func_del (pDb, pKey, 0);
        
    // Fail if DB_NOOVERWRITE and entry exists
    GetFilename (pDb, pKey, (UINT8**)&pszFilename, &pFile);
    if (pszFilename && pFile)
    {
        LOG ("");
        LOGX((szDbgTemp, "    *** Target file exists, aborting put: '%s'", pszFilename));
        fclose (pFile);
        free (pszFilename);
        FreeMutex (pDb);
        return(DB_KEYEXIST);
    }
    else
    {
        UINT32  ulSize      = 0;
        
        LOGX((szDbgTemp, "    Writing entry '%s'", pszFilename));
        
        if (ulSize > MAX_CACHE_ENTRY_SIZE)
        {
            LOG ("*** Cache entry too big");
            free (pszFilename);
            return(HXR_FAIL);
        }
        
        // Create the entry file
        if ((pFile = fopen (pszFilename, "w" BINARY_FILE)) != NULL)
        {
            WriteSection (pFile, pKey);
            WriteSection (pFile, pData);
            fclose (pFile);
            pFile = NULL;
        }
        else
        {
            LOG ("    File creation failed");
        }
    }

    FreeMutex (pDb);

    if (pszFilename)
	free(pszFilename);

    LOG (bFailed ? "...db_put failed" : "...db_put succeeded");
    return(bFailed ? DB_PUT_ERROR : DB_PUT_SUCCESS);
}

static HX_RESULT
WriteSection (FILE* pFile, const DBT* pDbThang)
{
    UINT8      pSizeStr[4] = { 0 };
    HX_RESULT  rc          = HXR_FAIL;
    
    if (pFile == NULL || pDbThang == NULL)
    {
        LOG ("WriteSection(): Null parameters");
        return(HXR_FAIL);
    }
    LOGX((szDbgTemp, "    WriteSection Size=%lu", pDbThang->size));

    pSizeStr[0] = (pDbThang->size >> 24) & 0xFF;
    pSizeStr[1] = (pDbThang->size >> 16) & 0xFF;
    pSizeStr[2] = (pDbThang->size >>  8) & 0xFF;
    pSizeStr[3] = (pDbThang->size >>  0) & 0xFF;   
    
    if (fwrite (pSizeStr, 1, sizeof pSizeStr, pFile) == sizeof pSizeStr)
    {
        if (fwrite (pDbThang->data, 1, pDbThang->size, pFile) == pDbThang->size)
            rc = HXR_OK;
    }    
    if (rc != HXR_OK)    
    {
        LOGX((szDbgTemp, "    Write failed [%08X] [%s]", pFile, strerror(errno)));
    }
    LOGX((szDbgTemp, "    File postion = %lu [%08X]", ftell(pFile), pFile));
    
    fflush(NULL);
    
    return(rc);
}            

int
db_func_seq (IUnknown* pContext, const DB *pDb, DBT *pKey, DBT *pHeader, UINT32 ulFlags)
{
    static UINT32     ulNext       = 0;
    static UINT8*     pKeyData     = NULL;
    static UINT8*     pHeaderData  = NULL;
    static HXBOOL       bFirstTime   = TRUE;
    HX_RESULT         rc           = HXR_FAIL;
    FILE*             pFile        = NULL;
    HXBOOL              bKeyValid    = FALSE;
    UINT32 ulSize                  = 0;
    UINT32            ulFileSize   = 0;
    UINT8             pSizeStr[4]  = { 0 };
    
    char*             pFilename    = NULL;
    char              pFullName[MAX_FILENAME]; /* Flawfinder: ignore */
    
    static FileList*  pTail        = NULL;
    static FileList   listHead     = { NULL, NULL };    // A nice linked list of directory entries

    LOG ("db_func_seq()");

    // data is valid until the routine is called again
    if (pKeyData) {
        free (pKeyData);
        pKeyData = NULL;
    }
    if (pHeaderData) {
        free (pHeaderData);
        pHeaderData = NULL;
    }

    if (bFirstTime == TRUE && ulFlags == R_NEXT)
    {
        bFirstTime = FALSE;
        ulFlags = R_FIRST;
    }
        
    switch (ulFlags) {
      case R_FIRST:
        // Create list of files, set index to zero
        CreateFileList ((DB*)pDb, &listHead);
        pTail = listHead.pNext;

        // Fall through to case 'R_NEXT'
                
      case R_NEXT:
      {
        // Get the key and data
        if (pTail == NULL)
        {
            FreeFileList (&listHead);
            return(DB_NOTFOUND);
        }   
        
        // Get the entry
        SafeSprintf(pFullName, MAX_FILENAME, "%-.400s%c%-.400s", pDb->pDir, OS_SEPARATOR_CHAR, pTail->pFilename);
        
        // Get the file's actual size to verify data length
        CHXDataFile* pPnFile  = CHXDataFile::Construct(pContext);
        HX_ASSERT(pPnFile);
        if (pPnFile != NULL)
        {
            if (pPnFile->Open (pFullName, O_RDONLY) == HXR_OK)
            {
                ulFileSize = pPnFile->GetSize();
                pPnFile->Close();
            }  
	    delete pPnFile;
        }
        
        if ((pFile = fopen (pFullName, "r")) != NULL)
        {
            
            // Get the Key size
            if (fread(pSizeStr, 1, sizeof pSizeStr, pFile) == sizeof pSizeStr)
            {
                ulSize = pSizeStr[0];                   // Architecture independent
                ulSize = (ulSize << 8) | pSizeStr[1]; 
                ulSize = (ulSize << 8) | pSizeStr[2]; 
                ulSize = (ulSize << 8) | pSizeStr[3];
                 
                
                if (sizeof ulSize + ulSize + sizeof (UINT32) > ulFileSize) 
                {
                    LOGX ((szDbgTemp, "*** Key size too large: %lu vs %lu", ulSize, ulFileSize));
                }
                else if (ulSize < 5) 
                {
                    LOG ("*** Key size too small");  // "http:" is the shortest
                }
                else
                {
                    
                    pKey->size = ulSize;
                    pKey->data = malloc (ulSize);
                    if (fread (pKey->data, 1, ulSize, pFile) != ulSize)
                    {
                        LOGX ((szDbgTemp, "*** Bad read on key data"));
                    }
                    else
                    {
                        LOGX ((szDbgTemp, "    Got Key[%04X]=%1.*s", pKey->size, pKey->size, pKey->data));
                        bKeyValid = TRUE;
                    }
                    pKeyData = (UINT8*)pKey->data;
                }
            }
            pSizeStr[0] = 0;
            pSizeStr[1] = 0;
            pSizeStr[2] = 0;
            pSizeStr[3] = 0;
            
            // Get the Data size
            if (bKeyValid && (fread(pSizeStr, 1, sizeof pSizeStr, pFile) == sizeof pSizeStr))
            {
                ulSize = pSizeStr[0];                   // Architecture independent
                ulSize = (ulSize << 8) | pSizeStr[1]; 
                ulSize = (ulSize << 8) | pSizeStr[2]; 
                ulSize = (ulSize << 8) | pSizeStr[3];
                 
                if (ulSize > MAX_CACHE_ENTRY_SIZE) 
                {
                    LOG ("*** Data size too large");
                }
                else
                {
                    INT32 nReadBytes = 0;
                    
                    pHeader->size = ulSize;
                    pHeader->data = malloc (ulSize);
                    if ((nReadBytes = fread (pHeader->data, 1, ulSize, pFile)) == (INT32) ulSize)
                    {
                        rc = HXR_OK;
                    }
                    else
                    {
                        LOGX ((szDbgTemp, "    Bad read on data data: %d vs %d", nReadBytes, ulSize));
                        rc = HXR_FAIL;
                    }
                    LOGX ((szDbgTemp, "    Got Data[%ld]", pHeader->size));
                    
                    pHeaderData = (UINT8*)pHeader->data;
                }
            }
            fclose (pFile);
        }
        else        
        {
            LOG ("*** Cannot open file for sequential access");
        }
        pTail = pTail->pNext;

        break;
      }  
      default:
        LOGX ((szDbgTemp, "*** Invalid flag = %lu", ulFlags));
        break;
    }

    return(rc);
}

static void
FreeFileList (FileList* pList)
{
    FileList* pTemp = pList;
    
    pList = pList->pNext;   // the first entry is not freeable
    pTemp->pNext = NULL;    // ...but it does need to be pruned
    
    while (pList)
    {
        pTemp = pList->pNext;
        
        if (pList->pFilename)
            free (pList->pFilename);
        free (pList);
        
        pList = pTemp;    
    } 
}

static void
CreateFileList (const DB* pDb, FileList* pList)
{
//  HANDLE           hSearch     = NULL;
//  WIN32_FIND_DATA  findData    = { 0 };
//  char             pCheckFilename[MAX_FILENAME];
    FileList*        pTail       = pList;
    char*            pszDllName  = NULL;
    CFindFile*	      pFileFinder = NULL;

    FreeFileList (pList);
    
    LOG ("CreateFileList()");
    
    pFileFinder = CFindFile::CreateFindFile(pDb->pDir, 0, "????????.???");
    if (NULL == pFileFinder)
    {
	return;
    }
    pszDllName = pFileFinder->FindFirst();
    while (pszDllName)
    {
	pszDllName	= pFileFinder->GetCurFilePath();
        
        if (pszDllName != NULL)
        {
            HX_ASSERT (strlen(pDb->pDir) < strlen(pszDllName));
            
            char *pszFileOnly = pszDllName + strlen(pDb->pDir) + 1;
            
            if (strlen(pszFileOnly) != 12)    // 8.3 filename
            {
//              LOGX ((szDbgTemp, "    Skipping '%s'  [Length]", pszDllName));
            }
            else if (strspn (pszFileOnly, "0123456789abcdefABCDEF.") != strlen(pszFileOnly))
            {
//              LOGX ((szDbgTemp, "    Skipping '%s'  [Alphabet]", pszFileOnly));
            }
            else
            {
//              LOGX ((szDbgTemp, "    Adding '%s'", pszDllName));
                // Add file to list
                pTail->pNext = (FileList*)malloc (sizeof (FileList));
                pTail = pTail->pNext;
                pTail->pNext = NULL;
                pTail->pFilename = (char*)malloc (strlen (pszFileOnly) + 1);
                strcpy (pTail->pFilename, pszFileOnly); /* Flawfinder: ignore */
            }
        }        
	pszDllName	= pFileFinder->FindNext();
    }
    HX_DELETE(pFileFinder);
}       

int
db_func_sync  (const DB *pDb, UINT32 ulFlags)
{
    LOG ("db_func_sync()");

    return(HXR_OK);    
}

int
db_func_fd    (const DB *pDb)
{
    return(HXR_FAIL);
}

/*
 * GetFilename (pDb, pszUrl, ppszFilename, ppFile)
 *
 * This is passed a URL and returns the filename to be used for that URL.
 * The file handle ppFile is set if an entry for the URL already exists.
 * The position of the file pointer is to the start of the data section
 * of the file when this function returns.
 *
 * If no file exists for a given key, a suitable filename is returned
 * and the value ppFile is set to NULL.
 *
 * The caller is responsible for freeing ppszFilename and closing ppFile.
 */
#define HASHED_FILENAME_OVERFLOW_MAX    1000

static HX_RESULT
GetFilename (const DB* pDb, const DBT* pKey, UINT8** ppszFilename, FILE** ppFile)
{
    UINT32  hash                                   = 0;
    char    pCheckFilename[MAX_FILENAME]; /* Flawfinder: ignore */
    char    rgucUsed[HASHED_FILENAME_OVERFLOW_MAX] = { 0 };  /* Flawfinder: ignore */ // Flag to indicate filename is used
    
//  HANDLE           hSearch     = NULL;
//  WIN32_FIND_DATA  findData    = { 0 };
    FILE*            pFile       = NULL;
    HXBOOL             bMatch      = FALSE;
    char*            pszDllName  = NULL;
    CFindFile*	      pFileFinder = NULL;

    LOG ("GetFilename()");
    // Make sure the return values are initially zero
    *ppszFilename = NULL;
    *ppFile = NULL;
    
    // get the has value
    hash = get_hash (pKey);
    
    // Search for alternate extensions
//  sprintf (pCheckFilename, "%s%c%08X.???", pDb->pDir, OS_SEPARATOR_CHAR, hash);
    
    // Get list of files which match the URL's hash value
    SafeSprintf(pCheckFilename, MAX_FILENAME, "%08X.???", hash);
    pFileFinder = CFindFile::CreateFindFile(pDb->pDir, 0, pCheckFilename);
    if (NULL == pFileFinder)
    {
	return HXR_FAIL;
    }
    pszDllName = pFileFinder->FindFirst();
    while (pszDllName)
    {
	pszDllName	= pFileFinder->GetCurFilePath();
        
        if (pszDllName != NULL)
        {
            UINT32 ulIndex = 0;

            LOGX ((szDbgTemp, "    Looking at [%s]", pszDllName));
        
            if (strlen(pszDllName) < 4)
                break;

            // Mark name as in use
            ulIndex = atol (pszDllName + strlen(pszDllName) - 4);
            if (ulIndex < HASHED_FILENAME_OVERFLOW_MAX)
                rgucUsed[ulIndex] = 1;
        
            // Get the URL from the file
            SafeSprintf(pCheckFilename, MAX_FILENAME, "%-.400s%c%-.400s", pDb->pDir, OS_SEPARATOR_CHAR, pszDllName);
            pFile = fopen (pszDllName, "r" BINARY_FILE "+");
            if (pFile == NULL)
            {
                LOGX ((szDbgTemp, "    Cannot open file '%s'  [%s]", pCheckFilename, strerror(errno)));
                break;
            }
            if (CompareKey (pKey, pFile))    
            {
                bMatch = TRUE;
                break;
            }
            fclose (pFile);
	    pszDllName = pFileFinder->FindNext();
        }
    }
    if (bMatch == TRUE)
    {
        LOG ("    Match");
        if (ppszFilename)
        {
            int lenFilename = strlen(pszDllName) + 2;
            *ppszFilename = (UINT8*)malloc (lenFilename);
            SafeSprintf ((char*)*ppszFilename, lenFilename, "%s", pszDllName); /* Flawfinder: ignore */
        }
        if (ppFile)
        {
            *ppFile = pFile;
        }
    }
    else
    {
        // No match, return first valid name
        if (ppszFilename)
        {
            UINT32 ulIndex = 0;
            for (ulIndex = 0; ulIndex < HASHED_FILENAME_OVERFLOW_MAX; ulIndex++)
            {
                if (rgucUsed[ulIndex] == 0)
                {
		    int lenFilename = strlen (pDb->pDir) + 16;
                    *ppszFilename = (UINT8*)malloc (lenFilename);
                    SafeSprintf ((char*)*ppszFilename, lenFilename, "%s%c%08X.%03lu", pDb->pDir, OS_SEPARATOR_CHAR, hash, ulIndex); /* Flawfinder: ignore */
                    LOGX ((szDbgTemp, "    No match, return first valid name [%s]", *ppszFilename));
                    break;
                } 
            }
        }
    }

    HX_DELETE(pFileFinder);

    return (pFile ? HXR_OK : HXR_FAIL);    
}

/*
 * CompareKey() -- The hash matches the key's hash, verify by reading the
 *                 key string from the file and doing a full compare.
 */    
static HXBOOL
CompareKey (const DBT* pKey, FILE* pFile)
{
    UINT32    ulSize        = 0;
    UINT8     pSizeStr[4]   = { 0 };
    char*     pKeyStr       = NULL;
    HXBOOL      bMatch        = FALSE;
    
    if (fread(pSizeStr, 1, sizeof pSizeStr, pFile) == sizeof pSizeStr)
    {
        ulSize = pSizeStr[0];                   // Architecture independent
        ulSize = (ulSize << 8) | pSizeStr[1]; 
        ulSize = (ulSize << 8) | pSizeStr[2]; 
        ulSize = (ulSize << 8) | pSizeStr[3];
         
        if (ulSize != pKey->size)
        {
            LOG ("    Size check fails");
            fclose (pFile);
            return (FALSE);
        }
        pKeyStr = (char*)malloc (ulSize);
        if (pKeyStr)
        {
            if (fread (pKeyStr, 1, ulSize, pFile) != ulSize)
            {
                LOG ("    Error reading key string");
            }
            else
            {
                LOGX ((szDbgTemp, "    '%1.*s' vs '%s'  [%lu]", ulSize, pKeyStr, pKey->data, ulSize));
                if (strncmp (pKeyStr, (char*)pKey->data, ulSize) == 0)
                {
                    bMatch = TRUE;
                }
            }
        }
        if (pKeyStr)
            free (pKeyStr);

        return(bMatch);
    }
    else
    {
        LOG ("*** Could not read size bytes");
    }
    return(FALSE);
}

static HX_RESULT
GetKeyAndData (char* pFilename, DBT* pKey, DBT* pData)
{
    return(FALSE);
}
    
/*
 * The following inclusion of C code will be replaced by a change
 * in the UMAKE file to add a platform dependent target for 'pndbsupp'.
 */
 
#ifdef _WIN32
# include "./platform/win/hxdbsupp.c"
#endif

#if defined(_UNIX) || defined(__TCS__)
# include "./platform/unix/hxdbsupp.c"
#endif

#ifdef _MACINTOSH
# include "./platform/mac/hxdbsupp.cpp"
#endif

#ifdef _OPENWAVE
# include "./platform/openwave/hxdbsupp.c"
#endif

