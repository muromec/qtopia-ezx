#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_win_vc7_makefile.py,v 1.16 2006/06/19 23:11:32 jfinnecy Exp $ 
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
#   This file was contributed by Christian Buchner
# 
# ***** END LICENSE BLOCK *****
# 
# 
""" Makefile generator which generates nmake files and VC7 SLN/VCPROJ files """

## This 'inherits' umake_win_makefile
import os
import umake
import types
import re

umake.my_exec_file(os.path.join(umake.UMAKE_ROOT, "umake_win_makefile.py"), None, globals())

import md5

import log
log.debug( 'Imported: $Id: umake_win_vc7_makefile.py,v 1.16 2006/06/19 23:11:32 jfinnecy Exp $' )

project_extension = "vcproj"
workspace_extension = "sln"

##
## FIXME list:
##   o Make "real" projects instead of projects that just invoke makefiles
##

def hexify(str):
    ret = []
    for c in str:
        ret.append("%02X" % ord(c))
    return string.join(ret,"")

try:
    md5.new("").hexdigest()

    def md5hex(str):
        return string.upper(md5.new(str).hexdigest())

except AttributeError:
    def md5hex(str):
        return hexify(md5.new(str).digest())

guidhash={}
def make_guid(str):
    if guidhash.has_key(str):
        return guidhash[str]
    guid = md5hex(str)
    guid = "{%s-%s-%s-%s-%s}" % (guid[0:8], guid[8:12], guid[12:16], guid[16:20], guid[20:32] )
    guidhash[str]=guid
    return guid

class xml:
    def __init__(self, tagname, attributes=None, data=None):
        self.tagname=tagname
        self.attributes=attributes
        self.data=data

    def encode(self, x):
        x=str(x)
        x=string.replace(x,"&","&amp;")
        x=string.replace(x,"<","&lt;")
        x=string.replace(x,">","&gt;")
        x=string.replace(x,'"',"&quot;")
        return x

    def quot(self, x):
        return '"'+self.encode(x)+'"'

    def generate(self, indent=0):
        i="\t" * indent
        ret=[ "%s<%s" % (i, self.tagname) ]
        if self.attributes:
            ## Contrary to the XML standard, VC7 always needs the
            ## "Name" attribute to be first
            if self.attributes.has_key("Name"):
                ret.append( "\n%s\tName=%s" %
                            (i,self.quot(self.attributes["Name"])))
                
            for (key, val) in self.attributes.items():
                if key == "Name":
                    continue
                ret.append( "\n%s\t%s=%s" %  (i,key,self.quot(val)))
        if self.data != None:
            ret.append(">\n")
            for d in self.data:
                if type(d) == types.StringType:
                    ret.append("\t")
                    ret.append(i)
                    ret.append(string.join(string.split(self.encode(d),"\n"),
                                           "\n\t"+i))
                else:
                    ret.extend(d.generate(indent+1))
                ret.append("\n")
            ret.append("%s</%s>" % (i, self.tagname))
        else:
            ret.append("/>\n")

        return ret;


def ProcessOptions(line, func, options, case_insensetive=None):    
    log.trace( 'entry' , [line, func, options, case_insensetive] )
    
    if type(options) == types.StringType:
        o={}
        options=string.replace(options,":",":\n")
        options=string.replace(options,";",";\n")
        options=string.replace(options,"=","=\n")
        options=string.replace(options,".",".\n")
        for x in string.split(options,"\n"):
            if x:
                if case_insensetive:
                    o[x[:-1]]=x[-1:]
                else:
                    o[x[:-1]]=string.lower(x[-1:])
        options=o

    opt = ''
    for arg in umake_lib.split_arguments(line):
        
        if opt:
            func(opt, arg)
            opt=''
            continue

        if arg[0] in ['/', '-']:
            opt=arg[1:]
            if case_insensetive:
                opt=string.lower(opt)

            arg=''
            odata=options.get(opt)
            if not odata:
                otmp = string.split(opt, ":", 1)
                if len(otmp) > 1 and options.get(otmp[0]) in [':',';']:
                   opt = otmp[0]
                   arg = otmp[1]
                   odata = options.get(opt)

            if not odata and len(opt)>1:
                odata=options.get(opt[0:2])
                if odata:
                    arg=opt[2:]
                    opt=opt[0:2]

            if not odata:
                odata=options.get(opt[0])
                if odata:
                    arg=opt[1:]
                    opt=opt[0]

            if odata in [':','='] and not arg:
                continue

            func(opt, arg)
            opt=''
        else:
            func('', arg)

    log.trace( 'exit' )

