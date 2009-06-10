/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: stlicinf.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _ST_LIC_INFO_H_
#define _ST_LIC_INFO_H_

/*
 * this is a part of 
 * the per stream type licenses feature 
 * which used in conjunction with the distributed licensing feature
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxmon.h"

struct StreamTypeLicenseInfo
{
    // registry id for server.StreamType.Authentication
    // OR server.StreamType.AdServing, etc.
    UINT32 m_ulThereMaxLicRegId;  // reg id of max lic from the other side
    UINT32 m_ulThereConnsRegId;  // reg id of conns from the other side
    UINT32 m_ulMaxLicRegId;
    UINT32 m_ulConnsRegId;
    UINT32 m_ulNewConnsRegId;
    UINT32 m_ulDisconnectedConnsRegId;
    UINT32 m_ulTriggerRegId;
    UINT32 m_ulDeltaRegId;
    // reg id of the streamtype composite prop
    // eg.
    //     Server.StreamType.General
    //     Server.StreamType.Authentication.Commerce
    //     Server.StreamType.AdServing
    UINT32 m_ulStreamTypeRegId; 

    IHXRegistry* m_pRegistry;
    IHXRegistry2* m_pRegistry2;

    StreamTypeLicenseInfo(const StreamTypeLicenseInfo& pSTLI)
	: m_ulThereMaxLicRegId(pSTLI.m_ulThereMaxLicRegId)
	, m_ulThereConnsRegId(pSTLI.m_ulThereConnsRegId)
	, m_ulMaxLicRegId(pSTLI.m_ulMaxLicRegId)
	, m_ulConnsRegId(pSTLI.m_ulConnsRegId)
	, m_ulNewConnsRegId(pSTLI.m_ulNewConnsRegId)
	, m_ulDisconnectedConnsRegId(pSTLI.m_ulDisconnectedConnsRegId)
	, m_ulTriggerRegId(pSTLI.m_ulTriggerRegId)
	, m_ulDeltaRegId(pSTLI.m_ulDeltaRegId)
	, m_ulStreamTypeRegId(pSTLI.m_ulStreamTypeRegId)
	, m_pRegistry(pSTLI.m_pRegistry)
	, m_pRegistry2(pSTLI.m_pRegistry2)
    {
	if (m_pRegistry)
	    m_pRegistry->AddRef();
	if (m_pRegistry2)
	    m_pRegistry2->AddRef();
    };

    StreamTypeLicenseInfo(IHXRegistry* pRegistry, 
	IHXRegistry2* pRegistry2)
	: m_ulThereMaxLicRegId(0)
	, m_ulThereConnsRegId(0)
	, m_ulMaxLicRegId(0)
	, m_ulConnsRegId(0)
	, m_ulNewConnsRegId(0)
	, m_ulDisconnectedConnsRegId(0)
	, m_ulTriggerRegId(0)
	, m_ulDeltaRegId(0)
	, m_ulStreamTypeRegId(0)
	, m_pRegistry(pRegistry)
	, m_pRegistry2(pRegistry2)
    {
	if (pRegistry)
	    pRegistry->AddRef();
	if (m_pRegistry2)
	    m_pRegistry2->AddRef();
    };

    ~StreamTypeLicenseInfo()
    {
	HX_RELEASE(m_pRegistry);
	HX_RELEASE(m_pRegistry2);
    };

    INT32 GetMaxLic();
    INT32 GetConns();
    INT32 GetNewConns();
    INT32 GetDisconnectedConns();
    INT32 GetDelta();

    HX_RESULT SetMaxLic(INT32 lMaxLic);
    HX_RESULT SetConns(INT32 lConns);
    HX_RESULT SetNewConns(INT32 lConns);
    HX_RESULT SetDisconnectedConns(INT32 lConns);
    HX_RESULT SetDelta(INT32 lDelta);

    HX_RESULT AddToMaxLic(INT32 lMaxLicDelta, INT32& lNewVal);
    HX_RESULT AddToDelta(INT32 lDelta, INT32& lNewVal);
    HX_RESULT AddToConns(INT32 lConnsDelta, INT32& lNewVal); 
    HX_RESULT AddToNewConns(INT32 lConnsDelta, INT32& lNewVal); 
    HX_RESULT AddToDisconnectedConns(INT32 lConnsDelta, INT32& lNewVal); 

    HX_RESULT SubtractFromMaxLic(INT32 lMaxLicDelta, INT32& lNewVal);
    HX_RESULT SubtractFromDelta(INT32 lDelta, INT32& lNewVal);
    HX_RESULT SubtractFromConns(INT32 lConnsDelta, INT32& lNewVal);
    HX_RESULT SubtractFromNewConns(INT32 lConnsDelta, INT32& lNewVal);
    HX_RESULT SubtractFromDisconnectedConns(INT32 lConnsDelta, 
	INT32& lNewVal);

    INT32 CalcMasterDelta();
    INT32 CalcDroneDelta();

    HX_RESULT SetTrigger(); // set it to 1 to trigger the watch callbacks
};

inline INT32 
StreamTypeLicenseInfo::GetMaxLic()
{
    INT32 lMaxLic = 0;
    m_pRegistry->GetIntById(m_ulMaxLicRegId, lMaxLic);
    return lMaxLic;
}

inline INT32
StreamTypeLicenseInfo::GetConns()
{
    INT32 lConns = 0;
    m_pRegistry->GetIntById(m_ulConnsRegId, lConns);
    return lConns;
}

inline INT32
StreamTypeLicenseInfo::GetNewConns()
{
    INT32 lConns = 0;
    m_pRegistry->GetIntById(m_ulNewConnsRegId, lConns);
    return lConns;
}

inline INT32
StreamTypeLicenseInfo::GetDisconnectedConns()
{
    INT32 lConns = 0;
    m_pRegistry->GetIntById(m_ulDisconnectedConnsRegId, lConns);
    return lConns;
}

inline INT32 
StreamTypeLicenseInfo::GetDelta()
{
    INT32 lDelta = 0;
    m_pRegistry->GetIntById(m_ulDeltaRegId, lDelta);
    return lDelta;
}

inline HX_RESULT 
StreamTypeLicenseInfo::SetMaxLic(INT32 lMaxLic)
{
    return m_pRegistry->SetIntById(m_ulMaxLicRegId, lMaxLic);
}

inline HX_RESULT
StreamTypeLicenseInfo::SetConns(INT32 lConns)
{
    return m_pRegistry->SetIntById(m_ulConnsRegId, lConns);
}

inline HX_RESULT
StreamTypeLicenseInfo::SetNewConns(INT32 lConns)
{
    return m_pRegistry->SetIntById(m_ulNewConnsRegId, lConns);
}

inline HX_RESULT
StreamTypeLicenseInfo::SetDisconnectedConns(INT32 lConns)
{
    return m_pRegistry->SetIntById(m_ulDisconnectedConnsRegId, lConns);
}

inline HX_RESULT 
StreamTypeLicenseInfo::SetDelta(INT32 lDelta)
{
    return m_pRegistry->SetIntById(m_ulDeltaRegId, lDelta);
}

inline INT32 
StreamTypeLicenseInfo::CalcMasterDelta()
{
    return GetMaxLic() - GetConns();
}

inline INT32 
StreamTypeLicenseInfo::CalcDroneDelta()
{
    return GetConns() - GetMaxLic();
}

#endif /* _ST_LIC_INFO_H_ */

