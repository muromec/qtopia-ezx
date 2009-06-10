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

#ifndef _AUFFPLIN_H_
#define _AUFFPLIN_H_
#include "baseobj.h"

class CAUFileFormat :public CHXBaseCountingObject,
                     public IHXPlugin,
                     public IHXFileFormatObject,
                     public IHXFileResponse,
                     public IHXFileStatResponse,
                     public IHXPendingStatus,
                     public IHXPacketFormat,
                     public IHXInterruptSafe
{
private:
    void			doPacketCalculations();

    LONG32			m_lRefCount;
    IUnknown*			m_pContext;
    IHXFileObject*		m_pFileObject;
    IHXFileStat*                m_pFileStat;
    IHXFormatResponse*		m_pFFResponse;
    IHXCommonClassFactory*	m_pCommonClassFactory;

    HXBOOL			m_bSwap;
    UINT32			m_ulDataOffset;
    UINT32			m_ulDataLength;
    UINT32                      m_ulFileSize;
    UINT32			m_ulDataFormat;
    UINT32			m_ulRate;
    UINT32			m_ulChannels;
    UINT16			m_ulEncodedBitsPerSample;
    UINT16			m_ulDecodedBitsPerSample;
    HXBOOL			m_bSendOpaqueData;

    HXBOOL			m_bHeaderSent;
    double			m_fTimePerPacket;
    ULONG32			m_ulPacketIdx;
    UINT32			m_ulPacketSize;
    UINT32			m_ulHeaderOffset;
    IHXRequest*		m_pRequest;
    HXBOOL			m_bFirstGetPacket;

    typedef enum
    {
	Ready, InitPending,
	GetFileHeaderSeekPending, GetFileHeaderReadPending,
	GetStreamHeaderSeekPending, GetStreamHeaderReadPending,
	GetPacketSeekPending, GetPacketReadPending,
	SeekSeekPending, GetStreamHeaderStatDonePending
    }
    AUState;

    AUState			m_state;

    typedef enum
    {
        PFMT_RDT,
        PFMT_RTP
    }
    PacketFormat;

    PacketFormat        	m_packetFormat;


    static const char*		zm_pDescription;
    static const char*		zm_pCopyright;
    static const char*		zm_pMoreInfoURL;

    static const char*		zm_pFileMimeTypes[];
    static const char*		zm_pFileExtensions[];
    static const char*		zm_pFileOpenNames[];

    static const char*		zm_pPacketFormats[];


    ~CAUFileFormat();


public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown**ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload2();
    CAUFileFormat();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // *** IHXPlugin methods ***

    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    // *** IHXFileFormatObject methods ***

    /************************************************************************
     *	Method:
     *	    IHXFileFormatObject::GetFileFormatInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of file format
     *	    plugins.
     */
    STDMETHOD(GetFileFormatInfo)
				(THIS_
				REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames);

    /************************************************************************
     *	Method:
     *	    IHXFileFormatObject::InitFileFormat
     *	Purpose:
     *	    Initializes the File Format aspects of the plugin.
     *	    plugins.
     */
    STDMETHOD(InitFileFormat)
			(THIS_
		        IHXRequest*		/*IN*/	pRequest,
			IHXFormatResponse*	/*IN*/	pFormatResponse,
			IHXFileObject*		/*IN*/  pFileObject);

    STDMETHOD(Close)		(THIS);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileFormatObject::GetFileHeader
    //  Purpose:
    //  	Called by controller to ask the file format for the number of
    //  	headers in the file. The file format should call the
    //  	IHXFileFormatSession::FileHeaderReady() for the IHXFileFormat-
    //  	Session object that was passed in during initialization, when the
    //  	header count is available.
    //
    STDMETHOD(GetFileHeader)	(THIS);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileFormatObject::GetStreamHeader
    //  Purpose:
    //  	Called by controller to ask the file format for the header for
    //  	a particular stream in the file. The file format should call
    //  	IHXFileFormatSession::StreamHeaderReady() for the IHXFileFormatSession
    //  	object that was passed in during initialization, when the header
    //  	is available.
    //
    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16		unStreamNumber);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileFormatObject::GetPacket
    //  Purpose:
    //  	Called by controller to ask the file format for the next packet
    //  	for a particular stream in the file. The file format should call
    //  	IHXFileFormatSession::PacketReady() for the IHXFileFormatSession
    //  	object that was passed in during initialization, when the packet
    //  	is available.
    //
    STDMETHOD(GetPacket)	(THIS_
				UINT16		unStreamNumber);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileFormatObject::Seek
    //  Purpose:
    //  	Called by controller to tell the file format to seek to the
    //  	nearest packet to the requested offset. The file format should
    //  	call IHXFileFormatSession::SeekDone() for the IHXFileFormat-
    //  	Session object that was passed in during initialization, when
    //  	the seek has completed.
    //
    STDMETHOD(Seek)		(THIS_
				ULONG32		ulOffset);

    // *** IHXFileResponse methods ***

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //    IHXFileResponse::InitDone
    //  Purpose:
    //    Notification interface provided by users of the IHXFileObject
    //    interface. This method is called by the IHXFileObject when the
    //    initialization of the file is complete, and the Mime type is
    //    available for the request file. If the URL is not valid for the
    //    file system, the status HXR_FAILED should be returned,
    //    with a mime type of NULL. If the URL is valid but the mime type
    //    is unknown, then the status HXR_OK should be returned with
    //    a mime type of NULL.
    //
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT	    status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::CloseDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	close of the file is complete.
    //
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::ReadDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last read from the file is complete and a buffer is available.
    //
    STDMETHOD(ReadDone)		(THIS_
				HX_RESULT	    status,
				IHXBuffer*	    pBuffer);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::WriteDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last write to the file is complete.
    //
    STDMETHOD(WriteDone)	(THIS_
				HX_RESULT	    status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::SeekDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last seek in the file is complete.
    //
    STDMETHOD(SeekDone)		(THIS_
				HX_RESULT	    status);

     /************************************************************************
     *	Method:
     *	    IHXFileResponse::FileObjectReady
     *	Purpose:
     *	    Notification interface provided by users of the IHXFileObject
     *	    interface. This method is called by the IHXFileObject when the
     *	    requested FileObject is ready. It may return NULL with
     *	    PN_STATUS_FAIL if the requested filename did not exist in the
     *	    same pool.
     */
    STDMETHOD(FileObjectReady)		(THIS_
					HX_RESULT	status,
					IHXFileObject* pFileObject);

    // IHXPendingStatus methods

    /************************************************************************
     *	Method:
     *	    IHXPendingStatus::GetStatus
     *	Purpose:
     *	    Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus)	(THIS_
				REF(UINT16) uStatusCode,
				REF(IHXBuffer*) pStatusDesc,
				REF(UINT16) ulPercentDone);


    /* IHXFileStatResponse methods */
    STDMETHOD(StatDone) (THIS_ HX_RESULT status,
                               UINT32    ulSize,
                               UINT32    ulCreationTime,
                               UINT32    ulAccessTime,
                               UINT32    ulModificationTime,
                               UINT32    ulMode);

  /*
     *	IHXInterruptSafe methods
     */

    /************************************************************************
     *	Method:
     *	    IHXInterruptSafe::IsInterruptSafe
     *	Purpose:
     *	    This is the function that will be called to determine if
     *	    interrupt time execution is supported.
     */
    STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS)
    					{ return TRUE; };


    // IHXPacketFormat methods

    STDMETHOD(GetSupportedPacketFormats)	(THIS_
    						REF(const char**) pFormats);
    STDMETHOD(SetPacketFormat)			(THIS_
    						const char* pFormat);
};


#endif /* _AUFFPLIN_H_ */

