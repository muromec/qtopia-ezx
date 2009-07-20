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

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "riff.h"
#include "riffres.h"
#include "hxassert.h"
#include "hxcomm.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif



#define LIST_CHUNK_ID 0x4c495354    /* 'LIST' */
#define IDX1_CHUNK_ID 0x69647831 /* 'INDEX' */
#define IFF_FILE_MAGIC_NUMBER 0x464f524d  /* 'FORM' */

STDMETHODIMP CRIFFReader::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList  qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this},
	{ GET_IIDHANDLE(IID_IHXFileResponse), (IHXFileResponse*)this},
	{ GET_IIDHANDLE(IID_IHXThreadSafeMethods), (IHXThreadSafeMethods*)this},
    };

    HX_RESULT retVal = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    if (FAILED(retVal) && m_pResponse)
    {
        retVal = m_pResponse->QueryInterface(riid, ppvObj);
    }
    return retVal;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::AddRef
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) CRIFFReader::AddRef()
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
STDMETHODIMP_(ULONG32) CRIFFReader::Release()
{
    if ( InterlockedDecrement(&m_lRefCount) > 0 )
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

CRIFFReader::CRIFFReader(IUnknown* pContext,
                         CRIFFResponse* pResponse,
                         IHXFileObject* pFileObject)
    : m_pReassemblyBuffer(NULL)
    , m_ulChunkBytesRead(0)
    , m_ulChunkSize(0)
{
    m_pContext = pContext;
    m_pResponse = pResponse;
    m_pFileObject = pFileObject;

    if ( m_pFileObject )
    {
        m_pFileObject->AddRef();
    }

    if ( m_pContext )
        m_pContext->AddRef();

    if ( m_pResponse )
        m_pResponse->AddRef();

    m_bFileIsOpen = FALSE;
    m_lRefCount = 0;
    m_pFilename = 0;
    m_ulSizeDiff = 0;
    m_ulThisChunkOffset = 0;

    //Every time we Read() a data chunk and the amount of the Read is
    // odd, we add one to byte-align for the next Read, but we want to
    // know that we did this in ReadDone() so we use the following to
    // keep track so, in ReadDone(), we can reduce the buffer size by
    // one if necessary so the renderer will get just the bytes it
    // needs:
    m_ulFileSpecifiedReadSize = 0;
}

CRIFFReader::~CRIFFReader()
{
    if ( m_bFileIsOpen )
        Close();

    if ( m_pFilename )
    {
        delete [] m_pFilename;
        m_pFilename = 0;
    }
}

HX_RESULT
CRIFFReader::Open(char* filename)
{
    if ( !m_pFileObject )
        return HXR_UNEXPECTED;

    if ( filename )
    {
        m_pFilename = new char[strlen(filename) + 1];
        strcpy(m_pFilename, filename); /* Flawfinder: ignore */
    }
    m_state = RS_InitPending;

    m_ulCurOffset = 0;
    m_levelInfo[0].m_startOffset = 0;
    m_ulLevel = 0;

/*
    if(filename)
    {
    IHXRequest* pRequest;

    if (m_pFileObject->GetRequest(pRequest) == HXR_OK)
    {
        pRequest->SetURL(filename);
        pRequest->Release();
    }
    else
    {
        return HXR_FAILED;
    }
    }
*/

    return m_pFileObject->Init(HX_FILE_READ | HX_FILE_BINARY, this);

//    return HXR_OK;
}

STDMETHODIMP
CRIFFReader::InitDone(HX_RESULT status)
{

    if ( status != HXR_OK )
    {
        m_state = RS_Ready;
        m_pResponse->RIFFOpenDone(status);
        return HXR_OK;
    }

    m_bFileIsOpen = TRUE;

    m_state = RS_GetFileTypePending;
    return m_pFileObject->Read(sizeof(UINT32) * 2);
}

HX_RESULT
CRIFFReader::Close()
{
    if ( m_pFileObject )
    {
        m_pFileObject->Close();
        m_pFileObject->Release();
        m_pFileObject = NULL;
    }

    if ( m_pContext )
    {
        m_pContext->Release();
        m_pContext = NULL;
    }

    if ( m_pResponse )
    {
        m_pResponse->Release();
        m_pResponse = NULL;
    }

    m_bFileIsOpen = FALSE;

    return HXR_OK;
}

HX_RESULT
CRIFFReader::FindChunk(UINT32 id, HXBOOL bRelative)
{
    m_ulFindChunkId = id;
    m_state = RS_FileStartSeekPending;

    if ( bRelative && m_levelInfo[m_ulLevel].started )
        m_ulSeekOffset = m_levelInfo[m_ulLevel].m_nextChunkOffset;
    else
        m_ulSeekOffset = m_levelInfo[m_ulLevel].m_startOffset;

    m_levelInfo[m_ulLevel].started = TRUE;

    return(m_pFileObject->Seek(m_ulSeekOffset, FALSE) );
}

HX_RESULT
CRIFFReader::GetChunk()
{
    m_state = RS_ReadChunkHeaderPending;
    return(m_pFileObject->Read(sizeof(UINT32) * 2) ); /* Get type and len */
}

HX_RESULT
CRIFFReader::FindNextChunk()
{
    m_state = RS_GetChunkHeaderPending;
    return(m_pFileObject->Read(sizeof(UINT32) * 2) ); /* Get type and len */ 
}

STDMETHODIMP
CRIFFReader::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    UCHAR* buf;
    UINT32 len;
    UINT16 uRem = 0;

    HX_RESULT result = HXR_OK;

    if ( !m_pResponse )
        return HXR_FAIL;

    if ( pBuffer )
        pBuffer->Get(buf, len);
    m_ulCurOffset += len;

    //Keep track of "bytes read" value so CRIFFReader::ReadDone()
    // can know whether or not it needs to decrement by one the size
    // of the buffer it passes off, since some codecs (Intel's ILVC,
    // for one) allow for odd numbered video frame sizes:
    UINT32 ulActualLenOfBuffToUse = len;
    if ( RS_ReadChunkBodyPending == m_state  && pBuffer )
    {
        ulActualLenOfBuffToUse = m_ulFileSpecifiedReadSize;
        if ( len > ulActualLenOfBuffToUse  &&
             len - ulActualLenOfBuffToUse == 1 )
        {
            //Then a Read(n+1) was done becuase n was odd, so
            // make sure buffer has size of n so renderer will
            // get just the bytes it needs:
            pBuffer->SetSize(ulActualLenOfBuffToUse);
        }
    }

    switch ( m_state )
    {
        case RS_GetFileTypePending:
            if ( len != sizeof(UINT32) * 2 )
            {
                m_state = RS_Ready;
                m_pResponse->RIFFOpenDone(HXR_FAILED);
                return HXR_UNEXPECTED;
            }

            if ( (UINT32)getlong(buf) == RIFF_FILE_MAGIC_NUMBER )
            {
                m_state = RS_GetActualFileTypePending;
                m_ulFileType = RIFF_FILE_MAGIC_NUMBER;
                m_bLittleEndian = TRUE;
                m_ulChunkBodyLen = GetLong(&buf[4]);
                m_ulSizeDiff = 0;
                m_levelInfo[m_ulLevel].m_nextChunkOffset = m_ulCurOffset + m_ulChunkBodyLen;
                m_pFileObject->Read(sizeof(UINT32));
                return status;
            }
            else if ( (UINT32)getlong(buf) == IFF_FILE_MAGIC_NUMBER )
            {
                m_state = RS_GetActualFileTypePending;
                m_ulFileType = IFF_FILE_MAGIC_NUMBER;
                m_bLittleEndian = FALSE;
                m_ulChunkBodyLen = GetLong(&buf[4]);
                m_ulSizeDiff = 0;
                m_levelInfo[m_ulLevel].m_nextChunkOffset = m_ulCurOffset + m_ulChunkBodyLen;
                m_levelInfo[m_ulLevel].m_startOffset = 12;

                m_pFileObject->Read(sizeof(UINT32));
                return status;
            }
            else
            {
                m_ulFileType = (UINT32)getlong(buf);
                m_bLittleEndian = FALSE;
                m_ulChunkBodyLen = GetLong(&buf[4]);
                m_ulSizeDiff = 8;
                m_levelInfo[m_ulLevel].m_nextChunkOffset = m_ulCurOffset + m_ulChunkBodyLen - 8;
            }

            m_state = RS_Ready;
            result = m_pResponse->RIFFOpenDone(status);
            return(HXR_OK==status? result:status);

        case RS_GetActualFileTypePending:
            if ( len != sizeof(UINT32) )
            {
                m_state = RS_Ready;
                m_pResponse->RIFFOpenDone(HXR_FAILED);
                return HXR_UNEXPECTED;
            }

            m_ulSubFileType = (UINT32)GetLong(buf);
            m_state = RS_Ready;
            result = m_pResponse->RIFFOpenDone(status);
            return(HXR_OK==status? result:status);

        case RS_ChunkHeaderReadPending:
            if ( len != sizeof(UINT32) + sizeof(UINT32) ||
                 (HXR_OK != status) )
            {
                m_state = RS_Ready;
                m_pResponse->RIFFFindChunkDone(HXR_FAILED, 0);
                return HXR_UNEXPECTED;
            }

            if ( (UINT32)getlong(buf) == m_ulFindChunkId )
            {
                // Found the chunk we were asked for
                m_state = RS_Ready;
                m_ulChunkBodyLen = GetLong(&buf[4]);
                m_ulChunkSize = m_ulChunkBodyLen;

                // Make sure the body length is aligned(2 bytes)
                if ( m_ulFileType == RIFF_FILE_MAGIC_NUMBER )
                {
                    uRem = (UINT16)(m_ulChunkBodyLen % 2);
                    if ( uRem != 0 )
                    {
                        m_ulChunkBodyLen += 1;
                    }
                }

                m_levelInfo[m_ulLevel].m_nextChunkOffset = m_ulCurOffset + m_ulChunkBodyLen;
                m_ulThisChunkOffset = m_ulCurOffset;

                m_levelInfo[m_ulLevel].m_nextChunkOffset -= m_ulSizeDiff;

                m_ulChunkType = m_ulFindChunkId;

                if ( m_ulFindChunkId == LIST_CHUNK_ID )
                {
                    m_state = RS_GetListTypePending;
                    m_pFileObject->Read(sizeof(UINT32));
                }
                else
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFFindChunkDone(status,
                                                   m_ulChunkBodyLen - m_ulSizeDiff);
                }
                return status;
            }
            else
            {
                // Didn't find it, go to the next chunk.
                m_state = RS_ChunkBodySeekPending;
                m_ulSeekOffset = m_ulCurOffset + GetLong(&buf[4]);

                /* Are we at the end of .rm file */
                if ( m_ulSeekOffset == m_ulCurOffset &&
                     m_ulFileType != RIFF_FILE_MAGIC_NUMBER &&
                     m_ulFileType != IFF_FILE_MAGIC_NUMBER )
                {
                    m_pResponse->RIFFFindChunkDone(HXR_FAILED, 0);
                    return HXR_OK;
                }

                if ( m_ulFileType == RIFF_FILE_MAGIC_NUMBER ||
                     m_ulFileType == IFF_FILE_MAGIC_NUMBER )
                {
                    // Make sure the seek offset is aligned(2 bytes)
                    uRem = (UINT16)(m_ulSeekOffset % 2);
                    if ( uRem != 0 )
                    {
                        m_ulSeekOffset += 1;
                    }
                }

                m_ulSeekOffset -= m_ulSizeDiff;

                if ( m_ulSeekOffset == m_ulCurOffset )
                {
                    m_state = RS_ChunkHeaderReadPending;
                    return m_pFileObject->Read(sizeof(UINT32) * 2);
                }
                m_state = RS_ChunkBodySeekPending;
                return m_pFileObject->Seek(m_ulSeekOffset, FALSE);
            }

        case RS_GetListTypePending:
            if ( len != sizeof(UINT32) )
            {
                m_state = RS_Ready;
                m_pResponse->RIFFFindChunkDone(HXR_FAILED, 0);
                return HXR_UNEXPECTED;
            }

            m_ulChunkSubType = getlong(buf);
            m_state = RS_Ready;
            m_pResponse->RIFFFindChunkDone(status, m_ulChunkBodyLen);
            return HXR_OK;
        case RS_ReadChunkHeaderPending:
            {
                if ( HXR_OK != status )
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFGetChunkDone(HXR_FAILED, 0, NULL);
                    return HXR_OK;
                }

                m_ulGetChunkType = (UINT32)getlong(buf);
                m_state = RS_ReadChunkBodyPending;
                LONG32 baseLen = GetLong(&buf[4]);
                m_ulChunkSize = baseLen;

                if ( (m_ulFileType == RIFF_FILE_MAGIC_NUMBER) &&
                     (m_ulGetChunkType == (UINT32)0) )
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFGetChunkDone(HXR_FAILED, 0, NULL);
                    return HXR_OK;
                }
                if ( baseLen == 0 )
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFGetChunkDone(HXR_OK, m_ulGetChunkType, NULL);
                    return HXR_OK;
                }

                HX_RESULT resultOfRead = HXR_OK;

