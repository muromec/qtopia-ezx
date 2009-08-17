/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rssmgr.h,v 1.2 2003/09/25 07:32:03 atin Exp $
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

#ifndef _RSSMGR_H_
#define _RSSMGR_H_

#include "servlist.h"
#include "hxcom.h"
#include "proc.h"

#define DEFAULT_RSS_INTERVAL 60

// Reg Entries
#define REGENTRY_RSSINTERVAL        "config.RSSInterval"
#define REGENTRY_RSSROLLTYPE        "config.RSSRollType"
#define REGENTRY_RSSLOGFILENAME     "config.RSSLogFilename"
#define REGENTRY_RSSLOGDIRECTORY    "config.RSSLogDirectory"
#define REGENTRY_RSSLOGROLLTIME     "config.RSSLogRollTime"
#define REGENTRY_RSSLOGROLLOFFSET   "config.RSSLogRollOffset"
#define REGENTRY_RSSLOGROLLSIZE     "config.RSSLogRollSize"
#define REGENTRY_RSSLOGPRUNETHRESH  "config.RSSLogPruneThreshold"

// Reg ID Index
enum {
    REGID_RSSINTERVAL = 0,
    REGID_RSSLOGFILENAME,
    REGID_RSSLOGDIRECTORY,
    REGID_RSSLOGROLLTIME,
    REGID_RSSLOGROLLOFFSET,
    REGID_RSSLOGROLLSIZE,
    REGID_RSSLOGPRUNETHRESH,
    REGID_MAX
};

class LogOutput;
class LogFileOutput;

class RSSMReportElem : public HXListElem
{
public:
    RSSMReportElem(IHXRSSReport* pStatObj);
    ~RSSMReportElem();

    IHXRSSReport* m_pStatObj;
};

class RSSManager : public BaseCallback
                 , public IHXPropWatchResponse
                 , public IHXRSSManager
{
public:
    RSSManager(Process* pProc);
    ~RSSManager();

    // Only use this function in extreme situations
    // when we don't want to lock any mutexes or open
    // any files (like during a stack trace).
    static void CrashOutput(const char *pString) { m_pOutput->CrashOutput(pString); }

    // IUnknown Methods
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj);

    STDMETHOD_(UINT32,AddRef)               (THIS);

    STDMETHOD_(UINT32,Release)              (THIS);

    // BaseCallback
    STDMETHOD(Func)             (THIS);


    // IHXPropWatchResponse Methods
    STDMETHODIMP AddedProp(const UINT32 id,
                           const HXPropType propType,
                           const UINT32 ulParentID)
                                { return HXR_OK; }
    STDMETHODIMP DeletedProp(const UINT32 id,
                             const UINT32 ulParentID)
                                { return HXR_OK; }
    STDMETHODIMP ModifiedProp(const UINT32 id,
                              const HXPropType propType,
                              const UINT32 ulParentID);

    // IHXRSSManager Methods
    STDMETHOD(Register)         (THIS_
                                 IHXRSSReport* pStatObj);
    STDMETHOD(Remove)           (THIS_
                                 IHXRSSReport* pStatsObj);

    static LogFileOutput       *m_pOutput;
    IHXRegistry         *m_pRegistry;
    IHXScheduler        *m_pScheduler;
    IHXPropWatch        *m_pPropWatch;
    static UINT32       m_ulRSSReportInterval;
    static BOOL         m_bRSSReportEnabled;

protected:

    void    InitOutput();

    UINT32   m_ulRefCount;
    HXList*  m_pStatsList;
    Process* m_pProc;
    UINT32   m_nRegIds[REGID_MAX];
};


#endif /* _RSSMGR_H_ */
