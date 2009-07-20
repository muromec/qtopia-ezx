/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrtsp2.h,v 1.10 2006/12/07 15:17:42 jc Exp $
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

#ifndef _HXRTSP2_H_
#define _HXRTSP2_H_

#include "hxcom.h"
#include "hxtypes.h"

class RTSPTransport;

typedef _INTERFACE      IHXTCPSocket            IHXTCPSocket;
typedef _INTERFACE      IHXSocket               IHXSocket;
typedef _INTERFACE      IHXBuffer               IHXBuffer;
typedef _INTERFACE      IHXList                 IHXList;
typedef _INTERFACE      IHXListIterator         IHXListIterator;
typedef _INTERFACE      IHXMIMEParameter        IHXMIMEParameter;
typedef _INTERFACE      IHXMIMEField            IHXMIMEField;
typedef _INTERFACE      IHXMIMEHeader           IHXMIMEHeader;
typedef _INTERFACE      IHXRTSPMessage          IHXRTSPMessage;
typedef _INTERFACE      IHXRTSPRequestMessage   IHXRTSPRequestMessage;
typedef _INTERFACE      IHXRTSPResponseMessage  IHXRTSPResponseMessage;
typedef _INTERFACE      IHXRTSPInterleavedPacket IHXRTSPInterleavedPacket;
typedef _INTERFACE      IHXRTSPConsumer         IHXRTSPConsumer;
typedef _INTERFACE      IHXRTSPProtocolResponse IHXRTSPProtocolResponse;
typedef _INTERFACE      IHXRTSPProtocol         IHXRTSPProtocol;
typedef _INTERFACE      IHXClientStats          IHXClientStats;
typedef _INTERFACE      IHXSessionStats         IHXSessionStats;

/*
 * RTSP message limits:
 *
 *   Max pending bytes: 64k
 *     This is the upper bound on any portion of an RTSP message (command,
 *     header, packet, entity).
 *
 *   Max header size: 64k
 *   Max headers: 100
 *     I thought about restricting header length to something smallor than
 *     the default 64k but that isn't really practical.  Some headers such
 *     as RTP-Info contain urls and so we must allow headers to be as long
 *     as the command line.
 *
 *   Max entity size: 64k
 *     Entities are only used in our system for SDP.  A typical SDP block is
 *     around 4k, so this should leave plenty of room.
 */
#define RTSP_MAX_PENDING_BYTES  (64*1024)
#define RTSP_MAX_COMMAND_SIZE   (64*1024)
#define RTSP_MAX_HEADER_SIZE    (64*1024)
#define RTSP_MAX_HEADER_COUNT   100
#define RTSP_MAX_ENTITY_SIZE    (64*1024)
// No packet limit needed, 64k is physical maximum

enum RTSPMethod
{
      RTSP_UNKNOWN
    , RTSP_ANNOUNCE
    , RTSP_DESCRIBE
    , RTSP_GET_PARAM
    , RTSP_OPTIONS
    , RTSP_PAUSE
    , RTSP_PLAY
    , RTSP_RECORD
    , RTSP_REDIRECT
    , RTSP_SETUP
    , RTSP_SET_PARAM
    , RTSP_TEARDOWN
    , RTSP_PLAYNOW
    , RTSP_EXTENSION
    , RTSP_RESP
};

#define RTSP_VERB_NONE       0
#define RTSP_VERB_ANNOUNCE   1
#define RTSP_VERB_DESCRIBE   2
#define RTSP_VERB_GETPARAM   3
#define RTSP_VERB_OPTIONS    4
#define RTSP_VERB_PAUSE      5
#define RTSP_VERB_PLAY       6
#define RTSP_VERB_RECORD     7
#define RTSP_VERB_REDIRECT   8
#define RTSP_VERB_SETUP      9
#define RTSP_VERB_SETPARAM  10
#define RTSP_VERB_TEARDOWN  11
#define RTSP_VERB_PLAYNOW   12
#define RTSP_VERB_EXTENSION 13

#define RTSP_RES_AGAIN   1
#define RTSP_RES_DONE    2
#define RTSP_RES_PARTIAL 3
#define RTSP_RES_INVALID 4

#define RS_READY    1
#define RS_HDR      2
#define RS_DATA     3
#define RS_FIN      4

