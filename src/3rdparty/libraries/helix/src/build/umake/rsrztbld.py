# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: rsrztbld.py,v 1.6 2006/04/24 23:34:04 jfinnecy Exp $ 
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
"""This is a fork of the consumer installer functions for the server.
This is no longer used on newer branches of the server, but RealServer 8.x
uses it."""

import os
import string
import struct
import ascript
import sysinfo

temp_dir_name = 'temp'
product_distcode = ''
product_executable = ''
mac_path_translation_table = string.maketrans('\\', ':')
unix_path_translation_table = string.maketrans('\\', '/')
g_rzt_apple_script = None

# By default we use Xtreme compression for all installers, and the rztbld
# module resets this to 1 so that RUP files use ICT
USE_XTR_COMPRESSION = 0

def Initialize(platform, project):
    if project.module_dir == '' or project.module_dir == ':':
        project.module_dir = project.target_name   

    if platform.name == 'mac':
        project.src_root_path = os.environ['SOURCE_ROOT']

    if (project.build_choices.count('release') > 0):
        debug_release = 'release'
    else:
        debug_release = 'debug'

    project.target_dir = os.path.join(project.src_root_path, debug_release)

    if platform.name == 'mac':
        global g_rzt_apple_script
        g_rzt_apple_script = ascript.CreateAppleScript('on all()')
        g_rzt_apple_script.Append(
            'verify path "%s"' % (project.target_dir))

    # Copy pncrt*.dll which is needed for pnpkg.exe on win32
    if platform.name == 'win32':
        if project.IsDefined("HELIX_CONFIG_RN_CRT"):
            project.writeln('\t' + platform.mkdir.execute(project.target_dir))

            pncrt_path = os.path.join(os.pardir, 'pncrt')
            if (project.build_choices.count('release') > 0):
                pncrt_path = os.path.join(pncrt_path, 'pncrt.dll')
            else:
                pncrt_path = os.path.join(pncrt_path, 'pncrtd.dll')
            project.writeln('\t' + platform.copy.cmd + ' ' +  pncrt_path + ' ' + project.target_dir)

        pnpkg_path = os.path.join(os.pardir, 'pnpkg', project.output_dir)
        project.writeln('\t' + platform.copy.cmd + ' ' +  pnpkg_path + ' ' + project.target_dir)


def CreateProductHeader(header_name, product_name, main_component):
    file = open(header_name + '.hdr', 'w')
    file.write('PRODUCT ' + product_name + '|' + main_component[0] + '\n')
    file.close()

def AddHeader(platform, project, header_name, component, add_options):

    specfile_path = os.path.join(project.src_root_path, component[2])
    verfile_path = os.path.join(project.src_root_path, component[1])

    if platform.type == 'unix':
        verfile_path = string.translate(verfile_path, unix_path_translation_table)
        specfile_path = string.translate(specfile_path, unix_path_translation_table)

    if (platform.type == 'unix'):
        index = string.rfind(specfile_path, '/')
    else:
        index = string.rfind(specfile_path, '\\')
    specfile_name = specfile_path[index+1:]

    if platform.name == 'mac':
        verfile_path = string.translate(verfile_path, mac_path_translation_table)
        specfile_path = string.translate(specfile_path, mac_path_translation_table)

    out_file = open(header_name + '.hdr', add_options)
    in_file = open(verfile_path, 'r')

    try:
        lines = in_file.readlines()
    except:
        raise VersionError, 'Error opening ' + verfile_path
        
    for line in lines:
        if string.find(line, '#define TARVER_STRING_VERSION') == 0:
            index = string.find(line, '"')
            line = line[ index + 1 : ]
            index = string.find(line, '"')
            line = line[ : index ]
            out_file.write('COMPONENT ' + component[0] + '|' + line + '|' + specfile_name + '\n')

    out_file.close()
    in_file.close()

    if project.target_name != project.module_dir:
        specfile_name = specfile_name + '_' + project.target_name

    out_file = open(specfile_name, 'w')
    in_file = open(specfile_path, 'r')

    try:
        lines = in_file.readlines()
    except:
        raise VersionError, 'Error opening ' + specfile_path

    for line in lines:


        ## Exclude other platform's commands
        include_line = 1
        if (string.find(line, '#') == 0):
            include_line = 0
            line = line[ 1 : ]
            index = string.find(line, '#')
            platform_index = string.find(line, platform.name)
            if(platform_index != -1 and platform_index < index):
                include_line = 1
                line = line[ index + 1 : ]
                line = string.lstrip(line)

        if (include_line == 1):
                output_line = ''

                ## Insert platform specific dll names
                index = string.find(line, 'dll_name(')
                while index != -1:
                    output_line = output_line + line[ : index ]
                    line = line[ index + 9 : ]
                    index = string.find(line, ')')
                    platform.versioning.version = ''
                    output_line = output_line + platform.versioning.create_dll_name(line[ : index ], line[ : index ])
                    platform.versioning.version = ''
                    line = line[ index + 1 : ]
                    index = string.find(line, 'dll_name(')

                output_line = output_line + line

                out_file.write(output_line)

    out_file.close()
    in_file.close()

