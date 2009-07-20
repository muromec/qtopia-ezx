#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_pb.py,v 1.45 2007/04/17 22:40:29 milko Exp $ 
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

""" Projectbuild backend for umake """

## Python imports
import re
import string
import types
import os
import copy
import marshal
import re

if __name__ == '__main__':
    import sys
    ## add Python import paths
    build_root = os.environ['BUILD_ROOT']
    sys.path.insert(0, os.path.join(build_root, 'lib'))
    sys.path.insert(0, os.path.join(build_root, 'umake'))


## Local imports
import log
log.debug( 'Imported: $Id: umake_pb.py,v 1.45 2007/04/17 22:40:29 milko Exp $' )

import umake_lib
import umake_makefile
import shell
import makefile
import err


## objectVersion corresponds to the same value in the
## project files.
## PB=38
## XCode 1.2=39
## Xcode 2.1=42

objectVersion=38

import sysinfo
if 'xcode' in sysinfo.family_list:
    objectVersion=39

if 'xcode21' in sysinfo.family_list:
    objectVersion=41

if 'xcode22' in sysinfo.family_list:
    objectVersion=42

## This codec class is used to translate an in-memory
## structure to a PB/Xcode project file.
## It is also able to parse project files into in-memory structures
class codec:

    def __init__(self):
        import time
        self.timestamp=int(time.time())

    # Whitespace
    ws_reg=re.compile("(([ \r\n\t])|(//[^\r\n]*))*")

    # Word
    word_reg=re.compile("[^ \r\n\t;,]*")

    # Int
    int_reg=re.compile("^[0-9][0-9]*$")

    # Whitespace semicolon
    ws_semi_ws_reg=re.compile("(([ \r\n\t])|(//[^\r\n]*))*;(([ \r\n\t])|(//[^\r\n]*))*")

    # Comma whitespace
    ws_comma_ws_reg=re.compile("(([ \r\n\t])|(//[^\r\n]*))*,(([ \r\n\t])|(//[^\r\n]*))*")

    # Whitespace equal sign whitespace
    ws_eq_ws_reg=re.compile("(([ \r\n\t])|(//[^\r\n]*))*=(([ \r\n\t])|(//[^\r\n]*))*")

    ## Set pos to the next non-whitespace
    def skipspace(self):
        self.pos = self.ws_reg.match(self.data, self.pos).end()
        return
    
        while 1:
            #while self.data[self.pos] in [" ", "\r", "\n", "\t"]:
            #    self.pos=self.pos+1

            self.pos = self.ws_reg.match(self.data, self.pos).end()

            if self.data[self.pos:self.pos+2] != "//":
                return

            while self.data[self.pos] not in ["\r", "\n"]:
                self.pos=self.pos+1

    ## Parse a word
    def get_word(self):
        self.skipspace()
        start=self.pos

        self.pos = self.word_reg.match(self.data, self.pos).end()
        #while self.data[self.pos] not in [" ", "\r", "\n", "\t", ";", ","]:
        #    self.pos=self.pos+1

        return self.data[start:self.pos]

    ## If next part is "str", go past it and return true
    def gobble(self, str):
        if self.data[self.pos:self.pos+len(str)] == str:
            self.pos=self.pos+len(str)
            return 1
        else:
            return 0

    ## If the next character is "str" skip past it and return true
    def gobble1(self, str):
        if self.data[self.pos] == str:
            self.pos=self.pos+1
            return 1
        else:
            return 0

    ## Parse and unqote a string
    def get_string(self):
        ret=[]
        q=string.find(self.data,'"',self.pos)
        s=string.find(self.data,'\\',self.pos)

        while s != -1 and s < q:
            ret.append(self.data[self.pos:s])
            ret.append(self.data[s+1])
            self.pos=s+2
            s=string.find(self.data,'\\',self.pos)
            if q < self.pos:
                q=string.find(self.data,'"',self.pos)

        ret.append(self.data[self.pos:q])
        self.pos=q+1

        return string.join(ret,"")

    ## Read a plist and return a structure
    def unserialize(self):
        # print "pos = %d %s" %(self.pos, repr(self.data[self.pos:self.pos+8]))
        self.skipspace()

        if self.gobble1("{"): ## hash
            ret={}
            self.skipspace()
            while not self.gobble1("}"):
                key=self.unserialize()
                self.pos = self.ws_eq_ws_reg.match(self.data, self.pos).end()
                ret[key]=self.unserialize()
                self.pos = self.ws_semi_ws_reg.match(self.data, self.pos).end()

            return ret

        if self.gobble1("("): ## list
            ret=[]
            self.skipspace()
            while not self.gobble1(")"):
                ret.append(self.unserialize())
                self.pos = self.ws_comma_ws_reg.match(self.data, self.pos).end()
            return ret

        if self.gobble1('"'): ## string
            return self.get_string()

        return self.get_word()
        #if self.int_reg.match(w):
        #    return int(w)
        #return w
            
            
    ############################################

    str_unquoted_reg=re.compile("^[a-zA-Z./0-9_]*$")

    ## Take a structure, return a plist
    def serialize(self, data, indent=0):
        t = type(data)
        if t == types.StringType:
            if data and self.str_unquoted_reg.match(data):
                return data
            return '"%s"' % string.replace(string.replace(data,"\\","\\\\"),'"','\\"')

        if t == types.DictType:
            indent = indent+1
            ret=[ "{" ]
            keys=data.keys()
            keys.sort()
            for key in keys:
                if data[key] != None:
                    ret.append("\t%s = %s;" % (
                        self.serialize(key),
                        self.serialize(data[key], indent)))
            ret.append("}")
            return string.join(ret,"\n" + "\t" * indent)

        if t == types.IntType:
            return str(data)

        if t == types.ListType:
            indent = indent+1
            ret=[ "(" ]
            for v in data:
                ret.append("\t%s," % self.serialize(v, indent))
            ret.append(")")
            return string.join(ret,"\n" + "\t" * indent)

    ############################################


    ## Replace pointers with string references to make
    ## circular references possible. Just like xcode does.
    def mangle(self, data):
        t=type(data)

        if t == types.InstanceType:
            if self.ids.has_key(data):
                return self.ids[data]

            id = "11ADDED1%08X%08X" % (self.timestamp, self.counter)
            self.counter = self.counter + 1
            self.ids[data]=id

            tmp = self.mangle(data.__dict__)
            tmp["isa"]=data.__class__.__name__
            self.data["objects"][id]=tmp
            return id
                
        if t == types.DictType:
            ret={}
            for k in data.keys():
                ret[k]=self.mangle(data[k])
            return ret

        if t == types.ListType:
            return map(self.mangle, data)

        #print "LITERAL: %s" % repr(data)
        return data

    def convert(self,data):
        self.counter=1
        self.ids={}

        self.data={
            "archiveVersion" :1,
            "objectVersion" : objectVersion,
            "classes" : {},
            "objects" : {},
            }

        self.data["rootObject"]=self.mangle(data)

        return self.data


    ############################################

    def unmangle(self, data):
        t=type(data)

        if t == types.StringType:
            if self.objects.has_key(data):
                return self.objects[data]
            return data

        if t == types.DictType:
            ret={}
            for k in data.keys():
                ret[k]=self.unmangle(data[k])
            return ret

        if t == types.ListType:
            return map(self.unmangle, data)

        return data


    def unconvert(self, data):
        if type(data) == types.StringType:
            self.data=data
            self.pos=0
            data=self.unserialize()
        
        ## Instanciate all objects
        self.objects={}
        for o in data["objects"].keys():
            odata=data["objects"][o]
            self.objects[o]=globals()[odata["isa"]]()

        for o in data["objects"].keys():
            ob=self.objects[o]
            odata=data["objects"][o]
            for k in odata.keys():
                ob.__dict__[k]=self.unmangle(odata[k])

        return self.objects[data["rootObject"]]
        
    ############################################

    ## Convert to readable format
    def pp(self, data, indent=0):
        ret=[]
        t=type(data)
        if t == types.InstanceType:
            if self.done.has_key(data):
                return "$%s" % self.done[data]
            num=len(self.done)
            self.done[data]=num

            return "$%d = %s %s" % (
                num,
                data.__class__.__name__,
                self.pp(data.__dict__, indent))

        if t == types.DictType:
            if not data:
                return "{}"
            ret=["{ // %d element(s)" % len(data) ]
            for k in data.keys():
                if k != "isa":
                    ret.append("  %s = %s;" %(k,self.pp(data[k],indent+1)))
            ret.append("}");
            return string.join(ret,"\n" + ("  " * indent))

        if t == types.ListType:
            if not data:
                return "()"
            ret=["( // %d element(s)" % len(data) ]
            for k in data:
                ret.append("  %s," % (self.pp(k, indent+1)))
            ret.append(")");
            return string.join(ret,"\n" + ("  " * indent) )

        return self.serialize(data)

            

    def prettyprint(self,data):
        self.counter=1
        self.done={}
        return self.pp(data,0)



