/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: flob_wrap.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "proc.h"
#include "dispatchq.h"
#include "server_engine.h"
#include "mutex.h"

#include "fsys_wrap.h"
#include "flob_wrap_callbacks.h"
#include "flob_wrap.h"
#include "fres_wrap.h"
#include "hxfiles.h"

#define DO_CONDITIONAL_UNLOCK_MAIN				\
    BOOL bDidUnlock = FALSE;					\
    if (m_myproc->pc->engine->m_bMutexProtection == TRUE)	\
    {								\
	HXMutexUnlock(g_pServerMainLock);			\
	bDidUnlock = TRUE;					\
    }

#define DO_CONDITIONAL_RELOCK_MAIN		\
    if (bDidUnlock)				\
    {						\
	HXMutexLock(g_pServerMainLock);		\
    }

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP FileObjectWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXFileObject *)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileObject))
    {
	if(m_has_file_object)
	{
	    AddRef();
	    *ppvObj = (IHXFileObject*)this;
	    return HXR_OK;
	}
    }
    else if (IsEqualIID(riid, IID_IHXFileStat))
    {
	if(m_has_file_stat)
	{
	    AddRef();
	    *ppvObj = (IHXFileStat*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXFileExists))
    {
	if(m_has_file_exists)
	{
	    AddRef();
	    *ppvObj = (IHXFileExists*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXFileMimeMapper))
    {
	if(m_has_file_mime_mapper)
	{
	    AddRef();
	    *ppvObj = (IHXFileMimeMapper*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXBroadcastMapper))
    {
	if(m_has_broadcast_mapper)
	{
	    AddRef();
	    *ppvObj = (IHXBroadcastMapper*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXGetFileFromSamePool))
    {
	if(m_has_pool_object)
	{
	    AddRef();
	    *ppvObj = (IHXGetFileFromSamePool*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXRequestHandler))
    {
	if(m_has_request_handler)
	{
	    AddRef();
	    *ppvObj = (IHXRequestHandler*)this;
	    return HXR_OK;
	}
    }
    else if(IsEqualIID(riid, IID_IHXPostDataHandler))
    {
	if(m_has_post_data_handler)
	{
	    AddRef();
	    *ppvObj = (IHXPostDataHandler*)this;
	    return HXR_OK;
	}
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
STDMETHODIMP_(ULONG32) FileObjectWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) FileObjectWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

FileObjectWrapper::FileObjectWrapper(FileSystemWrapper* fsw,
				     Process* myproc,
				     Process* fs_proc) :
    m_lRefCount(0),
    m_parent(fsw),
    m_myproc(myproc),
    m_fs_proc(fs_proc),
    m_has_file_object(FALSE),
    m_has_file_stat(FALSE),
    m_has_file_exists(FALSE),
    m_has_file_mime_mapper(FALSE),
    m_has_broadcast_mapper(FALSE),
    m_has_pool_object(FALSE),
    m_has_request_handler(FALSE),
    m_has_post_data_handler(FALSE),
    m_created_file(FALSE),
    m_got_request(FALSE),
    m_got_filename(FALSE),
    m_pFileObject(NULL),
    m_pFileStat(NULL),
    m_pFileExists(NULL),
    m_pBroadcastMapper(NULL),
    m_pMimeMapper(NULL),
    m_pFileResponse(NULL),
    m_pFileExistsResponse(NULL),
    m_pFileMimeMapperResponse(NULL),
    m_pBroadcastMapperResponse(NULL),
    m_pFileStatResponse(NULL),
    m_pObject(NULL),
    m_pPoolObject(NULL),
    m_pRequestHandler(NULL),
    m_pPostDataHandler(NULL),
    m_pRequest(NULL),
    m_pPath(NULL),
    m_pFilename(NULL)
{
    m_parent->AddRef();
}

FileObjectWrapper::FileObjectWrapper(IUnknown* pObject,
				     FileSystemWrapper* fsw,
				     Process* myproc,
				     Process* fs_proc) :
    m_lRefCount(0),
    m_parent(fsw),
    m_myproc(myproc),
    m_fs_proc(fs_proc),
    m_has_file_object(FALSE),
    m_has_file_stat(FALSE),
    m_has_file_exists(FALSE),
    m_has_file_mime_mapper(FALSE),
    m_has_pool_object(FALSE),
    m_has_request_handler(FALSE),
    m_has_broadcast_mapper(FALSE),
    m_has_post_data_handler(FALSE),
    m_created_file(FALSE),
    m_got_request(FALSE),
    m_got_filename(FALSE),
    m_pFileExists(NULL),
    m_pFileObject(NULL),
    m_pFileStat(NULL),
    m_pBroadcastMapper(NULL),
    m_pMimeMapper(NULL),
    m_pFileResponse(NULL),
    m_pFileExistsResponse(NULL),
    m_pFileMimeMapperResponse(NULL),
    m_pBroadcastMapperResponse(NULL),
    m_pFileStatResponse(NULL),
    m_pObject(pObject),
    m_pPoolObject(NULL),
    m_pRequestHandler(NULL),
    m_pPostDataHandler(NULL),
    m_pRequest(NULL),
    m_pPath(NULL),
    m_pFilename(NULL)
{
    m_parent->AddRef();
    m_pObject->AddRef();

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXFileObject,
					 (void**)&m_pFileObject))
    {
	m_has_file_object = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXFileStat,
					(void**)&m_pFileStat))
    {
	m_has_file_stat = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXFileExists,
					 (void**)&m_pFileExists))
    {
	m_has_file_exists = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXFileMimeMapper,
						(void**)&m_pMimeMapper))
    {
	m_has_file_mime_mapper = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXBroadcastMapper,
				          (void**)&m_pBroadcastMapper))
    {
	m_has_broadcast_mapper = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXGetFileFromSamePool,
					 (void**)&m_pPoolObject))
    {
	m_has_pool_object = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXRequestHandler,
					 (void**)&m_pRequestHandler))
    {
	m_has_request_handler = TRUE;
    }

    if(HXR_OK == m_pObject->QueryInterface(IID_IHXPostDataHandler,
					 (void**)&m_pPostDataHandler))
    {
	m_has_post_data_handler = TRUE;
    }

    m_created_file = TRUE;
}

