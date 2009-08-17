/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tsfob.cpp,v 1.5 2005/08/15 17:36:53 srobinson Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxformt.h"
#include "hxmon.h"
#include "tsfob.h"
#include "debug.h"
#include "proc.h"
#include "mutex.h"
#include "server_engine.h"
#include "server_context.h"
#include "hxassert.h"
#include "globals.h"
#include "cdist_defs.h"

/* pUnknown MUST have exactly 1 reference */
void ThreadSafeFileObjectWrapper::MakeThreadSafe(REF(IUnknown*) pUnknown,
    Process* pProc)
{
    if (pUnknown == NULL)
    {
	return;
    }

    pUnknown = (IHXFileObject*)new ThreadSafeFileObjectWrapper(
	pUnknown, pProc);
}


ThreadSafeFileObjectWrapper::ThreadSafeFileObjectWrapper(IUnknown* pFob,
    Process* pProc)
    : m_pMIIStats(NULL)
{
    HX_VERIFY(HXR_OK ==
	pFob->QueryInterface(IID_IHXFileObject, (void **)&m_pFileObject));

    m_ulRefCount = 1;
    HXAtomicIncUINT32(g_pFileObjs);

    IHXThreadSafeMethods* pThreadSafe;
    if (HXR_OK == pFob->QueryInterface(IID_IHXThreadSafeMethods,
             (void**)&pThreadSafe))
    {
        m_bThreadSafeRead = (pThreadSafe->IsThreadSafe()
	    & HX_THREADSAFE_METHOD_FS_READ)
	    || g_bForceThreadSafePlugins ? TRUE : FALSE;
        pThreadSafe->Release();
    }
    else 
    {
        m_bThreadSafeRead = g_bForceThreadSafePlugins ? TRUE : FALSE;
    }

    m_pInfo = new ThreadSafeFileObjectInfo;
    m_pInfo->m_pProc = pProc;
    m_pInfo->m_ulRefCount = 1;
    m_pInfo->m_bDidLock = FALSE;

    pFob->Release();
}

ThreadSafeFileObjectWrapper::~ThreadSafeFileObjectWrapper()
{
    HXAtomicDecUINT32(g_pFileObjs);
    m_pFileObject->Release();
    m_pInfo->Release();
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileObject*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileObject))
    {
        AddRef();
	*ppvObj = (IHXFileObject*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
        AddRef();
	*ppvObj = (IHXThreadSafeMethods*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXMIIReadStatCollection))
    {
        AddRef();
	*ppvObj = (IHXMIIReadStatCollection*)this;
	return HXR_OK;
    }
    else
    {
	return m_pFileObject->QueryInterface(riid, ppvObj);
    }
}

