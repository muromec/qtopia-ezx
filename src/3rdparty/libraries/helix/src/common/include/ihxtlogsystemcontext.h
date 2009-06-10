/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxtlogsystemcontext.h,v 1.7 2007/07/06 20:43:42 jfinnecy Exp $
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



#ifndef _IHXTLOGSYSTEMCONTEXT_H
#define _IHXTLOGSYSTEMCONTEXT_H

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogSystemContext
 *
 *  Purpose:
 *			Allow the caller to send thread specific information into the log system
 *  
 *
 */

// {1AFCEEBD-17B1-4698-A4A4-1C8D84565512}
DEFINE_GUID(IID_IHXTLogSystemContext, 
0x1afceebd, 0x17b1, 0x4698, 0xa4, 0xa4, 0x1c, 0x8d, 0x84, 0x56, 0x55, 0x12);

#undef INTERFACE
#define INTERFACE IHXTLogSystemContext

DECLARE_INTERFACE_(IHXTLogSystemContext, IUnknown)
{
	STDMETHOD(PushContext) (THIS_ const char* szContext) PURE;
	STDMETHOD(PopContext) (THIS) PURE;
	STDMETHOD(SetThreadJobName) (THIS_ const char* szJobName, UINT32 nThreadId, HXBOOL bPropogateChange = FALSE) PURE;
	STDMETHOD(SetParentChildRelationship) (THIS_ UINT32 nParentThreadId, UINT32 nChildThreadId) PURE;
	STDMETHOD(EndThread) (UINT32 nThreadId) PURE;
	STDMETHOD(SetThreadFileAndLine) (THIS_ const char* szFilename, UINT32 nLineNum) PURE;
	STDMETHOD(SetDefaultJobName) (THIS_ const char* szJobName) PURE;
};

#if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS)
#include "hxtsmartpointer.h"
HXT_MAKE_SMART_PTR(IHXTLogSystemContext)
#endif

#endif // _IHXTLOGSYSTEMCONTEXT_H