class Config(Config):
    """Class representing one profile in a MSDEV project"""

    def Option(self, opt, arg):
        if opt == "nologo":
            self.toolopts["SuppressStartupBanner"]="TRUE"
            return

        print "Unknown Option (%s): %s: %s" % (self.toolopts["Name"],
                                               repr(opt),
                                               repr(arg))
        if opt:
            self.xopts.append("/%s%s " % (opt, arg))
        

    def SimpleCompilerOption(self, opt, arg):
        if string.lower(opt) == "i":
            self.includes.append(arg)
            return

        if string.lower(opt) == "d":
            self.defines.append(arg)
            return

        return self.Option(opt,arg)


    def trueorfalse(self, arg):
        if arg == "-":
            return "FALSE"
        return "TRUE"

    def CompilerOption(self, opt, arg):
        if opt in [ "FD","c","Tp" ]:
            return

        if opt == "W":
            self.toolopts["WarningLevel"]=arg
            return

        if opt == "wd":
            self.toolopts["DisableSpecificWarnings"]=arg
            return

        if opt == "FI":
            self.forced_includes.append(arg)
            return

        if opt == "Fd":
            self.toolopts["ProgramDataBaseFileName"]=arg
            return

        if opt == "O":
            if arg == "d":
                arg = "0"
                
            if arg in [ "0","1","2" ]:
                self.toolopts["Optimization"]=arg
                return

            if arg[0] == "y":
                self.toolopts["OmitFramePointers"]=self.trueorfalse(arg[1:])
                return

        if opt == "M":
            if arg == "T":
                self.toolopts["RuntimeLibrary"]=0
                return
            if arg == "Td":
                self.toolopts["RuntimeLibrary"]=1
                return
            if arg == "D":
                self.toolopts["RuntimeLibrary"]=2
                return
            if arg == "Dd":
                self.toolopts["RuntimeLibrary"]=3
                return
            if arg == "L":
                self.toolopts["RuntimeLibrary"]=4
                return
            if arg == "Ld":
                self.toolopts["RuntimeLibrary"]=5
                return

        if opt == "Y":
            if arg[0] == "X":
                if arg == "X":
                    self.toolopts["UsePrecompiledHeader"]=2
                else:
                    self.toolopts["PrecompiledHeaderThrough"]=arg[1:]
                return

        if opt == "F":
            if arg[0] == "D":
                return
            
            if arg[0] == "p":
                self.toolopts["PrecompiledHeaderFile"]=arg[1:]
                return
            
        if opt == "G":
            if arg[0] == "X":
                self.toolopts["ExceptionHandling"]=self.trueorfalse(arg[1:])
                return
            if arg[0] == "R":
                self.toolopts["RuntimeTypeInfo"]=self.trueorfalse(arg[1:])
                return
            if arg == "d": # CDECL
                self.toolopts["CallingConvention"]=0
                return
            if arg == "r": # FASTCALL
                self.toolopts["CallingConvention"]=1
                return
            if arg == "z": # STDCALL
                self.toolopts["CallingConvention"]=2
                return


        if opt[:1] == "Z":
            if opt == "Z7":
                self.toolopts["DebugInformationFormat"]="1"
                return

            if opt == "Zd":
                self.toolopts["DebugInformationFormat"]="2"
                return

            if opt == "Zi":
                self.toolopts["DebugInformationFormat"]="3"
                return

            if opt == "ZI":
                self.toolopts["DebugInformationFormat"]="4"
                return

            if opt == "Zp":
                self.toolopts["StructMemberAlignment"]=arg
                return

        if opt in [ "Zm" ]:
            self.xopts.append("/%s%s " % (opt, arg))
            return

        return self.SimpleCompilerOption(opt, arg)

    def ResourceOption(self, opt, arg):
        if opt == "l":
            self.toolopts["Culture"]=string.atoi(arg,0)
            return

        return self.SimpleCompilerOption(opt, arg)

    def MidlOption(self, opt, arg):
        if opt == "O":
            if arg in ["icf","if"]:
                self.toolopts["GenerateStublessProxies"]="TRUE"
                return

        return self.ResourceOption(opt, arg)

    def LinkerOption(self, opt, arg):        
        if opt == "":
            if arg == "$(LINKER_OPTIONS)":
                return

            self.xdeps.append(arg)
            return

        if opt == "out":
            self.toolopts["OutputFile"]=arg
            return

        if opt == "base":
            self.toolopts["BaseAddress"]=arg
            return

        if opt == "implib":
            self.toolopts["ImportLibrary"]=arg
            return

        if opt == "subsystem":
            if string.lower(arg) == "console":
                self.toolopts["SubSystem"]=1
                return
            if string.lower(arg) == "windows":
                self.toolopts["SubSystem"]=2
                return

        if opt == "machine":
            if string.lower(arg) in ["x86","i386"]:
                self.toolopts["TargetMachine"]=1
                return

        if opt == "nodefaultlib":
            self.nodefaultlibs.extend(string.split(arg,","))
            return

        if opt in [ "dll", "lib" ]:
            return

        if opt == "pdb":
            if arg == "none":
                return

            self.toolopts["ProgramDatabaseFile"]=arg
            return

        if opt == "def":
            self.toolopts["ModuleDefinitionFile"]=arg
            return
            
        if opt == "debug":
            self.toolopts["GenerateDebugInformation"]="TRUE"
            return

        if opt == "incremental":
            self.toolopts["LinkIncremental"]=2
            return

        if opt == "incremental:no":
            self.toolopts["LinkIncremental"]=1
            return

        if opt == "libpath":
            if self.toolopts.has_key("AdditionalLibraryDirectories"):
                arg=self.toolopts["AdditionalLibraryDirectories"]+";"+arg
            self.toolopts["AdditionalLibraryDirectories"]=arg
            return

        if opt == "map":
            self.toolopts["GenerateMapFile"]=1
            if arg:
                self.toolopts["MapFileName"]=arg
            return

        if opt == "align":
            self.xopts.append("/%s:%s " % (opt, arg))
            return
                      
        # TODO: As far as I can tell, self.Option() has no return
        # value ever. Why call it in such a way that implies that it does?
        # Clean this up as its own change, later.
        return self.Option(opt, arg)
        


    def parse_options(self, name, flags, func, argstring, defopts=None,
                      case_insensetive = None):
                          
        log.trace( 'entry', [name, flags, func, argstring, defopts, case_insensetive] )
        
        self.toolopts = defopts or {}
        self.toolopts["Name"]=name
        
        self.xopts=[]
        self.includes=[]
        self.forced_includes=[]
        self.defines=[]
        self.nodefaultlibs=[]
        self.xdeps=[]

        # expand_variables() is a recursive method that generates mountains
        # of data if logging is done inside of it. Instead, we'll need to 
        # wrap the calls here to see what the params/return values are.
        # This still adds 300k to the logfiles in a simple case, we can push
        # up the execution path and instead investigate where variables are
        # getting set.
        line = makefile.expand_variables("$(%s)"%flags,  self.variables)        
        
        ProcessOptions( line, func, argstring, case_insensetive)

        self.toolopts["ForcedIncludeFiles"]=string.join(self.forced_includes,",")
        self.toolopts["AdditionalOptions"]=string.join(self.xopts)
        self.toolopts["AdditionalIncludeDirectories"]=string.join(self.includes,",")
        self.toolopts["PreprocessorDefinitions"]=string.join(self.defines,",")
        self.toolopts["IgnoreDefaultLibraryNames"]=string.join(self.nodefaultlibs,";")
        self.toolopts["AdditionalDependencies"]=string.join(self.xdeps," ")

        log.trace( 'exit' , self.toolopts )
        return self.toolopts
            
    def generate_xml(self, project):        
        log.trace( 'entry', [project] )
                
        self.cc_toolopts=self.parse_options("VCCLCompilerTool",
                                            "ALL_CC_FLAGS",
                                            self.CompilerOption,
                                            "D=I=Zm=Zp=FI=W=O=M=G=Y=F=wd=Fd=",
                                            defopts={"Optimization":"0" })


        self.rc_toolopts=self.parse_options("VCResourceCompilerTool",
                                            "RCFLAGS",
                                            self.ResourceOption,"D=d=I=i=l=")

        self.midl_toolopts=self.parse_options("VCMIDLTool",
                                              "MTLFLAGS",
                                              self.MidlOption,"D=d=I=i=l=O=")


        l="out:base:implib:subsystem:nodefaultlib:pdb:machine:def:libpath:map;align:"
        
        self.linker_toolopts=self.parse_options("VCLinkerTool",
                                                "LINKER_OPTIONS",
                                                self.LinkerOption,
                                                l,None, 1)
  
                                                
        if self.target_type == "exe":
            conftype = 1
        elif self.target_type == "dll":
            conftype = 2
        elif self.target_type == "lib":
            conftype = 4
        else:
            conftype = 0

        if conftype != 0:
            tools = [
                xml("Tool", self.cc_toolopts),
                xml("Tool",
                    { "Name":"VCCustomBuildTool", }),
                xml("Tool", self.midl_toolopts ),
                xml("Tool",
                    { "Name":"VCPostBuildEventTool",
                      "Description":"Copying...",
                      "CommandLine":string.join(self.post_build_cmds,"\n")}),
                xml("Tool",
                    { "Name":"VCPreBuildEventTool", }),
                xml("Tool",
                    { "Name":"VCPreLinkEventTool", }),
                xml("Tool", self.rc_toolopts),
                xml("Tool",
                    { "Name":"VCWebServiceProxyGeneratorTool", }),
                xml("Tool",
                    { "Name":"VCXMLDataGeneratorTool", }),
                xml("Tool",
                    { "Name":"VCWebDeploymentTool", }),
                xml("Tool",
                    { "Name":"VCManagedWrapeprGeneratorTool", }),
                xml("Tool",
                    { "Name":"VCAuxiliaryManagedWrapperGeneratorTool", }),
                # xml("Tool",
                #   { "Name":""SupercalifragilisticexpalidociusTool", }),
                ]

            if conftype == 4:
                tools.append(
                    xml("Tool",{ "Name":"VCLibrarianTool",
                                 "OutputFile":self.linker_toolopts["OutputFile"]}))
            else:
                tools.append(xml("Tool", self.linker_toolopts))

                
        else:
            tools= [
                xml("Tool",
                    { "Name":"VCNMakeTool",
                      "BuildCommandLine":
                      "nmake.exe /NOLOGO /f %s all copy" %
                      project.makefile_name,
                      "ReBuildCommandLine":
                      "nmake.exe /NOLOGO /f %s /a all copy" %
                      project.makefile_name,
                      "CleanCommandLine":
                      "nmake.exe /NOLOGO /f %s clean" %
                      project.makefile_name,
                      "Output":self.variables["OUTFILE"] }),
                ]
        
        return xml("Configuration",
                   { "Name": self.type,
                     "OutputDirectory":self.variables["OUTDIR"],
                     "IntermediateDirectory":self.variables["OBJDIR"],
                     "ConfigurationType":conftype },
                   tools)
                   
        log.trace( 'exit' )
                
