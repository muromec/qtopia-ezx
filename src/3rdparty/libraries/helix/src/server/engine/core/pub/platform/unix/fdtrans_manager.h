/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fdtrans_manager.h,v 1.4 2004/09/17 23:40:58 tmarshall Exp $
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

#ifndef _FDTRANSMANAGER_H
#define _FDTRANSMANAGER_H

#define MAX_SOCKCBQLEN 1024

class Process;
class SockDispatchCallback;

class SockDispatchCallbackQueue
{
public:
    SockDispatchCallbackQueue(void) : m_len(0) {}

    BOOL IsEmpty(void) { return (m_len == 0); }
    BOOL IsFull(void) { return (m_len >= MAX_SOCKCBQLEN); }
    UINT32 GetCount(void) { return m_len; }
    void Insert(SockDispatchCallback* cb) { m_cb[m_len++] = cb; }
    SockDispatchCallback* Remove(void) { return m_cb[--m_len]; }

private:
    UINT32                      m_len;
    SockDispatchCallback*       m_cb[MAX_SOCKCBQLEN];
};

class FDTransferManager
{
public:
    FDTransferManager() : m_hiproc(0), m_loproc(0xffff) {}
    ~FDTransferManager() { /* XXXTDM: Cleanup? */ }

    void AddToFDBlock(Process*, SockDispatchCallback*, int);
    void AttemptFDBlock(Process* proc);
    void AttemptFDBlock(Process* proc, int procnum);
    UINT32 GetCount(int procnum);

private:
    SockDispatchCallbackQueue   m_cbq[MAX_THREADS];
    int                         m_hiproc;
    int                         m_loproc;
};

#endif