def Cleanup(platform, project):
    ## CLEANUP TEMP
    if platform.name == 'win32' or platform.name == 'win16':
        project.writeln('\trm -f -r ' + temp_dir_name)

        ## Remove installer executable
        installer_exe = os.path.join(project.src_root_path, "rssetshell")
        installer_exe = os.path.join(installer_exe, project.output_dir + 's')
        installer_exe = os.path.join(installer_exe, "setup.exe")
        project.writeln('\trm -f ' + installer_exe)
    elif platform.type == 'unix':
        project.writeln('\trm -f -r ' + temp_dir_name)
    elif platform.name == 'mac':
        temp_dir = os.path.join(project.src_root_path, project.module_dir)
        temp_dir = os.path.join(temp_dir, temp_dir_name)
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                'tell application "Finder"',
                'with timeout of 99999 seconds',
                'if folder "%s" exists then' % (temp_dir),
                'delete folder "%s"' % (temp_dir),
                'set result to ""',
                'end if',
                'end timeout',
                'end tell'
                )


def AddComponent(platform, project, component, dirs, files, dlls):

    temp_dir = os.path.join(project.src_root_path, project.module_dir)
    copydir  = temp_dir
    temp_dir = os.path.join(temp_dir, temp_dir_name)

    ## Create directories for files to copy
    if platform.name == 'mac':
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
            'verify path "%s"' % (temp_dir))
    else:
        project.writeln('\t' + platform.mkdir.execute(temp_dir_name))

    for dir in dirs:
        if platform.name == 'mac':
            global g_rzt_apple_script
            g_rzt_apple_script.Append(
                'verify path "%s"' % (os.path.join(temp_dir, dir)))
        else:
            project.writeln('\t' + platform.mkdir.execute(os.path.join(temp_dir_name, dir)))

    str_copycommand = '\t' + platform.copy.cmd + ' '
    if (platform.name == 'win16'):
        str_copycommand = '\tdrpog /noerr_copy '
        project.writeln('\tdel CopyLogFile.txt')

    specfile_path = os.path.join(project.src_root_path, component[2])
    if platform.type == 'unix':
        specfile_path = string.translate(specfile_path, unix_path_translation_table)
        index = string.rfind(specfile_path, '/')
    else:
        index = string.rfind(specfile_path, '\\')

    specfile_name = specfile_path[index+1:]
    specfile_out_path = os.path.join(temp_dir, specfile_name)
    if project.target_name != project.module_dir:
        specfile_name = specfile_name + '_' + project.target_name

    ## Copy spec file of a component
    if platform.name == 'mac':
        spec_path = os.path.join(project.src_root_path, component[2])
        index = string.rfind(temp_dir, ':')
        spec_path = temp_dir[:index]
        spec_path = os.path.join(spec_path, specfile_name)
        corrected_file = string.translate(spec_path, mac_path_translation_table)
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                '-- Copy Files and Folders into Temporary Directory',
                'tell application "Finder"',
                'with timeout of 99999 seconds',
                'AkuaCopy file "%s" to folder "%s" with overwriting' % (corrected_file, temp_dir)
                )
    else:
        project.writeln(str_copycommand + specfile_name + ' ' + specfile_out_path)

    for dll in dlls:
        platform.versioning.version = ''
        dll_name = platform.versioning.create_dll_name(dll[0], dll[2])
        dll_location = os.path.join(dll[2], project.output_dir)
        dll_location = os.path.join(dll_location, dll_name)
        if dll[1] == '':
            dest = temp_dir
        else:
            dest = os.path.join(temp_dir, dll[1])
        if platform.name == 'mac':
            global g_rzt_apple_script
            g_rzt_apple_script.Append(
                    'AkuaCopy file "%s" to folder "%s" with overwriting' % (os.path.join(project.src_root_path, dll_location), dest)
                    )
        else:
            project.writeln(str_copycommand + os.path.join(project.src_root_path, dll_location) + ' ' + dest)

    for file in files:
        if file[1] == '':
            dest = temp_dir
        else:
            dest = os.path.join(temp_dir, file[1])
                
        if platform.name == 'mac':
            srcfile = string.translate(file[0], mac_path_translation_table)
            srcfile = project.src_root_path + ':' + srcfile 
            corrected_dest = string.translate(dest, mac_path_translation_table)
            if os.path.isdir(srcfile):  # copy entire folder
                global g_rzt_apple_script
                g_rzt_apple_script.Append(
                        'Copy folder "%s" to folder "%s"' % (srcfile, corrected_dest)
                        )
            else:
                global g_rzt_apple_script
                g_rzt_apple_script.Append(
                        'AkuaCopy file "%s" to folder "%s" with overwriting' % (srcfile, corrected_dest)
                        )
        else:
            if platform.type == 'unix':
                srcfile = string.translate(file[0], unix_path_translation_table)
                temp = str_copycommand + '"' + os.path.join(project.src_root_path, srcfile) + '"' + ' ' + dest
                project.writeln(string.replace(temp, '*', '"*"'))
            else:
                project.writeln(str_copycommand + '"' + os.path.join(project.src_root_path, file[0]) + '"' + ' ' + dest)

    if platform.name == 'mac':
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                'end timeout',
                'end tell'
                )

