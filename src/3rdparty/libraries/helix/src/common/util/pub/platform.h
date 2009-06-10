/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: platform.h,v 1.6 2007/07/06 20:39:23 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/* Platform Tags */
#if	defined _LINUX
#define PLATFORM "Linux 1.2.x"
#define LIC_PLATFORM 'A'
#elif	(defined _ALPHA && defined _UNIX)
#define PLATFORM "DEC Alpha UNIX 3.2"
#define LIC_PLATFORM 'B'
#elif	defined _BSDI
#define PLATFORM "BSDI-2.0"
#define LIC_PLATFORM 'C'
#elif	defined _SOLARIS
#define PLATFORM "Solaris 2.x"
#define LIC_PLATFORM 'D'
#elif	defined _SUNOS
#define PLATFORM "SunOS 4.1.x"
#define LIC_PLATFORM 'E'
#elif defined _SGI
#define PLATFORM "Irix 5.x"
#define LIC_PLATFORM 'F'
#elif defined _IRIX
#define PLATFORM "Irix 6.x"
#define LIC_PLATFORM 'F'
#elif defined _FREEBSD
#define PLATFORM "FreeBSD-2.1.x"
#define LIC_PLATFORM 'G'
#elif defined _HPUX
#define PLATFORM "HP-UX 10.01"
#define LIC_PLATFORM 'H'
#elif defined _AIX
#define PLATFORM "AIX V4"
#define LIC_PLATFORM 'I'
#elif	(defined ALPHA && defined _WINDOWS)
#define PLATFORM "NT 3.5.x (DEC Alpha)"
#define LIC_PLATFORM 'K'
#elif	(defined _WIN32 && defined _WINDOWS)
#define PLATFORM "NT 3.5.x"
#define LIC_PLATFORM 'J'
#elif	(defined _MACINTOSH)
#define PLATFORM		"Macintosh"
#define LIC_PLATFORM		'M'
#elif defined __QNXNTO__
#define PLATFORM		"QNX Neutrino 2.x"
#define LIC_PLATFORM		'N'
#elif defined _OPENBSD
#define PLATFORM "OpenBSD 3.x"
#define LIC_PLATFORM 'P'
#elif defined _NETBSD
#define PLATFORM "NetBSD 1.x"
#define LIC_PLATFORM 'Q'
#endif /* defined _LINUX */

#endif /*_PLATFORM_H_ */
