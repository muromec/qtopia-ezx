# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: macuber.py,v 1.3 2006/04/24 23:34:02 jfinnecy Exp $ 
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
import ascript
import os
import outmsg
import string
import glob

def generate_uberxml(build_list, module_id_build_dict, uberxml_output_name, uber_dir):
        tag1 = '#TARGET#'
        tag2 = '#TARGETORDER#'
        tag3 = '#GROUP#'
        tag4 = '#MAKE_ALL_SUBTARGETS#'
        
        dependencies_section_empty = '<SUBTARGETLIST></SUBTARGETLIST>'
        
        uber_template = GetUberTemplate()
        
        old_dir = os.getcwd()
        
        # make a list of the module names we're building; we'll exclude
        # any dependencies that are not in the list
        
        modules_built_id_list = [ x.id for x in build_list ]
        
        # scoop up the uber text from each directory, add the depend
        
        success_list = []
        failure_list = []
        module_targetname_dict = {} # by module.id's, the names of the targets built by this module
        all_module_targetnames_list = []
        
        for module in build_list:
                # outmsg.send("module id: %s" % ( module.id ))
                # outmsg.send("modules_dependancy_id_list: %s" % ( string.join(module.dependancy_id_list, ',')))
                # outmsg.send("module_build_list: %s" % ( string.join( [x for x in module_id_build_dict[module.id]] , ',')))
                
                # move to the module's project_xml directory
                
                module_dir = os.path.join(old_dir, module.name)
                project_xml_dir = os.path.join(module_dir, "project_xml")
                
                if not os.path.isdir(project_xml_dir):
                        outmsg.send("No project_xml directory found for module %s" % (module.name))
                        failure_list = failure_list + [module.name]
                        continue
                
                os.chdir(project_xml_dir)
                
                # find all uber files we need to incorporate
                
                uber_file_names = glob.glob("*_uber.xml")
                
                if (len(uber_file_names) < 1):
                        outmsg.send("No uber xml found for module %s" % (module.name))
                        failure_list = failure_list + [module.name]
                
                for uber_file_name in uber_file_names:
                        # outmsg.send("Extracting uber-project XML from %s for module %s" % (uber_file_name, module.name))
                        
                        # switch to the module directory and read in the module's uber XML data
                        try:
                                uber_file = open(uber_file_name, 'r')
                                uber_text = uber_file.read()
                                uber_file.close()
                                
                        except:
                                outmsg.send("==> Cannot read from %s for module %s" % (uber_file_name, module.name))
                                continue
                        
                        success_list = success_list + [module.name]
                        
                        try:
                                (uber_target_name, uber_text_1, uber_text_2, uber_text_3, uber_output_path) = string.split(uber_text, "<<>>")

                        except:
                                outmsg.send("==> Cannot parse %s from module %s for <<>> sections" % (uber_file_name, module.name))
                                continue
                        
                        uber_target_name = string.strip(uber_target_name)
                        
                        # store the target name or names for this module for later use
                        if not module_targetname_dict.has_key(module.id):
                                module_targetname_dict[module.id] = []
                        module_targetname_dict[module.id] = module_targetname_dict[module.id] + [uber_target_name]
                        
                        # also add this as a subtarget for the Make All target
                        all_module_targetnames_list.append(uber_target_name)
                        
                        # now we look for the <SUBTARGETLIST></SUBTARGETLIST> in the target section of the
                        # module's XML, and replace it with a list of all actual dependencies
                        
                        # make a list containing a targetname section for each dependency (only for
                        # dependencies built, to avoid things like include)
                        
                        # unfortunately, module.dependancy_id_list only has the modules listed in the bif,
                        # which could include name_only targets that are not really built
                        
                        # our subtarget list is the subtarget names for the build list for this modules,
                        # but only the ones actually built and not this one
                        subtarget_list = []
                        for subtarget_module in module_id_build_dict[module.id]:
                                if (subtarget_module in modules_built_id_list) and (subtarget_module != module.id):
                                        if module_targetname_dict.has_key(subtarget_module):
                                                subtarget_list = subtarget_list + module_targetname_dict[subtarget_module]
                                        else:
                                                subtarget_list = subtarget_list + [subtarget_module]
                                                outmsg.send("Subtarget_module name not found: %s" % (subtarget_module))
                        
                        # outmsg.send("  actual subtarget_list: %s" % ( string.join(subtarget_list, ',')))

                        dependencies_section_xml = MakeDependenciesSubtargetListXML(subtarget_list)
                        
                        # make a subtarget list containing our dependencies list, and replace the empty one
                        # in the template with ours
                        
                        uber_text_1 = string.replace(uber_text_1, dependencies_section_empty, dependencies_section_xml, 1)
                        
                        # if an empty subtarget list is left in the template, remove it
                        uber_text_1 = string.replace(uber_text_1, dependencies_section_empty, "", 1)
                        
                        # finally, follow the target, targetorder, and group placeholders in the top level
                        # template with the data for this module
                        
                        template1 = string.replace(uber_template, tag1, tag1 + '\n' + uber_text_1, 1)
                        template2 = string.replace(template1, tag2, tag2 + '\n' + uber_text_2, 1)
                        uber_template = string.replace(template2, tag3, tag3 + '\n' + uber_text_3, 1)
                        
        # if we found any uber.xml files, generate the master uber.xml file
        
        if len(success_list) > 0:
                
                outmsg.send("Extracted uber-project XML from %s" % (string.join(success_list, ", ")))
                
                # insert the Make All subtarget list
                make_all_dependencies_section_xml = MakeDependenciesSubtargetListXML(all_module_targetnames_list)
                uber_template = string.replace(uber_template, tag4, make_all_dependencies_section_xml, 1)
                
                # remove the placeholder tags from the template
                uber_template = string.replace(uber_template, tag1, "", 1)
                uber_template = string.replace(uber_template, tag2, "", 1)
                uber_template = string.replace(uber_template, tag3, "", 1)
                
                outmsg.send("Writing %s" % (uberxml_output_name))
                
                # write out the file into an uber directory in the source directory
                try:
                        if not os.path.isdir(uber_dir):
                                os.mkdir(uber_dir)
                        
                        os.chdir(uber_dir)
                        
                        # delete any old xml file with the same name as our uber
                        if os.path.isfile(uberxml_output_name):
                                os.remove(uberxml_output_name)
                        
                        uber_template_outfile = open(uberxml_output_name, 'w')
                        uber_template_outfile.write(uber_template)
                        uber_template_outfile.close()
                except IOError:
                        outmsg.send( "*** Failed to write file %s" % (uberxml_output_name) )
        
        if len(failure_list) > 0:
                outmsg.send("Failed to find uber-project XML from %s" % (string.join(failure_list, ", ")))
        
        os.chdir(old_dir)
        return

