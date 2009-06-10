/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fsio.cpp,v 1.13 2008/02/05 06:06:08 vkathuria Exp $
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
#include "hlxclib/stdlib.h"
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxtypes.h"

#include "debug.h"

#include "roundup.h"

#ifndef _MACINTOSH		// if hxheap.h is included here, OpenTransport.h will not compile
#include "hxheap.h"
#endif
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "bio.h"
#include "sio.h"

#include "fsio.h"
#ifndef _BREW
#include "fio.h"
#endif
#include "hxassert.h"

#ifdef _MACINTOSH		// so we include it here
#include "hxheap.h"
#endif

FSIO::FSIO(IO* i, int bsize)
{
    would_block = 0;
    io = i;
    bufsize = bsize;

    flags = (int)(i->flags() & (O_ACCMODE & ~O_BINARY));
    if (flags == (O_RDONLY & ~O_BINARY) || flags == (O_RDWR & ~O_BINARY))
    {
	reader.end = reader.ptr = new Byte[bufsize];
	reader.creg->set_buf(reader.ptr, bufsize);
    }
    if (flags == (O_WRONLY & ~O_BINARY) || flags == (O_RDWR & ~O_BINARY))
    {
	writer.end = writer.ptr = new Byte[bufsize];
	writer.creg->set_buf(writer.ptr, bufsize);
    }
    pathname = 0;
}

FSIO::FSIO(const char* name, int f, mode_t m)
{
    would_block = 0;
    io = new FileIO(name, f, m);
    mode = m;
    flags = f & (O_ACCMODE & ~O_BINARY);
    err = (int)io->error();
    bufsize = 0x1000;
    if (flags == (O_RDONLY & ~O_BINARY) || flags == (O_RDWR & ~O_BINARY))
    {
	reader.end = reader.ptr = new Byte[bufsize];
	reader.creg->set_buf(reader.ptr, bufsize);
    }
    if (flags == (O_WRONLY & ~O_BINARY) || flags == (O_RDWR & ~O_BINARY))
    {
    // N.B. 4096 bytes should be big enough to hold a superblock
	writer.end = writer.ptr = new Byte[bufsize];
	writer.creg->set_buf(writer.ptr, bufsize);
    }
    pathname = new char[strlen(name) + 1];
    strcpy (pathname, name); /* Flawfinder: ignore */
}

FSIO::~FSIO()
{
    /*
     * Delete any buffers we allocated.
     * Regions will be freed by SIO::~SIO().
     */
    if (flags == O_RDONLY || flags == O_RDWR)
    {
	Region* reg = reader.regs;
	while (reg)
	{
	    delete[] reg->base;
	    reg = reg->next;
	}
    }
    if (flags == O_WRONLY || flags == O_RDWR)
    {
	ASSERT(writer.creg->refcount == 0);

	Region* reg = writer.regs;
	while (reg)
	{
	    //DPRINTF(D_INFO, ("deleting write region %p base %p next %p\n", reg, reg->base, reg->next));
	    delete[] reg->base;
	    reg = reg->next;
	}
    }

    if (delete_io)
	delete io;

    if (pathname)
    {
	delete[] pathname;
	pathname = 0;
    }
}

int
FSIO::get_would_block()
{
    return would_block;
}

Byte*
FSIO::_read_alloc(int& size)
{
    int count = _read_count();

    if (count == 0 && (reader.eof || err))
	return 0;

    ASSERT(reader.ptr <= reader.end);

    off_t off = read_offset();

    Byte* b = reader.ptr;
    reader.ptr += size;

    /*
     * if there isn't enough space in the current buffer,
     * update the current region.
     */
    if (reader.ptr > reader.creg->limit)
    {
	int bsize = roundup(size, bufsize);
	Byte* oldbuffer = 0;

	if (reader.creg->refcount != 0)
	{
	    /* creg is in use => allocate a new region */
	    reader.creg->limit = b;
	    reader.creg = new Region(reader.creg);
	    reader.creg->set_buf(new unsigned char[bsize], bsize);
	}
	else if (bsize != bufsize)
	{
	    /* need a bigger buffer => allocate a new buffer */
	    oldbuffer = reader.creg->base;
	    reader.creg->set_buf(new unsigned char[bsize], bsize);
	} /* else reuse existing buffer and region */

	reader.creg->flush_off = reader.creg->off = off;

	/*
	 * if there is unused but read data in the
	 * old buffer, copy it to the new buffer.
	 */
	if (count)
	    memmove(reader.creg->base, b, count);
	if (oldbuffer)
	    delete[] oldbuffer;
	b = reader.creg->base;
	reader.ptr = b + size;
	reader.end = b + count;
    }

    int readcount = (int)io->read(reader.end, _read_space());

    if (readcount <= 0)
    {
	if (readcount == 0)
	    reader.eof = 1;
	else if (io->error() != EWOULDBLOCK)
	{
	    err = (int)io->error();
	    DPRINTF(D_INFO, ("read_alloc error %d count %d\n", err, count));
	}

	/* return if the buffer is empty and we have seen eof or err */
	if (count == 0 && (reader.eof || err))
	{
	    reader.ptr = reader.end;
	    return 0;
	}

    }
    else
	reader.end += readcount;

    /* truncate request if not enough data was read in */
    if (reader.ptr > reader.end)
    {
	size = reader.end - b;
	ASSERT(size >= 0);
	reader.ptr = reader.end;
    }
    reader.creg->refcount++;
    return b;
}

