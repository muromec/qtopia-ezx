# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: buildapp.py,v 1.55 2006/07/20 20:41:06 jfinnecy Exp $ 
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
"""The main build system application class, BuildApp, lives here."""

import os
import sys
import string
import time
import re
import traceback
import getopt
import posixpath
import time

## Macintosh stuff
try:
    import macfs
except:
    pass

## our own stuff
import log
log.debug( 'Imported: $Id: buildapp.py,v 1.55 2006/07/20 20:41:06 jfinnecy Exp $' )
import output
import outmsg
import shell
import bldreg
import sysinfo
import err
import chaingang
import version

import stat


##
## begin entry points
##

def RunBuild(settings_list):
    app = BuildApp(settings_list)
    app.main()


##
## end entry points
##


OPTION_STRING = "klxvqfecCuynUhaom:d:t:s:r:j:p:P:D:"


class BuildApp:
    IDENT = 'Build System (V%s)' % version.getVersionString()

    def __init__(self, settings_list):
        ## the argument list
        self.option_list, self.argument_list = \
                          getopt.getopt(settings_list, OPTION_STRING)
        log.debug( "Unmodified option-list: %s" % self.option_list )

        ## Set default branch if no branch requested on command line.
        if not self.isBranchSpecified():
            self.setDefaultBranch()                                  
        log.info( "Using options: %s" % self.option_list )
        
        ## the data file used for this build
        self.build_xml_file = ''

        ## by-name hash table of all modules in the
        ## build data file, self.build_xml_file
        self.module_hash = {}

        ## hash table of settings
        self.settings = {}

        ## list of build choices, given on the command line
        ## by -t debug,archive,bla1,bla2...  this is really
        ## kind of a hack for getting additional information
        ## to the build system without changing the remote
        ## build system protocol
        self.settings['build_options'] = []

        ## name of code package to create if in archive mode
        self.archive_filename = 'archive.rna'

        ## where built targets get copied to
        self.build_output_file = 'build.out'
        self.build_branch = ''
        self.build_type = 'debug'
        self.skip_to_module = ''
        self.archive_name = ''
        self.full_archive_name = ''

        ## cvs-tag information
        self.everything_from_cvs_tag = ''
        self.default_cvs_tag = ''
        self.default_cvs_tag_type = ''

        self.everything_from_cvs_date = ''
        self.default_cvs_date = ''

        self.default_cvs_root = ''

        ## list of target ids buildapp was invoked with
        self.target_id_list = []

        ## object of type DependList formed from the combination
        ## of targets in self.target_id_list
        self.depends = None
        
        # for Mac OS X uber project, we build a dependency dictionary with
        # the build list of each module
        if os.environ.get('BUILD_ON_PLATFORM','') == 'MacOSX':
                self.module_id_build_dict = {}

    def isBranchSpecified( self ):
        log.trace( 'entry' )
        specified = 0
        for ( k , v ) in self.option_list:
            if '-m' == k:
                specified = 1
        log.trace( 'exit' , [ specified ] )
        return specified
                
    def setDefaultBranch( self ):
        log.trace( 'entry' )
        branch = bldreg.get_value_default('build', 'branch', 'helix')
        self.option_list.append( ('-m' , branch ) )
        log.trace( 'exit' )        
        
    def main(self):
        log.trace('entry')
        log.info('Running BuildApp::main() in %s' % os.getcwd() )
        self.setup_tasks()
        self.run()
        self.shutdown_tasks()
        log.trace('exit')


    def setup_tasks(self):
        ## setup outmsg callbacks for local filtering
        outmsg.set_send_hook(self.send_cb)
        outmsg.set_error_hook(self.error_cb)
        outmsg.set_debug_hook(self.debug_cb)
        outmsg.set_verbose_hook(self.verbose_cb)

        self.parse_settings()


    def shutdown_tasks(self):
        ## remove output hooks
        outmsg.set_send_hook(None)
        outmsg.set_error_hook(None)
        outmsg.set_debug_hook(None)
        outmsg.set_verbose_hook(None)

        ## remove output file
        if self.output_name != '-':
            if hasattr(sys.stdout, "RemoveOutputFile"):
                sys.stdout.RemoveOutputFile(self.output_name)


    def getopt_list(self, option):
        list = []

        for option_pair in self.option_list:
            if option_pair[0] == option:
                list.append(option_pair[1])

        return list


    def getopt_bool(self, option):
        list = self.getopt_list(option)

        ## returns a simple "true" for a option without arguments
        if len(list):
            return 1

        return 0


    def getopt_string(self, option):
        list = self.getopt_list(option)

        if not len(list):
            return ''

        return list[0]


    def parse_settings(self):
        self.settings['verbose_mode_flag']  = self.getopt_bool('-v')
        self.settings['quiet_mode_flag']    = self.getopt_bool('-q')
        self.settings['clean_mode_flag']    = self.getopt_bool('-c')
        self.settings['clobber_mode_flag']  = self.getopt_bool('-C')
        self.settings['update_mode_flag']   = self.getopt_bool('-u')
        self.settings['yes_mode_flag']      = self.getopt_bool('-y')
        self.settings['no_umake_mode_flag'] = self.getopt_bool('-n')
        self.settings['umake_only_flag']    = self.getopt_bool('-U')
        self.settings['checkout_only_flag'] = self.getopt_bool('-h')
        self.settings['archive_build_flag'] = self.getopt_bool('-a')
        self.settings['no_compile_flag']    = self.getopt_bool('-o')
        self.settings['no_make_depend_flag']= self.getopt_bool('-e')
        self.settings['no_make_copy_flag']=   self.getopt_bool('-l')
        self.settings['no_cvs_checkout_flag']= self.getopt_bool('-k')

        ## REQUIRED build branch argument
        self.build_branch = self.getopt_string('-m')


        ## PROFILE (overrieds PROFILE_ID)
        profile = self.getopt_string("-P")
        if profile:
            os.environ["PROFILE_ID"]=profile


        ## halt priority
        halt_priority = self.getopt_string("-p")
        if halt_priority not in ["", "red", "yellow", "green"]:
            e = err.Error()
            e.Set("The -p option requires one of three arguments: "\
                  "red, yellow, or green.")
            raise err.error, e
        else:
            self.settings['halt_priority'] = halt_priority

        ## copy dir
        copy_path = self.getopt_string('-d')
        if self.getopt_string('-d'):
            self.settings['copy_path'] = self.getopt_string('-d')

        ## build_type {debug, release, ...} -> now build_options
        ## to allow multiple options to the build system
        build_options = self.getopt_list('-t')

        ## special handling of the 'debug' and 'release' options
        if build_options.count('debug'):
            self.build_type = 'debug'
            ## get rid of the conflicting option
            while build_options.count('release'):
                build_options.remove('release')

        elif build_options.count('release'):
            self.build_type = 'release'

            ## get rid of the conflicting option
            while build_options.count('debug'):
                build_options.remove('debug')

        else:
            self.build_type = 'debug'
            build_options.append('debug')

        self.settings['build_options'] = build_options

        ## skip to module
        skip_to_module = self.getopt_string('-s')
        if skip_to_module:
            self.skip_to_module = skip_to_module

        ## cvs tag
        cvs_tag = self.getopt_string('-r')
        if cvs_tag:
            self.everything_from_cvs_tag = cvs_tag
            self.default_cvs_tag = cvs_tag

        ## cvs tag
        cvs_date = self.getopt_string('-D')
        if cvs_date:
            self.everything_from_cvs_date = cvs_date
            self.default_cvs_date = cvs_date

        ## required arguments: target-list ...
        self.target_id_list = self.argument_list

        ## number of parallel "jobs" for GNU/parallel make
        jobs = self.getopt_string("-j")
        if jobs:
            self.settings["make_jobs"] = jobs


    def run(self):
        ## save the base path & branch we're working out of in the registry

        self.start_time=time.time()

        #if self.settings.get('umake_only_flag'):
        #  bldreg.sync_off()

        bldreg.set_value('build', 'path', os.getcwd())
        bldreg.set_value('build', 'branch', self.build_branch)

        ## set stdout/stderr output
        self.set_output(self.build_output_file)

        ## if there isn't a copy path defined, use the build type string as
        ## the copy path
        if not self.settings.get('copy_path'):
            self.settings['copy_path'] = self.build_type

        ## check for specified targets
        if len(self.target_id_list) < 1:
            e = err.Error()
            e.Set("You need to specify a target to build.")
            raise err.error, e

        ## check for important environment variables and
        ## remove some build system logging files
        self.setup_build()

        ## read the build information file, and create a
        ## dependency list for the build targets
        self.setup_depends()

        ## print out some information about this build
        self.build_info()

        ## cvs checkout source code and get binary distributions for this build
        outmsg.send('getting files')
        log.info( 'Checking out source code and binary distributions.' )
        # TODO: this might be a good place to check for -k flag.
        self.get_files()

        ## update the platform-specific header file in the include/ directory
        self.update_platform()

        ## return now if we only want to checkout the files
        if self.settings.get('checkout_only_flag'): return

        ## compile the target
        if not self.settings.get('no_compile_flag'):
            outmsg.send('compiling')
            self.collate_modules()

        if not self.settings.get('umake_only_flag'):
            ## md5 sign the binaries
            outmsg.send('signing output binaries')
            self.sign()

        ## archive build
        if self.settings.get('archive_build_flag'):
            self.archive_build()

        if not self.settings['build_options'].count('buildfarm_build'):
            if self.settings['build_options'].count('distribute'):
                import distributions
                distributions.DistCheckin().process_checkins(
                    self.settings['copy_path'])

        bldreg.sync_on()

        t=int(time.time() - self.start_time)
        print
        print "Build complete in %02d:%02d, %d of %d modules failed." % (
            t/60,t%60,
            self.errors, self.built)

    def setup_build(self):
        pass
        #if os.path.isfile('md5_sign.txt'):
        #    os.unlink('md5_sign.txt')


    def setup_depends(self):
        log.trace( 'entry' )
        target_tmp=self.target_id_list[:]

        ## the -m <bif-branch> can either be a BIF id or a filename,
        ## we do the tests here...
        branch_list = None

        checkout_mode = 0
        if self.settings.get("no_cvs_checkout_flag"):
            checkout_mode=3

        log.info( 'Setting up dependency data.' )
        log.debug( "Looking for file %s" % self.build_branch )
        if os.path.isfile(self.build_branch):
            log.debug( "Found file %s, will use as build_xml_file" % self.build_branch )
            self.build_xml_file = self.build_branch

        elif (bldreg.get("build","last_cvs_tag",None) ==
              self.everything_from_cvs_tag and
              bldreg.get("build","last_cvs_date",None) ==
              self.everything_from_cvs_date and
              bldreg.get("build","last_branch",None) ==
              self.build_branch):

            log.debug( 'Using build registry data to search branchlist data for %s.' % self.build_branch )
            import branchlist
            branch_list = branchlist.BranchList(
                self.everything_from_cvs_tag,
                self.everything_from_cvs_date,
                checkout_mode or 2)
            self.build_xml_file=bldreg.get("build","last_xml_file",None)
            log.debug( "Setting build_xml_file '%s' from last_xml_file in buildreg" % self.build_xml_file )
        else:
            log.debug( "Doing a full search of branchlist data." )
            import branchlist
            branch_list = branchlist.BranchList(
                self.everything_from_cvs_tag,
                self.everything_from_cvs_date,
                checkout_mode)
            self.build_xml_file = branch_list.file(self.build_branch)
            log.debug( "Setting build_xml_file '%s' from branchlist data" % self.build_xml_file )

        if not self.build_xml_file:
            e = err.Error()
            e.Set("Cannot find bif file for the branch=\"%s\"." % (
                self.build_branch))
            raise err.error, e

        bldreg.set_value("build","last_cvs_tag",self.everything_from_cvs_tag)
        bldreg.set_value("build","last_cvs_date",self.everything_from_cvs_date)
        bldreg.set_value("build","last_branch",self.build_branch)
        bldreg.set_value("build","last_xml_file",self.build_xml_file)

        ## parse the BIF/XML file and get information for this build
        log.info( "Parsing BIF \"%s\"" % (self.build_xml_file) )
        
        import bif
        bif_data = bif.load_bif(self.build_xml_file, branch_list)
        self.build_branch = bif_data.build_id
        self.module_hash = bif_data.module_hash
        log.debug( 'Found modules: %s' % self.module_hash.keys() )
        
        ## Check the profile
        profile=os.environ.get("PROFILE_ID",
                               bldreg.get("build","profile","default"))

        log.debug( "Looking for profile %s in module hash." % profile )
        if bif_data.module_hash.has_key(profile):
            log.debug( 'Found profile %s' % profile )
            profile_module = bif_data.module_hash[profile]
            bldreg.set_value("build","bif_profile",profile)
            if profile_module.type != profile_module.MODULE_PROFILE:
                log.info( "WARNING: Using Profile '%s' from BIF," % profile )
                log.info( "even though it does not have type='profile'." )
            target_tmp.append(profile)
        else:
            log.debug( 'Did not find profile %s' % profile )            
            if not os.path.isfile(profile+".pf"):
                log.debug( 'Profile file %s.pf does not exist.' % profile )
                import branchlist
                log.debug( 'Fetching profiles from branchlist data.' )
                profile_list = branchlist.ProfileList(
                    self.everything_from_cvs_tag,
                    self.everything_from_cvs_date,
                    checkout_mode)
                profile = profile_list.file(profile)
                log.debug( 'Branchlist updates completed.' )
            log.debug( 'Setting PROFILE_ID = %s in environment.' % profile )
            os.environ["PROFILE_ID"] = profile

        bldreg.set_value("build","profile",profile)

        ## if there wasn't a command-line specified cvs branch,
        ## then use the default branch specified in the XML file
        if self.default_cvs_tag == "":
            log.debug( 'Using default cvs branch from bif data: %s' % bif_data.default_cvs_tag )
            self.default_cvs_tag = bif_data.default_cvs_tag

        if self.default_cvs_tag_type == "":
            log.debug( 'Using default cvs tag type from bif data: %s' % bif_data.default_cvs_tag_type )
            self.default_cvs_tag_type = bif_data.default_cvs_tag_type

        ## CVS timestamp
        if self.default_cvs_date == "":
            log.debug( 'Using default cvs date from bif data: %s' % bif_data.default_cvs_date )
            self.default_cvs_date = bif_data.default_cvs_date

        ## CVS ROOT
        if self.default_cvs_root == "":
            log.debug( 'Using default cvs root from bif data: %s' % bif_data.default_cvs_root )
            self.default_cvs_root = bif_data.default_cvs_root
        
        ## check that targets are valid
        log.debug( 'Checking for target validity.' )
        for target_id in self.target_id_list:
            if not self.module_hash.has_key(target_id):
                e = err.Error()
                e.Set("Cannot find the target=\"%s\" you requested in "\
                      "the bif file." % (target_id))
                log.error( "Target '%s' was not found in bif '%s'." % ( target_id , self.build_xml_file) )
                log.trace( 'exit' )
                raise err.error, e
            else:
                log.info( "Using target '%s' from bif '%s'." % ( target_id , self.build_xml_file ) )

        ## create a dependency list for all targets
        log.info( 'Building dependency tree for targets.' )
        import dependlist

        self.depends = dependlist.DependList(bif_data, target_tmp)
        
        
        if os.environ.get('BUILD_ON_PLATFORM','') == 'MacOSX':
            # This section calculates the top-level dependencies for each module
            # That is, it gets the list of dependencies listed in the bif file for the module,
            # replacing any name-only ("fake") modules with those actual dependencies
            # for the name-only module that apply to this platform.
            #
            # When it's done, we will have module_id_build_dict filled with 
            # a mapping of module id to dependency module ids, like
            #   module_id_build_dict['rmacore'] = ['pnmisc' ,'pncont', ...]
            #
            # The entries of the dictionary are used later to determine the list of
            # subtargets for eact target project in the uber

            log.info( "Determining uber subtargets for each module to be built.")

            build_list = self.depends.build_list()
            for module in build_list:
                self.module_id_build_dict[module.id] = []
                dependancy_id_list = module.dependancy_id_list
                while 1:
                    # for each id, get the module; if it's a real module we're building,
                    # then we want its id; if it is a name only (fake) module, we want to
                    # put its dependencies in the dependancy_id_list for the next iteration

                    real_modules_ids_list = []
                    for x in build_list:
                        if x.id in dependancy_id_list:
                            real_modules_ids_list.append(x.id)

                    fake_modules_ids_list = []
                    for x in dependancy_id_list:
                        if x not in real_modules_ids_list:
                            fake_modules_ids_list.append(x)

                    # add all the real modules to our dependency list in the dict unless they are already in it
                    new_unique_module_ids_list = []
                    for x in real_modules_ids_list:
                        if not x in self.module_id_build_dict[module.id]:
                            new_unique_module_ids_list.append(x)

                    self.module_id_build_dict[module.id] = self.module_id_build_dict[module.id] + new_unique_module_ids_list

                    #outmsg.send('module: %s' % (module.id))
                    #outmsg.send('  real dependancies: %s' % (string.join(real_modules_ids_list, ',')))
                    #outmsg.send('  fake dependancies: %s' % (string.join(fake_modules_ids_list, ',')))

                    if len(fake_modules_ids_list) < 1:
                        break   # break out of while loop since no fake modules need to be handled

                    else:
                        # get the dependancies for each fake module and make them our new dependancy list for the
                        # next go round
                        #
                        # first, get the modules for the ids in our fake list

                        fake_modules_list = []
                        for x in self.depends.module_list:
                            if (x.id in fake_modules_ids_list) and  (self.depends.chk_platform(x)):
                                fake_modules_list.append(x)

                        # next, put the items in their dependency lists into the fake list
                        dependancy_id_list = []
                        for fakemodule in fake_modules_list:
                            dependancy_id_list = dependancy_id_list + fakemodule.dependancy_id_list

                        # if there are no dependencies left to resolve, we're done
                        if len(dependancy_id_list) < 1:
                            break

                # these simple lines would replace the above and give us the list of all modules
                # which are dependencies of this module
                #module_buildlist = dependlist.DependList(bif_data, [module.id]).build_list()
                #self.module_build_list[module.id] = module_buildlist

                #outmsg.send('module: %s  build list: %s' % (module.id, string.join(self.module_id_build_dict[module.id])))
                                
        log.trace( 'exit' )

    ##
    ## GETTING SOURCE/BINARIES FOR BUILD
    ##

    def get_files(self):
        log.trace( 'entry' )
        t=time.time()
        ## download and unpack distribution/archive files
        dist_module_name_list = self.get_dist_files()

        ## SAFETY CHECK: if the distribution unpacking didn't avoid
        ## stepping on CVS module, then we have a problem
        for module_name in self.depends.checkout_list("name"):
            if module_name in dist_module_name_list:
                e = err.Error()
                e.Set("The directory created by the distribution "\
                      "module=\"%s\" is the same as the directory specified "\
                      "by a CVS module in the bif file." % (module_name))
                raise err.error, e

        self.checkout_files()
        t=int(time.time() - t)
        print "Checkout done in %02d:%02d" % (t/60,t%60)
        log.trace( 'exit' )


    def remove_modules(self):
        log.info('Examining modules for conflicts.')

        remove_module_dir_list = self.depends.list('path')

        # build a list of directories to remove
        # making sure that they are only the ones
        # we need to remove
        conflict_directory_list = []
        for directory in os.listdir(os.curdir):
            if remove_module_dir_list.count(directory):
                conflict_directory_list.append(directory)
                continue

            if directory == self.settings.get('copy_path'):
                conflict_directory_list.append(directory)
                continue

        # return now if there are no conflicts
        if len(conflict_directory_list) == 0:
            return

        # remove the directories without prompting
        # if yes_mode_flag is set
        if self.settings.get('yes_mode_flag'):
            reply = "y"
        else:
            outmsg.send('existing modules:')
            for directory in conflict_directory_list:
                outmsg.send("  %s" % directory)

            while 1:
                reply = string.lower(raw_input('remove modules [y/n]: '))
                if len(reply) > 0 and string.lower(reply[0]) == 'y' \
                       or string.lower(reply[0]) == 'n':
                    reply = reply[0]
                    break

        if reply == 'y':
            for directory in conflict_directory_list:
                outmsg.send('removing %s' % (directory))
                try:
                    shell.rm(directory)
                except:
                    print "Failed to remove module %s, build cannot continue." % (directory)
                    sys.exit(1)
                    


    def get_dist_files(self):
        log.trace( 'entry' )
        if self.settings.get('clobber_mode_flag'):
            self.remove_modules()

        module_list = []

        ## if we're not in update mode or clobber mode, only
        ## get the distribution modules we don't already have
        if self.settings.get('update_mode_flag') or \
           self.settings.get('clobber_mode_flag'):
            log.debug( 'Update/clobber' )
            module_list = self.depends.distribution_list()
        else:
            for module in self.depends.distribution_list():
                log.debug( 'Looking for module: %s' % module.name )
                if not os.path.exists(module.name):
                    log.debug( 'Module not found, adding to list.' )
                    module_list.append(module)
                else:
                    log.debug( 'Module exists, skipping.' )

        log.debug( 'After depends: %s' % module_list )
        
        tmp=[]
        for module in module_list:
            if os.path.splitext(module.name)[1] == ".rna":
                tmp.append(module)

        if len(tmp) == 0:
            log.debug( 'No distribution modules requested.' )
            return []

        log.debug('Downloading distribution modules: %s' % tmp )
        outmsg.send('downloading distribution binaries')
        new_directory_list = self.get_distribution_list(
            tmp,
            self.default_cvs_tag,
            self.default_cvs_date)
        
        log.trace( 'exit' )
        return new_directory_list


    def get_archive(self, module):
        ## we don't know what modules might be lurking
        ## in a single archive so we take a snapshot
        ## of the directory now and when we're done unpacking
        ## distributions, take the difference and return it
        ## as a list
        old_listdir = os.listdir(os.curdir)

        local_path = module.name
        url = "http://horton/archive/%s/%s" % (
            sysinfo.distribution_id, module.name)
        outmsg.send(url)

        ## remove any current archive with the same name
        if os.path.exists(local_path):
            shell.rm(local_path)

        ## download archive
        try:
            import urllib
            urllib.urlretrieve(url, module.name)
        except IOError, ioe:
            e = err.Error()
            e.Set("Could not download archive specified in bif file "
                  "from url=\"%s\" because of IOError=\"%s\"." % (
                url, str(ioe)))
            raise err.error, e

        ## get a directory listing, and form a filter function which
        ## prevents unpacking of modules which already exist
        dir_list = self.depends.checkout_list('name') + \
                   self.depends.distribution_list('name')

        ## add the "build" directory to the list to prevent it from
        ## getting overwritten
        dir_list.append("build")

        ## add the "distribution" directory to the list to prevent it from
        ## getting overwritten
        dir_list.append("distribution")

        ## This could be slow... -Hubbe
        def filter_func(x, dir_list = dir_list):
            for e in range(0, len(dir_list)):
                dir=dir_list[e]
                if x[:len(dir)] == dir:
                    if len(x) == len(dir) or x[len(dir)]=='/':
                        print "archive skipping=\"%s\"" % (x)

                        ## Swap to first for efficiency
                        if e:
                            dir_list[0], dir_list[e] = (dir, dir_list[0])
                        return 0

            return 1

        ## extract and delete the archive
        import archive
        archive.RNA_Extract(local_path, filter_func, 1)

        shell.rm(local_path)

        ## take a snapshot of the new directory list
        ## FIXME: This is not good if distributions actually go into subdirs!
        new_directory_list = []
        new_listdir = os.listdir(os.curdir)

        for directory in new_listdir:
            if not old_listdir.count(directory):
                new_directory_list.append(directory)

        return new_directory_list


    def get_distribution(self, module, cvs_tag, cvs_date, cvs_path):
        ## distributions with a .rna extension are treated specially --
        ## these are usually huge archives of an entire build which are
        ## too big to revision control
        base, ext = os.path.splitext(module.name)
        if ext == ".rna":
            return self.get_archive(module)

        ## FIXME: add a command-line-option for this!
        root = self.default_cvs_root
        if module.cvs_root:
            root = module.cvs_root

        profile = os.environ.get("PROFILE_ID","default")

        import distributions
        df=distributions.DistFinder(module.name, root, cvs_tag, cvs_date, cvs_path)
        return df.get_distribution(sysinfo.distribution_id,
                                   profile,
                                   self.build_type,
                                   search_cvs = not self.settings.get("no_cvs_checkout_flag"))
                                


    def get_distribution_list(self, module_list, default_cvs_tag, default_cvs_date):
        new_directory_list = []

        for module in module_list:

            ## Don't check these out
            if module.checkin_dep_only:
                if "distribute" not in self.settings['build_options'] and \
                   "make_distributions" not in self.settings['build_options']:
                    continue
            
            cvs_tag = default_cvs_tag
            if module.cvs_tag_flag:
                cvs_tag = module.cvs_tag

            cvs_date = default_cvs_date
            if module.cvs_date_flag:
                cvs_date = module.cvs_date

            new_directory_list = new_directory_list +\
                                 self.get_distribution(module, cvs_tag, cvs_date, module.cvs_path)

        return new_directory_list


    def checkout_module(self, modules):
        paths=[]
        dists=[]

        for module in modules:
            if module.type not in ["cvs","distribution"]:
                continue

            if os.path.exists(module.name):
                print "%s exists" % module.name
                if ( not self.settings.get('update_mode_flag') and \
                     not self.settings.get('clobber_mode_flag') ):
                    continue

            if module.type == "distribution":
                if os.path.splitext(module.name)[1] != ".rna":
                    dists.append(module)
            else:
                paths.append(module.name)

        tag=self.default_cvs_tag
        if self.everything_from_cvs_tag:
            tag = self.everything_from_cvs_tag
        elif module.cvs_tag_flag:
            tag = module.cvs_tag

        date=self.default_cvs_date
        if self.everything_from_cvs_date:
            date = self.everything_from_cvs_date
        elif module.cvs_date_flag:
            date = module.cvs_date;

        ## FIXME: add a command-line-option for this!
        root = self.default_cvs_root
        if module.cvs_root:
            root = module.cvs_root

        if paths:
            if module.cvs_path:
                d=apply(os.path.join,[os.curdir]+string.split(module.cvs_path,"/"))
                if os.path.isdir(d):
                    print "Copying %s to %s" % (d, module.name)
                    shell.cp(d, module.name)
                else:
                    self.checkout_files_from_tag([ module.cvs_path ],
                                                 tag,
                                                 root,
                                                 module.name,
                                                 date)

                ## This will probably never happen
                if len(paths) > 1:
                    checkout_modules(self, modules[:-1])

            else:
                print "OK"
                self.checkout_files_from_tag(paths,
                                             tag,
                                             root,
                                             None,
                                             date)
            
        for module in dists:
            self.get_distribution(module, tag, date, module.cvs_path)
        

    def checkout_files(self):
        import chaingang

        lst=self.depends.distribution_list()
        if not self.settings.get("no_cvs_checkout_flag"):
            lst.extend(self.depends.checkout_list())

        chaingang.ProcessModules_grouped(lst,
                                         self.checkout_module,
                                         int(os.environ.get("CHECKOUT_THREADS","2")))

        import cvs
        cvs.MagicFixDir()

        ## verify all modules checked out
        broken = 0
        for module in self.depends.checkout_list():
            if not os.path.isdir(module.path()):
                broken = broken + 1
                if module.error_message:
                    outmsg.error(module.error_message)
                else:
                    outmsg.error('module %s failed to check out' % (module.name))

        if broken:
            outmsg.send('')
            outmsg.error("================================================")
            outmsg.error("Some modules failed to check out, please look in")
            outmsg.error("build.out to find out why.")
            outmsg.error("================================================")
        
        outmsg.send('')


    def checkout_files_from_tag(self,
                                module_name_list,
                                cvs_tag,
                                repository,
                                az  = None,
                                date = None):
        report_cvs_tag = cvs_tag
        if not len(cvs_tag):
            report_cvs_tag = "HEAD"

        if az:
            for module_name in module_name_list:
                outmsg.send("checking out [%s]%s tag=\"%s\" from path=\"%s\"" % (
                    repository, az, report_cvs_tag, module_name))
        else:
            for module_name in module_name_list:
                outmsg.send("checking out [%s]%s tag=\"%s\"" % (
                    repository, module_name, report_cvs_tag))

        import cvs
        cvs.Checkout(cvs_tag, module_name_list, repository, az, date)


    ## updates the platform.h in modules that wish it
    def update_platform(self):
        for module in self.depends.list():
            if module.update_platform_header_flag:
                import header
                header.update_platform_header(module, self.build_branch)

        # Backwards compatible cruft
        if self.module_hash.has_key("include") and \
           os.path.exists(os.path.join(os.curdir, "include","platform.h")):
            import header
            header.update_platform_header(self.module_hash["include"],
                                          self.build_branch)

            

    def collate_output(self, text):
        outmsg.send(text[:-1])


    def collate_error(self, text):
        outmsg.error(text[:-1])


    def collate_halt_or_print_error(self, module, e):
        self.errors = self.errors + 1
        if self.settings.get("halt_priority"):
            priority_list = ["red", "yellow", "green"]
            i1 = priority_list.index(self.settings.get("halt_priority"))
            i2 = priority_list.index(module.halt_priority)
            if i2 <= i1:
                raise err.error, e

        outmsg.send("\n" + e.Text())


    ## Stage 1
    ##  umake / clean / depend
    ##   Depends on umake being done dependencies
    ## Stage 2
    ##   objects
    ##   Depends on stage 1
    ## Stage 3
    ##   link / copy
    ##   Depends on stage 2 and link/copy done in dependencies
    def collate_module(self, module, basedir, stage=0):
        os.chdir(basedir)

        if os.path.exists("killbuild"):
            outmsg.error("Build killed by killfile!")
            sys.exit(1)

        import plugin

        ## change directory to the module directory
        old_dir    = os.getcwd()
        module_dir = os.path.join(old_dir, module.path())

        ## check if the module path exists
        if not os.path.isdir(module_dir):
            outmsg.error('directory %s does not exist' % (module_dir))
            return

        ## set the current-module registry setting
        bldreg.set_value('current-module', 'id', module.id)
        bldreg.set_value('current-module', 'name', module.name)

        ## change to module subdirectory
        outmsg.send('from directory %s' % (old_dir))
        outmsg.send('entering directory %s' % (module.name))
        os.chdir(module_dir)

        if stage in [0,3]:
            self.built = self.built + 1

        try:
            ## load collation class/plugin for module
            collate_class = plugin.load_class(module)
            if collate_class:
                collate = collate_class(module, self.settings)
                collate.set_output_func(self.collate_output)
                collate.set_error_func(self.collate_error)

                try:
                    collate.set_stage(stage)
                except AttributeError:
                    pass

                collate.run()

        ## pass KeyboardInterrupt
        except KeyboardInterrupt:
            raise

        ## if someone calls sys.exit() in a plugin, catch it here
        ## and continue
        except SystemExit:
            pass

        except err.error, e:
            self.collate_halt_or_print_error(module, e)

        ## all other exceptions catch and print traceback
        except:
            e = err.Error()
            e.Set("A exception occurred while running plugin.")
            e.SetTraceback(sys.exc_info())
            self.collate_halt_or_print_error(module, e)

        ## clear the current-module registry section
        bldreg.clear_section('current-module')

        ## change back to original directory
        outmsg.send("leaving directory %s" % (module_dir))
        try:
            os.chdir(old_dir)
        except os.error:
            e = err.Error()
            e.Set("could not return to directory=\"%s\"." % (old_dir))
            raise err.error, e


    # Kind of crufty..
    def write_uber_umakefile(self):
        def mangle(filename):
            filename = string.lower(filename)
            if os.name == "mac" and filename[0]!=':' and filename[:2]!="./":
                filaneme = ':' + filename
            filename = os.path.normpath(filename)
            return filename

        umf=[]

        bl = self.depends.build_list()

        for m in bl:
            um = os.path.join(m.name, "Umakefil")
            lum=mangle(um)
            #print "MANGLE: %s => %s" % (repr(lum) , repr(um))
            mf = bldreg.get_value_default("makefile",lum,None)
            if mf:
                umf.append('project.AddSubModule(%s, %s)' %
                           ( repr(um), repr(mf) ))

        umf.append('ProjectTarget()')

        ## Only write the file if need be
        umakefil_data = string.join(umf,"\n")
        try:
            if open("Umakefil","r").read() == umakefil_data:
                return
        except IOError:
            pass
        
        open("Umakefil","w").write(string.join(umf,"\n"))

    def collate_modules(self):
        log.trace('entry')
        ## Write some stuff to the registry (we need this for *all* modules)

        umake_includefiles=[]
        print "registrating..."
        for mod in self.depends.list():
            id = mod.id
            bldreg.set_value("bifmodule_type",id,mod.type)
            bldreg.set_value("bifmodule_deplist",id,mod.dependancy_id_list)
            bldreg.set_value("bifmodule_source_deplist",id,mod.source_dependancy_id_list)
            bldreg.set_value("bifmodule_checkin_deplist",id,mod.checkin_dependancy_id_list)
            ## Prioritize modules without the no_build attribute
            if not mod.no_build_flag or not bldreg.get("bifmodule_path_to_id",string.lower(mod.name),None):
                bldreg.set_value("bifmodule_path_to_id",string.lower(mod.name),id)
            bldreg.set_value("bifmodule_id_to_path",id,string.lower(mod.name))
            umake_includefiles.extend(mod.umake_includefiles)

        bldreg.set_value("umake","includefiles",umake_includefiles)

        ## form list of modules we have to deal with
        build_list = self.depends.build_list()

        ## check if we need to skip to a module
        if len(self.skip_to_module):
            build_name_list = self.depends.build_list('name')
            try:
                index = build_name_list.index(self.skip_to_module)
            except ValueError:
                e = err.Error()
                e.Set("Skip to module option for module=\"%s\", but "\
                      "that module was not in the bif file." % (
                    self.skip_to_module))
                raise err.error, e

            ## cut off all the modules up to the skip module
            build_list = build_list[index:]

        self.errors=0
        self.built=0


        print "modulating..."

        make_jobs=chaingang.default_concurrancy
        log.debug('Found default_concurrancy = %s in buildapp.py' % make_jobs)
        if self.settings.has_key("make_jobs"):
            log.debug("... but overriding with object's make_jobs setting: %s"
                                    % int(self.settings['make_jobs']))
            make_jobs=int(self.settings["make_jobs"])

        basedir=os.getcwd()
        jobhash1={}
        jobhash3={}
        for module in build_list:
            mdir=os.path.join(os.getcwd(),module.path())

            if ( os.path.isfile(os.path.join(mdir,"Buildfil")) or
                 os.path.isfile(os.path.join(mdir,"buildfil")) or
                 make_jobs <= 1 or
                 module.get_attribute("serialize") ):

                job3=chaingang.ChainGangJob(module.id,
                                           self.collate_module,
                                           (module, basedir, 0))
                job1=job3
                job1.set_weight(5)
            else:
                job1=chaingang.ChainGangJob(module.id+" umake",
                                           self.collate_module,
                                           (module, basedir, 1))
                job1.set_weight(2)
                job2=chaingang.ChainGangJob(module.id+" objects",
                                           self.collate_module,
                                           (module, basedir, 2))
                job2.set_weight(4)

                ## Modules which are both dynamic and static needs more time
                if module.build_static_flag and \
                   not module.build_static_only_flag:
                    job2.set_weight(7)

                job3=chaingang.ChainGangJob(module.id,
                                           self.collate_module,
                                           (module, basedir, 3))

                job1.add_sub_job(job2)
                job2.add_sub_job(job3)

            jobhash1[module.id]=job1
            jobhash3[module.id]=job3

        log.info('Prioritizing module list.')
        todo=[]
        for module in build_list:
            job1=jobhash1[module.id]
            job3=jobhash3[module.id]
            toplevel=1
            done={}
            deps=module.dependancy_id_list
            for dep in deps:
                if done.has_key(dep):
                    continue
                done[dep]=1
                if jobhash1.has_key(dep):
                    jobhash1[dep].add_sub_job(job1)
                    jobhash3[dep].add_sub_job(job3)
                    toplevel=0
                else:
                    deps.extend(self.module_hash[dep].dependancy_id_list)

            if toplevel:
                todo.append(job1)

        chaingang.ChainGang( todo, make_jobs ).run()
        os.chdir(basedir)

        ## Cross platform compatible buzzword compliant uber project generation
        self.write_uber_umakefile()
        import umake
        umake.INIT()
        umake.Umake(self.settings["build_options"][:])
        log.trace('exit')


    def sign(self):
        """Scan the self.settings["copy_path"], and md5 sign all the
        files in it.  Write the signatures to the md5_sign.txt file."""

        import md5sign

        try:
            last_time=os.stat("md5_sign.txt")[stat.ST_MTIME]
        except:
            last_time=0


        time_string = time.ctime(time.time())
        copy_path = self.settings["copy_path"]
        outmsg.send('Build Complete: %s' % (time_string))

        if not os.path.isdir(copy_path):
            outmsg.send("copy path=\"%s\" not found" % (copy_path))

        try:
            file_list = os.listdir(copy_path)
        except os.error:
            e = err.Error()
            e.Set("Cannot list directory=\"%s\"." % (copy_path))
            raise err.error, e

        try:
            fil = open("md5_sign.txt", "a")
        except IOError:
            e = err.Error()
            e.Set("Unable to open md5_sign.txt for writing.")
            raise err.error, e

        file_list.sort()
        for file in file_list:
            path = os.path.join(copy_path, file)
            if os.path.exists(path) and \
                   os.stat(path)[stat.ST_MTIME] >= last_time:
                if os.path.isfile(path):
                    text = md5sign.md5_fingerprint(path)
                else:
                    text = "DIR (%s)" % path

                outmsg.send(text)
                fil.write("%s\n" % (text))
                    
        fil.close()


    def archive_build(self):
        ## name the output package
        archive_filename = self.archive_filename

        ## remove archive file if it exists
        if os.path.isfile(archive_filename):
            outmesg.send('removing old archive %s' % (archive_filename))
            try:
                os.remove(archive_filename)
            except os.error:
                outmsg.send('failed to remove %s' % (archive_filename))

        outmsg.send('archiving build to %s' % (archive_filename))


        ## form a list of contents of this build
        archive_path_list = os.listdir(os.getcwd())

        ## remove build.out from archive
        if archive_path_list.count('build.out'):
            archive_path_list.remove('build.out')
        else:
            outmsg.error('no build.out file found')

        ## remove the copy path from the archive
        if archive_path_list.count(self.settings['copy_path']):
            archive_path_list.remove(self.settings['copy_path'])
        else:
            outmsg.error('no copy path %s file found' % (
                self.settings['copy_path']))


        ## omit the 'OBJS' directory from each module
        real_path_list = []

        for path in archive_path_list:
            if os.path.isdir(path):
                for file in os.listdir(path):
                    if file != 'OBJS':
                        real_path_list.append(os.path.join(path, file))
            else:
                real_path_list.append(path)


        ## display the list of paths being archived and create archive
        for path in real_path_list:
            outmsg.send(path)

        import archive
        archive.Archive(archive_filename, real_path_list)

        ## get the size of the archive for reporting
        filehandle = open(archive_filename, 'r')
        filehandle.seek(0, 2)
        ## convert size to MB
        size = filehandle.tell() / 1048576
        filehandle.close()

        outmsg.send("file=\"%s\" size=\"%dMB\"" % (archive_filename, size))


    def build_info(self):
        outmsg.send('\n' + self.IDENT)
        outmsg.send('time: %s' % (time.ctime(time.time())))
        outmsg.send('outfile: %s' % (self.output_name))
        outmsg.send('branch: %s' % (self.build_branch))
        outmsg.send('platform: %s' % (sysinfo.id))
        outmsg.send('distribution/archive from: %s' % (sysinfo.distribution_id))
        outmsg.send('build Type: %s' % (self.build_type))
        outmsg.send('build options: %s' % (
            string.join(self.settings['build_options'], ', ')))
        outmsg.send('profile: %s' % bldreg.get_value("build","profile"))

        if self.default_cvs_tag:
            outmsg.send('cvs revision: %s' % (self.default_cvs_tag))

        if self.default_cvs_date:
            outmsg.send('cvs timestamp: %s' % (self.default_cvs_date))

        if self.settings.get('copy_path'):
            outmsg.send('copy target: %s' % (self.settings['copy_path']))

        if self.settings.get('archive_build_flag'):
            outmsg.send('archive build mode')

        if self.settings.get('clean_mode_flag'):
            outmsg.send('clean mode')

        if self.settings.get('clobber_mode_flag'):
            outmsg.send('clobber mode')

        if self.settings.get('checkout_only_mode_flag'):
            outmsg.send('checkout mode')

        if self.settings.get('quiet_mode_flag'):
            outmsg.send('quiet mode')

        if self.settings.get('update_mode_flag'):
            outmsg.send('update mode')

        if self.settings.get('verbose_mode_flag'):
            outmsg.send('verbose mode')

        if self.settings.get('yes_mode_flag'):
            outmsg.send('non-interactive mode')

        if self.settings.get('no_make_depend_flag'):
            outmsg.send('no makedepend mode')

        if self.settings.get('no_umake_mode_flag'):
            outmsg.send('no umake mode')

        if self.settings.get('umake_only_flag'):
            outmsg.send('only run umake')

        if self.settings.get('no_compile_flag'):
            outmsg.send('no compile mode')

        if self.settings.get('no_cvs_checkout_flag'):
            outmsg.send('no cvs checkout mode')

        if self.settings.get('no_make_copy_flag'):
            outmsg.send('no make copy mode')

        self.settings['verbose_mode_flag']  = self.getopt_bool('-v')
        self.settings['quiet_mode_flag']    = self.getopt_bool('-q')
        self.settings['clean_mode_flag']    = self.getopt_bool('-c')
        self.settings['clobber_mode_flag']  = self.getopt_bool('-C')
        self.settings['update_mode_flag']   = self.getopt_bool('-u')
        self.settings['yes_mode_flag']      = self.getopt_bool('-y')
        self.settings['no_umake_mode_flag'] = self.getopt_bool('-n')
        self.settings['umake_only_flag']    = self.getopt_bool('-U')
        self.settings['checkout_only_flag'] = self.getopt_bool('-h')
        self.settings['archive_build_flag'] = self.getopt_bool('-a')
        self.settings['no_compile_flag']    = self.getopt_bool('-o')
        self.settings['no_make_depend_flag']= self.getopt_bool('-e')
        self.settings['no_cvs_checkout_flag']= self.getopt_bool('-k')

        outmsg.send('target(s): %s' % (string.join(self.target_id_list)))

        ## This will print the version of the compiler and other such things
        import umake
        umake.INIT()
        umake.Umake( ["__print_version_and_exit__"] )
        
        outmsg.send('')


    ## output handling
    def set_output(self, default = None):
        # So many output handlers all over the place. :/
        log.trace( 'entry' )
        filename = default or self.build_output_file or '-'

        if filename == '-':
            self.output_name = '<stdout>'
        else:
            self.output_name = filename

            ## remove old backup output file
            try:
                os.unlink('%s.bkp' % (self.output_name))
            except os.error:
                pass

            ## move most recent to backup
            try:
                os.rename(self.output_name, '%s.bkp' % (self.output_name))
            except os.error:
                pass

            ## add the file as a output destination
            if hasattr(sys.stdout, "AddOutputFile"):
                log.debug( 'Adding %s as an output file.' % self.output_name )
                sys.stdout.AddOutputFile(self.output_name)

            ## set the file/creator type on the Macintosh so it opens in
            ## CodeWarrior IDE instead of SimpleText
            if sysinfo.platform == "mac":
                fsp = macfs.FSSpec(self.output_name)
                fsp.SetCreatorType('CWIE', 'TEXT')
        
        log.trace( 'exit' )

    ## all outmsg hook callbacks
    def send_cb(self, text):
        if self.settings.get('quiet_mode_flag'):
            return
        sys.stdout.write('%s\n' % text )

    def error_cb(self, text):
        sys.stdout.write('%s\n' % text )

    def debug_cb(self, text):
        sys.stdout.write('%s\n' % text )

    def verbose_cb(self, text):
        if self.settings.get('quiet_mode_flag'):
            return

        if self.settings.get('verbose_mode_flag'):
            sys.stdout.write('%s\n' % text )
        else:
            if hasattr(sys.stdout, "write_blocked"):
                sys.stdout.write_blocked("%s\n" % text)
            elif hasattr(sys.stdout, "Block"):
                sys.stdout.Block()
                sys.stdout.write('%s\n' % text )
                sys.stdout.UnBlock()
