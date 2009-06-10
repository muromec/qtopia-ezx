/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: _main.h,v 1.25 2006/04/03 18:40:54 seansmith Exp $
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

#ifndef _MAIN_H_
#define _MAIN_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "simple_callback.h"
#include "base_callback.h"
#include "plgnhand.h"
#include "hxmon.h"
#include "timeval.h"
#include "proc.h"
#include "bcast_defs.h"
#include "hxpropw.h"
#include "ihxlist.h"
#include "hxrssmgr.h"
#include "hxlogoutputs.h"
#include "hxbuffer.h"

extern int* VOLATILE pResolverProcInitMutex;

#if !defined(_WIN32)
int* MakeProcess(const char* processname, SimpleCallback* cb,
    DispatchQueue* dispatch_queue, int flags = 0, void* linuxstuff = 0);
#else
int* MakeProcess(const char* processname, SimpleCallback* cb,
    DispatchQueue* dispatch_queue, int flags = 0);
#endif

#define STARTUP_LOG     "startup.log"
extern char g_szStartupLog[];
extern BOOL g_bUseStartupLog;

static void
suprintf(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
#if defined(__amd64__)
    va_list args_copy;
    va_copy(args_copy, args);
    vfprintf(stdout, pFmt, args_copy);
#else
    vfprintf(stdout, pFmt, args);
#endif
    fflush(stdout);
    if (g_bUseStartupLog)
    {
        FILE *fp = fopen(g_szStartupLog, "a");
        if (fp)
        {
            vfprintf(fp, pFmt, args);
            fflush(fp);
            fclose(fp);
        }
    }
#if defined(__amd64__)
    va_end(args_copy);
#endif
    va_end(args);
}

void terminate(int code);
BOOL ShutdownServer(BOOL bManually = FALSE);
BOOL RestartServer(BOOL bManually = FALSE);
void perform_restart();
BOOL RestartOrKillServer();
void RestartOnFault(BOOL bRestart);
const char* GetProgName(const char* arg);
void StopAliveChecker();

class Server;
class Config;
class StreamerInfo;
class LogDB;
class LBLAcceptor;

class CoreTransferCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
    Process*            core_proc;
    int                 m_debug_level;
    int                 m_debug_func_level;

    static const char*  zm_pRequiredPlugins[];
    static const INT16  zm_nReqPlugChecksum;
    int* volatile core_waiting_for_refresh;

private:
                        ~CoreTransferCallback() {};


    INT16               GetNumRequiredPlugins(char** ppRequiredPlugins);
};

class CreateStreamerCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
private:
                        ~CreateStreamerCallback() {};

};

class CreateMiscProcCallback : public SimpleCallback
{
public:
    void                        func(Process* proc);
    BOOL                        m_bNeedSocket;
    PluginHandler::Plugin*      m_plugin;
    LBLAcceptor*                m_pLBLAcceptor;
private:
                        ~CreateMiscProcCallback() {};

};

class CreateResolverCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
    int                 calling_procnum;
    SimpleCallback*     resolver_cb;
private:
                        ~CreateResolverCallback() {};

};

class WriteConfigCallback : public SimpleCallback, public BaseCallback
{
public:
    WriteConfigCallback ();
    WriteConfigCallback (const char * pKeyName);

    void                func(Process* proc);
    STDMETHOD(Func)     (THIS);
    int                 calling_procnum;
    SimpleCallback*     resolver_cb;
private:
    ~WriteConfigCallback();
    char*                m_pKeyName;
    Process*             m_pProc;

};

class OncePerSecond : public BaseCallback
{
public:
    OncePerSecond(Process* pProc);
    STDMETHOD(Func)     (THIS);
    Process*            m_pProc;
private:

};


class RSSCoreStatsReport: public IHXRSSReport
{
public:
    RSSCoreStatsReport(Process *pProc);
    ~RSSCoreStatsReport();


    Process* m_pProc;

    // IUnknown COM Interface Methods
    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);
    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();

    // IHXRSSReport Methods
    STDMETHODIMP Report(IHXLogOutput *pOutput, BOOL bEchoToStdout,
        HXTimeval tSkedNow);
    RSSManager* m_pRSSManager;

