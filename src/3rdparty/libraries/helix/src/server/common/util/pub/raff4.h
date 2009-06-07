/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: raff4.h,v 1.3 2004/06/02 17:18:29 tmarshall Exp $
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

#define RA_FORMAT_ID            0x2e7261fd
#define RA_FORMAT_3_ID          0x2e726133
#define RA_FORMAT_4_ID          0x2e726134
#define RA_EVENT_ID             0x45766e74
#define RA_EVENT_CONTAINER_ID   0x45764864
#define RA_TOC_ID               0x52615443

enum { FILE_STREAM, NET_STREAM, LIVE_STREAM };

struct RAFF4_header {                           // offset    sample values
        UINT16         version;                // 0x00         0004
        UINT16         revision;               // 0x02         0000
        UINT16         header_length;          // 0x04         0091
        UINT16         compression_type;       // 0x06         0002
        UINT32        granularity;            // 0x08     00000028
        UINT32        total_bytes;            // 0x0c     0005dc00
        UINT32        bytes_per_minute;       // 0x10     000217b6
        UINT32        bytes_per_minute2;      // 0x14     0001d4bf
        UINT16         interleave_factor;      // 0x18         0010
        UINT16         interleave_block_size;  // 0x1a         00f0
        UINT32        user_data;              // 0x1c     00000000
        float           sample_rate;            // 0x20     1f400000
        UINT16         sample_size;            // 0x24         0010
        UINT16         channels;               // 0x26         0001
        char            interleave_code[5];     // 0x28   04496e7434 [4]Int4
        char            compression_code[5];    // 0x2d   04666d7573 [4]fmus
        char            is_interleaved;         // 0x32           01
        char            copy_byte;              // 0x33           00
        char            stream_type;            // 0x34           00
};

//      char            tlen;                   // 0x35           29
//      char            title[tlen];            // 0x36 Mary had a....
//      char            alen;                   // 0x5f           0e
//      char            author[alen];           // 0x60 ...
//      char            clen;                   // 0x6e           21
//      char            copyright[clen];        // 0x6f
//      char            aplen;                  // 0x90           00
//      char            app[aplen];             // 0x90 ...
// total length ==                      // 0x91
