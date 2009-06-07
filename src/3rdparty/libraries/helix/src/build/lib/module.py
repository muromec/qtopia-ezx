# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: module.py,v 1.22 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Implements the Module class, storing data for module entries in *.bif
files.  A module has a id, name, and directory; these can all be the same
or all different depending on the module, so it is important to understand
them here.

module id:
A absolutely unique identifier defined within the BIF file to reference
this module for its dependancies.

module name:
The CVS name of the module, used to checkout and update the module.

module directory_list:
The list of directorioes the module will checkout to; this can be different
than the module name if we are using module aliasing, or it can be a whole
list of things if multiple top-level directories are stored in a distribution
archive."""

import sys
import string
import err
import os
import sysinfo

class SDK:
    def __init__(self, name, path,
                 error_message = "",
                 system_id = None,
                 ifexists = None):
        self.name=name
        self.path=path
        self.error_message=error_message
        self.system_id=system_id
        self.ifexists=ifexists

    def check_platform(self, sysid=None):
        ## Check the for=...
        if not self.system_id:
            return 1

        if not sysid:
            if fnmatch.fnmatch(sysinfo.id, self.system_id):
                return 1

            for l in sysinfo.family_list:
                if fnmatch.fnmatch(l, s.system_id):
                    return 1
        else:
            if fnmatch.fnmatch(sysid, self.system_id):
                return 1

            for l in sysinfo.PLATFORM_HASH.get(sysid).family_list:
                if fnmatch.fnmatch(l, s.system_id):
                    return 1

        return 0


    def write(self):
        ret = '  <sdk name="%s"' % self.name

        if self.system_id:
            ret=ret+' for="%s"' % self.system_id

        if self.ifexists:
            ret=ret+' ifexists="%s"' % self.ifexists

        if self.path:
            ret=ret+' path="%s"' % self.path

        if self.error_message:
            ret=ret+">"
            for m in string.split(self.error_message,"\n"):
                ret=ret+"\n    %s" % m
            ret=ret+"  </sdk>"
        else:
            ret=ret+"/>"

        return ret


class Module:
    """Data associated with one module entry defined in the *.bif files."""

    ## types of modules
    MODULE_CVS = "cvs"
    MODULE_NAME_ONLY = "name_only"
    MODULE_DISTRIBUTION = "distribution"
    MODULE_INSTALLER = "installer"
    MODULE_PROFILE = "profile"
    
    module_type_list = [
        MODULE_CVS,
        MODULE_NAME_ONLY,
        MODULE_DISTRIBUTION,
        MODULE_INSTALLER,
        MODULE_PROFILE
        ]
    
    def __init__(self, id, name = None, filename = None, line_number = None):
        # unique ID of module
        self.id = string.strip(id)
        
        # name of module
        if name:
            self.name = string.strip(name)
        else:
            self.name = self.id

        self.filename = filename
        self.line_number = line_number

        self.bif_version = 100
        self.default_profile = None
        self.default_options = None

        ## REQUIRED SANITY CHECKS
        self.__name_check()

        ## default module type is CVS
        self.type = self.MODULE_CVS

        ## group this module belongs to
        self.group = ''

        ## english description of module
        self.description = ''

        ## list containing the id of dependancy modules
        ## listed in the information file
        self.dependancy_id_list = []

        ## list containing the id of source dependancy modules
        ## listed in the information file
        self.source_dependancy_id_list = []

        ## list containing the id of checkin dependancy modules
        ## listed in the information file
        self.checkin_dependancy_id_list = []

        ## list containing the names of platform types to
        ## exclusively build this module under
        self.platform_include_list = []

        ## list containing the names of platform types to
        ## NOT build this module under
        self.platform_exclude_list = []

        ## list containing the names of profile types to
        ## exclusively build this module under
        self.profile_include_list = []

        ## list containing the names of profile types to
        ## NOT build this module under
        self.profile_exclude_list = []

        ## list containing the names of defines to
        ## exclusively build this module under
        self.define_include_list = []

        ## list containing the names of defines to
        ## NOT build this module under
        self.define_exclude_list = []

        ## Umake prefix files
        self.umake_includefiles = []

        ## flags
        self.define_include_list_flag = 0
        self.define_exclude_list_flag = 0
        self.platform_include_list_flag = 0
        self.platform_exclude_list_flag = 0
        self.profile_include_list_flag = 0
        self.profile_exclude_list_flag = 0
        self.build_static_flag          = 0
        self.build_static_only_flag     = 0
        self.build_dynamic_only_flag    = 0
        self.build_number_flag          = 0
        self.version_file_flag          = 0
        self.update_platform_header_flag= 0
        self.no_build_flag              = 0
        self.cvs_tag_flag               = 0
        self.cvs_date_flag              = 0
        self.installer_flag             = 0

        ## attributes
        self.attributes = {}
        
        ## cvs tag specified for this module
        self.cvs_tag = ''
        self.cvs_tag_type = 'branch'
        self.cvs_root = ''
        self.cvs_date = ''
        self.cvs_path = None

        ## halt priority, default to green
        self.halt_priority = "green"

        ## Checkout error message
        self.error_message=''

        ## Required SDKs
        self.sdks = []

        ## Defines
        self.defines = {}

        ## This means this module is only needed when checking in code
        ## with -tdistribute
        self.checkin_dep_only=0

    def __name_check(self):
        """Check the module.name value for invalid charactors."""
        
        for sep in [ '\\', ':', ' ', '#']:
            if sep in self.name or sep in self.id:
                e = err.Error()
                e.Set("The module id=\"%s\" with name=\"%s\" in the bif "\
                      "file has the illegal charactor=\"%s\" in it." % (
                    self.id, self.name, sep))
                raise err.error, e

    def set_type(self, type):
        """Set the module type.  This is usually: cvs(default), or
        distribution."""

        if type not in self.module_type_list:
            e = err.Error()
            e.Set("The module id=\"%s\" in the bif file is set to a "\
                  "unknown type=\"%s\"." % (self.id, type))
            raise err.error, e

        self.type = type

    def location(self):
        if self.filename and self.line_number != None:
            return "%s:%d" % (self.filename, self.line_number)
        return "-"

    def desc(self):
        return "%s(%s)" % (self.id, self.location())

    def set_group(self, group):
        """Set the company group field the module belongs to."""
        self.group = group
        
    def set_description(self, attribute_string):
        """Set the text description of the module."""
        self.description = string.strip(attribute_string)

    def set_halt_priority(self, priority):
        if priority not in ["red", "yellow", "green"]:
            e = err.Error()
            e.Set("The module id=\"%s\" with name=\"%s\" in the bif "\
                  "file has a incorrect halt_priority setting=\"%s\"." % (
                self.id, self.name, priority))
            raise err.error, e
        self.halt_priority = priority
        
    def set_dependancy_id_list(self, attribute_string):
        """Given a space-seperated string of dependent module ids,
        split it up and add each to the dependancy_id_list."""
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.dependancy_id_list.append(string.strip(item))

    def set_source_dependancy_id_list(self, attribute_string):
        """Given a space-seperated string of dependent module ids,
        split it up and add each to the dependancy_id_list."""
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.source_dependancy_id_list.append(string.strip(item))

    def set_checkin_dependancy_id_list(self, attribute_string):
        """Given a space-seperated string of dependent module ids,
        split it up and add each to the dependancy_id_list."""
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.checkin_dependancy_id_list.append(string.strip(item))

    def set_platform_include_list(self, attribute_string):
        """Given a space-seperated sting of platforms for this module
        to be included on, split it up and add each to the
        platform_include_list, and set the platform_include_list_flag
        to true."""

        self.platform_include_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.platform_include_list.append(string.strip(item))

    def set_platform_exclude_list(self, attribute_string):
        """Same as set_platform_include_list, but for the
        platform_exclude_list."""
        
        self.platform_exclude_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.platform_exclude_list.append(string.strip(item))

    def set_profile_include_list(self, attribute_string):
        """Given a space-seperated sting of profiles for this module
        to be included on, split it up and add each to the
        profile_include_list, and set the profile_include_list_flag
        to true."""

        self.profile_include_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.profile_include_list.append(string.strip(item))

    def set_profile_exclude_list(self, attribute_string):
        """Same as set_profile_include_list, but for the
        profile_exclude_list."""
        
        self.profile_exclude_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.profile_exclude_list.append(string.strip(item))


    def set_define_include_list(self, attribute_string):
        """Given a space-seperated sting of defines for this module
        to be included on, split it up and add each to the
        define_include_list, and set the define_include_list_flag
        to true."""

        self.define_include_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.define_include_list.append(string.strip(item))

    def set_define_exclude_list(self, attribute_string):
        """Same as set_define_include_list, but for the
        define_exclude_list."""
        
        self.define_exclude_list_flag = 1
        attribute_string = string.strip(attribute_string)
        for item in string.split(attribute_string):
            if item != '':
                self.define_exclude_list.append(string.strip(item))


    def set_attribute(self, attribute):
        self.attributes[attribute]=1

    def unset_attribute(self, attribute):
        self.attributes[attribute]=0

    def get_attribute(self, attribute):
        return self.attributes.get(attribute)

    def set_build_static(self, attribute_string = None):
        """Set the build_static_flag to true, indicating the module
        should be built with the "static" umake option, in addition
        to being built with normal options."""
        self.build_static_flag = 1

    def set_build_static_only(self, attribute_string = None):
        """Same as set_build_static, except the module should only be built
        once with the static umake option."""

        self.build_static_flag = 1
        self.build_static_only_flag = 1

    def set_build_dynamic_only(self, attribute_string = None):
        """Same as set_build_static, except the module should only be built
        once with the static umake option."""

        self.build_static_flag = 0
        self.build_static_only_flag = 0
        self.build_dynamic_only_flag = 1

    def set_build_number(self, attribute_string = None):
        """Set the build number flag.  This is depricated."""
        self.build_number_flag = 1

    def set_version_file(self, attribute_string = None):
        """Set the version_file_flag to true, indicating the module
        has a version file which should be incremented on build farm
        builds."""
        self.version_file_flag  = 1

    def set_update_platform_header(self, attribute_string = None):
        """Set the update_platform_header to true, indicating the module
        has a platform.h file which should be updated to reflect the
        current build."""
        self.update_platform_header_flag  = 1

    def set_cvs_tag(self, cvs_tag, cvs_tag_type = None):
        """Set this module to come from a given CVS tag, and indicate
        if the tag is a CVS branch tag, or a normal CVS tag.  The
        cvs_tag_type was once used to determine if updates to the version
        files in the module could be checked into CVS by the automated
        build farm."""

        self.cvs_tag = string.strip(cvs_tag)
        self.cvs_tag_flag = 1

        ## set the tag type (branch/tag)
        if not cvs_tag_type:
            self.cvs_tag_type = 'branch'
        elif string.lower(cvs_tag_type) == 'tag':
            self.cvs_tag_type = 'tag'
        elif string.lower(cvs_tag_type) == 'branch':
            self.cvs_tag_type = 'branch'

    def set_cvs_date(self, cvs_date):
        """Set this module to come from a given CVS date"""

        self.cvs_date = string.strip(cvs_date)
        self.cvs_date_flag = 1

    def set_cvs_root(self, cvs_root):
        """Set what CVS repository to get this module from"""
        self.cvs_root = cvs_root

    def set_no_build(self, attribute_string = ''):
        """Set the no_build_flag, indicating no action should be taken
        on this module other than checking it out."""
        self.no_build_flag = 1

    def write(self, bif_data = None):
        """Write the BIF XML for this module to standard output.""" 

        line_list = []

        # <module ....>
        line_list.append('<!-- %s -->' % (string.upper(self.id)))
        line = '<module id="%s"' % (self.id)
        if self.id != self.name:
            line = '%s name="%s"' % (line, self.name)

        line = '%s group="%s"' % (line, self.group)

        if self.type != self.MODULE_CVS:
            line = '%s type="%s"' % (line, self.type)

        if self.bif_version != 100:
           line = '%s  version="%d.%02d"' % (line,
                                             self.bif_version/100,
                                             self.bif_version % 100)

        line = '%s>' % (line)

        line_list.append(line)

        if self.filename != "-":
            line_list.append('  <location file="%s" line="%d"/>' % (
                self.filename,
                self.line_number))

        ## description
        if len(self.description) > 0:
            line_list.append('  <description>')
            line_list.append('  %s' % (self.description))
            line_list.append('  </description>')
            line_list.append('')

        if self.error_message:
            line_list.append('  <checkout_error_message>')
            line_list.append('  %s' % self.error_message)
            line_list.append('  </checkout_error_message>')
            line_list.append('')

        addnl=0
        defroot=None
        deftag=None
        deftagtype=None
        defdate=None
        if bif_data:
            defroot=bif_data.default_cvs_root
            deftag=bif_data.default_cvs_tag
            deftagtype=bif_data.default_cvs_tag
            defdate=bif_data.default_cvs_date

        newstyle = 0
        if self.cvs_root != defroot:
            line_list.append('  <cvs root="%s"/>' % (self.cvs_root or ""))
            newstyle = 1
            addnl=1

        if self.cvs_date != defdate:
            line_list.append('  <cvs date="%s"/>' % (self.cvs_date or ""))
            newstyle = 1
            addnl=1

        if self.cvs_path:
            line_list.append('  <cvs path="%s"/>' % (self.cvs_path))
            newstyle = 1
            addnl=1

        if self.default_profile:
            line_list.append('  <default profile="%s"/>' %
                             (self.default_profile))
            addnl=1

        if self.cvs_tag != deftag or self.cvs_tag_type != deftagtype:
            if newstyle:
                if self.cvs_tag_type == "branch":
                    line_list.append('  <cvs branch="%s"/>' % (self.cvs_tag))
                else:
                    line_list.append('  <cvs tag="%s"/>' % (self.cvs_tag))
            else:
                line_list.append('  <cvstag id="%s" type="%s"/>' % (
                    self.cvs_tag,
                    self.cvs_tag_type))
            addnl=1
            
        if self.default_options:
            line_list.append('  <default options="%s"/>' %
                             (self.default_options))
            addnl=1

        if addnl:
            line_list.append("")

        for s in self.sdks:
            line_list.append(s.write())

        if self.sdks:
            line_list.append('')
    
        # flags
        if self.build_number_flag:
            line_list.append('  <attribute id="build_number"/>')
            
        if self.version_file_flag:
            line_list.append('  <attribute id="has_version_file"/>')

        if self.update_platform_header_flag:
            line_list.append('  <attribute id="update_platform_header"/>')

        if self.build_static_flag and not self.build_static_only_flag:
            line_list.append('  <attribute id="static_build"/>')

        if self.build_static_only_flag and self.build_static_flag:
            line_list.append('  <attribute id="static_build_only"/>')

        if self.build_dynamic_only_flag:
            line_list.append('  <attribute id="dynamic_build_only"/>')

        if self.no_build_flag:
            line_list.append('  <attribute id="no_build"/>')

        for a in self.attributes.keys():
            if self.attributes[a]:
                line_list.append('  <attribute id="%s"/>' % a)

        if string.find(line_list[-1], 'attribute'):
            line_list.append('')

        # includeplatforms
        if self.platform_include_list_flag:
            line_list.append('  <includeplatforms>')
            line_list.append('    %s' % (string.join(self.platform_include_list)))
            line_list.append('  </includeplatforms>')
            line_list.append('')

        # excludeplatforms
        if self.platform_exclude_list_flag:
            line_list.append('  <excludeplatforms>')
            line_list.append('    %s' % (string.join(self.platform_exclude_list)))
            line_list.append('  </excludeplatforms>')
            line_list.append('')

        # includeprofiles
        if self.profile_include_list_flag:
            line_list.append('  <includeprofiles>')
            line_list.append('    %s' % (string.join(self.profile_include_list)))
            line_list.append('  </includeprofiles>')
            line_list.append('')

        # excludeprofiles
        if self.profile_exclude_list_flag:
            line_list.append('  <excludeprofiles>')
            line_list.append('    %s' % (string.join(self.profile_exclude_list)))
            line_list.append('  </excludeprofiles>')
            line_list.append('')

        # includedefines
        if self.define_include_list_flag:
            line_list.append('  <ifdef>')
            line_list.append('    %s' % (string.join(self.define_include_list)))
            line_list.append('  </ifdef>')
            line_list.append('')

        # excludedefines
        if self.define_exclude_list_flag:
            line_list.append('  <ifndef>')
            line_list.append('    %s' % (string.join(self.define_exclude_list)))
            line_list.append('  </ifndef>')
            line_list.append('')

        if len(self.defines):
            line_list.append("  <defines>")
            for d in self.defines.keys():
                line_list.append('    %s=%s' % (d, self.defines[d]))
            line_list.append("  </defines>")


        if len(self.umake_includefiles):
            line_list.append("  <umake_includefiles>")
            line_list.append('    %s' % (string.join(self.umake_includefiles)))
            line_list.append("  </umake_includefiles>")


        # dependancy list
        if len(self.dependancy_id_list) > 0:
            depend_list = []
            for depend in self.dependancy_id_list:
                depend_list.append(depend)
            
            line_list.append('  <dependlist>')
            while (len(depend_list)):
                line_list.append('    %s' % (string.join(depend_list[:5])))
                depend_list = depend_list[5:]
            line_list.append('  </dependlist>')

        # dependancy list
        if len(self.source_dependancy_id_list) > 0:
            depend_list = []
            for depend in self.source_dependancy_id_list:
                depend_list.append(depend)
            
            line_list.append('  <source_dependlist>')
            while (len(depend_list)):
                line_list.append('    %s' % (string.join(depend_list[:5])))
                depend_list = depend_list[5:]
            line_list.append('  </source_dependlist>')

        # dependancy list
        if len(self.checkin_dependancy_id_list) > 0:
            depend_list = []
            for depend in self.checkin_dependancy_id_list:
                depend_list.append(depend)
            
            line_list.append('  <checkin_dependlist>')
            while (len(depend_list)):
                line_list.append('    %s' % (string.join(depend_list[:5])))
                depend_list = depend_list[5:]
            line_list.append('  </checkin_dependlist>')

        line_list.append('</module>')

        return "    " + string.join(line_list,"\n    ") + "\n";

        for line in line_list:
            ret.append('    %s' % (line))


    def path(self):
        if '/' in self.name:
            return apply(os.path.join, [os.curdir] + string.split(self.name,"/"))
        else:
            return self.name

    def checkout(self,
                 root=None,
                 tag=None,
                 date=None,
                 az=None,  ## Native path
                 nonrecursive = None,
                 zap=None,
                 download_only=None,
                 system_id=None,
                 profile=None,
                 build_type=None):

        import distributions
        import cvs
        import shell

        try:
            if self.checkin_dep_only:
                return
        except AttributeError:
            pass

        if self.type == "cvs":
            distributions.setup()
            tmpdir=distributions.tmpdir()
            tmpdir=os.path.join(tmpdir,"module")

            cvs.Checkout(tag or self.cvs_tag,
                         self.cvs_path or self.name,
                         root or self.cvs_root,
                         tmpdir,
                         date or self.cvs_date,
                         nonrecursive,
                         zap)

            if ( ( tag or self.cvs_tag) and
                 (date or self.cvs_date) and
                 not os.path.exists(tmpdir) ):
                
                cvs.Checkout(self.cvs_tag,
                             self.cvs_path or self.name,
                             root or self.cvs_root,
                             tmpdir,
                             self.cvs_date,
                             nonrecursive,
                             zap)

            if not os.path.exists(tmpdir):
                raise cvs.cvs_error

            shell.mkdir(os.path.dirname(az or self.path()))
            os.rename(tmpdir, az or self.path())

            distributions.cleanup()

        elif self.type == "distribution":
            df=distributions.DistFinder(self.name,
                                        root or self.cvs_root,
                                        tag or self.tag,
                                        date or self.cvs_date)
            fun = df.get_distribution
            if download_only:
                fun=df.download_distribution
                if not system_id:
                    system_id = sysinfo.id

                return df.get_distribution(
                    sysinfo.PLATFORM_HASH.get(system_id).distribution_id,
                    profile,
                    build_type)



## entrypoints
def CreateModule(id, name = None, filename = None, line_number = None):
    """Creates one instance of the Module class with the given id, and
    optional CVS name, if it is different from the module id."""
    return Module(id, name, filename, line_number)
