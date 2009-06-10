/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: reglog.cpp,v 1.6 2005/07/20 21:46:59 dcollins Exp $ 
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/signal.h"
#include "hlxclib/time.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxrendr.h"
#include "hxmon.h"
#include "ihxpckts.h"
#include "hxiids.h"
#include "hxerror.h"
#include "hxcfg.h"
#include "hxstats.h"
#include "cpqueue.h"
#include "hxstrutl.h"   //SafeSprintf
#include "random32.h"
#include "reglog.h"

#include "debug.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

#define D_LOGGER_ENTRY 0x00100000
#define D_LOGGER_INFO  0x00200000

//int nDebugLevel = D_LOGGER_ENTRY|D_LOGGER_INFO;

//#undef DPRINTF
//#define	DPRINTF(mask,x)	if (nDebugLevel & (mask)) dprintf x; else

CRegLogger::CRegLogger(IUnknown* pContext, const char* pszLogRootRegKey, 
        BOOL bCleanEntries /* [optional] = TRUE */,
        UINT32 nCleanDelay /* [optional] = 5000 ms. */ )
    :m_lRefCount(0)
    ,m_pContext(pContext)
    ,m_pRegistry(NULL)
    ,m_pCommonClassFactory(NULL)
    ,m_pLogRootRegKey(NULL)
    ,m_hCallback(0)
    ,m_pIdQ(NULL)
    ,m_ulCounter(0)
    ,m_bCleanEntries(bCleanEntries)
    ,m_nCleanDelay(nCleanDelay)
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::CRegLogger()\n"));

    if (m_pContext)
    {
    	m_pContext->AddRef();

        HX_RESULT res = m_pContext->QueryInterface(IID_IHXRegistry, 
				         (void**)&m_pRegistry);
        
        res = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				         (void**)&m_pCommonClassFactory);
    }
    
    m_ulCounter = random32((UINT32)(PTR_INT)this) % 10000;
    
    HX_ASSERT(pszLogRootRegKey && *pszLogRootRegKey);
    m_pLogRootRegKey = MakeBuffer(pszLogRootRegKey);
}

CRegLogger::~CRegLogger()
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::~RegLoggers()\n"));

    if (m_hCallback)
    {
	    IHXScheduler* pSched = NULL;
	    if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXScheduler, 
                                                        (void**)&pSched)))
        {
            pSched->Remove(m_hCallback);
        }
	    HX_RELEASE(pSched);
    }
    
    HX_RELEASE(m_pLogRootRegKey);
    HX_DELETE(m_pIdQ);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pCommonClassFactory);
}

// *** IUnknown methods ***
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP 
CRegLogger::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	    AddRef();
	    *ppvObj = this;
	    return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
	    AddRef();
	    *ppvObj = (IHXCallback*)this;
	    return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(UINT32) 
CRegLogger::AddRef()
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::AddRef()\n"));
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(UINT32)
CRegLogger::Release()
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::Release()\n"));
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CRegLogger::LogThis(IHXBuffer* pLogMsg)
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::UpdateRegistryLog()\n"));

    HX_ASSERT(m_pRegistry);
    HX_ASSERT(pLogMsg);
    
    if (!(pLogMsg && pLogMsg->GetSize()))
    {
        return HXR_OK;
    }
    
    char* szRegKey = NULL;
    szRegKey = new char[20 + strlen((const char*)m_pLogRootRegKey->GetBuffer())];

    UINT32 nLoopCount = 0;
    HX_RESULT res = HXR_FAILED;
    while (FAILED(res))
    {
        if (++nLoopCount > 100)
        {
            DPRINTF(D_LOGGER_INFO, ("Can't AddStr after %ld tries\n", nLoopCount));
            break;
        }

        sprintf(szRegKey, "%s.%d.Entry", 
            (const char*)m_pLogRootRegKey->GetBuffer(), ++m_ulCounter);
            
        if (FAILED(res = m_pRegistry->AddStr(szRegKey, pLogMsg)))
        {
            DPRINTF(D_LOGGER_INFO, ("AddStr collision\n", nLoopCount));

            // some other CRegLogger's ulCounter is in the same range so
            // skip ahead a random amount 
            m_ulCounter += (random32(0) % 100);
        }
    }
    
    if (SUCCEEDED(res))
    {
        DPRINTF(D_LOGGER_INFO, ("Wrote %ld bytes to '%s'\n%s\n", 
            pLogMsg->GetSize(), szRegKey, (const char*)pLogMsg->GetBuffer()));

        if (GetIsCleanEntriesEnabled())
        {
            if (!m_pIdQ)
            {
                m_pIdQ = new CPtrQueue(100);
            }
            if (!m_pIdQ)
            {
                DPRINTF(D_LOGGER_ENTRY, ("Couldn't Create Id Queue\n"));
                return HXR_OUTOFMEMORY;
            }
        
            sprintf(szRegKey, "%s.%d", 
                (const char*)m_pLogRootRegKey->GetBuffer(), m_ulCounter);

            UINT32 nRegId = m_pRegistry->GetId(szRegKey);
            if (nRegId)
            {
                DPRINTF(D_LOGGER_INFO, ("Queuing RegId %ld\n", nRegId));
            
                m_pIdQ->EnQueuePtr((void *)nRegId);
                SetTimer(); //to clean nRegId
            }
        }
    }

    return res;
}

