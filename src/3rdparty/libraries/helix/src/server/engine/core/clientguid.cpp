/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clientguid.cpp,v 1.12 2007/05/23 18:52:58 seansmith Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

/*
 * ClientGUIDTable:
 * ClientGUIDEntry:  A persistent table of client data, indexed by GUID.
 *
 *
 * Implemented to allow multiple GETs from an HTTP client (such as WMP) to
 * find the same ClientRegTree, and hence have aggregated statistics.
 *
 * Also allows incoming POSTs from WMP to find the correct GET client.
 *
 * The Table class protects its table with a mutex.  It's up to the users
 * of the returned Entry object to ensure that their access/modification of
 * Entry data are threadsafe, either by usage design or with explicit mutexes.
 *
 * It's assumed that the Client that creates the Entry will be the one to
 * Done() it, so we're on the correct proc to use the Entry's m_pContext to
 * set a callback.
 *
 */

#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "dict.h"
#include "debug.h"
#include "config.h"
#include "netbyte.h"

#include "mutex.h"
#include "server_context.h"
#include "server_info.h"

#include "hxstats.h"
#include "server_stats.h"

#include "clientguid.h"

const int ClientGUIDEntry::MAX_TIME_STRING_LEN = 80;

/*
 * ClientGUIDTable
 */

ClientGUIDTable::ClientGUIDTable()
{
    m_Mutex = HXMutexCreate();
    HXMutexInit(m_Mutex);
    m_pTable = new CHXMapStringToOb;

    // If this server doesn't do MMS-HTTP at all, I'm not sure we'll need this,
    // so we pass FALSE as the alloc-now parameter.
    // If it is used, we want to start with a reasonable HTTP client count.

    m_pTable->InitHashTable(1000, FALSE);
}

ClientGUIDTable::~ClientGUIDTable()
{
    // this destructor should never be called.
    HX_ASSERT(0);

    delete m_pTable;
    HXMutexDestroy(m_Mutex);
}

/*
 * ClientGUIDTable::GetCreateEntry()
 *
 * Get existing table entry if it exists, or create a new one.
 *
 * Entry is returned with 1 (additional) AddRef on it.
 *
 * playerGUID == NULL returns an Entry with no slot in the global table
 * (a "private" entry).
 *
 * Use case: an HTTP client connects, disconnects, connects again.  We
 * expect, therefore, that there be no Entry or an Entry with no Client.
 * If you have a different use case, you'll have to decide whether the
 * Entry switches Clients, keeps the old Client, or maintains a list.
 */
ClientGUIDEntry*
ClientGUIDTable::GetCreateEntry(const char* playerGUID, Client* pClient)
{
    ClientGUIDEntry* pEntry = NULL;

    // Is there already an entry?

    if (playerGUID)
    {
        HXMutexLock(m_Mutex);
        m_pTable->Lookup(playerGUID, (void*&) pEntry);

        // must make sure pEntry isn't deleted before we can inspect it.

        if (pEntry) pEntry->AddRef();        // AddRef() for caller (did exist)
        HXMutexUnlock(m_Mutex);
    }

    if (pEntry)
    {
        // Entry already in table

        // But is it defunct?  (Entries are marked Defunct before they're
        // physically removed.)

        // Must lock mutex to make sure we're fully through the critical
        // section in ClientGUIDEntry::Func().

        BOOL bDefunct = FALSE;

        HXMutexLock(pEntry->m_DefunctMutex);
        bDefunct = pEntry->m_bDefunct;
        HXMutexUnlock(pEntry->m_DefunctMutex);

        if (bDefunct)
        {
            RemoveEntry(pEntry);  // must ensure it's removed before we add
            pEntry->Release();    // the same key below.
            pEntry = NULL;
        }
        if (pEntry)
        {
            pEntry->ClientGUIDEntryInit(pClient);
        }
    }

    if (!pEntry)
    {
        // No entry in table
        // Create new entry

        pEntry = new ClientGUIDEntry();
        pEntry->AddRef();                      // AddRef() for caller (created)
        pEntry->ClientGUIDEntryInit(pClient);

        // NULL playerGUID => private GUIDEntry (no table entry)

        if (playerGUID)
        {
            pEntry->AddRef();                   // AddRef() for table
            pEntry->m_bInTable = TRUE;

            HXMutexLock(m_Mutex);
            (*m_pTable)[playerGUID] = pEntry;   // add to table
            HXMutexUnlock(m_Mutex);

            pEntry->SetPlayerGUID(playerGUID);  // remember key so it can
                                                //  remove itself later
        }
    }

    return pEntry;
}

