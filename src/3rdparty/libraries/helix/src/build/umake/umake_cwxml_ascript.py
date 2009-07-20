# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_cwxml_ascript.py,v 1.8 2006/06/19 23:11:32 jfinnecy Exp $ 
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
import bldreg

import log
log.debug( 'Imported: $Id: umake_cwxml_ascript.py,v 1.8 2006/06/19 23:11:32 jfinnecy Exp $' )

import umake_ascript
from umake_ascript import *


## This function has a large portion of shared code with
## umake_ascript.py, should probably be made a into a class..
## -Hubbe
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
    if project.project_name == '':
        mprj.project_file = "%s.prj" % (mprj.target_name)
        mprj.cwtarget_name = "%s" % (mprj.target_name)
        mprj.cwtarget_name = "%s" % ("kikkipoo")
    else:
        mprj.project_file = "%s.prj" % (project.project_name)
        mprj.cwtarget_name = "%s" % (project.project_name)

    makefile = os.path.join(project.module_directory(),
                            project.makefile_name)

    mprj.cwtarget_name = declaw_name(makefile)

    mprj.project_file_path = os.path.join(os.getcwd(), mprj.project_file)

    ## project data foldername/folder path
    mprj.project_data = "%s Data" % (mprj.target_name)
    mprj.project_data_path = "%s" % os.path.join(os.getcwd(), mprj.project_data)

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

        if lib_name not in mprj.library_list:
            mprj.library_list.append(lib_name)
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
        mprj.source_list.append(mprj.export_file)

    ## customize the "PPC Project", "PPC PEF" panel, setting
    ## target output and type
    ppc_project = mprj.preferences["PPC Project"]
    ppc_pef = mprj.preferences["PPC PEF"]
    
    mprj.output_name = project.OutputName()
        
    ## set target name
    if ppc_project["MWProject_PPC_outfile"] == "":
        ppc_project["MWProject_PPC_outfile"] = "%s" % (mprj.output_name)
    ppc_pef["MWPEF_fragmentname"] = ppc_project["MWProject_PPC_outfile"]
    
    ## targe type/output file type
    if mprj.target_type == "lib":
        ppc_project["MWProject_PPC_type"] = "Library"
        ppc_project["MWProject_PPC_filetype"] = "????"
        ppc_pef["MWPEF_exports"] = "None"
    elif mprj.target_type == "exe":
        ppc_project["MWProject_PPC_type"] = "Application"
        ppc_project["MWProject_PPC_filetype"] = "APPL"
        ppc_pef["MWPEF_exports"] = "None"
    elif mprj.target_type == "dll":
        ppc_project["MWProject_PPC_type"] = "SharedLibrary"

        # mayhave been over-ridden by netscape plugin's pcf
        if ppc_project["MWProject_PPC_filetype"] == "":
            ppc_project["MWProject_PPC_filetype"] = "shlb"
    
        ## if the target type is a DLL, check for a export file in the
        ## listed sources
        ##
        use_export_file = 0
        
        for source in mprj.source_list:
            if string.lower(source[-4:]) == ".exp":
                use_export_file = 1
                break

        if use_export_file:
            ppc_pef["MWPEF_exports"] = "PragmaAndFile"
        else:
            ppc_pef["MWPEF_exports"] = "Pragma"


    ## tweak the PPC Linker settings
    ppc_linker = mprj.preferences["PPC Linker"]

    if mprj.target_type == "lib" or mprj.target_type == "dll":
        if not ppc_linker.has_key("MWLinker_PPC_initname"):
            ppc_linker["MWLinker_PPC_initname"] = "__initialize"
        if not ppc_linker.has_key("MWLinker_PPC_termname"):
            ppc_linker["MWLinker_PPC_termname"] = "__terminate"
        if not ppc_linker.has_key("MWLinker_PPC_mainname"):
            ppc_linker["MWLinker_PPC_mainname"] = ""
    elif mprj.target_type == "exe":
        if not ppc_linker.has_key("MWLinker_PPC_initname"):
            ppc_linker["MWLinker_PPC_initname"] = ""
        if not ppc_linker.has_key("MWLinker_PPC_termname"):
            ppc_linker["MWLinker_PPC_termname"] = ""
        if not ppc_linker.has_key("MWLinker_PPC_mainname"):
            ppc_linker["MWLinker_PPC_mainname"] = "__start"

    return mprj
    
        

## replace the text between <VALUE> and </VALUE>, just after the key, with theValue
## eg. <SETTING><NAME>MWProject_PPC_outfile</NAME><VALUE>pndebug.lib</VALUE></SETTING>

