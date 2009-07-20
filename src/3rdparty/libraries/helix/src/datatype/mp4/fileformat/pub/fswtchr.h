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

#ifndef _FSWTCHR_H_
#define _FSWTCHR_H_


/****************************************************************************
 *  Defines
 */
#define FSWTCHR_READ_ALL	0xFFFFFFFF

#ifdef QTCONFIG_FSWITCHER

/****************************************************************************
 *  Includes
 */
#include "hlxclib/string.h"
#include "hxfswch.h"
#include "hxcomm.h"


/****************************************************************************
 * 
 *  Class:
 *	CFileSwitcher
 *
 *  Purpose:
 *	Enables use of multiple files by multiple client objects.
 *
 */
class CFileSwitcher :	public IHXFileSwitcher,
			public IHXFileResponse,
			public IHXGetFileFromSamePoolResponse,
			public IHXCallback
{
public:
    /*
     *	Constructor/Destructor
     */
    CFileSwitcher(void);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXFileSwitcher methods
     */
    STDMETHOD(Init)	(THIS_
			IHXFileObject* pFileObject,
			ULONG32 ulFlags,
			IHXFileResponse* pResponse,
			IUnknown* pContext,
			UINT16 uMaxChildCount);

    STDMETHOD(Read)	(THIS_
			ULONG32 ulSize,
			IHXFileResponse* pResponse,
			const char* pFileName = NULL);

    STDMETHOD(Write)	(THIS_
			IHXBuffer* pBuffer,
			IHXFileResponse* pResponse,
			const char* pFileName = NULL);

    STDMETHOD(Seek)	(THIS_
			ULONG32 ulOffset,
			HXBOOL bRelative,
			IHXFileResponse* pResponse,
			const char* pFileName = NULL);

    STDMETHOD(Close)	(THIS_
			IHXFileResponse *pResponse,
			const char* pFileName = NULL);

    STDMETHOD(Advise)	(THIS_
			ULONG32 ulInfo,
			const char* pFileName = NULL);

    /*
     *	IHXFileResponse methods
     */
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT status);

    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT status);

    STDMETHOD(ReadDone)		(THIS_ 
				HX_RESULT status,
				IHXBuffer* pBuffer);

    STDMETHOD(WriteDone)	(THIS_ 
				HX_RESULT status);

    STDMETHOD(SeekDone)		(THIS_ 
				HX_RESULT status);

    /*
     *	IHXGetFileFromSamePoolResponse method
     */
    STDMETHOD(FileObjectReady)	(THIS_
				HX_RESULT status,
				IUnknown* pUnknown);

   /*
    *  IHXCallback methods
    */
   STDMETHOD(Func)		(THIS);

private:
    typedef enum
    {
	FSWCHR_Offline,
	FSWCHR_Ready,
	FSWCHR_ProcInit,
	FSWCHR_ProcRead,
	FSWCHR_ProcWrite,
	FSWCHR_ProcSeek,
	FSWCHR_ProcClose,
	FSWCHR_ProcAdvise
    } FileSwitcherState;

    class FileHandle
    {
    public:
	FileHandle(void)
	    : m_pFileName(NULL)
	    , m_pFileObject(NULL) {;}

	char* m_pFileName;
	IHXFileObject *m_pFileObject;

	void Clear(void)
	{
	    HX_VECTOR_DELETE(m_pFileName);
	    HX_RELEASE(m_pFileObject);
	}

	const char* SetName(const char* pFileName)
	{
	    ULONG32 ulNameSize;

	    HX_VECTOR_DELETE(m_pFileName);

	    if (pFileName)
	    {
		ulNameSize = (ULONG32) (strlen(pFileName) + 1);

		m_pFileName = new char [ulNameSize];
		if (m_pFileName)
		{
		    memcpy(m_pFileName, pFileName, ulNameSize); /* Flawfinder: ignore */
		}
	    }

	    return m_pFileName;
	}

	~FileHandle()
	{
	    HX_VECTOR_DELETE(m_pFileName);
	    HX_RELEASE(m_pFileObject);
	}
    };

    inline void Reset(void);
    HX_RESULT HandleFailureAsync(HX_RESULT status);
    HX_RESULT HandleFailureSync(HX_RESULT status);
    HX_RESULT FileHandleReady(void);
    HX_RESULT GetFileHandle(const char* pFileName);
    HX_RESULT CloseFileHandleObject(FileHandle *pFileHandle);
    inline FileHandle* FindFileHandle(const char* pFileName);
    inline FileHandle* SelectStaleHandle(void);
    inline HX_RESULT ProcessRead(void);
    inline HX_RESULT ReadNextFragment(void);

    HXBOOL m_bSyncMode;

    UINT16 m_uMaxChildCount;
    UINT16 m_uCurrentChildCount;
    UINT16 m_uLastDisownedChild;
    FileHandle* m_pHandleTable;

    ULONG32 m_ulFlags;
    ULONG32 m_ulSize;
    ULONG32 m_ulChunkSize;
    ULONG32 m_ulProcessedSize;
    ULONG32 m_ulFragmentCount;
    HXBOOL m_bRelative;
    IHXBuffer* m_pBuffer;

    FileHandle* m_pCurrentHandle;

    IHXFileResponse* m_pResponse;
    IHXCommonClassFactory* m_pClassFactory;
    IHXScheduler* m_pScheduler;

    FileSwitcherState m_State;
    HXBOOL m_bClosing;
    HX_RESULT m_CloseStatus;

    LONG32 m_lRefCount;

    ~CFileSwitcher();

};

#else	// QTCONFIG_FSWITCHER

#include "fswtchr_passthrough.h"

#endif	// QTCONFIG_FSWITCHER

#endif  // _FSWTCHR_H_
