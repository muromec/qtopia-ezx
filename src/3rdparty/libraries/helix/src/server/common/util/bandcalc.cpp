/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bandcalc.cpp,v 1.4 2003/03/08 20:56:12 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"

#include "bandcalc.h"


/*  AvgBandwidthCalc */

AvgBandwidthCalc::AvgBandwidthCalc()
{
    m_ulCurrentBucket = (UINT32)-1;
    memset(m_pBuckets, 0xff, sizeof(UINT32) * 10);
    m_bFirstRun = TRUE;
}

void
AvgBandwidthCalc::SentPacket(UINT32 ulSize, HXTimeval tNow)
{
    if (m_bFirstRun)
    {
	m_bFirstRun = FALSE;
	m_ulCurrentBucket = tNow.tv_sec % 10;
	m_pBuckets[m_ulCurrentBucket] = 0;
    }
    else
    {
        if (tNow.tv_sec - m_lasttvsecs >= 10)
        {
            memset(m_pBuckets, 0xff, sizeof(UINT32) * 10);
            m_ulCurrentBucket = tNow.tv_sec % 10;
            m_pBuckets[m_ulCurrentBucket] = 0;
        }
        else while (m_ulCurrentBucket != tNow.tv_sec % 10)
        {
	        m_ulCurrentBucket = (m_ulCurrentBucket + 1) % 10;
	        m_pBuckets[m_ulCurrentBucket] = 0;
        }
    }
    m_pBuckets[m_ulCurrentBucket] += ulSize;
    m_lasttvsecs = tNow.tv_sec;
}

UINT32
AvgBandwidthCalc::GetAverageBandwidth()
{
    UINT32 ulDataPoints = 10;
    UINT32 ulBandwidth = 0;

    for (UINT32 i = 0; i < 10; i++)
    {
	if (m_pBuckets[i] == (UINT32)-1 || m_ulCurrentBucket == i)
	{
	    ulDataPoints--;
	}
	else
	{
	    ulBandwidth += 8 * m_pBuckets[i];
	}
    }
    return ulDataPoints == 0 ? 0 : (ulBandwidth / ulDataPoints);
}
