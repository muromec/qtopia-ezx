# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: ushell.py,v 1.3 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Univeral shell utilities.  These provide file copy, removal, isdir, and
other path and file manipulation using UNIX style paths on all platforms.
All the path translation is done for you, so just program like you're on
a UNIX box and be happy."""

import os
import sys
import string
import getopt
import stat
import re
import posixpath
import glob
try:
    import macfs, MacOS
except:
    pass

import shell


class UniversalShell:
    """Multi-platform shell utilities which do path convsion to use
    UNIX (posix) style paths on all platforms."""

    def exists(self, path):
        return os.path.exists(self.native_path(path))
    
    def isfile(self, path):
        return os.path.isfile(self.native_path(path))

    def isdir(self, path):
        return os.path.isdir(self.native_path(path))

    def islink(self, path):
        return os.path.islink(self.native_path(path))

    def getcwd(self):
        return self.posix_path(os.getcwd())
        
    def ls(self, path):
        ls_list = []

        for item in  os.listdir(self.native_path(path)):
            ls_list.append(self.posix_path(item))

        return ls_list
    
    def cp(self, source_path, destination_path):
        native_source_path = self.native_path(source_path)
        native_destination_path = self.native_path(destination_path)        
        shell.cp(native_source_path, native_destination_path)

    def cpdir(self, source_path, destination_path):
        native_source_path = self.native_path(source_path)
        native_destination_path = self.native_path(destination_path)
        shell.cpdir(native_source_path, native_destination_path)

    def rm(self, path):
        shell.rm(self.native_path(path))
        
    def find(self, path, match, unmatch):
        found_file_list = []

        ## use match/unmatch to filter the find list
        match_re = None
        unmatch_re = None
        
        if len(match):   match_re = re.compile(match)
        if len(unmatch): unmatch_re = re.compile(unmatch)

        ## find stuff
        file_list, dir_list = shell.find(self.native_path(path))
        file_list = file_list + dir_list

        ## if there are no filters
        if not (match_re or unmatch_re):
            for file in file_list:
                found_file_list.append(self.posix_path(file))
            return found_file_list

        ## filter file list
        for file in file_list:
            file = self.posix_path(file)

            if match_re and match_re.match(file):
                found_file_list.append(file)
                continue
            if unmatch_re and unmatch_re.match(file):
                continue
            if not match_re:
                found_file_list.append(file)
            
        return found_file_list

    def chdir(self, path):
        os.chdir(self.native_path(path))

    def mkdir(self, path):
        shell.mkdir(self.native_path(path))

    def run(self, path):
        return shell.run(self.native_path(path))


class POSIXUniversalShell(UniversalShell):
    def posix_path(self, path):
        return path
    
    def native_path(self, path):
        return path


class DOSUniversalShell(UniversalShell):
    posix_path_translation_table = string.maketrans('\\', '/')
    dos_path_translation_table = string.maketrans('/', '\\')
    
    def posix_path(self, path):
        if not len(path): return ''
        return string.translate(path, self.posix_path_translation_table)

    def native_path(self, path):
        if not len(path): return ''
        return string.translate(path, self.dos_path_translation_table)


class MacUniversalShell(UniversalShell):
    def getcwd(self):
        return self.posix_path('%s' % (os.getcwd()))
        
    def posix_path(self, path):
        if not len(path): return ''

        ## absolute or relative path
        #if path[0] == ':' or string.find(path, ':') < 0:
        if path[0] == ':':
            absolute_path_flag = 0
            path = path[1:]
        elif string.find(path, ':') < 0:
            absolute_path_flag = 0
        else:
            absolute_path_flag = 1
            
        if path[-1] == ':':
            directory_flag = 1
            path = path[:-1]
        else:
            directory_flag = 0

        path_list = []
        for path_item in string.split(path, ':'):
            if not len(path_item):
                path_list.append('..')
            else:
                path_list.append(path_item)

        posix_path = string.join(path_list, '/')

        if absolute_path_flag:
            posix_path = '/%s' % (posix_path)
            
        if directory_flag:
            posix_path = '%s/' % (posix_path)

        return posix_path

    def native_path(self, path):
        if not len(path): return ''
        absolute_path_flag = 0

        ## remove any multiple path separators to make parsing easier
        while 1:
            old_path = path
            path = string.replace(path, '//', '/')
            if path == old_path:
                break

        ## recognize a absolute path, chop it off of the posix
        ## path so it won't be translated; set absolute path flag
        ## so we can add the leading colen if this is a relative path
        if path[0] == '/':
            path = path[1:]
            absolute_path_flag = 1

        if path[:2] == './':
            path = path[2:]

        path_list = []
        for item in string.split(path, '/'):
            if item == '..':
                path_list.append('')
            else:
                path_list.append(item)

        mac_path = string.join(path_list, ':')

        ## absolute or relative path
        if not absolute_path_flag:
            mac_path = ':%s' % (mac_path)

        return mac_path


## instace of shell utilities based on platform
_ushell = None

if os.name == 'posix':
    _ushell = POSIXUniversalShell()
elif os.name == 'nt' or os.name == 'dos':
    _ushell = DOSUniversalShell()
elif os.name == 'mac':
    _ushell = MacUniversalShell()


## exported shell utilities
def ls(path = './'):
    return _ushell.ls(path)

def cp(source_path, destination_path):
    _ushell.cp(source_path, destination_path)

def cpdir(source_path, destination_path):
    _ushell.cpdir(source_path, destination_path)
    
def rm(path):
    _ushell.rm(path)

def find(path = './', match = '', unmatch = ''):
    return _ushell.find(path, match, unmatch)

def chdir(path):
    _univeal_shell.chdir(path)

def mkdir(path):
    _ushell.mkdir(path)

def exists(path):
    return _ushell.exists(path)

def isfile(path):
    return _ushell.isfile(path)

def isdir(path):
    return _ushell.isdir(path)

def islink(path):
    return _ushell.islink(path)

def getcwd():
    return _ushell.getcwd()

def posix_path(path):
    return _ushell.posix_path(path)

def native_path(path):
    return _ushell.native_path(path)

## path functions
join = posixpath.join
split = posixpath.split
basename = posixpath.basename

## path constants
curdir = './'
pardir = '../'