## PBX classes...
## All of the classes below correspond directly with
## with classes from the xcode project files
    
class PBXFileReference:
    def __init__(self, path=None):
        self.fileEncoding = 30 ## int (4=? 30=?)
        # self.name = None       ## string (optional)
        self.path = path       ## string
        self.refType = 4       ## int (0=ABS,3=output path rel?,4=group rel)
        if path and os.path.isabs(path):
            self.refType=0

        ## XCODE support
        if objectVersion > 38:
            self.sourceTree = "<group>"
            if self.refType == 0:
                self.sourceTree = "<absolute>"

            if path:
                p, ext=os.path.splitext(path)

                if ext == ".strings":
                    self.fileEncoding=10
                    self.expectedFileType="text.plist.strings"

                if ext == ".plist":
                    self.fileEncoding=4
                    self.expectedFileType="text.plist"

                if ext == ".bundle":
                    self.expectedFileType="wrapper.cfbundle"

                if ext == ".app":
                    self.expectedFileType="wrapper.application"

                if ext == ".nib":
                    self.expectedFileType="wrapper.nib"

                if ext == ".framework":
                    self.expectedFileType="wrapper.framework"

                if ext == ".a":
                    self.expectedFileType="archive.ar"
                
                if string.lower(ext) in [".c"]:
                    self.expectedFileType="sourcecode.c.c"

                if string.lower(ext) in [".cpp",".cc",".cxx"]:
                    self.expectedFileType="sourcecode.cpp.cpp"

                if string.lower(ext) in [".h",".hh"]:
                    self.expectedFileType="sourcecode.c.h"

            if self.__class__ != PBXFileReference:
                self.fallbackIsa="PBXFileReference"

    def copy(self):
        return self

class PBXApplicationReference(PBXFileReference):
    pass
        
class PBXLibraryReference(PBXFileReference):
    pass
        
class PBXFrameworkReference(PBXFileReference):
    pass

class PBXFolderReference(PBXFileReference):
    pass

class PBXBundleReference(PBXFileReference):
    pass

class PBXBuildFile:
    def __init__(self, ref = None, settings = None):
        self.settings={}  ## Hash, { "ATTRIBUTES":[] }
        self.fileRef=ref  ## PBXFileReference
        if settings:
            self.settings = settings

    def copy(self):
        return copy.copy(self)

class PBXBuildStyle:
    def __init__(self):
        self.buildRules = []     ## array of ???????
        self.buildSettings = {}  ## hash, overrides settings in PBX*Target ?
        self.name = None         ## Name of this build style

    def copy(self):
        ret=self.copy(ret)
        ret.buildRules=copy_array(self.BuildRules)
        ret.buildSettings=copy.copy(ret.buildSettings)
        return ret
    
class PBXExecutable:
    def copy(self):
        return self