STDMETHODIMP
CRegLogger::SetTimer()
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::SetTimer()\n"));

    HX_RESULT res = HXR_OK;

    if (m_hCallback)
    {
        DPRINTF(D_LOGGER_INFO, ("Timer already set\n"));
    }
    else
    {
	    IHXScheduler* pSched = NULL;
	    if (SUCCEEDED(res = m_pContext->QueryInterface(IID_IHXScheduler, 
                                                        (void**)&pSched)))
        {
            if (SUCCEEDED(res = pSched->RelativeEnter(this, GetCleanDelay())))
            {
                m_hCallback = res;
            }
        }
	    HX_RELEASE(pSched);
    }
        
    return res;
}

/************************************************************************
 *	Method:
 *	    IHXCallback::Func
 *	Purpose:
 *	    This is the function that will be called when a callback is
 *	    to be executed.
 * 
 *      Deletes the next registry property pulled of the RegId queue, and
 *      fires another timer if there is anything left in the queue.
 */
STDMETHODIMP
CRegLogger::Func()
{
    DPRINTF(D_LOGGER_ENTRY, ("CRegLogger::Func()\n"));

    m_hCallback = 0;
    
    if (m_pIdQ && m_pIdQ->GetQueuedItemCount())
    {
        BOOL bValid = FALSE;
        UINT32 nRegId = (UINT32)(PTR_INT)m_pIdQ->DeQueuePtr(bValid);
        if (nRegId && bValid)
        {
            DPRINTF(D_LOGGER_INFO, ("Dequeued RegId %ld\n", nRegId));
            m_pRegistry->DeleteById(nRegId);
            if (m_pIdQ->GetQueuedItemCount())
            {
                SetTimer();
            }
            else
            {
                DPRINTF(D_LOGGER_INFO, ("Queue is now empty\n"));
            }
        }
    }
    
    return HXR_OK;
}

STDMETHODIMP_(BOOL)
CRegLogger::GetIsCleanEntriesEnabled()
{
    return m_bCleanEntries;
}

/* how long should the logger wait before deleting the next log entry 
    from the registry - should be long enough for the templatized logging
    system to have send the entry to its output destination(s).
*/
STDMETHODIMP_(UINT32)
CRegLogger::GetCleanDelay()
{
    return m_nCleanDelay;
}

IHXBuffer*
CRegLogger::MakeBuffer(const char* str)
{
    IHXBuffer* pBuffer = NULL;
    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
		(void**)&pBuffer)))
	{
	    pBuffer->Set((const unsigned char*)str, strlen(str) + 1);
	}
    return pBuffer;
}

STDMETHODIMP_(IHXBuffer*)
CRegLogger::MakeBuffer(UINT32 nSize)
{
    IHXBuffer* pBuffer = NULL;
    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
		(void**)&pBuffer)))
	{
    	pBuffer->SetSize(nSize);
	}
    return pBuffer;
}


/*
    subclass of RegLogger using configuration parameters
    from a hardwired list in the config file,
    also adds:
    - a helper function to check if configuartion 
    logging is enabled
    
    - a method to construct a log entry based on the common
    log format
*/
CConfigLogger::CConfigLogger(IUnknown* pContext)
    :CRegLogger(pContext, CONFIGLOG_LOG_ROOTREGKEY)
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::CConfigLogger()\n"));
}

