/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: progdown.cpp,v 1.11 2006/01/30 21:58:03 ping Exp $
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
#include "hxcom.h"
#include "ihxpckts.h"
#include "hlxclib/sys/stat.h"
#include "hxdataf.h"
#include "hxprefs.h"
#include "hxmon.h"
#include "hxcomm.h"
#include "hxcbobj.h"
#include "hxprefutil.h"
#include "pckunpck.h"
#include "baseobj.h"
#include "hxtick.h"
#include "smplmlog.h"
#include "progdown.h"

CProgressiveDownloadMonitor::CProgressiveDownloadMonitor()
{
    MLOG_PD(NULL, "CON CProgressiveDownloadMonitor 0x%08x\n", this);
    m_pContext               = NULL;
    m_pDataFile              = NULL;
    m_pResponse              = NULL;
    m_pScheduler             = NULL;
    m_pRegistry              = NULL;
    m_pStatCallback          = NULL;
    m_pProgCallback          = NULL;
    m_ulStatCallbackInterval = PROGDOWN_STAT_INTERVAL_DEFAULT;
    m_ulCurStatInterval      = PROGDOWN_STAT_INTERVAL_INITIAL;
    m_ulProgCallbackInterval = PROGDOWN_FAIL_INTERVAL_DEFAULT;
    m_ulFinishedTime         = PROGDOWN_FINISHED_TIME_DEFAULT;
    m_ulLastFileSize         = 0;
    m_ulTickAtLastFileSize   = 0;
    m_ulURLRegistryID        = 0;
    m_ulIsProgRegistryID     = 0;
    m_ulFormerProgRetryCount = 0;
    m_ulFormerProgRetryInit  = PROGDOWN_FORMER_PROG_RETRY_INIT;
    m_ulNotProgRetryCount    = 0;
    m_ulNotProgRetryInit     = PROGDOWN_NOT_PROG_RETRY_INIT;
    m_bIsProgressive         = FALSE;
    m_bMonitorEnabled        = TRUE;
    m_bHasBeenProgressive    = FALSE;
}

CProgressiveDownloadMonitor::~CProgressiveDownloadMonitor()
{
    MLOG_PD(NULL, "DES CProgressiveDownloadMonitor 0x%08x\n", this);
    Close();
}

HX_RESULT CProgressiveDownloadMonitor::Init(IUnknown* pContext, IHXDataFile* pFile,
                                            CProgressiveDownloadMonitorResponse* pResponse)
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::Init() this=0x%08x\n", this);
    HX_RESULT retVal = HXR_FAIL;

    if (pContext && pFile && pResponse)
    {
        // Clear out any current state
        Close();
        // Save the context
        m_pContext = pContext;
        m_pContext->AddRef();
        // Save the file
        m_pDataFile = pFile;
        // XXXMEH - We will not AddRef the IHXDataFile for now, because
        // some users of the IHXFileObject do not call IHXFileObject::Close()
        // before releasing the file object. If that is the case, then 
        // the IHXDataFile never destructs. If we ever exhaustively go through
        // and verify that all users of smplfsys correctly call ::Close(),
        // then we can remove the commented-out AddRef().
//        m_pDataFile->AddRef();
        // Save the response pointer
        m_pResponse = pResponse;
        m_pResponse->AddRef();
        // Get the schedule interface
        retVal = m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
        if (SUCCEEDED(retVal))
        {
            // Get the IHXRegistry interface
            m_pContext->QueryInterface(IID_IHXRegistry, (void**) &m_pRegistry);
            // Check preferences to see if alternate values
            // for stat callback interval, fail callback interval
            // and finished duration are specified
            CheckPreferenceValues(m_bMonitorEnabled,
                                  m_ulStatCallbackInterval,
                                  m_ulProgCallbackInterval,
                                  m_ulFinishedTime,
                                  m_ulFormerProgRetryInit,
                                  m_ulNotProgRetryInit);
            // Initialize the progressive state variables
            m_ulLastFileSize       = GetFileSizeNow();
            m_ulTickAtLastFileSize = HX_GET_BETTERTICKCOUNT();
            // Initialize the number of times to retry
            // if the file has not been progressive
            m_ulNotProgRetryCount = m_ulNotProgRetryInit;
            // Init the registry stats
            InitRegistryStats();
        }
    }

    return retVal;
}

