# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: header.py,v 1.5 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Writes include/platform.h."""

import os
import sys
import re
import string
import outmsg
import sysinfo
import err
import time

def update_platform_header(module, build_branch):
    """Write a new include/platform.h header file defining
    TARVER_STR_PLATFORM to the current platform."""

    path = apply( os.path.join, [ os.curdir ] +
                  string.split(module.name,"/") +
                  [ "platform.h" ])

    if not os.path.isfile(path):
        e = err.Error()
        e.Set("The required include file=\"%s\" could not be found." % (
            path))
        raise err.error, e

    ver_path = apply( os.path.join, [ os.curdir ] +
                  string.split(module.name,"/") +
                  ["PlatformVersion.ini" ])
    
    ## figure out what the platform string should be
    if os.name == 'posix':
        plat_string = sysinfo.id
    else:
        plat_string = sysinfo.platform

    if os.path.isfile(ver_path):
        ## update platform version
        ver_data = open(ver_path, "r").read()
        ver_data = string.split(ver_data, "\n")
        buildtime = time.localtime(time.time())
        build_time = time.strftime("%d %b %Y %H:%M", buildtime)
        plat_string = (ver_data)[0] + " " + build_time
        outmsg.send("Updating Platform Version = %s" % (plat_string))

    outmsg.send("updating file=\"%s\" for platform=\"%s\"" % (
        path, plat_string))

    olddata = open(path,"r").read()
    copyright=""
    if olddata[:2] == "/*":
        copyright = string.split(olddata,"*/")[0] +"*/\n\n"

    data = """%s#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#define TARVER_STR_PLATFORM "%s"
#define TARVER_STR_BUILD_BRANCH "%s"
#endif
""" % (copyright, plat_string, build_branch)

    ## Don't update the file if it already contains the right stuff
    try:
        if open(path, "r").read() == data:
            return
    except:
        pass

    try:
        fil = open(path, "w")
    except IOError:
        e = err.Error()
        e.Set("Could not write to file=\"%s\"." % (path))
        raise err.error, e

    fil.write(data)

    fil.close()
