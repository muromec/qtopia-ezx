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
// m4asm.h: .m4a stream mixer

#ifndef _M4ASM_H_
#define _M4ASM_H_

#include "mp4fwplg.h"

// fdecl
class CMP4Atom;
class CMP4Atom_mvhd;
class CMP4Atom_tkhd;
class CMP4Atom_mdhd;
class CMP4Atom_stsd;
class CMP4Archiver;

class CM4AStreamMixer : public IMP4StreamMixer
{
public:
    CM4AStreamMixer( IUnknown* pContext, CMP4Archiver* pArchiver );
    ~CM4AStreamMixer();
    
    /*
     * IUnknown
     */
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)  ( THIS );
    STDMETHOD_(ULONG32,Release) ( THIS );

    /*
     * IMP4StreamMixer routines
     */
    STDMETHOD(SetProperties)    ( THIS_ IHXValues* pProperties   );
    STDMETHOD(SetFileHeader)    ( THIS_ IHXValues* pFileHeader   );
    STDMETHOD(SetStreamHeader)  ( THIS_ IHXValues* pStreamHeader );
    STDMETHOD(SetPacket)        ( THIS_ IHXPacket* pPacket       );
    STDMETHOD(StreamDone)       ( THIS_ UINT16 unStreamNumber    );


    // XXXAQL
    HX_RESULT StartBuildStsdEntry( CMP4Atom_stsd* pStsd, UINT16 usStreamNum );
    IHXValues* GetStreamHeader() { return m_pStreamHeader; }

private:
    // Build the entire atom tree
    HX_RESULT BuildM4AAtoms();
    
    // Build the metadata subtree
    HX_RESULT BuildM4AMetaData( CMP4Atom* pUdta );

    // Build out an stsd entry for a specific stream
    HX_RESULT BuildStsdEntry( CMP4Atom_stsd* pStsd, UINT16 usStreamNum );
    
    // Fix up the duration if it differs from the reported one
    HX_RESULT UpdateDuration( UINT32 ulDuration );
    
    // Routine to dump out atom contents
    HX_RESULT DumpAtoms();
    
    LONG32   m_lRefCount;

    CMP4Atom* m_pRootAtom;

    // We retain copies of these so we can adjust the duration
    // after all packets are received without having to do
    // multiple atom searches
    CMP4Atom_mvhd* m_pMvhd;
    CMP4Atom_mdhd* m_pMdhd;
    CMP4Atom_tkhd* m_pTkhd;

    UINT32     m_ulReportedDuration;
    UINT32     m_ulActualDuration;
    UINT32     m_ulLastTimestamp;
    UINT32     m_ulFirstTimestamp;
    UINT32     m_ulFirstRTPTimestamp;
    
    BOOL       m_bMoovAtEnd;
    
    IUnknown*  m_pContext;
    IHXValues* m_pFileHeader;
    IHXValues* m_pStreamHeader;
    
    CMP4Archiver* m_pArchiver;

    enum RTPPacketStream
    {
	UNCHECKED = 0,
	IS_RTP    = 1,
	NOT_RTP   = 2,
    };

    RTPPacketStream m_eIsRtp;
    
    UINT32 m_uiByteCount;
    UINT32 m_uiReservedBlockCount;
};


#endif // _M4ASM_H_