void FileObjectReleaseCallback::func(Process* p)
{
    if(m_pFileExists)
	m_pFileExists->Release();

    if(m_pMimeMapper)
	m_pMimeMapper->Release();
    
    if(m_pBroadcastMapper)
	m_pBroadcastMapper->Release();

    if(m_pFileObject)
	m_pFileObject->Release();

    if(m_pFileStat)
	m_pFileStat->Release();

    if(m_pObject)
	m_pObject->Release();

    if(m_pPoolObject)
	m_pPoolObject->Release();

    if(m_pRequestHandler)
	m_pRequestHandler->Release();

    if(m_pRequest)
	m_pRequest->Release();

    if(m_pPostDataHandler)
	m_pPostDataHandler->Release();

    delete this;
}

FileObjectWrapper::~FileObjectWrapper()
{
    if(m_parent)
	m_parent->Release();

    if(m_pFileResponse)
    {
	m_pFileResponse->Release();
	m_pFileResponse = NULL;
    }

    if(m_pFileExistsResponse)
    {
	m_pFileExistsResponse->Release();
	m_pFileExistsResponse = NULL;
    }

    if(m_pBroadcastMapperResponse)
    {
	m_pBroadcastMapperResponse->Release();
	m_pBroadcastMapperResponse = NULL;
    }

    if(m_pFileMimeMapperResponse)
    {
	m_pFileMimeMapperResponse->Release();
	m_pFileMimeMapperResponse = NULL;
    }

    if (m_pFileStatResponse)
    {
	m_pFileStatResponse->Release();
	m_pFileStatResponse = NULL;
    }

    FileObjectReleaseCallback* forcb;
    forcb = new FileObjectReleaseCallback(m_pFileExists,
					  m_pMimeMapper,
					  m_pBroadcastMapper,
					  m_pFileObject,
					  m_pFileStat,
					  m_pPoolObject,
					  m_pObject,
					  m_pRequestHandler,
					  m_pPostDataHandler,
					  m_pRequest);
    m_myproc->pc->dispatchq->send(m_myproc, forcb, m_fs_proc->procnum());
}


