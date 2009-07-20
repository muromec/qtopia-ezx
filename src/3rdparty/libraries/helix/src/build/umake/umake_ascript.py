# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_ascript.py,v 1.10 2006/06/19 23:11:32 jfinnecy Exp $ 
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
"""This is the Mactinosh/AppleScript/CodeWarrior back end for Umake.  It
generates a AppleScript file which, when run, tries its best to emulate
a standard Makefile.  It uses AppleScript to generate a CodeWarrior project,
and also emits copy commands (using the AppleScript extentions in AkauSweets)
to copy the targets once built."""

import os
import sys
import string
import re
import time
import types
import posixpath
import macfs
import ascript
import umake_lib
import macpath

import log
log.debug( 'Imported: $Id: umake_ascript.py,v 1.10 2006/06/19 23:11:32 jfinnecy Exp $' )

def condense_mac_path(_path):
    """Remove skipped directories from a macintosh path, because two
    path seperators in a row means to backstep in Macintosh path speak."""

    plist = string.split(_path, ":")
    path_list = []

    for pc in plist:
        if pc == "":
            path_list = path_list[:-1]
        else:
            path_list.append(pc)

    return string.join(path_list, ":")


def extract_ascw_path(_as_path, project):
    """extract_ascw_path takes a path string in AppleScript Codewarrior
    form and extracts the data from it, returning a 3-tuple of strings:

    Input String:
      '{name:":Win32-x86 Support:", recursive:true, origin:shell relative}'
    Returns Tuple:
      (":Win32-x86 Support:", "true", "shell relative")"""

    _as_path = string.strip(_as_path)
    if len(_as_path) == 0:
        umake_lib.fatal("extract_ascw_path() called with empty path")
    if _as_path[0] != "{" or _as_path[-1] != "}":
        umake_lib.fatal(
            "extract_ascw_path() called with invalid path=\"%s\"" % (_as_path))

    _as_path = _as_path[1:-1]
    list = string.split(_as_path, ",")

    name = ""
    recursive = ""
    origin = ""

    for item in list:
        i = string.find(item, ":")
        key = string.lower(string.strip(item[:i]))
        value = string.strip(item[i+1:])

        if key == "name":
            name = value[1:-1]
            if name[:7] == "SRCROOT":
                name = os.path.join(project.src_root_path, name[7:])
        elif key == "recursive":
            recursive = value
        elif key == "origin":
            origin = value
        else:
            umake_lib.fatal(
                "extract_ascw_path: unhandled field=\"%s\"" % (key))

    return (name, recursive, origin)


