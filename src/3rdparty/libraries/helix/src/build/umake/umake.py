# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake.py,v 1.60 2007/06/13 01:12:50 jfinnecy Exp $ 
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
"""This file is the execution enviornment for Umake.  Within the umake.py
namespace, all the *.upp (post-processed Umakefil/*.pcf files) are
executed.  Try to keep the namespace clean, to discourage the use of utility
functions by over zealous Umakefil writiers (that's developers).  This is
also the namespace where the platform configuration files (*.cf) are
executed."""

import os
import sys
import string
import re
import types
import copy

import err
import sysinfo
import ascript

import umake_lib
import umakepp
import bldreg
import outmsg
import buildtarget

import marshal

import compile
import imp
import chaingang
import version
import log
log.debug( 'Imported: $Id: umake.py,v 1.60 2007/06/13 01:12:50 jfinnecy Exp $' )

fatal = umake_lib.fatal

def stringp(str):
    if type(str) == types.StringType:
        return 1

    try:
        if type(str) == types.UnicodeType:
            return 1
    except AttributeError:
        pass

    return 0

def INIT():
    global NO_PCF_EXECUTION
    global USE_COMMON_OBJ

    ## constant globals
    NO_PCF_EXECUTION = 0
    USE_COMMON_OBJ = 0

INIT()

try:
    BUILD_ROOT = os.environ["BUILD_ROOT"]
    UMAKE_ROOT = os.path.join(BUILD_ROOT, "umake")
    UMAKECF_ROOT = os.path.join(BUILD_ROOT, "umakecf")
    UMAKEPF_ROOT = os.path.join(BUILD_ROOT, "umakepf")

except KeyError:
    umake_lib.fatal("set your BUILD_ROOT environment variable")

## context globals
class UmakeGlobals:
    """Save and restore global variables in this object to make calling
    Umake recursivly easier."""

    def __init__(self):
        self.project = Project()
        self.platform = Platform()
        self.common_defines = ""
        self.common_libraries = ""
        self.common_includes = ""
        self.common_srcs = ""
        self.sub_umakes = ""
        self.sub_platform_name_pcfs = ""
        self.sub_platform_type_pcfs = ""
        self.globals = None
        self.umake_epilogue_callbacks=[]
        self.umake_identify_callbacks=[]

    def SaveGlobals(self):
        global project, platform, common_defines, common_libraries,\
               common_includes, common_srcs, sub_umakes,\
               sub_platform_name_pcfs, sub_platform_type_pcfs, \
               umake_epilogue_callbacks, umake_identify_callbacks

        try:
            self.project = project
            self.platform = platform
            self.common_defines = common_defines
            self.common_libraries = common_libraries
            self.common_includes = common_includes
            self.common_srcs = common_srcs
            self.sub_umakes = sub_umakes
            self.sub_platform_name_pcfs = sub_platform_name_pcfs
            self.sub_platform_type_pcfs = sub_platform_type_pcfs
            self.umake_epilogue_callbacks = umake_epilogue_callbacks
            self.umake_identify_callbacks = umake_identify_callbacks
        except NameError:
            pass

        self.globals = globals().copy()

    def SetGlobals(self):

        if self.globals:
            g = globals()
            g.clear()
            g.update(self.globals)
        
        global project, platform, common_defines, common_libraries,\
               common_includes, common_srcs, sub_umakes,\
               sub_platform_name_pcfs, sub_platform_type_pcfs, \
               umake_epilogue_callbacks, umake_identify_callbacks
        
        project = self.project
        platform = self.platform
        common_defines = self.common_defines
        common_libraries = self.common_libraries
        common_includes = self.common_includes
        common_srcs = self.common_srcs
        sub_umakes = self.sub_umakes
        sub_platform_name_pcfs = self.sub_platform_name_pcfs
        sub_platform_type_pcfs = self.sub_platform_type_pcfs
        umake_epilogue_callbacks = self.umake_epilogue_callbacks
        umake_identify_callbacks = self.umake_identify_callbacks


def diff_mappings(a, b):
    m = {}
    for k in a.keys() + b.keys():
        m[k] = 1

    keys = m.keys()
    keys.sort()
    for k in keys:
        if a.has_key(k):
            if b.has_key(k):
                if a[k] != b[k]:
                    print "Value differance for key %s" % repr(k)
                    print "%s != %s" % (repr(a[k]), repr(b[k]))
            else:
                print "KEY %s missing in 'b'" % repr(k)
        else:
            print "KEY %s missing in 'a'" % repr(k)

def safe_execfile(filename):
    """A execfile() function with extra checks meant to exec pre-processed
    Umakefils (*.upp)."""

    umake_lib.debug("safe_execfile=\"%s\"" % (filename))
    if not os.path.isfile(filename):
        return

    globs = globals().copy()
    approved = {"project":1,
                "platform":1,
                "common_defines":1,
                "common_libraries":1,
                "common_includes":1,
                "common_srcs":1,
                "sub_umakes":1,
                "sub_platform_name_pcfs":1,
                "sub_platform_type_pcfs":1 }

    #kglobals.sort()
    #klocals.sort()
    # print "PROJ: %s" % repr(project)
    #print "GLOBALS: %s" % repr(kglobals)
    #print "LOCALS : %s" % repr(klocals)


    try:
        execfile(filename, globals())

    except "NONSTANDARD":
        raise

    except err.error, e:
        e.SetTraceback(sys.exc_info())
        raise err.error, e

    except:
        e = err.Error()
        path = os.path.join(os.getcwd(), filename)
        e.Set("Exception while executing file=\"%s\"." % (path))
        e.SetTraceback(sys.exc_info())
        raise err.error, e

    for key in globals().keys():
        if not globs.has_key(key):
            #umake_lib.namespace_modified("modified globals=\"%s\"" % (key))
            del globals()[key]
        elif not approved.has_key(key) and globals()[key] != globs[key]:
            umake_lib.namespace_modified("modified globals=\"%s\"" % (key))



class FakeSYS:
    platform=sys.platform

class FakeOSPath:
    def __getattr__(self, name):
        if name[:2] == "is":
            raise "SIDE EFFECT"
        if name[:3] == "get":
            raise "SIDE EFFECT"
        if name in ["exists","walk"]:
            raise "SIDE EFFECT"

        return getattr(os.path, name)

class FakeOS:
    def __init__(self):
        self.path=FakeOSPath()

def check_standard_makefile(filename):
    def fakefunction(*args):
        return

    def fakefunction_str(*args):
        return "-"

    platform=Platform()
    platform.cc=Compiler()
    platform.rc=Compiler()
    platform.cxx=Compiler()

    bi={}
    if type(__builtins__) == types.DictType:
        for x in __builtins__.keys():
            bi[x]=__builtins__[x]
    else:
        for x in dir(__builtins__):
            bi[x]=getattr(__builtins__,x)

    ## Remove dangerous functions
    del bi["open"]

    def IMP(name, globals, locals, fromlist):
        if name == "sys":
            return FakeSYS()
        if name == "os":
            return FakeOS()

        return None

    bi["__import__"]= IMP

    g={
       "__builtins__": bi,
       "sys":FakeSYS(),
       "os":FakeOS(),
       "cc":platform.cc,
       "cxx":platform.cxx,
       "rc":platform.rc,
       "sysinfo":sysinfo,
       "string":string,
       "UmakefileVersion":fakefunction,
       "project":Project(),
       "platform":platform,
       "GetSDKPath":fakefunction_str,
       "ProgramTarget":fakefunction,
       "ProgramWithResourceTarget":fakefunction,
       "DLLWithResourceTarget":fakefunction,
       "ProgramTarget":fakefunction,
       "LibraryTarget":fakefunction,
       "DLLTarget":fakefunction,
       "AllTarget":fakefunction,
       "CopyTarget":fakefunction,
       "EmptyTarget":fakefunction,
       "CreateStaticPluginTable":fakefunction,
       "ProjectTarget":fakefunction,
       "UpdateReleaseNotes":fakefunction,
       "MultiTargetMake":fakefunction,
       "UseCommonObjects":fakefunction,
       "CommonDefines":fakefunction,
       "CommonLibraries":fakefunction,
       "CommonIncludes":fakefunction,
       "CommonSources":fakefunction,

       "common_libraries":"",
       "common_defines":"",
       "common_includes":"",
       "common_srcs":"",
    }
    try:
        execfile(filename, g,g)
        return 1
    except:
	print "Umakefile uses generic python code."
        if g.get("generic_umakefile_override"):
            print "Overridden by umakefile creator"
            return 1

        ## Magic build resurrection code
        c=chaingang.current_chaingang.Get()
        r=0
        if c:
           id=bldreg.get("current-module","id","")
           deps=bldreg.get("bifmodule_deplist",id,[])
           done={}
           for m in deps:
               if done.get(m):
                   continue
               done[m]=1

               mtype = bldreg.get("bifmodule_type", m, "")
               if mtype  == "name_only":
                   deps.extend(bldreg.get("bifmodule_deplist",m,[]))
                   continue
               
               if c.reschedule_by_id(m):
                   r=r+1

        if r:
            print "Generic python code detected, rescheduling umake call."
            raise "NONSTANDARD"
        return 1
    

def safe_execfile_namespace(file_name):
    """This execfiles() a python source file by importing it into memory,
    striping out all the troublesome bad newline charactors,
    and then execing that string without a global context.
    the symbols from the file are then returned in a dictionary."""

    file_name=umake_lib.ci_find_file(file_name)

    if not os.path.isfile(file_name):
        return None

    code = open(file_name, "r").read()
    code = string.replace(code, "\r", "\n")
    code = string.rstrip(code) + "\n"

    symbols = {}
    exec(code, {}, symbols)
    return symbols

def my_exec_file(path, filename = None, globs = None):
    """Execs() a file"""

    if not globs:
        globs=globals()

    if filename == None:
        filename = os.path.basename(path)

    path=umake_lib.ci_find_file(path)
        
    if not path or not os.path.exists(path):
        umake_lib.fatal("configuration file=\"%s\" not found" % (filename))

    compfile = path + "c"

    if sysinfo.host_type == 'mac' and len(os.path.basename(compfile)) > 31:
        execfile(path, globs)
        return

    cf_mtime = umake_lib.mtime(path)

    if cf_mtime > project.cf_last_modified:
        project.cf_last_modified = cf_mtime

    magic=""
    try:
        magic=open(compfile,"rb").read(4)
    except IOError:
        pass

    if magic != imp.get_magic() or umake_lib.mtime(compfile) < cf_mtime:
        import py_compile
        umake_lib.debug("compiling file=\"%s\"" % (path))
        py_compile.compile(path, compfile, filename)

    try:
        fc=open(compfile,"rb")
        fc.seek(8)
        code=marshal.load(fc)
        fc.close()
    except:
        try:
            fc.close()
        except:
            pass
        print "Bad cfc file, please try again"
        os.unlink(compfile)
        raise

    exec code in globs
    

