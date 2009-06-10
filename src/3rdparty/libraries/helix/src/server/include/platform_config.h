/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: platform_config.h,v 1.4 2005/08/10 17:26:02 dcollins Exp $
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

#ifndef _PLATFORM_CONFIG_
#define _PLATFORM_CONFIG_

#ifdef _UNIX

#if !defined(_LINUX) || (defined(__GNUC__) && __GNUC__>=3)
#ifndef PTHREADS_SUPPORTED
#define PTHREADS_SUPPORTED
#endif //PTHREADS_SUPPORTED
#endif //defined(_LINUX)...

#ifndef SHARED_FD_SUPPORT
#define SHARED_FD_SUPPORT
#endif //SHARED_FD_SUPPORT
#endif //_UNIX

#if defined(_SOLARIS)
#ifndef DEV_POLL_SUPPORT
#define DEV_POLL_SUPPORT
#endif //DEV_POLL_SUPPORT
#endif


#endif /* _PLATFORM_CONFIG_ */
