/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbianglobalvideoparameters.cpp,v 1.11 2009/03/30 20:39:53 praveenkumar Exp $
 *
 * Copyright Notices:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 *
 * Patent Notices: This file may contain technology protected by one or
 * more of the patents listed at www.helixcommunity.org
 *
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its
 * licensors, and made available by RealNetworks subject to the current
 * version of the RealNetworks Public Source License (the "RPSL")
 * available at  http://www.helixcommunity.org/content/rpsl unless
 * you have licensed the file under the current version of the
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its
 * licensors disclaim any implied patent license under the GPL.
 * If you wish to allow use of your version of this file only under
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 *
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing,
 * adapting or translating, any or all of this material other than
 * pursuant to the license terms referred to above requires the prior
 * written consent of RealNetworks and its licensors
 *
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributors: Nokia Inc
 *
 *
 * ***** END LICENSE BLOCK ***** */

#include "symbianglobalvideoparameters.h"


SymbianGlobalVideoParameters::SymbianGlobalVideoParameters():
  m_bDSAStatus( TRUE ),
  m_bAntiAlias( FALSE ),
  m_rotationValue( 0 ),
  m_bFrameRectValid( FALSE ),
  m_fHeightPerc( 100.0 ),
  m_bValidDisplayRegion( FALSE ),
  m_fWidthPerc( 100.0 )
#ifdef SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT
  ,m_lScreenNumber( 0 ),
  m_bScreenNumberSet( FALSE )
#endif //SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT
  ,m_ulScalingTypeValue(EMMFDefaultScaling)
#if defined(HELIX_FEATURE_SYMBIAN_GRAPHICS_SURFACES)
  ,m_bUseSurfaces(FALSE)
  ,m_ulDisplayIDArrayIndex(0)
#endif //HELIX_FEATURE_SYMBIAN_GRAPHICS_SURFACES
{
	memset(&m_clipRect, 0, sizeof(m_clipRect));
	memset(&m_windowRect, 0, sizeof(m_windowRect));
}

SymbianGlobalVideoParameters::~SymbianGlobalVideoParameters()
{
    m_ClipRegion.Close(); 
}

static void DestroySymbianGlobalVideoParameters(void* pObj )
{
    SymbianGlobalVideoParameters* pData = (SymbianGlobalVideoParameters*)pObj;
    delete pData;
}

SymbianGlobalVideoParameters* SymbianGlobalVideoParameters::Instance()
{
    SymbianGlobalVideoParameters* pRet = NULL;

    HXGlobalManager* pGM = HXGlobalManager::Instance();

    SymbianGlobalVideoParameters** pInstance = reinterpret_cast<SymbianGlobalVideoParameters**>(pGM->Get((const void *)SYMBIAN_GLOBAL_VIDEO_PARAMETERS_ID));

    if (!pInstance)
    {
        pRet = new SymbianGlobalVideoParameters();

        if (pRet)
        {
            pGM->Add( (GlobalID)SYMBIAN_GLOBAL_VIDEO_PARAMETERS_ID, (GlobalType)pRet, &DestroySymbianGlobalVideoParameters );
        }
    }
    else
    {
        pRet = *pInstance;
    }

    return pRet;
}

