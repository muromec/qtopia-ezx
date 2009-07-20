/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: windir.cpp,v 1.7 2004/07/09 18:20:11 hubbe Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include "hlxclib/errno.h"
#include "hlxclib/io.h"

#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxdirlist.h"

#include "windir.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

Dir_list*
Dir_list::create(const char* path)
{
    return new Windows_dir_list(path);
}

Windows_dir_list::Windows_dir_list(const char* path) :
    _file_handle(INVALID_HANDLE_VALUE),
    _cur_filename(0),
    _next_error(0),
    _path(new_string(path))
{
    char new_path[_MAX_PATH+10]; /* Flawfinder: ignore */

    SafeSprintf(new_path, _MAX_PATH+10, "%s\\*.*", _path);
    
    _file_handle = FindFirstFile(OS_STRING(new_path), &_file_info);

    if (_file_handle == INVALID_HANDLE_VALUE)
	_error = EINVAL;
}

Windows_dir_list::~Windows_dir_list()
{
    delete[] _path;
    delete[] _cur_filename;

    if (_file_handle != INVALID_HANDLE_VALUE)
	FindClose(_file_handle);
}

const char* Windows_dir_list::get_next()
{
    char*   return_value;
    
    if (_next_error != 0)
    {
	_error = _next_error;
	return 0;
    }

    delete [] _cur_filename;
    _cur_filename = new_string(OS_STRING(_file_info.cFileName));

    return_value = _cur_filename;

    if (FindNextFile(_file_handle,&_file_info) == 0) 
	_next_error = ENOENT;

    return return_value;
}

int
Windows_dir_list::valid_file_name(const char* name)
{
    if ( (strcmp(name, "..") == 0) || (strcmp(name, ".") == 0) )
    {
	return 0;
    }
    return 1;
}

int
Windows_dir_list::is_empty()
{
    int isEmpty = 1;
    char		new_path[_MAX_PATH+10]; /* Flawfinder: ignore */
    HANDLE		f_handle = INVALID_HANDLE_VALUE;

    WIN32_FIND_DATA 	f_info;

    SafeSprintf(new_path, _MAX_PATH+10, "%s/*.*", _path);

    f_handle = FindFirstFile(OS_STRING(new_path), &f_info);

    if (f_handle == INVALID_HANDLE_VALUE)
    {
	do
	{
	    if (valid_file_name(OS_STRING(f_info.cFileName)))
		isEmpty = 0;
	}
	while (isEmpty && (FindNextFile(f_handle, &f_info) != 0));
	
    }
    else
	_error = EINVAL;

    FindClose(f_handle);

    return isEmpty;
}


