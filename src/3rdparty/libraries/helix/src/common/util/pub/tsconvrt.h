/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tsconvrt.h,v 1.10 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _TSCONVRT_H_
#define _TSCONVRT_H_

/****************************************************************************
 *  Defines
 */
#define TSCONVRT_INPUT	FALSE
#define TSCONVRT_OUTPUT	TRUE

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "tconverter.h"


/****************************************************************************
 *  CTSConverter
 */
class CTSConverter
{
public:
    CTSConverter(ULONG32 ulTicksPerSecondIn = 1000,
		 ULONG32 ulTicksPerSecondOut = 1000)
	: m_Converter(CHXTimestampConverter::FACTORS,
		      ulTicksPerSecondOut,
		      ulTicksPerSecondIn)
    {
	;
    }

    ~CTSConverter()
    {
	;
    }

    void SetBase(ULONG32 ulTicksPerSecondIn, 
		 ULONG32 ulTicksPerSecondOut)
    {
	m_Converter.ReInit(CHXTimestampConverter::FACTORS,
			   ulTicksPerSecondOut, 
			   ulTicksPerSecondIn);
    }

    void SetOffset(ULONG32 ulOffset, HXBOOL bUseOutScale = TRUE)
    {
	if (bUseOutScale)
	{
	    m_Converter.setHXAnchor(ulOffset);
	}
	else
	{
	    m_Converter.setRTPAnchor(ulOffset);
	}
    }

    ULONG32 GetOffset(HXBOOL bUseOutScale = TRUE)
    {
	if (bUseOutScale)
	{
	    return m_Converter.GetHXAnchor();
	}

	return m_Converter.GetRTPAnchor();
    }

    void SetAnchor(ULONG32 ulTicksIn, ULONG32 ulTicksOut)
    {
	m_Converter.setAnchor(ulTicksOut, ulTicksIn);
    }

    void Reset(void)
    {
	m_Converter.setAnchor(0, 0);
    }

    ULONG32 Convert(ULONG32 ulInTicks)
    {
	return m_Converter.rtp2hxa(ulInTicks);
    }

    ULONG32 ConvertVector(ULONG32 ulInTicks)
    {
	return m_Converter.rtp2hxa_raw(ulInTicks);
    }
	
private:
    CHXTimestampConverter m_Converter;
};

#endif	// _TSCONVRT_H_
