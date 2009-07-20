/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dispatchq.h,v 1.6 2007/07/13 17:54:58 dcollins Exp $ 
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

#ifndef _DISPATCHQ_H_
#define _DISPATCHQ_H_

#include <stdio.h>
#ifdef _UNIX
#include <signal.h>
#include <fcntl.h>
#endif

#include "debug.h"

#include "proc.h"
#include "base_callback.h"
#include "simple_callback.h"
#include "mutex.h"

#if defined _WIN32
#include "kill.h"
#endif

#define QUEUE_SIZE  8192

// #define DISPATCHQ_RSS /* This prints DispatchQueue info in RSS */


class DispatchQueue {
public:
    DispatchQueue();
    ~DispatchQueue();

    void		init(Process* proc);
    int			send(Process* proc, SimpleCallback* m, int procnum);
    int			execute(Process* proc);
    int			cancel_crashed_func(Process* proc);

#ifdef DISPATCHQ_RSS
    void		printCounters(Process* proc, char*& pBuf);
#endif

    int                 count(int procnum) { return size[procnum]; }
    BOOL		has_dispatchq(int procnum) { return m_bHasDQ[procnum]; }

#if defined _LINUX && defined SHARED_FD_SUPPORT
    void              reset_trigger_flag(int procnum)
    {
      if (m_bSendTrigger[procnum] == 0)
          HXAtomicIncUINT32(&m_bSendTrigger[procnum]);
#ifdef DISPATCHQ_RSS
      m_pTriggersRcvd[procnum]++;
#endif
    }
#endif /* _LINUX */
private:
    SimpleCallback**	queues[MAX_THREADS];
    int			top[MAX_THREADS];
    int                 bottom[MAX_THREADS];
    int                 size[MAX_THREADS];
    HX_MUTEX		m_pMutex[MAX_THREADS];
    HX_MUTEX          m_pWriteMutex[MAX_THREADS];
    BOOL		m_bHasDQ[MAX_THREADS];
#if defined _LINUX && defined SHARED_FD_SUPPORT
    UINT32              m_bSendTrigger[MAX_THREADS];
#else
    BOOL              m_bExecuted[MAX_THREADS];
#endif /* _LINUX && SHARED_FD_SUPPORT */

#ifdef DISPATCHQ_RSS
    UINT32              m_pTriggersSent[MAX_THREADS];
    UINT32              m_pTriggersRcvd[MAX_THREADS];
    UINT32              m_pSendCounter[MAX_THREADS];
    UINT32              m_pRecvCounter[MAX_THREADS];
    UINT32              m_pExecCounter[MAX_THREADS];
    UINT64              m_ullSendTotal;
    UINT64              m_ullRecvTotal;
    UINT64              m_ullExecTotal;
#endif
};

#ifdef _UNIX
class DQTriggerCallback : public BaseCallback
{
public:
    DQTriggerCallback(Process* proc, int fd)
      : m_pProc(proc)
      , m_nPipeReadFD(fd)
      , m_nProcnum(0)
      , m_pDQ(0)
    {};
    STDMETHOD(Func)     (THIS)
    {
      char x;
      read(m_nPipeReadFD, &x, 1);
#if defined _LINUX && defined SHARED_FD_SUPPORT
      if (!m_nProcnum)
      {
          m_pDQ = m_pProc->pc->dispatchq;
          m_nProcnum = m_pProc->procnum();
      }
      m_pDQ->reset_trigger_flag(m_nProcnum);
#endif /* _LINUX && SHARED_FD_SUPPORT */
      return HXR_OK;
    }
private:
                        ~DQTriggerCallback() {};
    Process*          m_pProc;
    int                       m_nPipeReadFD;
    int                       m_nProcnum;
    DispatchQueue*    m_pDQ;
};
#endif /* _UNIX */
#endif