off_t
FSIO::read_seek(off_t new_off, int whence)
{
    // reset error
    err = 0;

    // Compute actual offset
    off_t offset = read_offset();
    switch (whence)
    {
	case SEEK_SET:
	    break;
	case SEEK_CUR:
	    new_off += offset;
	    break;
#if 0 /* XXX PSH - SIO no longer supports file_size */
	case SEEK_END:
	    new_off += file_size();
	    break;
#endif /* 0 */
	default:
	    err = EINVAL;
	    return 0;
    }

    // see if seek is within the current region
    if (reader.creg->off <= new_off && new_off <= offset + _read_count())
    {
	reader.ptr += new_off - offset;
	return new_off;
    }

    offset = new_off;

    // relocate creg
    if (reader.creg->refcount == 0)
    {
	Byte* base = reader.remove(reader.creg);
	if (base)
	    delete[] base;
    }
    else
    {
	reader.creg->limit = reader.end;	// adjust limit
    }

    Region** rp;
    Region *reg = reader.find(new_off, rp);

    if (reg)
    {
	reader.ptr = reg->base + new_off - reg->off;
	// XXX use another field in reg so that we can continue using
	// already allocated space past where we left off. May be.
	reader.end = reg->limit;
	new_off = reg->off + reg->limit - reg->base;
    }
    else
    {
	reg = new Region;
	reg->next = *rp;
	*rp = reg;
	reg->set_buf(new Byte[bufsize], bufsize, new_off);
	reader.ptr = reader.end = reg->base;
    }
    reader.creg = reg;
    reader.eof = 0;
    if (io->seek(new_off, SEEK_SET) == -1)
    {
	err = (int)io->error();
	// XXX clean up creg -- its offset is not valid
	return 0;
    }
    return offset;
}

Byte*
FSIO::read_alloc(int& size, off_t new_off, int whence)
{
    if (read_seek(new_off, whence) == (off_t)-1)
	return 0;
    return SIO::read_alloc(size);
}

void
FSIO::_read_free(Byte* buf)
{
    Region **rp;
    Region* reg = reader.find(buf, rp);

    if (!reg)
	return;

    reg->refcount--;
    if (reg->refcount == 0)
    {
	*rp = reg->next;
	delete[] reg->base;
	delete reg;
    }
}

Byte*
FSIO::_write_alloc(int& size)
{
    int count = _write_count();

    // XXX deal with any outstanding writes
    if (err)
	return 0;

    ASSERT(writer.ptr <= writer.end);

    off_t off = write_offset();

    Byte* b = writer.ptr;
    writer.ptr += size;

    if (writer.ptr > writer.creg->limit)
    {
	int bsize = roundup(size, bufsize);
	Byte* oldbuffer = 0;

	/*
	 * If creg is in use, we must allocate a new region
	 */

	if (writer.creg->refcount != 0)
	{
	    writer.creg->limit = b;
	    writer.creg = new Region(writer.creg);
	    writer.creg->set_buf(new unsigned char[bsize], bsize);
	}
	else if (bsize != bufsize)
	{
	    oldbuffer = writer.creg->base;
	    writer.creg->set_buf(new unsigned char[bsize], bsize);
	} /* else reuse existing buffer and region */

	writer.creg->flush_off = writer.creg->off = off;

	if (count)
	    memmove(writer.creg->base, b, count);
	if (oldbuffer)
	    delete[] oldbuffer;
	b = writer.creg->base;
	writer.ptr = b + size;
	writer.end = b + count;
    }

    if (writer.ptr > writer.end)
	writer.end = writer.ptr;
    writer.creg->refcount++;
    return b;
}

