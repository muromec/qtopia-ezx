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
// m4asm: .m4a stream mixer (built for a single stream)

#include "hxassert.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "mp4arch.h"
#include "m4asm.h"
#include "mp4atoms.h"

#include "m4amdatoms.h"

// Added for branching off to type specific atoms build
#include "m4aatomgateway.h"

//#define M4A_DEFAULT_FREE_BLOCK_SIZE (32 + (32 * 1024))
#define M4A_RESERVE_BLOCK_SIZE    (32 * 1024)

#define M4A_DEFAULT_METADATA_RESERVE_SIZE (2 * 1024)

#define SAMPLERATE_AS_MEDIA_TIMESCALE

// class to handle managing the stbl, will eventually
//  be pulled out into mp4fwlib
class CStblManager
{
public:
    CStblManager( CMP4Atom_stbl* pStbl )
	{
	    m_uiLastTimeStamp   = 0;
	    m_uiBaseWriteOffset = 0;
	    m_uiSampleCounter   = 0;
	    m_uiCurrentOffset   = 0;

	    m_pStbl = pStbl;
	    m_pStts = (CMP4Atom_stts*)m_pStbl->FindChild("stts");
	    m_pStsz = (CMP4Atom_stsz*)m_pStbl->FindChild("stsz");
	    m_pStco = (CMP4Atom_stco*)m_pStbl->FindChild("stco");
	    m_pStsc = (CMP4Atom_stsc*)m_pStbl->FindChild("stsc");

	    m_pStbl->AddRef();
	    m_pStts->AddRef();
	    m_pStsz->AddRef();
	    m_pStco->AddRef();
	    m_pStsc->AddRef();
	}
    ~CStblManager()
	{
	    m_pStbl->Release();
	    m_pStts->Release();
	    m_pStsz->Release();
	    m_pStco->Release();
	    m_pStsc->Release();
	}

    // Should be called before NotifyPacket
    STDMETHOD( NotifyBaseWriteOffset ) ( THIS_ UINT32 uiBaseOffset )
	{
	    m_uiBaseWriteOffset = uiBaseOffset;
	    return HXR_OK;
	}

    STDMETHOD( NotifyPacket ) ( THIS_ IHXPacket* pPacket, UINT32 ulBaseTime, BOOL bIsRTP );
    STDMETHOD( NotifyDone )   ( THIS );

    STDMETHOD( EstimateSize ) ( UINT32 ulDuration, UINT32 ulSamplesPerSecond, UINT32& ulEstimatedSize );
    
private:
    UINT32 m_uiLastTimeStamp;
    UINT32 m_uiBaseWriteOffset;
    UINT32 m_uiSampleCounter;
    UINT32 m_uiCurrentOffset;

    CMP4Atom_stbl* m_pStbl;
    CMP4Atom_stts* m_pStts;
    CMP4Atom_stsz* m_pStsz;
    CMP4Atom_stco* m_pStco;
    CMP4Atom_stsc* m_pStsc;
};

// how many samples in a chunk before we start a new chunk
#define SAMPLE_WRAP_COUNT 21

STDMETHODIMP CStblManager::NotifyPacket( THIS_ IHXPacket* pPacket, UINT32 ulBaseTime, BOOL bIsRTP )
{
    HX_RESULT retVal = HXR_OK;

    if( bIsRTP )
    {
	IHXRTPPacket* pRTPPacket = NULL;
	pPacket->QueryInterface( IID_IHXRTPPacket, (void**) &pRTPPacket );

	UINT32 uiTime = pRTPPacket->GetRTPTime();

	if( uiTime && uiTime >= ulBaseTime )
	{
	    uiTime -= ulBaseTime;
	    
	    if( !m_uiLastTimeStamp )
	    {
		// We add two here; one for this packet, and one for the 0 timestamp packet
		m_pStts->AddSamplePair( 2, uiTime );
	    }
	    else
	    {
		UINT32 uiDelta = (UINT32)m_pStts->GetLastSampleDelta();
		if( (uiTime - m_uiLastTimeStamp) == uiDelta )
		{
		    m_pStts->IncrementLastSampleCount();
		}
		else
		{
		    m_pStts->AddSamplePair( 1, uiTime - m_uiLastTimeStamp );
		}
	    }
	    m_uiLastTimeStamp = uiTime;
	}
	HX_RELEASE( pRTPPacket );
    }
    else
    {
	// XXX TODO Currently non rtp streams are not supported. The appropriate
	// solution here will be to add a tsconverter to handle adjusting timestamps
	// to the correct value..
	HX_ASSERT( bIsRTP );
    }

    // update stsz
    // unfortunately we have to get the buffer and do the addref/release fun in order to
    // check the size of the packet
    IHXBuffer* pBuf = pPacket->GetBuffer();
    UINT32 uiSize = pBuf->GetSize();
    HX_RELEASE( pBuf );
    
    m_pStsz->AddSampleSize( uiSize );

    // update stco
    if( (m_uiSampleCounter % SAMPLE_WRAP_COUNT) == 0 )
    {
	// new chunk offset
	m_pStco->AddChunkOffset( m_uiBaseWriteOffset + m_uiCurrentOffset );

    }

    // increment file offset; note we never modify the basewriteoffset, this
    // allows us to go back later and rewrite the entire chunk table if needed
    m_uiCurrentOffset += uiSize;

    m_uiSampleCounter++;

    return retVal;
}

