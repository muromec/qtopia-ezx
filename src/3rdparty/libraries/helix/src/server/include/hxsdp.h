/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsdp.h,v 1.4 2004/04/16 23:50:21 darrick Exp $ 
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

#ifndef _HXSDP_H_
#define _HXSDP_H_

typedef _INTERFACE IHXSDPAttrib    IHXSDPAttrib;
typedef _INTERFACE IHXSDPTimeDesc  IHXSDPTimeDesc;
typedef _INTERFACE IHXSDPMedia     IHXSDPMedia;
typedef _INTERFACE IHXSDP          IHXSDP;

/*
 * SDP Attribute
 */
// IHXSDPAttrib: 538d82b4-a284-4527-aef5-47ce87d831cc
DEFINE_GUID(IID_IHXSDPAttrib, 0x538d82b4, 0xa284, 0x4527,
			0xae, 0xf5, 0x47, 0xce, 0x87, 0xd8, 0x31, 0xcc);
#undef  INTERFACE
#define INTERFACE   IHXSDPAttrib
DECLARE_INTERFACE_(IHXSDPAttrib, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXSDPAttrib
    STDMETHOD_(UINT32,GetSize)          ( THIS ) PURE;
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf ) PURE;
    STDMETHOD(GetName)                  ( THIS_ REF(IHXBuffer*) pbufName ) PURE;
    STDMETHOD(SetName)                  ( THIS_ IHXBuffer*      pbufName ) PURE;
    STDMETHOD_(UINT32,GetType)          ( THIS ) PURE;
    STDMETHOD(SetType)                  ( THIS_ UINT32 sdptype ) PURE;
    STDMETHOD(GetValue)                 ( THIS_ REF(IHXBuffer*) pbufValue ) PURE;
    STDMETHOD(SetValue)                 ( THIS_ IHXBuffer*      pbufValue ) PURE;
    STDMETHOD(GetIntValue)              ( THIS_ REF(INT32)       iValue ) PURE;
    STDMETHOD(SetIntValue)              ( THIS_ INT32            iValue ) PURE;
    STDMETHOD(GetStrValue)              ( THIS_ REF(IHXBuffer*) pbufValue ) PURE;
    STDMETHOD(SetStrValue)              ( THIS_ IHXBuffer*      pbufValue ) PURE;
    STDMETHOD(GetBufValue)              ( THIS_ REF(IHXBuffer*) pbufValue ) PURE;
    STDMETHOD(SetBufValue)              ( THIS_ IHXBuffer*      pbufValue ) PURE;
};

/*
 * SDP Time Description: tr*
 */
// IHXSDPTimeDesc: ffc46a97-965d-41cb-a350-de2c486e7290
DEFINE_GUID(IID_IHXSDPTimeDesc, 0xffc46a97, 0x965d, 0x41cb,
			0xa3, 0x50, 0xde, 0x2c, 0x48, 0x6e, 0x72, 0x90);
#undef  INTERFACE
#define INTERFACE   IHXSDPTimeDesc
DECLARE_INTERFACE_(IHXSDPTimeDesc, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXSDPTimeDesc
    STDMETHOD_(UINT32,GetSize)          ( THIS ) PURE;
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf ) PURE;
    STDMETHOD(GetTime)                  ( THIS_ REF(IHXBuffer*) pbufTime ) PURE;
    STDMETHOD(SetTime)                  ( THIS_ IHXBuffer* pbufTime ) PURE;
    STDMETHOD(GetRepeatList)            ( THIS_ REF(IHXList*) plistRepeat ) PURE;
    STDMETHOD(GetRepeatListConst)       ( THIS_ REF(IHXList*) plistRepeat ) PURE;
};

/*
 * SDP Media block: mi?c?b?k?a*
 */
// IHXSDPMedia: 30069090-a6dc-47c5-891f-9eea9c860a6b
DEFINE_GUID(IID_IHXSDPMedia, 0x30069090, 0xa6dc, 0x47c5,
			0x89, 0x1f, 0x9e, 0xea, 0x9c, 0x86, 0x0a, 0x6b);
#undef  INTERFACE
#define INTERFACE   IHXSDPMedia
DECLARE_INTERFACE_(IHXSDPMedia, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXSDPMedia
    STDMETHOD_(UINT32,GetSize)          ( THIS ) PURE;
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf ) PURE;
    STDMETHOD(GetMediaName)             ( THIS_ REF(IHXBuffer*) pbufMediaName ) PURE;
    STDMETHOD(SetMediaName)             ( THIS_ IHXBuffer*      pbufMediaName ) PURE;
    STDMETHOD(GetTitle)                 ( THIS_ REF(IHXBuffer*) pbufTitle ) PURE;
    STDMETHOD(SetTitle)                 ( THIS_ IHXBuffer*      pbufTitle ) PURE;
    STDMETHOD(GetConnInfo)              ( THIS_ REF(IHXBuffer*) pbufConnInfo ) PURE;
    STDMETHOD(SetConnInfo)              ( THIS_ IHXBuffer*      pbufConnInfo ) PURE;
    STDMETHOD(GetBwidInfo)              ( THIS_ REF(IHXBuffer*) pbufBwidInfo ) PURE;
    STDMETHOD(SetBwidInfo)              ( THIS_ IHXBuffer*      pbufBwidInfo ) PURE;
    STDMETHOD(GetEncryptKey)            ( THIS_ REF(IHXBuffer*) pbufEncryptKey ) PURE;
    STDMETHOD(SetEncryptKey)            ( THIS_ IHXBuffer*      pbufEncryptKey ) PURE;
    STDMETHOD(GetAttribList)            ( THIS_ REF(IHXList*)   plistAttrib ) PURE;
    STDMETHOD(GetAttribListConst)       ( THIS_ REF(IHXList*)   plistAttrib ) PURE;
};