def ProjectToMacCWProjectData(platform, project):
    """Takes a Platform and Project class, defined in umake.py, and
    creates a MacCWProjectData class from them.  The MacCWProjectData class
    is then fed to the CodeWarrior AppleScript generator.  Data from the
    Project and Platform classes are munged in various ways in this function.
    There are many "make it work" hacks here."""

    mprj = MacCWProjectData()
    mprj.platform = platform
    mprj.project = project

    mprj.target_name = project.target_name
    mprj.target_type = project.getTargetType()

    mprj.define_list = project.defines[:]
    mprj.prefix_file_include_list = project.prefix_file_include_list[:]

    ## setup paths/file names
    mprj.project_file = "%s.prj" % (mprj.target_name)
    mprj.project_file_path = os.path.join(os.getcwd(), mprj.project_file)

    ## project data foldername/folder path
    mprj.project_data = "%s Data" % (mprj.target_name)

    ## prefix file filename/path
    mprj.prefix_file = "%s_prefix.h" % (mprj.target_name)
    mprj.prefix_file_path = mprj.prefix_file
    mprj.rprefix_file = "r%s_prefix.r" % (mprj.target_name)
    mprj.rprefix_file_path = mprj.rprefix_file

    ## resource targets
    if project.with_resource_flag:
        mprj.rtarget = "%s.%s" % (
            project.resource_target, platform.resource_dll_suffix)
        mprj.rfile = project.resourcefile

        ## resource project filename/path
        mprj.rproject_file = "r%s.prj" % (mprj.target_name)
        mprj.rproject_file_path = os.path.join(os.getcwd(), mprj.rproject_file)

        ## resource project data foldername/folder path
        mprj.rproject_data = "r%s Data" % (mprj.target_name)

    ## output foldername/folder path
    mprj.output_dir = project.output_dir
    mprj.output_dir_path = condense_mac_path(
        os.path.join(os.getcwd(), mprj.output_dir))

    ## target dir foldername/folder path
    mprj.target_dir = project.target_dir
    mprj.target_dir_path = condense_mac_path(
        os.path.join(os.getcwd(), mprj.target_dir))

    ## copy over the "preferences" nested hash from the project
    for (panel, pref) in project.preferences.items():
        ## skip copying some of the panels which are handled
        ## seperately
        if panel == "Access Paths":
            for (key, value) in pref.items():
                key = string.lower(key)
                if key == "always full search":
                    mprj.always_full_search = (value == "true")

        try:
            temp = mprj.preferences[panel]
        except KeyError:
            temp = mprj.preferences[panel] = {}

        for (pref_key, pref_value) in pref.items():
            temp[pref_key] = pref_value

    ## includes are processed at the end of this, but they are
    ## accumeulated here first
    include_path_list = []

    ## create soruce_list from project.sources, adding the source
    ## path (if any) to the user access path list
    mprj.source_list = []
    for source in project.sources:
        source_path, source_name = os.path.split(source)
        mprj.source_list.append(source_name)
        if source_path and source_path not in include_path_list:
            include_path_list.append(source_path)

    ## add libraries to sources
    ## we have to add the libraries to mprj.source_list by splitting
    ## any path away (if there is a path) and adding it to the includes
    ## list, which ends up in the "Access Paths->User Paths" panel
    library_list = project.libraries + project.libraries2 + \
                   project.local_libs + project.dynamic_libraries + \
                   project.sys_libraries

    ## XXX: don't include the module libraries for static libraries
    ##      this is a hack; normally, we don't link any libraries into
    ##      static libs; on the Macintosh, dynamic library links to the
    ##      static library are inherited by whatever program or shared
    ##      library links in the static lib, and programmers have used
    ##      this feature in our code base to avoid listing all the
    ##      libraries programs/dll's link to in their Makefiles... -JMP
    if mprj.target_type != "lib":
        library_list = project.module_libs + library_list

    for library in library_list:
        lib_path, lib_name = os.path.split(library)

        ## only add to the weak link list if the library was added
        if lib_name in project.weak_link_list:
            mprj.weak_link_list.append(lib_name)

        if lib_name not in mprj.source_list:
            mprj.source_list.append(lib_name)
        if lib_path and lib_path not in include_path_list:
            include_path_list.append(lib_path)

    ## Access Paths (System Paths/User Paths)
    for path in platform.system_paths + project.system_paths:
        mprj.system_paths.append(extract_ascw_path(path,project))

    for path in platform.user_paths:
        mprj.user_paths.append(extract_ascw_path(path,project))

    ## include this for the path to the XRS(resource) dll
    ## XXX: this should be moved -JMP
    mprj.user_paths.append( (mprj.output_dir, "false", "project relative") )

    ## mix in source/lib/project.includes here
    ## drop non-unique paths
    temp_list = project.includes + include_path_list
    include_path_list = []
    for include in temp_list:
        if include not in include_path_list:
            include_path_list.append(include)

    for include in include_path_list:
        if include[-1] != ":":
            include = "%s:" % (include)

        mprj.user_paths.append( (include, "false", "project relative") )

    ## Resource Access Paths
    for path in platform.rsystem_paths:
        mprj.rsystem_paths.append(extract_ascw_path(path,project))

    for path in platform.ruser_paths:
        mprj.ruser_paths.append(extract_ascw_path(path,project))

    for include in project.resourceincludes:
        if include[-1] != ":":
            include = "%s:" % (include)

        if os.path.isdir(include):
            mprj.ruser_paths.append( (include, "false", "project relative") )
        else:
            umake_lib.warning(
                "dropping non-existant include path=\"%s\"" % (include))

    
    ## export file
    mprj.export_file = ""
    if len(project.exported_func):
        mprj.export_file = "%s.exp" % (mprj.target_name)
        mprj.export_list = project.exported_func
        if mprj.export_file not in mprj.source_list:
            mprj.source_list.append(mprj.export_file)

    ## customize the "PPC Project", "PPC PEF" panel, setting
    ## target output and type
    ppc_project = mprj.preferences["PPC Project"]
    ppc_pef = mprj.preferences["PPC PEF"]

    ## warnings
    if ppc_project["Project Type"] != "xxxProjType":
        umake_lib.warning('panel="Project Type" modified to="%s"' % (
            ppc_project["Project Type"]))

    if ppc_project["File Name"] != "xxxFileName":
        umake_lib.warning('panel="File Name" modified to="%s"' % (
            ppc_project["File Name"]))
        mprj.output_name = ppc_project["File Name"][1:-1]
    else:
        mprj.output_name = project.OutputName()

    if ppc_project["File Type"] != "xxxFileType":
        umake_lib.warning('panel="File Type" modified to="%s"' % (
            ppc_project["File Type"]))

    if ppc_pef["Fragment Name"] != "xxxFragmentName":
        umake_lib.warning('panel="Fragment Name" modified to="%s"' % (
            ppc_pef["Fragment Name"]))

    ## set target name
    ppc_project["File Name"] = '"%s"' % (mprj.output_name)
    ppc_pef["Fragment Name"] = ppc_project["File Name"]

    ## targe type/output file type
    if mprj.target_type == "lib":
        ppc_project["Project Type"] = "library"
        ## only set the filetype to 'shlb' if it has not been specified
        if ppc_project["File Type"] == "xxxFileType":
            ppc_project["File Type"] = '"????"'
    elif mprj.target_type == "exe":
        ppc_project["Project Type"] = "standard application"
        ## only set the filetype to 'shlb' if it has not been specified
        if ppc_project["File Type"] == "xxxFileType":
            ppc_project["File Type"] = '"APPL"'
    elif mprj.target_type == "dll":
        ppc_project["Project Type"] = "shared library"
        ## only set the filetype to 'shlb' if it has not been specified
        if ppc_project["File Type"] == "xxxFileType":
            ppc_project["File Type"] = '"shlb"'

    ## tweak the PPC Linker settings
    ppc_linker = mprj.preferences["PPC Linker"]

    if mprj.target_type == "lib" or mprj.target_type == "dll":
        if not ppc_linker.has_key("Initialization Name"):
            ppc_linker["Initialization Name"] = '"__initialize"'
        if not ppc_linker.has_key("Termination Name"):
            ppc_linker["Termination Name"] = '"__terminate"'
        if not ppc_linker.has_key("Main Name"):
            ppc_linker["Main Name"] = '""'
    elif mprj.target_type == "exe":
        if not ppc_linker.has_key("Initialization Name"):
            ppc_linker["Initialization Name"] = '""'
        if not ppc_linker.has_key("Termination Name"):
            ppc_linker["Termination Name"] = '""'
        if not ppc_linker.has_key("Main Name"):
            ppc_linker["Main Name"] = '"__start"'

    ## if the target type is a DLL, check for a export file in the
    ## listed sources
    ##
    ## XXX: umake should generate a .exp file from the
    ##      project.exported_functions list, like it does for
    ##      Windows and Unix -JMP  It now does! - CXZ
    if mprj.target_type == "dll":
        use_export_file = 0

        for source in mprj.source_list:
            if string.lower(source[-4:]) == ".exp":
                use_export_file = 1
                break

        if use_export_file:
            ppc_pef["Export Symbols"] = "expfile"

    return mprj