STDMETHODIMP CStblManager::NotifyDone( THIS )
{
    HX_RESULT retVal = HXR_OK;

    // set up the stsc here
    if( m_uiSampleCounter >= SAMPLE_WRAP_COUNT )
    {
	m_pStsc->AddNewEntry( 1, SAMPLE_WRAP_COUNT, 1 );
    }

    // now handle an entry for the remainder (if any)
    UINT32 uiSamples    = (m_uiSampleCounter % SAMPLE_WRAP_COUNT);

    if( uiSamples )
    {
	// add one to the chunk index since the div gives
	// us the number of whole chunks, and we want the
	// index for the first incomplete one
	UINT32 uiChunkIndex = (m_uiSampleCounter / SAMPLE_WRAP_COUNT) + 1;
	m_pStsc->AddNewEntry( uiChunkIndex, uiSamples, 1 );
    }

    return retVal;
}

STDMETHODIMP CStblManager::EstimateSize( UINT32 ulDuration, UINT32 ulSamplesPerSecond, UINT32& ulEstimatedSize )
{
    HX_RESULT retVal = HXR_FAIL;

    if( ulDuration && ulSamplesPerSecond )
    {
	UINT32 ulEstimatedPackets = (ulSamplesPerSecond / 1024) * (ulDuration / 1000);

	// First determine the stsz size
	ulEstimatedSize = ulEstimatedPackets * sizeof(UINT32);

	// Next the size of the stco
	ulEstimatedSize += (( 1 + (ulEstimatedPackets / SAMPLE_WRAP_COUNT) ) * sizeof(UINT32));
	
	retVal = HXR_OK;
    }
    
    return retVal;
}

static class CStblManager* g_pStblManager = NULL;

CM4AStreamMixer::CM4AStreamMixer( IUnknown* pContext, CMP4Archiver* pArchiver )
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    m_pStreamHeader = NULL;
    m_lRefCount = 0;
    HX_ASSERT( pArchiver );

    pArchiver->AddRef();
    m_pArchiver = pArchiver;

    m_pRootAtom = new CMP4Atom_file();
    HX_ASSERT( m_pRootAtom );

    m_pRootAtom->AddRef();
    
    m_pMvhd = NULL;
    m_pMdhd = NULL;
    m_pTkhd = NULL;

    m_ulLastTimestamp = m_ulActualDuration = m_ulReportedDuration = 0;
    m_uiByteCount = 0;
    m_uiReservedBlockCount = 0;
    m_ulFirstTimestamp = m_ulFirstRTPTimestamp = 0;

    m_eIsRtp = UNCHECKED;
    m_bMoovAtEnd = FALSE;
}


CM4AStreamMixer::~CM4AStreamMixer()
{
    delete g_pStblManager;
    g_pStblManager = NULL;
    
    HX_RELEASE( m_pArchiver );
    HX_RELEASE( m_pFileHeader );
    HX_RELEASE( m_pStreamHeader );
    HX_RELEASE( m_pRootAtom );
    HX_RELEASE( m_pContext );
}

