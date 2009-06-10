# -*- python -*-
#
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_symbianmmp.py,v 1.5 2006/09/21 18:59:57 damann Exp $
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
#  Contributor(s):  
#  
#  ***** END LICENSE BLOCK ***** 
#
"""Implement the Makefile back end for Umake.  This will generate a Symbian
bld.inf and MMP file to build targets."""

import bldreg
import makefile
import os
import re
import string
import umake
import umake_lib
import umake_makefile

mmp_files = []
bldinf_cfg_exports = []
bldinf_dist_exports = []
bldinf_armv5_dist_exports = []
bldinf_winscw_dist_exports = []

def convertFileToRelativePath(src_root_path, full_path):
    ##
    ##  If given a full path, the drive letter and full path
    ##  will be changed to a path reletive to the top level
    ##  source directory
    ##
    rel_path = ''

    ##
    ##  Strip off the base path and make it relative
    ##
    base_path = bldreg.get_value('build', 'path')
               
    tmpPath = string.split(full_path,base_path)

    ##
    ##  Check if there was a full path or just a filename
    ##
    if len(tmpPath) == 1:
        ##
        ##  Set the return value to the filename
        ##
        rel_path = str(tmpPath[0])
    else:
        ##
        ##  The full path has been removed.  The remaining path
        ##  is relative to the root path.  Add in the relative path
        ##  to the current directory
        ##
        tmpPath[0] = src_root_path
        tmpPath[1] = "." + tmpPath[1]
        rel_path = os.path.normpath(os.path.join(src_root_path, tmpPath[-1]))

    return rel_path

def convertPathToRelativePath(cwd_path, full_path):
    ##
    ##  If given a full path, the drive letter and full path
    ##  will be changed to a path reletive to the top level
    ##  source directory
    ##
    rel_path = ''

    ##
    ##  Strip off the base path and make it relative
    ##
    base_path = bldreg.get_value('build', 'path')
    tmpPath = string.split(full_path,base_path)

    rel_path = "%s%s" % (cwd_path,tmpPath[-1])

    return rel_path

def remove_duplicates(list):
    new_list = []
    for entry in list:
        if entry not in new_list:
            new_list.append(entry)

    return new_list

def checkForDistributionLibrary(pathed_lib, export_list):
    (lib_path,lib_filename) = os.path.split(pathed_lib)

    ##
    ##  Check for distribution libraries that need to be exported
    ##
    if ( re.search('datatype_dist', lib_path ) or re.search('protocol_dist', lib_path) ):
        export_list.append(pathed_lib)

        ##
        ##  Prepend dist_ to the filename to ensure unique name
        ##
        return "dist_%s" % lib_filename
    else:
        return lib_filename