def exec_config_file(filename):
    """Execs() a umake .cf configuration file for a platform.  It magicly
    appends the correct path based on the BUILD_ROOT."""

    umake_lib.debug("exec_config_file=\"%s\"" % (filename))
    my_exec_file(os.path.join(UMAKECF_ROOT, filename), filename)

def exec_profile_file(filename):
    """Execs() a umake .pf profile file. It magicly
    appends the correct path based on the BUILD_ROOT."""
    global UMAKEPF_ROOT

    umake_lib.debug("exec_profile_file=\"%s\"" % (filename))
    path=os.path.join(UMAKEPF_ROOT, filename)

    old_umakepf_root=UMAKEPF_ROOT
    UMAKEPF_ROOT=os.path.dirname(path)
    try:
        my_exec_file(path, filename)
    finally:
        UMAKEPF_ROOT=old_umakepf_root

def find_umakerc():
    """Execute $UMAKERC, $HOME/.umakerc, %HOMEDRIVE%HOMEPATH/umakerc.py or
       %preferencesfolder%:umakerc"""
    f=os.environ.get("UMAKERC","")
    if f and os.path.isfile(f):
        return f

    home=os.environ.get("HOME","")
    if home:
        f=os.path.join(home, ".umakerc")
        if os.path.isfile(f):
            return f

    
    homedrive=os.environ.get("HOMEDRIVE","")
    homepath=os.environ.get("HOMEPATH","")
    if homedrive and homepath:
        f=os.path.join(homedrive + homepath,"umakerc.py")
        if os.path.isfile(f):
            return f

    if sys.platform == "mac":
        import macfs
        import MACFS
        vrefnum, curdir = macfs.FindFolder(
            MACFS.kOnAppropriateDisk,
            MACFS.kPreferencesFolderType,
            0)
        fss = macfs.FSSpec((vrefnum, curdir, 'umakerc'))
        f= fss.as_pathname()
        if os.path.isfile(f):
            return f

    return None
        

def exec_umakerc():
    """Execute $UMAKERC, $HOME/.umakerc, %HOMEDRIVE%HOMEPATH/umakerc.py or
       %preferencesfolder%:umakerc"""

    file = find_umakerc()

    if file:
        project.cf_last_modified = max(project.cf_last_modified, umake_lib.mtime(file))
        execfile(file, globals())


def join_string(*args):
    """Joins a varible number of strings into one string.  This function
    is used in many *.cf files."""
    
    out_string = ''

    for i in range(len(args)):
        if args[i] == None or not len(args[i]):
            continue

        out_string = out_string + args[i]

    return out_string


def form_string(*args):
    """Joins a varible number of strings into one string with space
    delimeters.  This function is used in many *.cf files."""
    
    out_string = ""

    for i in range(len(args)):
        if args[i] == None or not len(args[i]):
            continue

        out_string = out_string + args[i]
        if i < len(args) - 1:
            out_string = out_string + " "

    return out_string

def rmlist(list1, list2):
    """Removes items in list2 from list1, case insensitive."""
    tmp={}
    for item in list2:
        tmp[string.lower(item)]=1

    lst = []
    for item in list1:
        if not tmp.get(string.lower(item)):
            lst.append(item)

    return lst

## Add a callback which will be called
## after all .pf, .cf umakefiles and umakerc files
## have been parsed. This callback can thus modify
## the behaviour of the project in any way it sees fit.
def AddUmakeCallback(call, *args):
    global umake_epilogue_callbacks
    umake_epilogue_callbacks.append( (call, args) )


def AddIdentifyCallback(call, *args):
    """Add a callback which will be called if the -v option is given
    to umake"""
    global umake_identify_callbacks
    umake_identify_callbacks.append( (call, args) )


class BuildRule:
    def __init__(self, source, target, command):
        self.source_suffix = source
        self.target_suffix = target
        self.command = command
        self.__wantsProcess = 1
        self.output_dir   = None
        
    def disableProcessing( self ):
        """disableProcessing()
        
        Turns off processing for files belonging to this build rule.
        """
        self.__wantsProcess = 0
        
    def wantsProcessing( self ):
        """wantsProcessing() --> boolean
        
        Does this build rule want to be processed? When would this ever be no?
        """
        return self.__wantsProcess
                
        
class Command:
    """Base class of most platform build commands.  It is common for
    subclasses of command to define a method called execute that returns
    a string that calls the command on the given arguments."""

    def __init__(self):
        self.args       = {"default" : ""}
        self.cmd        = ""
        self.make_var   = ""
        self.make_flags = ""

    def form_args(self):
        """Builds a command line based on the build choices."""
        
        arg_str = ""
        for choice in project.build_choices:
            if self.args.has_key(choice):
                arg_str = "%s%s " % (arg_str, self.args[choice])
        return arg_str

    def setup_command_var(self):
        """Returns a string that represents this command as a variable."""
        
        if len(self.make_var):
            return "%s=%s" % (self.make_var, self.cmd)
        return ""

    def setup_flags_var(self):
        """Returns a string that represents the arguments for this command
        as a make variable."""
        
        if len(self.make_flags):
            return "%s=%s" % (self.make_flags, self.form_args())
        return ""

    def execute(self):
        return "%s %s" % (platform.form_var(self.make_var), self.form_args())

    def copy(self, new_make_var):
        ret=copy.deepcopy(self)
        ret.make_var = new_make_var
        ret.make_flags = new_make_var+"FLAGS"
        return ret;
        

class Compiler(Command):
    """A compiler is any command that takes one source file and creates a
    target file based on the source file."""

    def __init__(self):
        Command.__init__(self)
        self.target_arg  = ""
        self.source_arg  = ""
        self.define_arg  = ""
        self.include_arg = ""
        self.prefix_include_arg = ""
        self.includes = None  # defaults to project.includes
        self.defines = None   # defaults to project.defines

    def execute(self, target_file, src_file):
        extra_args = ""

        ## include special compiler arguments for this target type
        if self.args.has_key( project.getTargetType() ):
            extra_args = self.args[ project.getTargetType() ]
                                 
        return form_string(
            platform.form_var(self.make_var),
            platform.form_var(self.make_flags),
            extra_args,
            self.target_arg,
            target_file,
            self.source_arg,
            src_file)

    def setup_flags_var(self):
        prefix=self.make_var
        if prefix in ["CC","CXX"]:
            prefix=""
            
        def_str = form_string(
            platform.form_var(prefix+"INCLUDES"), platform.form_var(prefix+"DEFINES"))
        return form_string(self.make_flags, '=', self.form_args(), def_str)

    def copy(self, new_make_var):
        ret=Command.copy(self, new_make_var)
        return ret

    def SetIncludes(self, *args):
        self.includes=list(args)

    def AddIncludes(self, *args):
        if not self.includes:
            self.includes=project.includes[:]
        self.includes.extend(args)

    def RemoveIncludes(self, *args):
        if not self.includes:
            return
        self.includes = rmlist(self.includes, args)

    def SetDefines(self, *args):
        self.defines=list(args)

    def AddDefines(self, *args):
        if not self.defines:
            self.defines=project.defines[:]
        self.defines.extend(args)

    def RemoveDefines(self, *args):
        if not self.defines:
            return
        self.defines = rmlist(self.defines, args)


class Linker(Command):
    """A linker is any command that takes many source files and creates
    a target file based on those source files."""

    def __init__(self):
        Command.__init__(self)
        self.make_var   = "LD"
        self.make_flags = "LDFLAGS"
        self.target_arg = ""

    def execute(self, target_file, objects, static_libs, dynamic_libs):
        return form_string(
            self.cmd,
            self.form_args() + self.target_arg + target_file,
            objects,
            static_libs,
            dynamic_libs)


class Linker2:
    """Replaces the Linker class with a better API."""

    def __init__(self):
        self.linker2 = 1

    def LinkLIB(self, target_path, objects):
        """Returns a list of strings.  Each string is a command it be executed
        in order, to create a static library."""
        
        umake_lib.warning("Linker2: LinkLIB unimplemented")
        return []

    def CleanLIB(self, target_path):
        """Returns a list of strings.  Each string is a command it be executed
        in order, removing the target, and all the intermediate components
        of a static library."""
        
        umake_lib.warning("Linker2: CleanLIB unimplemented")
        return []

    def LinkDLL(self, target_path, objects, static_libs, dynamic_libs):
        """Same as LinkLIB, but creates a DLL (dynamic link library)."""
        
        umake_lib.warning("Linker2: LinkDLL unimplemented")
        return []

    def CleanDLL(self, target_path):
        """Same as CleanLIB, but for a DLL."""
        
        umake_lib.warning("Linker2: CleanDLL unimplemented")
        return []

    def LinkEXE(self, target_path, objects, static_libs, dynamic_libs):
        """Same as LinkLIB, but creates a executeable.""" 
        
        umake_lib.warning("Linker2: LinkEXE unimplemented")
        return []

    def CleanEXE(self, target_path):
        """Same as CleanLIB, but for a executeable."""
        
        umake_lib.warning("Linker2: CleanEXE unimplemented")
        return []


class MakeDepend(Command):
    """This is its own command because it takes a list of sources and an
    output directory and messes with the Makefile in the current directory
    to creately create dependencies based on header includes."""

    def __init__(self):
        Command.__init__(self)
        self.make_var       = "MAKE_DEP"
        self.make_flags     = "MAKE_DEP_FLAGS"
        self.output_dir_arg = ""

    def execute(self, sources, output_dir):
        if not (output_dir):
            out_str = ''
        else:
            out_str = form_string(
                self.output_dir_arg,
                output_dir + platform.path_sep)

        return form_string(
            platform.form_var(self.make_var),
            platform.form_var(self.make_flags),
            out_str,
            sources)

    def setup_flags_var(self):
        return form_string(
            self.make_flags,
            '=',
            self.form_args(),
            platform.form_var("INCLUDES"),
            platform.form_var("DEFINES"))


class Utility(Command):
    def execute(self, target):
        return "%s %s %s" % (
            platform.form_var(self.make_var), self.form_args(), target)


