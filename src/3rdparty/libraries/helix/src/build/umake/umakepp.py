# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umakepp.py,v 1.5 2006/04/24 23:34:04 jfinnecy Exp $ 
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
"""Umake pre-processor and code morpher.  Umakefil/*.pcf files have
depricated syntax which is converted to new syntax, commented out, and
morphed before it can be executed through umake.py."""

import os
import string
import re
import sys
import umake_lib

def pylex(ib, name):
    """Takes a input buffer of Python.  It then removes comments, blank lines,
    explicit line continuations, and implicit line continuations.  This
    prepares the Python code for the regular expression matching used to
    morph the code."""

    if len(ib) == 0:
        return ""

    ib = string.replace(ib, "\r\n","\n")
    ib = string.replace(ib, "\r","\n")

    ## This is not safe - Hubbe
    #ib = string.expandtabs(ib, 8)


    if ib[-1] != "\n":
        ib = ib + "\n"

    ci = 0
    li = 1
    lines = []
    
    r_begin = re.compile("[ \t]*[^] \t()[{}\"'\n#\\\\]*")
    r_white = re.compile("[ \t]*")
    r_normal = re.compile("[^] \t()[{}\"'\n#\\\\]*")
    _openers = ["(", "[", "{"]
    _closers = [")", "]", "}"]
    r_normstr= re.compile("[^\n\"'\\\\]*")

    while ci < len(ib):
        ## Preserve whitespace in beginning
        m = r_begin.match(ib, ci)
        line = m.group(0)
        ci = m.end(0)

        par = 0
        while 1:
            # print "POS = %s:%d (%s, %d) par=%d" % (name, ci+1, repr(ib[ci]), ord(ib[ci]),par)

            if ib[ci] == "\n":
                li = li + 1
                ci = ci + 1

                if par:
                    if line[-1] != " ":
                        line=line+" "

                    if ci >= len(ib):
                        print "umakepp: Missing close parenthesis/braces in file %s" % (name)
                        return []
                else:
                    break
                
            if ib[ci] in _openers:
                par = par + 1
                line = line + ib[ci]
                ci = ci + 1

            if ib[ci] in _closers:
                par = par - 1
                line = line + ib[ci]
                ci = ci + 1
            
            if ib[ci] in [" ", "\t"]:
                if line[-1] != " ":
                    line=line+" "
                m=r_white.match(ib, ci)
                ci=m.end(0)

            if ib[ci] == "#":
                p = string.find(ib, "\n", ci)
                ci = p
                continue

            if ib[ci] in ["'", '"']:
                s = ib[ci]
                ci = ci + 1
                while 1:
                    m=r_normstr.match(ib, ci)
                    s=s+m.group(0)
                    ci=m.end(0)

                    if ib[ci] == "\n":
                        print "umakepp: Newline in string %s:%d" % (name, li)
                        return []

                    if ib[ci] == '\\':
                        if ib[ci+1] == '\n':
                            ci = ci + 2
                            li = li + 1
                            if ci >= len(ib):
                                print "umakepp: End of file in string %s" % (name)
                                return []
                            continue
                        s = s + ib[ci:ci+2]
                        ci = ci + 2
                        continue

                    s=s+ib[ci]
                    ci = ci + 1
                    if s[-1] == s[0]:
                        break

                line = line + s

            if ib[ci] == '\\':
                if ib[ci+1] == '\n':
                    ci = ci + 2
                    li = li + 1
                    if ci >= len(ib):
                        break
                else:
                    s = s + ib[ci:ci+2]
                    ci = ci + 2

            m=r_normal.match(ib,ci)
            line = line + m.group(0)
            ci=m.end(0)
                
        lines.append(line)

    return lines



def remove_line_continuations(um_buff, name):
    """Takes a string of Python code as input, runs it through pylex() to
    remove line continuations, then it removes all the blank lines.  It
    returns a list of strings with one Python statment per line."""

    umake_lib.debug("calling pylex..")
    _line_list = pylex(um_buff, name)
    umake_lib.debug("pylex done")

    line_list = []
    for line in _line_list:
        line = string.rstrip(line)
        if len(line) == 0:
            continue
        line_list.append(line)

    return line_list

##
## Umakefil/*.pcf command translation functions
##
def rpl_print(m):
    temp = "pre-processor removed=\"%s\"" % (m.group(2))
    return "%spass ## %s" % (m.group(1), temp)

def rpl_target_name(m):
    return "project.SetFakeTargetName(%s)" % (m.group(2))

def rpl_lib_subdir(m):
    return "project.output_dir"

def rpl_pass(m):
    temp = "pre-processor removed=\"%s\"" % (m.group(1))
    return "pass ## %s" % (temp)

def rpl_EmptyTarget(m):
    temp = "pre-processor removed=\"%s\"" % (m.group(1))
    return "EmptyTarget() ## %s" % (temp)

def rpl_sources(m):
    return "project.AddSources(%s)" % (m.group(2))
