# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: build_exe.py,v 1.2 2006/07/06 19:28:05 jfinnecy Exp $ 
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
"""This is the build system user interface.  The menu system is implemented
here.  A number of sanity checks, plus some work-arounds for common system
problems are here.  After the menu system has gathered input for a build,
it hands that information to the buildapp.BuildApp() class, which does the
real work."""

import os
import sys
import string
import err
import bldreg
import branchlist
import log
log.debug( 'Imported: $Id: build_exe.py,v 1.2 2006/07/06 19:28:05 jfinnecy Exp $' )
import version

def system_checks():
    """Check things on the system that could be hosed.  Check to make
    sure the user isn't running the build system in the build directory."""

    reg=bldreg.find_registry_path() 
    if reg and reg != os.path.join(os.getcwd(), "build.reg"):
        print "You have previously run builds in '%s'," % os.path.dirname(reg)
        print "running build in this directory will mess up that build."
        print "If you really want to run build in this directory, you must"
        print "delete or rename '%s' or create an" % reg
        print "empty file called 'build.reg' in this directory."
        
        sys.exit(1);

    if os.path.basename(os.getcwd()) == 'build':
        e = err.Error()
        e.Set("You are trying to run 'build' in the build dir.")
        raise err.error, e

    ## Create an empy registry if it does not already exist
    if not os.path.exists("build.reg"):
        open("build.reg","a")

    bldreg.clear_section("sdk")
    bldreg.clear_section("extra_dependencies")
    bldreg.clear_section("distribute")


def invoke_buildsystem(arg_list):
    """Import buildapp and run the build system."""

    import buildapp
    app = buildapp.RunBuild(arg_list)


def man_page():
    """Print the man page from doc/buildcmd.txt."""

    temp = os.path.join(os.environ["BUILD_ROOT"], "doc", "buildcmd.txt")
    if os.name == "posix":
        pager = os.environ.get("PAGER","more")
        if not os.system(pager + ' "%s"' % temp):
            return
        
    # print open(temp, "r").read()
    file = open(temp, "r")

    i = 0
    while 1:
        line = file.readline()

        if not line:
            break

        ## FIXME: Double newlines!!!
        print line[:-1]
        i = i + 1
        if i % 20 == 0:
            raw_input('[ENTER] To Continue')


def command_line_args():
    """Parse command line arguments if the build system is invoked with
    command-line arguments to avoid the build menu."""

    ## check if we should print out man page and exit
    help_list = ["--man", "--help", "-?", "/?"]
    if sys.argv[1] in help_list:
        man_page()
        return

    ## save current directory
    old_dir = os.getcwd()

    arg_list = sys.argv[1:]

    ## call the build system
    invoke_buildsystem(arg_list)

    ## return to old dir
    os.chdir(old_dir)


class BasicMenu:
    def prompt(self):
        return "Select %s" % self.toselect

    def invalid(self):
        return "Invalid input, please try again"

    def get_choices_as_array(self):
        return self.choices

    def get_extra_choices_as_array(self):
        return self.invis

    def get_choice_by_num(self, i):
        return self.get_choices_as_array()[i]

    def get_choice_number(self, c):
        return self.get_choices_as_array().index(c)

    def num_choices(self):
        return len(self.get_choices_as_array())

    def print_menu(self):
        print
        for n in range(22):
            if self.pos >= self.num_choices():
                self.pos=0
                break
            
            print '[%d] %s' %(self.pos, self.get_choice_by_num(self.pos))
            self.pos = self.pos + 1


    def validate(self, input):
        input = string.strip(input)
            
        try:
            i = int(string.strip(input))
            input=self.get_choice_by_num(i)
        except IndexError:
            return None
        except ValueError:
            pass

        if input in self.get_choices_as_array():
            return input
        
        if input in self.get_extra_choices_as_array():
            return input

        return None

    def match(self, input):
        ret=[]
        input=string.lower(input)
        for c in self.get_choices_as_array():
            if string.find(string.lower(c),input) != -1:
                ret.append(c)
        return ret
        
    def get(self):
        self.print_menu()

                    
        while 1:
            help=["Q exits"]
            if not self.pos:
                help.append("? for list")
            else:
                help.append("enter for more")


            input = raw_input('%s (%s): ' %
                              (self.prompt(),
                               string.join(help,", ")))

            if input == "":
                if self.pos:
                    self.print_menu()
                continue


            if input == "?":
                self.pos=0;
                self.print_menu()
                continue

            if input == "q" or input=="Q":
                return None

            ret=self.validate(input)
            if ret:
                return ret

            matches=self.match(input)
            if matches:
                for m in matches:
                    print '[%d] %s' % (self.get_choice_number(m), m)
            else:
                print self.invalid()

    def __init__(self, toselect, choices, invis = None):
        self.invis = invis or []
        self.choices=choices
        self.toselect=toselect
        self.pos=0


