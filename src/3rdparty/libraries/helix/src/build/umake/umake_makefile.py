# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_makefile.py,v 1.34 2006/08/03 19:58:51 jfinnecy Exp $ 
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
"""Implement the Makefile back end for Umake.  This is used by both UNIX and
Windows targets to generate Makefiles to build targets."""

import os
import string
import types
import umake_lib
import makefile
import shell
import re
import sysinfo

import log
log.debug( 'Imported: $Id: umake_makefile.py,v 1.34 2006/08/03 19:58:51 jfinnecy Exp $' )

def prepend_str_list(prepend_str, list):
    """Given a string and a list of strings, returns a new list with the
    string pre-pended to each item in the list."""
    
    new_list = []
    for current_str in list:
        new_list.append(prepend_str + current_str)
    return new_list


class makefile_generator(umake_lib.Targets):
    def writeln(self, text):
        self.makefile.append(text+"\n")

    def write(self, text):
        self.makefile.append(text)
        
    def wantsSign( self ):
        if self.project.getTargetType() == 'dll' and \
               self.project.BuildOption("drmsign") and \
               self.project.CheckDRMSign():
                   return 1
        else:
            return 0
            
    def signFile( self , target ):
        command = "python %s" % os.path.join(os.environ.get("BUILD_ROOT"), "bin", "sign.py")

        # Add the arguments to the command line.
        signType = self.project.getSignType()
        if signType:
            command += ' -t %s' % signType

        options = self.project.getSignOptions()
        if options:
            for ( k , v ) in options.items():
                command += ' -z "%s:%s"' % ( k , v )

        self.writeln( "\t%s %s" % ( command , target ))
     
    def sign( self , target ):
        if self.wantsSign():
            self.signFile( target )

    def AssertTarget(self, targ):
        if self.created_targets.has_key(targ):
            print "umake: warning: Duplicate target %s" % targ
        else:
            self.created_targets[targ]=1

    def mkdir(self, dir, recursive = 0):
        """Returns a string that should make a directory
        on the current platform, if recursive = 1, then it returns
        a list of makedir strings that when executed in order
        creates a deep subdirectory."""

        umake_lib.debug("Project.mkdir")

        if not recursive:
            return self.platform.mkdir.execute(dir)

        ## Special Recursive mode for Makefiles
        cmds = []
        done = { None:1, "":1, os.sep:1, os.curdir:1, os.pardir:1 }
        while 1:
            if done.has_key(dir):
                break
            (head, tail) = os.path.split(dir)
            if not done.has_key(tail):
                cmds.append(self.platform.mkdir.execute(dir))
            done[dir]=1
            dir = head

        cmds.reverse()
        return cmds

    def build_quoted_arg_list(self, args, prefix="", qtype="make"):

        ## Dependencies don't have prefixes
        if prefix:
            qtype="shell"

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

        ## NMAKE/cmd.exe quoting
        def do_quote_nmake(str):
            if re.match('^[^ "]*$', str):
                return str

            str = re.sub(r'(\\*)"', r'\1\1\"', str + '"')
            return '"%s"' % str[:-2]
        
        if qtype == "make":
            if string.lower(os.path.basename(self.platform.make.cmd)) == 'nmake':
                do_quote = do_quote_nmake
            else:
                ## (GNU) make quoting (semi-posix)
                def do_quote(str):
                    if re.match('^[^ "]*$', str):
                        return str

                    str = re.sub(r'(\\*)([ "])', r'\1\1\2', str)
                    return str

        else:
            if sysinfo.host_type == "win32":
                do_quote = do_quote_nmake
            else:
                ## POSIX quoting
                def do_quote(str):
                    if re.match('^[-a-zA-Z0-9_+=.,/@]*$', str):
                        return str

                    str = re.sub(r'([^- a-zA-Z0-9_+=.,/@])', r'\\\1', str)
                    return '"' + str + '"'


        return string.join(map(do_quote, tmp)," ")

    def compat_quote(self, args, prefix="", qtype="file"):
        if self.project.FeatureIsEnabled("makefile_quotes"):
            return self.build_quoted_arg_list(args, prefix, qtype)
        else:
            if prefix:
                args=prepend_str_list(prefix, args)
            return string.join(args)

    def write_macros(self):
        """Writes all the macros (variables) to the Makefile."""

        umake_lib.debug("Project.write_macros")

        def str_list(list):
            return string.join(list)


        self.writeln("## Generated from %s, do not edit, do not commit to cvs!" % (self.project.umakefile_name ))
        self.writeln("")

        ## print out all command and command flag variables
        ## in the platform command list, taking care not to
        ## print blank lines for commands/command flas which
        ## do not exist
        for current_command in self.platform.command_list:
            command_var = current_command.setup_command_var()
            flags_var = current_command.setup_flags_var()
            if len(command_var):
                self.writeln(command_var)
            if len(flags_var):
                self.writeln(flags_var)

        ## write out env variables for compilers
        for build_rule in self.platform.build_rules.values():
            current_command = build_rule.command
            command_var = current_command.setup_command_var()
            flags_var = current_command.setup_flags_var()
            if len(command_var):
                self.writeln(command_var)
            if len(flags_var):
                self.writeln(flags_var)

        ## LINKER
        if not hasattr(self.platform.link, "linker2"):
            self.writeln(self.platform.link.setup_command_var())
            self.writeln(self.platform.link.setup_flags_var())

        ## SRCS
        self.writeln("SRCS=%s" % (self.compat_quote(self.project.sources)))

        ## COMPILED_OBJS, SOURCE_OBJS, OBJS
        self.writeln("OBJS=%s %s" % (
            self.platform.form_var("COMPILED_OBJS"),
            self.platform.form_var("SOURCE_OBJS")))

        self.writeln("COMPILED_OBJS=%s" % (
            self.compat_quote(self.project.objects)))
        self.writeln('SOURCE_OBJS=%s' % (
            self.compat_quote(self.project.objsrcs)))    

        ## INCLUDES
        self.writeln("INCLUDES=%s" % self.build_quoted_arg_list(
            self.project.includes,
            self.platform.include_arg))

        ## DEFINES
        if self.platform.cc.prefix_include_arg:
            defdir = self.project.output_dir
            shell.mkdir(defdir)
            name = os.path.join(self.project.module_directory(),
                                self.project.makefile_name)
            prefix_file_name = umake_lib.declaw_name(name)+"_ribodefs.h"
            prefix_file_name=os.path.join(defdir, prefix_file_name)
            
            lines=[]

            defs=self.project.defines
            defs.sort()
            for d in defs:
                ind = string.find(d,"=")
                if ind == -1:
                    lines.append("#ifndef %s" % (d))
                    lines.append("#define %s 1" % (d))
                else:
                    lines.append("#ifndef %s" % (d[:ind]))
                    lines.append("#define %s %s" % (d[:ind],d[ind+1:]))
                lines.append("#endif")

            for include in self.project.prefix_file_include_list:
                ## Ugly magic stuff
                if type(include) == types.StringType:
                    if include[0] == '"' or include[0] == '<':
                        lines.append("#include %s" % (include))
                    elif include[0] == '#':
                        lines.append("%s" % (include))
                    else:
                        lines.append("#include <%s>" % (include))
                elif type(include) == types.ListType:
                    lines.extend(include)

            data = string.join(lines,"\n")+"\n"

            umake_lib.write_file(prefix_file_name, data)

            self.writeln("DEFINES=%s%s %s%s" % (
                self.platform.include_arg,
                os.curdir,
                self.platform.cc.prefix_include_arg,
                prefix_file_name))
        else:
            self.writeln("DEFINES=%s" % self.build_quoted_arg_list(
                self.project.defines,
                self.platform.define_arg))

        ## STATIC_LIBS
        static_libs = self.project.libraries + self.project.libraries2 + \
                      self.project.local_libs + self.project.module_libs
        self.writeln("STATIC_LIBS=%s" % (self.compat_quote(static_libs)))

        ## DYNAMIC_LIBS
        self.writeln("DYNAMIC_LIBS=%s %s" % (
            self.compat_quote(self.project.dynamic_libraries),
            self.compat_quote(self.project.sys_libraries,
                              self.platform.sys_lib_arg)))

        self.writeln("")

        ## suffixes
        if len(self.platform.suffix_list):
            self.writeln(".SUFFIXES: %s" % (
                string.join(self.platform.suffix_list)))
            self.writeln("")

        ## default/misc build rules
        for rule in self.platform.build_rules.values():

            ## Add custom INCLUDE/DEFINE variables for each compiler
            ## (Except CC/CXX, which uses INCLUDE/DEFINES)
            if rule.command.make_var not in [ "CC", "CXX" ]:
                if rule.command.define_arg and rule.command.make_var:
                    try:
                        defs = None
                        try:
                            defs = rule.command.defines
                        except AttributeError:
                            pass

                        if defs == None:
                            defs = self.project.defines
                        
                        self.writeln("%sDEFINES=%s" % (
                            rule.command.make_var,
                            self.build_quoted_arg_list(defs,
                                                  rule.command.define_arg)))
                        self.writeln("")
                    except:
                        pass

                if rule.command.include_arg and rule.command.make_var:
                    try:
                        includes = None
                        try:
                            includes = rule.command.includes
                        except AttributeError:
                            pass

                        if includes == None:
                            includes = self.project.includes
                        
                        self.writeln("%sINCLUDES=%s" % (
                            rule.command.make_var,
                            self.build_quoted_arg_list(includes,
                                                  rule.command.include_arg)))
                        self.writeln("")
                    except:
                        pass

            if self.platform.build_rules.get(rule.source_suffix,None) == rule:
                rule_str = "%s%s%s" % (
                    rule.source_suffix, rule.target_suffix,
                    self.platform.make_depend)
                self.writeln(rule_str)

                cmd_str = rule.command.execute(
                    self.platform.make_target, self.platform.make_source)
                self.writeln("\t%s" % (cmd_str))
                self.writeln("")


    def write_object_depends(self):
        """Write out a dependancy rule for each source file so its object file
        can be written to a alternative target directory."""

        umake_lib.debug("Project.write_object_depends")

        for path in self.project.sources:
            sourcefile = umake_lib.SourceFile(
                self.platform, path, self.project.object_dir)
            if not os.path.isfile(sourcefile.path):
                continue

            self.writeln("%s: %s" % (
                sourcefile.obj_path, sourcefile.path))

            ## make the object target directory
            (obj_path, obj_basename) = os.path.split(sourcefile.obj_path)

            for dir in self.mkdir(obj_path, 1):
                ## the "-" sign allows the command to fail but be okay in
                ## the Makefile; for parallel builds, the directory creation
                ## may have already been handled by another process
                self.writeln("\t-@%s" % (dir))

            ## write the command to build the object
            temp = sourcefile.build_rule.command.execute(
                sourcefile.obj_path, sourcefile.path)

            self.writeln("\t%s" % (temp))

    def call_submake(self, makefile,
                     rule = None,
                     ignore_errors=None,
                     do_cd = 1):

        if do_cd:
            dir = os.path.dirname(makefile)
            mf = os.path.basename(makefile)
        else:
            mf = makefile
            dir = ""

        if string.lower(os.path.basename(self.platform.make.cmd)) == 'nmake':
            ## Win
            nmake = 1
            cmd = "$(MAKE) $(SUBMAKEFLAGS) SUBMAKEFLAGS=\"$(SUBMAKEFLAGS)\" -nologo -f %s" % mf
        else:
            ## Unix
            nmake = 0
            cmd = "$(MAKE) $(SUBMAKEFLAGS) SUBMAKEFLAGS=\"$(SUBMAKEFLAGS)\" -f %s" % mf
            
        if rule:
            cmd = "%s %s" % (cmd, rule)

        if len(dir) and dir != ".":
            if not nmake:
                cmd="$-$cd %s && %s" % (dir, cmd)
            else:
                cmd="cd %s\n\t$-$%s\n\tcd $(MAKEDIR)" % ( dir, cmd )
        else:
            cmd="$-$"+cmd

        if ignore_errors:
            dash = "-"
        else:
            dash = ""
            
        return "\t" + string.replace(cmd,"$-$",dash)
    
    def call_all_submake(self, rule = None, ignore_errors=None):
        for sumake in self.project.submakes:
            self.writeln(self.call_submake(sumake.makefile(),
                                           rule,
                                           sumake.ignore_errors or ignore_errors))
        
    def AllTarget(self, targets = ''):
        if self.created_targets.has_key("all"):
            return
        self.created_targets["all"]=1

        self.writeln( self.project.getAllLine( targets ) )
        
        self.call_all_submake(None)
        
        self.writeln('')
        
        if 'nonobj' != self.project.getTargetType():
            if not self.created_targets.has_key(self.project.object_dir):
                self.created_targets[self.project.object_dir]=1
                self.writeln("%s:" % (self.project.object_dir))
                
                for new_dir in self.mkdir(self.project.object_dir, 1):
                    self.writeln("\t-@%s" % (new_dir))
                self.writeln('')
                
            self.writeln("all_objects: $(OBJS)")
            self.call_all_submake("all_objects")
            self.writeln('')
            
        
        
    def LinkTarget( self ):
        output_path = self.project.getOutputPath()
        
        objects = self.platform.form_var("OBJS")
                
        if self.project.getTargetType() != "lib":
            static_libs = self.platform.form_var("STATIC_LIBS")
            dynamic_libs = self.platform.form_var("DYNAMIC_LIBS")
        else:
            static_libs = ''
            dynamic_libs = ''

        self.writeln("%s: %s %s" % (output_path, objects, static_libs))

        if len(self.project.output_dir):
            for new_dir in self.mkdir(self.project.output_dir, 1):
                self.writeln("\t-@%s" % (new_dir))

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

                if self.platform.make_toc.cmd:
                    cmd_list.append( self.platform.make_toc.execute(output_path) )
            else:
                cmd_list = [ self.platform.link.execute(output_path,
                                                        objects,
                                                        static_libs,
                                                        dynamic_libs) ]
                
                                                        
        for cmd in cmd_list:
            self.writeln("\t%s" % (cmd))

            
    def MainTarget(self):
        if self.project.getTargetType() == '':
            return

        output_path = self.project.getOutputPath()
        
        self.AllTarget( output_path )

        ## linking
        if self.project.wantsLinking():
            self.LinkTarget()                

        self.sign( output_path )

        self.writeln('')
        
        ## object dependencies
        if len(self.project.object_dir):
            self.write_object_depends()
            
        self.CleanTarget()
        
        self.CopyTarget()
            
        self.DependTarget()


    def CleanTarget(self, targets = ""):
        """Make clean"""
        if self.created_targets.has_key("clean"):
            return
        self.created_targets["clean"]=1

        self.writeln("clean:")
            
        if targets == " ":
            targets = ""

        toclean = self.project.getCleanItems()

        if targets or toclean:
            if string.lower(os.path.basename(self.platform.make.cmd)) == 'nmake':
                limit=128
            else:
                limit=4096

            if hasattr(self.platform,"rmfile"):
                C="-"+self.platform.form_var("RM_FILE")
            else:
                C=self.platform.form_var("RM")
                
            n=0
            cmd=C
            if targets:
                cmd=cmd+" "+targets
                n=n+1
            for x in toclean:
                x=self.compat_quote([x])
                if n and len(cmd)+len(x)+1 >= limit:
                    self.writeln("\t%s" % cmd)
                    cmd=C
                n=n+1
                cmd=cmd + " " + x

            if n:
                self.writeln("\t%s" % cmd)

        self.call_all_submake("clean", 1)
        self.writeln('')


    def DependTarget(self):
        if self.created_targets.has_key("depend"):
            return
        self.created_targets["depend"]=1
        
        self.writeln("depend:")
        self.call_all_submake("depend")
        if len(self.project.sources):
            temp = self.platform.make_dep.execute(
                self.platform.form_var("SRCS"), self.project.object_dir)
            self.writeln("\t%s" % (temp))
        self.writeln('')



    def CopyTarget(self, path_list = []):
        if self.created_targets.has_key("copy"):
            return
        self.created_targets["copy"]=1

        
        deps = []
        allcopy = []
        path_list = path_list + self.project.copy_target_list
        for spath in path_list:
            dpath = os.path.join(
                self.project.target_dir, os.path.basename(spath))
            allcopy.append( ( spath, dpath ) )

        ## Create extra copies for debugging
        for dpath in self.project.debug_copies:
            spath = self.project.getOutputPath()
            dest = os.path.join(dpath, os.path.basename(spath))
            allcopy.append( ( spath, dest ) )

        spacecopy = []
        ## Write out all copy targets
        for (spath, dpath) in allcopy:
            if " " in dpath:
                spacecopy.append( (spath,dpath) )
                continue
            dir = os.path.dirname(dpath)
            self.writeln("%s: %s" % (dpath, spath))
            for l in self.mkdir(dir, 1):
                self.writeln("\t-@%s" % l)

            if os.path.basename(spath) == os.path.basename(dpath):
                if hasattr(self.platform,"rmfile"):
                    self.writeln('\t-%s "%s"' % (self.platform.rmfile.cmd, dpath))
                else:
                    self.writeln('\t%s "%s"' % (self.platform.rm.cmd, dpath))
                
            self.writeln('\t%s "%s" "%s"' % (self.platform.copy.cmd, spath, dpath))
            self.writeln("")
            deps.append(dpath)

        self.writeln("copy: %s" % string.join(deps," "))
        self.call_all_submake("copy")

        ## Write out copy commands with spaces in them
        for (spath, dpath) in spacecopy:
            dir = os.path.dirname(dpath)
            for l in self.mkdir(dir, 1):
                self.writeln("\t-@%s" % l)

            if os.path.basename(spath) == os.path.basename(dpath):
                if hasattr(self.platform,"rmfile"):
                    self.writeln('\t-%s "%s"' % (self.platform.rmfile.cmd, dpath))
                else:
                    self.writeln('\t%s "%s"' % (self.platform.rm.cmd, dpath))
                
            self.writeln('\t%s "%s" "%s"' % (self.platform.copy.cmd, spath, dpath))
        self.writeln('')

    def EmptyTarget(self):
        self.AssertTarget("all")
        self.writeln("all:")
        self.AssertTarget("depend")
        self.writeln("depend:")
        self.AssertTarget("clean")
        self.writeln("clean:")
        self.AssertTarget("dist")
        self.writeln("dist:")
        self.AssertTarget("copy")
        self.writeln("copy:")