def Strip(platform, project, dirs):
    # Windows doesn't seem to have problems w/symbols, and the
    # cygnus strip command does strange things for me.
    if (platform.type != 'unix'):
        return 'no strip support for this platform'

    temp_dir = os.path.join(project.src_root_path, project.module_dir)
    temp_dir = os.path.join(temp_dir, temp_dir_name)

    for dir in dirs:
        target = os.path.join(temp_dir,dir[0])
        target = os.path.join(target,dir[1])
        if (platform.name == 'irix6'):
            project.writeln('\t' + 'strip -f ' + target)
        else:
            project.writeln('\t' + 'strip ' + target)

def CreateRZT(platform, project, rzt_file_name, bind_dirs):

    if platform.name == 'win32' or platform.name == 'win16':
        ##  MAKE ALL TEMP FILES WRITEABLE...
        project.writeln('\tattrib -r ' + os.path.join(temp_dir_name, '*.*') + ' /s')

        ##  RUN DISTCODE
        if product_distcode != '' and product_executable != '':
            project.writeln('\tdistcode ' + os.path.join(temp_dir_name, product_executable) + ' ' + product_distcode)

    if (platform.name == 'win32'):
        project.writeln('\tcd ' + temp_dir_name)
        
        for bind_dir in bind_dirs:
            project.writeln('\tcd ' + bind_dir)
            project.writeln('\tEDITBIN /BIND:PATH=.;..\\ *.dll')
            project.writeln('\tcd ' + os.pardir)
            
        project.writeln('\tcd ' + os.pardir)

    if (platform.name == 'win32'):
        pnpkg_path = project.target_dir
    else:
        pnpkg_path = os.path.join(project.src_root_path, 'pnpkg')
        pnpkg_path = os.path.join(pnpkg_path, project.output_dir)

    pnpkg_path = os.path.join(pnpkg_path, 'pnpkg')

    ##  RUN PNPKG
    if platform.name == 'win32' or platform.name == 'win16':
        project.writeln('\tcd ' + temp_dir_name)
        if USE_XTR_COMPRESSION == 0:
                project.writeln('\t' + os.path.join(os.pardir, pnpkg_path) + ' -e ' + os.path.join(os.pardir, rzt_file_name) + '.hdr -f ' + os.path.join(os.pardir, rzt_file_name) + '.rzt ' + '*.*')
        else:
                project.writeln('\t' + os.path.join(os.pardir, pnpkg_path) + ' -c xtr -e ' + os.path.join(os.pardir, rzt_file_name) + '.hdr -f ' + os.path.join(os.pardir, rzt_file_name) + '.rzt ' + '*.*')
        project.writeln('\tcd ' + os.pardir)

    elif platform.type == 'unix':
        ##  MAKE ALL TEMP FILES WRITEABLE...
        project.writeln('\t(cd ' + temp_dir_name + '; chmod -R a+x *)')

        ##  RUN DISTCODE
        # if product_distcode != '' and product_executable != '':
        #     project.writeln('\tdistcode ' + product_executable + ' ' + product_distcode)

        ##  RUN PNPKG
        path_to_pnpkg = os.path.join(os.pardir, os.pardir)
        path_to_pnpkg = os.path.join(path_to_pnpkg, 'pnpkg')
        path_to_pnpkg = os.path.join(path_to_pnpkg, project.output_dir)
        path_to_archive = os.path.join(os.pardir, 'archive.rzt')
        path_to_header =  os.path.join(os.pardir, rzt_file_name + '.hdr')

        project.writeln('\t(cd ' + temp_dir_name + '; ' + os.path.join(path_to_pnpkg,'pnpkg')+ ' -ve ' + \
            path_to_header + ' -f ' + path_to_archive + ' *)')

    elif platform.name == 'mac':
        # create a binary data file required by the PnPkg application
        # create a unique name for each .bin file       
        datafile_path = os.path.join(project.src_root_path, project.module_dir, rzt_file_name + '.bin')
        header_path = os.path.join(project.src_root_path, project.module_dir, rzt_file_name + '.hdr')
        archive_path = os.path.join(project.src_root_path, project.module_dir, rzt_file_name + '.rzt')
        temp_dir = os.path.join(project.src_root_path, project.module_dir, temp_dir_name)

        o = open(datafile_path, 'wb')

        items          = 0
        tempstr        = temp_dir + ':' + '\000'

        o.write(struct.pack('h', items+6))
        o.write('pnpkg\000')
        o.write('-e\000')
        o.write(header_path + '\000')
        o.write('-f\000')
        o.write(archive_path + '\000')
        o.write(tempstr)
        o.close()

        ## test to see if the pnpkg application built; if it didn't, then
        ## don't try to call it because a dialog will pop up and hang the
        ## remote build system; perhaps we should just sys.exit(1) at this
        ## point so the build system knows not to try and run the script --JMP
        if os.path.exists(pnpkg_path):
            global g_rzt_apple_script
            g_rzt_apple_script.Append(
                    '-- Make the Archive',
                    'tell application "%s"' % (pnpkg_path),
                    'run',
                    'with timeout of 99999 seconds',
                    'open "%s"' % (datafile_path),
                    'end timeout',
                    'quit',
                    'end tell')


