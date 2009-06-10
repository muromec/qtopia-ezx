/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsmstr.h,v 1.11 2007/07/06 21:58:16 jfinnecy Exp $
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

#ifndef _HXSMSTR_H_
#define _HXSMSTR_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxbdwdth.h" /* IHXBandwidthNegotiator */
#include "hxasm.h"    /* IHXASMStream2 */
#include "hxengin.h"  /* IXHCallback */
#include "hxmap.h"    /* CHXMapPtrToPtr */

class	ASMRuleBook;
class	HXStream;
class	HXSource;
class	CASMRuleState;
struct	IHXRegistry;
struct	IHXErrorMessages;

class HXASMStream : public IHXASMStream2,
		    public IHXStreamBandwidthNegotiator,
		    public IHXStreamBandwidthBias,
		    public IHXASMProps,
		    public IHXAtomicRuleGather
{
public:
    HXASMStream(HXStream* pStream, HXSource* pSource);
    ~HXASMStream();

    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    STDMETHOD(AddStreamSink)	(THIS_
				IHXASMStreamSink*	pASMStreamSink);
    
    STDMETHOD(RemoveStreamSink)	(THIS_
				IHXASMStreamSink*	pASMStreamSink);

    STDMETHOD(Subscribe)	(THIS_
				UINT16	uRuleNumber);

    STDMETHOD(Unsubscribe)	(THIS_
				UINT16	uRuleNumber);

    /*
     *	IHXASMStream2 methods
     */
    STDMETHOD(Disable)	        (THIS_
				UINT16	uRuleNumber);

    STDMETHOD(Enable)	        (THIS_
				UINT16	uRuleNumber);

    STDMETHOD(ReCompute)	(THIS);

    STDMETHOD_(HXBOOL,IsEnabled) (THIS_
				UINT16	uRuleNumber);

    STDMETHOD(LockSubscriptions) (THIS);

    STDMETHOD (UnlockSubscriptions) (THIS);

    STDMETHOD_(HXBOOL,AreSubscriptionsLocked)(THIS);


    /*
     *	IHXStreamBandwidthNegotiator methods
     */
    STDMETHOD(SetBandwidthUsage)	(THIS_
					REF(UINT32) ulRecvBitRate,
					REF(HXBOOL) bTimeStampDelivery);

    STDMETHOD(HandleSlowSource)         (THIS_
					UINT32 ulRecvBitRate);

    STDMETHOD(GetFixedBandwidth)	(THIS_
					REF(UINT32) ulBitRate);

    STDMETHOD(GetThresholdInfo)		(THIS_
					float*	    pThreshold,
					REF(UINT32) ulNumThreshold);

    STDMETHOD(UnRegister)		(THIS);

    STDMETHOD_(ULONG32,GetNumThresholds)(THIS);

    /*
     *	IHXStreamBandwidthBias methods
     */
    STDMETHOD(GetBiasFactor)	(THIS_
				REF(INT32) ulBiasFactor);

    STDMETHOD(SetBiasFactor)	(THIS_
				INT32 ulBiasFactor);

    /* IHXASMProps methods */

    STDMETHOD(GetPreData)	(THIS_
				REF(UINT32) ulPreData);

    STDMETHOD(GetBandwidth)	(THIS_
				REF(UINT32) ulBandwidth);

    STDMETHOD(RuleGather)	(THIS_
    				CHXSimpleList* pList);
    STDMETHOD(RuleFlush)	(THIS_
    				CHXSimpleList* pList);

    HX_RESULT	ResetASMSource	(IHXASMSource* pASMSource);

    HXBOOL	IsTimeStampDelivery() {return m_bTimeStampDeliveryMode;}

    void	PostEndTimePacket(IHXPacket* pPacket, HXBOOL& bSentMe, HXBOOL& bEndMe);

    void	ResetASMRuleState(void);

    class LossCheckCallback: public IHXCallback
    {
    public:
        LossCheckCallback(HXASMStream* pASMStream);

        STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
        STDMETHOD_(UINT32, AddRef ) (THIS);
        STDMETHOD_(UINT32, Release) (THIS);

        STDMETHOD(Func)             (THIS);

    private:
        HXASMStream*    m_pASMStream;
        LONG32          m_lRefCount;
        
        ~LossCheckCallback() {};
    };

    HX_BITFIELD		m_bTimeStampDeliveryMode : 1;
    HX_BITFIELD		m_bInitialSubscribe : 1;

private:
    typedef enum {resEnabled,
                  resDisabled,
                  resLocked} RuleEnableState;

    HX_BITFIELD		m_bHasExpression : 1;
    HX_BITFIELD		m_bEndOneRuleEndAll : 1;

    UINT32		m_ulLastLimitBandwidth;
    INT32		m_lRefCount;
    CHXMapPtrToPtr*     m_pStreamSinkMap;
    IHXValues*          m_pHeader;
    UINT16	        m_uStreamNumber;
    UINT16		m_nNumRules;
    HXSource*		m_pSource;
    IHXASMSource*      m_pASMSource;
    IHXRegistry*     m_pRegistry;
    IHXScheduler*      m_pScheduler;
    IHXCommonClassFactory* m_pCCF;
    IHXAtomicRuleChange* m_pAtomicRuleChange;
    ASMRuleBook*        m_pRuleBook;
    LossCheckCallback*  m_pLossCB;
    CallbackHandle      m_ulLossCBHandle;
    UINT32		m_ulBandwidthAllocation;
    UINT32		m_ulFixedBandwidth;
    HXBOOL*               m_pSubInfo;
    UINT32*		m_ulRuleBw;
    UINT32*		m_ulRulePreData;
    UINT32		m_ulCurrentPreData;
    UINT32		m_ulCurrentBandwidth;
    UINT32		m_ulLastBandwidth;
    HXBOOL*		m_bRuleTimeStampDelivery;
    UINT32		m_ulSubscribedBw;
    INT32		m_lBias;
    UINT32		m_bStartRecalc;
    UINT32              m_ulIDRecv;
    UINT32              m_ulIDLost;
    UINT32		m_ulIDClipBandwidth;
    char		m_szRecv[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
    char		m_szLost[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
    char		m_szClipBandwidth[MAX_DISPLAY_NAME]; /* Flawfinder: ignore */
    CHXSimpleList*	m_pSubList;
    IHXValues*		m_pSubscriptionVariables;

    HXBOOL*		m_pRuleSubscribeStatus;
    CASMRuleState*	m_pASMRuleState;
    RuleEnableState*    m_pRuleEnableState;
    HXBOOL                m_bSubsLocked;

    void Recalc();
    void RecalcCurrentProps();
    float ComputeLost();

#ifndef GOLD
    IHXErrorMessages*  m_pEM;
#endif

    friend class LossCheckCallback;
};

#endif  /* ifndef _HXSMSTR_H_ */