class Versioning:
    """This class is used almost exclueivly to read the *.ver files for version
    numbers, then use those version numbers to create a name for output
    DLLs."""

    version_match = re.compile(
        '^#define\s+TARVER_STRING_VERSION\s+\"(\d+)\.(\d+)\.(\d+)\.(\d+)\".*$')
    
    def __init__(self):
        self.version = ''

    def create_dll_name(self, target, path = ''):
        umake_lib.fatal("Versioning.create_dll_name needs to be overriden")

    def get_version(self, target, path):
        vMajor = vMinor = vRelease = vBuild = 0

        if len(path):
            # this means the version file is not in this directory
            filename = '%s.ver' % (os.path.join(os.pardir, path, target))
        else:
            # this means the version file is in this directory
            filename = '%s.ver' % (os.path.join(os.curdir, target))

        try:
            filehandle = open(filename, 'r')
        except IOError, e:
            umake_lib.fatal(
                'could not find %s for version information' % (filename))

        for line in filehandle.readlines():
            match_obj = self.version_match.match(line)
            if match_obj:
                vMajor = match_obj.group(1)
                vMinor = match_obj.group(2)
                vRelease = match_obj.group(3)
                vBuild = match_obj.group(4)
                break

        filehandle.close()
        
        self.version = (vMajor, vMinor, vRelease, vBuild)
        return self.version


    ## the path assumptions ARE as ugly as they look...
    ## we have to search for both Names.py and names.py because
    ## you can never tell how the case-insensitive OS's will check
    ## things in...    
    def get_name(self, old_name, path = ''):
        if len(path):
            path = os.path.join(os.pardir, path)
            path1 = os.path.join(path, 'Names.py')
            path2 = os.path.join(path, 'names.py')
        else:
            path1 = 'Names.py'
            path2 = 'names.py'

        if os.path.isfile(path1):
            ns = safe_execfile_namespace(path1)
        elif os.path.isfile(path2):
            ns = safe_execfile_namespace(path2)
        else:
            ns = None

        if ns:
            try:
                return ns['names'][platform.name]
            except KeyError:
                pass

            try:
                return ns['names'][platform.reference]
            except KeyError:
                pass

            ## if the above methods fail, go backwards through
            ## the platform.pcf_prefix_list 
            list = platform.pcf_prefix_list[:]
            list.reverse()
            for pcf_prefix in list:
                try:
                    return ns['names'][pcf_prefix]
                except KeyError:
                    pass

        return ""


class Platform:
    """This is where OS specific information is stored. This information
    should apply to every single project ever built. For example: on unix
    the delete file command is always 'rm' so it belongs here.
    Due to my inexperience with Python a couple things in this class are
    already defined in the os module. I might clean that up later."""

    def __init__(self):
        ## legacy
        self.name = self.reference = sys.platform
        self.config_dir = UMAKE_ROOT

        ## list the prefixes for the PCF files which should be
        ## executed
        self.pcf_prefix_list    = []
  
        ## set to "1" to enable static builds
        self.can_build_static = 0
        
        ## defined in the .cf file
        self.type               = ''
        self.path_sep           = ''
        self.line_cont          = ''
        self.var_begin          = ''
        self.var_end            = ''
        self.make_prog_arg      = ''
        self.make_depend_char   = ''
        self.sys_lib_arg        = ''
        self.inc_subdir         = ''
        self.mkdir              = ''
        self.x_root_dir         = ''
        self.object_suffix      = ''

        ## suffexes for target types
        self.exe_suffix          = ''
        self.library_suffix      = ''
        self.library_prefix      = ''
        self.dll_suffix          = ''
        self.resource_dll_suffix = ''

        self.rm                 = Command()
        self.rmdir              = Command()
        self.make               = Command()
        self.make_dep           = MakeDepend()
        self.make_lib           = Linker()
        self.link               = Linker()
        self.make_toc           = Utility()
        self.copy               = Command()

        self.build_rules        = {}
        self.defines            = []
        self.suffix_list        = []
        self.specs              = []
        self.system_paths       = []

        self.command_list       = [self.rm,
                                   self.rmdir,
                                   self.make_dep,
                                   self.make_lib,
                                   self.make_toc,
                                   self.copy,
                                   self.make]

    def form_var(self, var_name):
        return '%s%s%s' % (self.var_begin, var_name, self.var_end)


            

