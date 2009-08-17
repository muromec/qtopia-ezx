/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tconverter_flp.h,v 1.4 2004/07/09 18:23:41 hubbe Exp $
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


#ifndef _TCONVERTER_FLP_H_
#define _TCONVERTER_FLP_H_

/*****************************************************************************
 *  Defines
 */
#define MAX_ULONG32_AS_DOUBLE	(4294967295.0)
#define RANGE_ULONG32_AS_DOUBLE	(4294967296.0)

/*****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxassert.h"


/*****************************************************************************
 *  CHXTimestampConverterFLP
 */
class CHXTimestampConverterFLP
{
public:
    /* will use only one of conversion types */
    enum ConversionType
    {
	FACTORS,
	SAMPLES
    };

    typedef struct
    {
	ULONG32 ulBaseHXA;
	ULONG32 ulBaseRTP;
    } ConversionFactors;

    void ReInit(double fHX2RTPFactor)
    {
	m_ulHXAnchor = 0;
	m_ulRTPAnchor = 0;
	m_ulLastRTPTS = 0;
	m_ulLastHXTS = 0;
	m_llFatRTPTS = 0;
	m_llFatHXTS = 0;
	m_fAnchorDeltaInRTP = 0.0;
	m_fFactor = fHX2RTPFactor;
	m_ulHxaFactor = 0;
	m_ulRtpFactor = 0;
    }

    /*
     * if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    void ReInit(ConversionType cvtType,
		UINT32 ulVal1,
		UINT32 ulVal2 = 0)
    {
	double fHX2RTPFactor;
	ULONG32 ulHxaFactor;
	ULONG32 ulRtpFactor;

	if (cvtType == FACTORS)
	{
	    ulHxaFactor = ulVal1;
	    ulRtpFactor = ulVal2;
	}
	else
	{
	    ulHxaFactor = 1000;	    // SamplesPerSecond
	    ulRtpFactor = ulVal1;   // SamplesPerSecond
	}

	fHX2RTPFactor = ((double) ulRtpFactor) / ((double) ulHxaFactor);	

	ReInit(fHX2RTPFactor);

	m_ulHxaFactor = ulHxaFactor;
	m_ulRtpFactor = ulRtpFactor;
    }

    void ReInit(ConversionFactors cvtFactors)
    {
	ReInit(FACTORS, cvtFactors.ulBaseHXA, cvtFactors.ulBaseRTP);
    }

    /* default constructor */
    CHXTimestampConverterFLP()
	: m_fFactor(1.0)
	, m_ulHXAnchor(0)
	, m_ulRTPAnchor(0)
	, m_ulLastRTPTS(0)
	, m_ulLastHXTS(0)
	, m_llFatRTPTS(0)
	, m_llFatHXTS(0)
	, m_fAnchorDeltaInRTP(0.0)
	, m_ulHxaFactor(1)
	, m_ulRtpFactor(1)
    {
	;  // nothing to do
    }
    
    /*
     * if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    CHXTimestampConverterFLP(ConversionType cvtType,
			     UINT32 ulVal1, 
			     UINT32 ulVal2 = 0)
    {
	ReInit(cvtType, ulVal1, ulVal2);
    }

    CHXTimestampConverterFLP(ConversionFactors convFactors)
    {
	ReInit(FACTORS, convFactors.ulBaseHXA, convFactors.ulBaseRTP);
    }

    CHXTimestampConverterFLP(double fHXA2RTPFactor)
    {
	ReInit(fHXA2RTPFactor);
    }

    /* assignment */
    CHXTimestampConverterFLP& operator=(const CHXTimestampConverterFLP& lhs)
    {
	m_fFactor  = lhs.m_fFactor;
	m_ulHXAnchor = lhs.m_ulHXAnchor;
	m_ulRTPAnchor = lhs.m_ulRTPAnchor;
	m_ulLastRTPTS = lhs.m_ulLastRTPTS;
	m_ulLastHXTS = lhs.m_ulLastHXTS;
	m_llFatRTPTS = lhs.m_llFatRTPTS;
	m_llFatHXTS = lhs.m_llFatHXTS;
	m_fAnchorDeltaInRTP = lhs.m_fAnchorDeltaInRTP;

	return *this;
    }
    
    ULONG32 doubleToULONG32(double fVal)
    {
	if (fVal >= 0.0)
	{
	    return ((ULONG32) (fVal + 0.5));
	}
	else
	{
	    return ((ULONG32) (fVal - 0.5));
	}
    }