class Project(Project):
    """Class representing and generating one MSDEV project"""

    def generate_xml(self):
        guid = make_guid(self.name)

        config_xml=[]
        for c in self.configurations:
            config_xml.append(c.generate_xml(self))

        file_xml=[]
        resource_xml=[]
        for sourcefile in self.sourcefiles:
            if sourcefile.build_rule.source_suffix in [ ".rc",".mc" ] \
               or c.target_type not in ["exe","dll","lib"]:
                resource_xml.append(xml("File",{"RelativePath":sourcefile.path}))
            else:
                file_xml.append(xml("File",
                                    {"RelativePath":sourcefile.path},
                                    [xml("FileConfiguration",
                                         {"Name":"Build|Win32"},
                                         [xml("Tool",
                                              {"Name":"VCCLCompilerTool",
                                               "ObjectFile":sourcefile.obj_path})])]))
                

        header_xml=[]
        for source in self.headerfiles:
            header_xml.append(xml("File",{"RelativePath":source}))

        return xml("VisualStudioProject",
                   { "ProjectType":"Visual C++",
                     "Version":"7.10",
                     "Name":self.name,
                     "ProjectGUID":guid,
                     "Keyword":"MakeFileProj", },
                   [ xml("Platforms", None,
                         [ xml("Platform",{ "Name":"Win32" } ) ] ),
                     xml("Configurations", {}, config_xml),
                     xml("Files", {},
                         [ xml("Filter",
                               { "Name":"Source Files",
                                 "Filter":"cpp;c;cxx;def;odl;idl;hpj;bat;asm"
                                 },
                               file_xml),
                           xml("Filter",
                               { "Name":"Header Files",
                                 "Filter":"h;hpp;hxx;hm;inl;inc",
                                 },
                               header_xml),
                           xml("Filter",
                               { "Name":"Resource Files",
                                 "Filter":"rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe",
                                 },
                               resource_xml)
                           ]),
                     xml("Globals",{},[]),
                     ])

    def generate(self):
        text = [ ]
        
        text.append('<?xml version="1.0" encoding = "Windows-1252"?>\n')
        text.extend(self.generate_xml().generate())
        text=string.join(text,"")
        text=string.replace(text,"$(MODULE)",self.name)

        return text;


