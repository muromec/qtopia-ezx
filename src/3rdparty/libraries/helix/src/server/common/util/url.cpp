/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: url.cpp,v 1.20 2008/07/03 21:54:17 dcollins Exp $
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

#include <stdlib.h>
#include <stdio.h>

#ifdef _UNIX
#include <pwd.h>
#endif /* _UNIX */
#if defined(_LSB)
#include <arpa/inet.h>
#endif

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "debug.h"
#include "fio.h"
#include "fsio.h"
#include "hxdirlist.h"
#include "hxstrutl.h"
#include "proc.h"
#include "rafile.h"
#include "bwe.h"
#include "srcerrs.h"
#include "config.h"
#include "url.h"
#include "base_errmsg.h"

/*
 * Syntax:
 *      url: local | full
 *      local: path ["?" parameters] ["/trackID=2"]
 *      full: "pnm://" | "rtsp://" host [":" port] "/" url
 */
URL::URL(const char* _url, int _url_len)
: host(NULL)
, full(NULL)
, port(NULL)
, name(NULL)
, ext(NULL)
, user(NULL)
, parameters(NULL)
, segment(NULL)
, key(NULL)
, len(_url_len)
, err(0)
, bwe(NULL)
, found_bwe(NULL)
, error_bwe(NULL)
, max_bandwidth(0)
{
    /*
     *  Create a nul terminated copy and avoid using the original.
     */
    int path_len = _url_len;
    int key_len = path_len;
    full = new_string(_url, _url_len);
    char* dot = 0;
    char* temp = 0;
    char* segment_eq = 0;
    char* pszResource = NULL;

    int urllen = _url_len;
    const char* pHost;
    const char* pPort;
    const char* pResource = full;

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;

    if (path_len > 7 && strncasecmp(full, "rtsp://", 7) == 0)
    {
        pHost = full + 7;
        pResource = strchr(pHost, '/');
        int hostlen = (pResource) ? pResource - pHost : urllen - 7;

        if (hostlen)
        {
            if (pHost[0] == '[')
            {
                // Looks like an IPv6 address
                pPort = strchr(pHost, ']');
                if (!pPort || pPort - pHost > hostlen-1)
                {
                    // No closing brace in server name
                    goto error_return;                
                }
                pPort = (pPort[1] == ':') ? &pPort[1] : NULL;
            }
            else
            {
                // Handle as a non-IPv6 address
                pPort = strchr(pHost, ':');
                if (!pPort || pPort - pHost > hostlen-1)
                {
                    // Port wasn't specified
                    pPort = NULL;                
                
                }
                else
                {
                    if (pPort - pHost < hostlen-1)
                    {
                        temp = (char*)strchr(pPort+1, ':');
                        if (temp && temp - pHost <= hostlen-1)
                        {
                            // Looks like IPv6 without square braces
                            goto error_return;
                        }
                    }
                }
            }
        }
        else
        {
            goto error_return;
        }

        int portlen;
        if (pPort)
        {
            portlen = hostlen - (pPort - pHost) - 1;
            hostlen = hostlen - portlen - 1;
            port = new_string(pPort+1, portlen);
        }
        else
        {
            port = new_string("554", 3);
        }

        char* rest;
        addr.sin_port = htons((UINT16)strtol(port, &rest, 10));

        host = new_string(pHost, hostlen);
    }

    if (pResource)
    {
        /*
         * path now points to right after the (optional) server spec.
         * Skip any leading slashes and `.' in the path
         */
        for (;;)
        {
            while (*pResource == '/')
                pResource++;
            if (pResource[0] != '.' || pResource[1] != '/')
                break;
            pResource += 2;
        }
    }

    // the checking of *pResource is to handle "rtsp://server:port/" correctly, 
    // which is a valid url, and should be treated the same way as "rtsp://server:port"
    if(pResource && *pResource)
    {
        path_len -= pResource - full;
        int path_param_len = path_len; // Length of path + "?" + parameters if any.

        if(!path_len)
        {
            goto error_return;
        }

        /*
         * Check that path does not go `above' the root
         */
        if (check_url(pResource, path_len) < 0)
            goto error_return;

        /*
         * If this is a local path (i.e. no server spec),
         * check for and separate ~user if any.
         */

        if (!host)
        {
            if (*pResource == '~')
            {
                char* p = (char*)memchr(pResource, '/', path_len);
                if (!p)
                    goto error_return;

                int user_len = p - pResource;
                user = new_string(pResource, user_len);
            }
        }

        // remember everything after (pnm/rtsp://<host>:<port>/)
        pszResource = new_string(pResource, strlen(pResource));

        /*
         * Separate parameters if any
         */
        parameters = (char*)memchr(pResource, '?', path_len);
        if (parameters)
        {
            path_len--; // strip the '?'.
            int param_len = path_param_len - path_len; 
            parameters = new_string(parameters, param_len);
            key_len -= param_len;
            
        }

        // If there is no path length after separating the parameters,
        // then the URL was JUST parameters, and is not valid!
        if(!path_len)
        {
            goto error_return;
        }

        /*
         * Get segment if any
         */
        segment_eq = (char*)memchr(pResource, '=', path_len);
        if (segment_eq)
        {
            // find beginning of segment
            char* p_current = segment_eq;
            int plen;
            while(p_current != pResource)
            {
                p_current--;
                if(*p_current == '/')
                {
                    plen = path_len - (p_current - pResource);
                    segment = p_current + 1;
                    key_len -= plen;
                    path_len -= plen;
                }
            }
            segment = segment ? new_string(segment, plen) : NULL;
        }
        else
        {
            if(pResource[path_len-1] == '/')
            {
                key_len -= 1;
                path_len -= 1;
            }
        }

        name = new_string(pResource, path_len);

        temp = name;
        while (*temp)
        {
            if (*temp == '.')
                dot = temp;
            temp++;
        }

        if (!dot)
        {
            ext = new_string("");
            //goto error_return; XXXJR - Who said URLs have to have extensions??
        }
        else
        {
            dot++;
            ext = new_string(dot);
        }

        key = new_string(full, key_len);
        HX_VECTOR_DELETE(full);
        full = pszResource;
    }
    else
    {
        key = new_string(full, key_len);
        name = new_string("");
    }

    err = 0;

    DPRINTF(D_INFO,
    ("URL: path:%s key:%s name:%s user:%s host:%s parameters:%s segment:%s\n",
            full ? full : "", key ? key : "", name ? name : "",
            user ? user : "", host ? host : "",
            parameters ? parameters : "", segment ? segment : ""));
    return;

error_return:
    HX_VECTOR_DELETE(pszResource);
    HX_VECTOR_DELETE(parameters);
    HX_VECTOR_DELETE(segment);
    HX_VECTOR_DELETE(host);
    HX_VECTOR_DELETE(user);
    HX_VECTOR_DELETE(name);
    name = new_string("");
    HX_VECTOR_DELETE(ext);
    ext = new_string("");
    HX_VECTOR_DELETE(key);
    key = new_string("");
    err = SE_INVALID_PATH;
#if 0
    DPRINTF(D_INFO, ("URL: failed on:%s\n", _url ? _url : ""));
#endif
}

