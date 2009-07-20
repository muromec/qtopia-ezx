/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sinkctl.h,v 1.15 2007/10/22 21:08:20 ehyche Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _SINKCTL_
#define _SINKCTL_

#include "hxplayvelocity.h"
#include "ihxtlogsystem.h"
#include "errdbg.h"
#include "baseobj.h"

struct IHXInterruptState;
struct IHXClientEngine;
struct IHXCallback;
struct IHXScheduler;
class  CHXSimpleList;
class  ErrorCallback;
class  CHXGenericCallback;
class CRingBuffer;

#define HX_ADVISE_SINK_FLAG_ONPOSLENGTH          0x0001
#define HX_ADVISE_SINK_FLAG_ONPRESENTATIONOPENED 0x0002
#define HX_ADVISE_SINK_FLAG_ONPRESENTATIONCLOSED 0x0004
#define HX_ADVISE_SINK_FLAG_ONSTATISTICSCHANGED  0x0008
#define HX_ADVISE_SINK_FLAG_ONPRESEEK            0x0010
#define HX_ADVISE_SINK_FLAG_ONPOSTSEEK           0x0020
#define HX_ADVISE_SINK_FLAG_ONSTOP               0x0040
#define HX_ADVISE_SINK_FLAG_ONPAUSE              0x0080
#define HX_ADVISE_SINK_FLAG_ONBEGIN              0x0100
#define HX_ADVISE_SINK_FLAG_ONBUFFERING          0x0200
#define HX_ADVISE_SINK_FLAG_ONCONTACTING         0x0400
#define HX_ADVISE_SINK_FLAG_UPDATEVELOCITY       0x0800
#define HX_ADVISE_SINK_FLAG_UPDATEKEYFRAMEMODE   0x1000
#define HX_ADVISE_SINK_FLAG_ALL                  0x1FFF


