/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: progdown.h,v 1.7 2005/03/14 19:41:59 bobclark Exp $
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

#ifndef PROGDOWN_H
#define PROGDOWN_H

// Forward declarations
typedef _INTERFACE IHXDataFile  IHXDataFile;
typedef _INTERFACE IHXScheduler IHXScheduler;
typedef _INTERFACE IHXRegistry  IHXRegistry;
class CHXGenericCallback;

// Defines - these can all be overridden
// with preferences (see CheckPreferenceValues())
#define PROGDOWN_STAT_INTERVAL_DEFAULT  1000 // Check filesize at this interval
#define PROGDOWN_STAT_INTERVAL_INITIAL     8 // Initially start at this interval and backoff (CANNOT BE ZERO)
#define PROGDOWN_FAIL_INTERVAL_DEFAULT   100 // Retry failed seeks/reads at this interval
#define PROGDOWN_FINISHED_TIME_DEFAULT  5000 // If filesize unchanged for this duration,
                                             // then assume the download is finished
#define PROGDOWN_FORMER_PROG_RETRY_INIT   20 // If we were formerly progressive but not now, retry this many
                                             // times before failing out
#define PROGDOWN_NOT_PROG_RETRY_INIT      40 // If we have never been progressive, retry this many
                                             // times before failing out

class CProgressiveDownloadMonitorResponse : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // CProgressiveDownloadMonitorResponse methods
    STDMETHOD(ProgressiveCallback) (THIS) PURE;
    STDMETHOD(ProgressiveStatCallback) (THIS) PURE;
};

class CProgressiveDownloadMonitor : public CHXBaseCountingObject
{
public:
    CProgressiveDownloadMonitor();
    virtual ~CProgressiveDownloadMonitor();

    HX_RESULT   Init(IUnknown* pContext, IHXDataFile* pFile,
                     CProgressiveDownloadMonitorResponse* pResponse);
    HX_RESULT   Close();
    HXBOOL        IsProgressive() { return m_bIsProgressive; }
    HXBOOL        HasBeenProgressive() { return m_bHasBeenProgressive; }
    UINT32      GetFormerProgressiveRetryCount() { return m_ulFormerProgRetryCount; }
    void        DecrementFormerProgressiveRetryCount() { if (m_ulFormerProgRetryCount) m_ulFormerProgRetryCount--; }
    UINT32      GetNotProgressiveRetryCount() { return m_ulNotProgRetryCount; }
    void        DecrementNotProgressiveRetryCount() { if (m_ulNotProgRetryCount) m_ulNotProgRetryCount--; }
    void        ResetNotProgressiveRetryCount() { m_ulNotProgRetryCount = m_ulNotProgRetryInit; }
    UINT32      GetLastFileSize() { return m_ulLastFileSize; }
    HX_RESULT   ScheduleCallback();
    HXBOOL        IsCallbackPending();
    void        CancelCallback();
    HX_RESULT   BeginSizeMonitoring();
    void        EndSizeMonitoring();
    UINT32      GetFileSizeNow();
    void        MonitorFileSize();

    static void StatCallback(void* pArg);
    static void ProgCallback(void* pArg);
protected:
    IUnknown*                            m_pContext;
    IHXDataFile*                         m_pDataFile;
    CProgressiveDownloadMonitorResponse* m_pResponse;
    IHXScheduler*                        m_pScheduler;
    IHXRegistry*                         m_pRegistry;
    CHXGenericCallback*                  m_pStatCallback;
    CHXGenericCallback*                  m_pProgCallback;
    UINT32                               m_ulStatCallbackInterval;
    UINT32                               m_ulCurStatInterval;
    UINT32                               m_ulProgCallbackInterval;
    UINT32                               m_ulFinishedTime;
    UINT32                               m_ulLastFileSize;
    UINT32                               m_ulTickAtLastFileSize;
    UINT32                               m_ulURLRegistryID;
    UINT32                               m_ulIsProgRegistryID;
    UINT32                               m_ulFormerProgRetryCount;
    UINT32                               m_ulFormerProgRetryInit;
    UINT32                               m_ulNotProgRetryCount;
    UINT32                               m_ulNotProgRetryInit;
    HXBOOL                                 m_bIsProgressive;
    HXBOOL                                 m_bMonitorEnabled;
    HXBOOL                                 m_bHasBeenProgressive;

    void      CheckPreferenceValues(REF(HXBOOL)   rbMonitorEnabled,
                                    REF(UINT32) rulStatCallbackInterval,
                                    REF(UINT32) rulProgCallbackInterval,
                                    REF(UINT32) rulFinishedTime,
                                    REF(UINT32) rulFormerProgRetryCount,
                                    REF(UINT32) rulNotProgRetryCount);
    HX_RESULT InitRegistryStats();
    void      UpdateRegistryStats();
    void      ScheduleStatCallback();
};

#endif /* #ifndef PROGDOWN_H */
