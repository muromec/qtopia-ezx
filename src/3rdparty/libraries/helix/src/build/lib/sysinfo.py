# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: sysinfo.py,v 1.102 2007/06/04 18:44:07 jfinnecy Exp $ 
#   
#  Copyright Notices: 
#   
#  Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
#   
#  Patent Notices: This file may contain technology protected by one or  
#  more of the patents listed at www.helixcommunity.org 
#   
#  1.   The contents of this file, and the files included with this file, 
#  are protected by copyright controlled by RealNetworks and its  
#  licensors, and made available by RealNetworks subject to the current  
#  version of the RealNetworks Public Source License (the "RPSL")  
#  available at  * http://www.helixcommunity.org/content/rpsl unless  
#  you have licensed the file under the current version of the  
#  RealNetworks Community Source License (the "RCSL") available at 
#  http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
#  will apply.  You may also obtain the license terms directly from 
#  RealNetworks.  You may not use this file except in compliance with 
#  the RPSL or, if you have a valid RCSL with RealNetworks applicable 
#  to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
#  the rights, obligations and limitations governing use of the 
#  contents of the file. 
#   
#  2.  Alternatively, the contents of this file may be used under the 
#  terms of the GNU General Public License Version 2 (the 
#  "GPL") in which case the provisions of the GPL are applicable 
#  instead of those above.  Please note that RealNetworks and its  
#  licensors disclaim any implied patent license under the GPL.   
#  If you wish to allow use of your version of this file only under  
#  the terms of the GPL, and not to allow others 
#  to use your version of this file under the terms of either the RPSL 
#  or RCSL, indicate your decision by deleting Paragraph 1 above 
#  and replace them with the notice and other provisions required by 
#  the GPL. If you do not delete Paragraph 1 above, a recipient may 
#  use your version of this file under the terms of any one of the 
#  RPSL, the RCSL or the GPL. 
#   
#  This file is part of the Helix DNA Technology.  RealNetworks is the 
#  developer of the Original Code and owns the copyrights in the 
#  portions it created.   Copying, including reproducing, storing,  
#  adapting or translating, any or all of this material other than  
#  pursuant to the license terms referred to above requires the prior  
#  written consent of RealNetworks and its licensors 
#   
#  This file, and the files included with this file, is distributed 
#  and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
#  WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
#  AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
#  WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
#  FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
#   
#  Technology Compatibility Kit Test Suite(s) Location:  
#     http://www.helixcommunity.org/content/tck 
#   
# Contributor(s):
# 
# ***** END LICENSE BLOCK *****
# 
"""System type dection module."""

import sys
import os
import string
import err


## description of supported archatectures
ARCH_HASH = {
    'alpha'    : 'Digital Alpha Processor',
    'i386'     : 'Intel x86',
    'powerpc'  : 'Motarola PowerPC',
    'arm'      : 'StrongARM',
    'sparc'    : 'Sun Sparc/UltraSparc',
    'm68k'     : 'Motarola 68k',
    'sh4'      : 'SH4',
    'parisc'   : 'HP PA/RISC Processor',
    'ia64'     : 'Intel IA64',
    'amd64'    : 'AMD64/Opteron/X86_64',
    'mips'     : 'MIPS',
    'tm1'      : 'TriMedia 1300',
    'osx-ub'   : 'Universal Binary for OS X',
    }


PLATFORM_HASH = {}
def AddPlatform(platform):
    global PLATFORM_HASH
    PLATFORM_HASH[platform.id] = platform


class Platform:
    def __init__(self,
                 id,
                 platform,
                 arch,
                 distribution_id,
                 family_list,
                 host_type = None):
        self.id = id
        self.platform = platform
        self.arch = arch
        self.family_list = family_list
        self.distribution_id = distribution_id

        if host_type:
            self.host_type=host_type
        else:
            if platform in ['win32', 'mac']:
                self.host_type=platform
            elif 'win' in family_list:
                self.host_type='win32'
            elif 'unix' in family_list:
                self.host_type='posix'
            else:
                print "Warning, unable to guess host type for %s" % id
            
## define all platforms

AddPlatform(Platform(
    id = 'aix-4.2-powerpc',
    platform = 'aix4',
    arch = 'powerpc',
    distribution_id = 'aix-4.2-powerpc',
    family_list = ['unix', 'aix4', 'aix-4.2'] ))

AddPlatform(Platform(
    id = 'aix-4.3-powerpc',
    platform = 'aix4',
    arch = 'powerpc',
    distribution_id = 'aix-4.3-powerpc',
    family_list = ['unix', 'aix4', 'aix-4.3'] ))

AddPlatform(Platform(
    id = 'beos-4.5-i386',
    platform = 'beos',
    arch = 'i386',
    distribution_id = 'beos-4.5-i386',
    family_list = ['unix', 'beos', 'beos-4.5'] ))

AddPlatform(Platform(
    id = 'freebsd-2.2-i386',
    platform = 'freebsd2',
    arch = 'i386',
    distribution_id = 'freebsd-2.2-i386',
    family_list = ['unix', 'freebsd', 'freebsd2', 'freebsd-2.2'] ))

