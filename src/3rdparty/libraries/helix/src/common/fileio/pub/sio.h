/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sio.h,v 1.6 2005/03/14 19:36:30 bobclark Exp $
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

#ifndef _SIO_H_
#define _SIO_H_

#include "hlxclib/errno.h"

#include "debug.h"
#include "fio.h"

class IO;

class SIO
{
public:
			SIO();
    virtual		~SIO() = 0;
    int			error();
    int			end_of_file();

    Byte*		read_alloc(int& size);
    virtual Byte*	read_alloc(int& size, off_t seek, int whence)=0;
    Byte*		read_realloc(Byte* buf, int oldsize, int& size);
    int			read_undo(Byte* buf, int len);
    void		read_free(Byte* buf);
    off_t		read_offset();
    virtual off_t	read_seek(off_t off, int whence) = 0;
    virtual int		read_pushback(Byte* buf, int len) = 0;
    int			read_count();

    Byte*		write_alloc(int& size);
    virtual Byte*	write_alloc(int& size, off_t seek, int whence)=0;
    Byte*		write_realloc(Byte* buf, int oldsize, int& size);
    int			write_undo(Byte* buf, int len);
    int			write_free(Byte* buf);
    off_t		write_offset();
    virtual off_t	write_seek(off_t off, int whence) = 0;
    int			write_flush();
    int			write_flush_count();
    HXBOOL		write_flush_needed();

    virtual off_t	file_size() = 0;
    void		unset_eof();
    virtual char *	get_pathname() { return NULL;};
    virtual int		get_would_block() {return 0;}
    void		set_delete_io(int flag) { delete_io = flag; }


    struct Region
    {
	Region*		next;
	Region*		prev;
	off_t		off;	    // offset into file
	off_t		flush_off;  // offset into file for writing
	Byte*		base;	    // this is where data starts
	Byte*		limit;	    // this is where data ends
	int		refcount;

			Region();
			Region(Region* link);
	void		set_buf(Byte* b, int size, off_t offset = 0);
    };
protected:
    struct Region_list
    {
	off_t		off;	// current offset into file
	Byte*		ptr;	// ptr to data on next access
	Byte*		end;	// data ends here
	Region*		regs;	// list of regions
	Region*		creg;	// current region
	int		eof;	// 1 if end of file

			Region_list();
			~Region_list();
	Region*		find(Byte* buf, Region**& rp);
	Region*		find(off_t offt, Region**& rp);
	Byte*		remove(Region* reg);
    };
    IO*			io;		// object that does the real io
    int			flags;
    off_t		write_off;	// data written up to this point
    Region_list		reader;
    Region_list		writer;
    int			err;		// last error
    int			delete_io;

    virtual Byte*	_read_alloc(int& size) = 0;
    virtual void	_read_free(Byte* buf) = 0;
    int			_read_count();
    int			_read_space();

    virtual Byte*	_write_alloc(int& size) = 0;
    virtual int		_write_free(Byte* buf) = 0;
    virtual int		_write_flush(Region* reg) = 0;
    int			_write_count();
    int			_write_space();
};

inline
SIO::Region::Region() {
    next = 0;
    prev = 0;
    refcount = 0;
}

inline
SIO::Region::Region(SIO::Region* link) {
    link->next = this;
    prev = link;
    next = 0;
    refcount = 0;
}

inline void
SIO::Region::set_buf(Byte* b, int size, off_t offset) {
    base = b;
    limit = b + size;
    flush_off = off = offset;
}

inline
SIO::Region_list::Region_list() {
    creg = regs = new Region;
    off = 0;
    eof = 0;
}

inline
SIO::Region_list::~Region_list() {
    while (regs) {
	Region* next = regs->next;
	delete regs;
	regs = next;
        if (next) next->prev = 0;
    }
}

inline int
SIO::error() {
    return err;
}

inline int
SIO::end_of_file() {
    return reader.eof;
}

inline
SIO::SIO() {
    err = 0;
    write_off = 0;
    delete_io = 1;
}

inline
SIO::~SIO() {
}

inline int
SIO::_read_count() {
    return reader.end - reader.ptr;
}

inline int
SIO::_read_space() {
    return reader.creg->limit - reader.end;
}

inline off_t
SIO::read_offset() {
    return reader.creg->off + (reader.ptr - reader.creg->base);
}

