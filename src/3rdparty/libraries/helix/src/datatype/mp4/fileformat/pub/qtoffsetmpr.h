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

#ifndef _QTOFFSETMPR_H_
#define _QTOFFSETMPR_H_

/****************************************************************************
 *  Includes
 */
#include "qttrkmgr.h"


/****************************************************************************
 * 
 *  Class:
 *	CQTOffsetToTimeMapper
 *
 *  Purpose:
 *	Provides mapping of file offset to playble file time.
 *      The mapping is constructed through analysis of chunk offset tables
 *  of active streams.  This algorithm covers broad range of cases and is
 *  fairly modest in memory and CPU consumption.
 *  The algroithm however, does not handle well temporally edited files
 *  via edit lists as edit lists are not considered during map creation.
 *  If edit lists are used, the map can error in both directions (overestimate
 *  or underestimate time corresponding to byte offset).
 *  All other cases should be handled with reasonable level of accuracy:
 *      - any interleaving or non-interleaved situation is handled precisely
 *      - temporally scrambled data chunks are handled precisely
 *      - presence of extraneous data embedded in media data chunks may 
 *        cause underestimation of time corresponding to byte offset.
 *        Overestimation is also possible but is kept in check by linearly
 *        interpolated time accross estimated media data span and
 *        is thus unlikely to occur.
 *      - multi-rate files or files containing hint tracks or other 
 *        non-rendering tracks may result in underestimation of time
 *        corresponding to byte offset.
 *	  Overestimation is also possible but is kept in check by linearly
 *        interpolated time accross estimated media data span and
 *        is thus unlikely to occur.
 */
class CQTOffsetToTimeMapper
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTOffsetToTimeMapper(void);
    ~CQTOffsetToTimeMapper();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTTrackManager* pTrackManager,
	           UINT32 ulFileDuration);
    HX_RESULT Map(UINT32 ulOffset, UINT32& ulTimeOut);
    
private:
    void Clear(void);

    inline UINT32 HXMulDiv(UINT32 left, UINT32 right, UINT32 bottom);

    static int compareOffsetMapEntry(const void *elem1, const void *elem2);
    

    typedef struct
    {
	UINT32 ulOffset;
	UINT16 uStream;
	UINT32 ulStreamTime;
	UINT32 ulOtherStreamsMinTime;
	HXBOOL bOutOfSequence;
    } QTOffsetMapEntry;

    UINT32 m_ulNumOffsets;
    QTOffsetMapEntry* m_pOffsetMap;
    QTOffsetMapEntry** m_pOffsetMapIndex;

    UINT16 m_uNumStreams;
    UINT32* m_pStreamAvgBitrate;

    UINT32 m_ulOffsetIdx;

    UINT32 m_ulFileDuration;
};

#endif  // _QTOFFSETMPR_H_