AddPlatform(Platform(
    id = 'freebsd-3.0-i386',
    platform = 'freebsd3',
    arch = 'i386',
    distribution_id = 'freebsd-3.0-i386',
    family_list = ['unix', 'freebsd', 'freebsd3', 'freebsd-3.0'] ))

AddPlatform(Platform(
    id = 'freebsd-4.0-i386',
    platform = 'freebsd4',
    arch = 'i386',
    distribution_id = 'freebsd-4.0-i386',
    family_list = ['unix', 'freebsd', 'freebsd4', 'freebsd-4.0'] ))

AddPlatform(Platform(
    id = 'freebsd-4.0-i586',
    platform = 'freebsd4',
    arch = 'i386',
    distribution_id = 'freebsd-4.0-i386',
    family_list = ['unix', 'freebsd', 'freebsd4', 'freebsd-4.0', 'freebsd-4.0-i586'] ))

AddPlatform(Platform(
    id = 'freebsd-5.0-i586',
    platform = 'freebsd5',
    arch = 'i386',
    distribution_id = 'freebsd-5.0-i586',
    family_list = ['unix', 'freebsd', 'freebsd5', 'freebsd-5.0', 'freebsd-5.0-i586'] ))

AddPlatform(Platform(
    id = 'freebsd-6.0-i586',
    platform = 'freebsd6',
    arch = 'i386',
    distribution_id = 'freebsd-6.0-i586',
    family_list = ['unix', 'freebsd', 'freebsd6', 'freebsd-6.0', 'freebsd-6.0-i586'] ))

AddPlatform(Platform(
    id = 'freebsd-7.0-i586',
    platform = 'freebsd7',
    arch = 'i386',
    distribution_id = 'freebsd-7.0-i586',
    family_list = ['unix', 'freebsd', 'freebsd7', 'freebsd-7.0', 'freebsd-7.0-i586'] ))

AddPlatform(Platform(
    id = 'openbsd-3.3-i586',
    platform = 'openbsd33',
    arch = 'i386',
    distribution_id = 'openbsd-3.3-i586',
    family_list = ['unix', 'openbsd', 'openbsd3', 'openbsd-3.3', 'openbsd-3.3-i586'] ))

AddPlatform(Platform(
    id = 'hpux-11.0-parisc',
    platform = 'hp-uxB',
    arch = 'parisc',
    distribution_id = 'hpux-11.0-parisc',
    family_list = ['unix', 'hp-uxB', 'hpux-11.0'] ))

