# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: archive.py,v 1.25 2006/07/06 19:28:05 jfinnecy Exp $ 
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
"""Multi-platform compressed (using zlib) archive format.  None of the
publicly available archive formats worked well between Mac, Windows, and
UNIX, so this was written to fill that gap for the purposes of the build
system.  It supports data and resource forks for the Macintosh.  It has a
internal MIME table for distinguishing ASCII files from binary files, as well
as a binary-checking algroithm it falls back on if the file extention is not
in the MIME table.  It stores line ending internally as UNIX "\n" endings,
but writes out the correct line endings for ASCII files when decompressing
on a given platform.  For Windows, "\r\n", and for Macintosh, "\r"."""

import os
import sys
import string
import getopt
import stat
import types

try:
    import zlib
except:
    pass

have_macfs=0
have_macos=0
try:
    import MacOS
    have_macos=1
    if not hasattr(MacOS,"GetCreatorAndType"):
        import macfs
        have_macfs=1
except:
    pass

try:
    os.utime
    have_os_utime=1
except:
    have_os_utime=0

try:
    os.chmod
    have_os_chmod=1
except:
    have_os_chmod=0

## these constants effect .rna file compatibility!!!
_rna_version = "1.1"
_num_length = 32
_mac_second_diff = 2082816000.0 
##

## these constants effect preformance/debugging
_debug = 0
_max_read = 8192
_zlib_level = 6
##


## extention table for mapping extiontions to automagic
## ASCII or binary file, and creator/type for the Macintosh

class FType:
    def __init__(self, file_t, creator, ftype):
        self.file_t = file_t
        self.creator = creator
        self.type = ftype

_ext_table = {
    "xml" : FType("A", "CWIE", "TEXT"),
    "bif" : FType("A", "CWIE", "TEXT"),
    "bat" : FType("A", "CWIE", "TEXT"),
    "s"   : FType("A", "CWIE", "TEXT"),
    "S"   : FType("A", "CWIE", "TEXT"),
    "exp" : FType("A", "CWIE", "TEXT"),
    "cpp" : FType("A", "CWIE", "TEXT"),
    "CPP" : FType("A", "CWIE", "TEXT"),
    "cp"  : FType("A", "CWIE", "TEXT"),
    "r"   : FType("A", "CWIE", "TEXT"),
    "c"   : FType("A", "CWIE", "TEXT"),
    "C"   : FType("A", "CWIE", "TEXT"),
    "h"   : FType("A", "CWIE", "TEXT"),
    "H"   : FType("A", "CWIE", "TEXT"),
    "py"  : FType("A", "CWIE", "TEXT"),
    "pcf" : FType("A", "CWIE", "TEXT"),
    "cf"  : FType("A", "CWIE", "TEXT"),
    "pm"  : FType("A", "CWIE", "TEXT"),
    "pl"  : FType("A", "CWIE", "TEXT"),
    "cfg" : FType("A", "CWIE", "TEXT"),
    "spc" : FType("A", "CWIE", "TEXT"),
    "txt" : FType("A", "CWIE", "TEXT"),
    "cc"  : FType("A", "CWIE", "TEXT"),
    "asm" : FType("A", "CWIE", "TEXT"),
    "cp"  : FType("A", "CWIE", "TEXT"),
    "htm" : FType("A", "CWIE", "TEXT"),
    "html": FType("A", "CWIE", "TEXT"),
    "ver" : FType("A", "CWIE", "TEXT"),
    "ini" : FType("A", "CWIE", "TEXT"),

    "res" : FType("B", "????", "BINA"),
    "lib" : FType("B", "CWIE", "MPLF"),
    "o"   : FType("B", "????", "BINA"),
    "obj" : FType("B", "????", "BINA"),
    "OBJ" : FType("B", "????", "BINA"),
    "exe" : FType("B", "????", "BINA"),
    "dll" : FType("B", "????", "shlb"),
    "DLL" : FType("B", "????", "shlb"),
    "a"   : FType("B", "????", "BINA"),
    "bmp" : FType("B", "????", "BINA"),
    "bmp" : FType("B", "????", "BINA"),
    "class":FType("B", "????", "BINA"),
    "rna" : FType("B", "????", "BINA"),
    "dat" : FType("B", "????", "BINA"),
    "jpg" : FType("B", "????", "BINA"),
    "JPG" : FType("B", "????", "BINA"),
    "gif" : FType("B", "????", "BINA"),
    "GIF" : FType("B", "????", "BINA"),
    "png" : FType("B", "????", "BINA"),
    "PNG" : FType("B", "????", "BINA"),
    "rm"  : FType("B", "????", "BINA"),
    "mp3" : FType("B", "????", "BINA"),
    "zip" : FType("B", "????", "BINA"),
    "jar" : FType("B", "????", "BINA"),
    "hqx" : FType("B", "????", "BINA"),
    "sit" : FType("B", "????", "BINA"),
    "pdf" : FType("B", "????", "BINA"),
    "PDF" : FType("B", "????", "BINA"),
    }

