/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tconverter.h,v 1.3 2004/07/09 18:23:37 hubbe Exp $
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


#ifndef _TCONVERTER_H_
#define _TCONVERTER_H_

#ifdef HELIX_FEATURE_FLP_T_CONVERTER
#include "../tconverter/flpoint/tconverter_flp.h"
#define CHXTimestampConverter_IMPLEMENTER_CLASS CHXTimestampConverterFLP
#else	// TCONVERTER_FIXEDPOINT
#include "../tconverter/fxpoint/tconverter_fxp.h"
#define CHXTimestampConverter_IMPLEMENTER_CLASS CHXTimestampConverterFXP
#endif	// TCONVERTER_FIXEDPOINT


class CHXTimestampConverter
{
public:
    /* will use only one of conversion types */
    typedef enum
    {
	FACTORS = CHXTimestampConverter_IMPLEMENTER_CLASS::FACTORS,
	SAMPLES = CHXTimestampConverter_IMPLEMENTER_CLASS::SAMPLES
    } ConversionType;

    typedef CHXTimestampConverter_IMPLEMENTER_CLASS::ConversionFactors
	    ConversionFactors;

    /*
     * if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    void ReInit(ConversionType cvtType,
		UINT32 ulVal1,
		UINT32 ulVal2 = 0)
    {
	m_Impl.ReInit((CHXTimestampConverter_IMPLEMENTER_CLASS::ConversionType) cvtType, 
		      ulVal1, 
		      ulVal2);
    }

    void ReInit(ConversionFactors cvtFactors)
    {
	m_Impl.ReInit(cvtFactors);
    }

    /* default constructor */
    CHXTimestampConverter()
    {
	;  // nothing to do
    }
    
    /*
     * if CvtType == FACTORS, ulVal1 is RMA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    CHXTimestampConverter(ConversionType cvtType,
			  UINT32 ulVal1, 
			  UINT32 ulVal2 = 0)
	: m_Impl((CHXTimestampConverter_IMPLEMENTER_CLASS::ConversionType) cvtType, 
		 ulVal1, 
		 ulVal2)
    {
	;
    }

    CHXTimestampConverter(ConversionFactors cvtFactors)
	: m_Impl(cvtFactors)
    {
	;
    }

    /*
     * assignment
     */
    CHXTimestampConverter& operator=(const CHXTimestampConverter& lhs)
    {
	m_Impl = lhs.m_Impl;
	return *this;
    }
    
    /*
     * conversions
     */
    UINT32 hxa2rtp(UINT32 ulHxaTS)
    {
	return m_Impl.hxa2rtp(ulHxaTS);
    }

    UINT32 rtp2hxa(UINT32 ulRtpTS)
    {
	return m_Impl.rtp2hxa(ulRtpTS);
    }

    UINT32 hxa2rtp_raw(UINT32 ulHxaTS)
    {
	return m_Impl.hxa2rtp_raw(ulHxaTS);
    }

    UINT32 rtp2hxa_raw(UINT32 ulRtpTS)
    {
	return m_Impl.rtp2hxa_raw(ulRtpTS);
    }
    
    /*
     * anchors
     */
    void setHXAnchor(UINT32 ulHXAnchor)
    {
	m_Impl.setHXAnchor(ulHXAnchor);
    }

    void setRTPAnchor(UINT32 ulRTPAnchor)
    {
	m_Impl.setRTPAnchor(ulRTPAnchor);
    }

    void setAnchor(UINT32 ulHXAnchor, UINT32 ulRTPAnchor)
    {
	m_Impl.setAnchor(ulHXAnchor, ulRTPAnchor);
    }

    ULONG32 GetRTPAnchor(void)
    {
	return m_Impl.GetRTPAnchor();
    }

    ULONG32 GetHXAnchor(void)
    {
	return m_Impl.GetHXAnchor();
    }

    /*
     * factors
     */
    ConversionFactors GetConversionFactors(void)
    {
	return m_Impl.GetConversionFactors();
    }

private:

    CHXTimestampConverter_IMPLEMENTER_CLASS m_Impl;
};

#endif /* _TCONVERTER_H_ */