def rpl_objsrcs(m):
    return "project.AddSourceObjects(%s)" % (m.group(2))
def rpl_includes(m):
    return "project.AddIncludes(%s)" % (m.group(2))
def rpl_defines(m):
    return "project.AddDefines(%s)" % (m.group(2))
def rpl_libraries(m):
    return "project.AddLibraries(%s)" % (m.group(2))
def rpl_libraries2(m):
    return "project.AddLibraries2(%s)" % (m.group(2))
def rpl_sys_libraries(m):
    return "project.AddSystemLibraries(%s)" % (m.group(2))
def rpl_dynamic_libraries(m):
    return "project.AddDynamicLibraries(%s)" % (m.group(2))
def rpl_module_libs(m):
    return "project.AddModuleLibraries(%s)" % (m.group(2))
def rpl_local_libs(m):
    return "project.AddLocalLibraries(%s)" % (m.group(2))
def rpl_system_paths(m):
    return "project.AddSystemPaths(%s)" % (m.group(2))
def rpl_exported_func(m):
    return "project.AddExportedFunctions(%s)" % (m.group(2))

def rpl_BuildProgramTarget(m):
    return "ProgramTarget(%s)" % (m.group(2))
def rpl_NormalLibraryTarget(m):
    return "LibraryTarget(%s)" % (m.group(2))


## regular-expression generateors
def project_re1(attr):
    return "(project\.%s\s*\[[^:]*:[^\]]*\]\s*=\s*\[(.*)\]$)" % (attr)

def project_re2(attr):
    return "(project\.%s\.append\((.*)\)$)" % (attr)

def project_re3(attr):
    return "(project\.%s\s*=\s*\[(.*)\]$)" % (attr)

def project_re4(attr):
    return "(project\.%s\s*=\s*project\.%s\s*\+\s*\[(.*)\]$)" % (attr, attr)

_rpl_list = [
    (re.compile("^(\s*)(print\s+.*)"), rpl_print),

    (re.compile("(project\.target_name\s*=\s*(.*))"), rpl_target_name),
    
    (re.compile("(project\.target_type\s*=[^=])"), rpl_pass),
    (re.compile("(project\.object_dir\s*=[^=])"), rpl_pass),
    (re.compile("(project\.output_dir\s*=[^=])"), rpl_pass),
    (re.compile("(project\.dll_type\s*=[^=])"), rpl_pass),
    (re.compile("(project\.src_root_path\s*=[^=])"), rpl_pass),
    (re.compile("(project\.target_dir\s*=[^=])"), rpl_pass),
    (re.compile("(project\.module_dir\s*=[^=])"), rpl_pass),
    (re.compile("(project\.project_dir\s*=[^=])"), rpl_pass),
    (re.compile("(project\.makefile_path\s*=[^=])"), rpl_pass),

    (re.compile("(platform\.lib_subdir\s*=[^=])"), rpl_pass),
    (re.compile("(platform\.lib_subdir)"), rpl_lib_subdir),
    
    (re.compile("(SetupDefines\(\))"), rpl_pass),
    (re.compile("(CPPSuffixRule\(\))"), rpl_pass),
    (re.compile("(SetupTargetDir\(\))"), rpl_pass),
    (re.compile("(DependTarget\(\))"), rpl_pass),
    (re.compile("(ResourceOnlyDLLTarget\(.*\))"), rpl_pass),
    (re.compile("(EmptyTargets\(\))"), rpl_EmptyTarget),
    (re.compile("(SimpleEmptyRules\(\))"), rpl_EmptyTarget),
    
    (re.compile(project_re1("defines")), rpl_defines),
    (re.compile(project_re2("defines")), rpl_defines),
    (re.compile(project_re3("defines")), rpl_defines),
    (re.compile(project_re4("defines")), rpl_defines),
    
    (re.compile(project_re1("sources")), rpl_sources),
    (re.compile(project_re2("sources")), rpl_sources),
    (re.compile(project_re4("sources")), rpl_sources),
    
    (re.compile(project_re1("objsrcs")), rpl_objsrcs),
    (re.compile(project_re2("objsrcs")), rpl_objsrcs),
    (re.compile(project_re4("objsrcs")), rpl_objsrcs),
    
    (re.compile(project_re1("includes")), rpl_includes),
    (re.compile(project_re2("includes")), rpl_includes),
    (re.compile(project_re4("includes")), rpl_includes),

    (re.compile(project_re1("libraries")), rpl_libraries),
    (re.compile(project_re2("libraries")), rpl_libraries),
    (re.compile(project_re4("libraries")), rpl_libraries),

    (re.compile(project_re1("libraries2")), rpl_libraries2),
    (re.compile(project_re2("libraries2")), rpl_libraries2),
    (re.compile(project_re4("libraries2")), rpl_libraries2),

    (re.compile(project_re1("sys_libraries")), rpl_sys_libraries),
    (re.compile(project_re2("sys_libraries")), rpl_sys_libraries),
    (re.compile(project_re4("sys_libraries")), rpl_sys_libraries),

    (re.compile(project_re1("dynamic_libraries")), rpl_dynamic_libraries),
    (re.compile(project_re2("dynamic_libraries")), rpl_dynamic_libraries),
    (re.compile(project_re4("dynamic_libraries")), rpl_dynamic_libraries),

    (re.compile(project_re1("module_libs")), rpl_module_libs),
    (re.compile(project_re2("module_libs")), rpl_module_libs),
    (re.compile(project_re4("module_libs")), rpl_module_libs),

    (re.compile(project_re1("local_libs")), rpl_local_libs),
    (re.compile(project_re2("local_libs")), rpl_local_libs),
    (re.compile(project_re4("local_libs")), rpl_local_libs),

    (re.compile(project_re1("system_paths")), rpl_system_paths),
    (re.compile(project_re2("system_paths")), rpl_system_paths),
    (re.compile(project_re4("system_paths")), rpl_system_paths),

    (re.compile(project_re1("exported_func")), rpl_exported_func),
    (re.compile(project_re2("exported_func")), rpl_exported_func),
    (re.compile(project_re4("exported_func")), rpl_exported_func),

    (re.compile("(BuildProgramTarget\s*\(\s*([^,]+),.*\))"),
     rpl_BuildProgramTarget),
    (re.compile("(NormalLibraryTarget\s*\(\s*([^,]+),.*\))"),
     rpl_NormalLibraryTarget),
    ]