#ifdef CHUNK_READ_SIZE_LIMIT
                // If the chunk is greater than MAX_READ_SIZE, we break the
                // read into smaller portions to improve performance under the
                // Simple File System:
                if (baseLen > MAX_READ_SIZE)
                {
                    IHXCommonClassFactory* pClassFactory = NULL;

                    if (m_pContext && SUCCEEDED(m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                                                          (void**) &pClassFactory)))
                    {
                        pClassFactory->CreateInstance(IID_IHXBuffer, (void**) &m_pReassemblyBuffer);
                    }

                    if (m_pReassemblyBuffer)
                    {
                        m_pReassemblyBuffer->SetSize(baseLen);

                        m_ulChunkBytesRead = 0;
                        m_ulChunkSize = baseLen;

                        // Here we munge baselen in the interests of code
                        // simplicity:
                        baseLen = MAX_READ_SIZE;
                    }
                }
#endif // CHUNK_READ_SIZE_LIMIT

                if ( m_ulFileType == RIFF_FILE_MAGIC_NUMBER ||
                     m_ulFileType == IFF_FILE_MAGIC_NUMBER )
                {
                    // Make sure the chunk is aligned(2 bytes)
                    uRem = (UINT16)((baseLen) % 2);

                    //NOTE: if m_ulCurOffset is greater than 0x7FFFFFFF, GetLong
                    // returns a negative number.  Any negative number % 2 (at
                    // least in Windows) returns 0 or -1, and ((UINT16)-1) == 0xFFFF
                    // so uRem thus can be one of the following: {0, 1, 0xFFFF}.
                    // The following if() conditional will, however, still work as
                    // expected:
                    if ( uRem != 0 )
                    {
                        m_ulFileSpecifiedReadSize = baseLen;
                        resultOfRead = m_pFileObject->Read(baseLen+1);
                    }
                    else
                    {
                        m_ulFileSpecifiedReadSize = baseLen;
                        resultOfRead = m_pFileObject->Read(baseLen);
                    }
                }
                else
                {
                    m_ulFileSpecifiedReadSize = baseLen;
                    resultOfRead = m_pFileObject->Read(baseLen);
                }
                return resultOfRead;
            }

        case RS_GetChunkHeaderPending:
            {
                if ( HXR_OK != status )
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFGetChunkDone(HXR_FAILED, 0, NULL);
                    return HXR_OK;
                }

                m_ulChunkType = (UINT32)getlong(buf);
                m_state = RS_ReadChunkBodyPending;
                LONG32 baseLen = GetLong(&buf[4]);
                m_ulChunkSize = baseLen;

				if ((m_ulFileType == RIFF_FILE_MAGIC_NUMBER ||
					m_ulFileType == IFF_FILE_MAGIC_NUMBER) &&
					(baseLen % 2 == 1))
				{
					baseLen++;
				}

                if ( (m_ulFileType == RIFF_FILE_MAGIC_NUMBER) &&
                     (m_ulGetChunkType == (UINT32)0) )
                {
                    m_state = RS_Ready;
                    m_pResponse->RIFFGetChunkDone(HXR_FAILED, 0, NULL);
                    return HXR_OK;
                }

                m_pResponse->RIFFFindChunkDone(HXR_OK, baseLen);
                return HXR_OK;
            }

        case RS_ReadChunkBodyPending:
