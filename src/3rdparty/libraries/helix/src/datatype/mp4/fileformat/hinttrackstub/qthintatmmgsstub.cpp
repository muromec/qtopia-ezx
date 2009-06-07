/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 *  Defines
 */
#define QT_RTP_HEADER_SIZE	12


/****************************************************************************
 *  Includes
 */
#include "qtatmmgs.h"
#include "qttrkmgr.h"


/****************************************************************************
 *  Hint Reference Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_HintReference_Manager::CQT_HintReference_Manager(void)
    : m_pHintAtom(NULL)
    , m_ulTrackRefIdx(QT_BAD_IDX)
    , m_ulTrackID(0)
    , m_ulNumEntries(0)
{
    ;
}

CQT_HintReference_Manager::~CQT_HintReference_Manager()
{
    ;
}

/****************************************************************************
 *  Main Interface
 */
HX_RESULT CQT_HintReference_Manager::Init(CQTAtom* pAtom)
{ 
    return HXR_NOTIMPL;
}


/****************************************************************************
 *  Track Info Manager
 */
/****************************************************************************
 *  Public functions
 */
ULONG32 CQT_TrackInfo_Manager::GetPayloadType(void)
{
    return m_ulPayloadType;
}

ULONG32 CQT_TrackInfo_Manager::GetPayloadNameLength(void)
{
    return 0;
}

UINT8* CQT_TrackInfo_Manager::GetPayloadName(void)
{
    return NULL;
}

/****************************************************************************
 *  Private functions
 */
HX_RESULT CQT_TrackInfo_Manager::InitHinted(CQTAtom* pAtom,
					    CQT_SampleDescription_Manager* pSampleDescManager,
					    CQTTrackManager* pTrackManager,
					    CQT_MovieInfo_Manager* pMovieInfo)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQT_TrackInfo_Manager::CheckForcePacketization(CQT_sdp_Atom* pSDPAtom, 
							 IUnknown* pContext)
{
    return HXR_NOTIMPL;
}


/****************************************************************************
 *  Movie Info Manager
 */
/****************************************************************************
 *  Public Methods
 */
ULONG32 CQT_MovieInfo_Manager::GetSDPLength(void)
{
    return 0;
}

UINT8* CQT_MovieInfo_Manager::GetSDP(void)
{
    return NULL;
}

/****************************************************************************
 *  Private Methods
 */
HX_RESULT CQT_MovieInfo_Manager::ParseMovieHintInfo(CQTAtom* pAtom)
{
    return HXR_OK;
}
