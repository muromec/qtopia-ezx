/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tconverter_fxp.h,v 1.6 2008/09/23 20:37:58 anuj_dhamija Exp $
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


#ifndef _TCONVERT_FXP_H_
#define _TCONVERT_FXP_H_

/*****************************************************************************
 *  Includes
 */
#include "hxtypes.h"


/*****************************************************************************
 *  CHXTimestampConverterFXP
 */
class CHXTimestampConverterFXP
{
public:
    /* will use only one of conversion types */
    typedef enum
    {
	FACTORS,
	SAMPLES
    } ConversionType;

    typedef struct
    {
	ULONG32 ulBaseHXA;
	ULONG32 ulBaseRTP;
    } ConversionFactors;
    
    /*
     * constructors/destructor
     */
    CHXTimestampConverterFXP()
    {
	ReInit(FACTORS, 1, 1);
    }
    
    /*
     * if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    CHXTimestampConverterFXP(ConversionType cvtType,
			     UINT32 ulVal1, 
			     UINT32 ulVal2 = 0)
    {
	ReInit(cvtType, ulVal1, ulVal2);
    }

    CHXTimestampConverterFXP(ConversionFactors convFactors)
    {
	ReInit(FACTORS, convFactors.ulBaseHXA, convFactors.ulBaseRTP);
    }

    ~CHXTimestampConverterFXP()
    {
	;
    }


    /*
     * reinit
     */
    /*
     * if CvtType == FACTORS, ulVal1 is HXA Factor, ulVal2 is RTP Factor
     * if CvtType == SAMPLES, ulVal1 is Sample Rate
     */
    void ReInit(ConversionType cvtType, UINT32 ulVal1, UINT32 ulVal2 = 0);

    void ReInit(ConversionFactors cvtFactors)
    {
	ReInit(FACTORS, cvtFactors.ulBaseHXA, cvtFactors.ulBaseRTP);
    }

    /*
     * assignment
     */
    CHXTimestampConverterFXP& operator=(const CHXTimestampConverterFXP& lhs);
   
    /*
     * conversions
     */
    UINT32 hxa2rtp(UINT32 ulHxaTS)
    {
	return timeConvert(ulHxaTS, m_hxaTime, m_rtpTime);
    }

    UINT32 rtp2hxa(UINT32 ulRtpTS)
    {
	return timeConvert(ulRtpTS, m_rtpTime, m_hxaTime);
    }

    UINT32 hxa2rtp_raw(UINT32 ulHxaTS)
    {
	return timeConvertRaw(ulHxaTS, m_hxaTime, m_rtpTime);
    }

    UINT32 rtp2hxa_raw(UINT32 ulRtpTS)
    {
	return timeConvertRaw(ulRtpTS, m_rtpTime, m_hxaTime);
    }
    
    /*
     * anchors
     */
    void setHXAnchor(UINT32 ulHXAnchor);
    void setRTPAnchor(UINT32 ulRTPAnchor);
    void setAnchor(UINT32 ulHXAnchor, UINT32 ulRTPAnchor);

    ULONG32 GetRTPAnchor(void)
    {
	return m_hxaTime.ulAnchor;
    }

    ULONG32 GetHXAnchor(void)
    {
	return m_hxaTime.ulAnchor;
    }

    /*
     * factors
     */
    ConversionFactors GetConversionFactors(void)
    {
	ConversionFactors convFactors = {m_hxaTime.ulBase, m_rtpTime.ulBase};

	return convFactors;
    }

private:
    typedef struct
    {
	LONG32	lQ;	    // Quant			   - In Common Base
	LONG32	lR;	    // Remainder		   - In Native Base
	ULONG32 ulRef;	    // Reference (Last Time Stamp) - In Native Base
	ULONG32 ulAnchor;   // Anchor (Origin value)	   - In Native Base
	ULONG32 ulBase;	    // Native Base		   - In Ticks per Common Base
	ULONG32 ulHalfBase; // Native Base/2 (for rounding)- In Ticks per Common Base
    } TimeCoordinate;

    ULONG32 timeConvertRaw(ULONG32 ulT1, TimeCoordinate& T1, TimeCoordinate& T2)
    {

	if(T1.ulBase==0) //prevent divison by zero
	{
	return 0;
	}
	
	return (ulT1 / T1.ulBase) * T2.ulBase + 
	        ((ulT1 % T1.ulBase) * T2.ulBase + T1.ulHalfBase) / T1.ulBase;
    }

    ULONG32 timeConvert(ULONG32 ulT1, TimeCoordinate& T1, TimeCoordinate& T2);

    TimeCoordinate m_hxaTime;
    TimeCoordinate m_rtpTime;
};

#endif /* _TCONVERT_FXP_H_ */

