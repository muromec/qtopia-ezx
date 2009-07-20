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

///////////////////////////////////////////////////////////////////////////////
// Defines
#define   INITGUID     // Interface ID's

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "hlxclib/string.h"     // strrchr
#include "hlxclib/stdlib.h"     // atol
#include "hlxclib/stdio.h"      // sprintf

#include "hxtypes.h"    // Standard Real types
#include "hxcom.h"      // IUnknown
#include "hxcomm.h"     // IHXCommonClassFactory
#include "ihxpckts.h"   // IHXBuffer, IHXPacket, IHXValues
#include "ihxfgbuf.h"
#include "hxplugn.h"    // IHXPlugin
#include "hxfiles.h"    // IHXFileResponse
#include "hxformt.h"    // IHXFileFormatObject
#include "hxcore.h"     // IHXPlayer
#include "hxerror.h"    // IHXErrorMessages
#include "hxmon.h"      // IHXRegistry
#include "defslice.h"   // Registry types
#include "dbcsutil.h"   // Double byte char set functions
#include "hxver.h"
#include "hxupgrd.h"    // IHXUpgradeCollection
#include "hxbuffer.h"
#include "hxprefs.h"
#include "hxtick.h" 
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"   // IHXMediaBytesToMediaDur, IHXPDStatusMgr
#include "hxslist.h"    /* CHXSimpleList */
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS) */

#include "ringbuf.h"

#include "mp3format.h"  // CMp3Format
#include "mp3ff.h"      // CRnMp3Fmt

#include "mhead.h"      // MP3 header info
#include "dxhead.h"     // Xing MP3 header info
#include "pckunpck.h"
#include "metautil.h"
#include "hxtlogutil.h"

#include "hxstrutl.h"

//whether we have libiconv support
#ifdef _UNIX
#ifdef ANDROID
   //Android has no libiconv support
#else
   // normal unix has libiconv
   #define HAS_ICONV 1
#endif 
#endif  //_UNIX

#ifdef HAS_ICONV
#include <iconv.h>
#endif

#define MP3FF_4BYTE_SYSTEM 0x000001ba
#define MP3FF_4BYTE_VIDEO  0x000001b3
#define MP3FF_4BYTE_RIFF   0x52494646
#define MP3FF_4BYTE_CDXA   0x43445841
#define MP3FF_4BYTE_free   0x66726565
#define MP3FF_4BYTE_skip   0x736B6970
#define MP3FF_4BYTE_pnot   0x706e6f74
#define MP3FF_4BYTE_mdat   0x6d646174
#define MP3FF_4BYTE_moov   0x6d6f6f76

//#include "rmfftype.h" // for the HX_SAVE_ENABLED flag

#if defined(HELIX_FEATURE_LOG_STATICDLLACCESS) || defined(_AIX)
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Rnmp3fmt);
#endif


IHXBuffer* NewPacketBufferGetter(
    IHXCommonClassFactory * pClassFactory, // not used for new version
    IHXBuffer * pExistingBuffer,
    UCHAR * pModFrameStart,
    int nModFrameSize
    )
{
    IHXBuffer * pPacketBuffer = new CHXBufferFragment(
        pExistingBuffer,
        pModFrameStart,
        nModFrameSize);
    
    HX_ASSERT(pPacketBuffer);
    HX_ADDREF(pPacketBuffer);
    return pPacketBuffer;
}

IHXBuffer* OldPacketBufferGetter(
    IHXCommonClassFactory * pClassFactory,
    IHXBuffer * pExistingBuffer, // not used for old version
    UCHAR * pModFrameStart, // not used for old version
    int nModFrameSize // not used for old version
    )
{
    IHXBuffer * pPacketBuffer = NULL;
    pClassFactory->CreateInstance(CLSID_IHXBuffer,
                                  (void**)&pPacketBuffer);
    return pPacketBuffer;
}

