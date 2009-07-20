/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rtspmsg2.h,v 1.3 2003/05/13 20:53:05 tmarshall Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#ifndef _RTSPMSG2_H_
#define _RTSPMSG2_H_

#include "hxcomm.h"

// The Product Formerly Known As RealProxy has a CRTSPMessage class so
// call ours something different
class CRTSPMessageBase : public IHXRTSPMessage
{
public:
    CRTSPMessageBase(IHXFastAlloc* pFastAlloc);
    virtual ~CRTSPMessageBase(void);

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXRTSPMessage
    STDMETHOD_(UINT16,GetVersion)   (THIS);
    STDMETHOD(SetVersion)           (THIS_ UINT16 ver);
    STDMETHOD_(int,GetMajorVersion) (THIS);
    STDMETHOD(SetMajorVersion)      (THIS_ int uMajorVer);
    STDMETHOD_(int,GetMinorVersion) (THIS);
    STDMETHOD(SetMinorVersion)      (THIS_ int uMinorVer);
    STDMETHOD_(UINT32,GetCSeq)      (THIS);
    STDMETHOD(SetCSeq)              (THIS_ UINT32 cseq);
    STDMETHOD(GetContent)           (THIS_ REF(IHXBuffer*) pbufContent);
    STDMETHOD(SetContent)           (THIS_ IHXBuffer* pbuf);
    STDMETHOD(GetHeaderList)        (THIS_ REF(IHXList*) plistHeaders);
    STDMETHOD(GetHeaderListConst)   (THIS_ REF(IHXList*) plistHeaders);
    STDMETHOD(GetHeader)            (THIS_ const char* key, REF(IHXMIMEHeader*) pHeader);
    STDMETHOD(AddHeader)            (THIS_ IHXMIMEHeader* pHeader);
    STDMETHOD(SetHeader)            (THIS_ IHXMIMEHeader* pHeader);
    STDMETHOD(RemoveHeader)         (THIS_ const char* key);
    STDMETHOD_(void,ReplaceDelimiters)(THIS_ 
				    BOOL bReplaceDelimiters,
				    int nReplacementDelimiter);

protected:
    UINT32  GetHeaderCount(void);
    int     ParseHeader   (IHXBuffer* pbuf, UINT32* ppos);
    int     ParseData     (IHXBuffer* pbuf, UINT32* ppos);

protected:
    ULONG32             m_ulRefCount;
    int                 m_iRecvState;
    UINT32              m_ulRecvLeft;
    UINT16              m_uVer;
    int                 m_nMajorVersion;
    int                 m_nMinorVersion;
    IHXList*            m_plistHeaders;
    IHXBuffer*          m_pbufContent;
    BOOL                m_bReplaceDelimiters;
    int	                m_nReplacementDelimiter;
    IHXFastAlloc*       m_pFastAlloc;
};

class CRTSPRequestMessage : public CRTSPMessageBase,
                            public IHXRTSPConsumer,
                            public IHXRTSPRequestMessage
{
public:
    CRTSPRequestMessage(IHXFastAlloc* pFastAlloc);
    virtual ~CRTSPRequestMessage(void);

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXRTSPConsumer
    STDMETHOD(ReadDone)             (THIS_ IHXBuffer* pbufPacket, UINT32* ppos);
    STDMETHOD_(UINT32,GetSize)      (THIS);
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf);
    STDMETHOD_(UINT32,AsBuffer)     (THIS_ REF(IHXBuffer*) pbuf );

    // IHXRTSPRequestMessage
    STDMETHOD_(RTSPMethod,GetMethod)(THIS);
    STDMETHOD_(UINT32,GetVerb)      (THIS);
    STDMETHOD(SetVerb)              (THIS_ UINT32 verb);
    STDMETHOD(GetVerbEx)            (THIS_ REF(IHXBuffer*) pbufVerb);
    STDMETHOD(SetVerbEx)            (THIS_ IHXBuffer* pbufVerb);
    STDMETHOD(GetUrl)               (THIS_ REF(IHXBuffer*) pbufUrl);
    STDMETHOD(SetUrl)               (THIS_ IHXBuffer* pbufUrl);

protected:
    int ParseCommand(IHXBuffer* pbuf, UINT32* ppos);

protected:
    IHXBuffer*          m_pbufMessage;
    IHXBuffer*          m_pbufVerb;
    IHXBuffer*          m_pbufUrl;
};

class CRTSPResponseMessage : public CRTSPMessageBase,
                             public IHXRTSPConsumer,
                             public IHXRTSPResponseMessage
{
public:
    CRTSPResponseMessage(IHXFastAlloc* pFastAlloc);
    virtual ~CRTSPResponseMessage(void);

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXRTSPConsumer
    STDMETHOD(ReadDone)             (THIS_ IHXBuffer* pbufPacket, UINT32* ppos);
    STDMETHOD_(UINT32,GetSize)      (THIS);
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf);
    STDMETHOD_(UINT32,AsBuffer)     (THIS_ REF(IHXBuffer*) pbuf );

    // IHXRTSPResponseMessage
    STDMETHOD(GetStatusCode)        (THIS_ REF(UINT32) status);
    STDMETHOD(SetStatusCode)        (THIS_ UINT32 status);
    STDMETHOD(GetStatusText)        (THIS_ REF(IHXBuffer*) pbufStatus);
    STDMETHOD(SetStatusText)        (THIS_ IHXBuffer* pbufStatus);

protected:
    int ParseCommand(IHXBuffer* pbuf, UINT32* ppos);

private:
    IHXBuffer*          m_pbufMessage;
    UINT32              m_statuscode;
    IHXBuffer*          m_pbufReason;
};

class CRTSPInterleavedPacket : public IHXRTSPConsumer,
                               public IHXRTSPInterleavedPacket
{
public:
    CRTSPInterleavedPacket(IHXFastAlloc* pFastAlloc);
    virtual ~CRTSPInterleavedPacket(void);

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXRTSPConsumer
    STDMETHOD(ReadDone)             (THIS_ IHXBuffer* pbufPacket, UINT32* ppos);
    STDMETHOD_(UINT32,GetSize)      (THIS);
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf);
    STDMETHOD_(UINT32,AsBuffer)     (THIS_ REF(IHXBuffer*) pbuf );

    // IHXRTSPInterleavedPacket
    STDMETHOD(Get)                  (THIS_ REF(BYTE) byChan,
                                           REF(IHXBuffer*) pbufData);
    STDMETHOD(Set)                  (THIS_ BYTE byChan, IHXBuffer* pbufData);

protected:
    ULONG32             m_ulRefCount;
    int                 m_iRecvState;
    UINT32              m_uRecvLeft;
    IHXBuffer*          m_pbufPacket;
    IHXFastAlloc*       m_pFastAlloc;
};

#endif /* ndef _RTSPMSG2_H_ */

