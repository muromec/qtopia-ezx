/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fullpathname.h,v 1.5 2004/07/09 18:23:21 hubbe Exp $
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

#pragma once

#include "hxstring.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MAC_UNIX) || defined(_CARBON)

// 8-bit encodings:
//   HFS path is in system encoding (required by Metrowerks libraries)
//   POSIX path is in UTF8 encoding (prefs assumes system encoding so this can't be saved)
//   URLs are percent-escaped

// HFS path <--> FSRef
OSErr HFSPathFromFSRef(const FSRef *ref, CHXString& fullPathName);
OSErr FSRefFromHFSPath(const char *path, FSRef *outRef);

// HFS path <--> URL
OSStatus URLFromHFSPath(const char *pszPath, CHXString& strURL);
OSStatus HFSPathFromURL(const char *pszURL, CHXString& fullPathName);

// "Current" path type (HFS on CFM builds, POSIX on Mach-O builds)
OSErr PathFromFSRef(const FSRef *ref, CHXString& fullPathName);
OSErr FSRefFromPath(const char *path, FSRef *outRef);
OSErr PathFromURL(const char *pszURL, CHXString& fullPathName);
OSErr URLFromPath(const char *pszPath, CHXString& strURL);

CFStringEncoding CurrentPathEncoding();

// POSIX path <--> FSRef (POSIX path is in UTF8 encoding)
OSStatus POSIXPathFromFSRef(const FSRef *ref, CHXString &fullPathName);
OSStatus FSRefFromPosixPath (const char *pszPath, FSRef *pRef);

// POSIX path <--> URL
OSStatus URLFromPOSIXPath(const char *pszPath, CHXString& strURL);
OSStatus POSIXPathFromURL(const char *pszURL, CHXString& fullPathName);

// HFS partial <--> full path name (hopefully never needed)
OSErr FullFromPartialHFSPath(const char *pszPartialOrFullPath, CHXString& fullPathName);
OSErr FullFromPartialPOSIXPath(const char *pszPartialOrFullPath, CHXString& fullPathName);

#endif // _CARBON

// Deprecated routines
//
// These are old HFS path routines; these should not be used under Carbon since they
// will fail on long filenames

OSErr PathNameFromDirID(long dirID, short vRefNum, CHXString &fullPathName);
OSErr PathNameFromWD(long vRefNum, CHXString &pathName);
OSErr PathNameFromFSSpec(const FSSpec* fsSpec, CHXString &fullPathName);
OSErr FSSpecFromPathName(const char *path, FSSpecPtr spec);


#ifdef __cplusplus
}
#endif