def SetPreferenceValue(theText, theKey, newValue):
    d = "<SETTING><NAME>" + theKey + "</NAME><VALUE>"
    start = string.index(theText, d)
    end = string.index(theText, "</VALUE>", start + len(d))
    return theText[:start+len(d)]+newValue+theText[end:]
    
## this function is used to set file list items, user access paths, group
## list items, etc.', replaces an instance of theSearchString with an XML
## block for each value in theValues', theTemplate describes the XML block;
## THE_VALUE# in the template is replaced with each item in theValues',
## theValuess is a comma delimited list of values like files,user access paths',

def SetPreferenceBlock(theText, theValues, theTemplate, theSearchString):
    formattedValues = SetPrefBlockValues(theTemplate, theValues)
    return string.replace(theText, theSearchString, formattedValues)

def SetPrefBlockValues(theTemplate, theValues):
    ret = ""
    for aValue in string.split(theValues,","):
        ret = ret + string.replace(theTemplate,"#THE_VALUE#", aValue)
    return ret

def SetAccessPathBlock(theText, paths, recursives, origins, theSearchString):
    gAccessPathBlockString="""
                   <SETTING>
                       <SETTING><NAME>SearchPath</NAME>
                           <SETTING><NAME>Path</NAME><VALUE>#PATH#</VALUE></SETTING>
                           <SETTING><NAME>PathFormat</NAME><VALUE>MacOS</VALUE></SETTING>
                           <SETTING><NAME>PathRoot</NAME><VALUE>#PathRoot#</VALUE></SETTING>
                       </SETTING>
                       <SETTING><NAME>Recursive</NAME><VALUE>#Recursive#</VALUE></SETTING>
                       <SETTING><NAME>FrameworkPath</NAME><VALUE>#Framework#</VALUE></SETTING>
                       <SETTING><NAME>HostFlags</NAME><VALUE>All</VALUE></SETTING>
                   </SETTING>
"""
    formattedValues = ""
    i = 0;
    for path in paths:
        tmp=gAccessPathBlockString
        if os.path.isabs(path) and path[0] != '#':
            tmp=string.replace(tmp, "#PathRoot#", "Absolute")

            ## Required ugly KLUGE
            if os.environ.get("FAKE_MAC","NO") == "YES":
                import posixpath
                path=posixpath.normpath(path)
                tmp=string.replace(tmp, "MacOS", "Unix")
        
        # set the path    
        tmp=string.replace(tmp, "#PATH#", path)

        # set the path root
        if theSearchString == "#USER_SEARCH_PATHS#":
            # to be safe and compatible -- ignore recursive and origin flags
            tmp=string.replace(tmp, "#PathRoot#", "Project")
            tmp=string.replace(tmp, "#Recursive#", "false")
            tmp=string.replace(tmp, "#Framework#", "false")
        else:
            # to be safe and compatible -- ignore recursive and origin flags UNLESS we have a System origin
            if ((len(recursives) == len(paths)) and (len(origins) == len(paths)) and (string.find(origins[i], "system") >= 0)):
                # system framework... honor the origin, and recursive
                tmp=string.replace(tmp, "#PathRoot#", "OS X Volume")
                tmp=string.replace(tmp, "#Framework#", "true")
                if (string.find(recursives[i], "true") >= 0):
                    tmp=string.replace(tmp, "#Recursive#", "true")
                else:
                    tmp=string.replace(tmp, "#Recursive#", "false")
            else:
                # not a system framework... business as usual
                tmp=string.replace(tmp, "#PathRoot#", "CodeWarrior")
                tmp=string.replace(tmp, "#Recursive#", "true")
                tmp=string.replace(tmp, "#Framework#", "false")
       
        formattedValues = formattedValues + tmp
        i = i+1
        
    return string.replace(theText, theSearchString, formattedValues)

def SetFileListBlock(theText, theValues, theSearchString, projectType, fileTypes):
    gFileBlockString = """
                <FILE>
                    <PATHTYPE>Name</PATHTYPE>
                    <PATH>#THE_VALUE#</PATH>
                    <PATHFORMAT>MacOS</PATHFORMAT>
                    <FILEKIND>#FileKind#</FILEKIND>
                    <FILEFLAGS>#FileFlags#</FILEFLAGS>
                </FILE>
"""

    formattedValues=SetPrefBlockValues(gFileBlockString, theValues)
    formattedValues=string.replace(formattedValues, "#FileKind#", projectType)
    formattedValues=string.replace(formattedValues, "#FileFlags#", fileTypes)
    return string.replace(theText, theSearchString, formattedValues)