class PBXBuildPhase:
    def __init__(self):
        self.buildActionMask = 0x7fffffff ## int
        self.files = []                   ## array of PBXBuildFile
        self.runOnlyForDeploymentPostprocessing = 0  ## int

    def copy(self):
        ret = copy.copy(self)
        ret.files=copy_array(self.files)
        return ret

class PBXHeadersBuildPhase(PBXBuildPhase):
    pass

class PBXResourcesBuildPhase(PBXBuildPhase):
    pass

class PBXSourcesBuildPhase(PBXBuildPhase):
    pass

class PBXFrameworksBuildPhase(PBXBuildPhase):
    pass

class PBXRezBuildPhase(PBXBuildPhase):
    pass


class PBXShellScriptBuildPhase(PBXBuildPhase):
    def __init__(self, cmd="", shell="/bin/sh"):
        PBXBuildPhase.__init__(self)
        self.neededFileNames=[]
        self.generatedFileNames=[]
        self.shellPath=shell
        self.shellScript=cmd
        
    def copy(self):
        ret = copy.copy(self)
        ret.neededFileNames=copy_array(self.neededFileNames)
        ret.generatedFileNames=copy_array(self.generatedFileNames)
        return ret

class PBXCopyFilesBuildPhase(PBXBuildPhase):
    def __init__(self, to=None):
        PBXBuildPhase.__init__(self)
        if to:
            self.dstPath=to
            self.dstSubfolderSpec=16  ## product relative

def copy_array(arr):
    ret=[]
    for x in arr:
        ret.append(x.copy())
    return ret

class PBXNativeTarget:
    def __init__(self, name = None):
        self.buildPhases = []   ## array of PBX*BuildPhase
        self.buildSettings = {} ## A bunch of settingss
        self.dependencies = []  ## Array of PBXTargetDependency
        self.name = name        ## string, name of target
        self.productName = "Ribosome Project" ## String, purpose ?
        self.productSettingsXML = None ## XML data (optional?)

    def copy(self):
        ret = copy.copy(self)
        ret.buildSettings = copy.copy(ret.buildSettings)
        ret.buildPhases = copy_array(ret.buildPhases)
        ret.dependencies = copy_array(self.dependencies)
        return ret

class PBXLibraryTarget(PBXNativeTarget):
    pass

class PBXBundleTarget(PBXNativeTarget):
    pass

class PBXApplicationTarget(PBXNativeTarget):
    pass

class PBXFrameworkTarget(PBXNativeTarget):
    pass

class PBXAggregateTarget(PBXNativeTarget):
    pass

if objectVersion > 38:
    def PBXLibraryTarget(name=None):
        ret=PBXNativeTarget(name)
        ret.productType="com.apple.product-type.library.static"
        return ret;

    def PBXApplicationTarget(name=None):
        ret=PBXNativeTarget(name)
        ret.productType="com.apple.product-type.application"
        return ret;

    def PBXBundleTarget(name=None):
        ret=PBXNativeTarget(name)
        ret.productType="com.apple.product-type.bundle"
        return ret;

    def PBXFrameworkTarget(name=None):
        ret=PBXNativeTarget(name)
        ret.productType="com.apple.product-type.framework"
        return ret;



class PBXLegacyTarget(PBXNativeTarget):
    def __init__(self, name = None):
        PBXNativeTarget.__init__(self, name)
        self.buildArgumentsString="$ACTION"
        self.settingsToPassInEnvironment = -1
        self.settingsToExpand=0
        self.settingsToPassOnCommandLine=0
        self.passBuildSettingsInEnvironment=1
        self.buildToolPath="/usr/bin/gnumake"

    def copy(self):
        return copy.copy(self)

class PBXContainerItemProxy:
    def __init__(self, remote=None, containerportal=None):
        self.proxyType=1
        if remote:
            self.remoteGlobalIDString = remote
            self.remoteInfo = remote.name
        self.containerPortal = containerportal

    def copy(self):
        return copy.copy(self)

class PBXGroup:
    def set_path(self, path = None):
        if path:
            self.refType = 4
            self.path = path
            if os.path.isabs(path):
                self.refType = 0

        if objectVersion > 38:
            self.sourceTree = "<group>"
            if self.refType == 0:
                self.sourceTree = "<absolute>"
            
    def __init__(self, name=None, path=None):
        self.children = []    ## array of objects (references?)
        self.name = name           ## String
        self.refType = 4           ## int
        # self.path

    def copy(self):
        return self

    def add(self, pbxsomething):
        self.children.append(pbxsomething)

    def add_file(self, sourcefile):
        self.add(PBXFileReference(sourcefile))
    

class PBXVariantGroup(PBXGroup):
    pass


class PBXProject:
    def __init__(self):
        self.buildStyles = []          ## array of PBXBuildStyle
        if objectVersion == 41:
            initialSettings = {}
            initialSettings["MACOS_DEPLOYMENT_TARGET"] = "10.3"
            initialSettings["SDKROOT"] = "/Developer/SDKs/MacOSX10.3.9.sdk"
            initialSettings["GCC_VERSION"] = "3.3"
            self.buildSettings = initialSettings
        if objectVersion >= 42:
            initialSettings = {}
            initialSettings["MACOS_DEPLOYMENT_TARGET"] = "10.4"
            initialSettings["SDKROOT"] = "/Developer/SDKs/MacOSX10.4u.sdk"
            # if I need to build the Intel side with gcc4 and the ppc
            # side with gcc3 I can do something like the following:
            # initialSettings["MACOS_DEPLOYMENT_TARGET_ppc"] = "10.3"
            # initialSettings["SDKROOT_ppc"] = "/Developer/SDKs/MacOSX10.3.9.sdk"
            # initialSettings["GCC_VERSION_ppc"] = "3.3"
            # initialSettings["MACOS_DEPLOYMENT_TARGET_i386"] = "10.4"
            # initialSettings["SDKROOT_i386"] = "/Developer/SDKs/MacOSX10.4u.sdk"
            # initialSettings["GCC_VERSION_i386"] = "4.0"
            if 'xcode23' in sysinfo.family_list:
                initialSettings["DEBUG_INFORMATION_FORMAT"] = "dwarf"

            self.buildSettings = initialSettings
        self.hasScannedForEncodings=1  ## int
        self.mainGroup = PBXGroup("All Files")    ## PBXGroup
        self.projectDirPath=""         ## string
        self.targets = []              ## array of PBX*Target
        # self.projectRefGroup = None  ## PBXGroup

    def copy(self):
        return self


