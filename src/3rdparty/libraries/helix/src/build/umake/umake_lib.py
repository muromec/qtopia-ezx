# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_lib.py,v 1.34 2006/06/19 23:11:32 jfinnecy Exp $ 
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
"""This file contains common library functions, such as error output, and
base classes used by other parts of the Umake system.  Only code common
to all Umake platforms should go here."""

import os
import sys
import string
import types
import err
import bldreg
import re
import stat
import sysinfo

import log
log.debug( 'Imported: $Id: umake_lib.py,v 1.34 2006/06/19 23:11:32 jfinnecy Exp $' )

DEBUG=0

def fatal(text):
    """Raise a fatal exception."""
    e = err.Error()
    e.Set(text)
    raise err.error, e


def warning(text):
    """Print out a warning and exit."""
    print "UMAKE Warning: %s" % (text)


def namespace_modified(text):
    """Print out a namespace modified message."""
    print "UMAKE Namespace Modified: %s" % (text)


def debug(text):
    """Print out a debug message."""
    if DEBUG:
        print text


def listify(stuff):
    """Takes a string, tuple, list, or lists of sublists, and makes a new,
    single list out of all the strings in them."""
    
    ## input is string
    if type(stuff) == types.StringType:
        return [stuff]

    try:
        if type(stuff) == types.UnicodeType:
            return [stuff]
    except AttributeError:
        pass

    ## has to be a list or tuple
    if type(stuff) != types.ListType and type(stuff) != types.TupleType:
        return []

    list = []

    for item in stuff:
        list.extend(listify(item))

    return list


def ci_find_file(find_filename):
    """Searches the current directory for filename case-insesitive on
    case-sensitive operating systems.  Returns None if it cannot find
    the file.  There's a bug here: while we search for the filename
    in a case-insensitive way, we don't do the same for the directory."""


    if os.path.exists(find_filename):
        return find_filename

    (path, basename) = os.path.split(find_filename)

    ## os.listdir cannot take a blank path as the current dir,
    list_path = path or os.curdir
    list_path = ci_find_file(list_path)
    if not list_path:
        return None

    ## Ugly hack for OS9
    if sysinfo.host_type == "mac":
        import archive
        basename=archive.mac_name_mangler(basename)
        p=os.path.join(list_path, basename)
        if os.path.exists(p):
            return p

    find_filename = string.lower(basename)
    for filename in os.listdir(list_path):
        if find_filename == string.lower(filename):
            return os.path.join(list_path, filename)

    return None


def fix_path_mac(path):
    """Macintosh implementation of fix_path() seperated out for clarity.  This
    is called by fix_path()."""

    if not path or not len(path):
        return ""

    if ':' in path and '/' not in path and '\\' not in path:
        ## The path is already a mac path!
        return path

    path = string.replace(path, "/", ":")

    mac_path = ""
    last_backdir = 0
    last_curdir = 0

    # no directory info defaults to current dir
    if path[0] == ":":
        import macpath
        mac_path=string.split(macpath.normpath(os.getcwd()),":")[0]
    elif path[0] != ".":
        mac_path = ":"

    i = 0
    while i < len(path):
        # translate current directory
        if path[i:i+2] == ".:":
            if not last_curdir and not last_backdir:
                mac_path = mac_path + ":"

            last_curdir = 1
            i = i + 2
            continue

        # translate stepping back a directory
        if path[i:i+3] == "..:":
            if last_backdir or last_curdir:
                mac_path = mac_path + ":"
            else:
                mac_path = mac_path + "::"
                last_backdir = 1
            i = i + 3
            continue

        # append to mac_path
        mac_path = mac_path + path[i]
        i = i + 1
        last_curdir = 0
        last_backdir = 0

    return mac_path

    
def fix_path(path):
    """Takes a path which is in either POSIX or native format, and converts it
    to a native path on the running system."""

    if not path or not len(path):
        return ""
    
    if sysinfo.host_type == "mac":
        # print "fix_path_mac(%s) = > %s\n" % (path, fix_path_mac(path))
        return fix_path_mac(path)
    
    path = string.replace(path, "/", os.sep)
    return path

