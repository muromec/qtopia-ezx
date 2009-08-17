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
// ra10sh.cpp: realaudio10 stream handler
// this is an initial version designed primarily to remove
// the formatting from ra10 payload data and generate raw
// aac, which can then be written out to a file. longer term,
// instead of having a few local routines (which generate .m4a
// formatting and interact directly with the archiver), there
// would be a layout object which would handle blending all
// streams as required. to this end, temporary portions of this
// file have been marked as such.

#include "hxcom.h"
#include "hxcomm.h"
#include "hxiids.h"
#include "chxpckts.h"
#include "hxassert.h"
#include "hxtypes.h"

#include "m4asm.h"
#include "ra10sh.h"

#include "vbrdepack.h"
#include "raparser.h"  /* datatype/rm/audio/common/pub/raparser.h, streamparam */
#include "rmfftype.h"  /* common/include/rmfftype.h, multistreamheader */
#include "netbyte.h"   /* common/util/pub/netbyte.h, dwtohost */
#include "aacconstants.h" /* datatype/mp4/common/pub/, for eAACConfigAudioSpecificCfg */

#include "mp4atoms.h"  /* this is for objecttype and streamtype; fix later */

// These are the mime types we support
const char* CRA10StreamHandler::m_pszFileMimeTypes[] =
{
    "audio/x-pn-realaudio",
    NULL
};
const char* CRA10StreamHandler::m_pszFileExtensions[] =
{
    NULL
};
const char* CRA10StreamHandler::m_pszFileOpenNames[] =
{
    NULL
};

// PrepM4AFile: Write out the ftyp header
//static void PrepM4A(IMP4);

// PrepM4AHeader: Write out metadata headers (udta)
//static void PrepM4AHeader(CBaseArchiver2* pArchiver, IHXValues* pStreamHeader);

// CRA10StreamHandler code
CRA10StreamHandler::CRA10StreamHandler( IUnknown* pContext, IMP4StreamMixer* pMixer )
{
    m_lRefCount = 0;
    m_unStreamNumber = 0;
    m_uiRTPTimestamp = 0;

    HX_ASSERT(pMixer);
    pMixer->AddRef();
    m_pStreamMixer = pMixer;
    
    HX_ASSERT( pContext );
    pContext->AddRef();
    m_pContext = pContext;

    m_pCommonClassFactory = NULL;
    m_pDepacketizer = NULL;
}

CRA10StreamHandler::~CRA10StreamHandler( )
{
    HX_RELEASE( m_pStreamMixer );
    HX_RELEASE( m_pCommonClassFactory );
    HX_RELEASE( m_pContext );

    HX_DELETE( m_pDepacketizer );
}