URL::URL(URL* url)
{
    host = url->host? new_string(url->host) : 0;
    port = url->port? new_string(url->port) : 0;
    addr = url->addr;
    name = url->name ? new_string(url->name) : 0;
    ext  = url->ext  ? new_string(url->ext) : 0;
    user = url->user ? new_string(url->user) : 0;
    parameters = url->parameters ? new_string(url->parameters) : 0;
    segment = url->segment ? new_string(url->segment) : 0;
    full = url->full ? new_string(url->full) : 0;
    key = url->key ? new_string(url->key) : 0;
    len = url->len;
    err = url->err;
    max_bandwidth = url->max_bandwidth;
    URL_copy_bwe(bwe, url->bwe);
    found_bwe = url->found_bwe;
    URL_copy_bwe(error_bwe, url->error_bwe);
}

URL::~URL()
{
    HX_VECTOR_DELETE(full);
    HX_VECTOR_DELETE(port);
    HX_VECTOR_DELETE(host);
    HX_VECTOR_DELETE(name);
    HX_VECTOR_DELETE(ext);
    HX_VECTOR_DELETE(user);
    HX_VECTOR_DELETE(key);
    HX_VECTOR_DELETE(parameters);
    HX_VECTOR_DELETE(segment);
    HX_VECTOR_DELETE(bwe);
    HX_VECTOR_DELETE(error_bwe);
}

