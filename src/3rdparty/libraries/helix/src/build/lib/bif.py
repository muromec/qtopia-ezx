# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: bif.py,v 1.33 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Functions and classes used in *.bif file loading, compiling, error
checking, and parsing."""

import os
import sys
import string
import stat
import cPickle

from module import CreateModule
import outmsg
import err
import fnmatch
import sysinfo
import types
import module
import timestamp
import re
import time
import copy
import log
import utils

bif_shadows=[]

BIFParser=None
have_expat=0

# Later, this can go in a CVS module or something.
cvsTagTypes = [ 'root' , 'tag' , 'branch' , 'path' , 'date' ]

def AddShadow(file):
    bif_shadows.append(file)

## load a BIF file, returning the bif_data and saving the compiled
## version to a compiled BIF file (BIC file)
def load_bif(filename, branchlist = None, include_shadows=1):
    bif_data = BIFData()

    global BIFParser
    global have_expat

    if not BIFParser:
        try:
            install_new_bif_parser()
            have_expat=1
        except ImportError:
            print "Unable to find expat module, using old BIF parser"
            install_old_bif_parser()

    try:
        BIFParser(filename, bif_data, branchlist)
    except err.error, e:
        if have_expat:
            print "WARNING: this bif file fails to parse with new BIF parser"
            print e
            install_old_bif_parser()
            try:
                BIFParser(filename, bif_data, branchlist)
            finally:
                install_new_bif_parser()
        else:
            raise

    orig_modules = bif_data.module_hash.keys()

    if include_shadows:
        id=bif_data.build_id
        for shadow in bif_shadows:
            for mod in bif_data.module_hash.values():
                mod.inherited=1

            if not branchlist:
                import branchlist
                branchlist=branchlist.BranchList()

            shadow_file_name=branchlist.file(shadow)

            if not shadow_file_name:
                if include_shadows > 1:
                    continue
                
                e=err.Error()
                e.Set("Unable to find shadow BIF file \"%s\"." % shadow)
                raise err.error, e

            BIFParser(shadow_file_name, bif_data, branchlist)

        tmp={}
        for m in orig_modules:
            tmp[m]=1
            
        for m in bif_data.module_hash.keys():
            if not tmp.has_key(m):
                bif_data.module_hash[m].set_attribute("from_shadow_only")
            
        bif_data.build_id=id

    return bif_data


## load a BIFData class either from the compiled BIC file or from
## the BIF file
def load_bif_data(filename, branchlist = None, shadow=1):
    bif_data = load_bif(filename, branchlist, shadow)
    return bif_data


class Default:
    def __init__(self,
                 profile=None,
                 target=None,
                 options=None,
                 system_id = None):
        self.profile=profile
        self.target=target
        self.options=options
        self.system_id=system_id

    def write(self):
        ret="  <default"
        if self.target:
            ret = ret +' target="%s"' % self.target
        if self.profile:
            ret = ret +' profile="%s"' % self.profile
        if self.options != None:
            ret = ret +' options="%s"' % self.options
        if self.system_id:
            ret = ret +' for="%s"' % self.system_id
        return ret+"/>"
        

## data structure for information contained in BIF files
class BIFData:
    def __init__(self):
        self.build_id = ''
        self.default_cvs_tag = ''
        self.default_cvs_tag_type = 'branch'
        self.default_cvs_root = ''
        self.default_cvs_date = ''
        self.module_hash = {}

        self.defaults = []

        self.default_target = ''
        self.default_profile = ''
        self.default_options = ''
        self.bif_version = 100
        self.expires = '2005-01-01'

    def set_build_id(self, build_id):
        self.build_id = build_id

    def get_expiration(self):
        return timestamp.Timestamp( self.expires )

    def set_default_cvs_tag(self, default_cvs_tag):
        self.default_cvs_tag = default_cvs_tag

    def set_default_cvs_tag_type(self, default_cvs_tag_type):
        self.default_cvs_tag_type = default_cvs_tag_type

    def set_default_cvs_root(self, default_cvs_root):
        self.default_cvs_root = default_cvs_root

    def add_module(self, module):
        if self.module_hash.has_key(module.id):
            mod = self.module_hash[module.id]
            if not mod.__dict__.has_key("inherited"):
                e = err.Error()
                e.Set("Two or more modules have id=\"%s\" in the bif file." % (
                    module.id))
                raise err.error, e
        self.module_hash[module.id] = module

    def write(self, all=0):
        ret = [
            "<?xml version=\"1.0\" ?>",
            "<!-- $Id: -->",
            '<build id="%s"' % self.build_id,
            ]

        ## For backwards compatibility, expires and version must
        ## be excluded if they have their default values.
        ## (In fact, no attribute other than id may exist in the
        ##  <build> tag for old versions of the build system.)
        if self.bif_version != 100:
            ret[-1]=ret[-1] + ' version="%d.%02d"' % (
                self.bif_version/100,
                self.bif_version % 100 )

        if self.expires != '2005-01-01':
            ret[-1]=ret[-1] + ' expires="%s"' % self.expires

        ret[-1]=ret[-1]+">"

        ret.append("  <!-- defaults --> ")

        addnl=0
        if self.default_cvs_root:
            ret.append('  <cvs root="%s"/>' % self.default_cvs_root)
            addnl=1

        if self.default_cvs_tag:
            ret.append('  <cvs %s="%s"/>' % (self.default_cvs_tag_type,
                                             self.default_cvs_tag))
            addnl=1
            
        if self.default_cvs_date:
            ret.append('  <cvs date="%s"/>' % (self.default_cvs_date))
            addnl=1

        if addnl:
            ret.append("")

        for d in self.defaults:
            ret.append(d.write())

        ret.append("  <targets>")

        ids = self.module_hash.keys()
        ids.sort()
        for id in ids:
            m=self.module_hash[id]
            if (all or
                not m.attributes.get("from_shadow_only") or
                m.attributes.get("sdk_depend_only")):

                ret.append(self.module_hash[id].write(self))

        ret.append("  </targets>")
        ret.append("</build>")
        
        return string.join(ret,"\n") + "\n"
            
## the BIF file XML Parser
TAG_build = "build"
TAG_cvstag = "cvstag"
TAG_default = "default"
TAG_cvs = "cvs"
TAG_targets = "targets"
TAG_module = "module"
TAG_description = "description"
TAG_attribute = "attribute"
TAG_includeplatforms = "includeplatforms"
TAG_excludeplatforms = "excludeplatforms"
TAG_includeprofiles = "includeprofiles"
TAG_excludeprofiles = "excludeprofiles"
TAG_dependlist = "dependlist"
TAG_source_dependlist = "source_dependlist"
TAG_checkin_dependlist = "checkin_dependlist"
TAG_halt = "halt"
TAG_sdk = "sdk"
TAG_errmsg = "checkout_error_message"
TAG_defines = "defines"
TAG_ifdef = "ifdef"
TAG_ifndef = "ifndef"
TAG_umake_includefiles = "umake_includefiles"
TAG_version = "version"


class BIFParserFunctions:
    def __init__(self, filename, bif_data = None, branchlist = None):
        if not bif_data:
            bif_data = BIFData()
        self.bif_data = bif_data
        self.branchlist = branchlist
        
        ## intermediate
        self.module = None
        
        ## parsing state stack
        self.tag_stack = []
        self.current_tag = None
        self.last_module=""

        self.linenum = 0
        self.filename=filename

        expireTS = self.bif_data.get_expiration()

        tmp = ( self.bif_data.bif_version,
                self.bif_data.build_id,
                self.bif_data.expires,
                self.bif_data.default_cvs_tag,
                self.bif_data.default_cvs_tag_type,
                self.bif_data.default_cvs_root,
                self.bif_data.default_cvs_date )

        ## Historical compatibility
        if self.bif_data.bif_version >= 203:
            self.bif_data.default_cvs_tag = ''
            self.bif_data.default_cvs_tag_type = 'branch'
            self.bif_data.default_cvs_root = ''
            self.bif_data.default_cvs_date = ''

        self.bif_data.expires = '2005-01-01'
        self.bif_data.bif_version = 100
        
        self.parse_bif(filename)

        if tmp[1]:
            t2 = self.bif_data.get_expiration()

            if t2 < expireTS:
                self.warning("Inherited file expires before this file!!")

            (self.bif_data.bif_version,
             self.bif_data.build_id,
             self.bif_data.expires
             ) = tmp[:3]

            if self.bif_data.bif_version >= 203:
                ( self.bif_data.default_cvs_tag,
                  self.bif_data.default_cvs_tag_type,
                  self.bif_data.default_cvs_root,
                  self.bif_data.default_cvs_date ) = tmp[3:]


        ## Delete the 'inherited' flag
        for mod in self.bif_data.module_hash.values():
            try:
                del mod.inherited
            except AttributeError:
                pass

    def parse_bif(self, filename):
        fil = open(filename, "r")
        while 1:
            line = fil.readline()
            if not line:
                break

            self.linenum = self.linenum + 1
            self.feed(line)

    def location(self):
        return "%s:%d" % (self.filename, self.lineno)

    def error(self, text):
        e = err.Error()
        e.Set("bif parser(%s): %s" % (self.location(), text))
        raise err.error, e

    def warning(self, text):
        outmsg.send("[WARNING] bif parser(%s): %s" % (self.location(), text))

    def fix_data_cb(self):
        self.__data_cb=getattr(self,
                               "handle_data_"+self.current_tag,
                               self.handle_data_default)
        
        

    def push_tag(self, tag):
        #print "%d: <%s>" % (self.linenum, tag)
        self.tag_stack.append(tag)
        self.current_tag = tag
        self.fix_data_cb()

    def pop_tag(self, tag):
        #print "%d: </%s>" % (self.linenum, tag)
        if len(self.tag_stack) == 0:
            self.error("pop xml tag with empty stack")
        
        if self.tag_stack[-1] != tag:
            self.error("pop xml tag=\"%s\" but expected tag=\"%s\"" % (
                tag, self.tag_stack[-1]))
        
        self.tag_stack = self.tag_stack[:-1]

        if len(self.tag_stack):
            self.current_tag = self.tag_stack[-1]
        else:
            self.current_tag=""

        self.fix_data_cb()

    def handle_data_description(self, data):
        self.module.set_description(data)

    def handle_data_includeplatforms(self, data):
        self.module.set_platform_include_list(data)

    def handle_data_excludeplatforms(self, data):
        self.module.set_platform_exclude_list(data)

    def handle_data_includeprofiles(self, data):
        self.module.set_profile_include_list(data)

    def handle_data_excludeprofiles(self, data):
        self.module.set_profile_exclude_list(data)

    def handle_data_ifdef(self, data):
        self.module.set_define_include_list(data)
        
    def handle_data_ifndef(self, data):
            self.module.set_define_exclude_list(data)

    def handle_data_dependlist(self, data):
        self.module.set_dependancy_id_list(data)
        
    def handle_data_source_dependlist(self, data):
        self.module.set_source_dependancy_id_list(data)

    def handle_data_checkin_dependlist(self, data):
        self.module.set_checkin_dependancy_id_list(data)
        
    def handle_data_umake_includefiles(self, data):
        self.module.umake_includefiles.extend(string.split(data))
        
    def handle_data_defines(self, data):
        for d in string.split(data):
            val = "1"
            ind = string.find(d, "=")
            if ind != -1:
                val = d[ind+1:]
                d=d[:ind]
            self.module.defines[d]=val

    def handle_data_sdk(self, data):
        self.module.sdks[-1].error_message = self.module.sdks[-1].error_message + data + "\r\n"
        
    def handle_data_checkout_error_message(self, data):
        self.module.error_message = self.module.error_message + data + "\r\n"

    def handle_data_default(self, data):
        self.warning("invalid data=\"%s\"" % (data))

    def handle_data(self, data):
        data = string.strip(data)
        if not data:
            return

        self.__data_cb(data)

    ## <build>
    def start_build(self, attr):
        self.push_tag(TAG_build)

        try:
            self.bif_data.set_build_id(attr['id'])
        except KeyError:
            self.error("<build> requires \"id\"")

        try:
            v = attr["version"]
            v=string.split(v,".")
            v=int(v[0])*100 + int(v[1])
            self.bif_data.bif_version=v
        except KeyError:
            pass

        try:
            expires=attr['expires']
            if not expires:
                raise timestamp.TimestampException
            expTS = timestamp.Timestamp( expires )
            nowTS = timestamp.Timestamp()
            
            if nowTS > expTS:
                self.warning("This BIF file has expired!")
            elif nowTS + 30 * 24 * 60 * 60 > expTS:
                self.warning("This BIF file will expire within a month")
            elif nowTS + 3 * 365 * 24 * 60 * 60 < expTS:
                self.warning("This expiration date is invalid, using 2005-01-01!!!")
                expires="2005-01-01"

        except timestamp.TimestampException:
            self.warning("Invalid expiration date (%s), using 2005-01-1" % repr(expires))
            expires="2005-01-01"            
        except KeyError:
            self.warning("This BIF file lacks an expiration date, using 2005-01-01")
            expires="2005-01-01"
            
        self.bif_data.expires=expires
        
        
    ## </build>
    def end_build(self):
        self.pop_tag(TAG_build)

    def start_version(self, attr):
        try:
            v=attr['id']
        except KeyError:
            self.error("<version> requires \"id\"")

        v=string.split(v,".")
        v=int(v[0])*100 + int(v[1])

        if self.current_tag == TAG_build:
            self.bif_data.bif_version=v
        elif self.current_tag == TAG_module:
            self.module.bif_version=v
        else:
            self.error("<version> in wrong place")


    def start_inherit(self, attr):
        idtmp=self.bif_data.build_id
        try:
            tid = attr["id"]
        except KeyError:
            self.error("<inherit> requires \"id\"")

        try:
            v = attr["version"]
            v=string.split(v,".")
            v=int(v[0])*100 + int(v[1])
            self.bif_data.bif_version=v
        except KeyError:
            pass
        
        if not self.branchlist:
            import branchlist
            self.branchlist = branchlist.BranchList()
            
        file_name=self.branchlist.file(tid)

        if not file_name:
            self.error("Unable to find BIF file \"%s\" for inherit." % tid)

        BIFParser(file_name, self.bif_data, self.branchlist)
        for mod in self.bif_data.module_hash.values():
            mod.inherited=1
                
        self.bif_data.build_id=idtmp

    ## <sdk name="sdk_name" [ for="system_id_glob" ] [ path="default_path" ] [ ifexists="file or dir" ]/>
    def start_sdk(self, attr):
        self.push_tag(TAG_sdk)

        self.module.sdks.append(
            module.SDK(attr.get("name"),
                       attr.get("path"),
                       "",
                       attr.get("for"),
                       attr.get("ifexists")))


    def end_sdk(self):
        self.pop_tag(TAG_sdk)
        

    ## <default [for="system_id_glob"] [profile=".."] [target=".."] [options=".."] />
    def start_default(self, attr):
        if self.current_tag != TAG_build:
            self.error("<default> in wrong place")

        self.bif_data.defaults.append(
            Default(attr.get("profile"),
                    attr.get("target"),
                    attr.get("options"),
                    attr.get("for")))
                    

        if attr.has_key("for"):
            if not fnmatch.fnmatch(sysinfo.id ,attr['for']):
                match =1
                for l in sysinfo.family_list:
                    if fnmatch.fnmatch(l ,attr['for']):
                        match=1
                        break
                if not match:
                    return

        if self.current_tag == TAG_module:
            if attr.has_key("profile"):
                self.module.default_profile = attr['profile']
                
            if attr.has_key("options"):
                self.module.default_options = attr['options']
        else:
            if attr.has_key("profile"):
                self.bif_data.default_profile = attr['profile']
                
            if attr.has_key("target"):
                self.bif_data.default_target = attr['target']
        
            if attr.has_key("options"):
                self.bif_data.default_options = attr['options']

        
    ## <cvstag id="..."/>
    def start_cvstag(self, attr):
        try:
            tid = attr["id"]
        except KeyError:
            self.error("<cvstag> requires \"id\"")

        ## set global default CVS tag
        if self.current_tag == TAG_build:
            self.bif_data.set_default_cvs_tag(tid)
            if attr.has_key('type'):
                self.bif_data.set_default_cvs_tag_type(attr['type'])

        ## set the CVS tag/type for module
        elif self.current_tag == TAG_module:
            if attr.has_key('type'):
                self.module.set_cvs_tag(tid, attr['type'])
            else:
                self.module.set_cvs_tag(tid)

        else:
            self.error("<cvstag> in wrong place")

    ## <cvs root="..." tag="..." branch="..." path="..." date="..."/>
    def start_cvs(self, attr):
        tid = None
        type = None
        root = None
        path = None
        date = None
        
        try:
            root = attr["root"]
        except KeyError:
            pass

        try:
            tid = attr["tag"]
            type = "tag"
        except KeyError:
            pass

        try:
            tid = attr["branch"]
            type = "branch"
        except KeyError:
            pass

        try:
            path = attr["path"]
        except KeyError:
            pass

        try:
            date = attr["date"]
        except KeyError:
            pass

        if not root and tid == None and path == None and date == None:
            self.error('<cvs> requires "root", "tag", "branch", "path" or "date"')
            
        ## set global default CVS tag
        if self.current_tag == TAG_build:
            if root:
                self.bif_data.set_default_cvs_root(root)
            if tid != None:
                self.bif_data.set_default_cvs_tag(tid)
                if type:
                    self.bif_data.set_default_cvs_tag_type(type)
            if path:
                self.error("<cvs path=.../> in wrong place")

            if date:
                self.bif_data.default_cvs_date = date

        ## set the CVS tag/type for module
        elif self.current_tag == TAG_module:
            if root:
                self.module.set_cvs_root(root)

            if tid != None:
                if type:
                    self.module.set_cvs_tag(tid, type)
                else:
                    self.module.set_cvs_tag(tid)

            if path:
                self.module.cvs_path = path

            if date:
                self.module.set_cvs_date(date)
                
        else:
            self.error("<cvs> in wrong place")

    ## <targets>
    def start_targets(self, attr):
        self.push_tag(TAG_targets)

    ## </targets>
    def end_targets(self):
        self.pop_tag(TAG_targets)


    ## <checkout_error_message>
    def start_checkout_error_message(self, attr):
        self.push_tag(TAG_errmsg)

    ## </checkout_error_message>
    def end_checkout_error_message(self):
        self.pop_tag(TAG_errmsg)

    def start_location(self, attr):
        if attr.has_key("file"):
            self.module.filename=attr['file']
        if attr.has_key("line"):
            self.module.line_number=int(attr['line'])

## <module id="..." name="..." group="..." type="..." inherit="...">
    def start_module(self, attr):
        self.push_tag(TAG_module)

        if attr.has_key('inherit'):
            h=attr['inherit']
            if not self.bif_data.module_hash.has_key(h):
                self.error("Cannot find module to inherit: '%s'" % h)
            self.module = copy.deepcopy(self.bif_data.module_hash[h])
            self.module.inherited=0

            self.module.filename=self.filename
            self.module.line_number=self.linenum

            if attr.has_key('id'):
                self.module.id = attr['id']

            if attr.has_key('name'):
                self.module.name = attr['name']
        else:
            try:
                mid = attr["id"]
            except KeyError:
                self.error("<module> requires \"id\"")

            mname = None
            if attr.has_key('name'):
                mname = attr['name']

            self.module = CreateModule(mid, mname, self.filename, self.linenum)
            self.module.bif_version = self.bif_data.bif_version

        try:
            v = attr["version"]
            v=string.split(v,".")
            v=int(v[0])*100 + int(v[1])
            self.module.bif_version=v
        except KeyError:
            pass

        ## Bind root/date/tag from global defaults
        if self.bif_data.default_cvs_tag_type:
            self.module.set_cvs_tag(self.bif_data.default_cvs_tag,
                                    self.bif_data.default_cvs_tag_type)
        else:
            self.module.set_cvs_tag(self.bif_data.default_cvs_tag)

        if self.bif_data.default_cvs_date:
            self.module.set_cvs_date(self.bif_data.default_cvs_date)

        if self.bif_data.default_cvs_root:
            self.module.set_cvs_root(self.bif_data.default_cvs_root)


        if attr.has_key('group'):
            self.module.set_group(attr['group'])

        if attr.has_key('type'):
            mtype = attr["type"]
            
            if mtype == 'distribution':
                self.module.set_type(self.module.MODULE_DISTRIBUTION)
            elif mtype == 'name_only':
                self.module.set_type(self.module.MODULE_NAME_ONLY)
            elif mtype == 'installer':
                self.module.set_type(self.module.MODULE_INSTALLER)
            elif mtype == 'profile':
                self.module.set_type(self.module.MODULE_PROFILE)
            else:
                self.error("unsupported module type=\"%s\"" % (mtype))

    ## </module>
    def end_module(self):
        self.pop_tag(TAG_module)
        self.bif_data.add_module(self.module)
        self.module = None

    ## <description>
    def start_description(self, attr):
        self.push_tag(TAG_description)

    ## </description>
    def end_description(self):
        self.pop_tag(TAG_description)

    ## <attribute id="..."/>
    def start_attribute(self, attr):
        if self.current_tag != TAG_module:
            self.error("<attribute> in wrong place")

        try:
            aid = attr["id"]
        except KeyError:
            self.error("<attribute> requires \"id\"")

        if aid == 'build_number':
            self.module.set_build_number()
        elif aid == 'has_version_file':
            self.module.set_version_file()
        elif aid == 'update_platform_header':
            self.module.set_update_platform_header()
        elif aid == 'static_build':
            self.module.set_build_static()
        elif aid == 'static_build_only':
            self.module.set_build_static_only()
        elif aid == 'dynamic_build_only':
            self.module.set_build_dynamic_only()
        elif aid == 'no_build':
            self.module.set_no_build()
        else:
            self.module.set_attribute(aid)
        

    ## <halt priority="..."/>
    def start_halt(self, attr):
        if self.current_tag != TAG_module:
            self.error("<halt> in wrong place")

        try:
            priority = attr["priority"]
        except KeyError:
            self.error("<halt> requires \"priority\"")

        if priority not in ["red", "yellow", "green"]:
            self.error("<halt priority=\"%s\"> invalid, must be: "\
                       "red, yellow, green" % (prioriey))

        self.module.set_halt_priority(priority)

            
    ## <includeplatforms>
    def start_includeplatforms(self, attr):
        self.push_tag(TAG_includeplatforms)

    ## </includeplatforms>
    def end_includeplatforms(self):
        self.pop_tag(TAG_includeplatforms)

    ## <excludeplatforms>
    def start_excludeplatforms(self, attr):
        self.push_tag(TAG_excludeplatforms)

    ## </excludeplatforms>
    def end_excludeplatforms(self):
        self.pop_tag(TAG_excludeplatforms)

    ## <includeprofiles>
    def start_includeprofiles(self, attr):
        self.push_tag(TAG_includeprofiles)

    ## </includeprofiles>
    def end_includeprofiles(self):
        self.pop_tag(TAG_includeprofiles)

    ## <excludeprofiles>
    def start_excludeprofiles(self, attr):
        self.push_tag(TAG_excludeprofiles)

    ## </excludeprofiles>
    def end_excludeprofiles(self):
        self.pop_tag(TAG_excludeprofiles)

    ## <dependlist>
    def start_dependlist(self, attr):
        self.push_tag(TAG_dependlist)

    ## </dependlist>
    def end_dependlist(self):
        self.pop_tag(TAG_dependlist)

    ## <source_dependlist>
    def start_source_dependlist(self, attr):
        self.push_tag(TAG_source_dependlist)

    ## </source_dependlist>
    def end_source_dependlist(self):
        self.pop_tag(TAG_source_dependlist)

    ## <checkin_dependlist>
    def start_checkin_dependlist(self, attr):
        self.push_tag(TAG_checkin_dependlist)

    ## </checkin_dependlist>
    def end_checkin_dependlist(self):
        self.pop_tag(TAG_checkin_dependlist)

    ## <defines>
    def start_defines(self, attr):
        self.push_tag(TAG_defines)

    ## </defines>
    def end_defines(self):
        self.pop_tag(TAG_defines)

    ## <ifdef>
    def start_ifdef(self, attr):
        self.push_tag(TAG_ifdef)

    ## </ifdef>
    def end_ifdef(self):
        self.pop_tag(TAG_ifdef)

    ## <ifndef>
    def start_ifndef(self, attr):
        self.push_tag(TAG_ifndef)

    ## </ifndef>
    def end_ifndef(self):
        self.pop_tag(TAG_ifndef)

    ## <umake_includefiles>
    def start_umake_includefiles(self, attr):
        self.push_tag(TAG_umake_includefiles)

    ## </umake_includefiles>
    def end_umake_includefiles(self):
        self.pop_tag(TAG_umake_includefiles)

def install_old_bif_parser():
    global BIFParser
    import xmllib

    class OLDBIFParser(xmllib.XMLParser, BIFParserFunctions):
        def __init__(self, filename, bif_data = None, branchlist = None):
            import xmllib
            xmllib.XMLParser.__init__(self)
            BIFParserFunctions.__init__(self, filename, bif_data, branchlist)

        def handle_data(self, data):
            BIFParserFunctions.handle_data(self, data)
            


    BIFParser=OLDBIFParser

def install_new_bif_parser():
    global BIFParser
    import xml.parsers.expat

    class NewBIFParser(BIFParserFunctions):

        def __init__(self, filename, bif_data = None, branchlist = None):
            import xml.parsers.expat
            self.__parser=xml.parsers.expat.ParserCreate()
            self.__start={}
            self.__end={}

            self.__parser.StartElementHandler = self.__start_element
            self.__parser.EndElementHandler = self.__end_element
            self.__parser.CharacterDataHandler = self.handle_data
            try:
                self.__parser.buffer_text=1
            except AttributeError:
                pass

            for key in BIFParserFunctions.__dict__.keys():
                if key[:6] == 'start_':
                    self.__start[key[6:]]=getattr(self, key)
                if key[:4] == 'end_':
                    self.__end[key[4:]]=getattr(self, key)

            BIFParserFunctions.__init__(self, filename, bif_data, branchlist)

        def parse_bif(self, filename):
            import xml.parsers.expat
            try:
                self.__parser.Parse(open(filename, "r").read(), 1);
            except xml.parsers.expat.ExpatError:
                e=err.Error();
                e.Set("bif parser(line %d): %s" %(
                    self.__parser.ErrorLineNumber,
                    xml.parsers.expat.ErrorString(self.__parser.ErrorLineNumber)));
                raise err.error, e

        def location(self):
            l=self.filename
            if self.last_module:
                l=l+" near module %s" % self.last_module
            else:
                l=l+" near beginning"
            if self.current_tag:
                l=l+" in <%s>" % self.current_tag

            return l


        def __start_element(self, name, attrs):
            if self.__start.has_key(name):
                nattrs={}
                for (key, value) in attrs.items():
                    nattrs[key.encode("iso-8859-1")]=value.encode("iso-8859-1")
                attrs=nattrs
                self.__start[name](attrs)

        def __end_element(self, name):
            if self.__end.has_key(name):
                self.__end[name]()

        def handle_data(self, data):
            data=data.encode("iso-8859-1")
            data=re.sub("\n\\s+","\n", data)
            BIFParserFunctions.handle_data(self, data)

    BIFParser=NewBIFParser

    
## useful...
def module_list_to_module_id_list(module_list):
    module_id_list = []
    for module in module_list:
        module_id_list.append(module.id)
    return module_id_list


## check for circular dependancies
class CircularDependCheck:
    def __init__(self, bif_data):
        self.bif_data = bif_data
        
        self.good_module_list = []
        self.bad_module_list = []

        ## flag that allows us to abort the recursion
        self.abort = 0
        
        ## working stack
        self.stack = []

        ## list of discovered cycles
        self.cycle_list = []

        ## list of module IDs without modules
        self.unknown_module_id_list = []
        
    def run(self):
        for module in self.bif_data.module_hash.values():
            outmsg.verbose("checking module id=\"%s\"" % (module.id))
            self.rc_check(module)

    def rc_check(self, _module):
        ## somthing happened
        if self.abort:
            return
        
        ## if this module is in the good_module_list, then we've
        ## been certified good
        if self.good_module_list.count(_module):
            return
        
        ## check for cycle (circular dependancy)
        if self.stack.count(_module):

            ## add entire stack to the bad module list
            for module in self.stack:
                if not self.bad_module_list.count(module):
                    self.bad_module_list.append(module)

            ## record the discovered cycle
            cycle = self.stack[self.stack.index(_module):]
            if not self.check_duplicate_in_cycle_list(cycle):
                self.cycle_list.append(cycle)

            self.abort = 1
            return

        ## push current module onto the stack
        self.stack.append(_module)

        ## check for a dependancy in the stack
        for module_id in _module.dependancy_id_list:
            try:
                chk_module = self.bif_data.module_hash[module_id]
            except KeyError:
                ## log modules that do not exist
                self.unknown_module_id_list.append((_module.id, module_id))
                continue

            self.rc_check(chk_module)        

        ## if we're not bad, then we're good!
        if not self.bad_module_list.count(_module):
            self.good_module_list.append(_module)

        ## pop off the stack
        self.stack.remove(_module)
        
    def compare_lists(self, list1, list2):
        ## if the lengths don't match, then the lists
        ## don't match
        if len(list1) != len(list2):
            return 0

        for index in range(len(list1)):
            if list1[index] != list2[index]:
                return 0

        return 1

    def check_duplicate_in_cycle_list(self, cycle):
        for temp in self.cycle_list:
            if self.compare_lists(temp, cycle):
                return 1

        return 0
                
    def print_module_list(self, list):
        print '--- MODULE DUMP ---'
        
        for module in list:
            print module.id

        print '------------------'


def CheckBIFData(bif_data):
    cdep_check = CircularDependCheck(bif_data)
    cdep_check.run()

    ## print any circular dependancies
    if len(cdep_check.cycle_list):
        print '*** found BIF circular dependancies ***'
        
        for cycle in cdep_check.cycle_list:
            temp = "bif dependancy cycle=\"%s\"" % (
                string.join(module_list_to_module_id_list(cycle), '->'))
            outmsg.error(temp)

    ## print unresolved module ids
    if len(cdep_check.unknown_module_id_list):
        for unknown in cdep_check.unknown_module_id_list:
            outmsg.error('in dependlist of %s found unknown dependancy %s' % (
                unknown[0], unknown[1]))

def CheckBIFFile(filename):
    if not os.path.isfile(filename):
        print 'file not found'
        sys.exit(1)

    ## load BIF file
    print 'loading...'
    bif_data = load_bif_data(filename)


def rdiff(a, b, done):
    if a == b:
        return 1

    if done.has_key( repr( (a,b) ) ):
        return 1

    done[ repr( (a,b) ) ] = 1

    ta=type(a)
    tb=type(b)

    if ta == tb:
        if ta == types.InstanceType:
            if not rdiff(a.__class__,b.__class__, done):
                print "Class differs"
                return 0
            
            if not rdiff(a.__dict__,b.__dict__, done):
                print "Dict differs"
                return 0

            return 1
            
        if ta == types.DictType:
            for k in a.keys():
                if not b.has_key(k):
                    print "Only in a: %s" % repr(k)
                    return 0
                
            for k in b.keys():
                if not a.has_key(k):
                    print "Only in b: %s" % repr(k)
                    return 0

            for k in a.keys():
                if not rdiff(a[k], b[k], done):
                    print "Value for key %s differs." % repr(k)
                    return 0

            return 1

        if ta == types.TupleType or ta==types.ListType:
            if not rdiff(len(a), len(b), done):
                print "length differs"
                return 0
            n=0
            while n < len(a):
                if not rdiff(a[n], b[n], done):
                    print "index %d differs" % n
                    return 0
                n = n + 1
            return 1

    print "%s != %s " % (repr(a), repr(b))
    print "%s != %s " % (repr(b), repr(a))
    return 0

    
class BIFException( Exception ):
    def __init__( self , value ):
        self.value = value
        
    def __str__( self ):
        return repr( self.value )
        
        
class BIF:
    """New class for better BIF handling. 
    
    Likes to raise BIFException when problems arise.
    
    Methods:
        
        * Actions        
        addModule()
        loadData()
        removeModule()
        saveData()
        
        * Getters        
        getBIFVersion()
        getBuildID()
        getDataAsFormattedString()
        getDataAsString()
        getDataSource()
        getDefaultCVSDate()
        getDefaultCVSRoot()
        getDefaultCVSTag()
        getDefaultCVSTagType()        
        getDefaultOptions()
        getDefaultProfile()
        getDefaults()
        getDefaultTarget()
        getExpiration()
        getModuleName()
        getModules()
        
        * Setters                
        setBuildID()
        setDataSource()
        setDefaultCVSRoot()
        setDefaultCVSTag()
        setDefaultCVSTagType()
    """
    
    #
    # Public Methods.
    #  
    def addModule( self , module ):
        """addModule(m)
        
        Adds module m to the BIF data in memory.
        """
        try:
            log.trace( 'entry' , [ module.id ] )
        except:
            errorString = 'addModule() invoked with proper module object.'
            log.error( errorString )
            raise BIFException( errorString )
        
        self.__checkObject()
        
        try:
            log.info( 'Trying to add module %s to in-memory BIF.' % module.name )
            self.__data.add_module( module )
        except err.error, e:
            log.error( e.text )
            raise BIFException( e.text )
            
        log.info( 'Successfully added module %s.' % module.name )
        log.trace( 'exit' )
        
        
    def loadData( self ):
        """loadData()
        
        Loads the BIF data from the data source.
        
        Precondition: data source must be specified and exist.
        """
        log.trace( 'entry' )
        
        # Check preconditions.
        errorString = ''
        if not self.__dataSource:
            errorString = 'Tried to loadData() with no data source.'
        elif not os.path.exists( self.__dataSource ):
            errorString = 'Data source "%s" does not exist' % self.__dataSource
        else:
            log.info( 'Loading BIF data from %s' % self.__dataSource )
            # For now, this is our wrapper to the old BIF stuff.
            self.__data = load_bif( self.__dataSource )
        
        if errorString:
            log.error( errorString )            
            raise BIFException( errorString )
        
        log.trace( 'exit' )            
                  
            
    def removeModule( self , moduleID ):
        """removeModule(m) 
        
        Removes module m from the bif data."""
        log.trace( 'entry' , [ moduleID ] )

        self.__checkObject()
        
        try:
            name = self.getModuleName( moduleID )
        except KeyError:
            errorString = 'Could not find module "%s" in BIF data.' % moduleID
            log.error( errorString )
            raise BIFException( errorString )
            
        log.info( 'Deleting all traces of module: %s' % moduleID )
        
        # WARNING: this code was not examined when brought over from
        # createpackage.py.  The 'while 1:' bothers me, but the thing works...
        for m in self.__data.module_hash.values():
            log.debug( 'Cleaning %s from module %s' % ( moduleID , m.id ) )
            try:
                while 1:
                    del m.source_dependancy_id_list[m.source_dependancy_id_list.index(moduleID)]
            except ValueError:
                pass
            try:
                while 1:
                    del m.dependancy_id_list[m.dependancy_id_list.index(moduleID)]
            except ValueError:
                pass
        
        del self.__data.module_hash[moduleID]
        
        log.trace( 'exit' )
        

    def saveData( self ):
        """saveData()
        
        Writes the BIF data to the data source.
        
        Precondition: must have a data source specified.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        # Check preconditions.
        if self.__dataSource:
            # Get the data in its writable format.
            data = self.getDataAsFormattedString()
            
            # Try to write the data out to a file.
            try:
                dataSource = open( self.__dataSource , 'w' )
                dataSource.write( data )
                log.info( 'Saved BIF data to %s' % self.__dataSource )
            except:
                log.error( 'Could not save to %s' % self.__dataSource )            
                
            dataSource.close()
        else:
            errorString = 'No data source specified before call to saveData().'
            log.error( errorString )
            raise BIFException( errorString )
        
        log.trace( 'exit' )
        
    
    #
    # Accessors.
    #
    def getBIFVersion( self ):
        """getBIFVersion() --> string
        
        Returns the BIF version as a formatted string.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        version = self.__data.bif_version
        
        major = version / 100
        minor = version % 100
        
        version = '%d.%02d' % ( major , minor )

        log.trace( 'exit' , [ version ] )
        return version
        
        
    def getBuildID( self ):
        """getBuildID() --> string
        
        Returns the buildID.
        """
        # BuildID appears to be build branch, bif branch, or any of a half-dozen
        # other names. This has nothing to do with the 'BuildID' as used in the
        # context of the build database or the build farm.
        log.trace( 'entry' ) 
        
        self.__checkObject() 
        
        id = self.__data.build_id
        
        log.trace( 'exit' , [ id ] )
        return id
    
        
    def getDataAsFormattedString( self ):        
        """getFormattedString() --> string
        
        Returns string formatted for output to the BIF data source.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        # FIXME: One thing was noticed during unit testing: shadow modules
        # [without digging, <attribute id="from_shadow_only"/> is my guess
        # as to the guilty tag.] don't seem to be getting generated in the
        # output data. Is this by design? The dependencies are still there, 
        # which may(?) cause probs.
        data = self.__data.write()
        
        log.trace( 'exit' )
        return data

        
    def getDataAsString( self ):
        """getDataString() --> string
        
        Returns basic data as a string suitable for printing/logging.
        """
        log.trace( 'entry' )
        
        self.__checkObject()  
        
        dataString = ''
        for item in dir( self.__data ):
            value = getattr( self.__data , item )
            if isinstance( value , type({}) ):
                dataString += utils.formatHash( item , value )
            else:
                dataString += ( '%s = %s' % ( item , value  ) ) + '\n'
                
        log.trace( 'exit' )
        return dataString

        
    def getDataSource( self ):
        """getDataSource() --> string
        
        Returns the data source.
        """
        log.trace( 'entry' ) 
                
        source = self.__dataSource
        
        log.trace( 'exit' , [ source ] )
        return source
                
        
    def getDefaultCVSDate( self ):
        """getDefaultCVSDate() --> string
        
        Returns default CVS date as a string of formate YYYY-MM-DD
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        date = self.__data.default_cvs_date
        
        log.trace( 'exit' , [ date ] )
        return date
        
        
    def getDefaultCVSRoot( self ):
        """getDefaultCVSRoot() --> string
        
        Returns default CVS root.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        cvsRoot = self.__data.default_cvs_root
        
        log.trace( 'exit' , [ cvsRoot ] )
        return cvsRoot
        
        
    def getDefaultCVSTag( self ):
        """getDefaultCVSTag() --> string
        
        Returns default CVS tag.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        cvsTag = self.__data.default_cvs_tag
        
        log.trace( 'exit' , [ cvsTag ] )
        return cvsTag
        
        
    def getDefaultCVSTagType( self ):
        """getDefaultCVSTagType() --> string

        Returns default CVS tag type.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        cvsTagType = self.__data.default_cvs_tag_type
        
        log.trace( 'exit' , [ cvsTagType ] )
        return cvsTagType
        
            
    def getDefaultOptions( self ):
        """getDefaultOptions() --> string
        
        Returns default options.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        options = self.__data.default_options
        
        log.trace( 'exit' , [ options ] )
        return options
               
        
    def getDefaultProfile( self ):
        """getDefaultProfile() --> string
        
        Returns default profile.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        profile = self.__data.default_profile
        
        log.trace( 'exit' , [ profile ] )
        return profile
                
        
    def getDefaults( self ):
        """getDefaults() --> list
        
        Returns list of Default objects.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        defaults = self.__data.defaults
        
        log.trace( 'exit' )
        return defaults
        
        
    def getDefaultTarget( self ):
        """getDefaultTarget() --> string
        
        Returns default target.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        target = self.__data.default_target
        
        log.trace( 'exit' , [ target ] )
        return target
       
        
    def getExpiration( self ):
        """getExpiration() --> timestamp
        
        Returns the expiration as a timestamp object.
        """
        log.trace( 'entry' )
        
        self.__checkObject()
        
        ts = self.__data.get_expiration()
        
        log.trace( 'exit' , [ ts.getLocalTicks() ] )
        return ts
        
        
    def getModuleName( self , moduleKey ):
        """getModuleName(k) --> string
        
        Returns the name of the module in the hash with key k.
        """
        # FIXME: public access to module data - push this call down to module.
        log.trace( 'entry' )
        
        self.__checkObject()
        
        name = self.__data.module_hash[ moduleKey ].name
        
        log.trace( 'exit' , [ name ] )
        return name
        
        
    def getModules( self ):
        """getModules() --> list
        
        Returns a list of module objects.
        """
        # TODO: Evaluate whether this is what we really need. Maybe we want
        # the module_hash itself, or maybe we want the keys for reference back
        # into the hash on future calls. Find out where this is being used and 
        # why.
        log.trace( 'entry' )        
        
        self.__checkObject()
        
        modules = self.__data.module_hash.values()
        
        log.trace( 'exit' )
        return modules

        
    def setBuildID( self , buildID ):
        """setBuildID(i)
        
        Sets the BuildID equal to i.
        """
        log.trace( 'entry' , [ buildID ] )        
        
        self.__checkObject()        
        
        self.__data.build_id = buildID        
        
        log.trace( 'exit' )
        

    def setDataSource( self , dataSource ):
        """setDataSource(s)
        
        Sets the data source equal to s.
        """
        log.trace( 'entry' , [ dataSource ] )

        # The data source does not have to exist yet, because we may be
        # specifying a new target for the in-memory data. No checks needed.        
        self.__dataSource = dataSource
        
        log.trace( 'exit' )        
        
        
    def setDefaultCVSRoot( self , cvsRoot ):
        """setDefaultCVSRoot(r)
        
        Sets the default CVS root to r.
        """
        log.trace( 'entry' , [ cvsRoot ] )
        
        self.__checkObject()
        
        self.__data.set_default_cvs_root( cvsRoot )
        
        log.trace( 'exit' )
        
        
    def setDefaultCVSTag( self , cvsTag ):
        """setDefaultCVSTag(t)
        
        Sets the default CVS tag to t.
        """
        log.trace( 'entry' , [ cvsTag ] )
        
        self.__checkObject()
        
        self.__data.set_default_cvs_tag( cvsTag )
        
        log.trace( 'exit' )
        
        
    def setDefaultCVSTagType( self , tagType ):
        """setDefaultCVSTagType(t)
        
        Sets the default CVS tag type to t.
        """
        log.trace( 'entry' , [ tagType ] )
        
        self.__checkObject()
        
        if tagType in cvsTagTypes:
            self.__data.set_default_cvs_tag_type( tagType )
        else:
            errorString = 'CVS Tag Types must be one of: %s' % cvsTagTypes
            log.error( errorString )
            raise BIFException( errorString )
                    
        log.trace( 'exit' )
        
 
    #
    # Constructors.
    #
    def __init__( self ):
        # Private member data.     
        self.__dataSource = None
        self.__data       = None
        
    
    #
    # Private methods.
    #
    def __checkObject( self ):
        """___checkObject()
        
        Checks for proper object initialization. Raises BIFException if object
        data has not been initialized.
        """
        if not self.__data:
            errorString = 'BIF object not properly initialized.'
            log.error( errorString )
            raise BIFException( errorString )
            

            