/*
 * ClientGUIDEntry::GetEntry()
 *
 * Call ClientGUIDEntryInit() on the returned entry if you want to assign
 * the entry to a new client.
 */
ClientGUIDEntry*
ClientGUIDTable::GetEntry(const char* playerGUID)
{
    ClientGUIDEntry* pEntry = NULL;
    HXMutexLock(m_Mutex);
    m_pTable->Lookup(playerGUID, (void*&) pEntry);
    HXMutexUnlock(m_Mutex);

    return pEntry;
}

BOOL
ClientGUIDTable::RemoveEntry(ClientGUIDEntry* pEntry)
{
    /*
     * Make sure we only remove once; if a request for a certain GUID
     * comes just too late, we'll remove the old entry, create a new one,
     * and add it in ClientGUIDTable::GetCreateEntry(); we don't want the
     * old entry's Func() call to remove the new entry.  (The remove is
     * based on GUID, not pEntry ptr.)
     */
    BOOL bInTable;
    BOOL bFound = FALSE;

    HX_ASSERT(pEntry);
    if (!pEntry) return FALSE;

    HXMutexLock(pEntry->m_RemoveMutex);

    bInTable = pEntry->m_bInTable;
    pEntry->m_bInTable = FALSE;

    HXMutexUnlock(pEntry->m_RemoveMutex);

    HX_ASSERT(!bInTable || pEntry->GetPlayerGUID());
    if (bInTable && pEntry->GetPlayerGUID())
    {
        HXMutexLock(m_Mutex);
        bFound = m_pTable->RemoveKey(pEntry->GetPlayerGUID());
        HXMutexUnlock(m_Mutex);

        HX_ASSERT(bFound);
        if (bFound) pEntry->Release();   // Release() for table
    }

    return bFound;
}

/*
 * ClientGUIDEntry
 */

ClientGUIDEntry::ClientGUIDEntry()
    : m_lRefCount(0)
      , m_pClient(NULL)
      , m_pPlayerGUID(NULL)
      , m_pContext(NULL)
      , m_bDefunct(FALSE)
      , m_bDone(FALSE)
      , m_bInTable(FALSE)
      , m_nDestructDelay(0)
      , m_pClientStats(NULL)
{
    m_DefunctMutex = HXMutexCreate();
    HXMutexInit(m_DefunctMutex);
    m_RemoveMutex = HXMutexCreate();
    HXMutexInit(m_RemoveMutex);
}

ClientGUIDEntry::~ClientGUIDEntry()
{
    HX_ASSERT(m_bDone);

    // XXXDPL - Safety check, in case we forgot to call Done().
    if (!m_bDone)
    {
        if (m_pClientStats)
        {
             m_pClient->m_pProc->pc->client_stats_manager->RemoveClient(m_pClientStats->GetID(),  m_pClient->m_pProc);
        }
        HX_RELEASE(m_pClientStats);
        HX_RELEASE(m_pClient);
    }

    HX_RELEASE(m_pContext);
    HXMutexDestroy(m_RemoveMutex);
    HXMutexDestroy(m_DefunctMutex);
}

void
ClientGUIDEntry::SetPlayerGUID(const char* playerGUID)
{
    // This is the key under which Entry was stored to GUIDTable.
    // NULL => no entry in GUIDTable.

    HX_VECTOR_DELETE(m_pPlayerGUID);

    if (playerGUID)
    {
        int len = strlen(playerGUID) + 1;

        m_pPlayerGUID = new char [ len ];
        memcpy(m_pPlayerGUID, playerGUID, len);
    }
}

/*
 * ClientGUIDEntry::ClientGUIDEntryInit()
 *
 * Assumes only one client at a time 'owns' the entry, that old clients
 * always call Done() before new ones take ownership of the entry, and that
 * this procedure is serialized so a mutex isn't needed.
 *
 * Add a check or mutex if this assumption is bad.
 */
