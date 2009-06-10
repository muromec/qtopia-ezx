/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rssmgr.cpp,v 1.6 2006/03/22 19:45:37 tknox Exp $
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

#include "hxcomm.h"
#include "servlist.h"
#include "hxpropw.h"
#include "server_context.h"
#include "hxassert.h"
#include "base_callback.h"
#include "imutex.h"
#include "mutexlist.h"
#include "hxrssmgr.h"
#include "logoutputs.h"
#include "proc_container.h"

#include "rssmgr.h"

// Default RSS Log values...
#define DEFAULT_RSSLOGPRUNETHRESH (1024 * 100)   // 100 megs
#define DEFAULT_ROLL_TYPE         ABSOLUTE_TIME  // Daily (absolute time)
#define DEFAULT_ABSOLUTE_ROLLTIME "00:00"        // midnight
#define DEFAULT_ROLL_FILE_SIZE    (1024 * 50)    // 50 megs
#define DEFAULT_RELATIVE_ROLLTIME (60 * 24)      // 1 day
#define DEFAULT_LOG_FILENAME      "rsslogs"

RSSMReportElem::RSSMReportElem(IHXRSSReport* pStatObj)
    : HXListElem()
    , m_pStatObj(pStatObj)
{
    HX_ASSERT(pStatObj);
    pStatObj->AddRef();
}

RSSMReportElem::~RSSMReportElem()
{
    HX_RELEASE(m_pStatObj);
}


/////////////////////////////////////////////////////////////////////////
//  Constructor
//
RSSManager::RSSManager(Process* pProc)
    : m_pProc(pProc)
    , m_ulRefCount(0)
    , m_pStatsList(NULL)
{
    HX_ASSERT(m_pProc);

    m_pStatsList = new HXList;

    // Get the registry and scheduler...
    m_pProc->pc->server_context->QueryInterface(IID_IHXRegistry,  (void**)&m_pRegistry);
    m_pProc->pc->server_context->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);

    HX_ASSERT(m_pStatsList);
    HX_ASSERT(m_pRegistry);
    HX_ASSERT(m_pScheduler);

    // Initialize m_pOutput, and set with appropriate config values
    InitOutput();

    // Schedule first report and watches...
    m_pRegistry->CreatePropWatch(m_pPropWatch);
    
    HX_ASSERT(m_pPropWatch);

    m_pPropWatch->Init(this);

    m_nRegIds[REGID_RSSINTERVAL] = m_pPropWatch->SetWatchByName(REGENTRY_RSSINTERVAL);
    m_nRegIds[REGID_RSSLOGFILENAME]  = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGFILENAME);
    m_nRegIds[REGID_RSSLOGDIRECTORY] = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGDIRECTORY);
    m_nRegIds[REGID_RSSLOGROLLTIME]  = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGROLLTIME);
    m_nRegIds[REGID_RSSLOGROLLOFFSET] = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGROLLOFFSET);
    m_nRegIds[REGID_RSSLOGROLLSIZE] = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGROLLSIZE);
    m_nRegIds[REGID_RSSLOGPRUNETHRESH] = m_pPropWatch->SetWatchByName(REGENTRY_RSSLOGPRUNETHRESH);

    m_pScheduler->RelativeEnter(this, m_ulRSSReportInterval * 1000);
}

/////////////////////////////////////////////////////////////////////////
//  Destructor
//
RSSManager::~RSSManager()
{
    HX_DELETE(m_pStatsList);
}