STDMETHODIMP CM4AStreamMixer::QueryInterface( THIS_ REFIID riid, void** ppvObj )
{
    // dont support, for now
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CM4AStreamMixer::AddRef()
{
    return InterlockedIncrement( &m_lRefCount );
}
STDMETHODIMP_(ULONG32) CM4AStreamMixer::Release()
{
    if( InterlockedDecrement( &m_lRefCount ) > 0 )
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP CM4AStreamMixer::SetProperties( THIS_ IHXValues* pProperties )
{
    return HXR_OK;
}

STDMETHODIMP CM4AStreamMixer::SetFileHeader( THIS_ IHXValues* pFileHeader )
{
    HX_RESULT retVal = HXR_OK;

    pFileHeader->AddRef();
    m_pFileHeader = pFileHeader;

    return retVal;
}

STDMETHODIMP CM4AStreamMixer::SetStreamHeader( THIS_ IHXValues* pStreamHeader )
{
    HX_RESULT retVal = HXR_OK;

    pStreamHeader->AddRef();
    m_pStreamHeader = pStreamHeader;
    retVal = BuildM4AAtoms();

    if( SUCCEEDED( retVal ) )
    {
	// This stream header is good; pass it on to the archiver
	retVal = m_pArchiver->StreamHeaderReady( pStreamHeader );
    }
    
    if( SUCCEEDED( retVal ) )
    {
	UINT32 ulPadSize = 0;

	if( m_uiReservedBlockCount )
	{
	    ulPadSize = m_uiReservedBlockCount * M4A_RESERVE_BLOCK_SIZE;
	}
	else if( m_bMoovAtEnd )
	{
	    // just provide enough free space for the ftyp atom; the header on the 'free'
	    // atom will use the space required for the mdat header
	    CMP4Atom* pFtyp = m_pRootAtom->FindChild( "ftyp" );
	    if( pFtyp )
	    {
		ulPadSize = pFtyp->GetCurrentSize();
	    }
	}
	if( ulPadSize )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_free* pPad = new CMP4Atom_free( ulPadSize );
	
	    if( pPad )
	    {
		retVal = m_pArchiver->WriteAtom( pPad, FALSE );

		delete pPad;
	    }
	}
    }
    return retVal;
}

STDMETHODIMP CM4AStreamMixer::SetPacket( THIS_ IHXPacket* pPacket )
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if( pPacket )
    {
	IHXBuffer* pBuf = pPacket->GetBuffer();
	UINT32 uiByteCount = pBuf->GetSize();
	HX_RELEASE( pBuf );

	if( m_eIsRtp == UNCHECKED )
	{
	    HX_RESULT res;
	    IHXRTPPacket* pRtpPacket = NULL;
	    res = pPacket->QueryInterface( IID_IHXRTPPacket, (void**) &pRtpPacket );
	    if( SUCCEEDED( res ) )
	    {
		m_ulFirstRTPTimestamp = pRtpPacket->GetRTPTime();
		m_eIsRtp = IS_RTP;
		
		HX_RELEASE( pRtpPacket );
	    }
	    else
	    {
		m_eIsRtp = NOT_RTP;
	    }

	    // Make use of the fact that the rtp check only happens once, and
	    // set this
	    m_ulFirstTimestamp = pPacket->GetTime();
	}

	// XXX This code should be blended in with the stblmanager code below when
	// this is moved into a mp4 atom library. Basically we need to determine the
	// actual length of the stream because it may very from the reported duration
	// The way we do this is by assuming that the current packet is the same
	// size as the last one. Inaccurate, but probably better than the provided duration
	UINT32 ulTime = pPacket->GetTime();
	m_ulActualDuration = ulTime + (ulTime - m_ulLastTimestamp);
	m_ulLastTimestamp  = ulTime;

	g_pStblManager->NotifyPacket( pPacket, m_ulFirstRTPTimestamp, (m_eIsRtp == IS_RTP) );

	retVal = m_pArchiver->PacketReady( pPacket );
	if( SUCCEEDED( retVal ) )
	{
	    m_uiByteCount += uiByteCount;
	}
    }
    return retVal;
}

