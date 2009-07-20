/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: flob_wrap_callbacks.h,v 1.2 2003/01/23 23:42:56 damonlan Exp $ 
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
    
#ifndef _FLOB_WRAP_CALLBACKS_H_
#define _FLOB_WRAP_CALLBACKS_H_

#include "ihxpckts.h"
#include "hxstrutl.h"
#include "hxauth.h"
#include "simple_callback.h"
#include "flob_wrap.h"

class FileObjectWrapper;

class FileObjectReleaseCallback : public SimpleCallback
{
public:
    FileObjectReleaseCallback(IHXFileExists* pFileExists,
			      IHXFileMimeMapper* pMimeMapper,
			      IHXBroadcastMapper *pBroadcastMapper,
			      IHXFileObject* pFileObject,
			      IHXFileStat* pFileStat,
			      IHXGetFileFromSamePool* pPoolObject,
			      IUnknown* pObject,
			      IHXRequestHandler* pRequestHandler,
			      IHXPostDataHandler* pPostDataHandler,
			      IHXRequest* pRequest) :
	m_pFileExists(pFileExists),
	m_pMimeMapper(pMimeMapper),
	m_pBroadcastMapper(pBroadcastMapper),
	m_pFileObject(pFileObject),
	m_pPoolObject(pPoolObject),
	m_pObject(pObject),
	m_pRequestHandler(pRequestHandler),
	m_pRequest(pRequest),
	m_pFileStat(pFileStat),
	m_pPostDataHandler(pPostDataHandler),
	m_fow(0)
    {
    };
    void func(Process* p);

    IHXFileExists* m_pFileExists;
    IHXFileMimeMapper* m_pMimeMapper;
    IHXBroadcastMapper *m_pBroadcastMapper;
    IHXFileObject* m_pFileObject;
    IHXGetFileFromSamePool* m_pPoolObject;
    IUnknown* m_pObject;
    IHXRequestHandler* m_pRequestHandler;
    IHXRequest* m_pRequest;
    IHXFileStat* m_pFileStat;
    IHXPostDataHandler* m_pPostDataHandler;
    FileObjectWrapper* m_fow;
};

class CreatedFileCallback : public SimpleCallback
{
public:
    CreatedFileCallback(FileObjectWrapper* fow, HX_RESULT status) :
	m_fow(fow),
	m_status(status)
    {
	m_fow->AddRef();
    };
    ~CreatedFileCallback()
    {
	m_fow->Release();
    }

    void func(Process* p);
    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
};

class CreateFileCallback : public SimpleCallback
{
public:
    CreateFileCallback(FileObjectWrapper* fow,
		       BOOL async) :
	m_fow(fow),
	m_bAsync(async)
    {
	m_fow->AddRef();
    };
    ~CreateFileCallback()
    {
	m_fow->Release();
    }

    void func(Process* p);
    
    FileObjectWrapper* m_fow;
    BOOL m_bAsync;
};

class InitFileCallback : public SimpleCallback
{
public:
    InitFileCallback(FileObjectWrapper* flob,
		     ULONG32 ulFlags) :
	m_fow(flob),
	m_ulFlags(ulFlags)
    {
	m_fow->AddRef();
    };
    ~InitFileCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    ULONG32 m_ulFlags;
};

class SetURLCallback : public SimpleCallback
{
public:
    SetURLCallback(FileObjectWrapper* fow,
		   const char* pURL) :
	m_fow(fow)
    {
	m_pURL = new_string(pURL);
	m_fow->AddRef();
    };
    ~SetURLCallback()
    {
	delete [] m_pURL;
	m_fow->Release();
    }
    void func(Process* p);
    FileObjectWrapper* m_fow;
    char* m_pURL;
};

class ReadFileCallback : public SimpleCallback
{
public:
    ReadFileCallback(FileObjectWrapper* fow,
		     UINT32 ulCount) :
	m_fow(fow), m_ulCount(ulCount)
    {
	m_fow->AddRef();
    };
    ~ReadFileCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    UINT32 m_ulCount;
};