#if XXXSMP
int
URL_check(char* url)
{
        char* cp = url;

        if (*cp == '~') {
                if (cp = strchr(cp, '/')) {
                        cp++;
                }
                else {
                        return 0;
                }
        }

        int level = 0;
        char* np = strchr(cp, '/');

        while (*cp) {

                //
                // If there are no more '/', then just test the last part of the
                // url
                //

                if (np == 0) {
                        if (strlen(cp) == 2 && !memcmp(cp, "..", 2)) {
                                level--;
                        }
                        else {
                                level++;
                        }
                        break;
                }


                //
                // (np - cp) is the part of the url to be tested. If (np - cp) == 0
                // then *cp == '/', and the level count is unaffected
                //

                if ((np - cp) == 2 && !memcmp(cp, "..", 2)) {
                        if (level == 0) {
                                return -1;
                        }
                        level--;
                }
                else if ((np - cp) > 0) {
                        level++;
                }

                cp = np + 1;
                np = strchr(cp, '/');
        }

        return level;
}
#endif

#ifdef XXXJR
char*
URL_get_user_path(Process* p, Config* c, char* name)
{
        Dict_entry* user_entry;

        user_entry = c->user_list()->find(name);
        if (user_entry) {
                return ((User_info*)user_entry->obj)->base_path;
        }
#ifdef unix
        else {
                struct passwd* pw;
                pw = getpwnam(name);
                if(!pw) {
                        return 0;
                }
                return pw->pw_dir;
        }
#endif /* unix */
        return 0;
}
#endif

char*
URL_get_user(const char* url, char* name)
{
#ifdef _AIX
#undef MAXPATH
#endif
        const char* user_start;
        int         user_end;

        user_start = strchr(url,'~');
        if (user_start == 0) {
                name[0] = 0;
                return 0;
        }
        user_start++;
        user_end = strcspn(user_start,"/");
        if (user_end > FileIO::MAXPATH) {
                name[0] = 0;
                return 0;
        }
        strncpy(name,user_start,user_end);
        name[user_end] = 0;

        return name;
}

int
URL_encoding_check(RA_file* rafile, URL* url)
{
    BW_encoding* bwe = url->bwe;
    int          bw_is_acceptable = 0;
    UINT16      player_bandwidth;

    //
    // If no encoding list is provided then the encoding check is
    // assumed to have passed
    //

    if (!bwe) {
        return 1;
    }

    if (rafile->version < 4) {

        //
        // Encoding assumed to be 14.4
        //

        while (bwe->bw) {
            if (bwe->bw <= 18 && memcmp(bwe->comp, "lpcJ", 4) == 0) {
                return 1;
            }
            bwe++;
        }
    }
    else {
        player_bandwidth = (UINT16) (url->max_bandwidth / 800);
        while (bwe->bw)
        {
            if (player_bandwidth >= rafile->bw())
            {
                if (memcmp(bwe->comp, rafile->compression_code,
                           rafile->code_len) == 0)
                {
                    return 1;
                }
                else
                {
                    bw_is_acceptable = 1;
                }
            }
            bwe++;
        }
    }
    if (bw_is_acceptable)
    {
        url->err = SE_NO_CODEC;
    }
    else
    {
        url->err = SE_INVALID_BANDWIDTH;
    }

    return 0;
}