class mmp_generator(umake_makefile.makefile_generator):

    def writeln(self, text):
        self.makefile.append(text+"\n")

    def AddMmpSource(self, project, src_file):
        pathed_file = convertFileToRelativePath(project.src_root_path, src_file)
        path, file = os.path.split(pathed_file)            
            
        if path == self.last_path:
            self.writeln("SOURCE %s" % file)
        else:
            if path == '':
                self.writeln("\nSOURCEPATH .")
            else:
                self.writeln("\nSOURCEPATH %s" % path)

            self.writeln("SOURCE %s" % file)
            self.last_path = path

    def generateMmp(self, platform, project):
        self.writeln("//")
        self.writeln("// Generated from %s, do not edit, do not commit to cvs!" % (project.umakefile_name ))
        self.writeln("//")
        self.writeln("#include <domain\osextensions\platform_paths.hrh>")
                   
        self.writeln("")
            
        ##
        ##  TARGET
        ##
        target_name = project.getOutputPath()
        self.writeln("TARGET %s" % (target_name))

        ##
        ##  TARGETTYPE
        ##
        if ( project.getTargetType() == "exe" and project.BuildOption('make-mmf') ):
            type = "plugin"
        else:
            type = project.getTargetType()

        self.writeln("TARGETTYPE %s" % (type))

        ##
        ##  Put the UID, VENDORID, and CAPABILITY into the MMP for TARGETTYPE 
        ##  of plugin and dll
        ##
        if ( type != "lib" ):
            ##
            ##  TARGETPATH
            ##
            ##  Write the dlls and plugins to the sys/bin directory
            ##  The path is relative to epoc32\release\platform\variant\z\
            ##
            self.writeln("TARGETPATH sys/bin")

            ##  UID
            ##
            self.writeln("UID 0x%x 0x%x" % 
                (project.symbianUtil.uid2, project.symbianUtil.uid3))

            self.writeln("")

            ##
            ##  VENDORID
            ##
            self.writeln("VENDORID %s" % project.symbianMmpUtils.vendorID)

            ##
            ##  CAPABILITY
            ##
            self.writeln("CAPABILITY %s" % project.symbianUtil.capabilities)
         
        self.writeln("")

        ##
        ##  Check for any resource files
        ##
        if project.symbianUtil.rss_files:
            ##
            ##  START RESOURCE
            ##
            rss_file = self.compat_quote(project.symbianUtil.rss_files)
            self.writeln("START RESOURCE %s" % rss_file)

            ##
            ##  TARGET
            ##
            ##  Target should be the project.target_name with rsc extention
            ##
            rss_target = project.target_name + '.rsc'
            self.writeln("TARGET %s" % rss_target)

            ##
            ##  END
            ##
            self.writeln("END")
            self.writeln("")

        ##
        ##  instruct an ARMV5 build to not build the project for the THUMB 
        ##  instruction set, but for the ARM instruction set.
        ##
        self.writeln("ALWAYS_BUILD_AS_ARM")
         
        self.writeln("")

        ##
        ##  DEFFILE
        ##
        if project.getTargetType() == "dll":
            ##
            ##  Create an entry for armv5 and winscw
            ##
            self.writeln("#if defined(MARM_ARMV5)")
            def_file = "%s_armv5.def" % (project.target_name)
            self.writeln("DEFFILE %s" % os.path.join(".",def_file))
            self.writeln("#else if defined(WINSCW)")
            def_file = "%s_winscw.def" % (project.target_name)
            self.writeln("DEFFILE %s" % os.path.join(".",def_file))
            self.writeln("#endif")
            self.writeln("")

            self.writeln("NOSTRICTDEF")
            self.writeln("")

        ##
        ##  OPTION
        ##
        ##  Print out options for both armv5 and winscw
        ##
        self.writeln("")
        self.writeln("OPTION %s" % project.symbianMmpUtils.armv5CompOptions)
        self.writeln("OPTION %s" % project.symbianMmpUtils.winscwCompOptions)
        self.writeln("")

        ## 
        ##  SOURCE
        ## 
        for source in project.sources:
            self.AddMmpSource(project, source)

        self.writeln("")

        if project.symbianMmpUtils.armv5Sources:
            self.writeln("#if defined(MARM_ARMV5)")
            for source in project.symbianMmpUtils.armv5Sources:
                self.AddMmpSource(project, source)
            self.writeln("\n#endif\n")

        if project.symbianMmpUtils.winscwSources:
            self.writeln("#if defined(WINSCW)")
            for source in project.symbianMmpUtils.winscwSources:
                self.AddMmpSource(project, source)
            self.writeln("\n#endif\n")

        ## 
        ##  USERINCLUDE
        ##  
        ##  Expand paths and check for duplicates
        ##  
        inc_list = []

        for inc in project.includes:
            pathed_inc = os.path.abspath(inc)
            rel_inc = convertPathToRelativePath(project.src_root_path, pathed_inc)
            inc_list.append(rel_inc)

        user_inc_list = remove_duplicates(inc_list)

        for x in user_inc_list:
            if os.path.exists(x):
                (drive, path) = os.path.splitdrive(x)
                self.writeln("USERINCLUDE %s" % path)

        self.writeln("")

        ##
        ##  SYSTEMINCLUDE
        ##
        ##  Add sections for all system includes and 
        ##  armv5 and winscw specific system includes
        ##
        
        self.writeln("MW_LAYER_SYSTEMINCLUDE")

        for x in project.symbianMmpUtils.systemIncludes:
            temp_path = '%s' % os.path.join(umake.GetSDKPath('SYMBIANSDK'), x)
            if os.path.exists(temp_path):
               (drive, path) = os.path.splitdrive(temp_path)
               self.writeln("SYSTEMINCLUDE %s" % path)

        self.writeln("\n#if defined(MARM_ARMV5)")
        for x in project.symbianMmpUtils.armv5SystemIncludes:
            temp_path = '%s' % os.path.join(umake.GetSDKPath('SYMBIANSDK'), x)
            if os.path.exists(temp_path):
                (drive, path) = os.path.splitdrive(temp_path)
                self.writeln("SYSTEMINCLUDE %s" % path)

        self.writeln("#else if defined(WINSCW)")
        for x in project.symbianMmpUtils.winscwSystemIncludes:
            temp_path = '%s' % os.path.join(umake.GetSDKPath('SYMBIANSDK'), x)
            if os.path.exists(temp_path):
                (drive, path) = os.path.splitdrive(temp_path)
                self.writeln("SYSTEMINCLUDE %s" % path)

        self.writeln("#endif")

        self.writeln("")

        ##
        ##  MACRO
        ##
        ##  Add sections for all defines ,armv5, and winscw specific defines
        ##
        self.writeln("MACRO %s" % (self.compat_quote(project.defines)))

        self.writeln("")

        self.writeln("#if defined(MARM_ARMV5)")
        self.writeln("MACRO %s" % (self.compat_quote(project.symbianMmpUtils.armv5Defines)))
        self.writeln("#else if defined(WINSCW)")
        self.writeln("MACRO %s" % (self.compat_quote(project.symbianMmpUtils.winscwDefines)))
        self.writeln("#endif")

        self.writeln("")

        if ( type != "lib" ):
            ##
            ##  STATICLIBRARY
            ##
            static_libs = self.project.libraries + self.project.libraries2 + \
                          self.project.local_libs + self.project.module_libs

            if static_libs:
                staticLibs = []
                
                ##
                ##  Remove duplicates to remove warnings
                new_static_libs = remove_duplicates(static_libs)

                ##
                ##  Strip off the path from the static lib list
                ##
                for pathed_lib in new_static_libs:
                    ##
                    ##  Check for distribution libraries that need to be exported
                    ##
                    lib_name = checkForDistributionLibrary(pathed_lib, bldinf_dist_exports)

                    staticLibs.append(lib_name)

                self.writeln("STATICLIBRARY %s" % (self.compat_quote(staticLibs)))

            if project.symbianMmpUtils.armv5Libraries:
                self.writeln("\n#if defined(MARM_ARMV5)")
                for x in project.symbianMmpUtils.GetArmv5Libraries():
                    lib_name = checkForDistributionLibrary(x, bldinf_armv5_dist_exports)
                    self.writeln("STATICLIBRARY %s" % lib_name)
                self.writeln("#endif")

            if project.symbianMmpUtils.winscwLibraries:
                self.writeln("\n#if defined(WINSCW)")
                for x in project.symbianMmpUtils.GetWinscwLibraries():
                    lib_name = checkForDistributionLibrary(x, bldinf_winscw_dist_exports)
                    self.writeln("STATICLIBRARY %s" % lib_name)
                self.writeln("#endif")

            ## LIBRARY
            libraries = project.dynamic_libraries + project.sys_libraries 

            new_libraries = remove_duplicates(libraries)

            self.writeln("")

            if libraries: 
                self.writeln("LIBRARY %s" % (self.compat_quote(new_libraries)))


    def __init__(self, platform, project, mmp_filename):
        umake_lib.Targets.__init__(self, platform, project)

        self.makefile = project.pre_target_buff[:]

        self.last_path = ''

        ##
        ##  Write out exports to global bld.inf file
        ##
        if project.symbianMmpUtils.mmpCfgExports:
            for export in project.symbianMmpUtils.mmpCfgExports:
                bldinf_cfg_exports.append(export)

        ## 
        ##  Generate the mmp file
        ##
        if project.target_name:
            self.generateMmp(platform, project)
            mfile = string.join(self.makefile,'')
            mfile = str(mfile)
            open(mmp_filename, "w").write(str(mfile))

            ##
            ##  Add mmp file to global list
            ##
            mmp_files.append("%s" % mmp_filename)


    def generateBldInf(self, project):
        """Create the bld.inf file for the project"""

        fil = open('bld.inf', 'w')

        ##
        ##  Set to the default platform list
        ##
        fil.write('PRJ_PLATFORMS\nDEFAULT\n')

        fil.write('\nPRJ_MMPFILES\n')
 
        unknown_mmp = []

        while mmp_files:
            mmp = mmp_files.pop(0);

            if os.path.isfile(mmp):
                ##
                ##  For platform lists other than default
                ##
                if project.symbianMmpUtils.platformExclusion:
                    for platEx in project.symbianMmpUtils.platformExclusion:
                        if platEx == 'ARMV5':
                            fil.write("#if !defined(%s)\n" % 'MARM_ARMV5')
                            fil.write("%s\n" % mmp)
                            fil.write("#endif")
                        elif platEx == 'WINSCW':
                            fil.write("#if !defined(%s)\n" % 'WINSCW')
                            fil.write("%s\n" % mmp)
                            fil.write("#endif")
                        else:
                            print "UMAKE_SYMBIANMMP:  Invalid Platform Exclusion!!!"
                else:
                    fil.write("%s\n" % mmp)
            else:
                unknown_mmp.append(mmp)
        ##
        ##  Add the MMP files not found back into the MMP list
        ##
        for entry in unknown_mmp:
            mmp_files.append(entry)

        ##
        ##  Determine if there are any mmp files in other directories
        ##
        other_dirs = []
        for sumake in project.submakes:
            dir = os.path.dirname(sumake.makefile())

            if len(dir) and dir != ".":
                if dir not in other_dirs:
                    other_dirs.append(dir)

        while other_dirs:
            dir = other_dirs.pop(0)
            fil.write('#include "%s"\n' % os.path.join(dir, "bld.inf"))

        fil.close()