HX_RESULT SymbianGlobalVideoParameters::GetWindowParameters(HXxRect &clipRect, HXxRect &windowRect)
{
	HX_RESULT retVal = HXR_OK;
	clipRect = m_clipRect;
	windowRect = m_windowRect;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetWindowParameters(HXxRect clipRect, HXxRect windowRect)
{
	HX_RESULT retVal = HXR_OK;
	m_clipRect = clipRect;
	m_windowRect = windowRect;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::GetDisplayRegion(HXxRegion aRegion)
{
	HX_RESULT retVal = HXR_OK;

	RRegion* tempRegion;
	tempRegion  = (RRegion*)aRegion;

	if(tempRegion != NULL)
	{
        tempRegion->Clear();
        tempRegion->Copy(m_ClipRegion);
    }

	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetDisplayRegion(HXxRegion aRegion)
{

	HX_RESULT retVal = HXR_OK;
    // Fetch the clipping region.
    m_ClipRegion.Clear();
    m_ClipRegion.Copy(*(TRegion*)aRegion);
    m_bValidDisplayRegion = TRUE;

    return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::GetScalingParameters(HXFLOAT &fWidthPerc, HXFLOAT &fHeightPerc, HXBOOL &bAntiAlias)
{
	HX_RESULT retVal = HXR_OK;
	fWidthPerc  = m_fWidthPerc;
	fHeightPerc = m_fHeightPerc;
    bAntiAlias  = m_bAntiAlias;
    return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetScalingParameters(HXFLOAT fWidthPerc, HXFLOAT fHeightPerc, HXBOOL bAntiAlias)
{
	HX_RESULT retVal = HXR_OK;
	m_fWidthPerc  = fWidthPerc;
	m_fHeightPerc = fHeightPerc;
    m_bAntiAlias  = bAntiAlias;
    return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::GetDSAStatus(HXBOOL &bValue)
{
	HX_RESULT retVal = HXR_OK;
	bValue = m_bDSAStatus;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetDSAStatus(HXBOOL bValue)
{
	HX_RESULT retVal = HXR_OK;
	m_bDSAStatus = bValue;
	return retVal;
}

#ifdef SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT
HXBOOL SymbianGlobalVideoParameters::GetInitScreenNumber(INT32& lScreenNumber) const
{
	lScreenNumber = m_lScreenNumber;
	return m_bScreenNumberSet;
}

HX_RESULT SymbianGlobalVideoParameters::SetInitScreenNumber(INT32 lScreenNumber)
{
	m_bScreenNumberSet = TRUE;
	m_lScreenNumber = lScreenNumber;
	return HXR_OK;
}

HX_RESULT SymbianGlobalVideoParameters::ClearInitScreenNumber()
{
    m_bScreenNumberSet = FALSE;
    return HXR_OK;
}

#endif //SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT

HX_RESULT SymbianGlobalVideoParameters::GetRotationValue(UINT32 &ulValue)
{
	HX_RESULT retVal = HXR_OK;
	ulValue = m_rotationValue;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetRotationValue(UINT32 ulValue)
{
	HX_RESULT retVal = HXR_OK;
	m_rotationValue = ulValue;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::GetFrameRect(HXxRect &sValue)
{
	HX_RESULT retVal = HXR_OK;
	sValue = m_frameRect;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetFrameRect(HXxRect sValue)
{
	HX_RESULT retVal = HXR_OK;
	m_frameRect = sValue;
	m_bFrameRectValid = TRUE;
	return retVal;
}

HXBOOL SymbianGlobalVideoParameters::IsFrameRectValid(void)
{
	return m_bFrameRectValid;
}

HXBOOL SymbianGlobalVideoParameters::IsDisplayRegionValid(void)
{
	return(m_bValidDisplayRegion);
}

HX_RESULT SymbianGlobalVideoParameters::GetScalingTypeValue(UINT32 &ulValue)
{
	HX_RESULT retVal = HXR_OK;
	ulValue = m_ulScalingTypeValue;
	return retVal;
}

HX_RESULT SymbianGlobalVideoParameters::SetScalingTypeValue(UINT32 ulValue)
{
	HX_RESULT retVal = HXR_OK;
	m_ulScalingTypeValue = ulValue;
	return retVal;
}

#if defined(HELIX_FEATURE_SYMBIAN_GRAPHICS_SURFACES)
HX_RESULT SymbianGlobalVideoParameters::SetUseSurfaces()
{
	HX_RESULT retVal = HXR_OK;
	m_bUseSurfaces = TRUE;
	return retVal;
}

HXBOOL SymbianGlobalVideoParameters::GetUseSurfaces()
{
	return m_bUseSurfaces;
}

#endif //HELIX_FEATURE_SYMBIAN_GRAPHICS_SURFACES
