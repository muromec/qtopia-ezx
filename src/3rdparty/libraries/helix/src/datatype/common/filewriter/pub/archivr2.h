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
#ifndef _ARCHIVER_H_
#define _ARCHIVER_H_

/****************************************************************************
 *  Defines
 */
#define MAX_VOLUME_GUID_NAME	40


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"

#include "hxcomm.h"
#include "hxerror.h"
#include "hxfwrtr.h"
#include "pckunpck.h"

#include "timeval.h"

#include "flcreatr.h"


class CFileCreator;
class CNGTFileObject;

typedef enum
{
    UNKNOWN_STREAM,
    AUDIO_STREAM,
    VIDEO_STREAM,
    EVENT_STREAM,
    IMAGEMAP_STREAM
} STREAM_TYPE;

typedef enum
{
    AR_UNKNOWN,
    AR_INITIALIZING,
    AR_READY,
    AR_ROTATING
} AR_STATUS;

typedef enum
{
    DIR_INITIALIZING,
    DIR_CREATING,
    DIR_DONE
} DIR_STATUS;

/////////////////////////////////////////////////////////////////////////////
// 
//  Class: CBaseArchiver2
//
//  Description: This abstract base class contains much of the code to
//    perform the management of a live archiving session. More complex
//    classes can inherit from this class in order to share code and
//    extend its functionality to perform particular forms of archiving, 
//    like SureStream archiving or Bandwidth Negotiated archiving.
//
class CBaseArchiver2 : public IHXFileCreatorResponse
{
public:
    CBaseArchiver2(IUnknown* pContext, 
		  IHXFileWriterMonitor* pMonitor, 
		  IHXPropertyAdviser* pAdviser);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID		riid,
	    				void**		ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *  Public API
     */
    HX_RESULT	Init			(IHXRequest* pRequest);
    HX_RESULT	FileHeaderReady		(IHXValues* pHeader);
    HX_RESULT	StreamHeaderReady	(IHXValues* pHeader);
    HX_RESULT	PacketReady		(IHXPacket* pPacket);
    HX_RESULT	Done			();
    HX_RESULT	Abort			(void);

    /*
     *  IHXFileCreatorResponse methods
     */
    STDMETHOD(InitDone)			(HX_RESULT status);
    STDMETHOD(ArchiveDirReady)		(HX_RESULT status);
    STDMETHOD(ArchiveFileReady)		(HX_RESULT status,
					IHXFileObject* pFileObject);
    STDMETHOD(ExistingFileReady)	(HX_RESULT status,
					IHXFileObject* pFileObject);

    static HX_RESULT  ExtractMetaInfo	(IHXValues* pHeader,
					 IHXValues* &pMetaInfo,
					 const char** ppRestrictedFields,
					 ULONG32 ulNumRestrictedFields,
					 IUnknown* pContext);

    static HXBOOL IsSetEmpty		(IHXValues* pValues);

    static ULONG32 BufferStringLength	    (IHXBuffer* pBuffer);
    static IHXBuffer* CreateTrimmedBuffer  (IHXBuffer* pBuffer, IUnknown* pContext);
    static void Hash64ToFileName	    (char* pHash64);
    static HX_RESULT GenGUIDFileName	    (char* pFileName);
    static const char* FileObjectToName	    (IHXFileObject* pFileObject);
    static HX_RESULT UpdateStreamVersion    (IHXValues* pHeader, 
					     UINT8 uMajor,
					     UINT8 uMinor);
					 
    static HX_RESULT ConvertFilepathToURL(const char* szProtocol, const char* szFilePath, CHXString& strUrl);

protected:
    virtual ~CBaseArchiver2();


