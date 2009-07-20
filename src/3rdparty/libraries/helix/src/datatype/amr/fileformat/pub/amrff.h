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

#ifndef AMRFF_H
#define AMRFF_H

#define AMR_HEADER_READ_SIZE    64
#define AMR_READ_SIZE         4096
#define AMR_IDEAL_PACKET_SIZE  512
#define AMR_MIN_CONSEC_FRAMES    5

// Forward declarations
typedef _INTERFACE IHXRequest            IHXRequest;
typedef _INTERFACE IHXFormatResponse     IHXFormatResponse;
typedef _INTERFACE IHXFileObject         IHXFileObject;
typedef _INTERFACE IHXBuffer             IHXBuffer;
typedef _INTERFACE IHXCommonClassFactory IHXCommonClassFactory;
typedef _INTERFACE IHXFileObject         IHXFileObject;
typedef _INTERFACE IHXFormatResponse     IHXFormatResponse;
typedef _INTERFACE IHXFileStat           IHXFileStat;
class CHXAMRPayloadFormatPacketizer;


class CAMRFileFormat : public CHXBaseCountingObject,
                       public IHXPlugin, 
                       public IHXFileFormatObject,
                       public IHXFileResponse,
                       public IHXFileStatResponse,
                       public IHXInterruptSafe
{
public:
    CAMRFileFormat();
    virtual ~CAMRFileFormat();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXPlugin methods
    STDMETHOD(GetPluginInfo) (THIS_ REF(HXBOOL)        rbLoadMultiple,
                                    REF(const char*) rpszDescription,
                                    REF(const char*) rpszCopyright,
                                    REF(const char*) rpszMoreInfoURL,
                                    REF(ULONG32)     rulVersionNumber);
    STDMETHOD(InitPlugin)    (THIS_ IUnknown* pContext);

    // IHXFileFormatObject methods
    STDMETHOD(GetFileFormatInfo) (THIS_ REF(const char**) rppszFileMimeTypes,
                                        REF(const char**) rppszFileExtensions,
                                        REF(const char**) rppszFileOpenNames);
    STDMETHOD(InitFileFormat)    (THIS_ IHXRequest*        pRequest, 
                                        IHXFormatResponse* pFormatResponse,
                                        IHXFileObject*     pFileObject);
    STDMETHOD(Close)             (THIS);
    STDMETHOD(GetFileHeader)     (THIS);
    STDMETHOD(GetStreamHeader)   (THIS_ UINT16 unStreamNumber);
    STDMETHOD(GetPacket)         (THIS_ UINT16 unStreamNumber);
    STDMETHOD(Seek)              (THIS_ ULONG32 ulOffset);

    // IHXFileResponse methods
    STDMETHOD(InitDone)  (THIS_ HX_RESULT status);
    STDMETHOD(SeekDone)  (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone)  (THIS_ HX_RESULT status, IHXBuffer *pBuffer);
    STDMETHOD(WriteDone) (THIS_ HX_RESULT status);
    STDMETHOD(CloseDone) (THIS_ HX_RESULT status);

    // IHXFileStatResponse methods
    STDMETHOD(StatDone) (THIS_ HX_RESULT status, UINT32 ulSize, UINT32 ulCreationTime,
                               UINT32 ulAccessTime, UINT32 ulModificationTime, UINT32 ulMode);

    // IHXInterruptSafe methods
    STDMETHOD_(HXBOOL,IsInterruptSafe) (THIS) { return TRUE; };

    // CAMRFileFormat methods
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload2(void);
private:
    typedef enum
    {
        StateReady,
        StateInitFileFormatInitDonePending,
        StateInitFileFormatStatDonePending,
        StateFileHeaderSeekDonePending,
        StateFileHeaderReadDonePending,
        StateStreamHeaderSeekDonePending,
        StateStreamHeaderReadDonePending,
        StateGetPacketSeekDonePending,
        StateGetPacketReadDonePending
    } AMRFFState;

    INT32                          m_lRefCount;
    IUnknown*                      m_pContext;
    IHXCommonClassFactory*         m_pCommonClassFactory;
    IHXFileObject*                 m_pFileObject;
    IHXFormatResponse*             m_pFormatResponse;
    AMRFFState                     m_eState;
    IHXFileStat*                   m_pFileStat;
    UINT32                         m_ulHeaderSize;
    UINT32                         m_ulBytesPerFrame;
    UINT32                         m_ulFileSize;
    UINT32                         m_ulNumChannels;
    UINT32                         m_ulNextFileOffset;
    UINT32                         m_ulNextTimeStamp;
    CHXAMRPayloadFormatPacketizer* m_pPayloadFormat;
    HX_BITFIELD                    m_bWideBand : 1;
    HX_BITFIELD                    m_bScanForFrameBegin : 1;

    static const char* const m_pszDescription;
    static const char* const m_pszCopyright;
    static const char* const m_pszMoreInfoURL;
    static const char* const m_ppszFileMimeTypes[];
    static const char* const m_ppszFileExtensions[];
    static const char* const m_ppszFileOpenNames[];
    static const char* const m_ppszStreamMimeTypes[];
    static const BYTE        m_pMagicSingle[6];
    static const BYTE        m_pMagicSingleWB[9];
    static const BYTE        m_pMagicMulti[12];
    static const BYTE        m_pMagicMultiWB[15];
    static const BYTE        m_ucNumChannelsMap[16];

    HX_RESULT GetStreamInfo(IHXBuffer* pBuffer, UINT32 ulOffset,
                            REF(UINT32) rulAvgBitRate, REF(UINT32) rulBytesPerFrame);
    HX_RESULT MakeRawFilePacket(IHXBuffer* pBuffer, UINT32 ulTimeStamp,
                                REF(IHXPacket*) rpPacket);
};

#endif // #ifndef AMRFF_H

