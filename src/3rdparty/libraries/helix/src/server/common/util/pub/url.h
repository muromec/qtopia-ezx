/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: url.h,v 1.9 2004/12/05 05:26:43 seansmith Exp $
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

#ifndef _URL_H_
#define _URL_H_

#include "hlxclib/sys/socket.h"

struct BW_encoding;
class Mstream;
class SIO;
class RA_file;
class Process;
class Config;

class URL
{
public:
                        URL(const char* u, int len);
                        URL(URL*);
                        ~URL();
    int                 error();
    char*               host;
    char*               port;
    sockaddr_in         addr;
    char*               name;
    char*               ext;
    char*               user;
    char*               parameters;
    char*               segment;
    char*               full;
    char*               key;
    int                 len;
    int                 err;
    BW_encoding*        bwe;
    BW_encoding*        found_bwe;
    BW_encoding*        error_bwe;
    UINT32              max_bandwidth;
};

inline int
URL::error() {
    return err;
}

class RA_file;

char*       URL_get_user_path(Process* p, Config* c, char* name);
char*       URL_get_user(const char* url, char* name);
int         URL_encoding_check(RA_file* rafile, URL* url);
RA_file*    URL_open(Process* proc, Config* config, URL* url);
SIO*        URL_eventfile_open(Process* proc, Config* config, URL* url);
RA_file*    URL_error(Process* proc, Config* config, URL* url);
RA_file*    URL_find_bw_file(Process* proc, const char* path, URL* url);
SIO*        URL_find_bw_event_file(Process* proc, const char* path, URL* url);
void        URL_fill_bwe(BW_encoding* bwe, int bw, char* compression_code);
void        URL_copy_bwe(BW_encoding*& to, BW_encoding* from);
int         URL_check_codecs(Process* proc, const char* path, URL* url);

int         check_url(const char* path, int& len);
char*       convert_url(Process* proc, Config* c, char* url, char* path);

#endif/*_URL_H_*/