class MacCWProjectData:
    """Specialized class for holding the abstract, post-tweaked project
    data for being fed to the CodeWarrior AppleScript generator."""

    def __init__(self):
        self.target_name = ""
        self.target_type = ""
        
        self.source_list = []
        self.library_list = []
        self.define_list = []
        self.weak_link_list = []
        self.prefix_file_include_list = []
        self.preferences = {}
        self.ide_path = os.environ["BUILD_SW"]

        ## Access Paths: list of tuples (path, recursive, origin)
        self.user_paths = []
        self.system_paths = []
        self.ruser_paths = []
        self.rsystem_paths = []
        
        ## flag to indicate to full search user_paths, defaults off
        self.always_full_search = 0
        
        ## project file filename/path
        self.project_file = ""
        self.project_file_path = ""

        ## project data foldername/folder path
        self.project_data = ""
        self.project_data_path = ""

        ## prefix file filename/path
        self.prefix_file = ""
        self.prefix_file_path = ""

        ## resource targets
        self.rtarget = ""
        self.rfile = ""

        ## resource project filename/path
        self.rproject_file = ""
        self.rproject_file_path = ""

        ## resource project data foldername/folder path
        self.rproject_data = ""
        self.rproject_data_path = ""

        ## resource prefix file filename/path
        self.rprefix_file = ""
        self.rprefix_file_path = ""

        ## output foldername/folder path
        self.output_dir = ""
        self.output_dir_path = ""

        ## target dir foldername/folder path
        self.target_dir = ""
        self.target_dir_path = ""
        
        ## CodeWarrior target names - may be different from target names in multi-target modules
        self.cwtarget_name = ""

        ## arbitrary applescript run after the build
        self.post_build_script = ""
        
        self.project_dir = ""