/*
 * Simple MIME headers:
 *   "CSeq: 5"
 *   "Content-Type: application/sdp"
 *
 * Non-simple MIME headers:
 *   "Foo:"
 *   "Transport: rdt;cp=6970;mode=play,rtp;cp=6970;mode=play"
 *
 *   In the last example, the fields are "rdt;client_port=6970;mode=play"
 *   and "rtp;client_port=6970;mode=play".  Each field is composed of a
 *   list of parameters.  Each parameter is an attr/value pair.  The value
 *   for any parameter may be NULL (non-existent).  Example:
 *
 *   attr  value
 *   ----  -----
 *   rdt   NULL
 *   cp    6970
 *   mode  play
 */

// IHXMIMEParameter: 8ae57afa-902c-4327-8c00-315785cdc243
DEFINE_GUID(IID_IHXMIMEParameter, 0x8ae57afa, 0x902c, 0x4327,
            0x8c, 0x00, 0x31, 0x57, 0x85, 0xcd, 0xc2, 0x43);

#undef  INTERFACE
#define INTERFACE   IHXMIMEParameter

DECLARE_INTERFACE_(IHXMIMEParameter, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXMIMEParameter
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf) PURE;
    STDMETHOD(Get)                  (THIS_ REF(IHXBuffer*) pbufAttr,
                                           REF(IHXBuffer*) pbufVal) PURE;
    STDMETHOD(Set)                  (THIS_ IHXBuffer* pbufAttr,
                                           IHXBuffer* pbufVal) PURE;
};

/*
 * A MIME field consists of zero or more parameters.  A parameter consists
 * of exactly one attr/value pair.  The value may be NULL (not present) as
 * in "rdt" above.  The parameters may be iterated with the GetFirstParam/
 * GetNextParam methods.
 *
 * Lazy parsing is used for incoming fields.  The first call to
 * GetFirstParam parses the parameters (but does no copying).  All calls to
 * parameter-related methods for incoming fields will fail until
 * GetFirstParam is called.
 *
 * A parameter may be added using InsertParam or AppendParam.  The
 * InsertParam method takes three arguments.  The first is a position
 * marker.  The second and third are the new parameter, which will be
 * inserted after the marker.  If the marker is NULL, it denotes the head of
 * the list.  The "current" parameter is reset to the first parameter.  This
 * method is O(1) if the marker is NULL and O(n) if not.  The AppendParam
 * method appends the new parameter at the tail of the list.  The "current"
 * parameter remains the same.  This method is O(1).
 *
 * A parameter may be removed using RemoveParam or RemoveCurrentParam.  The
 * RemoveParam method takes a pointer to the parameter attribute to be
 * removed.  This method is O(n).  The RemoveCurrentParam method removes the
 * parameter that was last returned in GetFirstParam/GetNextParam.  The new
 * "current" parameter is the next parameter, if any.  This method is O(1).
 *
 * A parameter's value may be changed using SetValue or SetCurrentValue.
 * The SetValue method takes a pointer to the parameter attribute to be
 * changed.  This method is O(n).  The SetCurrentValue method changes the
 * parameter that was last returned in GetFirstParam/GetNextParam.  This
 * method is O(1).
 */
// IHXMIMEField: 0946eed6-0501-4fc3-94bb-3023a0e523c7
DEFINE_GUID(IID_IHXMIMEField, 0x946eed6, 0x0501, 0x4fc3,
            0x94, 0xbb, 0x30, 0x23, 0xa0, 0xe5, 0x23, 0xc7);

#undef  INTERFACE
#define INTERFACE   IHXMIMEField

DECLARE_INTERFACE_(IHXMIMEField, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXMIMEField
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf) PURE;
    STDMETHOD(GetFirstParam)        (THIS_
                                    REF(IHXMIMEParameter*) pParam) PURE;
    STDMETHOD(GetParamList)         (THIS_ REF(IHXList*) plistParam) PURE;
    STDMETHOD(GetParamListConst)    (THIS_ REF(IHXList*) plistParam) PURE;
};