void
CreateFileCallback::func(Process* p)
{
    if (HXR_OK != m_fow->m_parent->m_pRealFS->CreateFile(&m_fow->m_pObject))
    {
	if(m_bAsync)
	{
	    CreatedFileCallback* cb = new CreatedFileCallback(m_fow, 
							      HXR_FAILED);
	    m_fow->m_fs_proc->pc->dispatchq->send(m_fow->m_fs_proc,
					     cb,
					     m_fow->m_myproc->procnum());
	}
	return;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXFileObject,
						(void**)&m_fow->m_pFileObject))
    {
	m_fow->m_has_file_object = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXFileStat,
						(void**)&m_fow->m_pFileStat))
    {
	m_fow->m_has_file_stat = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXFileExists,
						(void**)&m_fow->m_pFileExists))
    {
	m_fow->m_has_file_exists = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXFileMimeMapper,
						(void**)&m_fow->m_pMimeMapper))
    {
	m_fow->m_has_file_mime_mapper = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXBroadcastMapper,
				           (void**)&m_fow->m_pBroadcastMapper))
    {
	m_fow->m_has_broadcast_mapper = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXGetFileFromSamePool,
						(void**)&m_fow->m_pPoolObject))
    {
	m_fow->m_has_pool_object = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXRequestHandler,
					   (void**)&m_fow->m_pRequestHandler))
    {
	m_fow->m_has_request_handler = TRUE;
    }

    if(HXR_OK == m_fow->m_pObject->QueryInterface(IID_IHXPostDataHandler,
					 (void**)&m_fow->m_pPostDataHandler))
    {
	m_fow->m_has_post_data_handler = TRUE;
    }

    if(m_bAsync)
    {
	CreatedFileCallback* cb = new CreatedFileCallback(m_fow,
							  HXR_OK);
	m_fow->m_fs_proc->pc->dispatchq->send(m_fow->m_fs_proc,
					 cb,
					 m_fow->m_myproc->procnum());
    }
    else
    {
	m_fow->m_created_file = TRUE;
    }
    delete this;
}

void
CreatedFileCallback::func(Process* p)
{
    m_fow->m_parent->AsyncCreateDone(m_status, m_fow);
    delete this;
}

void
InitFileCallback::func(Process* p)
{
    FileResponseWrapper* frw = new FileResponseWrapper(m_fow);

    m_fow->m_pFileObject->Init(m_ulFlags,
			       frw);

    delete this;
}

// Special method that does the actual create when the fileSYSTEMs CreateFile
// is called
HX_RESULT 
FileObjectWrapper::Create()
{
    CreateFileCallback* cb = new CreateFileCallback(this, FALSE);
    
    m_created_file = FALSE;
    m_myproc->pc->dispatchq->send(m_myproc, cb, m_fs_proc->procnum());
 
    DO_CONDITIONAL_UNLOCK_MAIN
    while(!m_created_file);
    DO_CONDITIONAL_RELOCK_MAIN

    return HXR_OK;
}

// Even more special method that does an asynchronous create file
HX_RESULT
FileObjectWrapper::AsyncCreate()
{
    CreateFileCallback* cb = new CreateFileCallback(this, TRUE);

    m_myproc->pc->dispatchq->send(m_myproc, cb, m_fs_proc->procnum());

    return HXR_OK;
}

