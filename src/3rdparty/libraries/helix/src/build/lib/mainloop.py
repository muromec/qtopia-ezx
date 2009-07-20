# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: mainloop.py,v 1.3 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Implements a async, thread-safe scheduling engine.  This file is used in
build/lib, and in mserver/.  It should be kept syncronized to the same
version."""

import os
import string
import socket
import errno
import select
import time
import thread


## callback classes
class Callback:
    def __init__(self, callback, data):
        self.enabled_lock = thread.allocate_lock()
        self.enabled = 1
        
        self.recursion_count = 0
        self.callback = callback
        self.data = data


    def enable(self):
        self.enabled_lock.acquire(1)
        self.enabled = 1
        self.enabled_lock.release()


    def disable(self):
        self.enabled_lock.acquire(1)
        self.enabled = 0
        self.enabled_lock.release()


    def dispatch(self):
        self.enabled_lock.acquire(1)
        enabled = self.enabled
        self.enabled_lock.release()

        if not enabled:
            return
        
        self.recursion_count = self.recursion_count + 1
        self.execute()
        self.recursion_count = self.recursion_count - 1


    def execute(self):
        if self.data:
            self.callback(self.data)
        else:
            self.callback()


class TimeoutCallback(Callback):
    def __init__(self, seconds, callback, data):
        Callback.__init__(self, callback, data)
        self.expiration = time.time() + float(seconds)


    def is_expired(self, time):
        return time > self.expiration 


    def expires(self, time):
        return self.expiration - time


class IOCallback(Callback):
    def __init__(self, sock, callback, data):
        Callback.__init__(self, callback, data)
        self.sock = sock


    def execute(self):
        if self.data:
            self.callback(self.sock, self.data)
        else:
            self.callback(self.sock)


class MainLoop:
    error = 'MainLoop.error'
    
    def __init__(self):
        self.done_flag = 0

        ## list of timeout callbacks
        self.timeout_list = []

        ## list of idle callbacks
        self.idle_list = []
        
        ## sockets for select() reading/writing
        self.read_hash = {}
        self.read_list = []

        self.write_hash = {}
        self.write_list = []


    def main(self):
        ## go until done
        while not self.done_flag:
            self.main_loop_iteration()

        ## give re-birth to the network engine, thereby de-referencing
        ## and freeing everything that the current network engine references
        delete_engine()


    def done(self):
        self.done_flag = 1


    def add_timeout_cb(self, seconds, callback, data):
        ## add the timeout
        timeout = TimeoutCallback(seconds, callback, data)

        ## add to the timeout_list
        index = 0
        for index in range(len(self.timeout_list)):
            if self.timeout_list[index].expiration > timeout.expiration:
                break

        self.timeout_list.insert(index, timeout)
        return id(timeout)


    def remove_timeout_cb(self, id):
        for timeout in self.timeout_list:
            if id(timeout) == id:
                self.timeout_list.remove(timeout)
                break


    def add_read_cb(self, sock, callback, data):
        try:
            sock.fileno()
        except AttributeError:
            raise MainLoop.error, 'bad type %s' % (str(type(sock)))
        
        if not self.read_hash.has_key(sock):
            self.read_list.append(sock)
        self.read_hash[sock] = IOCallback(sock, callback, data)


    def remove_read_cb(self, sock):
        try:
            self.read_hash[sock].disable()
            del self.read_hash[sock]
        except KeyError:
            pass
        else:
            self.read_list.remove(sock)


    def add_write_cb(self, sock, callback, data):
        try:
            sock.fileno()
        except AttributeError:
            raise MainLoop.error, 'bad type %s' % (str(type(sock)))
        
        if not self.write_hash.has_key(sock):
            self.write_list.append(sock)
        self.write_hash[sock] = IOCallback(sock, callback, data)


    def remove_write_cb(self, sock):
        try:
            self.write_hash[sock].disable()
            del self.write_hash[sock]
        except KeyError:
            pass
        else:
            self.write_list.remove(sock)
        

    def calculate_next_timeout(self):
        ## no timeouts
        if not len(self.timeout_list):
            return None

        ## compute timeout from soonest timeout callback
        seconds = self.timeout_list[0].expires(time.time())
        
        if seconds < 0:
            seconds = 0

        return seconds


    def select(self, seconds):
        ## only run select() if there are valid sockets
        if len(self.read_list) or len(self.write_list):
            return select.select(self.read_list, self.write_list, [], seconds)

        if seconds:
            time.sleep(seconds)

        return [], [], []


    def main_loop_iteration(self):
        callback_list = self.main_loop_poll()
        self.main_loop_dispatch(callback_list)


    def main_loop_poll(self):
        callback_list = []

        seconds = self.calculate_next_timeout()

        ## make the select() call; catch a lot of potential errors
        ## and prune the select lists if needed
        try:
            read_list, write_list, exception_list = self.select(seconds)
        
        except select.error, e:
            sys.stderr.write('select.error: %s' % (str(e)))
            return self.find_bad_sockets()
        
        except ValueError:
            sys.stderr.write('ValueError')
            return self.find_bad_sockets()
            
        except TypeError:
            sys.stderr.write('TypeError')
            return self.find_bad_sockets()

        else:
            for sock in read_list:
                try:
                    callback_list.append(self.read_hash[sock])
                except KeyError:
                    pass

            for sock in write_list:
                try:
                    callback_list.append(self.write_hash[sock])
                except KeyError:
                    pass


        ## add in expired timeouts
        callback_list = callback_list + self.get_expired_timeouts()
        
        return callback_list


    def main_loop_dispatch(self, callback_list):
        for callback in callback_list:
            callback.dispatch()


    def find_bad_sockets(self):
        callback_list = []
        
        ## go through the list of all sockets in the networking engine
        ## and preform a getsockname() on them; if this throws an
        ## exception, then the socket is invalid and should be removed

        for sock in self.read_list[:]:
            try:
                sock.getsockname()
                
            except socket.error:
                try:
                    callback_list.append(self.read_hash[sock])
                except KeyError:
                    continue
                
            except AttributeError:
                continue

        for sock in self.write_list[:]:
            try:
                sock.getsockname()
                
            except socket.error:
                try:
                    callback_list.append(self.write_hash[sock])
                except KeyError:
                    continue
                
            except AttributeError:
                continue
    
        return callback_list


    def get_expired_timeouts(self):
        ## if there are no timeouts
        if not len(self.timeout_list):
            return []

        current_time = time.time()
        
        ## create a new timeout list of active callbacks
        index = 0
        for index in range(len(self.timeout_list)):
            if not self.timeout_list[index].is_expired(current_time):
                index = index - 1
                break

        ## off-by-one crap!
        if index < 0:
            return []

        ## slice off the timed-out callbacks
        timeout_list = self.timeout_list[:index+1]
        self.timeout_list = self.timeout_list[index+1:]

        return timeout_list



class ThreadedMainLoop(MainLoop):
    def __init__(self):
        MainLoop.__init__(self)

        ## mutex/locks for internal data
        self.timeout_lock = thread.allocate_lock()
        self.idle_lock = thread.allocate_lock()
        self.read_lock = thread.allocate_lock()
        self.write_lock = thread.allocate_lock()

        ## pipe for waking up a select()ing thread
        self.wakeup_needed = 0
        self.wakeup_needed_lock = thread.allocate_lock()
        self.init_wakeup()
        

    def init_wakeup(self):
        rfd, wfd = os.pipe()
        self.thread_wakeup_read = os.fdopen(rfd, 'rb', 0)
        self.thread_wakeup_write = os.fdopen(wfd, 'wb', 0)
        self.add_read_cb(self.thread_wakeup_read, self.wakeup_cb, None)


    def wakeup_cb(self, thread_wakeup_read):
        self.wakeup_needed_lock.acquire(1)
        self.thread_wakeup_read.read(1)
        self.wakeup_needed_lock.release()
        

    def wakeup(self):
        self.wakeup_needed_lock.acquire(1)
        
        if self.wakeup_needed:
            self.thread_wakeup_write.write('Z')
            self.wakeup_needed = 0

        self.wakeup_needed_lock.release()


    def main(self):
        MainLoop.main(self)

        ## now remove the wakeup sockets so the mainloop will
        ## be de-referenced and be garbage collected
        self.remove_read_cb(self.thread_wakeup_read)


    def done(self):
        MainLoop.done(self)
        self.wakeup()
        

    def add_timeout_cb(self, seconds, callback, data):
        self.timeout_lock.acquire(1)
        MainLoop.add_timeout_cb(self, seconds, callback, data)
        self.timeout_lock.release()
        self.wakeup()

        
    def remove_timeout_cb(self, id):
        self.timeout_lock.acquire(1)
        MainLoop.remove_timeout_cb(self, id)
        self.timeout_lock.release()

        
    def add_read_cb(self, sock, callback, data):
        self.read_lock.acquire(1)
        MainLoop.add_read_cb(self, sock, callback, data)
        self.read_lock.release()
        self.wakeup()


    def remove_read_cb(self, sock):
        self.read_lock.acquire(1)
        MainLoop.remove_read_cb(self, sock)
        self.read_lock.release()
        

    def add_write_cb(self, sock, callback, data):
        self.write_lock.acquire(1)
        MainLoop.add_write_cb(self, sock, callback, data)
        self.write_lock.release()
        self.wakeup()
        

    def remove_write_cb(self, sock):
        self.write_lock.acquire(1)
        MainLoop.remove_write_cb(self, sock)
        self.write_lock.release()


    def calculate_next_timeout(self):
        self.timeout_lock.acquire(1)
        seconds = MainLoop.calculate_next_timeout(self)
        self.timeout_lock.release()

        return seconds


    def select(self, seconds):
        self.read_lock.acquire(1)
        read_list = self.read_list[:]
        self.read_lock.release()

        self.write_lock.acquire(1)
        write_list = self.write_list[:]
        self.write_lock.release()

        return select.select(read_list, write_list, [], seconds)


    def main_loop_iteration(self):
        ## at this point, a thread wakeup will most likely be needed
        self.wakeup_needed_lock.acquire(1)
        self.wakeup_needed = 1
        self.wakeup_needed_lock.release()

        callback_list = self.main_loop_poll()

        ## no wakeup needs to occur during dispatch
        self.wakeup_needed_lock.acquire(1)
        self.wakeup_needed = 0
        self.wakeup_needed_lock.release()  

        self.main_loop_dispatch(callback_list)


    def find_bad_sockets(self):
        self.read_lock.acquire(1)
        self.write_lock.acquire(1)
        
        callback_list = MainLoop.find_bad_sockets(self)

        self.read_lock.release()
        self.write_lock.release()

        return callback_list
    
            
    def get_expired_timeouts(self):
        self.timeout_lock.acquire(1)
        callback_list = MainLoop.get_expired_timeouts(self)
        self.timeout_lock.release()
        return callback_list



## *&#$ NT/Win32 can't select on files, so the pipe() mechanism doesn't
## work...

class Win32ThreadedMainLoop(ThreadedMainLoop):
    def init_wakeup(self):
        ## bind a server to localhost
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(0)
        
        sock.bind(("127.0.0.1", 0))
        sock.listen(1)
        addr, port = sock.getsockname()

        ## create a socket to connect to the server
        self.thread_wakeup_write = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.thread_wakeup_write.setblocking(0)
        try:
            self.thread_wakeup_write.connect((addr, port))
        except socket.error:
            pass

        ## crazy loopy
        while 1:
            try:
                self.thread_wakeup_read, addr = sock.accept()
            except socket.error:
                time.sleep(0.1)
            else:
                sock.close()
                break

        self.thread_wakeup_read.setblocking(0)
        self.add_read_cb(self.thread_wakeup_read, self.wakeup_cb, None)


    def wakeup_cb(self, thread_wakeup_read):
        self.wakeup_needed_lock.acquire(1)
        
        try:
            self.thread_wakeup_read.recv(1)
        except socket.error:
            pass
        
        self.wakeup_needed_lock.release()
        

    def wakeup(self):
        self.wakeup_needed_lock.acquire(1)
        
        if self.wakeup_needed:
            self.thread_wakeup_write.send('Z')
            self.wakeup_needed = 0

        self.wakeup_needed_lock.release()


## use the correct class of MainLoop depending on platform type
_mainloop_class = None

if os.name == 'posix':
     _mainloop_class = ThreadedMainLoop
elif os.name == 'nt' or os.name == 'dos':
     _mainloop_class = Win32ThreadedMainLoop
elif os.name == 'mac':
     _mainloop_class = MainLoop


## working instance of the network engine
_engine_hash = {}
_engine_hash_lock = thread.allocate_lock()


def get_thread_engine(thread_ident):
    _engine_hash_lock.acquire(1)

    try:
        current_engine = _engine_hash[thread_ident]
    except KeyError:
        _engine_hash[thread_ident] = current_engine = _mainloop_class()

    _engine_hash_lock.release()
    return current_engine


def delete_thread_engine(thread_ident):
    _engine_hash_lock.acquire(1)

    try:
        del _engine_hash[thread_ident]
    except KeyError:
        pass

    _engine_hash_lock.release()
  

def get_engine():
    return get_thread_engine(thread.get_ident())


def delete_engine():
    delete_thread_engine(thread.get_ident())


## mainloop API
def main():
    get_engine().main()

def main_loop_iteration():
    get_engine().main_loop_iteration()

def done():
    get_engine().done()

def add_timeout_cb(seconds, callback, data = None):
    return get_engine().add_timeout_cb(seconds, callback, data)

def remove_timeout_cb(id):
    get_engine().remove_timeout_cb(id)

def add_idle_cb(callback, data = None):
    get_engine().add_timeout_cb(0, callback, data)

def add_read_cb(sock, callback, data = None):
    get_engine().add_read_cb(sock, callback, data)

def remove_read_cb(sock):
    get_engine().remove_read_cb(sock)
    
def add_write_cb(sock, callback, data = None):
    get_engine().add_write_cb(sock, callback, data)

def remove_write_cb(sock):
    get_engine().remove_write_cb(sock)

def add_tcp_server(port, callback, data = None):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', port))
    sock.listen(5)

    add_read_cb(sock, callback, data)
    return sock


## scheduling on other threads
def add_timeout_cb_on_thread(thread_ident, seconds, callback, data = None):
    get_thread_engine(thread_ident).add_timeout_cb(seconds, callback, data)
    
def remove_timeout_cb_on_thread(thread_ident, id):
    get_thread_engine(thread_ident).remove_timeout_cb(id)
    
def add_idle_cb_on_thread(thread_ident, callback, data = None):
    get_thread_engine(thread_ident).add_timeout_cb(0, callback, data)
    
def add_read_cb_on_thread(thread_ident, sock, callback, data = None):
    get_thread_engine(thread_ident).add_read_cb(sock, callback, data)

def remove_read_cb_on_thread(thread_ident, sock):
    get_thread_engine(thread_ident).remove_read_cb(sock)
    
def add_write_cb_on_thread(thread_ident, sock, callback, data = None):
    get_thread_engine(thread_ident).add_write_cb(sock, callback, data)

def remove_write_cb_on_thread(thread_ident, sock):
    get_thread_engine(thread_ident).remove_write_cb(sock)

def add_tcp_server_on_thread(thread_ident, port, callback, data = None):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', port))
    sock.listen(5)

    add_read_cb_on_thread(thread_ident, sock, callback, data)
    return sock