class Project:
    """This is where project specific information is stored. This information
    should only apply to the current project being build. For example: the
    list of source files (sources) is always specific to a given project.
    Build choices is a list of strings that enables the user of umake to
    parameterize a given umake file. For example: if you sometimes want to
    build with purify, you could call umake with -t "purify" and then put in
    a ton of if statements to setup compilers and such when
    build_choices.count('purify') > 0."""        

    # DEPR: 3.0
    if version.getMajorVersion() < 3:
        # If someone does a query on project.target_type, it will instead
        # hit the target_typeWrapper object with a getattr __str__,
        # causing execution of this wrapper's method.
        # This maintains backwards compatibility until deprecation time.
        # No one should be doing a set on target_type, as that code has been
        # getting filtered out by the umake pre-processor for some time...
        class target_typeWrapper:
            def deprecate( self ):
                msg = 'DEPRECATED: using project.target_type is deprecated and'
                msg += ' scheduled for removal in version 3.0 - use'
                msg += ' getTargetType() instead.'
                log.warn( msg )
                
            def __str__( self ):
                self.deprecate()
                return project.getTargetType()
                
            def __eq__( self , value ):
                self.deprecate()
                return project.getTargetType() == value
                
    def __init__(self):
        self.pre_target_buff = []
        self.post_target_buff = []
        self.current_buffer = 'pre_target_buff'
        
        self.__target_name = ""
        self.__output_name = ""
        self.__drm_sign = 0
        self.__signType    = ''
        self.__signOptions = {}
        
        self.target_name = ""
        self.dll_type = "plugin"
        
        self.__wantsLinking = 1
        
        self.buildTarget = None        
        
        # DEPR: 3.0
        if version.getMajorVersion() < 3:
            # Point target_type to the deprecation wrapper.
            self.target_type = self.target_typeWrapper()
            # log.info( 'Trying to get target_type through wrapper.' )
            # log.info( 'Got %s' % self.target_type )
            
        ## Last modified cf file
        self.cf_last_modified = 0

        ## UPP file mtime (zero if upp file was regenerated)
        self.upp_last_modified = 0


        ## When creating a dll, this is the name of the lib file that goes with it
        self.opt_target_name=None

        ## output_dir is the base subdirectory for built targets
        ## within a module
        self.output_dir = ""

        ## object_dir is the directory to put object files in within
        ## a module
        self.object_dir = ""

        ## src_root_path is the path of the directories where all
        ## the modules are checked out to
        self.src_root_path = "ERROR ERROR"
        self.module_depth = 1

        ## target_dir is the path targets are copied to in "make copy"
        self.target_dir = ""

        ## project_dir is the path to the directory the project was
        ## launched, as far as I can tell --JMP
        self.project_dir = ""
        self.module_dir = ""
        
        ## copy_target_list is a list of files to be placed in the "make copy"
        ## section of the Makefile to be copied to the target_dir
        self.copy_target_list = []

        ##
        ## Extra copy commands
        self.debug_copies = []
        
        self.makefile_path = ""
        self.postbuildmake = ""

        self.sources = []
        self.objects = []
        self.objsrcs = []
        self.includes = []
        self.defines = []
        self.system_paths = []
        self.exported_func = []
        self.exported_function_protos = {}

        ## static libraries
        self.libraries = []
        self.libraries2 = []   
        self.module_libs = []
        self.local_libs = []

        ## dynamic libraries
        self.dynamic_libraries = []
        self.sys_libraries = []
        self.sys_frameworks = []

        # Library paths
        self.libpaths = []
        
        ## dll versioning on/off
        self.versioning_off        = 0
        
        self.build_choices         = ["default"]
        self.makefile_name         = "Makefile"
        self.umakefile_name        = "Umakefil"

        ## with resource
        self.with_resource_flag    = 0
        self.resource_target       = ''
        self.resourcefile          = ''
        self.resourceincludes      = ''

        ## Extra targets
        self.xtargets              = []
        self.alldepends            = ''
        self.submakes              = []

        ## the preferences hash is a really wacky/bad way of
        ## setting panel preferences in CW
        self.preferences = {}

        ## weak_link_list are a set of system libraries which
        ## need weak symbol resolution
        self.weak_link_list = []

        ## prefix_file_include_list should only be used from the .cf
        ## files to add a list of includes to the prefix file
        self.prefix_file_include_list = []

        ## FIXME: description
        self.distribute_location=None
        self.distribute_files={}

        ## These are features which are off
        ## by default. You can turn them on by calling
        ## project.SetBuildVersion
        ## (or project.SetBuild2002Version)
        self.features = {
            "submodules" : 0,
            "versioning_off" : 0,
            "static_implies_nodll" : 0,
            "makefile_quotes" : 0,
            }

        ## Extra makefile dependencies
        self.file_dependencies = {}

    def disableLinking( self ):
        """disableLinking()
        
        Disables linking of this project's target. Typically means that we want
        to run a compiler on some source file(s), but then do nothing else with
        the output from that process.
        """
        self.__wantsLinking = 0
        
    def wantsLinking( self ):
        """wantsLinking() --> boolean
        
        Does this project's target want to participate in the linking step?
        """
        return self.__wantsLinking
        
    def getCleanItems( self ):
        lines = []
        if hasattr(platform.link, "linker2"):
            if self.buildTarget:
                fn = self.buildTarget.getCleanFunction()
            else:
                fn = lambda p: []
            lines = fn( self.getOutputPath() )
        else:
            if self.getTargetType():
                if string.strip( self.getOutputPath() ):
                    lines = [ self.getOutputPath() ]
        
        lines.extend( self.objects )
        
        return lines
        
    def getAllLine( self , targets ):
        if self.buildTarget:            
            return self.buildTarget.getAllLine( self.object_dir , targets )
        else:
            return 'all: %s %s' % ( self.object_dir , targets)

    def isSignable( self ):
        """isSignable() --> boolean
        
        Returns true if target is a signable type.
        """
        return self.buildTarget.isSignable()
            
    def getTargetType( self ):
        """getTargetType() --> string
        
        Returns the target type, if any. Otherwise, returns empty string.
        """
        if self.buildTarget:
            return self.buildTarget.getTargetType()
        else:
            return ''            
            
    def getOutputPath( self ):
        """getOutputPath() --> string
        
        Returns the path to the target. Will be processed by the build target,
        if any.
        """
        path = ''
        if self.buildTarget:
            path = self.buildTarget.getOutputPath( self.output_dir , 
                                                   self.getOutputName())
        else:
            path = os.path.join( self.output_dir , self.getOutputName() )
            
        return path 
        
    def getOutputName( self ):        
        """getOutputName() --> string
        
        Processes the target name according to the build target type, if any. 
        Otherwise, returns the unprocessed target name.
        """
        name = self.target_name
        
        if self.buildTarget:
            name = self.buildTarget.getOutputName( name )
        
        return name
            
    def EnableFeature(self, feature):
        if not self.features.has_key(feature):
            umake_lib.fatal("Feature '%s' not supported." % feature)

        if feature == "versioning_off":
            self.versioning_off = 2 ## magic
            
        self.features[feature]=1

    def DisableFeature(self, feature):
        if not self.features.has_key(feature):
            umake_lib.fatal("Feature '%s' not supported." % feature)

        if feature == "versioning_off":
            if self.versioning_off == 2: ## magic?
                self.versioning_off = 0

        self.features[feature]=0

    def FeatureIsEnabled(self, feature):
        return self.features.get(feature,0)

    def SetBuildVersion(self,t):
        if t < 20020822:
            return
        self.features["submodules"]=1

    def SetBuild2002Version(self,t):
        if t < 20020620:
            return
        self.features["submodules"]=1


    def writeln(self, buff):
        if not stringp(buff):
            umake_lib.fatal("project.writeln(): invalid argument type")
        self.__dict__[self.current_buffer].append(buff+"\n")

    def write(self, buff):
        if not stringp(buff):
            umake_lib.fatal("project.write(): invalid argument type")
        self.__dict__[self.current_buffer].append(buff)


    ## For backwards compatibility ONLY!!
    def append_script(self, script):
        for l in script.script_list:
            self.writeln(l)

    ## Fixme, this should generate the makefile and return it
    def get_buffer(self):
        return string.join(self.__dict__[self.current_buffer],'')
        
    def clear_buffer(self):
        self.post_target_buff = ""
        self.pre_target_buff = ""

    
    def SetModuleDepth(self, depth):
        print "UMAKE: Warning, project.SetModuleDepth is not required anymore and will soon dissappear!!!"


    def AddCommonDefines(self, cdefines):
        for define in string.split(cdefines):
            self.AddDefines(define)

    def AddCommonLibraries(self, clibraries):
        for library in string.split(clibraries):
            self.AddModuleLibraries(library)

    def AddCommonIncludes(self, cincludes):
        for include in string.split(cincludes):
            self.AddIncludes(include)

    def AddCommonSources(self, csources):
        for source in string.split(csources):
            self.AddSources(source)

    def TargetName(self):
        return self.__target_name

    def SetTargetName(self, target_name):
        self.current_buffer = 'post_target_buff'
        self.__target_name = target_name
        self.target_name = target_name

    def SetFakeTargetName(self, target_name):
        self.current_buffer = 'post_target_buff'
        self.target_name = target_name

    def SetTargetType(self, target_type):
        self.current_buffer = 'post_target_buff'
        if target_type not in buildtarget.getValidTypes():
            umake_lib.fatal("invalid target type=\"%s\"" % (target_type))
        # Assumes we have a platform object at this step. Platform object is
        # created by instantiation of UmakeGlobals object.
        self.buildTarget = buildtarget.createBuildTarget( target_type , platform )
        
    def OutputName(self):
        return self.__output_name

    def SetOutputName(self, output_name):
        self.__output_name = output_name
        
    def SetTargetDirectory(self, target_dir):
        self.target_dir = target_dir

    def DRMSign(self):
        self.__drm_sign = 1

    def setSignType( self , signType ):
        self.__signType = signType
        
    def addSignOption( self , key , value ):
        self.__signOptions[ key ] = value
        
    def getSignType( self ):
        return self.__signType
        
    def getSignOptions( self ):
        return self.__signOptions
        
    def CheckDRMSign(self):
        return self.__drm_sign

    def SetDLLTypePlugin(self):
        self.dll_type = 'plugin'

    def SetDLLTypeCodec(self):
        self.dll_type = 'codec'

    def SetDLLTypeCommon(self):
        self.dll_type = 'common'

    def AddBuildChoice(self, choice):
        self.AddBuildOption(choice)

    def RemoveBuildChoice(self, choice):
        self.RemoveBuildOption(choice)

    def AddBuildOption(self, choice):
        if not project.build_choices.count(choice):
            project.build_choices.append(choice)

    def RemoveBuildOption(self, choice):
        while project.build_choices.count(choice):
            project.build_choices.remove(choice)
            
    def BuildOption(self, option):
        if option in self.build_choices:
            return 1

        if option == "nodll":
            if self.FeatureIsEnabled("static_implies_nodll"):
                if ( "static" in self.build_choices  or 
                     "static_only" in self.build_choices ):
                    return 1
        return 0

    def AddSources(self, *args):
        self.sources.extend(umake_lib.listify(args))

    def AddModuleSources(self, *args):
        for source in umake_lib.listify(args):
            self.sources.append(
                apply(os.path.join,
                      [ self.src_root_path ] +
                      string.split(source,"/")))


    def RemoveSources(self, *args):
        self.sources = rmlist(self.sources, umake_lib.listify(args))

    def AddSourceObjects(self, *args):
        self.objsrcs = self.objsrcs + umake_lib.listify(args)

    def RemoveSourceObjects(self, *args):
        self.objsrcs = rmlist(self.objsrcs, umake_lib.listify(args))

    def AddIncludes(self, *args):
        self.includes = self.includes + umake_lib.listify(args)

    def RemoveIncludes(self, *args):
        self.includes = rmlist(self.includes, umake_lib.listify(args))

    def AddModuleIncludes(self, *args):
        for dir in umake_lib.listify(args):
            dir = apply(os.path.join,
                        [ self.src_root_path ] +
                        string.split(dir,"/"))
            self.AddIncludes(dir)

    def RemoveModuleIncludes(self, *args):
        for dir in umake_lib.listify(args):
            dir = apply(os.path.join,
                        [ self.src_root_path ] +
                        string.split(dir,"/"))
            self.RemoveIncludes(dir)

    def AddDefines(self, *args):
        for define in umake_lib.listify(args):
            define = string.strip(define)

            if define in self.defines:
                pass
                # umake_lib.warning("throwing out duplicate define=\"%s\"" % (define))
            else:
                self.defines.append(define)

    def RemoveDefines(self, *args):
        tmp={}
        for d in umake_lib.listify(args):
            tmp[d]=1

        defs=[]
        for d in self.defines:
            if not (tmp.get(d) or tmp.get(string.split(d,'=')[0])):
                defs.append(d)
        self.defines = defs

    def IsDefined(self, ppdef):
        if '=' not in ppdef:
            for d in self.defines:
                if string.split(d,"=")[0] == ppdef:
                    return 1
        else:
            if ppdef in self.defines:
                return 1

        return 0

    def DefineValue(self, ppdef):
        for d in self.defines:
            tmp=string.split(d,"=")
            if tmp[0] == ppdef:
                if len(tmp) > 1:
                    return tmp[1]
                else:
                    return ""
        return None

    def addLibpaths( self , *args ):
        self.libpaths = self.libpaths + umake_lib.listify( args )
        
    def removeLibpaths( self , *args ):
        self.libpaths = rmlist( self.libpaths , umake_lib.listify( args ) )
        
    def getLibpaths( self ):
        """getLibpath() --> list
        
        Returns a list of libpaths in the project.
        """
        return self.libpaths
        
    def AddLibraries(self, *args):
        self.libraries = self.libraries + umake_lib.listify(args)

    def RemoveLibraries(self, *args):
        self.libraries = rmlist(self.libraries, umake_lib.listify(args))

    def AddLibraries2(self, *args):
        self.libraries2 = self.libraries2 + umake_lib.listify(args)

    def RemoveLibraries2(self, *args):
        self.libraries2 = rmlist(self.libraries2, umake_lib.listify(args))

    def AddSystemLibraries(self, *args):
        self.sys_libraries = self.sys_libraries + umake_lib.listify(args)

    def RemoveSystemLibraries(self, *args):
        self.sys_libraries = rmlist(self.sys_libraries, umake_lib.listify(args))
        
    def AddSystemFrameworks(self, *args):
        self.sys_frameworks = self.sys_frameworks + umake_lib.listify(args)

    def RemoveSystemFrameworks(self, *args):
        self.sys_frameworks = rmlist(self.sys_frameworks, umake_lib.listify(args))
        
    def AddDynamicLibraries(self, *args):
        self.dynamic_libraries = self.dynamic_libraries + umake_lib.listify(args)

    def RemoveDynamicLibraries(self, *args):
        self.dynamic_libraries = rmlist(
            self.dynamic_libraries, umake_lib.listify(args))
                       
    def AddStaticSystemLibraries(self, *args):
        try:
            for l in umake_lib.listify(args):
                self.dynamic_libraries.append(platform.link.make_lib_static(l))
        except KeyError:
            print "UMAKE Warning: This platform does not support project.AddStaticSystemLibraries()"
            return

    def RemoveStaticSystemLibraries(self, *args):
        try:
            tmp = []
            for l in umake_lib.listify(args):
                tmp.append(platform.link.make_lib_static(l))

        except KeyError:
            print "UMAKE Warning: This platform does not support project.RemoveStaticSystemLibraries()"

            raise

        self.dynamic_libraries = rmlist(
            self.dynamic_libraries, tmp)
        

    def setup( self , platform ):
        """setup()
        
        Sets up the project data for writing to the makefile.
        
        This is a refactor in progress, with code coming over from 
        umake_lib.SetupProject()
        """
        log.trace( 'entry' )
        
        if sysinfo.host_type != "mac":
            self.__buildObjectLists( platform )
        
        log.trace( 'exit' )
        
        
    def __buildObjectLists( self , platform ):
        """__buildObjectLists( p )
        
        Iterates through the project's source data and makes the object file
        data for the given platform p.
        
        This method will only process source files where wantsProcess() is
        true.
        """        
        for path in self.sources[:]:
            log.debug( 'Setting up source and object file data for %s' % path )            
            sourcefile = umake_lib.SourceFile(platform, path, self.object_dir)            
            if sourcefile.wantsProcessing():
                self.__updateSourceAndObjectDataForSourcefile( sourcefile )
            else:
                log.debug( "'%s' does not want processing - skipping." % sourcefile.getSourceFile() )

                
    def __updateSourceAndObjectDataForSourcefile( self , src ):
        """__updateSourceAndObjectDataForSourcefile( s )
        
        Updates the project's object and source data for the given 
        sourcefile s, depending on whether the source file and/or object file
        exist.
        
        Logs a warning if both the source and object file don't exist.
        """
        if src.doesSourceExist():
            self.__addObject( src.getObjectFile() )             
        else:            
            log.warn( "Missing source file '%s'." %  src.getSourceFile() )            
            if src.doesObjectExist():
                self.__replaceSourceWithObject( src )                                                
            else:
                log.warn("Could not find object file '%s' for missing source '%s'." \
                    % ( src.getObjectFile() , src.getSourceFile() ) )

    
    def __replaceSourceWithObject( self , sourcefile ):
        """__replaceSourceWithObject( s )
        
        For a given sourcefile s, removes the source file from the source data
        and adds the object file to the source-objects list.
        
        Use this when the source file is missing, but the expected object file
        is already present.
        """
        log.trace( 'entry' , [ sourcefile.getSourceFile() ] )
        
        src = sourcefile.getSourceFile()
        obj = sourcefile.getObjectFile()        
        
        log.debug( "Replacing source file '%s' with object file '%s'" % ( src , obj ) )                        
        self.RemoveSources( src )
        self.AddSourceObjects( obj )
        
        log.trace( 'exit' )
        
    
    def __addObject( self , obj ):
        """__addObject( o )
        
        Adds the object file o to the project.
        """
        self.objects.append( obj )

        
    def AddModuleLibraries(self, *args):
        tmp=umake_lib.listify(args)
        self.module_libs.extend(tmp)

        for lib in tmp:
            lib = string.strip(lib)
            lib = string.replace(lib,"\\","/")
            libparts = string.split(lib,"/")
            lib=libparts[-1]
            m = re.match(r'([^[]+)\[([^]]+)\]', lib)
            if m:
                libparts[-1]=m.group(1)
                lib = m.group(2)
            
            hook_file = apply(os.path.join,
                              [self.src_root_path]+
                              libparts+
                              [lib + "_linkhook.cf"])

            hook_file=umake_lib.ci_find_file(hook_file)
            if hook_file:
                outmsg.verbose("exec_linkhook_file=\"%s\"" % (hook_file))
                my_exec_file(hook_file)

    def RemoveModuleLibraries(self, *args):
        self.module_libs = rmlist(self.module_libs, umake_lib.listify(args))
        
    def AddLocalLibraries(self, *args):
        self.local_libs = self.local_libs + umake_lib.listify(args)

    def RemoveLocalLibraries(self, *args):
        self.local_libs = rmlist(self.local_libs, umake_lib.listify(args))

    def AddSystemPaths(self, *args):
        self.system_paths = self.system_paths + umake_lib.listify(args)

    def RemoveSystemPaths(self, *args):
        self.system_paths = rmlist(self.system_paths, umake_lib.listify(args))
        
    def AddExportedFunctions(self, *args):
        self.exported_func = self.exported_func + umake_lib.listify(args)

    def ExportFunction(self, name, proto, include_path = None, include = None):
        self.exported_func.append(name)
        self.exported_function_protos[name]=[ proto, include, include_path ]

    def RemoveExportedFunctions(self, *args):
        self.exported_func = rmlist(self.exported_func, umake_lib.listify(args))

    ## project.AddDebugOutput:
    ## This function takes an environment variable and a relative
    ## path as arguments. If the environment variable exists, a
    ## link to/alias to/copy of the output will be placed in that
    ## directory. Example:
    ##
    ##   project.AddDebugOutput("RP_DEBUG_BASEDIR","plugins/rv/")
    ##
    ## This will copy the output into $RP_DEBUG_BASEDIR/plugins/rv/
    ## Since the paths are often platform specific, this functions
    ## should normally only be used in pcf files.
    ##
    ## NOTA BENE:
    ##   The last argument of this function must end with a slash
    ##
    def AddDebugOutput(self, env, rel):
        op=os.environ.get(env,None)
        if op:
            self.debug_copies.append( os.path.join(op, rel) )
        
    def AddCopyTargets(self, *args):
        self.copy_target_list.extend(umake_lib.listify(args))

    def RemoveCopyTargets(self, *args):
        self.copy_target_list = rmlist(
            self.copy_target_list, umake_lib.listify(args))

    def RemoveAllCopyTargets(self):
        self.copy_target_list = []

    def SetPreference(self, panel, setting, value):
        try:
            temp = self.preferences[panel]
        except KeyError:
            temp = self.preferences[panel] = {}
        
        temp[setting] = value

    def Set(self, name, value):
        """Compiler-dependant settings goes here"""
        self.preferences[name] = value

    def AddWeakLinkLibrary(self, *args):
        self.weak_link_list = self.weak_link_list + umake_lib.listify(args)

    def RemoveWeakLinkLibrary(self, *args):
        self.weak_link_list = rmlist(self.weak_link_list, umake_lib.listify(args))
        
    def AddPrefixFileInclude(self, *args):
        self.prefix_file_include_list = self.prefix_file_include_list + \
                                        umake_lib.listify(args)

    def RemovePrefixFileInclude(self, *args):
        self.prefix_file_include_list = rmlist(
            self.prefix_file_include_list, umake_lib.listify(args))

    def AddFileDependency(self, file, *args):
        if not self.file_dependencies.has_key(file):
            self.file_dependencies[file]=[]
        self.file_dependencies[file].extend(list(args))
        

    class SubModule:
        def umakefile(self):
            return self.umf

        def abs_umakefile(self):
            return self.abs_umf
        
        def makefile(self):
            return self.mf

        def abs_makefile(self):
            return self.abs_mf

        def dependencies(self):
            return self.deps
                
    def AddSubModule(self,
                     umf,
                     mf = None,
                     dependencies = None,
                     ignore_errors = None):

        abs_umf = os.path.join(self.module_directory(), umf)
        if mf:
            abs_mf = os.path.join(self.module_directory(), mf)
        else:
            lum = self.mangle(abs_umf)
            abs_mf = bldreg.get("makefile",lum,None)
            # print "depth=%d moddir=%s abs_umf=%s LUM=%s  abs_mf=%s" % ( self.module_depth, repr(self.module_directory()), repr(abs_umf), repr(lum), repr(abs_mf))

            if not abs_mf:
                umake_lib.fatal("Failed to find %s in registry." % repr(os.path.join(os.getcwd(), umf)))

            if os.path.dirname(abs_mf) == self.module_directory():
                mf = os.path.basename(abs_mf)
            else:
                mf = os.path.join(self.src_root_path, abs_mf)
                

            # print "depth=%d moddir=%s abs_umf=%s LUM=%s  abs_mf=%s mf=%s, rp=%s" % ( self.module_depth, repr(self.module_directory()), repr(abs_umf), repr(lum), repr(abs_mf),repr(mf),self.src_root_path)

        ## Create the module object
        mod = self.SubModule()
        mod.umf = umf
        mod.mf = mf
        mod.abs_umf = abs_umf
        mod.abs_mf = abs_mf
        mod.deps = dependencies
        mod.ignore_errors = ignore_errors

        self.submakes.append(mod)
        

    def _module_path_parts(self):
        tmp = os.getcwd()
        r = []

        if sysinfo.host_type == "mac":
            if tmp[-1] == ":":
                tmp = tmp[:-1]
                
        for a in range(0,self.module_depth):
            tmp, base = os.path.split(tmp)
            r = [ base ] + r

        return r
        

    def module_handle(self):
        r=self._module_path_parts()
        return string.join(r,"/")


    def module_directory(self):
        r=self._module_path_parts()

        if sysinfo.host_type == "mac":
            r = [ ":" ] + r

        if len(r):
            tmp=apply(os.path.join, r)
        else:
            tmp = ""

        return  tmp


    def set_created_by(self, target, abs_make):
        ltarget=self.mangle(target)
        labs_make=string.lower(abs_make)

        by = bldreg.get("created_by",ltarget, labs_make)

        if by != labs_make:
            print "UMAKE Warning: %s is created by both %s and %s" % (target, by, abs_make)
            ## Try to maintain compatibility
            bldreg.set_value("standard_makefile",
                             self.mangle(abs_make),
                             0)
            

        bldreg.set_value("created_by",
                         ltarget,
                         labs_make)


    def write_registry_info(self):
        """Writes some basic information about the target into the build
        system registry.  This requires the import of bldreg.py from the
        build system."""

        ## get the working directory
        module_directory = self.module_directory()

        abs_umake = os.path.join(module_directory,
                                 self.umakefile_name)

        abs_make = os.path.join(module_directory,
                                self.makefile_name)

        bldreg.set_value("umakefile",
                         self.mangle(abs_make),
                         abs_umake )

        bldreg.set_value("makefile",
                         self.mangle(abs_umake),
                         abs_make)

        ## Write dependency info
        if self.getTargetType() != 'lib':        
            libs = bldreg.get("extra_dependencies",abs_make, [])
            src_root=os.path.normpath(os.path.join(os.getcwd(),self.src_root_path))
            if src_root[-1] != os.sep:
                src_root=src_root + os.sep
                
            for m in self.module_libs + self.sys_frameworks + \
                    self.libraries + self.libraries2:

                m=os.path.normpath(os.path.join(os.getcwd(), m))
                if string.lower(m[:len(src_root)]) != string.lower(src_root):
                    break

                m=m[len(src_root):]
                if len(m) and m[0] in [':', '/', '\\']:
                    m=m[1:]
                libs.append(m)
                
            if self.getTargetType() == '':
                libs.append("FROMBIF")

            bldreg.set_value("dependencies",
                             self.mangle(abs_make),
                             libs)

        if self.getTargetType() != '' or \
               len(self.pre_target_buff) or \
               len(self.post_target_buff):
            pass
        else:
            makefiles = []
            for sumake in self.submakes:
                makefiles.append(sumake.abs_makefile())

            bldreg.set_value("alias",
                             self.mangle(abs_make),
                             makefiles)

            previous="FROMBIF"
            for sumake in self.submakes:
                amf=sumake.abs_makefile()
                deps=bldreg.get("extra_dependencies",amf, [])
                deps.append(previous)
                bldreg.set_value("extra_dependencies",amf, deps)
                previous=amf

        bldreg.set_value("standard_makefile",
                         self.mangle(abs_make),
                         self.isstandard())

        ## Don't write anything more if this is a custom makefile without
        ## known targets
        if not ( self.getTargetType() and self.target_name ):
            return
        
        target=self.target_name
        path = self.getOutputPath()


        ## the section "target_directory" keeps a mapping of target ID
        ## to the subdirectory it was built under; this allows us to
        ## easily check if there is a target ID collision where targets
        ## in two different subdirectories (modules) have the same name
        try:
            check_modue_dir = bldreg.get_value("target_directory", target)
        except KeyError:
            check_modue_dir = ""

        if len(check_modue_dir):
            if check_modue_dir != module_directory:
                temp = "target name=\"%s\" conflicts with directory=\"%s\""
                umake_lib.warning(temp % (target, check_modue_dir))

        bldreg.set_value("target_directory", target, module_directory)

        ## create the full target and object path
        target_path = os.path.join(module_directory, path)
        object_path = os.path.join(module_directory, self.object_dir)

        ## write the target/path key to the registry
        umake_lib.debug("[targets] %s = %s" % (target, target_path))
        bldreg.set_value("targets", target, target_path)

        handle=self.module_handle()
        bldreg.set_value("handle_to_file", "%s[%s]" % (handle, target), target_path)
        if string.split(handle,"/")[-1] == target:
            bldreg.set_value("handle_to_file", target, target_path)
            

        bldreg.set_value("file_to_handle", target_path, "%s[%s]" % (handle, target))

        self.set_created_by(target_path, abs_make)

        ## KLUGE
        if self.getTargetType() == 'dll':
            t2 = self.target_name
            if self.opt_target_name:
                t2 = self.opt_target_name
            t2 =  "%s%s.%s" % (platform.library_prefix,
                               t2,
                               platform.library_suffix)
            t2 = os.path.join(module_directory, self.output_dir, t2)
            self.set_created_by(t2, abs_make)


        ## write the path to the object files
        umake_lib.debug("[objects] %s = %s" % (target, object_path))
        bldreg.set_value("objects", target, object_path)

        ## write exported functions
        if len(self.exported_func):
            exported_functions = string.join(self.exported_func, ",")
            umake_lib.debug("[export] %s = %s" % (target, exported_functions))
            bldreg.set_value("export", target, exported_functions)
            for func in self.exported_func:
                if self.exported_function_protos.has_key(func):
                    bldreg.set_value("export_protos",target+"::"+func,
                                     self.exported_function_protos[func])
                                 
        else:
            bldreg.clear_value("export", target)

        ## set target type
        umake_lib.debug("[types] %s = %s" % ( target , self.getTargetType() ))
        bldreg.set_value( "types" , target , self.getTargetType() )


        if ("distribute" in self.build_choices or \
            "make_distributions" in self.build_choices ):
            if self.distribute_location:
                out = self.getOutputName()
                self.distribute_files[out]=self.distribute_location

            for (frm, to) in self.distribute_files.items():
                ## FIXME add path checks

                path = apply(os.path.join,
                             [ project.src_root_path ] +
                             string.split(to,"/"))

                ## FIXME: Allow specifying dbg/rel/.
                path=umake_lib.fix_library_path(platform,
                                                project,
                                                path,
                                                self.getTargetType() )


                cvsdir=os.path.join(os.path.dirname(path), "CVS")
                root = open(os.path.join(cvsdir,"Root"),"r").read()
                repository = open(os.path.join(cvsdir,"Repository"),"r").read()
                tag = ''
                try:
                    tag = open(os.path.join(cvsdir,"Tag"),"r").read()
                    if len(tag) and tag[0] == 'T':
                        tag=tag[1:]
                    else:
                        tag=''
                except IOError:
                    pass

                bldreg.set_value("distribute",
                                 frm,
                                 ( os.path.basename(path),
                                   string.strip(root),
                                   string.strip(repository),
                                   string.strip(tag) ))
                               

    def isstandard(self):
        """Standard projects can be distributed, nonstandard cannot"""
        if self.pre_target_buff or self.post_target_buff:
            #print "NONSTANDARD BUFFERS"
            return 0

        for t in self.xtargets:
            if t in ["all", "copy", "depend", "clean"]:
                continue
            #print "NONSTANDARD XTARGETS"
            return 0

        #print "ISSTANDARD: %d" % standard_makefile
        #if not standard_makefile:
        #    print self.pre_target_buff
        #    print self.post_target_buff
        #    print self.xtargets

        for sumake in self.submakes:
            amf=sumake.abs_makefile()
            std=bldreg.get("standard_makefile",self.mangle(amf),0)
            #print "ISSTANDARD(%s): %d" % (amf, std)
            if not std:
                return 0

        return 1

    def mangle(self, filename):
        filename = string.lower(filename)
        if sysinfo.host_type == "mac" and filename[0]!=':' and filename[:2]!="./":
            filaneme = ':' + filename
        filename = os.path.normpath(filename)
        return filename


    ###
    ### Module dependency handling
    ###

    def get_uber_makes(self):
        class UberMaker:
            def __init__(self, parent):
                self.src_root_path = parent.src_root_path
                self.SubModule = parent.SubModule
                self.uber_submakes = []
                self.done={}
                self.mangle = parent.mangle
                self.stats={}
                self.expcache={}
                self.addbifdepcache={}

            def inc(self, stat):
                self.stats[stat]=self.stats.get(stat,0)+1

            def show_stats(self):
                tmp=self.stats.items()
                tmp.sort(lambda x, y: x[1] - y[1])
                for (stat, num) in tmp:
                    print "%10d %s" % (num, stat)

            def expand_aliases(self, mf):
                ## Search for makefile aliases
                ## Multi-target umakefiles of various
                ## kinds typically generate aliases
                if self.expcache.has_key(mf):
                    return self.expcache[mf]

                tmp = []
                done = {}
                maks = [ mf ]
                for m in maks:
                    if done.has_key(m):
                        continue
                    done[m]=1
                    a = bldreg.get("alias", self.mangle(m), None)
                    if a:
                        maks.extend(a)
                    else:
                        tmp.append(m)

                #print "expand_aliases(%s) => %s" % (repr(mf), repr(tmp))
                self.expcache[mf]=tmp
                return tmp



            def addbifdep(self, depid):
                #print "ADDBIFDEP(%s)" % depid
                if self.addbifdepcache.has_key(depid):
                    return self.addbifdepcache[depid]

                ret={}
                type = bldreg.get("bifmodule_type", depid, "")
                if type == "cvs":
                    path=bldreg.get("bifmodule_id_to_path",depid,None)
                    if path:
                        path = string.replace(path,"/",os.sep)
                        if sysinfo.host_type == "mac":
                            path = ":" + path

                        abs_mf=os.path.join(path,"Makefile")
                        for x in self.expand_aliases(abs_mf):
                            ret[x]=1
                elif type == "name_only":
                    for x in bldreg.get("bifmodule_deplist",depid,[]):
                        ret.update(self.addbifdep(x))

                self.addbifdepcache[depid]=ret
                return ret


            def low_add(self,
                        umf,
                        abs_umf,
                        mf,
                        abs_mf,
                        ignore_errors = 0):

                lmf=self.mangle(abs_mf)
                if(self.done.has_key(lmf)):
                    return self.done[lmf]

                self.done[lmf]=1

                if not mf:
                    mf = os.path.join(self.src_root_path, abs_mf)

                if not abs_umf:
                    abs_umf = bldreg.get("umakefile",lmf,None)
                    if not abs_umf:
                        self.done[lmf]=0
                        return 0
                    
                if not umf:
                    umf = os.path.join(self.src_root_path, abs_umf)
                    

                #print "low_add(umf=%s,abs_umf=%s,mf=%s,abs_mf=%s,err=%d,lmf=%s)" % (umf, abs_umf, mf, abs_mf, ignore_errors,lmf)

                deps=bldreg.get("dependencies",lmf,[])
                #print "low_add deps = %s" % repr(deps)
                tmp={}
                for d in deps:
                    if d == "FROMBIF":
                        modid = umake_lib.module_from_file(abs_umf)
                        if modid:
                            for depid in bldreg.get("bifmodule_deplist",modid,[]):
                                tmp.update(self.addbifdep(depid))
                        else:
                            print "Umake warning: Module does not exist in BIF file."
                        continue

                    dum = bldreg.get("created_by",self.mangle(d), None)
                    #print "     DEP %s (%s) => %s" % (d, self.mangle(d), repr(dum))
                    if dum:
                        tmp[dum]=1

                tmp=tmp.keys()
                tmp2=[]
                for d in tmp:
                    if self.low_add_mf(d):
                        tmp2.append(d)

                ## Create the module object
                mod = self.SubModule()
                mod.umf = umf
                mod.mf = mf
                mod.abs_umf = abs_umf
                mod.abs_mf = abs_mf
                mod.deps = tmp2
                mod.ignore_errors = ignore_errors
                self.uber_submakes.append(mod)
                self.done[lmf]=mod
                return mod

            def low_add_mf(self, abs_mf, ignore_errors = 0):
                return self.low_add(None, None, None, abs_mf, ignore_errors)

            def add_um(self, um, ignore_errors = 0):
                lum=self.mangle(abs_um)
                ## No makefile, no module
                abs_mf = bldreg.get("makefile",lum,None)
                if not abs_mf:
                    return
                mf = os.path.join(self.src_root_path, abs_mf)
                umf = os.path.join(self.src_root_path, abs_umf)
                return self.low_add(umf, abs_umf, mf, abs_mf, ignore_errors)

            def add_mf(self, abs_mf, ignore_errors = 0):
                for mf in self.expand_aliases(abs_mf):
                    self.low_add_mf(mf, ignore_errors)

        u = UberMaker(self)
        u.add_mf(os.path.join(self.module_directory(), self.makefile_name))
        u.show_stats()
        return u.uber_submakes


    def SetDistLocation(self,
                        release=None,
                        debug=None,
                        any=None):
        location=None
        if "release" in self.build_choices:
            location=release or any
        else:
            location=debug or any
            
        self.distribute_location=location

    def DistributeFile(self,
                       file,
                       release=None,
                       debug=None,
                       any=None):
        location=None
        if "release" in self.build_choices:
            location=release or any
        else:
            location=debug or any
            
        self.distribute_files[file]=location
        self.AddCopyTargets(file)
                        

    ## FIXME: Is posix path right here, or do we need to naturalize it?
    def SetVersionFile(self, file):
        self.version_file = file