def WriteResourcePrefixFile(mprj):
    """Write the prefix header file for resource projects."""

    line_list = [
        "/* Resource Prefix File Generated by Umake */",
        "#include <types.r>",
        "#define _MACINTOSH",
        ]


    data=string.join(line_list,"\n") + "\n"

    umake_lib.write_file(mprj.rprefix_file_path, data)

    fsspec = macfs.FSSpec(mprj.rprefix_file_path)
    fsspec.SetCreatorType('CWIE', 'TEXT')


def WritePrefixFile(mprj):
    """Write the prefix header file for projects.  Prefix files on the
    Macintosh are, by default, always included in every source file before
    any other header file.  Therefore, it is a useful place to set up
    #defines, since the Mac doesn't support doing them any other way."""

    line_list = [
        "/* Prefix File Generated by Umake */",
        ]

    ## get all the defines into one list
    line_list.append("/* begin defines */")

    for define in mprj.define_list:
        index = string.find(define, '=')

        ## define without value, just set to true
        if index == -1:
            line_list.append("#define %s 1" % (define))
            continue

        ## define with value
        key = string.strip(define[:index])
        value = string.strip(define[index+1:])
        line_list.append('#define %s %s' % (key, value))

    line_list.append("/* end defines */")

    ## PPC vs. 68k, very legacy...
    line_list = line_list + [
        "#ifdef __POWERPC__",
        "        #define _MACPPC 1",
        "#else",
        "        #define _MAC68K 1",
        "#endif /* __POWERPC__ */",
        ]

    ## included in everything
    for include in mprj.prefix_file_include_list:
        line_list.append("#include <%s>" % (include))

    ## write the file and set creator/type
    data=string.join(line_list,"\n") + "\n"

    umake_lib.write_file(mprj.prefix_file_path, data)

    fsspec = macfs.FSSpec(mprj.prefix_file_path)
    fsspec.SetCreatorType('CWIE', 'TEXT')


class CWAccessPath:
    """Stores all the information for setting the Access Paths panel in
    CodeWarrior.  CodeWarrior projects don't differentiate between include
    paths and library search paths, so we put them all in here.  Also,
    each path has to be specified if it is to be searched recursivly,
    and where the path origin should be: project relative, or shell (compiler)
    relative, or a system framework path"""

    def __init__(self):
        self.system_paths = []
        self.user_paths = []
        self.settings = {}

    def in_paths_list(self, paths_list, path, recursive, origin):
        p1 = (path, recursive, origin)
        for p2 in paths_list:
            if p1 == p2:
                return 1
        return 0

    def add_path(self, list, path, recursive, origin):
        recursive = string.lower(recursive)
        if recursive not in ["true", "false"]:
            umake_lib.fatal(
                'CWAccessPath: recursive must be "true" or "false"')

        origin = string.lower(origin)
        if origin not in ["project relative", "shell relative"]:
            umake_lib.fatal(
                'CWAccessPath: origin must be "project relative" or '\
                '"shell relative"')

        list.append( (path, recursive, origin) )

    def AddSetting(self, key, value):
        self.settings[key] = value

    def AddSystemPath(self, path, recursive, origin):
        if not self.in_paths_list(self.system_paths, path, recursive, origin):
            self.add_path(self.system_paths, path, recursive, origin)

    def AddUserPath(self, path, recursive, origin):
        if not self.in_paths_list(self.user_paths, path, recursive, origin):
            self.add_path(self.user_paths, path, recursive, origin)

    def String(self):
        def cw_path(p, r, o):
            return '{name:"%s",recursive:%s,origin:%s}' % (p,r,o)

        lst = []
        
        sys_list = []
        for (path, recursive, origin) in self.system_paths:
            sys_list.append(cw_path(path, recursive, origin))
        lst.append("System Paths:{%s}" % (string.join(sys_list, ",")))

        user_list = []
        for (path, recursive, origin) in self.user_paths:
            user_list.append(cw_path(path, recursive, origin))
        lst.append("User Paths:{%s}" % (string.join(user_list, ",")))

        for (key, value) in self.settings.items():
            lst.append("%s:%s" % (key, value))

        return "{%s}" % (string.join(lst, ","))

        