void
ClientGUIDEntry::ClientGUIDEntryInit(Client* pClient)
{
    HX_ASSERT(pClient);

    HX_RELEASE(m_pClient);
    m_pClient = pClient;

    IHXBuffer* pValue = NULL;

    if (m_pClient)
    {
        time_t     start_time;
        char       start_time_string[MAX_TIME_STRING_LEN];
        struct tm start_time_struct;

        m_pClient->AddRef();

        HX_RELEASE(m_pContext);
         m_pClient->m_pProc->pc->server_context->
            QueryInterface(IID_IUnknown, (void**) &m_pContext);

        if (pClient->m_pProc->pc->client_stats_manager->UseRegistryForStats())
        {
            m_RegTree.RegTreeInit(pClient, pValue);
        }

        if (!m_pClientStats && !m_pClient->m_bIsAProxy)
        {
            m_pClientStats = new ClientStats(pClient->m_pProc);
            m_pClientStats->AddRef();

            m_pClient->m_pStats = m_pClientStats;
            m_pClient->m_pStats->AddRef();

            time(&start_time);
            hx_localtime_r(&start_time, &start_time_struct);
            strftime(start_time_string, MAX_TIME_STRING_LEN, "%d/%b/%Y:%H:%M:%S",
                     &start_time_struct);
            pClient->m_pProc->pc->common_class_factory->CreateInstance(CLSID_IHXBuffer, (void**)&pValue);
            pValue->Set((const BYTE*)start_time_string, strlen(start_time_string) + 1);
            m_pClientStats->SetStartTime(pValue);


            pClient->m_pProc->pc->client_stats_manager->AddClient(m_pClientStats,  m_pClient->m_pProc);
            pClient->SetClientStatsObjId(m_pClientStats->GetID());

        }
    }

    HX_RELEASE(pValue);
}

HX_RESULT
ClientGUIDEntry::Done()
{
    HX_ASSERT(!m_bDefunct);

    m_bDone = TRUE;

    if (m_pClientStats)
    {
        if (m_pClient)
        {
             m_pClient->m_pProc->pc->client_stats_manager->RemoveClient(m_pClientStats->GetID(),  m_pClient->m_pProc);
        }
    }

    HX_RELEASE(m_pClientStats);
    HX_RELEASE(m_pClient);

    // set self-remove timer, if requested.

    if (m_nDestructDelay)
    {
        // Set callback to destroy ourself if no one picks us up
        // Must always get a scheduler for the current context.

        IHXThreadSafeScheduler* pScheduler = NULL;
        m_pContext->QueryInterface(IID_IHXThreadSafeScheduler,
                                       (void**) &pScheduler);
        pScheduler->RelativeEnter(this, m_nDestructDelay * 1000);
        pScheduler->Release();
    }
    else
    {
        // No delay needed; but because of table reference, we can't put
        // this in the dtor.

        m_bDefunct = TRUE;
        RemoveFromTable();
    }

    return HXR_OK;
}

HX_RESULT
ClientGUIDEntry::SetDestructDelay(unsigned int seconds)
{
    /*
     * Taking the max means multiple requestors need not cooperate.
     * Allow zero to cancel it, though.
     */

    if (seconds)
    {
        seconds = MAX(m_nDestructDelay, seconds);
    }

    m_nDestructDelay = seconds;
    return HXR_OK;
}

BOOL
ClientGUIDEntry::RemoveFromTable()
{
    BOOL removed = FALSE;

    if (m_pPlayerGUID)
    {
        removed = g_pClientGUIDTable->RemoveEntry(this);
        delete [] m_pPlayerGUID;
        m_pPlayerGUID = 0;
    }
    return removed;
}

/*
 * IHXCallback methods
 */
STDMETHODIMP
ClientGUIDEntry::Func()
{
    /*
     * The purpose of scheduling this callback is to delay object destruction.
     *
     * To prevent the race condition when looking up an entry which this
     * callback is about to destroy, we use the defunct boolean/mutex here and
     * in the Get{Create}Entry calls.
     *
     * We don't bother to remove
     * callbacks from the scheduler when new requests come in, because it would
     * be bothersome to Send a remove function to the correct proc, if
     * different from the one calling ClientGUIDEntryInit() again.  The
     * m_bDefunct should tell you whether this callback is really deleting the
     * entry or not.
     */

    HXMutexLock(m_DefunctMutex);
    if (m_lRefCount <= 2)
    {
        HX_ASSERT(m_lRefCount == 2 || !m_bInTable);
        m_bDefunct = TRUE;
    }
    HXMutexUnlock(m_DefunctMutex);

    if (m_bDefunct)
    {
        RemoveFromTable();
    }
    return HXR_OK;
}


/*
 * IUnknown methods
 */
STDMETHODIMP
ClientGUIDEntry::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
ClientGUIDEntry::AddRef()
{
    HXAtomicIncINT32(&m_lRefCount);   // HXAtomicIncRetINT32 is more costly,
    return 1;                         // and not needed.
}


STDMETHODIMP_(ULONG32)
ClientGUIDEntry::Release()
{
    INT32 refcount;
    if ((refcount = HXAtomicDecRetINT32(&m_lRefCount)) > 0)
    {
        return refcount;
    }

    delete this;
    return 0;
}