private:

    bool        m_bFirstFire;
    UINT32      m_ulRefCount;

    Timeval     m_tLastTime;
    UINT32      m_ulLastBytesServed;
    UINT32      m_ulLastForcedSelects;
    UINT32      m_ulLastPPS;
    UINT32      m_ulLastOverloads;
    UINT32      m_ulLastNoBufs;
    UINT32      m_ulLastOtherUDPErrs;
    UINT32      m_ulLastNewClients;
    UINT32      m_ulLastLeavingClients;
    UINT32      m_ulLastMutexCollisions;
    UINT32      m_ulLastMemOps;
    UINT32      m_ulLastSchedulerItems;
    UINT32      m_ulLastSchedulerItemsWithMutex;
#ifdef XXXAAKTESTRTSP
    UINT32      m_ulLastOptionsMsg;
    UINT32      m_ulLastDescribeMsg;
    UINT32      m_ulLastSetupMsg;
    UINT32      m_ulLastSetParameterMsg;
    UINT32      m_ulLastPlayMsg;
    UINT32      m_ulLastTeardownMsg;
#endif
#if ENABLE_LATENCY_STATS
    UINT32      m_ulLastCorePassCBTime;
    UINT32      m_ulLastCorePassCBCnt;
    UINT32      m_ulLastCorePassCBMax;
    UINT32      m_ulLastDispatchTime;
    UINT32      m_ulLastDispatchCnt;
    UINT32      m_ulLastDispatchMax;
    UINT32      m_ulLastStreamerTime;
    UINT32      m_ulLastStreamerCnt;
    UINT32      m_ulLastStreamerMax;
    UINT32      m_ulLastFirstReadTime;
    UINT32      m_ulLastFirstReadCnt;
    UINT32      m_ulLastFirstReadMax;
#endif
    UINT32      m_ulLastCachedMallocs;
    UINT32      m_ulLastCachedMisses;
    UINT32      m_ulLastCachedNew;
    UINT32      m_ulLastCachedDelete;
    UINT32      m_ulLastShortTermAlloc;
    UINT32      m_ulLastShortTermFree;
    UINT32      m_ulLastPageFrees;
    UINT32      m_ulLastPagesAllocated;
    UINT32      m_ulLastFreeListEntriesSearched;
    UINT32      m_ulLastBehind;
    UINT32      m_ulLastResends;
    UINT32      m_ulLastAggreg;
    UINT32      m_ulLastWouldBlockCount;
    UINT32      m_ulLastSocketAcceptCount;
    UINT32      m_ulLastConcurrentOps;
    UINT32      m_ulLastConcurrentMemOps;
    UINT32      m_ulLastSchedulerElems;
    UINT32      m_ulLastISchedulerElems;
    UINT32      m_ulLastTotalNetReaders;
    UINT32      m_ulLastTotalNetWriters;
    UINT32      m_ulLastMutexNetReaders;
    UINT32      m_ulLastMutexNetWriters;
    UINT32      m_ulLastIdlePPMs;
    double      m_fLastBroadcastDistBytes;
    UINT32      m_ulLastBroadcastDistPackets;
    BrcvStatistics  m_LastBrcvStats;
    BdstStatistics  m_LastBdstStats;
    UINT32      m_ulLastBroadcastPacketsDropped;
    UINT32      m_ulLastBroadcastPPMOverflows;
    UINT32      m_ulLastFastBytesRead;
    UINT32      m_ulLastSlowBytesRead;
    UINT32      m_ulLastInternalBytesRead;
    UINT32      m_LastMainLoops[MAX_THREADS+1];
};

class PortWatcher: public IHXPropWatchResponse
{
public:
    PortWatcher(Process* pProc);

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(AddedProp)        (THIS_
                                const UINT32            ulId,
                                const HXPropType       propType,
                                const UINT32            ulParentID);

    STDMETHOD(ModifiedProp)     (THIS_
                                const UINT32            ulId,
                                const HXPropType       propType,
                                const UINT32            ulParentID);

    STDMETHOD(DeletedProp)      (THIS_
                                const UINT32            ulId,
                                const UINT32            ulParentID);
private:
    Process*    m_pProc;
    IHXRegistry* m_pReg;
};

class UnixTimerInitCallback : public SimpleCallback
{
public:
    void                func(Process* proc);

private:
                        ~UnixTimerInitCallback() {};

};

#endif /* _MAIN_H_ */
