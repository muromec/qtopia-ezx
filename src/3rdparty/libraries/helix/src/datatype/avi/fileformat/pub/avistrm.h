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

#ifndef _AVISTRM_H_
#define _AVISTRM_H_

#include "aviffpln.h"
#include "hxwintyp.h"
#include "riffres.h"    // CRIFFResponse
#include "legacy.h"

#define MAX_COLOR_TABLE_SIZE 256
#define MAX_CHUNK_PREFETCH 2

#define PRIOR_SEEK_TIME		1500

class CAVIFileFormat;
class CAVIIndex;
class CRIFFReader;

// CAVIStream classes contain per-stream state:
// headers, indexes, RIFF readers, packetizers, and packets.
// Not to be confused with VFW's AVIStreamInfo
class CAVIStream : public CRIFFResponse
{
public:
    CAVIStream(CAVIFileFormat* pOuter, UINT16 usStream,
               CAVIFileFormat::MainHeader& mainHeader,
               BOOL bLocalPlayback, IUnknown* pContext);

	~CAVIStream();

	void Close();
    
	// Header methods:
    HX_RESULT SetHeader(UINT16 usStream, IHXBuffer* pHeader);
    HX_RESULT SetFormat(IHXBuffer* pFormat);
    HX_RESULT SetPacketFormat(UINT32 ulPacketFormat);

    HX_RESULT SetOpaque(IHXBuffer* pOpaque);

    HX_RESULT SetIndex(CAVIIndex* pIndex);

    void SetPendingHeaderRequest();
    BOOL PendingHeaderRequest();

    HX_RESULT GetHeader(IHXValues* pNewHeader);

    BOOL IsAudio();
    double GetDuration();

    // Packet handling methods:
//    HX_RESULT InitForReading(CRIFFReader* pReader);
	HX_RESULT CAVIStream::InitForReading(IUnknown* pContext, IHXFileObject* pFile);
    BOOL ReadInitialized();

    BOOL HasPackets();
    UINT32 GetPendingPacketCount();
    UINT32 IncrementPendingPacketCount();
    void ClearPendingPacketCount();

    // Chunk handling methods:
    void Seek(UINT32 ulTime);       // Resets pending packet count

    // On failure, returns NULL; on success, returns AddRef'd packet,
    // decrements pending packet count
    IHXPacket* GetNextPacket();
    UINT32 PeekPacketTime();

    // A slice may be a single packet or a huge swath; for now, a slice is a
    // chunk
    void GetNextSlice();

    BOOL CanPrefetchSlice();
    UINT32 PeekPrefetchTime();     // Next unloaded slice
	UINT32 GetMaxChunkRead();
	void CloseStream();
//	void Close();

    BOOL AtEndOfStream();
    void DiscardPendingIO();

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

//    PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME

protected:
//    ~CAVIStream();

    enum PacketFormat
    {
        PFMT_RDT,
        PFMT_RTP
    } m_ePacketFormat;

    HXBOOL SetRuleBook(PacketFormat packetFormat, IHXValues* pHeader,
                       IHXCommonClassFactory* pCommonClassFactory,
                         ULONG32 ulAvgBitRate, INT32 lRTPPayloadType);

#pragma pack(1)
    struct RGBQuad
    {
        UINT8 ubRed;
        UINT8 ubGreen;
        UINT8 ubBlue;
        UINT8 ubReserved;
    };

    struct StreamHeader
    {
        UINT32 ulType;        // fcc type
        UINT32 ulHandler;
        UINT32 ulFlags;
        INT16   sPriority;
        INT16   sLanguage;
        UINT32 ulInitialFrames;
        UINT32 ulScale;
        UINT32 ulRate;        // dwRate / dwScale == samples/second
        UINT32 ulStart;
        UINT32 ulLength;      // in above units
        UINT32 ulSuggestedBufferSize;
        UINT32 ulQuality;
        UINT32 ulSampleSize;
        INT16   sTop;
        INT16   sLeft;
        INT16   sBottom;
        INT16   sRight;
    } m_header;

    double m_fChunksPerSecond;
    double m_fSamplesPerSecond;
    UCHAR* m_pFormat; /* BitmapInfo for video stream,
                         WaveInfo for audio stream,
                         somthing else for text? */
    BOOL   m_bEndianSwap16;

    // The following variable is added to get correct Time information
    // for the AUDIO streams. In case of RAW PCM Audio data, number_of_chunks
    // and chunk_size cannot be basis of calculating TimeStamps for packets
    // chunk_size can vary from chunk to chunk
    // there could be some partial chunks towards the end.
    // TS is calculated from the number of bytes already packetized before this current
    // packet and sampling frequency

    UINT32 m_ulTotalBytesRead;
    UINT32 m_ulTotalBytesPacketised;
    BOOL m_bSeeking;
    ULONG32 m_ulSeekTime;




    struct BitmapInfo
    {
        UINT32 ulSize;
        INT32  ulWidth;
        INT32  ulHeight;
        UINT16 usPlanes;
        UINT16 usBitCount;
        UINT32 ulCompression;
        UINT32 ulSizeImage;
        INT32  ulXPelsPerMeter;
        INT32  ulYPelsPerMeter;
        UINT32 ulClrUsed;
        UINT32 ulClrImportant;

        RGBQuad rgbQuad[MAX_COLOR_TABLE_SIZE];
    };

    struct WaveInfo
    {
        UINT16      usFormatTag;
        UINT16      usChannels;
        UINT32      ulSamplesPerSec;
        UINT32      ulAvgBytesPerSec;
        UINT16      usBlockAlign;
        UINT16      usBitsPerSample;  // bps, mono data
        UINT16      usSize;
    };

    struct HeaderInfo
    {
        HXxRect       frameRect;
        UINT32        ulSampleRate;
        UINT32        ulSampleScale;
        BitmapInfo    bitmapInfo;
    };
#pragma pack()

    enum state
    {
          ePreHeader
        , ePreReader
        , eChunkSeek		
        , eChunkFind
        , eChunkRead
        , eReady
    } m_state;


    CAVIFileFormat* m_pOuter;
    CAVIIndex* m_pIndex;
	IHXFileObject* m_pFile;

    CAVIFileFormat::MainHeader m_mainHeader;

    BOOL m_bPendingHeaderRequest;
    UINT16 m_usStream;

    // Packet handling:
    UINT32 m_ulPendingPacketRequests;  // The number of outstanding unsatisfied
                                       // packet requests.

    IHXPayloadFormatObject* m_pPayloadFormatter;
    IHXPacket* m_pNextPacket;

    // To do: create local playback format object and AVI audio format object
    BOOL m_bLocalPlayback;

    CRIFFReader* m_pReader;

    struct ChunkInfo
    {
        BOOL bKeyChunk;
        IHXBuffer* pBuffer;
    };

    ChunkInfo m_chunkPrefetchArray[MAX_CHUNK_PREFETCH];   // Stream data

    UINT32 m_ulMinReadChunk;
    UINT32 m_ulMaxReadChunk;        // exclusive
    UINT32 m_ulChunkReadTarget;     // The target of current file operations

    BOOL   m_bRead;                 // All data (chunks) read.  Reset on seek.
    BOOL   m_bDiscardPendingIO;

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;

private:
	INT32			m_lRefCount;
};

#endif // _AVISTRM_H_
