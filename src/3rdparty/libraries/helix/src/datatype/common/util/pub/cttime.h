/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cttime.h,v 1.3 2005/03/14 19:24:45 bobclark Exp $
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

#ifndef __CTTIME_H__
#define __CTTIME_H__

class CTrackTimeInfo
{
public:
    UINT32 m_ulTrackStartTime;
    UINT32 m_ulTrackEndTime;
    UINT32 m_ulDelay;
    INT32 m_lOffset;

    HXBOOL m_bForceTrackStartTime;
    HXBOOL m_bForceTrackEndTime;
    HXBOOL m_bForceTrackDelay;
    
    CTrackTimeInfo(CTrackTimeInfo*& pTimeInfo)
    {
	m_ulTrackStartTime = pTimeInfo->m_ulTrackStartTime;
	m_ulTrackEndTime = pTimeInfo->m_ulTrackEndTime;
	m_ulDelay = pTimeInfo->m_ulDelay;
	m_lOffset = pTimeInfo->m_lOffset;

	m_bForceTrackStartTime = pTimeInfo->m_bForceTrackStartTime;
	m_bForceTrackEndTime = pTimeInfo->m_bForceTrackEndTime;
	m_bForceTrackDelay = pTimeInfo->m_bForceTrackDelay;
    }
    CTrackTimeInfo(IHXValues* pHeaderValues)
	: m_bForceTrackStartTime(FALSE)
	, m_bForceTrackEndTime(FALSE)
	, m_bForceTrackDelay(FALSE)
    {
	m_ulTrackStartTime = 0;
	if (HXR_OK == pHeaderValues->GetPropertyULONG32("TrackStartTime", m_ulTrackStartTime))
	{
	    m_bForceTrackStartTime = TRUE;
	}

	m_ulDelay = 0;
	if (HXR_OK == pHeaderValues->GetPropertyULONG32("Delay", m_ulDelay))
	{
	    m_bForceTrackDelay = TRUE;
	}

	m_ulTrackEndTime = 0;
	if (HXR_OK == pHeaderValues->GetPropertyULONG32("TrackEndTime", m_ulTrackEndTime))
	{
	    m_bForceTrackEndTime = TRUE;
	}

	m_lOffset = m_ulTrackStartTime  - m_ulDelay;
    }

    UINT32 AdjustTime(UINT32 ulTime)
    {
	UINT32 ulRetVal = 0;
	if (m_lOffset < 0 && ulTime < (UINT32) (-m_lOffset))
	{
	    ulRetVal = 0;
	}
	else
	{
	    ulRetVal = ulTime + m_lOffset;
	}

	return ulRetVal;
    }
};
#endif /* _CTTIME__H_ */