#ifdef CHUNK_READ_SIZE_LIMIT
            if (m_pReassemblyBuffer)
            {
                BYTE* pAssemblyStart = m_pReassemblyBuffer->GetBuffer();
                HX_ASSERT(pAssemblyStart);

                HX_ASSERT(m_ulChunkSize - m_ulChunkBytesRead >= pBuffer->GetSize());
                UINT32 ulBytesToCopy = pBuffer->GetSize();
                if (pBuffer->GetSize() > m_ulChunkSize - m_ulChunkBytesRead)
                    ulBytesToCopy = m_ulChunkSize - m_ulChunkBytesRead;
                memcpy(pAssemblyStart + m_ulChunkBytesRead, pBuffer->GetBuffer(), /* Flawfinder: ignore */
                       ulBytesToCopy);
                m_ulChunkBytesRead += pBuffer->GetSize();

                if (m_ulChunkBytesRead == m_ulChunkSize || FAILED(status) ||
                    pBuffer->GetSize() == 0)
                {
                    m_state = RS_Ready;
                    IHXBuffer* pOldBuffer = m_pReassemblyBuffer;
                    m_pReassemblyBuffer = NULL;
                    m_pResponse->RIFFGetChunkDone(status, m_ulGetChunkType, pOldBuffer);
                    HX_RELEASE(pOldBuffer);
                    return HXR_OK;
                }
                else
                {
                    UINT32 baseLen = m_ulChunkSize - m_ulChunkBytesRead;

                    if (baseLen > MAX_READ_SIZE)
                    {
                        baseLen = MAX_READ_SIZE;
                    }

                    HX_RESULT resultOfRead = HXR_OK;

                    if ( m_ulFileType == RIFF_FILE_MAGIC_NUMBER ||
                         m_ulFileType == IFF_FILE_MAGIC_NUMBER )
                    {
                        // Make sure the chunk is aligned(2 bytes)
                        uRem = (UINT16)((baseLen) % 2);

                        //NOTE: if m_ulCurOffset is greater than 0x7FFFFFFF, GetLong
                        // returns a negative number.  Any negative number % 2 (at
                        // least in Windows) returns 0 or -1, and ((UINT16)-1) == 0xFFFF
                        // so uRem thus can be one of the following: {0, 1, 0xFFFF}.
                        // The following if() conditional will, however, still work as
                        // expected:
                        if ( uRem != 0 )
                        {
                            m_ulFileSpecifiedReadSize = baseLen;
                            resultOfRead = m_pFileObject->Read(baseLen+1);
                        }
                        else
                        {
                            m_ulFileSpecifiedReadSize = baseLen;
                            resultOfRead = m_pFileObject->Read(baseLen);
                        }
                    }
                    else
                    {
                        m_ulFileSpecifiedReadSize = baseLen;
                        resultOfRead = m_pFileObject->Read(baseLen);
                    }
                    return resultOfRead;
                }
            }