## file table, like above but for complete file names
_file_table = {
    "Umakefil"    : FType("A", "CWIE", "TEXT"),
    "umakefil"    : FType("A", "CWIE", "TEXT"),
    "Tag"         : FType("A", "CWIE", "TEXT"),
    "Entries"     : FType("A", "CWIE", "TEXT"),
    "Repository"  : FType("A", "CWIE", "TEXT"),
    "Root"        : FType("A", "CWIE", "TEXT"),
    }
    
## default creator/types for misc binary and ASCII files
_binary_default = ("????", "BINA")
_ascii_default  = ("CWIE", "TEXT")


## entrypoints

def Archive(archive_path, path_list):
    RNA_Archive(archive_path, path_list)

def Index(archive_path):
    RNA_Index(archive_path)

def Extract(archive_path):
    RNA_Extract(archive_path)

def ExtractDir(archive_path, dir):
    def filter_func(x, dir = dir):
        return x[:len(dir)] == dir
    RNA_Extract(archive_path, filter_func)

## private
if have_macos and hasattr(MacOS,"GetCreatorAndType"):

    def getcreatorandtype(path):
        return MacOS.GetCreatorAndType(path)

    def setcreatorandtype(path, creator, type):
        return MacOS.SetCreatorAndType(path, creator, type)

elif have_macfs:
    def getcreatorandtype(path):
        try:
            fsp = macfs.FSSpec(mac_path(path))
            (rec.creator, rec.type) = fsp.GetCreatorType()
        except MacOS.Error:
            pass

    def setcreatorandtype(path, creator, type):
        try:
            fsp = macfs.FSSpec(mac_path(filename))
            fsp.SetCreatorType(creator, type)
        except MacOS.Error:
            pass

else:
    def getcreatorandtype(path):
        return None

    def setcreatorandtype(path, creator, type):
        pass

##    

## fake file class which is a infinite sink
class FSink:
    def __init__(self):
        self.__tell = 0
    
    def write(self, data):
        self.__tell = self.__tell + len(data)

    def tell(self):
        return self.__tell

    def close(self):
        self.__tell = 0

def RNA_Archive_stream(arch, path_list):
    archive_file_list, archive_directory_list, delete_list = get_target_list(path_list)


    ## separate record lists of all files and directories
    record_list = []
    for path in archive_file_list:
        rec=read_file(arch, path)
        record_list.append(rec)
        rec.debug()

    for path in archive_directory_list:
        rec=read_directory_info(path)
        record_list.append(rec)
        rec.debug()

    for path in delete_list:
        rec=delete_record(path)
        record_list.append(rec)
        rec.debug()

    ## write file index table
    index_location = arch.tell()
    for rec in record_list:
        rec.write(arch)

    ## write the beginning of the index location at the end of the file
    arch.write(string.zfill(str(index_location), _num_length))


def RNA_Archive(archive_path, path_list):
    ## open the archive file for writing

    arch = open(archive_path, "wb")
    RNA_Archive_stream(arch, path_list)
    arch.close()

    ## set the file type of the archive to binary on Macintosh
    if have_macfs:
        try:
            fsp = macfs.FSSpec(mac_path(archive_path))
            fsp.SetCreatorType("????", "BINA")
        except MacOS.Error:
            pass