off_t
FSIO::write_seek(off_t new_off, int whence)
{
    /*
     * Before calling write_seek, the program must ensure that all
     * regions have been freed. This may be unnecessarily restrictive,
     * but it makes the routine much easier to code
     */

    if (writer.regs != writer.creg && writer.creg->refcount != 0)
    {
	err = EINVAL;
	return 0;
    }

    // Compute actual offset
    off_t offset = writer.creg->off;

    switch (whence)
    {
	case SEEK_SET:
	    break;
	case SEEK_CUR:
	    new_off += offset;
	    break;
#if 0 /* XXX PSH - sio no longer supports file_size() */
	case SEEK_END:
	    new_off += file_size();
	    break;
#endif /* 0 */
	default:
	    err = EINVAL;
	    return 0;
    }

    // relocate creg

    writer.ptr = writer.end = writer.creg->base;
    write_off = writer.creg->flush_off = writer.creg->off = new_off;

    if (io->seek(new_off, SEEK_SET) == -1)
    {
	err = (int)io->error();
	return 0;
    }

    return new_off;
}

Byte*
FSIO::write_alloc(int& size, off_t seek, int whence)
{
    // XXX this code's been disabled since 1996 so it was removed
    // XXX incomplete
    return 0;
}

int
FSIO::_write_free(Byte* buf)
{
    Region** rp;
    Region* reg = writer.find(buf, rp);

    if (!reg || reg->refcount == 0)
    {
	err = EINVAL;
	return -1;
    }

    reg->refcount--;

    if (reg->refcount == 0)
    {
	if (reg == writer.creg)
	{
	    off_t off = write_offset();

	    writer.creg->limit = writer.end;
	    writer.creg = new Region(writer.creg);
	    writer.creg->set_buf(new unsigned char[bufsize], bufsize, off);
	    writer.ptr = writer.end = writer.creg->base;
	}

	if (_write_flush(reg) < 0)
	{
	    if (err)
		return -1;

	    return 0;
	}

	*rp = reg->next;
	delete[] reg->base;
	delete reg;
    }

    return 0;
}

int
FSIO::_write_flush(Region* reg)
{
    Region* r = reg;

    /*
     * Can only flush if region is unreferenced and everything previous
     * to this region has been written out
     */

    if (r->refcount != 0 || write_off != r->flush_off)
	return -1;

    do
    {
	Region* next;
	int flush_count = HX_SAFEINT(r->flush_off - r->off);
	Byte* b = r->base + flush_count;
	int count = r->limit - r->base - flush_count;

	INT32 writecount = io->write(b, count);
	would_block = 0;

	if (writecount < count)
	{
            if (writecount < 0)
            {
	        if (io->error() != EWOULDBLOCK)
	        {
		    err = (int)io->error();
		    DPRINTF(D_INFO, ("write_flush error %d\n", err));

		    /*
		     * If the current region is not the starting region,
		     * then the next pointer must be updated
		     */

		    if (r != reg)
		        reg->next = r;

		    return -1;
	        }
	        else
	        {
		    would_block = 1;
	        }
            }
            else
            {
		/*
		 * Adjust region by the number of bytes written out
		 */

		r->flush_off += writecount;
		write_off = r->flush_off;

                would_block = 1;
            }


	    /*
	     * If the current region has not been flushed, then return
	     * a fail status
	     */

	    if (r == reg)
		return -1;

	    reg->next = r;
	    return 0;
	}

	write_off = r->flush_off + count;
	next = r->next;
	if (r != reg)
	{
	    /* delete all but the first region */
	    delete[] r->base;
	    delete r;
	}
	r = next;
    } while (r && r->refcount == 0 && r != writer.creg);

    reg->next = r;
    return 0;
}

/*
 * Push back some data in the current buffer.
 * This can be used to initialize the buffer
 * with some data. Use read_undo() or read_realloc()
 * To put back anything read with read_alloc().
 * XXX this is a minimal implementation.
 * Ideally we want a read/writeable object
 * So that read_pushback is not needed.
 */
int
FSIO::read_pushback(Byte* buf, int len)
{
	int space = reader.creg->limit - reader.end;
	if (space < len)
		len = space;
	memcpy(reader.end, buf, len); /* Flawfinder: ignore */
	reader.end += len;
	reader.creg->off += len;
	return len;
}

off_t
FSIO::file_size()
{
    return io->file_size();
}