#endif // CHUNK_READ_SIZE_LIMIT

            m_state = RS_Ready;
  	    m_pResponse->RIFFGetChunkDone(status, m_ulGetChunkType, pBuffer);
            return HXR_OK;
        case RS_DataReadPending:
            m_state = RS_Ready;
            return m_pResponse->RIFFReadDone(status, pBuffer);
        default:
            //m_state = RS_Ready;
            return HXR_UNEXPECTED;
    }
    return status;
}

STDMETHODIMP
CRIFFReader::SeekDone(HX_RESULT status)
{
    /* This may happen in HTTP streaming when the file system
     * is in still a seeking mode when the next seek is issued.
     * The file system will then call SeekDone with a status of
     * HXR_CANCELLED for the pending seek.
     */
    if ( status == HXR_CANCELLED )
    {
        return HXR_OK;
    }

    if ( status == HXR_OK )
    {
        m_ulCurOffset = m_ulSeekOffset;
    }

    HX_RESULT result = HXR_OK;

    switch ( m_state )
    {
        case RS_ChunkBodySeekPending:
            m_state = RS_ChunkHeaderReadPending;
            result = m_pFileObject->Read(sizeof(UINT32) + sizeof(UINT32));
            return(HXR_OK == status? result:status);
        case RS_FileStartSeekPending:
            m_state = RS_ChunkHeaderReadPending;
            result = m_pFileObject->Read(sizeof(UINT32) + sizeof(UINT32));
            return(HXR_OK == status? result:status);
        case RS_AscendSeekPending:
            m_state = RS_Ready;
            result = m_pResponse->RIFFAscendDone(status);
            return(HXR_OK == status? result:status);
        case RS_UserSeekPending:
            m_state = RS_Ready;
            result = m_pResponse->RIFFSeekDone(status);
            return(HXR_OK == status? result:status);
        default:
            return HXR_UNEXPECTED;
    }
}

