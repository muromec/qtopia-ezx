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

#ifndef _QTFFPLIN_H_
#define _QTFFPLIN_H_

/****************************************************************************
 *  Defines
 */
#define QTFF_RDT_FORMAT	    0
#define QTFF_RTP_FORMAT	    1

#define MAX_QTFORMAT_FLAVOR 0x07
#define MAX_QTFLAVORS	    (MAX_QTFORMAT_FLAVOR + 1)


/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxplugn.h"
#include "hxformt.h"
#include "hxfiles.h"
#include "hxerror.h"
#include "hxasm.h"
#include "iqttrack.h"
#include "fswtchr.h"
#include "atomizer.h"
#include "qtatmmgs.h"
#include "qttrkmgr.h"
#include "qtpacketizerfct.h"
#ifdef QTCONFIG_BFRAG
#include "bfrag.h"
#endif	// QTCONFIG_BFRAG
#ifdef QTCONFIG_BFRAG_FACTORY
#include "bfragfct.h"
#endif	// QTCONFIG_BFRAG_FACTORY
#include "qtffrefcounter.h"
#include "hxprdnld.h"


class CQTOffsetToTimeMapper;


/****************************************************************************
 * 
 *  Class:
 *	CQTFileFormat
 *
 *  Purpose:
 *	Implements Quick Time File Format
 */
class CQTFileFormat :	public IHXPlugin, 
			public IHXFileFormatObject, 
			public IHXFormatResponse,
			public IHXFileResponse,
			public IHXAtomizationCommander,
			public IHXAtomizerResponse,
			public IHXASMSource,
			public IHXPacketFormat,
			public IQTTrackResponse,
			public IHXMediaBytesToMediaDur
{
public:
    /*
     *	Constructor
     */
    CQTFileFormat();

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
				REF(HXBOOL) bLoadMultiple,
				REF(const char*) pDescription,
				REF(const char*) pCopyright,
				REF(const char*) pMoreInfoURL,
				REF(ULONG32) ulVersionNumber
				);

    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    /*
     *	IHXFileFormatObject methods
     */
    STDMETHOD(GetFileFormatInfo)    
				(THIS_
				REF(const char**) pFileMimeTypes,
				REF(const char**) pFileExtensions,
				REF(const char**) pFileOpenNames);

    STDMETHOD(InitFileFormat)	
			(THIS_
		        IHXRequest* pRequest, 
			IHXFormatResponse* pFormatResponse,
			IHXFileObject* pFileObject);

    STDMETHOD(Close)		(THIS);

    STDMETHOD(GetFileHeader)	(THIS);

    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16 unStreamNumber);
    
    STDMETHOD(GetPacket)	(THIS_
				UINT16 unStreamNumber);

    STDMETHOD(Seek)		(THIS_
				ULONG32 ulOffset);

    /*
     *	IHXFileResponse
     */
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT status);

    STDMETHOD(CloseDone)    (THIS_
			    HX_RESULT status);

    STDMETHOD(ReadDone)	    (THIS_ 
		    	    HX_RESULT status,
			    IHXBuffer* pBuffer);

    STDMETHOD(WriteDone)    (THIS_ 
			    HX_RESULT status);

    STDMETHOD(SeekDone)	    (THIS_ 
			    HX_RESULT status);

    /*
     *	IHXFormatResponse methods
     */

   STDMETHOD(PacketReady)		(THIS_
					HX_RESULT	status,
					IHXPacket*	pPacket);

    STDMETHOD(FileHeaderReady)		(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) ;

    STDMETHOD(StreamHeaderReady)	(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) ;

    STDMETHOD(StreamDone)		(THIS_
					UINT16		unStreamNumber) ;

    /*
        These methods are duplicated in IHXFileResponse.			    

        STDMETHOD(InitDone)	    (THIS_
     	        	            HX_RESULT status);
     
        STDMETHOD(SeekDone)	    (THIS_ 
     		                    HX_RESULT status);
     
    */

    /*
     *	IHXAtomizationCommander
     */
    STDMETHOD_(QTAtomizerCmd,GetAtomCommand)	(THIS_
						QTAtomType AtomType,
						CQTAtom* pParent);


    /*
     *	IHXAtomizerResponse
     */
    STDMETHOD(AtomReady)    (THIS_
			    HX_RESULT status,
			    CQTAtom* pRootAtom);

    /*
     *	IHXASMSource methods
     */
    STDMETHOD (Subscribe)   (THIS_
			    UINT16 uStreamNumber,
			    UINT16 uRuleNumber);

    STDMETHOD (Unsubscribe) (THIS_
			    UINT16 uStreamNumber,
			    UINT16 uRuleNumber);

    /*
     *	IHXPacketFormat methods
     */
    STDMETHOD(GetSupportedPacketFormats)    (THIS_
					    REF(const char**) pFormats);

    STDMETHOD(SetPacketFormat)		    (THIS_
					    const char* pFormat);

    /*
     *	IQTTrackResponse Interface
     */
    virtual HX_RESULT PacketReady(UINT16 uStreamNum, 
				  HX_RESULT status, 
				  IHXPacket* pPacket);

    /*
     *  IHXMediaBytesToMediaDur methods
     */        
    STDMETHOD(ConvertFileOffsetToDur)   (THIS_
                                        UINT32 /*IN*/ ulLastReadOffset,
                                        UINT32 /*IN*/ ulCompleteFileSize,
                                        REF(UINT32) /*OUT*/ ulREFDuration);

    STDMETHOD(GetFileDuration)          (THIS_
                                        UINT32 /*IN*/ ulCompleteFileSize,
                                        REF(UINT32) /*OUT*/ ulREFDur);

    /*
     *	Public Accessors
     */
    IUnknown* GetContext(void)	    { return m_pContext; }

    ULONG32 GetPacketFormat(void)   { return m_ulPacketFormat; }

    /*
     *	Public Managers
     */
    CQT_MovieInfo_Manager m_MovieInfo;
    CQTTrackManager m_TrackManager;