inline Byte*
SIO::read_alloc(int& size) {

    if (size < 0) {
	DPRINTF(D_ERROR,("illegal read_alloc request %d bytes\n", size));
	err = EINVAL;
	return 0;
    }

    Byte* p = reader.ptr + size;
    if (p <= reader.end) {

	reader.creg->refcount++;
	
	/*
	 * Return the base of the region if the user requests a
	 * read_alloc of 0 bytes. This avoids corrupting the region
	 * list when the corresponding call to read_free is made
	 *
	 * NOTE: read_undo and read_realloc may NOT be performed on
	 *	 the pointer returned
	 */

	if (size == 0)
	    return reader.creg->base;

	Byte* b = reader.ptr;
	reader.ptr = p;
	return b;
    }
    return _read_alloc(size);
}

inline Byte*
SIO::read_realloc(Byte* buf, int oldsize, int& size)
{
    // realloc allowed only on the very last allocation
    if (buf + oldsize != reader.ptr) {
	size = oldsize;
	return buf;
    }

    // shrink case is easy, just unread the bytes beyond size
    if (size <= oldsize) {
	reader.ptr -= oldsize - size;
	reader.eof = 0;
	return buf;
    }

    // essentially do an unread and a new read_alloc
    reader.ptr = buf;
    reader.creg->refcount--;
    return read_alloc(size);
}

inline void
SIO::read_free(Byte* buf)
{
    if (!buf)
	return;

    /*
     * If buf == reader.creg->base then it may have come from a read_alloc
     * of 0 bytes. We must free this pointer even if it has the same address
     * as reader.end
     */

    if (buf == reader.creg->base || (reader.creg->base < buf && buf < reader.end))
    {
	reader.creg->refcount--;
	return;
    }

    DPRINTF(D_ALLOC, ("freeing %p\n", buf));
    _read_free(buf);
}

/*
 * read_undo is same as
 *	buf = read_realloc(buf, len, 0); read_free(buf);
 */
inline int
SIO::read_undo(Byte* buf, int len)
{
    // can not read undo in case of any error
    if (err)
	return -1;

    if (buf + len == reader.ptr) {
	reader.ptr = buf;
	reader.creg->refcount--;
    } else {
	DPRINTF(D_ALLOC, ("freeing %p count %d\n", buf, len));
	_read_free(buf);
    }
    // XXX err may need to be cleared
    reader.eof = 0;
    return 0;
}

inline int
SIO::_write_count() {
    return writer.end - writer.ptr;
}

inline int
SIO::_write_space() {
    return writer.creg->limit - writer.ptr;
}

inline Byte*
SIO::write_alloc(int& size) {
    // XXX this needs to be moved to the non file case.
    // sio is a generic interface and can not assume
    // any specific buffering scheme.
    Byte* p = writer.ptr + size;
    if (p <= writer.creg->limit) {

	writer.creg->refcount++;

	/*
	 * Return the base of the region if the user requests a
	 * write_alloc of 0 bytes. This keeps write_alloc orthogonal
	 * with read_alloc
	 *
	 * NOTE: write_undo and write_realloc may NOT be performed on
	 *	 the pointer returned
	 */

	if (size == 0)
	    return writer.creg->base;

	Byte* b = writer.ptr;
	writer.end = writer.ptr = p;
	return b;
    }
    return _write_alloc(size);
}

inline Byte*
SIO::write_realloc(Byte* buf, int oldsize, int& size)
{
    // XXX this code's been disabled since 1997 so it was removed
    // XXX unsupported
    return 0;
}

inline int
SIO::write_free(Byte* buf)
{
    //
    // let the derived class decide what to do with the data
    //

    return _write_free(buf);
}

/*
 * write_undo is same as
 *	buf = write_realloc(buf, len, 0); write_free(buf);
 */
inline int
SIO::write_undo(Byte* buf, int len)
{
    // XXX this code's been disabled since 1997 so it was removed
    // XXX unsupported
    return -1;
}

inline int
SIO::write_flush()
{
    Region* reg = writer.regs;

    if (reg == writer.creg)
	return 0;

    if (_write_flush(reg) < 0)
    {
	if (err)
	    return -1;

	return 0;
    }

    writer.regs = reg->next;
    if (writer.regs) writer.regs->prev = 0;
    delete[] reg->base;
    delete reg;
    
    return 0;
}

inline off_t
SIO::write_offset() {
    return writer.creg->off + (writer.ptr - writer.creg->base);
}

inline void
SIO::unset_eof() {
    reader.eof = 0;
}

inline int
SIO::read_count() {
    return _read_count();
}
#endif/*_SIO_H*/
