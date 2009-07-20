# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_win_makefile.py,v 1.13 2006/06/19 23:11:32 jfinnecy Exp $ 
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
""" Makefile generator which generates nmake files and VC6 DSP/DSW files """

import string

import makefile
import umake_makefile
import umake_lib
import umake
import bldreg
import os


import log
log.debug( 'Imported: $Id: umake_win_makefile.py,v 1.13 2006/06/19 23:11:32 jfinnecy Exp $' )

project_extension = "dsp"
workspace_extension = "dsw"

##
## FIXME list:
## o add the 'make copy' commands to the post-build-commands
## o Make multitarget makefiles use dependencies to build
##   the subprojects instead of using a Makefile project
##   (Is it possible to create a name-only project??)


def prepend_str_list(prepend_str, list):
    """Given a string and a list of strings, returns a new list with the
    string pre-pended to each item in the list."""
    
    new_list = []
    for current_str in list:
        new_list.append(prepend_str + current_str)
    return new_list



###
### What follows is a bunch of text fragments
### which make up a *.dsp file
###
header1="""# Microsoft Developer Studio Project File - Name="$(MODULE)" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **
"""

header2="""
CFG=$(MODULE) - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "$(MODULE).mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "$(MODULE).mak" CFG="$(MODULE) - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE """

header3="""!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=$(CC)
MTL=midl.exe
RSC=$(RC)
"""

config="""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries $(DEBUG)
# PROP BASE Output_Dir "$(OUTDIR)"
# PROP BASE Intermediate_Dir "$(OBJDIR)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries $(DEBUG)
# PROP Output_Dir "$(OUTDIR)"
# PROP Intermediate_Dir "$(OBJDIR)"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP $(ALL_CC_FLAGS)
# ADD CPP $(ALL_CC_FLAGS)
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC $(RCFLAGS)
# ADD RSC $(RCFLAGS)
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo 
"""

config_exe="""LINK32=$(LD)
# ADD BASE LINK32 $(LDFLAGS)
# ADD LINK32 $(LINKER_OPTIONS)
"""
config_lib="""LIB32=$(LD) -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /OUT:$(OUTFILE) $(SOURCE_OBJS)
"""
config_make="""
# PROP BASE Cmd_Line "$(MAKE) /f $(MAKEFILE)"
# PROP Cmd_Line "$(MAKE) /f $(MAKEFILE)"
# PROP Rebuild_Opt "/a"
"""

config_postbuildcmd="""
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=$(POSTBUILDCMDS)
# End Special Build Tool
"""



class Config:
    """Class representing one profile in a MSDEV project"""
    
    def __init__(self,
                 type,
                 variables,
                 target_type,
                 post_build = []):
        self.variables = variables.copy()
        self.type=type
        self.variables["TYPE"]=type
        self.target_type = target_type
        if string.lower(type[:6]) == "debug":
            self.variables["DEBUG"]="1"
        else:
            self.variables["DEBUG"]="0"
        self.post_build_cmds = post_build


    def generate(self, project):
        global config
        global config_link
        global config_lib
        global config_make

        if self.target_type == "lib":
            text = config + config_lib
        elif self.target_type == "":
            text = config + config_make
        else:
            text = config + config_exe

        self.variables["POSTBUILDCMDS"]=string.join(self.post_build_cmds,"\t")
        if len(self.post_build_cmds):
            text = text + config_postbuildcmd
            
        return makefile.expand_variables(text, self.variables)


    def generate_message(self):
        if self.target_type == "exe":
            text="""!MESSAGE "$(MODULE) - Win32 $(TYPE)" (based on "Win32 (x86) Application")"""
        elif self.target_type == "lib":
            text="""!MESSAGE "$(MODULE) - Win32 $(TYPE)" (based on "Win32 (x86) Static Library")"""
        elif self.target_type == "dll":
            text="""!MESSAGE "$(MODULE) - Win32 $(TYPE)" (based on "Win32 (x86) Dynamic-Link Library")"""
        elif self.target_type == "":
            text="""!MESSAGE "$(MODULE) - Win32 $(TYPE)" (based on "Win32 (x86) External Target")"""
        else:
            raise "Target type '%s' is not valid!" % self.target_type
        return makefile.expand_variables(text, self.variables)

    def generate_message2(self):
        text='# Name "$(MODULE) - Win32 $(TYPE)"'
        return makefile.expand_variables(text, self.variables)