def make_makefile(platform, project):
    """This will generate symbian mmp files instead of makefiles """

    mmp_filename = project.target_name + '.mmp'

    mmp_gen = mmp_generator(platform, project, mmp_filename)

    ##
    ##  If makefile name is Makefile, we have completed the makefile process
    ##  for multitargetmake and single target makes for each module
    ##
    ##  If this is the Umakefil in the src root path, generate the top level bld.inf
    ##
    cwd = convertFileToRelativePath(project.src_root_path, os.getcwd())

    if project.makefile_name == "Makefile":
        ##
        ##  If this is the Umakefil in the src root path, generate the top level bld.inf
        ##
        if cwd == '.':
            generate_top_level_bld_inf(project)
        else:
            mmp_gen.generateBldInf(project)

    ## Pretend like everything went well
    return None

def convertDistributionFilename(filename):
    ##  
    ##  Strip off drive letter and add dist_ to the lib name to avoid name conflicts
    ##  
    (export_path,export_filename) = os.path.split(filename)
    dest_filename = "dist_%s" % export_filename

    ##
    ##  Change the export filename to relative to group dir
    ##  under the source root
    ##
    src_file = convertFileToRelativePath("..", filename)

    ##
    ##  Check for the standard distribution.  If it is not there check
    ##  for platform dependent distributions in the source root
    ##
    if os.path.exists(src_file):
       armv5DistSrc  = src_file
       winscwDistSrc = src_file
    else:
       ##
       ##  Change the file path to include platform extention
       ##
       armv5DistSrc  = string.replace(src_file,'_dist', '_dist_armv5')
       winscwDistSrc = string.replace(src_file,'_dist', '_dist_winscw')

    return (armv5DistSrc, winscwDistSrc, dest_filename)

