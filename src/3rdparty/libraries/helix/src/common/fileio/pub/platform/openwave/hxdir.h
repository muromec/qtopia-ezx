/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir.h,v 1.4 2005/03/14 19:36:31 bobclark Exp $
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

#ifndef _OPWAVEHXDIR_H
#define _OPWAVEHXDIR_H

#include"hxbasedir.h"

#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"

class CFindFile;

class CHXDirectory : public XHXDirectory
{
public:
    CHXDirectory();
    ~CHXDirectory();

    virtual HXBOOL SetTempPath(HXXHANDLE hpsHandle, const char* szRelPath);

    /* Creates directory. */
    virtual HXBOOL Create();

    /* Checks if directory exists. */    
    virtual HXBOOL IsValid();

    /* Deletes file. */
    virtual HXBOOL DeleteFile(const char* szRelPath);

    /* Sets itself to current directory. */
    virtual HXBOOL SetCurrentDir();

    /* Makes itself a current directory. */
    virtual HXBOOL MakeCurrentDir();

    /* Starts enumeration process. */
    virtual FSOBJ FindFirst(const char* szPattern, char* szPath, UINT16 nSize);

    /* Continues enumeration process. */
    virtual FSOBJ FindNext(char* szPath, UINT16 nSize);

    /* renames a file */
    virtual UINT32 Rename(const char* szOldName, const char* szNewName);


protected:
    HXBOOL IsValidFileDirName(const char* szPath);

    CFindFile* m_pFileFinder;

    /* Deletes empty directory. */
    virtual HXBOOL DeleteDirectory();
};

#endif // _OPWAVEHXDIR_H