STDMETHODIMP CM4AStreamMixer::StreamDone( THIS_ UINT16 unStreamNumber )
{
    HX_RESULT retVal = HXR_OK;

    // See if we were given the correct duration. If not, update.
    m_ulActualDuration -= m_ulFirstTimestamp;
    if( m_ulReportedDuration != m_ulActualDuration )
    {
	UpdateDuration( m_ulActualDuration );
    }

    // Notify the stbl manager we are done
    g_pStblManager->NotifyDone();

    CMP4Atom_mdat* pMdat = (CMP4Atom_mdat*)m_pRootAtom->FindChild("mdat");
    pMdat->SetBytesWritten( m_uiByteCount );

    // Wrap up our atom tree
    if( m_bMoovAtEnd )
    {
	CMP4Atom* pFtyp = m_pRootAtom->FindChild( "ftyp" );
	CMP4Atom* pMoov = m_pRootAtom->FindChild( "moov" );

	if( pMdat && pFtyp && pMoov )
	{
	    // In this situation, we need to notify the mdat atom of how many
	    // bytes we wrote out to it, then set the base location for the moov atom
	    // appropriately
	    // Note that mdat->GetCurrentSize() does not give back the number of bytes written
	
	    pMoov->NotifyWriteOffset( pMdat->GetLastWriteOffset() + pMdat->GetCurrentSize() + m_uiByteCount );

	    // Now write the atoms individually; order does not matter since all the atoms
	    // are anchored to specific file locations
	    m_pArchiver->WriteAtom( pFtyp, TRUE );
	    m_pArchiver->WriteAtom( pMdat, TRUE );
	    m_pArchiver->WriteAtom( pMoov, TRUE );
	}
	
    }
    else
    {
	// In this situation, we need to resize our top level free space atom
	// to reflect the size of the ftyp,moov,mdat atoms. 

	// XXX this only works if we only have one free atom
	CMP4Atom_free* pFree = (CMP4Atom_free*)m_pRootAtom->FindChild( "free" );
	if( pFree && pMdat )
	{
	    UINT32 uiFreeSize    = 8 + (m_uiReservedBlockCount * M4A_RESERVE_BLOCK_SIZE);
	    UINT32 uiCurrentSize = m_pRootAtom->GetCurrentSize(TRUE);

	    // XXX TODO
	    //  This is currently a major issue; we can't dynamically resize the file or shift
	    //  all the data, but the metadata has outgrown our reserved size
	
	    HX_ASSERT( uiFreeSize >= uiCurrentSize );
	    if( uiFreeSize >= uiCurrentSize )
	    {
		// Update the free size atom
		pFree->SetFreeSize( uiFreeSize - uiCurrentSize );
	    }
	 
	    // And write out our atoms
	    m_pArchiver->WriteAtom( m_pRootAtom, TRUE );
	}
    }
    
    m_pArchiver->Done();
    m_pTkhd = NULL;
    m_pMdhd = NULL;
    m_pMvhd = NULL;
    
    return retVal;
}

