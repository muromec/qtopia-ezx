/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CHXTSID_H
#define CHXTSID_H

#include "hlxclib/string.h" //for memset/memcpy

#include "hxassert.h"
#include "mutex.h"

/*
 *  The whole table now is zero based and we return id+1 when they ask
 *  for a new id, and return table[id-1] when they want the data.
 *  In this way this class will actually WORK and all of the code using this
 *  can still think a 0 return is bad.
 *                                                 -paulm
 */

class CHXTSID
{
public:
    enum {
      IDTABLE_DEFAULT_VALUE =0,
      IDTABLE_START_SIZE = 10000
    };

    CHXTSID(UINT32 size = IDTABLE_START_SIZE);
    ~CHXTSID();

    UINT32    create(void* ptr = (void*)IDTABLE_DEFAULT_VALUE);
    void*     destroy(UINT32 id);
    void*     get(UINT32 id);
    void      set(UINT32 id, void* ptr);
    UINT32    get_size();
    HX_RESULT m_LastError;

private:
    HX_MUTEX m_pMutex;
    UINT32   table_size;
    UINT32   increment_factor;
    UINT32   slots_used;
    UINT32   last_id;

    void** table_ptr;
};



inline
CHXTSID::CHXTSID(UINT32 size)
: m_LastError(HXR_OK),
  m_pMutex(NULL)
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

    m_pMutex = HXCreateMutex();

    memset (table_ptr, IDTABLE_DEFAULT_VALUE, table_size * sizeof(void*));
}

inline
CHXTSID::~CHXTSID()
{
    HXDestroyMutex(m_pMutex);
    m_pMutex = NULL;

    delete [] table_ptr;
}

inline
void*
CHXTSID::get(UINT32 id)
{
// This assert has been disabled to prevent udp resend packets
// from killing the server. If a player has a bug in it and sends
// a corrupted udp_packet it could return an id larger than the
// table_size here. The udp_accept logic needs the IDTABLE_DEFAULT_VALUE to
// toss the bad packet.
//    ASSERT(id < table_size);

    HXMutexLock(m_pMutex);
    id--;
    if (id < table_size)
    {
        HXMutexUnlock(m_pMutex);
        return table_ptr[id];
    }
    else
    {
        HXMutexUnlock(m_pMutex);
        return (void*)IDTABLE_DEFAULT_VALUE;
    }
}

inline
void
CHXTSID::set(UINT32 id, void* ptr)
{
    HXMutexLock(m_pMutex);
    id--;
    ASSERT(id < table_size);

    if (id < table_size)
        table_ptr[id] = ptr;

    HXMutexUnlock(m_pMutex);
}

inline
UINT32
CHXTSID::create(void* ptr)
{
    HXMutexLock(m_pMutex);
    if (slots_used > table_size * 0.7)
    {
        void** tmp_table_ptr = new void* [table_size + increment_factor];

        memcpy (tmp_table_ptr, table_ptr, table_size * sizeof(void*)); /* Flawfinder: ignore */
        memset (tmp_table_ptr + table_size, IDTABLE_DEFAULT_VALUE, increment_factor * sizeof(void*));

        delete [] table_ptr;
        table_ptr = tmp_table_ptr;

        table_size += increment_factor;
        increment_factor = table_size / 2;
    }

    UINT32 new_id = (last_id + 1) % table_size;
    while(table_ptr[new_id] != (void*)IDTABLE_DEFAULT_VALUE)
        new_id = (new_id + 1) % table_size;
    last_id = new_id;
    table_ptr[new_id] = ptr;
    slots_used++;

    HXMutexUnlock(m_pMutex);

    return new_id+1;
}

inline
void*
CHXTSID::destroy(UINT32 id)
{
    id--;
    ASSERT(id < table_size);

    if (id > table_size)
    {
        return 0;
    }

    HXMutexLock(m_pMutex);
    void* ptr = table_ptr[id];
    if (ptr == (void*)IDTABLE_DEFAULT_VALUE)
    {
        HXMutexUnlock(m_pMutex);
        return 0;
    }

    table_ptr[id] = (void*)IDTABLE_DEFAULT_VALUE;
    ASSERT(slots_used);
    if (slots_used)
    {
        slots_used--;
    }

    HXMutexUnlock(m_pMutex);
    return ptr;
}

inline
UINT32
CHXTSID::get_size()
{
    return table_size;
}

#endif