#
###############################################################################
#

def UmakefileVersion(major, minor, micro = 0):
    if major < 2000:
        num = major * 10000 + minor * 100 + micro
        if num < 20000:
            return
        project.EnableFeature("submodules")
        if num < 20100:
            return
        project.EnableFeature("versioning_off")
        if num < 20200:
            return
        project.EnableFeature("static_implies_nodll")
        if num < 20400:
            return
        project.EnableFeature("makefile_quotes")
    else:
        major = major - 2000
        num = major * 10000 + minor * 100 + micro

        if num < 20000:
            return
        project.EnableFeature("submodules")



def GetSDKPath(sdk_name, defval=None):
    """This returns the path to a named SDK"""
    import sdk
    path = sdk.GetPath(sdk_name)

    if not path:
        path=defval

    if not path:
        umake_lib.fatal("  Failed to find path to\n  SDK named '%s'. Please read documentation for instructions\n  on how to obtain and install this SDK." % sdk_name)

    path=umake_lib.fix_path(path)

    return os.path.normpath(os.path.join(os.getcwd(),project.src_root_path, path))

def SetSDKPath(sdk_name, path):
    """Set the path to a named SDK"""
    import sdk
    sdk.SetPath(sdk_name, path)



def get_module_output(module, *xpath):
    """Easy way to get something from the output dir of a module.
    First argument should be the path from the top to the module and
    the second (if present) should be the file/dir you want from the
    output dir of that module. All paths should use / to separate dirs."""
    
    parts=[ project.src_root_path ]
    parts.extend(string.split(module,"/"))
    parts.append(project.output_dir)
    for x in xpath:
        parts.extend(string.split(x,"/"))

    return apply(os.path.join, parts)



