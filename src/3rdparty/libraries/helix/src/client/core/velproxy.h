/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef VELPROXY_H
#define VELPROXY_H

// Forward declarations
typedef _INTERFACE IHXPacket                   IHXPacket;
typedef _INTERFACE IHXValues                   IHXValues;
typedef _INTERFACE IHXPlaybackVelocityResponse IHXPlaybackVelocityResponse;
typedef _INTERFACE IHXPlaybackVelocityCaps     IHXPlaybackVelocityCaps;
typedef _INTERFACE IHXCommonClassFactory       IHXCommonClassFactory;
typedef _INTERFACE IHXRequest                  IHXRequest;
typedef _INTERFACE IHXFileObject               IHXFileObject;
class RuleToFlagMap;
class CHXSimpleList;

// Defines
#define VELPROXY_DEFAULT_NUM_VELOCITY_TIERS 3

class CHXStreamInfo
{
public:
    CHXStreamInfo(UINT32 ulStreamNumber);
    ~CHXStreamInfo();

    UINT32         m_ulStreamNumber;
    UINT32         m_ulLastASMSwitchOnTimeStamp;
    UINT32         m_ulLastOriginalTimeStamp;
    UINT32         m_ulLastWarpedTimeStamp;
    UINT32         m_ulInitialPacketTime;
    UINT32         m_ulLastInitialPacketTime;
    CHXSimpleList* m_pReversalQueue;
    RuleToFlagMap* m_pRuleToFlagMap;
    HX_BITFIELD    m_bIsVideo : 1;
    HX_BITFIELD    m_bIsRealVideo : 1;
    HX_BITFIELD    m_bIsSparseStream : 1;
    HX_BITFIELD    m_bUseOffsetHandler : 1;
    HX_BITFIELD    m_bKeyFrameModeTransition : 1;
    HX_BITFIELD    m_bFirstPacketAfterInternalSeek : 1;
    HX_BITFIELD    m_bExternalPacketRequested : 1;
    HX_BITFIELD    m_bReversalPacketRequested : 1;
    HX_BITFIELD    m_bReTimeStampPacketRequested : 1;
    HX_BITFIELD    m_bInternalPacketRequested : 1;
    HX_BITFIELD    m_bReversalBufferingInProgress : 1;

    void ClearPacketQueue(CHXSimpleList* pQueue);
};

class CHXPlaybackVelocityProxy : public IHXFormatResponse,
                                 public IHXBackChannel,
                                 public IHXPlaybackVelocity,
                                 public IHXPacketTimeOffsetHandlerResponse
{
public:
    CHXPlaybackVelocityProxy(IUnknown* pContext);
    virtual ~CHXPlaybackVelocityProxy();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXFormatResponse methods
    STDMETHOD(InitDone)          (THIS_ HX_RESULT status);
    STDMETHOD(PacketReady)       (THIS_ HX_RESULT status, IHXPacket* pPacket);
    STDMETHOD(SeekDone)          (THIS_ HX_RESULT status);
    STDMETHOD(FileHeaderReady)   (THIS_ HX_RESULT status, IHXValues* pHeader);
    STDMETHOD(StreamHeaderReady) (THIS_ HX_RESULT status, IHXValues* pHeader);
    STDMETHOD(StreamDone)        (THIS_ UINT16 unStreamNumber);

    // IHXBackChannel methods
    STDMETHOD(PacketReady) (THIS_ IHXPacket* pPacket);

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS);
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS);
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS);
    STDMETHOD(CloseVelocityControl)        (THIS);

    // IHXPacketTimeOffsetHandlerResponse methods
    STDMETHOD(TimeOffsetPacketReady) (THIS_ IHXPacket* pPacket);

    // CHXPlaybackVelocityProxy methods
    STDMETHOD(Init)  (THIS_ IHXFormatResponse* pResponse);
    STDMETHOD(Close) (THIS);