class Project:
    """Class representing and generating one MSDEV project"""
    
    def __init__(self, name, makefile_name, target_type, variables):
        log.trace('entry', [name, makefile_name, target_type, variables] )
        self.name = name
        self.makefile_name = makefile_name
        self.target_type = target_type
        self.configurations = []
        self.sourcefiles = []
        self.headerfiles = []
        self.variables = variables

    def AddConfig(self, type, variables, postbuild = []):
        self.configurations.append(Config(type, variables, self.target_type, postbuild))

    def AddSourceFile(self, sourcefile):
        self.sourcefiles.append( sourcefile )

    def AddHeaderFile(self, filename):
        self.headerfiles.append(filename)

    def AddHeaderFiles(self, filenames):
        self.headerfiles.extend(filenames)


    def generate(self):
        global header1
        global header2
        global header3
        text = [ makefile.expand_variables(header1, self.variables) ]

        if self.target_type == "exe":
            text.append('# TARGTYPE "Win32 (x86) Application" 0x0101')
        elif self.target_type == "lib":
            text.append('# TARGTYPE "Win32 (x86) Static Library" 0x0104')
        elif self.target_type == "dll":
            text.append('# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102')
        elif self.target_type == "":
            text.append('# TARGTYPE "Win32 (x86) External Target" 0x0106')

        text.append(makefile.expand_variables(header2, self.variables))

        for config in self.configurations:
            text.append(config.generate_message())
        text.append(makefile.expand_variables(header3, self.variables))

        i=0
        ifelse="IF"
        for config in self.configurations:
            text.append('!%s  "$(CFG)" == "$(MODULE) - Win32 %s"' % (ifelse, config.type))
            text.append(config.generate(self))
            ifelse="ELSEIF"
        text.append("!ENDIF")

        text.append("")
        text.append("# Begin Target")
        text.append("")

        for config in self.configurations:
            text.append(config.generate_message2())

        #text.append('# Begin Group "Resources"')
        #text.append("")
        #text.append('# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"')
        #text.append("# End Group")

        text.append('# Begin Group "Source Files"')

        for sourcefile in self.sourcefiles:
            text.append('# Begin Source File')
            text.append('')
            text.append('SOURCE=%s' % sourcefile.path)
            if sourcefile.obj_path:
                text.append('# PROP Intermediate_Dir "%s"' % os.path.dirname(sourcefile.obj_path))
            if sourcefile.build_rule.source_suffix in [".cpp", ".cc",".c"]:
                xflags=self.variables["EXTRA_%s_FLAGS" % sourcefile.build_rule.command.make_var]
                if xflags:
                    text.append('# ADD CPP %s' % xflags)

            text.append('# End Source File')
            
        text.append('# End Group')

        text.append('# Begin Group "Header Files"')

        for source in self.headerfiles:
            text.append('# Begin Source File')
            text.append('')
            text.append('SOURCE=%s' % source)
            text.append('# End Source File')
            
        text.append('# End Group')
        text.append('# End Target')
        text.append('# End Project')
        
        
        text.append("")

        text=string.join(text,"\n")
        text=string.replace(text,"$(MODULE)",self.name)
        return text;


