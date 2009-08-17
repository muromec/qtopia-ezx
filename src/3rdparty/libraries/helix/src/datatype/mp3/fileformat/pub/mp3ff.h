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

#ifndef _HXMP3FMT_H_
#define _HXMP3FMT_H_

#include "baseobj.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define MY_PLUGIN_VERSION       0
#define MY_DESCRIPTION         "RealNetworks MP3 File Format Plugin"
#define MY_COPYRIGHT           "(c) 1999-2000 RealNetworks, Inc. All rights reserved."
#define MY_MORE_INFO_URL       "http://www.real.com"
#define MY_FILE_MIME_TYPES     {"audio/rn-mpeg", "audio/mpeg", "audio/mpg", "audio/mp3", "audio/x-mpeg", "audio/x-mpg", "audio/x-mp3", NULL}
#define MY_FILE_EXTENSIONS     {"mp3", "mp2", "mpa", "mp1", "mpga", "mpg", "mpeg", "mpv", "dat", NULL}
#define MY_FILE_OPEN_NAMES     {"MPEG Audio Files (.mp3;.mp2;.mpa;.mp1;.mpga)", NULL}

#define MY_STREAM_MIME_TYPE    "audio/X-MP3-draft-00"
#define MY_RTP_MIME_TYPE       "audio/MPA"
#define MY_LOCAL_MIME_TYPE     "audio/MPEG-ELEMENTARY"

#define MAX_METADATA 2048

#define MAX_GARBAGE_BYTES 262144
#define PROGDOWN_CHECK_INTERVAL 1000

// A bitrate difference higher than 25 can cause more than 0.5 second error in a 5 min clip
#define MAX_ALLOWABLE_BITRATE_DIFF	25

enum { MY_FILE_HEADER_START = 0 };
enum { MY_FILE_HEADER_LENGTH = 4096};
enum { MY_STREAM_NO = 0 };
enum { ID3HeaderLen = 128,
       BytesBeforeID3Header = 1
     };

///////////////////////////////////////////////////////////////////////////////
// Typedefs
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
        Ready,
        InitPending,
        StatDonePending,
        GetId3HeaderSeekPending,
        GetId3HeaderReadPending,
        GetFileHeaderSeekPending,
        GetFileHeaderReadPending,
        GetStreamHeaderSeekPending,
        GetStreamHeaderReadPending,
        GetStreamHeaderStatDonePending,
        GetPacketSeekPending,
        GetPacketReadPending,
        GetPacketStatDonePending,
        SeekSeekPending,
        SeekToOffsetNoRead
} MyState;

typedef enum
{
    eHXUnknown,
    eHXVideo,
    eHXSystem,
    eHXAudio
} eStreamType;

typedef enum
{
    eHXMPEG,
    eHXAC3,
    eHXLPCM,
    eHXDTS
} eAudioType;

typedef enum
{
    eHXMPEG1,
    eHXMPEG2
} eVideoType;

typedef struct
{
    eStreamType     eType;
    eAudioType      eAudio;
    eVideoType      eVideo;

    ULONG32         ulStreamSize;
    UINT32          ulBitRate;
    double          dTimePerFrame;

    double          dPacketSize;

    UCHAR           cHeader;

    int             nWidth;
    int             nHeight;
} tStreamInfo;

// Stat codes
enum
{
    PICTURE_START_CODE  = 0x00000100,
    USER_DATA_CODE      = 0x000001b2,
    SEQ_START_CODE      = 0x000001b3,
    SEQ_ERROR_CODE      = 0x000001b4,
    EXT_DATA_START_CODE = 0x000001b5,
    SEQUENCE_END_CODE   = 0x000001b7,
    GROUP_START_CODE    = 0x000001b8,
    ISO_END_CODE        = 0x000001b9,
    PACK_HEADER         = 0x000001ba,
    SYSTEM_HEADER       = 0x000001bb,
    PRIVATE_STREAM_1    = 0x000001bd,
    PADDING_PACKET      = 0x000001be,
    PRIVATE_STREAM_2    = 0x000001bf,
    VIDEO_PACKET        = 0x000001e0,
    AUDIO_PACKET        = 0x000001c0,
    NO_START_CODE       = 0xffffffff
};

///////////////////////////////////////////////////////////////////////////////
//
//  CRnMp3Fmt Class
//
//  This class inherits the interfaces required to create a File Format
//  plug-in. The IHXFileFormatObject interface consists of routines specific
//  to File Format plug-ins. The IHXFileResponse interface is needed because
//  of the asynchronous nature of the RealMedia file system. All plug-ins
//  also require the IHXPlugin interface to handle fundamental plug-in
//  operations. Since we are using COM, this class also inherits COM's
//  IUnknown interface to handle reference counting and interface query.
//

