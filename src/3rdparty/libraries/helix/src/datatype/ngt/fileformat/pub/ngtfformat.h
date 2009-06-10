/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ngtfformat.h,v 1.4 2007/07/06 22:01:27 jfinnecy Exp $
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

#ifndef _NGTFFORMAT_H_
#define _NGTFFORMAT_H_

/****************************************************************************
 *  Defines
 */
#define NGTFF_INVALID_TIMESTAMP 0xFFFFFFFF


/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxformt.h"
#include "hxfiles.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxmeta.h"

#include "unkimp.h"
#include "baseobj.h"

class CNGTFormatResponseReceiver;
class CNGTFileObject;


/****************************************************************************
 *  Globals
 */
typedef enum
{
    NUGTFFSourceType_Local,
    NUGTFFSourceType_Remote
} NGTFFSourceType;


/****************************************************************************
 * 
 *  Class:
 *	CNGTFileFormat
 *
 *  Purpose:
 *	Implements RTSP protcol in form of a File Format
 */
class CNGTFileFormat :	public CUnknownIMP,
			public IHXPlugin,
			public IHXFileFormatObject,
			public IHXMetaFileFormatResponse,
			public IHXAdvise,
			public CHXBaseCountingObject
{
    DECLARE_UNKNOWN(CNGTFileFormat)

public:
    /*
     *	Constructor/Destructor
     */
    CNGTFileFormat();


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
     *	IHXMetaFileFormatResponse methods
     */
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT status);

    /*
     *	IHXAdvise methods
     */
    STDMETHOD(Advise)   (THIS_ ULONG32 ulInfo);

    /*
     *	Public methods
     */
    HX_RESULT InitDone(NGTFFSourceType eSourceType, HX_RESULT status);

    HX_RESULT FileHeaderReady(NGTFFSourceType eSourceType, 
			      HX_RESULT status,
			      IHXValues* pHeader);

    HX_RESULT StreamHeaderReady(NGTFFSourceType eSourceType,
				HX_RESULT status,
				IHXValues* pHeader);

    HX_RESULT PacketReady(NGTFFSourceType eSourceType,
			  HX_RESULT status, 
			  IHXPacket* pPacket);

    HX_RESULT SeekDone(NGTFFSourceType eSourceType, HX_RESULT status);

    HX_RESULT StreamDone(NGTFFSourceType eSourceType,
			 UINT16 uStreamNumber);
    