///////////////////////////////////////////////////////////////////////////////
//  HXCreateInstance                                        ref:  hxplugn.h
//
//  This routine creates a new instance of the CRnMp3Fmt class.
//  It is called when the RMA core application is launched, and whenever
//  an URL associated with this plug-in is opened.

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT
(HXCREATEINSTANCE)(IUnknown** ppExFileFormatObj)
{
    *ppExFileFormatObj = (IUnknown*)(IHXPlugin*)new CRnMp3Fmt();
    if (*ppExFileFormatObj != NULL)
    {
        (*ppExFileFormatObj)->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

/****************************************************************************
 *
 *  Function:
 *
 *  CanUnload2()
 *
 *  Purpose:
 *
 *  Function implemented by all plugin DLL's if it returns HXR_OK
 *  then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK );
}

///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt static variables                    ref:  filefmt1.h
//
//  These variables are passed to the RMA core to provide information about
//  this plug-in. They are required to be static in order to remain valid
//  for the lifetime of the plug-in.

const char* const CRnMp3Fmt::zm_pDescription = MY_DESCRIPTION;
const char* const CRnMp3Fmt::zm_pCopyright   = HXVER_COPYRIGHT;
const char* const CRnMp3Fmt::zm_pMoreInfoURL = HXVER_MOREINFO;
const char* const CRnMp3Fmt::zm_pFileMimeTypes[]  = MY_FILE_MIME_TYPES;
const char* const CRnMp3Fmt::zm_pFileExtensions[] = MY_FILE_EXTENSIONS;
const char* const CRnMp3Fmt::zm_pFileOpenNames[]  = MY_FILE_OPEN_NAMES;
// Leave rtp out until the bug is fixed that makes it always get used.
//const char* CRnMp3Fmt::zm_pPacketFormats[]  = {"rdt", "rtp", NULL};
const char* const CRnMp3Fmt::zm_pPacketFormats[]  = {"rdt", NULL};

///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt::CRnMp3Fmt               ref:  filefmt1.h
//
//  Constructor
//
CRnMp3Fmt::CRnMp3Fmt(void)
    : m_RefCount      (0),
      m_pClassFactory (NULL),
      m_pFileObj      (NULL),
      m_pFileStat(NULL),
      m_pStatus       (NULL),
      m_pError        (NULL),
      m_pRegistry     (NULL),
      m_szPlayerReg   (NULL),
      m_State         (Ready),
      m_ulNextPacketDeliveryTime(0),
      m_ulFileSize(0),
      m_ulMaxSampRate(0),
      m_ulMetaReadSize(0),
      m_ulNextMetaPos((UINT32)-1),
      m_ulMetaLength(0),
      m_ulBytesRead(0),
      m_ulFileOffset(0),
      m_nChannels(0),
      m_dNextPts(0.0),
      m_pFmtBuf(NULL),
      m_pMetaProps(NULL),
      m_pContext(NULL),
      m_pMp3Fmt(NULL),
      m_ulGarbageBytesRead(0),
      m_ulCurrentDuration(0),
      m_ulLastStatTick(0),
      m_bEOF(0),
      m_bRtp(0),
      m_bHasVbrHdr(0),
      m_bSkipVbrHdr(0),
      m_bStreaming(0),
      m_bMetaPacket(0),
      m_bNeedPacket(0),
      m_bCheckBadData(0),
      m_bLive(0),
      m_bLicensed(0),
      m_bClosed(TRUE),
      m_bAcceptMetaInfo(FALSE),
      m_bAllMetaInfo(FALSE),
      m_bIsVBR(FALSE),
      m_bFirstMeta(FALSE),
      m_bFileSizeChanged(FALSE),
      m_bFileDurationChanged(FALSE),
      m_pCreatePacketFunction(NULL)
{
    memset(&m_ReadBuf, 0, sizeof(m_ReadBuf));
    memset(&m_Info, 0, sizeof(m_Info));
    memset(&m_RtpPackets, 0, sizeof(m_RtpPackets));

    m_pMp3Fmt = new CMp3Format;
}


#if defined(MPA_FMT_DRAFT00)

///////////////////////////////////////////////////////////////////////////
// IHXPacketFormat Interface Methods                   ref:  rmaformat.h
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRnMp3Fmt::GetSupportedPacketFormats(REF(const char**) pFormats)
{
    pFormats = (const char**) zm_pPacketFormats;

    return HXR_OK;
}

STDMETHODIMP CRnMp3Fmt::SetPacketFormat(const char* pFormat)
{
    if (!strcmp(pFormat, "rtp"))
        m_bRtp = 1;
    else
        m_bRtp = 0;

    return HXR_OK;
}

#endif /* #if defined(MPA_FMT_DRAFT00) */

///////////////////////////////////////////////////////////////////////////////
//  IHXPlugin::GetPluginInfo                                ref:  hxplugn.h
//
//  This routine returns descriptive information about the plug-in, most
//  of which is used in the About box of the user interface. It is called
//  when the RMA core application is launched.
//
STDMETHODIMP
CRnMp3Fmt::GetPluginInfo(REF(int)         bLoadMultiple,
                         REF(const char*) pDescription,
                         REF(const char*) pCopyright,
                         REF(const char*) pMoreInfoURL,
                         REF(UINT32)      versionNumber)
{
    // File Format plug-ins MUST be able to be multiply instantiated
    bLoadMultiple = TRUE;

    pDescription  = (const char*) zm_pDescription;
    pCopyright    = (const char*) zm_pCopyright;
    pMoreInfoURL  = (const char*) zm_pMoreInfoURL;
    versionNumber = MY_PLUGIN_VERSION;

    return HXR_OK;
}


STDMETHODIMP CRnMp3Fmt::Advise(ULONG32 ulInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    // Returning failure here indicates to the file object that
    // we are the kind of fileformat that will attempt
    // to just keep reading off the end of the file
    // and that we expect that a read we do will fail
    // at some point.

    return retVal;
}

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
///////////////////////////////////////////////////////////////////////////////
//  IHXMediaBytesToMediaDur methods                             ref: hxprdnld.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXMediaBytesToMediaDur::ConvertFileOffsetToDur         ref: hxprdnld.h
//  Purpose:
//      Gets the duration associated with the 
//      
//      With ulLastReadOffset, the FF can match up where its last Read
//      was with what it knows the dur was for that Read and thus
//      better estimate the duration at the given file offset.
//
//      It is possible that the duration is resolved to be indefinite
//      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
//
//      Also, ulFileSize can be HX_PROGDOWNLD_UKNOWN_FILE_SIZE and
//      this method should still try to associate a duration with the
//      bytes up to the last read.
//
//      If this fails (returns HXR_FAIL) then calling object should
//      instead notify its IHXPDStatusMgr that the file size
//      changed but that it doesn't have new dur info.  That way, at
//      least the observer can know (& show) progress is happening.
///
STDMETHODIMP
CRnMp3Fmt::ConvertFileOffsetToDur(UINT32 /*IN*/ ulLastReadOffset,
                                  UINT32 /*IN*/ ulCompleteFileSize,
                                  REF(UINT32) /*OUT*/ ulREFDur)
{
    HX_RESULT hxrslt = HXR_FAIL;
    ulREFDur = HX_PROGDOWNLD_UNKNOWN_DURATION;

    if (HX_PROGDOWNLD_UNKNOWN_FILE_SIZE == ulLastReadOffset)
    {
        hxrslt = HXR_INVALID_PARAMETER;
    }
    else
    {
        // Note: we'll get a really-large ulLastReadOffset at first when the
        // id3 tag is being seeked-for, but the next one will be correct:

        if ((m_Info.ulBitRate>>3) > 0)
        {
            HX_ASSERT(ulLastReadOffset > m_ulFileOffset);
            if (ulLastReadOffset > m_ulFileOffset)
            {
                double dDur = ( (double)(ulLastReadOffset - m_ulFileOffset) /
                                (double)(m_Info.ulBitRate>>3) ) * 1000.0;
                ulREFDur = (UINT32)dDur;
                hxrslt = HXR_OK;
            }
        }
        else // /We don't know our bitrate, thus we haven't parsed our header yet:
        {
            hxrslt = HXR_FAIL;
        }

    }

    return hxrslt;
}

///////////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXMediaBytesToMediaDur::GetFileDuration                ref: hxprdnld.h
//  Purpose:
//      Gets the duration associated with the entire file.
//
//      It is possible that the duration is resolved to be indefinite
//      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
//
//      Callers can pass in HX_PROGDOWNLD_UKNOWN_FILE_SIZE if ulFileSize
//      is not known to them and FF may still be able to determine
//      ulREFDur.  However, if FF can't determine the duration of whole
//      file (e.g., in a SMIL2 file), it returns HXR_FAIL and ulREFDur is
//      set to HX_PROGDOWNLD_UNKNOWN_DURATION.
///
STDMETHODIMP
CRnMp3Fmt::GetFileDuration(UINT32 /*IN*/ ulFileSize,
                           REF(UINT32) /*OUT*/ ulREFDur)
{
    HX_RESULT hxrslt = HXR_OK;

    if (m_Info.ulBitRate>>3 > 0)
    {
        double dDur = 0.0;
        UINT32 ulTotalFileSize = ulFileSize;
        if (HX_PROGDOWNLD_UNKNOWN_FILE_SIZE == ulTotalFileSize)
        {
            ulTotalFileSize = m_ulFileSize;
        }
        dDur = (ulTotalFileSize - m_ulFileOffset) /
            (double)(m_Info.ulBitRate>>3) * 1000.0;
        ulREFDur = (UINT32)dDur;
    }
    else // /We don't know our bitrate, thus we haven't parsed our header yet:
    {
        ulREFDur = m_ulCurrentDuration;
        if (0 == m_ulCurrentDuration)
        {
            ulREFDur = HX_PROGDOWNLD_UNKNOWN_DURATION;
            hxrslt = HXR_FAIL;
        }
    }

    return hxrslt;
} 


#endif /* end #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */




///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::GetFileFormatInfo                  ref:  hxformt.h
//
//  This routine returns crucial information required to associate this
//  plug-in with a given MIME type. This information tells the core which
//  File Format plug-in to use for a particular URL. The routine is called
//  when the RMA core application is launched.
//
STDMETHODIMP
CRnMp3Fmt::GetFileFormatInfo(REF(const char**) pFileMimeTypes,
                             REF(const char**) pFileExtensions,
                             REF(const char**) pFileOpenNames)
{
    pFileMimeTypes  = (const char**) zm_pFileMimeTypes;
    pFileExtensions = (const char**) zm_pFileExtensions;
    pFileOpenNames  = (const char**) zm_pFileOpenNames;

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXPlugin::InitPlugin                                   ref:  hxplugn.h
//
//  This routine performs initialization steps such as determining whether
//  required interfaces are available. It is called when the RMA core
//  application is launched, and whenever an URL associated with this
//  plug-in is opened.
//
STDMETHODIMP
CRnMp3Fmt::InitPlugin(IUnknown* pHXCore)
{
    HX_ENABLE_LOGGING(pHXCore);
    HX_RELEASE(m_pContext);
    m_pContext = pHXCore;
    m_pContext->AddRef();

    pHXCore->QueryInterface(IID_IHXCommonClassFactory,
                            (void**)&m_pClassFactory);
    if (m_pClassFactory == NULL)
        return HXR_NOTIMPL;

    m_bStreaming = 1;
	
    // Check if we are being loaded by the player or ther server.  If
    // the context contains IHXPlayer, we are in the player, else, we
    // are in the server.  This determines whether packet loss is possilbe
    // or not (only possible in the server).  So, if m_bStreaming is
    // set to 1, we must do data reformatting to handle packet loss in the
    // render plugin.
    //
    // Also, check the server license for streaming mpa
    IHXPlayer  *pPlayer = NULL;
    pHXCore->QueryInterface(IID_IHXPlayer, (void**)&pPlayer);

    if (pPlayer)
    {
#if defined(HELIX_FEATURE_REGISTRY)
#if defined(HELIX_FEATURE_STATS)
        // Get the player's registry
        IHXRegistryID *pRegistryId = NULL;
        pPlayer->QueryInterface(IID_IHXRegistryID, (void**)&pRegistryId);

        UINT32 ulPlayerRegistryID = 0;
        if (pRegistryId)
        {
            // Get the player's base registry name
            pRegistryId->GetID(ulPlayerRegistryID);
            HX_RELEASE(pRegistryId);
        }
#endif //HELIX_FEATURE_STATS

		// Get the core's registry
		pHXCore->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
		if (m_pRegistry)
        {
#if defined(HELIX_FEATURE_STATS)            
            if (ulPlayerRegistryID)
            {
                m_pRegistry->GetPropName(ulPlayerRegistryID, m_szPlayerReg);

                m_szPlayerReg->SetSize(m_szPlayerReg->GetSize() + 25);
                strcat((char*)m_szPlayerReg->GetBuffer(), ".Author"); /* Flawfinder: ignore */
            }
#else            
            CHXString   strTemp;
            IHXBuffer* pBuffer = NULL;

            strTemp.Format("%s","Title");
            if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
            {
                m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
                m_pRegistry->AddStr(strTemp,pBuffer);
                HXLOGL1(HXLOG_MP3X, "CRnMp3Fmt::InitPlugin entry for title");
            }
            HX_RELEASE(pBuffer);
            
            strTemp.Format("%s","Author");
            if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
            {
                m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
                m_pRegistry->AddStr(strTemp,pBuffer);
                HXLOGL1(HXLOG_MP3X, "CRnMp3Fmt::InitPlugin entry for author");
            }
            HX_RELEASE(pBuffer);
#endif //HELIX_FEATURE_STATS           
        }
#endif /* #if defined(HELIX_FEATURE_REGISTRY) */

        HX_RELEASE(pPlayer);
        m_bStreaming = 0;
        m_bLicensed = 1;
    }

#if defined (MPA_FMT_DRAFT00) && defined(HELIX_FEATURE_REGISTRY)
    else
    {
        // Query registry interface
        INT32  nLicensed = 0;

        IHXRegistry *pRegistry = NULL;
        pHXCore->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

        if (!pRegistry)
            return HXR_UNEXPECTED;

        // On the Server, check the license section of the registry
        if (HXR_OK != pRegistry->GetIntByName(REGISTRY_REALMPA_ENABLED, nLicensed))
            nLicensed = LICENSE_REALMPA_ENABLED;

        m_bLicensed = (nLicensed) ? (TRUE) : (FALSE);

        HX_RELEASE(pRegistry);
    }
#endif //MPA_FMT_DRAFT00

    pHXCore->QueryInterface(IID_IHXErrorMessages, (void**)&m_pError);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::InitFileFormat                     ref:  hxformt.h
//
//  This routine initializes the file, and stores references to objects used
//  throughout the example. It is called whenever an URL associated with this
//  plug-in is opened.
//
STDMETHODIMP
CRnMp3Fmt::InitFileFormat(IHXRequest*        pRequest,
                          IHXFormatResponse* pFormatResponse,
                          IHXFileObject*     pFileObject)
{
    HXLOGL3(HXLOG_MP3X,"InitFileFormat");
    // The format response object is used to notify RMA core of our status
    m_pStatus = pFormatResponse;
    if (m_pStatus != NULL)
        m_pStatus->AddRef();

#if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO)
    // See if meta info has been requested
    HXBOOL bAcceptMetaInfo = m_bAcceptMetaInfo;
    HXBOOL bAllMetaInfo    = m_bAllMetaInfo;
    CheckMetaInfoRequest(pRequest,
                         m_pClassFactory,
                         bAcceptMetaInfo,
                         bAllMetaInfo,
                         m_pMetaProps);
    m_bAcceptMetaInfo = bAcceptMetaInfo;
    m_bAllMetaInfo    = bAllMetaInfo;
#endif /* #if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO) */

    // The file object is used to handle file I/O
    m_pFileObj = pFileObject;
    if (m_pFileObj != NULL)
    {
        m_pFileObj->AddRef();

        // Initialize and check validity of file. Also, associate the file
        // with a response object which is notified when the file operation is
        // complete.

        m_bClosed = FALSE;
        m_State = InitPending;
        m_pFileObj->Init(HX_FILE_READ | HX_FILE_BINARY, this);  // asynchronous

        // Since this class was designated at the response object, this
        // class's IHXFileResponse::InitDone method will be called when
        // the initialization of the file is complete. (See InitDone).
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXFileResponse::InitDone                               ref:  hxfiles.h
//
//  This routine notifies the RMA core that the initialization of the file is
//  done. It is called automatically when the initialization of the file
//  has completed.
//
STDMETHODIMP
CRnMp3Fmt::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == InitPending)
    {
        HX_RELEASE(m_pFileStat);
        m_pFileObj->QueryInterface(IID_IHXFileStat, (void**)&m_pFileStat);
        if (m_pFileStat)
        {
            m_State = StatDonePending;
            retVal = m_pFileStat->Stat((IHXFileStatResponse*) this);
        }
        else
        {
            retVal = FinishInitDone(status);
        }
    }

    return retVal;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::GetFileHeader                      ref:  hxformt.h
//
//  This routine returns to the RMA core an object containing the file
//  header information. Several routines are actually required to complete
//  the process due to the asynchronous nature of the RMA file system. This
//  method is called by the RMA core after the file has been initialized.
//
STDMETHODIMP
CRnMp3Fmt::GetFileHeader(void)
{
    HXLOGL3(HXLOG_MP3X,"GetFileHeader");
    if (m_State != Ready || m_bClosed)
        return HXR_UNEXPECTED;

    // For local files, look for ID3 v1 tags
    if (m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESS) != HXR_ADVISE_PREFER_LINEAR &&
        m_ulFileSize > ID3HeaderLen)
    {
        m_State = GetId3HeaderSeekPending;

        // Inform the file system that we need an actual seek.  An http seek
        // may download all the data to the seek point which is bad especially
        // on a huge seek.
        m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESSONLY);

        // For some reason, http 1.1 servers on mp3.com do not give us data if
        // we seek to the last 128 bytes. So, fudge this to seek to 129
        // bytes from the end (ID3HeaderLen-1) and adjust the read pointer
        // in ReadDone.
        m_pFileObj->Seek(m_ulFileSize-ID3HeaderLen-BytesBeforeID3Header, FALSE);

        return HXR_OK;
    }

    // Since Seek() is asynchronous we need to note our state in order to
    // correctly respond in SeekDone() when finished.
    m_State = GetFileHeaderSeekPending;

    // Move to the beginning of the header data in the file
    m_pFileObj->Seek(MY_FILE_HEADER_START+m_ulFileOffset, FALSE);

    return HXR_OK;
}

#ifdef HAS_ICONV
static int 
code_convert(const char* from_charset, const char* to_charset, const char* inbuf, size_t inlen, char* outbuf, size_t outlen)  
{  
    iconv_t cd;  
   
    cd = iconv_open(to_charset,from_charset);  
    if (cd == 0)  
    {
        iconv_close(cd);
        return -1;  
    }
    memset(outbuf,0,outlen);
#ifdef __SUNPRO_CC
    if (iconv(cd, (const char* const*)&inbuf, &inlen, &outbuf, &outlen) == (size_t) -1L)   
#else
    if (iconv(cd, (char **)&inbuf, &inlen, &outbuf, &outlen) == (size_t) -1L)   
#endif
    {
        iconv_close(cd);
        return -1;  
    }
    iconv_close(cd);  
    return 0;  
}
#endif

void 
CRnMp3Fmt::try_convert(char*encoding, char* inbuf, IHXBuffer*& pBuffer, int outlen)
{
#ifdef HAS_ICONV 
    int retVal= -1;
    if (!pBuffer)
    {
        CreateSizedBufferCCF(pBuffer, m_pContext, (outlen+1) * 4);
    }

    if (encoding && strlen(encoding))
    {
        HXBOOL br = TRUE;
        while(br && retVal== -1)  
        {
            char* ptr = strchr(encoding, ':');
            int n=0;
            if(!ptr)
            {
                n = strlen(encoding);
                br = FALSE;
            }
            else
            {
                n = ptr-encoding;
            }
            CHXString str(encoding,n);
            retVal = code_convert(str.GetBuffer(0), "utf-8", inbuf, strlen(inbuf), (char*)pBuffer->GetBuffer(), pBuffer->GetSize());  
            encoding = ptr +1;
        }       
    }
    if(retVal == -1)
    {
        code_convert("iso-8859-1", "utf-8", inbuf, strlen(inbuf), (char*)pBuffer->GetBuffer(), pBuffer->GetSize());
    }
    return;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt::MyCreateFileHeaderObj_hr             ref:  filefmt1.h
//
//  This routine creates a "file header" object and passes it off to the
//  RMA core, which in turn transports it to the renderer. This object must
//  contain a property stating the number of streams contained in this file
//  format. Any additional header information read from the file can also be
//  placed in this object. This method is called after the header data from
//  the file has been completely read.
//
HX_RESULT
CRnMp3Fmt::MyCreateFileHeaderObj_hr(HX_RESULT   status,
                                    IHXBuffer* pHeader)
{
    HXLOGL4(HXLOG_MP3X,"MyCreateFileHeaderObj_hr");
    // We are in the process of handling the GetFileHeader() request...
    // See GetFileHeader(), SeekDone(), and ReadDone() for previous steps.
    m_State = Ready;

    // Create new object containing the header data
    IHXValues* pHeaderObj = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pHeaderObj);
    if (pHeaderObj != NULL)
    {
        char pUpgrade[16] = "\0";
        HRESULT hr = HXR_OK;
        INT16 wStreams = InitStream_n(pHeader, &m_Info, hr, pUpgrade, sizeof(pUpgrade));

        // The header proceeding the data is too large for
        // our buffer.  So seek past header and try again.
        if (!wStreams)
        {
            HX_RELEASE(pHeaderObj);
            if (m_ulFileOffset)
            {
                m_State = GetFileHeaderSeekPending;
                m_pFileObj->Seek(MY_FILE_HEADER_START+m_ulFileOffset, FALSE);

                return hr;
            }
            // Server is full
            else
            {
                m_pError->Report(HXLOG_ERR, HXR_NOT_AUTHORIZED, 0,
                                 "The requested server is full.",
                                 MY_MORE_INFO_URL);

                return HXR_NOT_AUTHORIZED;
            }
        }

        // Invalid stream
        else if (wStreams < 0)
        {
            HX_RELEASE(pHeaderObj);
#if defined (MPA_FMT_DRAFT00)
            // Print to common display if we are running on a server
            if (m_bStreaming)
                printf("Unsupported File Type.  The selected file is not a valid MPEG audio file.\n");
#endif //MPA_FMT_DRAFT00


            // Send a player AU request if necessary
            IHXPlayer  *pPlayer = NULL;
            m_pContext->QueryInterface(IID_IHXPlayer, (void**)&pPlayer);

            if (pPlayer && strlen(pUpgrade))
            {
                IHXUpgradeCollection* pUpgradeCol = NULL;
                pPlayer->QueryInterface(IID_IHXUpgradeCollection, (void**)&pUpgradeCol);

                if (pUpgradeCol)
                {
                    IHXBuffer *pText = NULL;

                    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pText);
                    if (!pText)
                        return HXR_FAIL;

                    pText->Set((const unsigned char*)pUpgrade, strlen(pUpgrade)+1);
                    pUpgradeCol->Add(eUT_FileTypeNotSupported, pText, 0,0);

                    HX_RELEASE(pText);
                }

                HX_RELEASE(pUpgradeCol);
            }

            HX_RELEASE(pPlayer);

            m_pStatus->FileHeaderReady(hr, NULL);
            return hr;
        }

        pHeaderObj->SetPropertyULONG32("StreamCount",  1);

        // Disable slider for "live" streams (http streaming)
        if (m_bLive)
            pHeaderObj->SetPropertyULONG32("LiveStream", 1);


        // Set title
        int   nLen = 0;
        char* pszTmp = (char*) m_pMp3Fmt->GetId3Title(nLen);
        IHXBuffer* pBuffer = NULL;
        char* encoding = NULL;
#ifdef _UNIX
        encoding = getenv("HELIX_ID3_TAG_ENCODING");
#endif
        try_convert(encoding, pszTmp, pBuffer, nLen);
        if (pBuffer)
        {
            pszTmp = ( char*) pBuffer->GetBuffer();
        }
        if (nLen)
        {
            SetCStringPropertyCCF(pHeaderObj,
                                  "Title",
                                  (const char*) pszTmp,
                                  m_pContext,
                                  TRUE); // Forces a SetPropertyBuffer()
        }

        // Set artist
        nLen   = 0;
        pszTmp = (char*) m_pMp3Fmt->GetId3Artist(nLen);
        HX_RELEASE(pBuffer);
        try_convert(encoding, pszTmp, pBuffer, nLen);
        if (pBuffer)
        {
            pszTmp = ( char*) pBuffer->GetBuffer();
        }
        if (nLen)
        {
            SetCStringPropertyCCF(pHeaderObj,
                                  "Author",
                                  (const char*) pszTmp,
                                  m_pContext,
                                  TRUE); // Forces a SetPropertyBuffer()
        }

        // Set album
        nLen   = 0;
        pszTmp = (char*) m_pMp3Fmt->GetId3Album(nLen);
        HX_RELEASE(pBuffer);
        try_convert(encoding, pszTmp, pBuffer, nLen);
        if (pBuffer)
        {
            pszTmp = ( char*) pBuffer->GetBuffer();
        }
        if (nLen)
        {
            SetCStringPropertyCCF(pHeaderObj,
                                  "Abstract",
                                  (const char*) pszTmp,
                                  m_pContext,
                                  TRUE); // Forces a SetPropertyBuffer()
        }

        // Set genre
        nLen   = 0;
        pszTmp = (char*) m_pMp3Fmt->GetId3Genre(nLen);
        HX_RELEASE(pBuffer);
        try_convert(encoding, pszTmp, pBuffer, nLen);
        if (pBuffer)
        {
            pszTmp = ( char*) pBuffer->GetBuffer();
        }
        if (nLen)
        {
            SetCStringPropertyCCF(pHeaderObj,
                                  "Genre",
                                  (const char*) pszTmp,
                                  m_pContext,
                                  TRUE); // Forces a SetPropertyBuffer()
        }
        HX_RELEASE(pBuffer);

        // Optional Property: "OpaqueData"
        // Any other user defined data
        //pHeaderObj->SetPropertyBuffer("OpaqueData", pHeader);

        // Move back to the beginning of the file
        m_pFileObj->Seek(MY_FILE_HEADER_START+m_ulFileOffset, FALSE);

        // do not allow recording of MP3 audio at this time
        //pHeaderObj->SetPropertyULONG32("Flags",HX_SAVE_ENABLED);

#if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO)
        // If additional meta info has been requested, then
        // provide it here.
        if (m_bAcceptMetaInfo)
        {
            if (m_bAllMetaInfo)
            {
                SetMetaInfo(pHeaderObj, "SrcCodec");
                SetMetaInfo(pHeaderObj, "SrcBitRate");
                SetMetaInfo(pHeaderObj, "SrcVBREnabled");
                SetMetaInfo(pHeaderObj, "SrcInterleaved");
                SetMetaInfo(pHeaderObj, "SrcSamplesPerSecond");
                SetMetaInfo(pHeaderObj, "SrcBitsPerSample");
                SetMetaInfo(pHeaderObj, "SrcNumChannels");
            }
            else
            {
                if (m_pMetaProps)
                {
                    const char* pszProp = NULL;
                    UINT32      ulTmp   = 0;
                    HX_RESULT rv = m_pMetaProps->GetFirstPropertyULONG32(pszProp, ulTmp);
                    while (SUCCEEDED(rv))
                    {
                        SetMetaInfo(pHeaderObj, pszProp);
                        rv = m_pMetaProps->GetNextPropertyULONG32(pszProp, ulTmp);
                    }

                }

            }
        }
#endif /* #if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO) */

        // Notify the RMA core that header object is ready
        m_pStatus->FileHeaderReady(status, pHeaderObj);

        // Release the object since we are done with it
        HX_RELEASE(pHeaderObj);
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::GetStreamHeader                    ref:  hxformt.h
//
//  This routine returns to the RMA core an object containing the stream
//  header information for a particular stream. Several routines are actually
//  required to complete the process due to the asynchronous nature of the
//  RMA file system. This method is called (after the file header has been
//  read) by the RMA core for each stream in the file format.
//
STDMETHODIMP CRnMp3Fmt::GetStreamHeader(UINT16 streamNo)
{
    HXLOGL3(HXLOG_MP3X,"GetStreamHeader(%u)", streamNo);
    if ((m_State != Ready) || (streamNo != MY_STREAM_NO) || m_bClosed)
        return HXR_UNEXPECTED;

    if (eHXUnknown == m_Info.eType)
        return HXR_INVALID_FILE;

#if defined(HELIX_FEATURE_PROGDOWN)
    if (m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESS) == HXR_ADVISE_PREFER_LINEAR &&
        m_pFileStat)
    {
        // If we have a linear filesystem, then it could be
        // getting longer as we read which would increase our duration.
        // So let's try checking the filesize once before we create the
        // stream header, which contains the duration
        m_State = GetStreamHeaderStatDonePending;
        m_pFileStat->Stat((IHXFileStatResponse*) this);
    }
    else
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */
    {
        m_State = GetStreamHeaderSeekPending;
        MyCreateStreamHeaderObj_v(HX_STATUS_OK, NULL);
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt::MyCreateStreamHeaderObj_v            ref:  filefmt1.h
//
//  This routine creates a "stream header" object and passes it off to the
//  RMA core, which in turn transports it to the renderer. This object must
//  contain certain properties required to characterize the stream. Any
//  additional stream information read from the file can also be placed in
//  this object. This method is called after the stream header data from the
//  file has been completely read.
void
CRnMp3Fmt::MyCreateStreamHeaderObj_v(HX_RESULT   status,
                                     IHXBuffer* pStreamHeader)
{
    m_State = Ready;

    // Create new object containing the stream header data
    IHXValues* pStreamHeaderObj = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXValues,
                                    (void**)&pStreamHeaderObj);
    if (pStreamHeaderObj != NULL)
    {
        // REQUIRED Properties:
        pStreamHeaderObj->SetPropertyULONG32("StreamNumber", MY_STREAM_NO);
        pStreamHeaderObj->SetPropertyULONG32("AvgBitRate", m_Info.ulBitRate);
        //retain SampleRate and NumChannels for backwards compatibility
        pStreamHeaderObj->SetPropertyULONG32("SampleRate", m_ulMaxSampRate);
        pStreamHeaderObj->SetPropertyULONG32("SamplesPerSecond", m_ulMaxSampRate);
        pStreamHeaderObj->SetPropertyULONG32("NumChannels", m_nChannels);
        pStreamHeaderObj->SetPropertyULONG32("Channels", m_nChannels);

        UINT32 ulPreRoll = 1000;
        if (m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESS) == HXR_ADVISE_PREFER_LINEAR)
            ulPreRoll += 2000;

        pStreamHeaderObj->SetPropertyULONG32("Preroll", ulPreRoll);

        m_Info.dPacketSize = HX_MIN(m_Info.dPacketSize, kReadSize);

        double dDur;
        dDur = (m_ulFileSize - m_ulFileOffset) / (double)(m_Info.ulBitRate>>3) * 1000.0;
        m_ulCurrentDuration = (UINT32) dDur;
        HXLOGL3(HXLOG_MP3X, "MyCreateStreamHeaderObj_v() Duration=%lu", m_ulCurrentDuration);

        pStreamHeaderObj->SetPropertyULONG32("Duration", m_ulCurrentDuration);
        pStreamHeaderObj->SetPropertyULONG32("MaxPacketSize", 1024);
        pStreamHeaderObj->SetPropertyULONG32("AvgPacketSize", (ULONG32)m_Info.dPacketSize);

        pStreamHeaderObj->SetPropertyULONG32("StartTime", 0);

#if defined (MPA_FMT_DRAFT00)
        // MPA is 14 Dynamic is > 96
        if (m_bRtp)
        {
            pStreamHeaderObj->SetPropertyULONG32("RTPPayloadType", 14);
            pStreamHeaderObj->SetPropertyULONG32("RTPTimestampConversionFactor", 90);
            pStreamHeaderObj->SetPropertyULONG32("HXTimestampConversionFactor", 1);
        }
#endif //MPA_FMT_DRAFT00

        // "MimeType": this stream's MIME type. This associates this
        // stream type with a particular renderer.
        const char* pszTmp = MY_LOCAL_MIME_TYPE;
#if defined (MPA_FMT_DRAFT00)
        if (m_bStreaming)
        {
            if(m_bRtp)
                pszTmp = MY_RTP_MIME_TYPE;
            else
                pszTmp = MY_STREAM_MIME_TYPE;
        }
#endif //MPA_FMT_DRAFT00
        SetCStringPropertyCCF(pStreamHeaderObj, "MimeType", pszTmp, m_pContext);

        // Set the ASM rule book
        char* pRuleBook = new char[sizeof(char) * 100];
        if (pRuleBook)
        {
            // Create the string
            if (m_bRtp)
            {
                SafeSprintf(pRuleBook, 100,
                            "marker=0, AverageBandwidth=%ld, Priority=9, "
                            "timestampdelivery=true;",
                            m_Info.ulBitRate);
            }
            else
            {
                SafeSprintf(pRuleBook, 100,
                            "AverageBandwidth=%ld, AverageBandwidthStd=0, "
                            "Priority=9;",
                            m_Info.ulBitRate);
            }
            // Set it into the stream header
            SetCStringPropertyCCF(pStreamHeaderObj, "ASMRuleBook",
                                  (const char*) pRuleBook, m_pContext);
        }
        HX_VECTOR_DELETE(pRuleBook);

#if defined(HELIX_FEATURE_SERVER)
        if (!m_bLicensed)
        {
            if (m_pError)
            {
                m_pError->Report(HXLOG_ALERT, HXR_NOT_LICENSED,
                                 0, "This server is NOT licensed to deliver MPEG Audio "
                                 "streams. A Player attempting to access MPEG Audio content "
                                 "has been disconnected. Please contact RealNetworks to "
                                 "obtain a license for this feature.\n", NULL);
            }

            status = HXR_NOT_LICENSED;
        }
#endif /* #if defined(HELIX_FEATURE_SERVER) */

        // Notify the RMA core that stream header object is ready
        m_pStatus->StreamHeaderReady(status, pStreamHeaderObj);

        // Release the object since we are done with it
        HX_RELEASE(pStreamHeaderObj);
    }
}

///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::GetPacket                          ref:  hxformt.h
//
//  This routine returns to the RMA core an object containing the packet
//  data for a particular stream. Several routines are actually required to
//  complete the process due to the asynchronous nature of the RMA file
//  system. This method is called by the RMA core each time it needs another
//  packet.
//
STDMETHODIMP
CRnMp3Fmt::GetPacket(UINT16 streamNo)
{
    HXLOGL4(HXLOG_MP3X,"GetPacket(%u)", streamNo);
    // with asyncfsys it is possible that a GetPacket() call is outstanding
    // even after Close() has been called. so check for m_bClosed for that
    // case (b'cuz m_pFileObj is released inside Close())
    if (m_bClosed || (m_State != Ready) || (streamNo != MY_STREAM_NO))
        return HXR_UNEXPECTED;

    HX_RESULT retVal  = HXR_OK;
    HXBOOL      bFinish = TRUE;
#if defined(HELIX_FEATURE_PROGDOWN)
    // Is the filesystem linear?
    if (m_pFileStat &&
        (m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESS) == HXR_ADVISE_PREFER_LINEAR ||
         m_bFileSizeChanged))
    {
        UINT32 ulTick = HX_GET_BETTERTICKCOUNT();
        if (ulTick - m_ulLastStatTick > PROGDOWN_CHECK_INTERVAL)
        {
            HXLOGL4(HXLOG_MP3X, "\tStatting the file", ulTick);
            // Update the last tick time
            m_ulLastStatTick = ulTick;
            // Clear the flag that says to call FinishGetPacket()
            bFinish = FALSE;
            // Set the state
            m_State = GetPacketStatDonePending;
            // Stat the file
            retVal = m_pFileStat->Stat((IHXFileStatResponse*) this);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

    if (bFinish)
    {
        retVal = FinishGetPacket();
    }

    return retVal;
}


///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt::MyCreatePacketObj_hr                 ref:  filefmt1.h
//
//  This routine creates a packet object and passes it off to the RMA core,
//  which in turn transports it to the renderer. The object contains the
//  packet data read from the file along with the time when it should be
//  delivered to the renderer. If there are no more packets available for
//  this stream, the RMA core is notified. This method is called after the
//  packet data from the file has been read.
//
HX_RESULT
CRnMp3Fmt::MyCreatePacketObj_hr(HX_RESULT   status,
                                tReadBuffer* pPacketData)
{
    HXLOGL4(HXLOG_MP3X,"MyCreatePacketObj_hr");
    m_State = Ready;

    HX_ASSERT(!m_bClosed);
    if (status == HX_STATUS_OK)
    {
        // Create new object containing the packet data
        IHXPacket* pPacketObj = NULL;
        m_pClassFactory->CreateInstance(CLSID_IHXPacket,
                                        (void**)&pPacketObj);
        if (pPacketObj != NULL)
        {
            // Fill in the packet
            UINT32 deliveryTime = m_ulNextPacketDeliveryTime;
            UINT16 streamNo     = MY_STREAM_NO;
            UINT8  ASMFlags     = HX_ASM_SWITCH_ON | HX_ASM_SWITCH_OFF;
            UINT16 ASMRuleNo    = 0;

            IHXBuffer* pPacketBuffer = NULL;
#if defined (MPA_FMT_DRAFT00)
          TOP:
#endif /* #if defined (MPA_FMT_DRAFT00) */
            int     nSyncSize = 0;
            UCHAR   *pModFrameStart = NULL;

            // Extract an audio frame from the read buffer
            pModFrameStart = GetMP3Frame_p(pPacketData, nSyncSize);

            // Handle read buffer wraps
            if (!pModFrameStart)
            {
                UCHAR   bNoData = m_bEOF;

                // Check for mp3 streams w/ garbage on the end (non-SureStream)
                // If the next 2 bytes are not a sync word, stop playback
                if (pPacketData->dwBytesRemaining >= 2)
                {
                    if (pPacketData->dwBufferSize == pPacketData->dwBytesRemaining ||
                        pPacketData->pBuffer[0] != 0xFF ||
                        !m_pMp3Fmt->IsValidSyncWord(pPacketData->pBuffer[1]))
                    {
                        // The rest of this buffer does not contain
                        // a syncword, so skip the buffer
                        m_bCheckBadData = 1;
                    }
                    else
                    {
                        // We have a syncword, but just not
                        // enough data in the buffer to complete the
                        // frame. So this is NOT bad data, just another
                        // read is necessary
                        m_bCheckBadData = 0;
                    }
                }

                if (bNoData)
                {
#if defined (MPA_FMT_DRAFT00)
                    // Check for last frame when streaming - we buffer one frame
                    if (m_bStreaming && !m_bRtp)
                    {
                        UINT32 ulTemp = 0;
                        pModFrameStart = m_pFmtBuf->GetReadPointer(ulTemp);

                        bNoData = (ulTemp == 0);
                    }
#endif //MPA_FMT_DRAFT00

                    if (bNoData)
                    {

                        HX_RELEASE(pPacketData->pReadBuffer);
                        pPacketData->dwBytesRemaining = 0;

                        m_bEOF = TRUE;
                        m_pStatus->StreamDone(0);

                        HX_RELEASE(pPacketObj);
                        HX_RELEASE(pPacketBuffer);

                        return HXR_OK;
                    }
                }

                // If we need more data, seek back the number of bytes
                // we have left and read more.
                if (!pModFrameStart)
                {
                    m_bNeedPacket = 1;
                    m_State = GetPacketSeekPending;

                    // XXXMEH - if m_bCheckBadData == 0, then we
                    // found a frame, but it is just incomplete.
                    // Therefore, we need to read some more to
                    // complete the frame. This is a normal occurrence.
                    // We will seek to the end of the last frame we
                    // found and do a read. This should allow us to
                    // get the entire frame in the read buffer.
                    // However, if m_bCheckBadData == 1, then we
                    // didn't find a frame in the rest of the buffer
                    // we currently have. Therefore, don't want to
                    // seek to the end of the last frame, we want to
                    // seek to the end of where we just read, thus
                    // skipping this data in which we know no frames
                    // are present.
                    if (!m_bCheckBadData)
                    {
                        m_ulBytesRead -= pPacketData->dwBytesRemaining;
                    }
                    else
                    {
                        // Add to the amount of bad data we're skipping
                        m_ulGarbageBytesRead += pPacketData->dwBytesRemaining;
                    }

                    pPacketData->dwBytesRemaining = 0;
                    HX_RELEASE(pPacketData->pReadBuffer);

                    if (m_ulGarbageBytesRead < MAX_GARBAGE_BYTES)
                    {
                        m_pFileObj->Seek(m_ulBytesRead, FALSE);
                    }
                    else
                    {
                        m_State = Ready;
                        m_pStatus->StreamDone(0);
                    }

                    HX_RELEASE(pPacketObj);
                    HX_RELEASE(pPacketBuffer);

                    return HXR_OK;
                }
            }
//            else
//            {
//                UINT32 ulOffset = m_ulBytesRead - pPacketData->dwBytesRemaining;
//                char szDbgStr[256];
//                sprintf(szDbgStr, "Frame of size %d found at offset %lu (%02x %02x %02x %02x) next offset = %lu\n",
//                        nSyncSize, ulOffset, pPacketData->pBuffer[0], pPacketData->pBuffer[1],
//                        pPacketData->pBuffer[2], pPacketData->pBuffer[3], ulOffset + nSyncSize);
//                OutputDebugString(szDbgStr);
//            }

            // Reformat the current MP3 frame if we are streaming
            int nModFrameSize = nSyncSize;

#if defined (MPA_FMT_DRAFT00)
            if (m_bStreaming && !m_bRtp)
            {
                // We need 1+ audio frames
                UINT32  ulReadSize = 0;
                pModFrameStart = m_pFmtBuf->GetReadPointer(ulReadSize);

                UINT32 nSync = m_pMp3Fmt->CheckValidFrame(pModFrameStart, ulReadSize);

                if (ulReadSize >= nSync + 6 ||
                    m_bEOF)
                {
                    nModFrameSize = m_pMp3Fmt->ReformatMP3Frame(&pModFrameStart,
                                                                ulReadSize,
                                                                m_pFmtBuf->GetPrevBytes());

                    // If we did not have enough back data to reformat, skip this frame
                    // in the format buffer
                    if (!nModFrameSize)
                        m_pFmtBuf->AdvanceRead(nSync);
                }
                else
                    nModFrameSize = 0;

                if (nModFrameSize)
                    m_pFmtBuf->AdvanceRead(nSync);
                else
                {
                    // Are we advancing the buffer by some
                    // positive amount?
                    if (nSyncSize > 0)
                    {
                        // Move to next sync word
                        pPacketData->pBuffer += nSyncSize;
                        pPacketData->dwBytesRemaining -= nSyncSize;

                        goto TOP;
                    }
                    else
                    {
						// If we get here, then we have gotten
                        // lost on parsing this file, and likely
                        // the file is corrupt or unsupported.
                        // If we had tried to advance the buffer
                        // by zero, we would have entered an infinite
                        // loop. Therefore, we bail out.
                        HX_RELEASE(pPacketObj);
                        HX_RELEASE(pPacketBuffer);
                        HX_RELEASE(pPacketData->pReadBuffer);
                        m_pStatus->StreamDone(0);
                        return HXR_OK;
                    }
                }
            }
#endif //MPA_FMT_DRAFT00

            // We need the pPacketBuffer here, but not before.
            pPacketBuffer = (*m_pCreatePacketFunction)( m_pClassFactory,
                                                        pPacketData->pReadBuffer,
                                                        pModFrameStart,
                                                        nModFrameSize );

            // Set packet data
            if (nModFrameSize)
            {
#if defined (MPA_FMT_DRAFT00)
                // Copy the rtp payload header
                if (m_bRtp)
                {
                    pPacketBuffer->SetSize(nModFrameSize+m_RtpPackets.ulHeaderSize);
                    

                    if (pPacketBuffer->GetSize() != (UINT32)nModFrameSize+m_RtpPackets.ulHeaderSize)
                    {
						HX_RELEASE(pPacketBuffer);
                        HX_RELEASE(pPacketObj);

                        return HXR_OUTOFMEMORY;
                    }

                    memcpy(pPacketBuffer->GetBuffer(), /* Flawfinder: ignore */
                           m_RtpPackets.aHeader,
                           m_RtpPackets.ulHeaderSize);

                    memcpy(pPacketBuffer->GetBuffer()+m_RtpPackets.ulHeaderSize, /* Flawfinder: ignore */
                           pModFrameStart,
                           nModFrameSize);

                    // Reset rtp packet values
                    memset(&m_RtpPackets.aHeader, 0, sizeof(m_RtpPackets.aHeader));
                    m_RtpPackets.bPacketReady = 0;
                    m_RtpPackets.ulHeaders = 0;
                    m_RtpPackets.ulDataChunks = 0;
                    m_RtpPackets.ulBytesFree = RTP_PACKET_SIZE;
                }
#endif //MPA_FMT_DRAFT00
                if( m_bStreaming || m_bRtp || m_pMp3Fmt->GetMetaRepeat() )
                {
					pPacketBuffer->Set(pModFrameStart,nModFrameSize);
                }

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
                // If this packet has meta data, left shift the mp3
                // data over the meta data.
                if (m_bMetaPacket)
                {
					UCHAR *pTemp = pPacketBuffer->GetBuffer();

                    if (m_bRtp)
                        pTemp += 4;

                    UINT32 ulBytesToTitle = m_ulNextMetaPos -
                        (m_ulBytesRead - pPacketData->dwBytesRemaining);

                    for (UINT32 i=ulBytesToTitle;
                         i<(UINT32)nModFrameSize - m_ulMetaLength; i++)
                        pTemp[i] = pTemp[i+m_ulMetaLength];

                    pPacketBuffer->SetSize(pPacketBuffer->GetSize()-m_ulMetaLength);

                    // Set next meta data offset
                    m_ulNextMetaPos += m_pMp3Fmt->GetMetaRepeat() + m_ulMetaLength;
                    m_ulMetaLength = 0;
                    m_bMetaPacket = 0;
                }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST

                pPacketObj->Set(pPacketBuffer, deliveryTime, streamNo,
                                ASMFlags, ASMRuleNo);
            }

            // We know we've read good data now, so clear the
            // amount of consecutive garbage bytes we've read.
            m_ulGarbageBytesRead = 0;

            // Set the delivery time for the next packet. Note that each
            // packet is stamped with a delivery time which is used to pace
            // the delivery of packets to the rendering plug-ins.
            m_dNextPts += m_Info.dTimePerFrame;
            m_ulNextPacketDeliveryTime = (UINT32)m_dNextPts;

            if(m_bIsVBR && (m_ulNextPacketDeliveryTime > m_ulCurrentDuration))
            {
				// Compute a new duration
                UINT32 ulRemainingFilesSize = m_ulFileSize -
                    (m_ulBytesRead - pPacketData->dwBytesRemaining);
                double ulRemainingDuration =
                    ulRemainingFilesSize / (double)(m_Info.ulBitRate>>3) * 1000.0;
                m_ulCurrentDuration += (UINT32) ulRemainingDuration;
                m_bFileDurationChanged = TRUE;
                UpdateDuration();
            }

            // Move to next sync word
            pPacketData->pBuffer += nSyncSize;
            pPacketData->dwBytesRemaining -= nSyncSize;

            // Notify the RMA core that the packet object is ready
            HXLOGL4(HXLOG_MP3X, "\tCalling PacketReady() pts=%lu", pPacketObj->GetTime());
            if (nModFrameSize)
                m_pStatus->PacketReady(status, pPacketObj);

            // Release the object since we are done with it
            HX_RELEASE(pPacketObj);
            HX_RELEASE(pPacketBuffer);
        }
        else
            return HXR_OUTOFMEMORY;
    }

    // No more packets are available for this stream
    else
    {
		// Notify the RMA core that the stream is done
        m_pStatus->StreamDone(0);
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXFileResponse::SeekDone                               ref:  hxfiles.h
//
//  This routine is automatically called when the Seek() operation in the
//  file is complete. Other actions are then taken dependent on the current
//  processing state.
//
STDMETHODIMP
CRnMp3Fmt::SeekDone(HX_RESULT status)
{
    if (m_bClosed)
    {
        return HXR_UNEXPECTED;
    }

    switch (m_State)
    {
       case SeekSeekPending:
           // We are in the process of handling a Seek() request...
           m_State = Ready;

           // Notify the RMA core that the Seek() has completed
           m_pStatus->SeekDone(status);
           break;

       case GetId3HeaderSeekPending:

           // Return to default access
           m_pFileObj->Advise(HX_FILEADVISE_ANYACCESS);
           // If we failed,then we need to try again with
           // this new HX_FILEADVISE_ANYACCESS mode. If we
           // succeeded, then we can go ahead and read.
           if (SUCCEEDED(status))
           {
               // We successfully seeked, so now read the ID3 tags, if any
               m_State = GetId3HeaderReadPending;
               m_pFileObj->Read(ID3HeaderLen+BytesBeforeID3Header);
           }
           else
           {
               // We tried to seek to the end of the file
               // in order to get the ID3 tags, but the seek
               // failed (that can sometimes happen if we're
               // playing via http and we have a HTTP 1.0 server)
               // If this happens, then we won't worry about getting
               // the ID3 tags - we'll simply go ahead and seek
               // back to the beginning of the file like we do
               // after we read the ID3 tags.
               m_State = GetFileHeaderSeekPending;
               m_pFileObj->Seek(MY_FILE_HEADER_START, FALSE);
           }

           break;

       case GetFileHeaderSeekPending:
       {
           m_State = GetFileHeaderReadPending;

           // Read the file header data from the file.
           // Read more when we are streaming - we need more
           // accurate avg bitrate for vbrs when streaming.
           UINT32 ulReadSize = MY_FILE_HEADER_LENGTH << 3;
           if (m_bLive ||
               m_pFileObj->Advise(HX_FILEADVISE_RANDOMACCESS) == HXR_ADVISE_PREFER_LINEAR)
           {
               ulReadSize = MY_FILE_HEADER_LENGTH;
           }
           m_pFileObj->Read(ulReadSize);
       }
       break;

       case GetStreamHeaderSeekPending:
           MyCreateStreamHeaderObj_v(HX_STATUS_OK, NULL);
           break;

       case GetPacketSeekPending:
       {
           m_State = GetPacketReadPending;
           UINT32  ulReadSize = kReadSize;

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
           if (m_ulMetaReadSize)
           {
               ulReadSize += m_ulMetaReadSize;
               m_ulMetaReadSize = 0;
           }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST

           m_pFileObj->Read(ulReadSize);

           break;
       }

       case SeekToOffsetNoRead:
           m_State = Ready;
           break;

       default:
           return HXR_UNEXPECTED;
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IHXFileResponse::ReadDone                               ref:  hxfiles.h
//
//  This routine is automatically called when the Read() from the file is
//  done. Other actions are then taken dependent on the current processing
//  state.
//
STDMETHODIMP
CRnMp3Fmt::ReadDone(HX_RESULT   status,
                    IHXBuffer* pBufferRead)
{
    HXLOGL4(HXLOG_MP3X,"ReadDone");
    /* This may happen in HTTP streaming when the file system
     * is in still a seeking mode when the next seek is issued.
     * The file system will then call SeekDone with a status of
     * HXR_CANCELLED for the pending seek.
     */
    if (status == HXR_CANCELLED)
    {
        return HXR_OK;
    }
    else if (m_bClosed)
    {
        return HXR_UNEXPECTED;
    }

    switch (m_State)
    {
       case GetId3HeaderReadPending:
       {
           if (pBufferRead)
           {
               // Try to extract ID3v1 header data
               INT32 lTemp;
               m_pMp3Fmt->CheckForHeaders(pBufferRead->GetBuffer()+BytesBeforeID3Header,
                                          pBufferRead->GetSize()-BytesBeforeID3Header,
                                          lTemp);
           }

           // Go back to the beginning and init the stream
           m_State = GetFileHeaderSeekPending;
           m_pFileObj->Seek(MY_FILE_HEADER_START, FALSE);
           break;
       }

       case GetFileHeaderReadPending:
           if (!pBufferRead)
               return HXR_UNEXPECTED;

           return MyCreateFileHeaderObj_hr(status, pBufferRead);
           break;

       case GetStreamHeaderReadPending:
           if (!pBufferRead)
               return HXR_UNEXPECTED;

           MyCreateStreamHeaderObj_v(status, pBufferRead);
           break;

       case GetPacketReadPending:
       {
           // Buffer the read packet
           if (HXR_OK == status && pBufferRead && pBufferRead->GetSize())
           {
               m_ulBytesRead += pBufferRead->GetSize();

               m_ReadBuf.pReadBuffer = pBufferRead;
               m_ReadBuf.pReadBuffer->AddRef();
               m_ReadBuf.pBuffer = pBufferRead->GetBuffer();
               m_ReadBuf.dwBytesRemaining = pBufferRead->GetSize();
               m_ReadBuf.dwBufferSize = pBufferRead->GetSize();

               if (m_ReadBuf.dwBufferSize < kReadSize)
                   m_bEOF = TRUE;
               else
                   m_bEOF = FALSE;

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)

               // Check for the first shoutcast song title meta data.
               // Some servers do not send the data when we expect them
               // to so sync the first location.
               if (m_bFirstMeta)
               {
                    HXLOGL4(HXLOG_MP3X,"ReadDone First MetaData");
                   int nBytes = m_ReadBuf.dwBufferSize;
                   char *pScan = (char*)m_ReadBuf.pBuffer;
                   char *p = NULL;

                   // Look for "StreamTitle" in the binary buffer.  Since meta
                   // data is text inside of a binary stream, we can not use
                   // string functions.
                   while (nBytes>0)
                   {
                       p = (char*)memchr((const char *)pScan, 'S', nBytes);

                       if (p)
                       {
                           nBytes -= p - pScan;

                           if (nBytes > 10 &&
                               !strncmp((const char *)p, "StreamTitle", 11))
                           {
                               // Meta data starts one byte before "StreamTitle"
                               int offset = (PTR_INT)p - (PTR_INT)m_ReadBuf.pBuffer - 1;
                               HX_ASSERT(m_ulNextMetaPos == m_ulBytesRead - m_ReadBuf.dwBytesRemaining + offset);

                               m_ulNextMetaPos = m_ulBytesRead - m_ReadBuf.dwBytesRemaining + offset;
                               m_bFirstMeta = FALSE;

                               break;
                           }

                           pScan = p+1;
                           --nBytes;
                       }
                       else
                       {
                           break;
                       }
                   }
               }

               // Find the length of the next ShoutCast meta packet
               if (m_ulNextMetaPos < m_ulBytesRead && !m_ulMetaLength)
               {
                   UINT32 ulBytesToMeta = m_ulNextMetaPos - (m_ulBytesRead -
                                                             m_ReadBuf.dwBytesRemaining);

                   m_ulMetaLength = m_ReadBuf.pBuffer[ulBytesToMeta] * 16 + 1;

                   // The meta data spans this buffer.  So, move the file pointer
                   // to the start of the meta data and change the properties of
                   // m_ReadBuf to exclude the partial meta data.  This way, we
                   // will get the meta data on our next read.
                   if (m_ulMetaLength + ulBytesToMeta > m_ReadBuf.dwBufferSize)
                   {
                       if(m_ulMetaLength < MAX_METADATA)
                       {
                           m_ulBytesRead -= m_ReadBuf.dwBufferSize -
                               ulBytesToMeta;
                           m_ReadBuf.dwBufferSize -= m_ReadBuf.dwBufferSize -
                               ulBytesToMeta;
                           m_ReadBuf.dwBytesRemaining =
                               m_ReadBuf.dwBufferSize;
                           m_ulMetaReadSize = m_ulMetaLength;
                           m_ulMetaLength = 0;

                           m_State = SeekToOffsetNoRead;
                           m_pFileObj->Seek(m_ulBytesRead, FALSE);
                       }
                       // this is not good meta data
                       else
                       {
                           m_ulNextMetaPos += m_pMp3Fmt->GetMetaRepeat();
                           m_ulMetaLength = 0;
                       }
                   }

                   unsigned char *pTemp = &m_ReadBuf.pBuffer[ulBytesToMeta+1];

                   // Update player's gui with the new song title
                   if (m_ulMetaLength > 1 &&
                       !strncmp((const char *)pTemp, "StreamTitle", 11))
                   {
                       IHXBuffer  *pTitle = NULL;
                       m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                       (void**)&pTitle);
                       if (pTitle)
                       {
                           pTemp = m_ReadBuf.pBuffer+ulBytesToMeta+14;
                           size_t nLen;

                           // Get the length of just the song title
                           pTemp = (unsigned char*)dbcsFindChar(
                               (const char*)pTemp, ';', m_ulMetaLength);

                           if(pTemp)
                           {
                               nLen = pTemp - 1 -
                                   (m_ReadBuf.pBuffer+ulBytesToMeta+14);
                           }
                           else
                           {
                               nLen = dbcsStrSize((const char*)
                                                  (m_ReadBuf.pBuffer+ulBytesToMeta+14),
                                                  m_ulMetaLength);

                               if(nLen == m_ulMetaLength)
                               {
                                   // Not good metadata
                                   m_ulNextMetaPos +=
                                       m_pMp3Fmt->GetMetaRepeat();
                                   m_ulMetaLength = 0;
                               }
                           }

#if !defined(HELIX_FEATURE_SERVER)
#if defined(HELIX_FEATURE_REGISTRY)
                           if(m_ulMetaLength)
                           {
                               // Put the song title in an IHXBuffer
                               pTitle->Set(m_ReadBuf.pBuffer+ulBytesToMeta+14,
                                           nLen+1);
                               pTitle->GetBuffer()[nLen] = '\0';
#if defined(HELIX_FEATURE_STATS)
							   m_pRegistry->SetStrByName(
                                   (char*)m_szPlayerReg->GetBuffer(), pTitle);
#else 
                                HXLOGL1(HXLOG_MP3X, "CRnMp3Fmt::ReadDone set author");                                   
                                CHXString   strTemp;
                                strTemp.Format("%s","Author");
                                m_pRegistry->SetStrByName(strTemp, pTitle);
#endif //HELIX_FEATURE_STATS                                
                               HX_RELEASE(pTitle);

                               m_pClassFactory->CreateInstance(
                                   CLSID_IHXBuffer, (void**)&pTitle);

                               // Set station as title
                               int nTitle = 0;
                               m_pMp3Fmt->GetId3Title(nTitle);
                               if (nTitle && pTitle)
                               {
                                   pTitle->Set(m_pMp3Fmt->GetId3Title(nTitle),
                                               nTitle+1);

                                   // Put the song title in the registry
                                   char szTitle[128]; /* Flawfinder: ignore */
#if defined(HELIX_FEATURE_STATS)
								   SafeStrCpy(szTitle,
                                              (char*)m_szPlayerReg->GetBuffer(), 128);
								   strcpy(&szTitle[strlen(szTitle)-6], /* Flawfinder: ignore */
                                          "Title");
	                                   m_pRegistry->SetStrByName(szTitle, pTitle);
#else   
                                    HXLOGL1(HXLOG_MP3X, "CRnMp3Fmt::ReadDone set title");                                 
                                    CHXString   strTemp;
                                    strTemp.Format("%s","Title");
                                    m_pRegistry->SetStrByName(strTemp, pTitle);
                                    HX_RELEASE(pTitle);
#endif                                    
                               }
                           }
#endif /* #if defined(HELIX_FEATURE_REGISTRY) */

#endif
                           HX_RELEASE(pTitle);
                       }
                   }
               }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST
           }

           // If there was an error, set EOF
           else
           {
               m_bEOF = TRUE;

               HX_RELEASE(m_ReadBuf.pReadBuffer);
               m_pStatus->StreamDone(0);

               return HXR_OK;
           }

           m_ReadBuf.status = status;

           // If the read call did not finish inside of GetPacket
           // (http streaming), then create a packet when the read finished.
           if (m_bNeedPacket && m_ReadBuf.dwBytesRemaining)
           {
               m_bNeedPacket = 0;

               return MyCreatePacketObj_hr(m_ReadBuf.status, &m_ReadBuf);
           }
       }
       break;

       default:
           return HXR_UNEXPECTED;
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  Method:
//      IHXFileStatResponse::StatDone
//  Purpose:
//      Called in response to a Stat call on a IHXFileStat object
//
//
STDMETHODIMP
CRnMp3Fmt::StatDone(HX_RESULT status,
                    UINT32 ulSize,
                    UINT32 ulCreationTime,
                    UINT32 ulAccessTime,
                    UINT32 ulModificationTime,
                    UINT32 ulMode)
{
    HXLOGL4(HXLOG_MP3X, "StatDone(0x%08x,%lu,%lu,%lu,%lu,%lu)",
            status, ulSize, ulCreationTime, ulAccessTime, ulModificationTime, ulMode);
    HX_RESULT retVal = HXR_UNEXPECTED;
    if (m_bClosed)
    {
        return HXR_UNEXPECTED;
    }

    // Save the time of the last stat
    m_ulLastStatTick = HX_GET_BETTERTICKCOUNT();

    if (m_State == StatDonePending)
    {
        SetFileSize(status, ulSize);

        retVal = FinishInitDone(HXR_OK);
    }
#if defined(HELIX_FEATURE_PROGDOWN)
    else if (m_State == GetStreamHeaderStatDonePending)
    {
        // Check to see if the file size changed
        if (SUCCEEDED(status) && ulSize != m_ulFileSize)
        {
            m_bFileSizeChanged = TRUE;
        }
        // Set the file size
        SetFileSize(status, ulSize);
        // Now create the stream header
        m_State = GetStreamHeaderSeekPending;
        MyCreateStreamHeaderObj_v(HX_STATUS_OK, NULL);
    }
    else if (m_State == GetPacketStatDonePending)
    {
        // Did our filesize change?
        if (SUCCEEDED(status) && ulSize != m_ulFileSize)
        {
            HXLOGL4(HXLOG_MP3X, "\tFileSize changed: old=%lu new=%lu", m_ulFileSize, ulSize);
            // Save the new size
            m_ulFileSize = ulSize;
            // Set the flag saying the file size has
            // changed at least once
            m_bFileSizeChanged = TRUE;
            // The filesize changed, so recompute the
            // new duration and update the duration
            UpdateDuration();
        }
        // Continue with the GetPacket() call
        retVal = FinishGetPacket();
    }
#endif /* #if defined(HELIX_FEATURE_PROGDOWN) */

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::Seek                               ref:  hxformt.h
//
//  This routine moves to the packet nearest to the requested time in the
//  file. It is called, for example, in response to the user moving the
//  slider to a different location in the playback timeline.
//
STDMETHODIMP
CRnMp3Fmt::Seek(UINT32 requestedTime)
{
    if (m_bClosed)
    {
        return HXR_UNEXPECTED;
    }

    HXLOGL3(HXLOG_MP3X,"Seek(%lu)", requestedTime);
    // Find nearest packet to the requested time
    UINT32 ulPacketNo = (UINT32)(requestedTime / m_Info.dTimePerFrame);
    UINT32 ulPacketStart = ulPacketNo * m_Info.dPacketSize + m_ulFileOffset;

    m_dNextPts = ulPacketNo * m_Info.dTimePerFrame;
    m_ulNextPacketDeliveryTime = (UINT32)(m_dNextPts);

    // Note state, since Seek() is asynchronous
    m_State = SeekSeekPending;

    // Release our buffered reads
    DiscardReadBuffers_v();

    m_bEOF = FALSE;

#if defined (MPA_FMT_DRAFT00)
    // If we are reformatting mp3 frames, make sure we
    // skip the vbr header at the start of the file.
    if (!ulPacketStart && m_bStreaming && !m_bRtp)
        m_bSkipVbrHdr = m_bHasVbrHdr;

    memset(&m_RtpPackets, 0, sizeof(m_RtpPackets));
    m_RtpPackets.ulBytesFree = RTP_PACKET_SIZE;
    m_RtpPackets.dScr = m_dNextPts;
    m_RtpPackets.dPts = m_dNextPts;
#endif //MPA_FMT_DRAFT00

    // Move to the requested location in the file
    m_ulBytesRead = ulPacketStart;
    m_pFileObj->Seek(ulPacketStart, FALSE);

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXFileFormatObject::Close                              ref:  hxformt.h
//
//  This routine handles any cleanup required before closing the plug-in. All
//  references to objects should be released and memory deallocated. This is
//  called by the RMA core when the playback is finished or stopped.
//
STDMETHODIMP
CRnMp3Fmt::Close(void)
{
    HXLOGL3(HXLOG_MP3X,"Close()");
    m_bClosed = TRUE;

    HX_RELEASE(m_pFileStat);
    if (m_pFileObj != NULL)
    {
        m_pFileObj->Close();
        HX_RELEASE(m_pFileObj);
    }

#if defined(HELIX_FEATURE_REGISTRY)
    if (m_pRegistry && m_szPlayerReg)
    {
        m_pRegistry->SetStrByName((char*)m_szPlayerReg->GetBuffer(), NULL);
    }
    HX_RELEASE(m_pRegistry);
#endif /* #if defined(HELIX_FEATURE_REGISTRY) */

    HX_RELEASE(m_pStatus);
    HX_RELEASE(m_pError);
    HX_RELEASE(m_szPlayerReg);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pMetaProps);

#if defined(MPA_FMT_DRAFT00)
    HX_DELETE(m_pFmtBuf);
#endif /* #if defined(MPA_FMT_DRAFT00) */

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXFileResponse::CloseDone, WriteDone                   ref:  hxfiles.h
//
//  This routines are not used in this example, but are required to complete
//  the IHXFileResponse interface.
//
STDMETHODIMP CRnMp3Fmt::CloseDone(HX_RESULT /* result */)
{
    return HXR_OK;
}

STDMETHODIMP CRnMp3Fmt::WriteDone(HX_RESULT /* result */)
{
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  CRnMp3Fmt::~CRnMp3Fmt              ref:  filefmt1.h
//
//  Destructor
//
CRnMp3Fmt::~CRnMp3Fmt(void)
{
    Close();
    DiscardReadBuffers_v();

    HX_DELETE(m_pMp3Fmt);
}


// IUnknown COM Interface Methods

///////////////////////////////////////////////////////////////////////////////
//  IUnknown::AddRef                                            ref:  hxcom.h
//
//  This routine increases the object reference count in a thread safe
//  manner. The reference count is used to manage the lifetime of an object.
//  This method must be explicitly called by the user whenever a new
//  reference to an object is used.
//
STDMETHODIMP_(UINT32)
    CRnMp3Fmt::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown::Release                                           ref:  hxcom.h
//
//  This routine decreases the object reference count in a thread safe
//  manner, and deletes the object if no more references to it exist. It must
//  be called explicitly by the user whenever an object is no longer needed.
//
STDMETHODIMP_(UINT32)
    CRnMp3Fmt::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
        return m_RefCount;

    delete this;

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown::QueryInterface                                    ref:  hxcom.h
//
//  This routine indicates which interfaces this object supports. If a given
//  interface is supported, the object's reference count is incremented, and
//  a reference to that interface is returned. Otherwise a NULL object and
//  error code are returned. This method is called by other objects to
//  discover the functionality of this object.
//
STDMETHODIMP
CRnMp3Fmt::QueryInterface(REFIID interfaceID,
                          void** ppInterfaceObj)
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXPlugin*)this;
        return HXR_OK;
    }

    // IHXPlugin interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXPlugin))
    {
        AddRef();
        *ppInterfaceObj = (IHXPlugin*)this;
        return HXR_OK;
    }

    // IHXFileResponse interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXFileResponse))
    {
        AddRef();
        *ppInterfaceObj = (IHXFileResponse*)this;
        return HXR_OK;
    }

    // IHXFileFormatObject interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXFileFormatObject))
    {
        AddRef();
        *ppInterfaceObj = (IHXFileFormatObject*)this;
        return HXR_OK;
    }
#if defined(MPA_FMT_DRAFT00)
    // IHXPacketFormat interface is supported
    else if (IsEqualIID(interfaceID, IID_IHXPacketFormat))
    {
        AddRef();
        *ppInterfaceObj = (IHXPacketFormat*)this;
        return HXR_OK;
    }
#endif /* #if defined(MPA_FMT_DRAFT00) */
#if defined(HELIX_FEATURE_PROGDOWN)
    else if (IsEqualIID(interfaceID, IID_IHXAdvise))
    {
        AddRef();
        *ppInterfaceObj = (IHXAdvise*)this;
        return HXR_OK;
    }
#endif
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(interfaceID, IID_IHXPDStatusMgr))
    {
        if (m_pFileObj)
        {
            return (m_pFileObj->QueryInterface(interfaceID, ppInterfaceObj));
        }
        // /Else fall through and return no_interface.
    }
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(interfaceID, IID_IHXMediaBytesToMediaDur))
    {
        AddRef();
        *ppInterfaceObj = (IHXMediaBytesToMediaDur*) this;
        return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}

void CRnMp3Fmt::DiscardReadBuffers_v()
{
    HX_RELEASE(m_ReadBuf.pReadBuffer);

#if defined(MPA_FMT_DRAFT00)
    if (m_pFmtBuf)
        m_pFmtBuf->Reset();
#endif /* #if defined(MPA_FMT_DRAFT00) */
}

UCHAR* CRnMp3Fmt::GetMP3Frame_p(tReadBuffer* pPacketData, int &nSyncSize)
{
    INT32   lScan = 0;
    UCHAR   *pFrame = NULL;

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
    // Sync word is split by meta data
    if (m_ulMetaLength &&
        (m_ulNextMetaPos - (m_ulBytesRead - pPacketData->dwBytesRemaining)
         <= 3))
    {
        IHXBuffer* pBuf;
        if(SUCCEEDED(m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                     (void**)&pBuf)) && pBuf)
        {
            //pBuf->Set(pPacketData->pBuffer, pPacketData->dwBytesRemaining);
            UINT32 ulBytesToTitle = m_ulNextMetaPos - (m_ulBytesRead -
                                                       pPacketData->dwBytesRemaining);

            
            if(SUCCEEDED(pBuf->SetSize(
                             pPacketData->dwBytesRemaining - m_ulMetaLength)))
            {
                UCHAR* pNewBuf = pBuf->GetBuffer();
                UCHAR* pOldBuf = pPacketData->pBuffer;
                memcpy(pNewBuf, pOldBuf, ulBytesToTitle); /* Flawfinder: ignore */
                memcpy(pNewBuf + ulBytesToTitle, /* Flawfinder: ignore */
                       pOldBuf + ulBytesToTitle + m_ulMetaLength,
                       pPacketData->dwBytesRemaining - m_ulMetaLength -
                       ulBytesToTitle);
                HX_RELEASE(pPacketData->pReadBuffer);

                pPacketData->pReadBuffer = pBuf;
                pPacketData->pBuffer = pNewBuf;
                pPacketData->dwBufferSize = pBuf->GetSize();
                pPacketData->dwBytesRemaining = pPacketData->dwBufferSize;
            }
        }

        // Set next meta data offset
        m_ulNextMetaPos += m_pMp3Fmt->GetMetaRepeat() + m_ulMetaLength;
        m_ulMetaLength = 0;
    }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST

    // We should be at an mp3 frame, make sure we are
    nSyncSize = m_pMp3Fmt->CheckValidFrame(pPacketData->pBuffer,
                                           pPacketData->dwBytesRemaining);

    // If we were not at a frame, scan for the next one
    if (!nSyncSize)
        lScan = m_pMp3Fmt->ScanForSyncWord(pPacketData->pBuffer,
                                           pPacketData->dwBytesRemaining,
                                           nSyncSize);
   
#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
    // Check this frame for meta data
    if (m_ulNextMetaPos <= m_ulBytesRead - pPacketData->dwBytesRemaining + nSyncSize)
    {
        if (nSyncSize + m_ulMetaLength > pPacketData->dwBytesRemaining)
            return NULL;

        m_bMetaPacket = 1;
        if (!(m_bStreaming && !m_bRtp))
        {
            nSyncSize += m_ulMetaLength;
            lScan = 0;
        }
    }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST

    // Move to current frame
    if (lScan > 0)
    {
        pPacketData->pBuffer += lScan;
        pPacketData->dwBytesRemaining -= lScan;
    }

    if (lScan >= 0)
        pFrame = pPacketData->pBuffer;

#if defined(MPA_FMT_DRAFT00)
    // Copy into format buffer if streaming
    if (m_bStreaming && !m_bRtp)
    {
        if (m_bSkipVbrHdr)
            m_bSkipVbrHdr = 0;
        else
        {
            UINT32 ulBytes = 0;

            if (pFrame)
                ulBytes = m_pFmtBuf->CopyData(pFrame, nSyncSize);

            UCHAR  *pTemp = NULL;
            pTemp = m_pFmtBuf->GetReadPointer(ulBytes);

            UINT32  nSync = m_pMp3Fmt->CheckValidFrame(pTemp, ulBytes);

            // Make sure we have 1+ frame in the buffer
            if (!nSync ||
                (m_pFmtBuf->GetBytesInBuffer() > (UINT32)nSync+6 &&
                 ulBytes < (UINT32)nSync + 6))
            {
                // Preserve enough data for main_data_begin
                m_pFmtBuf->Wrap(512);
            }
        }
    }
#endif //MPA_FMT_DRAFT00


    return pFrame;
}

INT32 CRnMp3Fmt::GetStartCode(UINT8 **ppBuffer,
                              UINT32 &ulBytes)
{
    if ((INT32)ulBytes < 4)
        return -1;

    UINT8 *pStart = *ppBuffer + 2,
        *pEnd = *ppBuffer + ulBytes - 4;

    INT32 ulCode = -1;

    while (pStart < pEnd)
    {
        // Look for a 1
        pStart = (UINT8*)memchr(pStart, 1, pEnd-pStart);

        if (!pStart)
            return -1;

        // If the previous 2 bytes are 0's assume it is a start code
        if (pStart[-1] || pStart[-2])
            ++pStart;
        else
        {
            ulCode = 0x00000100 + pStart[1];
            pStart -= 2;

            break;

        }
    }

    ulBytes -= pStart - *ppBuffer;
    *ppBuffer = pStart;

    return ulCode;
}

///////////////////////////////////////////////////////////////////////////////
// Function:    InitStream_n
// Purpose:     Determine the stream type, prepare our pluigin to handle it.
// Params:      pDataRead is a buffer read from disk size MY_FILE_HEADER_LENGTH
//              pInfo is a stream info struct to describe this file
// Returns:     The number of audio streams in this file
//              or 0 meaning the header info is greater than MY_FILE_HEADER_LENGTH
//              or -1 for an unsupported file.
// Author:      cts
///////////////////////////////////////////////////////////////////////////////
UINT16 CRnMp3Fmt::InitStream_n(IHXBuffer* pDataRead,
                               tStreamInfo *pInfo,
                               HX_RESULT &hr,
                               char* pUpgrade,
                               int nBufLen)
{
    UCHAR   *pBuffer = pDataRead->GetBuffer();
    UINT32  dwSize = pDataRead->GetSize();
    INT32   lHeaderSize = 0;

    hr = HXR_OK;

#if defined(HELIX_FEATURE_MP3FF_LENIENT)
    UINT32 ulTag0 = Get4Byte(&pBuffer[0], dwSize);
    UINT32 ulTag4 = Get4Byte(&pBuffer[4], dwSize - 4);
    UINT32 ulTag8 = Get4Byte(&pBuffer[8], dwSize - 8);

    // Since we are saying we support .mpg and .dat streams (in the
    // MPEG world, system streams and VideoCDs), check for these streams
    // and notify the core and upgrade is required.  This code allows us
    // to play mislabled mp3 (ie mp3 stream with an .mpg extension).

    if(ulTag0 == MP3FF_4BYTE_SYSTEM || // Check for MPEG system streams
       ulTag0 == MP3FF_4BYTE_VIDEO  || // Check for MPEG video streams
       (pBuffer[0] == 0x47 &&          // Check for MPEG2 transport streams
        dwSize > 188 &&
        pBuffer[188] == 0x47)       ||
       (ulTag0 == MP3FF_4BYTE_RIFF &&  // Check for VideoCD
        ulTag8 == MP3FF_4BYTE_CDXA) ||
       (ulTag0 == MP3FF_4BYTE_RIFF &&  // Check for AVI files
        pBuffer[8]  == 'A'         &&
        pBuffer[9]  == 'V'         &&
        pBuffer[10] == 'I'))
    {
        hr = HXR_REQUEST_UPGRADE;
        return (UINT16)-1;
    }
    // Check for QuickTime Atoms (found some labled mpg which
    // unfortunately contianed bytes that looked like mp3 frames).
    // There are five atom types (free, skip, pnot, mdat, moov)
    else if (ulTag4 == MP3FF_4BYTE_free ||
             ulTag4 == MP3FF_4BYTE_skip ||
             ulTag4 == MP3FF_4BYTE_pnot ||
             ulTag4 == MP3FF_4BYTE_mdat ||
             ulTag4 == MP3FF_4BYTE_moov)
    {
        if (pUpgrade)
        {
            SafeStrCpy(pUpgrade, "video/quicktime", nBufLen);
            hr = HXR_REQUEST_UPGRADE;
        }
        else
            hr = HXR_INVALID_FILE;

        return (UINT16)-1;
    }
#endif /* #if defined(HELIX_FEATURE_MP3FF_LENIENT) */

    // Check for audio only streams
    if(0xFF == *pBuffer &&
       (0xF0 == (pBuffer[1] & 0xF0) ||
        0xE0 == (pBuffer[1] & 0xF0)))   // MPEG2.5
    {
        pInfo->eType = eHXAudio;
        pInfo->eAudio = eHXMPEG;

        // Packet length is one audio frame
        int     nFrame;
        INT32   lScan = 0;
        lScan = m_pMp3Fmt->ScanForSyncWord(pBuffer, dwSize, nFrame);
        if (lScan == -1)
        {
            hr = HXR_INVALID_FILE;
            return (UINT16)-1;
        }
        else if (lScan)
            goto SCAN_JUMP;

        // Init our reformatter
        if (!m_pMp3Fmt->Init(pBuffer, dwSize))
        {
            goto SCAN_JUMP;
        }

        pInfo->dPacketSize = nFrame;
        pInfo->cHeader = *(pBuffer+1);

        int             nLayer = 0,
            nSamplesPerFrame = 0,
            nPadding = 0;

        m_nChannels = 0;
                
        HXBOOL bRes = m_pMp3Fmt->GetEncodeInfo(pBuffer, dwSize,
                                               pInfo->ulBitRate, m_ulMaxSampRate,
                                               m_nChannels, nLayer, nSamplesPerFrame, nPadding);
                                        
        if(bRes != TRUE)
            goto SCAN_JUMP;

        // Convert to ms
        pInfo->dTimePerFrame = nSamplesPerFrame * 1000.0 / m_ulMaxSampRate;

        // Do not include the header info in file duration
        UINT32 ulFileSize = m_ulFileSize - m_ulFileOffset;

        // Look for Xing Specific mp3 header
        if (3 == nLayer)
        {
            XHEADDATA   xHead;
            memset(&xHead, 0, sizeof(xHead));

            if (GetXingHeader(&xHead, pBuffer) && xHead.flags & FRAMES_FLAG && xHead.frames > 0)
            {
                m_bHasVbrHdr =
                    m_bSkipVbrHdr = 1;
                m_bIsVBR = TRUE;

                pInfo->ulBitRate = (UINT32)(((double) ulFileSize * 8.0) / (xHead.frames * pInfo->dTimePerFrame / 1000.0));
                pInfo->dPacketSize = (double)ulFileSize / xHead.frames;
            }
            // Handle vbr strems w/o a header.  Look at a few frames and calculate
            // an average bitrate from these.
            else
            {
                int     nChannels = 0;
                UINT32  ulBitRate = pInfo->ulBitRate,
                    ulFrameSize = nFrame,
                    ulFrames = 1;

                // Check if frame is padded;
                int bPaddingFlag = FALSE;
                if (nPadding)
                    bPaddingFlag = TRUE;


                while (nFrame && dwSize > (UINT32)nFrame)
                {
                    pBuffer += nFrame;
                    dwSize -= nFrame;

                    UINT32 ulSampleRate = 0;

                    m_pMp3Fmt->GetEncodeInfo(pBuffer, dwSize,
                                             pInfo->ulBitRate, ulSampleRate,
                                             nChannels, nLayer, nSamplesPerFrame, nPadding);
                                        
                    nFrame = m_pMp3Fmt->CheckValidFrame(pBuffer, dwSize);

                    // Oh joy!! meta data!!
                    if (!nFrame)
                    {
                        INT32 lScan = m_pMp3Fmt->ScanForSyncWord(pBuffer, dwSize, nFrame);

                        if (lScan > 0)
                        {
                            pBuffer += lScan - 1;
                            dwSize -= lScan -1 ;
                            nFrame = 1;
                        }

                        continue;
                    }

                    // Check if frame is padded;
                    if (nPadding)
                        bPaddingFlag = TRUE;

                    if (ulSampleRate > m_ulMaxSampRate)
                    {
                        m_ulMaxSampRate = ulSampleRate;
                    }

                    ulFrameSize += nFrame;

                    ulBitRate += pInfo->ulBitRate;
                    ++ulFrames;
                }

                // Check for vbr
                if (ulBitRate / ulFrames != pInfo->ulBitRate)
                {
                    m_bIsVBR = TRUE;
                    pInfo->ulBitRate = ulBitRate / ulFrames;
                    pInfo->ulBitRate += pInfo->ulBitRate / 10;
                }
                else if (!bPaddingFlag)
                {
                    nFrame =  ulFrameSize/ulFrames;
                    ulBitRate = m_ulMaxSampRate*nFrame*8/nSamplesPerFrame;
                    if ((pInfo->ulBitRate > ulBitRate) &&
                        ((pInfo->ulBitRate - ulBitRate) > MAX_ALLOWABLE_BITRATE_DIFF ))
                        pInfo->ulBitRate = ulBitRate;
                }

                pInfo->dPacketSize = (double)ulFrameSize / ulFrames;
            }
        }

        return 1;
    }

    // Check for mp3 streams with headers
    else if (m_pMp3Fmt->CheckForHeaders(pBuffer, dwSize, lHeaderSize))
    {
        // Handle multiple Id3v2 tags
        m_ulFileOffset += lHeaderSize;
        m_ulBytesRead = m_ulFileOffset;

#if defined(HELIX_FEATURE_MP3FF_SHOUTCAST)
        if (m_pMp3Fmt->GetHeaderType() == eShoutCast ||
            m_pMp3Fmt->GetHeaderType() == eIceCast)
        {
            m_bLive = 1;

            // Set first offset to meta data
            if (m_pMp3Fmt->GetMetaRepeat())
            {
                m_ulNextMetaPos = m_pMp3Fmt->GetMetaOffset() + m_pMp3Fmt->GetMetaRepeat();
                m_bFirstMeta = TRUE;
            }
        }
#endif //HELIX_FEATURE_MP3FF_SHOUTCAST

        return 0;
    }

    // Scan the data looking for video/audio packets
    else
    {
        UCHAR *pBitStream;
        UINT32 nBytesInStream;

        // If we are beyond some known header, check for a sync word
        // after all 0 padding bits.
        if (!*pBuffer)
        {
            pBitStream = pBuffer;
            nBytesInStream = dwSize;

            for (; nBytesInStream && !*pBitStream; nBytesInStream--)
                ++pBitStream;

            if(0xFF == *pBitStream &&
               nBytesInStream > 1 &&
               (0xF0 == (pBitStream[1] & 0xF0) ||
                0xE0 == (pBitStream[1] & 0xF0)))   // MPEG2.5
            {
                m_ulFileOffset += pBitStream-pBuffer;
                m_ulBytesRead = m_ulFileOffset;

                return 0;
            }
        }

        pBitStream = pBuffer;
        nBytesInStream = dwSize;

    {
        // Is this actually an mpeg video or system stream?
        INT32 nStartCode = GetStartCode(&pBitStream, nBytesInStream);

        switch (nStartCode)
        {
           case PACK_HEADER:
           case SEQ_START_CODE:
           case SYSTEM_HEADER:
           case GROUP_START_CODE:
           case VIDEO_PACKET:
           case AUDIO_PACKET:

               hr = HXR_REQUEST_UPGRADE;
               return (UINT16)-1;
        }
    }

      SCAN_JUMP:
    // Scan bitstream for an mpeg audio frame
    int nFrame;
    INT32 lScan = m_pMp3Fmt->ScanForSyncWord(pBuffer, dwSize, nFrame);

    if (lScan > -1)
    {
        m_ulFileOffset += lScan;
        m_ulBytesRead += m_ulFileOffset;
        return 0;
    }
    else
    {
        hr = HXR_INVALID_FILE;
        return (UINT16)-1;
    }
    }
}

UINT32 CRnMp3Fmt::Get4Byte(UINT8* pBuf, UINT32 ulSize)
{
    UINT32 ulRet = 0;

    if (pBuf && ulSize >= 4)
    {
        ulRet = (pBuf[0] << 24) |
            (pBuf[1] << 16) |
            (pBuf[2] <<  8) |
            pBuf[3];
    }

    return ulRet;
}

#if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO)

void CRnMp3Fmt::SetMetaInfo(IHXValues* pFileHeader, const char* pszProp)
{
    if (pFileHeader && pszProp)
    {
        if (!strcmp(pszProp, "SrcCodec"))
        {
            SetCStringPropertyCCF(pFileHeader, "SrcCodec", "mp3", m_pContext);
        }
        else if (!strcmp(pszProp, "SrcBitRate"))
        {
            pFileHeader->SetPropertyULONG32("SrcBitRate", m_Info.ulBitRate);
        }
        else if (!strcmp(pszProp, "SrcVBREnabled"))
        {
            pFileHeader->SetPropertyULONG32("SrcVBREnabled", m_bIsVBR);
        }
        else if (!strcmp(pszProp, "SrcInterleaved"))
        {
            pFileHeader->SetPropertyULONG32("SrcInterleaved", 0);
        }
        else if (!strcmp(pszProp, "SrcSamplesPerSecond"))
        {
            pFileHeader->SetPropertyULONG32("SrcSamplesPerSecond", m_ulMaxSampRate);
        }
        else if (!strcmp(pszProp, "SrcBitsPerSample"))
        {
            pFileHeader->SetPropertyULONG32("SrcBitsPerSample", 16);
        }
        else if (!strcmp(pszProp, "SrcNumChannels"))
        {
            pFileHeader->SetPropertyULONG32("SrcNumChannels", (UINT32) m_nChannels);
        }
    }
}

#endif /* #if defined(HELIX_FEATURE_MP3FF_ONDEMANDMETAINFO) */

HX_RESULT CRnMp3Fmt::FinishInitDone(HX_RESULT status)
{
    // There are 2 ways to get a packet buffer: a heap optimal way
    // (the 'new' way), or the 'old' way, which involves making a
    // whole new buffer. We can't optimize heap usage (by re-using
    // the read buffer) if m_bRtp is true, because that requires
    // increasing the buffer size: see code under comment 'Copy the
    // rtp payload header'. We do this here because m_bRtp is set in
    // SetPacketFormat, which will have been called at this point.
    // We also use the 'old' packet buffer getter if we are running
    // as part of the server (m_bStreaming -- bad name), or if we are
    // playing a metadata file.
    if( m_bStreaming || m_bRtp || m_pMp3Fmt->GetMetaRepeat() )
    {
        m_pCreatePacketFunction = &OldPacketBufferGetter;
    }
    else
    {
        m_pCreatePacketFunction = &NewPacketBufferGetter;
    }

#if defined (MPA_FMT_DRAFT00)
    if (!m_bClosed && m_bStreaming && !m_bRtp)
        m_pFmtBuf = new CIHXRingBuffer(m_pClassFactory, 8192, (1024<<1)+512);
#endif //MPA_FMT_DRAFT00

    // Notify RMA core that intialization started in InitFileFormat is done
    m_State = Ready;
    AddRef();
    m_pStatus->InitDone(status);
    Release();

    return HXR_OK;
}

void CRnMp3Fmt::SetFileSize(HX_RESULT status, UINT32 ulSize)
{
    if (HXR_OK == status && !ulSize)
    {
        // If we do not know the file length, assume it is a live file
        m_ulFileSize = 54000000;
    }
    else if (status != HXR_OK)
    {
        // Stat failed, assume it is a live file
        m_ulFileSize = 99999999;
    }
    else
    {
        m_ulFileSize = ulSize;
    }

    if (m_ulFileSize == 54000000 ||
        m_ulFileSize == 34000000 ||
        m_ulFileSize == 99999999)
    {
        m_bLive = 1;
    }
}

HX_RESULT CRnMp3Fmt::FinishGetPacket()
{
    // Read our first buffer
    if (!m_ReadBuf.pReadBuffer)
    {
        m_bNeedPacket = 1;

        m_State = GetPacketReadPending;
        m_pFileObj->Read(kReadSize);

        return HXR_OK;
    }

    return MyCreatePacketObj_hr(m_ReadBuf.status, &m_ReadBuf);
}

void CRnMp3Fmt::UpdateDuration()
{
    if (m_pContext)
    {
        if(m_bFileDurationChanged)
        {
            m_bFileDurationChanged = FALSE;
        }
        else
        {
            // Compute a new duration
            double dDur = (m_ulFileSize  - m_ulFileOffset) / (double)(m_Info.ulBitRate>>3) * 1000.0;
            m_ulCurrentDuration = (UINT32) dDur;
        }
        HXLOGL4(HXLOG_MP3X, "\t\tNew Duration = %lu", m_ulCurrentDuration);
        // QI the context for IHXStreamSource
        IHXStreamSource* pStreamSource = NULL;
        m_pContext->QueryInterface(IID_IHXStreamSource, (void**) &pStreamSource);
        if (pStreamSource)
        {
            IUnknown* pUnkStream = NULL;
            pStreamSource->GetStream(0, pUnkStream);
            if (pUnkStream)
            {
                // QI for IHXStream
                IHXStream* pStream = NULL;
                pUnkStream->QueryInterface(IID_IHXStream, (void**) &pStream);
                if (pStream)
                {
                    IHXLayoutStream* pLayoutStream = NULL;
                    pStream->QueryInterface(IID_IHXLayoutStream, (void**) &pLayoutStream);
                    if (pLayoutStream)
                    {
                        IHXValues* pProps = NULL;
                        pLayoutStream->GetProperties(pProps);
                        if (pProps)
                        {
                            // Set the duration
                            pProps->SetPropertyULONG32("Duration", m_ulCurrentDuration);
                            // Set the properties back into the layout stream
                            HXLOGL4(HXLOG_MP3X, "\t\t\tCalling LayoutStream with new new Duration = %lu", m_ulCurrentDuration);
                            pLayoutStream->SetProperties(pProps);
                        }
                        HX_RELEASE(pProps);
                    }
                    HX_RELEASE(pLayoutStream);
                }
                HX_RELEASE(pStream);
            }
            HX_RELEASE(pUnkStream);
        }
        HX_RELEASE(pStreamSource);
    }
}
