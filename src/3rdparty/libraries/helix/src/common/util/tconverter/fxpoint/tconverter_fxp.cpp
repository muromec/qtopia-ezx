/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tconverter_fxp.cpp,v 1.5 2006/07/19 14:16:14 damann Exp $
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


/*****************************************************************************
 *  Includes
 */
#include "tconverter_fxp.h"

/*****************************************************************************
 *  CHXTimestampConverterFXP Class
 */
/*****************************************************************************
 *  ReInit
 *	if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
 *	if CvtType == SAMPLES, ulVal1 is Sample Rate
 */
void CHXTimestampConverterFXP::ReInit(ConversionType cvtType,
			     UINT32 ulVal1,
			     UINT32 ulVal2)
{
    if (cvtType == FACTORS)
    {
	m_hxaTime.ulBase = ulVal1;	// HXFactor
	m_rtpTime.ulBase = ulVal2;	// RTPFactor
    }
    else
    {
	m_hxaTime.ulBase = 1000;	// HXFactor = 1000 samples/sec
	m_rtpTime.ulBase = ulVal1;	// RTPFactor
    }

    m_hxaTime.lQ = 0;
    m_hxaTime.lR = 0;
    m_hxaTime.ulRef = 0;
    m_hxaTime.ulAnchor = 0;
    m_hxaTime.ulHalfBase = m_hxaTime.ulBase / 2;

    m_rtpTime.lQ = 0;
    m_rtpTime.lR = 0;
    m_rtpTime.ulRef = 0;
    m_rtpTime.ulAnchor = 0;
    m_rtpTime.ulHalfBase = m_rtpTime.ulBase / 2;
}

/*****************************************************************************
 *  timeConvert
 */
ULONG32 CHXTimestampConverterFXP::timeConvert(ULONG32 ulT1,
					      TimeCoordinate& T1,
					      TimeCoordinate& T2)
{
    LONG32 lSamples;
    LONG32 ldQ;
    LONG32 ldR;

    // Compute the change in samples (+ or -)
    lSamples = ulT1 - T1.ulRef;
    T1.ulRef = ulT1;

    // Compute the Quant in Common Base (+ or -)
    // and remainder in samples (always +)
    T1.lR += lSamples;	// Could become - at this moment

    ldQ = ((LONG32) (T1.lR - T1.ulBase)) / ((LONG32) T1.ulBase);
    ldR = ldQ * ((LONG32) T1.ulBase);
    T1.lR -= ldR;	// Always yields +
    T1.lQ += ldQ;	// + or -

    // Convert the number of samples accumulate from anchor point to
    // other timebase
    return ((ULONG32) (T2.ulAnchor +
		       (T1.lQ * T2.ulBase) +
		       (T1.lR * T2.ulBase + T1.ulHalfBase) / T1.ulBase));
}

/*****************************************************************************
 *  setAnchor
 */
void CHXTimestampConverterFXP::setHXAnchor(UINT32 ulHXAnchor)
{
    m_hxaTime.ulRef = m_hxaTime.ulAnchor = ulHXAnchor;
    m_rtpTime.ulRef = m_rtpTime.ulAnchor = timeConvertRaw(ulHXAnchor, m_hxaTime, m_rtpTime);
    m_rtpTime.lQ = m_rtpTime.lR = 0;
    m_hxaTime.lQ = m_hxaTime.lR = 0;
}

void CHXTimestampConverterFXP::setRTPAnchor(UINT32 ulRTPAnchor)
{
    m_rtpTime.ulRef = m_rtpTime.ulAnchor = ulRTPAnchor;
    m_hxaTime.ulRef = m_hxaTime.ulAnchor = timeConvertRaw(ulRTPAnchor, m_rtpTime, m_hxaTime);
    m_rtpTime.lQ = m_rtpTime.lR = 0;
    m_hxaTime.lQ = m_hxaTime.lR = 0;
}

void CHXTimestampConverterFXP::setAnchor(UINT32 ulHXAnchor, UINT32 ulRTPAnchor)
{
    m_hxaTime.ulRef = m_hxaTime.ulAnchor = ulHXAnchor;
    m_rtpTime.ulRef = m_rtpTime.ulAnchor = ulRTPAnchor;
    m_rtpTime.lQ = m_rtpTime.lR = 0;
    m_hxaTime.lQ = m_hxaTime.lR = 0;
}

/*****************************************************************************
 *  assignment
 */
CHXTimestampConverterFXP& CHXTimestampConverterFXP::operator=(const CHXTimestampConverterFXP& lhs)
{
    m_hxaTime = lhs.m_hxaTime;
    m_rtpTime = lhs.m_rtpTime;

    return *this;
}