/*
 * SDP: vosi?u?e?p?c?b?(tr*)+z?k?a*(mi?c?b?k?a*)*
 */
// IHXSDP: 6fe2a32b-36fa-4fd4-8809-1c5ada9bea15
DEFINE_GUID(IID_IHXSDP, 0x6fe2a32b, 0x36fa, 0x4fd4,
			0x88, 0x09, 0x1c, 0x5a, 0xda, 0x9b, 0xea, 0x15);
#undef  INTERFACE
#define INTERFACE   IHXSDP
DECLARE_INTERFACE_(IHXSDP, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXSDP
    STDMETHOD_(UINT32,GetSize)          ( THIS ) PURE;
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf ) PURE;
    STDMETHOD_(IHXBuffer*,AsBuffer)    ( THIS ) PURE;
    STDMETHOD_(UINT32,GetVersion)       ( THIS ) PURE;
    STDMETHOD(SetVersion)               ( THIS_ UINT32 ver ) PURE;
    STDMETHOD(GetOrigin)                ( THIS_ REF(IHXBuffer*) pbufOrigin ) PURE;
    STDMETHOD(SetOrigin)                ( THIS_ IHXBuffer*      pbufOrigin ) PURE;
    STDMETHOD(GetName)                  ( THIS_ REF(IHXBuffer*) pbufName ) PURE;
    STDMETHOD(SetName)                  ( THIS_ IHXBuffer*      pbufName ) PURE;
    STDMETHOD(GetInfo)                  ( THIS_ REF(IHXBuffer*) pbufInfo ) PURE;
    STDMETHOD(SetInfo)                  ( THIS_ IHXBuffer*      pbufInfo ) PURE;
    STDMETHOD(GetUrl)                   ( THIS_ REF(IHXBuffer*) pbufUrl ) PURE;
    STDMETHOD(SetUrl)                   ( THIS_ IHXBuffer*      pbufUrl ) PURE;
    STDMETHOD(GetEmail)                 ( THIS_ REF(IHXBuffer*) pbufEmail ) PURE;
    STDMETHOD(SetEmail)                 ( THIS_ IHXBuffer*      pbufEmail ) PURE;
    STDMETHOD(GetPhone)                 ( THIS_ REF(IHXBuffer*) pbufPhone ) PURE;
    STDMETHOD(SetPhone)                 ( THIS_ IHXBuffer*      pbufPhone ) PURE;
    STDMETHOD(GetConnInfo)              ( THIS_ REF(IHXBuffer*) pbufConnInfo ) PURE;
    STDMETHOD(SetConnInfo)              ( THIS_ IHXBuffer*      pbufConnInfo ) PURE;
    STDMETHOD(GetBwidInfo)              ( THIS_ REF(IHXBuffer*) pbufBwidInfo ) PURE;
    STDMETHOD(SetBwidInfo)              ( THIS_ IHXBuffer*      pbufBwidInfo ) PURE;
    STDMETHOD(GetTimeDescList)          ( THIS_ REF(IHXList*)   plistTimeDesc ) PURE;
    STDMETHOD(GetTimeDescListConst)     ( THIS_ REF(IHXList*)   plistTimeDesc ) PURE;
    STDMETHOD(GetTimeZone)              ( THIS_ REF(IHXBuffer*) pbufTimeZone ) PURE;
    STDMETHOD(SetTimeZone)              ( THIS_ IHXBuffer*      pbufTimeZone ) PURE;
    STDMETHOD(GetEncryptKey)            ( THIS_ REF(IHXBuffer*) pbufEncryptKey ) PURE;
    STDMETHOD(SetEncryptKey)            ( THIS_ IHXBuffer*      pbufEncryptKey ) PURE;
    STDMETHOD(GetAttribList)            ( THIS_ REF(IHXList*)   plistAttribs ) PURE;
    STDMETHOD(GetAttribListConst)       ( THIS_ REF(IHXList*)   plistAttribs ) PURE;
    STDMETHOD(GetMediaList)             ( THIS_ REF(IHXList*)   plistMedia ) PURE;
    STDMETHOD(GetMediaListConst)        ( THIS_ REF(IHXList*)   plistMedia ) PURE;
};

// IHXSDPAggregateStats: 4ed8aabe-5597-410b-a2-90-81-81-be-1e-24-03);

DEFINE_GUID(IID_IHXSDPAggregateStats, 0x4ed8aabe, 0x5597, 0x410b, 0xa2, 0x90, 0x81,            0x81, 0xbe, 0x1e, 0x24, 0x03);

#undef  INTERFACE
#define INTERFACE   IHXSDPAggregateStats
DECLARE_INTERFACE_(IHXSDPAggregateStats, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    
    STDMETHOD(IncrementSDPDownloadCount)    (THIS_ UINT32 ulProtocolId,
                                                   BOOL bSuccess) PURE;
    STDMETHOD(IncrementSDPSessionInitCount) (THIS_ UINT32 ulProtocolId,
                                                   BOOL bSuccess) PURE;
};


#endif /* ndef _HXSDP_H_ */
