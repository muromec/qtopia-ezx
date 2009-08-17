/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: progsink.h,v 1.5 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _PROGSINK_H_
#define _PROGSINK_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */

typedef _INTERFACE IUnknown			IUnknown;
typedef _INTERFACE IHXProgressSink		IHXProgressSink;
typedef _INTERFACE IHXProgressSinkControl      IHXProgressSinkControl;
 
/****************************************************************************
 * 
 *  Interface:
 *
 *  IHXProgressSink
 *
 *  Purpose:
 *
 *  Supports callback notification about a job's progress
 *
 *  IHXProgressSink
 *
 *  {6F8C5FB0-C1D3-11d2-871B-00C0F031C266}
 *
 */
DEFINE_GUID(IID_IHXProgressSink, 
    0x6f8c5fb0, 0xc1d3, 0x11d2, 0x87, 0x1b, 0x0, 0xc0, 0xf0, 0x31, 0xc2, 0x66);


#undef  INTERFACE
#define INTERFACE   IHXProgressSink

DECLARE_INTERFACE_(IHXProgressSink, IUnknown)
{
    /************************************************************************
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)	(THIS) PURE;

    STDMETHOD_(UINT32,Release)	(THIS) PURE;

    /************************************************************************
     *	IHXProgressSink methods
     */
   
    /************************************************************************
     *	Method:
     *	    IHXProgressSink::SetProgress
     *	Purpose:
     *	    Set the percent complete.
     *	Parameters:
     *	    ulPercentComplete - [in] Percent of the merge that is complete.
     */
    STDMETHOD(SetProgress)	    (THIS_ UINT32 ulPercentComplete) PURE;

    /************************************************************************
     *	Method:
     *	    IHXProgressSink::NotifyStart
     *	Purpose:
     *	    Notification that the job is starting.
     */
    STDMETHOD(NotifyStart)	    (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXProgressSink::NotifyFinish
     *	Purpose:
     *	    Notification that the job is complete.
     */
    STDMETHOD(NotifyFinish)	    (THIS) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *  IHXProgressSinkControl
 *
 *  Purpose:
 *
 *  Used to register progress callbacks
 *
 *  IHXProgressSinkControl
 *
 *  {DC464800-C1D3-11d2-871B-00C0F031C266}
 *
 */
DEFINE_GUID(IID_IHXProgressSinkControl, 
    0xdc464800, 0xc1d3, 0x11d2, 0x87, 0x1b, 0x0, 0xc0, 0xf0, 0x31, 0xc2, 0x66);


#undef  INTERFACE
#define INTERFACE   IHXProgressSinkControl

DECLARE_INTERFACE_(IHXProgressSinkControl, IUnknown)
{
    /************************************************************************
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)	(THIS) PURE;

    STDMETHOD_(UINT32,Release)	(THIS) PURE;

    /************************************************************************
     *	IHXProgressSinkControl methods
     */

    /* Type of progress sink */
    enum SINK_TYPE
    {
        FILE_MERGE_PROGRESS	= 0
    };
   
    /************************************************************************
     *	Method:
     *	    IHXProgressSinkControl::AddSink
     *	Purpose:
     *	    Adds sink.
     *	Parameters:
     *      nSinkType - [in] Type of progress sink 
     *	    pObj - [in] Object that supports the IHXProgressSink interface.
     */
    STDMETHOD(AddSink)	    (THIS_ SINK_TYPE nSinkType, IHXProgressSink* pObj) PURE;

    /************************************************************************
     *	Method:
     *	    IHXProgressSinkControl::RemoveSink
     *	Purpose:
     *	    Removes sink.
     *	Parameters:
     *      nSinkType - [in] Type of progress sink 
     *	    pObj - [in] Object that supports the IHXProgressSink interface.
     */
    STDMETHOD(RemoveSink)	    (THIS_ SINK_TYPE nSinkType, IHXProgressSink* pObj) PURE;

};

#endif //_PROGSINK_H_