class PBXTargetDependency:
    def __init__(self, target = None, root=None):
        self.target=target # a PBX*Target

        if objectVersion > 38:
            self.targetProxy = PBXContainerItemProxy(target, root)

    def copy(self):
        return copy.copy(self)

###########################################################

## This class represents a projectbuilder project
## It is used to keep track of all the meta-information
## used to build a hierarchy of instances of the classes
## above.
    
class projectbuilder:

    def __init__(self):
        self.pbx=PBXProject()
        self.group_cache={
            "/": self.pbx.mainGroup,
            "": self.pbx.mainGroup
            }

        self.file_cache = {}
        self.basename=""

    ## This creates a group in the file view
    def create_group(self, path):
        if self.group_cache.has_key(path):
            return self.group_cache[path]
        dir, name = os.path.split(path)
        parent = self.create_group(dir)
        if name:
            group = PBXGroup(name)
            parent.add(group)
        else:
            group = parent
        self.group_cache[path] = group
        return group


    ## Adds a file to the project, creates
    ## groups for directories
    def add_file(self, file, cls, prefix, where):
        if self.file_cache.has_key(prefix+file):
            return self.file_cache[prefix+file]
        dir, name = os.path.split(file)

        g=self.create_group(os.path.join(self.basename,
                                         where,
                                         dir))
        f=cls(file)
        g.add(f)
        f.name=name
        self.file_cache[prefix+file]=f
        return f

    ## I don't remember exactly why this is required, but
    ## it adds a file to a project without creating groups
    ## for directories. This is primarily used for output files
    def add_file_basename(self, file, cls, prefix, where):
        if self.file_cache.has_key(prefix+file):
            return self.file_cache[prefix+file]
        dir, name = os.path.split(file)

        g=self.create_group(os.path.join(self.basename,
                                         where))
        f=cls(file)
        f.name=name
        g.add(f)
        self.file_cache[prefix+file]=f
        return f
        

    ## The following functions add particular types of files to the
    ## project.
    
    def add_source_file(self, file,  where="Sources"):
        return self.add_file(file, PBXFileReference, "F", where);

    def add_library(self, file,  where="Libraries & Frameworks"):
        return self.add_file_basename(file, PBXLibraryReference, "L", where);

    def add_framework(self, file,  where="Libraries & Frameworks"):
        return self.add_file_basename(file, PBXFrameworkReference, "F", where);

    def add_application(self, file, where="Products"):
        return self.add_file(file, PBXApplicationReference, "A", where);

    def add_bundle(self, file, where="Products"):
        return self.add_file(file, PBXBundleReference, "B", where);

    def add_bundle(self, file, where="Products"):
        return self.add_file(file, PBXFrameworkReference, "f", where);


    def add_resource_file(self, file,  where="Resources"):
        return self.add_file_basename(file, PBXFileReference, "r", where);

    def add_resource_folder(self, file,  where="Resources"):
        return self.add_file_basename(file, PBXFolderReference, "R", where);


    ## Save a marshalled version of the project which
    ## can be read back quickly when creating ubers

    def quicksave(self, name):
        c=codec()
        sdata=c.convert(self.pbx)
        marshal.dump(sdata, open(name+"-pbtmp","w"))


    ## Save project file (and quicksave)

    def save(self, name):
        if objectVersion == 38:
            dirname="%s.pbproj"
        elif objectVersion <= 41:
            dirname="%s.xcode"
        else:
            dirname="%s.xcodeproj"
        dirname = dirname % name
        #print "Saving: %s/project.pbxproj" % dirname
        shell.mkdir(dirname)
        c=codec()
        sdata=c.convert(self.pbx)
        data=c.serialize(sdata);

        file=dirname+"/project.pbxproj"
        contents=""

        if os.path.exists(file):
            contents=open(file,"r").read()

        if contents != data:
            open(file,"w").write(data)

        #marshal.dump(sdata, open(name+"-pbtmp","w"))

        #print "Saving:\n"
        #print codec().prettyprint(self.pbx)

        #print "Result:\n"
        #print codec().prettyprint(codec().unconvert(open(dirname+"/project.pbxproj").read()))


    ## Ugly hack to fix relative project paths to be relative to
    ## a new directory. This is needed when combining projects into
    ## uber-projects.
        
    def fix_rel_path(self, f, all=1, dir=None):
        if os.path.isabs(f):
            return f

        n=os.path.join(dir, f)
        n=os.path.normpath(n)
        if all:
            return n

        if f[0] == '-':
            return f;

        ## Ugly
        if not os.path.exists(n):
            return f

        return n

    ## Call fix_rel_path on a bunch of paths
    def fix_rel_paths(self, paths, all=1, dir=None):
        ret=[]
        for path in string.split(paths):
            ret.append(self.fix_rel_path(path, all, dir))
        return string.join(ret)

    ## Fix a particular setting
    def fix_setting(self, t, y, all=1, dir=None):
        if t.buildSettings.has_key(y):
            t.buildSettings[y]=self.fix_rel_paths(t.buildSettings[y], all, dir)

    ## Add a project to this project file
    def AddProject_nofix(self, filename):
        if not os.path.exists(filename+"-pbtmp"):
            print "UMAKE WARNING: Unable to read %s" % (filename+"-pbtmp")
            return []

        data=marshal.load(open(filename+"-pbtmp"))
        x=codec().unconvert(data)

        self.pbx.targets.extend(x.targets)

        for group in x.mainGroup.children:
            self.pbx.mainGroup.children.append(group)

        return x.targets

    ## Add a project and fix relative paths
    def AddProject(self, filename, suffix=""):
        if not os.path.exists(filename+"-pbtmp"):
            print "UMAKE WARNING: Unable to read %s" % (filename+"-pbtmp")
            return []

        data=marshal.load(open(filename+"-pbtmp"))
        x=codec().unconvert(data)

        dir=os.path.dirname(filename)

        for t in x.targets:
            t.name = t.name + suffix
            
            self.fix_setting(t, "OTHER_LDFLAGS", 0, dir)
            self.fix_setting(t, "PREFIX_HEADER", 0, dir)
            self.fix_setting(t, "OBJROOT", 0, dir)
            self.fix_setting(t, "DSTROOT", 0, dir)
            self.fix_setting(t, "SYMROOT", 0, dir)
            self.fix_setting(t, "TEMP_DIR", 0, dir)
            self.fix_setting(t, "HEADER_SEARCH_PATHS", 0, dir)
            self.fix_setting(t, "LIBRARY_SEARCH_PATHS", 0, dir)
            self.fix_setting(t, "FRAMEWORK_SEARCH_PATHS", 0, dir)
            self.fix_setting(t, "RIBOSOME_OUTPUT_DIR", 0, dir)
            self.fix_setting(t, "DEPLOYMENT_LOCATION", 0, dir)
            self.fix_setting(t, "INFOPLIST_FILE", 0, dir)
            self.fix_setting(t, "EXPORTED_SYMBOLS_FILE", 0, dir)

            #p=os.path.join("...",dir,"build",t.productName)
            #p=os.path.normpath(p)
            #t.productName = p
            #t.buildSettings["PRODUCT_NAME"] = p
            #t.productReference.path = p
           
            
        self.pbx.targets.extend(x.targets)

        for group in x.mainGroup.children:
            group.set_path(dir)
            self.pbx.mainGroup.children.append(group)

        return x.targets

    ## Add copy target
    def CopyTargets(self, targets, suffix=""):
        ret = []
        for t in targets:
            t=t.copy()
            t.name=t.name+suffix
            ret.append(t)

        self.pbx.targets.extend(ret)

        return ret;

