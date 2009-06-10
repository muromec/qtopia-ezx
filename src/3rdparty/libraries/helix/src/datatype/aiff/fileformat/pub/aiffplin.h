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

#ifndef _AIFFPLIN_H_
#define _AIFFPLIN_H_

#include "legacy.h"
#include "riffres.h"


class AIFFFileFormat : public CHXBaseCountingObject,
                       public IHXPlugin,
                       public IHXFileFormatObject,
                       public CRIFFResponse,
                       public IHXPendingStatus,
                       public IHXInterruptSafe
{
private:
    INT32 m_lRefCount;
    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;
    IHXRequest* m_pRequest;
    IHXFormatResponse* m_pFFResponse;
    IHXFileObject* m_pFileObject;
    CRIFFReader* m_pRiffReader;
    HXBOOL m_bHeaderSent;
    UINT32 m_ulPacketSize;
    UINT32 m_CommChunkLen;
    AudioPCMHEADER m_AudioPCMHEADER;
    UINT32 m_ulAvgBitRate;
    UINT32 m_ulDuration;
    UINT32 m_ulBytesSent;
    UINT32 m_ulSeekOffset;
    UINT32 m_ulLastPacketEndTime;

    typedef enum
    {
	AS_Ready,
	AS_InitPending,
	AS_FindCommChunkPending,
	AS_ReadCommChunkPending,
	AS_FindSSNDChunkPending,
	AS_GetPacketReadPending,
	AS_SeekFindChunkPending,
	AS_SeekPending
    } AIFFState;

    AIFFState m_state;
    static const char* const	zm_pDescription;
    static const char* const	zm_pCopyright;
    static const char* const	zm_pMoreInfoURL;

    static const char* const	zm_pFileMimeTypes[];
    static const char* const	zm_pFileExtensions[];
    static const char* const	zm_pFileOpenNames[];

    ~AIFFFileFormat();

public:
    AIFFFileFormat();
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown**ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload2();

    STDMETHOD(QueryInterface)    (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)    (THIS);
    STDMETHOD_(UINT32,Release)   (THIS);

    STDMETHOD(GetPluginInfo)     (REF(HXBOOL) bLoadMultiple,
				  REF(const char*) pDescription,
				  REF(const char*)pCopyright,
				  REF(const char*)pMoreInfoURL,
				  REF(ULONG32) ulVersionNumber);
    STDMETHOD(InitPlugin)        (IUnknown* pContext);
    STDMETHOD(GetFileFormatInfo) (REF(const char **)pFileMimeTypes,
				  REF(const char**) pFileExtensions,
				  REF(const char**)pFileOpenNames);
    STDMETHOD(InitFileFormat)    (IHXRequest* pRequest,
				  IHXFormatResponse* pFormatResponse,
				  IHXFileObject* pFileObject);
    STDMETHOD(Close)             ();
    STDMETHOD(GetFileHeader)     ();
    STDMETHOD(GetStreamHeader)   (UINT16 unStreamNumber);
    STDMETHOD(GetPacket)         (UINT16 unStreamNumber);
    STDMETHOD(Seek)              (UINT32 ulOffset);
    STDMETHOD(GetStatus)         (REF(UINT16) uStatusCode,
				  REF(IHXBuffer*)pStatusDesc,
				  REF(UINT16) ulPercentDone);

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


    // CRIFFResponse methods
    virtual STDMETHODIMP RIFFOpenDone(HX_RESULT);
    virtual STDMETHODIMP RIFFCloseDone(HX_RESULT);
    virtual STDMETHODIMP RIFFFindChunkDone(HX_RESULT status, UINT32 len);
    virtual STDMETHODIMP RIFFDescendDone(HX_RESULT);
    virtual STDMETHODIMP RIFFAscendDone(HX_RESULT);
    virtual STDMETHODIMP RIFFReadDone(HX_RESULT, IHXBuffer*);
    virtual STDMETHODIMP RIFFSeekDone(HX_RESULT);
    virtual STDMETHODIMP RIFFGetChunkDone(HX_RESULT, UINT32, IHXBuffer*);

};

#endif