def fixabspath(project, path):
    if os.path.isabs(path):
        sr=os.path.normpath(os.path.join(os.getcwd(),project.src_root_path))
        if sr[-1] != os.sep:
            sr=sr+os.sep
        if string.lower(sr) == string.lower(path)[:len(sr)]:
            return os.path.join(project.src_root_path, path[len(sr):])
    return path

    
def SetupPlatform(platform):
    """Twek some settings in the Platform object, and run the linker class's
    set_type() method.  The .c and .cpp build rules are generated here."""
    
    ## the C & C++ compiler build rules are handled outside the
    ## platform.build_rules hash table, so we set them here since
    ## they may have been replaced in the Umakefil/*.pcf file
    import umake

    try:
        platform.build_rules[".cpp"] = umake.BuildRule(
            ".cpp", ".%s" % (platform.object_suffix), platform.cxx)
    except AttributeError: # I hate these - hubbe
        pass
        
    try:
        platform.build_rules[".c"]  = umake.BuildRule(
            ".c", ".%s" % (platform.object_suffix), platform.cc)
    except AttributeError: # I hate these - hubbe
        pass

    ## set the set_type function in the linker class; the project's target
    ## type MUST be set before this is done, and is only required for the
    ## old linker classes, which is why we don't do it for Linker2 derived
    ## linkers
    if not hasattr(platform.link, "linker2"):
        fun = None
        try:
            fun = platform.link.set_type
        except AttributeError:
            pass

        if fun:
            fun()


class SourceFile:
    """Given a the platform object, a source path, and a object output
    directory form a number of useful intermediate paths helping to deal
    with the source file."""
    
    def __init__(self, platform, path, output_dir):
        self.path = path
        (base, self.ext) = os.path.splitext(self.path)

        base = os.path.normpath(base)
        if os.path.isabs(base):
            base = string.replace(base,"\\","_")
            base = string.replace(base,":","_")
            base = string.replace(base,"/","_")
        else:
            ## Trim off any ../ from the beginning of the path
            base = string.replace(base,"\\","/")
            tmp = string.split(base, "/")
            x = 0
            while len(tmp) > x + 1:
                if tmp[x] in [ os.curdir, os.pardir ]:
                    x = x + 1
                else:
                    break
            if x:
                tmp = [ "par%d" % x ] + tmp[x:]
            base = string.join(tmp,os.sep)

        rules=platform.build_rules
        self.build_rule = rules.get(path,
                                    rules.get(os.path.basename(path),
                                              rules.get(self.ext, None)))

        if not self.build_rule:
            fatal('no build rule for extention="%s"' % (self.ext))
        
        self.obj_path = "%s%s" % (base, self.build_rule.target_suffix)
        if self.build_rule.output_dir != None:
            self.obj_path = os.path.join(self.build_rule.output_dir, self.obj_path)
        elif output_dir:
            self.obj_path = os.path.join(output_dir, self.obj_path)

            
    def getSourceFile( self ):
        """getSourceFile() --> string
        
        Returns the source file name of this sourcefile.
        """
        return self.path
        
        
    def getObjectFile( self ):
        """getObjectFile() --> string
        
        Returns the expected object file name of this sourcefile.
        """
        return self.obj_path
                
    def wantsProcessing( self ):
        """wantsProcess() --> boolean
        
        Does this sourcefile want to be processed?
        """
        return self.build_rule.wantsProcessing()
                
    def doesObjectExist( self ):
        """doesObjectExist()
        
        Does the object file already exist in the filesystem?
        """
        return os.path.isfile( self.obj_path )
        
        
    def doesSourceExist( self ):
        """doesSourceExist()
        
        Does the source file already exist in the filesystem?
        """
        return os.path.isfile( self.path )
        
        
## file = a path relative to the source root
## returns the module name
module_from_file_cache = {}

