/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: http.h,v 1.10 2006/10/03 23:19:07 tknox Exp $
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

#ifndef _HTTP_H_
#define _HTTP_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "client.h"
#include "hxprot.h"

#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxauthn.h"
#include "imalloc.h"
#include "player.h"
#include "fsmanager.h"

class HTTP;
class HTTPProtocol;
class CHXPtrArray;

typedef enum
{
    HS_KeepAliveInit,
    HS_GetInit,
    HS_PostInit,
    HS_GetFile,
    HS_PostFile,
    HS_GetFileInit,
    HS_PostFileInit,
    HS_PostFileGetPostFile,
    HS_PostFilePostFileInit,
    HS_PostFileWrite
} HTTPState;

class HTTPFileResponse : public IHXFileResponse,
                         public IHXFileStatResponse,
                         public IHXFileMimeMapperResponse,
                         public IHXThreadSafeMethods
{
public:
    HTTPFileResponse(HTTP* h);
    ~HTTPFileResponse();
    BOOL                            m_bDefunct;

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT           status);
    STDMETHOD(CloseDone)                (THIS_
                                        HX_RESULT           status);
    STDMETHOD(ReadDone)                 (THIS_
                                        HX_RESULT           status,
                                        IHXBuffer*          pBuffer);
    STDMETHOD(WriteDone)                (THIS_
                                        HX_RESULT           status);
    STDMETHOD(SeekDone)                 (THIS_
                                        HX_RESULT           status);
    STDMETHOD(StatDone)                 (THIS_
                                         HX_RESULT status,
                                         UINT32 ulSize,
                                         UINT32 ulCreationTime,
                                         UINT32 ulAccessTime,
                                         UINT32 ulModificationTime,
                                         UINT32 ulFlags);
    STDMETHOD(MimeTypeFound) (THIS_
                              HX_RESULT status,
                              const char* mimeType);

    /*  IHXThreadSafe Methods */
    STDMETHOD_(UINT32,IsThreadSafe)(THIS);

private:
    LONG32                          m_lRefCount;
    HTTP*                           m_pHTTP;
    IHXFileStat*                    m_pFileStat;
    IHXFileMimeMapper*              m_file_mime;
    UINT32                          m_ulSize;
    UINT32                          m_ulCreationTime;
};

class HTTPFileSystemManagerResponse : public IHXFileSystemManagerResponse,
                                      public IHXHTTPRedirectResponse
{
private:
    HTTP*                       m_pHTTP;
    BOOL                        m_bHTTPDone;
    LONG32                      m_lRefCount;

public:
    HTTPFileSystemManagerResponse(HTTP* h)
        { m_lRefCount = 0; m_pHTTP = h; m_bHTTPDone = FALSE;}
    ~HTTPFileSystemManagerResponse()        {}

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(InitDone)         (THIS_ HX_RESULT);
    STDMETHOD(FileObjectReady)  (THIS_ HX_RESULT, IUnknown*);
    STDMETHOD(DirObjectReady)   (THIS_ HX_RESULT, IUnknown*);

    STDMETHOD(RedirectDone)     (THIS_ IHXBuffer* pURL);

    void HTTPDone();

private:
    STDMETHOD_(BOOL, IsAccessAllowed)(char* url, char*** HTTP_paths);
};

class HTTPChallenge : public IHXChallenge
{
private:
    INT32 m_lRefCount;
    HTTP* m_pHTTP;

    ~HTTPChallenge();

public:
    HTTPChallenge(HTTP* pHTTP);

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(SendChallenge)
    (
        IHXChallengeResponse* pIHXChallengeResponseSender,
        IHXRequest* pIHXRequestChallenge
    );
};

class HTTP
{
public:
    HTTP(IHXSocket* pSock, HTTPProtocol* pProtocol, Client* pClient);
    ~HTTP();

    int         flushed_tcp(int flush_count);
    HX_RESULT   http_start(IHXRequest* pRequest,
                           IHXValues* pAuthValues);
    HX_RESULT   http_post(char* url, UINT32 content_len, BOOL bNeedHandler);
    HX_RESULT   http_post_data(char* data, int len);
    HX_RESULT   http_post_done();
    HX_RESULT   InitFileSystem(BOOL bAutoRedirect);
    void        Done();

    HX_RESULT WriteReady(void);


    IHXFileObject*                  m_pFileObject;
    IHXSocket*                      m_pSock;
    HTTPProtocol*                   m_pProtocol;
    Client*                         m_pClient;
    FSManager*                      m_FSManager;
    HTTPFileResponse*               m_pFileResponse;
    int                             m_bDone;
    char*                           m_url;
    char*                           m_content;
    CHXPtrArray*                    m_post_buffer;
    UINT32                          m_post_buffer_pos;
    UINT32                          m_post_buffer_write_pos;
    BOOL                            m_post_received;
    UINT32                          m_content_len;
    UINT32                          m_wrote_len;
    IHXRequest*                     m_pRequest;
    HTTPState                       m_state;
    HTTPFileSystemManagerResponse*  m_pFSMR;
    IHXRequestContext*              m_pRequestContextCurrent;
    IHXChallengeResponse*           m_pChallengeResponseRequester;
    IHXPostDataHandler*             m_pPostHandler;
    BOOL                            m_bNeedHandler;
    IHXBuffer*                      m_pPostDataBuffer;
};

#endif  // _HTTP_H_
