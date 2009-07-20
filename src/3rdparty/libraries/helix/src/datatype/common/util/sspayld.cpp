/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sspayld.cpp,v 1.6 2007/07/06 22:00:23 jfinnecy Exp $
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
#include "hxcom.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "netbyte.h"

#include "sspayld.h"

#define DEFAULT_MAX_PACKET_SIZE 1500

SimpleSegmentPayloadFormat::SimpleSegmentPayloadFormat()
: m_lRefCount(0)
, m_pCommonClassFactory(NULL)
, m_pStreamHeader(NULL)
, m_bPacketize(FALSE)
, m_ulMaxSegmentSize(DEFAULT_MAX_PACKET_SIZE)
, m_ulTotalSegments(0)
, m_pSegmentedPackets(NULL)
, m_pAssembledPacket(NULL)
, m_bBigEndian(FALSE)
{
    m_pSegmentedPackets = new CHXSimpleList();
    m_bBigEndian = TestBigEndian();
}

SimpleSegmentPayloadFormat::~SimpleSegmentPayloadFormat()
{
    Reset();
    HX_DELETE(m_pSegmentedPackets);
    HX_RELEASE(m_pCommonClassFactory);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::QueryInterface
//  Purpose:
//  Implement this to export the interfaces supported by your
//  object.
//
STDMETHODIMP
SimpleSegmentPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this},
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*)this},
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::AddRef
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32)
SimpleSegmentPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::Release
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32)
SimpleSegmentPayloadFormat::Release()
{
    if ( InterlockedDecrement(&m_lRefCount) > 0 )
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
SimpleSegmentPayloadFormat::Init( IUnknown* pContext,
                                  HXBOOL bPacketize )
{
    HX_ASSERT(pContext);
    if ( !pContext )
    {
        return HXR_FAIL;
    }

    HX_RELEASE(m_pCommonClassFactory);
    m_pCommonClassFactory = NULL;

    if ( HXR_OK != pContext->QueryInterface( IID_IHXCommonClassFactory,
                                             (void**) &m_pCommonClassFactory )
       )
    {
        return HXR_FAIL;
    }

    m_bPacketize = bPacketize;

    return HXR_OK;
}

STDMETHODIMP
SimpleSegmentPayloadFormat::Reset()
{
    // Reset() is presumed not to be the complete opposite of Init();
    // We reset stream information but keep our context and packetization
    // orientation until the next Init()
    HX_RELEASE( m_pStreamHeader );
    m_pStreamHeader = NULL;
    m_ulMaxSegmentSize = 0;

    HX_RELEASE( m_pAssembledPacket );
    m_pAssembledPacket = NULL;

    return Flush();
}

STDMETHODIMP
SimpleSegmentPayloadFormat::SetStreamHeader( IHXValues* pHeader )
{
    HX_ASSERT( pHeader );
    if ( !pHeader )
    {
        return HXR_FAIL;
    }

    HX_RELEASE( m_pStreamHeader );

    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    if ( HXR_OK != m_pStreamHeader->GetPropertyULONG32( "MaxPacketSize", m_ulMaxSegmentSize ) )
    {
        m_ulMaxSegmentSize = DEFAULT_MAX_PACKET_SIZE;
    }

    return HXR_OK;
}

STDMETHODIMP
SimpleSegmentPayloadFormat::GetStreamHeader( REF(IHXValues*) pHeader )
{
    HX_ASSERT( m_pStreamHeader );
    pHeader = m_pStreamHeader;
    pHeader->AddRef();

    return HXR_OK;
}

STDMETHODIMP
SimpleSegmentPayloadFormat::SetPacket( IHXPacket* pPacket )
{
    HX_ASSERT(pPacket);
    if ( !pPacket )
    {
        return HXR_FAIL;
    }

    if ( m_bPacketize )
    {
        // Segment the packet here rather than in GetPacket(), on
        // the assumption that GetPacket() is a time sensitive function:
        // We implicitly allow multiple SetPackets() before a GetPacket()
        // in this case; it is up to the client to realize there are multiple
        // segmented packets in our queue.
        IHXBuffer* pPacketBuffer = pPacket->GetBuffer();
        HX_ASSERT( pPacketBuffer );
        if ( !pPacketBuffer )
        {
            return HXR_FAIL;
        }

        UCHAR* pPacketBufferData = pPacketBuffer->GetBuffer();
        UINT32 uiBytesRemaining = pPacketBuffer->GetSize();
        // An unsafe cast: to retain backwards compatibility, not much we can do about it.
        // We could implement a high segment count as an escape sequence; this would break
        // however, in the unlikely event someone wanted to segment a packet into 65535
        // chunks using deployed code (CPNFragment).  To be investigated if the priority ever
        // becomes high.
        m_ulTotalSegments = (unsigned short) ( pPacketBuffer->GetSize() / m_ulMaxSegmentSize );
        if ( pPacketBuffer->GetSize() % m_ulMaxSegmentSize )
        {
            m_ulTotalSegments++;
        }

        IHXBuffer* pSegmentBuffer;
        IHXPacket* pSegmentPacket;

        for ( int i = 0; uiBytesRemaining; i++ )
        {
            if ( HXR_OK != m_pCommonClassFactory->CreateInstance( IID_IHXBuffer,
                                                                  (void**) &pSegmentBuffer )
               )
            {
                Flush();
                HX_RELEASE( pPacketBuffer );
                return HXR_OUTOFMEMORY;
            }

            if ( HXR_OK != m_pCommonClassFactory->CreateInstance( IID_IHXPacket,
                                                                  (void**) &pSegmentPacket )
               )
            {
                Flush();
                HX_RELEASE( pSegmentBuffer );
                HX_RELEASE( pPacketBuffer );
                return HXR_OUTOFMEMORY;
            }

            pSegmentBuffer->SetSize( sizeof(SEGMENTHEADER) + (uiBytesRemaining > m_ulMaxSegmentSize) ? m_ulMaxSegmentSize : uiBytesRemaining );
            LPSEGMENTHEADER pSegmentHeader = (LPSEGMENTHEADER) pSegmentBuffer->GetBuffer();

            pSegmentHeader->ulOffset = i * ( m_ulMaxSegmentSize );
            pSegmentHeader->usIndex = i;
            pSegmentHeader->usTotalSegments = m_ulTotalSegments;

            if ( !m_bBigEndian )
            {
                SwapDWordBytes(&pSegmentHeader->ulOffset, 1);
                SwapWordBytes(&pSegmentHeader->usIndex, 1);
                SwapWordBytes(&pSegmentHeader->usTotalSegments, 1);
            }

            memcpy( (UCHAR*) pSegmentHeader + sizeof(SEGMENTHEADER), /* Flawfinder: ignore */
                    pPacketBufferData + pSegmentHeader->ulOffset,
                    pSegmentBuffer->GetSize() - sizeof(SEGMENTHEADER)
                  );

            uiBytesRemaining -= pSegmentBuffer->GetSize() - sizeof(SEGMENTHEADER);;

            pSegmentPacket->Set( pSegmentBuffer,
                                 pPacket->GetTime(),
                                 pPacket->GetStreamNumber(),
                                 pPacket->GetASMFlags(),
                                 pPacket->GetASMRuleNumber()
                               );
            HX_RELEASE( pSegmentBuffer );

            m_pSegmentedPackets->AddTail( pSegmentPacket );
        }

        HX_RELEASE( pPacketBuffer );
    }
    else
    {
        // This PayloadFormat does not return incomplete data;
        // The client is assumed to be able to deal with lost data quantums
        // of the size of m_ulPacketSize:
        if ( pPacket->IsLost() )
        {
            Flush();
            return HXR_OK;
        }

        IHXBuffer* pPacketBuffer = pPacket->GetBuffer();
        UINT16 usSegment = ( (LPSEGMENTHEADER) pPacketBuffer->GetBuffer() )->usIndex;
        if ( !m_bBigEndian )
        {
            SwapWordBytes( &usSegment, 1 );
        }
        HX_RELEASE( pPacketBuffer );

        // Missing/lost segments?
        if (usSegment != m_pSegmentedPackets->GetCount())
        {
            // Since we flush on lost packets, a non-zero segment list
            // indicates a *missing* packet (bug in the core)
            HX_ASSERT(m_pSegmentedPackets->GetCount() == 0 && "Missing segments!");
            Flush();
            return HXR_OK;
        }

        pPacket->AddRef();
        m_pSegmentedPackets->AddTail( pPacket );

        // New frame?
        if ( usSegment == 0 )
        {
            IHXBuffer* pFirstBuffer = pPacket->GetBuffer();
            HX_ASSERT( pFirstBuffer );

            m_ulTotalSegments = ( (LPSEGMENTHEADER) pFirstBuffer->GetBuffer() )->usTotalSegments;
            if ( !m_bBigEndian )
            {
                SwapWordBytes( &m_ulTotalSegments, 1 );
            }
            HX_RELEASE( pFirstBuffer );

            if (m_ulTotalSegments == 0)
            {
                // No segments, eh?
                HX_ASSERT(FALSE && "Segment count zero - packetizer error!");
                Flush();
                return HXR_OK;
            }
        }

        // Sort and reconstruct the packet here, rather than in GetPacket(), on
        // the assumption that GetPacket() is a time sensitive function:
        if ( m_pSegmentedPackets->GetCount() == m_ulTotalSegments )
        {
            // We use a simple O(n) reconstruction here (no sorting)
            UINT32 uiPacketSize = 0;

            IHXBuffer* pSegmentBuffer = NULL;
            UCHAR* pSegmentBufferData = NULL;

            // Find packet size and swap headers:
            for ( CHXSimpleList::Iterator maxIterator = m_pSegmentedPackets->Begin();
                maxIterator != m_pSegmentedPackets->End();
                ++maxIterator )
            {
                pSegmentBuffer = ( (IHXPacket*) *maxIterator )->GetBuffer();
                pSegmentBufferData = pSegmentBuffer->GetBuffer();

                if ( !m_bBigEndian )
                {
                    SwapDWordBytes( &( (LPSEGMENTHEADER) pSegmentBufferData )->ulOffset, 1 );
                }

                if ( ( (LPSEGMENTHEADER) pSegmentBufferData )->ulOffset + pSegmentBuffer->GetSize() > uiPacketSize )
                {
                    uiPacketSize = ( (LPSEGMENTHEADER) pSegmentBufferData )->ulOffset + pSegmentBuffer->GetSize();
                }
                HX_RELEASE(pSegmentBuffer);
            }
            uiPacketSize -= sizeof( SEGMENTHEADER );

            // An assembled packet should have been collected by this point; we only assemble
            // one packet at a time, for now, for simplicity.
            HX_ASSERT( !m_pAssembledPacket );
            HX_RELEASE( m_pAssembledPacket );
            m_pAssembledPacket = NULL;

            // Create our assembled packet, buffer:
            IHXBuffer* pPacketBuffer = NULL;
            if ( HXR_OK != m_pCommonClassFactory->CreateInstance( IID_IHXBuffer,
                                                                  (void**) &pPacketBuffer )
               )
            {
                Flush();
                return HXR_FAIL;
            }

            if ( HXR_OK != m_pCommonClassFactory->CreateInstance( IID_IHXPacket,
                                                                  (void**) &m_pAssembledPacket )
               )
            {
                Flush();
                HX_RELEASE( pPacketBuffer );
                return HXR_FAIL;
            }

            pPacketBuffer->SetSize( uiPacketSize );
            IHXPacket* pPacketDetails = ( (IHXPacket*) m_pSegmentedPackets->GetHead() );
            m_pAssembledPacket->Set(pPacketBuffer,
                                    pPacketDetails->GetTime(),
                                    pPacketDetails->GetStreamNumber(),
                                    pPacketDetails->GetASMFlags(),
                                    pPacketDetails->GetASMRuleNumber()
                                   );

            // This reconstruction approach may not assemble in segment order, but the
            // performance penalty is small, and it keeps the code simple.  Using ulOffset
            // also allows clever datatypes/compatible payload formatters to insert gaps in
            // the buffer in which the renders could write data for their own bizzare
            // purposes (use at your own peril).
            UCHAR* pPacketBufferData = pPacketBuffer->GetBuffer();
            UINT32 ulPacketBufferSize = pPacketBuffer->GetSize();
            HX_RELEASE(pPacketBuffer);
            IHXPacket* pSegmentPacket = NULL;
            while ( !m_pSegmentedPackets->IsEmpty() )
            {
                pSegmentPacket = (IHXPacket*) m_pSegmentedPackets->RemoveHead();
                HX_ASSERT( pSegmentPacket );
                pSegmentBuffer = pSegmentPacket->GetBuffer();
                HX_ASSERT( pSegmentBuffer );
                pSegmentBufferData = pSegmentBuffer->GetBuffer();
                UINT32 ulMaxCopyBytes = ulPacketBufferSize - ( (LPSEGMENTHEADER) pSegmentBufferData )->ulOffset;
                UINT32 ulBytesToCopy = pSegmentBuffer->GetSize() - sizeof(SEGMENTHEADER);
                if (ulBytesToCopy > ulMaxCopyBytes) ulBytesToCopy = ulMaxCopyBytes;
                memcpy( pPacketBufferData + ( (LPSEGMENTHEADER) pSegmentBufferData )->ulOffset, /* Flawfinder: ignore */
                        pSegmentBufferData + sizeof(SEGMENTHEADER),
                        ulBytesToCopy);
                HX_RELEASE( pSegmentBuffer );
                HX_RELEASE( pSegmentPacket );
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP
SimpleSegmentPayloadFormat::GetPacket( REF(IHXPacket*) pPacket )
{
    pPacket = NULL;

    if ( m_bPacketize )
    {
        if ( m_pSegmentedPackets->IsEmpty() )
        {
            return HXR_UNEXPECTED;
        }
        else
        {
            pPacket = (IHXPacket*) m_pSegmentedPackets->RemoveHead();
        }
    }
    else
    {
        if ( m_pAssembledPacket )
        {
            pPacket = m_pAssembledPacket;
            m_pAssembledPacket = NULL;
        }
        else
        {
            return HXR_INCOMPLETE;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
SimpleSegmentPayloadFormat::Flush()
{
    // Release all of our segment packets; but keep any assembled
    while ( !m_pSegmentedPackets->IsEmpty() )
    {
        IHXPacket* pPacket = (IHXPacket*) m_pSegmentedPackets->RemoveHead();
        HX_RELEASE(pPacket);
    }
    m_ulTotalSegments = 0;

    return HXR_OK;
}
