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
// ra10sh.h: ra10 streamhandler
// the initial version of this will plug directly into a streamhandler and output a m4a; later versions
// would require a second component to be built up which would handle layout, and instead of calling archiver
// routines to write a file, this component would feed packets to the layout object.

#ifndef _RA10SH_H_
#define _RA10SH_H_

#include "mp4fwplg.h"

typedef _INTERFACE IHXCommonClassFactory IHXCommonClassFactory;
class CVBRSimpleDepacketizer;

class CRA10StreamHandler : public IMP4StreamHandler
{
public:
    CRA10StreamHandler( IUnknown* pContext, IMP4StreamMixer* pMixer );
    ~CRA10StreamHandler();
    
    /*
     * IUnknown
     */
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)  ( THIS );
    STDMETHOD_(ULONG32,Release) ( THIS );

    /*
     * IMP4StreamHandler
     */
    STDMETHOD(GetFormatInfo)    ( THIS_
                                  const char*** /*OUT*/ pFileMimeTypes,
                                  const char*** /*OUT*/ pFileExtensions,
                                  const char*** /*OUT*/ pFileOpenNames
                                );
    
    STDMETHOD(InitStreamHandler) ( THIS );
    
    STDMETHOD(SetFileHeader)    ( THIS_ IHXValues* pHeader );
    STDMETHOD(SetStreamHeader)  ( THIS_ IHXValues* pHeader );
    STDMETHOD(SetPacket)        ( THIS_ IHXPacket* pPacket );
    
    STDMETHOD(StreamDone)       ( THIS );

private:
    static const char* m_pszFileMimeTypes[];
    static const char* m_pszFileExtensions[];
    static const char* m_pszFileOpenNames[];
    
    LONG32   m_lRefCount;
    UINT16   m_unStreamNumber;

    IMP4StreamMixer* m_pStreamMixer;
    CVBRSimpleDepacketizer* m_pDepacketizer;
    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;

    UINT32 m_uiRTPTimestamp;
};

       

#endif // _RA10SH_H_