class MultipleChoiceMenu(BasicMenu):
    
    def validate(self, input):
        ret=[]
        for i in string.split(input):
            i=BasicMenu.validate(self,i)
            if i:
                ret.append(i)
            else:
                return None

        return ret


class ShellToolApp:
    """Interactive menu system ala-BBS for the build system."""

    IDENT = 'Build System Menu\nRibosome v%s' % version.getVersionString()

    def get_profile_list(self):
        if self.profile_list:
            return self.profile_list
        self.profile_list = branchlist.ProfileList(self.cvs_tag, self.cvs_date)
        return self.profile_list

    def __init__(self):
        self.bif_data_cache=None
        
        ## remember the working directory so the build
        ## system doesn't get confused by keyboard interrupts
        self.working_path = os.getcwd()

        self.cvs_tag = bldreg.get_value_default('build','cvs_tag',"")
        self.cvs_date = bldreg.get_value_default('build','cvs_date',"")

        ## default branch to build
        try:
            build_branch = os.environ["BUILD_BRANCH"]
        except KeyError:
            build_branch = "helix"
        self.build_branch = bldreg.get_value_default(
            "build", "branch", build_branch)

        ## get a list of possible build branches
        update=1

        if ( bldreg.get("build","last_cvs_date",None) == self.cvs_date and
             bldreg.get("build","last_cvs_tag",None) == self.cvs_tag and
             bldreg.get("build","last_branch",None) == self.build_branch):
            update=2

        self.branch_list = branchlist.BranchList(self.cvs_tag, self.cvs_date, update)
        self.profile_list = None


        ## Default command line
        self.history = bldreg.get_value_default('build','history', [])

        self.flags = bldreg.get_value_default('build', 'arguments', None)
        self.targets = bldreg.get_value_default('build', 'targets', None)
        self.profile = bldreg.get_value_default('build','profile', None)

        if not self.profile or not self.targets or self.flags == None:
            bif_data = self.GetBIF()
            if not bif_data:
                ## An old branch
                self.build_branch = "RealMedia"
                bif_data = self.GetBIF()
                if not bif_data:
                    branches = self.branch_list.get_list()
                    if not branches:
                        print "No BIF branches found, please configure your .buildrc!"
                        sys.exit(1)
                    self.build_branch=branches[0]
                    bif_data = self.GetBIF()
                    if not bif_data:
                        print "No BIF files found, please configure your .buildrc!"
                        sys.exit(1)
            
            if not self.profile:
                self.profile = bif_data.default_profile
                if not self.profile:
                    self.profile = "default"
                bldreg.set_value('build','profile',self.profile)

            if not self.targets:
                self.targets = string.split(bif_data.default_target)
                if not self.targets:
                    self.targets = [ "splay" ]
                bldreg.set_value('build','targets',self.targets)

            if self.flags == None:
                self.flags = []
                flags = string.split(bif_data.default_options)
                for flag in flags:
                    flag = "-t"+flag
                    if flag not in self.flags:
                        self.flags.append(flag)

                bldreg.set_value('build','arguments',self.flags)


        if not self.profile:
            self.profile="default"
            
        self.menu = [
            (self.BIFMenuOption, self.SetBIFBranch),
            (self.TargetMenuOption, self.SetTarget),
            (self.ProfileMenuOption, self.SetProfile),
            (self.BuildMenuOption, self.Build),
            ("Toggle make depend & makefiles (-e -n)", self.Toggle, "-e","-n"),
            ("Toggle release (-trelease)", self.Toggle, "-trelease"),
            ("Toggle 'make clean'  (-c)", self.Toggle, "-c"),
            ("Toggle clobber (Dangerous!) (-C)", self.Toggle, "-C"),
            ("Toggle halt-on-error (-p green)", self.Toggle, "-p","green"),
            ("Toggle verbose mode (-v)", self.Toggle, "-v"),
            ("Toggle static build (-tnodll)", self.Toggle, "-tnodll"),
            ("Checkout source for selected target now", self.CheckoutSource),
            (self.TagMenuOption, self.SetTag),
            ("Help Page (full help in build/doc/index.html)", self.Help)]


    def mkarglist(self, args):
        r = []
        import re;
        reg = re.compile("^[\\a-zA-Z0-9.~/]*$")
        for arg in args:
            import re
            if reg.match(arg):
                r.append(arg)
            else:
                r.append("'" + arg + "'")

        return string.join(r,' ')

    def print_menu(self):
        # FIXME: This menu is becoming rather long...
        ## Possible solution: list values where they are set
        ## Also, stop using repr() to show lists of arguments
        
        print
        print self.IDENT

        print "-> Current Directory: %s" % (os.getcwd())

        i = 0
        for m in self.menu:
            tmp = m[0]
            if callable(tmp):
                tmp = tmp()
            print "[%d] %s" % (i, tmp)
            i =i + 1

        for h in self.history:
            print "[%d] run history: build %s" % (i, self.mkarglist(h))
            i = i + 1

        print "[Q] Quit"

    def main(self):
        self.done = 0
        while not self.done:
            self.print_menu()

            try:
                input = raw_input("Enter selection or flags: ")

            except EOFError:
                print
                self.Quit()
                continue

            if string.lower(input) in [ "q", "quit" ]:
                self.Quit()
                continue

            if len(input) and input[0]=='-':
                for arg in string.split(input):
                    self.Toggle(string.strip(arg))
                continue

            try:
                i = int(input)
                
            except ValueError:
                print
                print "** Invalid Input **"
                continue


            if i < 0 or i >= len(self.menu) + len(self.history):
                print
                print "** Invalid Input **"
                continue

            if i >= len(self.menu):
                self.flags = self.history [ i - len(self.menu) ]
                i = 3

            ## call
            apply(self.menu[i][1], self.menu[i][2:])


    def build(self, flags):
        ## add the branch to the arg_list
        arg_list = ['-m', self.build_branch]

        ## Add the cvs tag
        if self.cvs_tag:
            arg_list.extend(["-r", self.cvs_tag])

        if self.cvs_date:
            arg_list.extend(["-D", self.cvs_date])
            
        arg_list.extend(flags + self.targets)

        if self.GetBIF().module_hash.has_key(self.profile):
            os.environ["PROFILE_ID"] = self.profile
        else:
            os.environ["PROFILE_ID"] = self.get_profile_list().file(self.profile)

        ## run
        invoke_buildsystem(arg_list)
        os.chdir(self.working_path)

    def TagMenuOption(self):
        tag = ""
        if self.cvs_tag:
            tag=self.cvs_tag
        if self.cvs_date:
            tag=self.cvs_date
        if self.cvs_tag and self.cvs_date:
            tag=self.cvs_tag +"@"+self.cvs_date

        if tag:
            tag=" (currently: %s)" % tag
        
        return "Set Tag/Branch/Timestamp%s" % (tag)

    def SetTag(self):
        print "Current CVS Tag : \"%s\"" % self.cvs_tag
        print "Current CVS Date: \"%s\"" % self.cvs_date
        print ""
        print "You may enter a new tag, branch or date, to enter"
        print "both a branch and a date, put a @ in between."
        print "Example: yourbranch@2002/01/20 05:20:22"
        print "Note that if you are using BIFs and/or profiles"
        print "directly from CVS, then those files will be updated"
        print "using the information you enter here."
        print ""

        tag=raw_input("Enter branch, tag, date or branch@date: ")
        parts = string.split(tag,"@")
        self.cvs_tag=""
        self.cvs_date=""
        if len(parts) > 1:
            self.cvs_tag=parts[0]
            self.cvs_date=parts[1]
        elif (" " in tag) or ("/" in tag):
            self.cvs_date=tag
        else:
            self.cvs_tag=tag

        bldreg.set_value('build','cvs_date',self.cvs_date)
        bldreg.set_value('build','cvs_tag',self.cvs_tag)

        ## Update branchlist and profile_list
        
        import branchlist
        self.branch_list = branchlist.BranchList(self.cvs_tag, self.cvs_date)
        self.profile_list = None
        

    def ProfileMenuOption(self):
        return "Set Profile (%s)" % self.profile

    def SetProfile(self):
        print

        files_scanned=0
        profiles = []

        for p in self.GetModules():
            if p.type == p.MODULE_PROFILE:
                profiles.append(p.id)

        max_bif=len(profiles)
        tmp = self.get_profile_list().get_list()
        tmp.sort()
        profiles.extend(tmp)

        profile=BasicMenu("profile",profiles).get()
        if not profile:
            print "Profile not changed"
        else:
            bldreg.set_value('build','profile',profile)
            self.profile=profile

    def BuildMenuOption(self):
        return "run: build %s" % self.mkarglist(self.flags)

    def Build(self):
        if self.flags in self.history:
            i=self.history.index(self.flags)
            del self.history[i]

        self.history = [ self.flags ] + self.history
        if len(self.history) > 5:
            self.history = self.history[:5]

        bldreg.set_value('build','history',self.history)

        # print "%r" % self.history

        self.build(self.flags)


    def Toggle(self, *flags):
        for flag in flags:
            if flag in self.flags:
                i = self.flags.index(flag)
                del self.flags[i]
            else:
                self.flags.append(flag)
        bldreg.set_value('build','arguments',self.flags)

    def CustomToggle(self):
        print "Descriptions of custom build options are under the Help"
        print "menu option."
        print
        reply = raw_input("[Custom Build Arguments]: ")
        for arg in string.split(reply):
            self.Toggle(string.strip(arg))

    def TargetMenuOption(self):
        return "Set Target(s) (%s)" % self.mkarglist(self.targets)

    def GetBIF(self):
        if self.bif_data_cache:
            if self.bif_data_cache[0] == self.build_branch and \
               self.bif_data_cache[1] == self.cvs_tag and \
               self.bif_data_cache[2] == self.cvs_date:
                return self.bif_data_cache[3]
        
        filename = self.branch_list.file(self.build_branch)
        if not filename:
            return None

        print 'reading %s file' % (filename)
        
        import bif
        self.bif_data_cache = (
            self.build_branch,
            self.cvs_tag,
            self.cvs_date,
            bif.load_bif_data(filename, self.branch_list) )
        
        return self.bif_data_cache[3]
        

    def GetModules(self):
        bif_data = self.GetBIF()
        modules = bif_data.module_hash.values()
        modules.sort(lambda x, y: cmp(x.id, y.id))
        return modules

    def SetTarget(self):
        modules = self.GetModules()

        toplevel=[]
        groups={}
        all=[]
        for m in modules:
            if m.get_attribute("from_shadow_only"):
                continue

            g=m.group or "other"
            if not groups.get(g):
                groups[g]=[m.id]
            else:
                groups[g].append(m.id)

            all.append(m.id)

            if m.get_attribute("primary_target"):
                toplevel.append(m.id)


        ## No point in creating menues with one choice
        if not toplevel and (len(all) < 20 or len(groups) <= 1):
            toplevel=all
        else:
            for g in groups.keys() + ["all"]:
                toplevel.append("List %s targets..." % g)

        groups["all"]=all
        groups[""]=toplevel

            
        group = ""
        while 1:
            selection = MultipleChoiceMenu("target",
                                           groups[group],
                                           all).get()
            if not selection:
                if group:
                    group = ""
                    continue
                
                print "Target not changed"
                return

            if len(selection) == 1:
                w=string.split(selection[0])
                if len(w) > 2 and w[0] == "List" and w[-1]=="targets...":
                    group=string.join(w[1:-1])
                    continue

            self.targets =  selection
            bldreg.set_value('build','targets',self.targets)
            break

    def CheckoutSource(self):
        self.build(self.flags + ["-h"])

    def BIFMenuOption(self):
        return "Set BIF branch (%s)" % (self.build_branch)

    def SetBIFBranch(self):

        branch_list = self.branch_list.get_bif_list()
        branch_list.sort()

        bif=BasicMenu("BIF",branch_list).get()
        if not bif:
            print "BIF Branch not changed"
            return


        bldreg.set_value('build', 'branch', bif)
        self.build_branch = bif

        bif_data = self.GetBIF()

        if bif_data.default_profile:
            bldreg.set_value('build','profile',bif_data.default_profile)
            self.profile=self.get_profile_list().file(bif_data.default_profile)

        if bif_data.default_target:
            self.targets = string.split(bif_data.default_target)
            bldreg.set_value('build','targets',self.targets)

        if bif_data.default_options:
            flags = string.split(bif_data.default_options)
            for flag in flags:
                flag = "-t"+flag
                if flag not in self.flags:
                    self.flags.append(flag)

            bldreg.set_value('build','arguments',self.flags)
        print

    def Help(self):
        man_page()

    def Quit(self):
        self.done = 1