CConfigLogger::~CConfigLogger()
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::~CConfigLogger()\n"));
}

BOOL 
CConfigLogger::IsLoggingEnabled(IHXRegistry* pRegistry)
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::IsLoggingEnabled()\n"));

    HX_ASSERT(pRegistry);
    
    //default is FALSE if Enabled var is not found
    INT32 nEnabled = 0;
    pRegistry->GetIntByName(CONFIGLOG_CONFIG_ENABLED_REGKEY, nEnabled);
    
    return (nEnabled != 0) ? TRUE : FALSE;
}

STDMETHODIMP_(BOOL)
CConfigLogger::GetIsCleanEntriesEnabled()
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::GetIsCleanEntriesEnabled()\n"));

    INT32 nClean = 0;
    if (SUCCEEDED(m_pRegistry->GetIntByName(CONFIGLOG_CONFIG_CLEAN_REGKEY, nClean)))
    {
        return (BOOL)nClean;
    }
    else
    {
        //not in the registry? then init with the default value
        return m_bCleanEntries;
    }
}

STDMETHODIMP_(UINT32)
CConfigLogger::GetCleanDelay()
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::GetCleanDelay()\n"));

    INT32 nDelay = 0;
    if (SUCCEEDED(m_pRegistry->GetIntByName(CONFIGLOG_CONFIG_CLEANDELAY_REGKEY, 
                nDelay)) && (nDelay >= REGLOG_CLEANDELAY_MIN))
    {
        return (UINT32)nDelay;
    }
    else
    {
        //not in the registry? then init with the default value
        return m_nCleanDelay;
    }
}


STDMETHODIMP
CConfigLogger::LogConfigClient(IHXRequest* pRequest, const char* pszUserId, 
                          const char* pszLogMsg)
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::LogAdminClient()\n"));

    HX_RESULT res = HXR_FAIL;
    
	IHXBuffer* pBufEntry = NULL;
	res = MakeLogEntry(pRequest, pszUserId, pszLogMsg, pBufEntry);
    if (pBufEntry)
    {
        res = LogThis(pBufEntry);
    }    
    HX_RELEASE(pBufEntry);
    
    return res;
}