def RNA_Index(archive_path):
    if not os.path.isfile(archive_path):
        print "archive.py: file not found %s" % (archive_path)
        return

    ## debugging output is used to show the index
    global _debug
    _debug = 1

    arch = open(archive_path, "rb")

    ## find the location of the index in the file
    index_begin, index_end = index_begin_end(arch)

    ## seek to the beginning of the index and print
    arch.seek(index_begin)

    while arch.tell() < index_end:
        rec = Record()
        rec.read(arch)
        rec.debug()

    arch.close()


def RNA_Extract(archive_path, filter_func = None, posix_filter = None):
    if not os.path.isfile(archive_path):
        print "archive.py: file not found %s" % (archive_path)
        return

    arch = open(archive_path, "rb")
    archidx = open(archive_path, "rb")

    ## find the location of the index in the file
    index_begin, index_end = index_begin_end(archidx)

    ## seek to the beginning of the index
    archidx.seek(index_begin)

    directory_record_list = []
    while archidx.tell() < index_end:
        rec = Record()
        rec.read(archidx)
        rec.debug()

        ## filter out according to the filter function
        if filter_func:
            if posix_filter:
                yn=filter_func(rec.posixpath)
            else:
                yn=filter_func(rec.path)

            if not yn:
                arch.seek(rec.data_len + rec.resc_len, 1)
                continue

        print "%s %s" % (rec.file_t, rec.path)
        
        if rec.file_t == "R":
            delete_file(rec)
        elif rec.file_t == "D":
            directory_record_list.append(rec)
        else:
            write_file(arch, rec)

    ## write directory information in decending directory order
    record_hash = {}
    for rec in directory_record_list:
        record_hash[rec.path] = rec

    key_list = record_hash.keys()
    key_list.sort()
    key_list.reverse()

    for key in key_list:
        write_directory_info(record_hash[key])
        
    
class Record:
    def debug(self):
        if not _debug:
            return

        print "path = %s" % (self.path)
        print "        file_t = %s   creator = %s  type = %s " % (self.file_t, self.creator, self.type)
        print "        data_len = %d  resc_len = %d" % (self.data_len, self.resc_len)
        print "        mode = %d  atime = %d  mtime = %d" % (self.mode, self.atime, self.mtime)

    
    def read(self, fil):
        ## first part of the record is the length of the path, the rest of
        ## the record is fixed-length

        try:
            path_len = int(fil.read(_num_length))
        except ValueError:
            raise error, "archive corrupt, invalid path_len in record"

        rec_data_len = path_len + 9 + (5 * _num_length)
        rec_data = fil.read(rec_data_len)

        ## grab the path, and then chop it off the rest of
        ## the buffer
        self.posixpath=rec_data[:path_len]
        self.path = native_path(self.posixpath)
        index = path_len

        ## read file type: binary=B, ascii=A, directory=D
        self.file_t = rec_data[index]
        index = index + 1

        ## file createor
        index2 = index + 4
        self.creator = rec_data[index:index2]
        index = index2

        ## file type
        index2 = index + 4
        self.type = rec_data[index:index2]
        index = index2

        ## read data fork length
        index2 = index + _num_length
        try:
            self.data_len = int(rec_data[index:index2])
        except ValueError:
            raise error, "archive corrupt, invalid data_len in record"

        index = index2

        ## read resource fork length
        index2 = index + _num_length
        try:
            self.resc_len  = int(rec_data[index:index2])
        except ValueError:
            raise error, "archive corrupt, invalid resc_len in record"
        index = index2

        ## read mode bits
        index2 = index + _num_length
        try:
            self.mode = int(rec_data[index:index2])
        except ValueError:
            raise error, "archive corrupt, invalid mode in record"
        index = index2

        ## read atime
        index2 = index + _num_length
        try:
            self.atime = int(rec_data[index:index2])
        except ValueError:
            raise error, "archive corrupt, invalid atime in record"
        index = index2
        
        ## read mtime
        index2 = index + _num_length
        try:
            self.mtime = int(rec_data[index:index2])
        except ValueError:
            raise error, "archive corrupt, invalid mtime in record"

    def write(self, fil):
        ## store paths as relative file paths
        path = self.path
        while path[:1] == os.sep:
            path = path[1:]

        ## internal format for paths is the UNIX format
        path = self.__posix_path(path)

        ## file creator/type much be 4 charactors
        creator = self.creator
        if len(creator) != 4:
            creator = "    "

        ftype = self.type
        if len(ftype) != 4:
            ftype = "    "

        rec_data = string.zfill(len(path), _num_length) +\
                   path + self.file_t + creator + ftype +\
                   string.zfill(self.data_len, _num_length) +\
                   string.zfill(self.resc_len, _num_length) +\
                   string.zfill(self.mode, _num_length) +\
                   string.zfill(self.atime, _num_length) +\
                   string.zfill(self.mtime, _num_length)
        
        fil.write(rec_data)

    def __posix_path(self, path):
        return string.translate(path, string.maketrans(os.sep, "/"))

