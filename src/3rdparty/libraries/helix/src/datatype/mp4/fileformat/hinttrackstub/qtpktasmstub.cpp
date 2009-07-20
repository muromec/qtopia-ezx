/****************************************************************************
 * 
 *  $Id: qtpktasmstub.cpp,v 1.8 2005/04/27 13:57:41 ehyche Exp $
 *
 *  Copyright (C) 1995-1999 RealNetworks, Inc. All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary 
 *  information of Progressive Networks, Inc, and is licensed
 *  subject to restrictions on use and distribution.
 *
 *
 *  Packet Assembler
 *
 */

/****************************************************************************
 *  Includes
 */
#define QTASM_MAX_SEGMENT_FRAG   20

/****************************************************************************
 *  Includes
 */
#include "qtffplin.h"
#include "qtpktasm.h"
#include "qtffrefcounter.h"

/****************************************************************************
 *  Class CQTPacketAssembler
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTPacketAssembler::CQTPacketAssembler(void)
    : m_pSample(NULL)
    , m_pHintTrack(NULL)
    , m_pDataTrack(NULL)
    , m_pClassFactory(NULL)
    , m_pFileFormat(NULL)
    , m_pPacket(NULL)
    , m_pCurrentSegmentStart(NULL)
    , m_ulCurrentSegmentSize(0)
    , m_ulCurrentSegmentIdx(0)
    , m_ulSegmentFragments(0)
    , m_lRefCount(0)
{
    g_nRefCount_qtff++;
}

CQTPacketAssembler::~CQTPacketAssembler()
{
    g_nRefCount_qtff--;
}

/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQTPacketAssembler::Init(CQTFileFormat* pFileFormat)
{
    return HXR_OK;
}

/****************************************************************************
 *  SegmentReady
 */
HX_RESULT CQTPacketAssembler::SegmentReady(HX_RESULT status,
					   IHXBuffer *pBuffer,
					   ULONG32 ulOffset,
					   ULONG32 ulSize)
{
    return HXR_NOTIMPL;
}

/****************************************************************************
 *  DisablePacketTimeOffset
 */
void CQTPacketAssembler::DisablePacketTimeOffset(HXBOOL bDoDisable)
{
    ;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CQTPacketAssembler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CQTPacketAssembler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CQTPacketAssembler::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
