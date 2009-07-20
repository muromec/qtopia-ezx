/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timeline.h,v 1.13 2006/08/16 17:29:34 gwright Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _TIMELINE_
#define _TIMELINE_

// forward decl...
struct	IHXCallback;
struct	IHXInterruptState;

class	HXPlayer;
class	HXScheduler;
class   Timeline;

class   CHXMapLongToObj;
class	CHXMapPtrToPtr;

#ifdef _MACINTOSH
#ifndef _MAC_MACHO
#include <timer.h>
#endif

typedef struct TMInfo
{
    TMTask  tmTask;			// original and revised TM task record
    long    tmRefCon;		// space to pass application specific data
}TMInfo, *TMInfoPtr;

#endif

#if defined(_UNIX) || defined(_SYMBIAN) || defined(__TCS__)
#define CALLBACK
#endif

class Timeline
{
public:
		    Timeline(void);
		    ~Timeline();


    // Timeline will drive the indicated scheduler if specified.
    // Otherwise, timeline will drive a scheduler obtained from context.
    HX_RESULT	    Init(IUnknown* pContext, 
			 IHXScheduler* pScheduler = NULL, 
			 HXBOOL bUseDeferredTask = TRUE);
    HX_RESULT	    Pause(void);
    HX_RESULT	    Resume(void);
    HX_RESULT	    Seek(ULONG32 ulSeekTime);
    HX_RESULT	    SetStartTime(ULONG32 ulTime);
    HX_RESULT	    SetGranularity(ULONG32 ulNumMillisecs);

    HX_RESULT	    OnTimeSync(HXBOOL bAtInterrupt = FALSE);

    void	    Done(void);

#if defined(_WINDOWS) || defined(_SYMBIAN)
public:
    static void CALLBACK
		    NonMMTimerProc( 
			void*    hwnd,	// handle of window for timer messages 
			UINT32   uMsg,	// WM_TIMER message
			UINT32   idEvent,	// timer identifier
			ULONG32  dwTime 	// current system time
		    );
protected:
    UINT32		m_uiTimerID;
    UINT32		m_uiTimerIDFixup;
    IHXAsyncTimer*	m_pAsyncTimer;

#if !defined(HELIX_CONFIG_NOSTATICS)
    static CHXMapLongToObj  m_zTimerMap;
#else
    static const CHXMapLongToObj* const m_zTimerMap;
#endif
    static void KillTimerFixup(UINT  idTimer, struct TimerObjects*  pTimerObject);
#endif

#ifdef _MACINTOSH
public:
    static pascal void		DeferredTaskProc(long param);
    static pascal void		MacTimerProc(TMInfoPtr task);

    HX_BITFIELD			m_bUseDeferredTasks : 1;
    
    // m_bIsQuitting and m_bAnyCallbackPending need to be UINT32s because
    // emergency interrupt removal assumes it.
    UINT32			m_bIsQuitting;
    UINT32			m_bAnyCallbackPending;
    DeferredTask		m_DeferredTaskStruct;
    TMInfo			m_tmInfo;			  
    TimerUPP			m_uppTask;
    IHXInterruptState*		m_pInterruptState;
    
#ifdef THREADS_SUPPORTED
    IHXMutex*			m_pCoreMutex;
    void			SetCoreMutex(IHXMutex* pMutex) {m_pCoreMutex = pMutex; HX_ADDREF(m_pCoreMutex);};
#endif
    
    void InterruptTimeSync (void);

    struct GestaltDeferredStruct
    {
	UINT32* quitting;
	UINT32* pending;
	ProcessSerialNumber psn;
    };

#endif


#if defined(_UNIX) || defined(_MACINTOSH) || defined(__TCS__) || defined(_OPENWAVE)

public:
    static void CallAllTimeSyncs(void);

protected:
#if !defined(HELIX_CONFIG_NOSTATICS)
    static CHXMapPtrToPtr   m_zTimerMap;
#else
    static const CHXMapPtrToPtr* const m_zTimerMap;
#endif

#endif /* _UNIX || _MACINTOSH */

protected:
    ULONG32		m_ulGranularity;
    HX_BITFIELD		m_bIsTimerPending : 1;
    HX_BITFIELD		m_bPaused : 1;
    HX_BITFIELD		m_bTimerFixup : 1;
    HXPlayer*		m_pPlayer;

    IUnknown*		m_pContext;
    IHXScheduler*	m_pScheduler;	
    IHXScheduler2*	m_pScheduler2;

    ULONG32		m_ulLastCallbackTime;

public:
    void SetTimerFixup(HXBOOL val){m_bTimerFixup = val;};
protected:

};

struct TimerObjects
{
    Timeline*	m_pTimeline;
    UINT32	m_uiTimer;
};

#endif //_TIMELINE_
