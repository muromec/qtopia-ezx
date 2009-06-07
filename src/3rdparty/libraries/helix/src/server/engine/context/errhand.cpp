/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: errhand.cpp,v 1.10 2005/07/20 21:47:37 dcollins Exp $ 
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

#ifdef _UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#endif // UNIX

#include "hlxclib/stdio.h"
#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "dispatchq.h"
#include "error_sink_handler.h"
#include "simple_callback.h"
#include "errhand.h"
#include "hxslist.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "servreg.h"
#include "timeval.h"
#include "hxtime.h"
#include "hxproc.h"
#include "servsked.h"
#include "servbuffer.h"
#include "server_version.h"

extern BOOL g_bShowDebugErrorMessages;

const char zpSeverityTable[] =
{
     '!', // HXLOG_EMERG
     'A', // HXLOG_ALERT
     'C', // HXLOG_CRIT
     'E', // HXLOG_ERR
     'W', // HXLOG_WARNING
     'N', // HXLOG_NOTICE
     'I', // HXLOG_INFO
     'D'  // HXLOG_DEBUG
};

ErrorHandler::ErrorHandler(Process* pProc):
    m_lRefCount(0),
    m_pProc(pProc)
{
}

ErrorHandler::~ErrorHandler()
{
}

/* IHXErrorMessages methods */

STDMETHODIMP
ErrorHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXErrorMessages))
    {
	AddRef();
	*ppvObj = (IHXErrorMessages*)this;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ErrorHandler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
ErrorHandler::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(IHXBuffer*) 
ErrorHandler::GetErrorText(HX_RESULT ulHXCode)
{
    IHXBuffer* pBuffer = NULL;
    return pBuffer; 
} 

STDMETHODIMP
ErrorHandler::Report(const UINT8     unSeverity,
                     HX_RESULT	     ulHXCode,
                     const ULONG32   ulUserCode,
                     const char*     pUserString,
                     const char*     pMoreInfoURL)
{
    IHXBuffer*  	pBufUserString = 0;
    IHXBuffer*  	pBufMoreInfoURL = 0;
    ErrorCallback*	cb;
    BOOL		bFirst    = TRUE;
    BOOL		bSeenSink = FALSE;

    for (CHXSimpleList::Iterator i = 
	    m_pProc->pc->error_sink_handler->m_pSinkList->Begin();
	    i != m_pProc->pc->error_sink_handler->m_pSinkList->End();
	    ++i
	)
    {
	if (NULL != ((ErrorSinkHandler::Sink*)(*i))->m_pErrorSink)
	{
	    bSeenSink = TRUE;
	}
	else
	    continue;

	if ((unSeverity < ((ErrorSinkHandler::Sink*)(*i))->m_unLowSeverity) ||
	    (unSeverity > ((ErrorSinkHandler::Sink*)(*i))->m_unHighSeverity))
	{
	    continue;
	}

	cb = new ErrorCallback;

	cb->m_pErrSink    = ((ErrorSinkHandler::Sink*)(*i))->m_pErrorSink;
	cb->m_unSeverity    = unSeverity;
	cb->m_ulHXCode     = ulHXCode;
	cb->m_ulUserCode    = ulUserCode;

	if (bFirst)
	{
	    if (pUserString)
	    {
		pBufUserString = new ServerBuffer(TRUE);
		pBufUserString->Set((const BYTE*)pUserString,
		    strlen(pUserString) + 1);
	    }

	    if (pMoreInfoURL)
	    {
		pBufMoreInfoURL = new ServerBuffer(TRUE);
		pBufMoreInfoURL->Set((const BYTE*)pMoreInfoURL,
		    strlen(pMoreInfoURL) + 1);
	    }

	    bFirst = FALSE;
	}

	cb->m_pUserString = pBufUserString;
	if (pBufUserString)
            pBufUserString->AddRef();

	cb->m_pMoreInfoURL = pBufMoreInfoURL;
	if (pBufMoreInfoURL)
            pBufMoreInfoURL->AddRef();

        if (m_pProc->pc->dispatchq->send(m_pProc, cb,
	    ((ErrorSinkHandler::Sink*)(*i))->m_nProcnum))
        {
#ifdef DEBUG
            printf("ErrorHandler::Report: dispatchq error -- %d, %08x %08x '%s' '%s'\n",
                   unSeverity, ulHXCode, ulUserCode,
                   pUserString ? pUserString : "<null>",
                   pMoreInfoURL ? pMoreInfoURL : "<null>");
#endif
        }
    }

    HX_RELEASE(pBufUserString);
    HX_RELEASE(pBufMoreInfoURL);

    /* Show Non-Debug messages if there are no sinks */
    /* Always show debug messages, if requested */
    if ((bSeenSink == FALSE && unSeverity != HXLOG_DEBUG) ||
        (unSeverity == HXLOG_DEBUG && g_bShowDebugErrorMessages))
    {
	/*
	 * Just output the error to the screen if we don't have any error
	 * sinks available.
	 */
	printf ("%c: %s\n", (unSeverity < sizeof(zpSeverityTable)) ?
	    zpSeverityTable[unSeverity] : '-', pUserString);

	fflush(stdout);

        /*
         * Also, make a valiant attempt to try to append to the error log file.
         * If we hit this code our normal error sink is not loaded, so we'll
         * have to do it ourselves.  Errors (unSeverity <= 3) only unless
         * showing all messages was requested.
         */
        if ((unSeverity <= 3 || g_bShowDebugErrorMessages) &&
            m_pProc->pc->registry && m_pProc->pc->scheduler)
        {
            IHXBuffer* pLogFile = 0;
            char* szLogFile = 0;
            FILE* f = 0;

        // Check for the filename used by the standard error log template.
            if (HXR_OK ==
                m_pProc->pc->registry->GetStr("config.LoggingTemplates.Standard Error Log.Outputs.ErrorLog.Filename", pLogFile,
			                      m_pProc))
            {
                szLogFile = (char*)pLogFile->GetBuffer();
            }
            if (szLogFile)
            {
                f = fopen(szLogFile, "a+");
            }
            if (f)
            {
                const char* szSeverityPrefix = ((unSeverity <= 3) ? "***" : "");

                char szTime[32];
                HXTimeval Now =
                    m_pProc->pc->scheduler->GetCurrentSchedulerTime();
                struct tm tm;
                hx_localtime_r((const time_t *)&Now.tv_sec, &tm);
                strftime(szTime, sizeof szTime, "%d-%b-%y %H:%M:%S", &tm);
    
                char* szEOL = "\n";
                if (pUserString[0] &&
	            pUserString[strlen(pUserString)-1] == '\n')
                {
                    szEOL = "";
                }

	        fprintf (f, "%s%s.%03ld %s(%ld): %s%s",
                         szSeverityPrefix, szTime, Now.tv_usec/1000,
                         ServerVersion::ExecutableName(), process_id(),
                         pUserString, szEOL);
                fclose(f);
            }
            HX_RELEASE(pLogFile);
        }
    }

    return HXR_OK;
}

ErrorHandler::ErrorCallback::~ErrorCallback()
{
    HX_RELEASE(m_pUserString);
    HX_RELEASE(m_pMoreInfoURL);
}

void
ErrorHandler::ErrorCallback::func(Process* proc)
{
    HX_ASSERT(m_pErrSink);

    m_pErrSink->ErrorOccurred(m_unSeverity, m_ulHXCode, m_ulUserCode,
	(char *)(m_pUserString  ? m_pUserString->GetBuffer()  : 0),
	(char *)(m_pMoreInfoURL ? m_pMoreInfoURL->GetBuffer() : 0));

    delete this;
}