HX_RESULT CM4AStreamMixer::BuildM4AAtoms()
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    UINT32 ulDuration = 0;

    m_pStreamHeader->GetPropertyULONG32("Duration", ulDuration);

    m_ulReportedDuration = ulDuration;

    // This addresses a problem where line in encoding sets the duration to -1
    if( m_ulReportedDuration == 0xFFFFFFFF )
    {
	m_ulReportedDuration = ulDuration = 0;
    }
    
    if( !m_ulReportedDuration )
    {
	m_bMoovAtEnd = TRUE;
    }
    
    // Prep the basic atom structure. This will change when properties are used
    // this is order dependent, since we can't shuffle children yet

    // At the top level we set up 4 main atoms: ftyp, moov, free, mdat
    // If duration is absent, we set up 3 main atoms: ftyp, mdat, moov

    CMP4Atom* pAtom = NULL;
    CMP4Atom_ftyp* pFtyp = NULL;
    CMP4Atom_free* pFree = NULL;
    CMP4Atom_moov* pMoov = NULL;
    CMP4Atom_mdat* pMdat = NULL;
    
    pFtyp = new CMP4Atom_ftyp;
    if( pFtyp )
    {
	m_pRootAtom->AddChild( pFtyp );
	retVal = HXR_OK;
    }
    
    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_OUTOFMEMORY;
	pMoov = new CMP4Atom_moov;

	if( pMoov )
	{
	    retVal = HXR_OK;
	}
    }
    
    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_OUTOFMEMORY;
	pMdat = new CMP4Atom_mdat();
	
	if( pMdat )
	{
	    retVal = HXR_OK;
	}
    }


    if( SUCCEEDED( retVal ) )
    {
	if( m_bMoovAtEnd )
	{
	    m_pRootAtom->AddChild( pMdat );
	    m_pRootAtom->AddChild( pMoov );
	}
	else
	{
	    m_pRootAtom->AddChild( pMoov );
	    if( SUCCEEDED( retVal ) )
	    {
		// Initially we set the free block to have 0 bytes in it; the logic
		// here is that we will fill out the other atoms and determine how much
		// free space we want to have left
		retVal = HXR_OUTOFMEMORY;
		pFree = new CMP4Atom_free(0);

		if( pFree )
		{
		    m_pRootAtom->AddChild( pFree );
		    retVal = HXR_OK;
		}
	    }
	    m_pRootAtom->AddChild( pMdat );
	}
    }

    // Fetch the moov atom back so we can do some setup
    retVal = HXR_UNEXPECTED;
    pMoov = (CMP4Atom_moov*) m_pRootAtom->FindChild( "moov" );
    
    if( pMoov )
    {
	retVal = HXR_OK;
    }

    // Set up pMoov children
    // moov
    //   mvhd
    //   trak
    //   udta
    CMP4Atom_trak* pTrak;
    
    if( SUCCEEDED( retVal ) )
    {
	// mvhd
	retVal = HXR_OUTOFMEMORY;
	m_pMvhd = new CMP4Atom_mvhd();
	if( m_pMvhd )
	{
	    retVal = HXR_OK;

	    // Set the timescale (hz) but not duration
	    m_pMvhd->SetTimescale(1000);
	    
	    // XXX singletrack, so our track id is 1
	    m_pMvhd->SetNextTrackID( 2 );

	    pMoov->AddChild( m_pMvhd );
	}

	// trak
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pTrak = new CMP4Atom_trak();

	    if( pTrak )
	    {
		retVal = HXR_OK;

		// set up
		pMoov->AddChild( pTrak );
	    }
	}

	// udta
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_udta* pUdta = new CMP4Atom_udta();
	    if( pUdta )
	    {
		retVal = HXR_OK;

		// Call out to build the metadata tree
		BuildM4AMetaData( pUdta );

		pMoov->AddChild( pUdta );
	    }
	}
    }

    // Set up pTrak children
    // trak
    //   tkhd
    //   mdia
    CMP4Atom_mdia* pMdia;
    
    if( SUCCEEDED( retVal ) )
    {
	// tkhd
	retVal = HXR_OUTOFMEMORY;
	m_pTkhd = new CMP4Atom_tkhd();

	if( m_pTkhd )
	{
	    retVal = HXR_OK;

	    // XXX
	    m_pTkhd->SetTrackID(1);

	    pTrak->AddChild( m_pTkhd );
	}

	// mdia
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pMdia = new CMP4Atom_mdia();

	    if( pMdia )
	    {
		retVal = HXR_OK;

		pTrak->AddChild( pMdia );
	    }
	}
    }

    // Set up pMdia children
    // mdia
    //   mdhd
    //   hdlr
    //   minf
    CMP4Atom_minf* pMinf;

    if( SUCCEEDED( retVal ) )
    {
	// mdhd
	retVal = HXR_OUTOFMEMORY;
	m_pMdhd = new CMP4Atom_mdhd();

	if( m_pMdhd )
	{
	    retVal = HXR_OK;

	    // Set the language to undetermined
	    // The codes for this field are from ISO-639-2/T
	    m_pMdhd->SetLanguage('u','n','d');

	    pMdia->AddChild( m_pMdhd );

	}

	// hdlr
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_hdlr* pHdlr = new CMP4Atom_hdlr();
	    if( pHdlr )
	    {
		retVal = HXR_OK;

		// XXX fix up for multistream
		pHdlr->SetHandlerType("soun");
		pMdia->AddChild( pHdlr );
	    }
	}
	
	// minf
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pMinf = new CMP4Atom_minf();
	    if( pMinf )
	    {
		retVal = HXR_OK;
		pMdia->AddChild( pMinf );
	    }
	}
    }

    // Set up pMinf children
    // minf
    //   smhd
    //   dinf
    //     dref
    //       url
    //   stbl
    CMP4Atom_stbl* pStbl;

    if( SUCCEEDED( retVal ) )
    {
	// smhd. This atom is specific to audio
	retVal = HXR_OUTOFMEMORY;
	CMP4Atom_smhd* pSmhd = new CMP4Atom_smhd();

	if( pSmhd )
	{
	    retVal = HXR_OK;

	    pMinf->AddChild( pSmhd );
	}

	// dinf, dref, url
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_dinf* pDinf = new CMP4Atom_dinf();

	    if( pDinf )
	    {
		retVal = HXR_OK;
		pMinf->AddChild( pDinf );

		retVal = HXR_OUTOFMEMORY;
		CMP4Atom_dref* pDref = new CMP4Atom_dref();

		if( pDref )
		{
		    retVal = HXR_OK;
		    pDinf->AddChild( pDref );

		    retVal = HXR_OUTOFMEMORY;
		    CMP4Atom_url* pUrl = new CMP4Atom_url();

		    if( pUrl )
		    {
			retVal = HXR_OK;
			pDref->AddChild( pUrl );
		    }
		}
	    }
	}

	// stbl
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pStbl = new CMP4Atom_stbl();

	    if( pStbl )
	    {
		retVal = HXR_OK;
		pMinf->AddChild( pStbl );

	    }
	}
    }

    // Set up the stbl
    // stbl
    //   stsd
    //     mp4a
    //       esds
    //   stts
    //   stsc
    //   stsz
    //   stco
    if( SUCCEEDED( retVal ) )
    {
	// stsd
	retVal = HXR_OUTOFMEMORY;
	CMP4Atom_stsd* pStsd = new CMP4Atom_stsd();

	if( pStsd )
	{
	    retVal = HXR_OK;

	    pStbl->AddChild( pStsd );
	}

	if( SUCCEEDED( retVal ) )
	{
	    // Build out the stsd for our single stream
            // This CM4AAtomsGateway object is used to branch off to
            // type specific atoms build process. For M4A type, this is
            // equivalent to original direct call to BuildStsdEntry(pStsd, 0) 
            CM4AAtomsGateway AtomsGateway;
            retVal = AtomsGateway.BuildSpecificStsdEntry(this, pStsd, 0);
	}
		
	// stts
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_stts* pStts = new CMP4Atom_stts();

	    if( pStts )
	    {
		retVal = HXR_OK;

		// XXX not initialized
		pStbl->AddChild( pStts );
	    }
	}
	
	// stsc
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    CMP4Atom_stsc* pStsc = new CMP4Atom_stsc();

	    if( pStsc )
	    {
		retVal = HXR_OK;

		// XXX not initialized
		pStbl->AddChild( pStsc );
	    }
	}

	// stsz
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CMP4Atom_stsz* pStsz = new CMP4Atom_stsz();

	    if( pStsz )
	    {
		retVal = HXR_OK;

		pStbl->AddChild( pStsz );
	    }
	}

	// stco
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CMP4Atom_stco* pStco = new CMP4Atom_stco();

	    if( pStco )
	    {
		retVal = HXR_OK;

		pStbl->AddChild( pStco );
	    }
	}
	
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    
	    // pass off the stbl atom to the stbl manager
	    g_pStblManager = new CStblManager( pStbl );

	    if( g_pStblManager )
	    {
		retVal = HXR_OK;
	    }
	}
    }

    if( SUCCEEDED( retVal ) )
    {
	// Set the duration in our new atoms to the reported one
	UpdateDuration( m_ulReportedDuration );
    
	// Set the base offsets for our atoms.
	// In the default case, where a duration is available, all the atom
	// data is written in a padding block preceding the actual content.
	// This lets us set a write offset for the root atom at 0 and be done with
	// it.
	// When duration isn't available, the atoms are on both sides of the content,
	// so we need to specify the location for each top level atom and write them
	// individually

	UINT32 ulStblWriteOffset = 0;
	if( m_bMoovAtEnd )
	{
	    // Without a duration, we write the ftyp, followed immediately by mdat, 
	    pFtyp->NotifyWriteOffset( 0 );
	    pMdat->NotifyWriteOffset( pFtyp->GetCurrentSize() );

	    // Since we dont know where the moov atom will start, we dont set a
	    // writeoffset for this just yet. We do, however, need to update the stblmanager
	    ulStblWriteOffset = pFtyp->GetCurrentSize() + pMdat->GetCurrentSize();

	    m_uiReservedBlockCount = 0;
	}
	else
	{
	    m_pRootAtom->NotifyWriteOffset( 0 );
    
	    // Approximate how much space we need for the metadata and reserve a few blocks
	    // This is relatively accurate, given an accurate ReportedDuration.
	    
	    UINT32 ulSamplesPerSecond, ulEstimatedSize;
	    m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond", ulSamplesPerSecond);
	    
	    g_pStblManager->EstimateSize( m_ulReportedDuration, ulSamplesPerSecond, ulEstimatedSize );

	    m_uiReservedBlockCount = 1 + (ulEstimatedSize / M4A_RESERVE_BLOCK_SIZE);

	    // If our estimate puts the final block at 75% full or more, allocate space for another block
	    // since ulEstimatedSize only really applies for the stbl.
	    if( ( 3 * (M4A_RESERVE_BLOCK_SIZE / 4)) < (ulEstimatedSize % M4A_RESERVE_BLOCK_SIZE) )
	    {
		m_uiReservedBlockCount++;
	    }

	    ulStblWriteOffset = 8 + (m_uiReservedBlockCount * M4A_RESERVE_BLOCK_SIZE);
	}

	// Let the stbl manager know where in the file it will be writing
	g_pStblManager->NotifyBaseWriteOffset( ulStblWriteOffset );
    }
    
    return retVal;
}