def WriteExportFile(mprj):
    ## write the file and set creator/type
    if len(mprj.export_file):
        fil = open(mprj.export_file, 'w')
        for exported_func in mprj.export_list:
            fil.write("%s\n" % (exported_func))
        fil.close()

        fsspec = macfs.FSSpec(mprj.export_file)
        fsspec.SetCreatorType('CWIE', 'TEXT')


class ASMakefile:

    def __init__(self, mprj):
        self.mprj = mprj
        ## start the script and define subroutines
        script = ascript.CreateAppleScript()
        self.script = script
        all =  []
        self.all = all

        for sumake in mprj.project.submakes:
            m_path = macpath.normpath(macpath.join(os.getcwd(), sumake.makefile()))
            all.extend( [
                'set scriptobj to (load script file "%s")' % (m_path),
                'tell scriptobj',
                '  set myerrors to myerrors & return & all()',
                'end tell' ] )

        script.Extend(mprj.project.pre_target_buff)

        self.Clean()


        if mprj.project.getTargetType() != "":

            WritePrefixFile(mprj)
            WriteResourcePrefixFile(mprj)    
            WriteExportFile(mprj)

            self.VerifyPath()
            self.ExtractCWErrors()    

            if len(mprj.weak_link_list):
                self.SetWeakLink()

            if len(mprj.post_build_script):
                self.PostBuildScript()

            if mprj.rtarget:
                self.DefineResourceProject()

            ## FIXME, only do this for if asked to 'clean' build
            all.extend( [
                '  --clean out old project and data by default',
                '  Clean()' ])

            ## write the "all" function, like "make all" in a Makefile
            all.extend( [
                '  --make the output directory, and target directory',
                '  verifypath("%s")' % (mprj.output_dir_path),
                '  verifypath("%s")' % (mprj.target_dir_path) ] )

            if mprj.rtarget:
                all.extend( [
                    '--build the windows resource dll',
                    'myerrors = myerrors & return & ResourceProject()' ] )


            self.RunProject()

            ## for DLL target types, we make a alias to the dll with a .lib
            ## extention so developers can do implicit linking to DLL's the
            ## same way they do on Windows
            ##
            ## Note: when Windows compiles a DLL, it creates a companion .LIB
            ##       library with the same name; you link to the DLL by linking
            ##       in the stub .LIB
            absolute_output_dir = os.path.join(os.getcwd(), mprj.output_dir)
            absolute_output_path = os.path.join(absolute_output_dir, mprj.output_name)

            if mprj.target_type == "dll":

                if mprj.project.opt_target_name:
                    alias_name= mprj.project.opt_target_name
                else:
                    alias_name= mprj.target_name

                alias_name = "%s.LIB" % (string.upper(alias_name))
                absolute_alias_path = os.path.join(absolute_output_dir, alias_name)

                all.extend( [
                    'tell application "Finder"',
                    '  try',
                    '    if (file "%s") exists then' % (absolute_alias_path),
                    '    else',
                    '      make new alias file to (file "%s") at (folder "%s") '\
                    '       with properties {name:"%s"}' % (
                            absolute_output_path, absolute_output_dir, alias_name),
                    '    end if',
                    '   on error',
                    '  end try',
                    'end tell' ])

            if len(mprj.post_build_script):
                all.extend( [
                    '--execute the custom subroutine',
                    'DoPostBuild()' ])

            ## copy the built target and finish off the script
            all.extend( [
                '-- Copy results to common output folder',
                'tell application "Finder"',
                '  if (file "%s") exists then' % (absolute_output_path),
                '      Duplicate file "%s" to folder "%s" with replacing' % (
                    absolute_output_path, mprj.target_dir_path),
                '  end if',
                'end tell' ])

        if len(all):
            script.Append('on all()',
                          'set myerrors to ""')
            script.Extend(all)
            script.Append(
                'return myerrors',
                'end all',
                '-- run the "all()" function',
                'return all()')


        script.Extend(mprj.project.post_target_buff)

        ## for DRM signing
        if mprj.project.getTargetType() == "dll" and \
               mprj.project.BuildOption("drmsign") and \
               mprj.project.CheckDRMSign():
            import shell
            shell.mkdir(os.path.dirname(absolute_output_path))
            open(absolute_output_path+"--drmsign","w").write("signme!")

    def RunProject(self):
        mprj = self.mprj
        script = self.script
        all = self.all

        all.extend([
            '  --build the main CodeWarrior Project',
            '  tell application %s' % (mprj.ide_path),
            '    Launch',
            '    with timeout of 99999 seconds',
            '      try',
            '        --open project or create a new one',
            '        set projectName to "%s"' % (mprj.project_file_path),
            '        try',
            '          open "%s"' % (mprj.project_file_path),
            '        on error number errnum',
            '          if errnum is -43 then -- Project not found',
            '            Create Project "%s"' % (mprj.project_file_path),
            '          end if',
            '        end try',
            '        --set prefix files',
            '        set preferences of panel "C/C++ Compiler" to '\
                         '{Prefix File:"%s"}' % (mprj.prefix_file),
            '        set preferences of panel "Rez Compiler" to '\
                         '{RezPrefix File:"%s"}' % (mprj.rprefix_file),
            '        --set preferences'])

        ## set misc preferences
        for (panel, pref) in mprj.preferences.items():
            list = []
            for (pref_key, pref_value) in pref.items():
                list.append("%s:%s" % (pref_key, pref_value))

            all.extend(['set preferences of panel "%s" to {%s}' % (
                panel, string.join(list, ","))])

        ## set access paths
        cwap = CWAccessPath()
        if mprj.always_full_search:
            cwap.AddSetting("Always Full Search", "true")
        else:
            cwap.AddSetting("Always Full Search", "false")
        cwap.AddSetting("Convert Paths", "true")

        for (path, recursive, origin) in mprj.system_paths:
            cwap.AddSystemPath(path, recursive, origin)

        for (path, recursive, origin) in mprj.user_paths:
            cwap.AddUserPath(path, recursive, origin)

        all.extend([
            'Set Preferences of panel "Access Paths" to {User Paths:{},System Paths:{},Always Full Search:false}',
            'set preferences of panel "Access Paths" to %s' % (cwap.String())])

        ## add source files
        all.extend([
            '--add source files',
            'Add Files {"%s"}' % (string.join(mprj.source_list, '","'))])

        ## list of system/misc libraries which need to be "weak linked"
        for library in mprj.weak_link_list:
            all.extend(['my SetWeakLink("%s")' % (library)])

        all.extend([
            '        set cwErrorList to Make Project with ExternalEditor',
            '        set myerrors to my ExtractCWErrors(cwErrorList)',
            '        Close Project',
            '        on error errText number errnum',
            '        try',
            '          Close Project',
            '        end try',
            '        --rethrow the error',
            '        return errText & return',
            '      end try',
            '    end timeout',
            '  end tell' ])


    def VerifyPath(self):
        self.script.Append(
            '-- VerifyPath creates the path up to the specified folder if it doesnt already exist',
            '-- example usage: VerifyPath("Hard Disk:A:B")   where B is the target folder that should exist',
            'on VerifyPath(thePath)',
            '       -- first save the old text item delimiter and use ":" as the text item delimiter',
            '       set savedTextItemDelimiters to AppleScript\'s text item delimiters',
            '       set AppleScript\'s text item delimiters to ":"',
            '       ',
            '       set numPathItems to (count of text items of thePath)',
            '       if numPathItems > 1 then',
            '               set previousPartialPath to text item 1 of thePath -- previous path starts as the disk name',
            '               repeat with n from 2 to numPathItems',
            '                       -- make a partial path through the nth item',
            '                       set partialPath to (text items 1 through n of thePath) as string',
            '                       tell application "Finder"',
            '                               if not (exists (folder partialPath)) then',
            '                                       if n is 2 then',
            '                                               make new folder at disk previousPartialPath with properties {name:(text item n of thePath)}',
            '                                       else',
            '                                               make new folder at folder previousPartialPath with properties {name:(text item n of thePath)}',
            '                                       end if',
            '                               end if',
            '                       end tell',
            '                       set previousPartialPath to partialPath',
            '               end repeat',
            'end if',
            '       -- restore the text item delimiter',
            '       set AppleScript\'s text item delimiters to savedTextItemDelimiters',
            'end VerifyPath'
            )


    def SetWeakLink(self):
        """Emits a AppleScript function which can be called to set the Weak Link
        attribute for a dynamic library linked to the target."""

        self.script.Append(
            '-- subroutine for setting weak linking on a ProjectFile',
            'on SetWeakLink(projectFileName)',
            '  tell application %s' % (self.mprj.ide_path),
            '    set segs to Get Segments',
            '    set segNum to 0',
            '    repeat with thisSeg in segs',
            '      set segNum to segNum + 1',
            '      set numFiles to filecount of thisSeg',
            '      repeat with fileNum from 1 to numFiles',
            '        set the currentFile to (Get Project File fileNum Segment segNum)',
            '        set currentFileName to the name of currentFile',
            '        if (name of currentFile) = projectFileName then',
            '          -- set weak link on currentFile',
            '          set %sclass Weak%s of currentFile to true' % (chr(199), chr(200)), # XXX: CW2 BUG!!!
            '          Set Project File currentFileName to currentFile',
            '        end if',
            '      end repeat',
            '    end repeat',
            '  end tell',
            'end SetWeakLink')

    def Clean(self):
        """Emits a AppleScript function to clean the target, project file, and
        project file data for a target."""

        self.script.Append(
            '-- Allow the items to be made clean',
            'on Clean()',
            )

        for sumake in self.mprj.project.submakes:
            m_path = macpath.normpath(macpath.join(os.getcwd(), sumake.makefile()))
            self.script.Append(
                '  set scriptobj to (load script file "%s")' % (m_path),
                '  tell scriptobj',
                '    Clean()',
                '  end tell' )

        if self.mprj.project.getTargetType() != "":
            self.script.Append(
                '  tell application "Finder"',
                '    with timeout of 99999 seconds',
                '      if file "%s" exists then' % (self.mprj.project_file_path),
                '        delete file "%s"' % (self.mprj.project_file_path),
                '      end if',
                '      if folder "%s" exists then' % (self.mprj.project_data_path),
                '        delete folder "%s"' % (self.mprj.project_data_path),
                '      end if')

            if self.mprj.rtarget:
                self.script.Append(
                    '      if file "%s" exists then' % (self.mprj.rproject_file_path),
                    '        delete file "%s"' % (self.mprj.rproject_file_path),
                    '      end if',
                    '      if folder "%s" exists then' % (self.mprj.rproject_data_path),
                    '        delete folder "%s"' % (self.mprj.rproject_data_path),
                    '      end if')

            self.script.Append(
                '    end timeout',
                '  end tell')

        self.script.Append(
            'end Clean',
            '')


    def PostBuildScript(self):
        self.script.Append('on DoPostBuild()')
        ## we must split the buffer up and get the newlines out
        self.script.Extend(string.split(mprj.post_build_script, '\n'))
        self.script.Append('end DoPostBuild')

    def DefineResourceProject(self):
        ## assemble path lists
        mprj = self.mprj
        cwap = CWAccessPath()
        if mprj.always_full_search:
            cwap.AddSetting("Always Full Search", "true")
        else:
            cwap.AddSetting("Always Full Search", "false")
        cwap.AddSetting("Convert Paths", "true")

        for (name, recursive, origin) in mprj.rsystem_paths:
            cwap.AddSystemPath(name, recursive, origin)

        for (name, recursive, origin) in mprj.ruser_paths:
            cwap.AddUserPath(name, recursive, origin)

        self.script.Append(
            'on ResourceProject()',
            'tell application %s' % (mprj.ide_path),
            'Launch',
            'try',
            'open "%s"' % (mprj.rproject_file_path),
            'on error number errnum',
            'if errnum is -43 then -- Project not found',
            'Create Project "%s"' % (mprj.rproject_file_path),
            'end if',
            'end try',
            '--set preferences',
            'set preferences of panel "Target Settings" to '\
                '{Linker:"Win32 x86 Linker",'\
                'Target Name:"%s",'\
                'Output Directory Path:"%s",'\
                'Output Directory Origin:project relative}' % (
                mprj.rtarget, mprj.output_dir),
            'set preferences of panel "C/C++ Compiler" to '\
                '{Prefix File:"ansi_prefix.win32.h"}',
            'set preferences of panel "x86 Project" to '\
                '{Project Type:shared library, File Name:"%s"}' % (mprj.rtarget),
            'set preferences of panel "x86 Linker" to {Entry Point Usage:none}',
            'Set Preferences of panel "Access Paths" to {User Paths:{},System Paths:{},Always Full Search:false}',
            'set preferences of panel "Access Paths" to %s' % (cwap.String()),
            'set preferences of panel "WinRC Compiler" to '\
                '{Prefix File:"ResourcePrefix.h"}',
            'try',
            'Add Files {"%s"}' % (mprj.rfile),
            'on error',
            'end try',
            'set cwErrorList to Make Project with ExternalEditor',
            'set myerrors to my ExtractCWErrors(cwErrorList)',
            'Close Project',
            'end tell',
            'return myerrors',
            'end ResourceProject')


    def ExtractCWErrors(self):
        """Emits a AppleScript function useful in extracting errors from
        CodeWarrior, so they may be returned as a string."""


        self.script.Append(
            '(*',
            ' * This subroutine extracts the contents of CodeWarrior\'s',
            ' * Error and Warnings',
            ' * window, and returns them as a text string',
            ' *)',
            'on ExtractCWErrors(errorList)',
            '  set logText to ""',
            '  -- Parse each error and convert to string',
            '  set numWarnings to 0',
            '  repeat with errInfo in errorList',
            '    tell application %s' % (self.mprj.ide_path),
            '          set errType to messagekind of errInfo',
            '          set errText to message of errInfo as string',
            '          set errTypeString to ">>unknown error type<< "',
            '          if errType is compiler error then',
            '            set errTypeString to "Compiler Error "',
            '            set errFile to file of errInfo as string',
            '            set errLine to lineNumber of errInfo as string',
            '            set logText to logText & errTypeString & "in " & errFile & " (line " & errLine & "): " & errText & return',
            '          else if errType is compiler warning then',
            '            numWarnings = numWarnings + 1',
            '          else if errType is linker error then',
            '            set errTypeString to "Linker Error "',
            '            set logText to logText & errTypeString & ": " & errText & return',
            '          else if errType is linker warning then',
            '            set errTypeString to "Linker Warning "',
            '            set logText to logText & errTypeString & ": " & errText & return',
            '          else',
            '            set logText to logText & errTypeString & ": " & errText & return',
            '          end if',
            '        end tell',
            '  end repeat',
            '  if (numWarnings > 0)',
            '    if logText is not ""',
            '      set logText to logText & "(" & numWarnings & ") Compiler Warnings"',
            '    end if',
            '  end if',
            '  return logText',
            'end ExtractCWErrors')