def SetGroupBlock(theText, theValues, theSearchString, targetName):

    gGroupBlockString= """
               <FILEREF>
               <TARGETNAME>#target#</TARGETNAME>
               <PATHTYPE>Name</PATHTYPE>
               <PATH>#THE_VALUE#</PATH>
               <PATHFORMAT>MacOS</PATHFORMAT>
               </FILEREF>
"""

    formattedValues=SetPrefBlockValues(gGroupBlockString, theValues)
    formattedValues=string.replace(formattedValues, "#target#", targetName)
    return string.replace(theText, theSearchString, formattedValues)


def SetFrameworkBlock(theText, theValues, theSearchString):

    gFrameworkBlockString="""
                <FRAMEWORK>
                    <FILEREF>
                        <PATHTYPE>Name</PATHTYPE>
                        <PATH>#THE_VALUE#.framework</PATH>
                        <PATHFORMAT>MacOS</PATHFORMAT>
                    </FILEREF>
                    <DYNAMICLIBRARY>#THE_VALUE#</DYNAMICLIBRARY>
                </FRAMEWORK>
"""

    formattedValues=SetPrefBlockValues(gFrameworkBlockString, theValues)
    return string.replace(theText, theSearchString, formattedValues)



def WriteProjectSettingsXML(mprj, templateName):
    """ Writes out the Applescript which writes the <SETTINGS> section of
    the CodeWarrior XML. This includes all the CW project preferences,
    source file list and link order.  This function is used to write
    out the settings for 'project.xml' and 'project_uber.xml' """

    template_filename = os.path.join(os.environ['BUILD_ROOT'],"bin","mac", templateName)
    template_text = open(template_filename,"r").read()
                 
    ## set access paths
    user_list = []
    if templateName == "project_uber.xml":  
        # uber can't have local paths eg. ':', ':pub:'
        # they must be like '::pndebug:', '::pndebug:pub:'
        module_path = os.getcwd()
        src_root_path = macpath.normpath(os.path.join(module_path,mprj.project.src_root_path))+":"

        
        for (path, recursive, origin) in mprj.user_paths:
            umake_lib.debug("USER_PATH: %s => (%s)" %
                            (repr(path),
                             repr(mprj.project.src_root_path)))

            path = macpath.normpath(macpath.join(module_path, path))
            if path[:len(src_root_path)] == src_root_path:
                path = "#SRC_ROOT_PATH#" + path[len(src_root_path):]

            umake_lib.debug("USER_PATH: => %s (%s)" %
                            (repr(path),
                             repr(src_root_path)))

            user_list.append(path)
    else:
        for (path, recursive, origin) in mprj.user_paths:
            path = macpath.normpath(path)
            user_list.append(path)

    ## Set CodeWarrior prefs
    empty_list = []
    template_text = SetAccessPathBlock(template_text,
                                       user_list,
                                       empty_list,
                                       empty_list,
                                       "#USER_SEARCH_PATHS#")

    system_list = []
    recursive_list = []
    origin_list = []
    for (path, recursive, origin) in mprj.system_paths:
        system_list.append(path)
        recursive_list.append(recursive)
        origin_list.append(origin)

    template_text = SetAccessPathBlock(template_text,
                                       system_list,
                                       recursive_list,
                                       origin_list,
                                       "#SYSTEM_SEARCH_PATHS#")

    ## add files
    file_list = string.join(mprj.source_list, ',')
    source_dir,output_dir = macpath.split(mprj.output_dir_path)
    
    target_type = ''
    if output_dir == "debug":
        target_type = "debug"

    template_text = SetFileListBlock(template_text,
                                     file_list,
                                     "#TEXT_FILE_LIST#",
                                     "Text",
                                     target_type)

    source_list = []
    for file_name in mprj.source_list:
        source_list.append(file_name)

    if len(mprj.library_list):
        library_list = string.join(mprj.library_list, ',')
        template_text = SetFileListBlock(template_text,
                                         library_list,
                                         "#LIB_FILE_LIST#",
                                         "Library",
                                         "")

        # add libs to source list since they need to be
        # included with groups, link order iterms
        for library in mprj.library_list:
            lib_path, lib_name = os.path.split(library)
            if lib_name not in mprj.source_list:
                source_list.append(lib_name)
    else:
        template_text=string.replace(template_text, "#LIB_FILE_LIST#", "")
        
    # link order
    file_list = string.join(source_list, ',')

    gLinkOrderBlockString="""
                <FILEREF>
                    <PATHTYPE>Name</PATHTYPE>
                    <PATH>#THE_VALUE#</PATH>
                    <PATHFORMAT>MacOS</PATHFORMAT>
                </FILEREF>
"""

    template_text = SetPreferenceBlock(template_text,
                                       file_list,
                                       gLinkOrderBlockString,
                                       "#LINK_ORDER_ITEMS#")
    
    ## add frameworks
    if len(mprj.project.sys_frameworks):
        framework_string_list = string.join(mprj.project.sys_frameworks, ',')
        template_text=SetFrameworkBlock(template_text,
                                        framework_string_list,
                                        "#FRAMEWORK_LIST#")
    else:
        template_text=string.replace(template_text, "#FRAMEWORK_LIST#", "")

    ## group order
    template_text=SetGroupBlock(template_text,
                                file_list,
                                "#GROUP_LIST_ITEMS#",
                                mprj.cwtarget_name)
    template_text = string.replace(template_text,
                                   "#GROUP_NAME#",
                                   mprj.cwtarget_name)

    ## CW project preferences
    template_text=SetPreferenceValue(template_text, "MWFrontEnd_C_prefixname", mprj.prefix_file)
    template_text=SetPreferenceValue(template_text, "MWRez_Language_prefixname", mprj.rprefix_file)
                
    ## set remaining preferences - theses are defined in macos-carbon-powerpc-cw6.cf
    for (panel, pref) in mprj.preferences.items():
        for (pref_key, pref_value) in pref.items():
            template_text = SetPreferenceValue(template_text,
                                               pref_key,
                                               pref_value)


    # set target dir to debug/release
    if templateName == "project_uber.xml":  
        module_path = mprj.project_file_path[:-1]
        module_path, module_name = os.path.split(module_path)
        module_path, module_name = os.path.split(module_path)
        template_text = string.replace(template_text,
                                       "#TARGET_DIR#",
                                       "#SRC_ROOT_PATH#%s:%s:" % (module_name,output_dir))
                                       
        template_text = string.replace(template_text,
                                       "#OUTPUT_FILE_NAME#",
                                       "%s:%s" % (output_dir,mprj.output_name))
    else:
        template_text = string.replace(template_text,
                                       "#TARGET_DIR#",
                                       ":%s:" % (output_dir))

    template_text = string.replace(template_text,
                                   "#TARGET_NAME#",
                                   mprj.cwtarget_name)

    return template_text

    