/*
 * A MIME header is a "key: val" pair, eg. "CSeq: 5".  The value consists of
 * zero or more fields. For simple cases, the value is exactly one field
 * with exactly one token and may be retrieved as a specified type with the
 * GetValueAs methods.  For non-simple cases, the value is a list of fields
 * which may be iterated with the GetFirstField/GetNextField methods.
 *
 * Lazy parsing is used for incoming headers.  The first call to
 * GetFirstField parses the fields (but does no copying).  All calls to
 * field-related methods for incoming headers will fail until GetFirstField
 * is called.
 *
 * A field may be added using InsertField or AppendField.  The InsertField
 * method takes two arguments.  The first is a position marker.  The second
 * is the new field, which will be inserted after the marker.  If the marker
 * is NULL, it denotes the head of the list.  The "current" field is reset
 * to the first field.  This method is O(1) if the marker is NULL and O(n)
 * if not.  The AppendField method appends the new field at the tail of the
 * list.  The "current" field remains the same.  This method is O(1).
 *
 * A field may be removed using RemoveField or RemoveCurrentField.  The
 * RemoveField method takes a pointer to the field to be removed.  This
 * method is O(n).  The RemoveCurrentField method removes the field that was
 * last returned in GetFirstField/GetNextField.  The new "current" field is
 * the next field, if any.  This method is O(1).
 */
// IHXMIMEHeader: 97e681a3-bd71-4b81-8fa0-81199e799ae7
DEFINE_GUID(IID_IHXMIMEHeader, 0x97e681a3, 0xbd71, 0x4b81,
            0x8f, 0xa0, 0x81, 0x19, 0x9e, 0x79, 0x9a, 0xe7);

#undef  INTERFACE
#define INTERFACE   IHXMIMEHeader

DECLARE_INTERFACE_(IHXMIMEHeader, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXMIMEHeader
    STDMETHOD_(void,ReplaceDelimiters)(THIS_
                                    HXBOOL bReplaceDelimiters,
                                    int nReplacementDelimiter) PURE;
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf) PURE;
    STDMETHOD(GetKey)               (THIS_ REF(IHXBuffer*)pbufKey) PURE;
    STDMETHOD(SetKey)               (THIS_ IHXBuffer* pbufKey) PURE;
    STDMETHOD(GetFirstField)        (THIS_ REF(IHXMIMEField*) pField) PURE;
    STDMETHOD(GetFieldList)         (THIS_ REF(IHXList*) plistFields) PURE;
    STDMETHOD(GetFieldListConst)    (THIS_ REF(IHXList*) plistFields) PURE;
    STDMETHOD(GetValueAsInt)        (THIS_ REF(INT32) val) PURE;
    STDMETHOD(SetValueFromInt)      (THIS_ INT32 val) PURE;
    STDMETHOD(GetValueAsUint)       (THIS_ REF(UINT32) val) PURE;
    STDMETHOD(SetValueFromUint)     (THIS_ UINT32 val) PURE;
    STDMETHOD(GetValueAsBuffer)     (THIS_ REF(IHXBuffer*)pbufVal) PURE;
    STDMETHOD(SetValueFromBuffer)   (THIS_ IHXBuffer* pbufVal) PURE;

    //XXXTDM: added these for convenience when writing client code.
    //        we should come up with a clean, standardized interface.
    STDMETHOD(SetFromString)        (THIS_ const char* szKey,
                                           const char* szVal) PURE;
    STDMETHOD(SetFromBuffer)        (THIS_ const char* szKey,
                                           IHXBuffer* pbufVal) PURE;
};

// IHXRTSPMessage: 1bff98ab-e5c9-459d-80ee-b80d20e4f30e
DEFINE_GUID(IID_IHXRTSPMessage, 0x1bff98ab, 0xe5c9, 0x459d,
            0x80, 0xee, 0xb8, 0x0d, 0x20, 0xe4, 0xf3, 0x0e);

#undef  INTERFACE
#define INTERFACE   IHXRTSPMessage

