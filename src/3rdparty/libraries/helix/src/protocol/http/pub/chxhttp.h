/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxhttp.h,v 1.4 2007/07/13 16:56:03 ping Exp $
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
 * Contributor(s): Stanislav Bobrovskiy
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _CHXHTTP_H_
#define _CHXHTTP_H_

#include "hxhttp.h"
#include "unkimp.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxccf.h"

class CHXHttp : public IHXHttp2,
                public IHXHttpInitialize,
                public CUnknownIMP
{
    // the IUnknown implementation declaration
    DECLARE_UNKNOWN(CHXHttp)

public:
    CHXHttp();
    ~CHXHttp();

    /*
     *	IHXHttpInitialize methods
     */
    
    STDMETHOD(Init)   (THIS_ IUnknown* pContext);
    STDMETHOD(Destroy)(THIS);
    
    /*
     *	IHXHttp methods
     */
    // Client is responsible for supplying in pContext:
    // IHXPreferences - to supply proxy, timeout and HTTP cache preferences.
    // IHXErrorMessages - for error reporting (optional).
    STDMETHOD_(HXBOOL,Initialize)       (THIS_ IUnknown* pContext);
    STDMETHOD_(HXBOOL,Get)		(THIS_ const char* szURL);
    STDMETHOD_(UINT32,GetBufferSize)	(THIS);
    STDMETHOD(SetBufferSize)		(THIS_ UINT32 nBufferSize);
    STDMETHOD(SetConnectionTimeout)	(THIS_ UINT32 nSeconds);
    STDMETHOD(Terminate)		(THIS);

    /*
     *	IHXHttp2 methods
     */
    STDMETHOD(Post) (THIS_ const char* szURL, UINT32 nPostDataSize);
    STDMETHOD(GetFromPosition) (THIS_ const char* szURL, UINT32 nPosition, 
                                const char* pLastModified);

 protected:
    STDMETHOD(SendHTTPRequest)(THIS_ const char* szURL, UINT32 nPostDataSize, 
                               UINT32 nPosition, const char* pLastModified);

    void SendOnGetDone(HXBOOL bSuccess);
    void SendOnDataReceived(IHXBuffer* pBuffer);
    void SendOnHeaders(IHXValues* pHeaders);

    HX_RESULT GetPostData();

 private:
    friend class CHXHttpFileResponse;

    IHXCommonClassFactory* m_pCCF;
    IHXHttpResponse*    m_pIHttpResponse; 
    IHXFileResponse*	m_pFileResponse; 
    HXBOOL		m_bGetDoneSent; 
    IHXRequest*         m_pRequest; 
    IHXFileObject*      m_pFileObject; 
    UINT32		m_ulBufferSize;
    UINT32		m_ulConnTimeout;
    IHXPlugin*          m_pHttpFS;
    IHXHTTPRedirectResponse* m_pRedirectResponse;

    HXBOOL		m_bIsHttpPost;

    // Buffer retrieved from client is stored in case we need to resend it
    IHXBuffer*		m_pPostDataBuffer;

    HXBOOL              m_bInitialized;

    HX_RESULT           m_statResult;
    UINT32		m_nContentLength;
    UINT32              m_nBytesWritten;
    UINT32              m_nTotalBytesToRead;
    HXBOOL              m_bChunkedResponse;
};

class CHXHttpFileResponse : public IHXFileResponse,
                            public IHXFileStatResponse,
                            public IHXHTTPRedirectResponse,
                            public CUnknownIMP
{
 public:
    CHXHttpFileResponse(CHXHttp* pHttp);
    ~CHXHttpFileResponse();
    
    DECLARE_UNKNOWN_NOCREATE(CHXHttpFileResponse)

    /*
     *	IHXFileResponse methods
     */

    STDMETHOD(InitDone)		(THIS_
                                 HX_RESULT	status);

    STDMETHOD(CloseDone)	(THIS_
                                 HX_RESULT	status);

    STDMETHOD(ReadDone)		(THIS_
                                 HX_RESULT	status,
                                 IHXBuffer*	pBuffer);

    STDMETHOD(WriteDone)	(THIS_
                                 HX_RESULT	status);

    STDMETHOD(SeekDone)		(THIS_
                                 HX_RESULT	status);

    STDMETHOD(StatDone)		(THIS_
				 HX_RESULT status,
				 UINT32 ulSize,
				 UINT32 ulCreationTime,
				 UINT32 ulAccessTime,
				 UINT32 ulModificationTime,
				 UINT32 ulMode);

    /*
     *	IHXHTTPRedirectResponse methods
     */
    STDMETHOD(RedirectDone)	(THIS_
				 IHXBuffer* pURL);


 private:
    CHXHttp*	        m_pHttp;
    HXBOOL              m_bReadSucceeded;
};

#endif // _CHXHTTP_H_