    /*
     * conversions
     */
    UINT32 hxa2rtp(UINT32 ulHxaTS)
    {
	ULONG32 ulRTPTS;
	LONG32 lHXDelta = (LONG32) (ulHxaTS - m_ulLastHXTS);

	m_ulLastHXTS = ulHxaTS;
	m_llFatHXTS += lHXDelta;

	ulRTPTS = doubleToULONG32((INT64_TO_DOUBLE(m_llFatHXTS  - m_ulHXAnchor)) *
				  m_fFactor);
	ulRTPTS += m_ulRTPAnchor;

	return ulRTPTS;
    }

    UINT32 rtp2hxa(UINT32 ulRtpTS)
    {
	ULONG32 ulHXTS;
	LONG32 lRTPDelta = (LONG32) (ulRtpTS - m_ulLastRTPTS);

	m_ulLastRTPTS = ulRtpTS;
	m_llFatRTPTS += lRTPDelta;

	ulHXTS = doubleToULONG32((INT64_TO_DOUBLE(m_llFatRTPTS - m_ulRTPAnchor)) /
				  m_fFactor);
	ulHXTS += m_ulHXAnchor;

	return ulHXTS;
    }

    UINT32 hxa2rtp_raw(UINT32 ulHxaTS)
    {
	return ((ULONG32) (ulHxaTS * ((double) m_fFactor)) + 0.5);
    }

    UINT32 rtp2hxa_raw(UINT32 ulRtpTS)
    {
	return ((ULONG32) (ulRtpTS / ((double) m_fFactor)) + 0.5);
    }
    
    /*
     * anchors
     */
    void setHXAnchor(UINT32 ulHXAnchor)
    {
	m_ulHXAnchor = ulHXAnchor;
	m_ulRTPAnchor = doubleToULONG32(((double) m_ulHXAnchor) * m_fFactor +
					m_fAnchorDeltaInRTP);
		     
	m_ulLastHXTS = m_ulHXAnchor;
	m_ulLastRTPTS = m_ulRTPAnchor;
	m_llFatHXTS = m_ulLastHXTS;
	m_llFatRTPTS = m_ulLastRTPTS;
    }

    void setRTPAnchor(UINT32 ulRTPAnchor)
    {
	m_ulRTPAnchor = ulRTPAnchor;
	m_ulHXAnchor = doubleToULONG32((((double) m_ulRTPAnchor) + m_fAnchorDeltaInRTP) / 
					m_fFactor);

	m_ulLastHXTS = m_ulHXAnchor;
	m_ulLastRTPTS = m_ulRTPAnchor;
	m_llFatHXTS = m_ulLastHXTS;
	m_llFatRTPTS = m_ulLastRTPTS;
    }

    void setAnchor(UINT32 ulHXAnchor, UINT32 ulRTPAnchor)
    {
	m_ulHXAnchor = ulHXAnchor;
	m_ulRTPAnchor = ulRTPAnchor;

	m_ulLastHXTS = m_ulHXAnchor;
	m_ulLastRTPTS = m_ulRTPAnchor;
	m_llFatHXTS = m_ulLastHXTS;
	m_llFatRTPTS = m_ulLastRTPTS;

	m_fAnchorDeltaInRTP = ((double) m_ulRTPAnchor) - 
			      ((double) m_ulHXAnchor) * m_fFactor;
    }

    ULONG32 GetRTPAnchor(void)
    {
	return m_ulRTPAnchor;
    }

    ULONG32 GetHXAnchor(void)
    {
	return m_ulHXAnchor;
    }

    /*
     * factors
     */
    ConversionFactors GetConversionFactors(void)
    {
	ConversionFactors convFactors = {m_ulHxaFactor, m_ulRtpFactor};

	return convFactors;
    }

    double GetHX2RTPFactor(void)
    {
	return m_fFactor;
    }

private:
    double m_fFactor;
    ULONG32 m_ulHXAnchor;
    ULONG32 m_ulRTPAnchor;
    double m_fAnchorDeltaInRTP;
    ULONG32 m_ulLastRTPTS;
    ULONG32 m_ulLastHXTS;
    INT64 m_llFatRTPTS;
    INT64 m_llFatHXTS;

    ULONG32 m_ulHxaFactor;
    ULONG32 m_ulRtpFactor;
};

#endif /* _TCONVERTER_FLP_H_ */