RA_file*
URL_find_bw_file(Process* proc, const char* path, URL* url)
{
#ifdef _AIX
#undef MAXPATH
#endif
    BW_encoding* bwe = url->bwe;
    char* file = new char[FileIO::MAXPATH];

    while (bwe->bw)
    {
        RA_file* rafile;
        sprintf(file, "%s%c%*.*s.%d", path, FileIO::PATH_SEP, 4, 4, bwe->file,
                bwe->bw);
        rafile = new RA_file(file, O_RDONLY);
        if (!rafile->error())
        {
            url->found_bwe = bwe;
            return rafile;
        }
        delete rafile;
        bwe++;
    }
    url->err = SE_INVALID_BANDWIDTH;
    URL_check_codecs(proc, path, url);
    HX_VECTOR_DELETE(file);
    return 0;
}

//
// Bandwidth event files is an ENSO addition
//

SIO*
URL_find_bw_event_file(Process* proc, const char* path, URL* url)
{
#ifdef _AIX
#undef MAXPATH
#endif
    BW_encoding* bwe = url->found_bwe;
    char* file = new char[FileIO::MAXPATH];

    sprintf(file,"%s%c%*.*s.%d.rae", path, FileIO::PATH_SEP, 4, 4, bwe->file, bwe->bw);
    SIO* event_file = new FSIO(file, O_RDONLY);
    if (event_file->error() == 0)
        return event_file;
    delete event_file;
    HX_VECTOR_DELETE(file);
    return 0;
}

RA_file*
URL_error(Process* proc, Config* config, URL* url)
{
#if 0
    const char* pValue = config->GetString(proc, "config.DefaultErrorFile");

    if (pValue)
    {
        int error_file_len = strlen(pValue);
        URL* error_url = new URL(pValue,
                                 error_file_len);
        URL_copy_bwe(error_url->bwe, url->bwe);

        RA_file* rafile = URL_open(proc, config, error_url);

        if (rafile) {
            if (URL_encoding_check(rafile, error_url)) {
                DPRINTF(D_INFO, ("serving error file %s\n", pValue));
                delete error_url;
                return rafile;
            }
            delete rafile;
        }
        delete error_url;
    }
#endif

    return 0;
}

RA_file*
URL_open(Process* proc, Config* config, URL* url)
{
#ifdef _AIX
#undef MAXPATH
#endif
    /*
     * Translate stream name to local file path
     */

    char* path = new char[FileIO::MAXPATH];
    if (!convert_url(proc, config, url->name, path))
    {
        url->err = SE_INVALID_PATH;
        HX_VECTOR_DELETE(path);
        return 0;
    }

    RA_file* rafile = 0;

    if (FileIO::is_directory(path))
    {
        rafile = URL_find_bw_file(proc, path, url);
        if (!rafile)
        {
            HX_VECTOR_DELETE(path);
            return 0;
        }
    }
    else
    {
        rafile = new RA_file(path, O_RDONLY);
        if ((url->err = rafile->error()) != 0)
        {
            if (rafile->error() == ENOENT)
                url->err = SE_UNKNOWN_PATH;
            delete rafile;
            HX_VECTOR_DELETE(path);
            return 0;
        }
        if (URL_encoding_check(rafile, url) == 0)
        {
            delete rafile;
            HX_VECTOR_DELETE(path);
            return 0;
        }
    }

    HX_VECTOR_DELETE(path);
    return rafile;
}

