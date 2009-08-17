/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fio.h,v 1.3 2004/07/09 18:20:23 hubbe Exp $
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

#ifndef _UNIXFIO_H_
#define _UNIXFIO_H_

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bio.h"

#ifdef _AIX
#undef MAXPATH
#endif

class FileIO : public IO
{
public:
	       		FileIO(const char* file, LONG32 flags, 
			       mode_t mode = 0666);
			~FileIO();
    virtual LONG32	close();
    virtual LONG32	read(void* buf, LONG32 size);
    virtual LONG32	write(const void* buf, LONG32 size);
    virtual off_t	seek(off_t off, LONG32 whence);
    virtual LONG32	error();
    virtual LONG32	flags();
    virtual off_t	file_size();

    static void		local_path(char* path);
    static const LONG32	MAXPATH;
    static const char	PATH_SEP;
    static const char* 	NEWLINE; /* Used to have = "\n"; but Irix CC barfed */
    static int		is_directory(char* path);

protected:
    int 	fd;
    LONG32	err;
    LONG32	_flags;
};

inline
FileIO::FileIO(const char* file, LONG32 flags, mode_t mode) 
{
    err = 0;
    fd = -1;
    fd = ::open(file, flags, mode);
    _flags = flags;
    if (fd < 0)
	err = errno;
}

inline LONG32
FileIO::close() 
{
    LONG32 ret = ::close(fd);
    fd = -1;
    if (ret < 0)
	err = errno;
    return ret;
}

inline
FileIO::~FileIO() 
{
    if (fd >= 0)
	close();
    fd = -1;
}

inline off_t
FileIO::seek(off_t off, LONG32 whence) 
{
    off_t ret = ::lseek(fd, off, whence);
    if (ret < 0)
	err = errno;
    return ret;
}

inline LONG32
FileIO::read(void * buf, LONG32 len) 
{
    int ret = ::read(fd, buf, len);
    if (ret < 0)
	err = errno;
    return ret;
}

inline LONG32
FileIO::write(const void * buf, LONG32 len) 
{
    LONG32 ret = ::write(fd, buf, len);
    if (ret < 0)
	err = errno;
    return ret;
}

inline off_t
FileIO::file_size()
{
    struct stat st;
    if (fstat(fd, &st) < 0)
    {
	err = errno;
	return (off_t)-1;
    }
    return st.st_size;
}

inline LONG32
FileIO::error()
{
    return err;
}

inline LONG32
FileIO::flags()
{
    return _flags;
}

inline void
FileIO::local_path(char* path)
{
}

inline int
FileIO::is_directory(char* path)
{
    struct stat st;

    if (stat(path, &st) < 0)
    {
	return 0;
    }

    return st.st_mode & S_IFDIR;
}

#endif /* _UNIXFIO_H_ */