class CRnMp3Fmt : public IHXFileFormatObject,
                  public IHXFileResponse,
                  public IHXFileStatResponse,
#if defined(MPA_FMT_DRAFT00)
                  public IHXPacketFormat,
#endif
                  public IHXPlugin,
                  public IHXThreadSafeMethods,
                  public IHXAdvise,
                  public CHXBaseCountingObject
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
                , public IHXMediaBytesToMediaDur
#endif /* end #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */
{
public:
        CRnMp3Fmt();

    ///////////////////////////////////////////////////////////////////////////
    // IHXFileFormatObject Interface Methods               ref:  hxformt.h
    STDMETHOD(GetFileFormatInfo)
        (THIS_
          REF(const char**) pFileMimeTypes,
          REF(const char**) pFileExtensions,
          REF(const char**) pFileOpenNames
        );

    STDMETHOD(InitFileFormat)
        (THIS_
          IHXRequest*        pRequest,
          IHXFormatResponse* pFormatResponse,
          IHXFileObject*     pFileObject
        );

    STDMETHOD(GetFileHeader  ) (THIS);
    STDMETHOD(GetStreamHeader) (THIS_ UINT16 streamNo);
    STDMETHOD(GetPacket      ) (THIS_ UINT16 streamNo);
    STDMETHOD(Seek           ) (THIS_ UINT32 requestedTime);
    STDMETHOD(Close          ) (THIS);


    ///////////////////////////////////////////////////////////////////////////
    // IHXFileResponse Interface Methods                   ref:  hxfiles.h
    STDMETHOD(InitDone ) (THIS_ HX_RESULT status);
    STDMETHOD(SeekDone ) (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone ) (THIS_ HX_RESULT status, IHXBuffer* pBuffer);
    STDMETHOD(WriteDone) (THIS_ HX_RESULT status);
    STDMETHOD(CloseDone) (THIS_ HX_RESULT status);


    ///////////////////////////////////////////////////////////////////////////
    // IHXFileStatResponse methods
    STDMETHOD(StatDone)         (THIS_
                HX_RESULT status,
                UINT32 ulSize,
                UINT32 ulCreationTime,
                UINT32 ulAccessTime,
                UINT32 ulModificationTime,
                UINT32 ulMode);

#if defined(MPA_FMT_DRAFT00)
    ///////////////////////////////////////////////////////////////////////////
    // IHXPacketFormat Interface Methods                   ref:  rmaformat.h
    STDMETHOD(GetSupportedPacketFormats) (THIS_ REF(const char**) pFormats);
    STDMETHOD(SetPacketFormat) (THIS_ const char* pFormat);
#endif /* #if defined(MPA_FMT_DRAFT00) */

    ///////////////////////////////////////////////////////////////////////////
    // IHXPlugin Interface Methods                         ref:  hxplugn.h
    STDMETHOD(GetPluginInfo)
        (THIS_
          REF(int)         bLoadMultiple,
          REF(const char*) pDescription,
          REF(const char*) pCopyright,
          REF(const char*) pMoreInfoURL,
          REF(UINT32)      versionNumber
        );

    STDMETHOD(InitPlugin) (THIS_ IUnknown* pHXCore);

    ///////////////////////////////////////////////////////////////////////////
    // IHXThreadSafeMethods methods                        ref: hxengin.h
    STDMETHOD_(UINT32,IsThreadSafe) (THIS);

    // IHXAdvise methods
    STDMETHOD(Advise)   (THIS_ ULONG32 ulInfo);

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    ///////////////////////////////////////////////////////////////////////////
    //  IHXMediaBytesToMediaDur methods

    STDMETHOD(ConvertFileOffsetToDur) (THIS_
             UINT32 /*IN*/ ulLastReadOffset,
             UINT32 /*IN*/ ulCompleteFileSize,
             REF(UINT32) /*OUT*/ ulREFDuration);

    STDMETHOD(GetFileDuration) (THIS_
            UINT32 /*IN*/ ulFileSize,
            REF(UINT32) /*OUT*/ ulREFDur);

#endif /* end #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */

    
    ///////////////////////////////////////////////////////////////////////////
    // IUnknown COM Interface Methods                          ref:  hxcom.h
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);


