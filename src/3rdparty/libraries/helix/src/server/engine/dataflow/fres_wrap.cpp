/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fres_wrap.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "proc.h"
#include "dispatchq.h"
#include "mutex.h"

#include "fsys_wrap.h"
#include "flob_wrap.h"
#include "fres_wrap.h"

FileResponseWrapper::FileResponseWrapper(FileObjectWrapper* fow) :
    m_fow(fow)
    , m_lRefCount(0)
{
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP FileResponseWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*)this;
	return HXR_OK;
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
STDMETHODIMP_(ULONG32) FileResponseWrapper::AddRef()
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
STDMETHODIMP_(ULONG32) FileResponseWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void
InitDoneCallback::func(Process* p)
{
    if(m_fow->m_pFileResponse)
	m_fow->m_pFileResponse->InitDone(m_status);
    delete this;
}

STDMETHODIMP
FileResponseWrapper::InitDone(HX_RESULT status)
{
    InitDoneCallback* idcb = new InitDoneCallback(m_fow, status);
    m_fow->m_myproc->pc->dispatchq->send(m_fow->m_myproc,
				      idcb,
				      m_fow->m_myproc->procnum());
    return HXR_OK;
}

void
CloseDoneCallback::func(Process* p)
{
    if(m_fow->m_pFileResponse)
	m_fow->CloseDone(m_status);
    delete this;
}

STDMETHODIMP
FileResponseWrapper::CloseDone(HX_RESULT	status)
{
    CloseDoneCallback* cdcb = new CloseDoneCallback(m_fow, status);
    m_fow->m_myproc->pc->dispatchq->send(m_fow->m_myproc,
				     cdcb,
				     m_fow->m_myproc->procnum());
    return HXR_OK;
}

void
ReadDoneCallback::func(Process* p)
{
    if(m_fow->m_pFileResponse)
	m_fow->m_pFileResponse->ReadDone(m_status, m_pBuffer);

    delete this;
}

STDMETHODIMP
FileResponseWrapper::ReadDone(HX_RESULT	    status,
				IHXBuffer*	    pBuffer)
{
    ReadDoneCallback* rdcb = new ReadDoneCallback(m_fow, status, pBuffer);
    m_fow->m_myproc->pc->dispatchq->send(m_fow->m_myproc,
				     rdcb,
				     m_fow->m_myproc->procnum());
    return HXR_OK;
}

void
WriteDoneCallback::func(Process* p)
{
    if(m_fow->m_pFileResponse)
	m_fow->m_pFileResponse->WriteDone(m_status);
    delete this;
}

STDMETHODIMP
FileResponseWrapper::WriteDone(HX_RESULT	    status)
{
    WriteDoneCallback* wdcb = new WriteDoneCallback(m_fow, status);
    m_fow->m_myproc->pc->dispatchq->send(m_fow->m_myproc,
				     wdcb,
				     m_fow->m_myproc->procnum());
    return HXR_OK;
}

void
SeekDoneCallback::func(Process* p)
{
    if(m_fow->m_pFileResponse)
	m_fow->m_pFileResponse->SeekDone(m_status);
    delete this;
}

STDMETHODIMP
FileResponseWrapper::SeekDone(HX_RESULT	    status)
{
    SeekDoneCallback* sdcb = new SeekDoneCallback(m_fow, status);
    m_fow->m_myproc->pc->dispatchq->send(m_fow->m_myproc,
				     sdcb,
				     m_fow->m_myproc->procnum());
    return HXR_OK;
}

