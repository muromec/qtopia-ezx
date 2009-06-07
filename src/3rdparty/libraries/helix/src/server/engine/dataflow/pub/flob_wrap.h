/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: flob_wrap.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#include "flob_wrap_callbacks.h"

#ifndef _FLOB_WRAP_H_
#define _FLOB_WRAP_H_

#include "hxfiles.h"

class Process;
class FileSystemWrapper;
class FileObjectWrapper;
class InitDoneCallback;

class FileObjectReleaseCallback;
class CreatedFileCallback;
class CreateFileCallback;
class InitFileCallback;
class SetURLCallback;
class ReadFileCallback;
class SeekFileCallback;
class GetPathCallback;
class GetFilenameCallback;
class SetRequestCallback;
class GetRequestCallback;
class WriteFileCallback;
class StatFileCallback;
class AdviseFileCallback;
class CloseFileCallback;
class GetFileObjectFromPoolCallback;
class DoesExistCallback;
class DoesExistDoneCallback;
class FindMimeTypeCallback;
class MimeTypeFoundCallback;
class FindBroadcastTypeCallback;
class BroadcastTypeFoundCallback;
class FileObjectReadyCallback;
class StatCallback;
class StatDoneCallback;

class FileObjectWrapper : public IHXFileObject,
			  public IHXFileExists,
			  public IHXFileMimeMapper,
			  public IHXBroadcastMapper,
			  public IHXFileExistsResponse,
			  public IHXFileMimeMapperResponse,
			  public IHXBroadcastMapperResponse,
			  public IHXGetFileFromSamePool,
			  public IHXGetFileFromSamePoolResponse,
			  public IHXRequestHandler,
			  public IHXFileStat,
			  public IHXFileStatResponse,
			  public IHXPostDataHandler
{
private:
    LONG32			m_lRefCount;

    ~FileObjectWrapper();

public:
    FileSystemWrapper*          m_parent;

    // Things various callbacks use
    VOLATILE int            m_created_file;
    VOLATILE int            m_got_request;
    VOLATILE int            m_got_filename;

    BOOL m_has_file_object;
    BOOL m_has_file_exists;
    BOOL m_has_file_mime_mapper;
    BOOL m_has_broadcast_mapper;
    BOOL m_has_pool_object;
    BOOL m_has_request_handler;
    BOOL m_has_post_data_handler;
    BOOL m_has_file_stat;

    IUnknown*            m_pObject;
    IHXFileObject*      m_pFileObject;
    IHXFileStat*	 m_pFileStat;
    IHXFileExists*      m_pFileExists;
    IHXBroadcastMapper* m_pBroadcastMapper;
    IHXFileMimeMapper*  m_pMimeMapper;
    IHXGetFileFromSamePool* m_pPoolObject;
    IHXRequestHandler*  m_pRequestHandler;
    IHXPostDataHandler*  m_pPostDataHandler;
    IHXRequest*         m_pRequest;

    IHXFileResponse*            m_pFileResponse;
    IHXFileExistsResponse*      m_pFileExistsResponse;
    IHXFileMimeMapperResponse*  m_pFileMimeMapperResponse;
    IHXBroadcastMapperResponse* m_pBroadcastMapperResponse;
    IHXGetFileFromSamePoolResponse* m_pPoolResponse;
    IHXFileStatResponse*	 m_pFileStatResponse;

    Process*            m_fs_proc;
    Process*            m_myproc;
    const char*         m_pPath;
    const char*         m_pFilename;

    FileObjectWrapper(FileSystemWrapper* parent,
		      Process* myproc,
		      Process* fs_proc);
    FileObjectWrapper(IUnknown*,
		      FileSystemWrapper* parent,
		      Process* myproc, Process* fs_proc);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXFileStat methods
     */

    STDMETHOD(Stat)		(THIS_
				IHXFileStatResponse* pFileStatResponse
				);

    /*
     *	IHXFileObject methods
     */
    STDMETHOD(Init)		(THIS_
				ULONG32		    /*IN*/	ulFlags,
				IHXFileResponse*   /*IN*/	pFileResponse);
    STDMETHOD(Close)	    	(THIS);
    STDMETHOD(Read)		(THIS_
    	    	    	    	ULONG32	    	    ulCount);
    STDMETHOD(Write)	    	(THIS_
    	    	    	    	IHXBuffer*	    pBuffer);
    STDMETHOD(Seek)		(THIS_
    	    	    	    	 ULONG32	    ulOffset,
				 BOOL               bRelative);
    STDMETHOD(Advise)	(THIS_
			ULONG32 ulInfo);

    STDMETHOD(GetFilename)      (THIS_
				 REF(const char*) pFilename);

    STDMETHOD(DoesExist) (THIS_
			  const char* pPath,
			  IHXFileExistsResponse* pResponse);
    STDMETHOD(FindMimeType) (THIS_
			     const char* pURL,
			     IHXFileMimeMapperResponse* pResponse);
    STDMETHOD(FindBroadcastType) (THIS_
				  const char* pURL,
				  IHXBroadcastMapperResponse* pResponse);

    STDMETHOD(DoesExistDone) (THIS_ BOOL bExist);
    STDMETHOD(MimeTypeFound) (THIS_ 
			      HX_RESULT status,
			      const char* pMimeType);
    STDMETHOD(BroadcastTypeFound) (THIS_
				   HX_RESULT status,
				   const char* pBroadcastType);

    /* IHXFileStatResponse */
    STDMETHOD(StatDone)		(THIS_
				 HX_RESULT status,
				 UINT32 ulSize,
				 UINT32 ulCreationTime,
				 UINT32 ulAccessTime,
				 UINT32 ulModificationTime,
				 UINT32 ulMode);

    STDMETHOD(GetFileObjectFromPool) (THIS_
				      IHXGetFileFromSamePoolResponse*);
    STDMETHOD(FileObjectReady) (THIS_
				HX_RESULT status,
				IUnknown* pObject);
    
    STDMETHOD(SetRequest)	(THIS_
				IHXRequest* pRequest);
    
    STDMETHOD(GetRequest)	(THIS_
				REF(IHXRequest*) pRequest);

    HX_RESULT Create();
    HX_RESULT AsyncCreate();
    HX_RESULT CloseDone(HX_RESULT status);

    /* IHXPostDataHandler */
    STDMETHOD(PostData) (THIS_ IHXBuffer* pBuffer);
};

#endif
