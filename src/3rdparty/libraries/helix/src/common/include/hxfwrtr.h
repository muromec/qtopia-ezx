/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxfwrtr.h,v 1.4 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXFWRTR_H_
#define _HXFWRTR_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"

#include "ihxpckts.h"
#include "hxfiles.h"


// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXFileWriterMonitor
 *
 *  Purpose:
 *
 *	Interface for File Writer Monitor object.
 *
 *  IID_IHXFileWriterMonitor
 *
 *	{B5615DE1-42A6-11d5-A90C-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXFileWriterMonitor,
0xb5615de1, 0x42a6, 0x11d5, 0xa9, 0xc, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXFileWriterMonitor

DECLARE_INTERFACE_(IHXFileWriterMonitor, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXFileWriterMonitor methods
     */
    STDMETHOD(OnStatus)		(THIS_
				HX_RESULT status,
				IHXValues* pInfoList
				) PURE;

    STDMETHOD(OnVolumeInitiation)	(THIS_
				HX_RESULT status,
				const char* pName,
				REF(ULONG32) ulTag
				) PURE;

    STDMETHOD(OnPacketsReady)	(THIS_
				HX_RESULT status
				) PURE;

    STDMETHOD(OnVolumeCompletion)	(THIS_
				HX_RESULT status,
				ULONG32 ulTag
				) PURE;

    STDMETHOD(OnCompletion)	(THIS_
				HX_RESULT status
				) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPropertyAdviser
 *
 *  Purpose:
 *
 *	Interface for File Writer Monitor object.
 *
 *  IID_IHXPropertyAdviser
 *
 *	{264FD2F0-432B-11d5-A90D-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXPropertyAdviser, 
0x264fd2f0, 0x432b, 0x11d5, 0xa9, 0xd, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXPropertyAdviser

DECLARE_INTERFACE_(IHXPropertyAdviser, IUnknown)
{
	/*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXPropertyAdviser methods
     */
    STDMETHOD(GetPropertyULONG32)	(THIS_
					const char*      pPropertyName,
					REF(ULONG32)     ulPropertyName
					) PURE;
    
    STDMETHOD(GetPropertyBuffer)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue
					) PURE;

    STDMETHOD(GetPropertyCString)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue
					) PURE;

    STDMETHOD(GetPropertySet)		(THIS_
					const char*	 pPropertySetName,
					REF(IHXValues*) pPropertySet
					) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXFileWriter
 *
 *  Purpose:
 *
 *	Interface for File Writer object.
 *
 *  IID_IHXFileWriter
 *
 *	{B5615DE0-42A6-11d5-A90C-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXFileWriter, 
0xb5615de0, 0x42a6, 0x11d5, 0xa9, 0xc, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXFileWriter

DECLARE_INTERFACE_(IHXFileWriter, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXFileWriter methods
     */
    /************************************************************************
     *	Method:
     *	    IHXFileFormatObject::GetFileFormatInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of file format 
     *	    plugins.
     */
    STDMETHOD(GetFileFormatInfo)(THIS_
				REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames
				) PURE;

    STDMETHOD(InitFileWriter)	(THIS_
				IHXRequest* pRequest,
				IHXFileWriterMonitor* pMonitor,
				IHXPropertyAdviser* pAdviser
				) PURE;

    STDMETHOD(Close)		(THIS) PURE;

    STDMETHOD(Abort)		(THIS) PURE;

    STDMETHOD(SetFileHeader)	(THIS_
				IHXValues* pHeader
				) PURE;

    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader
				) PURE;

    STDMETHOD(SetProperties)	(THIS_
				IHXValues* pProperties
				) PURE;
    
    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket
				) PURE;

    STDMETHOD(StreamDone)	(THIS_
				UINT16 unStreamNumber
				) PURE;
};
// $EndPrivate.

#endif  // _HXFWRTR_H_
