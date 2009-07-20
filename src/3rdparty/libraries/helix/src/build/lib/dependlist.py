# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: dependlist.py,v 1.27 2006/07/06 19:28:05 jfinnecy Exp $ 
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
"""Functions and clases for computing module dependency lists given a
BIFData class."""

import os
import sys
import string
import types
import err
import sysinfo
import bldreg
import sdk

class ModuleDependancyStack:
    """Given a list of module_id's, compute a stack of levels where
    level n+1 depends on the modules in level n."""

    def __init__(self, bif_data, module_id_list, filter_func = None):
        self.bif_data = bif_data
        self.doing={}
        self.done={}
        self.ordered_list=[]
        self.filter_func=filter_func

        ## resolve the module_ids with the bif_data
        for module_id in module_id_list:
            self.recursive_resolv_dependencies(module_id,"-")

        ## Cleanup
        self.done={}

    def circular_dependancy_error(self, mod):
        l = [ mod ]
        tmp = self.doing[mod]
        while mod != tmp:
            l.append(tmp)
            tmp = self.doing[tmp]
        l.append(mod)
            
        e = err.Error()
        import bif
        e.Set("A circular module dependancy was detected in the bif "\
              "file while computing the dependancy list for the target "\
              "involving the following modules: %s" %
              string.join(l,"->"))
        raise err.error, e

    def dependancy_error(self, module_id, depmodule_id):
        e = err.Error()
        e.Set("While computing the dependancy list for your target, "\
              "module id=\"%s\" has a dependancy on module=\"%s\" which "\
              "was not found in the bif file." % (module_id, depmodule_id))
        raise err.error, e

    def recursive_resolv_dependencies(self, module_id, last = ""):
        if self.doing.has_key(module_id):
            self.circular_dependancy_error(module_id)
                
        if self.done.has_key(module_id):
            return
        self.done[module_id]=1

        ## lookup dependancy module; fail if it doesn't exist
        try:
            module = self.bif_data.module_hash[module_id]
        except KeyError:
            self.dependancy_error(last, module_id)

        # Why compute dependencies for modules which will not be built?
        #if not module.no_build_flag:

        ## Check {exclude,include}{platform,profile}
        if self.filter_func and not self.filter_func(module):
            return

        for depmod_id in module.dependancy_id_list:
            self.doing[module_id]=depmod_id
            self.recursive_resolv_dependencies(depmod_id, module_id)
            
        if len(module.dependancy_id_list):
            del self.doing[module_id]
        
        self.ordered_list.append(module)


    def get_ordered_module_list(self):
        return self.ordered_list