HX_RESULT CProgressiveDownloadMonitor::Close()
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::Close() this=0x%08x\n", this);
    HX_RESULT retVal = HXR_OK;

    // Clear any stat callback
    if (m_pStatCallback) m_pStatCallback->Cancel(m_pScheduler);
    HX_RELEASE(m_pStatCallback);
    // Clear any fail callback
    CancelCallback();
    HX_RELEASE(m_pProgCallback);
    // Release member variables
    HX_RELEASE(m_pContext);
        // XXXMEH - We will not AddRef the IHXDataFile for now, because
        // some users of the IHXFileObject do not call IHXFileObject::Close()
        // before releasing the file object. If that is the case, then 
        // the IHXDataFile never destructs. If we ever exhaustively go through
        // and verify that all users of smplfsys correctly call ::Close(),
        // then we can remove the commented-out HX_RELEASE()
//    HX_RELEASE(m_pDataFile);
    m_pDataFile = NULL;
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pScheduler);
    if (m_pRegistry)
    {
	if (m_ulURLRegistryID)
	{
	    m_pRegistry->DeleteById(m_ulURLRegistryID);
	}
	if (m_ulIsProgRegistryID)
	{
	    m_pRegistry->DeleteById(m_ulIsProgRegistryID);
	}
    }
	HX_RELEASE(m_pRegistry);
    m_ulURLRegistryID        = 0;
    m_ulIsProgRegistryID     = 0;

    // Reset state variables to defaults
    m_ulStatCallbackInterval = PROGDOWN_STAT_INTERVAL_DEFAULT;
    m_ulCurStatInterval      = PROGDOWN_STAT_INTERVAL_INITIAL;
    m_ulProgCallbackInterval = PROGDOWN_FAIL_INTERVAL_DEFAULT;
    m_ulFinishedTime         = PROGDOWN_FINISHED_TIME_DEFAULT;
    m_ulLastFileSize         = 0;
    m_ulTickAtLastFileSize   = 0;
    m_ulFormerProgRetryCount = 0;
    m_ulFormerProgRetryInit  = PROGDOWN_FORMER_PROG_RETRY_INIT;
    m_ulNotProgRetryCount    = 0;
    m_ulNotProgRetryInit     = PROGDOWN_NOT_PROG_RETRY_INIT;
    m_bIsProgressive         = FALSE;
    m_bMonitorEnabled        = TRUE;
    m_bHasBeenProgressive    = FALSE;

    return retVal;
}

HX_RESULT CProgressiveDownloadMonitor::ScheduleCallback()
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::ScheduleCallback() this=0x%08x tick=%lu\n",
            this, HX_GET_BETTERTICKCOUNT());
    HX_RESULT retVal = HXR_FAIL;

    if (m_pScheduler)
    {
        if (!m_pProgCallback)
        {
            m_pProgCallback = new CHXGenericCallback(this, CProgressiveDownloadMonitor::ProgCallback);
            if (m_pProgCallback)
            {
                m_pProgCallback->AddRef();
            }
        }
        if (m_pProgCallback)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Do we already have a callback pending?
            if (!m_pProgCallback->IsCallbackPending())
            {
                // No callback pending, schedule one
                m_pProgCallback->ScheduleRelative(m_pScheduler, m_ulProgCallbackInterval);
            }
        }
    }

    return retVal;
}

HXBOOL CProgressiveDownloadMonitor::IsCallbackPending()
{
    HXBOOL bRet = FALSE;

    if (m_pProgCallback)
    {
        bRet = m_pProgCallback->IsCallbackPending();
    }

    return bRet;
}

