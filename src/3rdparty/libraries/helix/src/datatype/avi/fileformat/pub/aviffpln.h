/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id$ 
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

#ifndef _AVIFFPLIN_H_
#define _AVIFFPLIN_H_

//#include "pncom.h"
//#include "rmaplugn.h"
//#include "rmaformt.h"
//#include "rmaerror.h"
//#include "riff.h"
//#include "riffres.h"
//#include "rmapends.h"
//#include "rmaengin.h"
//#include "rmaccf.h"
//#include "carray.h"     // CPNPtrArray

#if 1

#include "chxxtype.h"
#include "hxcom.h"
#include "hxassert.h"
#include "audhead.h"
#include "legacy.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxpends.h"
#include "hxplugn.h"
#include "riff.h"
#include "riffres.h"
#include "fragment.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxcore.h"


#include "hxmarsh.h"
#include "hxcodec.h"
#include "netbyte.h"
#include "hxstrutl.h"
#include "rtptypes.h"
#include "hxrelinf.h"

#include "hxheap.h"
#include "rmfftype.h"   // for the HX_SAVE_ENABLED flag
#include "hxver.h"

#include "carray.h"

#endif

class CAVIStream;
class CAVIIndex;

/////////////////////////////////////////////////////////////////////////////
//
//  Class:
//
//      CAVIFileFormat
//
//  Purpose:
//
//      AVI implementation of a basic file format.
//

