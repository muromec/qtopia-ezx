/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: stlicinf.cpp,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
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

#include "stlicinf.h"

// do something with the returned values from the ModifiedIntById()
HX_RESULT 
StreamTypeLicenseInfo::AddToMaxLic(INT32 lMaxLicDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulMaxLicRegId, 
	lMaxLicDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::AddToDelta(INT32 lDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulDeltaRegId, 
	lDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::AddToConns(INT32 lConnsDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulConnsRegId, 
	lConnsDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::AddToNewConns(INT32 lConnsDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulNewConnsRegId, 
	lConnsDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::AddToDisconnectedConns(INT32 lConnsDelta, 
    INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(
	m_ulDisconnectedConnsRegId, lConnsDelta, lNewVal, 0);
}

/*
 * all params to SubtractFromXXX() r assumed to b POSITIVE INTEGERS
 * so they r multiplied by -1 before adding them to the registry vars
 * via BoundedModifyXXX()
 */
HX_RESULT 
StreamTypeLicenseInfo::SubtractFromMaxLic(INT32 lMaxLicDelta, 
    INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulMaxLicRegId, 
	-1 * lMaxLicDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::SubtractFromDelta(INT32 lDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulDeltaRegId, 
	-1 * lDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::SubtractFromConns(INT32 lConnsDelta, INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulConnsRegId, 
	-1 * lConnsDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::SubtractFromNewConns(INT32 lConnsDelta, 
    INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(m_ulNewConnsRegId, 
	-1 * lConnsDelta, lNewVal, 0);
}

HX_RESULT 
StreamTypeLicenseInfo::SubtractFromDisconnectedConns(INT32 lConnsDelta, 
    INT32& lNewVal)
{
    return m_pRegistry2->BoundedModifyIntById(
	m_ulDisconnectedConnsRegId, -1 * lConnsDelta, lNewVal, 0);
}

// set trigger to 1 to fire off the watch callbacks
HX_RESULT 
StreamTypeLicenseInfo::SetTrigger()
{
    HX_RESULT ret = m_pRegistry->SetIntById(m_ulTriggerRegId, 1);
    return ret;
}