def module_from_file(file):
    if module_from_file_cache.has_key(file):
        return module_from_file_cache[file]

    module = file
    parts = []
    root = [ ":", ".", "", "..","/","\\"]
    while 1:
        tmp = os.path.split(module)
        parts.append(tmp[1])
        if tmp[0] in root:
            break
        root.append(tmp[0])
        module = tmp[0]

    parts.reverse()
    mparts = []
    while parts:
        module=string.join(parts, "/")
        id=bldreg.get("bifmodule_path_to_id",string.lower(module),None)
        if id:
            module_from_file_cache[file]=id
            return id
        parts=parts[:-1]
    return None


## file = a path relative to the current directory
## returns the module name
def module_from_file_rel(project, file):
    if os.path.isabs(file):
        return None
    
    module = file
    parts = []
    srcroot=os.path.normpath(project.src_root_path)
    while not module in [ ":", ".", "", ".."]:
        #print "DWIM: %s (%s)" % (repr(module), repr(srcroot))
        tmp = os.path.split(module)
        parts.append(tmp[1])
        if tmp[0] == srcroot:
            parts.reverse()
            return module_from_file(string.join(parts,os.sep))
        module = tmp[0]
    return None


def GetModuleDependencies(project, modname = None):
    try:
        return project.__recursive_dependency_check_done__
    except:
        if modname == None:
            module_directory = project.module_directory()
            modname = module_from_file(module_directory)
        if not modname:
            return {}
        done = { modname:1 }
        bifdeps = [ modname ]
        for d in bifdeps:
            for n in  bldreg.get("bifmodule_deplist",d,[]):
                if not done.has_key(n):
                    bifdeps.append(n)
                    done[n]=1

        project.__recursive_dependency_check_done__ = done
        #print "dependencies %s => %s" % (repr(modname), repr(done))
        return done


# Check each path in "paths" to make sure that they come from
# modules which are included in our BIF dependency list.
def CheckModuleDependencies_rel(platform, project, type, paths):
    # I feel nice, so we check aginainst the recursive tree of
    # dependencies rather than against just the dependencies in this
    # module... /Hubbe

    if project.getTargetType() == 'lib':
        return

    module_directory = project.module_directory()
    modname = module_from_file(module_directory)

    if not modname:
        return

    done = GetModuleDependencies(project, modname)

    warnings_printed = globals().get("__umake_warnings_printed_cache__",{})
        
    for path in paths:
        m = module_from_file_rel(project, path)
        if m:
            #print "Checking %s => %s (%s)" % ( repr(path), repr(m), repr(done.get(m,-1)))
            if not done.has_key(m):
                w = modname +":" + m
                if warnings_printed.get(w,0) < 2:
                    print "Warning: Missing BIF dep %s (%s %s)" % (repr(m), type, path)
                    warnings_printed[w]=2

                    ## Magic build resurrection code
                    import chaingang
                    c=chaingang.current_chaingang.Get()
                    if c:
                        if c.reschedule_by_id2(m):
                            print "Bonus super-secret slow-motion round activated!"

    globals()["__umake_warnings_printed_cache__"]=warnings_printed

# Check each path in "paths" to make sure that they come from
# modules which are included in our BIF source dependency list.
def CheckModuleSourceDependencies_rel(platform, project, type, paths):
    # I feel nice, so we check aginainst the recursive tree of
    # dependencies rather than against just the dependencies in this
    # module... /Hubbe

    module_directory = project.module_directory()
    modname = module_from_file(module_directory)

    if not modname:
        return

    try:
        done = project.__recursive_source_dependency_check_done__
    except:
        done = GetModuleDependencies(project, modname).copy()
        bifdeps = done.keys()

        for d in bifdeps:
            for n in bldreg.get("bifmodule_source_deplist",d,[]):
                if not done.has_key(n):
                    bifdeps.append(n)
                    done[n]=1

        project.__recursive_source_dependency_check_done__ = done

    warnings_printed = globals().get("__umake_warnings_printed_cache__",{})
        
    for path in paths:
        m = module_from_file_rel(project, path)
        if m:
            if not done.has_key(m):
                w = modname +":" + m
                if not warnings_printed.has_key(w):
                    print "Warning: Missing BIF source dep %s (%s %s)" % (repr(m), type, path)
                    warnings_printed[w]=1

    globals()["__umake_warnings_printed_cache__"]=warnings_printed