def UseCommonObjects():
    global USE_COMMON_OBJ
    USE_COMMON_OBJ = 1

def CommonDefines(*args):
    global common_defines
    common_defines = string.join(umake_lib.listify(args))

def CommonLibraries(*args):
    global common_libraries
    common_libraries = string.join(umake_lib.listify(args))

def CommonIncludes(*args):
    global common_includes
    common_includes = string.join(umake_lib.listify(args))

def CommonSources(*args):
    global common_srcs
    common_srcs = string.join(umake_lib.listify(args))

def __common_target(target):
    project.AddCommonDefines(common_defines)
    project.AddCommonLibraries(common_libraries)
    project.AddCommonIncludes(common_includes)
    project.AddCommonSources(common_srcs)
    project.SetTargetName(target)
    project.SetOutputName( project.getOutputName() )
    project.AddCopyTargets( project.getOutputPath() )
    

def __with_resource_target(rtarget, rfile, includes):
    project.with_resource_flag = 1
    project.resource_target = rtarget
    project.resourcefile = rfile
    project.resourceincludes = []
    
    if type(includes) == types.ListType:
        for item in includes:
            project.resourceincludes.append(umake_lib.fix_path(item))
            
    elif type(includes) == types.StringType:
        project.resourceincludes.append(umake_lib.fix_path(includes))
            
    else:
        umake_lib.fatal("invalid includes type")
    

