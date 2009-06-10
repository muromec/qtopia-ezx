/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fres_wrap.h,v 1.2 2003/01/23 23:42:56 damonlan Exp $ 
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

#ifndef _FRES_WRAP_H_
#define _FRES_WRAP_H_

#include "hxfiles.h"

class FileResponseWrapper;
class FileObjectWrapper;


class InitDoneCallback : public SimpleCallback
{
public:
    InitDoneCallback(FileObjectWrapper* fow,
		     HX_RESULT status) :
	m_fow(fow), m_status(status)
    { 
	m_fow->AddRef();
    };
    ~InitDoneCallback()
    {
	m_fow->Release();
    }

    void func(Process* p);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
};

class CloseDoneCallback : public SimpleCallback
{
public:
    CloseDoneCallback(FileObjectWrapper* fow,
		      HX_RESULT status) :
	m_fow(fow), m_status(status)
    { 
	m_fow->AddRef();
    }
    ~CloseDoneCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);
    
    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
};

class ReadDoneCallback : public SimpleCallback
{
public:
    ReadDoneCallback(FileObjectWrapper* fow,
		     HX_RESULT status,
		     IHXBuffer* pBuffer) :
	m_fow(fow), m_status(status), m_pBuffer(pBuffer)
    {
	m_fow->AddRef();
	if((status == HXR_OK) && m_pBuffer)
	    m_pBuffer->AddRef();
	else
	    m_pBuffer = NULL;
    };

    ~ReadDoneCallback()
    {
	m_fow->Release();
	if(m_pBuffer)
	{
	    m_pBuffer->Release();
	}
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
    IHXBuffer* m_pBuffer;
};

class WriteDoneCallback : public SimpleCallback
{
public:
    WriteDoneCallback(FileObjectWrapper* fow,
		      HX_RESULT status) :
	m_fow(fow), m_status(status)
    {
	m_fow->AddRef();
    }
    ~WriteDoneCallback()
    {
	m_fow->Release();
    }
    void func(Process* p);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
};

class SeekDoneCallback : public SimpleCallback
{
public:
    SeekDoneCallback(FileObjectWrapper* fow,
		     HX_RESULT status) :
	m_fow(fow), m_status(status)
    {
	m_fow->AddRef();
    };
    ~SeekDoneCallback()
    {
	m_fow->Release();
    }
    void func(Process*);

    FileObjectWrapper* m_fow;
    HX_RESULT m_status;
};

class FileResponseWrapper : public IHXFileResponse
{
private:
    LONG32                      m_lRefCount;
    FileObjectWrapper*          m_fow;

public:
    FileResponseWrapper(FileObjectWrapper* fow);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    // *** IHXFileResponse methods ***
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT	    status);
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	status);
    STDMETHOD(ReadDone)		(THIS_ 
				HX_RESULT	    status,
				IHXBuffer*	    pBuffer);
    STDMETHOD(WriteDone)	(THIS_ 
				HX_RESULT	    status);
    STDMETHOD(SeekDone)		(THIS_ 
				HX_RESULT	    status);
};

#endif
