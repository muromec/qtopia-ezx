/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: recordctl.h,v 1.11 2008/04/17 21:43:51 ping Exp $
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

#ifndef _RECORDCTL_H_
#define _RECORDCTL_H_

#include "unkimp.h"
#include "hxformt.h"
#include "hxrecord.h"
#include "carray.h"
#include "hxslist.h"

class CStreamMergeSorter;

typedef struct _PendingPutPacket
{
    IHXPacket*  pPacket;
    INT32       nTimeOffset;

} PendingPutPacket;

struct HXRecordControlStreamInfo
{
    HXBOOL m_bPacketRequested;
    HXBOOL m_bSparseStream;
    HXBOOL m_bStreamDone;
    UINT32 m_ulLastTimestamp;    
};

class HXRecordControl : public IHXFormatResponse
{
public:
    HXRecordControl(IUnknown* pUnkPlayer, IUnknown* pUnkSource);

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXFormatResponse methods
    STDMETHOD(InitDone)    (THIS_ HX_RESULT status){ return HXR_OK; };
    STDMETHOD(PacketReady) (THIS_ HX_RESULT status, IHXPacket*  pPacket);
    STDMETHOD(SeekDone)    (THIS_ HX_RESULT status){ return HXR_OK; };
    STDMETHOD(FileHeaderReady) (THIS_ HX_RESULT status, IHXValues* pHeader){ return HXR_OK; };
    STDMETHOD(StreamHeaderReady) (THIS_ HX_RESULT status, IHXValues* pHeader){ return HXR_OK; };
    STDMETHOD(StreamDone)  (THIS_ UINT16 unStreamNumber){ return HXR_OK; };

    HXBOOL              IsValid(){ return m_pRecordSource ? TRUE : FALSE; };

    HXBOOL              CanGetPackets(){ return m_bCanGetPackets; };
    void                Cleanup();
    HX_RESULT           Seek(ULONG32 seekTime);
    HX_RESULT           GetPacket(UINT16 usStreamNumber, IHXPacket*& pPacket);

    HXBOOL              CanAcceptPackets();
    HX_RESULT           OnFileHeader(IHXValues* pValues);
    HX_RESULT           OnStreamHeader(IHXValues* pValues);
    HX_RESULT           OnPacket(IHXPacket* pPacket, INT32 nTimeOffset);
    void                SetSource(IUnknown* pUnkSource);
    void                OnEndOfPackets();
    HX_RESULT           GetRecordSource(REF(IHXRecordSource*) rpSource);
    HX_RESULT           HandleStreamDone(HX_RESULT status, UINT16 usStreamNumber);
    UINT32              GetLatestTimestampWritten() { return m_ulLatestTimestampWritten; }
    UINT32              GetLatestTimestampRead() { return m_ulLatestTimestampRead; }
    HXBOOL              IsWritten() { return m_bFirstTimestampWritten; }
    HXBOOL		IsRead() { return m_bFirstTimestampRead; }
    HXBOOL              IsFinishedWriting() { return m_bFinishedWriting; }
    HXBOOL		IsFinishedReading() { return m_bFinishedReading; }

protected:
    virtual             ~HXRecordControl(void);
    void                ReadRecordSourcePrefs(IUnknown* pUnkPlayer);
    HX_RESULT           WritePacket(IHXPacket* pPacket, INT32 lTimeOffset);
    HX_RESULT           SendPacket(IHXPacket* pPacket, INT32 lTimeOffset);
    HX_RESULT           WriteAllAvailablePackets();
    HXBOOL              IsSparseStream(const char* pszMimeType);
    void                ChangeLostPacketTimestamp(IHXPacket* pPacket, UINT32 ulNewTime);

    LONG32                     m_lRefCount;
    IHXRecordSource*           m_pRecordSource;
    IHXRecordService*          m_pRecordService;
    CHXPtrArray                m_PendingGetPackets;
    UINT32                     m_ulNumStreams;
    HXRecordControlStreamInfo* m_pStreamInfo;
    HXBOOL                     m_bFirstTimestampWritten;
    UINT32                     m_ulLatestTimestampWritten;
    HXBOOL                     m_bFirstTimestampRead;
    UINT32                     m_ulLatestTimestampRead;
    HXBOOL                     m_bFinishedWriting;
    HXBOOL                     m_bFinishedReading;
    HXBOOL                     m_bDisableRSVelocity;
    HXBOOL                     m_bDisableRSMergeSort;
    UINT32                     m_ulRSTimeSpanLimit;
    UINT32                     m_ulRSQueueDepthLimit;
    HXBOOL                     m_bCanGetPackets;
    INT32                      m_lTimeOffset;
    CHXSimpleList              m_PendingPutPackets;
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
    CStreamMergeSorter*        m_pMergeSorter;
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */

private:

};

#endif // _RECORDCTL_H_


