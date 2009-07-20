/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thrhypnv.cpp,v 1.3 2008/02/19 10:22:58 vkathuria Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxprefs.h"
#include "hxhyper.h"
#include "hxtick.h"
#include "hxslist.h"
#include "hxthread.h"
#include "pckunpck.h"
#include "hxhypnv.h"
#include "thrhypnv.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

enum
{
     REINIT = 0
    ,ONURL
};

void* HyperThreadRoutine(void * pArg);

#ifdef _WIN32
#define HXMSG_QUIT		(WM_USER + 1000)	/* Exit from the thread */
#define HXURL_COMMAND		(WM_USER + 1001)	/* URL Command */
#else
#define HXMSG_QUIT		1000	    	/* Exit from the thread */
#define HXURL_COMMAND		1001		/* URL Command */
#endif /*_WIN32*/

/****************************************************************************
 *
 *  Interface:
 *
 *	HXHyperNavigate
 *
 *  Purpose:
 *
 *	TBD
 *
 *  IID_IHXHyperNavigate:
 *
 *	{00000900-61DF-11d0-9CEE-080017035B43}
 *
 */


HXThreadHyperNavigate::HXThreadHyperNavigate() :
      m_lRefCount(0)
    , m_pContext(0)
    , m_bInitialized(FALSE)
    , m_pThread(NULL)
    , m_pQuitEvent(NULL)
    , m_bUseThread(FALSE)
    , m_pHyperNavigate(NULL)
{
}

HXThreadHyperNavigate::~HXThreadHyperNavigate()
{
    Stop();
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your
//		object.
//
STDMETHODIMP HXThreadHyperNavigate::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXHyperNavigate), (IHXHyperNavigate*)this },
            { GET_IIDHANDLE(IID_IHXHyperNavigate2), (IHXHyperNavigate2*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXHyperNavigate*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXThreadHyperNavigate::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXThreadHyperNavigate::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP HXThreadHyperNavigate::Init(IUnknown* pContext)
{
    if (!pContext)
    {
	return HXR_UNEXPECTED;
    }

    HX_RESULT theErr = HXR_OK;

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    if (!m_bUseThread)
    {
	if (!m_pHyperNavigate)
	{
	    m_pHyperNavigate = new HXHyperNavigate;
	    m_pHyperNavigate->AddRef();
	}

	theErr = m_pHyperNavigate->Init(m_pContext);
    }
    else
    {
	StartHyperThread();

	HyperCommand* pCommand = new HyperCommand;
	pCommand->m_Type = REINIT;
	HXThreadMessage msg(HXURL_COMMAND, (void*) pCommand, NULL);
	m_pThread->PostMessage(&msg, NULL);
    }

    m_bInitialized = TRUE;

    return theErr;
}

void		    
HXThreadHyperNavigate::UseThread(HXBOOL bUseThread)
{
#ifdef THREADS_SUPPORTED
    m_bUseThread = bUseThread;

    if (!m_bInitialized)
    {
	return;
    }

    if (m_bUseThread)
    {
	if (!m_pThread)
	{
	    HX_RELEASE(m_pHyperNavigate);
	    StartHyperThread();
	    /* Reinitialize*/
	    Init(m_pContext);
	}
    }
    else
    {
	if (m_pThread)
	{
	    StopHyperThread();

	    HX_ASSERT(m_pHyperNavigate == NULL);
	    /* Reinitialize*/
	    Init(m_pContext);
	}
    }
#endif /*THREADS_SUPPORTED*/
}

void		    
HXThreadHyperNavigate::Stop(void)
{
    StopHyperThread();

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pHyperNavigate);
}


/*
 *	IHXHyperNavigate methods
 */

/************************************************************************
 *	Method:
 *	    IHXHyperNavigate::GoToURL
 *	Purpose:
 *	    Performs a simple Go To URL operation.
 */
STDMETHODIMP HXThreadHyperNavigate::GoToURL( const char* pURL,
					const char* pTarget)
{
    if (!pURL)
    {
	return HXR_UNEXPECTED;
    }

    if (!m_bUseThread)
    {
	return m_pHyperNavigate->GoToURL(pURL, pTarget);
    }
    else
    {
	HyperCommand* pCommand = new HyperCommand;
	pCommand->m_Type = ONURL;
	pCommand->m_pURL = new char[strlen(pURL) + 1];
	strcpy(pCommand->m_pURL, pURL); /* Flawfinder: ignore */

	if (pTarget)
	{
	    pCommand->m_pTarget = new char[strlen(pTarget) + 1];
	    strcpy(pCommand->m_pTarget, pTarget); /* Flawfinder: ignore */
	}

	HXThreadMessage msg(HXURL_COMMAND, (void*) pCommand, NULL);
	m_pThread->PostMessage(&msg, NULL);
	return HXR_OK;
    }
}

/************************************************************************
 *	Method:
 *	    IHXHyperNavigate2::Execute
 *	Purpose:
 *	    
 *	Parameters:
 *      pURL:	    URL (absolute or relative)
 *	    pTargetInstance:	
 *	    pTargetApplication: 
 *	    pTargetRegion:
 *	    pParams:
 */
STDMETHODIMP 
HXThreadHyperNavigate::Execute(const char* pURL,
			  const char* pTargetInstance,
			  const char* pTargetApplication,
			  const char* pTargetRegion,
			  IHXValues* pParams)
{
    return GoToURL(pURL, pTargetInstance);
}


void
HXThreadHyperNavigate::StartHyperThread()
{
    if (m_pThread)
    {
	return;
    }

    HX_RELEASE(m_pHyperNavigate);

    CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pThread, m_pContext);
    CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);

    m_pThread->CreateThread(HyperThreadRoutine, (void*) this, 0);
    m_pThread->SetThreadName("HyperNavigate Thread");
}