def ProgramWithResourceTarget(target, rtarget, rfile, includes):
    if not stringp(target):
        umake_lib.fatal("invalid argument")
    elif not stringp(rtarget):
        umake_lib.fatal("invalid argument")
    elif not stringp(rtarget):
        umake_lib.fatal("invalid argument")
    elif type(includes) not in [types.StringType, types.ListType]:
        umake_lib.fatal("invalid argument")

    project.SetTargetType("exe")
    __with_resource_target(rtarget, rfile, includes)
    __common_target(target)

def DLLWithResourceTarget(target, rtarget, rfile, includes):
    if not stringp(target):
        umake_lib.fatal("invalid argument")
    elif not stringp(rtarget):
        umake_lib.fatal("invalid argument")
    elif not stringp(rtarget):
        umake_lib.fatal("invalid argument")
    elif type(includes) not in [types.StringType, types.ListType]:
        umake_lib.fatal("invalid argument")

    project.SetTargetType("dll")
    __with_resource_target(rtarget, rfile, includes)
    __common_target(target)


def ProgramTarget(target):
    if not stringp(target):
        umake_lib.fatal("invalid argument")

    project.SetTargetType("exe")
    __common_target(target)

def LibraryTarget(target):
    if not stringp(target):
        umake_lib.fatal("invalid argument")
    project.SetTargetType("lib")
    __common_target(target)

def NonObjectTarget( target ):
    if not stringp( target ):
        umake_lib.fatal( 'invalid argument' )
    project.SetTargetType( 'nonobj' )
    project.disableLinking()
    __common_target( target )
    
def DLLTarget(target, libname = None):
    if not stringp(target):
        umake_lib.fatal("invalid argument")

    ## create a static library instead of
    if project.BuildOption("nodll"):
            
        ## set the type of DLL which is being built into a static
        ## library; this is used for creating a "static" player
        bldreg.set_value("dll_type", target, project.dll_type)

        ## to ensure entrypoints are of a well-known form and unique
        ## in the static version of the DLL, their names are mangled
        ## using the following define
        project.AddDefines("_PLUGINNAME=%s" % (target))
        LibraryTarget(target)
        return
        
    project.SetTargetType("dll")
    __common_target(target)
    project.opt_target_name=libname
 
## DLLOnlyTarget() always builds DLL regardless the build option(i.e. "nodll")
## LibTarget() always builds the LIB
## DLLTarget() builds the target depending on the build option(i.e. "nodll")
def DLLOnlyTarget(target, libname = None):
    if not stringp(target):
        umake_lib.fatal("invalid argument")

    project.SetTargetType("dll")
    __common_target(target)
    project.opt_target_name=libname
    
## FIXME
def AllTarget(depends):
    if not stringp(target):
        umake_lib.fatal("invalid argument")

    project.alldepends.append(depends)
    project.xtargets.append("all")

## FIXME
def CopyTarget(target_list = []):
    if type(target_list) != types.ListType:
        umake_lib.fatal("invalid argument")

    project.AddCopyTargets(target_list)
    project.xtargets.append("copy")

## FIXME
def EmptyTarget():
    project.xtargets.append("empty")
    
def CreateStaticPluginTable(*args):
    plugin_list = umake_lib.listify(args)

    import umake_codegen
    umake_codegen.WriteDLLTab(platform, project, plugin_list)

def ProjectTarget():
    """ Make a multi-project makefile, all projects must have been
    added with project.AddModule() first"""
    project.xtargets.extend( [ "all","depend","copy","clean" ])
    project.current_buffer = 'post_target_buff'

def UpdateReleaseNotes(infile, outfile, verfile = None, buildtime = 0):
    """ Substitute variables in a release notes file with the current
    module version and/or date."""

    # we require time for strftime
    import time

    # default: use the project name as the base for the .ver file
    if not verfile:
        verfile = project.target_name

    # remove extension from the filename-- get_version() will add it for us
    (verfile, ext) = os.path.splitext(verfile)

    # grab the version from the verfile
    version = ('0', '0', '0', '0')
    try:
        version = platform.versioning.get_version(verfile, "")
    except IOError:
        outmsg.error("UpdateReleaseNotes: unable to obtain version " +
                     "info from %s.ver\n" % verfile)

    # default time is localtime
    if not buildtime:
        buildtime = time.localtime(time.time())

    # read input
    try:
        data = open(infile, "r").read()
    except IOError:
        outmsg.error("UpdateReleaseNotes: unable to read template file " +
                     "%s\n" % infile)
        return

    # setup some substitution values
    subs = { "version"      : "%s.%s.%s.%s" % version,
             "major"        : version[0],
             "minor"        : version[1],
             "release"      : version[2],
             "build"        : version[3],
             "YYYYMMDD"     : time.strftime("%Y%m%d", buildtime),
             "YYYY"         : time.strftime("%Y", buildtime),
             "YY"           : time.strftime("%y", buildtime),
             "MM"           : time.strftime("%m", buildtime),
             "DD"           : time.strftime("%d", buildtime),
             "month"        : time.strftime("%B", buildtime),
             "mon"          : time.strftime("%b", buildtime),
             "datetime"     : time.strftime("%c", buildtime),
             "time"         : time.strftime("%X", buildtime),
             "date"         : time.strftime("%x", buildtime),
             "24hour"       : time.strftime("%H", buildtime),
             "hour"         : time.strftime("%I", buildtime),
             "min"          : time.strftime("%M", buildtime),
             "ampm"         : time.strftime("%p", buildtime),
             "sec"          : time.strftime("%S", buildtime),
             "weekday"      : time.strftime("%A", buildtime),
             "wkdy"         : time.strftime("%a", buildtime),
             "timezone"     : time.strftime("%Z", buildtime),
             "distribution" : sysinfo.distribution_id,
             "arch"         : sysinfo.arch,
             "platform"     : sysinfo.platform,
             "target"       : project.target_name }


    # pre-compile our regexp
    regexp = re.compile("%(\w+)%")

    # parse input data, replace matched keywords
    output = []
    pos = 0
    while 1:
        match = regexp.search(data, pos)
        if not match:
            break
        output.append(data[pos:match.start(0)])
        key = match.group(1)
        output.append(subs.get(key, "%" + key + "%"))
        pos = match.end(0)
    output.append(data[pos:])

    # write output
    try:
        umake_lib.write_file(outfile, string.join(output, ""))
    except IOError:
        outmsg.error("UpdateReleaseNotes: unable to write output " +
                     "file %s\n" % outfile)



def MultiTargetMake(*args):
    ## use either old syntax, or new syntax for Multi-target Makefiles
    if len(args):
        umakefile_list = umake_lib.listify(args)
        platform_name_pcf_files_list = []
        platform_type_pcf_files_list = []
    else:
        umakefile_list = string.split(sub_umakes)
        platform_name_pcf_files_list = string.split(sub_platform_name_pcfs)
        platform_type_pcf_files_list = string.split(sub_platform_type_pcfs)


    ## the SubUmake class contains data used for one
    ## sub-umake target... this is not pretty -JMP
    class SubUmake:
        def __init__(self):
            self.umakefil = ""
            self.name_pcf = ""
            self.type_pcf = ""
            self.makefile = ""

    sumake_list = []
    for i in range(len(umakefile_list)):
        sumake = SubUmake()
    
        sumake.umakefil = umakefile_list[i]
        if string.lower(os.path.basename(sumake.umakefil)) == "umakefil":
            sumake.makefile = os.path.join(os.path.dirname(sumake.umakefil), "Makefile")
        else:
            sumake.makefile = "%s.mak" % (sumake.umakefil)
            
        if project.BuildOption("static"):
            ( dirname, basename) = os.path.split(sumake.makefile)
            basename = "static_" + basename
            sumake.makefile = os.path.join(dirname, basename)

        if len(platform_name_pcf_files_list) > i:
            sumake.name_pcf = platform_name_pcf_files_list[i]

        if len(platform_type_pcf_files_list) > i:
            sumake.type_pcf = platform_type_pcf_files_list[i]
 
        ## old-style MultiTargetMake() files are somtimes messed up
        ## where the developer has listed the same .pcf file for both
        ## the name_pcf and type_pcf
        if sumake.name_pcf == sumake.type_pcf:
            sumake.type_pcf = ""
 
        sumake_list.append(sumake)

    ## for each sumake, recursivly call Umake()
    olddir = os.getcwd()
    for sumake in sumake_list:
        if project.features["submodules"]:
            (dir, umakefile)=os.path.split(sumake.umakefil)
            makefile=os.path.basename(sumake.makefile)
            if dir:
                os.chdir(dir)
        else:
            umakefile=sumake.umakefil
            makefile=sumake.makefile

        um_opts = UmakeOptions()
        um_opts.build_options = project.build_choices[:]
        um_opts.platform_type_pcf = sumake.type_pcf
        um_opts.platform_name_pcf = sumake.name_pcf
        um_opts.umakefile_name = umakefile
        um_opts.makefile_name = makefile
        um_opts.multitarget_pcf_prefix = umakefile

        ## objects must go in a different directory since the same
        ## file might be compiled multiple times with different
        ## defines
        ##
        ## if USE_COMMON_OBJ is true, use the same objects; this is an
        ## optimization which can be turned on if the difference between
        ## two targets is only in their source files
        if not USE_COMMON_OBJ:
            if string.lower(os.path.basename(umakefile)) != "umakefil":
                um_opts.object_dir = os.path.join(
                    project.object_dir, os.path.basename(umakefile))
        try:
            RunUmake(um_opts)
        except err.error, e:
            os.chdir(olddir)
            e.text = e.text + "\n  called from %s" % (os.path.join(os.getcwd(), project.umakefile_name))
            raise

        os.chdir(olddir)

        ## Makefile name will be fetched from registry
        project.AddSubModule(sumake.umakefil)

    ProjectTarget()

###############################################################################
#
# UMAKE Entrypoints
#

class UmakeOptions:
    def __init__(self):
        self.build_options = None
        self.object_dir = None
        self.umakefile_name = None
        self.makefile_name = None
        self.multitarget_pcf_prefix = None
        self.platform_type_pcf = None
        self.platform_name_pcf = None

    def Process(self):
        """Tweaks the options umake was invoked with, selecting defaults and
        removing conflicting options if necessary."""

        ## set defaults from the given umake options
        if not self.umakefile_name:
            self.umakefile_name = "Umakefil"

        if not self.makefile_name:
            self.makefile_name = "Makefile"

        ## fix the umakefil name for case-insensitive operating systems
        temp = umake_lib.ci_find_file(self.umakefile_name)
        if temp == None:
            #print project.build_choices
            if "__print_version_and_exit__" not in self.build_options:
                umake_lib.fatal("umakefil=\"%s\" not found" % (
                    os.path.join(os.getcwd(),self.umakefile_name)))
        else:
            self.umakefile_name = temp

        for opt in self.build_options[:]:
            while self.build_options.count(opt) > 1:
                self.build_options.remove(opt)

        if "release" in self.build_options or "relsymbl" in self.build_options:
            if "debug" in self.build_options:
                self.build_options.remove("debug")
        else:
            if "debug" not in self.build_options:
                self.build_options.append("debug")

