/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rafile.h,v 1.3 2004/06/02 17:18:29 tmarshall Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#ifndef _RAFILE_H_
#define _RAFILE_H_

#include "sio.h"
#include "timeval.h"

class RA_file {
public:
    struct TOC {
        UINT32        id;
        UINT32        offset;
    };

    struct Chunk {
        UINT32        id;
        UINT32        size;
    };

    int         version;
    int         compression;
    char*       compression_code;
    int         code_len;

    int         bytes_per_minute;
    int         bytes_per_minute2;
    float       bytes_per_sec;
    float       bytes_per_decisec;
    float       bytes_per_usec;

    int         total_bytes;
    int         granularity;
    int         framesize;
    int         channels;

    int         interleave;
    int         is_interleaved;
    int         use_interleave;
    int         superblocksize;
    int         block_index;
    int         block_offset;
    int         read_count;
    Byte*       block;

    int         header_length;
    off_t       offset;
    int         live;
    off_t       chunk_offset;
    UINT32    chunk_size;

    char*       name;
    mode_t      mode;
    int         flags;

                RA_file(char* name, int flags, mode_t mode = 0666);
                RA_file(SIO* io);
                ~RA_file();

    void        init_members();
    int         error();
    int         init();
    void        term();
    UINT16      bw();
    UINT32      bw_kbps();
    int         file_size();
    UINT32      duration(INT32 bytes); // in seconds.

    off_t       read_seek(off_t off, int whence);
    off_t       read_seek(Timeval t, int whence);
    off_t       time2off(Timeval t);
    off_t       decisec2off(int t);
    Timeval     off2time(off_t off);
    UINT32      off2decisec(off_t off);
    UINT32      off2millisec(off_t off);

    Byte*       read_header(int& size);
    Byte*       read_block(int& size);
    Byte*       write_header(int& size);
    void        write_header_done();

    Byte*       read_alloc(int& size);
    Byte*       read_realloc(Byte* buf, int oldsize, int& size);
    void        read_free(Byte* buf);
    void        read_undo(Byte* buf, int len);
    void        unset_eof();

    Byte*       write_alloc(int& size);
    Byte*       write_realloc(Byte* buf, int oldsize, int& size);
    void        write_free(Byte* buf);

    Chunk*      get_chunk(off_t off, int whence);
    Chunk*      find_chunk(UINT32 id);
    Byte*       get_raff4_header();
    TOC*        get_TOC();
    TOC*        reconstruct_TOC();
    TOC*        toc;

    void        do_interleave(Byte* block, Byte* data, int count);
    void        do_deinterleave(Byte* block, Byte* data, int count);
    int         refcount;

private:
    int         err;
    SIO*        sio;
    Byte*       header;
    off_t       next_chunk_offset;
};

inline int
RA_file::error() {
    return err;
}

#if 0 /* XXX PSH - SIO no longer supports file_size */
inline int
RA_file::file_size() {
    return sio->file_size();
}
#endif /* 0 */

inline void
RA_file::unset_eof() {
    sio->unset_eof();
}

inline off_t
RA_file::time2off(Timeval tv) {
    return (off_t)(tv.tv_sec * bytes_per_sec + tv.tv_usec * bytes_per_usec);
}

inline off_t
RA_file::decisec2off(int t) {
    return (off_t)(t * bytes_per_decisec);
}

inline Timeval
RA_file::off2time(off_t off) {
    Timeval tv;
    tv.tv_sec = (INT32)(off/bytes_per_sec);
    tv.tv_usec = (INT32)((off - tv.tv_sec*bytes_per_sec)/bytes_per_usec);
    return tv;
}


inline UINT32
RA_file::off2decisec(off_t off) {
    return (UINT32)(off/bytes_per_decisec);
}

inline UINT32
RA_file::off2millisec(off_t off) {
    return (UINT32)((off/bytes_per_decisec) * 100);
}

inline UINT32
RA_file::bw_kbps()
{
    // Return value in units of kilo bits/sec
    if (!bytes_per_minute)
    {
        return 0;
    }
    // The plus one is to compensate for rounding error.
    return (bytes_per_minute * 8 / 60 / 1000) + 1;
}

inline UINT16
RA_file::bw()
{
    // Return value in units of 100 bytes/sec (the + 5999 generates the same
    //effect as if you called ceil() on it).
    return ((bytes_per_minute+5999) / 60 / 100);
}

#endif/*_RAFILE_H_*/