def find_rcfile(name):
    """Execute $UMAKERC, $HOME/.umakerc, %HOMEDRIVE%HOMEPATH/umakerc.py or
       %preferencesfolder%:umakerc"""
    f=os.environ.get(string.upper(name),"")
    if f and os.path.isfile(f):
        return f

    home=os.environ.get("HOME","")
    if home:
        f=os.path.join(home,"." + name)
        if os.path.isfile(f):
            return f

    
    homedrive=os.environ.get("HOMEDRIVE","")
    homepath=os.environ.get("HOMEPATH","")
    if homedrive and homepath:
        f=os.path.join(homedrive + homepath, "."+name)
        if os.path.isfile(f):
            return f

    if sys.platform == "mac":
        import macfs
        import MACFS
        vrefnum, curdir = macfs.FindFolder(
            MACFS.kOnAppropriateDisk,
            MACFS.kPreferencesFolderType,
            0)
        fss = macfs.FSSpec((vrefnum, curdir, name))
        f= fss.as_pathname()
        if os.path.isfile(f):
            return f

    return None

def find_rcfile_location(name):
    """Execute $UMAKERC, $HOME/.umakerc, %HOMEDRIVE%HOMEPATH/umakerc.py or
       %preferencesfolder%:umakerc"""
    f=os.environ.get(string.upper(name),"")
    if f and os.path.isdir(os.path.dirname(f)):
        return f

    home=os.environ.get("HOME","")
    if home and os.path.isdir(home):
        return os.path.join(home,"." + name)
    
    homedrive=os.environ.get("HOMEDRIVE","")
    homepath=os.environ.get("HOMEPATH","")
    if homedrive and homepath:
        f=os.path.join(homedrive + homepath, "."+name)
        if os.path.isdir(os.path.dirname(f)):
            return f

    if sys.platform == "mac":
        import macfs
        import MACFS
        vrefnum, curdir = macfs.FindFolder(
            MACFS.kOnAppropriateDisk,
            MACFS.kPreferencesFolderType,
            0)
        fss = macfs.FSSpec((vrefnum, curdir, name))
        return fss.as_pathname()

    return None


