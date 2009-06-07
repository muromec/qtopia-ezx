/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rafile.cpp,v 1.4 2004/06/02 17:18:29 tmarshall Exp $
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


#include <stdio.h>
#include <string.h>
#include "hxtypes.h"

#include "debug.h"

#include "roundup.h"
#include "timeval.h"
#include "hxmarsh.h"
#include "hxstrutl.h"

#include "sockio.h"
#include "bio.h"
#include "sio.h"
#include "fsio.h"
#include "rafile.h"
#include "raff4.h"

#ifdef _MACINTOSH
#include "hxassert.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif


void
RA_file::init_members()
{
    version                             = 0;
    compression                         = 0;
    compression_code                    = 0;
    code_len                            = 0;
    bytes_per_minute                    = 0;
    bytes_per_minute2                   = 0;
    bytes_per_sec                       = (float) 0.0;
    bytes_per_decisec                   = (float) 0.0;
    bytes_per_usec                      = (float) 0.0;
    total_bytes                         = 0;
    granularity                         = 0;
    framesize                           = 0;
    channels                            = 0;
    interleave                          = 0;
    is_interleaved                      = 0;
    use_interleave                      = 0;
    superblocksize                      = 0;
    block_index                         = 0;
    read_count                          = 0;
    block                               = 0;
    header_length                       = 0;
    offset                              = 0;
    live                                = 0;
    chunk_offset                        = 0;
    chunk_size                          = 0;
    name                                = 0;
    mode                                = 0;
    flags                               = 0;
    toc                                 = 0;
    err                                 = 0;
    sio                                 = 0;
    header                              = 0;

}

RA_file::RA_file(char* _name, int _flags, mode_t _mode)
{
    init_members();

    name = new_string(_name);
    flags = _flags;
    mode = _mode;
    sio = new FSIO(name, flags, mode);

    err = sio->error();
    if (err)
    {
        delete sio;
        sio = 0;
        return;
    }
    init();
}

RA_file::RA_file(SIO* sio_file)
{
    init_members();
    sio                 = sio_file;
    init();
}

RA_file::~RA_file()
{
    term();
    HX_VECTOR_DELETE(name);
}

void
RA_file::term()
{
    //XXX Why do we do this?
    //if (!sio)
    //  return;
    HX_VECTOR_DELETE(header);
    HX_VECTOR_DELETE(toc);
    HX_VECTOR_DELETE(block);
    HX_VECTOR_DELETE(compression_code);
    HX_DELETE(sio);
}