class Workspace(Workspace):
    """Class representing and generating one MSDEV solution"""

    def generate(self):
        text = []
        text.append('Microsoft Visual Studio Solution File, Format Version 8.00')

        self.text=text
        self.done={}
        self.project_byname={}

        for (name, path) in self.projects:
            self.project_byname[name]=path

        for (name, path) in self.projects:
            self.generate_project(name)

        text.append('Global')

        text.append('\tGlobalSection(SolutionConfiguration) = preSolution')
        text.append('\t\tBuild = Build')
        text.append('\tEndGlobalSection')

        text.append('\tGlobalSection(ProjectConfiguration) = postSolution')
        for (name, path) in self.projects:
            guid = make_guid(name)
            text.append('\t\t%s.Build.ActiveCfg = Build|Win32' % (guid))
            text.append('\t\t%s.Build.Build.0 = Build|Win32' % (guid))
        text.append('\tEndGlobalSection')

        text.append('\tGlobalSection(ExtensibilityGlobals) = postSolution')
        text.append('\tEndGlobalSection')

        text.append('\tGlobalSection(ExtensibilityAddIns) = postSolution')
        text.append('\tEndGlobalSection')

        text.append('EndGlobal')

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
        guid = make_guid(name)
        text.append('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "%s", "%s", "%s"' % (name, path, guid))
        text.append('\tProjectSection(ProjectDependencies) = postProject')
        for d in self.dependencies.get(name,[]):
            depguid = make_guid(d)
            text.append('\t\t%s = %s\n' % (depguid, depguid))
        text.append('\tEndProjectSection')
        text.append('EndProject')


class project_generator(project_generator):
    """Class which generates a VCPROJ file from the data in
    in the platform and project classes"""

    def add_configurations(self):
        self.do_config("Build|Win32")

