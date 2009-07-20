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
/****************************************************************************

 */

#ifndef _MP4WRTR_H_
#define _MP4WRTR_H_

/****************************************************************************
 *  Defines
 */
#define FILE_HEADER_SET					"FileHeader"
#define STRM_HEADER_SET					"StreamHeader"
#define FWRT_PROPERTY_SET				"Properties"
#define SUPLEMENTED_STRM_HEADER_SET		"SuplementedStreamHeader"


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"

#include "hxfwrtr.h"
#include "hxplugn.h"
#include "hxengin.h"

#include "cstrmsrt.h"
#include "mp4fwplg.h"

class CBaseArchiver2;


/****************************************************************************
 *  Globals
 */
extern INT32 g_nRefCount_mp4wr;


/****************************************************************************
 *  CMP4FileWriter Class
 */
class CMP4FileWriter :	public IHXPlugin, 
			public IHXFileWriter,
			public IHXFileWriterMonitor,
			public IHXPropertyAdviser,
			public IHXCallback
{
public:
    /*
     *	Constructor/Desctructor
     */
    CMP4FileWriter(IUnknown* pContext = NULL);


    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /*
     *	IHXPlugin methods
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(BOOL) bLoadMultiple,
				REF(const char*) pDescription,
				REF(const char*) pCopyright,
				REF(const char*) pMoreInfoURL,
				REF(ULONG32) ulVersionNumber
				);

    STDMETHOD(InitPlugin)	(THIS_
				IUnknown*   /*IN*/  pContext);

    /*
     *	IHXFileWriter methods
     */
    STDMETHOD(GetFileFormatInfo)(THIS_
				REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames
				);

    STDMETHOD(InitFileWriter)	(THIS_
				IHXRequest* pRequest,
				IHXFileWriterMonitor* pMonitor,
				IHXPropertyAdviser* pAdviser
				);

    STDMETHOD(Close)		(THIS);

    STDMETHOD(Abort)		(THIS);

    STDMETHOD(SetFileHeader)	(THIS_
				IHXValues* pHeader
				);

    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader
				);

    STDMETHOD(SetProperties)	(THIS_
				IHXValues* pProperties
				);
    
    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket
				);

    STDMETHOD(StreamDone)	(THIS_
				UINT16 unStreamNumber
				);

    /*
     *	IHXFileWriterMonitor methods
     */
    STDMETHOD(OnStatus)		(THIS_
				HX_RESULT status,
				IHXValues* pInfoList
				);

    STDMETHOD(OnVolumeInitiation)	(THIS_
				HX_RESULT status,
				const char* pFileName,
				REF(ULONG32) ulTag
				);

    STDMETHOD(OnPacketsReady)	(THIS_
				HX_RESULT status
				);

    STDMETHOD(OnVolumeCompletion)	(THIS_
				HX_RESULT status,
				ULONG32 ulTag
				);

    STDMETHOD(OnCompletion)	(THIS_
				HX_RESULT status
				);

    /*
     *	IHXPropertyAdviser methods
     */
    STDMETHOD(GetPropertyULONG32)   (THIS_
				    const char*      pPropertyName,
				    REF(ULONG32)     ulPropertyValue
				    );
    
    STDMETHOD(GetPropertyBuffer)    (THIS_
				    const char*      pPropertyName,
				    REF(IHXBuffer*) pPropertyValue
				    );
    
    STDMETHOD(GetPropertyCString)   (THIS_
				    const char*      pPropertyName,
				    REF(IHXBuffer*) pPropertyValue
				    );
    
    STDMETHOD(GetPropertySet)	    (THIS_
				    const char*	     pPropertySetName,
				    REF(IHXValues*) pPropertySet
				    );

    /*
     *	IHXCallback methods
     */
    STDMETHOD(Func)			(THIS); 

    static const char* zm_pFileMimeTypes[];
    static const char* zm_pFileExtensions[];
    static const char* zm_pFileOpenNames[];

private:
    typedef enum
    {
	Offline,
	PacketsReady,
	Finishing
    } WriterState;

    class FileHeaderInfo
    {
    public:
	FileHeaderInfo()
	    : m_pFileHeader(NULL)
	    , m_bFileHeaderModified(FALSE)
	    , m_bFullyNative(TRUE)
	    , m_bEncrypted(FALSE)
	    , m_bVBR(FALSE)
	{
	    ;
	}
	
	~FileHeaderInfo()
	{
	    HX_RELEASE(m_pFileHeader);
	}
	
	IHXValues* m_pFileHeader;
	BOOL m_bFileHeaderModified;
	BOOL m_bFullyNative;
	BOOL m_bEncrypted;
	BOOL m_bVBR;
    };
    
    class StreamHeaderInfo
    {
    public:
	StreamHeaderInfo()
	    : m_pStreamHeader(NULL)
	    , m_bStreamHeaderModified(FALSE)
	    , m_bIsNativeStream(TRUE)
	    , m_bIsMultiStream(FALSE)
	    , m_bIsEncrypted(FALSE)
	    , m_bIsVBR(FALSE)
	    , m_bStreamDone(FALSE)
	{
	    ;
	}
	
	~StreamHeaderInfo()
	{
	    HX_RELEASE(m_pStreamHeader);
	}
	
	IHXValues* m_pStreamHeader;
	BOOL	    m_bStreamHeaderModified;
	BOOL	    m_bIsNativeStream;
	BOOL	    m_bIsMultiStream;
	BOOL	    m_bIsEncrypted;
	BOOL	    m_bIsVBR;
	BOOL	    m_bStreamDone;
    };

    static const char* zm_pDescription;
    static const char* zm_pCopyright;
    static const char* zm_pMoreInfoURL;

    HX_RESULT TestForMimeSupport( IMP4StreamHandler* pStreamHandler, const char* pszMimeType );

    HX_RESULT CreateAndInitArchiver(void);
    HX_RESULT SuplementStreamHeader(IHXValues* pHeader);

    IUnknown* m_pContext;

    IHXRequest* m_pRequest;
    IHXFileWriterMonitor* m_pMonitor;
    IHXPropertyAdviser* m_pAdviser;
    IMP4StreamHandler** m_ppStreamHandlers;
    IMP4StreamMixer* m_pStreamMixer;
    
    CBaseArchiver2* m_pArchiver;

    IHXValues* m_pProperties;

    FileHeaderInfo m_FileHeaderInfo;
    StreamHeaderInfo* m_pStreamHeaderInfo;

    WriterState m_State;
    BOOL m_bClosing;

    UINT16 m_uStreamCount;
    UINT16 m_uStreamHeaderCount;
    UINT16 m_uStreamDoneCount;

    ULONG32 m_ulPktCount;

    IHXScheduler* m_pScheduler;
    CallbackHandle m_CallbackHandle;

    ULONG32 m_ulVolumeTag;

    UINT32 m_ulFirstTimestamp;
    UINT32 m_ulFirstRTPTimestamp;
    
    LONG32 m_lRefCount;

    ~CMP4FileWriter();
};

#endif /* _MP4WRTR_H_ */
