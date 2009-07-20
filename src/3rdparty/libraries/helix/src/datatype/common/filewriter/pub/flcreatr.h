/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
#ifndef _FLCREATR_H_
#define _FLCREATR_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE	IHXFileCreatorResponse	    IHXFileCreatorResponse;


typedef enum
{
    FC_UNKNOWN,
    FC_INITIALIZING,
    FC_READY,
    FC_CREATING_DIR,
    FC_CREATING_FILE,
    FC_FINDING_FILE
} FC_STATUS;

/////////////////////////////////////////////////////////////////////////////
// 
//  Class: CFileCreator
//
//  Description: This class handles all of the details involved with archive
//    file creation. The caller simply asks an object of this class to
//    create a file or directory for archiving, and the CFileCreator will
//    take care of requesting the file from the file system, renaming any
//    file or directory which already exists under that name, and finally 
//    returning the file object in a response function call.
//
class CFileCreator : public IHXFileStatResponse,
		     public IHXDirHandlerResponse
{
public:
    CFileCreator(IHXFileCreatorResponse* pOwner, 
		 IUnknown* pContext,
		 HXBOOL bBlastFiles = FALSE);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID		riid,
	    				void**		ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXDirHandlerResponse methods
     */
    STDMETHOD(InitDirHandlerDone)	(THIS_
					HX_RESULT	status);
    STDMETHOD(CloseDirHandlerDone)	(THIS_
					HX_RESULT	status);
    STDMETHOD(MakeDirDone)		(THIS_ 
					HX_RESULT	status);
    STDMETHOD(ReadDirDone)		(THIS_ 
					HX_RESULT	status,
					IHXBuffer*	pBuffer);

    /*
     *	IHXFileStatResponse methods
     */
    STDMETHOD(StatDone)			(THIS_
					HX_RESULT status,
					UINT32 ulSize,
					UINT32 ulCreationTime,
					UINT32 ulAccessTime,
					UINT32 ulModificationTime,
					UINT32 ulMode);

    /*
     *  Public API
     */
    HX_RESULT	Init			();
    HX_RESULT	Done			();
    HX_RESULT	CreateArchiveDir	(const char* pName, HXBOOL bResolveConflict = FALSE);
    HX_RESULT	CreateArchiveFile	(const char* pName, HXBOOL bResolveConflict = TRUE);
    HX_RESULT	FindExistingFile	(const char* pName);
    HX_RESULT	FileSystemReady		(HX_RESULT   status);
    HX_RESULT	FileObjectReady		(HX_RESULT   status, 
					IUnknown* pUnknown);

    // Helper methods
    HX_RESULT	RemoveObject		(IHXFileObject* pObject);
    HX_RESULT	RenameObject		(IHXFileObject* pObject,
					 const char* pNewFileName = NULL,
					 HXBOOL bPathChanged = FALSE);
    HX_RESULT	MakeDirectory		(IHXFileObject* pObject);


    class FSMResponse : public IHXFileSystemManagerResponse
    {
    public:
	FSMResponse(CFileCreator* pOwner);
	virtual ~FSMResponse();

	/*
	 *  IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
					REFIID		riid,
					void**		ppvObj);
	STDMETHOD_(ULONG32,AddRef)	(THIS);
	STDMETHOD_(ULONG32,Release)	(THIS);

	/*
	 *  IHXFileSystemManagerResponse methods
	 */
	STDMETHOD(InitDone)		(THIS_
					HX_RESULT	status);
	STDMETHOD(FileObjectReady)	(THIS_
					HX_RESULT	status,
					IUnknown*	pObject);
	STDMETHOD(DirObjectReady)	(THIS_
					HX_RESULT	status,
					IUnknown*	pDirObject);

	LONG32			m_lRefCount;
	CFileCreator*		m_pOwner;
    };

private:
    virtual ~CFileCreator();


    LONG32				m_lRefCount;
    FC_STATUS				m_FCStatus;
    HXBOOL				m_bResolveConflict;
    HXBOOL				m_bBlastFiles;
    IHXFileCreatorResponse*		m_pOwner;
    IUnknown*				m_pContext;
    IHXErrorMessages*			m_pErrorMessages;
    IHXCommonClassFactory*		m_pClassFactory;
    IHXFileSystemManager*		m_pFSManager;
    IHXFileSystemManagerResponse*	m_pFSMResponse;
    IHXFileObject*			m_pFileObject;
    IHXDirHandler*			m_pDirHandler;
    HXBOOL				m_bFileSystemReady;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileCreatorResponse
 * 
 *  Purpose:
 * 
 *	Response interface for the Live Archiving file creator.
 * 
 *  IID_IHXFileCreatorResponse:
 * 
 *	{00002D00-0901-11D1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXFileCreatorResponse, 0x00002D00, 0x901, 0x11d1, 0x8b, 
	    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileCreatorResponse

DECLARE_INTERFACE_(IHXFileCreatorResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileCreatorResponse::InitDone
     *	Purpose:
     *	    Called in response to an Init call, indicating that the file
     *	    creator is ready to be used.
     */
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT   status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileCreatorResponse::ArchiveDirReady
     *	Purpose:
     *	    Called in response to a CreateArchiveDir call, with a status
     *	    indicating the success or failure of the operation.
     */
    STDMETHOD(ArchiveDirReady) (THIS_
				HX_RESULT   status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileCreatorResponse::ArchiveFileReady
     *	Purpose:
     *	    Called in response to a CreateArchiveFile call, with a status
     *	    indicating the success or failure of the operation.
     */
    STDMETHOD(ArchiveFileReady) (THIS_
				HX_RESULT	status,
				IHXFileObject* pFileObject) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileCreatorResponse::ExistingFileReady
     *	Purpose:
     *	    Called in response to a FindExistingFile call, with a status
     *	    indicating the success or failure of the operation.
     */
    STDMETHOD(ExistingFileReady) (THIS_
				 HX_RESULT	 status,
				 IHXFileObject* pFileObject) PURE;
};

#endif /* _FLCREATR_H_ */