def CreateInstaller(platform, project):

    rzt_src_path = os.path.join(project.src_root_path, project.module_dir)
    rzt_src_path = os.path.join(rzt_src_path, project.target_name + '.rzt')

    if platform.name == 'win32' or platform.name == 'win16':
        project.writeln('\tcd ' + os.pardir)
        project.writeln('\tcd ' + 'rssetshell')

        project.writeln('\tnmake clean')
        project.writeln('\tnmake depend')

        ## Copy RZT file
        project.writeln('\t' + platform.copy.cmd + ' ' + rzt_src_path + ' archive.rzt')

        project.writeln('\tnmake')

        project.writeln('\tcd ' + os.pardir)
        project.writeln('\tcd ' + project.module_dir)
    elif platform.type == 'unix':
        inst_module = 'v80_%s_%s.bin' % (sysinfo.id, project.target_name)
        
        setshell_base = os.path.join(project.src_root_path, 'rssetshell') 
        setshell_path = os.path.join(setshell_base, project.output_dir)

        ## Copy RZT file
        project.writeln('\tcp archive.rzt ' + setshell_path)

        ## Create a self extracting exe file containing the archive.rzt file
        project.writeln('\t(cd ' + setshell_path + '; python ../selfextr.py archive.rzt ../unix/archive.h)')
        project.writeln('\t(cd ' + setshell_base + '; make clean)')
        project.writeln('\t(cd ' + setshell_base + '; make)')
        project.writeln('\t(cd ' + setshell_path + '; cat setup archive.rzt > ' + inst_module + ')') 
        project.writeln('\t(cd ' + setshell_path + '; chmod a+x '+ inst_module +')')
    elif platform.name == 'mac':
        temp_dir = os.path.join(project.src_root_path, project.module_dir)
        temp_dir = os.path.join(temp_dir, temp_dir_name)
        setshell_path1 = os.path.join(project.src_root_path, 'rssetshell', project.output_dir, 'rssetshell')
        setshell_path2 = os.path.join(temp_dir, 'rssetshell')

        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                '-- Copy the archive',
                'tell application "Finder"',
                'with timeout of 99999 seconds',
                
                'try',
                'AkuaCopy file "%s" to folder "%s"' % (setshell_path1, temp_dir),
                'on error',
                'error "The SETSHELL application did not build"',
                'end try',
                
                'end timeout',
                'end tell',

                '-- Combine the archive and setshell application',
                'FileToResource "%s" ResourceType "ARCH" ResourceID 128 SourceFile "%s"' % (setshell_path2, rzt_src_path)
                )