def fix_library_path(platform, project, path, target_type=None):

    if not target_type:
        target_type = "lib"
        
    path = string.strip(path)

    m = re.match(r'([^[]+)\[([^]]+)\]', path)
    
    if m:
        lib = m.group(2)
        dir = m.group(1)

        dir = fix_path(dir)

        ## UGLO
        force_odir=0
        force_common=0

        if lib[:2] == "./":
            force_odir=1
            lib=lib[2:]

        if lib[:3] == "../":
            force_common=1
            lib=lib[3:]

        if target_type == "lib":
            lname="%s%s.%s" % (platform.library_prefix,
                               lib,
                               platform.library_suffix)
        elif target_type == "dll":
            lname = platform.versioning.create_dll_name(lib)
        elif target_type == "exe":
            if len(platform.exe_suffix):
                lname = "%s.%s" % (output_name, platform.exe_suffix)
            else:
                lname = lib

        path = os.path.join(dir,
                            project.output_dir,
                            lname)

        if not force_odir:
            if force_common or not os.path.exists(path):
                tmppath = os.path.join(dir, lname)

                if os.path.exists(tmppath) or force_common:
                    path = tmppath

        return path
    
    return fix_path(path)

def SetupProject(platform, project):
    """Tweaks a Project object after its data has been modified by Umakefil
    execution.  Paths that may be in UNIX form are converted to local form,
    defines from the platform.defines list are added to the project
    defines, multiple defines are filtered out, and much, much more!"""

    ## include the module's own public directory if it's not already
    ## included from the Umakefil/*.pcf file
    if "./pub" not in project.includes:
        project.AddIncludes("./pub")
    
    includes = []
    for include in project.includes:
        include = string.strip(include)
        include = fix_path(include)
        include = fixabspath(project, include)
        if include in includes:
            pass
            # warning("removing duplicate include path=\"%s\"" % (include))
        else:
            includes.append(include)
    project.includes = includes

    ## This is a *source* dependency, not a build dependency
    CheckModuleSourceDependencies_rel(platform, project, "include path", includes)

    ## libraries
    libraries = []
    for lib in project.libraries:
        libraries.append( fix_library_path(platform, project, lib ) )
    project.libraries = libraries

    ## libraries2
    ## Bizarro, this seems to be a noop -Hubbe
    list = project.libraries2
    libraries2 = []
    for lib in list:
        include = string.strip(lib)
        include = fix_path(lib)
        libraries2.append(lib)
    project.libraries2 = libraries2

    CheckModuleDependencies_rel(platform, project, "library", libraries + libraries2)

    ## project.local_libs
    ## Another noop
    local_libs = []
    for lib in project.local_libs:
        include = string.strip(lib)
        include = fix_path(lib)
        local_libs.append(lib)
    project.local_libs = local_libs
    
    CheckModuleDependencies_rel(platform, project, "library", local_libs)

    ## defines
    defines = []
    for define in platform.defines + project.defines:
        define = string.strip(define)
        if define[:2] == "-D":
            define = define[2:]
        if define in defines:
            pass
            # warning("removing duplicate define=\"%s\"" % (define))
        else:
            defines.append(define)
    project.defines = defines
            
    ## sources
    sources = []
    for source in project.sources:
        source = string.strip(source)
        source = fix_path(source)
        if source in sources:
            warning("removing duplicate source file=\"%s\"" % (source))
        else:
            sources.append(source)
    project.sources = sources

    ## This is a *source* dependency, not a build dependency
    CheckModuleSourceDependencies_rel(platform, project, "source file", sources)

    # The beginning of a beautiful friendship. This is the first step of
    # refactoring this particular beast into smaller methods in the class(es)
    # they belong in. In this particular case, code was moved to the project
    # class, accessible by way of this public function. As more of SetupProject
    # finds its way to the project class, it can probably be moved into
    # the setup() method (or more likely, into methods that project.setup()
    # can call).
    #
    # Next question is, do we need to pass platform, or can it be / has it been
    # set in the project object already?
    project.setup( platform )
    
    ## the section below processes the libraries added in 
    ## project.module_libs, which are specified with a
    ## single name
    ##
    ## for this to work, the module_lib name needs to be
    ## both the name of the subdirectory(module), and for
    ## the library to be a static library with the same name
    ## with the platform's library suffix extention
    ##
    ## we generate the full paths to the libraries, and
    ## generate paths to the include directories for the
    ## modules

    ## setup project.module_libs
    module_libs = []
    for lib in project.module_libs:
        ## create the library name/path and add it to the project

        lib = string.strip(lib)
        lib = string.replace(lib,"\\","/")
        libparts = string.split(lib,"/")
        lib = libparts[-1]

        ## Magic, normally path/to/module becomes: $src/path/to/module/$out/module.lib
        ## However, path/to/module[l] becomes $src/path/to/module/$out/l.lib
        m = re.match(r'([^[]+)\[([^]]+)\]', lib)
        if m:
            libparts[-1]=m.group(1)
            lib = m.group(2)

        parts = [ project.src_root_path ] + \
                  libparts + \
                  [ project.output_dir,
                    "%s%s__%s.%s" % (platform.library_prefix,
                                     lib,
                                     string.join(libparts,"_"),
                                     platform.library_suffix) ]

        libpath = apply(os.path.join, parts)
        #print "TESTING libpath=%s" % libpath

        if not (os.path.exists(libpath) or os.path.islink(libpath)):
            parts = [ project.src_root_path ] + \
                      libparts + \
                      [ project.output_dir,
                        "%s%s.%s" % (platform.library_prefix,
                                     lib,
                                     platform.library_suffix) ]

            libpath = apply(os.path.join, parts)

            if not os.path.exists(libpath):
                parts = [ project.src_root_path ] + \
                        libparts + \
                        ["%s%s.%s" % (platform.library_prefix,
                                      lib,
                                      platform.library_suffix) ]
                tmppath = apply(os.path.join, parts)
                if os.path.exists(tmppath):
                    libpath = tmppath
            
        module_libs.append(libpath)

        ## add include paths for the library
        pub_path = apply(os.path.join, [project.src_root_path]+libparts+["pub"])

        debug(" srcroot = %s" % (repr(project.src_root_path)))
        debug(" libparts = %s" % (repr(libparts)))
        debug(" libpath = %s" % (repr(libpath)))

        debug("Checking pub path: %s (%d)" % (repr(pub_path), os.path.isdir(pub_path)))

        if os.path.isdir(pub_path) and pub_path not in project.includes:
            project.AddIncludes(pub_path)

        ## XXX: skipping "pub" is a hack to work around bad configurations -JMP
        if platform.inc_subdir and platform.inc_subdir != "pub":
            inc_subdir_path = os.path.join(pub_path, platform.inc_subdir)
            if os.path.isdir(inc_subdir_path) and \
               inc_subdir_path not in project.includes:
                project.AddIncludes(inc_subdir_path)
            
    project.module_libs = module_libs
    #print "checking module deps on: %s" % repr(module_libs)
    CheckModuleDependencies_rel(platform, project, "module library", module_libs)

    ## for a all-static target, the _STATICALLY_LINKED define is
    ## required
    if project.BuildOption("nodll"):
        project.AddDefines("_STATICALLY_LINKED")

    ## This is an ugly ugly ugly backwards-compatible hack
    ## It allows for pre-build2002 umakefiles to be located
    ## in subdirectories with (more or less) the old semantics.
    ## -Hubbe
    if project.module_depth == 1:
        project.makefile_name = os.path.basename(project.makefile_name)

        