int
RA_file::init()
{
    int count;
    Byte* h;
    Byte file_header[16];

    live = 0;
    count = 20;
    channels = 1;
    h = header = sio->read_alloc(count);
    if (!h)
        goto error1;

    if (count < 20  || memcmp(h, ".ra\xFD", 4) != 0)
    {
        header = 0;
        goto error2;
    }

    memcpy(file_header, h, 16);

    h += 4;
    version = HX_SAFEINT(getshort(h));              h += 2;
    if (version == 4)
    {
        // put back all but the first 8 bytes
        int header_size = 8;
        header = sio->read_realloc(header, count, header_size);
        sio->read_free(header);
        header = get_raff4_header();
        if (!header)
            goto error1;
        RAFF4_header* h4 = (RAFF4_header*)header;
        bytes_per_minute = HX_SAFEINT(ntohl(h4->bytes_per_minute2));
        if (!bytes_per_minute)
            bytes_per_minute = HX_SAFEINT(ntohl(h4->bytes_per_minute));
        total_bytes = HX_SAFEINT(ntohl(h4->total_bytes));
        granularity = HX_SAFEINT(ntohl(h4->granularity));
        compression = ntohs(h4->compression_type);
        interleave = ntohs(h4->interleave_factor);
        framesize = ntohs(h4->interleave_block_size);
        channels = ntohs(h4->channels);
        code_len = *h4->compression_code;
        compression_code = new char[code_len];
        memcpy(compression_code, &h4->compression_code[1], code_len);
        is_interleaved = h4->is_interleaved;

        live = h4->stream_type == LIVE_STREAM;
        h = header;
        header = new Byte[header_length + 16];
        memcpy(header, file_header, 16);
        memcpy(header + 16, h, header_length);
        header_length += 16;
    }
    else if (version == 3)
    {
        header_length = HX_SAFEINT(8+getshort(h));  h += 2;
        compression = HX_SAFEINT(getshort(h));      h += 2;
        granularity = HX_SAFEINT(getlong(h));       h += 4;
        bytes_per_minute = HX_SAFEINT(getlong(h));  h += 4;
        total_bytes = HX_SAFEINT(getlong(h));       h += 4;
        header = sio->read_realloc(header, count, header_length);
        is_interleaved = 0;
        interleave = 12;        // XXX
        framesize = 240;        // XXX
        h += *h + 1;            // skip title
        h += *h + 1;            // skip author
        h += *h + 1;            // skip copyright
        live = *h == 4 && strncmp((char*)h+1, "LIVE", *h) == 0;
        h = header;
        header = new Byte[header_length];
        memcpy(header, h, header_length);

        //Put in default compression code
        compression_code = new char[4];
        code_len = 4;
        memcpy(compression_code, "lpcJ", code_len);
#if 0 /* XXX PSH no more file_size */
        next_chunk_offset = sio->file_size();
#endif
    }
    else if (version < 3)
    {
        header_length = 28;
        compression = HX_SAFEINT(getshort(h));      h += 2;
        granularity = HX_SAFEINT(getlong(h));       h += 4;
        bytes_per_minute = HX_SAFEINT(getlong(h));  h += 4;
        total_bytes = HX_SAFEINT(getlong(h));       h += 4;
        is_interleaved = 0;
        interleave = 12;        // XXX
        framesize = 240;        // XXX
        // fake format 3 header
        h = header;
        header = new Byte[header_length];
        memcpy(header, h, 4);
        putshort(header+4, 3);
        putshort(header+6, 20);
        memcpy(header + 8, h + 6, 14);
        memset(header+22, 0, 6);
        //Put in default compression code
        compression_code = new char[4];
        code_len = 4;
        memcpy(compression_code, "lpcJ", code_len);
#if 0 /* XXX PSH no more file_size */
        next_chunk_offset = sio->file_size();
#endif
    }
    else
    {
        h = header;
        header = 0;
        goto error2;
    }

    /*
     * Make sure parameters in the ra file are `reasonable'
     */
    if (0 > interleave || interleave > 16 ||
        0 > granularity || granularity > 1024 ||
        0 > channels || channels > 4 ||
        0 > bytes_per_minute || bytes_per_minute > 48000*2*channels*60 ||
        0 > framesize || framesize > 10240)
    {
#ifdef DEBUG
        fprintf(stderr, "Bad header: ileave: %d gran: %d, channels: %d, "
                "frame: %d, bpm: %d\n", interleave, granularity, channels,
                framesize, bytes_per_minute);
#endif
        goto error2;
    }
    superblocksize = interleave*framesize;
    block_index = interleave;
    bytes_per_sec = (float) (bytes_per_minute / 60.0);
    bytes_per_decisec = (float) (bytes_per_minute / 600.0);
    bytes_per_usec = (float) (bytes_per_minute / 60000000.0);
    use_interleave = is_interleaved;

    // this is where audio starts
    chunk_offset = sio->read_offset();

//      DPRINTF(D_INFO, ("ver: %d, hlen: %d, gran: %d, frame: %d, comp: %d\n",
//              version, header_length, granularity, framesize, compression));
    sio->read_free(h);
    return 0;
error2:
    sio->read_free(h);
error1:
    err = 100;  // XXX fix this
    delete sio;
    sio = 0;
    return 1;
}


Byte*
RA_file::read_header(int& size)
{
    size = header_length;
    return header;
}

void
RA_file::do_deinterleave(Byte* superblock, Byte* data, int count)
{
    //     <------ ig ------>|
    // input:  0,ig,2ig,..(i-1)ig, g,ig+g,2ig+g,..(i-1)ig+g,ig+2g,...
    // output: 0,g,2g,3g,..,ig-g,  ig,ig+g,...
    // k increases by ig or decreases by (i-1)ig-g
    //
    int k = 0;
    for (int j = 0; j < count; j += granularity)
    {
        memcpy(superblock+j, data+k, granularity);
        k += framesize;
        if (k >= superblocksize)
            k -= superblocksize - granularity;
    }
}

void
RA_file::do_interleave(Byte* superblock, Byte* data, int count)
{
    //     <------ ig ------>|
    // input:  0,g,2g,3g,..,ig-g,  ig,ig+g,...
    // output: 0,ig,2ig,..(i-1)ig, g,ig+g,2ig+g,..(i-1)ig+g, 2g,...
    // k increases by ig or decreases by (i-1)ig-g
    //
    int k = 0;
    for (int j = 0; j < count; j += granularity)
    {
        memcpy(superblock+k, data+j, granularity);
        k += framesize;
        if (k >= superblocksize)
            k -= superblocksize - granularity;
    }
}

