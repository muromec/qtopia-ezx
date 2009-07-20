/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_process.h,v 1.4 2005/08/05 01:10:36 atin Exp $ 
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
/*
 *  server_process.h
 *
 *  Server's implementation of IHXProcess.  This object is meant to give server
 *  plugins the ability to create server processes (along with the shared memory).
 *  This object doesn't need to be used by the server, itself.  It can use the
 *  Process object.
 *
 */

#ifndef _SERVER_PROCESS_H_
#define _SERVER_PROCESS_H_

#include "proc.h"

class ServerHXProcess : public IHXProcess
{
public:
    ServerHXProcess(Process* pCopyProc);
    virtual ~ServerHXProcess();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)          (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)         (THIS);
    STDMETHOD_(ULONG32,Release)        (THIS);
    STDMETHOD(Start)                   (THIS_
                                       const char* pProcessName,
                                       IHXProcessEntryPoint* pEntryPoint);

private:
    ULONG32 m_ulRefCount;
    ULONG32 m_ulPid;
    Process* m_pCopyProc;
};

/***********************************************8
 *
 * ThreadLocalInfo
 *
 * Wrapper to provide IHXThreadLocal interface to plugins.
 */
class ThreadLocalInfo : public IHXThreadLocal
{
public:
    ThreadLocalInfo() : m_ulRefCount(0) {};

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)          (THIS_
                                       REFIID riid,
                                       void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)         (THIS);

    STDMETHOD_(ULONG32,Release)        (THIS);

    /*
     * IHXThreadLocal methods
     */

    STDMETHOD_(int, GetMaxThreads)	();
    STDMETHOD_(int, GetThreadNumber)	();

private:
    virtual ~ThreadLocalInfo()  {};

    UINT32		m_ulRefCount;
};

#endif /* _SERVER_PROCESS_H_ */
