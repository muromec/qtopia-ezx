/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ml_allowance_mgr.h,v 1.6 2005/04/26 03:55:13 jc Exp $ 
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

#ifndef _ML_ALLOWANCE_MGR_H_
#define _ML_ALLOWANCE_MGR_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxauth.h"
#include "proc.h"
#include "player.h"


class MLAllowanceMgr : public AllowanceMgr
{
public:
    STDMETHOD        (QueryInterface)    (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_        (ULONG32, AddRef)   (THIS);
    STDMETHOD_        (ULONG32, Release)  (THIS);
    STDMETHOD        (AllowanceMgrDone)  (THIS);

    MLAllowanceMgr(Process* pProc, 
                   Player::Session* pSession,
                   IHXPlayerConnectionAdviseSink* pPCAdviseSink);


    // *** IHXPlayerConnectionAdviseSink methods *** //

    STDMETHOD (OnConnection)            (THIS_ IHXPlayerConnectionResponse* pResponse);
    STDMETHOD (SetPlayerController) (THIS_ IHXPlayerController* pPlayerController);
    STDMETHOD (SetRegistryID)       (THIS_ UINT32 ulPlayerRegistryID);
    STDMETHOD (OnURL)                    (THIS_ IHXRequest* pRequest);
    STDMETHOD (OnBegin)                    (THIS);
    STDMETHOD (OnPause)                    (THIS);
    STDMETHOD (OnStop)                    (THIS);
    STDMETHOD (OnDone)                    (THIS);

    BOOL      IsPlaybackAllowed        ();

    STDMETHOD (OnConnectionDone)    (THIS_ HX_RESULT status);
    STDMETHOD (OnURLDone)            (THIS_ HX_RESULT status);
    STDMETHOD (OnBeginDone)            (THIS_ HX_RESULT status);
    STDMETHOD (OnStopDone)            (THIS_ HX_RESULT status);
    STDMETHOD (OnPauseDone)            (THIS_ HX_RESULT status);

    // added as part of Startup Optimization work
    STDMETHOD_ (void, PrintDebugInfo)  (THIS_);
    STDMETHOD_ (BOOL, AcceptMountPoint)  (THIS_ const char* pszMountPoint,
                                                INT32 uLen);

private:
    virtual ~MLAllowanceMgr();

    LONG32                          m_lRefCount;
    Player::Session*                m_pSession;
    IHXMidBoxNotify*               m_pMidBoxNotify;
};

#endif // _ALLOWANCE_MGR_H_