protected:
    enum RTSState
    {
        kRTSStateReady,
        kRTSStateSendingPackets,
        kRTSStateGettingPackets
    };

    INT32                             m_lRefCount;
    IUnknown*                         m_pContext;
    IHXCommonClassFactory*            m_pCommonClassFactory;
    IHXFormatResponse*                m_pResponse;
    UINT32                            m_ulFrameRate;
    UINT32                            m_ulFrameInterval;
    UINT32                            m_ulBackwardsSeekInterval;
    INT32                             m_lPlaybackVelocity;
    UINT32                            m_ulNumStreams;
    CHXStreamInfo**                   m_ppStreamInfo;
    IHXPacketTimeOffsetHandler*       m_pOffsetHandler;
    IHXPlaybackVelocityResponse*      m_pVelocityResponse;
    IHXPlaybackVelocityTimeRegulator* m_pTimeRegulator;
    UINT32                            m_ulLastReverseSeekTime;
    UINT32                            m_ulExternalSeekTime;
    UINT32                            m_ulNumPrefVelocityTiers;
    UINT32*                           m_pulPrefFrameRate;
    UINT32*                           m_pulPrefVelocity;
    HX_BITFIELD                       m_bSendingReversalPackets : 1;
    HX_BITFIELD                       m_bDisableReTimeStamping : 1;
    HX_BITFIELD                       m_bKeyFrameMode : 1;
    HX_BITFIELD                       m_bExternalSeekRequested : 1;
    HX_BITFIELD                       m_bInternalSeekRequested : 1;
    HX_BITFIELD                       m_bReversalBufferingInProgress : 1;
    HX_BITFIELD                       m_bSourceCanDoReverse : 1;

    void              ClearStreamInfoArray();
    HX_RESULT         ReTimeStampAndSendPacket(IHXPacket* pPacket, CHXStreamInfo* pInfo, UINT32 ulNewTime);
    HX_RESULT         _PacketReady(HX_RESULT status, IHXPacket* pPacket, CHXStreamInfo* pInfo);
    HX_RESULT         _SeekDone(HX_RESULT status);
    HX_RESULT         AddPacketToQueue(CHXSimpleList*& rpQueue, IHXPacket* pPacket, HXBOOL bHead);
    HXBOOL            SendPacketFromAnyReversalQueue();
    HXBOOL            SendPacketFromThisReversalQueue(CHXStreamInfo* pInfo);
    UINT32            GetNumQueuedReversalPackets();
    UINT32            ComputeBackwardSeekTime(UINT32 ulStopTime);
    UINT32            ComputeWarpedTimeStamp(UINT32 ulOriginalTime);
    HX_RESULT         ExternalGetPacket(UINT32 ulStreamNumber);
    HX_RESULT         ExternalSeek(UINT32 ulSeekTime);
    HX_RESULT         PacketReadyFilterKeyFrame(IHXPacket* pPacket, CHXStreamInfo* pInfo);
    HXBOOL            IsKeyFramePacket(IHXPacket* pPacket, CHXStreamInfo* pInfo);
    HX_RESULT         PacketReadyReversal(IHXPacket* pPacket, CHXStreamInfo* pInfo);
    HX_RESULT         PacketReadyReTimeStamp(IHXPacket* pPacket, CHXStreamInfo* pInfo);
    HX_RESULT         GetPacketReTimeStamp(CHXStreamInfo* pInfo);
    HX_RESULT         GetPacketReversal(CHXStreamInfo* pInfo);
    HX_RESULT         GetPacketReversalAllStreams();
    void              ReadVelocityPrefs(IUnknown* pContext);
    UINT32            GetFrameRateFromVelocity(INT32 lVelocity, UINT32 ulNumVelocityTiers,
                                               UINT32* pulVelArr, UINT32* pulFrameRateArr);
    HX_RESULT         InitFileHeader(IHXValues* pHeader);
    HX_RESULT         InitStreamHeader(IHXValues* pHeader);
    void              SetKeyFrameModeTransitionFlag();
    virtual void      _SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch) = 0;
    virtual void      _SetKeyFrameMode(HXBOOL bKeyFrameMode) = 0;
    virtual HX_RESULT _BackChannelPacketReady(IHXPacket* pPacket) = 0;
    virtual HX_RESULT _GetOffsetHandler(REF(IHXPacketTimeOffsetHandler*) rpHandler) = 0;
    virtual HX_RESULT _Seek(UINT32 ulOffset) = 0;
    virtual HX_RESULT _GetPacket(CHXStreamInfo* pInfo) = 0;
    virtual HX_RESULT _QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps) = 0;

    static const UINT32 m_ulDefaultVelocity[VELPROXY_DEFAULT_NUM_VELOCITY_TIERS];
    static const UINT32 m_ulDefaultFrameRate[VELPROXY_DEFAULT_NUM_VELOCITY_TIERS+1];
};

