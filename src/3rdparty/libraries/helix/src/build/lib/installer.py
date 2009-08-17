# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: installer.py,v 1.4 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Sub-class of the Collate class from collate.py.  The Installer class
acts as a common base class for build system plugins wanting to implement
a installer through the use of build system plugins.  Installers are usually
implemented through Makefiles."""

import os
import sys
import string
import posixpath
import types
import collate
import err
import ushell
import bldreg
import bif
import dependlist
import branchlist


class Installer(collate.Collate):
    def __init__(self, module, settings):
        ## initalize parent class
        collate.Collate.__init__(self, module, settings)

        ## Umake-target identifier to target output lookup
        self.__target_hash = None

    ## uses a pre-loaded information from the build system registry
    ## to return a absolute path for a requested target ID; this is
    ## the ID the target used in it's Umakefil
        
    def get_target_path(self, target):
        ## don't load the data until needed
        if self.__target_hash == None:
            self.__load_target_hash()
        
        if self.__target_hash.has_key(target):
            return self.__target_hash[target]
        else:
            return ''

        
    ## used by installers to copy somthing to the copydir
        
    def copy_to_copydir(self, path):
        ## build the copy directory path
        if self.settings.get('copy_path'):
            base_path = bldreg.get_value('build', 'path')
            copy_path = os.path.join(base_path, self.settings['copy_path'])
            print copy_path
            copy_path = ushell.posix_path(copy_path)
            print copy_path
        else:
            self.error('no copy_path set')
            return

        ## make the copy_path directory
        if not ushell.isdir(copy_path):
            ushell.mkdir(copy_path)
        
        ushell.cp(path, copy_path)



    ## return the list of module paths on the current branch
    ## for a given target or list of targets -- this method
    ## preforms dynamic type checking to accept a single target
    ## as a string or multiple targets as a list

    def get_module_path_hash(self, target):
        ## retrieve the branch ID from the registry
        branch = bldreg.get_value('build', 'branch')

        ## find the BIF file for this branch
        branch_list = branchlist.BranchList()
        bif_file = branch_list.file(branch)
        if not bif_file:
            e = err.Error()
            e.Set("Cannot find bif file=\"%s\"." % (bif_file))
            raise err.error, e

        ## parse the BIF/XML file and get information for this build
        bif_data = bif.load_bif_data(bif_file, branch_list)

        ## feed the BIF information to a dependancy list, and set
        ## the dependancy list's targets
        depend = dependlist.DependList(bif_data, target)

        ## this takes it's best shot at returning a list of all the
        ## subdirectory names checked out by this target
        module_list = depend.distribution_list() + depend.checkout_list()
        
        base_path = ushell.posix_path(bldreg.get_value('build', 'path'))

        module_path_hash = {}
        for module in module_list:
            module_path_hash[module.id] = ushell.join(base_path, module.name)

        return module_path_hash



    def __load_target_hash(self):
        try:
            key_list = bldreg.section_key_list('targets')
        except KeyError:
            return

        ## figure out the base absolute path
        base_path = ushell.posix_path(bldreg.get_value('build', 'path'))

        self.__target_hash = {}

        for key in key_list:
            target_path = ushell.join(
                base_path, ushell.posix_path(bldreg.get_value('targets', key)))
            
            self.__target_hash[key] = target_path