def native_path(path, sep=os.sep):
    if not path or path == "":
        return ""

    path = string.translate(path, string.maketrans("/", sep))

    ## Mac paths
    if sep == ':':
        mac_path = ""
        last_backdir = 0
        last_curdir = 0

        ## no directory info defaults to current dir
        if not path[0] in ".:":
            mac_path = ":" 

        i = 0
        while i < len(path):
            ## translate current directory
            if path[i:i+2] == ".:":
                if not last_curdir and not last_backdir:
                    mac_path = mac_path + ":"

                last_curdir = 1
                i = i + 2
                continue

            ## translate stepping back a directory
            if path[i:i+3] == "..:":
                if last_backdir or last_curdir:
                    mac_path = mac_path + ":"
                else:
                    mac_path = mac_path + "::"
                    last_backdir = 1
                i = i + 3
                continue

            ## append to mac_path
            mac_path = mac_path + path[i]
            i = i + 1
            last_curdir = 0
            last_backdir = 0

        path = mac_path

    return path

def mac_path(path):
    ## Convert native path to mac path
    if os.sep == ":":
        return path
    #print "mac_path(%s) => %s" % (repr(path), native_path(path, ":"))
    return native_path(path, ":")

def index_begin_end(fil):
    fil.seek(-_num_length, 2)
    try:
        begin = int(fil.read(_num_length))
    except ValueError:
        raise error, "archive corrupt, cannot read index location"
    end = fil.tell() - _num_length
    
    return begin, end


def get_target_list(path_list):
    origional_directory_list = []
    directory_list = []
    file_list = []
    delete_list = []

    # get all directories in the path list
    for path in path_list:
        if os.path.isfile(path) or os.path.islink(path):
                file_list.append(path)

        elif os.path.isdir(path):
            origional_directory_list.append(path)
            directory_list.extend(get_directories(path))
            continue

        elif not os.path.exists(path):
            delete_list.append(path)
            

    # get all files in given paths 
    for directory in directory_list:
        try:
            path_list = os.listdir(directory)
        except:
            continue
        
        for path in path_list:
            path = os.path.join(directory, path)
            if os.path.isfile(path) or os.path.islink(path):
                file_list.append(path)

    # decompose origional paths so the archive system
    # can make directory records for them that will set the
    # correct access/modification dates and permissions
    for directory in origional_directory_list:
        while 1:
            base, junk = os.path.split(directory)
            if base == "" or \
               base == os.curdir or \
               base == os.pardir or \
               base == os.sep:
                break

            directory_list.append(base)
            directory = base

    return file_list, directory_list, delete_list


def get_directories(directory):
    directory_list = [directory]

    try:
        path_list = os.listdir(directory)
    except:
        return directory_list

    for path in path_list:
        path = os.path.join(directory, path)
        
        if os.path.isdir(path) and not os.path.islink(path):
            directory_list = directory_list + get_directories(path)

    return directory_list


