# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: branch.py,v 1.19 2006/07/06 19:28:05 jfinnecy Exp $ 
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
"""Not part of the build system, but a really useful script for branching
the code base.  Given a origional brach, and target, it creates a new .bif
file, checks it in, and branches all the targets it contains."""

import os
import sys
import string
import getopt

import bif
import dependlist
import branchlist
import copy
import cvs

import types
import chaingang
import distributions
import shell
import module

class Branch:


    def process_module(self, imodule):

        if imodule.type in ["cvs", "distribution"]:
            tmpdir="branch_tmp_"+str(distributions.my_get_thread_ident())
            shell.rm(tmpdir)

            try:

                if imodule.type == "cvs":
                    imodule.checkout(
                        date = self.source_date,
                        tag = self.source_tag,
                        as = tmpdir)
                else:
                    cvs.Checkout(self.source_tag or imodule.cvs_tag,
                                 "distribution",
                                 imodule.cvs_root,
                                 tmpdir,
                                 self.source_date or imodule.cvs_date)

                cmd='cvs tag %s %s "%s"' %( self.cvs_flags,self.dash_b,  self.tag )
                print "Running: %s (in %s + %s)" % (cmd, os.getcwd(), tmpdir)
                status, output = shell.run(cmd, dir = tmpdir)
                print output
                if status:
                    print "Tagging module %s failed\n" % imodule.id

            except cvs.cvs_error:
                print "Tagging module %s failed\n" % imodule.id
            
            shell.rm(tmpdir)
            
    
    def __init__(self,
                 module_id,
                 branch_tag,
                 source_branch,
                 source_tag,
                 source_date,
                 default_profile=None,
                 default_options=None,
                 branch_or_tag = "branch",
                 ribosome=None,
                 cvs_flags = '',
                 bif_dir = None):

        self.source_tag = source_tag
        self.source_date = source_date
        self.tag = branch_tag
        
        action=string.upper(branch_or_tag)
        todo={}

        self.cvs_flags=cvs_flags

        if type(module_id) != types.ListType:
            module_id=[ module_id ]

        if branch_or_tag == "branch":
            self.dash_b="-b"
        else:
            self.dash_b=""

        ## find the BIF file for the branch name
        branch_list = branchlist.BranchList(source_tag, source_date)
        old_filename = branch_list.file(source_branch)
        if not old_filename:
            print "[%s] no BIF file for branch=\"%s\"" % (action,source_branch)
            return 0

        
        if bif_dir:
            ## Get this from HEAD
            bif_path = branchlist.BranchList().paths[bif_dir]
        else:
            ## Ugly, but works
            bif_path, old_basename = os.path.split(old_filename)
            

        ## parse old BIF file, and create the new BIF file
        ## Do not include shadows
        print "[%s] parsing file=\"%s\"" % (action,old_filename)
        bdata1 = bif.load_bif_data(old_filename, branch_list, 0)

        ## compute the dependancy list for the target

        if module_id == [ "ALL" ]:
            print "[%s] %sing all modules" % (action, string.capitalize(branch_or_tag))
            deplist1_list = bdata1.module_hash.values()
        else:
            print "[%s] computing dependancy tree for target=%s" % (action,repr(module_id))
            deplist1_list = dependlist.DependList(bdata1, module_id).list()

        ## Create a new BIFData object
        bdata2=bif.BIFData()
        bdata2.set_build_id(branch_tag)
        bdata2.set_default_cvs_root(bdata1.default_cvs_root)
        bdata2.set_default_cvs_tag(branch_tag)
        bdata2.set_default_cvs_tag_type("branch")  ## is it?

        #bdata2.defaults = copy.deepcopy(bdata1.defaults)
        #for d in bdata2.defaults:
        #     d.target = string.join(module_id)
        if module_id == [ "ALL" ]:
            bdata2.defaults = [
                bif.Default(default_profile,
                            string.join(module_id),
                            default_options) ]
        else:
            bdata2.defaults = [
                bif.Default(default_profile,
                            string.join(module_id),
                            default_options) ]
            
        for tmp in deplist1_list:
            # reset the cvs tag
            mod = copy.deepcopy(tmp)
            mod.cvs_tag_flag=0
            mod.cvs_date=""
            mod.cvs_tag_type="branch"
            mod.cvs_tag=branch_tag
            bdata2.add_module(mod)

        self.cmds = []
        if branch_or_tag == "branch":
            ## write out a new XML file for the branch
            basename = "%s.bif" % (branch_tag)
            filename = os.path.join(bif_path, basename)
            print "[%s] writing xml file=\"%s\"" % (action,filename)
            fil = open(filename, "w")
            fil.write(bdata2.write())
            fil.close()

            
            ## cvs commit the new file
            cmd="cvs add %s" % basename
            print "Running: %s" % cmd
            status, output = shell.run(cmd, dir=bif_path)
            print output
            cmd='cvs commit -m "branch script submission" %s' % basename
            print "Running: %s" % cmd
            status, output = shell.run(cmd, dir=bif_path)
            print output

        modules_to_tag = []

        build_module = module.CreateModule("buildsystem","")
        if ribosome:
            build_module.set_cvs_root("helix")
            build_module.name="ribosome/build"
        else:
            build_module.set_cvs_root("real")
            build_module.name="build"

        modules_to_tag.append(build_module)

        ## Tag/Branch BIF files
        ## FIXME: BIF inheritance makes it hard to know which BIF
        ## files to tag/branch, so we tag/branch *all* of them.

        for (prefix, path)  in branch_list.path_list:
            if path[0] == '[':
                tmp=string.split(path[1:],"]")
                repository=tmp[0]
                cvspath=string.join(tmp[1:],"]")

                if cvspath and cvspath[0]=='/':
                    cvspath=cvspath[1:]

                mod=module.CreateModule("%s_bifs" % prefix, cvspath)
                mod.set_cvs_root(repository)

                modules_to_tag.append(mod)

        

        ## compute the dependancy list for the target
        print "[%s] computing dependancy tree for target=%s" % (action, repr(module_id))
        ## print some useful info
        if module_id == [ "ALL" ]:
            deplist2_list = bdata2.module_hash.values()
            print "[%s] targets=\"%d\"" % (action, len(deplist2_list))
        else:
            deplist2 = dependlist.DependList(bdata2, module_id)

            print "[%s] targets=\"%d\"" % (action, len(deplist2.list()))
            print "[%s] cvs targets=\"%d\"" % (action, len(deplist2.checkout_list()))
            print "[%s] dist targets=\"%d\"" % (action, len(deplist2.distribution_list()))


        dists_done={}
        for mod in deplist1_list:

            if mod.type not in ["cvs", "distribution" ]:
                continue

            if mod.type == "distribution":
                if dists_done.has_key(mod.cvs_root):
                    continue
                dists_done[mod.cvs_root]=1

            modules_to_tag.append(mod)

        #print "TAGME: %s" % repr(modules_to_tag)
        chaingang.ProcessModules_anyorder(modules_to_tag, self.process_module)