#    def NolinkTarget( self ):
#        self.project.object_dir = ''
        
    def __init__(self, platform, project):
        umake_lib.Targets.__init__(self, platform, project)
        self.created_targets = {}

        self.makefile = project.pre_target_buff[:]

        if project.isstandard():
            self.writeln("__STANDARD__=YES")

        self.writeln("SRCROOT=%s" % project.src_root_path)


        ## Should this go in MainTarget as well??
        self.write_macros()

        ## Cruft to allow people to create their own makefiles...
        for targ in project.xtargets:
            silly=string.upper(targ[:1])+targ[1:]+"Target"
            makefile_generator.__dict__[silly](self)

        ## Really make a makefile!
        self.MainTarget()

        ## Add extra dependencies
        for file in project.file_dependencies.keys():
            for dep in project.file_dependencies[file]:
                self.writeln("%s: %s" % (file, dep))

        self.makefile.extend(project.post_target_buff)


        ## Avoid re-parsing the makefile if it was mostly hand-written
        mfile = string.join(self.makefile,'')
        if project.getTargetType():
            mfile = makefile.ParseMakefile(mfile)
            self.mfile=mfile

            ## do some basic checks, print warnings for now...
            ## this will be extended later to intelligently merge
            ## duplicate target names -JMP
            name_list = []
            for name in mfile.target_list:
                if name in name_list:
                    umake_lib.warning(
                        "makefile has duplicate target names=\"%s\"" % (name))
                else:
                    name_list.append(name)

            mfile = str(mfile)
            
        ## write out the makefile
        open(project.makefile_name, "w").write(str(mfile))

                
def make_makefile(platform, project):
    makefile_generator(platform, project)
    
    ## Pretend like everything went well
    return None

def compile():
    """This will eventually be the primary function for invoking make"""
    import shell
    shell.run("make")