def file_type(path):
    if os.path.islink(path):
        return "L"
    
    basename = os.path.basename(path)

    try:
        return _file_table[basename].file_t
    except KeyError:
        pass

    ## try to determine file type by extention
    (base, ext) = os.path.splitext(basename)
    ext = ext[1:]
    if len(ext):
        try:
            return _ext_table[ext].file_t
        except KeyError:
            pass
        
    ## the extention/file is not in the table
    ## so use algrithim to try to figure it out
    return check_file_type(path)


def lookup_creator_type(path, file_t):
    basename = os.path.basename(path)

    try:
        return (_file_table[basename].creator, _file_table[basename].type)
    except KeyError:
        pass

    ## try to determine file type by extention
    (base, ext) = os.path.splitext(basename)
    ext = ext[1:]
    if len(ext):
        try:
            return (_ext_table[ext].creator, _ext_table[ext].type)
        except KeyError:
            pass
   
    ## the extention/file is not in the table so use defaults
    if file_t == "A":
        return _ascii_default
    else:
        return _binary_default


# "A" = ascii
# "B" = binary
# "L" = link
def check_file_type(path):

    ## Check what CVS thinks..
    entries=os.path.join(os.path.dirname(path),"CVS","Entries")
    if os.path.exists(entries):
        base=os.path.basename(path)
        for l in open(entries,"r").readlines():
            fields=string.split(l,"/")
            if len(fields) < 5:
                continue
            if fields[1] == base:
                if fields[4] == "-kb":
                    return "B"
                if fields[4] == "":
                    return "A"
                break

    ## Heuristics, does the file contain funny characters??
    non_text = 0

#    print path
    
    fil = open(path, "rb")
    buff = fil.read(512)
    fil.close()

    ten_percent = len(buff) / 10 + 1

    for i in range(len(buff)):
        a = ord(buff[i])
        if (a < 8) or (a > 13 and a < 32) or (a > 126):
            non_text = non_text + 1

    if non_text >= ten_percent:
        return "B"

    return "A"


def zread(out_fil, in_fil):
    zdata_len = 0
    zobj = zlib.compressobj(_zlib_level)

    data = in_fil.read(_max_read)
    while data:
        zdata = zobj.compress(data)
        zdata_len = zdata_len + len(zdata)
        out_fil.write(zdata)

        data = in_fil.read(_max_read)
        
    zdata = zobj.flush()
    zdata_len = zdata_len + len(zdata)
    out_fil.write(zdata)

    return zdata_len


def zwrite(in_fil, zlen, out_fil):
    bytes_to_read = zlen
    zobj = zlib.decompressobj()
    
    while bytes_to_read > 0:
        zbuff = in_fil.read(min(_max_read, bytes_to_read))
        bytes_to_read = bytes_to_read - len(zbuff)
        
        buff = zobj.decompress(zbuff)
        out_fil.write(buff)
        
    out_fil.write(zobj.flush())

def openrf(path, mode):

    if have_macos:
        try:
            return MacOS.openrf(mac_path(path), "*"+mode)
        except MacOS.Error:
            pass

    try:
        return open(os.path.join(path,"..namedfork","rsrc"),mode)
    except IOError:
        pass

    return None


