/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: Solaris56Fix.h,v 1.3 2004/07/09 18:19:26 hubbe Exp $
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

#ifndef _SOLARIS56FIX_H
#define _SOLARIS56FIX_H


typedef unsigned char               uint8_t;
typedef unsigned int                uint32_t;
typedef unsigned long long  uint64_t;
typedef uint64_t        upad64_t;
/*
 * POSIX definitions are same as defined in thread.h and synch.h.
 * Any changes made to here should be reflected in corresponding
 * files as described in comments.
 */
typedef unsigned int    pthread_t;      /* = thread_t in thread.h */
typedef unsigned int    pthread_key_t;  /* = thread_key_t in thread.h */

typedef struct  _pthread_mutex {                /* = mutex_t in synch.h */
        struct {
                uint8_t         __pthread_mutex_flag[4];
                uint32_t        __pthread_mutex_type;
        } __pthread_mutex_flags;
        union {
                struct {
                        uint8_t __pthread_mutex_pad[8];
                } __pthread_mutex_lock64;
                upad64_t __pthread_mutex_owner64;
        } __pthread_mutex_lock;
        upad64_t __pthread_mutex_data;
} pthread_mutex_t;

typedef struct  _pthread_cond {         /* = cond_t in synch.h */
        struct {
                uint8_t         __pthread_cond_flag[4];
                uint32_t        __pthread_cond_type;
        } __pthread_cond_flags;
        upad64_t __pthread_cond_data;
} pthread_cond_t;

/*
 * attributes for threads, dynamically allocated by library
 */
typedef struct _pthread_attr {
        void    *__pthread_attrp;
} pthread_attr_t;


/*
 * attributes for mutex, dynamically allocated by library
 */
typedef struct _pthread_mutexattr {
        void    *__pthread_mutexattrp;
} pthread_mutexattr_t;


/*
 * attributes for cond, dynamically allocated by library
 */
typedef struct _pthread_condattr {
        void    *__pthread_condattrp;
} pthread_condattr_t;

/*
 * pthread_once
 */
typedef struct  _once {
        upad64_t        __pthread_once_pad[4];
} pthread_once_t;



#endif /* _SOLARIS56FIX_H */
