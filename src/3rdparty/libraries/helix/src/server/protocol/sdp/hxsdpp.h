/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsdpp.h,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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
/*
 *  This is an implementation of SDP, as specified in RFC 2327.
 */
#ifndef _HXSDPP_H_
#define _HXSDPP_H_

#include "hxcomm.h"    // IHXFastAlloc


/*
 * The spec gives a rigorous ordering for the lines within an SDP message. 
 * I have reproduced this below, but it is not fully enforced.  Previous
 * versions of the RealSystem produced SDP which was improperly ordered and
 * we must be able to handle it.  Specifically, the t= line appears after
 * several a= lines in RealSystem 7.0.
 *
 *
 * The email and phone (e=, p=) fields are a bit ill-defined in the spec. 
 * It says "Either an email field or a phone field must be specified. 
 * Additional email and phone fields are allowed." But it also says "If
 * these are present, they should be specified before the first media
 * field", as if they were optional and their placement was in doubt. 
 * Furthermore, it says "More than one email or phone field can be given for
 * a session description", but does not mention this in the field order.  We
 * treat them as optional and allow no more than one of each.
 *
 *
 * Attribute fields may be of two forms:
 *   - property attributes.  A property attribute is simply of the form
 *     "a=<flag>".
 *   - value attributes.  They are of the form "a=<attribute>:<value>".
 *     Value attributes may be further classified in RMA by type, as
 *     "a=<attribute>:<type>;<value>":
 *       - Integer "a=<attribute>:integer;int-val"
 *       - String  "a=<attribute>:string;quoted-escaped-string"
 *       - Buffer  "a=<attribute>:buffer;quoted-base64-buffer"
 *
 *
 * v=0
 * o=- 938128914 938128914 IN IP4 192.168.143.13
 * s=G2 Audio Experience
 * i=RealNetworks ©1998
 * c=IN IP4 0.0.0.0
 * t=0 0
 * a=SdpplinVersion:1610642970
 * a=Flags:integer;2
 * a=IsRealDataType:integer;1
 * a=StreamCount:integer;1
 * a=Title:buffer;"RzIgQXVkaW8gRXhwZXJpZW5jZQA="
 * a=Copyright:buffer;"qTE5OTgA"
 * a=Author:buffer;"UmVhbE5ldHdvcmtzAA=="
 * a=range:npt=0-39.007000
 * m=audio 0 RTP/AVP 101
 * b=AS:65
 * a=control:streamid=0
 * a=range:npt=0-39.007000
 * a=length:npt=39.007000
 * a=rtpmap:101 x-pn-realaudio
 * a=mimetype:string;"audio/x-pn-realaudio"
 * a=MinimumSwitchOverlap:integer;200
 * a=StartTime:integer;0
 * a=AvgBitRate:integer;64695
 * a=EndOneRuleEndAll:integer;1
 * a=AvgPacketSize:integer;744
 * a=SeekGreaterOnSwitch:integer;0
 * a=Preroll:integer;4536
 * a=MaxPacketSize:integer;744
 * a=MaxBitRate:integer;64695
 * a=RMFF 1.0 Flags:buffer;"AAgAAgAAAAIAAAACAAAAAgAA"
 * a=OpaqueData:buffer;"..."
 * a=StreamName:string;"audio/x-pn-multirate-realaudio logical stream"
 * a=ASMRuleBook:string;"..."
 */

// Bit 1 denotes prop/val, bits 2-3 denote type (if val)
#define SDP_ATTR_UNKNOWN        0x00    /* xxx0 */
#define SDP_ATTR_PROP           0x01    /* xx01 */
#define SDP_ATTR_VAL            0x03    /* xx11 */
#define SDP_ATTR_VAL_GENERIC    0x03    /* 0011 */
#define SDP_ATTR_VAL_INT        0x07    /* 0111 */
#define SDP_ATTR_VAL_STR        0x0B    /* 1011 */
#define SDP_ATTR_VAL_BUF        0x0F    /* 1111 */

class CSDPAttrib : public IHXSDPAttrib
{
public:
    CSDPAttrib( IHXFastAlloc* pFastAlloc );
    virtual ~CSDPAttrib( void );