class DependList:
    """Once loaded up with a BIFData class and a target list, this class
    can be used to return the dependency list for the target in a number of
    ways, and in a number of formats."""

    def __init__(self, bif_data, target_list):
        self.bif_data = bif_data

        if type(target_list) == types.StringType:
            self.target_list = [target_list]
        elif type(target_list) == types.ListType:
            self.target_list = target_list

        self.defines = {}

        ## This could potentially be a very very heavy calculation
        ## Luckily, the normal case only takes one or two iterations
        while 1:
            num_defines=-1
            while len(self.defines) > num_defines:
                num_defines = len(self.defines)

                tmp=self.target_list[:]

                ## Include "always" modules
                for (modid, module) in self.bif_data.module_hash.items():
                    if module.attributes.get("always",None):
                        tmp.append(modid)
                        
                self.module_list = self.compute_dependancy_list(tmp,
                                                                self.chk_platform)

                # 
                # Propagate the static_build attribute to dependant modules
                # This could be done once and for all above really...
                # No error checking should be needed since all dependancies
                # Have already been resolved once above. -Hubbe
                #

                ## This is actual dual mark-and-sweep procedure,
                ## once for static propagation, and once for dynamic
                ## propagation

                statics = []            # A list containing all static modules
                statics_done = {}       # And a hash for fast lookups
                for module in self.module_list:
                    if  module.build_static_flag:
                        statics_done[module] = 1
                        statics.append(module)

                # Magic trick, we append stuff to 'statics' from inside the loop
                # This way no recursion is needed. -Hubbe
                for module in statics:
                    for depid in module.dependancy_id_list:
                        depmod = self.bif_data.module_hash[depid]
                        if not statics_done.has_key(depmod) \
                               and not depmod.build_dynamic_only_flag:
                            statics.append(depmod)
                            statics_done[depmod] = 1

                dynamics = []         # A list containing all dynamic modules
                dynamics_done = {}    # And a hash for fast lookups

                ## All targets are built dynamically by default
                for modid in self.target_list:
                    module=self.bif_data.module_hash[modid]
                    if not module.build_static_only_flag:
                        dynamics_done[module] = 1
                        dynamics.append(module)

                ## All dynamic_only modules are built dynamically
                for module in self.module_list:
                    if module.build_dynamic_only_flag:
                        dynamics_done[module] = 1
                        dynamics.append(module)

                # Magic trick, we append stuff to 'dynamics' from inside the loop
                # This way no recursion is needed. -Hubbe
                for module in dynamics:
                    for depid in module.dependancy_id_list:
                        depmod = self.bif_data.module_hash[depid]
                        if not dynamics_done.has_key(depmod) \
                               and not depmod.build_static_only_flag:
                            dynamics.append(depmod)
                            dynamics_done[depmod] = 1

                for module in self.module_list:
                    if statics_done.get(module):
                        if dynamics_done.get(module):
                            module.set_build_static()
                        else:
                            module.set_build_static_only()


                ## Read all defines in these modules
                ## If new defines are found since last iteration, start from top
                ## This could be really really really slow if we're unlucky.
                self.defines = {}
                for module in self.module_list:
                    for d in module.defines.keys():
                        self.defines[d]=module.defines[d]

                # Find any source_dependancies which are not already
                # in the list of modules and mark them as nobuild and
                # add them to the list of modules.
                modules_done = {}
                for module in self.module_list:
                    modules_done[module]=1
                    module.source_dep_only=0
                    module.checkin_dep_only=0

                ## Source dependencies do *not* add defines
                ## Same for checkout dependencies
                for module in self.module_list:

                    if module.checkin_dep_only:
                        continue
                    
                    for sourcedep in module.source_dependancy_id_list:
                        sdep_module = self.bif_data.module_hash[sourcedep]
                        if not modules_done.has_key(sdep_module):
                            sdep_module.source_dep_only = 1
                            self.module_list.append(sdep_module)
                            modules_done[sdep_module]=1

                    if module.source_dep_only:
                        continue
                    
                    for cidep in module.checkin_dependancy_id_list:
                        cidep_module = self.bif_data.module_hash[cidep]
                        if not modules_done.has_key(cidep_module):
                            cidep_module.checkin_dep_only = 1
                            cidep_module.source_dep_only = 1
                            self.module_list.append(cidep_module)
                            modules_done[cidep_module]=1

                        

            ## Check for missing SDKs, if additional
            ## dependencies were added, iterate again
            if not self.check_sdks_pass1():
                break

            #print "SDK DEP ADDED, GO FISH!!! (%d)" % len(self.module_list)

        ## Now that we have figured out all dependencies
        ## we can do the required modifications

        bldreg.set_value("build","defines",self.defines)

        for module in self.module_list:
            if module.source_dep_only:
                module.no_build_flag=1

        self.check_sdks_pass2()
        self.check_duplicates()

    def check_duplicates(self):
        mhash={}
        namehash={}
        bl=self.build_list()
        for module in bl:
            mhash[module.id]=module
            namehash[module.name]=module

        for module in bl:
            name=module.name
            
            while 1:
                m2=namehash.get(name)
                if m2 and m2 != module:
                    deps=[ m2.id, module.id ]
                    deps_done={}
                    for d in deps:
                        #print d
                        n=deps_done.get(d,0)
                        deps_done[d]=n+1
                        if n:
                            continue
                        if mhash.has_key(d):
                            deps.extend(mhash[d].dependancy_id_list)

                    #print deps_done

                    if deps_done[m2.id] + deps_done[module.id] > 2:
                        break

                    print "module[%s].deps+=%s (conflict)" % (m2.desc(), module.id)
                    m2.dependancy_id_list.append(module.id)
                    break

                pos = string.rfind(name,"/")
                if pos == -1:
                    break
                else:
                    name = name[:pos]



    def check_sdk_platform(self, sdk):
        return sdk.check_platform(sysinfo.platform)

    def sdk_iterator(self, modules, fun, *args):
        """Call 'func' for every undefined SDK in 'modules'"""

        #print "sdk_iterator: %s" % repr(fun)

        for module in modules:
            if not self.chk_platform(module):
                continue

            if module.checkin_dep_only:
                continue

            for s in module.sdks:
                if not self.check_sdk_platform(s):
                    continue
                if sdk.GetPath(s.name):
                    continue

                #print "FOO %s %s\n" % (repr(fun),repr(args))
                ret=apply( fun, (module, s) + args)
                if ret:
                    return ret

        return 0


    def mark_sdk_iter(self, module, s, sdktmp):
        """Create a hash which maps sdk names to their definition,
        the hash will contain zeroes for SDKs which are required but
        not yet defined"""
        
        if sdktmp.has_key(s.name) and sdktmp[s.name]:
            return

        sdktmp[s.name]=0

        if not s.path:
            return

        if not os.path.exists(s.path):
            if module.type not in  [module.MODULE_CVS,
                                    module.MODULE_DISTRIBUTION]:
                return
            if module.name != s.path[:len(module.name)]:
                return

        if s.ifexists and not os.path.exists(s.ifexists):
            return

        sdktmp[s.name]=s.path
        #print "SDK %s = %s" % (s.name, s.path)


    def find_sdk_module_iter(self, module, s, sdk_name, name_only = 0):
        """Iterator which finds a module which defined a previously
        undefined SDK"""

        if name_only and module.type != module.MODULE_NAME_ONLY:
            return 0
        
        tmp={}
        self.mark_sdk_iter(module, s, tmp)
        
        if tmp.has_key(sdk_name) and tmp[sdk_name]:
            return module.id
        

    def add_sdk_dependencies_iter(self, module, s, sdktmp):
        """Go through all undefined SDKS and try to define them"""
        ## Zero means it's required but not yet defined
        if sdktmp[s.name] == 0:
            #print "UNDEFINED SDK '%s', ask the trashpile..." % s.name
            ## Man, this is going to be slow...
            m=self.sdk_iterator(self.bif_data.module_hash.values(),
                                self.find_sdk_module_iter,
                                s.name, 1)

            if not m:
                m=self.sdk_iterator(self.bif_data.module_hash.values(),
                                    self.find_sdk_module_iter,
                                    s.name, 0)

                

            #print "Trashpile says: %s" % repr(m)
            if m:
                print "module[%s].deps+=%s (SDK)" % (module.desc(), m)
                module.dependancy_id_list.append(m)
                module.source_dependancy_id_list.append(m)
                self.bif_data.module_hash[m].set_attribute('sdk_depend_only')
                sdktmp[0]=sdktmp[0]+1
                sdktmp[s.name]="..."

        return 0

    def check_sdks_pass1(self):
        sdktmp={}
        # Find undefined SDKs
        self.sdk_iterator(self.module_list,
                          self.mark_sdk_iter,
                          sdktmp)

        sdktmp[0]=0

        # Find modules which define those SDKs
        self.sdk_iterator(self.module_list,
                          self.add_sdk_dependencies_iter,
                          sdktmp)

        ## Return the number of dependencies that were added
        return sdktmp[0]
                    

    ## FIXME:
    ##  1) Only print SDK warning messages once, even if there
    ##     are multiple SDK tags
    ##  2) If there are multiple SDK tags, but only one with a
    ##     an error message, be sure to use that error message
    def finalize_sdks_iter(self, module, s, sdktmp):

        if sdktmp[s.name]:
            sdk.SetPath(s.name, sdktmp[s.name])
            return
        
        sdep=None
        try:
            sdep=module.source_dep_only
        except:
            pass
        
        if sdep:
            return
        
        print
        print "=============================================="
        print "Failed to find path to SDK named '%s'." % s.name
        if s.error_message:
            print
            print s.error_message
        else:
            print "Please read documentation for instructions on"
            print "how to obtain and install this SDK."
            
        print 
        print "See file://%s/doc/buildrc.html for more" % os.environ.get("BUILD_ROOT")
        print "information about .buildrc files and SetSDKPath()."
        print 
        
        print "Your build will continue in 10 seconds, but "
        print "will probably not complete successfully."
        print "=============================================="
        print
        import time
        time.sleep(10);

    def check_sdks_pass2(self):
        sdktmp={}
        # Find all SDKs
        self.sdk_iterator(self.module_list,
                          self.mark_sdk_iter,
                          sdktmp)

        # Export SDK defines or print error if undefined
        self.sdk_iterator(self.module_list,
                          self.finalize_sdks_iter,
                          sdktmp)
                    
        

    ## used to determin platforms for include/exclude list
    def chk_platform_list(self, platform_list, family_list):
        for platform in platform_list:
            if platform in family_list:
                return 1
        return 0

    ## used to determin profiles for include/exclude list
    def chk_profile_list(self, profile_list, profile):
        import fnmatch
        for p in profile_list:
            if fnmatch.fnmatch(profile, p):
                return 1
        return 0

    ## used to determin profiles for include/exclude list
    def chk_define_list(self, define_list, defines):
        for d in define_list:
            if defines.has_key(d):
                return 1
        return 0

    def chk_platform(self, module, fam = None, profile = None, defines = None):
        if not fam:
            fam = sysinfo.family_list

        if module.platform_include_list_flag:
            if not self.chk_platform_list(module.platform_include_list, fam):
                return 0
        if module.platform_exclude_list_flag:
            if self.chk_platform_list(module.platform_exclude_list, fam):
                return 0

        if not profile:
            profile=os.environ.get("PROFILE_ID","default")

        profile=os.path.basename(profile)

        if module.profile_include_list_flag:
            if not self.chk_profile_list(module.profile_include_list, profile):
                return 0
        if module.profile_exclude_list_flag:
            if self.chk_profile_list(module.profile_exclude_list, profile):
                return 0

        if not defines:
            defines = self.defines

        if module.define_include_list_flag:
            if not self.chk_define_list(module.define_include_list, defines):
                return 0

        if module.define_exclude_list_flag:
            if self.chk_define_list(module.define_exclude_list, defines):
                return 0

        return 1

    ## compute a combined dependancy list given a list of targets
    def compute_dependancy_list(self, target_list, ff = None):
        mds = ModuleDependancyStack(self.bif_data, target_list, ff)
        return mds.get_ordered_module_list()

    def cvs_sublist(self, cvs_tag = None, default_flag = 0):
        chk_func = lambda module: module.type == module.MODULE_CVS
        
        module_name_list = []
        for module in self.module_list:
            if not chk_func(module):
                continue
            
            ## if there was no cvs tag set, we want to continue
            ## unless we are looking for the default modules

            if not module.cvs_tag_flag and default_flag:
                module_name_list.append(module.name)
            elif module.cvs_tag == cvs_tag:
                module_name_list.append(module.name)

        return module_name_list

    ## this is better than it use to be...
    def get_sublist(self, chk_func, return_type, cvs_tag):
        func = None
        if return_type == "module":
            func = lambda module: module
        elif return_type == "id":
            func = lambda module: module.id
        elif return_type == "name":
            func = lambda module: module.name
        elif return_type == "path":
            func = lambda module: module.path() ## func = module.Module.path  #less obvious-hubbe

        sublist = []
        for module in self.module_list:

            ## Fix bug only for new BIFs
            if module.bif_version >= 203:
                if not self.chk_platform(module):
                    continue
            
            if not chk_func(module):
                continue
            
            if cvs_tag == None or module.cvs_tag == cvs_tag:
                sublist.append(func(module))
                
        return sublist

    def list(self, return_type = 'module', cvs_tag = None):
        func = lambda module: 1
        return self.get_sublist(func, return_type, cvs_tag)

    ## the list of modules which are not distribution
    ## modules and are not name-only modules
    def checkout_list(self, return_type = 'module', cvs_tag = None):
        func = lambda module: module.type == module.MODULE_CVS
        return self.get_sublist(func, return_type, cvs_tag)

    ## the list of modules which are distribution
    ## modules and are not name-only modules
    def distribution_list(self, return_type = 'module', cvs_tag = None):
        def func(module, self = self):
            if not self.chk_platform(module):
                return 0
            if module.type != module.MODULE_DISTRIBUTION:
                return 0
            return 1

        return self.get_sublist(func, return_type, cvs_tag)

    ## the list of modules which are distribution
    ## modules and are not name-only modules
    def version_file_list(self, return_type = 'module', cvs_tag = None):
        func = lambda module: module.version_file_flag
        return self.get_sublist(func, return_type, cvs_tag)

    ## the list of modules which are not distribution
    ## modules and are not name-only modules and are
    ## no marked as 'no build' modules
    def build_list(self, return_type = 'module', cvs_tag = None):
        def func(module, self = self):
            if module.type != module.MODULE_CVS:
                return 0
            if not self.chk_platform(module):
                return 0
            if module.no_build_flag:
                return 0
            return 1

        return self.get_sublist(func, return_type, cvs_tag)

    ## returns a list of the unique branches found in the list
    def cvs_tag_list(self):
        branch_list = []
        for module in self.checkout_list():
            if not module.cvs_tag_flag:
                continue
            
            if not branch_list.count(module.cvs_tag):
                branch_list.append(module.cvs_tag)

        return branch_list


