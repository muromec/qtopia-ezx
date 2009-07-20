/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtpwrap.h,v 1.20 2009/05/01 17:49:42 jzeng Exp $
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

#ifndef _RTPWRAP_H_
#define _RTPWRAP_H_

#include "rtppkt.h" // Base class definitions from PMC
#include "hxmap.h"
#include "hxslist.h"
#include "hxassert.h"
#include "hxinline.h"

class RTPPacket : public RTPPacketBase
{
public:
    RTPPacket()
    {
        /* 
         * XXXST I should be doing this in the initialization parameters
         * except the PMC generated class doesn't have a constructor so 
         * I cannot set them in this constructor.
         */
     
        version_flag = 0;
        padding_flag = 0;
        extension_flag = 0;
        csrc_len = 0;
        marker_flag = 0;
        payload = 0;
        seq_no = 0;
        timestamp = 0;
        ssrc = 0;
        csrc = 0;
        op_code = 0;
        op_code_data_length = 0;
        asm_flags = 0;
        asm_rule = 0;
        op_code_data = 0;
        data.data = 0;
        data.len = 0;
    }

    ~RTPPacket()
    {
        if (csrc)
            delete [] csrc;

        if (op_code_data)
            delete [] op_code_data;
    }

    void SetCSrc(INT32* pchNewData, UINT32 ulLen)
    {
        if (csrc)
            delete [] csrc;

        csrc = new UINT32[ulLen];
        memcpy(csrc, pchNewData, ulLen * sizeof(INT32)); /* Flawfinder: ignore */
    }
    void SetOpCodeData(INT32* pchNewData, UINT32 ulLen)
    {
        if (op_code_data)
            delete [] op_code_data;

        op_code_data = new UINT32[ulLen];
        memcpy(op_code_data, pchNewData, ulLen * sizeof(INT32)); /* Flawfinder: ignore */
    }
};

class RTCPPacket : public RTCPPacketBase
{
public:
    RTCPPacket()
    {
        version_flag = 0;
        padding_flag = 0;
        count = 0;	
        packet_type = 0;
        length = 0;
        sr_ssrc = 0;
        ntp_sec = 0;
        ntp_frac = 0;
        rtp_ts = 0;
        psent = 0;
        osent = 0;
        sr_data = 0;
        rr_ssrc = 0;
        rr_data = 0;
        sdes_data = 0;
        bye_src = 0;
        // used for APP
    //    app_name = 0;  // not a pointer...
        app_data = 0;
        m_pAPPItems = 0;
        m_bAPPDataSet = FALSE;
        m_pNADUInfo = 0;
    }

    ~RTCPPacket()
    {
        CleanBuffers();
    }

    UINT8*  pack(UINT8* data, UINT32 &len);
    UINT8*  unpack(UINT8* data, UINT32 len);

    void SetSenderReport	 (ReceptionReport* pNewData, UINT32 ulLen)
    {
        if (sr_data)
            delete [] sr_data;

        sr_data = new ReceptionReport[ulLen];
        memcpy(sr_data, pNewData, ulLen * sizeof(ReceptionReport)); /* Flawfinder: ignore */
    }
    void SetReceiverReport(ReceptionReport* pNewData, UINT32 ulLen)
    {
        if (rr_data)
            delete [] rr_data;

        rr_data = new ReceptionReport[ulLen];
        memcpy(rr_data, pNewData, ulLen * sizeof(ReceptionReport)); /* Flawfinder: ignore */
    }
    void SetSDES (UINT8* pchNewData, UINT32 ulLen)
    {
        if (sdes_data)
            delete [] sdes_data;

        sdes_data = new UINT8[ulLen];
        memcpy(sdes_data, pchNewData, ulLen * sizeof(UINT8)); /* Flawfinder: ignore */
    }
    void SetByeSrc (UINT32* pulNewData, UINT32 ulLen)
    {
        if (bye_src)
            delete [] bye_src;

        bye_src = new UINT32[ulLen];
        memcpy(bye_src, pulNewData, ulLen * sizeof(UINT32)); /* Flawfinder: ignore */
    }
    SDESItem* AddSDESItem (UINT32 ulSSrc, SDESItem item)
    {
        /*
         * Attempt to lookup this SSRC id from our map
         */
     
        CHXSimpleList*  pList = NULL;
        SDESItem*	    pItem = new SDESItem;

        if (!m_mapSDESSources.Lookup((LONG32)ulSSrc, (void*&)pList))
        {
            /* Not in map */
            
            pList = new CHXSimpleList;
            m_mapSDESSources[(LONG32)ulSSrc] = pList;
        }

        pItem->sdes_type = item.sdes_type;
        pItem->length = item.length;
        pItem->data = new UINT8[pItem->length];
        memcpy(pItem->data, item.data, item.length * sizeof(UINT8)); /* Flawfinder: ignore */

        pList->AddTail(pItem);

        return pItem;
    }
    void SetAPPItem (APPItem* item, UINT32 ulCount)
    {
        HX_VECTOR_DELETE(m_pAPPItems);
        m_bAPPDataSet = FALSE;

        m_pAPPItems = new APPItem[ulCount];
        memcpy(m_pAPPItems, item, ulCount * sizeof (APPItem)); /* Flawfinder: ignore */
    }