def read_file(arch, path):
    rec = Record()
    rec.path = path
    rec.file_t = file_type(rec.path)
    rec.data_len = 0
    rec.resc_len = 0

    rec.creator = "    "
    rec.type = "    "
    rec.mode = 0
    rec.atime = 0
    rec.mtime = 0
    
    (rec.creator, rec.type) = lookup_creator_type(rec.path, rec.file_t)

    if rec.file_t == "L":
        str=os.readlink(rec.path)
        arch.write(str)
        rec.data_len=len(str)
    else:
        if rec.file_t == "B":
            fil = open(rec.path, "rb")
        else:
            fil = open(rec.path, "r")

        print "%s %s" % (rec.file_t, rec.path)

        ## compress-read the file, writing it to the
        ## archive and returning the compressed length
        rec.data_len = zread(arch, fil)
        fil.close()

        ## MACINTOSH SPECIFIC
        try:
            (rec.creator, rec.type) = getcreatorandtype(rec.path)
        except TypeError:
            pass

        fork_fil = openrf(rec.path, "rb");
        if fork_fil:
            ## compress read the data fork, returning
            ## the compressed length
            rec.resc_len = zread(arch, fork_fil)
            fork_fil.close()

        ## stat the file
        st = os.stat(rec.path)

        ## get file mode, atime, and mtime
        rec.mode = st[stat.ST_MODE]
        rec.atime = st[stat.ST_ATIME]
        rec.mtime = st[stat.ST_MTIME]

        ## Silly macs!
        if type(rec.atime) == types.FloatType:
            rec.atime = int(rec.atime - _mac_second_diff)
            rec.mtime = int(rec.mtime - _mac_second_diff)

    return rec

def recursive_rm(path):
    if os.path.isdir(path) and not os.path.islink(path):
        for p in os.listdir(path):
            recursive_rm(os.path.join(path,p))
    os.unlink(path)

def delete_file(rec):
    path = rec.path
    if os.path.exists(path):
        recursive_rm(path)


## truncate filename to 31 chars on the Macintosh
def mac_name_mangler(fname):
    if len(fname) <= 31:
        return fname
    base, ext = os.path.splitext(fname)
    base=string.replace(base,"-","")
    base=string.replace(base,"_","")
    fname=base+ext
    if len(fname) <= 31:
        return fname
    base=string.replace(base,"e","")
    base=string.replace(base,"i","")
    base=string.replace(base,"a","")
    base=string.replace(base,"o","")
    base=string.replace(base,"u","")
    fname=base+ext
    if len(fname) <= 31:
        return fname

    ## Crop
    return base[:31 - len(ext)] + ext


def write_file(arch, rec):
    ## make sure the directory specified exists,
    ## if it doesn't then create it properly
    #print "x %s" % rec.path
    make_dirs = []
    filename=rec.path
    (path, basename) = os.path.split(filename)

    ## Mac paths
    if os.sep == ":":
        filename=os.path.join(path, mac_name_mangler(basename))

    ## create directory path
    while path != "" and not os.path.exists(path):
        make_dirs.insert(0, path)
        (path, basename) = os.path.split(path)

    for path in make_dirs:
        os.mkdir(path)

    ## check if file exists
    if os.path.exists(filename):
        if not os.path.isfile(filename) and not os.path.islink(filename):
            return 0
        else:
            try:
                os.unlink(filename)
            except os.error:
                print "ERROR: could not remove %s" % (filename)

    ## write the file -- attempt to find another filename if
    ## it cannot be opened
    try:

        if rec.file_t == "L":
            to=arch.read(rec.data_len)
            try:
                os.unlink(filename)
            except OSError:
                pass
            os.symlink(to, filename)
            return 1
        
        if rec.file_t == "B":
            fil = open(filename, "wb")
        else:
            fil = open(filename, "w")
    except IOError:
        print "ERROR: could not write to %s" % (filename)
        fil = FSink()
        zwrite(arch, rec.data_len, fil)
        return 1

    ## read compressed file
    zwrite(arch, rec.data_len, fil)
    fil.close()

    ## resource fork
    if rec.resc_len:
        fil = openrf(filename, "wb")
        if not fil:
            fil = FSink()
        zwrite(arch, rec.resc_len, fil)
        fil.close()

    ## MACINTOSH SPECIFIC
    setcreatorandtype(filename, rec.creator, rec.type)
    if have_macfs:
        try:
            fsp = macfs.FSSpec(mac_path(filename))
            mtime = float(rec.mtime + _mac_second_diff)
            fsp.SetDates(mtime, mtime, mtime)
        except MacOS.Error:
            pass

    if have_os_utime:
        try:
            os.utime(filename, (rec.atime, rec.mtime))
        except OSError:
            pass

    ## set file mode, atime, mtime
    if have_os_chmod:
        os.chmod(filename, rec.mode)

    return 1