DECLARE_INTERFACE_(IHXRTSPMessage, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPMessage
    STDMETHOD_(UINT16,GetVersion)   (THIS) PURE;
    STDMETHOD(SetVersion)           (THIS_ UINT16 ver) PURE;
    STDMETHOD_(int,GetMajorVersion) (THIS) PURE;
    STDMETHOD(SetMajorVersion)      (THIS_ int uMajorVer) PURE;
    STDMETHOD_(int,GetMinorVersion) (THIS) PURE;
    STDMETHOD(SetMinorVersion)      (THIS_ int uMinorVer) PURE;
    STDMETHOD_(UINT32,GetCSeq)      (THIS) PURE;
    STDMETHOD(SetCSeq)              (THIS_ UINT32 cseq) PURE;
    STDMETHOD(GetContent)           (THIS_ REF(IHXBuffer*) pbufContent) PURE;
    STDMETHOD(SetContent)           (THIS_ IHXBuffer* pbuf) PURE;
    STDMETHOD(GetHeader)            (THIS_ const char* key,
                                           REF(IHXMIMEHeader*)pHeader) PURE;
    STDMETHOD(AddHeader)            (THIS_ IHXMIMEHeader* pHeader) PURE;
    STDMETHOD(SetHeader)            (THIS_ IHXMIMEHeader* pHeader) PURE;
    STDMETHOD(RemoveHeader)         (THIS_ const char* key) PURE;
    STDMETHOD(GetHeaderList)        (THIS_ REF(IHXList*) plistHeaders) PURE;
    STDMETHOD(GetHeaderListConst)   (THIS_ REF(IHXList*) plistHeaders) PURE;
    STDMETHOD_(void,ReplaceDelimiters)(THIS_
                                    HXBOOL bReplaceDelimiters,
                                    int nReplacementDelimiter) PURE;
};

// IHXRTSPRequestMessage: ddb0e73f-0d5a-4fd1-bdc8-957f0d872a33
DEFINE_GUID(IID_IHXRTSPRequestMessage, 0xddb0e73f, 0x0d5a, 0x4fd1,
            0xbd, 0xc8, 0x95, 0x7f, 0x0d, 0x87, 0x2a, 0x33);

#undef  INTERFACE
#define INTERFACE   IHXRTSPRequestMessage

DECLARE_INTERFACE_(IHXRTSPRequestMessage, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPRequestMessage
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_
                                    BYTE* pBuf) PURE;
    STDMETHOD_(RTSPMethod,GetMethod)(THIS) PURE;
    STDMETHOD_(UINT32,GetVerb)      (THIS) PURE;
    STDMETHOD(SetVerb)              (THIS_
                                    UINT32 verb) PURE;
    STDMETHOD(GetVerbEx)            (THIS_
                                    REF(IHXBuffer*) pBufVerb) PURE;
    STDMETHOD(SetVerbEx)            (THIS_
                                    IHXBuffer* pBufVerb) PURE;
    STDMETHOD(GetUrl)               (THIS_
                                    REF(IHXBuffer*) pBufUrl) PURE;
    STDMETHOD(SetUrl)               (THIS_
                                    IHXBuffer* pBufUrl) PURE;
};

// IHXRTSPResponseMessage: 876baec2-ec9e-41dc-8cb6-e874b60fbad6
DEFINE_GUID(IID_IHXRTSPResponseMessage, 0x876baec2, 0xec9e, 0x41dc,
            0x8c, 0xb6, 0xe8, 0x74, 0xb6, 0x0f, 0xba, 0xd6);

#undef  INTERFACE
#define INTERFACE   IHXRTSPResponseMessage

DECLARE_INTERFACE_(IHXRTSPResponseMessage, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPResponseMessage
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf) PURE;
    STDMETHOD(GetStatusCode)        (THIS_ REF(UINT32) status) PURE;
    STDMETHOD(SetStatusCode)        (THIS_ UINT32 status) PURE;
};

// IHXRTSPInterleavedPacket: 4d737eff-8218-4762-ace3-fcf27c08f916
DEFINE_GUID(IID_IHXRTSPInterleavedPacket, 0x4d737eff, 0x8218, 0x4762,
            0xac, 0xe3, 0xfc, 0xf2, 0x7c, 0x08, 0xf9, 0x16);

#undef  INTERFACE
#define INTERFACE   IHXRTSPInterleavedPacket

DECLARE_INTERFACE_(IHXRTSPInterleavedPacket, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPInterleavedPacket
    STDMETHOD(Get)                  (THIS_ REF(BYTE) byChan,
                                           REF(IHXBuffer*) pbufData) PURE;
    STDMETHOD(Set)                  (THIS_ BYTE byChan,
                                           IHXBuffer* pbufData) PURE;
};