class SeekFileCallback : public SimpleCallback
{
public:
    SeekFileCallback(FileObjectWrapper* fow,
		     UINT32 ulOffset,
		     BOOL   bRelative) :
	m_fow(fow), m_ulOffset(ulOffset), m_bRelative(bRelative)
    {
	m_fow->AddRef();
    };
    ~SeekFileCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    UINT32 m_ulOffset;
    BOOL m_bRelative;
};

class GetPathCallback : public SimpleCallback
{
public:
    GetPathCallback(FileObjectWrapper* fow) :
	m_fow(fow)
    {
	m_fow->AddRef();
    };
    ~GetPathCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
};

class GetFilenameCallback : public SimpleCallback
{
public:
    GetFilenameCallback(FileObjectWrapper* fow) :
	m_fow(fow)
    {
	m_fow->AddRef();
    };
    ~GetFilenameCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
};

class SetRequestCallback : public SimpleCallback
{
public:
    SetRequestCallback(FileObjectWrapper* fow, IHXRequest* pRequest) :
	m_fow(fow),
	m_pRequest(pRequest)
    {
	    m_pRequest->AddRef();
	    m_fow->AddRef();
    };
    ~SetRequestCallback()
    {
	m_pRequest->Release();
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    IHXRequest* m_pRequest;
};

class GetRequestCallback : public SimpleCallback
{
public:
    GetRequestCallback(FileObjectWrapper* fow) :
	m_fow(fow)
    {
	m_fow->AddRef();
    };
    ~GetRequestCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
};

class WriteFileCallback : public SimpleCallback
{
public:
    WriteFileCallback(FileObjectWrapper* fow,
		     IHXBuffer* pBuffer) : 
	m_fow(fow), m_pBuffer(pBuffer)
    {
	m_fow->AddRef();
	m_pBuffer->AddRef();
    };
    ~WriteFileCallback()
    {
	m_fow->Release();
	m_pBuffer->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    IHXBuffer* m_pBuffer;
};

class StatFileCallback : public SimpleCallback
{
public:
    StatFileCallback(FileObjectWrapper* fow) 
	: m_fow(fow)
    {
	m_fow->AddRef();
    }
    ~StatFileCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
};

class AdviseFileCallback : public SimpleCallback
{
public:
    AdviseFileCallback(FileObjectWrapper* fow,
		       UINT32 ulInfo) :
	m_fow(fow), m_ulInfo(ulInfo)
    {
	m_fow->AddRef();
    }
    ~AdviseFileCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    UINT32 m_ulInfo;
};

class CloseFileCallback : public SimpleCallback
{
public:
    CloseFileCallback(FileObjectWrapper* fow) 
	: m_fow(fow)
    {
	m_fow->AddRef();
    }
    ~CloseFileCallback()
    {
	m_fow->Release();
    }
    void func(Process*);
    FileObjectWrapper* m_fow;
};
		      
class GetFileObjectFromPoolCallback : public SimpleCallback
{
public:
    GetFileObjectFromPoolCallback(FileObjectWrapper* fow) :
	m_fow(fow)
    {
	m_fow->AddRef();
    }
    ~GetFileObjectFromPoolCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
};

class DoesExistCallback : public SimpleCallback
{
public:
    DoesExistCallback(FileObjectWrapper* fow,
		      const char* pPath) :
	m_fow(fow), m_pPath(pPath)
    {
	m_fow->AddRef();
    }
    ~DoesExistCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    const char* m_pPath;
};

class DoesExistDoneCallback : public SimpleCallback
{
public:
    DoesExistDoneCallback(FileObjectWrapper* fow,
			  BOOL bExist,
			  IHXFileExistsResponse* pResponse):
	m_fow(fow), m_bExist(bExist), m_pResponse(pResponse)
    {
	m_fow->AddRef();
    };
    ~DoesExistDoneCallback()
    {
	m_fow->Release();
    }

    void func(Process*);
    