void CProgressiveDownloadMonitor::CancelCallback()
{
    if (m_pProgCallback && m_pProgCallback->IsCallbackPending())
    {
        m_pProgCallback->Cancel(m_pScheduler);
    }
}

HX_RESULT CProgressiveDownloadMonitor::BeginSizeMonitoring()
{
    HX_RESULT retVal = HXR_FAIL;

    // Create the stat callback if it doesn't
    // already exist
    if (!m_pStatCallback)
    {
        m_pStatCallback = new CHXGenericCallback(this, CProgressiveDownloadMonitor::StatCallback);
        if (m_pStatCallback)
        {
            m_pStatCallback->AddRef();
        }
    }
    if (m_pStatCallback)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Do we already have a callback scheduled? If so,
        // then we have already begun size monitoring.
        if (!m_pStatCallback->IsCallbackPending())
        {
            // Initialize the progressive state variables
            m_ulLastFileSize       = GetFileSizeNow();
            m_ulTickAtLastFileSize = HX_GET_BETTERTICKCOUNT();
            m_ulCurStatInterval    = PROGDOWN_STAT_INTERVAL_INITIAL;
            // Kick off the callbacks, if monitoring is enabled
            ScheduleStatCallback();
        }
    }

    return retVal;
}

void CProgressiveDownloadMonitor::EndSizeMonitoring()
{
    // If there was a size monitoring callback
    // scheduled, then cancel it
    if (m_pStatCallback)
    {
        m_pStatCallback->Cancel(m_pScheduler);
    }
}

UINT32 CProgressiveDownloadMonitor::GetFileSizeNow()
{
    UINT32 ulRet = 0;

    if (m_pDataFile)
    {
        // Stat the file
        struct stat sStat;
        if (SUCCEEDED(m_pDataFile->Stat(&sStat)))
        {
            // Set the return value
            ulRet = (UINT32) sStat.st_size;
        }
    }

    return ulRet;
}

void CProgressiveDownloadMonitor::MonitorFileSize()
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::MonitorFileSize() this=0x%08x tick=%lu\n",
            this, HX_GET_BETTERTICKCOUNT());
    // Save the current m_bIsProgressive value
    HXBOOL bTmp = m_bIsProgressive;
    // Get the file size and tick count
    UINT32 ulFileSize = GetFileSizeNow();
    UINT32 ulTick     = HX_GET_BETTERTICKCOUNT();
    // Is the file size the same as m_ulLastFileSize?
    if (ulFileSize != m_ulLastFileSize)
    {
        MLOG_PD(NULL, "\tNewFileSize=%lu\n", ulFileSize);
        // The filesize has changed, so we know
        // this file is progressive
        m_bIsProgressive = TRUE;
        // Reset the former progressive retry count to
        // its initial value
        m_ulFormerProgRetryCount = m_ulFormerProgRetryInit;
        // Reset the not progressive retry count to
        // its initial value
        m_ulNotProgRetryCount = m_ulNotProgRetryInit;
        // If the file size is different even once,
        // then this file has a history of being progressive.
        // We keep this variable around in addition to
        // m_bIsProgressive since m_bIsProgressive can change
        // when the file finishes downloading or when the
        // download agent pauses. Since we want to treat files
        // which have a history of being progressive differently
        // than files which don't, we need to keep m_bHasBeenProgressive.
        m_bHasBeenProgressive = TRUE;
        // Update the last file size
        m_ulLastFileSize = ulFileSize;
        // Update the tick count
        m_ulTickAtLastFileSize = ulTick;
    }
    else
    {
        // The filesize is the same. How
        // long has it been the same?
        UINT32 ulDiff = CALCULATE_ELAPSED_TICKS(m_ulTickAtLastFileSize, ulTick);
        // If the file size has been the
        // same for at least m_ulFinishedTime
        // milliseconds, then we assume it has
        // finished downloading
        if (ulDiff > m_ulFinishedTime && m_bIsProgressive)
        {
            MLOG_PD(NULL, "\tFile has been %lu bytes for %lu ms. DECLARING FINISHED.\n",
                    ulFileSize, ulDiff);
            m_bIsProgressive = FALSE;
        }
    }
    // Did the value of m_bIsProgressive change?
    if (bTmp != m_bIsProgressive)
    {
        // Update the registry statistics
        UpdateRegistryStats();
    }
}