    LONG32				m_lRefCount;
    AR_STATUS				m_ARStatus;
    HXBOOL				m_bHandlingStoredPackets;
    DIR_STATUS				m_DirStatus;
    IUnknown*				m_pContext;
    IHXFileWriterMonitor*		m_pMonitor;
    IHXPropertyAdviser*			m_pAdviser;
    IHXCommonClassFactory*		m_pClassFactory;
    IHXScheduler*			m_pScheduler;
    IHXErrorMessages*			m_pErrorMessages;
    CFileCreator*			m_pFileCreator;
    HXBOOL				m_bFileCreatorReady;
    IHXValues*				m_pFileHeader;
    IHXValues*				m_pMetaInfo;
    UINT32				m_ulNumStreams;
    UINT32				m_ulActiveStreams;
    CHXSimpleList			m_StreamHeaders;
    CHXString				m_ArchiveDir;
    CHXString				m_BaseName;
    CHXString				m_FileName;
    CHXString				m_ProtocolName;
    CHXString				m_AddOnExtension;
    CHXString				m_TempDirPath;
    INT32				m_iFileTime;
    INT32				m_iFileSize;
    HXBOOL				m_bUseNuggetFormat;
    UINT32				m_ulNuggetMinLocalDuration;
    CNGTFileObject*			m_pNuggetFileObject;
    UINT32				m_ulMaxPreroll;
    UINT32				m_ulMaxDuration;
    HXBOOL				m_bKnownDuration;
    HXBOOL				m_bKnownPreroll;
    HXBOOL				m_bEnforceSaneValues;
    HXBOOL				m_bNeedsRelativeTS;
    HXBOOL				m_bContainerFormatEnabled;
    HXBOOL				m_bIsFullyNative;
    HXBOOL				m_ulPadContentToSize;
    HXBOOL				m_bBlastFiles;
    HXBOOL				m_bRotateAsFallback;
    HXBOOL				m_bTempDirUsed;
    HXBOOL				m_bUseTempFiles;
    HXBOOL				m_bRecordAsLive;
    Timeval				m_StartTime;
    CHXSimpleList			m_StoredPackets;
    HXBOOL				m_bClosing;
    UINT32				m_ulVolumeNo;
    CHXString				m_VolumeName;
    UINT32				m_ulArchiverID;
    UINT32				m_ulDirsNeeded;
    UINT32				m_ulDirsCreated;
    char**				m_ppDirectoryList;
    char				m_pVolumeGUIDName[MAX_VOLUME_GUID_NAME]; /* Flawfinder: ignore */

    virtual HX_RESULT OnInit		    (IHXRequest* pRequest);
    virtual HX_RESULT OnAbort		    (void);
    virtual HX_RESULT OnNewFileHeader	    (IHXValues* pHeader);
    virtual HX_RESULT OnNewMetaInfo	    (IHXValues* pMetaInfo);
    virtual HX_RESULT OnNewStreamHeader	    (IHXValues* pHeader);
    virtual HX_RESULT OnNewPacket	    (IHXPacket* pPacket);
    virtual HX_RESULT ComputeMinLocalDuration();
    virtual HX_RESULT ProcessStreamHeaders  ();
    virtual HX_RESULT CreateFileObjects	    ();
    virtual HX_RESULT CreateArchiveVolumeName();
    virtual HX_RESULT CreateTempVolumeName  (CHXString& strTempFilename);
    virtual HX_RESULT OnDone		    ();
    
    HX_RESULT	      CreateDirObjects	    ();
    HX_RESULT	      CreateDirectoryList   ();
    HX_RESULT	      MakeNuggetFileObject  (IHXFileObject* pFileObject,
					     const char* pFileMimeType,
					     IHXFileObject* &pNuggetFileObject);
    void	      SetNuggetLocalDuration(UINT32 ulNuggetLocalDuration);
    HX_RESULT	      ArchiveDirectoryReady (HX_RESULT status);
    
    HX_RESULT	      BeginPacketFlow	    ();
    virtual HXBOOL      MimeTypeOK	    (const char* pMimeType);
    HXBOOL	      ReadyToArchive	    ();
    void	      HandleStoredPackets   ();
    HX_RESULT	      ExtractFileMetaInfo   (IHXValues* pHeader);

    HX_RESULT	      InitTargetName	    (IHXRequest* pRequest);
    void	      NormalizePath	    (CHXString* pPath);
    HX_RESULT	      UpdateStatusULONG32   (HX_RESULT status,
					     const char* pValueName,
					     ULONG32 ulValue);
    HX_RESULT	      UpdateStatus2ULONG32  (HX_RESULT status,
					     const char* pValueName1,
					     ULONG32 ulValue1,
					     const char* pValueName2,
					     ULONG32 ulValue2);

    HX_RESULT	      DumpValues	    (IHXValues* pValues);

	HX_RESULT NotifyTempFileCreated(const char* pszTempFileName);
};


#endif /* _ARCHIVER_H_ */