def generate_top_level_bld_inf(project):
    bld_inf_file = []

    group_dir   = os.path.join(project.src_root_path, "group")
    output_file = os.path.join(project.src_root_path, "group", "bld.inf")

    ##
    ##  Create group directory if it doesn't exist
    ##
    if not os.path.exists(group_dir):
        os.mkdir(group_dir)
       
    ##
    ##   Add supported platforms
    ##
    bld_inf_file.append('PRJ_PLATFORMS')
    bld_inf_file.append('DEFAULT\n')

    ##
    ##   Get the exports from the mmp creation
    ##
    bld_inf_file.append('PRJ_EXPORTS\n')

    ##
    ##  Get a relative base directory for the output
    ##
    tmpBasePath = '%s' % os.path.join(umake.GetSDKPath('SYMBIANSDK'), "epoc32\\release")
    (drive, basePath) = os.path.splitdrive(tmpBasePath)

    armv5UrelPath  = '%s\\armv5\\urel' % basePath
    armv5UdebPath  = '%s\\armv5\\udeb' % basePath
    winscwUrelPath = '%s\\winscw\\urel' % basePath
    winscwUdebPath = '%s\\winscw\\udeb' % basePath

    winscwCfgUrelPath = '%s' % os.path.join(basePath, "winscw\\urel\\z\\resource\\")
    winscwCfgUdebPath = '%s' % os.path.join(basePath, "winscw\\udeb\\z\\resource\\")

    ##
    ##  Setup the config file exports
    ##
    armv5UdebCfgs = []
    armv5UrelCfgs = []
    winscwUdebCfgs = []
    winscwUrelCfgs = []

    cfg_exports = remove_duplicates(bldinf_cfg_exports)

    while cfg_exports:
        tmp_cfg_file = cfg_exports.pop(0)

        (cfg_path,cfg_file) = os.path.split(tmp_cfg_file)

        ##
        ##  Change the export filename to relative to group dir
        ##  under the source root
        ##
        src_cfg_file = convertFileToRelativePath("..", tmp_cfg_file)

        armv5UdebCfgs.append('%s %s' % ( src_cfg_file, os.path.join(armv5UdebPath, cfg_file)))
        armv5UrelCfgs.append('%s %s' % ( src_cfg_file, os.path.join(armv5UrelPath, cfg_file)))
        winscwUdebCfgs.append('%s %s' % ( src_cfg_file, os.path.join(winscwCfgUdebPath, cfg_file)))
        winscwUrelCfgs.append('%s %s' % ( src_cfg_file, os.path.join(winscwCfgUrelPath, cfg_file)))

    ##
    ##  Setup the distribution file exports
    ##
    armv5UrelExportList  = []
    armv5UdebExportList  = []
    winscwUrelExportList = []
    winscwUdebExportList = []

    dist_exports = remove_duplicates(bldinf_dist_exports)
    armv5_dist_exports = remove_duplicates(bldinf_armv5_dist_exports)
    winscw_dist_exports = remove_duplicates(bldinf_winscw_dist_exports)

    while dist_exports:
        orig_file = dist_exports.pop(0)

        (armv5DistSrc, winscwDistSrc, dest_filename) = convertDistributionFilename(orig_file)

        armv5UrelExportList.append( '%s %s' % ( armv5DistSrc, os.path.join(armv5UrelPath, dest_filename) ) )
        armv5UdebExportList.append( '%s %s' % ( armv5DistSrc, os.path.join(armv5UdebPath, dest_filename) ) )

        winscwUrelExportList.append( '%s %s' % ( winscwDistSrc, os.path.join(winscwUrelPath, dest_filename) ) )
        winscwUdebExportList.append( '%s %s' % ( winscwDistSrc, os.path.join(winscwUdebPath, dest_filename) ) )

    while armv5_dist_exports:
        orig_file = armv5_dist_exports.pop(0)
        (armv5DistSrc, winscwDistSrc, dest_filename) = convertDistributionFilename(orig_file)

        armv5UrelExportList.append( '%s %s' % ( armv5DistSrc, os.path.join(armv5UrelPath, dest_filename) ) )
        armv5UdebExportList.append( '%s %s' % ( armv5DistSrc, os.path.join(armv5UdebPath, dest_filename) ) )

    while winscw_dist_exports:
        orig_file = winscw_dist_exports.pop(0)
        (armv5DistSrc, winscwDistSrc, dest_filename) = convertDistributionFilename(orig_file)

        winscwUrelExportList.append( '%s %s' % ( winscwDistSrc, os.path.join(winscwUrelPath, dest_filename) ) )
        winscwUdebExportList.append( '%s %s' % ( winscwDistSrc, os.path.join(winscwUdebPath, dest_filename) ) )

    ##
    ##  Write the armv5 and winscw export data to the bld.inf file
    ##
    bld_inf_file.append('#ifndef INTMM_CFG_EXPORT')

    for x in armv5UrelCfgs:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in armv5UdebCfgs:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in winscwUrelCfgs:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in winscwUdebCfgs:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('#endif')

    bld_inf_file.append('')

    for x in armv5UrelExportList:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in armv5UdebExportList:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in winscwUrelExportList:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    for x in winscwUdebExportList:
        bld_inf_file.append('%s' % x)

    bld_inf_file.append('')

    bld_inf_file.append('PRJ_MMPFILES')

    ##
    ##  Get the bld.inf file list from the uber Umakefil submake list
    ##
    other_dirs = []

    for subumake in project.submakes:
        dir = os.path.dirname(subumake.makefile())

        if len(dir) and dir != ".":
            if dir not in other_dirs:
                other_dirs.append(dir)

    while other_dirs:
        dir = other_dirs.pop(0)
        filename = os.path.join("..", dir, "bld.inf")
        bldinf_filepath = os.path.normpath(filename)
        bld_inf_file.append('#include "%s"' % bldinf_filepath)

    open(output_file,"w").write(string.join(bld_inf_file,"\n"))
