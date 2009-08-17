/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtpclsnc.h,v 1.5 2005/08/02 18:00:50 albertofloyd Exp $
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

#ifndef _RTPCLSNC_H_
#define _RTPCLSNC_H_

/****************************************************************************
 *  Defines
 */
#define RTPCL_ACCEPTABLE_SYNC_NOISE	3   // in ms


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxassert.h"
#include "ntptime.h"
#include "tconverter.h"

typedef _INTERFACE IHXTransportSyncServer  IHXTransportSyncServer;


/****************************************************************************
 *  CRTPClientStreamSync
 */
class CRTPClientStreamSync
{
public:
    /*
     *	Costructor/Destructor
     */
    CRTPClientStreamSync(void);

    ~CRTPClientStreamSync(); 


    /*
     *	Main Methods
     */
    HX_RESULT Init(ULONG32 ulHXAFactor,
		   ULONG32 ulRTPFactor,
		   IHXTransportSyncServer* pSyncServer = NULL,
		   HXBOOL bIsSyncMaster = FALSE,
		   ULONG32 ulAcceptableSyncNoise = 
				RTPCL_ACCEPTABLE_SYNC_NOISE);
    HX_RESULT Init(CHXTimestampConverter* pTSConverter,
		   IHXTransportSyncServer* pSyncServer = NULL,
		   HXBOOL bIsSyncMaster = FALSE,
		   ULONG32 ulAcceptableSyncNoise = 
				RTPCL_ACCEPTABLE_SYNC_NOISE);
    
    void Reset(void);
    void Close(void);

    HX_RESULT SetStartTime(ULONG32 ulRefHXStartTime,
			   HXBOOL bAsSecondaryRecipient = FALSE);
    HX_RESULT SetStartSync(ULONG32 ulRTPTime,
			   ULONG32 ulHXTime,
			   HXBOOL bIsNominalHXTime = FALSE,
			   ULONG32 ulRefHXStartTime = 0);
    HX_RESULT HandleMasterSync(ULONG32 ulHXTime, LONG32 lHXOffsetToMaster);
    HX_RESULT AnchorSync(ULONG32 ulHXTime, ULONG32 ulNTPHXTime);
    HX_RESULT HandleRTCPSync(NTPTime ntpTime, ULONG32 ulRTPTime);

    ULONG32 RTP2SyncRTP(ULONG32 ulRTPTime)
    {
	return (ulRTPTime +
		m_lSyncOffsetRTP +
		m_lOffsetToMasterRTP - 
		m_lTimeOffsetRTP);
    }

    ULONG32 RTP2SyncHX(ULONG32 ulRTPTime)
    {
	ULONG32 ulHXTime = ulRTPTime;

	if (m_pTSConverter)
	{
	    ulHXTime = m_pTSConverter->rtp2hxa(ulRTPTime);
	}

	ulHXTime += (ULONG32)(m_lSyncOffsetHX + 
		      m_lOffsetToMasterHX - 
		      m_lTimeOffsetHX);
	
	return ulHXTime;
    }

    HXBOOL IsSyncMaster(void)	{ return m_bIsSyncMaster; }
    HXBOOL IsStartSyncSet(void)   { return m_bStartSyncSet; }
    HXBOOL IsStronglySynced(void)	{ return m_bIsStronglySynced; }

    LONG32 GetSyncOffsetRTP(void)   { return m_lSyncOffsetRTP; }
    LONG32 GetSyncOffsetHX(void)   { return m_lSyncOffsetHX; }

private:
    void ReleaseTSConverter(void);

    CHXTimestampConverter* m_pTSConverter;
    HXBOOL m_bIsImportedTSConverter;

    HXBOOL m_bIsSyncMaster;

    HXBOOL m_bStartSyncSet;
    HXBOOL m_bIsStronglySynced;
    HXBOOL m_bNTPtoHXOffsetSet;
    LONG32 m_lNTPtoHXOffset;
    LONG32 m_lTimeOffsetRTP;
    LONG32 m_lTimeOffsetHX;
    LONG32 m_lSyncOffsetRTP;
    LONG32 m_lSyncOffsetHX;
    LONG32 m_lOffsetToMasterRTP;
    LONG32 m_lOffsetToMasterHX;
    HXBOOL m_bStartTimeSet;
    ULONG32 m_ulStartTime;
    ULONG32 m_ulAcceptableSyncNoise;

    IHXTransportSyncServer* m_pSyncServer;
};

#endif // _CPACEMKR_H_

