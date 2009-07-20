/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxconnbwinfo.h,v 1.14 2007/07/06 21:58:11 jfinnecy Exp $
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
#ifndef HXCONNBWINFO_H
#define HXCONNBWINFO_H

#include "hxcom.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxpreftr.h"
#include "unkimp.h"
#include "chxmapptrtoptr.h"
#include "hxslist.h"
#include "hxmovavg.h"
#include "hxmovmed.h"
#include "hxsmbw.h"
#include "hxthread.h"
#include "hxnetifsinkhlpr.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "statinfo.h"

class HXConnectionBWInfo : public CUnknownIMP,
                           public IHXConnectionBWInfo,
                           public IHXAutoBWCalibrationAdviseSink,
                           public IHXStatistics
{
public:
    static HX_RESULT Create(IUnknown* pContext, IUnknown* pOuter, IUnknown*& pObj);
    /*
     *  IUnknown methods
     */
    DECLARE_UNKNOWN_NOCREATE(HXConnectionBWInfo)
        
    /*
     *  IHXConnectionBWInfo methods
     */
    STDMETHOD(AddABDInfo)(THIS_ IHXAutoBWDetection* pABD,
                          IHXPreferredTransport* pPrefTransport);
    STDMETHOD(RemoveABDInfo)(THIS_ IHXAutoBWDetection* pABD);

    STDMETHOD(GetConnectionBW)(THIS_ REF(UINT32) uBw,
                               HXBOOL bDetectedBWOnly);

    STDMETHOD(AddSink)   (THIS_ IHXConnectionBWAdviseSink* pSink);
    STDMETHOD(RemoveSink)(THIS_ IHXConnectionBWAdviseSink* pSink);

    /*
     *  IHXAutoBWCalibrationAdviseSink methods
     */
    STDMETHOD(AutoBWCalibrationStarted) (THIS_ const char* pszServer);
    STDMETHOD(AutoBWCalibrationDone)    (THIS_ HX_RESULT  status, UINT32 uBW);

    /*
     *  IHXStatistics methods
     */
    STDMETHOD (InitializeStatistics) (THIS_ UINT32  /*IN*/ ulRegistryID);
    STDMETHOD (UpdateStatistics)     (THIS);

protected:
    friend class HXConnBWInfoABDSink;

    void AutoBWDetectDone(IHXAutoBWDetection* pABD, 
                          HX_RESULT           status,
                          UINT32               uBW);

    HX_RESULT NetInterfacesUpdated();
    static HX_RESULT static_NetInterfacesUpdated(void* pObj);

private:
    HXConnectionBWInfo();
    ~HXConnectionBWInfo();

    HX_RESULT init(IUnknown* pContext);
    HX_RESULT createABDSink(IHXAutoBWDetection* pABD);
    void removeABDSink(IHXAutoBWDetection* pABD);
    void clearTransportMap();
    void clearSinkMap();
    HXBOOL needToRecalc();
    void recalc();
    void handleNewABDData(IHXAutoBWDetection* pABD, UINT32 uBW);
    void handleABDNotSupported(IHXAutoBWDetection* pABD);
    void handleABDError(IHXAutoBWDetection* pABD, HX_RESULT status);
    void dispatchSinkCalls(UINT32 uBW);
    UINT32 prefTransportAvg();
    inline IHXPreferredTransport* getTransport(IHXAutoBWDetection* pABD) const;
    HX_RESULT updateBWStats(UINT32 ulBW);

    typedef enum {
        acsInit,
        acsPending,
        acsError
    } ABDCalibrateState;
    
    CHXMapPtrToPtr                 m_abdToPrefTransportMap;
    CHXMapPtrToPtr                 m_abdToSinkMap;
    IUnknown*                      m_pContext;
    IHXPreferredTransportManager*  m_pPrefTranMgr;
    IHXAutoBWCalibration*          m_pABDCal;
    IHXBandwidthManager*           m_pBWManager;
    HXNetInterfaceAdviseSinkHelper m_netIFSinkHlpr;
    UINT32                         m_uConnectionBW;
    UINT32                         m_bIsDetectedBW;
    UINT32                         m_uCachedBwPref;
    HXBOOL                         m_bNeedRecalc;
    CStatisticEntry*               m_pEstimatedBWStats;
    ABDCalibrateState              m_abdCalState;
    UINT32                         m_uABDCalibrationBW;
    IHXMutex*                      m_pMutex;
    CHXSimpleList                  m_sinkList;
    LISTPOSITION                   m_sinkDispatchItr;
    HXMovingMedian                 m_dynamicABDMed;
    HXBOOL                         m_bGotDynamicABDSample;
    HXMovingAverage                m_connectionBWAvg;
};

inline 
IHXPreferredTransport* 
HXConnectionBWInfo::getTransport(IHXAutoBWDetection* pABD) const
{
    IHXPreferredTransport* pRet = NULL;
    void* pTmp = NULL;

    if (m_abdToPrefTransportMap.Lookup((void*)pABD, pTmp))
    {
        pRet = (IHXPreferredTransport*)pTmp;
        if (pRet)
        {
            pRet->AddRef();
        }
    }

    return pRet;
}

#endif /* HXCONNBWINFO_H */