class CHXPlaybackVelocityProxyFF : public CHXPlaybackVelocityProxy,
                                   public IHXFileFormatObject
{
public:
    CHXPlaybackVelocityProxyFF(IUnknown* pContext, IHXFileFormatObject* pFileFormat);
    virtual ~CHXPlaybackVelocityProxyFF();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXFileFormatObject methods
    STDMETHOD(GetFileFormatInfo) (THIS_ REF(const char**) pFileMimeTypes,
                                        REF(const char**) pFileExtensions,
                                        REF(const char**) pFileOpenNames);
    STDMETHOD(InitFileFormat)    (THIS_ IHXRequest*        pRequest,
                                        IHXFormatResponse* pFormatResponse,
                                        IHXFileObject*     pFileObject);
    STDMETHOD(GetFileHeader)     (THIS);
    STDMETHOD(GetStreamHeader)   (THIS_ UINT16 unStreamNumber);
    STDMETHOD(GetPacket)         (THIS_ UINT16 unStreamNumber);
    STDMETHOD(Seek)              (THIS_ ULONG32 ulOffset);
    STDMETHOD(Close)             (THIS);
protected:
    IHXFileFormatObject* m_pFileFormat;

    virtual void      _SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    virtual void      _SetKeyFrameMode(HXBOOL bKeyFrameMode);
    virtual HX_RESULT _BackChannelPacketReady(IHXPacket* pPacket);
    virtual HX_RESULT _GetOffsetHandler(REF(IHXPacketTimeOffsetHandler*) rpHandler);
    virtual HX_RESULT _Seek(UINT32 ulOffset);
    virtual HX_RESULT _GetPacket(CHXStreamInfo* pInfo);
    virtual HX_RESULT _QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps);
};

class CHXPlaybackVelocityProxyRS : public CHXPlaybackVelocityProxy,
                                   public IHXRecordSource
{
public:
    CHXPlaybackVelocityProxyRS(IUnknown* pContext, IHXRecordSource* pRecordSource);
    virtual ~CHXPlaybackVelocityProxyRS();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXRecordSource methods
    STDMETHOD(OnFileHeader)      (THIS_ IHXValues* pValues);
    STDMETHOD(OnStreamHeader)    (THIS_ IHXValues* pValues);
    STDMETHOD(OnPacket)          (THIS_ IHXPacket* pPacket, INT32 nTimeOffset);
    STDMETHOD(OnEndOfPackets)    (THIS);
    STDMETHOD(Flush)             (THIS);
    STDMETHOD(SetFormatResponse) (THIS_ IHXFormatResponse* pFormatResponse);
    STDMETHOD(GetFormatResponse) (THIS_ REF(IHXFormatResponse*) pFormatResponse);
    STDMETHOD(GetFileHeader)	 (THIS);
    STDMETHOD(GetStreamHeader)   (UINT32 uStreamNumber);
    STDMETHOD(GetPacket)         (THIS_ UINT16 nStreamNumber);
    STDMETHOD(Seek)              (THIS_ UINT32 nPosition);
    STDMETHOD(Pause)             (THIS);
    STDMETHOD(SetSource)         (THIS_ IUnknown* pUnkSource);

    // CHXPlaybackVelocityProxy methods
    STDMETHOD(Close) (THIS);

    // CHXPlaybackVelocityProxyRS methods
    void SetExpectSortedOnPacketFlag(HXBOOL bFlag) { m_bExpectSortedOnPacket = bFlag; }
protected:
    IHXRecordSource* m_pRecordSource;

    virtual void      _SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    virtual void      _SetKeyFrameMode(HXBOOL bKeyFrameMode);
    virtual HX_RESULT _BackChannelPacketReady(IHXPacket* pPacket);
    virtual HX_RESULT _GetOffsetHandler(REF(IHXPacketTimeOffsetHandler*) rpHandler);
    virtual HX_RESULT _Seek(UINT32 ulOffset);
    virtual HX_RESULT _GetPacket(CHXStreamInfo* pInfo);
    virtual HX_RESULT _QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps);

    HXBOOL m_bExpectSortedOnPacket;
    HXBOOL m_bSeenAnyPackets;
    UINT32 m_ulLastOnPacketTimestamp;
};

#endif /* #ifndef VELPROXY_H */