    void Init( IHXBuffer* pbufAttrib );
    void Parse( void );

    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)          ( THIS );
    STDMETHOD_(ULONG32,Release)         ( THIS );

    // IHXSDPAttrib
    STDMETHOD_(UINT32,GetSize)          ( THIS );
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf );
    STDMETHOD(GetName)                  ( THIS_ REF(IHXBuffer*) pbufName );
    STDMETHOD(SetName)                  ( THIS_ IHXBuffer*      pbufName );
    STDMETHOD_(UINT32,GetType)          ( THIS );
    STDMETHOD(SetType)                  ( THIS_ UINT32 sdptype );
    STDMETHOD(GetValue)                 ( THIS_ REF(IHXBuffer*) pbufValue );
    STDMETHOD(SetValue)                 ( THIS_ IHXBuffer*      pbufValue );
    STDMETHOD(GetIntValue)              ( THIS_ REF(INT32)       iValue );
    STDMETHOD(SetIntValue)              ( THIS_ INT32            iValue );
    STDMETHOD(GetStrValue)              ( THIS_ REF(IHXBuffer*) pbufValue );
    STDMETHOD(SetStrValue)              ( THIS_ IHXBuffer*      pbufValue );
    STDMETHOD(GetBufValue)              ( THIS_ REF(IHXBuffer*) pbufValue );
    STDMETHOD(SetBufValue)              ( THIS_ IHXBuffer*      pbufValue );

    void Get( REF(IHXBuffer*) pbufName, REF(IHXBuffer*) pbufVal );

    FAST_CACHE_MEM

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufAttrib;
    IHXBuffer*     m_pbufName;
    UINT32          m_sdptype;
    union {
        INT32       ival;   // Integer value
        IHXBuffer* bufval; // All others (raw, string, buffer)
    } m_sdpval;
    IHXFastAlloc*  m_pFastAlloc;
};

class CSDPTimeDesc : public IHXSDPTimeDesc
{
public:
    CSDPTimeDesc( IHXFastAlloc* pFastAlloc );
    virtual ~CSDPTimeDesc( void );

    int Init( IHXBuffer* pbufSDP, UINT32& rpos );

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)          ( THIS );
    STDMETHOD_(ULONG32,Release)         ( THIS );

    // IHXSDPTimeDesc
    STDMETHOD_(UINT32,GetSize)          ( THIS );
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf );
    STDMETHOD(GetTime)                  ( THIS_ REF(IHXBuffer*) pbufTime );
    STDMETHOD(SetTime)                  ( THIS_ IHXBuffer*      pbufTime );
    STDMETHOD(GetRepeatList)            ( THIS_ REF(IHXList*)   plistRepeat );
    STDMETHOD(GetRepeatListConst)       ( THIS_ REF(IHXList*)   plistRepeat );

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufTime;
    IHXList*       m_plistRepeat;
    IHXFastAlloc*  m_pFastAlloc;
};

class CSDPMedia : public IHXSDPMedia
{
public:
    CSDPMedia( IHXFastAlloc* pFastAlloc );
    virtual ~CSDPMedia( void );

    int Init( IHXBuffer* pbufSDP, UINT32& rpos );

    FAST_CACHE_MEM

    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)          ( THIS );
    STDMETHOD_(ULONG32,Release)         ( THIS );

    // IHXSDPMedia
    STDMETHOD_(UINT32,GetSize)          ( THIS );
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf );
    STDMETHOD(GetMediaName)             ( THIS_ REF(IHXBuffer*) pbufMediaName );
    STDMETHOD(SetMediaName)             ( THIS_ IHXBuffer*      pbufMediaName );
    STDMETHOD(GetTitle)                 ( THIS_ REF(IHXBuffer*) pbufTitle );
    STDMETHOD(SetTitle)                 ( THIS_ IHXBuffer*      pbufTitle );
    STDMETHOD(GetConnInfo)              ( THIS_ REF(IHXBuffer*) pbufConnInfo );
    STDMETHOD(SetConnInfo)              ( THIS_ IHXBuffer*      pbufConnInfo );
    STDMETHOD(GetBwidInfo)              ( THIS_ REF(IHXBuffer*) pbufBwidInfo );
    STDMETHOD(SetBwidInfo)              ( THIS_ IHXBuffer*      pbufBwidInfo );
    STDMETHOD(GetEncryptKey)            ( THIS_ REF(IHXBuffer*) pbufEncryptKey );
    STDMETHOD(SetEncryptKey)            ( THIS_ IHXBuffer*      pbufEncryptKey );
    STDMETHOD(GetAttribList)            ( THIS_ REF(IHXList*)   plistAttrib );
    STDMETHOD(GetAttribListConst)       ( THIS_ REF(IHXList*)   plistAttrib );

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufMediaName;    // m
    IHXBuffer*     m_pbufTitle;        // i?
    IHXBuffer*     m_pbufConnInfo;     // c?
    IHXBuffer*     m_pbufBwidInfo;     // b?
    IHXBuffer*     m_pbufEncryptKey;   // k?
    IHXList*       m_plistAttrib;      // a*
    IHXFastAlloc*  m_pFastAlloc;
};

