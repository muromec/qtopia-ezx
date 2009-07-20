/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxmedpltfmsched.h,v 1.7 2007/07/06 21:58:21 jfinnecy Exp $
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

#ifndef _CHXMEDPLTFMSCHED_H_
#define _CHXMEDPLTFMSCHED_H_

#include "unkimp.h"
#include "hxengin.h"
#include "hxsched.h"

class CHXMediaPlatformKicker;

class CHXMediaPlatformScheduler : public CUnknownIMP
				, public IHXScheduler
				, public IHXScheduler2
{
protected:
    UINT32			m_ulThreadID;
    HXScheduler*		m_pScheduler;
    CHXMediaPlatformKicker*	m_pKicker;

    HX_RESULT	_InternalQIMutex(REFIID riid, void** ppvObj);

    friend class CHXMediaPlatformKicker;

public:
    CHXMediaPlatformScheduler(void);
    ~CHXMediaPlatformScheduler(void);

    DECLARE_UNKNOWN_NOCREATE(CHXMediaPlatformScheduler);
    DECLARE_COM_CREATE_FUNCS(CHXMediaPlatformScheduler)

    /*
     * IHXScheduler methods
     */
    STDMETHOD_(CallbackHandle,RelativeEnter)	    (THIS_
						    IHXCallback* pCallback,
						    UINT32 ms);

    STDMETHOD_(CallbackHandle,AbsoluteEnter)	    (THIS_
						    IHXCallback* pCallback,
						    HXTimeval tVal);

    STDMETHOD(Remove)				    (THIS_
			    			    CallbackHandle Handle);

    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime)   (THIS);

    /*
     *	IHXScheduler2 methods
     */
    STDMETHOD(StartScheduler)			    (THIS);
    STDMETHOD(StopScheduler)			    (THIS);
    STDMETHOD(OnTimeSync)			    (THIS_
						    HXBOOL bAtInterrupt);
    STDMETHOD(SetInterrupt)			    (THIS_
						    HXBOOL bInterruptenable);
    STDMETHOD_(HXBOOL, IsAtInterruptTime)	    (THIS);
    STDMETHOD(SetMutex)				    (THIS_
						    IHXMutex* pMutex);
    STDMETHOD(GetMutex)				    (THIS_
						    REF(IHXMutex*) pMutex);
    STDMETHOD(NotifyPlayState)			    (THIS_
						    HXBOOL bInPlayingState);
    STDMETHOD_(HXBOOL, GetNextEventDueTimeDiff)	    (THIS_
						    ULONG32 &ulEarliestDueTimeDiff);
    STDMETHOD(WaitForNextEvent)			    (THIS_
						    ULONG32 ulTimeout);
    STDMETHOD_(ULONG32, GetThreadID)		    (THIS);
    STDMETHOD_(HXBOOL, AreImmediatesPending)	    (THIS);

    HX_RESULT	Init(IUnknown* pContext);
    HX_RESULT	AttachKicker(CHXMediaPlatformKicker* pKicker);
    HX_RESULT	DetachKicker(CHXMediaPlatformKicker* pKicker);
};

#endif /* _CHXMEDPLTFMSCHED_H_ */