ranlib_cache={}


## This class reads the umake settings and
## translates them PB/Xcode things, mostly
## utilizing calls in the projectbuilder class
## above.
class project_generator(umake_lib.Targets):


    ## Add a header file
    def add_header(self, header, attribs = None):
        if self.header_cache.has_key(header):
            bf=self.header_cache[header]
        else:
            bf=PBXBuildFile(self.pb.add_source_file(header,"Headers"))
            self.header_phase.files.append(bf)
            self.header_cache[header]=bf

        if attribs:
            if not bf.settings.has_key("ATTRIBUTES"):
                bf.settings = { "ATTRIBUTES" : attribs }
            else:
                for a in attribs:
                    if a not in bf.settings["ATTRIBUTES"]:
                        bf.settings["ATTRIBUTES"].append(a)

    ## All the work is done in __init__
    def __init__(self, platform, project):
        self.header_cache = {}
        
        umake_lib.Targets.__init__(self, platform, project)

        pb=projectbuilder()
        self.pb=pb
        settings = {}
        
        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)
        pb.basename=name


        if project.getTargetType() == "":
            target=PBXLegacyTarget(project.makefile_name)
            target.buildArgumentsString='-f "%s" $ACTION'%project.makefile_name
            self.target=target
            pb.pbx.targets.append(target)

            ## Call the standard UNIX makefile generator here..
            umake_makefile.makefile_generator(platform, project)
        else:

            ## Call makefile generator here, we can use the variables
            ## to set up the C compiler and other stuff
            mf_generator = pb_makefile_generator(platform, project)
            mf=mf_generator.mfile

            output_name = self.project.getOutputName()

            if project.getTargetType() == "exe":
                output_name=os.path.splitext(output_name)[0]
                target=PBXApplicationTarget(name)
                target.productReference=pb.add_application(output_name,"Products")

            elif project.getTargetType() == "lib":
                if objectVersion > 38:
                    output_name = output_name[3:-2]
                target=PBXLibraryTarget(name)
                target.productReference=pb.add_library(output_name,
                                                       "Products")
                settings["LIBRARY_STYLE"]="STATIC"

            elif project.getTargetType() == "dll":
                output_name=os.path.splitext(output_name)[0]
                if project.create_framework:
                    target=PBXFrameworkTarget(name)
                    target.productReference=pb.add_framework(output_name,
                                                             "Products")
                else:
                    target=PBXBundleTarget(name)
                    target.productReference=pb.add_bundle(output_name,
                                                      "Products")
                    if objectVersion == 38:
                        settings["LIBRARY_STYLE"]="Bundle"
                        settings["WRAPPER_EXTENSION"]="bundle"

            else:
                return
            
            ## Output relative file
            #full_name = os.path.join("..", project.output_dir, output_name)
            full_name = output_name
            target.productReference.refType=3
            target.productName = full_name

            if objectVersion > 38:
                target.sourceTree = "BUILT_PRODUCTS_DIR"
                target.includeInIndex = 0
                
            settings["PRODUCT_NAME"] = full_name
            target.productReference.path = full_name

            self.header_phase = PBXHeadersBuildPhase()
            self.resource_phase = PBXResourcesBuildPhase()
            self.source_phase = PBXSourcesBuildPhase()
            self.framework_phase = PBXFrameworksBuildPhase()
            self.rez_phase = PBXRezBuildPhase()

            target.buildPhases.extend( [
                self.header_phase,
                self.resource_phase,
                self.source_phase,
                self.framework_phase,
                self.rez_phase,
                ])

            self.target=target
            pb.pbx.targets.append(target)

            settings["OTHER_LDFLAGS"] = ""
            settings["OTHER_REZFLAGS"] = ""
            if objectVersion == 38:
                settings["DEBUG_SYMBOLS"]="NO"
            else:
                settings["GCC_GENERATE_DEBUGGING_SYMBOLS"]="NO"
            #settings["SYMROOT"]=project.output_dir
            #settings["OBJROOT"]=os.path.join(project.output_dir,"obj")
            #settings["TEMP_DIR"]=os.path.join(project.output_dir,"obj",name)

            ## Speedup
            if not self.project.BuildOption("save-disk"):
                if objectVersion == 38:
                    settings["PRECOMPILE_PREFIX_HEADER"]="YES"
                else:
                    settings["GCC_PRECOMPILE_PREFIX_HEADER"]="YES"

            vars = mf.get_variables()
            tmp_vars = copy.deepcopy(vars)

            tmp_vars["INCLUDES"] = "" ## INCLUDES are handled elsewhere
            flags=makefile.expand_variables("$(CCFLAGS)",  tmp_vars)
            c_flags=[]
            opt_flags=[]
            warn_flags=[]

            state = ""
            debug="NO"

            for flag in string.split(flags):
                if state:
                    if state == "prefix_header":
                        settings["PREFIX_HEADER"]=flag
                    state=""
                else:
                    ft=flag[:2]
                    if ft == "-W":
                        warn_flags.append(flag)
                    elif ft == "-O":
                        opt_flags.append(flag)
                    elif ft == "-g":
                        debug="YES"
                        if objectVersion == 38:
                            if settings.has_key("DEBUG_SYMBOLS"):
                                del settings["DEBUG_SYMBOLS"]
                        else:
                            settings["GCC_GENERATE_DEBUGGING_SYMBOLS"]="YES"
                    elif flag in [ "-p", "-pg" ]:
                        settings["PROFILING_CODE"]="YES"
                    elif flag == "-include":
                        state="prefix_header"
                    elif flag == "-faltivec" and objectVersion > 38:
                        settings["GCC_ALTIVEC_EXTENSIONS"]="YES"
                    elif flag == "-fobjec-exceptions":
                        settings["GCC_ENABLE_OBJC_EXCEPTIONS"]="YES"
                    else:
                        c_flags.append(flag)

            settings["OTHER_CFLAGS"] = string.join(c_flags)
            settings["WARNING_CFLAGS"] = string.join(warn_flags);
            settings["OPTIMIZATION_CFLAGS"] = string.join(opt_flags);

            settings["HEADER_SEARCH_PATHS"] = string.join(project.includes," ")
            settings["FRAMEWORK_SEARCH_PATHS"] = ""

            #settings["DSTROOT"]=self.project.target_dir
            settings["DEPLOYMENT_LOCATION"]=""
            settings["RIBOSOME_OUTPUT_DIR"]=project.output_dir

            if objectVersion > 38:
                settings["COPY_PHASE_STRIP"]="NO"
                settings["GCC_ENABLE_FIX_AND_CONTINUE"]=debug
                settings["ZERO_LINK"]="no"
                settings["CONFIGURATION_BUILD_DIR"]="$(BUILD_DIR)"
                if objectVersion == 41:
                    settings["MACOSX_DEPLOYMENT_TARGET"]="10.3"
                elif objectVersion >= 42:
                    settings["MACOSX_DEPLOYMENT_TARGET"]="10.4"
                    # arguably we should only do universal binaries
                    # for release builds.
                    some_architectures=[]
                    some_architectures.append("ppc")
                    some_architectures.append("i386")
                    settings["ARCHS"]=string.join(some_architectures)

                    if not project.BuildOption("debug"):
                        settings["GCC_SYMBOLS_PRIVATE_EXTERN"]="NO"

                        if self.project.getTargetType() == "dll":
                            expdir = self.project.output_dir
                            shell.mkdir(expdir)
                            expfile = os.path.join(self.project.module_directory(),
                                               self.project.makefile_name)
                            expfile = umake_lib.declaw_name(expfile)+".exp"
                            expfile=os.path.join(expdir, expfile)
                            settings["EXPORTED_SYMBOLS_FILE"]=expfile
            else:
                if not project.BuildOption("debug"):
                    settings["DEPLOYMENT_POSTPROCESSING"]="YES"

            target.buildSettings = settings

            ## Sources
            for file in project.sources:
                ext = os.path.splitext(file)[1]
                if ext in [ ".r" ]:
                    self.rez_phase.files.append(PBXBuildFile(
                        self.pb.add_source_file(file)))
                else:
                    self.source_phase.files.append(PBXBuildFile(
                        self.pb.add_source_file(file)))

            ## Resources, add res/mac-projectbuilder/*

            for p in project.resource_directories:
                variants={}
                if os.path.isdir(p):
                    for f in os.listdir(p):
                        if f in [ "CVS", ".DS_Store" ]:
                            continue

                        full_name=os.path.join(p,f)
                        if os.path.isdir(full_name):
                            if full_name[-6:] == ".lproj":
                                for res in os.listdir(full_name):
                                    if res in [ "CVS", ".DS_Store" ]:
                                        continue

                                    full_res=os.path.join(full_name, res)
                                    if variants.has_key(res):
                                        variants[res].append(full_res)
                                    else:
                                        variants[res]=[full_res]
                                continue
                            else:
                                tmp=self.pb.add_resource_folder(full_name, "Resources/"+os.path.basename(p))
                        else:
                            tmp=self.pb.add_resource_file(full_name, "Resources/"+os.path.basename(p))

                        self.resource_phase.files.append(PBXBuildFile(tmp))

                ## Create Variants
                res_group=self.pb.create_group(os.path.join(pb.basename,
                                                            "Resources/"+os.path.basename(p)))

                for var in variants.keys():
                    g=PBXVariantGroup(var)

                    for res in variants[var]:
                        f=PBXFileReference(res)
                        f.name=os.path.basename(string.split(res,".lproj")[0])
                        g.children.append(f)

                    res_group.children.append(g)
                    self.resource_phase.files.append(PBXBuildFile(g))


            ## Integrate copy phases
            for (f, t) in project.copy_phases:
                cp_phase = PBXCopyFilesBuildPhase()
                cp_phase.dstPath=t
                cp_phase.dstSubfolderSpec=1
                cp_phase.buildActionMask=12
                cp_phase.files=[]
                for file in f:
                    if os.path.isdir(file):
                        tmp=self.pb.add_resource_folder(file, "Other files")
                    else:
                        tmp=self.pb.add_resource_file(file, "Other files")
                    cp_phase.files.append(PBXBuildFile(tmp))


                target.buildPhases = [ cp_phase ] + target.buildPhases

            ## UGLY, we add *ALL* header files here
    ##         potential_headers = ["."]
    ##         for h in potential_headers:
    ##             if os.path.isdir(h):
    ##                 for f in os.listdir(h):
    ##                     potential_headers.append(os.path.join(h,f))
    ##             else:
    ##                 if string.lower(os.path.splitext(h)[1]) == ".h":
    ##                     h=os.path.normpath(h)
    ##                     self.add_header(h)


            for dir in project.includes:
                c=os.getcwd()
                fd=os.path.normpath(os.path.join(c,dir))
                if fd[:len(c)] != c:
                    continue
                if os.path.isdir(dir):
                    for f in os.listdir(dir):
                        h=os.path.join(dir,f)
                        if string.lower(os.path.splitext(h)[1]) == ".h":
                            h=os.path.normpath(h)
                            self.add_header(h)

            for h in project.public_headers:
                self.add_header(h, [ "Public" ])

            for h in project.private_headers:
                self.add_header(h, [ "Private" ])

            libsearchpaths=[]

            if project.getTargetType() != "lib":
                for fw in project.sys_frameworks:
                    self.framework_phase.files.append(PBXBuildFile(
                        self.pb.add_framework(fw)))

                for lib in string.split(makefile.expand_variables("$(STATIC_LIBS)",tmp_vars)):
                    self.framework_phase.files.append(PBXBuildFile(
                        self.pb.add_library(lib)))

                    if os.path.exists(lib):
                        global ranlib_cache
                        abslib=os.path.normpath(os.path.join(os.getcwd(), lib))
                        while os.path.islink(abslib):
                            abslib=os.path.normpath(os.path.join(
                                os.path.dirname(abslib),
                                os.readlink(abslib)))
                        if not ranlib_cache.has_key(abslib):
                            ranlib_cache[abslib]=1
                            print 'Running ranlib "%s"' % abslib
                            status, output = shell.run('ranlib "%s"' % abslib)
                            print output
                            if status:
                                raise "RANLIB on '%s' failed with error=%d" % (lib, status)

                    path = os.path.dirname(lib)
                    if path not in libsearchpaths:
                        libsearchpaths.append(path)

            settings["LIBRARY_SEARCH_PATHS"]=string.join(libsearchpaths)

            ## Import/override project.preferences
            for key in project.preferences.keys():
                if type(project.preferences[key]) == types.StringType:
                    settings[key]=project.preferences[key]

            name = os.path.join(self.project.module_directory(),
                                self.project.makefile_name)
            name = umake_lib.declaw_name(name)

            ## Get the productSettingsXML from an (option) variable in
            ## in the project object.
            if project.getTargetType() != "lib":
                try:
                    xmltmp = project.productSettingsXML
                    xmltmp=string.replace(xmltmp,"_NAME_",project.target_name);
                    vfile=project.target_name
                    try:
                        vfile=project.version_file
                    except:
                        pass

                    version=('0','0','0','0')
                    try:
                        version = platform.versioning.get_version(vfile,"")
                    except err.error, e:
                        print "Umake warning: Failed to find version file. (%s)" % vfile

                    xmltmp=string.replace(xmltmp,"_VERSION_",string.join(version,"."))
                    xmltmp=string.replace(xmltmp,"_VER_", string.join(version[:-1],"."))
                    xmltmp=string.replace(xmltmp,"_BUILD_", str(version[-1]))
                    xmltmp=string.replace(xmltmp,"_ORIGINATOR_",project.bundle_originator);

                    ## PB-do, but doesn't seem to harm Xcode
                    target.productSettingsXML = xmltmp

                    ## Xcode-do, but shouldn't harm PB
                    defdir = self.project.output_dir
                    shell.mkdir(defdir)
                    n = os.path.join(self.project.module_directory(),
                                     self.project.makefile_name)
                    plist_file_name = umake_lib.declaw_name(n)+"_info.plist"
                    plist_file_name=os.path.join(defdir, plist_file_name)
                    umake_lib.write_file(plist_file_name, xmltmp)
                    settings["INFOPLIST_FILE"]=plist_file_name

                except KeyError:
                    pass

            ## Should be last!
            base=os.path.basename( project.getOutputPath() )
                                                                     
            if self.project.getTargetType() == "dll" and debug != "YES":
                ## Build an export function table, hope this
                ## Does everything we need

                expdir = self.project.output_dir
                shell.mkdir(expdir)
                expfile = os.path.join(self.project.module_directory(),
                                       self.project.makefile_name)
                expfile = umake_lib.declaw_name(expfile)+".exp"
                expfile=os.path.join(expdir, expfile)

                contents=string.join(self.project.exported_func,"\n_")
                if contents:
                    contents="_%s\n" % contents

                umake_lib.write_file(expfile,contents);

                fname=base[:-7] ## Remove ".bundle"
                strip_phase=PBXShellScriptBuildPhase(
                    'if [ -f "build/%s/Contents/MacOS/%s" ] ; then strip -u -s "%s" "build/%s/Contents/MacOS/%s"; fi ' % \
                    (base,fname, expfile,base,fname))

                target.buildPhases.append(strip_phase)

            script_phase=PBXShellScriptBuildPhase(
                'test -e "$RIBOSOME_OUTPUT_DIR/%s" && %s "$RIBOSOME_OUTPUT_DIR/%s" ; %s "build/%s" "$RIBOSOME_OUTPUT_DIR" ' %
                (base,
                 platform.rm.cmd,
                 base,
                 platform.copy.cmd,
                 base))

            target.buildPhases.append(script_phase)

            ## This creates a named symlink to the target
            odir=self.project.output_dir
            shell.mkdir(odir)
            bas,ext=os.path.splitext(base)
            linkname="%s/%s__%s%s" % (
                odir,
                bas,
                string.replace(project.module_directory(),"/","_"),
                ext)

            if not os.path.exists(linkname) and not os.path.islink(linkname):
                os.symlink(base, linkname)

            ## End of lib/exe/dll target code

        ## Extra shellscriptphases
        tmp=[]
        for cmd in project.pre_build_commands:
            tmp.append(PBXShellScriptBuildPhase(cmd))

        target.buildPhases=tmp + target.buildPhases

        for cmd in project.post_build_commands:
            target.buildPhases.append(PBXShellScriptBuildPhase(cmd))

        pb.quicksave(name)


    def add_source_file(self, file):
        ret=PBXBuildFile(self.pb.add_source_file(file))
        self.source_phase.files.append(ret)
        return ret

    def add_dep(self, pb_target):
        self.target.dependencies.append(pb_target.target)