/*
 * The consumer is an abstract object that the protocol pumps data into.
 * It can be interleaved data, a request message, or a response message.
 * The return value from ReadDone() is one of:
 *   - RTSP_RES_AGAIN  : Data was processed, object is not complete.
 *   - RTSP_RES_DONE   : Data was processed, object is complete.
 *   - RTSP_RES_PARTIAL: Not enough data is available to process.
 *   - RTSP_RES_INVALID: Data was processed, not a valid rtsp message.
 *
 * As the consumer receives data, it creates static buffers that AddRef()
 * the protocol's buffer and refer to chunks within it.  When the consumer
 * is destroyed, it Release()'s the static buffers which in turn Release()
 * the protocol buffer. (see CHXStaticBuffer)
 */
// IHXRTSPConsumer: da62eb99-2120-410a-9866-90f7ec9cc15d
DEFINE_GUID(IID_IHXRTSPConsumer, 0xda62eb99, 0x2120, 0x410a,
            0x98, 0x66, 0x90, 0xf7, 0xec, 0x9c, 0xc1, 0x5d);

#undef  INTERFACE
#define INTERFACE   IHXRTSPConsumer

DECLARE_INTERFACE_(IHXRTSPConsumer, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPConsumer
    STDMETHOD(ReadDone)             (THIS_ IHXBuffer* pbufPacket,
                                           UINT32* ppos) PURE;
    STDMETHOD_(UINT32,GetSize)      (THIS) PURE;
    STDMETHOD_(UINT32,Write)        (THIS_ BYTE* pbuf) PURE;
    STDMETHOD_(UINT32,AsBuffer)     (THIS_ REF(IHXBuffer*) pbuf) PURE;
};

// IHXRTSPProtocolResponse: bf646cd4-922c-4b9c-ac92-96e774de5639
DEFINE_GUID(IID_IHXRTSPProtocolResponse, 0xbf646cd4, 0x922c, 0x4b9c,
            0xac, 0x92, 0x96, 0xe7, 0x74, 0xde, 0x56, 0x39);

#undef  INTERFACE
#define INTERFACE   IHXRTSPProtocolResponse

DECLARE_INTERFACE_(IHXRTSPProtocolResponse, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPProtocolResponse
    STDMETHOD(OnClosed)             (THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnError)              (THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnConnectDone)        (THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnPacket)             (THIS_ IHXRTSPInterleavedPacket* pPkt) PURE;

    STDMETHOD(OnOptionsRequest)     (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnDescribeRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnSetupRequest)       (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnPlayRequest)        (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnPauseRequest)       (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnAnnounceRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnRecordRequest)      (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnTeardownRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnGetParamRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnSetParamRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnRedirectRequest)    (THIS_ IHXRTSPRequestMessage* pMsg) PURE;
    STDMETHOD(OnExtensionRequest)   (THIS_ IHXRTSPRequestMessage* pMsg) PURE;

    STDMETHOD(OnOptionsResponse)    (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnDescribeResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnSetupResponse)      (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnPlayResponse)       (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnPauseResponse)      (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnAnnounceResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnRecordResponse)     (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnTeardownResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnGetParamResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnSetParamResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnRedirectResponse)   (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
    STDMETHOD(OnExtensionResponse)  (THIS_ IHXRTSPResponseMessage* pMsg) PURE;
};

// {605C5422-8F23-4100-851F-00A2A2E9DB29}
DEFINE_GUID(IID_IHXRTSPProtocolValuePass, 0x605c5422, 0x8f23, 0x4100, 
            0x85, 0x1f, 0x0, 0xa2, 0xa2, 0xe9, 0xdb, 0x29);

#undef  INTERFACE
#define INTERFACE   IHXRTSPProtocolValuePass

DECLARE_INTERFACE_(IHXRTSPProtocolValuePass, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(SetValues)   (THIS_
                            IHXValues* pValues) PURE;
};



// IHXRTSPProtocol: 29d8eebf-5597-410b-a290-8181be1e2430
DEFINE_GUID(IID_IHXRTSPProtocol, 0x29d8eebf, 0x5597, 0x410b,
            0xa2, 0x90, 0x81, 0x81, 0xbe, 0x1e, 0x24, 0x30);

#undef  INTERFACE
#define INTERFACE   IHXRTSPProtocol

DECLARE_INTERFACE_(IHXRTSPProtocol, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPProtocol
    STDMETHOD(Init)                 (THIS_ IHXRTSPProtocolResponse* presp,
                                           IUnknown* punkContext) PURE;
    //XXXTDM: The following two methods are for using an RTSPTransport object.
    //        Note that RTSPTransport is not a public class!!!
    STDMETHOD(GetSocket)            (THIS_ REF(IHXSocket*) pSock) PURE;
    STDMETHOD(SetTransport)         (THIS_ BYTE byChan, RTSPTransport* pTran)
                                                                          PURE;
    STDMETHOD(Connect)              (THIS_ const char* szHost,
                                           const char* szPort) PURE;
    STDMETHOD(Accept)               (THIS_ IHXSocket* psock) PURE;
    STDMETHOD(Close)                (THIS) PURE;
    STDMETHOD(SendRequest)          (THIS_ IHXRTSPRequestMessage* pReq) PURE;
    STDMETHOD(SendResponse)         (THIS_ IHXRTSPResponseMessage* pRsp) PURE;
    STDMETHOD(SendPacket)           (THIS_ IHXRTSPInterleavedPacket* pPkt)
                                                                          PURE;
};


// IHXRTSPEvewntsSink: {26A03092-49D6-483a-A12D-4D69E28856D0}
DEFINE_GUID(IID_IHXRTSPEventsSink, 0x26a03092, 0x49d6, 0x483a,
            0xa1, 0x2d, 0x4d, 0x69, 0xe2, 0x88, 0x56, 0xd0);


#undef  INTERFACE
#define INTERFACE   IHXRTSPEventsSink

DECLARE_INTERFACE_(IHXRTSPEventsSink, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)                       (THIS_
                                                     REFIID riid,
                                                     void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)                      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)                     (THIS) PURE;

    // IHXRTSPEventsSink methods

    STDMETHOD(OnRTSPEvents)                         (THIS_
                                                     IHXClientStats* pClientStats,
                                                     IHXSessionStats* pSessionStats,
                                                     IHXBuffer* pRTSPEvents,
                                                     IHXBuffer* pRTSPSessionID) PURE;
};