HX_RESULT CM4AStreamMixer::BuildM4AMetaData( CMP4Atom* pUdta )
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    // The metadata tree looks like this
    // udta
    //   meta
    //     hdlr
    //     ilst
    //     free
    CM4AAtom_meta* pMeta = new CM4AAtom_meta();

    if( pMeta )
    {
	retVal = HXR_OK;

	pUdta->AddChild( pMeta );
    }

    CMP4Atom_free* pFree = NULL;
    CM4AAtom_ilst* pIlst = NULL;
    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_OUTOFMEMORY;

	// hdlr
	CM4AAtom_hdlr* pHdlr = new CM4AAtom_hdlr();
	if( pHdlr )
	{
	    retVal = HXR_OK;
	    
	    pMeta->AddChild( pHdlr );
	}
	
	// ilst
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pIlst = new CM4AAtom_ilst();

	    if( pIlst )
	    {
		retVal = HXR_OK;
		
		pMeta->AddChild( pIlst );
	    }
	}

	// free. only create this if we are putting moov in the middle
	if( !m_bMoovAtEnd )
	{
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    
	    pFree = new CMP4Atom_free();

	    if( pFree )
	    {
		retVal = HXR_OK;
		
		// reserve space for metadata
		pFree->SetFreeSize(M4A_DEFAULT_METADATA_RESERVE_SIZE);
		pMeta->AddChild( pFree );
	    }
	}
    }
    }

    // Now build out the ilst
    if( SUCCEEDED( retVal ) )
    {
	CM4AAtom_data* pData = NULL;
	IHXBuffer* pBuf = NULL;

	if( SUCCEEDED( m_pFileHeader->GetPropertyBuffer( "Author", pBuf ) ) && pBuf )
	{
	    retVal = HXR_OUTOFMEMORY;
	    
	    CM4AAtom_art* pArtist = new CM4AAtom_art();
	    if( pArtist )
	    {
		retVal = HXR_OUTOFMEMORY;
		pData = new CM4AAtom_data();
		if( pData )
		{
		    retVal = pData->SetString( pBuf->GetBuffer() );
		    if( SUCCEEDED( retVal ) )
		    {
			pArtist->AddChild( pData );
			pIlst->AddChild( pArtist );
			pData = NULL;
		    }
		}
	    }
	    HX_RELEASE( pBuf );
	}
	if( SUCCEEDED( m_pFileHeader->GetPropertyBuffer( "Title", pBuf ) ) && pBuf )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CM4AAtom_nam* pName = new CM4AAtom_nam();
	    if( pName )
	    {
		retVal = HXR_OUTOFMEMORY;
		pData = new CM4AAtom_data();
		if( pData )
		{
		    retVal = pData->SetString( pBuf->GetBuffer() );
		    if( SUCCEEDED( retVal ) )
		    {
			pName->AddChild( pData );
			pIlst->AddChild( pName );
			pData = NULL;
		    }
		}
	    }
	    HX_RELEASE( pBuf );
	}

	if( SUCCEEDED( m_pFileHeader->GetPropertyCString( "Generated By", pBuf ) ) && pBuf )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CM4AAtom_too* pTools = new CM4AAtom_too();
	    if( pTools )
	    {
		retVal = HXR_OUTOFMEMORY;
		pData = new CM4AAtom_data();
		if( pData )
		{
		    retVal = pData->SetString( pBuf->GetBuffer() );
		    if( SUCCEEDED( retVal ) )
		    {
			pTools->AddChild( pData );
			pIlst->AddChild( pTools );
			pData = NULL;
		    }
		}
	    }
	    HX_RELEASE( pBuf );
	}
	
	if( SUCCEEDED( m_pFileHeader->GetPropertyBuffer( "Genre", pBuf ) ) && pBuf )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CM4AAtom_gen* pGenre = new CM4AAtom_gen();
	    if( pGenre )
	    {
		retVal = HXR_OUTOFMEMORY;
		pData = new CM4AAtom_data();
		if( pData )
		{
		    retVal = pData->SetString( pBuf->GetBuffer() );
		    if( SUCCEEDED( retVal ) )
		    {
			pGenre->AddChild( pData );
			pIlst->AddChild( pGenre );
			pData = NULL;
		    }
		}
	    }
	    HX_RELEASE( pBuf );
	}
	if( SUCCEEDED( m_pFileHeader->GetPropertyBuffer( "Album", pBuf ) ) && pBuf )
	{
	    retVal = HXR_OUTOFMEMORY;

	    CM4AAtom_alb* pAlbum = new CM4AAtom_alb();
	    if( pAlbum )
	    {
		retVal = HXR_OUTOFMEMORY;
		pData = new CM4AAtom_data();
		if( pData )
		{
		    retVal = pData->SetString( pBuf->GetBuffer() );
		    if( SUCCEEDED( retVal ) )
		    {
			pAlbum->AddChild( pData );
			pIlst->AddChild( pAlbum );
			pData = NULL;
		    }
		}
	    }
	    HX_RELEASE( pBuf );
	}

    }

    // Resize our free atom (if present) given our new metadata
    if( SUCCEEDED( retVal ) && pFree )
    {
	UINT32 size = pUdta->GetCurrentSize( TRUE );
	size -= M4A_DEFAULT_METADATA_RESERVE_SIZE;
	
	pFree->SetFreeSize( M4A_DEFAULT_METADATA_RESERVE_SIZE - size );
    }
    
    return retVal;
}