def read_directory_info(path):
    rec = Record()
    rec.path = path
    rec.file_t = "D"
    rec.creator = "    "
    rec.type = "    "
    rec.data_len = 0
    rec.resc_len = 0
    
    ## stat the directory
    st = os.stat(path)

    ## get directory mode, atime, and mtime
    rec.mode = st[stat.ST_MODE]
    rec.atime = st[stat.ST_ATIME]
    rec.mtime = st[stat.ST_MTIME]

    ## Silly macs!
    if type(rec.atime) == types.FloatType:
        rec.atime = int(rec.atime - _mac_second_diff)
        rec.mtime = int(rec.mtime - _mac_second_diff)

    return rec

def delete_record(path):
    rec = Record()
    rec.path = path
    rec.file_t = "R"
    rec.creator = "    "
    rec.type = "    "
    rec.data_len = 0
    rec.resc_len = 0
    rec.mode = 0
    rec.atime = 0
    rec.mtime = 0
    return rec


def write_directory_info(rec):
    ## create directory path
    make_dirs = []
    path = rec.path
    while path != "" and not os.path.exists(path):
        make_dirs.insert(0, path)
        (path, basename) = os.path.split(path)

    for path in make_dirs:
        try:
            os.mkdir(path)
        except:
            print "ERROR: cannot make directory %s" % (directory)
            return 0

    ## set mtime on Macintosh Folders
    if have_macfs:
        mtime = float(rec.mtime + _mac_second_diff)
        try:
            fsp = macfs.FSSpec(mac_path(rec.path))
            fsp.SetDates(mtime, mtime, mtime)
        except MacOS.Error:
            pass
        
    ## set directory mode/mtime/atime on posix/UNIX
    if have_os_utime:
        try:
            os.utime(rec.path, (rec.atime, rec.mtime))
        except OSError:
            pass
        try:
            os.chmod(rec.path, rec.mode)
        except OSError:
            pass

    return 1



## FOR INTERACTIVE USAGE
DEFAULT_ARCHIVE_NAME = "archive.rna"


def usage(pname):
    pname = os.path.basename(pname)
    
    print "%s: invalid usage" % (pname)
    print "python %s -c archfile path1 path2 ...   create archive" % (pname)
    print "python %s -i archfile                   print index" % (pname)
    print "python %s -e archfile                   extract archive" % (pname)
    sys.exit(1)


def mac_main():
    if len(sys.argv) < 2:
        usage(sys.argv[0])

    file_list = sys.argv[1:]

    # if only one file with .rna extentio, then extract
    if len(file_list) == 1:
        (path, basename) = os.path.split(file_list[0])

        # try to determine file type by extention
        i = string.rfind(basename, ".")
        if i != -1:
            if basename[i+1:] == "rna":
                os.chdir(path)
                Extract(basename)
                sys.exit(0)

    arch_file_list = []

    # else, create a archive with the default name
    for file in file_list:
        (path, basename) = os.path.split(file)
        arch_file_list.append(basename)

    os.chdir(path)
    Archive(DEFAULT_ARCHIVE_NAME, arch_file_list)


def command_line_main():
    program_name = sys.argv[0]
    try:
        (optlist, arglist) = getopt.getopt(sys.argv[1:], "c:e:i:")
    except:
        usage(program_name)

    if len(optlist) != 1:
        usage(program_name)

    ## create archive
    if optlist[0][0] == "-c":
        if len(arglist) < 1:
            usage(program_name)

        Archive(optlist[0][1], arglist)

    ## print index of archive
    elif optlist[0][0] == "-i":
        Index(optlist[0][1])

    ## extract archive
    elif  optlist[0][0] == "-e":
        ## if there's an argument in the arglist, then use that as
        ## the filter dir
        if not len(arglist):
            Extract(optlist[0][1])
        else:
            ExtractDir(optlist[0][1], arglist[0])


def main():
    if not have_os_utime:
        mac_main()
    else:
        command_line_main()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print "**BREAK**"
        sys.exit(1)

