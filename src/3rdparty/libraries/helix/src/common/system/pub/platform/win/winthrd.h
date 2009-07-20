/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winthrd.h,v 1.9 2007/07/06 20:42:03 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#include "hxtypes.h" //for STDMETHODCALLTYPE (win16 friendly)

#ifndef _HX_WIN_THREAD
#define _HX_WIN_THREAD

class HXWinThread : public HXThread
{
public:
    static ULONG32     GetCurrentThreadID();

    			HXWinThread	(void);
    virtual		~HXWinThread	(void);

    virtual HX_RESULT	CreateThread
					(void* (pExecAddr(void*)), 
					 void* pArg,
					 ULONG32 ulCreationFlags = 0);

    virtual HX_RESULT	Suspend		(void);
			    
    virtual HX_RESULT	Resume		(void);

    virtual HX_RESULT	SetPriority	(UINT32 ulPriority);

    virtual HX_RESULT	GetPriority	(UINT32& ulPriority);

    virtual HX_RESULT	YieldTimeSlice	(void);

    virtual HX_RESULT	Exit		(UINT32 ulExitCode); 

    virtual HX_RESULT	GetThreadId	(UINT32& ulThreadId);
    
    virtual void        SetThreadName(const char* pszName);

#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    virtual void	SetNetworkMessageConnection(conn* pConn);
    virtual HX_RESULT	PostNetworkMessage(HXThreadMessage* pMsg);

#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    
    virtual HX_RESULT	PostMessage(HXThreadMessage* pMsg, void* pWindowHandle = 0);

    virtual HX_RESULT	GetMessage(HXThreadMessage* pMsg, UINT32 ulMsgFilterMix = 0, UINT32 ulMsgFilterMax = 0);

    virtual HX_RESULT	DispatchMessage(HXThreadMessage* pMsg);


    struct HXWinThreadArgsAndAddr
    {
	void* m_pExecAddr;
	void* m_pArg;
	HXBOOL m_bThreadCanReceiveMessages;

	HXWinThreadArgsAndAddr() 
	    :	m_pExecAddr(0), 
		m_pArg(0),
		m_bThreadCanReceiveMessages(0)
	{
	}
    };

    static unsigned int STDMETHODCALLTYPE HXWinThreadStartRoutine(void *);
    HXWinThreadArgsAndAddr m_ArgsAndAddr;

private:

	    HXBOOL	ThreadCreated();

    ULONG32 m_ThreadHandle;
    unsigned int  m_ulThreadId;
    ULONG32 m_ulFlags;
};

class HXWinMutex : public HXMutex
{
public:
			HXWinMutex	(void);
    virtual		~HXWinMutex	(void);

    virtual HX_RESULT	Lock		(void);

    virtual HX_RESULT   Unlock		(void);

    virtual HX_RESULT   Trylock		(void);

private:

#ifdef _WIN32
    CRITICAL_SECTION	m_Mutex;
#endif
};

class HXWinNamedMutex : public HXMutex
{
public:
			HXWinNamedMutex	(char* name);
    virtual		~HXWinNamedMutex	(void);

    virtual HX_RESULT	Lock		(void);

    virtual HX_RESULT   Unlock		(void);

    virtual HX_RESULT   Trylock		(void);

private:

#ifdef _WIN32
    HANDLE	m_Mutex;
#endif
};

class HXWinEvent : public HXEvent
{
public:
			HXWinEvent	(const char* pEventName = NULL, 
					 HXBOOL bManualReset = TRUE);
    virtual		~HXWinEvent	(void);

    virtual HX_RESULT	SignalEvent	(void);

    virtual HX_RESULT	ResetEvent	(void);

    virtual void*	GetEventHandle	(void);

    virtual HX_RESULT	Wait		(UINT32 uTimeoutPeriod = ALLFS);

protected:

    HANDLE	m_Handle;
};

#endif /*_HX_WIN_THREAD*/
