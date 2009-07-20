/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcorsp.h,v 1.3 2004/07/09 18:23:37 hubbe Exp $
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

#ifndef __HXCORSP_H__
#define __HXCORSP_H__

struct IHXStream;
DEFINE_SMART_POINTER(IHXStream)
DEFINE_WRAPPED_POINTER(IHXStream)

struct IHXStreamSource;
DEFINE_SMART_POINTER(IHXStreamSource)
DEFINE_WRAPPED_POINTER(IHXStreamSource)

struct IHXPlayer;
DEFINE_SMART_POINTER(IHXPlayer)
DEFINE_WRAPPED_POINTER(IHXPlayer)

struct IHXClientEngine;
DEFINE_SMART_POINTER(IHXClientEngine)
DEFINE_WRAPPED_POINTER(IHXClientEngine)

struct IHXClientEngineSetup;
DEFINE_SMART_POINTER(IHXClientEngineSetup)
DEFINE_WRAPPED_POINTER(IHXClientEngineSetup)

struct IHXInfoLogger;
DEFINE_SMART_POINTER(IHXInfoLogger)
DEFINE_WRAPPED_POINTER(IHXInfoLogger)

struct IHXPersistenceManager;
DEFINE_SMART_POINTER(IHXPersistenceManager)
DEFINE_WRAPPED_POINTER(IHXPersistenceManager)

struct IHXDriverStreamManager;
DEFINE_SMART_POINTER(IHXDriverStreamManager)
DEFINE_WRAPPED_POINTER(IHXDriverStreamManager)

struct IHXRendererAdviseSink;
DEFINE_SMART_POINTER(IHXRendererAdviseSink)
DEFINE_WRAPPED_POINTER(IHXRendererAdviseSink)

struct IHXLayoutStream;
DEFINE_SMART_POINTER(IHXLayoutStream)
DEFINE_WRAPPED_POINTER(IHXLayoutStream)

struct IHXRendererUpgrade;
DEFINE_SMART_POINTER(IHXRendererUpgrade)
DEFINE_WRAPPED_POINTER(IHXRendererUpgrade)

struct IHXPrivateStreamSource;
DEFINE_SMART_POINTER(IHXPrivateStreamSource)
DEFINE_WRAPPED_POINTER(IHXPrivateStreamSource)

struct IHXPlayerProps;
DEFINE_SMART_POINTER(IHXPlayerProps)
DEFINE_WRAPPED_POINTER(IHXPlayerProps)

#endif // !__HXCORSP_H__