if __name__ == '__main__':

    def test_bif(bif_path, branchlist):
        ## get the bif_data sturcture
        print "loading %s" % (bif_path)
        try:
            bif_data = load_bif_data(bif_path, branchlist)
        except err.error, e:
            print "Didn't load...."
            print e.Text()
            return
        except:
            e = err.Error()
            e.Set("BIF file %s didn't load..." % bif_path)
            e.SetTraceback(sys.exc_info())
            print
            print e.Text()
            return
            
        open("./biftestfile-tmp.bif","w").write(bif_data.write())
        bif_data2 = load_bif_data("./biftestfile-tmp.bif")

        if not rdiff(bif_data, bif_data2, {}):
            print "Bif file %s not reproducable" % bif_path
            sys.exit(1)

        return bif_data

    import build_exe
    build_exe.call_buildrc()
    
    import getopt

    def main():
        bif_path = ''
        file_flag = 0
        all_flag=0
        branch_list=0
        (opt_list, arg_list) = getopt.getopt(sys.argv[1:], 'fa')

        ## check for the remote build argument
        for opt in opt_list:
            if opt[0] == '-f':
                file_flag = 1
            if opt[0] == '-a':
                all_flag = 1

        if all_flag:
            import branchlist
            branch_list = branchlist.BranchList()
            for branch_name in branch_list.list:
                fname = branch_list.file(branch_name)
                test_bif(fname, branch_list)

            print "Successful test"
            sys.exit(0)

        ## check that there was a argument specified
        if len(arg_list) < 1:
            pname = os.path.basename(sys.argv[0])
            print '%s: invalid usage' % (pname)
            print 'python %s [-f] build-branch' % (pname)
            print 'python %s -a' % (pname)
            print '-f: take argument as path instead of branch specification'
            sys.exit(1)

        ## if we've been given a branch specification, then find
        ## the build information file path for it
        if file_flag:
            bif_path = arg_list[0]
        else:
            import branchlist
            branch_list = branchlist.BranchList()
            bif_path = branch_list.file(arg_list[0])
            if not bif_path:
                print '%s invalid branch' % (arg_list[0])
                sys.exit(1)

        print test_bif(bif_path, branch_list).write()

    main()
    #import profile
    #profile.run('main()')
    
    
