/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespec.cpp,v 1.7 2007/07/06 20:35:17 jfinnecy Exp $
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

#include "hxstring.h"
#include "filespec.h"
#include "dbcs.h"

#define MAX_PATH_COMPONENTS 256

//////////////////////////////////////////////////////////
//
// Utility base class -- CHXPathParser
//
//////////////////////////////////////////////////////////

CHXPathParser::CHXPathParser()
: m_bEmpty(TRUE)
, m_bIsValid(FALSE)
{
}

CHXPathParser &CHXPathParser::operator=(const CHXPathParser &other)
{
    if(!other.IsSet())
    {
	m_bEmpty = TRUE;
	m_path = "";
    }
    else
	ParsePath(other.m_path);

    return *this;
}

HXBOOL CHXPathParser::operator==(const CHXPathParser &other)
{
    // for now, just returns if the paths are identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return m_path.CompareNoCase(other.m_path) == 0;
}

HXBOOL CHXPathParser::operator!=(const CHXPathParser &other)
{
    // for now, just returns if the paths are not identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return !(*this == other);
}

CHXString CHXPathParser::GetPathName() const
{
    return m_path;
}

HXBOOL CHXPathParser::IsSet() const
{
    return !m_bEmpty;
}

void CHXPathParser::UnSet()
{
	m_path = "";
	m_bEmpty = TRUE;
}

