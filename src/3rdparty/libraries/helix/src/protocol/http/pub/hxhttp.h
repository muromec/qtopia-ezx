/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxhttp.h,v 1.2 2005/06/15 02:40:24 rggammon Exp $
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

#ifndef _IHXHTTP_H_
#define _IHXHTTP_H_

#include "hxcom.h"

typedef _INTERFACE IHXValues IHXValues;
typedef _INTERFACE IHXBuffer IHXBuffer;


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXHttp
 *
 *  Purpose:
 *
 *      Implement this interface if your object can do an HTTP get
 *
 *  IID_IHXHttp:
 *
 *      {E7DDB0B0-9846-11d1-A5FE-006097E57C78}
 *
 */

DEFINE_GUID(IID_IHXHttp,
0xe7ddb0b0, 0x9846, 0x11d1, 0xa5, 0xfe, 0x0, 0x60, 0x97, 0xe5, 0x7c, 0x78);

#define CLSID_IHXHttp IID_IHXHttp

#undef INTERFACE
#define INTERFACE IHXHttp

DECLARE_INTERFACE_(IHXHttp, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttp::Initialize
     *  Purpose:
     *      Call to initialize.  Pass in an object that implements the IHXHttpResponse
     *      interface if you are interested in notifications about the current get operation
     *  Returns:
     *      TRUE if successful
     */
    STDMETHOD_(BOOL,Initialize)     (THIS_ IUnknown* pObj) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttp::Get
     *  Purpose:
     *      with the given URL does an http get to retrieve the information
     *  Returns:
     *      TRUE if the Get operation was started
     */
    STDMETHOD_(BOOL,Get)            (THIS_ const char* szURL) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttp::GetBufferSize
     *  Purpose:
     *      Gets the current size of the buffer used
     */
    STDMETHOD_(UINT32,GetBufferSize)            (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttp::SetBufferSize
     *  Purpose:
     *      Sets the buffer size.  If the buffer size is smaller then the length
     *      of the data then OnDataReceived is called multiple times until it is done
     */
    STDMETHOD(SetBufferSize)            (THIS_ UINT32 nBufferSize) PURE;


    /************************************************************************
     *  Method:
     *      IHXHttp::SetConnectionTimeout
     *  Purpose:
     *      Sets the timeout, in seconds, for the http connection.
     */
    STDMETHOD(SetConnectionTimeout)     (THIS_ UINT32 nSeconds) PURE;


    /************************************************************************
     *  Method:
     *      IHXHttp::Terminate
     *  Purpose:
     *      Called to cleanup.  Call this when you are done.  Shouldn't be called
     *      until the response object receives the OnGetDone notification or
     *      if there is no response object make sure you know the get has completed
     */
    STDMETHOD(Terminate)                (THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXHttp2
 *
 *  Purpose:
 *
 *      Extends IHXHttp interface
 *
 *  IID_IHXHttp2:
 *
 *      {E7DDB0B0-9846-11d1-B5FE-006097E57C87}
 *
 */
DEFINE_GUID(IID_IHXHttp2,
0xe7ddb0b0, 0x9846, 0x11d1, 0xb5, 0xfe, 0x0, 0x60, 0x97, 0xe5, 0x7c, 0x87);

#undef INTERFACE
#define INTERFACE IHXHttp2

DECLARE_INTERFACE_(IHXHttp2, IHXHttp)
{
    /************************************************************************
     *  Method:
     *      IHXHttp2::Post
     *  Purpose:
     *      HTTP Post support
     */
    STDMETHOD(Post) (THIS_ const char* szURL, UINT32 nPostDataSize) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttp2::GetFromPosition
     *  Purpose:
     *      Getting part of content starting with position.
     */
    STDMETHOD(GetFromPosition) (THIS_ const char* szURL, UINT32 nPosition, 
                                      const char* pLastModified) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXHttpResponse
 *
 *  Purpose:
 *
 *      Implement this interface if you object wants notifications
 *      about the current http operation
 *
 *  IID_IHXHttpResponse:
 *
 *      {0BDF0BB0-9847-11d1-A5FE-006097E57C78}
 *
 */

DEFINE_GUID(IID_IHXHttpResponse,
0xbdf0bb0, 0x9847, 0x11d1, 0xa5, 0xfe, 0x0, 0x60, 0x97, 0xe5, 0x7c, 0x78);

#undef INTERFACE
#define INTERFACE IHXHttpResponse

DECLARE_INTERFACE_(IHXHttpResponse, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse::OnHeaders
     *  Purpose:
     *      Called when the headers for the http response have been
     *      received and parsed into a IHXValues object
     */
    STDMETHOD(OnHeaders)        (THIS_ IHXValues* pHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse::OnDataReceived
     *  Purpose:
     *      When data comes in from the network this method is called with
     *      a buffer.  The buffer is at most the size set in SetBufferSize
     *      or a default value if not set
     */
    STDMETHOD(OnDataReceived)   (THIS_ IHXBuffer* pBuffer) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse::OnGetDone
     *  Purpose:
     *      Called when the entire get operation has been completed.  bSuccess
     *      is true if everything was successful
     */
    STDMETHOD(OnGetDone)                (THIS_ BOOL bSuccess) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXHttpResponse2
 *
 *  Purpose:
 *
 *      Extends IHXHttpResponse interface
 *
 *  IID_IHXHttpResponse2:
 *
 *      {E7DDB0B0-9846-11d1-B5FE-006095E57D87}
 *
 */

#define DATA_COMPLETE   0
#define DATA_PARTIAL    1

DEFINE_GUID(IID_IHXHttpResponse2,
0xe7ddb0b0, 0x9846, 0x11d1, 0xb5, 0xfe, 0x0, 0x60, 0x95, 0xe5, 0x7d, 0x87);

#undef INTERFACE
#define INTERFACE IHXHttpResponse2

DECLARE_INTERFACE_(IHXHttpResponse2, IHXHttpResponse)
{
    /************************************************************************
     *  Method:
     *      IHXHttpResponse2::GetPostData
     *  Purpose:
     *      Called to retrieve the next available chunk of data to be sent
     *      with HTTP Post.
     */
    STDMETHOD(GetPostData) (THIS_ REF(IHXBuffer*) pBuffer) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse2::InitPost
     *  Purpose:
     *      Called before GetPostData to reset (initialize) to the start of
     *      the HTTP Post data that will be sent.  This may be called more
     *      than once in the case of a redirection.
     */
    STDMETHOD(InitPost) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse2::SetDataType
     *  Purpose:
     *      Notifies about partial (DATA_PARTIAL) or complete (DATA_COMPLETE) 
     *      data arrival (for IHXHttp2::GetFromPosition).
     */
    STDMETHOD(SetDataType) (THIS_ UINT16 nDataType) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpResponse2::SetHeaders
     *  Purpose:
     *      Called to allow the client to set additional HTTP headers for
     *      the request.
     */
    STDMETHOD(SetHeaders) (THIS_ IHXValues* pHeaders) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXHttpInitialize
 *
 *  Purpose:
 *
 *      Implement this interface if your implementation needs a context
 *      for initialization
 *
 *  IID_IHXHttpInitialize:
 *
 *      {083912d6-6c54-4e13-a699-c5b306d761ae}
 *
 */

DEFINE_GUID(IID_IHXHttpInitialize,
0x083912d6, 0x6c54, 0x4e13, 0xa6, 0x99, 0xc5, 0xb3, 0x06, 0xd7, 0x61, 0xae);

#undef INTERFACE
#define INTERFACE IHXHttpInitialize

DECLARE_INTERFACE_(IHXHttpInitialize, IUnknown)
{
    /************************************************************************
     *  Method:
     *      IHXHttpInitialize::Init
     *  Purpose:
     *      Called to initialize the http service. pContext can be a 
     *      helix client engine.
     */
    STDMETHOD(Init)     (THIS_ IUnknown* pContext) PURE;

    /************************************************************************
     *  Method:
     *      IHXHttpInitialize::Destroy
     *  Purpose:
     *      Cleans up the http service -- the destructor will normally
     *      take care of this when all references have been released.
     */
    STDMETHOD(Destroy)  (THIS) PURE;
};
    


#endif //  _IHXHTTP_H_
