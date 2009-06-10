/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macdir.cc,v 1.5 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "FSSpecList.h"
#include "PN_MoreFiles.h"
#include "Fullpathname.h"

#include "dir.h"
#include "macdir.h"
#include "hxstrutl.h"



// Mac_dir_list will crash with long pathnames (>255 characters), and probably
// leaks the member variable filepath each time it is used.
//
// If anyone is using this, please let me know and I'll fix it.
// - Greg Robbins 10/25/01


Dir_list*	Dir_list::create(const char* path)
{
	HX_ASSERT(!"Mac_dir_list isnt safe; see comments in macdir.cc");
	
	Dir_list*	dirlist=new Mac_dir_list(path);

	return dirlist;
}


Mac_dir_list::Mac_dir_list(const char* path) 
:filecount(0)
,filelist(0L)
{

	char		temp[256];	/* Flawfinder: ignore */ // XXX Can't have a pascal string greater than 256.
	FSSpec	dirSpec;
	OSErr		err=noErr;
	short		fileIndex=1;
	FSSpec	fileSpec;
	
	filepath=new char[256];
	::memset(filepath,0,256);
	
	SafeStrCpy(temp,path, 256);
#ifdef _CARBON
	c2pstrcpy((StringPtr)temp, (char*)temp);
#else
	c2pstr(temp);
#endif

	CreateFSSpecList(&filelist);
	
	err=FSMakeFSSpec(0, 0, (StringPtr)temp, &dirSpec);
	
	if (filelist != 0L && err != noErr)	
	{
		DeleteFSSpecList(&filelist);
		filelist=0L;
		return;
	}//if
	
	// Build the file list.
	while (FSpIterateDirectory(&dirSpec,fileIndex,&fileSpec))
	{
	
		AddSpec(&filelist,&fileSpec);
	
		fileIndex++;
	}//while
	
	return;
}

Mac_dir_list::~Mac_dir_list() 
{

	if (filelist != 0L)	DeleteFSSpecList(&filelist);

}

const	char*	Mac_dir_list::get_next() 
{

	FSSpecPtr	theSpec;
	CHXString	filename;
	
	theSpec=GetIndSpec(&filelist,filecount);
	
	if (theSpec == 0L) return 0L;
	
	filecount++;
	
	PathNameFromFSSpec(theSpec,filename);
	
	SafeStrCpy(filepath,filename, 256);
	
	return (filepath);	
}