def WriteProjectXML(mprj):
    """ Writes out the CodeWarrior project definition XML 
        Also writes out in project_uber.xml the sections of the XML
        required by the uber-project creator"""
        
    xml=WriteProjectSettingsXML(mprj, "project.xml")
    filename=mprj.project.makefile_name+".xml"
    open(filename,"w").write(xml)

    xml=WriteProjectSettingsXML(mprj, "project_uber.xml")
    filename=mprj.project.makefile_name+"_uber.xmldata"
    open(filename,"w").write(xml)


class ASMakefile(umake_ascript.ASMakefile):
    def RunProject(self):
        mprj = self.mprj
        script = self.script
        all = self.all

        ## create the project XML files to be imported into CW
        WriteProjectXML(mprj)

        xml_filename=macpath.join(os.getcwd(), mprj.project.makefile_name+".xml")
        all.extend( [
            '  tell application %s' % (mprj.ide_path),
            '       with timeout of 99999 seconds',
            '       try',
            '           make new (project document) as "%s" with data ("%s")' \
            % (mprj.project_file_path, xml_filename),
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



# the outermost XML template for the uber project XML
UberTemplate = """
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


def declaw_name(name):
    name = macpath.normpath(name)
    while len(name) and name[0] == ':':
        name = name[1:]
    name = string.lower(name)
    name = string.replace(name,":","_")
    name = string.replace(name,"_makefile","")
    name = string.replace(name,".mak","")
    return name

##
## Generate an Uber-project for this umakefile and all it's dependencies
## 
def generate_uberxml(platform, project, mprj):
    global UberTemplate

    target_tag = '#TARGET#'
    targetorder_tag = '#TARGETORDER#'
    group_tag = '#GROUP#'
    make_all_subtargets_tag = '#MAKE_ALL_SUBTARGETS#'

    dependencies_section_empty = '<SUBTARGETLIST></SUBTARGETLIST>'

    uber_template = UberTemplate

    # scoop up the uber text from each directory, add the depend
    all_module_targetnames_list = []

    for mod in project.get_uber_makes():
        uber_file_name = mod.makefile() + "_uber.xmldata"
        umake_lib.debug("Reading uber-data %r" % repr(uber_file_name))
        # Read the generated UBER-file
        try:
            uber_text = open(uber_file_name, 'r').read()
        except:
            print "==> Cannot read from %s " % (uber_file_name )
            continue

        uber_text = string.replace(uber_text,"#SRC_ROOT_PATH#", project.src_root_path)

        try:
            (uber_target_name,
             uber_text_1,
             uber_text_2,
             uber_text_3,
             uber_output_path) = string.split(uber_text, "<<>>")

        except:
            print "==> Cannot parse %s from module %s for <<>> sections" % (uber_file_name, module.name)
            continue

        uber_target_name = string.strip(uber_target_name)

        # also add this as a subtarget for the Make All target
        all_module_targetnames_list.append(uber_target_name)

        # now we look for the <SUBTARGETLIST></SUBTARGETLIST> in the target section of the
        # module's XML, and replace it with a list of all actual dependencies

        # make a subtarget list containing our dependencies list, and replace the empty one
        # in the template with ours

        deps = mod.dependencies()[:]
        deps.reverse()
        dependencies_section_xml = MakeDependenciesSubtargetListXML(deps)
        uber_text_1 = string.replace(uber_text_1,
                                     dependencies_section_empty,
                                     dependencies_section_xml,
                                     1)
        
        # if an empty subtarget list is left in the template, remove it
        uber_text_1 = string.replace(uber_text_1,
                                     dependencies_section_empty,
                                     "",
                                     1)

        # finally, follow the target, targetorder, and group placeholders in the top level
        # template with the data for this module

        template1 = string.replace(uber_template, target_tag, target_tag + '\n' + uber_text_1, 1)
        template2 = string.replace(template1, targetorder_tag, targetorder_tag + '\n' + uber_text_2, 1)
        uber_template = string.replace(template2, group_tag, group_tag + '\n' + uber_text_3, 1)

    # if we found any uber.xml files, generate the master uber.xml file

    # print "Extracted uber-project XML from %s" % (string.join(success_list, ", "))

    # insert the Make All subtarget list
    make_all_dependencies_section_xml = MakeDependenciesSubtargetListXML(all_module_targetnames_list)
    uber_template = string.replace(uber_template,
                                   make_all_subtargets_tag,
                                   make_all_dependencies_section_xml, 1)

    # remove the placeholder tags from the template
    uber_template = string.replace(uber_template, target_tag, "", 1)
    uber_template = string.replace(uber_template, targetorder_tag, "", 1)
    uber_template = string.replace(uber_template, group_tag, "", 1)

    # write out the file into an uber directory in the source directory
    uberxml_output_name = project.makefile_name + "_uber.xml"
    print "Writing %s" % (uberxml_output_name)
    try:
        # delete any old xml file with the same name as our uber
        if os.path.isfile(uberxml_output_name):
            os.remove(uberxml_output_name)

        open(uberxml_output_name, 'w').write(uber_template)
    except IOError:
        print "*** Failed to write file %s" % (uberxml_output_name) 


def MakeDependenciesSubtargetListXML(subtarget_list):
    # given a list of modules, make a <SUBTARGETLIST> XML section
    ret = [ '<SUBTARGETLIST>']
    for x in subtarget_list:
        x = declaw_name(x)
        ret.append('<SUBTARGET><TARGETNAME>%s</TARGETNAME></SUBTARGET>' % x)
    ret.append('</SUBTARGETLIST>')
    return string.join(ret,"\n")

def make_makefile(platform, project):
    ## FIX pncrt madness
    if project.getTargetType() in [ 'dll' , 'exe' ]:
        fix_pncrt(project)

    ## Create Applescript
    mprj = ProjectToMacCWProjectData(platform, project)
    applescript_path = macfs.FSSpec(project.makefile_name).as_pathname()
    ASMakefile(mprj).script.CompileAndSave(applescript_path)

    ## Create Uber XML file
    generate_uberxml(platform, project, mprj)

    ## Create an applescript stub file for reading the Uber XML file
    # this needs better handling/reporting of build errors
    script = ascript.CreateAppleScript()
    script.Append(
        'tell application %s' % (os.environ["BUILD_SW"]),
        '  with timeout of 99999 seconds',
        '    try',
        '      make new (project document) as "%s" with data ("%s")' % (
        macpath.join(os.getcwd(),project.makefile_name+"_uber.prj"),
        macpath.join(os.getcwd(),project.makefile_name+"_uber.xml")),
        '      set cwErrorList to Make Project with ExternalEditor',
        '      Close Project',
        '    on error errText number errnum',
        '      return errText & return',
        '    end try',
        '  end timeout',
        'end tell')
    uber_ascript = macfs.FSSpec(project.makefile_name+"_uber").as_pathname()
    script.CompileAndSave(uber_ascript)

    ## Pretend like everything went well
    return None

def compile():
    """This will eventually be the primary function for invoking make"""
    pass