HX_RESULT
CRIFFReader::Seek(UINT32 offset, HXBOOL bRelative)
{
    m_ulSeekOffset = bRelative ?
                     m_ulCurOffset + offset :
                     m_ulThisChunkOffset + offset;
    m_state = RS_UserSeekPending;
    return m_pFileObject->Seek(m_ulSeekOffset, FALSE);
}

UINT32
CRIFFReader::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FSR_READDONE;
}

HX_RESULT
CRIFFReader::FileSeek(UINT32 offset)
{
    m_ulSeekOffset = offset;
    m_state = RS_UserSeekPending;
    return m_pFileObject->Seek(m_ulSeekOffset, FALSE);
}

HX_RESULT
CRIFFReader::Descend()
{
    if ( m_ulLevel > 0 && (!((m_ulChunkType == LIST_CHUNK_ID) || (m_ulChunkType == IDX1_CHUNK_ID))))
    {
        m_pResponse->RIFFDescendDone(HXR_FAILED);
        return HXR_UNEXPECTED;
    }

    m_ulLevel++;
    m_levelInfo[m_ulLevel].m_startOffset = m_ulCurOffset;

    if (m_ulFileType == RIFF_FILE_MAGIC_NUMBER ||
        m_ulFileType == IFF_FILE_MAGIC_NUMBER)
    {
        if (m_ulCurOffset % 2 == 1)
        {
            m_levelInfo[m_ulLevel].m_startOffset++;
        }
    }

    m_levelInfo[m_ulLevel].started     = FALSE;

    return m_pResponse->RIFFDescendDone(HXR_OK);
}

