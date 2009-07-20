/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: copyfile.cpp,v 1.5 2004/11/02 22:56:04 jchaney Exp $
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

#include <string.h>
#include "hxtypes.h"
#include "copyfile.h"


OSErr DoCopyFile(FSSpec &srcSpec, FSSpec &dstSpec, Boolean moveflag)
{
FInfo	dstinfo,srcinfo;
OSErr	err = noErr;
short	srcRefNum, dstRefNum;
Ptr		filePtr = nil;
UINT32	dataleft,fsize,vleft;

long	numBytes, blockSize;
	
	if (EqualFSSpec(dstSpec, srcSpec)) return noErr;

	err = FSpDelete(&dstSpec);
	if (err && err != fnfErr) return err;

	if ((dstSpec.vRefNum == srcSpec.vRefNum) && moveflag) {		// if on same volume, do it the easy way.
	FSSpec	dstFolder;
	
		err = FSpRename(&srcSpec,dstSpec.name);
		if (err) return err;
		
		memcpy(&dstSpec.name[1],&srcSpec.name[1],srcSpec.name[0]); /* Flawfinder: ignore */
		dstSpec.name[0] = srcSpec.name[0];

		err = FSMakeFSSpec(dstSpec.vRefNum, dstSpec.parID,NULL, &dstFolder);
		if(err) return err;
//		if (err = FSSpecFromDirID(dstSpec.vRefNum, dstSpec.parID, dstFolder)) return err;
		err = FSpCatMove(&srcSpec,&dstFolder);
		if (err) return err;
		return noErr;
	}
	err = GetFileSize(srcSpec,&fsize);
	if (err) return err;
	vleft = GetVolumeFreeSpace(dstSpec.vRefNum);
	
	if (fsize > vleft) return dskFulErr;
	
	// Get all Finder information for the existing file
	err = FSpGetFInfo(&srcSpec,&srcinfo);
	if (err) goto here;

	// Create the data fork of the new file
	FSpCreateResFile(&dstSpec, srcinfo.fdCreator, srcinfo.fdType,smCurrentScript);
	err = ResError();
	if (err) goto here;
	
	err = FSpGetFInfo(&dstSpec, &dstinfo);
	if (err) goto here;
	
	vleft = 0;
// Open up the data fork of the existing file. Copy the entire data fork of the
// source file into the destination file.
	blockSize = 0;
	err = FSpOpenDF(&srcSpec, fsRdPerm, &srcRefNum);
	if (!err) GetEOF(srcRefNum,&blockSize);
	dataleft = blockSize;
	if (blockSize && !err) {
		do {
			filePtr = NewPtr(blockSize);
			if (!filePtr) blockSize = blockSize * 3L / 4L;
		} while (!filePtr);
		
		err = FSpOpenDF(&dstSpec, fsWrPerm, &dstRefNum);
		if (!err) {
			err = SetFPos(srcRefNum, fsFromStart, 0L);
			err = SetFPos(dstRefNum, fsFromStart, 0L);
			do {
				numBytes = HX_MIN(blockSize,dataleft);
				err = FSRead(srcRefNum, &numBytes, filePtr);
				err = FSWrite(dstRefNum, &numBytes, filePtr);
				vleft += numBytes;
				dataleft -= numBytes;
			} while (dataleft);
			err = FSClose(dstRefNum);
		}
		DisposePtr(filePtr);
	}
	err = FSClose(srcRefNum);
// Open up the resource fork of the existing file. Only if there is a resource fork on the
// existing file do we create a resource fork for the new file. Copy the entire resource fork
// of the source file into the destination file.
	blockSize = 0;
	err = FSpOpenRF(&srcSpec, fsRdPerm, &srcRefNum);
	if (!err) GetEOF(srcRefNum,&blockSize);
	dataleft = blockSize;
	if (blockSize && !err) {
		do {
			filePtr = NewPtr(blockSize);
			if (!filePtr) blockSize = blockSize * 3L / 4L;
		} while (!filePtr);
		err = FSpOpenRF(&dstSpec, fsWrPerm, &dstRefNum);
		if (!err) {
			err = SetEOF(dstRefNum, 0L);				// set to zip
			err = SetFPos(srcRefNum, fsFromStart, 0L);
			err = SetFPos(dstRefNum, fsFromStart, 0L);
			do {
				numBytes = HX_MIN(blockSize,dataleft);
				err = FSRead(srcRefNum, &numBytes, filePtr);
				err = FSWrite(dstRefNum, &numBytes, filePtr);
				vleft += numBytes;
				dataleft -= numBytes;
			} while (dataleft);
			err = FSClose(dstRefNum);
		}
		DisposePtr(filePtr);
	}
	err = FSClose(srcRefNum);
	// Restore the (almost) exact finder information as the original source file
	srcinfo.fdLocation.h =
	srcinfo.fdLocation.v = -1;
	err = FSpSetFInfo(&dstSpec, &srcinfo); 
	
here:
	if (moveflag) return FSpDelete(&srcSpec);
	return noErr;
}

long GetVolumeFreeSpace(short vRefNum)
{
#ifdef _CARBON
	// xxxbobclark PBXGetVolInfoSync exists in OS 8.5 and later, but still
	// I'll leave the old code there for all non-Carbon. By only using it
	// in Carbon code we'll never even risk calling it accidentally.

	XVolumeParam xpb;
	xpb.ioCompletion = NULL;
	xpb.ioNamePtr = NULL;
	xpb.ioVRefNum = vRefNum;
	
	OSErr err = PBXGetVolInfoSync(&xpb);
	
	if (err) return 0;
	
	if (xpb.ioVFreeBytes > 0x000000007fffffff)
	{
		return 0x7fffffff;
	}
	
	return (long)xpb.ioVFreeBytes;
	
#else
HVolumeParam	block;
OSErr			err;
long			freeSpace = 0L;
Str255			volname = {0};

	memset(&block,0,sizeof(HVolumeParam));
	block.ioVRefNum = vRefNum;
	err = PBHGetVInfoSync((HParmBlkPtr)&block);
	if (!err) {
		err = GetVInfo(block.ioVDrvInfo,volname,&vRefNum,&freeSpace);
	}
	return freeSpace;
#endif
}

Boolean EqualFSSpec(FSSpec &srcSpec, FSSpec &dstSpec)
{
	Boolean bEqual = srcSpec.vRefNum == dstSpec.vRefNum;
	
	if(bEqual)
	{
		bEqual = srcSpec.parID == dstSpec.parID;
	}

	if(bEqual)
	{
		bEqual = srcSpec.name[0] == dstSpec.name[0];
	}
	
	if(bEqual)
	{
		bEqual = memcmp(&srcSpec.name[1],&dstSpec.name[1],srcSpec.name[0]) == 0;
	}

	return bEqual;
}

OSErr GetFileSize(FSSpec &srcSpec,UINT32 *fsize)
{
	short	srcRefNum;

	OSErr err = FSpOpenDF(&srcSpec, fsRdPerm, &srcRefNum);
	
	if (!err) 
	{
		err = GetEOF(srcRefNum,(long*)fsize);
		FSClose(srcRefNum);
	}
	
	if(err)
	{
		*fsize = 0;
	}
	
	return err;

}
