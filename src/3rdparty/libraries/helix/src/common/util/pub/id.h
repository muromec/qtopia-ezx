/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: id.h,v 1.8 2004/07/09 18:23:37 hubbe Exp $
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

#ifndef CHXID_H
#define CHXID_H

#include "hlxclib/string.h" //for memset/memcpy

#include "hxassert.h"

/*
 *  The whole table now is zero based and we return id+1 when they ask
 *  for a new id, and return table[id-1] when they want the data.
 *  In this way this class will actually WORK and all of the code using this
 *  can still think a 0 return is bad.
 *                                                 -paulm
 */

class CHXID
{
public:
    enum {
      DEFAULT_VALUE =0,
      TABLE_START_SIZE = 10000
    };

    CHXID(UINT32 size = TABLE_START_SIZE);
    ~CHXID();

    UINT32      create(void* ptr = (void*)DEFAULT_VALUE);
    void*       destroy(UINT32 id);
    void*       get(UINT32 id);
    void        set(UINT32 id, void* ptr);
    UINT32      get_size();
    HX_RESULT   m_LastError;

private:
    UINT32      table_size;
    UINT32      increment_factor;
    UINT32      slots_used;
    UINT32      last_id;

    void**      table_ptr;
};

inline
CHXID::CHXID(UINT32 size) :
    m_LastError(HXR_OK)
{
    ASSERT(size > 0);
    table_size = size;
    increment_factor = (table_size / 2) + 1;
    slots_used = 0;
    last_id    = table_size-1;
    table_ptr = new void* [table_size];
    if(!table_ptr)
    {
        m_LastError = HXR_OUTOFMEMORY;
        return;
    }

    memset (table_ptr, DEFAULT_VALUE, table_size * sizeof(void*));
}

inline
CHXID::~CHXID()
{
    delete [] table_ptr;
}

inline void*
CHXID::get(UINT32 id)
{
// This assert has been disabled to prevent udp resend packets
// from killing the server. If a player has a bug in it and sends
// a corrupted udp_packet it could return an id larger than the
// table_size here. The udp_accept logic needs the DEFAULT_VALUE to
// toss the bad packet.
//    ASSERT(id < table_size);

    id--;
    if (id < table_size)
        return table_ptr[id];
    else
        return (void*)DEFAULT_VALUE;
}

inline void
CHXID::set(UINT32 id, void* ptr)
{
    id--;
    ASSERT(id < table_size);

    if (id < table_size)
        table_ptr[id] = ptr;
}

inline UINT32
CHXID::create(void* ptr)
{
    if (slots_used > table_size * 0.7)
    {
        void** tmp_table_ptr = new void* [table_size + increment_factor];

        memcpy (tmp_table_ptr, table_ptr, table_size * sizeof(void*)); /* Flawfinder: ignore */
        memset (tmp_table_ptr + table_size, DEFAULT_VALUE, increment_factor * sizeof(void*));

        delete [] table_ptr;
        table_ptr = tmp_table_ptr;

        table_size += increment_factor;
        increment_factor = table_size / 2;
    }

    UINT32 new_id = (last_id + 1) % table_size;
    while(table_ptr[new_id] != (void*)DEFAULT_VALUE)
        new_id = (new_id + 1) % table_size;
    last_id = new_id;
    table_ptr[new_id] = ptr;
    slots_used++;

    return new_id+1;
}

inline void*
CHXID::destroy(UINT32 id)
{
    id--;
    ASSERT(id < table_size);

    if (id > table_size)
    {
        return 0;
    }

    void* ptr = table_ptr[id];
    if (ptr == (void*)DEFAULT_VALUE)
        return 0;

    table_ptr[id] = (void*)DEFAULT_VALUE;
    slots_used--;

    return ptr;
}

inline UINT32
CHXID::get_size()
{
    return table_size;
}

#endif
