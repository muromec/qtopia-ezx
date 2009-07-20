/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unixdir.cpp,v 1.4 2004/07/09 18:20:04 hubbe Exp $
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
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "hxtypes.h"

#include "hxstrutl.h"

#include "hxdirlist.h"
#include "unixdir.h"

Dir_list* Dir_list::create(const char* path)
{
    return new Unix_dir_list(path);
}

Unix_dir_list::Unix_dir_list(const char* _path)
{
    path = new_string(_path);
    _dir_handle = opendir(path);
    if (_dir_handle == 0) 
    {
	_error = errno;
    }
}

Unix_dir_list::~Unix_dir_list()
{
    delete[] path;
    if (_dir_handle)
    {
    	closedir(_dir_handle);
    }
}

const char* Unix_dir_list::get_next()
{
    struct dirent*  current_entry;

    current_entry = readdir(_dir_handle);
    if (current_entry == 0) 
    {
	_error = errno;
	return 0;
    }
    return current_entry->d_name;
}

int
Unix_dir_list::valid_file_name(const char* name)
{
    if ( (strcmp(name,"..") == 0) || (strcmp(name, ".") == 0) )
    {
	return 0;
    }
    return 1;
}

int
Unix_dir_list::is_empty()
{
    DIR*   		dh;
    struct dirent*	current_entry;

    dh = opendir(path);
    if (dh == 0)
    {
	_error = errno;
	return 1;
    }

    current_entry = readdir(dh);
    while (current_entry != 0)
    {
	if (valid_file_name(current_entry->d_name))
	{
	    closedir(dh);
	    return 0;
	}
    	current_entry = readdir(dh);
    }
    closedir(dh);
    return 1;
}
