/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bufio.h,v 1.5 2007/07/06 20:35:18 jfinnecy Exp $
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


#ifndef _BUFIO_H_
#define _BUFIO_H_

#include <string.h>
#include <errno.h>

#include "bio.h"
#include "fio.h"
#include "buffer.h"
#include "hxassert.h"

class Buffer_IO: public IO 
{
public:
			Buffer_IO(Buffer* buffer, INT32 flags);
			~Buffer_IO();
    virtual INT32	read(void* buf, INT32 size) = 0;
    INT32		write(const void* buf, INT32 size);
    virtual off_t	seek(off_t new_off, INT32 whence) = 0;
    virtual INT32	flush() = 0;
    INT32		close() { return 0; };
    Buffer*		buf;
    INT32		flags();
    INT32		error();
    virtual off_t	file_size();

protected:
    INT32		_flags;
    INT32		err;
};

class PipeBuf_IO : public Buffer_IO 
{
public:
		PipeBuf_IO();
		~PipeBuf_IO();
    INT32	read(void* buf, INT32 size);
    off_t	seek(off_t new_off, INT32 whence);
    INT32	flush();
};

class FileBuf_IO : public Buffer_IO 
{
public:
		FileBuf_IO();
		FileBuf_IO(GrowBuffer* buffer, INT32 o_flags = O_RDWR);
		~FileBuf_IO();
    INT32	read(void* buf, INT32 size);
    off_t	seek(off_t new_off, INT32 whence);
    INT32	flush();

private:
    off_t	offset;
};

inline
Buffer_IO::Buffer_IO(Buffer* buffer, INT32 o_flags) 
{
    buf = buffer;
    _flags = o_flags;
}

inline
Buffer_IO::~Buffer_IO() 
{
    if (buf->refcount == 0)
	delete buf;
}

inline INT32
Buffer_IO::flags()
{
    return _flags;
}

inline INT32
Buffer_IO::error()
{
    return err;
}

inline INT32
Buffer_IO::write(const void* b, INT32 size) 
{
    buf->ensure_space(size);
    memcpy(buf->space, b, size); /* Flawfinder: ignore */
    buf->space += size;
    return size;
}

inline off_t
Buffer_IO::file_size()
{
    ASSERT(1);
    return -1;
}

inline
PipeBuf_IO::PipeBuf_IO() 
    :
    Buffer_IO(new WrapBuffer(), O_RDWR)
{
    buf->init(1024);
}

inline
PipeBuf_IO::~PipeBuf_IO() 
{
    ASSERT(buf->refcount == 0);
}

inline INT32
PipeBuf_IO::read(void* b, INT32 size) 
{
    INT32 count = buf->count();
    if (count < size)
       size = count;
    memcpy(b, buf->data, size); /* Flawfinder: ignore */
    buf->data += size;
    return size;
}

inline off_t
PipeBuf_IO::seek(off_t new_off, INT32 whence) 
{
    return (off_t)-1;
}

inline INT32
PipeBuf_IO::flush() 
{
    buf->data = buf->space = buf->base;
    return 0;
}

inline
FileBuf_IO::FileBuf_IO() 
    :
    Buffer_IO(new GrowBuffer(), O_RDWR)
{
    buf->init(1024);
}

inline
FileBuf_IO::FileBuf_IO(GrowBuffer* buffer, INT32 o_flags) 
    :
    Buffer_IO(buffer, o_flags)
{
    ASSERT(buf);
    buf->refcount++;
    offset = 0;
}

inline
FileBuf_IO::~FileBuf_IO() 
{
    buf->refcount--;
}

inline INT32
FileBuf_IO::read(void* b, INT32 size) 
{
    INT32 count = buf->count() - offset;
    if (count < size)
       size = count;
    memcpy(b, buf->data + offset, size); /* Flawfinder: ignore */
    offset += size;
    return size;
}

inline off_t
FileBuf_IO::seek(off_t new_off, INT32 whence) 
{

    /*
     * XXX...this seek is for read only buffers. Seeking for write buffers
     *       would require using offset for both reading and writing
     */

    ASSERT(_flags == O_RDONLY);

    switch (whence)
    {
	case SEEK_SET:
	    break;
	case SEEK_CUR:
	    new_off += offset;
	    break;
	case SEEK_END:
	    new_off = buf->used();
	    break;
	default:
	    err = EINVAL;
	    return 0;
    }

    Byte* b = buf->base + new_off;

    /*
     * It is not possible to seek past the end of data
     */

    if (b > buf->space)
	return -1;

    offset = new_off;

    return 0;
}

inline INT32
FileBuf_IO::flush() 
{
    /*
     * Does not exist for FileBuf_IO
     */
    return -1;
}


#endif/*_BUFIO_H_*/
