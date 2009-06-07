/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils.h,v 1.12 2006/05/19 05:56:13 pankajgupta Exp $
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

#ifndef FILESPECUTILS_H
#define FILESPECUTILS_H

#include "hxstring.h"
#include "hxbuffer.h"
#include "filespec.h"

class CHXFileSpecUtils
{
	
public:
	// disk utilities
	static HX_RESULT GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& freeSpace);
	static HX_RESULT GetTotalSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& totalSpace);
	static HXBOOL IsDiskEjectable(const CHXDirSpecifier& volSpec);
	static HXBOOL IsDiskWritable(const CHXDirSpecifier& volSpec);

	// IsLocal returns TRUE if the file or directory is on a local volume
	// (not on a server)
	static HXBOOL IsDiskLocal(const CHXDirSpecifier& volSpec);

	// file/dir utilities

	static HX_RESULT GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext = NULL);
	static HX_RESULT GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize);
	
	static HXBOOL FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext = NULL);	// returns TRUE only if exists & is a file
	static HXBOOL DirectoryExists(const CHXDirSpecifier& dirSpec);	// returns TRUE only if exists & is a directory
	
	static HX_RESULT CreateDir(const CHXDirSpecifier& dirSpec);

	static HX_RESULT RemoveDir(const CHXDirSpecifier& dirSpec);	// deletes an empty directory
	static HX_RESULT RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext = NULL);
	
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
	// these should be implemented on other platforms too, eventually
	static HX_RESULT MakeFileReadOnly(const CHXFileSpecifier& fileSpec);
	static HX_RESULT MakeFileNotReadOnly(const CHXFileSpecifier& fileSpec);
	static HXBOOL IsDiskAudioCD(const CHXDirSpecifier& volSpec);
#endif
        
	static HX_RESULT RenameMoveFile(CHXFileSpecifier& fileSpec, const char* pNewNameIfAny, const CHXDirSpecifier* pNewDirectoryIfAny);
	
	// file read & write
	//
	// on Mac, the file spec may be updated when the file is written out, so it's not a const parameter
	
	static HX_RESULT ReadTextFile(const CHXFileSpecifier& fileSpec, CHXString& outStr);
	static HX_RESULT ReadBinaryFile(const CHXFileSpecifier& fileSpec, IHXBuffer*& pOutBuffer);
	
	static HX_RESULT WriteTextFile(CHXFileSpecifier& fileSpec, const CHXString& inStr, HXBOOL bReplaceExistingFile);
	static HX_RESULT WriteBinaryFile(CHXFileSpecifier& fileSpec, IHXBuffer* inBuffer, HXBOOL bReplaceExistingFile);
	

	// unique file name utilities
	//
	// These returns a file spec with a unique filename in the specified directory
	//
	// pszTemplate is of the form "filenameWILDCARD.ext", with the wildcard to
	//   be replaced by a 1-4 digit number greater than or equal to 2
	//   For example, "MyFile%%.txt" with the wildcard "%%" can become MyFile2.txt
	//
	// GetUniqueFileSpec checks if pszNameFirst is available and uses that; if the name
	//   is not available, it substitutes 2, 3, 4, etc. for the wildcard in the template
	//   to create a unique name.
	//   For example:  pszNameFirst="MyFile.txt" pszTemplate="MyFile_%%.txt" pszWildcard="%%"
	//
	// GetUniqueTempFileSpec tries with a random number initially, and increments that
	//   number until it has a unique name
	//
	// Note: all of these strings should come from resources to enable localizers to
	//       customize the creation of file names
	
	static CHXFileSpecifier GetUniqueFileSpec(const CHXDirSpecifier& locationSpec, 
										const char *pszNameFirst, 
										const char *pszTemplate, const char *pszWildcard);
										
	static CHXFileSpecifier GetUniqueTempFileSpec(const CHXDirSpecifier& locationSpec, 
										const char *pszTemplate, const char *pszWildcard);
	
	// temporary directory
	
	static CHXDirSpecifier GetSystemTempDirectory(void);

	// replace any illegal file/dir name characters; returns true if a change was made
	static HXBOOL MakeNameLegal(char *pszName);


	// application utilities
	
	static CHXFileSpecifier GetCurrentApplication(void);
	static CHXDirSpecifier GetCurrentApplicationDir(void);

	static CHXDirSpecifier GetAppDataDir(const char* szAppName);

	// Macintosh-specific utilities
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
	static FOURCC GetFileType(const CHXFileSpecifier& fileSpec);

	static CHXDirSpecifier MacFindFolder(short vRefNum, FolderType foldType);

	static CHXFileSpecifier SpecifyFileWithMacFindFolder(short vRefNum, FolderType foldType, const char *pszChildFile);
	static CHXDirSpecifier SpecifyFolderWithMacFindFolder(short vRefNum, FolderType foldType, const char *pszChildFolder);
	
	// Resolve will change the specifiers in place if they happened to point
	// to an alias file
	static HX_RESULT ResolveFileSpecifierAlias(CHXFileSpecifier& fileSpec);
	static HX_RESULT ResolveDirSpecifierAlias(CHXDirSpecifier& dirSpec);

	// MoveFileToTrash, MoveFolderToTrash
	
	// These move the file to the trash if possible, or else delete the file.
	
	static HX_RESULT MoveFileToTrash(const CHXFileSpecifier& fileSpec);
	static HX_RESULT MoveFolderToTrash(const CHXDirSpecifier& dirSpec);
	
	// MoveFileToCleaupAtStartup, MoveFolderToCleaupAtStartup
	
	// These move the file to the trash if possible, or else delete the file.
	// Non-const version updates the specifier after the file is moved
	
	static HX_RESULT MoveFileToCleanupAtStartup(const CHXFileSpecifier& fileSpec, HXBOOL bDeleteIfCantMove = TRUE);
	static HX_RESULT MoveFolderToCleanupAtStartup(const CHXDirSpecifier& dirSpec, HXBOOL bDeleteIfCantMove = TRUE);

	static HX_RESULT MoveFileToCleanupAtStartup(CHXFileSpecifier& fileSpec, HXBOOL bDeleteIfCantMove = TRUE);
	static HX_RESULT MoveFolderToCleanupAtStartup(CHXDirSpecifier& dirSpec, HXBOOL bDeleteIfCantMove = TRUE);
	
	// MoveFileToFolderWithRenaming, MoveFolderToFolderWithRenaming
	
	// These move the item to the target folder, renaming the item if necessary. For example,
	// if the file is called "filename" and the destination already has a "filename", the file
	// is renamed "filename_2".
	
	static HX_RESULT MoveFileToFolderWithRenaming(CHXFileSpecifier& fileSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove);
	static HX_RESULT MoveFolderToFolderWithRenaming(CHXDirSpecifier& dirSpec, const CHXDirSpecifier& targetSpec, HXBOOL bDeleteIfCantMove);

	#ifdef _DEBUG
		static void TestMacFileSpecUtils();
	#endif

#endif // _MACINTOSH

};

#endif // FILESPECUTILS_H