void HXThreadHyperNavigate::StopHyperThread()
{
    if (m_pThread)
    {
	HXThreadMessage msg(HXMSG_QUIT, NULL, NULL);
	if (m_pThread->PostMessage(&msg, NULL) == HXR_OK)
	{
	    m_pQuitEvent->Wait(ALLFS);
	}
#ifdef _MACINTOSH /* @@@CXZ - ALLFS sometimes causes browsers to hang when quitting embedded player */
	m_pQuitEvent->Wait(500);
#else
	m_pQuitEvent->Wait(ALLFS);
#endif
	m_pThread->Exit(0);
	HX_RELEASE(m_pThread);
    }

    HX_RELEASE(m_pQuitEvent);
}

void*
HyperThreadRoutine(void * pArg)
{
    HXThreadHyperNavigate* pThreadHyperNavigate = 
			    (HXThreadHyperNavigate*) pArg;

    IHXThread*	    pThread	= pThreadHyperNavigate->m_pThread;
    HyperCommand*   pCommand	= NULL;
    HXBOOL	    bDone	= FALSE;
    HXThreadMessage msg;


    if (!pThreadHyperNavigate->m_pHyperNavigate)
    {
	pThreadHyperNavigate->m_pHyperNavigate = new HXHyperNavigate;
	pThreadHyperNavigate->m_pHyperNavigate->AddRef();
    }

    HXHyperNavigate* pHyperNavigate	= 
				pThreadHyperNavigate->m_pHyperNavigate;

    while (!bDone && pThread->GetMessage(&msg, 0,0) == HXR_OK)
    {
	switch (msg.m_ulMessage)
	{
	    case HXURL_COMMAND:
		{
		    pCommand = (HyperCommand*) msg.m_pParam1; 
		    switch (pCommand->m_Type)
		    {
			case REINIT:
			    pHyperNavigate->Init(pThreadHyperNavigate->m_pContext);
			    break;
			case ONURL:
			    pHyperNavigate->GoToURL(pCommand->m_pURL, 
						    pCommand->m_pTarget);
			    break;
			default:
			    HX_ASSERT(FALSE);
			    break;
		    }

		    delete pCommand;
		}
		break;
	    case HXMSG_QUIT:
		bDone	= TRUE;
		break;
	    default:
		pThread->DispatchMessage(&msg);
		break;
	}
    }

    HX_RELEASE(pThreadHyperNavigate->m_pHyperNavigate);

    pThreadHyperNavigate->m_pQuitEvent->SignalEvent();


    return (void*) 0;
}