default_buildrc="""
AddCVS("helix",":pserver:anoncvs@localhost:/cvs")
AddBIFPath("common","[helix]common/build/BIF")
"""

#AddBIFPath("client","[helix]client/build/BIF")
#AddBIFPath("producer","[helix]producer/build/BIF")


def install_rcfile():
    """Give the user the option of installing a buildrc file"""

    location = find_rcfile_location("buildrc")

    print "It seems you have not configured your buildrc file yet."
    if location:
        print "If you like, I can install a default buildrc file for you:"
        print "-------------------------------------------------------"
        print default_buildrc
        print "-------------------------------------------------------"
        ans=raw_input("Would you like to install the above in '%s'? " % location)
        if ans and string.lower(ans)[0] == 'y':
            open(location,"w").write(default_buildrc)

            print "Done"
            print

            print "The default bulidrc will enable to you to work against the"
            print "anonymous Helix CVS repository only. If you require named"
            print "cvs access, or access to more repositories and/or BIF file"
            print "directories, please exit the build system now and customize"
            print "your buildrc file. You will also need to set up an ssh"
            print "tunnel to be able to access the helixcommunity cvs repository."
            print "Read https://www.helixcommunity.org/nonav/docs/ddSSHGuide.html"
            print "for more information."
            print
        else:
            print "buildrc file not installed"
            print

    else:
        print
        print "I am unable to find a suitable location for your buildrc."
        print "You may need to set the BUILDRC environment variable to"
        print "specify the location of your buildrc file. Please refer"
        print "to the following file for more documentation:"
        print os.path.join(os.environ["BUILD_ROOT"], "doc", "buildrc.html")
        print
            

    if not find_rcfile("buildrc"):
        print "Running build without a buildrc fill will probably fail."

    ans = raw_input("Would you like to continue? ('no' will exit the program) :")
    if ans and string.lower(ans)[0] == 'n':
        sys.exit(1)
    
def call_buildrc(install = 1):
    file = find_rcfile("buildrc")

    if not file and install:
        install_rcfile()

    if file:
        import cvs
        import branchlist
        import sdk
        import bif
        
        execfile(file, {
            "AddCVS" : cvs.Add,
            "AddMultiCVS" : cvs.AddMulti,
            "AddBIFPath": branchlist.AddBIFPath,
            "AddBIFShadow": bif.AddShadow,
            "AddProfilePath": branchlist.AddProfilePath,
            "RemoveProfilePath": branchlist.RemoveProfilePath,
            "SetSDKPath" : sdk.SetPath,
            "GetSDKPath" : sdk.GetPath,
            })


## called externally to run
def run():
    system_checks()

    ## if there are command-line arguments, then run non-interactive
    if len(sys.argv) > 1:
        call_buildrc(0)
        command_line_args()
    else:
        call_buildrc(1)
        ShellToolApp().main()