## This is ugly, output generators should not be allowed
## to change stuff in the project settings!
def fix_pncrt(project):
    """This is a fix for many of the Umakefil/*.pcf files which add
    this library in the wrong place, or incorrectly.  We take care
    to remove it here, then re-add it if necessary."""

    project.RemoveLibraries("pncrt.lib")
    project.RemoveLibraries("pncrtd.lib")

    if not project.IsDefined('HELIX_CONFIG_RN_CRT'):
        return

    project.RemoveSystemLibraries("pncrt.lib")
    project.RemoveSystemLibraries("pncrtd.lib")
    
    ## we remove pncrt entirely with "noruntime" option
    if project.BuildOption("noruntime"):
        return
    
    if project.BuildOption("debug"):
        project.AddLibraries("pncrtd.lib")
    else:
        project.AddLibraries("pncrt.lib")


def make_makefile(platform, project):
    ## FIX pncrt madness
    if project.getTargetType() in [ 'dll' , 'exe' ]:
        fix_pncrt(project)

    ## Create Applescript
    mprj = ProjectToMacCWProjectData(platform, project)
    applescript_path = macfs.FSSpec(project.makefile_name).as_pathname()
    ASMakefile(mprj).script.CompileAndSave(applescript_path)

    ## Pretend like everything went well
    return None