class Workspace:
    """Class representing and generating one MSDEV workspace"""
    def __init__(self):
        self.projects = []
        self.dependencies = {}


    def AddProject(self, name, path):
        self.projects.append( (name, path) )
        self.dependencies[name] = []

    def AddDependency(self, dependant, supporter):
        self.dependencies[dependant].append(supporter)



    def generate(self):
        text = []
        text.append('Microsoft Developer Studio Workspace File, Format Version 6.00')
        text.append('# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!')
        text.append('')

        self.text=text
        self.done={}
        self.project_byname={}

        for (name, path) in self.projects:
            self.project_byname[name]=path

        for (name, path) in self.projects:
            self.generate_project(name)

        return string.join(self.text,"\n")
    
    def generate_project(self, name):
        if self.done.has_key(name):
            return

        self.done[name]=1

        if not self.project_byname.has_key(name):
            return

        path = self.project_byname[name]

        for d in self.dependencies.get(name,[]):
            self.generate_project(d)
        
        text=self.text
        text.append('###############################################################################')
        text.append('')
        text.append('Project: "%s"=%s - Package Owner=<4>' % (name, path))
        text.append('')
        text.append('Package=<5>')
        text.append('{{{')
        text.append('}}}')
        text.append('')
        text.append('Package=<4>')
        text.append('{{{')
        for d in self.dependencies.get(name,[]):
            text.append('    Begin Project Dependency')
            text.append('    Project_Dep_Name %s' % d)
            text.append('    End Project Dependency')
        text.append('}}}')
        text.append('')


def subtract_array(a, b):
    tmp={}
    for x in b:
        tmp[x]=1
    ret=[]
    for x in a:
        if not tmp.has_key(x):
            ret.append(x)
    return ret

class project_generator(umake_lib.Targets):
    """Class which generates a DSP file from the data in
    in the platform and project classes"""

    def setup_variables(self):
        """Read the generated Makefile and grab all the variables
           from it. This can be optimized a lot since umake_makefile
           already knows all this stuff.. """
        log.trace('entry')
        mf=open(self.project.makefile_name,"r").read()
        mf=makefile.ParseMakefile(mf)
        self.mf = mf
        self.variables=mf.get_variables()
        self.variables["MAKEFILE"]=self.project.makefile_name        
        log.debug('self.variables = %s' % self.variables)
        log.trace('exit')
        
    def do_config(self,type):
        vars=self.variables.copy()
        cc=self.platform.cc

        ## This is modeled after umake.Compiler.execute
        extra_args = ""

        if cc.args.has_key(type):
            extra_args = cc.args[type]

        #vars["ALL_C_FLAGS"] = umake.form_string(
        #    self.platform.form_var(cc.make_flags),
        #    extra_args,
        #    cc.source_arg)

        # UGLY!
        OPTIONS={}
        for (suffix, rule) in self.platform.build_rules.items():
            c=rule.command
            tmp=c.execute('XyzzY%s' % rule.target_suffix,
                          'XyzzY%s' % rule.source_suffix)
            tmp=string.split(tmp,' ')
            n = []
            for x in tmp:
                if string.count(x,"XyzzY"):
                    continue
                if x == "$(%s)" % c.make_var:
                    continue

                n.append(x)
            tmp=string.join(n)
            OPTIONS[suffix]=tmp
            vars["ALL_%s_FLAGS" % c.make_var]=tmp

        
        for (suffix, rule) in self.platform.build_rules.items():
            c=rule.command
            f1=makefile.expand_variables(vars["ALL_%s_FLAGS" % c.make_var], vars,1)
            f2=makefile.expand_variables(OPTIONS[rule.source_suffix],vars,1)
            f3=string.join(subtract_array(string.split(f1), string.split(f2)))
            
            self.variables["EXTRA_%s_FLAGS" % c.make_var]=f3

        output_path = self.project.getOutputPath()
        vars["OUTFILE"]=output_path


        static_libs="$(STATIC_LIBS)"
        # static_libs=''  # Added automatically?
        dynamic_libs="$(DYNAMIC_LIBS)"
        objects = self.platform.form_var("SOURCE_OBJS")

        cmd_list=None
        if hasattr(self.platform.link, "linker2"):
            if self.project.getTargetType() == "lib":
                cmd_list=self.platform.link.LinkLIB(output_path,
                                                    objects)
            elif self.project.getTargetType() == "exe":
                cmd_list=self.platform.link.LinkEXE(output_path,
                                                    objects,
                                                    static_libs,
                                                    dynamic_libs)
            elif self.project.getTargetType() == "dll":
                cmd_list=self.platform.link.LinkDLL(output_path,
                                                    objects,
                                                    static_libs,
                                                    dynamic_libs)
        else:
            if self.project.getTargetType() == "lib":
                cmd_list = [ self.platform.make_lib.execute(output_path, objects, "", "") ]
            else:
                cmd_list = [ self.platform.link.execute(output_path,
                                                        objects,
                                                        static_libs,
                                                        dynamic_libs) ]

        if cmd_list:
            linkopts=cmd_list[0]
            tmp=string.split(linkopts," ")

            # Kluge!!!
            p = 0
            while tmp[p][0] != "/":
                p = p + 1

            linkopts=string.join(tmp[p:]," ")
            vars["LINKER_OPTIONS"] = linkopts
            vars["LD"]=string.join(tmp[:p]," ")

        vars["OBJDIR"]=self.project.object_dir
        vars["OUTDIR"]=self.project.output_dir


        ## Recover 'copy' target from Makefile
        postbuild = []

        for t in self.mf.target_list:
            if t.name == "copy":
                for cmd in t.command_list:
                    if not cmd:
                        continue
                    if cmd[0]=='-':
                        cmd=cmd[1:]
                    if cmd[0]=='@':
                        cmd=cmd[1:]
                    postbuild.append(cmd)
        
        self.proj.AddConfig(type, vars, postbuild)


    def add_configurations(self):
        self.do_config("Release")
        self.do_config("Debug")

    def __init__(self, platform, project):
        umake_lib.Targets.__init__(self, platform, project)

        self.variables = {}

        self.setup_variables()

        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)
            
        proj=Project(name,
                     self.project.makefile_name,
                     project.getTargetType(),
                     self.variables)
        self.proj=proj

        self.add_configurations()
            
        for src in project.sources:
            sourcefile = umake_lib.SourceFile(
                platform, src, project.object_dir)
            proj.AddSourceFile(sourcefile)

        cache = globals().get("__umake_win_makefile_header_cache__",
                              {"dir":""})


        if cache["dir"] != os.getcwd():
            cache["dir"]=os.getcwd()
            headers = []
            ## Not very pretty, we add all header files
            ## found in this directory (or any subdir)
            ## to the project.
            potential_headers = [ "." ]
            for h in potential_headers:
                if os.path.isdir(h):
                    for f in os.listdir(h):
                        potential_headers.append(os.path.join(h,f))
                else:
                    if string.lower(os.path.splitext(h)[1]) == ".h":
                        headers.append(h)

            cache["headers"] = headers
            globals()["__umake_win_makefile_header_cache__"] = cache


        proj.AddHeaderFiles(cache["headers"])

        global project_extension
        global workspace_extension
        
        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)
        data=proj.generate()

        umake_lib.write_file(name+"."+project_extension,proj.generate())