## print usage information
def usage():
    print 'Usage: branch.py [-F] [-T] [-R] [-j number-of-jobs] [-r cvs-tag-to-branch-from] [-D cvs-date-to-branch-from] [-B bif location] -b BIF-file-id TARGET NEW_CVS_BRANCH_NAME'
    print '  -F  move tag/branch tag if it already exists'
    print '  -T  Create a tag instead of a branch'
    print '  -R  Tag the Ribosome system, Without -R, [real]build '
    print '      will be taggedto be tagged instead.'
    print 'If target is "ALL", all targets will be branched.'
    print 'If the new branch name can be [LOCATION]BRANCHNAME if you like.'
    sys.exit(1)


## MAIN
if __name__ == '__main__':
    make_cmd="make"
    opt_list, arg_list = getopt.getopt(sys.argv[1:], 'r:b:D:Fj:RB:')

    if len(arg_list) != 2:
        usage()

    ## create a hash from the opt_list
    opt_hash = {}
    for opt in opt_list: opt_hash[opt[0]] = opt[1]

    source_tag = ''
    source_branch = ''
    source_date=''
    ribosome=None
    cvs_flags=''
    action="branch"
    bif_dir=None

    if opt_hash.has_key('-r'):
        source_tag = opt_hash['-r']

    if opt_hash.has_key('-R'):
        ribosome=1
        
    if opt_hash.has_key('-T'):
        action="tag"

    if opt_hash.has_key('-b'):
        source_branch = opt_hash['-b']

    if opt_hash.has_key('-B'):
        bif_path = opt_hash['-B']

    if opt_hash.has_key('-D'):
        source_date = opt_hash['-D']

    if opt_hash.has_key('-F'):
        cvs_flags = cvs_flags+" -F"

    target_module_id = arg_list[0]
    branch_name = arg_list[1]

    if branch_name[0] == '[':
        tmp=string.split(branch_name[1:],"]",1)
        bif_dir=tmp[0]
        branch_name=tmp[1]

    if not source_branch:
        usage()

    import build_exe
    build_exe.call_buildrc()

    Branch(target_module_id,
           branch_name,
           source_branch,
           source_tag,
           source_date,
           None,
           None,
           "branch",
           ribosome,
           cvs_flags,
           bif_dir)

