/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mediamrk.h,v 1.4 2007/07/06 21:58:11 jfinnecy Exp $
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

#if defined(HELIX_FEATURE_MEDIAMARKER)

#ifndef MEDIAMRK_H
#define MEDIAMRK_H

// Forward declarations
typedef _INTERFACE IHXMediaMarkerSink    IHXMediaMarkerSink;
typedef _INTERFACE IHXMediaMarkerManager IHXMediaMarkerManager;
typedef _INTERFACE IHXValues             IHXValues;
typedef _INTERFACE IHXBuffer             IHXBuffer;
class HXPlayer;
class CHXMapStringToOb;
class CHXMapPtrToPtr;
class CHXSimpleList;

class CMarkerInfo
{
public:
    CMarkerInfo(IHXBuffer* pURLStr,
                IHXBuffer* pNameStr,
                UINT32      ulTime,
                IHXValues* pOtherParams);
    virtual ~CMarkerInfo();

    IHXBuffer* m_pURLStr;
    IHXBuffer* m_pNameStr;
    UINT32      m_ulTime;
    IHXValues* m_pOtherParams;
};

class CHXMediaMarkerManager : public IHXMediaMarkerManager
{
public:
    CHXMediaMarkerManager(HXPlayer* pPlayer);
    virtual ~CHXMediaMarkerManager();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // CHXMediaMarkerManager methods
    STDMETHOD(Close) (THIS);

    // IHXMediaMarkerManager methods
    STDMETHOD(AddMediaMarkerSink)       (THIS_ IHXMediaMarkerSink* pSink);
    STDMETHOD(RemoveMediaMarkerSink)    (THIS_ IHXMediaMarkerSink* pSink);
    STDMETHOD(AddMediaMarkerSinkFilter) (THIS_ IHXMediaMarkerSink* pSink,
                                               IHXValues*          pFilter);
    STDMETHOD(ResolveMarker)            (THIS_ IHXBuffer* pURLStr,
                                               IHXBuffer* pMarkerNameStr,
                                               UINT32      ulTime,
                                               IHXValues* pOtherMarkerParams);
    STDMETHOD_(HXBOOL,IsMarkerResolved)   (THIS_ IHXBuffer* pURLStr,
                                               IHXBuffer* pMarkerNameStr);
    STDMETHOD(GetFirstMarker)           (THIS_ IHXMediaMarkerSink* pSink,
                                               REF(IHXBuffer*)     rpURLStr,
                                               REF(IHXBuffer*)     rpMarkerNameStr,
                                               REF(UINT32)          rulTime,
                                               REF(IHXValues*)     rpOtherMarkerParams);
    STDMETHOD(GetNextMarker)            (THIS_ IHXMediaMarkerSink* pSink,
                                               REF(IHXBuffer*)     rpURLStr,
                                               REF(IHXBuffer*)     rpMarkerNameStr,
                                               REF(UINT32)          rulTime,
                                               REF(IHXValues*)     rpOtherMarkerParams);
protected:
    INT32          m_lRefCount;
    HXPlayer*     m_pPlayer;
    CHXSimpleList* m_pMarkerList;
    CHXSimpleList* m_pSinkList;

    void           ClearMarkerList();
    void           ClearSinkList();
    HXBOOL           WasSinkAdded(IHXMediaMarkerSink* pSink);
    HX_RESULT      AddMarkerInfo(CMarkerInfo* pInfo);
    HXBOOL           AreMarkersIdentical(CMarkerInfo* pInfo1, CMarkerInfo* pInfo2);
    void           BroadcastMarkerToSinks(CMarkerInfo* pInfo);
    HXBOOL           SinkWantsThisMarker(IHXMediaMarkerSink* pSink,
                                       CMarkerInfo*         pInfo);
};

#endif /* #ifdef MEDIAMRK_H */

#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */
