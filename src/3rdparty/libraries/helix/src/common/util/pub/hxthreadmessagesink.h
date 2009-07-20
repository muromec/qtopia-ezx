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

//
// Provides a way to hook into a thread's message queue without neeeding to coordinate
// with the code that implemnents the thread message queue. Message posters target the
// given sink by specifying a sink handle with the message (an HWND on windows). Message
// handlers register with the message sink to process  messages. Only one handler may exist
// for a given message. 
//
 
//
#if !defined( HXTHREADMESSAGESINK_H__ )
#define HXTHREADMESSAGESINK_H__

#include "chxmapptrtoptr.h"
#include "hxrefcounted.h"
#include "hxengin.h"

class HXThreadMessage;

//XXXLCM fix this (see HXThread PostMessage)
#if defined(_WINDOWS)
typedef HWND MsgSinkHandle;
#else
typedef void* MsgSinkHandle;
#endif
const MsgSinkHandle INVALID_MSGSINK_HANDLE = 0;


class HXThreadMessageSink
: public HXRefCounted
{
public:
    class MessageHandler
    {
    public:
        virtual UINT32 HandleMessage(const HXThreadMessage& msg) = 0;
    };
public:
    // ref-counted per-thread instance
    static HX_RESULT GetThreadInstance(HXThreadMessageSink*& pSink, IUnknown* pContext);
    static void DestroyThreadInstance();
  
    HX_RESULT AddHandler(UINT32 msg, HXThreadMessageSink::MessageHandler* pHandler);
    HX_RESULT RemoveHandler(UINT32 msg);
    HXBOOL      IsHandled(UINT32 msg);
    
    MsgSinkHandle GetSinkHandle() const;
    UINT32 GetTID() const;

protected:
    HXThreadMessageSink();
    virtual ~HXThreadMessageSink();

    virtual HX_RESULT Init(IUnknown* pContext);
    static HX_RESULT Create(HXThreadMessageSink*& pMsgSink, IUnknown* pContext);
private:
    // HXRefCounted
    void FinalRelease();
    
protected:
    IUnknown* m_pContext;
    MsgSinkHandle m_handle;
    CHXMapPtrToPtr m_handlers;
    IHXMutex* m_pMutex;
    UINT32 m_tid;
};


inline 
MsgSinkHandle HXThreadMessageSink::GetSinkHandle() const
{
    return m_handle;
}

inline
UINT32 HXThreadMessageSink::GetTID() const
{
    return m_tid;
}

#endif	//if !defined( HXTHREADMESSAGESINK_H__ )

