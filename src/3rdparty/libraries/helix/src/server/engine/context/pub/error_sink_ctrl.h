/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: error_sink_ctrl.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _ERROR_SINK_CTRL_H_
#define _ERROR_SINK_CTRL_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "debug.h"
#include "proc.h"
#include "hxslist.h"
#include "error_sink_handler.h"

class ErrorSinkProc : public IHXErrorSinkControl {
public:
    	    	    	    ErrorSinkProc(Process* proc);

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(AddErrorSink)	(THIS_  IHXErrorSink*	pErrorSink,
					const UINT8	unLowSeverity,
					const UINT8	unHighSeverity);
    STDMETHOD(RemoveErrorSink)  (THIS_ IHXErrorSink*	pErrorSink);  

private:
    LONG32		    m_lRefCount;
    int			    m_nProcnum;
    Process*		    proc;
};

inline
ErrorSinkProc::ErrorSinkProc(Process* _proc) :
    m_lRefCount(0)
{
    proc = _proc;
    m_nProcnum = proc->procnum();
}

inline STDMETHODIMP_(ULONG32) 
ErrorSinkProc::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

inline STDMETHODIMP
ErrorSinkProc::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXErrorSinkControl))
    {
        AddRef();
        *ppvObj = (IHXErrorSinkControl*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

inline STDMETHODIMP_(ULONG32)
ErrorSinkProc::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    PANIC(("Warning: Last reference of IHXErrorSinkControl released\n"));
    return 0;
}

inline STDMETHODIMP
ErrorSinkProc::AddErrorSink(IHXErrorSink* pErrorSink,
			    const UINT8     unLowSeverity,
			    const UINT8     unHighSeverity)
{
    return proc->pc->error_sink_handler->AddErrorSink(pErrorSink,
	unLowSeverity, unHighSeverity, m_nProcnum);
}

inline STDMETHODIMP
ErrorSinkProc::RemoveErrorSink(IHXErrorSink* pErrorSink)
{
    return proc->pc->error_sink_handler->RemoveErrorSink(pErrorSink);
}

#endif /* _ERROR_SINK_CTRL_H_ */
