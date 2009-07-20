/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: packetq.h,v 1.4 2005/08/02 18:00:50 albertofloyd Exp $
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

#ifndef _PACKETQ_H_
#define _PACKETQ_H_

// #define _PKTQ_DEBUG

class PacketQueue
{
public:
    /****** Public Class Methods ******************************************/
    PacketQueue(const UINT32 ulWinSize = 25,
		const UINT16 unProbation = 2,
		const UINT32 ulWinTime = 0,
		const HXBOOL bUsesRTPPackets = FALSE);
    ~PacketQueue();

    HX_RESULT	Init(IHXCommonClassFactory* pClassFactory);

    HX_RESULT	AddPacket(UINT16 unSeq, 
			  IHXPacket* pPacket,
			  ULONG32 ulArrivalTime = 0);
    HX_RESULT	GetPacket(REF(IHXPacket*) pPacket, ULONG32 ulTimeNow = 0);
    HX_RESULT	GetNextTS(REF(UINT32)ulTS, ULONG32 ulTimeNow = 0);    
    UINT16	GetPercentDone(void);
    ULONG32	GetAge(ULONG32 ulTimeNow);


    /****** inlines ******/
    inline void	    SetMinWindowSize(UINT32 ulWinSize);
    inline void	    SetMinWindowTime(UINT32 ulWinTime);
    inline UINT32   GetMinWindowSize(void);
    UINT32 GetMinWindowTime(void)   { return m_ulMinWindowTime; }
    inline UINT32   GetLatePktCount(void);    
    inline UINT32   GetQueuedPktCount(void);
    inline HXBOOL	    IsUntouched(void)	{ return m_bInitial; }
    inline void	    SetFlexTimeWindow(HXBOOL bIsFlexTimeWindow);
    inline HXBOOL	    IsFlexTimeWindow(void)  { return m_bIsFlexTimeWindow; }
        
private:
    class CArrivedPacket
    {
    public:
	CArrivedPacket(IHXPacket* pPacket,
		       ULONG32 ulArrivalTS)
	: m_pPacket(pPacket)
	, m_ulArrivalTS(ulArrivalTS)
	{
	    if (pPacket)
	    {
		pPacket->AddRef();
	    }
	}

	~CArrivedPacket()
	{
	    HX_RELEASE(m_pPacket);
	}

	IHXPacket* m_pPacket;
	ULONG32 m_ulArrivalTS;
    };

    void	    ReInitVars(void);
    inline HXBOOL	    IsSeqNumGT(UINT16 seq1, UINT16 seq2)
    {
	return (((INT16) (seq1 - seq2)) > 0);
    }
    HXBOOL	    PacketSufficientlyAged(CArrivedPacket* pArrivedPacket,
					   ULONG32 ulTimeNow);
    inline HXBOOL	    IsBufferingForSure(void)
    {
	HXBOOL bIsBufferingForSure = FALSE;

	if (m_ulMinWindowTime == 0)
	{
	    bIsBufferingForSure = TRUE;
	    if (((UINT32) m_pBuf->GetCount()) >= m_ulMinWindowSize)
	    {
		bIsBufferingForSure = FALSE;
	    }
	}

	return bIsBufferingForSure;
    }

    CHXMapLongToObj* m_pBuf;

    const UINT16    m_unInitProbation;
    UINT32	    m_ulMinWindowSize;
    UINT32	    m_ulMinWindowTime;	    
    UINT32	    m_ulLate;
    UINT32	    m_ulLateSinceTermination;
    HXBOOL	    m_bIsFlexTimeWindow;
    HXBOOL	    m_bUsesRTPPackets;

    ULONG32	    m_ulLastReturnedArrivalTime;
    UINT16	    m_unLastReturnedArrivalSeq;
    HXBOOL	    m_bLastReturnedArrivalSet;

    UINT16	    m_unCurrent;
    HXBOOL	    m_bPacketReturned;
    IHXCommonClassFactory* m_pClassFactory;    

    HXBOOL	    m_bInitial;
    UINT16	    m_unProbation;
#ifdef _PKTQ_DEBUG
    FILE*	    m_pLogFile;
#endif
};


inline void	
PacketQueue::SetMinWindowSize(UINT32 ulWinSize)
{
#ifdef _PKTQ_DEBUG
    if (m_pLogFile)
    {
	fprintf(m_pLogFile, "**** Setting WindowSize: %u\n", ulWinSize);
	fflush(stdout);
    }	
#endif    
    m_ulMinWindowSize = ulWinSize;
}

inline void	
PacketQueue::SetMinWindowTime(UINT32 ulWinTime)
{
#ifdef _PKTQ_DEBUG
    if (m_pLogFile)
    {
	fprintf(m_pLogFile, "**** Setting WindowTime: %u\n", ulWinTime);
	fflush(stdout);
    }	
#endif    
    m_ulMinWindowTime = ulWinTime;
}

inline void 
PacketQueue::SetFlexTimeWindow(HXBOOL bIsFlexTimeWindow)
{
#ifdef _PKTQ_DEBUG
    if (m_pLogFile)
    {
	fprintf(m_pLogFile, "**** Setting Flex TimeWindow: %u\n", bIsFlexTimeWindow);
	fflush(stdout);
    }	
#endif	// _PKTQ_DEBUG

    m_bIsFlexTimeWindow = bIsFlexTimeWindow; 
}

inline UINT32
PacketQueue::GetMinWindowSize(void)
{
    return m_ulMinWindowSize;
}

inline UINT32   
PacketQueue::GetLatePktCount(void)
{
    return m_ulLate;
}

inline UINT32
PacketQueue::GetQueuedPktCount(void)
{
    return (UINT32)(m_pBuf ? m_pBuf->GetCount() : 0);
}

#endif 