## 
class workspace_generator(umake_lib.Targets):
    """Class which generates a DSW file from the data in
    in the platform and project classes"""

    def __init__(self, platform, project):
        global project_extension
        global workspace_extension
        
        umake_lib.Targets.__init__(self, platform, project)

        dsw=Workspace()

        projects = []
        makefiles = []
        
        for sumake in project.get_uber_makes():
            makefiles.append(sumake.abs_makefile())

            name = umake_lib.declaw_name(sumake.abs_makefile())
            path = os.path.dirname(sumake.makefile())
            path = os.path.join(path, umake_lib.declaw_name(sumake.abs_makefile()))
            dsw.AddProject(name,path+"."+project_extension)
            for dep in sumake.dependencies():
                dep = umake_lib.declaw_name(dep)
                dsw.AddDependency(name, dep)

        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)
        umake_lib.write_file(name+"."+workspace_extension,dsw.generate())


def make_makefile(platform, project):
    ret1 = umake_makefile.make_makefile(platform, project)
    if project.BuildOption("no_project_files"):
        return ret1

    if project.BuildOption("buildfarm_build") and \
           not project.BuildOption("archive") and \
           not project.BuildOption("project_files") and \
           not project.IsDefined("RIBOSOME_PROJECT_FILES"):
        return ret1

    # Make VC6 project files here
    ret2 = project_generator(platform, project)
    ret3 = workspace_generator(platform, project)

    return ret1 or ret2 or ret3

def compile():
    return umake_makefile.compile()