void CHXPathParser::ParsePath(const char *psz)
{
    m_path = psz;

    int i, nStart, nLength;
    int nPathLength = m_path.GetLength();

    m_bIsValid = TRUE; // possibly set to false later in this method
    m_bCannotBeFile = FALSE;
    m_bAbsolute = TRUE;
    m_bEmpty = FALSE;

    m_nVolumeLength = 0;
    m_nParentLength = 0;
    m_nSeparatorLength = 0;
    m_nNameLength = 0;
    m_nExtensionSeparatorLength = 0;
    m_nExtensionLength = 0;

    if(psz == NULL || m_path.IsEmpty())
    {
	m_bIsValid = FALSE;
	m_bCannotBeFile = TRUE;
	m_bEmpty = TRUE;

	return;
    }

    // first, parse the local disk name, if present
    if(nPathLength >= 2 && m_path[1] == ':')
    {
	// local disk/volume
	m_nVolumeLength = 2;
	nStart = 2;
    }
    else
    {
	m_nVolumeLength = 0;
	nStart = 0;
    }

    /*
       now, break the rest of the path up into components based on slashes
       Here is an example of how the path will be broken up.  The carats point
       to the start of each component.  The separator length will be the
       number of slashes following the carat, and the name will be the remaining
       characters in that component.

       \\hostname\bla\bla\bla\file.txt
       ^         ^   ^   ^   ^ 

       In the case of a trailing slash:

       \bla\bla\
       ^   ^   ^
    */

    int anStart[MAX_PATH_COMPONENTS];
    int anSeparatorLen[MAX_PATH_COMPONENTS];
    int anNameLen[MAX_PATH_COMPONENTS];
    int nNumComponents = 0;

    while(nStart < nPathLength && nNumComponents < MAX_PATH_COMPONENTS)
    {
	int nNumSep, nNameLen;

	// count the number of separators starting at position nStart
	for(nNumSep = 0; 
		nStart + nNumSep < nPathLength && 
			(m_path[nStart + nNumSep] == '\\' || 
			 m_path[nStart + nNumSep] == '/');
		nNumSep++);

	// count the number of non-separators starting at position nStart + nNumSep
	for(nNameLen = 0; 
		nStart + nNumSep + nNameLen < nPathLength && 
			(m_path[nStart + nNumSep + nNameLen] != '\\' &&
			 m_path[nStart + nNumSep + nNameLen] != '/');
		nNameLen++)
		if (HXIsLeadByte(m_path[nStart + nNumSep + nNameLen])) nNameLen++;

	anStart[nNumComponents] = nStart;
	anSeparatorLen[nNumComponents] = nNumSep;
	anNameLen[nNumComponents] = nNameLen;

	nNumComponents++;

	nStart += nNumSep + nNameLen;
    }

    // now we are ready to interpret the results
    if(nNumComponents == 0)
    {
	// no components at all.  If there was a volume, then this is a relative
	// path on the volume [and cannot be a file].  If there was no volume, then
	// this is invalid
	if(m_nVolumeLength > 0)
	{
	    m_bAbsolute = FALSE;
	    m_bCannotBeFile = TRUE;
	}
	else
	    m_bIsValid = FALSE;
    }
    else
    {
	// we will now start using nStart as the component number of the start of the actual path [excluding volume]

	nStart = 0;

	// here, we look for a possible \\hostname type of volume
	if(anSeparatorLen[0] > 1)
	{
	    if(m_nVolumeLength > 0)
	    {
		// this catches these invalid paths: C:\\\\temp, C:\\hostname\bla
		// the problem with the second one is that it has both a local and network volume
		// the problem with the first one is it has too many slashes to be an absolute path
		m_bIsValid = FALSE;

		// however, we continue parsing the path, as if this was ok
		nStart = 0;
	    }
	    else if(anSeparatorLen[0] > 2)
	    {
		// this catches these invalid paths: \\\\hostname\bla
		m_bIsValid = FALSE;

		// however, we continue parsing the path, as if this was ok,
		// and if it was not a hostname
		nStart = 0;
	    }
	    else
	    {
		// this catches these *valid* paths: \\hostname\bla
		m_nVolumeLength = anSeparatorLen[0] + anNameLen[0];
		nStart = 1;
	    }
	}


        /*
           now that we have dealt with all possible volumes, nStart points to the first component in the
           actual path.  There are still several possibilities:

           "" [i.e. no more components]
           \
           bla\
           bla.txt
           bla\bla.txt
           \bla\
           \bla.txt
           \bla\bla.txt

           these cannot be files: "", "bla\", "\bla\bla\"
         */

	if(nNumComponents == nStart || anNameLen[nNumComponents - 1] == 0)
	    m_bCannotBeFile = TRUE;

	// these are not valid: "\\\\\file.txt",

	if(nNumComponents > nStart && anSeparatorLen[nStart] > 1)
	    m_bIsValid = FALSE;

	// these are absolute: "\bla\file.txt"
	// there is one special case: the full path "\\hostname" is absolute

	if((nNumComponents > nStart && anSeparatorLen[nStart] >= 1) || m_path[0] == '\\' || m_path[0] == '/')
	    m_bAbsolute = TRUE;
	else
	    m_bAbsolute = FALSE;

	if(nNumComponents == nStart + 1 && anNameLen[nStart] == 0)
	{
	    // the path is simply: "\"
	    m_nParentLength = anSeparatorLen[nStart];
	    nStart = nPathLength;
	    nLength = 0;
	}
	else if(nNumComponents > nStart)
	{
	    // remove the last component if it is just a slash
	    if(nNumComponents > nStart && anNameLen[nNumComponents-1] == 0)
		nNumComponents--;

	    if(nNumComponents == nStart + 1)
	    {
		// just one component
		m_nParentLength = anSeparatorLen[nStart];

		// back to using nStart as an index into the path.
		// note that we must set nLength first, since it
		// uses nStart as the index
		nLength = anNameLen[nStart];
		nStart = anStart[nStart] + anSeparatorLen[nStart];
	    }
	    else
	    {
		// more than one component
		m_nParentLength = anStart[nNumComponents - 1] - m_nVolumeLength;
		m_nSeparatorLength = anSeparatorLen[nNumComponents - 1];

		// back to using nStart as an index into the path
		nLength = anNameLen[nNumComponents - 1];
		nStart = anStart[nNumComponents - 1] + anSeparatorLen[nNumComponents - 1];
	    }
	}
	else
	{
	    // no more components
	    nStart = nPathLength;
	    nLength = 0;
	}

	// now, use nStart and nLength as the portion of the path to look at for the name
	// now we break it up into name, extension separator, and extension
	for(i = nStart + nLength - 1; i >= nStart; i--)
	    if(m_path[i] == '.')
		break;

	if(i < nStart)
	    m_nNameLength = nLength;
	else
	{
	    m_nNameLength = i - nStart;
	    m_nExtensionSeparatorLength = 1;
	    m_nExtensionLength = nStart + nLength - i - 1;
	}
    }
}

CHXString CHXPathParser::GetSubstring(int nStart, int nLength) const
{
    if(nLength == -1)
	nLength = m_path.GetLength() - nStart;
    return m_path.Mid(nStart, nLength);
}

CHXString CHXPathParser::GetPersistentString() const
{
    return m_path;
}

void CHXPathParser::SetFromPersistentString(const char *pBuffer)
{
    ParsePath(pBuffer);
}

//////////////////////////////////////////////////////////
//
// CHXFileSpecifier
//
//////////////////////////////////////////////////////////

CHXFileSpecifier::CHXFileSpecifier()
{
}

CHXFileSpecifier::CHXFileSpecifier(const char* psz)
{
    m_parser.ParsePath(psz);
}

CHXFileSpecifier::CHXFileSpecifier(const CHXString &str)
{
    m_parser.ParsePath(str);
}

