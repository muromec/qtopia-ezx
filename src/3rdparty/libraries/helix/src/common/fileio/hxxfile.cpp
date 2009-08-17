/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxfile.cpp,v 1.13 2007/01/12 18:04:21 cybette Exp $
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

#include "hxtypes.h"

#include "hxstrutl.h"
#include "hxxfile.h"
#include "hlxclib/string.h"
#include "hlxclib/limits.h"
#include <ctype.h>

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_MAC_UNIX) || defined(_CARBON)
#include "platform/mac/fullpathname.h"
#include "filespecutils.h"	// for an assert in GetReasonableLocalFileName
#endif

#if defined _UNIX && !defined __QNXNTO__
void strlwr(char*);
#endif



/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		GetReasonableLocalFileName()
//
//	Purpose:
//
//		Converts some file URLs to reasonable file names.
//
//	Parameters:
//
//		CHXString& fileName
//		File reference or URL to convert to reasonable local file name.
//
//	Return:
//
//		None.
//
void HXXFile::GetReasonableLocalFileName(CHXString& fileName)
{
#if defined(_MAC_UNIX) || defined(_CARBON)
#ifdef _DEBUG
	CHXString debugSaveURL = (const char*) fileName;
#endif
	
	(void) PathFromURL(fileName, fileName);

#ifdef _DEBUG
	if (!CHXFileSpecUtils::FileExists(fileName))
	{
		CHXString msg;
		msg.Format("GetReasonableLocalFileName is returning invalid path '%s' given URL '%s'",
			 (const char*) fileName, (const char*) debugSaveURL);
		CFStringRef ref = CFStringCreateWithCString(nil, (const char*)msg, kCFStringEncodingUTF8);
		if (ref)
		{
			CFShowStr(ref);
			CFRelease(ref);
		}
	}
#endif // _DEBUG
	return;
	
#endif
	// Trim off any leading and trailing spaces... 
	// Oooh... those pesky QA folks!
	fileName.TrimLeft();
	fileName.TrimRight();

	char szProtocol[6]; /* Flawfinder: ignore */
	strncpy(szProtocol,(const char*)fileName,5); /* Flawfinder: ignore */
	szProtocol[5] = '\0';
	strlwr(szProtocol);
	
	if (strncasecmp(szProtocol,"file:",5) == 0)
	{
		fileName = fileName.Mid(5);

		// Get this, sometimes, Netscape actually puts three whacks!
		// now that's whacky. <g>
		// That's because the third one is the leading slash on
		// an absolute path, don't strip the damn thing off!
#if !defined(_UNIX) && !defined(_OPENWAVE)
		// This first case looks for 4 whacks meaning it is a UNC path on Windows should fail on Mac and UNIX
		if (fileName.Left(4) == "////")
		{
			fileName = fileName.Mid(2);
		}
		// This case occurs if the drive letter is present such as file:///G|/windows/test.rm
		else if (fileName.Left(3) == "///")
		{
			fileName = fileName.Mid(3);
		}
		// trim off those damn double whacks as well
		else 
#endif
		    if (fileName.Left(2) == "//")
		{
			fileName = fileName.Mid(2);
		}
	}


	//////////////////////////////////////////////////////
	// Replace all directory markers with correct platform
	// specific directory markers!
	//
#if defined (_MACINTOSH) || defined (_WINDOWS)
	int nFoundAt = 0;
	do
	{
		nFoundAt = fileName.Find('/');
		if (nFoundAt != -1)
		{
#ifdef _MACINTOSH
			fileName.SetAt(nFoundAt,':');
#else
			fileName.SetAt(nFoundAt,'\\');
#endif
		}

	}
	while(nFoundAt != -1);
#endif

#if defined (_UNIX)
	int nFoundAt = 0;
	do
	{
		nFoundAt = fileName.Find('\\');
		if (nFoundAt != -1)
		{
			fileName.SetAt(nFoundAt,'/');
		}

	}
	while(nFoundAt != -1);
#endif
}

HXBOOL HXXFile::IsPlusURL(const char* pURL)
{
    CHXString	strURL = pURL;
    
    // quick short-circuit: if there's no +, it's not a plus URL
    if (-1 == strURL.Find('+')) return FALSE;

    int		nSep = 0;
    CHXString	strFileName;

    // trim off the options from the URL
    nSep = strURL.ReverseFind('?');
    if (nSep >= 0)
    {
	strURL = strURL.Left(nSep);
    }

    GetReasonableLocalFileName(strURL);

    // trim off relative path if there is
#if defined (_MACINTOSH)
    nSep = strURL.ReverseFind(':');    
#elif defined (_WINDOWS)
    nSep = strURL.ReverseFind('\\');
#else
    nSep = strURL.ReverseFind('/');
#endif

    strFileName = strURL.Right(strURL.GetLength() - nSep - 1);

    // if the '+' is after the '.', then we say this is a plus URL
    int nPlusSign = strFileName.ReverseFind('+');
    if ( (nPlusSign >= 0) && (strFileName.Find('.') >= 0) && (nPlusSign > strFileName.Find('.')) )
    {
	return TRUE;
    }

    return FALSE;
}


ULONG32 HXXFile::GetFileLength(FILE* in)
{
#ifdef _OPENWAVE
    HX_ASSERT(!"HXXFile::GetFileLength() not implemented!");
    return 0;
#else
	ULONG32 was;
	ULONG32	length;

	was = ftell(in);
	fseek(in, 0, 2);
	length = ftell(in);
	fseek(in, was, 0);

	return length;
#endif /* _OPENWAVE */
}

