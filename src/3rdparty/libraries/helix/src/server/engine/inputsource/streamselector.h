/* ***** BEGIN LICENSE BLOCK *****  
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
#ifndef _STREAMSELECTOR_H_
#define _STREAMSELECTOR_H_

#include "ispifs.h"
#include "unkimp.h"

_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXUberStreamManager;
_INTERFACE IHXSessionStats;
_INTERFACE IHXQoSProfileConfigurator;
_INTERFACE IHXErrorMessages;
_INTERFACE IHXRateSelectionInfo;

/////////////////////////////////////////////////////////////////////////
// Class:
//  CStreamSelector
// Purpose:
//  Given an set of rate descriptions and some client characteristics, determines 
//  whether the rate descriptions are appropriate for stream selection/switching.
class CStreamSelector:
    public CUnknownIMP
{
    DECLARE_UNKNOWN(CStreamSelector)

public:

    // CStreamSelector methods
    CStreamSelector(); 
    virtual ~CStreamSelector();

    UINT32 GetMaxPreDecBufSize() {return m_selectionParam.m_ulMaxPreDecBufSize;}
    UINT32 GetPostDecBufPeriod() {return m_selectionParam.m_ulPostDecBufPeriod;}

    HX_RESULT Init(IHXUberStreamManager* pUberMgr, 
                   IUnknown* pContext, 
                   IHXSessionStats* pStats, 
                   IHXQoSProfileConfigurator* pConfig);

    HX_RESULT ExcludeUnusableStreams(THIS_ const StreamAdaptationParams* pStreamAdaptationParams = NULL);

    HX_RESULT SelectInitialRateDesc(IHXRateSelectionInfo* pRateSelInfo);
    HX_RESULT HandleRuleSubscriptions(IHXRateSelectionInfo* pRateSelInfo, 
                                      const UINT16 uiNumStreams,
                                      const UINT16*& pStreams);
    HX_RESULT HandleStreamLinkChar(IHXRateSelectionInfo* pRateSelInfo, 
                                   const UINT16 uiNumStreams,
                                   const UINT16*& pStreams);
    HX_RESULT HandleStreamRegistration(IHXRateSelectionInfo* pRateSelInfo, 
                                       const UINT16 uiNumStreams,
                                       const UINT16*& pStreams);
    HX_RESULT SetInitialRateDesc(UINT32 ulRate);
    HX_RESULT VerifyBandwidthGrouping(IHXRateDescription* pBandwidthGrouping);
    
private:
    HX_RESULT ExcludeBandwidthGroupings(UINT16 unStreamNum, UINT32 ulRate);
    HX_RESULT VerifyMaxTargetRate(void);

protected:

    // CStreamSelector methods
    HX_RESULT GetSelectionParams();
    HX_RESULT LogClientError();

    // Stream selection parameters */
    struct StreamSelectionParam
    {
	StreamSelectionParam()
	    : m_ulMaxTargetRate(0)
	    , m_ulMaxPreDecBufSize(0)
	    , m_ulInitTargetRate(0)
	{}	 
	UINT32	m_ulMaxTargetRate;    // bits/sec
	UINT32	m_ulMaxPreDecBufSize; // bytes
	UINT32  m_ulInitTargetRate; // bits/serc
	UINT32  m_ulPostDecBufPeriod; // 1/90000 sec
    } m_selectionParam;

    IHXUberStreamManager* m_pUberMgr;
    IHXSessionStats* m_pStats;
    IHXQoSProfileConfigurator* m_pQoSConfig;
    IHXErrorMessages* m_pErrorMsg;
    BOOL m_bDoRel6RateSelection;
    INT32 m_lDebugOutput;
};

#endif /* _STREAMSELECTOR_H_ */