class CSDP : public IHXSDP
{
public:
    CSDP( IHXFastAlloc* pFastAlloc );
    virtual ~CSDP( void );

    HX_RESULT Init( IHXBuffer* pbufSDP );

    FAST_CACHE_MEM

protected:

public:
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)          ( THIS );
    STDMETHOD_(ULONG32,Release)         ( THIS );

    // IHXSDP
    STDMETHOD_(UINT32,GetSize)          ( THIS );
    STDMETHOD_(UINT32,Write)            ( BYTE* pbuf );
    STDMETHOD_(IHXBuffer*,AsBuffer)    ( THIS );
    STDMETHOD_(UINT32,GetVersion)       ( THIS );
    STDMETHOD(SetVersion)               ( THIS_ UINT32 ver );
    STDMETHOD(GetOrigin)                ( THIS_ REF(IHXBuffer*) pbufOrigin );
    STDMETHOD(SetOrigin)                ( THIS_ IHXBuffer*      pbufOrigin );
    STDMETHOD(GetName)                  ( THIS_ REF(IHXBuffer*) pbufName );
    STDMETHOD(SetName)                  ( THIS_ IHXBuffer*      pbufName );
    STDMETHOD(GetInfo)                  ( THIS_ REF(IHXBuffer*) pbufInfo );
    STDMETHOD(SetInfo)                  ( THIS_ IHXBuffer*      pbufInfo );
    STDMETHOD(GetUrl)                   ( THIS_ REF(IHXBuffer*) pbufUrl );
    STDMETHOD(SetUrl)                   ( THIS_ IHXBuffer*      pbufUrl );
    STDMETHOD(GetEmail)                 ( THIS_ REF(IHXBuffer*) pbufEmail );
    STDMETHOD(SetEmail)                 ( THIS_ IHXBuffer*      pbufEmail );
    STDMETHOD(GetPhone)                 ( THIS_ REF(IHXBuffer*) pbufPhone );
    STDMETHOD(SetPhone)                 ( THIS_ IHXBuffer*      pbufPhone );
    STDMETHOD(GetConnInfo)              ( THIS_ REF(IHXBuffer*) pbufConnInfo );
    STDMETHOD(SetConnInfo)              ( THIS_ IHXBuffer*      pbufConnInfo );
    STDMETHOD(GetBwidInfo)              ( THIS_ REF(IHXBuffer*) pbufBwidInfo );
    STDMETHOD(SetBwidInfo)              ( THIS_ IHXBuffer*      pbufBwidInfo );
    STDMETHOD(GetTimeDescList)          ( THIS_ REF(IHXList*)   plistTimeDesc );
    STDMETHOD(GetTimeDescListConst)     ( THIS_ REF(IHXList*)   plistTimeDesc );
    STDMETHOD(GetTimeZone)              ( THIS_ REF(IHXBuffer*) pbufTimeZone );
    STDMETHOD(SetTimeZone)              ( THIS_ IHXBuffer*      pbufTimeZone );
    STDMETHOD(GetEncryptKey)            ( THIS_ REF(IHXBuffer*) pbufEncryptKey );
    STDMETHOD(SetEncryptKey)            ( THIS_ IHXBuffer*      pbufEncryptKey );
    STDMETHOD(GetAttribList)            ( THIS_ REF(IHXList*)   plistAttribs );
    STDMETHOD(GetAttribListConst)       ( THIS_ REF(IHXList*)   plistAttribs );
    STDMETHOD(GetMediaList)             ( THIS_ REF(IHXList*)   plistMedia );
    STDMETHOD(GetMediaListConst)        ( THIS_ REF(IHXList*)   plistMedia );

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufSDP;
    UINT32          m_nVersion;         // v
    IHXBuffer*     m_pbufOrigin;       // o
    IHXBuffer*     m_pbufName;         // s
    IHXBuffer*     m_pbufInfo;         // i?
    IHXBuffer*     m_pbufUrl;          // u?
    IHXBuffer*     m_pbufEmail;        // e?
    IHXBuffer*     m_pbufPhone;        // p?
    IHXBuffer*     m_pbufConnInfo;     // c?
    IHXBuffer*     m_pbufBwidInfo;     // b?
    IHXList*       m_plistTimeDesc;    // (tr*)+
    IHXBuffer*     m_pbufTimeZone;     // z?
    IHXBuffer*     m_pbufEncryptKey;   // k?
    IHXList*       m_plistAttrib;      // a*
    IHXList*       m_plistMedia;       // (mi?c?b?k?a*)*
    IHXFastAlloc*  m_pFastAlloc;
};

#endif /* ndef _HXSDPP_H_ */