#undef  INTERFACE
#define INTERFACE   IHXRTSPAggregateEventStats

DECLARE_INTERFACE_(IHXRTSPAggregateEventStats, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPAggregateEventStats methods

    STDMETHOD(UpdateClientRequestCount)     (THIS_ INT32 lCount,
                                                   RTSPMethod ulMethodId) PURE;
    STDMETHOD(UpdateServerRequestCount)     (THIS_ INT32 lCount,
                                                   RTSPMethod ulMethodId) PURE;
    STDMETHOD(UpdateClientResponseCount)    (THIS_ INT32 lCount,
                                                   RTSPMethod ulMethodId,
                                                   UINT32 ulStatusCode) PURE;
    STDMETHOD(UpdateServerResponseCount)    (THIS_ INT32 lCount,
                                                   RTSPMethod ulMethodId,
                                                   UINT32 ulStatusCode) PURE;
};


// IHXRTSPEventsManager {84988F28-9264-46ba-8B5A-B26BD6F16372}

DEFINE_GUID(IID_IHXRTSPEventsManager, 0x84988f28, 0x9264, 0x46ba,
            0x8b, 0x5a, 0xb2, 0x6b, 0xd6, 0xf1, 0x63, 0x72);


#undef  INTERFACE
#define INTERFACE   IHXRTSPEventsManager

DECLARE_INTERFACE_(IHXRTSPEventsManager, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)                       (THIS_
                                                     REFIID riid,
                                                     void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)                      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)                     (THIS) PURE;


    // IHXRTSPEventsManager methods

    STDMETHOD(RegisterSink)                         (THIS_
                                                     IHXRTSPEventsSink* pSink) PURE;

    STDMETHOD(RemoveSink)                           (THIS_
                                                     IHXRTSPEventsSink* pSink) PURE;

    STDMETHOD(OnRTSPEvents)                         (THIS_
                                                     IHXClientStats* pClientStats,
                                                     IHXSessionStats* pSessionStats,
                                                     IHXBuffer* pEvents,
                                                     IHXBuffer* pRTSPSessionID) PURE;

    STDMETHOD_(IHXRTSPAggregateEventStats*, GetAggregateStats) (THIS) PURE;
};


#endif /* _HXRTSP2_H_ */