/////////////////////////////////////////////////////////////////////////
//  RSSManager::InitOutput()
//
//  Initiallize m_pOutput with config values
void
RSSManager::InitOutput()
{
    // Fetch init values from the config file
    IHXBuffer *pBuf = NULL,
              *pRollTime = NULL;
    char *logfilename = NULL;
    char *logpath = NULL;
    INT32 nRollType = DEFAULT_ROLL_TYPE,
          nPruneThreshold,
          nRSSInterval;

    // Fetch the report interval from the config,
    // if not enabled via --rss
    if (!m_ulRSSReportInterval)
    {
        m_ulRSSReportInterval =
                SUCCEEDED(m_pRegistry->GetIntByName(REGENTRY_RSSINTERVAL, nRSSInterval)) ?
                    (UINT32)nRSSInterval :
                    DEFAULT_RSS_INTERVAL;
    }

    // Fetch RSS Roll Type...    
    m_pRegistry->GetIntByName(REGENTRY_RSSROLLTYPE, nRollType);

    // Fetch RSS Log filename...
    m_pRegistry->GetStrByName(REGENTRY_RSSLOGFILENAME, pBuf);
    if (pBuf)
    {
        logfilename = new char[strlen((const char *)pBuf->GetBuffer())+1];
        strcpy(logfilename, (const char *)pBuf->GetBuffer());
    }
    else
    {
        logfilename = new char[strlen(DEFAULT_LOG_FILENAME)+1];
        strcpy(logfilename, DEFAULT_LOG_FILENAME);
    }

    // Fetch RSS log directory,  like so...    
    m_pRegistry->GetStrByName(REGENTRY_RSSLOGDIRECTORY, pBuf);
    if (pBuf)
    {
        logpath = new char[strlen((const char *)pBuf->GetBuffer())+1];
        strcpy(logpath, (const char *)pBuf->GetBuffer());
    }
 
    // Create a LogFileOutput Object...
    m_pOutput = new LogFileOutput(logfilename, logpath, m_pScheduler, (IUnknown*) m_pProc->pc->server_context);

    switch (nRollType)
    {
    case ABSOLUTE_TIME:
        if (SUCCEEDED(m_pRegistry->GetStrByName(REGENTRY_RSSLOGROLLTIME, pBuf)))
        {
            m_pOutput->SetRollTimeAbs((const char*)pBuf->GetBuffer());
            break;
        }
        // Use default...
        m_pOutput->SetRollTimeAbs(DEFAULT_ABSOLUTE_ROLLTIME);
        break;
    case RELATIVE_TIME:
        INT32 nOffset;
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGROLLOFFSET, nOffset)))
        {
            m_pOutput->SetRollTimeOffset(nOffset);
            break;
        }
        m_pOutput->SetRollTimeOffset(DEFAULT_RELATIVE_ROLLTIME);
        break;
    case FILE_SIZE:
        INT32 nRollSize;
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGROLLSIZE, nRollSize)))
        {
            m_pOutput->SetRollSize(nRollSize);
            break;
        }
        m_pOutput->SetRollSize(DEFAULT_ROLL_FILE_SIZE);
        break;
    case NO_ROLL:
        m_pOutput->SetNoRoll();
        break;
    default:
        m_pOutput->SetRollTimeAbs(DEFAULT_ABSOLUTE_ROLLTIME);
        break;
    }

    // Fetch RSS prune threshold
    if (SUCCEEDED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGPRUNETHRESH, nPruneThreshold)))
    {
        m_pOutput->SetPruneThreshold(nPruneThreshold);
    }
    else
    {
        m_pOutput->SetPruneThreshold(DEFAULT_RSSLOGPRUNETHRESH);
    }

    m_pProc->pc->managed_mutex_list->AddMutex(m_pOutput->GetMutex());
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXRSSManager::QueryInterface
//
STDMETHODIMP
RSSManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
RSSManager::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
RSSManager::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      RSSManager::Register
//  Purpose:
//      Adds an IHXRSSReport object to the list
//
HX_RESULT
RSSManager::Register(IHXRSSReport* pStatObj)
{
    if (!m_pStatsList)
    {
        return HXR_FAILED;
    }

    RSSMReportElem* pRSSMRE = new RSSMReportElem(pStatObj);
    m_pStatsList->insert(pRSSMRE);

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      RSSManager::Remove
//  Purpose:
//      Used to remove a RSSReport object from the manager list
//
HX_RESULT
RSSManager::Remove(IHXRSSReport* pStatObj)
{
    HXList_iterator i(m_pStatsList);
    RSSMReportElem* pTmp = 0;

    // Look through all the stats objects to see if we
    // can find the one that needs to be removed
    for (; *i; ++i)
    {
        pTmp = (RSSMReportElem *)(*i);
        if (pStatObj == pTmp->m_pStatObj)
        {
            m_pStatsList->remove(pTmp);
            HX_DELETE(pTmp);
            return HXR_OK;
        }
    }

    return HXR_FAILED;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      BaseCallback::Func
//  Purpose:
//      To call the Report method on all register IHXRSSReport objects
//
STDMETHODIMP
RSSManager::Func()
{   
    HXList_iterator i(m_pStatsList);

    m_pScheduler->RelativeEnter(this, m_ulRSSReportInterval * 1000);

    if (m_pStatsList)
    {
	RSSMReportElem* pTmp = 0;

	for (; *i; ++i)
        {
	    pTmp = (RSSMReportElem *)(*i);
            pTmp->m_pStatObj->Report((IHXLogFileOutput*)m_pOutput,
		m_bRSSReportEnabled, m_pScheduler->GetCurrentSchedulerTime());
        }
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXPropWatchResponse::ModifiedProp
//  Purpose:
//      Keep track of config changes...
//
STDMETHODIMP
RSSManager::ModifiedProp(const UINT32 id,
                         const HXPropType propType,
                         const UINT32 ulParentID)
{
    INT32 nInterval;
    IHXBuffer *pBuf = NULL;


    // RSS Interval
    if (id == m_nRegIds[REGID_RSSINTERVAL])
    {
        if (FAILED(m_pRegistry->GetIntByName(REGENTRY_RSSINTERVAL, nInterval)))
        {
            nInterval = DEFAULT_RSS_INTERVAL;
        }
        if (nInterval < 1)
        {
            nInterval = DEFAULT_RSS_INTERVAL;
        }

        m_ulRSSReportInterval = (UINT32)nInterval;
    }
    // RSS Base Log Filename
    else if (id == m_nRegIds[REGID_RSSLOGFILENAME])
    {
        if (FAILED(m_pRegistry->GetStrByName(REGENTRY_RSSLOGFILENAME, pBuf)))
        {
            return HXR_FAILED;
        }

        if (pBuf)
        {
           HX_RESULT theErr = m_pOutput->SetBaseFilename((const char*)pBuf->GetBuffer());
        }
    }
    // RSS Log Directory
    else if (id == m_nRegIds[REGID_RSSLOGDIRECTORY])
    {
        if (FAILED(m_pRegistry->GetStrByName(REGENTRY_RSSLOGDIRECTORY, pBuf)))
        {
            return HXR_FAILED;
        }

        if (pBuf)
        {
            m_pOutput->SetLogDirectory((const char*)pBuf->GetBuffer());
        }
    }
    // RSS Log Roll Time
    else if (id == m_nRegIds[REGID_RSSLOGROLLTIME])
    {
        if (FAILED(m_pRegistry->GetStrByName(REGENTRY_RSSLOGROLLTIME, pBuf)))
        {
            return HXR_FAILED;
        }

        if (pBuf)
        {
           m_pOutput->SetRollTimeAbs((const char*)pBuf->GetBuffer());
        }
    }
    // RSS Log Roll Offset
    else if (id == m_nRegIds[REGID_RSSLOGROLLOFFSET])
    {
        INT32 nRollTimeOffset;
        if (FAILED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGROLLOFFSET, nRollTimeOffset)))
        {
            return HXR_FAILED;
        }

        m_pOutput->SetRollTimeOffset((UINT32)nRollTimeOffset);
    }    
    // RSS Log Roll Size
    else if (id == m_nRegIds[REGID_RSSLOGROLLSIZE])
    {
        INT32 nRollSize;
        if (FAILED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGROLLSIZE, nRollSize)))
        {
            return HXR_FAILED;
        }

        m_pOutput->SetRollSize((UINT32)nRollSize);
    }  
    // RSS Prune Threshold
    else if (id == m_nRegIds[REGID_RSSLOGPRUNETHRESH])
    {
        INT32 nPruneThreshold;
        if (FAILED(m_pRegistry->GetIntByName(REGENTRY_RSSLOGPRUNETHRESH, nPruneThreshold)))
        {
            return HXR_FAILED;
        }

        m_pOutput->SetPruneThreshold((UINT32)nPruneThreshold);
    }

    return HXR_OK;
}