STDMETHODIMP
FileObjectWrapper::Init(ULONG32     ulFlags,
			IHXFileResponse* pFileResponse)
{
    if(m_pFileResponse)
    {
	m_pFileResponse->Release();
    }
    m_pFileResponse = pFileResponse;
    m_pFileResponse->AddRef();

    InitFileCallback* ifcb = new InitFileCallback(this, ulFlags);
    m_myproc->pc->dispatchq->send(m_myproc, ifcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
GetFilenameCallback::func(Process* p)
{
    const char* pFilename;

    m_fow->m_pFileObject->GetFilename(pFilename);
    m_fow->m_pFilename = pFilename;
    m_fow->m_got_filename = TRUE;
    delete this;
}

STDMETHODIMP
FileObjectWrapper::GetFilename(REF(const char*)    /*OUT*/  pFilename)
{
    GetFilenameCallback* gfcb = new GetFilenameCallback(this);

    m_got_filename = FALSE;

    m_myproc->pc->dispatchq->send(m_myproc, gfcb, m_fs_proc->procnum());
    DO_CONDITIONAL_UNLOCK_MAIN
    while(!m_got_filename);
    DO_CONDITIONAL_RELOCK_MAIN

    pFilename = m_pFilename;

    return HXR_OK;
}

void
SetRequestCallback::func(Process* p)
{
    m_fow->m_pRequestHandler->SetRequest(m_pRequest);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::SetRequest(IHXRequest*    /*IN*/  pRequest)
{
    SetRequestCallback* srcb;

    /*
     * A broadcast plugin has its own IHXRequest object, which it will
     * not alter on a SetRequest() nor return on a GetRequest() call. On
     * a SetRequest() call, it will set the ResponseHeaders of the passed
     * in IHXRequest object to the RequestHeaders of its IHXRequest object.
     * Therefore, the FileObjectWrapper must keep a pointer to the
     * IHXRequest object that was passed into this function. It will then
     * return this pointer in response to a GetRequest() call
     */

    if (m_has_broadcast_mapper)
    {
	if (m_pRequest)
	{
	    m_pRequest->Release();
	}

	m_pRequest = pRequest;

	if (m_pRequest)
	{
	    m_pRequest->AddRef();
	}
    }

    srcb = new SetRequestCallback(this, pRequest);

    m_myproc->pc->dispatchq->send(m_myproc, srcb, m_fs_proc->procnum());

    return HXR_OK;
}

void
GetRequestCallback::func(Process* p)
{
    IHXRequest* pRequest;

    m_fow->m_pRequestHandler->GetRequest(pRequest);
    m_fow->m_pRequest = pRequest;
    m_fow->m_got_request = TRUE;
    delete this;
}

STDMETHODIMP
FileObjectWrapper::GetRequest(REF(IHXRequest*)    /*OUT*/  pRequest)
{
    GetRequestCallback* grcb;

    /*
     * Please see comment in SetRequest
     */

    if (m_has_broadcast_mapper)
    {
	pRequest = m_pRequest;

	if (pRequest)
	{
	    pRequest->AddRef();
	}

	return HXR_OK;
    }

    grcb = new GetRequestCallback(this);

    m_got_request = FALSE;

    m_myproc->pc->dispatchq->send(m_myproc, grcb, m_fs_proc->procnum());
    DO_CONDITIONAL_UNLOCK_MAIN
    while(!m_got_request);
    DO_CONDITIONAL_RELOCK_MAIN

    pRequest = m_pRequest;

    m_pRequest = 0;

    return HXR_OK;
}

void
CloseFileCallback::func(Process*)
{
    m_fow->m_pFileObject->Close();
    delete this;
}

STDMETHODIMP
FileObjectWrapper::Close()
{
    CloseFileCallback* cfcb = new CloseFileCallback(this);
    m_myproc->pc->dispatchq->send(m_myproc, cfcb, m_fs_proc->procnum());
    return HXR_OK;
}

HX_RESULT
FileObjectWrapper::CloseDone(HX_RESULT status)
{
    m_pFileResponse->CloseDone(status);
    m_pFileResponse->Release();
    m_pFileResponse = NULL;

    return HXR_OK;
}

void
ReadFileCallback::func(Process* p)
{
    m_fow->m_pFileObject->Read(m_ulCount);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::Read(ULONG32	    	    ulCount)
{
    ReadFileCallback* rfcb = new ReadFileCallback(this, ulCount);
    m_myproc->pc->dispatchq->send(m_myproc, rfcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
WriteFileCallback::func(Process* p)
{
    m_fow->m_pFileObject->Write(m_pBuffer);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::Write(IHXBuffer*	    pBuffer)
{
    WriteFileCallback* wfcb = new WriteFileCallback(this, pBuffer);
    m_myproc->pc->dispatchq->send(m_myproc, wfcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
SeekFileCallback::func(Process* p)
{
    m_fow->m_pFileObject->Seek(m_ulOffset, m_bRelative);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::Seek(ULONG32	    ulOffset,
			BOOL               bRelative)
{
    SeekFileCallback* sfcb = new SeekFileCallback(this, ulOffset, bRelative);
    m_myproc->pc->dispatchq->send(m_myproc, sfcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
AdviseFileCallback::func(Process* p)
{
    m_fow->m_pFileObject->Advise(m_ulInfo);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::Advise(ULONG32 ulInfo)
{
    AdviseFileCallback* afcb = new AdviseFileCallback(this, ulInfo);
    m_myproc->pc->dispatchq->send(m_myproc, afcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
GetFileObjectFromPoolCallback::func(Process* p)
{
    m_fow->m_pPoolObject->GetFileObjectFromPool(m_fow);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::GetFileObjectFromPool(
    IHXGetFileFromSamePoolResponse* response)
{
    m_pPoolResponse = response;
    m_pPoolResponse->AddRef();

    GetFileObjectFromPoolCallback* gfofpcb = new 
	GetFileObjectFromPoolCallback(this);
    m_myproc->pc->dispatchq->send(m_myproc, gfofpcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
DoesExistCallback::func(Process* p)
{
    HX_RESULT h_result;
    h_result = m_fow->m_pFileExists->DoesExist(m_pPath, m_fow);
    if(HXR_OK != h_result)
    {
	m_fow->DoesExistDone(FALSE);
    }
    delete this;
}

STDMETHODIMP
FileObjectWrapper::DoesExist(const char* pPath,
			     IHXFileExistsResponse* pResponse)
{
    m_pFileExistsResponse = pResponse;
    m_pFileExistsResponse->AddRef();
    DoesExistCallback* decb = new
	DoesExistCallback(this, pPath);
    m_myproc->pc->dispatchq->send(m_myproc, decb, m_fs_proc->procnum());
    return HXR_OK;
}

void
FindMimeTypeCallback::func(Process* p)
{
    m_fow->m_pMimeMapper->FindMimeType(m_pURL, m_fow);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::FindMimeType(const char* pURL,
				IHXFileMimeMapperResponse* pResponse)
{
    m_pFileMimeMapperResponse = pResponse;
    m_pFileMimeMapperResponse->AddRef();
    FindMimeTypeCallback* fmtcb = new
	FindMimeTypeCallback(this, pURL);
    m_myproc->pc->dispatchq->send(m_myproc, fmtcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
FindBroadcastTypeCallback::func(Process* p)
{
    m_fow->m_pBroadcastMapper->FindBroadcastType(m_pURL,
						 m_fow);
    delete this;
}

STDMETHODIMP
FileObjectWrapper::FindBroadcastType(const char*pURL,
				     IHXBroadcastMapperResponse* pResponse)
{
    m_pBroadcastMapperResponse = pResponse;
    m_pBroadcastMapperResponse->AddRef();
    FindBroadcastTypeCallback* fbtcb = new
	FindBroadcastTypeCallback(this, pURL);
    m_myproc->pc->dispatchq->send(m_myproc, fbtcb, m_fs_proc->procnum());
    return HXR_OK;
}

void
DoesExistDoneCallback::func(Process* p)
{
    m_pResponse->DoesExistDone(m_bExist);
    m_pResponse->Release();
    delete this;
}

STDMETHODIMP
FileObjectWrapper::DoesExistDone(BOOL bExist)
{
    // We may be in the right object, but we're in the wrong process. Rats.
    DoesExistDoneCallback* dedcb = new
	DoesExistDoneCallback(this, bExist, m_pFileExistsResponse);
    m_pFileExistsResponse = 0;
    m_fs_proc->pc->dispatchq->send(m_fs_proc, dedcb, m_myproc->procnum());
    return HXR_OK;
}

void
MimeTypeFoundCallback::func(Process* p)
{
    m_pResponse->MimeTypeFound(m_status, m_pMimeType);
    m_pResponse->Release();
    delete this;
}

STDMETHODIMP
FileObjectWrapper::MimeTypeFound(HX_RESULT status,
				 const char* pMimeType)
{
    MimeTypeFoundCallback* mtfcb = new
	MimeTypeFoundCallback(this, status, pMimeType, 
			      m_pFileMimeMapperResponse);
    m_pFileMimeMapperResponse = 0;
    m_fs_proc->pc->dispatchq->send(m_fs_proc, mtfcb, m_myproc->procnum());
    return HXR_OK;
}

void
BroadcastTypeFoundCallback::func(Process* p)
{
    m_pResponse->BroadcastTypeFound(m_status, m_pBroadcastType);
    m_pResponse->Release();
    delete this;
}

STDMETHODIMP
FileObjectWrapper::BroadcastTypeFound(HX_RESULT status,
				      const char* pBroadcastType)
{
    BroadcastTypeFoundCallback* btfcb = new
	BroadcastTypeFoundCallback(this, status, pBroadcastType,
				   m_pBroadcastMapperResponse);
    m_pBroadcastMapperResponse = 0;
    m_fs_proc->pc->dispatchq->send(m_fs_proc, btfcb, m_myproc->procnum());
    return HXR_OK;    
}

void FileObjectReadyCallback::func(Process* p)
{
    if(m_fow->m_pPoolResponse)
    {
	m_fow->m_pPoolResponse->FileObjectReady(m_status, m_pObject);
	m_fow->m_pPoolResponse->Release();
	m_fow->m_pPoolResponse = NULL;
    }

    delete this;
}

STDMETHODIMP
FileObjectWrapper::FileObjectReady(HX_RESULT status, 
				   IUnknown* pFileObject)
{
    IUnknown* pUnknown;
    FileObjectWrapper* wrapper;
    if(status == HXR_OK && pFileObject)
    {
	wrapper = new FileObjectWrapper(pFileObject,
					m_parent,
					m_myproc,
					m_fs_proc);
	wrapper->QueryInterface(IID_IUnknown,
				(void**)&pUnknown);
    }
    else
    {
	pUnknown = NULL;
    }

    FileObjectReadyCallback* forcb 
	= new FileObjectReadyCallback(this, status, pUnknown);

    HX_RELEASE(pUnknown);	
    m_fs_proc->pc->dispatchq->send(m_fs_proc,
			      forcb,
			      m_myproc->procnum());
    return HXR_OK;
}

STDMETHODIMP
FileObjectWrapper::Stat(IHXFileStatResponse* pFileStatResponse)
{
    m_pFileStatResponse = pFileStatResponse;
    m_pFileStatResponse->AddRef();
    StatCallback* sc = new StatCallback(this);
    m_myproc->pc->dispatchq->send(m_myproc, sc, m_fs_proc->procnum());
    return HXR_OK;
}

void
StatCallback::func(Process* p)
{
    m_fow->m_pFileStat->Stat(m_fow);
    delete this;
}


STDMETHODIMP
FileObjectWrapper::StatDone(HX_RESULT status,
			    UINT32 ulSize,
			    UINT32 ulCreationTime,
			    UINT32 ulAccessTime,
			    UINT32 ulModificationTime,
			    UINT32 ulMode)
{
    StatDoneCallback* sdcb = new StatDoneCallback(this,
						   status,
						   ulSize,
						   ulCreationTime,
						   ulAccessTime,
						   ulModificationTime,
						   ulMode,
						   m_pFileStatResponse);
    m_pFileStatResponse = 0;    
    m_fs_proc->pc->dispatchq->send(m_fs_proc, sdcb, m_myproc->procnum());
    return HXR_OK;
}

void
StatDoneCallback::func(Process* p)
{
    m_pResponse->StatDone(m_status,
			  m_ulSize,
			  m_ulCreationTime,
			  m_ulAccessTime,
			  m_ulModificationTime,
			  m_ulMode);
    m_pResponse->Release();
    delete this;
}

STDMETHODIMP
FileObjectWrapper::PostData(IHXBuffer* pBuffer)
{
    PostDataCallback* pdcb = new PostDataCallback(pBuffer, m_pPostDataHandler);
    m_fs_proc->pc->dispatchq->send(m_fs_proc, pdcb, m_myproc->procnum());
    return HXR_OK;
}

void
PostDataCallback::func(Process* p)
{
    m_pPostDataHandler->PostData(m_pBuffer);
    delete this;
}