## Generate a makefile which invokes
## pbxbuild/xcodebuild to run the project file generated.
class pb_makefile_generator(umake_makefile.makefile_generator):

    ## This only generates a call to pbxbuild
    def MainTarget(self):
        if self.project.getTargetType() == "":
            return

        output_path = self.project.getOutputPath()
        
        self.AllTarget( "build" )

        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)

        self.writeln(".PHONY:")
        self.writeln("")

        self.writeln("build: .PHONY")
        if objectVersion == 38:
            self.writeln('\tpbxbuild -target "%s"' % name)
        else:
            self.writeln('\txcodebuild -target "%s"' % name)

        self.sign( output_path )

        self.writeln('')
        
        self.CleanTarget()
        self.CopyTarget()
        self.DependTarget()

    ## Dummy dependtarget
    def DependTarget(self):
        if self.created_targets.has_key("depend"):
            return
        self.created_targets["depend"]=1
        
        self.writeln("depend:")
        self.writeln("")


    def CleanTarget(self, targets = ""):
        """Make clean"""
        if self.created_targets.has_key("clean"):
            return
        self.created_targets["clean"]=1

        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)

        self.writeln("clean:")
        if objectVersion == 38:
            self.writeln('\tpbxbuild -target "%s" clean' % name)
        else:
            self.writeln('\txcodebuild -target "%s" clean' % name)

        self.call_all_submake("clean", 1)
        self.writeln('')