#ifdef QTCONFIG_BFRAG_FACTORY
    CBufferFragmentFactory* m_pBufferFragmentFactory;
#endif	// QTCONFIG_BFRAG_FACTORY

protected:
    virtual CQTPacketizerFactory* BuildPacketizerFactory(void);

    virtual HX_RESULT GetSessionIdentity(IHXValues* pHeader,
					 CQT_MovieInfo_Manager* pMovieInfo);

    virtual ~CQTFileFormat(); 

#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)
    HX_RESULT SetPropertyOnHeader(HX_RESULT status, IHXValues* pHeader,
                                  const char* key, const char* value, 
                                  ULONG32 valueLength);

    HX_RESULT Set3GPPAssetInfoOnHeader(HX_RESULT status, IHXValues* pHeader);
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pClassFactory;
    IHXFormatResponse* m_pFFResponse;

private:
    typedef enum
    {
	QTFF_Offline,
	QTFF_Ready,
	QTFF_Error,
	QTFF_Init,
	QTFF_Atomize,
	QTFF_GetPacket,
	QTFF_PrimeCache,
	QTFF_SeekPending
    } QTFileFormatState;

    class CPacketCache
    {
    public:
	CPacketCache(void)
	    : pPacket(NULL)
	    , bPending(FALSE)
	    , bStreamDone(FALSE)    {;}

	~CPacketCache()	{ HX_RELEASE(pPacket); }

	IHXPacket* pPacket;
	HXBOOL	    bPending;
	HXBOOL	    bStreamDone;
    };

    inline UINT16 GetNextPacketStreamNum(void);
    HX_RESULT MakeFileHeader(HX_RESULT status);
    HX_RESULT MakeASMUberRuleBook(IHXValues* pHeader,
				  IHXValues* pParsedSDP);
    HX_RESULT ObtainStreamHeader(UINT16 unStreamNumber,
				 IHXValues* &pHeader);
    HX_RESULT AddMetaInfo(IHXValues* pHeader);
    HX_RESULT ExtractAcceptMetaInfo(IHXBuffer* pRequestedInfoBuffer);
    HX_RESULT HandleFailure(HX_RESULT status);

    HX_RESULT CheckLicense(void);
    void      ReportError(UINT32 ulErrorID, HX_RESULT retVal);
    HX_RESULT GetResourceErrorString(UINT32 ulErrorID, CHXString& rErrorStr);

    void  WarnIfNotHinted(HX_RESULT status, HXBOOL bIgnoreHintTracks);
    HX_RESULT  CreateNewFileFormatObject(const char* purl);
    IHXScheduler* m_pScheduler;

    IHXErrorMessages*	m_pErrorMessages;
    HXBOOL		m_bQTLicensed;
    HXBOOL                m_bMP4Licensed;
    HXBOOL                m_b3GPPRel6Licensed;

    HXBOOL		m_bViewSourceRequest;

    UINT8		m_uFormatFlavor;
    
    IHXRequest* m_pRequest;

    static const char* const zm_pDescription;
    static const char* const zm_pCopyright;
    static const char* const zm_pMoreInfoURL;
    
    static const char* const zm_pFileMimeTypes[];
    static const char* const zm_pFileExtensions[];
    static const char* const zm_pFileOpenNames[];
    static const char* const zm_pPacketFormats[];

    IHXFileSwitcher* m_pFileSwitcher;
    CAtomizer* m_pAtomizer;
    CQTPacketAssembler* m_pPacketAssembler;

    ULONG32 m_ulPendingSeekTime;

    CPacketCache* m_pPacketCache;
    UINT16 m_uNextPacketStreamNum;
    QTFileFormatState m_State;

    HXBOOL m_bUnregulatedStreamDataFlow;

    ULONG32 m_ulPacketFormat;

    LONG32 m_lRefCount;

    //	Values for mask
    typedef enum
    {
	META_INFO_NONE = 0,
	META_INFO_ALL = 1,
	META_INFO_WIDTH = 2,
	META_INFO_HEIGHT = 4
    } eMetaInfoRequest;

    UINT32 m_ulStreamMetaInfoMask;

    // Progressive Download assistance data
    UINT32 m_ulFileDuration;	// in milliseconds
    UINT32 m_bFileStructureEstablished;
    CQTOffsetToTimeMapper* m_pOffsetToTimeMapper;
    IHXFileObject*  m_pFileObject;
    UINT32 m_ulNextPacketTime;	// in milliseconds
    IHXFileFormatObject*   m_pRedirectFFObject;
};


#endif // ndef _QTFFPLIN_H_

