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

#ifndef _QTPACKETIZERFCT_H_
#define _QTPACKETIZERFCT_H_

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "ihxpckts.h"
#include "hxformt.h"


class CQT_TrackInfo_Manager;
class CQT_MovieInfo_Manager;
class CQTTrackManager;
class CQTTrack;


/****************************************************************************
 * 
 *  Class:
 *	CQTPacketizerFactory
 *
 *  Purpose:
 *	Implements Packetizer Factory
 */
class CQTPacketizerFactory
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTPacketizerFactory();

    virtual ~CQTPacketizerFactory();

    /*
     *	Main Interafce
     */
    virtual HX_RESULT Construct(IHXPayloadFormatObject* &pPacketizer,
				const char* pProtocol,
				CQT_TrackInfo_Manager* pTrackInfo,
				CQT_MovieInfo_Manager* pMovieInfo,
				CQTTrackManager* pTrackManager,
				CQTTrack* pTrack,
				IUnknown* pContext);
};

#endif  // _QTPACKETIZERFCT_H_