class CAVIFileFormat : public IHXPlugin
                     , public IHXFileFormatObject
                     , public CRIFFResponse
                     , public IHXPendingStatus
                     , public IHXPacketFormat
                     , public IHXInterruptSafe
                     , public IHXFileSystemManagerResponse
{
    friend class CAVIStream;  // for ExternalEvent()
    friend class CAVIIndex;   // for ExternalEvent()

public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);

    CAVIFileFormat();
    static BOOL   IsAVChunk(UINT32 ulChunkId);
    static UINT16 GetStream(UINT32 ulChunkId);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // *** IRMAPlugin methods ***

    /************************************************************************
     *  IRMAPlugin::GetPluginInfo
     *  Purpose:
     *      Returns the basic information about this plugin. Including:
     *
     *      bLoadMultiple   whether or not this plugin DLL can be loaded
     *              multiple times. All File Formats must set
     *              this value to TRUE.
     *      pDescription    which is used in about UIs (can be NULL)
     *      pCopyright      which is used in about UIs (can be NULL)
     *      pMoreInfoURL    which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo) (THIS_
                              REF(BOOL)    /*OUT*/ bLoadMultiple,
                              REF(const char*) /*OUT*/ pDescription,
                              REF(const char*) /*OUT*/ pCopyright,
                              REF(const char*) /*OUT*/ pMoreInfoURL,
                              REF(ULONG32)     /*OUT*/ ulVersionNumber);

    /************************************************************************
     *  IRMAPlugin::InitPlugin
     *  Purpose:
     *      Initializes the plugin for use. This interface must always be
     *      called before any other method is called. This is primarily needed
     *      so that the plugin can have access to the context for creation of
     *      IRMABuffers and IMalloc.
     */
    STDMETHOD(InitPlugin) (THIS_
                           IUnknown*   /*IN*/  pContext);

    // *** IRMAFileFormatObject methods ***

    /************************************************************************
     *  IRMAFileFormatObject::GetFileFormatInfo
     *  Purpose:
     *      Returns information vital to the instantiation of file format
     *      plugins.
     */
    STDMETHOD(GetFileFormatInfo) (THIS_
                                  REF(const char**) /*OUT*/ pFileMimeTypes,
                                  REF(const char**) /*OUT*/ pFileExtensions,
                                  REF(const char**) /*OUT*/ pFileOpenNames);

    /************************************************************************
     *  Method:
     *      IRMAFileFormatObject::InitFileFormat
     *  Purpose:
     *      Initializes the File Format aspects of the plugin.
     *      plugins.
     */
    STDMETHOD(InitFileFormat) (THIS_
                               IHXRequest*        /*IN*/  pRequest,
                               IHXFormatResponse* /*IN*/  pFormatResponse,
                               IHXFileObject*     /*IN*/  pFileObject);

    STDMETHOD(Close) (THIS);

    /////////////////////////////////////////////////////////////////////////
    //  IRMAFileFormatObject::GetFileHeader
    //  Purpose:
    //      Called by controller to ask the file format for the number of
    //      headers in the file. The file format should call the
    //      IRMAFileFormatSession::StreamCountReady() for the IRMAFileFormat-
    //      Session object that was passed in during initialization, when the
    //      header count is available.
    //
    STDMETHOD(GetFileHeader) (THIS);

    /////////////////////////////////////////////////////////////////////////
    //  IRMAFileFormatObject::GetStreamHeader
    //  Purpose:
    //      Called by controller to ask the file format for the header for
    //      a particular stream in the file. The file format should call
    //      IRMAFileFormatSession::StreamHeaderReady() for the IRMAFileFormatSession
    //      object that was passed in during initialization, when the header
    //      is available.
    //
    STDMETHOD(GetStreamHeader) (THIS_
                                UINT16 unStreamNumber);

    /////////////////////////////////////////////////////////////////////////
    //  IRMAFileFormatObject::GetPacket
    //  Purpose:
    //      Called by controller to ask the file format for the next packet
    //      for a particular stream in the file. The file format should call
    //      IRMAFileFormatSession::PacketReady() for the IRMAFileFormatSession
    //      object that was passed in during initialization, when the packet
    //      is available.
    //
    STDMETHOD(GetPacket) (THIS_
                          UINT16      unStreamNumber);

    /////////////////////////////////////////////////////////////////////////
    //  IRMAFileFormatObject::Seek
    //  Purpose:
    //      Called by controller to tell the file format to seek to the
    //      nearest packet to the requested offset. The file format should
    //      call IRMAFileFormatSession::SeekDone() for the IRMAFileFormat-
    //      Session object that was passed in during initialization, when
    //      the seek has completed.
    //
    STDMETHOD(Seek) (THIS_
                     ULONG32     ulOffset);

    // IRMAPendingStatus methods

    /************************************************************************
     *  IRMAPendingStatus::GetStatus
     *  Purpose:
     *      Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus) (THIS_
                          REF(UINT16) uStatusCode,
                          REF(IHXBuffer*) pStatusDesc,
                          REF(UINT16) ulPercentDone);

    /*
     *  IRMAInterruptSafe methods
     */

    /************************************************************************
     *  IRMAInterruptSafe::IsInterruptSafe
     *  Purpose:
     *      This is the function that will be called to determine if
     *      interrupt time execution is supported.
     */
    STDMETHOD_(BOOL,IsInterruptSafe) (THIS)
    {
        return TRUE;
    };

    // IRMAPacketFormat methods

    STDMETHOD(GetSupportedPacketFormats)        (THIS_
                                                 REF(const char**) pFormats);
    STDMETHOD(SetPacketFormat)                  (THIS_
                                                 const char* pFormat);

    // CRIFFResponse methods
    STDMETHOD(RIFFOpenDone) (HX_RESULT);
    STDMETHOD(RIFFCloseDone) (HX_RESULT);
    STDMETHOD(RIFFFindChunkDone) (HX_RESULT status, UINT32 len);
    STDMETHOD(RIFFDescendDone) (HX_RESULT);
    STDMETHOD(RIFFAscendDone) (HX_RESULT);
    STDMETHOD(RIFFReadDone) (HX_RESULT, IHXBuffer*);
    STDMETHOD(RIFFSeekDone) (HX_RESULT);
    STDMETHOD(RIFFGetChunkDone) (HX_RESULT, UINT32, IHXBuffer*);

    // *** IRMAFileResponse methods ***
    STDMETHOD(InitDone)         (THIS_ HX_RESULT status);
    STDMETHOD(FileObjectReady)  (THIS_
                                 HX_RESULT status,
                                 IUnknown* pFileObject);
    STDMETHOD(CloseDone)        (THIS_
                                 HX_RESULT status);
    STDMETHOD(ReadDone)         (THIS_
                                 HX_RESULT status,
                                 IHXBuffer* pBuffer);
    STDMETHOD(WriteDone)        (THIS_
                                 HX_RESULT status);
    STDMETHOD(SeekDone)         (THIS_
                                 HX_RESULT status);
    STDMETHOD(DirObjectReady)   (THIS_
                                 HX_RESULT status,
                                 IUnknown* pDirObject);