Byte*
RA_file::read_block(int& size)
{
    if (use_interleave == is_interleaved)
    {
        size = framesize;
        Byte* datablock = sio->read_alloc(size);
        if (datablock && size < framesize && live)
        {
            DPRINTF(D_OFFSET, ("%d bytes read\n", size));
            if (sio->read_undo(datablock, size) < 0)
            {
                DPRINTF(D_INFO, ("read_undo error in read_block 1\n"));
            }
            return 0;
        }
        if (!datablock || sio->read_offset() > next_chunk_offset)
        {
            if (live && !sio->error())
                sio->unset_eof();
            if (datablock)
            {
                sio->read_free(datablock);
                return 0;
            }
            DPRINTF(D_OFFSET, ("no bytes read\n"));
        }
        return datablock;
    }

    /*
     * If interleaved, we grab a super block
     * and ladle out block at a time.
     */
    if (block_index >= interleave)
    {
        read_count = superblocksize;
        Byte* data = sio->read_alloc(read_count);
        if (!data || sio->read_offset() > next_chunk_offset)
        {
            if (data)
            {
                sio->read_free(data);
            }
            if (live)
                sio->unset_eof();
            return 0;
        }

        if (read_count < superblocksize && live)
        {
            if (sio->read_undo(data, read_count) < 0)
            {
                DPRINTF(D_INFO, ("read_undo error in read_block 2\n"));
            }
            return 0;
        }

        if (!block)
            block = new Byte[superblocksize];
        if (is_interleaved)
            do_deinterleave(block, data, read_count);
        else
            do_interleave(block, data, read_count);
        sio->read_free(data);
        block_index = 0;
        block_offset = 0;
    }
    Byte* ptr = block + block_index*framesize;
    size = (read_count - block_offset) / (interleave - block_index++);
    size = roundup(size, granularity);
    if (read_count < superblocksize)
    {
        DPRINTF(D_OFFSET, ("ptr: %p, size: %d\n", ptr, size));
    }
    block_offset += size;
    return ptr;
}

Byte*
RA_file::read_alloc(int& size)
{
    return sio->read_alloc(size);
}

Byte*
RA_file::read_realloc(Byte* buf, int oldsize, int& size)
{
    if (block <= buf && buf < block + superblocksize)
        return 0;
#ifdef DEBUG
    if (block)
        fprintf(stderr, "read_realloc block %p buf %p\n", block, buf);
#endif
    return sio->read_realloc(buf, oldsize, size);
}

void
RA_file::read_free(Byte* buf)
{
    if (block <= buf && buf < block + superblocksize)
        return;
#ifdef DEBUG
    if (block)
        fprintf(stderr, "read_free block %p buf %p\n", block, buf);
#endif
    sio->read_free(buf);
}

void
RA_file::read_undo(Byte* buf, int len)
{
    if (block <= buf && buf < block + superblocksize)
        return;
#ifdef DEBUG
    if (block)
        fprintf(stderr, "read_undo block %p buf %p\n", block, buf);
#endif
    if (sio->read_undo(buf, len) < 0)
    {
        DPRINTF(D_INFO, ("read_undo error in read_undo\n"));
    }
}

off_t
RA_file::read_seek(Timeval t, int whence)
{
    return read_seek(time2off(t), whence);
}

off_t
RA_file::read_seek(off_t off, int whence)
{
    DPRINTF(D_OFFSET, ("Seeking to %ld, whence:%d\n", (long)off, whence));
    switch (whence)
    {
    case SEEK_SET:
#if 0 /* XXX PSH no more file_size */
        if (off > sio->file_size() - chunk_offset)
            off = sio->file_size() - chunk_offset;
#endif
        break;
    case SEEK_END:
#if 0 /* XXX PSH no more file_size */
        off += sio->file_size() - chunk_offset;
#endif
        // XXX deal with non live case
        whence = SEEK_SET;
        break;
    default:
        // XXX SEEK_CUR does not work (and is not used)
        return (off_t)-1;
    }

    if (is_interleaved && version >= 4)
    {
        off /= superblocksize;
        off *= superblocksize;
    }
    else
    {
        off /= granularity;
        off *= granularity;
    }

    if (version <= 3)
        block_index = interleave;

    off += chunk_offset;
    DPRINTF(D_OFFSET, ("Seeking to %ld, whence:%d\n", (long)off, whence));
    return sio->read_seek(off, whence);
}

Byte*
RA_file::write_alloc(int& size)
{
    return 0;
}

void
RA_file::write_free(Byte* buf)
{
}


