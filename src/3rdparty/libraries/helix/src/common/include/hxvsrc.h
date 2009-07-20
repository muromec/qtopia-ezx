/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvsrc.h,v 1.5 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _HXVSRC_H
#define _HXVSRC_H

typedef _INTERFACE	IHXStreamSource		IHXStreamSource;
typedef _INTERFACE	IHXFileObject			IHXFileObject;

// Interfaces definded in this file
typedef _INTERFACE	IHXFileViewSource		IHXFileViewSource;
typedef _INTERFACE	IHXFileViewSourceResponse	IHXFileViewSourceResponse;
typedef _INTERFACE	IHXViewSourceCommand		IHXViewSourceCommand;
typedef _INTERFACE	IHXViewSourceURLResponse	IHXViewSourceURLResponse;

// $Private:
typedef _INTERFACE	IHXClientViewSource		IHXClientViewSource;
typedef _INTERFACE	IHXClientViewSourceSink	IHXClientViewSourceSink;
// $EndPrivate.



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileViewSource
 * 
 *  IID_IHXFileViewSource:
 * 
 *	{00003500-0901-11d1-8B06-00A024406D59}
 * 
 */

enum SOURCE_TYPE
{
    RAW_SOURCE,
    HTML_SOURCE
};

DEFINE_GUID(IID_IHXFileViewSource, 0x00003500, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileViewSource

DECLARE_INTERFACE_(IHXFileViewSource, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXFileViewSource
     */
    STDMETHOD(InitViewSource)		(THIS_
	IHXFileObject*		    /*IN*/ pFileObject,
	IHXFileViewSourceResponse* /*IN*/ pResp,
	SOURCE_TYPE		    /*IN*/ sourceType,
	IHXValues*		    /*IN*/ pOptions) PURE;
    STDMETHOD(GetSource)    (THIS) PURE;
    STDMETHOD(Close)	    (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileViewSourceResponse
 * 
 *  IID_IHXFileViewSourceResponse:
 * 
 *	{00003501-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXFileViewSourceResponse, 0x00003501, 0x901, 0x11d1, 0x8b,
	    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileViewSourceResponse

DECLARE_INTERFACE_(IHXFileViewSourceResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *  IHXFileViewSourceResoponse
     */
    STDMETHOD(InitDone)		(THIS_ HX_RESULT status	) PURE;
    STDMETHOD(SourceReady)	(THIS_ HX_RESULT status,
	IHXBuffer* pSource ) PURE;
    STDMETHOD(CloseDone) (THIS_ HX_RESULT) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXViewSourceCommand
 * 
 *  IID_IHXViewSourceCommand:
 * 
 *	{00003504-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXViewSourceCommand, 0x00003504, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXViewSourceCommand

DECLARE_INTERFACE_(IHXViewSourceCommand, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXViewSourceCommand
     */
    STDMETHOD_(HXBOOL, CanViewSource)	(THIS_
					IHXStreamSource*		pStream) PURE;
    STDMETHOD(DoViewSource)		(THIS_
					IHXStreamSource*		pStream) PURE;
    STDMETHOD(GetViewSourceURL) (THIS_
					IHXStreamSource*		pSource,
					IHXViewSourceURLResponse*      pResp) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXViewSourceURLResponse
 * 
 *  IID_IHXViewSourceURLResponse:
 * 
 *	{00003505-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXViewSourceURLResponse, 0x00003505, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXViewSourceURLResponse

DECLARE_INTERFACE_(IHXViewSourceURLResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXViewSourceURLResponse
     */
    STDMETHOD(ViewSourceURLReady)	(THIS_	
					const char* /*out*/ pUrl) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewSource
 * 
 *  IID_IHXClientViewSource:
 * 
 *	{00003502-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewSource, 0x00003502, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewSource

DECLARE_INTERFACE_(IHXClientViewSource, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewSource
     */
    STDMETHOD(DoViewSource)		(THIS_
					IUnknown*		      /*IN*/ pPlayerContext,
					IHXStreamSource*	      /*IN*/ pSource) PURE;
    STDMETHOD_(HXBOOL, CanViewSource)	(THIS_
					IHXStreamSource*	      /*IN*/ pSource) PURE;

    STDMETHOD(GetViewSourceURL)		(THIS_
					IUnknown*                        pPlayerContext,
					IHXStreamSource*                pSource,
					IHXViewSourceURLResponse*       pResp) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewSourceSink
 * 
 *  IID_IHXClientViewSourceSink:
 * 
 *	{00003503-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewSourceSink, 0x00003503, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewSourceSink

DECLARE_INTERFACE_(IHXClientViewSourceSink, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewSourceSink
     */
    STDMETHOD(RegisterViewSourceHdlr)	(THIS_	
					IHXClientViewSource*	/*in*/	pViewSourceHdlr) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientViewRights
 * 
 *  IID_IHXClientViewRights:
 * 
 *	{00003506-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXClientViewRights, 0x00003506, 0x901, 0x11d1, 0x8b, 0x6,
	    0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientViewRights

DECLARE_INTERFACE_(IHXClientViewRights, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  IHXClientViewRights
     */
    STDMETHOD(ViewRights)		(THIS_
					IUnknown*		      /*IN*/ pPlayerContext) PURE;
    STDMETHOD_(HXBOOL, CanViewRights)	(THIS) PURE;

};
// $EndPrivate.

#endif