ULONG32
ThreadSafeFileObjectWrapper::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
ThreadSafeFileObjectWrapper::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Init(ULONG32 ulFlags,
    IHXFileResponse* pFileResponse)
{
    IHXFileResponse* pWrapper = new
	ThreadSafeFileResponseWrapper(pFileResponse, m_pInfo);

    pWrapper->AddRef();
    HX_RESULT ret = m_pFileObject->Init(ulFlags, pWrapper);
    pWrapper->Release();

    return ret;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Close()
{
    HX_RESULT ret;

    AddRef();

    if (m_bThreadSafeRead == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    ret = m_pFileObject->Close();

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();

    return ret;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Read(ULONG32 ulCount)
{
    HX_RESULT ret;

    // local-file access stat for mii/cdist

    if (m_pMIIStats)
    {
	HXAtomicAddUINT32(&m_pMIIStats->m_ulLocalReadBytes, ulCount);
    }

    AddRef();
    if (m_bThreadSafeRead == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    ret = m_pFileObject->Read(ulCount);

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();
    return ret;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Write(IHXBuffer* pBuffer)
{
    return m_pFileObject->Write(pBuffer);
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Seek(ULONG32 ulOffset, BOOL bRelative)
{
    HX_RESULT ret;

    AddRef();
    if (m_bThreadSafeRead == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    ret = m_pFileObject->Seek(ulOffset, bRelative);

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();
    return ret;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::Advise(ULONG32 ulInfo)
{
    return m_pFileObject->Advise(ulInfo);
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::GetFilename(REF(const char*) pFilename)
{
    return m_pFileObject->GetFilename(pFilename);
}

STDMETHODIMP_(UINT32)
ThreadSafeFileObjectWrapper::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FS_READ;
}
/*
 * IHXMIIReadStatCollection methods
 *
 * These were added to the tsfob because we were already wrapping all the
 * local (content) file objects with these, and I didn't want to add the
 * overhead of another wrapper.   jmevissen, 8/2001
 */
STDMETHODIMP
ThreadSafeFileObjectWrapper::SetMIIReadStatsEnabled(BOOL bEnable, 
						    BOOL* bOldValue)
{
    HX_RESULT status = HXR_OK;

    if (bOldValue)
    {
	*bOldValue = m_pMIIStats != NULL;
    }
    if (bEnable && !m_pMIIStats)
    {
	IHXRegistry* pRegistry = NULL;
	((IUnknown*)m_pInfo->m_pProc->pc->server_context)->
	    QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
	IHXBuffer* pBuf = NULL;
	status = pRegistry->GetBufByName(CDIST_REGISTRY_STATISTICS, pBuf);
	if (status == HXR_OK)
	{
	    m_pMIIStats = *((CDistMIIStatistics**)pBuf->GetBuffer());
	    pBuf->Release();
	}
	pRegistry->Release();
    }
    else
    {
	m_pMIIStats = NULL;
    }
    return status;
}

STDMETHODIMP
ThreadSafeFileObjectWrapper::GetMIIReadStatsEnabled(REF(BOOL) bEnabled)
{
    bEnabled = m_pMIIStats != NULL;
    return HXR_OK;
}

ThreadSafeFileResponseWrapper::ThreadSafeFileResponseWrapper(
    IHXFileResponse* pRealResponse, ThreadSafeFileObjectInfo* pInfo)
{
    m_pRealResponse = pRealResponse;
    m_pRealResponse->AddRef();

    IHXThreadSafeMethods* pThreadSafe;
    if (HXR_OK == m_pRealResponse->QueryInterface(IID_IHXThreadSafeMethods,
             (void**)&pThreadSafe))
    {
        m_bThreadSafeReadDone = (pThreadSafe->IsThreadSafe()
	    & HX_THREADSAFE_METHOD_FSR_READDONE)
	    || g_bForceThreadSafePlugins ? TRUE : FALSE;
        pThreadSafe->Release();
    }
    else 
    {
        m_bThreadSafeReadDone = g_bForceThreadSafePlugins ? TRUE : FALSE;
    }

    m_pInfo = pInfo;
    m_pInfo->AddRef();

    m_ulRefCount = 0;
}

ThreadSafeFileResponseWrapper::~ThreadSafeFileResponseWrapper()
{
    m_pRealResponse->Release();
    m_pInfo->Release();
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
        AddRef();
	*ppvObj = (IHXFileResponse*)this;
	return HXR_OK;
    }
    else
    {
	return m_pRealResponse->QueryInterface(riid, ppvObj);
    }
}

ULONG32
ThreadSafeFileResponseWrapper::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
ThreadSafeFileResponseWrapper::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::InitDone(HX_RESULT       status)
{
    if (m_pInfo && status == HXR_NO_MORE_FILES)
    {
	char szErr[256];
	snprintf(szErr, sizeof(szErr), 
		"The server has run out of file descriptors.  It is highly "
		"recommended that you raise the file descriptor limit and "
		"restart the server. (failed in open file)");
	m_pInfo->m_pProc->pc->error_handler->
		Report(HXLOG_ERR, 0, 0, szErr, NULL);
    }

    return m_pRealResponse->InitDone(status);
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::CloseDone(HX_RESULT       status)
{
    AddRef();

    if (m_pInfo->m_bDidLock && m_bThreadSafeReadDone)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    if (m_bThreadSafeReadDone == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    HX_RESULT ret = m_pRealResponse->CloseDone(status);

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();

    return ret;
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::ReadDone(HX_RESULT           status,
					IHXBuffer*         pBuffer)
{
    AddRef();

    if (m_pInfo->m_bDidLock && m_bThreadSafeReadDone)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    if (m_bThreadSafeReadDone == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    HX_RESULT ret = m_pRealResponse->ReadDone(status, pBuffer);

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();

    return ret;
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::WriteDone(HX_RESULT           status)
{
    return m_pRealResponse->WriteDone(status);
}

STDMETHODIMP
ThreadSafeFileResponseWrapper::SeekDone(HX_RESULT           status)
{
    AddRef();

    if (m_pInfo->m_bDidLock && m_bThreadSafeReadDone)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    if (m_bThreadSafeReadDone == FALSE &&
	m_pInfo->m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
        HXMutexLock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = TRUE;
	m_pInfo->m_bDidLock = TRUE;
    }

    HX_RESULT ret = m_pRealResponse->SeekDone(status);

    if (m_pInfo->m_bDidLock)
    {
        HXMutexUnlock(g_pServerMainLock);
        m_pInfo->m_pProc->pc->engine->m_bMutexProtection = FALSE;
	m_pInfo->m_bDidLock = FALSE;
    }

    Release();

    return ret;
}