HX_RESULT CM4AStreamMixer::StartBuildStsdEntry( CMP4Atom_stsd* pStsd, UINT16 usStreamNum )
{
    return BuildStsdEntry(pStsd, usStreamNum);
}

// Build out the stsd entry for a specific stream; we only have one stream in this case, but
// extending this to handle multiple streams should be easy
HX_RESULT CM4AStreamMixer::BuildStsdEntry( CMP4Atom_stsd* pStsd, UINT16 usStreamNum )
{
    // this would index on an array
    IHXValues* pStreamHeader = m_pStreamHeader;
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    IHXBuffer* pDecoderInfo = NULL;

    // the stream handler is supposed to build us a decoderinfo block to assist us
    // when building out this atom; look for it now
    retVal = pStreamHeader->GetPropertyBuffer("DecoderInfo", pDecoderInfo);

    if( SUCCEEDED( retVal ) )
    {
	UCHAR* pBuf = pDecoderInfo->GetBuffer();
	UINT32 len  = pDecoderInfo->GetSize();
	// here instead of assuming mp4a we would switch on a mime type perhaps
	CMP4Atom_mp4a* pMP4A = new CMP4Atom_mp4a();
	if( pMP4A )
	{
	    retVal = HXR_OK;
	    
	    UINT32 ulSampleRate = 1000;
            m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond", ulSampleRate);

            pMP4A->SetTimescale((UINT16) ulSampleRate);
	    
	    pStsd->AddChild( pMP4A );
	}

	CMP4Atom_esds* pEsds = NULL;
	
	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;
	    pEsds = new CMP4Atom_esds();
	    if( pEsds )
	    {
		retVal = HXR_OK;
	    }
	}

	if( SUCCEEDED( retVal ) )
	{
	    UINT32 ulBitRate = 0;
	    // Set up the esds
	    pEsds->SetObjectType( pBuf[0] );
	    pEsds->SetStreamType( pBuf[1] );

	    pStreamHeader->GetPropertyULONG32("AvgBitRate", ulBitRate);
	    pEsds->SetAvgBitrate( ulBitRate );
	    
	    pStreamHeader->GetPropertyULONG32("MaxBitRate", ulBitRate);
	    pEsds->SetMaxBitrate( ulBitRate );

	    // XXX TODO this is baseless and arbitrary
	    pEsds->SetBufferSizeDB( 6 * 1024 );

	    // Set the rest of the decoder specific info after the two values we used
	    pEsds->SetDecoderSpecificInfo( &( pBuf[2] ), len - 2 );

	    pMP4A->AddChild( pEsds );
	}
	HX_RELEASE( pDecoderInfo );
    }
    
    return retVal;
}

