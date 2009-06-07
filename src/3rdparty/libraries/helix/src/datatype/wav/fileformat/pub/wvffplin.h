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

#ifndef _WVFFORM_H_
#define _WVFFORM_H_

#include "riff.h"
#include "riffres.h"

class CWaveFileFormat :	public CHXBaseCountingObject,
                        public IHXPlugin, 
			public IHXFileFormatObject, 
			public CRIFFResponse,
			public IHXPendingStatus,
			public IHXPacketFormat,
			public IHXInterruptSafe
{
private:

    LONG32			m_lRefCount;
    IUnknown*			m_pContext;
    IHXCommonClassFactory*	m_pClassFactory;
    IHXFileObject*	    	m_pFileObject;
    IHXFormatResponse*		m_pFFResponse;
    CRIFFReader*                m_pRiffReader;
    HXBOOL			m_bHeaderSent;
    ULONG32			m_ulCurrentTime;
    IHXRequest*		m_pRequest;

    IHXBuffer*                 m_pFormatBuffer;

    char*			m_pszTitle;
    char*			m_pszAuthor;
    char*			m_pszCopyright;
    UINT32			m_ulThisTACLen;

    UINT32			m_ulBlockAlign;
    UINT32                      m_ulAvgBytesPerSec;
    UINT32			m_ulDataSizeInBytes;
    UINT32                      m_ulPacketSize;
    UINT32                      m_ulBytesSent;

    UINT32                      m_ulAvgBitRate;
    UINT32                      m_ulFormatChunkLen;
    UINT32                      m_ulHeaderOffset;

    IHXPayloadFormatObject*	m_pPayloadFormat;
    HXBOOL			m_bSwapSamples;

    typedef enum
    {
	WS_Ready, 
	WS_InitPending,
	WS_GetStreamHeaderSeekPending, 
	WS_GetStreamHeaderReadPending, 
	WS_GetPacketSeekPending, 
	WS_GetPacketReadPending,
	WS_SeekSeekPending,
	WS_DescendPending,
	WS_FindFmtChunkPending,
	WS_ReadFmtChunkPending,
	WS_FindDataChunkPending,
	WS_FindINFOChunkPending,
	WS_INFODescendPending,
	WS_INFOAscendPending,
	WS_FindINAMChunkPending,
	WS_ReadINAMChunkPending,
	WS_FindIARTChunkPending,
	WS_ReadIARTChunkPending,
	WS_FindICOPChunkPending,
	WS_ReadICOPChunkPending
    }
    WaveState;

    WaveState		m_state;

    typedef enum
    {
	PFMT_RDT,
	PFMT_RTP
    }
    PacketFormat;

    PacketFormat	m_packetFormat;

    static const char*		zm_pDescription;
    static const char*		zm_pCopyright;
    static const char*		zm_pMoreInfoURL;
    
    static const char*		zm_pFileMimeTypes[];
    static const char*		zm_pFileExtensions[];
    static const char*		zm_pFileOpenNames[];

    static const char*		zm_pABMRules[];

    static const char*		zm_pPacketFormats[];

    ~CWaveFileFormat();


    void doPacketSizeCalculations(UINT32 nSamplesPerSec,
				  UINT16 nChannels,
				  UINT16 nBitsPerSample);

public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown**ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload2();

    CWaveFileFormat();

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
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)        /*OUT*/ bLoadMultiple,
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
				REF(const char**) /*OUT*/ pFileOpenNames
				);

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
    //  	IHXFileFormatSession::HeaderCountReady() for the IHXFileFormat-
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

    // CRIFFResponse methods
    STDMETHOD(RIFFOpenDone)(HX_RESULT);
    STDMETHOD(RIFFCloseDone)(HX_RESULT);
    STDMETHOD(RIFFFindChunkDone)(HX_RESULT status, UINT32 len);
    STDMETHOD(RIFFDescendDone)(HX_RESULT);
    STDMETHOD(RIFFAscendDone)(HX_RESULT);
    STDMETHOD(RIFFReadDone)(HX_RESULT, IHXBuffer*);
    STDMETHOD(RIFFSeekDone)(HX_RESULT);
    STDMETHOD(RIFFGetChunkDone)(HX_RESULT, UINT32, IHXBuffer*);

    // IHXPacketFormat methods
    STDMETHOD(GetSupportedPacketFormats)	(THIS_
    						REF(const char**) pFormats);
    STDMETHOD(SetPacketFormat)			(THIS_
    						const char* pFormat);

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
};

#endif // ndef _WVFFORM_H_