AddPlatform(Platform(
    id = 'hpux-11.0-ia64',
    platform = 'hp-uxB',
    arch = 'ia64',
    distribution_id = 'hpux-11.0-ia64',
    family_list = ['unix', 'hp-uxB', 'hpux-11.0', 'hpux-11.0-ia64'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc6-mips-ps2',
    platform = 'linux2',
    arch = 'mips',
    distribution_id = 'linux-2.0-libc6-mips-ps2',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'ps2linux'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-iwmmxt-xscale',
    platform = 'linux2',
    arch = 'arm',
    distribution_id = 'linux-2.2-libc6-armv5te-gcc3.3-iwmmxt-softfloat',
    family_list = ['unix', 'linux', 'linux2','gcc3', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-xscale'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-ixp4xx-softfloat',
    platform = 'linux2',
    arch = 'arm',
    distribution_id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-ixp4xx-softfloat',
    family_list = ['unix', 'linux', 'linux2','gcc3', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-xscale'] ))

AddPlatform(Platform(
    id = 'irix-6.2-mips',
    platform = 'irix6',
    arch = 'mips',
    distribution_id = 'irix-6.2-mips',
    family_list = ['unix', 'irix', 'irix6', 'irix-6.2'] ))

AddPlatform(Platform(
    id = 'irix-6.3-mips',
    platform = 'irix6',
    arch = 'mips',
    distribution_id = 'irix-6.3-mips',
    family_list = ['unix', 'irix', 'irix6', 'irix-6.3'] ))

AddPlatform(Platform(
    id = 'irix-6.4-mips',
    platform = 'irix646',
    arch = 'mips',
    distribution_id = 'irix-6.4-mips',
    family_list = ['unix', 'irix', 'irix646', 'irix-6.4'] ))

AddPlatform(Platform(
    id = 'irix-6.5-mips',
    platform = 'irix646',
    arch = 'mips',
    distribution_id = 'irix-6.5-mips',
    family_list = ['unix', 'irix', 'irix646', 'irix-6.5'] ))

AddPlatform(Platform(
    id = 'irix-6.2-mips-gcc2.95',
    platform = 'irix6',
    arch = 'mips',
    distribution_id = 'irix-6.5-mips',
    family_list = ['unix', 'irix', 'irix6', 'irix-6.2'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc5-i386',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.0-libc5-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc6-i386',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.0-libc6-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc6-i386-gcc2.95',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.0-libc6-i386-gcc2.95',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-2.0-libc6-i386-gcc2.95'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc6-i386-gcc2.95-el',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.0-libc6-i386-gcc2.95-el',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-2.0-libc6-i386-gcc2.95'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-i386',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.2-libc6-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-2.2-libc6-i386'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-i586',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.2-libc6-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-2.2-libc6-i386',
                   'linux-2.2-libc6-i586'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-gcc32-i586',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.2-libc6-gcc32-i586',
    family_list = ['unix', 'linux', 'linux2','gcc3', 'linux-glibc-2.0',
                   'linux-2.2-libc6-i386'] ))

AddPlatform(Platform(
    id = 'linux-2.6-glibc24-gcc41-i486-lsb',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.6-glibc24-gcc41-i486-lsb',
    family_list = ['unix', 'linux', 'linux2','gcc3', 'linux-glibc-2.0'] ))


AddPlatform(Platform(
    id = 'linux-2.2-libc6-i586-server',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.2-libc6-i586-server',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-2.2-libc6-i386'] ))

AddPlatform(Platform(
    id = 'linux-2.0-libc6-alpha-gcc2.95',
    platform = 'linux2',
    arch = 'alpha',
    distribution_id = 'linux-2.0-libc6-alpha-gcc2.95',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-powerpc',
    platform = 'linux2',
    arch = 'powerpc',
    distribution_id = 'linux-2.2-libc6-powerpc',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-gcc32-powerpc',
    platform = 'linux2',
    arch = 'powerpc',
    distribution_id = 'linux-2.2-libc6-powerpc',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-powerpc64',
    platform = 'linux2',
    arch = 'powerpc',
    distribution_id = 'linux-2.2-libc6-powerpc',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-sparc',
    platform = 'linux2',
    arch = 'sparc',
    distribution_id = 'linux-2.2-libc6-sparc',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.4-libc6-ia64',
    platform = 'linux2',
    arch = 'ia64',
    distribution_id = 'linux-2.4-libc6-ia64',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv4l-cross-gcc2.95',
    platform = 'linux2',
    arch = 'armv4l',
    distribution_id = 'linux-2.2-libc6-armv4l-gcc2.95',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv4l-gcc2.95'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv4l-cross-gcc3.2-softfloat',
    platform = 'linux2',
    arch = 'armv4l',
    distribution_id = 'linux-2.2-libc6-armv4l-gcc3.2-softfloat',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv4l-gcc3.2-softfloat'] ))


AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-softfloat',
    platform = 'linux2',
    arch = 'armv5te',
    distribution_id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-softfloat',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv5te-gcc3.3-softfloat'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-iwmmxt-softfloat',
    platform = 'linux2',
    arch = 'armv5te',
    distribution_id = 'linux-2.2-libc6-armv5te-cross-gcc3.3-iwmmxt-softfloat',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv5te-gcc3.3-iwmmxt-softfloat'] ))

AddPlatform(Platform(
    id = 'linux-2.2-mizi-armv4l-xscale',
    platform = 'linux2',
    arch = 'armv4l',
    distribution_id = 'linux-2.2-mizi-armv4l-xscale',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-mizi-armv4l-xscale'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv4l-cross-gcc3.2',
    platform = 'linux2',
    arch = 'armv4l',
    distribution_id = 'linux-2.2-libc6-armv4l-gcc3.2',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv4l-gcc3.2'] ))

AddPlatform(Platform(
    id = 'linux-2.2-libc6-kerbango-powerpc',
    platform = 'linux2',
    arch = 'powerpc',
    distribution_id = 'linux-2.2-libc6-kerbango-powerpc',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0'] ))

AddPlatform(Platform(
    id = 'linux-2.4-glibc23-i386',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.4-glibc23-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.4-glibc23', 'linux-2.4-glibc23-i386'] ))

AddPlatform(Platform(
    id = 'linux-2.4-glibc23-i686',
    platform = 'linux2',
    arch = 'i686',
    distribution_id = 'linux-2.4-glibc23-i686',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.4-glibc23', 'linux-2.4-glibc23-i686'] ))

AddPlatform(Platform(
    id = 'linux-rhel3-i686',
    platform = 'linux2',
    arch = 'i686',
    distribution_id = 'linux-rhel3-i686',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.4-glibc23', 'linux-2.4-glibc23-i686', 'linux-rhel3-i686'] ))

AddPlatform(Platform(
    id = 'linux-2.6-glibc23-i386',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-2.6-glibc23-i386',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.6-glibc23', 'linux-2.6-glibc23-i386'] ))

AddPlatform(Platform(
    id = 'linux-2.6-glibc23-i686',
    platform = 'linux2',
    arch = 'i686',
    distribution_id = 'linux-2.6-glibc23-i686',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.6-glibc23', 'linux-2.6-glibc23-i686'] ))

AddPlatform(Platform(
    id = 'linux-2.6-glibc23-amd64',
    platform = 'linux2',
    arch = 'amd64',
    distribution_id = 'linux-2.6-glibc23-amd64',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.6-glibc23', 'linux-2.6-glibc23-amd64'] ))

AddPlatform(Platform(
    id = 'linux-rhel4-i686',
    platform = 'linux2',
    arch = 'i686',
    distribution_id = 'linux-rhel4-i686',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3',
                   'linux-2.6-glibc23', 'linux-2.6-glibc23-i686', 'linux-rhel4-i686'] ))

AddPlatform(Platform(
    id = 'linux-lsb3.1-i486',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-lsb3.1-i486',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.3', 'linux-2.6-glibc23', 'linux-lsb3.1-i486'] ))

AddPlatform(Platform(
    id = 'netbsd-1.4-i386',
    platform = 'netbsd1',
    arch = 'i386',
    distribution_id = 'netbsd-1.4-i386',
    family_list = ['unix', 'netbsd1', 'netbsd-1.4'] ))

AddPlatform(Platform(
    id = 'netbsd-1.6-i586',
    platform = 'netbsd16',
    arch = 'i386',
    distribution_id = 'netbsd-1.6-i586',
    family_list = ['unix', 'netbsd', 'netbsd-1.6', 'netbsd-1.6-i586'] ))

AddPlatform(Platform(
    id = 'osf-4.0-alpha',
    platform = 'osf1V4',
    arch = 'alpha',
    distribution_id = 'osf-4.0-alpha',
    family_list = ['unix', 'osf1V4', 'osf-4.0'] ))

AddPlatform(Platform(
    id = 'osf-5.1-alpha',
    platform = 'osf1V4',
    arch = 'alpha',
    distribution_id = 'osf-5.1-alpha',
    family_list = ['unix', 'osf1V5', 'osf-5.1']))

#
# These have no .cf files - Hubbe
#
#AddPlatform(Platform(
#    id = 'sce-ee-ps2kernel-mips', 
#    platform = 'ps2kernel', 
#    arch = 'mips5000',
#    distribution_id = 'ps2kernel-mips',
#    family_list = ['ps2kernel', 'ps2kernel2']))
#
#AddPlatform(Platform(
#    id = 'sce-iop-ps1kernel-mips', 
#    platform = 'ps1kernel', 
#    arch = 'mips3000',
#    distribution_id = 'ps1kernel-mips',
#    family_list = ['ps1kernel', 'ps1kernel2']))

AddPlatform(Platform(
    id = 'sunos-5.5.1-sparc',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.5.1-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.5']))

AddPlatform(Platform(
    id = 'sunos-5.6-sparc',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.6-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.6']))

AddPlatform(Platform(
    id = 'sunos-5.6-sparc-gcc',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.6-sparc-gcc',
    family_list = ['unix', 'sunos5', 'sunos-5.6']))

AddPlatform(Platform(
    id = 'sunos-5.6-sparc-native',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.6-sparc-native',
    family_list = ['unix', 'sunos5', 'sunos-5.6']))

AddPlatform(Platform(
    id = 'sunos-5.6-sparc-native-noultra',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.6-sparc-native-noultra',
    family_list = ['unix', 'sunos5', 'sunos-5.6']))

AddPlatform(Platform(
    id = 'sunos-5.7-sparc',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.7-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.7']))

AddPlatform(Platform(
    id = 'sunos-5.8-sparc',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.8-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.8-sparc-studio10',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.8-sparc',
    family_list = ['unix','sunstudio10', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.8-i386',
    platform = 'sunos5',
    arch = 'i386',
    distribution_id = 'sunos-5.8-i386',
    family_list = ['unix', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.8-i386-gcc',
    platform = 'sunos5',
    arch = 'i386',
    distribution_id = 'sunos-5.8-i386-gcc',
    family_list = ['unix', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.8-sparc-server',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.8-sparc-server',
    family_list = ['unix', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.8-sparc-gcc-server',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.8-sparc-server',
    family_list = ['unix', 'sunos5', 'sunos-5.8']))

AddPlatform(Platform(
    id = 'sunos-5.9-sparc-server',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.9-sparc-server',
    family_list = ['unix', 'sunos5', 'sunos-5.9']))

AddPlatform(Platform(
    id = 'sunos-5.10-sparc-server',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.10-sparc-server',
    family_list = ['unix', 'sunos5', 'sunos-5.10']))

AddPlatform(Platform(
    id = 'sunos-5.10-i386-studio10',
    platform = 'sunos5',
    arch = 'i386',
    distribution_id = 'sunos-5.10-i386',
    family_list = ['unix', 'sunos5', 'sunos-5.10', 'sunstudio10', 'sunos-5.10-i386']))

AddPlatform(Platform(
    id = 'sunos-5.10-i386-studio11',
    platform = 'sunos5',
    arch = 'i386',
    distribution_id = 'sunos-5.10-i386',
    family_list = ['unix', 'sunos5', 'sunos-5.10', 'sunstudio10', 'sunos-5.10-i386']))

AddPlatform(Platform(
    id = 'sunos-5.10-sparc-studio10',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.10-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.10', 'sunstudio10', 'sunos-5.10-sparc']))

AddPlatform(Platform(
    id = 'sunos-5.10-sparc-studio11',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.10-sparc',
    family_list = ['unix', 'sunos5', 'sunos-5.10', 'sunstudio11','sunos-5.10-sparc']))

AddPlatform(Platform(
    id = 'sunos-5.11-sparc-server',
    platform = 'sunos5',
    arch = 'sparc',
    distribution_id = 'sunos-5.11-sparc-server',
    family_list = ['unix', 'sunos5', 'sunos-5.11']))

AddPlatform(Platform(
    id = 'unixware-7.0-i386',
    platform = 'unixware5',
    arch = 'i386',
    distribution_id = 'unixware-7.0-i386',
    family_list = ['unix', 'unixware5', 'unixware-7.0']))

AddPlatform(Platform(
    id = 'openserver-5.0.5-i386',
    platform = 'sco_sv3',
    arch = 'i386',
    distribution_id = 'openserver-5.0.5-i386',
    family_list = ['unix', 'sco_sv3', 'openserver-5.0']))

AddPlatform(Platform(
    id = 'macos-powerpc',
    platform = 'mac',
    arch = 'powerpc',
    distribution_id = 'macos-powerpc',
    family_list = ['mac', 'mac-cfm']))

AddPlatform(Platform(
    id = 'macos-powerpc-cw6',
    platform = 'mac',
    arch = 'powerpc',
    distribution_id = 'macos-powerpc',
    family_list = ['mac']))

AddPlatform(Platform(
    id = 'macos-carbon-powerpc-cw6',
    platform = 'mac',
    arch = 'powerpc',
    distribution_id = 'macos-carbon-powerpc-cw6',
    family_list = ['mac', 'mac-cfm', 'macos-carbon-powerpc-cw6']))

AddPlatform(Platform(
    id = 'macos-carbon-powerpc-cw7',
    platform = 'mac',
    arch = 'powerpc',
    distribution_id = 'macos-carbon-powerpc-cw7',
    family_list = ['mac', 'mac-cfm', 'macos-carbon-powerpc-cw7']))

AddPlatform(Platform(
    id = 'macos-carbon-powerpc-cw8',
    platform = 'mac',
    arch = 'powerpc',
    distribution_id = 'macos-carbon-powerpc-cw8',
    family_list = ['mac', 'mac-cfm', 'macos-carbon-powerpc-cw8']))

AddPlatform(Platform(
    id = 'macos-carbon-powerpc-darwin-cw7',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-carbon-powerpc-darwin-cw7',
    family_list = ['mac', 'darwin', 'macos-carbon-powerpc-darwin-cw7'],
    host_type = 'mac'))

AddPlatform(Platform(
    id = 'macos-gcc3-pb-unix',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-gcc3-pb-unix',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix'],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc3-pb',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-gcc3-pb',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix'],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc3-xcode',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-gcc3-pb',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix', 'xcode' ],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc3-xcode15',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-gcc3-pb',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix', 'xcode' ],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc3-xcode21',
    platform = 'darwin',
    arch = 'powerpc',
    distribution_id = 'macos-gcc3-pb',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix', 'xcode', 'xcode21' ],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc4-xcode22',
    platform = 'darwin',
    arch = 'osx-ub',
    distribution_id = 'macos-gcc3-pb',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix', 'xcode', 'xcode21', 'xcode22' ],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'macos-gcc4-xcode23',
    platform = 'darwin',
    arch = 'osx-ub',
    distribution_id = 'macos-gcc4-xcode23',
    family_list = ['unix', 'mac-pb', 'gcc3', 'mac-unix', 'xcode', 'xcode21', 'xcode22', 'xcode23' ],
    host_type = 'posix'))

AddPlatform(Platform(
    id = 'win32-i386',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc6',
    family_list = ['win', 'windows', 'win32', 'win32-i386','win32-i386-vc5']))


AddPlatform(Platform(
    id = 'win32-i386-vc6',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc6',
    family_list = ['win', 'windows', 'win32', 'win32-i386','win32-i386-vc6']))

AddPlatform(Platform(
    id = 'win32-i386-xicl6',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc6',
    family_list = ['win', 'windows', 'win32', 'win32-i386','win32-i386-vc6']))

AddPlatform(Platform(
    id = 'win32-i386-vc6-intel',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc6',
    family_list = ['win', 'windows', 'win32', 'win32-i386','win32-i386-vc6']))

AddPlatform(Platform(
    id = 'win32-i386-vc7',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc7',
    family_list = ['win', 'windows', 'win32', 'win-vc7', 'win32-i386','win32-i386-vc7']))


AddPlatform(Platform(
    id = 'win32-i386-vc7-intel',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc7',
    family_list = ['win', 'windows', 'win32', 'win-vc7', 'win32-i386','win32-i386-vc7']))

AddPlatform(Platform(
    id = 'win32-i386-xicl7',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc7',
    family_list = ['win', 'windows', 'win32', 'win-vc7', 'win32-i386','win32-i386-vc7']))

AddPlatform(Platform(
    id = 'win32-i386-vc8',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc8',
    family_list = ['win', 'windows', 'win32', 'win-vc8', 'win32-i386','win32-i386-vc8']))

AddPlatform(Platform(
    id = 'win32-i386-winsock1',
    platform = 'win32',
    arch = 'i386',
    distribution_id = 'win32-i386-vc6',
    family_list = ['win', 'windows', 'win32', 'win32-i386', 'win32-i386-winsock1']))

AddPlatform(Platform(
    id = 'wince-sh4',
    platform = 'wince-sh4',
    arch = 'sh4',
    distribution_id = 'wince-sh4',
    family_list = ['windows', 'win', 'wince', 'wince-sh4']))


AddPlatform(Platform(
    id = 'wince-300-ppc-arm',
    platform = 'wince',
    arch = 'arm',
    distribution_id = 'wince-arm',
    family_list = ['windows', 'win', 'wince', 'wince-300-ppc-arm']))

AddPlatform(Platform(
    id = 'wince-300-ppc-armxs',
    platform = 'wince',
    arch = 'arm',
    distribution_id = 'wince-armxs',
    family_list = ['windows', 'win', 'wince', 'wince-300-ppc-armxs']))


AddPlatform(Platform(
    id = 'wince-300-ppc-emu',
    platform = 'wince',
    arch = 'i386',
    distribution_id = 'wince-emu',
    family_list = ['windows', 'win', 'wince', 'wince-300-ppc-emu']))

AddPlatform(Platform(
    id = 'wince-420-emu',
    platform = 'wince',
    arch = 'wince',
    distribution_id = 'wince-emu',
    family_list = ['windows', 'win', 'wince', 'wince-420', 'wince-420-emu']))

AddPlatform(Platform(
    id = 'wince-420-x86',
    platform = 'wince',
    arch = 'wince',
    distribution_id = 'wince-x86',
    family_list = ['windows', 'win', 'wince', 'wince-420', 'wince-420-x86']))

AddPlatform(Platform(
    id = 'wince-500-ppc-arm',
    platform = 'wince',
    arch = 'arm',
    distribution_id = 'wince-500-arm',
    family_list = ['windows', 'win', 'wince', 'wince-500-ppc']))

AddPlatform(Platform(
    id = 'wince-500-ppc-armxs-intel',
    platform = 'wince',
    arch = 'arm',
    distribution_id = 'wince-armxs',
    family_list = ['windows', 'win', 'wince', 'wince-500-ppc', 'wince-500-ppc', 'wince-500-ppc-arm']))

AddPlatform(Platform(
    id = 'linux-tivo-x86',
    platform = 'linux2',
    arch = 'i386',
    distribution_id = 'linux-tivo-x86',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'tivo', 'linux-tivo-x86'] ))

AddPlatform(Platform(
    id = 'linux-tivo-mips',
    platform = 'linux2',
    arch = 'mips',
    distribution_id = 'linux-tivo-mips',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'tivo', 'linux-tivo-mips'] ))

AddPlatform(Platform(
    id = 'win32-palm5sim-i386',
    platform = 'palm',
    arch = 'i386',
    distribution_id = 'win32-palm5sim-i386',
    family_list = ['palm', 'palm5', 'palm5sim'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'win32-palm5-m68k',
    platform = 'palm',
    arch = 'm68k',
    distribution_id = 'win32-palm5-m68k',
    family_list = ['palm', 'palm5', 'palm5-m68k'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'win32-palm5-arm',
    platform = 'palm',
    arch = 'arm',
    distribution_id = 'win32-palm5-arm',
    family_list = ['palm', 'palm5', 'palm5-arm'],
    host_type = 'win32'))

AddPlatform(Platform(
    id = 'win32-psos-arm',
    platform = 'psos',
    arch = 'arm',
    distribution_id = 'win32-psos-arm',
    family_list = ['psos', 'psos-arm'],
    host_type = 'win32'))

AddPlatform(Platform(
    id = 'win32-psos-arm7T',
    platform = 'psos',
    arch = 'arm',
    distribution_id = 'win32-psos-arm7T',
    family_list = ['psos', 'psos-arm'],
    host_type = 'win32'))

AddPlatform(Platform(
    id = 'win32-psos-arm9E',
    platform = 'psos',
    arch = 'arm',
    distribution_id = 'win32-psos-arm9E',
    family_list = ['psos', 'psos-arm'],
    host_type = 'win32'))

AddPlatform(Platform(
    id = 'ads12-arm',
    platform = 'arm',
    arch = 'arm',
    distribution_id = 'ads12-arm',
    family_list = ['ads', 'arm', 'ads12', 'ads12-arm'],
    host_type = 'win32'))

AddPlatform(Platform(
    id = 'symbian-61-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-61-emulator',
    family_list = ['symbian', 'symbian61', 'symbian-wins'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-61-armi',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-61-armi',
    family_list = ['symbian', 'symbian61', 'symbian-armi'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-61-thumb',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-61-thumb',
    family_list = ['symbian', 'symbian61', 'symbian-thumb'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-70s-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-70s-emulator',
    family_list = ['symbian', 'symbian70s', 'symbian-wins'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-70s-emulator-vc7',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-70s-emulator',
    family_list = ['symbian', 'symbian70s', 'symbian-wins', 'win-vc7'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-70s-armi',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-70s-armi',
    family_list = ['symbian', 'symbian70s', 'symbian-armi'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-70s-thumb',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-70s-thumb',
    family_list = ['symbian', 'symbian70s', 'symbian-thumb'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-80-thumb',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-80-thumb',
    family_list = ['symbian', 'symbian80', 'symbian-thumb'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-80-winscw-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-80-winscw-emulator',
    family_list = ['symbian', 'symbian80', 'symbian-winscw'],
    host_type = 'win32' ))
    
AddPlatform(Platform(
    id = 'symbian-81-armv5',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-81-armv5',
    family_list = ['symbian', 'symbian81', 'symbian-armv5'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-81-winscw-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-81-emulator',
    family_list = ['symbian', 'symbian81', 'symbian-winscw'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-90-armv5',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-90-armv5',
    family_list = ['symbian', 'symbian90', 'symbian-armv5'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-90-winscw-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-90-emulator',
    family_list = ['symbian', 'symbian90', 'symbian-winscw'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-91-winscw-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-91-winscw-emulator',
    family_list = ['symbian', 'symbian91', 'symbian-winscw'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-91-armv5-gcce',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-91-armv5-gcce',
    family_list = ['symbian', 'symbian91', 'symbian-armv5-gcce'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-91-armv5',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-91-armv5',
    family_list = ['symbian', 'symbian91', 'symbian-armv5'],
    host_type = 'win32' ))
    
AddPlatform(Platform(
    id = 'symbian-91-armv5e',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-91-armv5e',
    family_list = ['symbian', 'symbian91', 'symbian-armv5e'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-91-armv6',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-91-armv6',
    family_list = ['symbian', 'symbian91', 'symbian-armv6'],
    host_type = 'win32' ))    

AddPlatform(Platform(
    id = 'symbian-91-mmp',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-91-armv5',
    family_list = ['symbian', 'symbian91', 'symbian-armv5', 'symbian-winscw', 'symbian-mmp'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-s60-32-winscw-emulator',
    platform = 'symbian',
    arch = 'i386',
    distribution_id = 'symbian-s60-32-winscw-emulator',
    family_list = ['symbian', 'symbian91', 'symbian-winscw'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-s60-32-armv5',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-s60-32-armv5',
    family_list = ['symbian', 'symbian91', 'symbian-armv5'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'symbian-s60-32-mmp',
    platform = 'symbian',
    arch = 'arm',
    distribution_id = 'symbian-s60-32-armv5',
    family_list = ['symbian', 'symbian91', 'symbian-armv5', 'symbian-winscw', 'symbian-mmp'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'openwave-win-arm',
    platform = 'openwave',
    arch = 'arm',
    distribution_id = 'openwave-win-arm',
    family_list = ['openwave', 'openwave-win-arm'],
    host_type = 'win32' ))

AddPlatform(Platform(
    id = 'openwave-wins',               # Windows-based simulator
    platform = 'openwave',
    arch = 'i386',
    distribution_id = 'win32-i386',     # XXXSAB should be 'openwave-wins'
    family_list = ['openwave', 'openwave-wins'],
    host_type = 'win32' ))


AddPlatform(Platform(
    id = 'win32-tm1',
    platform='tm1',
    arch='tm1',
    distribution_id = 'tm1',
    family_list = ['tm1'],
    host_type = 'win32'))
 

AddPlatform(Platform(
    id = 'linux-2.2-libc6-armv4l-zaurus',
    platform = 'zaurus',
    arch = 'arm',
    distribution_id = 'linux-2.2-libc6-armv4l-zaurus',
    family_list = ['unix', 'linux', 'linux2', 'linux-glibc-2.0',
                   'linux-arm',
                   'linux-2.2-libc6-armv4l-zaurus'] ))

## returns the output of uname stripped
def uname_output(options = ''):
    text = os.popen('uname %s' % (options), 'r').read()
    text = string.strip(text)
    return text


def set_platform(theID):
    global id
    global platform
    global arch
    global distribution_id
    global family_list
    global host_type
    global platform_qualifier

    if "@" in theID:
        theID, platform_qualifier=string.split(theID,'@',1)

    pf=PLATFORM_HASH[theID]
    id = pf.id
    platform = pf.platform
    arch = pf.arch
    distribution_id = pf.distribution_id
    family_list = pf.family_list
    host_type = pf.host_type
    

def set_platform_linux2():
    arch = uname_output('-m')
    
    if arch[0] == 'i' and arch != 'ia64':
        if not os.path.exists('/lib/libc.so.6'):
            set_platform('linux-2.0-libc5-i386')
            return
        
        if uname_output("-r")[:3] == "2.0":
            set_platform('linux-2.0-libc6-i386')
            return

        gcc_version=os.popen("gcc -dumpversion").read()
        gcc_version=string.strip(gcc_version)

        if gcc_version[0] == "3":
            set_platform('linux-2.2-libc6-gcc32-i586')
        else:
            set_platform('linux-2.2-libc6-i586')

    ## Linux PPC Q3
    elif arch == 'ppc':
        set_platform('linux-2.2-libc6-powerpc')
        
    ## Linux/Redhat SPARC
    elif arch == 'sparc64' or arch == 'sparc':
        set_platform('linux-2.2-libc6-sparc')

    ## Linux Alpha
    elif arch == 'alpha':
        set_platform('linux-2.0-libc6-alpha-gcc2.95')

    elif arch == 'ia64':
        set_platform('linux-2.4-libc6-ia64')

    elif arch == 'x86_64':
        set_platform('linux-2.6-glibc23-amd64')

    else:
        e = err.Error()
        e.Set("System detection failed for unknown Linux system.")
        raise err.error, e
        

def set_platform_netbsd1():
    arch = uname_output('-m')

    ## ns1k is the NetBSD/PPC Liberate diskless workstation
    if arch == 'ns1k':
        set_platform('netbsd-1.4-ns1k-powerpc')

    ##  normal NetBSD
    elif arch == 'i386':
        set_platform('netbsd-1.4-i386')

    else:
        e = err.Error()
        e.Set("Unconfigured NetBSD system.")
        raise err.error, e
        

def set_platform_sunos5():
    version = uname_output('-r')

    if version == '5.5' or version == '5.5.1':
        set_platform('sunos-5.5.1-sparc')

    elif version == '5.6':
        set_platform('sunos-5.6-sparc')

    elif version == '5.7':
        set_platform('sunos-5.7-sparc')

    elif version == '5.8':
        set_platform('sunos-5.8-sparc')

    else:
        e = err.Error()
        e.Set("Unconfigured SunOS system.")
        raise err.error, e


def set_platform_irix6():
    version = uname_output('-r')

    if version == "6.2":
        set_platform("irix-6.2-mips")

    elif version == "6.3":
        set_platform("irix-6.3-mips")

    elif version == "6.4":
        set_platform("irix-6.4-mips")

    elif version == "6.5":
        set_platform("irix-6.5-mips")

    else:
        e = err.Error()
        e.Set("Unconfigured IRIX system.")
        raise err.error, e      
  

def set_platform_aix4():
    revision = uname_output("-r")

    if revision == "2":
        set_platform("aix-4.2-powerpc")

    elif revision == "3":
        set_platform("aix-4.3-powerpc")

    else:
        e = err.Error()
        e.Set("Unconfigured AIX system.")
        raise err.error, e


def guess_platform():
    ## first, check for the PLATFORM environment variable
    if os.environ.has_key("SYSTEM_ID"):
        id = os.environ['SYSTEM_ID']
        try:
            set_platform(id)
        except KeyError:
            e = err.Error()
            e.Set("Unconfigured SYSTEM_ID=%s.  "\
                  "Valid SYSTEM_ID values are=\"%s\"." % (
                id, string.join(PLATFORM_HASH.keys(), ", ")))
            raise err.error, e
        else:
            return
            
    if os.name == 'mac':
        set_platform('macos-powerpc')

    elif sys.platform == 'darwin':
        set_platform('macos-carbon-powerpc-darwin-cw7')

    elif os.name == 'nt' or os.name == 'dos':
        set_platform('win32-i386')

    elif sys.platform == 'aix4':
        set_platform_aix4()

    elif sys.platform == 'freebsd2':
        set_platform('freebsd-2.2-i386')

    elif sys.platform == 'freebsd3':
        set_platform('freebsd-3.0-i386')

    elif sys.platform == 'freebsd4':
        set_platform('freebsd-4.0-i386')

    elif sys.platform == 'freebsd5':
        set_platform('freebsd-5.0-i586')

    elif sys.platform == 'freebsd6':
        set_platform('freebsd-6.0-i586')

    elif sys.platform == 'hp-uxB':
        set_platform('hpux-11.0-parisc')

    elif sys.platform == 'irix6':
        set_platform_irix6()

    elif sys.platform == 'irix646':
        set_platform_irix6()

    elif string.find(sys.platform, 'linux') >= 0:
        set_platform_linux2()

    elif sys.platform == 'netbsd1':
        set_platform_netbsd1()

    elif sys.platform == 'osf1V4':
        set_platform('osf-4.0-alpha')

    elif sys.platform == 'osf1V5':
        set_platform('osf-5.1-alpha')

    elif sys.platform == 'sunos5':
        set_platform_sunos5()

    elif sys.platform == 'unixware5':
        set_platform('unixware-7.0-i386')

    elif sys.platform == 'sco_sv3':
        set_platform('openserver-5.0.5-i386')

    elif sys.platform == 'win32':
        set_platform('win32-i386-vc6')

    elif sys.platform == 'openbsd3':
        set_platform('openbsd-3.3-i586')

    else:
        e = err.Error()
        e.Set("Unknown platform, update sysinfo.py for platform=\"%s\"." % (
            sys.platform))
        raise err.error, e
        

## exported data
id = None
platform = None
platform_qualifier = None
arch = None
distribution_id = None
family_list = None
host_type = None

## runs at load time
guess_platform()