CHXFileSpecifier::CHXFileSpecifier(const CHXFileSpecifier &other)
{
    *this = other;
}

CHXFileSpecifier::~CHXFileSpecifier()
{
}

CHXFileSpecifier &CHXFileSpecifier::operator=(const CHXFileSpecifier &other)
{
    m_parser = other.m_parser;

    return *this;
}

CHXFileSpecifier& CHXFileSpecifier::operator=(const char* sPath)
{
	m_parser.ParsePath(sPath);
	return *this;
}

CHXFileSpecifier& CHXFileSpecifier::operator=(const CHXString& sPath)
{
	m_parser.ParsePath(sPath);
	return *this;
}

HXBOOL CHXFileSpecifier::operator==(const CHXFileSpecifier &other)
{
    // for now, just returns if the paths are identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return m_parser == other.m_parser;
}

HXBOOL CHXFileSpecifier::operator!=(const CHXFileSpecifier &other)
{
    // for now, just returns if the paths are not identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return m_parser != other.m_parser;
}

CHXString CHXFileSpecifier::GetPathName() const
{
    return m_parser.GetPathName();
}

CHXString CHXFileSpecifier::GetURL() const
{

    const HXBOOL kReplaceAll = TRUE;
    
    CHXString strPath, strURL;
    
    strPath = m_parser.GetPathName();

    if (strPath.GetLength() > 0)
    {
        strPath.FindAndReplace("\\", "/", kReplaceAll);	// replace path separators

        strURL = "file://";
	    strURL += strPath;
    }
    
    return strURL;
}

HXBOOL CHXFileSpecifier::IsSet() const
{
    return m_parser.IsSet();
}

CHXDirSpecifier CHXFileSpecifier::GetParentDirectory() const
{
    if(IsSet())
	return m_parser.GetSubstring(0, m_parser.m_nVolumeLength + m_parser.m_nParentLength);
    else
	return CHXDirSpecifier();
}

CHXDirSpecifier CHXFileSpecifier::GetVolume() const
{
    if(IsSet())
	return m_parser.GetSubstring(0, m_parser.m_nVolumeLength);
    else
	return CHXDirSpecifier();
}

CHXString CHXFileSpecifier::GetName() const
{
    if(IsSet())
	return m_parser.GetSubstring(m_parser.m_nVolumeLength + m_parser.m_nParentLength + m_parser.m_nSeparatorLength,
			    m_parser.m_nNameLength + m_parser.m_nExtensionSeparatorLength + m_parser.m_nExtensionLength);
    else
	return "";
}

CHXString CHXFileSpecifier::GetTitle() const
{
    if(IsSet())
	return m_parser.GetSubstring(m_parser.m_nVolumeLength + m_parser.m_nParentLength + m_parser.m_nSeparatorLength,
			    m_parser.m_nNameLength);
    else
	return "";
}

CHXString CHXFileSpecifier::GetExtension() const
{
    if(IsSet())
	return m_parser.GetSubstring(m_parser.m_nVolumeLength + m_parser.m_nParentLength + m_parser.m_nSeparatorLength +
			    m_parser.m_nNameLength + m_parser.m_nExtensionSeparatorLength,-1);
    else
	return "";
}

CHXString CHXFileSpecifier::GetPersistentString() const
{
    return m_parser.GetPersistentString();
}

HX_RESULT CHXFileSpecifier::SetFromPersistentString(const char *pBuffer)
{
    m_parser.SetFromPersistentString(pBuffer);
	return HXR_OK;

}

void CHXFileSpecifier::Unset()
{
	m_parser.UnSet();
}

HX_RESULT CHXFileSpecifier::SetFromURL(const char *pBuffer)
{
	CHXString 	strURL, strChoppedURL;
	int			nChop;
	int			nLength;
    const HXBOOL kReplaceAll = TRUE;
	
	Unset();
	
	strURL = pBuffer;
	
	nLength = strURL.GetLength();
	nChop = 0;
	
	if (strURL.Left(9) == "file:////")		nChop = 7;
	else if (strURL.Left(8) == "file:///")		nChop = 8;
	else if (strURL.Left(7) == "file://")	nChop = 7;
	else if (strURL.Left(5) == "file:")		nChop = 5;
	else if (strURL.Left(10) == "tfile:////")		nChop = 8;
	else if (strURL.Left(9) == "tfile:///")		nChop = 9;
	else if (strURL.Left(8) == "tfile://")	nChop = 8;
	else if (strURL.Left(6) == "tfile:")		nChop = 6;
	
	if (nChop > 0)
	{
		strChoppedURL = strURL.Right(nLength - nChop);
		strChoppedURL.FindAndReplace("|", ":", kReplaceAll);	
		strChoppedURL.FindAndReplace("/", "\\", kReplaceAll);	
	    m_parser.ParsePath(strChoppedURL);
	}
	
	return IsSet() ? HXR_OK : HXR_FAILED;
}

