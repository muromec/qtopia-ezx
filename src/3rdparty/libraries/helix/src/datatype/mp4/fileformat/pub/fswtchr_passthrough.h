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

#ifndef _FSWTCHR_PASSTHROUGH_H_
#define _FSWTCHR_PASSTHROUGH_H_

/****************************************************************************
 *  Includes
 */
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
class CFileSwitcherPassthrough : public IHXFileSwitcher,
				 public IHXFileResponse
{
public:
    /*
     *	Constructor/Destructor
     */
    CFileSwitcherPassthrough(void);

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

private:
    void Reset(void);
    void HandleFailureSync(void);

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

    FileSwitcherState m_State;

    IHXFileObject* m_pFileObject;
    IHXFileResponse* m_pResponse;
    LONG32 m_lRefCount;

    ~CFileSwitcherPassthrough();

};

#endif  // _FSWTCHR_PASSTHROUGH_H_