## MAIN
if __name__ == '__main__':
    import getopt
    import bif

    import build_exe
    build_exe.call_buildrc()

    focus = ''
    xml=0
    all=0
    
    opt_list, arg_list = getopt.getopt(sys.argv[1:], 'apf:')
    for opt in opt_list:
        if opt[0] == '-f':
            focus=opt[1]
        if opt[0] == '-p':
            xml=1
        if opt[0] == '-a':
            all=1

    if len(arg_list) < 2:
        print 'usage: python %s BIF target1 target2 ...' % (sys.argv[0])
        sys.exit(1)

    branch_name = arg_list[0]
    target_list = arg_list[1:]
    if not focus:
        focus = target_list[0]

    import branchlist
    branch_list = branchlist.BranchList()
    bif_filename = branch_list.file(branch_name)

    ## load BIF file
    bif_data = bif.load_bif_data(bif_filename)
    depend_list = DependList(bif_data, target_list)

    print '# TARGET LIST: %s' % (string.join(target_list))
    print '# BIF FILE: %s' % (bif_filename)
    print '# TOTAL TARGETS: %d' % (len(depend_list.list()))
    print '# CHECKOUT TARGETS: %d' % (len(depend_list.checkout_list()))
    print '# DISTRIBUTION TARGETS: %d' % (len(depend_list.distribution_list()))

    def dump(module, ind, xtra=""):
        if xml:
            print module.write()
            print
        else:
            attr=[]
            if module.no_build_flag:
                attr.append("NB")
            if module.build_dynamic_only_flag:
                attr.append("D")
            if module.build_static_only_flag:
                attr.append("S")
            if module.build_static_flag:
                attr.append("SD")

            print "%s-%s(%s)%s" % (" |"*ind, module.id, string.join(attr,","),xtra)


    def rdump(modid, modmap, done, indent=0):
        if not modmap.has_key(modid):
            return
        module=bif_data.module_hash[modid];
        if done.has_key(modid):
            sys.stdout.write("<%4d> " % done[modid])
            dump(module, indent)
            if not all:
                return
        else:
            mnum = len(done)+1
            done[modid] = mnum
            sys.stdout.write("[%4d] " % mnum)
            dump(module, indent)

        for i in modmap[modid]:
            rdump(i, modmap, done, indent+1)


    print "BUILD DEPEND TREE:"
    dep={}
    for m in depend_list.list():
        dep[m.id]=m.dependancy_id_list

    rdump(focus, dep, {})
    print


    print "REVERSE DEPEND TREE:"
    dep={}
    for m in depend_list.list():
        dep[m.id]=[]
        for i in m.dependancy_id_list:
            dep[i]=[]
        
    for m in depend_list.list():
        for i in m.dependancy_id_list:
            dep[i].append(m.id)

    rdump(focus, dep, {})
    print