class Targets:
    """Base class of all Umake target types.  This class implements only
    the most abstract parts of the target methods.  The real implementations
    are in the specific platform handling code."""

    def __init__(self, platform, project):
        self.platform = platform
        self.project = project

    def ProgramTarget(self, target):
        fatal("ProgramTarget unimplemented")

    def ProgramWithResourceTarget(self, target, rtarget, rfile, includes):
        fatal("ProgramWithResourceTarget unimplemented")

    def LibraryTarget(self, libname):
        fatal("LibraryTarget unimplemented")

    def DLLTarget(self, target):
        fatal("DLLTarget unimplemented")

    def DLLWithResourceTarget(self, target, rtarget, rfile, includes):
        fatal("DLLWithResourceTarget unimplemented")

    def AllTarget(self, depends):
        fatal("AllTarget unimplemented")

    def DependTarget(self):
        fatal("DependTarget unimplemented")

    def CopyTarget(self, target_list):
        fatal("CopyTarget unimplemented")

    def EmptyTarget(self):
        fatal("EmptyTargets unimplemented")


def mtime(filename):
    try:
        ret=os.stat(filename)[stat.ST_MTIME]
    except:
        ret=0

    return ret

def declaw_name(name):
    name = os.path.normpath(name)
    name = string.lower(name)
    name = string.replace(name,"-","_")
    name = string.replace(name,"/","_")
    name = string.replace(name,"\\","_")
    name = string.replace(name,".mak","")
    name = string.replace(name,"_makefile","")
    return name