    FileObjectWrapper* m_fow;
    BOOL m_bExist;
    IHXFileExistsResponse* m_pResponse;
};

class FindMimeTypeCallback : public SimpleCallback
{
public:
    FindMimeTypeCallback(FileObjectWrapper* fow,
			 const char* pURL) :
	m_fow(fow), m_pURL(pURL)
    {
	m_fow->AddRef();
    }
    ~FindMimeTypeCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    const char* m_pURL;
};

class MimeTypeFoundCallback : public SimpleCallback
{
public:
    MimeTypeFoundCallback(FileObjectWrapper* fow,
			  HX_RESULT status,
			  const char* pMimeType,
			  IHXFileMimeMapperResponse* pResponse) :
	m_fow(fow), m_status(status), m_pResponse(pResponse)
    {
	m_fow->AddRef();
	m_pMimeType = new_string(pMimeType);
    };
    ~MimeTypeFoundCallback()
    {
	m_fow->Release();
	delete[] (char*)m_pMimeType;
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    const char* m_pMimeType;
    HX_RESULT m_status;
    IHXFileMimeMapperResponse* m_pResponse;
};

class FindBroadcastTypeCallback : public SimpleCallback
{
public:
    FindBroadcastTypeCallback(FileObjectWrapper* fow,
			      const char* pURL) :
	m_fow(fow), m_pURL(pURL)
    {
	m_fow->AddRef();
    }
    ~FindBroadcastTypeCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    const char* m_pURL;
};

class BroadcastTypeFoundCallback : public SimpleCallback
{
public:
    BroadcastTypeFoundCallback(FileObjectWrapper* fow,
			  HX_RESULT status,
			  const char* pBroadcastType,
			  IHXBroadcastMapperResponse* pResponse) :
	m_fow(fow), m_status(status), m_pResponse(pResponse)
    {
	m_fow->AddRef();
	m_pBroadcastType = new_string(pBroadcastType);
    };
    ~BroadcastTypeFoundCallback()
    {
	m_fow->Release();
	delete[] (char*)m_pBroadcastType;
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    const char* m_pBroadcastType;
    HX_RESULT m_status;
    IHXBroadcastMapperResponse* m_pResponse;
};

class FileObjectReadyCallback : public SimpleCallback
{
public:
    FileObjectReadyCallback(FileObjectWrapper* fow,
			    HX_RESULT status,
			    IUnknown* pObject):
	m_fow(fow), m_status(status), m_pObject(pObject)
    {
	m_fow->AddRef();
	m_pObject->AddRef();
    };
    ~FileObjectReadyCallback()
    {
	m_fow->Release();
	m_pObject->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
    IUnknown* m_pObject;
};

class StatCallback : public SimpleCallback
{
public:
    StatCallback(FileObjectWrapper* fow)
	: m_fow(fow)
    {
	m_fow->AddRef();
    }
    ~StatCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
};

class StatDoneCallback : public SimpleCallback
{
public:
    StatDoneCallback(FileObjectWrapper* fow,
		     HX_RESULT status,
		     UINT32 ulSize,
		     UINT32 ulCreationTime,
		     UINT32 ulAccessTime,
		     UINT32 ulModificationTime,
		     UINT32 ulMode,
		     IHXFileStatResponse* pResponse)
	: m_fow(fow)
	, m_status(status)
	, m_ulSize(ulSize)
	, m_ulCreationTime(ulCreationTime)
	, m_ulAccessTime(ulAccessTime)
	, m_ulModificationTime(ulModificationTime)
	, m_ulMode(ulMode)
	, m_pResponse(pResponse)
    {
	m_fow->AddRef();
    }
    ~StatDoneCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
    UINT32 m_ulSize;
    UINT32 m_ulCreationTime;
    UINT32 m_ulAccessTime;
    UINT32 m_ulModificationTime;
    UINT32 m_ulMode;
    IHXFileStatResponse* m_pResponse;
};

class PostDataCallback : public SimpleCallback
{
public:
    PostDataCallback(IHXBuffer* pBuffer, IHXPostDataHandler* pPost)
    	: m_pBuffer(pBuffer)
	, m_pPostDataHandler(pPost)
    {
	if (m_pBuffer)
	{
	    m_pBuffer->AddRef();
	}
	m_pPostDataHandler->AddRef();
    };
    ~PostDataCallback()
    {
	if (m_pBuffer)
	{
	    m_pBuffer->Release();
	}
	m_pPostDataHandler->Release();
    }
    void func(Process* p);

    IHXBuffer* m_pBuffer;
    IHXPostDataHandler* m_pPostDataHandler;
};

#endif
