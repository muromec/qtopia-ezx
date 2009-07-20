/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hx_morefiles.h,v 1.5 2004/07/09 18:20:14 hubbe Exp $
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
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

/*
	This code was descended from Apple Sample Code, but the
	code may have been modified.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif




Boolean		GetLocalVRefnumFromIndex(short *index, short *realRefNum);
Boolean		GetVRefnumFromIndex(short	index,short	*realRefNum);
Boolean		FSpGetVersion(FSSpec	*theFile, Byte *majorversion, Byte *minorversion, Byte *revision);
Boolean		FSpGetVersionLong(FSSpec	*theFile, short	*theversion);
Boolean		FSpGetVersionMessage(FSSpec	*theFile, char	*theversion, unsigned long ulBufLen);

//OSErr		FSpGetItemSpec(FSSpec*	dirSpec, FSSpec*	itemSpec);
Boolean	  	FSpIterateDirectory(FSSpec*	dirSpec,short index,FSSpec* fileSpec);
OSErr		FSpGetNthDirectoryItem(short vRefNum, long parID, short index, FSSpec *theSpec);

Boolean		FSpGetFileAge(FSSpec*	file,long* minutes,long* seconds,long* ticks);

Boolean		IsFolder(FSSpec* spec);

OSErr	GetMoreVInfo(short volref, StringPtr name, unsigned long *volcreated, Boolean *islocked);


#ifdef _CARBON
// routines not present in MoreFilesX...
OSErr FSpGetDirectoryID(const FSSpec* pDirSpec, long *pDirID, Boolean *pIsDir);	
OSErr FSpMoveRenameCompat(const FSSpec * srcSpec, const FSSpec * dstSpec, ConstStr255Param  copyName);
OSErr	DeleteDirectory(short vRefNum, long dirID, ConstStr255Param name);
OSErr FSpFileCopy(const FSSpec *srcSpec, const FSSpec *dstSpec, ConstStr255Param copyName,
	void *copyBufferPtr, long copyBufferSize, Boolean preflight);
#endif

#ifdef __cplusplus
}
#endif