SIO*
URL_eventfile_open(Process* proc, Config* config, URL* url)
{
#ifdef _AIX
#undef MAXPATH
#endif
    /*
     * Translate stream name to local file path
     */

    char* path = new char[FileIO::MAXPATH];
    if (!convert_url(proc, config, url->name, path))
    {
        url->err = SE_INVALID_PATH;
        HX_VECTOR_DELETE(path);
        return 0;
    }

    SIO* eventfile = 0;

    if (FileIO::is_directory(path))
    {
        eventfile = URL_find_bw_event_file(proc, path, url);

        if (eventfile)
        {
            HX_VECTOR_DELETE(path);
            return eventfile;
        }
    }

    if ((eventfile = new FSIO(strcat(path, "e"), O_RDONLY)) != 0 &&
        eventfile->error())
    {
        delete eventfile;
        eventfile = 0;
    }

    HX_VECTOR_DELETE(path);
    return eventfile;
}

void
URL_fill_bwe(BW_encoding* bwe, int bw, char* compression_code)
{
    bwe->bw = bw;
    memcpy(bwe->comp, compression_code, 4);
    if (!memcmp(bwe->comp, "lpcJ", 4))
    {
        memcpy(bwe->file, "14_4", 4);
    }
    else
    {
        memcpy(bwe->file, bwe->comp, 4);
    }
}

void
URL_copy_bwe(BW_encoding*& to, BW_encoding* from)
{
    if (from == 0)
    {
        to = 0;
        return;
    }

    int i = 0;
    BW_encoding* bwe = from;
    while(bwe->bw)
    {
        i++;
        bwe++;
    }
    to = new BW_encoding[i+1];
    memcpy(to, from, sizeof(BW_encoding) * i);
    to[i].bw = 0;
}

/*
 * Check that the url can not be used to go above the root.
 * As long as the number of forward components >= ".." components,
 * the url path is valid.  The following URL prefixes are valid:
 *      "", "foo/..", "~foo", "~foo/xx/.."
 * The following are not valid:
 *      "..", "foo/../..", "~foo/..", "~foo/xx/../.."
 * This implies that the maximum number of ".." components in
 * a in a valid ~foo style path must be one less compared to a
 * rooted path of equal length.
 *
 * Note that the initial / is removed before we get here.
 * We also throw away any ? and chars after it.
 *
 * Return value is < 0 if the path does not stay below root.
 */
int
check_url(const char* path, int& len)
{
    const char *cp = path;
    int level = *cp == '~' ? -1 : 0;

    if (!*cp)
        return 0;

    /*
     * Process one path name component each time around.
     * If this is a ".." component reduce level count,
     * else increase level count.
     * Return if count drops below 0, or this is the last component
     * (Note that ? precedes parameters so drop it and the parameters)
     */
    do
    {
        if (*cp == '.')
        {
            if (*++cp == '.')
            {
                cp++;
                switch (*cp)
                {
                case '?':       len = cp - path; /* fallthrough */
                case '\0':      return --level;
                case '/':       if (--level < 0) return level; break;
                default:        level++;
                }
            }
            else if (*cp != '\0' && *cp != '/')
                level++;
        }
        else
            level++;

        for (;;)
        {
            switch (*cp++)
            {
            case '?':   len = cp - path; /* fallthrough */
            case '\0':  return level;
            case '/':   while (*cp == '/') cp++; break;
            default:    continue;
            }
            break;
        }
    }
    while (*cp);

    return level;
}

/*
 * Convert a local url path to a local file path.
 * ~user prefix if any is expanded by the config object.
 */