def MakeDependenciesSubtargetListXML(subtarget_list):
        # given a list of modules, make a <SUBTARGETLIST> XML section
        
        subtarget_list = [ MakeDependencySubtargetEntry(x) for x in subtarget_list ]
        subtarget_list_string = string.join(subtarget_list, '\n')
        
        full_string = '<SUBTARGETLIST>' + subtarget_list_string + '</SUBTARGETLIST>'
        return full_string


def MakeDependencySubtargetEntry(module_name):
        # given a module name, make a single <SUBTARGET><TARGETNAME> entry for the module name
        
        dependencies_entry = '<SUBTARGET><TARGETNAME>' + module_name + '</TARGETNAME></SUBTARGET>\n'
        return dependencies_entry


def GetUberTemplate():
        # this returns the outermost XML template for the uber project XML
        return """
<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<?codewarrior exportversion="1.0.1" ideversion="4.2" ?>

<PROJECT>
    <TARGETLIST>
#TARGET#
    <TARGET><NAME>Make All</NAME>#MAKE_ALL_SUBTARGETS#</TARGET>
    </TARGETLIST>

    <TARGETORDER>
#TARGETORDER#
      <ORDEREDTARGET><NAME>Make All</NAME></ORDEREDTARGET>
    </TARGETORDER>

    <GROUPLIST>
#GROUP#
    </GROUPLIST>

</PROJECT>
"""



def build_uberproject(uber_xml_path, uber_project_path, script_path):
        
        outmsg.send("Creating %s" % (os.path.basename(script_path)))
        
        script = ascript.CreateAppleScript()
        
        # this needs better handling/reporting of build errors
        script.Append(
                'tell application %s' % (os.environ["BUILD_SW"]),
                '  with timeout of 99999 seconds',
                '    try',
                '      make new (project document) as "%s" with data ("%s")' % (uber_project_path, uber_xml_path),
                '      set cwErrorList to Make Project with ExternalEditor',
                '      Close Project',
                '    on error errText number errnum',
                '      return errText & return',
                '    end try',
                '  end timeout',
                'end tell')
        script.CompileAndSave(script_path)
        
        if 0: # set to 1 to actually make the uber project
                script.CompileAndExecute()


def generate_ubersyms(build_list, uberxml_output_name, uber_dir):
        
        sym_folder_name = uberxml_output_name + " xSYMs"
        old_dir = os.getcwd()
        
        # make a list of the module names we're building; we'll exclude
        # any dependencies that are not in the list
        
        modules_built_id_list = [ x.name for x in build_list ]
        
        # look for sym files in the debug directory of each module
        
        sym_file_paths = []
        outmsg.send("Looking for sym files in %s" % (string.join(modules_built_id_list, ", ")))
        for module_name in modules_built_id_list:
                
                module_dir = os.path.join(old_dir, module_name)
                module_debug_dir = os.path.join(module_dir, "debug")
                if os.path.isdir(module_debug_dir):
                        os.chdir(module_debug_dir)
                        
                        # find all sym files, and add the paths to those files to our sym_file_paths list
                        sym_file_names = glob.glob("*.xSYM")
                        sym_file_paths = sym_file_paths + [ os.path.join(module_debug_dir, x) for x in sym_file_names ]
        
        if len(sym_file_paths) < 1:
                outmsg.send("No xSYM files found")
                os.chdir(old_dir)
                return
        
        # now make a folder containing aliases to the found sym files           
        if not os.path.isdir(uber_dir):
                os.mkdir(uber_dir)
        
        syms_dir = os.path.join(uber_dir, sym_folder_name)
        if not os.path.isdir(syms_dir):
                os.mkdir(syms_dir)
        
        os.chdir(syms_dir)
        
        script = ascript.CreateAppleScript()
        
        for alias_target_path in sym_file_paths:
                
                (target_dir, target_name) = os.path.split(alias_target_path)
                alias_full_path = os.path.join(syms_dir, target_name)
                
                script.Append(
                        'tell application "Finder"',
                        '  try',
                        '    if not (file "%s" exists) then' % (alias_full_path),
                        '       make new alias file to (file "%s") at (folder "%s") with properties {name:"%s"}' 
                                        % (alias_target_path, syms_dir, target_name),
                        '    end if',
                        '   on error',
                        '  end try',
                        'end tell')
        
        script_path = os.path.join(syms_dir, "Make Symfile Aliases Script")
        script.CompileAndExecute()
        
        os.chdir(old_dir)

