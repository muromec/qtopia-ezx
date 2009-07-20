/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxfilsp.h,v 1.4 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef __HXFILSP_H__
#define __HXFILSP_H__

struct IHXFileObject;
DEFINE_SMART_POINTER(IHXFileObject)
DEFINE_WRAPPED_POINTER(IHXFileObject)

struct IHXFileResponse;
DEFINE_SMART_POINTER(IHXFileResponse)
DEFINE_WRAPPED_POINTER(IHXFileResponse)

struct IHXFileSystemObject;
DEFINE_SMART_POINTER(IHXFileSystemObject)
DEFINE_WRAPPED_POINTER(IHXFileSystemObject)

struct IHXFileStat;
DEFINE_SMART_POINTER(IHXFileStat)
DEFINE_WRAPPED_POINTER(IHXFileStat)

struct IHXFileStatResponse;
DEFINE_SMART_POINTER(IHXFileStatResponse)
DEFINE_WRAPPED_POINTER(IHXFileStatResponse)

struct IHXFileSystemManager;
DEFINE_SMART_POINTER(IHXFileSystemManager)
DEFINE_WRAPPED_POINTER(IHXFileSystemManager)

struct IHXFileSystemManagerResponse;
DEFINE_SMART_POINTER(IHXFileSystemManagerResponse)
DEFINE_WRAPPED_POINTER(IHXFileSystemManagerResponse)

struct IHXFileExists;
DEFINE_SMART_POINTER(IHXFileExists)
DEFINE_WRAPPED_POINTER(IHXFileExists)

struct IHXFileExistsResponse;
DEFINE_SMART_POINTER(IHXFileExistsResponse)
DEFINE_WRAPPED_POINTER(IHXFileExistsResponse)

struct IHXFileMimeMapper;
DEFINE_SMART_POINTER(IHXFileMimeMapper)
DEFINE_WRAPPED_POINTER(IHXFileMimeMapper)

struct IHXFileMimeMapperResponse;
DEFINE_SMART_POINTER(IHXFileMimeMapperResponse)
DEFINE_WRAPPED_POINTER(IHXFileMimeMapperResponse)

struct IHXBroadcastMapper;
DEFINE_SMART_POINTER(IHXBroadcastMapper)
DEFINE_WRAPPED_POINTER(IHXBroadcastMapper)

struct IHXBroadcastMapperResponse;
DEFINE_SMART_POINTER(IHXBroadcastMapperResponse)
DEFINE_WRAPPED_POINTER(IHXBroadcastMapperResponse)

struct IHXGetFileFromSamePool;
DEFINE_SMART_POINTER(IHXGetFileFromSamePool)
DEFINE_WRAPPED_POINTER(IHXGetFileFromSamePool)

struct IHXGetFileFromSamePoolResponse;
DEFINE_SMART_POINTER(IHXGetFileFromSamePoolResponse)
DEFINE_WRAPPED_POINTER(IHXGetFileFromSamePoolResponse)

struct IHXFileAuthenticator;
DEFINE_SMART_POINTER(IHXFileAuthenticator)
DEFINE_WRAPPED_POINTER(IHXFileAuthenticator)

struct IHXRequestHandler;
DEFINE_SMART_POINTER(IHXRequestHandler)
DEFINE_WRAPPED_POINTER(IHXRequestHandler)

struct IHXRequestContext;
DEFINE_SMART_POINTER(IHXRequestContext)
DEFINE_WRAPPED_POINTER(IHXRequestContext)

struct IHXRequest;
DEFINE_SMART_POINTER(IHXRequest)
DEFINE_WRAPPED_POINTER(IHXRequest)

struct IHXFileRename;
DEFINE_SMART_POINTER(IHXFileRename)
DEFINE_WRAPPED_POINTER(IHXFileRename)

struct IHXDirHandler;
DEFINE_SMART_POINTER(IHXDirHandler)
DEFINE_WRAPPED_POINTER(IHXDirHandler)

struct IHXDirHandlerResponse;
DEFINE_SMART_POINTER(IHXDirHandlerResponse)
DEFINE_WRAPPED_POINTER(IHXDirHandlerResponse)

#endif // !__HXFILSP_H__