HX_RESULT
CRIFFReader::Ascend()
{
    m_ulLevel--;
    m_state = RS_AscendSeekPending;
    if ( m_ulLevel == 0 )
        m_ulSeekOffset = 0;
    else
    {
        m_ulSeekOffset = m_levelInfo[m_ulLevel].m_nextChunkOffset;
    }

    m_pFileObject->Seek(m_ulSeekOffset, FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP
CRIFFReader::CloseDone(HX_RESULT status)
{
    return HXR_OK;
}

STDMETHODIMP
CRIFFReader::WriteDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *      IHXFileResponse::FileObjectReady
 *  Purpose:
 *      Notification interface provided by users of the IHXFileObject
 *      interface. This method is called by the IHXFileObject when the
 *      requested FileObject is ready. It may return NULL with
 *      HX_RESULT_FAIL if the requested filename did not exist in the
 *      same pool.
 */
STDMETHODIMP
CRIFFReader::FileObjectReady
(
HX_RESULT status,
IHXFileObject* pFileObject)
{
    return HXR_OK;
}

HX_RESULT
CRIFFReader::InternalClose()
{
    return HXR_OK;
}

HX_RESULT
CRIFFReader::Read(UINT32 len)
{
    // Read from the file on our owner's behalf

    m_state = RS_DataReadPending;
    return m_pFileObject->Read(len);
}

UINT32
CRIFFReader::GetListType()
{
    return m_ulChunkSubType;
}

UINT32
CRIFFReader::GetOffset()
{
    return m_ulCurOffset;
}

UINT32
CRIFFReader::FileType()
{
    return m_ulFileType;
}

UINT32
CRIFFReader::FileSubtype()
{
    return m_ulSubFileType;
}

UINT32
CRIFFReader::GetChunkType()
{
    return m_ulChunkType;
}

UINT32
CRIFFReader::GetChunkRawSize(void)
{
    return m_ulChunkSize;
}