def get_pcf_files(um_opts, dir, prefix, file):
    pcf_file_list = []

    if not NO_PCF_EXECUTION:

        ## old-style MultiTarget pcf files, thank Jeff Ayres in 1998
        ## for this mess
        if um_opts.platform_type_pcf or um_opts.platform_name_pcf:
            if um_opts.platform_type_pcf:
                pcf_file_list.append(um_opts.platform_type_pcf)
            if um_opts.platform_name_pcf:
                pcf_file_list.append(um_opts.platform_name_pcf)

        ## new-style MultiTarget pcf files work in a similar way to normal
        ## pcf files, much better
        elif prefix:
            for pcf_prefix in platform.pcf_prefix_list:
                pcf_file_list.append(pcf_prefix)
                pcf_file_list.append(
                    "%s_%s" % (prefix, pcf_prefix))
            pcf_file_list.append("all")

        ## single-target pcf file gathering
        else:
            for pcf_prefix in platform.pcf_prefix_list:
                pcf_file_list.append(pcf_prefix)

    ## add ".pcf" file extention to the pcf_file_list
    for i in range(len(pcf_file_list)):
        pcf_file_list[i] = "%s%s.pcf" % (dir, pcf_file_list[i])

    pcf_file_list.append(dir+file)


    return pcf_file_list

def ExecUmakefil(um_opts):
    ## run the PCF files
    ## Multi target umake files will set platform_type_pcf and
    ## platform_name_pcf to specify the platform name and type pcf files
    ## NOTE: ORDER IS CRITICAL!!!

    pcf_file_list = []

    for file in bldreg.get("umake","includefiles",[]):
        pcf_file_list.extend(
            get_pcf_files(um_opts,
                          os.path.join(project.src_root_path,
                                       os.path.dirname(file)) + os.sep,
                          os.path.basename(file),
                          os.path.basename(file)))


    pcf_file_list.extend(
        get_pcf_files(um_opts,
                      "",
                      um_opts.multitarget_pcf_prefix,
                      project.umakefile_name))


    ## preprocess all the files involved with the Umake preprocessor
    ## come up with the path/name of the umake pre-processed file
    umpp_path = "%s.upp" % (project.umakefile_name)
    umpp = umakepp.UMPreProcessor(umpp_path)
    umpp.add_file_list(pcf_file_list)
    project.upp_last_modified = umpp.write()

    check_standard_makefile(umpp_path)
    safe_execfile(umpp_path)


def Win32StaticHackExecUmakefil(um_opts):
    ## stop recursion if build option "stop" is found
    if project.BuildOption("stop"):
        ExecUmakefil(um_opts)

    elif project.BuildOption("static_only"):
        ExecUmakefil(um_opts)

    elif project.BuildOption("static"):
        static_um_opts = copy.deepcopy(um_opts)
        static_um_opts.makefile_name = "SMakefil"
        static_um_opts.build_options.append("stop")
        RunUmake(static_um_opts)

        normal_um_opts = copy.deepcopy(um_opts)
        normal_um_opts.makefile_name = "NMakefil"
        normal_um_opts.build_options.remove("static")
        normal_um_opts.build_options.append("stop")
        RunUmake(normal_um_opts)

        ## XXX: major hack: notice errors are ignored in the static pass,
        ## but not ignored in the normal pass; this is because there
        ## are many modules which are built static, but not meant to be
        ## built static, and they fail to compile staticly; we can't ignore both,
        ## because then the error reporting would be all wrong! -JMP

        project.AddSubModule(project.umakefile_name,
                             "SMakefil",
                             [],
                             1)

        project.AddSubModule(project.umakefile_name,
                             "NMakefil",
                             [],
                             0)

        ProjectTarget()


def RunUmake(um_opts):
    ## Save globals so they can be restored at the end of the function
    old_globals = UmakeGlobals()
    old_globals.SaveGlobals()
    globals = UmakeGlobals()
    globals.SetGlobals()

    try:
        um_opts.Process()

        ## add in the build options
        for opt in um_opts.build_options:
            project.AddBuildOption(opt)

        tmp = os.getcwd()
        if sysinfo.host_type == "mac" and tmp[-1]==":":
            tmp = tmp[:-1]
        srcroot=""
        depth=0

        while not os.path.isfile(os.path.join(tmp, "build.reg")):
            #print "isnotfile: %s" % os.path.join(tmp, "build.reg")
            #print "tmp = %s" % repr(tmp)
            srcroot = os.path.join(srcroot, os.pardir)
            depth = depth + 1
            base = os.path.dirname(tmp)
            if base == tmp or not len(base):
                umake_lib.fatal("Failed to find build.reg, please run build in source root to initiate build system. Or create an empty file in the source root and try again.")
            tmp = base

        if not srcroot:
            srcroot = os.curdir

        # Kluge
        if sysinfo.host_type == 'mac':
            import macpath
            srcroot = macpath.normpath(srcroot)

        project.module_depth = depth
        project.src_root_path = srcroot

        ## Put additional python stuff in here
        sys.path.insert(0, os.path.join(srcroot, "common", "buildutils", "umakelib"))

        umake_lib.debug("CWD: %s, root = %s, depth=%d, mdir=%s" % (os.getcwd(), srcroot, depth, project.module_directory()))

        if not project.BuildOption("__print_version_and_exit__"):
            outmsg.verbose("UMAKE: %s -> %s in %s" % ( um_opts.umakefile_name, um_opts.makefile_name,  project.module_directory() ))

        ## set the target directory
        if project.BuildOption("debug"):
            target_dir = os.path.join(project.src_root_path, "debug")
        else:
            target_dir = os.path.join(project.src_root_path, "release")
        project.SetTargetDirectory(target_dir)

        project.umakefile_name = um_opts.umakefile_name
        project.platform = platform

        ## set the name of the output Makefile, delete the old one if it
        ## exists
        project.makefile_name = um_opts.makefile_name

        ## Import global defines from build.reg
        defs = bldreg.get("build","defines",{})
        for d in defs.keys():
            project.AddDefines("%s=%s" % (d, defs[d]))

        project.AddDefines("RIBOSOME_TEST_BRANCH")
        
        ## Exec Profile
        profile = os.environ.get("PROFILE_ID",bldreg.get("build","profile","default"))

        if profile and bldreg.get("build","bif_profile","") != profile:
            if not project.BuildOption("__print_version_and_exit__"):
                outmsg.verbose("UMAKE: Applying profile %s.pf" % ( profile ))
            exec_profile_file("%s.pf" % profile)

        ## platform specific configuration in UMAKE
        exec_config_file("%s.cf" % (sysinfo.id))

        ## build the object target directory -- this is done after the .cf
        ## file has been executed; otherwise we don't have our
        ## project.output_dir set
        if um_opts.object_dir:
            project.object_dir = um_opts.object_dir
        else:
            project.object_dir = os.path.join(project.output_dir, "obj")

        umake_lib.debug("-- BEGIN UMAKEFILE: %s --" % (project.umakefile_name))

        ## XXX: nasty Win32 static hack for installers
        ## this was moved out of the build system, and is hacked in here where
        ## we simulate a multi-target Makefile; this is actually cleaner than
        ## having it in the build system -JMP    
        if platform.can_build_static:
            if project.BuildOption("static") or project.BuildOption("static_only"):
                Win32StaticHackExecUmakefil(um_opts)
            else:
                ExecUmakefil(um_opts)
        else:
            project.RemoveBuildOption("static")
            project.RemoveBuildOption("static_only")
            ExecUmakefil(um_opts)

        exec_umakerc()

        if project.BuildOption("__print_version_and_exit__"):
            ## Call callbacks
            print "$Id: umake.py,v 1.60 2007/06/13 01:12:50 jfinnecy Exp $"
            global umake_identify_callbacks
            for (func, args) in umake_identify_callbacks:
                apply(func, args)
            return

        ## Call callbacks
        global umake_epilogue_callbacks
        log.debug( 'Executing UmakeCallbacks' )
        for (func, args) in umake_epilogue_callbacks:
            log.debug('Executing %s( %s )' % ( func , args ) )
            apply(func, args)

        ## Compatibility cruft
        if not project.features["submodules"]:
            project.target_dir = os.path.join(os.pardir,
                                              os.path.basename(project.target_dir))


        makefile_time = umake_lib.mtime(project.makefile_name)

        ## FIXME: If SYSTEM_ID has changed, this test is not valid!
        ## Or if any other option has changed for that matter.
        ## So I'm going to comment this out for now. -Hubbe
        if 0 and makefile_time > project.cf_last_modified and \
               makefile_time >= project.upp_last_modified:
            print "%s up to date." % (project.makefile_name)
        else:
            ## Delete old makefile
            if os.path.exists(project.makefile_name):
                os.remove(project.makefile_name)

            umake_lib.SetupPlatform(platform)
            umake_lib.SetupProject(platform, project)
            
            # Set the platform info in the build target object
            # project.buildTarget.setPlatform( platform )

            ## At this point, project and platform are done and should not be
            ## modified any more -- Save.
            project.write_registry_info()

            ## get the result from the "project" class, which also writes the
            ## Makefile to disk (it's in memory up to this point only!)

            mod = __import__(compile.get_umake_backend())
            #print "%s" % repr(dir(mod))
            mod.make_makefile(platform, project)

            umake_lib.debug("-- END UMAKEFILE: %s --" % (project.umakefile_name))
    finally:
        ## Restore Global Varibles
        old_globals.SetGlobals()
        # print "UMAKE POP!"


def Umake(
    build_options = None,
    object_dir = None,
    umakefile_name = None,
    makefile_name = None,
    platform_type_pcf = None,
    platform_name_pcf = None,
    multitarget_pcf_prefix = None):

    chaingang.chdir_lock.acquire()
    try:
        um_opts = UmakeOptions()
        um_opts.build_options = build_options
        um_opts.object_dir = object_dir
        um_opts.umakefile_name = umakefile_name
        um_opts.makefile_name = makefile_name
        um_opts.multitarget_pcf_prefix = multitarget_pcf_prefix
        um_opts.platform_type_pcf = platform_type_pcf
        um_opts.platform_name_pcf = platform_type_pcf

        RunUmake(um_opts)

    finally:
        chaingang.chdir_lock.release()

#
###############################################################################


if __name__ == '__main__':
    ## save lot's of hair pulling if someone messes up their environ
    umake_lib.fatal('never run "umake.py" directly; use "umake" instead.')

