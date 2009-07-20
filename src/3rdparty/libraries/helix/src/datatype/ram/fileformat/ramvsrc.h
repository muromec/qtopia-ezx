/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramvsrc.h,v 1.6 2007/07/06 22:01:28 jfinnecy Exp $
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

#ifndef _RAMVSRC_H_
#define _RAMVSRC_H_

class CBigByteGrowingQueue;

class CRAMViewSource :   public IHXFileResponse,
			    public IHXFileStatResponse,
			    public IHXFileViewSource
{
private:
    IHXFileViewSourceResponse* m_pViewSourceResponse;
    SOURCE_TYPE			m_type;
    
    IHXFileObject*		m_pFileObject;
    IUnknown*			m_pContext;
    IHXCommonClassFactory*	m_pCommonClassFactory;
    IHXFragmentedBuffer*	m_pBuffer;
    LONG32			m_lRefCount;
    IUnknown*			m_pContainer;
    IHXValues*			m_pOptions;


    HXBOOL			m_bMangleLinks;
    char*			m_pServerUrl;
    char*			m_pOurPath;
    char*			m_pDefaultView;
    char*			m_pFileName;
    UINT32			m_ulModDate;
    UINT32			m_ulFileSize;

    HX_RESULT PushHeader(CBigByteGrowingQueue* queue);
    HX_RESULT CollectOptions();
    HX_RESULT BuildSource(REF(IHXBuffer*) pOutput);

    HXBOOL PushOpenningHREF(const char* pPositionPointer, CBigByteGrowingQueue* pQueue);
    char* GetParameter(const char* pPositionPointer, UINT32 ulNameLen, HXBOOL bFullPath);
    char* EncryptParameter(char* pPath);
    UINT32 PushMangledDisplayedPath(const char* pIn, CBigByteGrowingQueue* pQueue);
    ~CRAMViewSource();

public:
    CRAMViewSource(IUnknown* pContext, IUnknown* pContainer);

        // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /************************************************************************
     *	Method:
     *	    IHXFileViewSource::InitViewSource
     *	Purpose:
     *	    Called by the user to init before a viewsource.
     */

    STDMETHOD(InitViewSource)	(THIS_
				IHXFileObject* pFileObject,
				IHXFileViewSourceResponse* pResp,
				SOURCE_TYPE sourceType,
				IHXValues* pOptions);

    /************************************************************************
     *	Method:
     *	    IHXFileViewSource::GetHTMLSource
     *	Purpose:
     *	    Called to get source html source.  Return the source
     *	through m_pViewSourceResoponse
     */
    STDMETHOD(GetSource)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXFileViewSource::Close()
     *	Purpose:
     *	    Called to shut things down
     *	Shared with IHXFileFormatObject
     */
    STDMETHOD(Close)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXFileStatResponse::StatDone(...)
     *	Purpose:
     *	    Called in response to our request for a stat.
     */
    STDMETHOD(StatDone)		(THIS_
				 HX_RESULT status,
				 UINT32 ulSize,
				 UINT32 ulCreationTime,
				 UINT32 ulAccessTime,
				 UINT32 ulModificationTime,
				 UINT32 ulMode);

    /************************************************************************
    *  Method:
    *    IHXFileResponse::InitDone
    *  Purpose:
    *    Notification interface provided by users of the IHXFileObject
    *    interface. This method is called by the IHXFileObject when the
    *    initialization of the file is complete, and the Mime type is
    *    available for the request file. If the URL is not valid for the
    *    file system, the status HXR_FAILED should be returned,
    *    with a mime type of NULL. If the URL is valid but the mime type
    *    is unknown, then the status HXR_OK should be returned with
    *    a mime type of NULL.
    */
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT	    status);

    /************************************************************************
    *  Method:
    *  	IHXFileResponse::CloseDone
    *  Purpose:
    *  	Notification interface provided by users of the IHXFileObject
    *  	interface. This method is called by the IHXFileObject when the
    *  	close of the file is complete.
    */
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	status);

    /************************************************************************
    *  Method:
    *  	IHXFileResponse::ReadDone
    *  Purpose:
    *  	Notification interface provided by users of the IHXFileObject
    *  	interface. This method is called by the IHXFileObject when the
    *  	last read from the file is complete and a buffer is available.
    */
    STDMETHOD(ReadDone)		(THIS_ 
				HX_RESULT	    status,
				IHXBuffer*	    pBuffer);

    /************************************************************************
    *  Method:
    *  	IHXFileResponse::WriteDone
    *  Purpose:
    *  	Notification interface provided by users of the IHXFileObject
    *  	interface. This method is called by the IHXFileObject when the
    *  	last write to the file is complete.
    */
    STDMETHOD(WriteDone)	(THIS_ 
				HX_RESULT	    status);

    /************************************************************************
    *  Method:
    *  	IHXFileResponse::SeekDone
    *  Purpose:
    *  	Notification interface provided by users of the IHXFileObject
    *  	interface. This method is called by the IHXFileObject when the
    *  	last seek in the file is complete.
    */
    STDMETHOD(SeekDone)		(THIS_ 
				HX_RESULT	    status);

};

#endif // _RAMVSRC_H_
