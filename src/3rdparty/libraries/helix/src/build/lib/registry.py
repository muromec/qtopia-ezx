# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: registry.py,v 1.8 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""A simple data registry for storing build system data.  The storage file
is the Windows .ini file format."""

import os
import stat
import string
import copy
import time
import chaingang
import shell

## implement a Registry back-end as a simple windows ".ini" style
## text file
class INIStorage:
    def __init__(self, path):
        self.__path = path
        self.__mtime = 0
        self.__storage_evaluated = 0
        self.__last_is_current=0

    def is_current(self):
        t=time.time()
        if t - self.__last_is_current < 2.0:
            return 1
        
	try:
            st = os.stat(self.__path)
        except IOError:
            return 0

        if self.__mtime == st[stat.ST_MTIME]:
            self.__last_is_current=t
            return 1

        return 0

    def save(self, registry_hash):
        #print "INIStorage.save()"
        
        temp_path = "%s.temp" % (self.__path)
        try:
            os.remove(temp_path)
        except os.error:
            pass

        filehandle = shell.Open(temp_path, "w")

        ## write header
        filehandle.write("## Build System Registry: Do Not Edit or Delete\n\n")

        ## create a sorted list of the section keys
        section_key_list = registry_hash.keys()
        section_key_list.sort()

        filehandle.write("evaluated=1\n\n")
        self.__storage_evaluated = 1

        for section in section_key_list:
            filehandle.write("[%s]\n" % (section))
            
            attribute_hash = registry_hash[section]
            attribute_key_list = attribute_hash.keys()
            attribute_key_list.sort()
            for key in attribute_key_list:
                value = attribute_hash[key]
                filehandle.write("%s=%s\n" % (key, repr(value)))

            filehandle.write("\n")

        filehandle.write("## Build system entry end, changed entries may be appended\n")
        filehandle.close()

        ## now move the temporary registry to the origional registry
        try:
            os.remove(self.__path)
        except os.error:
            pass

        os.rename(temp_path, self.__path)
        
        self.__mtime = os.stat(self.__path)[stat.ST_MTIME]
        
    def load(self):
        if not os.path.isfile(self.__path):
            return {}
        global_section_hash = {}
        
        self.__mtime = os.stat(self.__path)[stat.ST_MTIME]
        filehandle = open(self.__path, "r")

        ## clear out old registry and setup state vars
        registry_hash = {}
        section = None
        section_hash = global_section_hash
        
        while 1:
            line = filehandle.readline()
            if not line:
                break

            line = string.strip(line)

            if not len(line):
                continue

            ## optimizateion: take the ordinal value of the first
            ## and last charactor in the line
            first = ord(line[0])
            last = ord(line[-1])

            ## checks for a "#" comment
            if first == 35:
                continue

            ## check for new section, first == "[" last == "]"
            if first == 91 and last == 93:
                section = line[1:-1]
                section_hash = registry_hash.get(section,{})
                registry_hash[section] = section_hash
                continue

            index = string.find(line, "=")
            if index == -1:
                if line == "clear":
                    for k in section_hash.keys():
                        del section_hash[k]
                elif line[:4] == "del ":
                    try:
                        del section_hash[line[4:]]
                    except KeyError:
                        pass
                continue

            key = string.strip(line[:index])
            value = string.strip(line[index + 1:])
            if global_section_hash.has_key("evaluated"):
                value = eval(value)
            section_hash[key] = value

        if global_section_hash.has_key("evaluated"):
            self.__storage_evaluated=1
            
        return registry_hash


    def append(self, section, key, value, reghash):
        if not self.__storage_evaluated:
            self.save(reghash)
            return
        filehandle = open(self.__path, "a")
        filehandle.write("[%s]\n" % (section))
        filehandle.write("%s=%s\n" % (key, repr(value)))
        filehandle.close()
        self.__mtime = os.stat(self.__path)[stat.ST_MTIME]
        self.__last_is_current=time.time()

    def append_cs(self, section, reghash):
        if not self.__storage_evaluated:
            self.save(reghash)
            return
        filehandle = open(self.__path, "a")
        filehandle.write("[%s]\n" % (section))
        filehandle.write("clear\n")
        filehandle.close()
        self.__mtime = os.stat(self.__path)[stat.ST_MTIME]
        self.__last_is_current=time.time()

    def append_cv(self, section, key, reghash):
        if not self.__storage_evaluated:
            self.save(reghash)
            return
        filehandle = open(self.__path, "a")
        filehandle.write("[%s]\n" % (section))
        filehandle.write("del %s\n" % key)
        filehandle.close()
        self.__mtime = os.stat(self.__path)[stat.ST_MTIME]
        self.__last_is_current=time.time()


class Registry:
    def __init__(self, storage):
        self.__storage = storage
        self.__section_hash = {}
        self.__sync_always = 1
        self.__changed = 0
        self.__load()
        self.__append_count = 0

    def __load(self):
        self.__section_hash = None
        self.__section_hash = self.__storage.load()
        self.__changed = 0
        self.__append_count = 0

    def __load_if_not_current(self):
        if not self.__storage.is_current():
            self.__load()

    def __save(self):
        if self.__section_hash != None and self.__changed:
            self.__storage.save(self.__section_hash)
        self.__changed = 0
        self.__append_count = 0

    def __sync(self):
        if self.__sync_always:
            self.__save()

    def sync_on(self):
        self.__sync_always = 1
        self.__save()

    def sync_off(self):
        self.__sync_always = 0

    def close(self):
        self.__save()
        self.__section_hash == None
    
    def clear(self):
        self.__section_hash = {}
        self.__changed = 1
        self.__sync()

    def clear_section(self, section):
        self.__load_if_not_current()
        try:
            del self.__section_hash[section]
        except KeyError:
            return
        self.__changed = 1

        if self.__sync_always:
            if  self.__append_count > 300:
                self.__sync()
            else:
                self.__append_count = self.__append_count + 1
                self.__storage.append_cs(section, self.__section_hash)
                self.__changed=0
                
    def clear_value(self, section, key):
        self.__load_if_not_current()
        try:
            del self.__section_hash[section][key]
        except KeyError:
            return
        self.__changed = 1

        if self.__sync_always:
            if  self.__append_count > 300:
                self.__sync()
            else:
                self.__append_count = self.__append_count + 1
                self.__storage.append_cv(section, key, self.__section_hash)
                self.__changed=0

    def section_list(self):
        self.__load_if_not_current()
        return self.__section_hash.keys()

    def section_key_list(self, section):
        self.__load_if_not_current()
        return self.__section_hash.get(section,{}).keys()

    def get_value(self, section, key):
        self.__load_if_not_current()
        return self.__section_hash[section][key]

    def get(self, section, key, default = None):
        self.__load_if_not_current()
        return copy.deepcopy(self.__section_hash.get(section,{}).get(key,default))

    def get_value_default(self, section, key, value):
        try:
            return_value = self.get_value(section, key)
        except KeyError:
            self.set_value(section, key, value)
            return_value = value
        return copy.deepcopy(return_value)

    def set_value(self, section, key, value):
        self.__load_if_not_current()

        if self.__section_hash.get(section,{}).get(key, None) == value:
            return

        value=copy.deepcopy(value)
        try:
            self.__section_hash[section][key] = value
        except:
            self.__section_hash[section] = {}
            self.__section_hash[section][key] = value

        self.__changed=1

        if self.__sync_always:
            if self.__append_count > 300:
                self.__sync()
            else:
                self.__append_count = self.__append_count + 1
                self.__storage.append(section, key, value, self.__section_hash)
                self.__changed=0

    def uptake(self):
        tmp=self.__section_hash
        self.__load()

        sync=self.__sync_always
        self.__sync_always=1
        
        for section in tmp.keys():
            for key in tmp[section].keys():
                self.set_value(section, key, tmp[section][key])

        self.__sync_always=sync
        self.__save()

## entrypoints
def OpenRegistryFile(path):
    ini = INIStorage(path)
    registry = Registry(ini)
    return registry


## testing
if __name__ == "__main__":
    try:
        os.remove("test.reg")
    except:
        pass
        
    reg = OpenRegistryFile("test.reg")
    reg.sync_off()

    for i in range(500):
        section = "section_%d" % (i%10)
        key = "jpaint%d" % (i)
        value = "bla bla bal %d" % (i)

        print "section=%s key=%s value=%s" % (section,key,value)
        reg.set_value(section, key, value)

    ## clear section 1
    reg.clear_section("section_1")

    print "sync on.."
    reg.sync_on()

    ## close registry
    reg.close()
