/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cpacemkr.h,v 1.8 2006/03/08 19:13:40 ping Exp $
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

#ifndef _CPACEMKR_H_
#define _CPACEMKR_H_

/****************************************************************************
 *  Includes
 */
#include "hxpcmkr.h"


/****************************************************************************
 *  CVideoRenderer
 */
class CVideoPaceMaker : public IHXPaceMaker
{
public:
    /*
     *	Costructor/Destructor
     */
    CVideoPaceMaker(IUnknown* pContext);

    ~CVideoPaceMaker(); 

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /*
     *	IHXPaceMaker
     */
    STDMETHOD(Start)		(THIS_
				IHXPaceMakerResponse* pResponse,
				LONG32 lPriority,
				ULONG32 ulInterval,
				ULONG32 &ulId);

    STDMETHOD(Stop)		(THIS);

    STDMETHOD(Suspend)		(THIS_ 
				 HXBOOL bSuspend);

    STDMETHOD(Signal)		(THIS);

    STDMETHOD(WaitForStop)	(THIS);

    STDMETHOD(WaitForSuspend)	(THIS);

    STDMETHOD(SetPriority)	(THIS_
				LONG32 lPriority);

    STDMETHOD(SetInterval)	(THIS_
				ULONG32 ulInterval);

private:
    void OnThreadStart(void);
    void OnThreadEnd(void);
    inline HXBOOL IsActive(void)	    { return m_bActive; }
    inline HXBOOL IsSuspended(void)   { return m_bSuspended; }

    static void* ThreadRoutine(void* pArg);

    static LONG32 GetNextID() ;

    IUnknown* m_pContext;
    IHXPaceMakerResponse* m_pResponse;
    IHXThread* m_pThread;
    HXBOOL m_bActive;
    HXBOOL m_bThreadActive;
    HXBOOL m_bThreadIdle;
    HXBOOL m_bSuspend;
    HXBOOL m_bSuspended;
    ULONG32 m_ulBaseTime;
    ULONG32 m_ulTargetLeadTime;
    ULONG32 m_ulInterval;
    ULONG32 m_ulId;
    IHXEvent* m_pEvent;

    LONG32 m_lRefCount;
};

#endif // _CPACEMKR_H_