//////////////////////////////////////////////////////////
//
// CHXDirSpecifier
//
//////////////////////////////////////////////////////////

CHXDirSpecifier::CHXDirSpecifier()
{
}

CHXDirSpecifier::CHXDirSpecifier(const char* psz)
{
    m_parser.ParsePath(psz);
}

CHXDirSpecifier::CHXDirSpecifier(const CHXString &str)
{
    m_parser.ParsePath(str);
}

CHXDirSpecifier::CHXDirSpecifier(const CHXDirSpecifier &other)
{
    *this = other;
}

CHXDirSpecifier::~CHXDirSpecifier()
{
}

CHXDirSpecifier &CHXDirSpecifier::operator=(const CHXDirSpecifier &other)
{
    m_parser = other.m_parser;

    return *this;
}

CHXDirSpecifier& CHXDirSpecifier::operator=(const char* sPath)
{
	m_parser.ParsePath(sPath);
	return *this;
}

CHXDirSpecifier& CHXDirSpecifier::operator=(const CHXString& sPath)
{
	m_parser.ParsePath(sPath);
	return *this;
}

HXBOOL CHXDirSpecifier::operator==(const CHXDirSpecifier &other)
{
    // for now, just returns if the paths are identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return m_parser == other.m_parser;
}

HXBOOL CHXDirSpecifier::operator!=(const CHXDirSpecifier &other)
{
    // for now, just returns if the paths are not identical
    // in the future, should normalize paths (i.e. remove duplicate slashes, resolve ..\..\.. type issues
    return m_parser != other.m_parser;
}

CHXString CHXDirSpecifier::GetPathName() const
{
    return m_parser.GetPathName();
}

HXBOOL CHXDirSpecifier::IsSet() const
{
    return m_parser.IsSet();
}

HXBOOL CHXDirSpecifier::IsVolume() const
{
    return (IsSet() ? (m_parser.m_path.GetLength() == (UINT32)m_parser.m_nVolumeLength) : FALSE);
}

CHXString CHXDirSpecifier::GetName() const
{
    if(IsSet())
	return m_parser.GetSubstring(m_parser.m_nVolumeLength + m_parser.m_nParentLength + m_parser.m_nSeparatorLength,
			    m_parser.m_nNameLength + m_parser.m_nExtensionSeparatorLength + m_parser.m_nExtensionLength);
    else
	return "";
}

CHXDirSpecifier CHXDirSpecifier::GetParentDirectory() const
{
    if(IsSet())
	return m_parser.GetSubstring(0, m_parser.m_nVolumeLength + m_parser.m_nParentLength);
    else
	return CHXDirSpecifier();
}

CHXDirSpecifier CHXDirSpecifier::GetVolume() const
{
    if(IsSet())
	return m_parser.GetSubstring(0, m_parser.m_nVolumeLength);
    else
	return CHXDirSpecifier();
}

CHXFileSpecifier CHXDirSpecifier::SpecifyChildFile(const char *child) const
{
    if(IsSet())
    {
	int nlast_ch = m_parser.m_path.GetLength() - 1;

	if(m_parser.m_path.ReverseFind('\\') == nlast_ch || m_parser.m_path.ReverseFind('/') == nlast_ch)
	    return m_parser.m_path + child;
	else
	    return m_parser.m_path + '\\' + child;
    }
    else
	return child;
}

CHXDirSpecifier CHXDirSpecifier::SpecifyChildDirectory(const char *child) const
{
    if(IsSet())
    {
	int nlast_ch = m_parser.m_path.GetLength() - 1;

	if(m_parser.m_path.ReverseFind('\\') == nlast_ch || m_parser.m_path.ReverseFind('/') == nlast_ch)
	    return m_parser.m_path + child;
	else
	    return m_parser.m_path + '\\' + child;
    }
    else
	return child;
}

CHXString CHXDirSpecifier::GetPersistentString() const
{
    return m_parser.GetPersistentString();
}

HX_RESULT CHXDirSpecifier::SetFromPersistentString(const char *pBuffer)
{
    m_parser.SetFromPersistentString(pBuffer);
	return HXR_OK;
}


#if 0
Utility class:
might be things like
	IsLocal()	is this on a server or a local volume
	Rename()
	GetFilesInDirectory()  (gets a list into a buffer; much better than an iterator)
	GetFileType()
	GetFileModificationDate()
	GetFileSize()
#endif // 0

