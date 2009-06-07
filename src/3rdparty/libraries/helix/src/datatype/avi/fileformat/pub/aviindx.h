/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id:$ 
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

#ifndef _AVIINDX_H_
#define _AVIINDX_H_

// RMA includes:
#include "hxcom.h"
#include "hxfiles.h"
#include "carray.h"     // CPNPtrArray
#include "hxmap.h"      // CPNMapLongToObj
#include "riffres.h"    // CRIFFResponse

#define MAX_SLICE_SIZE 262144

class CAVIFileFormat;
class CRIFFReader;

class CAVIIndex : public CRIFFResponse // AVI global index
{

public:

    CAVIIndex();

    void Init(CAVIFileFormat* pOuter, IHXFileObject* pFile,
              IUnknown* pContext, UINT32 ulFirstMOVIOffset,
              UINT16 usStreamCount);

	void Close();

    // size not including header:
    void AddToIndex(UINT16 usStream, UINT32 ulChunk, UINT32 ulOffset,
                    UINT32 ulSize, BOOL bKeyChunk = TRUE); // offset includes header
    void FileRead(BOOL bRead);     // All chunks have been indexed; reset on seek

    BOOL IsKeyChunk(UINT16 usStream, UINT32 ulChunk);

    // The following return PNR_CHUNK_MISSING if it is known the referenced chunk
    // is not the file or PNR_NOT_INDEXABLE if it's outside our body of knowledge;
    // the closest chunk is still returned, however.
    HX_RESULT FindClosestChunk(UINT16 usStream, UINT32 ulChunk,
                               /* out */ UINT32& ulClosestOffset,
                               /* out */ UINT32& ulClosesStreamChunk);
    HX_RESULT FindClosestKeyChunk(UINT16 usStream, UINT32 ulChunk,
                               /* out */ UINT32& ulClosestOffset,
                               /* out */ UINT32& ulClosestStreamChunk);

    // The following return zero if no information is available:
    UINT32 GetMaxChunkSize(UINT16 usStream);        // not including chunk header
    double GetAverageChunkSize(UINT16 usStream);    // not including chunk header

    // Returns zero if we have no index; assumes transmission at average rate
    UINT32 GetMaxByteDeflict(UINT16 usStream);
    UINT32 GetByteTotal(UINT16 usStream);
    UINT32 GetChunkTotal(UINT16 usStream);

    BOOL   CanLoadSlice();
    BOOL   CanPreloadSlice();
    void   GetNextSlice();

    // CRIFFResponse:
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) ;
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    STDMETHOD(RIFFOpenDone)(HX_RESULT status);

    STDMETHOD(RIFFCloseDone)(HX_RESULT status);

    /* RIFFFindChunkDone is called when a FindChunk completes.
     * len is the length of the data in the chunk (i.e. Read(len)
     * would read the whole chunk)
     */
    STDMETHOD(RIFFFindChunkDone)(HX_RESULT status, UINT32 len);

    /* Called after a Descend completes */
    STDMETHOD(RIFFDescendDone)(HX_RESULT status);

    /* Called after an Ascend completes */
    STDMETHOD(RIFFAscendDone)(HX_RESULT status);

    /* Called when a read completes.  IHXBuffer contains the data read
     */
    STDMETHOD(RIFFReadDone)(HX_RESULT, IHXBuffer* pBuffer);

    /* Called when a seek completes */
    STDMETHOD(RIFFSeekDone)(HX_RESULT status);

    /* Called with the data from a GetChunk request */
    STDMETHOD(RIFFGetChunkDone)(HX_RESULT status, UINT32 chunkType,
                                IHXBuffer* pBuffer);

    INT32 GetChunkLength (UINT16 usStream, UINT32 ulChunkNumber);

protected:

    void SetMinimumChunkInterest(UINT16 usStream, UINT32 ulChunk);

#pragma pack(1)
    struct idx1Entry
    {
        UINT32  ulChunkId;
        UINT32  dwFlags;
        UINT32  ulChunkOffset;
        UINT32  ulChunkLength;
    };

    struct IndexEntry
    {
        BYTE   bKeyChunk;
        UINT32 ulOffset;
		UINT32 ulLength;
    };
#pragma pack()

    struct StreamSlice
    {
        UINT32 ulTotalBytes;           // to efficiently calculate preroll
        UINT32 ulTotalChunks;
        INT64  llByteDeflict;          // scratch value
        UINT32 ulPrerollBytes;
        UINT32 ulMaxChunkSize;
        UINT32 ulNextChunkRequired;
        UINT32 ulSliceStartChunk;
        UINT32 ulSliceEndChunk;
        UINT32 ulSliceEndOffset;
        BOOL    bSliceOffsetCapped;
        UINT32 ulLastKeyChunk;
        UINT32 ulLastKeyChunkOffset;
        IndexEntry entryArray[MAX_SLICE_SIZE];

        StreamSlice()
            : ulTotalBytes(0)
            , ulTotalChunks(0)
            , llByteDeflict(0)
            , ulPrerollBytes(0)
            , ulMaxChunkSize(0)
            , ulNextChunkRequired(0)
            , ulSliceStartChunk(0)
            , ulSliceEndChunk(0)
            , ulSliceEndOffset(0)
            ,  bSliceOffsetCapped(FALSE)
            , ulLastKeyChunk(0)
            , ulLastKeyChunkOffset(0)
            { };
    };

    // PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME
    ~CAVIIndex();

    CAVIFileFormat* m_pOuter;
	IHXFileObject* m_pFile;

    CRIFFReader* m_pReader;
    UINT32 m_ulIndexOffset;
    UINT32 m_ulIndexLength;

    BOOL   m_bRead;
    BOOL   m_bDiscardPendingIO;

    CHXPtrArray m_sliceArray;       // Per-stream slice container
    UINT16      m_usStreamCount;

    // For now, only one movi chunk/index per AVI is supported:
    UINT32 m_ulFirstMOVIOffset;
    UINT32 m_ulFirstRelativeMOVIOffset;
    BOOL   m_bRelativeIndexDetermined;

    // For a synthetic index:
    UINT32 m_ulFirstUnknownChunk;

    enum state
    {
        eReaderOpen,        // RIFF reader init
        eInitialDescend,
        eInitialSeek,       // Seeking to best known offset
        eIdx1Find,          // Search for next idx1 chunk
        eIdx1Descend,       // Descend idx1 chunk
        eSeekToRecord,
        eReadSlice,
        //eIdx1Ascend,      // Ascend idx1 chunk, for use with multiple indexes
        eReady
    } m_state;

    enum scanState
    {
        eInitial,
        ePreroll,
        eComplete
    } m_scanState;

    // TODO: In the future, support indexing of dynamically growing files.
	private:
	INT32 m_lRefCount;
};

#endif // _AVIINDX_H_