private:
    // Buffered Read
    typedef struct
    {
        IHXBuffer  *pReadBuffer;
        UCHAR       *pBuffer;
        ULONG32     dwBytesRemaining;
        ULONG32     dwBufferSize;
        HX_RESULT   status;
    } tReadBuffer;

    enum
    {
        kReadSize = 2048
    };

    ///////////////////////////////////////////////////////////////////////////
    // Private Class Variables
    INT32                  m_RefCount;         // Object's reference count
    IHXCommonClassFactory* m_pClassFactory;    // Creates common RMA classes
    IHXFileObject*         m_pFileObj;         // Used for file I/O
    IHXFileStat*           m_pFileStat;
    IHXFormatResponse*     m_pStatus;          // Reports status to RMA core
    IHXErrorMessages*      m_pError;           // Reports errors to system
    IHXRegistry*           m_pRegistry;
    IHXBuffer*             m_szPlayerReg;
    MyState                m_State;            // State used for asynch. calls
    UINT32                 m_ulNextPacketDeliveryTime; // Delivery time of next packet
    UINT32                 m_ulFileSize;
    UINT32                 m_ulMaxSampRate;    // Max pcm sample rate in this stream
    UINT32                 m_ulMetaReadSize;   // A custom read size for meta data
    UINT32                 m_ulNextMetaPos;    // Byte position of next meta data
    UINT32                 m_ulMetaLength;     // Length of the meta packet
    UINT32                 m_ulBytesRead;      // Number of bytes read from file
    UINT32                 m_ulFileOffset;     // Offset for start of file to data
    int                    m_nChannels;        // Number of channels in the stream
    double                 m_dNextPts;         // decimal ms of the next packet
    CIHXRingBuffer*        m_pFmtBuf;          // RingBuffer
    IHXValues*             m_pMetaProps;
    IUnknown*              m_pContext;
    tReadBuffer            m_ReadBuf;
    tRtpPacket             m_RtpPackets;
    tStreamInfo            m_Info;
    CMp3Format*            m_pMp3Fmt;
    UINT32                 m_ulGarbageBytesRead;
    UINT32                 m_ulCurrentDuration;
    UINT32                 m_ulLastStatTick;
    HX_BITFIELD            m_bEOF;
    HX_BITFIELD            m_bRtp;             // Are we using rtp protocol
    HX_BITFIELD            m_bHasVbrHdr;       // Do we have a vbr header
    HX_BITFIELD            m_bSkipVbrHdr;      // Don't rerformat vbr header
    HX_BITFIELD            m_bStreaming;       // Are we in a packet loss enviorn
    HX_BITFIELD            m_bMetaPacket;      // Does this frame contain meta data
    HX_BITFIELD            m_bNeedPacket;      // Read did not finish inside GetPacket
    HX_BITFIELD            m_bCheckBadData;    // If set, check the bad data for a frame
    HX_BITFIELD            m_bLive;            // ShoutCast/IceCast stream
    HX_BITFIELD            m_bLicensed;        // Is the user licensed to stream
    HX_BITFIELD            m_bClosed;
    HX_BITFIELD            m_bAcceptMetaInfo;
    HX_BITFIELD            m_bAllMetaInfo;
    HX_BITFIELD            m_bIsVBR;
    HX_BITFIELD            m_bFirstMeta;       // Confirm our meta location is accurate
    HX_BITFIELD            m_bFileSizeChanged;
    HX_BITFIELD            m_bFileDurationChanged;

    IHXBuffer*              (*m_pCreatePacketFunction) (IHXCommonClassFactory*, IHXBuffer*, UCHAR*, int );

    ///////////////////////////////////////////////////////////////////////////
    // Private Static Class Variables
    static const char* const    zm_pDescription;
    static const char* const    zm_pCopyright;
    static const char* const    zm_pMoreInfoURL;
    static const char* const    zm_pFileMimeTypes[];
    static const char* const    zm_pFileExtensions[];
    static const char* const    zm_pFileOpenNames[];
    static const char* const    zm_pPacketFormats[];

        ///////////////////////////////////////////////////////////////////////////
    // Private Class Methods
    ~CRnMp3Fmt();

    HX_RESULT   MyCreateFileHeaderObj_hr(HX_RESULT status, IHXBuffer* pDataRead);
    void        MyCreateStreamHeaderObj_v(HX_RESULT status, IHXBuffer* pDataRead);
    HX_RESULT   MyCreatePacketObj_hr(HX_RESULT status, tReadBuffer* pDataRead);

    void        DiscardReadBuffers_v();

    UCHAR*      GetMP3Frame_p(tReadBuffer* pPacketData, int &nSyncSize);
    UINT16      InitStream_n(IHXBuffer* pDataRead,
                             tStreamInfo *pInfo,
                             HX_RESULT &hr,
                             char* pUpgrade=NULL,
                             int nBufLen=0);

    INT32       GetStartCode(UINT8 **ppBuffer, UINT32 &ulBytes);
    UINT32      Get4Byte(UINT8* pBuf, UINT32 ulSize);

#if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO)
    void        SetMetaInfo(IHXValues* pFileHeader, const char* pszProp);
#endif /* #if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO) */


    HX_RESULT FinishInitDone(HX_RESULT status);
    void      SetFileSize(HX_RESULT status, UINT32 ulSize);
    HX_RESULT FinishGetPacket();
    void      UpdateDuration();
};

#endif // _HXMP3FMT_H_

