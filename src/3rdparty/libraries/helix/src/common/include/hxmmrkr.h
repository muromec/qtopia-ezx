/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmmrkr.h,v 1.4 2005/03/14 19:27:09 bobclark Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef HXMMRKR_H
#define HXMMRKR_H

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE IHXMediaMarkerManager IHXMediaMarkerManager;
typedef _INTERFACE IHXMediaMarkerSink    IHXMediaMarkerSink;
typedef _INTERFACE IHXValues             IHXValues;

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXMediaMarkerManager
 *
 *  Purpose:
 *
 *      Interface to manage media markers
 *
 *  IID_IHXMediaMarkerManager:
 *
 *      {46679D62-F7AC-4b0e-8000-F4EE90F785B1}
 *
 */
DEFINE_GUID(IID_IHXMediaMarkerManager, 0x46679d62, 0xf7ac, 0x4b0e, 0x80, 0x0, 0xf4, 
                             0xee, 0x90, 0xf7, 0x85, 0xb1);

DECLARE_INTERFACE_(IHXMediaMarkerManager, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXMediaMarkerManager methods
     */

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::AddMediaMarkerSink
     *  Purpose:
     *      Register a media marker sink
     */
    STDMETHOD(AddMediaMarkerSink) (THIS_ IHXMediaMarkerSink* pSink) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::RemoveMediaMarkerSink
     *  Purpose:
     *      Unregister a media marker sink
     */
    STDMETHOD(RemoveMediaMarkerSink) (THIS_ IHXMediaMarkerSink* pSink) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::AddMediaMarkerSinkFilter
     *  Purpose:
     *      Add a filter on the markers which will be
     *      received by this media marker sink. Multiple calls
     *      to this interface with the same sink set up an
     *      implicit OR between all the added filters for that sink.
     */
    STDMETHOD(AddMediaMarkerSinkFilter) (THIS_ IHXMediaMarkerSink* pSink,
                                               IHXValues*          pFilter) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::ResolveMarker
     *  Purpose:
     *      Method that media marker sources will use to tell the core
     *      about their markers
     */
    STDMETHOD(ResolveMarker) (THIS_ IHXBuffer* pURLStr,
                                    IHXBuffer* pMarkerNameStr,
                                    UINT32      ulTime,
                                    IHXValues* pOtherMarkerParams) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::IsMarkerResolved
     *  Purpose:
     *      Method to find if a specific media marker has been resolved
     *      and if so, find the time that it resolved to
     */
    STDMETHOD_(HXBOOL,IsMarkerResolved) (THIS_ IHXBuffer* pURLStr,
                                             IHXBuffer* pMarkerNameStr) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::GetFirstMarker
     *  Purpose:
     *      Retrieves the first marker using the currently-set
     *      filters for this marker sink.
     */
    STDMETHOD(GetFirstMarker) (THIS_ IHXMediaMarkerSink* pSink,
                                     REF(IHXBuffer*)     rpURLStr,
                                     REF(IHXBuffer*)     rpMarkerNameStr,
                                     REF(UINT32)          rulTime,
                                     REF(IHXValues*)     rpOtherMarkerParams) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerManager::GetNextMarker
     *  Purpose:
     *      Retrieves the next marker using the currently-set
     *      filters for this marker sink.
     */
    STDMETHOD(GetNextMarker) (THIS_ IHXMediaMarkerSink* pSink,
                                    REF(IHXBuffer*)     rpURLStr,
                                    REF(IHXBuffer*)     rpMarkerNameStr,
                                    REF(UINT32)          rulTime,
                                    REF(IHXValues*)     rpOtherMarkerParams) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXMediaMarkerSink
 *
 *  Purpose:
 *
 *      Interface to receive media marker resolved times
 *
 *  IID_IHXMediaMarkerSink
 *
 *      {A4643C85-5B52-4b07-A761-32CFB2F284FE}
 */
DEFINE_GUID(IID_IHXMediaMarkerSink, 0xa4643c85, 0x5b52, 0x4b07, 0xa7, 0x61, 0x32,
                                    0xcf, 0xb2, 0xf2, 0x84, 0xfe);

DECLARE_INTERFACE_(IHXMediaMarkerSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXMediaMarkerSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXMediaMarkerSink::MarkerResolved
     *  Purpose:
     *      Resolve the time for a marker
     */
    STDMETHOD(MarkerResolved) (THIS_ IHXBuffer* pURLStr,
                                     IHXBuffer* pMarkerNameStr,
                                     UINT32      ulTime,
                                     IHXValues* pOtherMarkerParams) PURE;
};

#endif /* HXMMRKR_H */