protected:
    typedef enum
    {
	NGTFF_Offline,
	NGTFF_Initializing,
	NGTFF_Ready,
	NGTFF_LocalSeeking,
	NGTFF_RemoteSeeking
    } NGTFileFormatState;

    typedef enum
    {
	NGTFFSTRM_LocalSelected,
	NGTFFSTRM_RemoteSelectedPendingAlignment,
	NGTFFSTRM_RemoteSelected
    } NGTFileFormatStreamState;

    typedef enum
    {
	NGTRS_Offline,
	NGTRS_Connecting,
	NGTRS_Connected,
	NGTRS_Expired,
	NGTRS_Incomaptible,
	NGTRS_Unavailable,
	NGTRS_Error
    } NGTRemoteSourceState;

    class CStreamStatus
    {
    public:
	CStreamStatus(void)
	    : bStreamDone(FALSE)
	    , bStreamPending(FALSE)
	    , bRemoteDone(FALSE)
	    , bLocalPending(FALSE)
	    , bLocalDone(FALSE)
	    , ulLastLocalTS(NGTFF_INVALID_TIMESTAMP)
	    , pRemoteSwitchToPacket(NULL)
	    , pLocalHeader(NULL)
	    , pRemoteHeader(NULL)
	    , eState(NGTFFSTRM_LocalSelected)
	{
	    ;
	}

	~CStreamStatus()
	{
	    HX_RELEASE(pRemoteSwitchToPacket);
	    HX_RELEASE(pLocalHeader);
	    HX_RELEASE(pRemoteHeader);
	}

	void Reset(void)
	{
	    bStreamDone = FALSE;
	    bStreamPending = FALSE;
	    bRemoteDone = FALSE;
	    bLocalPending = FALSE;
	    bLocalDone = FALSE;
	    ulLastLocalTS = NGTFF_INVALID_TIMESTAMP;
	    HX_RELEASE(pRemoteSwitchToPacket);
	    eState = NGTFFSTRM_LocalSelected;
	}

	HXBOOL	    bStreamDone;
	HXBOOL	    bStreamPending;
	HXBOOL	    bRemoteDone;
	HXBOOL	    bLocalPending;
	HXBOOL	    bLocalDone;
	UINT32	    ulLastLocalTS;
	IHXPacket*  pRemoteSwitchToPacket;
	IHXValues*  pLocalHeader;
	IHXValues*  pRemoteHeader;
	NGTFileFormatStreamState eState;
    };


    ~CNGTFileFormat();

    HX_RESULT InitChildFileFormat(IHXFileFormatObject *pFileFormat,
				  IHXRequest* pRequest,
				  IHXFormatResponse* pFormatResponse,
				  IHXFileObject *pFileObject);

    HX_RESULT InitRequest(IHXRequest* &pRequest, 
			  const char *pszFileName,
			  IHXValues* pRequestHeader);

    HX_RESULT CheckFileHeaderCompatibility(void);
    HX_RESULT CheckStreamHeaderCompatibility(void);
    HXBOOL TestStreamHeaderItemMatchBuffer(UINT16 uStrmNumber,
					   const char* pItemName, 
					   HXBOOL bAsString = FALSE);
    HXBOOL TestStreamHeaderItemMatchUINT32(UINT16 uStrmNumber,
					   const char* pItemName);
    HX_RESULT FetchRemoteStreamHeaders(void);
    HX_RESULT StartRemoteSource(void);
    HX_RESULT StartRemotePackets(void);
    HXBOOL AreAllStreamHeadersReceived(HXBOOL bRemote);
    HXBOOL AreAnyStreamsLocal(void);
    void RequestRemoteStreamPacketsFromLocalSource(void);
    void HandleRemoteSourceFailure(NGTRemoteSourceState eNewRemoteSourceState);
    HXBOOL IsRemoteSourceHalthy(void)
    { 
	return ((m_eRemoteSourceState == NGTRS_Connected) ||
		(m_eRemoteSourceState == NGTRS_Connecting));
    }

    static const char* StateToString(NGTFileFormatState eState);
    static const char* StateToString(NGTRemoteSourceState eState);
    static const char* StateToString(NGTFileFormatStreamState eState);
    static const char* StateToString(NGTFFSourceType eState);

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pClassFactory;
    IHXFormatResponse* m_pFFResponse;
    IHXRequest* m_pRequest;
    IHXErrorMessages* m_pErrorMessages;

    IHXFileFormatObject* m_pLocalFileFormat;
    IHXFileFormatObject* m_pRemoteFileFormat;

    CNGTFileObject* m_pNGTFileObject;

    IHXValues* m_pLocalFileHeader;
    IHXValues* m_pRemoteFileHeader;
    UINT16 m_uNumStreams;
    CStreamStatus* m_pStreamStatus;

    NGTFileFormatState m_eState;
    NGTRemoteSourceState m_eRemoteSourceState;
    UINT32 m_ulRemoteFFSeekPending;
    HXBOOL m_bExcludeRemoteSource;
   
    static const char* const zm_pDescription;
    static const char* const zm_pCopyright;
    static const char* const zm_pMoreInfoURL;
    
    static const char* const zm_pFileMimeTypes[];
    static const char* const zm_pFileExtensions[];
    static const char* const zm_pFileOpenNames[];

    static const char* const m_zpNGTFileFormatStateToStringMap[];
    static const char* const m_zpNGTRemoteSourceStateToStringMap[];
    static const char* const m_zpNGTFileFormatStreamStateToStringMap[];
    static const char* const m_zpNGTFFSourceTypeToStringMap[];
};


class CNGTFormatResponseReceiver : public CUnknownIMP,
				   public IHXFormatResponse
{
    DECLARE_UNKNOWN(CNGTFormatResponseReceiver)

public:
    CNGTFormatResponseReceiver(CNGTFileFormat* pNGTFileFormat,
			       NGTFFSourceType eSourceType)
	: m_pNGTFileFormat(pNGTFileFormat)
	, m_eSourceType(eSourceType)
    {
	m_pNGTFileFormat->AddRef();
    }

    /*
     *	IHXFormatObject
     */
    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status)
    {
	return m_pNGTFileFormat->InitDone(m_eSourceType, status);
    }

    STDMETHOD(PacketReady)		(THIS_
					HX_RESULT	status,
					IHXPacket*	pPacket)
    {
	return m_pNGTFileFormat->PacketReady(m_eSourceType, status, pPacket);
    }

    STDMETHOD(SeekDone)			(THIS_
					HX_RESULT	status)
    {
	return m_pNGTFileFormat->SeekDone(m_eSourceType, status);
    }

    STDMETHOD(FileHeaderReady)		(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader)
    {
	return m_pNGTFileFormat->FileHeaderReady(m_eSourceType, status, pHeader);
    }

    STDMETHOD(StreamHeaderReady)	(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader)
    {
	return m_pNGTFileFormat->StreamHeaderReady(m_eSourceType, status, pHeader);
    }

    STDMETHOD(StreamDone)		(THIS_
					UINT16		unStreamNumber)
    {
	return m_pNGTFileFormat->StreamDone(m_eSourceType, unStreamNumber);
    }

private:
    ~CNGTFormatResponseReceiver()
    {
	HX_RELEASE(m_pNGTFileFormat);
    }

    CNGTFileFormat* m_pNGTFileFormat;
    NGTFFSourceType m_eSourceType;
};


#endif	// _NGTFFORMAT_H_