## Generate a file which contains all the information required to build
## the uber project
class uber_generator(umake_lib.Targets):

    def __init__(self, platform, project):
        umake_lib.Targets.__init__(self, platform, project)

        uberdata = []
        for sumake in project.get_uber_makes():
            uberdata.append(  {
                "name":umake_lib.declaw_name(sumake.abs_makefile()),
                "abs_makefile":sumake.abs_makefile(),
                "makefile":sumake.makefile(),
                "dependencies":sumake.dependencies(),
                })
                

        name = os.path.join(self.project.module_directory(),
                            self.project.makefile_name)
        name = umake_lib.declaw_name(name)
        marshal.dump(uberdata, open(name+"-uberdata","w"))


## Merge projects together
## Each directory can only have one project file, so we must
## take all the projects in the current directory and merge them
## together into one file
## We also make sure to generate an uber project when possible
## Almost all of the heavy lifting is done by the projectbuilder class.
class update_project(umake_lib.Targets):
    
    def __init__(self, platform, project):
        umake_lib.Targets.__init__(self, platform, project)

        pb=projectbuilder()

        all=PBXAggregateTarget("All")
        pb.pbx.targets.append(all)

        uberdata=[]
        uberdata_done={}
        local_targets={}
        
        for p in os.listdir("."):
            if p[-6:] == "-pbtmp":
                path=p[:-6]
                at=pb.AddProject_nofix(path)
                key=os.path.normpath(os.path.join(os.getcwd(), path));
                local_targets[key]=at
        
            if p[-9:] == "-uberdata":
                for sumake in marshal.load(open(p)):
                    if not uberdata_done.has_key(sumake["name"]):
                        uberdata_done[sumake["name"]]=1
                        uberdata.append(sumake)
                        
                    
        targets={}
        for sumake in uberdata:
            name=umake_lib.declaw_name(sumake["abs_makefile"])
            path = os.path.dirname(sumake["makefile"])
            path = os.path.join(path, umake_lib.declaw_name(sumake["abs_makefile"]))
            key=os.path.normpath(os.path.join(os.getcwd(), path));
                
            if local_targets.has_key(key):
                at=pb.CopyTargets(local_targets[key], "_uber")
            else:
                at=pb.AddProject(path,"_uber")
                
            targets[name] = at
            for t in at:
                all.dependencies.append(PBXTargetDependency(t,pb.pbx))

        ## This could be a little more abstract...
        for sumake in uberdata:
            name=umake_lib.declaw_name(sumake["abs_makefile"])
            ax=targets[name]
            for dep in sumake["dependencies"]:
                dep=umake_lib.declaw_name(dep)
                if targets.has_key(dep):
                    ay=targets[dep]
                    for x in ax:
                        for y in ay:
                            x.dependencies.append(PBXTargetDependency(y,
                                                                      pb.pbx))
                else:
                    print "UMAKE WARNING: Unable to resolv dependency %s" % dep


        pb.save("Project")


## Entrypoint
def make_makefile(platform, project):
    
    #if os.path.islink("build"):  # Uncomment this later for more security
    #    shell.rm("build")
    #if not os.path.exists("build"):
    #    os.symlink(project.output_dir, "build")
    #
    #if not os.path.exists("..."):
    #    os.symlink(".", "...")
    #
    #if not os.path.exists("build/..."):
    #    os.symlink("..", "build/...")


    project_generator(platform, project)
    uber_generator(platform, project)
    update_project(platform, project)


## For testing
if __name__ == '__main__':
    import sys

    c=codec()
    #print c
    f=sys.stdin.read()
    d=c.unconvert(f)
    #print d
    print c.prettyprint(d)
    #print c.convert(d)
    
