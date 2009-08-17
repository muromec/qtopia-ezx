/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tsfob.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _TSFOB_H_
#define _TSFOB_H_

#include "hxengin.h"
#include "hxfiles.h"
#include "hxcache.h"

class Process;
class ThreadSafeFileResponseWrapper;
class CDistMIIStatistics;

class ThreadSafeFileObjectInfo
{
public:
    ULONG32 m_ulRefCount;
    Process* m_pProc;
    BOOL m_bDidLock;

    void AddRef() { m_ulRefCount++; }
    void Release() { m_ulRefCount--; if (m_ulRefCount == 0) delete this; }
};

class ThreadSafeFileObjectWrapper : public IHXFileObject,
				    public IHXThreadSafeMethods,
				    public IHXMIIReadStatCollection
{
private:
    ULONG32 m_ulRefCount;

    IHXFileObject*      	m_pFileObject;
    ThreadSafeFileObjectInfo*   m_pInfo;
    CDistMIIStatistics*		m_pMIIStats;

    ThreadSafeFileObjectWrapper(IUnknown* pFob, Process* pProc);
    ~ThreadSafeFileObjectWrapper();

    BOOL m_bThreadSafeRead;

public:

    static void MakeThreadSafe(REF(IUnknown*) pUnknown,
	Process* pProc);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

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

    STDMETHOD_(UINT32,IsThreadSafe)         (THIS);

    /*
     * IHXMIIReadStatCollection methods
     */
    STDMETHOD(SetMIIReadStatsEnabled)		(THIS_
						 BOOL bEnabled,
						 BOOL* bOldValue);

    STDMETHOD(GetMIIReadStatsEnabled)		(THIS_
						 REF(BOOL) bEnabled);

    friend class ThreadSafeFileResponseWrapper;
};


class ThreadSafeFileResponseWrapper : public IHXFileResponse
{
private:
    ~ThreadSafeFileResponseWrapper();

    ULONG32 m_ulRefCount;

    IHXFileResponse* m_pRealResponse;
    ThreadSafeFileObjectInfo*   m_pInfo;
    BOOL m_bThreadSafeReadDone;

public:
    ThreadSafeFileResponseWrapper(IHXFileResponse* pFres,
	ThreadSafeFileObjectInfo* pInfo);

    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(InitDone)     	(THIS_
                            	HX_RESULT       status);
    STDMETHOD(CloseDone)        (THIS_
                                HX_RESULT       status);
    STDMETHOD(ReadDone)         (THIS_
                                HX_RESULT           status,
                                IHXBuffer*         pBuffer);
    STDMETHOD(WriteDone)        (THIS_
                                HX_RESULT           status);
    STDMETHOD(SeekDone)         (THIS_
                                HX_RESULT           status);
};

#endif