def translate(_line_list):
    """This is the main code morphing function.  It takes a line list with
    exactly one Python statment per line, and compares it aginst a series
    of regular expressions.  It then converts it as necessary, and returns
    a new list of lines of morphed Umakefil code."""

    line_list = []

    for line in _line_list:

        ## fast-skip blank lines and comments
        if len(line) == 0 or string.lstrip(line)[0] == "#":
            line_list.append(line)
            continue

        ## run through the replacment list
        did_sub = 0
        for (r, f) in _rpl_list:
            (nl, n) = r.subn(f, line)

            if not n:
                continue

            line_list.append(nl)
            did_sub = 1
            break

        if not did_sub:
            line_list.append(line)

    return line_list

class UMPreProcessor:
    """Takes a list of Umakefil/*.pcf files and pre-processes them."""

    def __init__(self, path):
        """The argument, path, is the path of the output pre-processed
        Umakefil."""
        
        self.path = path
        self.path_list = []
        #self.mtime = 0

    def add_file_list(self, file_list):
        """The argument, file_list, is a list of Umakefil/*.pcf files to
        be combined, and pre-processed in order.  Added files which do
        not exist are ignored."""
        
        for path in file_list:
            if os.path.isfile(path):
                self.path_list.append(path)
                #self.mtime = max(self.mtime, umake_lib.mtime(path))

    def write_umakefil(self, line_list):
        """Given a list of lines, write out the pre-processed Umakefil to
        the output path."""

        # Disabled. This was originally intended to allow umake to
        # run only when needed, but too many external variable affect
        # how umake works to reliably detect when it needs to be re-run.
        # I suppose I better concentrate on making umake faster insead.
        #   -Hubbe
        #
        #upptime = umake_lib.mtime(self.path)
        #
        #if upptime > self.mtime:
        #    return upptime

        if os.path.exists(self.path):
            os.unlink(self.path)
    
        fil = open(self.path, "w")

        fil.write("### UMAKE PRE-PROCESSOR OUTPUT: DO NOT CHECK INTO CVS\n")
        fil.write("### -*- Mode: Python -*-\n")
        fil.write("### files: %s\n" % (string.join(self.path_list, " -> ")))
        fil.write("\n")

        for line in line_list:
            fil.write("%s\n" % (line))

        fil.write("\n")
        fil.write("### END UMAKE PRE-PROCESSOR OUTPUT\n")
        fil.close()

        return 0

    def preprocess(self):
        """Read all the added Umakefil/*.pcf files, pre-process them, and
        return a list of the lines of the pre-processed Umakefil."""
        
        pre_lines = []
        post_lines = []

        ## get the .pcf files
        for path in self.path_list:
            tmp = remove_line_continuations(open(path, "r").read() + "\n", path)
            try:
                i = tmp.index("<insert>")
                pre_lines = pre_lines + tmp[:i]
                post_lines = tmp[i+1:] + post_lines
            except ValueError:
                pre_lines = pre_lines + tmp

        ## many pre-processor filters
        line_list = translate(pre_lines + post_lines)

        return line_list

    def write(self):
        """Pre-process and write out the pre-processed Umakefil."""
        
        line_list = self.preprocess()
        return self.write_umakefil(line_list)


def main():
    import sys
    
    try:
        path_list = sys.argv[1:]
    except IndexError:
        print "give path dummy"
        sys.exit(1)

    umpp = UMPreProcessor("bla")
    umpp.add_file_list(path_list)
    
    for line in umpp.preprocess():
        print line


if __name__ == "__main__":
    main()
