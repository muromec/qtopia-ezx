/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fsys_wrap.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#endif // _UNIX

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxstring.h"
#include "proc.h"
#include "dispatchq.h"
#include "server_engine.h"
#include "mutex.h"

#include "fsys_wrap.h"
#include "flob_wrap.h"
#include "hxmap.h"
#include "fsmanager.h"

#define DO_CONDITIONAL_UNLOCK_MAIN                              \
    BOOL bDidUnlock = FALSE;                                    \
    if (m_myproc->pc->engine->m_bMutexProtection == TRUE)       \
    {                                                           \
	HXMutexUnlock(g_pServerMainLock);                       \
	bDidUnlock = TRUE;                                      \
    }

#define DO_CONDITIONAL_RELOCK_MAIN              \
    if (bDidUnlock)                             \
    {                                           \
	HXMutexLock(g_pServerMainLock);         \
    }

FileSystemWrapper::FileSystemWrapper(FSManager*		    response,
				     Process*		    myproc,
				     Process*		    fs_proc,
				     IHXFileSystemObject*  pFS) :
    m_myproc(myproc),
    m_fs_proc(fs_proc),
    m_pRealFS(pFS),
    m_lRefCount(0),
    m_response(response)
{
    if (m_response)
    {
	m_response->AddRef();
    }
}

FileSystemWrapper::~FileSystemWrapper()
{
    HX_RELEASE(m_response);
}

STDMETHODIMP
FileSystemWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileSystemObject))
    {
	AddRef();
	*ppvObj = (IHXFileSystemObject*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
FileSystemWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
FileSystemWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void
GetFileSystemInfoCallback::func(Process* p)
{
    m_fsw->m_pRealFS->GetFileSystemInfo
	(m_fsw->m_pShortName, m_fsw->m_pProtocol);

    m_fsw->m_got_fsys_info = TRUE;

    delete this;
}

STDMETHODIMP
FileSystemWrapper::GetFileSystemInfo(REF(const char*) pShortName,
				     REF(const char*) pProtocol)
{
    GetFileSystemInfoCallback* cb = new
	GetFileSystemInfoCallback(this);

    m_got_fsys_info = FALSE;
    if(m_fs_proc->pc->dispatchq->send(m_fs_proc, cb, m_fs_proc->procnum()) < 0)
	return HXR_FAIL;

    DO_CONDITIONAL_UNLOCK_MAIN
    while(!m_got_fsys_info);
    DO_CONDITIONAL_RELOCK_MAIN

    return HXR_OK;
}

void
InitFileSystemCallback::func(Process* p)
{
    m_fsw->m_pRealFS->InitFileSystem(m_options);

    delete this;
}

STDMETHODIMP
FileSystemWrapper::InitFileSystem(IHXValues* options)
{
    InitFileSystemCallback* cb = new InitFileSystemCallback(this, options);
    if(m_fs_proc->pc->dispatchq->send(m_fs_proc, cb, m_fs_proc->procnum()) < 0)
	return HXR_FAIL;

    return HXR_OK;
}

STDMETHODIMP
FileSystemWrapper::CreateFile(IUnknown** ppFileObject)
{
    FileObjectWrapper* fow = new FileObjectWrapper(this, m_myproc, m_fs_proc);
    fow->AddRef();

    if(fow)
    {
	if(HXR_OK == fow->Create())
	{
	    if(HXR_OK == fow->QueryInterface(IID_IUnknown,
					   (void**)ppFileObject))
	    {
		fow->Release();
		return HXR_OK;
	    }
	    return HXR_UNEXPECTED;
	}
	fow->Release();
    }	
    *ppFileObject = NULL;
    return HXR_OUTOFMEMORY;
}

HX_RESULT
FileSystemWrapper::AsyncCreateFile()
{
    FileObjectWrapper* fow = new FileObjectWrapper(this, m_myproc, m_fs_proc);
    fow->AddRef();

    if(fow)
    {
	return fow->AsyncCreate();
    }
    return HXR_FAIL;
}

HX_RESULT
FileSystemWrapper::AsyncCreateDone(HX_RESULT status,
				   FileObjectWrapper* fow)
{
    AddRef();
    HX_RESULT hresult = m_response->AsyncCreateFileDone(status, fow);
    Release();

    return hresult;
}

STDMETHODIMP
FileSystemWrapper::CreateDir(IUnknown** ppDirObject)
{
    return HXR_NOTIMPL;
}