/*
    creates a common log format-like log entry using the pRequest and szUserId
    parameters, and then appends pzsLogMsg to it (pszLogMsg should summarrize 
    the configuration changes that were made). The resulting string is placed
    in the pBufEntry output parameter.
*/
STDMETHODIMP
CConfigLogger::MakeLogEntry(IHXRequest* pRequest, const char* pszUserId, 
                              const char* pszLogMsg, REF(IHXBuffer*) pBufEntry)
{
    DPRINTF(D_LOGGER_ENTRY, ("CConfigLogger::MakeLogEntry()\n"));

    HX_RESULT res = HXR_OK;
    
	HX_RELEASE(pBufEntry);

    IHXClientStatsManager*	pClientStatsMgr	= NULL;
    IHXClientStats* 		pClientStats    = NULL;
    IHXBuffer* 				pHostAddr		= NULL;
    IHXBuffer* 				pAddr			= NULL;
    IHXBuffer* 				pProtocol		= NULL;
    IHXBuffer* 				pHTTPMethod     = NULL;
    IHXBuffer* 				pVersion	    = NULL;
    IHXBuffer* 				pUserAgent		= NULL;
    IHXBuffer* 				pURL    		= NULL;
    IHXBuffer* 				pTemp           = NULL;

    HX_VERIFY(SUCCEEDED(res = m_pContext->QueryInterface(IID_IHXClientStatsManager, 
    						(void**)&pClientStatsMgr)));
	if (!pClientStatsMgr)
	{
		return res;
	}

	IHXValues* pReqHeaders = NULL;
	if (SUCCEEDED(res = pRequest->GetRequestHeaders(pReqHeaders)))
	{
	    IHXBuffer* pConnId = NULL;
    	if (SUCCEEDED(res = pReqHeaders->GetPropertyCString("ClientStatsObjId", pConnId)))
		{
		    HX_VERIFY(pClientStats = pClientStatsMgr->GetClient(
                                atoi((const char*)pConnId->GetBuffer())));
			if (!pClientStats)
			{
				res = HXR_FAIL;
			}
		}
		HX_RELEASE(pConnId);
	}
	HX_RELEASE(pReqHeaders);

	if (!pClientStats)
	{
		return res;
	}

    pTemp = pClientStats->GetIPAddress();  
    pAddr = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    char		current_time_string[80];
    time_t		current_time;
    struct tm 	current_time_struct;
    LONG32		time_zone=0;
    char		sign=0;

    time(&current_time);
    hx_localtime_r(&current_time, &current_time_struct);
#if defined(__linux) || defined(WIN32) || defined __sgi || defined __sun || defined(_HPUX) || defined(_AIX)
    /*
    * From what I can see, timezone is the number of seconds it will take to
    * get to GMT which is backwards from FreeBSD
    */
    time_zone = -timezone;
    time_zone += (3600 * current_time_struct.tm_isdst);
#elif !defined __hpux && !defined _AIX
        time_zone = current_time_struct.tm_gmtoff;
#endif /* __linux */

    if (time_zone < 0)
    {
        time_zone = -time_zone;
        sign = '-';
    }
    else
    {
        sign = '+';
    }
    strftime(current_time_string, 80, "%d/%b/%Y:%H:%M:%S",
        &current_time_struct);

    pTemp = pClientStats->GetProtocol();
    pProtocol = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    pTemp = pClientStats->GetVersion();
    pVersion = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    pTemp = pClientStats->GetUserAgent();
    pUserAgent = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    IHXSessionStats* pSessionStats = pClientStats->GetSession(1);
    if (!pSessionStats)
    {
        HX_ASSERT(!"Admin Logger: No session stats object found!");
		pTemp = NULL;
    }
	else
	{
	    pTemp = pSessionStats->GetLogURL();
	    if (!pTemp)
	    {
	        pTemp = pSessionStats->GetURL();
	    }
	}
    pURL = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    pTemp = pSessionStats->GetInterfaceAddr();  
    pHostAddr = pTemp ? pTemp : MakeBuffer("UNKNOWN");

    HX_RELEASE(pSessionStats);

    pTemp = pClientStats->GetRequestMethod();
    pHTTPMethod = pTemp ? pTemp : MakeBuffer("GET");

// example of what we're trying to construct:
//127.0.0.1 172.23.110.110 - MyServer.AdminRealm/AdminUser [27/Jun/2003:12:57:37 -0700]  "POST admin/servvar.set.html HTTP/1.0" - - [Mozilla/4.0 (compatible;MSIE 6.0;Windows NT 5.1)]
	
	UINT32 nBufLen = 80; //formatting overhead
	nBufLen += strlen((char*)pHostAddr->GetBuffer()) +
        strlen((char*)pAddr->GetBuffer()) +
	    strlen(pszUserId) +
        strlen(current_time_string) +
        strlen((char*)pHTTPMethod->GetBuffer()) +
        strlen((char*)pURL->GetBuffer()) +
        strlen((char*)pProtocol->GetBuffer()) +
        strlen((char*)pVersion->GetBuffer()) +
        strlen((char*)pUserAgent->GetBuffer()) +
        strlen(pszLogMsg);
        
	pBufEntry = MakeBuffer(nBufLen);
	if (pBufEntry)
	{
		nBufLen = SafeSprintf((char*)pBufEntry->GetBuffer(), nBufLen, 
			"%s %s - %s [%s %c%02ld%02ld] \"%s %s %s/%s\" - - [%s]\n%s",
	        (char*)pHostAddr->GetBuffer(), 
	        (char*)pAddr->GetBuffer(), 
		    pszUserId,
	        current_time_string,
	        sign,
	        time_zone/3600,
	        time_zone %3600,
	        (char*)pHTTPMethod->GetBuffer(),
	        (char*)pURL->GetBuffer(),
	        (char*)pProtocol->GetBuffer(),
	        (char*)pVersion->GetBuffer(),
	        (char*)pUserAgent->GetBuffer(),
            pszLogMsg);

        pBufEntry->SetSize(nBufLen + 1);
	}
    //printf("LOG ENTRY:\n%s", (const char*)pBufEntry->GetBuffer());

    HX_RELEASE(pURL);
    HX_RELEASE(pHostAddr);
    HX_RELEASE(pAddr);
    HX_RELEASE(pProtocol);
    HX_RELEASE(pVersion);
    HX_RELEASE(pHTTPMethod);
    HX_RELEASE(pUserAgent);
    HX_RELEASE(pClientStats);
    HX_RELEASE(pClientStatsMgr);

    return res;
}