def write_file(name, data):
    if os.path.isfile(name):
        if open(name,"r").read() == data:
            return

    #print "WRITING FILE %s" % name
    open(name, "w").write(data)


## FIXME: Move these to shell.py

if sysinfo.host_type == "win32":
    def split_arguments(line):
        quoted=0
        ret=[]
        arg=[]
        q={' ':1, '\r':1, '\t':1,'\012':1, '"' : 2}
        #print line
        #print re.split( r'((?:(?:\\\\)*")|[ \r\n\012]*)',line)
        for part in re.split( r'((?:(?:\\\\)*")|[ \r\n\012]*)',line):
            if not len(part):
                continue
            t=q.get(part[-1],0)
            if t == 1:
                if quoted:
                    arg.append(part)
                else:
                    ret.append(string.join(arg,""))
                    arg=[]
            elif t == 2:
                arg.append("\\" * (len(part)/2))
                quoted=1-quoted
            else:
                arg.append(part)

        if arg:
            ret.append(string.join(arg,""))

        return ret

    def do_quote(str):
        if re.match('^[^ "]*$', str):
            return str

        str = re.sub(r'(\\*)"', r'\1\1\"', str + '"')
        return '"%s"' % str[:-2]

else:

    def split_arguments(line):
        line=string.strip(line)
        line=re.sub(line,"['\"\\ \t\n\r]","\0\\1")
        x=string.split(line,"\0")

        ret=x[:1]
        e=0
        while e<len(x):
            c=x[e][0]
            if c == '"':
                ret[-1]=ret[-1]+x[e][1:]
                e=e+1
                while x[e][0] != '"':
                    if len(x[e]) == 1 and x[e][0]=='\\' and x[e+1][0]=='"':
                        e=e+1
                    ret[-1]=ret[-1]+x[e][1:]
                    e=e+1
                ret[-1]=ret[-1]+x[e][1:]

            elif c == "'":
                ret[-1]=ret[-1]+x[e][1:]
                e=e+1
                while x[e][0] != "'":
                    ret[-1]=ret[-1]+x[e]
                    e=e+1
                ret[-1]=ret[-1]+x[e][1:]

            elif c in [' ', '\t', '\n','\r']:
                while len(x[e]) == 1:
                    if e+1 < len(x):
                        if x[e+1][0] in [' ','\t','\n','\r']:
                            e=e+1
                        else:
                            break
                    else:
                        return ret
                ret.append(x[e][1:])

            else:
                ret[-1]=ret[-1]+"\0"+x[e]

        
        return ret

    def do_quote(str):
        if re.match('^[-a-zA-Z0-9_+=.,/@]*$', str):
            return str
        
        str = re.sub(r'([^- a-zA-Z0-9_+=.,/@])', r'\\\1', str)
        return '"' + str + '"'

def build_quoted_arg_list(args, prefix=""):
    if len(args) == 0:
        return ""

    tmp=[]
    if len(prefix) and prefix[-1] == ' ':
        l=string.strip(prefix)
        for n in args:
            tmp.append(l)
            tmp.append(n)
    else:
        for n in args:
            tmp.append(prefix+n)

    return string.join(map(do_quote, tmp)," ")
    