STDMETHODIMP CRA10StreamHandler::QueryInterface( REFIID riid, void** ppvObj )
{
    // dont support, for now
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CRA10StreamHandler::AddRef()
{
    return InterlockedIncrement( &m_lRefCount );
}

STDMETHODIMP_(ULONG32) CRA10StreamHandler::Release()
{
    if( InterlockedDecrement( &m_lRefCount ) > 0 )
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP CRA10StreamHandler::GetFormatInfo( const char*** pppFileMimeTypes, const char*** pppFileExtensions, const char*** pppFileOpenNames )
{
    if( pppFileMimeTypes )
    {
	*pppFileMimeTypes = m_pszFileMimeTypes;
    }
    
    if( pppFileExtensions )
    {
	*pppFileExtensions = m_pszFileExtensions;
    }
    if( pppFileOpenNames )
    {
	*pppFileOpenNames = m_pszFileOpenNames;
    }
    
    return HXR_OK;
}

// InitStreamHandler; this is called after the mp4 filewriter
// has determined it has a stream that actually matches our supported
// mime types, and allows us to do any initialization that would be
// inappropriate in the constructor (such as memory allocation)
STDMETHODIMP CRA10StreamHandler::InitStreamHandler()
{
    HX_RESULT retVal;

    retVal = m_pContext->QueryInterface( IID_IHXCommonClassFactory, (void**) &m_pCommonClassFactory );
    
    return retVal;
}

STDMETHODIMP CRA10StreamHandler::SetFileHeader( IHXValues* pHeader )
{
    HX_RESULT retVal = HXR_OK;
    pHeader->AddRef();

    // ra10 has strange metadata, such that the album name, track title, and genre all live in a field called "Title"
    IHXBuffer* pTitle = NULL;
    if( SUCCEEDED( pHeader->GetPropertyBuffer( "Title", pTitle ) ) )
    {
	IHXBuffer* pItems[3];

	UCHAR* pBuffer = pTitle->GetBuffer();

	int start = 0;
	int element = 0;
	memset( pItems, 0, 3 * sizeof(IHXBuffer*) );
	
	// Hmm, i wonder what happens if one of the strings has a / in it
	for( int i = 0, sz = pTitle->GetSize(); i < sz && SUCCEEDED( retVal ); i++ )
	{
	    if( pBuffer[i] == '/' || i == (sz-1) )
	    {
		if( i != (sz-1) )
		{
		    pBuffer[i] = 0;
		}
		if( start != i )
		{
		    retVal = m_pCommonClassFactory->CreateInstance( IID_IHXBuffer, (void**) &( pItems[element] ) );
		    if( SUCCEEDED( retVal ) )
		    {
			retVal = pItems[element]->Set( &( pBuffer[start] ), (i - start) + 1 );
		    }
		}
		start = i+1;
		element++;
	    }
	}
	HX_RELEASE( pTitle );

	if( pItems[0] && SUCCEEDED( retVal ) )
	{
	    pHeader->SetPropertyBuffer( "Album", pItems[0] );
	    HX_RELEASE( pItems[0] );
	}
	// With the title, we have a dilemma; if no title was present but an artist and genre were,
	// we end up letting the a//g value pass through; this isn't what the writer wants. Our solution
	// is to NULL the existing buffer for the "Title", and make sure the writer only writes non-null
	// buffers. The alternative would be to have all other writers rename the title to a new field
	if( SUCCEEDED( retVal ) )
	{
	    pHeader->SetPropertyBuffer( "Title", pItems[1] );
	    HX_RELEASE( pItems[1] );
	}
	if( pItems[2] && SUCCEEDED( retVal ) )
	{
	    pHeader->SetPropertyBuffer( "Genre", pItems[2] );
	    HX_RELEASE( pItems[2] );
	}
    }
    
    pHeader->Release();
    return retVal;
}

// SetStreamHeader; called when the stream header for this stream is available
STDMETHODIMP CRA10StreamHandler::SetStreamHeader( IHXValues* pHeader )
{
    UINT32 ulStreamNumber;
    UINT32 ulSampleRate;
    UINT32 ulChannels;
    IHXBuffer* pOpaqueData = NULL;
    HX_RESULT retVal = HXR_FAIL;
    double fmsPerBlock = 0.0;
    UINT8* ucDecoderInfo = NULL;
    UINT32 uiDSILength = 0;
    
    pHeader->GetPropertyULONG32("StreamNumber",     ulStreamNumber);
    
    m_unStreamNumber = (UINT16) ulStreamNumber;
    
    // We have to perform this ridiculous sequence of steps to get the sample rate and channel count
    retVal = pHeader->GetPropertyBuffer("OpaqueData", pOpaqueData );
    if( SUCCEEDED( retVal ) ) {
	UCHAR* pCursor = NULL;
	UINT32 ulRAHeaderSize = 0;
	UCHAR* pBuffer = pOpaqueData->GetBuffer();
	UINT32 ulLen = pOpaqueData->GetSize();
	UINT32 ulID;
	
	memcpy( (void*)&ulID, pBuffer, sizeof(UINT32) );

	ulID = DwToHost(ulID);
	if( RM_MULTIHEADER_OBJECT == ulID )
	{
	    MultiStreamHeader multiStreamHeader;

	    pCursor = multiStreamHeader.unpack( pBuffer, ulLen );
	    ulLen -= (pCursor - pBuffer);

	    if( multiStreamHeader.num_headers > 1 )
	    {
		// We dont support multiple substreams atm
		retVal = HXR_FAIL;
	    }

	    memcpy( (void*)&ulRAHeaderSize, pCursor, sizeof(UINT32) );
	    pCursor += sizeof(UINT32);
	}
	else if( RA_FILE_MAGIC_NUMBER == ulID )
	{
	    pCursor = pBuffer;
	    ulRAHeaderSize = ulLen;
	}
	else
	{
	    retVal = HXR_FAIL;
	}

	if( SUCCEEDED( retVal ) )
	{
	    CStreamParam StreamParam;
	    retVal = StreamParam.ReadOneRAHeader( pCursor, ulRAHeaderSize );

	    if( SUCCEEDED( retVal ) )
	    {
		retVal = HXR_FAIL;
		
		if( (strcmp( StreamParam.codecID, "raac" ) == 0  ||
		     strcmp( StreamParam.codecID, "racp" ) == 0) &&
		    (strcmp( StreamParam.interleaverID, "vbrs" ) == 0 ||
		     strcmp( StreamParam.interleaverID, "vbrf" ) == 0) )
		{
		    // If the AudioSpecificConfig is available, save a copy of it
		    if( StreamParam.opaqueData[0] == eAACConfigAudioSpecificCfg )
		    {
			uiDSILength = StreamParam.ulOpaqueDataSize - 1;
			uiDSILength += 2; // space for OTI
			ucDecoderInfo = new UINT8[uiDSILength];

			retVal = HXR_OUTOFMEMORY;
			if( ucDecoderInfo )
			{
			    memcpy( ucDecoderInfo + 2, (StreamParam.opaqueData) + 1, uiDSILength );
			    retVal = HXR_OK;
			}
		    }
		    
		    ulSampleRate = StreamParam.ulSampleRate;
		    ulChannels   = StreamParam.uChannels;
	    
		    // Set this for the assembler, which needs it for timestamp info
		    pHeader->SetPropertyULONG32("SamplesPerSecond", ulSampleRate);

		    // Determine ms per block
		    if( StreamParam.ulBytesPerMin )
		    {
			fmsPerBlock =
			    ((double)(StreamParam.uInterleaveBlockSize * 60000)) / StreamParam.ulBytesPerMin;
		    }
		    
		    retVal = HXR_OK;
		}
	    }
	}

	HX_RELEASE( pOpaqueData );
    }
    
    // Build a simple audio specific config if we did not find one in the ra header
    if( SUCCEEDED( retVal ) && !ucDecoderInfo )
    {
	UCHAR  ucChannelConfiguration = (UCHAR) (ulChannels == 8 ? 7 : ulChannels);
	UINT16 usDSI = 0;

	retVal = HXR_OUTOFMEMORY;
	uiDSILength = 4;
	ucDecoderInfo = new UINT8[uiDSILength];
    
	if( ucDecoderInfo )
	{
	    // Determine sampling frequency index
	    // 14496-3 3rd sp1 / 1.6.3.3
	    UCHAR  ucSamplingFrequencyIndex = 0;
	    static const UINT32 SamplingFrequencies[] =
	    { 96000, 88200, 64000, 48000, 44100, 32000,
	      24000, 22050, 16000, 12000, 11025,  8000, 7350 };

	    retVal = HXR_FAIL;
	    for( int i = 0; i < sizeof(SamplingFrequencies); i++ )
	    {
		if( SamplingFrequencies[i] == ulSampleRate )
		{
		    ucSamplingFrequencyIndex = i;
		    retVal = HXR_OK;
		    break;
		}
	    }
	
	    // 14496-3 sp1
	    // Upper 5 bits: AudioObjectType. for our purposes we use 0x02 (AAC LC)
	    // Next  4 bits: SamplingFrequencyIndex
	    // Next  4 bits: ChannelConfiguration (number of channels - note that 7.1 is actually 7 here)
	    // 14496-3 sp4
	    // Next  1 bits: FrameLengthFlag
	    // Next  1 bits: DependsOnCoreCoder
	    // Next  1 bits: ExtensionFlag

	    usDSI = (0x02 << 11) | (ucSamplingFrequencyIndex << 7) | (ucChannelConfiguration << 3) | 0;

	    ucDecoderInfo[2] = (UCHAR) (usDSI >>    8);
	    ucDecoderInfo[3] = (UCHAR) (usDSI &  0xff);

	    retVal = HXR_OK;
	}
    }

    // Generate the OTI and stream type, copy the buffer to the streamheader
    if( SUCCEEDED( retVal ) )
    {
	// ObjectTypeIndication
	ucDecoderInfo[0] = CMP4Atom_esds::ObjectTypeIndicationValues::OTI_Audio_14496_3;
	// StreamType
	ucDecoderInfo[1] = CMP4Atom_esds::DecoderConfigStreamTypes::ST_AudioStream;
    
	retVal = HXR_OUTOFMEMORY;
	
	IHXBuffer* pBuffer = NULL;
	m_pCommonClassFactory->CreateInstance( IID_IHXBuffer, (void**) &pBuffer );

	if( pBuffer )
	{
	    pBuffer->Set( ucDecoderInfo, uiDSILength );
	    pHeader->SetPropertyBuffer( "DecoderInfo", pBuffer );

	    HX_RELEASE( pBuffer );
	    
	    retVal = HXR_OK;
	}
    }

    // Final step, initialize our depacketizer
    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_OUTOFMEMORY;
	m_pDepacketizer = new CVBRSimpleDepacketizer();
		
	if( m_pDepacketizer )
	{
	    m_pDepacketizer->Init( m_pContext, fmsPerBlock, m_unStreamNumber, 0, FALSE );
	    
	    m_pStreamMixer->SetStreamHeader( pHeader );
	    retVal = HXR_OK;
	}
    }
    
    HX_DELETE( ucDecoderInfo );
    
    return retVal;
}
static void HexDump( UCHAR* p, int len )
{
    UINT32* intp = (UINT32*) p;
    int i;
    int words = len >> 2;
    for( i = 0; i < (words-3); i += 4 )
    {
	printf("[%04x]: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	    i<<2, intp[i], intp[i+1], intp[i+2], intp[i+3]);
    }
    if( (i << 2) < len )
    {
	words++;
    }
    if( i < words )
    {
	printf("[%04x]:", i<<2);
	for( ; i < words; i++ )
	{
	    printf(" 0x%08x", intp[i]);
	}
	printf("\n");
    }
}

// SetPacket; called every time a packet comes in
STDMETHODIMP CRA10StreamHandler::SetPacket( IHXPacket* pPacket )
{
    HX_RESULT retVal = HXR_OK;
    if( pPacket )
    {
	IHXPacket* pDepackOut = NULL;

	m_pDepacketizer->PutPacket( pPacket );
	m_pDepacketizer->GetPacket( pDepackOut );

	HX_ASSERT( pDepackOut );
	while( pDepackOut && SUCCEEDED( retVal ) )
	{
	    // Convert this to an rtp packet
	    IHXRTPPacket* pRTPPacket = NULL;
	    
	    retVal = m_pCommonClassFactory->CreateInstance( IID_IHXRTPPacket, (void**) &pRTPPacket );
	    if( SUCCEEDED( retVal ) )
	    {
		ULONG32 ulRTPTime = m_uiRTPTimestamp;
		ULONG32 ulTime = 0;
		UINT16  uStreamNumber = 0, unASMRuleNumber = 0;
		UINT8   unASMFlags = 0;
		IHXBuffer* pBuf = NULL;
		retVal = pDepackOut->Get( pBuf, ulTime, uStreamNumber, unASMFlags, unASMRuleNumber );

		if( SUCCEEDED( retVal ) )
		{
		    retVal = pRTPPacket->SetRTP( pBuf, ulTime, ulRTPTime, uStreamNumber, unASMFlags, unASMRuleNumber );
		    if( SUCCEEDED( retVal ) )
		    {
			m_uiRTPTimestamp += 1024;
			retVal = m_pStreamMixer->SetPacket( pRTPPacket );
		    }
		    HX_RELEASE( pBuf );
		}
	    }
	    
	    HX_RELEASE( pRTPPacket );
	    
	    HX_RELEASE( pDepackOut );
	    m_pDepacketizer->GetPacket( pDepackOut );
	}
    }
    return HXR_OK;
}

// StreamDone; called after the final SetPacket() call for this stream
STDMETHODIMP CRA10StreamHandler::StreamDone()
{
    m_pStreamMixer->StreamDone( m_unStreamNumber );
    return HXR_OK;
}