def CopyRZT(platform, project, rzt_file_name):

    if platform.name == 'win32' or platform.name == 'win16':
        project.writeln('\t' + platform.copy.cmd + ' ' + rzt_file_name + '.rzt' + ' ' + project.target_dir)
    elif platform.type == 'unix':
        project.writeln('\t' + platform.copy.cmd + ' ' + rzt_file_name + '.rzt' + ' ' + project.target_dir)
    elif platform.name == 'mac':
        rzt_file_path = os.path.join(project.src_root_path, project.module_dir)
        rzt_file_path = os.path.join(rzt_file_path, rzt_file_name + '.rzt')
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                '-- Copy the archive',
                'tell application "Finder"',
                'with timeout of 99999 seconds',
                'AkuaCopy file "%s" to folder "%s"' % (rzt_file_path, os.path.join(project.src_root_path, project.output_dir)),
                'end timeout',
                'end tell'
                )


def CopyInstaller(platform, project):
    if platform.name == 'win32' or platform.name == 'win16':
        installer_exe = os.path.join(project.src_root_path, "rssetshell")
        installer_exe = os.path.join(installer_exe, project.output_dir + 's')
        installer_exe = os.path.join(installer_exe, "setup.exe")

        output_exe = os.path.join(project.target_dir, project.target_name + '.exe')

        project.writeln('\t' + platform.copy.cmd + ' ' + installer_exe + ' ' + output_exe)
    elif platform.type == 'unix':
        inst_module = 'v80_%s_%s.bin' % (sysinfo.id, project.target_name)
        
        installer_exe = os.path.join(project.src_root_path, 'rssetshell')
        installer_exe = os.path.join(installer_exe, project.output_dir)
        installer_exe = os.path.join(installer_exe, inst_module)

        output_exe = os.path.join(project.target_dir, inst_module)
        project.writeln('\tcp ' + installer_exe + ' ' + output_exe)
    elif platform.name == 'mac':
        # XXX: HACK! Choose name of final installer target.
        if project.target_name == 'playinst':
                installer_name = 'RealPlayer Installer'
        elif project.target_name == 'plusinst':
                installer_name = 'RealPlayerPlus Installer'
        elif project.target_name == 'encg2sdk':
                installer_name = 'RealProducer G2 SDK Installer'
        elif project.target_name == 'encinst':
                installer_name = 'RealProducer G2 Installer'
        elif project.target_name == 'pubinst':
                installer_name = 'RealProducer Plus G2 Installer'
        elif project.target_name == 'mppinst':
                installer_name = 'RealBandwidthSimulator Setup'
        elif project.target_name == 'preminst':
                installer_name = 'Real Premiere Plug-in Setup'
        elif project.target_name == 'geoprodinst':
                installer_name = 'RealProd Geo G2 Installer'
        elif project.target_name == 'geoprodplusinst':
                installer_name = 'RealProd Geo Plus G2 Installer'
        elif project.target_name == 'dsn6inst':
                installer_name = 'RealPlayer Installer'
        elif project.target_name == 'dsnpinst':
                installer_name = 'RealPlayerPlus Installer'
        elif project.target_name == 'mac_ah60':
                installer_name = '@HomePlayer Installer'
        elif project.target_name == 'mac_rm6u':
                installer_name = 'Education RM6U Installer'
        elif project.target_name == 'mac_rm6s':
                installer_name = 'ISP RM6S Installer'
        elif project.target_name == 'mac_rm6n':
                installer_name = 'Online RM6N Installer'
        elif project.target_name == 'mac_rm6z':
                installer_name = 'OEM RM6Z Installer'
        elif project.target_name == 'mac_rm6m':
                installer_name = 'Misc RM6M Installer'
        elif project.target_name == 'mac_rm6p':
                installer_name = 'Peripheral RM6P Installer'
        elif project.target_name == 'mac_rm6a':
                installer_name = 'Publishing RM6A Installer'
        elif project.target_name == 'mac_dsn6':
                installer_name = 'Disney DSN6 Installer'        
        elif project.target_name == 'mac_ns60':
                installer_name = 'Netscape NS60 Installer'      
        elif project.target_name == 'mac_aol6':
                installer_name = 'AOL AOL6 Installer'
        elif project.target_name == 'mac_elm6':
                installer_name = 'Earthlink ELM6 Installer'
        elif project.target_name == 'iw60inst':
                installer_name = 'IWorld IW6M Installer'
        elif project.target_name == 'mac_rm6c':
                installer_name = 'Generic RM6C Installer'                               
        elif project.target_name == 'nscpinst':
                installer_name = 'Netscape NS60 Installer'      
        elif project.target_name == 'mac_uvom':
                installer_name = 'UVOM Installer'                               
        elif project.target_name == 'rn7ninst':
                installer_name = 'RealPlayer Installer'                         
        elif project.target_name == 'stubinst':
                installer_name = 'RealStub Installer'                           
        elif project.target_name == 'alchinst':
                installer_name = 'Alchemy Installer'                            
        elif project.target_name == 'geminst':
                installer_name = 'Gemini Installer'                             
        elif project.target_name == 'aol7inst':
                installer_name = 'AOL7 Installer'
        elif project.target_name == 'mac_rm7u':
                installer_name = 'Education RM7U Installer'
        elif project.target_name == 'mac_rm7s':
                installer_name = 'ISP RM7S Installer'
        elif project.target_name == 'mac_rm7n':
                installer_name = 'Online RM7N Installer'
        elif project.target_name == 'mac_rm7z':
                installer_name = 'OEM RM7Z Installer'
        elif project.target_name == 'mac_rm7m':
                installer_name = 'Misc RM7M Installer'
        elif project.target_name == 'mac_rm7p':
                installer_name = 'Peripheral RM7P Installer'
        elif project.target_name == 'mac_rm7a':
                installer_name = 'Publishing RM7A Installer'                            
        else:
                installer_name = 'UNKNOWN INSTALLER'

        setshell_path = os.path.join(project.src_root_path, project.module_dir, temp_dir_name, 'rssetshell')
        installer_path = os.path.join(project.src_root_path, project.module_dir, temp_dir_name, installer_name)

        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                '--Rename the installer, copy it out of the tempdir, and dispose of the tempdir',
                'tell application "Finder"',
                'with timeout of 99999 seconds',
                'set name of file "%s" to "%s"' % (setshell_path, installer_name),
                'if file "%s" exists then' % (os.path.join(project.src_root_path, project.output_dir, installer_name)),
                'delete file "%s"' % (os.path.join(project.src_root_path, project.output_dir, installer_name)),
                'end if',
                'AkuaCopy file "%s" to folder "%s"' % (installer_path, os.path.join(project.src_root_path, project.output_dir)),
                'end timeout',
                'end tell'
                )


def Terminate(platform, project):
    if platform.name == 'mac':
        global g_rzt_apple_script
        g_rzt_apple_script.Append(
                'end all',
                'return all()'
                )
        
        project.append_script(g_rzt_apple_script)
        
        # free the AppleScript by de-referencing it
        g_rzt_apple_script = None