HX_RESULT CM4AStreamMixer::UpdateDuration( UINT32 ulDuration )
{
    HX_RESULT retVal = HXR_OK;

    m_pMvhd->SetDuration( ulDuration );
    m_pTkhd->SetDuration( ulDuration );
    
#ifdef SAMPLERATE_AS_MEDIA_TIMESCALE
    UINT32 ulSampleRate = 0;
    m_pStreamHeader->GetPropertyULONG32( "SamplesPerSecond", ulSampleRate );
    // XXX This uses one equal to that of the content sample rate
    m_pMdhd->SetTimescale(ulSampleRate);
    
    UINT64 u64Duration = (UINT64) ulDuration;
    u64Duration = (u64Duration * ulSampleRate ) / 1000;
    
    m_pMdhd->SetDuration((UINT32)u64Duration);
#else
    // This uses a timescale equal to that of the track header
    m_pMdhd->SetTimescale(1000);
    m_pMdhd->SetDuration(ulDuration);
#endif
    
    return retVal;
}


HX_RESULT CM4AStreamMixer::DumpAtoms()
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    IHXBuffer* pBuffer = NULL;
    
    CreateBufferCCF(pBuffer, m_pContext);
    if( pBuffer )
    {
	UINT32 size = m_pRootAtom->GetCurrentSize(TRUE);
	UCHAR* pBuf, *p;
	printf("atoms, total size %d\n", size );
	pBuffer->SetSize( size );
	pBuf = pBuffer->GetBuffer();
	
	// reference param
	p = pBuf;
	m_pRootAtom->WriteToBuffer( p, TRUE );

	printf("writing done, dumping to a file\n");
	IHXPacket* pPacket = new CHXPacket();

	if( pPacket )
	{
	    pPacket->AddRef();
	    pPacket->Set(pBuffer, 0, 0, 0, 0);
	    m_pArchiver->PacketReady( pPacket );
	    HX_RELEASE(pPacket);
	}
	HX_RELEASE( pBuffer );
    }
    return retVal;
}