class CHXAdviseSinkControl : public IHXClientAdviseSink,
                             public IHXInterruptSafe,
                             public IHXPlaybackVelocity,
                             public IHXPlaybackVelocityTimeRegulator,
                             public IHXPlaybackVelocityResponse
{
public:
    CHXAdviseSinkControl();
    ~CHXAdviseSinkControl();
    
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);

    /*
     *  IHXClientAdviseSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPosLength
     *  Purpose:
     *      Called to advise the client that the position or length of the
     *      current playback context has changed.
     */
    STDMETHOD(OnPosLength)              (THIS_
                                        UINT32    ulPosition,
                                        UINT32    ulLength);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationOpened
     *  Purpose:
     *      Called to advise the client a presentation has been opened.
     */
    STDMETHOD(OnPresentationOpened)     (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationClosed
     *  Purpose:
     *      Called to advise the client a presentation has been closed.
     */
    STDMETHOD(OnPresentationClosed)     (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStatisticsChanged
     *  Purpose:
     *      Called to advise the client that the presentation statistics
     *      have changed. 
     */
    STDMETHOD(OnStatisticsChanged)      (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPreSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek is
     *      about to occur. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek will be completed.
     *
     */
    STDMETHOD (OnPreSeek)       (THIS_
                                ULONG32             ulOldTime,
                                ULONG32             ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPostSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek has
     *      just occured. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek.
     *
     */
    STDMETHOD (OnPostSeek)      (THIS_
                                ULONG32             ulOldTime,
                                ULONG32             ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStop
     *  Purpose:
     *      Called by client engine to inform the client that a stop has
     *      just occured. 
     *
     */
    STDMETHOD (OnStop)          (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPause
     *  Purpose:
     *      Called by client engine to inform the client that a pause has
     *      just occured. The render is informed the last time for the 
     *      stream's time line before the pause.
     *
     */
    STDMETHOD (OnPause)         (THIS_
                                ULONG32             ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBegin
     *  Purpose:
     *      Called by client engine to inform the client that a begin or
     *      resume has just occured. The render is informed the first time 
     *      for the stream's time line after the resume.
     *
     */
    STDMETHOD (OnBegin)         (THIS_
                                ULONG32             ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBuffering
     *  Purpose:
     *      Called by client engine to inform the client that buffering
     *      of data is occuring. The render is informed of the reason for
     *      the buffering (start-up of stream, seek has occured, network
     *      congestion, etc.), as well as percentage complete of the 
     *      buffering process.
     *
     */
    STDMETHOD (OnBuffering)     (THIS_
                                ULONG32             ulFlags,
                                UINT16              unPercentComplete);


    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnContacting
     *  Purpose:
     *      Called by client engine to inform the client is contacting
     *      hosts(s).
     *
     */
    STDMETHOD (OnContacting)            (THIS_
                                        const char* pHostName);

    // IHXInteruptSafe methods. 
    STDMETHOD_(HXBOOL,IsInterruptSafe)            (THIS);

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse);
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps);
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch);
    STDMETHOD_(INT32,GetVelocity)          (THIS);
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode);
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS);
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return 0; }
    STDMETHOD(CloseVelocityControl)        (THIS);

    // IHXPlaybackVelocityTimeRegulator methods
    STDMETHOD(SetCurrentTimeWarp)      (THIS_ UINT32 ulOrigTime, UINT32 ulWarpTime);
    STDMETHOD(GetLastTimeWarp)         (THIS_ REF(UINT32) rulOrigTime, REF(UINT32) rulWarpTime);
    STDMETHOD_(UINT32,GetOriginalTime) (THIS_ UINT32 ulWarpTime);
    STDMETHOD_(UINT32,GetWarpedTime)   (THIS_ UINT32 ulOrigTime);

    // IHXPlaybackVelocityResponse methods
    STDMETHOD(UpdateVelocityCaps) (THIS_ IHXPlaybackVelocityCaps* pCaps) { return HXR_OK; }
    STDMETHOD(UpdateVelocity)     (THIS_ INT32 lVelocity);
    STDMETHOD(UpdateKeyFrameMode) (THIS_ HXBOOL bKeyFrameMode);

    /*
     *  CHXAdviseSinkControl methods
     */

    HX_RESULT AddAdviseSink(IHXClientAdviseSink*        pAdviseSink);

    HX_RESULT RemoveAdviseSink(IHXClientAdviseSink*     pAdviseSink);

    void      Init(IHXClientEngine* pEngine);

    void      DisableAdviseSink(UINT32 ulFlags) { m_ulEnabledFlags &= ~(ulFlags & HX_ADVISE_SINK_FLAG_ALL); }
    void      EnableAdviseSink(UINT32 ulFlags)  { m_ulEnabledFlags |= (ulFlags & HX_ADVISE_SINK_FLAG_ALL);  }
    HXBOOL      IsEnabled(UINT32 ulFlags)         { return ((m_ulEnabledFlags & ulFlags) ? TRUE : FALSE); }

private:

    CHXSimpleList      m_SinkList;
    LONG32             m_lRefCount;
    IHXInterruptState* m_pInterruptState;
    IHXScheduler*      m_pScheduler;
    INT32              m_lPlaybackVelocity;
    HXBOOL             m_bKeyFrameMode;
    CRingBuffer*       m_pOrigTime;
    CRingBuffer*       m_pWarpTime;
    UINT32             m_ulEnabledFlags;

    struct PlayerAdviseSink;
    friend struct PlayerAdviseSink;

    struct PlayerAdviseSink
    {
        PlayerAdviseSink(IHXClientAdviseSink*   pAdviseSink,
                        HXBOOL bInterruptSafe);

        ~PlayerAdviseSink();

        IHXClientAdviseSink* m_pAdviseSink;
        HXBOOL               m_bInterruptSafe;
        CHXSimpleList*       m_pPendingAdviseList;
    };

    struct PendingAdvise;
    friend struct PendingAdvise;

    struct PendingAdvise
    {
        PendingAdvise()
        {
            m_pHostName = NULL;
        }

        ~PendingAdvise()
        {
            if (m_pHostName)
            {
                delete [] m_pHostName;
            }
        }

        UINT32      m_ulAdviseType;
        UINT32      m_ulArg1;
        UINT32      m_ulArg2;
        char*       m_pHostName;
    };

    void AddToPendingList(PlayerAdviseSink* pPlayerAdviseSink, UINT32 ulType,
                          UINT32 ulArg1, UINT32 ulArg2, char* pHostName);

    void ProcessPendingRequests(PlayerAdviseSink* pPlayerAdviseSink);
    void ProcessAllRequests(void);

    CHXGenericCallback*    m_pCallback;
    static void AdviseSinkCallback(void* pParam);
    HX_RESULT ReallocRingBuffer(CRingBuffer* pBuf, REF(CRingBuffer*) rpBuf);
    void      ClearAllRegulatorTimes();
    void      IssueAdviseSinkCall(IHXClientAdviseSink* pSink, UINT32 ulType,
                                  UINT32 ulArg1, UINT32 ulArg2, char* pszArg3);
    void      CallAllAdviseSinks(UINT32 ulType, UINT32 ulArg1 = 0,
                                 UINT32 ulArg2 = 0, char* pszArg3 = NULL);
};


struct ErrorReport
{
    ErrorReport()
    {
        m_unSeverity    = 0;
        m_ulHXCode      = 0;
        m_ulUserCode    = 0;
        m_pUserString   = NULL;
        m_pMoreInfoURL  = NULL;
    }

    ~ErrorReport()
    {
        m_unSeverity    = 0;
        m_ulHXCode      = 0;
        m_ulUserCode    = 0;
        HX_VECTOR_DELETE(m_pUserString);
        HX_VECTOR_DELETE(m_pMoreInfoURL);
    }

    UINT8       m_unSeverity;
    HX_RESULT   m_ulHXCode;
    ULONG32     m_ulUserCode;
    char*       m_pUserString;
    char*       m_pMoreInfoURL;
};

class CHXErrorSinkControl : public IHXErrorSinkControl
{
    public:
        CHXErrorSinkControl();
        ~CHXErrorSinkControl();
        /*
         * IHXErrorSinkControl methods
         */
        STDMETHOD(AddErrorSink)         (THIS_ 
                                        IHXErrorSink*   pErrorSink,
                                        const UINT8     unLowSeverity,
                                        const UINT8     unHighSeverity);
        STDMETHOD(RemoveErrorSink)      (THIS_ IHXErrorSink*    pErrorSink);
        /*
         * IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)      (THIS);
        STDMETHOD_(ULONG32,Release)     (THIS);
    
        //get the severity range for pErrorSink
        void GetSeverityRange(IHXErrorSink* pErrorSink, UINT8& unLowSeverity, 
                                                         UINT8& unHighSeverity);
        void Init(IHXClientEngine* pEngine);
        void Close();
        
        HX_RESULT ErrorOccurred(const UINT8     unSeverity,  
                                const ULONG32   ulHXCode,
                                const ULONG32   ulUserCode,
                                const char*     pUserString,
                                const char*     pMoreInfoURL
                                );
        struct PlayerErrorSink
        {
            PlayerErrorSink(IHXErrorSink*  pErrorSink,
                            const UINT8     unLowSeverity,
                            const UINT8     unHighSeverity)
            {
                m_pErrorSink = pErrorSink;
                m_unLowSeverity = unLowSeverity;
                m_unHighSeverity = unHighSeverity;
            };
        
            IHXErrorSink*       m_pErrorSink;
            UINT8               m_unLowSeverity;
            UINT8               m_unHighSeverity;
        };
        
    public:
        CHXSimpleList   m_SinkList;
    
    private:

        HX_RESULT       CallReport(
                                const UINT8     unSeverity,  
                                HX_RESULT       ulHXCode,
                                const ULONG32   ulUserCode,
                                const char*     pUserString,
                                const char*     pMoreInfoURL);

        void            ReportPendingErrors(void);
    
        friend class ErrorCallback;

        LONG32              m_lRefCount;
        IHXInterruptState* m_pInterruptState;
        IHXScheduler*       m_pScheduler;
        CHXSimpleList*      m_pPendingErrorList;
        CHXGenericCallback* m_pErrorCallback;
        static void ErrorCallback(void* pParam);
};

#if defined(HELIX_FEATURE_LOGGING_TRANSLATOR)

class CHXErrorSinkTranslator : public IHXErrorSink,
                               public CHXBaseCountingObject
{
public:
    CHXErrorSinkTranslator();
    virtual ~CHXErrorSinkTranslator();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXErrorSink methods
    STDMETHOD(ErrorOccurred) (THIS_ const UINT8   unSeverity,
                                    const ULONG32 ulHXCode,
                                    const ULONG32 ulUserCode,
                                    const char*   pUserString,
                                    const char*   pMoreInfoURL);
protected:
    INT32 m_lRefCount;

    static const EHXTLogFuncArea m_ulCoreDebugTo4cc[NUM_DOL_CODES];
    static const EHXTLogCode     m_ulCoreDebugToLevel[NUM_DOL_CODES];
};

#endif /* #if defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */


///////////////////////////////////////////////////////////

#define HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE	 0x0001
#define HX_CLIENT_STATE_ADVISE_SINK_FLAG_ALL            0x000F

class CHXClientStateAdviseSink : 
                             public IHXClientStateAdviseSink,
                             public IHXInterruptSafe
{
public:
    CHXClientStateAdviseSink();
    ~CHXClientStateAdviseSink();
    
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);

    /*
     *  IHXClientStateAdviseSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXClientStateAdviseSink::OnStateChange
     *  Purpose:
     *      Called by client engine to inform the client that a state has changed
     *
     */
    STDMETHOD (OnStateChange)	(THIS_
                                UINT16	uOldState,
				UINT16	uNewState);

    // IHXInteruptSafe methods. 
    STDMETHOD_(HXBOOL,IsInterruptSafe)            (THIS);

    HX_RESULT AddClientStateAdviseSink(IHXClientStateAdviseSink*	pClientStateAdviseSink);
    HX_RESULT RemoveClientStateAdviseSink(IHXClientStateAdviseSink*	pClientStateAdviseSink);

    void      Init(IHXClientEngine* pEngine);

    void      DisableClientStateAdviseSink(UINT32 ulFlags) { m_ulEnabledFlags &= ~(ulFlags & HX_CLIENT_STATE_ADVISE_SINK_FLAG_ALL); }
    void      EnableClientStateAdviseSink(UINT32 ulFlags)  { m_ulEnabledFlags |= (ulFlags & HX_CLIENT_STATE_ADVISE_SINK_FLAG_ALL);  }
    HXBOOL    IsEnabled(UINT32 ulFlags)         { return ((m_ulEnabledFlags & ulFlags) ? TRUE : FALSE); }

private:

    CHXSimpleList       m_SinkList;
    LONG32              m_lRefCount;
    IHXInterruptState* m_pInterruptState;
    IHXScheduler*       m_pScheduler;
    UINT32              m_ulEnabledFlags;

    struct PlayerClientStateAdviseSink;
    friend struct PlayerClientStateAdviseSink;

    struct PlayerClientStateAdviseSink
    {
        PlayerClientStateAdviseSink(IHXClientStateAdviseSink*   pClientStateAdviseSink,
                        HXBOOL bInterruptSafe);

        ~PlayerClientStateAdviseSink();
    
        IHXClientStateAdviseSink*    m_pClientStateAdviseSink;
        HXBOOL                    m_bInterruptSafe;
        CHXSimpleList*          m_pPendingAdviseList;
    };

    struct PendingAdvise;
    friend struct PendingAdvise;

    struct PendingAdvise
    {
        PendingAdvise()
        {
            m_pHostName = NULL;
        }

        ~PendingAdvise()
        {
            if (m_pHostName)
            {
                delete [] m_pHostName;
            }
        }

        UINT32      m_ulAdviseType;
        UINT32      m_ulArg1;
        UINT32      m_ulArg2;
        char*       m_pHostName;
    };

    void AddToPendingList(PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink, UINT32 ulType,
                          UINT32 ulArg1, UINT32 ulArg2, char* pHostName);

    void ProcessPendingRequests(PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink);
    void ProcessAllRequests(void);

    CHXGenericCallback*    m_pCallback;
    static void ClientStateAdviseSinkCallback(void* pParam);
    void      IssueClientStateAdviseSinkCall(IHXClientStateAdviseSink* pSink, UINT32 ulType,
                                  UINT32 ulArg1, UINT32 ulArg2, char* pszArg3);
    void      CallAllClientStateAdviseSinks(UINT32 ulType, UINT32 ulArg1 = 0,
                                 UINT32 ulArg2 = 0, char* pszArg3 = NULL);
};


#endif /* _SINKCTL_ */