    void SetAPPData(const UINT8* pData, UINT32 ulLength)
    {
        // The data length MUST be a multiple of 4
        UINT32 ulAPPDataLen = ((ulLength + 3) & ~0x3);

        // Update the length member variable
        length = (UINT16)(2 + ((ulAPPDataLen) >> 2));

        HX_VECTOR_DELETE(app_data);
        app_data = new UINT8[ulAPPDataLen];

        if (app_data)
        {
            if (ulAPPDataLen != ulLength)
            {
                // Zero out the extra bytes
                memset(app_data + ulLength, 0,
                       ulAPPDataLen - ulLength);
                           
            }
            memcpy(app_data, pData, ulLength);
            m_bAPPDataSet = TRUE;
        }
    }
    
    // keys off of source id and contains a pointer to a list 
    // of SDESItems.
    CHXMapLongToObj	m_mapSDESSources;    
    // array of APPItem
    APPItem*		m_pAPPItems;
    HXBOOL                m_bAPPDataSet;

    //array of NADU packets
    RTCPNADUInfo*       m_pNADUInfo;

    void CleanBuffers();
};


#if defined (_DEFINE_INLINE)

HX_INLINE void
RTCPPacket::CleanBuffers()
{
    if (sr_data)
        delete [] sr_data;

    if (rr_data)
        delete [] rr_data;

    if (sdes_data)
        delete [] sdes_data;

    if (bye_src)
        delete [] bye_src;

    if (app_data)
    {
        delete [] app_data;
        m_bAPPDataSet = FALSE;
    }
        
    CHXMapLongToObj::Iterator	i;
    for (i = m_mapSDESSources.Begin(); i != m_mapSDESSources.End(); ++i)
    {
        CHXSimpleList*	pItems = (CHXSimpleList*) *i;
        while (!pItems->IsEmpty())
        {
            SDESItem*	pItem = (SDESItem*) pItems->RemoveHead();
            delete pItem;
        }
        delete pItems;
    }

    if (m_pAPPItems)
    {
        delete [] m_pAPPItems;
    }	
    
    if (m_pNADUInfo)
    {
        HX_VECTOR_DELETE(m_pNADUInfo);
    }
}

HX_INLINE UINT8*
RTCPPacket::unpack(UINT8* data, UINT32 len)
{
    CleanBuffers();

    UINT8* pBaseOff = RTCPPacketBase::unpack(data, len);

    // immediate error return
    if (!pBaseOff) return 0;

    /*
     * check to see if this is an SDES message and if it is then
     * parse out the SDES source chunks from the sdes_data blob
     *
     * Note rtppkt.h simply sets sdes_data (and length).  We love to split
     * parsing code across files, it makes maintenance so much more fun!
     *
     * NB: It is valid for length to be zero.
     */
    if (packet_type == RTCP_SDES)
    {
        HX_ASSERT(sdes_data != NULL);

        /* SDESItem objects keep ptrs so copy the buffer.  Is this needed? */
        UINT8*	pOff = new UINT8[length * 4];
        memcpy(pOff, sdes_data, length * 4 * sizeof(UINT8)); /* Flawfinder: ignore */
        sdes_data = pOff;

        for (UINT16 n = 0; n < count; n++)
        {
            UINT32 uSSRC;
            CHXSimpleList* pItemList = new CHXSimpleList;
            SDESItem* pItem;

            if ((pOff-sdes_data)+(4) > (length*4))
            {
                delete pItemList;
                return NULL;
            }

            uSSRC = GetDwordFromBufAndInc(pOff);
            do
            {
                pItem = new SDESItem;
                pOff = pItem->unpack(pOff, (UINT32)((length*4) - (pOff-sdes_data)));
                if (pOff == NULL)
                {
                    /* Error */
                    delete pItem;

                    while(!pItemList->IsEmpty())
                    {
                        SDESItem* pTemp = (SDESItem*)pItemList->RemoveHead();
                        delete pTemp;
                    }
                    delete pItemList;

                    count = 0;
                    return NULL;
                }
                pItemList->AddTail(pItem);
            }
            while (pItem->sdes_type != 0);

            // Do not store terminating item
            pItemList->RemoveTail();
            delete pItem;
            
            m_mapSDESSources[(LONG32)uSSRC] = pItemList;
        }
    }
    /*
     * Gatta do the same for APP msg!
     */
    else if (packet_type == RTCP_APP)
    {
        HX_ASSERT(app_data != NULL);

        // whether this is ours or not, we have to make own buffer
        // NB: RTCPPacketBase::unpack() succeeded so length>=2
        UINT32 ulDataLen = (length - 2) * 4;
        UINT8* pOff = new UINT8[ulDataLen];
        memcpy(pOff, app_data, (size_t)ulDataLen); /* Flawfinder: ignore */
        app_data = pOff;

        // This is a case dependent comparison (RFC 1889 s6.6).
        // RNWK or HELX APP packet
        if ((0 == memcmp(app_name, "RNWK", 4)) || 
            (0 == memcmp(app_name, "HELX", 4)))
        {
            // it's ours
            m_pAPPItems = new APPItem[count];

            for (UINT16 n = 0; n < count; n++)
            {
                pOff = m_pAPPItems[n].unpack(pOff, (UINT32)(((length - 2) * 4) - (pOff - app_data)));
                if (pOff == NULL)
                {
                    /* Error */
                    count = 0;
                    return NULL;
                }
            }
        }

        // PSS0 (NADU) APP packet
        // Ref: 3GPP TS 26.234 V6.3.0 Sec 6.2.3.2
        else if ((0 == memcmp(app_name, "PSS0", 4)) && count == 0)
        {
            // NADU specific App. data contains one or more NADU reports.
            //
            // length is specified as number of 32 bit words - 1.
            // An app packet is 12 bytes (3 blocks) and there are 3
            // blocks preceding the app packets, so there are 
            // length - 2 blocks 
            // The number of blocks is computed using length.
            //
            // Buffer length has already been verified to fit the 
            // length specified so this is safe for bounds checking.
            UINT16 unNADUInfoCount = (length - 2) / 3;
            m_pNADUInfo = new RTCPNADUInfo[unNADUInfoCount];

            for (UINT16 n = 0; n < unNADUInfoCount; ++n)
            {
                pOff = m_pNADUInfo[n].unpack(pOff, 
                        ulDataLen - (pOff - app_data));

                if (pOff == NULL)
                {
                    /* Error */
                    delete[] m_pNADUInfo;
                    return NULL;
                }
            }
        }
    }

    return pBaseOff;
}