void CProgressiveDownloadMonitor::StatCallback(void* pArg)
{
    if (pArg)
    {
        CProgressiveDownloadMonitor* pObj = (CProgressiveDownloadMonitor*) pArg;
        pObj->MonitorFileSize();
        if (pObj->m_pResponse)
        {
            pObj->m_pResponse->ProgressiveStatCallback();
        }
        pObj->ScheduleStatCallback();
    }
}

void CProgressiveDownloadMonitor::ProgCallback(void* pArg)
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::ProgCallback(0x%08x) tick=%lu\n",
            pArg, HX_GET_BETTERTICKCOUNT());
    if (pArg)
    {
        CProgressiveDownloadMonitor* pObj = (CProgressiveDownloadMonitor*) pArg;
        if (pObj->m_pResponse)
        {
            pObj->m_pResponse->ProgressiveCallback();
        }
    }
}

void CProgressiveDownloadMonitor::CheckPreferenceValues(REF(HXBOOL)   rbMonitorEnabled,
                                                        REF(UINT32) rulStatCallbackInterval,
                                                        REF(UINT32) rulProgCallbackInterval,
                                                        REF(UINT32) rulFinishedTime,
                                                        REF(UINT32) rulFormerProgRetryCount,
                                                        REF(UINT32) rulNotProgRetryCount)
{
    if (m_pContext)
    {
        IHXPreferences* pPrefs = NULL;
        m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
        if (pPrefs)
        {
            ReadPrefBOOL(pPrefs,  "ProgressiveDownload\\FileSizeMonitorEnabled",      rbMonitorEnabled);
            ReadPrefUINT32(pPrefs, "ProgressiveDownload\\FileSizeCheckInterval",       rulStatCallbackInterval);
            ReadPrefUINT32(pPrefs, "ProgressiveDownload\\FailureRetryInterval",        rulProgCallbackInterval);
            ReadPrefUINT32(pPrefs, "ProgressiveDownload\\DeclareFinishedDuration",     rulFinishedTime);
            ReadPrefUINT32(pPrefs, "ProgressiveDownload\\FormerProgressiveRetryCount", rulFormerProgRetryCount);
            ReadPrefUINT32(pPrefs, "ProgressiveDownload\\NotProgressiveRetryCount",    rulNotProgRetryCount);
            MLOG_PD(NULL,
                    "\tFileSizeMonitorEnabled      = %lu\n"
                    "\tFileSizeCheckInterval       = %lu\n"
                    "\tFailureRetryInterval        = %lu\n"
                    "\tDeclareFinishedDuration     = %lu\n"
                    "\tFormerProgressiveRetryCount = %lu\n"
                    "\tNotProgressiveRetryCount    = %lu\n",
                    rbMonitorEnabled, rulStatCallbackInterval,
                    rulProgCallbackInterval, rulFinishedTime,
                    rulFormerProgRetryCount, rulNotProgRetryCount);
        }
        HX_RELEASE(pPrefs);
    }
}

