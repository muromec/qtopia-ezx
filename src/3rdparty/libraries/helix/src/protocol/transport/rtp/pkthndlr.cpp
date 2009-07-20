/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pkthndlr.cpp,v 1.12 2009/03/17 16:40:10 jgordon Exp $
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

#include "hxtypes.h"	
#include "hxresult.h"	
#include "hxcom.h"  
#include "hlxclib/string.h"
#include "ihxpckts.h"
#include "hxassert.h"
#include "bufnum.h"
#include "rtpwrap.h"
#include "pkthndlr.h"

RTCPPacker::RTCPPacker()
    : m_pReport(NULL)
    , m_pSDES(NULL)
    , m_pBYE(NULL)
{
}


HX_RESULT
RTCPPacker::Set(RTCPPacket* pPkt)
{
    HX_ASSERT(pPkt);
    switch(pPkt->packet_type)
    {
    case RTCP_SR:
	HX_ASSERT(!m_pReport);
	m_pReport = pPkt;
	break;
    case RTCP_RR:
	HX_ASSERT(!m_pReport);
    	m_pReport = pPkt;
	break;
    case RTCP_SDES:
	m_pSDES = pPkt;
	break;
    case RTCP_BYE:
	m_pBYE = pPkt;
	break;
    case RTCP_APP:
        m_APPList.AddTail(pPkt);
	break;
    default:
	HX_ASSERT(!"RTCPPacker::Set():  Don't know this packet type");
	return HXR_FAIL;
    }	

    return HXR_OK;
}

void
RTCPPacker::PackOne(RTCPPacket* pPkt, REF(UCHAR*) pc, REF(UINT32) ulPackedLen)
{
    HX_ASSERT(pPkt && pc);

    pc = pPkt->pack(pc, ulPackedLen);    
}

/*
*   SR or RR must have been set
*   SDES must have been set
*/   
HX_RESULT
RTCPPacker::Pack(REF(IHXBuffer*) pBuf)
{    
    HX_ASSERT(m_pReport);
    HX_ASSERT(m_pSDES);
    HX_ASSERT(pBuf);	// needs to be an object

    UINT32 ulBufSize = (UINT32)((m_pReport->length + 1) * 4);
    ulBufSize += (UINT32)((m_pSDES->length + 1) * 4);

    if (m_pBYE)
    {
	ulBufSize += (UINT32)((m_pBYE->length + 1) * 4);
    }

    LISTPOSITION pos = m_APPList.GetHeadPosition();
    while (pos)
    {
        RTCPPacket* pAPP = 
            (RTCPPacket*)m_APPList.GetNext(pos);
	ulBufSize += (UINT32)((pAPP->length + 1) * 4);
    }

    // now pack!
    pBuf->SetSize(ulBufSize);
    UCHAR* pc = pBuf->GetBuffer();

    UINT32 ulPackedLen = 0;

    PackOne(m_pReport, pc, ulPackedLen);
    HX_ASSERT((((UINT32)m_pReport->length + 1) * 4) == ulPackedLen);

    PackOne(m_pSDES, pc, ulPackedLen);
    HX_ASSERT((((UINT32)m_pSDES->length + 1) * 4) == ulPackedLen);

    pos = m_APPList.GetHeadPosition();
    while (pos)
    {
        RTCPPacket* pAPP = 
            (RTCPPacket*)m_APPList.GetNext(pos);
	PackOne(pAPP, pc, ulPackedLen);
	HX_ASSERT((((UINT32)pAPP->length + 1) * 4) == ulPackedLen);		
    }

    /* BYE pkt needs to be the last pkt! */
    if (m_pBYE)
    {    
	PackOne(m_pBYE, pc, ulPackedLen);
	HX_ASSERT((((UINT32)m_pBYE->length + 1) * 4) == ulPackedLen);	
    }

    return HXR_OK;
}



RTCPUnPacker::RTCPUnPacker()
{
    /* nothing to do */
}

/*
*   if there are left over pkts in the list, delete them.
*/
RTCPUnPacker::~RTCPUnPacker()
{
    if (!m_PktList.IsEmpty())
    {
	CHXSimpleList::Iterator i;
	for (i = m_PktList.Begin(); i != m_PktList.End(); ++i)
	{
	    delete (RTCPPacket*)(*i);
        }
        m_PktList.RemoveAll();
    }

    HX_ASSERT(m_PktList.IsEmpty());
}

/*
*   Get a pkt in the head of the list.  Caller is responsible of deleting it
*/
HX_RESULT
RTCPUnPacker::Get(REF(RTCPPacket*) pPkt)
{
    if (m_PktList.IsEmpty())
    {
	return HXR_FAIL;
    }

    pPkt = (RTCPPacket*)m_PktList.RemoveHead();    

    HX_ASSERT(pPkt);
    return HXR_OK;
}

/*
*   RTCPPacket will be created for each pkt in this compound pkt.
*   Caller is responsible of freeing them
*/
HX_RESULT
RTCPUnPacker::UnPack(IHXBuffer* pCompound)
{
    //We shouldn't call RTCPUnPacker::Validate here. Validate throws out all 
    //packets even if only one of them is bad. Besides, RTCPPacket::unpack has 
    //pretty good error check to detect bad packets.

    BYTE* pFirst = pCompound->GetBuffer();
    BYTE* pNext = pFirst;
    RTCPPacket*	pPkt = NULL;

    
    while (pNext < (pFirst + pCompound->GetSize()))
    {
        pPkt = new RTCPPacket();

        pNext = pPkt->unpack(pNext, (UINT32)((pFirst + pCompound->GetSize()) - pNext));
        
        if(pNext == NULL)
        {
            //the packet is bad, we better bail out here.
            delete pPkt;
            break;
        }
        m_PktList.AddTail(pPkt);	
    }

    return HXR_OK;
}


/*
*   Copied from the RFC with a feeling...
*/
HX_RESULT
RTCPUnPacker::Validate(IHXBuffer* pCompound)
{
    HX_ASSERT(pCompound);
    
    UINT32 ulLen = pCompound->GetSize();
    BYTE* pc = pCompound->GetBuffer();
    BYTE* end = pc + ulLen;

    BYTE* pcTemp;    
    UINT16 unPktLen = 0;
    UINT8  uchVer = 0;
    do
    {
        pcTemp = pc;
        uchVer		= (UINT8)((*pcTemp++ & 0xc0) >> 6);

        // skip type
        ++pcTemp;

        unPktLen	= (UINT16)(*pcTemp++ << 8);
        unPktLen	|= *pcTemp++;

        pc = (pc + ((unPktLen + 1) * 4));
    } while (pc < end && 2 == uchVer);

    if (pc != end)
    {
        /* something is wrong */
        HX_ASSERT(!"RTCPUnPacker::Validate() failed");	
        return HXR_FAIL;
    }

    return HXR_OK;
}

