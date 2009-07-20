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
// m4ash.cpp: m4a stream handler

#include "hxcom.h"
#include "hxcomm.h"
#include "chxpckts.h"
#include "hxassert.h"

#include "m4asm.h"
#include "m4ash.h"

#include "mp4desc.h"    /* for parsing es descriptor/channel count */
#include "gaConfig.h"
#include "mp4atoms.h"   /* for esds, will be removed later */
#include "aacconstants.h"

// These are the mime types we support
const char* CM4AStreamHandler::m_pszFileMimeTypes[] =
{
    "audio/x-rn-mp4-rawau",
    NULL
};
const char* CM4AStreamHandler::m_pszFileExtensions[] =
{
    NULL
};
const char* CM4AStreamHandler::m_pszFileOpenNames[] =
{
    NULL
};

// CM4AStreamHandler code
CM4AStreamHandler::CM4AStreamHandler( IUnknown* pContext, IMP4StreamMixer* pMixer )
{
    m_lRefCount = 0;
    m_unStreamNumber = 0;

    HX_ASSERT(pMixer);
    pMixer->AddRef();
    m_pStreamMixer = pMixer;

    HX_ASSERT(pContext);
    pContext->AddRef();
    m_pContext = pContext;

    m_pCommonClassFactory = NULL;
}

CM4AStreamHandler::~CM4AStreamHandler()
{
    HX_RELEASE( m_pStreamMixer );
    HX_RELEASE( m_pContext );
    HX_RELEASE( m_pCommonClassFactory );
}

STDMETHODIMP CM4AStreamHandler::QueryInterface( REFIID riid, void** ppvObj )
{
    // dont support, for now
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CM4AStreamHandler::AddRef()
{
    return InterlockedIncrement( &m_lRefCount );
}

STDMETHODIMP_(ULONG32) CM4AStreamHandler::Release()
{
    if( InterlockedDecrement( &m_lRefCount ) > 0 )
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP CM4AStreamHandler::GetFormatInfo( const char*** pppFileMimeTypes, const char*** pppFileExtensions, const char*** pppFileOpenNames )
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
STDMETHODIMP CM4AStreamHandler::InitStreamHandler()
{
    HX_RESULT retVal;
    
    retVal = m_pContext->QueryInterface( IID_IHXCommonClassFactory, (void**) &m_pCommonClassFactory );

    return retVal;
}

STDMETHODIMP CM4AStreamHandler::SetFileHeader( IHXValues* pHeader )
{
    // We dont need the fileheader at all
    return HXR_OK;
}

// SetStreamHeader; called when the stream header for this stream is available
STDMETHODIMP CM4AStreamHandler::SetStreamHeader( IHXValues* pHeader )
{
    HX_RESULT retVal = HXR_OK;
    
    UINT32 ulStreamNumber;
    
    pHeader->GetPropertyULONG32("StreamNumber",     ulStreamNumber);
    m_unStreamNumber = (UINT16) ulStreamNumber;


    IHXBuffer* pOpaqueData = NULL;
    UINT8* ucDecoderInfo = NULL;
    UINT32 ulDSILength;

    retVal = pHeader->GetPropertyBuffer( "OpaqueData", pOpaqueData );

    if( SUCCEEDED( retVal ) )
    {
	retVal = GetASCFromESD( pOpaqueData, ucDecoderInfo, ulDSILength );
    }

    HX_RELEASE( pOpaqueData );
	

    if( SUCCEEDED( retVal ) )
    {
	IHXBuffer* pBuffer = NULL;
    
	retVal = HXR_OUTOFMEMORY;
	m_pCommonClassFactory->CreateInstance( IID_IHXBuffer, (void**) &pBuffer );

	if( pBuffer )
	{
	    UCHAR* pBuf;
	    retVal = pBuffer->SetSize( ulDSILength + 2 );
	    if( SUCCEEDED( retVal ) )
	    {
		pBuf = pBuffer->GetBuffer();
	    
		// ObjectTypeIndication
		pBuf[0] = CMP4Atom_esds::ObjectTypeIndicationValues::OTI_Audio_14496_3;
	    
		// StreamType
		pBuf[1] = CMP4Atom_esds::DecoderConfigStreamTypes::ST_AudioStream;
	    
		// Copy in the decoder specific info
		memcpy( pBuf + 2, ucDecoderInfo, ulDSILength );
		
		pHeader->SetPropertyBuffer("DecoderInfo", pBuffer );

		HX_RELEASE( pBuffer );
	    
		m_pStreamMixer->SetStreamHeader( pHeader );
		    retVal = HXR_OK;
	    }
	}
    }

    HX_DELETE( ucDecoderInfo );
    
    return retVal;
}

HX_RESULT CM4AStreamHandler::GetASCFromESD( IHXBuffer* pASC, UINT8*& pDSI, UINT32& ulDSILength )
{
    HX_RESULT retVal;

    UINT8* pBuf = pASC->GetBuffer();
    UINT32 ulSize = pASC->GetSize();
    ES_Descriptor ESDesc;
    retVal = ESDesc.Unpack( pBuf, ulSize );

    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_FAIL;

	// For the moment we only support MP4 audio
	if( ESDesc.m_pDecConfigDescr                              &&
	    ESDesc.m_pDecConfigDescr->m_pDecSpecificInfo          &&
	    ESDesc.m_pDecConfigDescr->m_uObjectProfileIndication == MP4OBJ_AUDIO_ISO_IEC_14496_3_D )
	    {
		UINT8* pData = ESDesc.m_pDecConfigDescr->m_pDecSpecificInfo->m_pData;
		UINT32 len = ESDesc.m_pDecConfigDescr->m_pDecSpecificInfo->m_ulLength;

		retVal = HXR_OUTOFMEMORY;

		pDSI = new UINT8[len];
		if( pDSI )
		{
		    memcpy( pDSI, pData, len );
		    ulDSILength = len;
		    retVal = HXR_OK;
		}
	    }
    }

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
STDMETHODIMP CM4AStreamHandler::SetPacket( IHXPacket* pPacket )
{
    HX_RESULT retVal = HXR_FAIL;
    if( pPacket )
    {
	// pass through
	retVal = m_pStreamMixer->SetPacket( pPacket );
    }
    return HXR_OK;
}

// StreamDone; called after the final SetPacket() call for this stream
STDMETHODIMP CM4AStreamHandler::StreamDone()
{
    m_pStreamMixer->StreamDone( m_unStreamNumber );
    return HXR_OK;
}

