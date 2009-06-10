#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: armerge_exe.py,v 1.2 2006/07/06 19:28:05 jfinnecy Exp $ 
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
#  Contributor(s):  
#  
#  ***** END LICENSE BLOCK ***** 
# 

"""This utility is called by UNIX Makefiles to merge a bunch of static
libraries into one library; this makes it so we don't have to have .a's listed
in order, and sometimes multiple times when they have cyclic dependancies
I'm going to regret writing this, but hopefully I'll be gone before
I have to deal with the problems it creates. -Jay.
Heh, I'm definately not going to regret writing this -- somebody else
will!!!"""

import os
import sys
import string
import getopt
import glob

import utils

ar_cmd="ar"

def find_absolute_path(_path):
    old_dir = os.getcwd()
    
    (path, basename) = os.path.split(_path)
    if path:
        os.chdir(path)
        
    absolute_path = os.getcwd()

    os.chdir(old_dir)
    return os.path.join(absolute_path, basename)

def check_archives_exists(member_list):
    for member in member_list:
        if not os.path.isfile(member):
            print "archive=\"%s\" not found" % (member)
            sys.exit(1)

def use_tempdir(temp_dir):
    try:
        utils.mkdirTree(temp_dir)
    except:
        pass

    if not os.path.isdir(temp_dir):
        print "cannot create temp directory=\"%s\"" % (temp_dir)
        sys.exit(1)

    os.chdir(temp_dir)

def unpack_member(member):
    (path, basename) = os.path.split(member)
    (base, ext) = os.path.splitext(basename)

    old_dir = os.getcwd()
    use_tempdir(base)

    os.system("%s x %s" % (ar_cmd, member))

    object_list = []
    for file in os.listdir(os.curdir):
        if file in [".", ".."]:
            continue
        
        mangled_name = "%s.%s" % (base, file)
        object_list.append(mangled_name)
        os.rename(file, os.path.join(os.pardir, mangled_name))
    
    os.chdir(old_dir)
    os.rmdir(base)
    return object_list

def ar_merge(archive, member_list, temp_dir):
    check_archives_exists(member_list)

    ## convert all paths to absolute paths
    archive = find_absolute_path(archive)
    for i in range(len(member_list)):
        member_list[i] = find_absolute_path(member_list[i])

    ## get rid of duplicates in the member list
    for member in member_list[:]:
        while member_list.count(member) > 1:
            member_list.remove(member)

    ## save old directory, change to temp dir
    old_dir = os.getcwd()
    if temp_dir:
        use_tempdir(temp_dir)

    object_list = []
    for member in member_list:
        object_list = object_list + unpack_member(member)
        
    ## make the new archive
    ## Keep the command-line length within acceptable limites
    xcmd = "%s scr %s" % (ar_cmd, archive)
    cmd = xcmd

    if os.name == "posix":
        maxCmdLineSize = 100000
    else:
        maxCmdLineSize = 4000
        
    for objName in object_list:
        if len(cmd) + len(objName) + 1 >= maxCmdLineSize:
            os.system(cmd)
            cmd = xcmd

        cmd = cmd + " " + objName

    os.system(cmd)

    ## remove all the archive members
    for object in object_list:
        try:
            os.remove(object)
        except:
            print "armerge: Failed to delete temporary object %s in %s." % (object, os.getcwd())
            sys.exit(1)

    os.chdir(old_dir)
    if temp_dir:
        os.rmdir(temp_dir)

def usage():
    print "armerge [-d temp_dir] [-a ar_cmd] <archive> member1 member2 ..."
    sys.exit(1)

def run():
    global ar_cmd
    
    archive = ""
    member_list = []
    temp_dir = ""
    
    try:
        (opts, args) = getopt.getopt(sys.argv[1:], "d:a:")
    except getopt.error:
        usage()

    for (opt, opt_arg) in opts:
        if opt == "-d":
            temp_dir = opt_arg
        if opt == "-a":
            ar_cmd = opt_arg

    try:
        archive = args[0]
    except IndexError:
        usage()

    member_list = args[1:]
    
    ar_merge(archive, member_list, temp_dir)