HXBOOL HXXFile::FindAndReplaceInFile
(
	CHXString&			fileNameIn, 
	CHXString& 			fileNameOut, 
	const char*			pFind,
	const char*			pReplace
)
{
#ifdef _OPENWAVE
    HX_ASSERT(!"HXXFile::FindAndReplaceInFile() not implemented!");
    return FALSE;
#else
	CHXString	strFileContents;
	char*		pFileContents = NULL;
	FILE*		fileIn = NULL;
	FILE*		fileOut = NULL;
	HXBOOL		bTheResult = FALSE;
	ULONG32 	length = 0;

	GetReasonableLocalFileName(fileNameIn);
	GetReasonableLocalFileName(fileNameOut);

	// First read the entire file in...
	fileIn=fopen(fileNameIn,"rb");
	if (fileIn==NULL) goto CleanUp;

	// determine the file length
	length = GetFileLength(fileIn);
	
	HX_ASSERT(length < INT_MAX);

	// get a buffer large enough for a zero terminated string.
	pFileContents = strFileContents.GetBuffer((int)(length+1));
	if (!pFileContents) goto CleanUp;

	// actually read the file in...
	fread(pFileContents,1,(int)length,fileIn);
	
	// set last byte to 0 as not to leave trailing characters at the end.
	pFileContents[length]=0;
	
	// We're done with the static buffer...
	strFileContents.ReleaseBuffer();

	// Actually let CHXString do all the work...
	strFileContents.FindAndReplace(pFind,pReplace);

	// After replacing the string, write the entire file out...
	fileOut=fopen(fileNameOut,"wb");
	if (fileOut==NULL) goto CleanUp;
	fwrite((const char *)strFileContents,1,strFileContents.GetLength(),fileOut);

	// If we made it this far then we are cool!
	bTheResult = TRUE;

CleanUp:
	if (fileIn != NULL) fclose(fileIn);
	if (fileOut != NULL) fclose(fileOut);

	return(bTheResult);
#endif /* _OPENWAVE */
}

void HXXFile::ExtractFileAndPath(const char* pFullPath, char* pFileName, UINT32 ulFileNameBufLen,
                                 char* pPath, UINT32 ulPathBufLen)
{
#ifdef _MACINTOSH
	UCHAR delim = ':';
#elif defined(_WINDOWS)
	UCHAR delim = '\\';
#else
	UCHAR delim = '/';
#endif
	CHXString strPath = pFullPath;
	CHXString strFile;

	int nEndOfPath = strPath.ReverseFind(delim);

	if (nEndOfPath != -1)
	{
		strFile = strPath.Mid(nEndOfPath+1);

#ifndef _MACINTOSH
		strPath = strPath.Left(nEndOfPath);
#else
		// on the Mac, let's have the : at the end to ensure
		// it's taken as a directory (otherwise a hard drive name,
		// with no colon, becomes a relative path)
		strPath = strPath.Left(nEndOfPath+1);
#endif
	}
	SafeStrCpy(pPath,strPath, ulPathBufLen);
	SafeStrCpy(pFileName,strFile, ulFileNameBufLen);
}



/////////////////////////////////////////////////////////////////////////////
//
//	Method:
//
//		ConvertHexCodesToChars()
//
//	Purpose:
//
//		Converts a string containing %XX hex character codes into.
//		actual characters.  (For instance, "%20" will be changed to
//		" ".)  This is useful for URLs that contain non-alphanumeric
//		characters that have been converted to hex-codes.
//
//	Parameters:
//
//		CHXString& fileName
//		File reference or URL to convert.
//
//	Return:
//
//		HXBOOL - true if successful, false if bad input or out of mem.
//
HXBOOL HXXFile::ConvertHexCodesToChars(CHXString& fileName)
{
	char* 	c 	= NULL;
        char*   cBase   = NULL;
	char* 	pTemp	= NULL;
	char*	pStop	= NULL;
	HXBOOL 	bOk 	= FALSE;
	long 	lVal	= 0;
	int 	nLen 	= 0;
	char	hex[3]; /* Flawfinder: ignore */
	 
	fileName.TrimLeft();
	fileName.TrimRight();

	// for doing string copies
	nLen = fileName.GetLength();
	pTemp = new char[nLen];
	if (!pTemp)
	{
		return FALSE;
	}
		
	cBase = c = fileName.GetBuffer(0);
	if (!c)
	{
		goto cleanup;
	}
	
	// init our temp array used for hex conversions
	memset(hex, 0, sizeof(char) * 3);
	
	// look for the first hex code
	c = strchr(c, '%');
	while (c)
	{
		// make sure we have enough buffer
		if (c[1] && c[2])
		{
			if (isxdigit(c[1]) && isxdigit(c[2]))
			{
				// hex convert two digits
				strncpy(hex, &(c[1]), 2); /* Flawfinder: ignore */
				lVal = strtol(hex, &pStop, 16);
				
				// replace the '%' with the actual char, then
				// shift the array down
				c[0] = (char)(lVal & 0xFF);
				SafeStrCpy(pTemp, &(c[3]), nLen);
				SafeStrCpy(&c[1], pTemp, nLen - (c+1-cBase));
			}
		}
		else
		{
			// out of chars, so quit
			break;
		}
		
		c++;
		c = strchr(c, '%');
	}
	
cleanup:
	fileName.ReleaseBuffer();

	if (pTemp)
	{
		HX_VECTOR_DELETE(pTemp);
	}
	
	return bOk;
}