RA_file::Chunk*
RA_file::get_chunk(off_t off, int whence)
{
    Chunk* headr;
    int len = sizeof *headr;

    headr = (Chunk *)sio->read_alloc(len, off, whence);
    if (!headr)
        return 0;
    if ((size_t)len < sizeof *headr)
    {
        sio->read_free((Byte*)headr);
        return 0;
    }
    return headr;
}

/*
 * Return header if such a chunk exists and position the stream
 * to the beginning of the data area for this chunk.
 */
RA_file::Chunk*
RA_file::find_chunk(UINT32 id)
{
    Chunk* headr;

    // first see if the next chunk is what we want
    headr = get_chunk(0, SEEK_CUR);
    if (!headr)
        return 0;

    if (htonl(id) == headr->id)
        return headr;

    sio->read_free((Byte*)headr);

    // now see if we have a TOC, if so locate the chunk in there
    if (!toc && (toc = get_TOC()) == 0)
        return 0;

    for (TOC* t = toc; t->id != RA_TOC_ID; t++)
        if (t->id == id)
            return get_chunk((off_t)t->offset, SEEK_SET);
    return 0;
}

RA_file::TOC*
RA_file::get_TOC()
{
    if (toc)
        return toc;

    int len = sizeof(TOC);
    TOC *t = (TOC *)sio->read_alloc(len, (off_t)-len, SEEK_END);
    if (!t || len != sizeof *t)
        return 0;

    UINT32 off = ntohl(t->offset);
    int id = HX_SAFEINT(ntohl(t->id));

    sio->read_free((Byte*)t);

    if (id != RA_TOC_ID)
    {
        DPRINTF(D_INFO, ("No TOC, reconstructing it...\n"));
        return reconstruct_TOC();
    }

    Chunk* headr = get_chunk(off, SEEK_SET);
    if (!headr)
    {
        DPRINTF(D_INFO, ("No TOC or a bad TOC\n"));
        return reconstruct_TOC();
    }
    off_t size = ntohl(headr->size);
    id = HX_SAFEINT(ntohl(headr->id));
    sio->read_free((Byte*)headr);
    int count = HX_SAFEINT(size);
    TOC * disktoc = (TOC*)read_alloc(count);
    if (!disktoc || count < size)
    {
#ifdef DEBUG
        fprintf(stderr, "Bad TOC chunk, reconstructing it...\n");
#endif
        if (disktoc)
            read_free((Byte*)disktoc);
        return reconstruct_TOC();
    }

    count /= sizeof(TOC);
    toc = new TOC[count];
    int i;
    for (i = 0, t = disktoc; i < count; i++, t++)
    {
        toc[i].id = ntohl(t->id);
        toc[i].offset = ntohl(t->offset);
    }
    sio->read_free((Byte*)disktoc);
    return toc;
}

RA_file::TOC*
RA_file::reconstruct_TOC()
{
    TOC toc_array[100];
    TOC *t = toc_array;
    UINT32 off = 8;

    for (;;)
    {
        Chunk* headr = get_chunk((off_t)off, SEEK_SET);
        if (!headr)
            break;
        t->id = ntohl(headr->id);
        if (t->id == RA_TOC_ID)
            break;
        t->offset = off;
        t++;
        // XXX expand toc_array if needed
        off += 8 + ntohl(headr->size);
        sio->read_free((Byte*)headr);
    }
    t->id = RA_TOC_ID;
    t->offset = off;
    t++;
    int count = t - &toc_array[0];
    toc = new TOC[count];
    memcpy(toc, toc_array, count*sizeof(TOC));
    return toc;
}

Byte*
RA_file::get_raff4_header()
{
    Chunk* chunk = find_chunk(RA_FORMAT_4_ID);
    if (!chunk)
    {
#ifdef DEBUG
        fprintf(stderr, "No valid format-4 chunk\n");
#endif
        return 0;
    }
    // XXX...Is this correct?
    // chunk is not used past this point so free it
    // XXX

    chunk_size = ntohl(chunk->size);
    next_chunk_offset = sio->read_offset() + chunk_size;
    sio->read_free((Byte*)chunk);

    int size = 20;
    Byte* h = sio->read_alloc(size);
    if (!h || size < 20 )
    {
        if (h)
            sio->read_free(h);
        return 0;
    }
    header_length  = htons(((RAFF4_header*)h)->header_length);
    int newsize = header_length;
    h = sio->read_realloc(h, size, newsize);

    if (!h || newsize < header_length)
    {
        if (h)
            sio->read_free(h);
        return 0;
    }
    return h;
}

//This return seconds.
UINT32
RA_file::duration(INT32 bytes)
{
    if (bytes_per_minute != 0)
    {
        return bytes * 60 / bytes_per_minute;
    }
    return 0;
}