HX_RESULT CProgressiveDownloadMonitor::InitRegistryStats()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pContext && m_pRegistry && m_pDataFile)
    {
        // Get the source registry ID
        IHXRegistryID* pRegistryID = NULL;
        retVal = m_pContext->QueryInterface(IID_IHXRegistryID, (void**) &pRegistryID);
        if (SUCCEEDED(retVal))
        {
            UINT32 ulSourceRegistryID = 0;
            retVal = pRegistryID->GetID(ulSourceRegistryID);
            if (SUCCEEDED(retVal))
            {
                // Get the source property name - this will
                // be something like "Statistics.Player0.Source0"
                IHXBuffer* pSourceName = NULL;
                retVal = m_pRegistry->GetPropName(ulSourceRegistryID, pSourceName);
                if (SUCCEEDED(retVal))
                {
                    // Construct the URL property name
                    CHXString cPropName((const char*) pSourceName->GetBuffer());
                    cPropName += ".URL";
                    // Construct the URL property value
                    CHXString cPropVal("file://");
                    IHXBuffer* pNameBuf = NULL;
                    if (m_pDataFile->Name(pNameBuf))
                    {
                        cPropVal += (const char*) pNameBuf->GetBuffer();
                    }
                    HX_RELEASE(pNameBuf);
                    // Put the URL property value into an IHXBuffer
                    IHXBuffer* pPropVal = NULL;
                    CreateStringBufferCCF(pPropVal, (const char*) cPropVal, m_pContext);
                    if (pPropVal)
                    {
                        // Does this property already exist?
                        IHXBuffer* pTmp = NULL;
                        if (SUCCEEDED(m_pRegistry->GetStrByName((const char*) cPropName, pTmp)))
                        {
                            // It does already exist
                            //
                            // Set the new value
                            m_pRegistry->SetStrByName((const char*) cPropName, pPropVal);
                            // Get the registry ID
                            m_ulURLRegistryID = m_pRegistry->GetId((const char*) cPropName);
                        }
                        else
                        {
                            // It does not already exist, so create it
                            // and get the registry id at the same time
                            m_ulURLRegistryID = m_pRegistry->AddStr((const char*) cPropName, pPropVal);
                        }
                        HX_RELEASE(pTmp);
                    }
                    HX_RELEASE(pPropVal);
                    // Construct the IsProgressive property name
                    cPropName  = (const char*) pSourceName->GetBuffer();
                    cPropName += ".IsProgressive";
                    // Does this property already exist?
                    INT32 lTmp = 0;
                    if (SUCCEEDED(m_pRegistry->GetIntByName((const char*) cPropName, lTmp)))
                    {
                        // It DOES already exist
                        //
                        // Set the new value
                        m_pRegistry->SetIntByName((const char*) cPropName, (m_bIsProgressive ? 1 : 0));
                        // Get the registry id
                        m_ulIsProgRegistryID = m_pRegistry->GetId((const char*) cPropName);
                    }
                    else
                    {
                        // It does NOT already exist
                        //
                        // Create the IsProgressive property and get
                        // the registry id at the same time
                        m_ulIsProgRegistryID = m_pRegistry->AddInt((const char*) cPropName,
                                                                   (m_bIsProgressive ? 1 : 0));
                    }
                }
                HX_RELEASE(pSourceName);
            }
        }
        HX_RELEASE(pRegistryID);
    }

    return retVal;
}

void CProgressiveDownloadMonitor::UpdateRegistryStats()
{
    MLOG_PD(NULL, "CProgressiveDownloadMonitor::UpdateRegistryStats() this=0x%08x\n", this);
    // Right now the only thing we are updating
    // is the IsProgressive property. We assume
    // that the URL property will never need to
    // change during playback.
    if (m_pRegistry && m_ulIsProgRegistryID)
    {
        m_pRegistry->SetIntById(m_ulIsProgRegistryID, (m_bIsProgressive ? 1 : 0));
    }
}

void CProgressiveDownloadMonitor::ScheduleStatCallback()
{
    if (m_pStatCallback && m_bMonitorEnabled)
    {
        MLOG_PD(NULL, "CProgressiveDownloadMonitor::ScheduleStatCallback() this=0x%08x m_ulCurStatInterval=%lu\n",
                this, m_ulCurStatInterval);
        m_pStatCallback->ScheduleRelative(m_pScheduler, m_ulCurStatInterval);
        // We start off with a small stat interval
        // and double the interval every time until
        // we reach m_ulStatCallbackInterval
        if (m_ulCurStatInterval < m_ulStatCallbackInterval)
        {
            m_ulCurStatInterval *= 2;
            if (m_ulCurStatInterval > m_ulStatCallbackInterval)
            {
                m_ulCurStatInterval = m_ulStatCallbackInterval;
            }
        }
    }
}