HX_INLINE UINT8*
RTCPPacket::pack(UINT8* data, UINT32& len)
{
    if (packet_type == RTCP_SDES)
    {
        /*
         * revert the m_mapSDESSources back to blob form
         */

        if (sdes_data != NULL)
        {
            delete [] sdes_data;
        }

        /* 
         * Order may be important here, using a map will not
         * guarentee order
         */

        UINT8* pBuf = new UINT8[0x1000];
        UINT8* pOff = pBuf;
        CHXMapLongToObj::Iterator iSrc = m_mapSDESSources.Begin();
        for (; iSrc != m_mapSDESSources.End(); ++iSrc)
        {
            INT32 lSrc = iSrc.get_key();
            CHXSimpleList*		pItems = (CHXSimpleList*) *iSrc;
            CHXSimpleList::Iterator	iItem = pItems->Begin();

            *pOff++ = (UINT8) (lSrc>>24);   *pOff++ = (UINT8) (lSrc>>16);
            *pOff++ = (UINT8) (lSrc>>8);    *pOff++ = (UINT8) (lSrc);

            for (; iItem != pItems->End(); ++iItem)
            {
                UINT32		ulItemLen = (UINT32)(pBuf - pOff);
                SDESItem*	pSDESItem = (SDESItem*)*iItem;
                
                pOff = pSDESItem->pack(pOff, ulItemLen);
                if (pSDESItem->data)
                {
                    delete [] pSDESItem->data;
                }
            }

            /*
             * Terminate the list with an item of type 0 (first byte)
             */
            *pOff++ = 0;
        
            /* 
             * Pad until 4 bytes boundary
             */
        
            int lLength = HX_SAFEINT(pOff - pBuf);
            
            if (lLength % 4)
            {	
                memset(pOff, 0, (size_t)(4 - (lLength % 4)));
                pOff += 4 - (lLength % 4);
            }    	       

            HX_ASSERT((HX_SAFEINT(pOff - pBuf) % 4) == 0);
        }

        sdes_data = new UINT8[pOff - pBuf];
        memcpy(sdes_data, pBuf, (pOff - pBuf) * sizeof(UINT8)); /* Flawfinder: ignore */

        length = (UINT16)((pOff - pBuf) / 4);

        delete [] pBuf;
    }
    else if (packet_type == RTCP_APP)
    {
        /*
         * revert the m_mapSDESSources back to blob form
         */

        if (!m_bAPPDataSet)
        {
            if (app_data != NULL)
            {
                delete [] app_data;
            }   
    
            UINT8*  pBuf = new UINT8[0x1000];
            UINT8*  pOff = pBuf;
            UINT32  ulDammy = 0;
            for (UINT32 i = 0; i < count; i++)
            {
                pOff = m_pAPPItems[i].pack(pOff, ulDammy);
            }

            app_data = new UINT8[pOff - pBuf];
            memcpy(app_data, pBuf, (pOff - pBuf) * sizeof(UINT8)); /* Flawfinder: ignore */

            delete [] pBuf;
        }
    }

    return RTCPPacketBase::pack(data, len);
}
#endif //_DEFINE_INLINE

#endif /* _RTPWRAP_H_ */