char *
convert_url(Process* proc, Config* c, char* url, char* path)
{
#ifdef _AIX
#undef MAXPATH
#endif
    int len = strlen(url);

    if (url[0] == '~')
    {
#ifdef XXXPSH_CONFIG
        char* next = strchr(url, '/');
        const char* rest;
        if (next)
        {
                *next = '\0';
                rest = next;
                len -= next-url;
        }
        else
        {
                rest = "";
                len = 0;
        }
        const char* user_path = c->user_path(&url[1]);

        char buffer[FileIO::MAXPATH];
#ifndef __MWERKS__
        if (strlen(user_path) == 0)
        {
            user_path = c->user_path("~*");
            if (*user_path == '~')
            {
#ifdef unix
                struct passwd* pw = getpwnam(&url[1]);
                if (!pw) return 0;
                user_path = pw->pw_dir;
#else
                return 0;
#endif
            }

            else
            {
                sprintf(buffer, "%s/%s", user_path, &url[1]);
                user_path = buffer;
            }
        }
#endif

        if (next)
            *next = '/';
        if (!user_path)
            return 0;
        const char* user_dir = c->GetString(proc, "config.UserDir");
        if (strlen(user_path) + 1 + strlen(user_dir) + len >= FileIO::MAXPATH)
            return 0;

        if (strlen(user_dir) > 0)
                sprintf(path, "%s/%s%s", user_path, user_dir, rest);
        else
                sprintf(path, "%s%s", user_path,rest);
#endif /* XXXPSH_CONFIG */
    }
    else
    {
        if (0)//XXXJRif (c->dynamic_isp_enabled() && c->directory_depth_parse())
        {
#ifdef XXXPSH_CONFIG
            char tmp_string[4];
            sprintf(tmp_string, "*%d", c->directory_depth_parse());
            const char* base_path = c->user_path(tmp_string);
            if (strlen(base_path) + 1 + len >= FileIO::MAXPATH)
                return 0;
            sprintf(path, "%s/%s", base_path, url);
#endif /* XXXPSH_CONFIG */
        }

        else
        {
            const char* base_path = c->GetString(proc, "config.BasePath");

            if (!base_path || strlen(base_path) + 1 + len >= FileIO::MAXPATH)
                return 0;
            sprintf(path, "%s/%s", base_path, url);
        }
    }

    FileIO::local_path(path);
    return path;
}

/*
 * This function assumes that we have already checked to see that there
 * is no file that this player can play.
 */
int
URL_check_codecs(Process* proc, const char* path, URL* url)
{
    Dir_list*       dir_list = Dir_list::create(path);
    const char*     current_file;
    UINT16         bandwidth;
    UINT16         player_bandwidth;
    BW_encoding*    bwe;
    int             codec_found = 0;
    char*           end_ptr;
    int             valid_files_found = 0;

    if (dir_list->error() != 0)
    {
        url->err = SE_INVALID_PATH;
        ERRMSG(proc->pc->error_handler,
               "error opening bandwidth directory %s error: %s\n",
               path, strerror(dir_list->error()));
        delete dir_list;
        return -1;
    }

    if (dir_list->is_empty())
    {
        url->err = SE_INVALID_PATH;
        ERRMSG(proc->pc->error_handler,
               "empty bandwidth directory %s\n", path);
        delete dir_list;
        return -1;
    }

    /*
     * Search through all the files in the bandwidth directory to
     * try and find a file that is within the players max_bandwidth, but
     * whose compression does not match any that the player sends
     * If there is such a file, then the error is a CODEC error
     * which means we should send the upgrade message to the player.
     */
    player_bandwidth = (UINT16) url->max_bandwidth / 8 / 100;
    while ((current_file = dir_list->get_next()) && !codec_found)
    {
        if (strlen(current_file) < 5)
        {
            continue;
        }
        bwe = url->bwe;
        while (bwe->bw && !codec_found)
        {
            if (memcmp(current_file, bwe->file, BWE_FILE_NAME_LEN) != 0)
            {
                bandwidth = (UINT16) strtol(current_file + BWE_FILE_NAME_LEN +
                                             1, &end_ptr, 0);
                if ( (current_file[BWE_FILE_NAME_LEN] != 0) &&
                     (end_ptr[0] == 0) )
                {
                    valid_files_found++;
                    if (bandwidth <= player_bandwidth)
                    {
                        codec_found = 1;
                    }
                }
            }
            bwe++;
        }
    }
    if (!valid_files_found)
    {
        ERRMSG(proc->pc->error_handler,
               "no valid files found in bandwidth directory %s\n", path);
        url->err = SE_INVALID_PATH;
    }
    if (codec_found)
    {
        url->err = SE_NO_CODEC;
    }
    delete dir_list;

    return 0;
}
