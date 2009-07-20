/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxerror.h,v 1.7 2005/12/08 00:23:15 jeffl Exp $
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

#ifndef _HXERROR_H_
#define _HXERROR_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE       IHXBuffer	    	    IHXBuffer;
typedef _INTERFACE       IHXErrorSinkControl	    IHXErrorSinkControl;


/* Message Severity values */

enum {
     HXLOG_EMERG     = 0, /* A panic condition.  Server / Player will halt or
			     restart. */

     HXLOG_ALERT     = 1, /* A condition that should be corrected immediately.
			     Needs user intervention to prevent problems. */

     HXLOG_CRIT      = 2, /* Critical conditions. */

     HXLOG_ERR       = 3, /* Errors. */

     HXLOG_WARNING   = 4, /* Warning messages. */

     HXLOG_NOTICE    = 5, /* Conditions that are not error conditions, but
			     should possibly be handled specially. */

     HXLOG_INFO      = 6, /* Informational messages. */

     HXLOG_DEBUG     = 7  /* Messages that contain information normally of use
			     only when debugging a program. */
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXErrorMessages
 * 
 *  Purpose:
 * 
 *	Error, event, and status message reporting interface
 * 
 *  IID_IHXErrorMessages:
 * 
 *	{00000800-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXErrorMessages, 0x00000800, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXErrorMessages

DECLARE_INTERFACE_(IHXErrorMessages, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXErrorMessages methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::Report
     *	Purpose:
     *	    Call this method to report an error, event, or status message.
     *	Parameters:
     *
     *	    const UINT8	unSeverity
     *	    Type of report. This value will impact how the player, tool, or
     *	    server will react to the report. Possible values are described 
     *	    above. Depending on the error type, an error message with the 
     *	    HXR code, anda string translation of that code will be displayed. 
     *	    The error dialog includes a "more info" section that displays the
     *	    user code and string, and a link to the more info URL. In the 
     *	    server these messages are logged to the log file.
     *
     *	    const ULONG32   ulHXCode
     *	    Well known HXR error code. This will be translated to a text
     *	    representation for display in an error dialog box or log file.
     *
     *	    const ULONG32   ulUserCode
     *	    User specific error code. This will NOT be translated to a text
     *	    representation. This can be any value the caller wants, it will
     *	    be logged or displayed but not interpretted.
     *
     *	    const char*	    pUserString
     *	    User specific error string. This will NOT be translated or 
     *	    modified. This can be any value the caller wants, it will
     *	    be logged or displayed but not interpretted.
     *
     *	    const char*	    pMoreInfoURL
     *	    User specific more info URL string.
     *
     */
    STDMETHOD(Report)		(THIS_
				const UINT8	unSeverity,  
				HX_RESULT	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL) PURE;

    /************************************************************************
     *	Method:
     *	    IHXErrorMessages::GetErrorText
     *	Purpose:
     *	    Call this method to get the text description of a HXR error code.
     *	Parameters:
     *	    HX_RESULT ulHXCode (A HXR error code)
     *  Return Value:
     *	    IHXBuffer* containing error text.
     */
    STDMETHOD_(IHXBuffer*, GetErrorText)	(THIS_
						HX_RESULT	ulHXCode) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXErrorSink
 * 
 *  Purpose:
 * 
 *	Error Sink Interface
 * 
 *  IID_IHXErrorSink:
 * 
 *	{00000801-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXErrorSink, 0x00000801, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXErrorSink

DECLARE_INTERFACE_(IHXErrorSink, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXErrorSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorSink::ErrorOccurred
     *	Purpose:
     *	    After you have registered your error sink with an
     *	    IHXErrorSinkControl (either in the server or player core) this
     *	    method will be called to report an error, event, or status message.
     *
     *	    The meaning of the arguments is exactly as described in
     *	    hxerror.h
     */
    STDMETHOD(ErrorOccurred)	(THIS_
				const UINT8	unSeverity,  
				const ULONG32	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL
				) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXErrorSinkControl
 * 
 *  Purpose:
 * 
 *	Error Sink Control Interface
 * 
 *  IID_IHXErrorSinkControl:
 * 
 *	{00000802-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXErrorSinkControl, 0x00000802, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXErrorSinkControl


DECLARE_INTERFACE_(IHXErrorSinkControl, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXErrorSinkControl methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorSinkControl::AddErrorSink
     *	Purpose:
     *	    Call this method to tell the sink controller to handle an error
     *	    sink.
     *
     *	    This method also allows you to set a range of severity levels which
     *	    you will receive reports for.
     *
     *      Note: You should specify an invalid range (Low = 1, High = 0 for
     *            example) if you don't want to receive any errors.
     *
     *	    The default severity range is HXLOG_EMERG to HXLOG_INFO (0-6).
     */
    STDMETHOD(AddErrorSink)	(THIS_
				IHXErrorSink*	pErrorSink,	
                                const UINT8     unLowSeverity,
                                const UINT8     unHighSeverity) PURE;

    /************************************************************************
     *	Method:
     *	    IHXErrorSinkControl::AddErrorSink
     *	Purpose:
     *	    Call this method to remove an error sink.
     */
    STDMETHOD(RemoveErrorSink)	(THIS_
				IHXErrorSink*	pErrorSink) PURE;

};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXErrorMessages)
DEFINE_SMART_PTR(IHXErrorSink)
DEFINE_SMART_PTR(IHXErrorSinkControl)

#endif /* _HXERROR_H_ */