protected:

//    PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME
    ~CAVIFileFormat();

    void ScanState();
    IHXValues* GetHeader();
    void SetInfo(IHXBuffer* pBuffer, UINT32 ulChunkType);
    HX_RESULT SetHeader(IHXBuffer* pBuffer);

    void IOEvent();

    char*  m_pszTitle;
    char*  m_pszAuthor;
    char*  m_pszCopyright;

    const char* m_pURL;

#pragma pack(1)
    typedef struct tag_MainHeader
    {
        UINT32 ulMicroSecPerFrame;	// frame display rate (or 0L)
        UINT32 ulMaxBytesPerSec;	// max. transfer rate
        UINT32 ulPaddingGranularity;	// pad to multiples of this
                                        // size; normally 2K.
        UINT32 ulFlags;		        // the ever-present flags
        UINT32 ulTotalFrames;	        // # frames in file
        UINT32 ulInitialFrames;
        UINT32 ulStreams;

        UINT32 ulSuggestedBufferSize;

        UINT32 ulWidth;
        UINT32 ulHeight;

        UINT32 ulScale;
        UINT32 ulRate;
        UINT32 ulStart;
        UINT32 ulLength;
    } MainHeader;
#pragma pack()

    MainHeader m_header;

    typedef enum
    {
          AS_InitPending
        , AS_OpenPending            // InitDone
        , AS_FileDescend            // triggered by GetFileHeader
        , AS_HDRLFind
        , AS_HDRLDescend
        , AS_AVIHFind
        , AS_AVIHRead
        , AS_STRLFind
        , AS_STRLDescend
        , AS_STRLScan
	    , AS_STRLScanDone
        , AS_STRLRead
        , AS_STRLAscend
        , AS_HDRLAscend
        , AS_INFOFind
        , AS_INFODescend
        , AS_INFOScan
        , AS_INFORead
        , AS_INFOAscend             // FileHeaderReady
        , AS_GetIndexFilePending
        , AS_IndexFileInit          // StreamHeaderReady, triggered by GetStreamHeader
        , AS_GetStreamFilePending   // triggered by GetPacket
        , AS_IOEvent                // PacketReady
        , AS_Ready
        , AS_Closed
    }
    AVIState;
    AVIState    m_state;

    HXBOOL m_bSeekPriming;
    BOOL m_bLocalPlayback;

    CHXPtrArray m_streamArray;
    UINT16      m_usStreamTarget; // The current target of callbacks

    CAVIIndex*  m_pIndex;

    CRIFFReader*        m_pGeneralReader;
    UINT32              m_ulMOVIOffset;

    IHXFileObject*     m_pFile;
    IHXFileSystemManager* m_pFileSystemManager;
    IHXFormatResponse* m_pFFResponse;
    IHXRequest*        m_pRequest; // XXXKB - what do we use this for?
    IHXErrorMessages*  m_pErrorMessages;

    ULONG32             m_ulRefCount;
    IUnknown*           m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;

    static const char* const     zm_pDescription;
    static const char* const     zm_pCopyright;
    static const char* const     zm_pMoreInfoURL;
    static const char* const     zm_pFileMimeTypes[];
    static const char* const     zm_pFileExtensions[];
    static const char* const     zm_pFileOpenNames[];
    static const char* const     zm_pPacketFormats[];
};

#endif // _AVIFFPLIN_H_

