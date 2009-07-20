# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: protocol.py,v 1.4 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Async/sync generic protocol system.  Uses mainloop.py, and cPickle
to pickle and send objects across a TCP socket."""

import sys
import string
import re
import socket
import cPickle
import Queue
import mainloop
import outmsg


## the passed network message objects
class Message:
    def __init__(self, type):
        self.type = type
        self.data = None
        
    def __repr__(self):
        print 'Message %s' % (self.type)

    def __str__(self):
        print 'Message %s' % (self.type)


## abstract interface for message pipes
class MessagePipe:
    def __init__(self):
        self.message_cb = None
        self.close_cb = None

    def set_message_cb(self, message_cb):
        self.message_cb = message_cb

    def set_close_cb(self, close_cb):
        self.close_cb = close_cb

    def send(self, message):
        pass

    def close(self):
        self.call_close_cb()

    def call_close_cb(self):
        if self.close_cb:
            self.close_cb(self)
                
    def call_message_cb(self, message):
        if self.message_cb:
            self.message_cb(self, message)
        


class LocalMessagePipe(MessagePipe):
    def __init__(self):
        MessagePipe.__init__(self)
        self.destination_message_pipe = None

        ## incoming_message_queue is a thread-safe queue where
        ## incoming messages are placed
        self.incoming_message_queue = Queue.Queue(0)


    def set_destination_message_pipe(self, destination_message_pipe):
        self.destination_message_pipe = destination_message_pipe


    def incoming_message(self, message):
        self.incoming_message_queue.put(message)


    def send(self, message):
        self.destination_message_pipe.incoming_message(message)


    def run_incoming_queue(self):
        while not self.incoming_message_queue.empty():
            message = self.incoming_message_queue.get()
            self.call_message_cb(message)


class SocketMessagePipe(MessagePipe):
    STATE_HEADER = 0
    STATE_MESSAGE = 1
    
    message_match = re.compile('^MESSAGE BYTES (\d+)$')

    
    def __init__(self, sock):
        MessagePipe.__init__(self)
        self.sock = sock

        ## message recieve state information
        self.message_length = 0
        self.state = self.STATE_HEADER
        
        ## send and recieve buffers
        self.recv_len = 0
        self.recv_buff = [ "" ]

        self.send_pos = 0
        self.send_buff = ''

        ## add socket to network engine
        self.sock.setblocking(0)
        mainloop.add_read_cb(self.sock, self.P_sock_read_cb, None)


    def send(self, message):
        ## add message to send buffer
        self.send_buff = self.send_buff[self.send_pos:]+self.P_encode(message)
        self.send_pos=0
        
        ## sanity check
        if not len(self.send_buff):
            return

        try:
            send_len = self.sock.send(self.send_buff)
        except socket.error, (err_number, err_string):
            print 'socket.error: %s %d' % (err_string, err_number)
            self.close()
            return

        ## zero bytes written is EOF, socket is closed
        if not send_len:
            self.close()
            return

        ## wrote the whole buffer
        if send_len == len(self.send_buff):
            self.send_buff = ''
            mainloop.remove_write_cb(self.sock)
            return

        ## schedule a write-ready callback to write the rest of the data
        if send_len < len(self.send_buff):
            self.send_buff = self.send_buff[send_len:]
            mainloop.add_write_cb(self.sock, self.P_sock_write_cb)
            return


    def close(self):
        mainloop.remove_read_cb(self.sock)
        mainloop.remove_write_cb(self.sock)
        self.call_close_cb()


    def P_sock_write_cb(self, sock):
        try:
            send_len = self.sock.send(self.send_buff[self.send_pos:self.send_pos+8192])
        except socket.error, (err_number, err_string):
            print 'socket.error: %s %d' % (err_string, err_number)
            self.close()
            return

        ## zero bytes written is EOF, socket is closed
        if not send_len:
            self.close()
            return

        ## wrote the whole buffer
        if send_len + self.send_pos == len(self.send_buff):
            self.send_buff = ''
            self.send_pos = 0
            mainloop.remove_write_cb(self.sock)
        else:
            self.send_pos = self.send_pos + send_len
            return


    def P_sock_read_cb(self, sock):
        try:
            buff = self.sock.recv(8192)
        except socket.error:
            self.close()
            return
        
        ## check for socket EOF
        if not buff:
            self.close()
            return
        
        self.recv_buff.append(buff)
        self.recv_len = self.recv_len + len(buff)
                
        ## loop until the buffer is empty or there is
        ## not enough data to decode a complete message
        while self.recv_len:
            
            ## read the header for an incoming message
            if self.state == self.STATE_HEADER:
                if not self.P_read_header():
                    break

            ## read and decode the message
            if self.state == self.STATE_MESSAGE:
                if not self.P_read_message():
                    break


    def P_read_header(self):
        while 1:
            line = self.P_read_line()
            if not line:
                return 0

            mgroup = self.message_match.match(line)
            if mgroup:
                break

        ## message line, get message length
        self.message_length = string.atoi(mgroup.group(1))
        self.state = self.STATE_MESSAGE
        return 1
            

    def P_read_message(self):
        if self.recv_len < self.message_length:
            return 0

        ## decode the message
        if len(self.recv_buff) > 1:
            self.recv_buff = [ string.join(self.recv_buff,"") ]
        message = self.P_decode(self.recv_buff[0][:self.message_length])

        ## remove the message from the buffer
        self.recv_buff[0] = self.recv_buff[0][self.message_length:]
        self.recv_len = len(self.recv_buff[0])

        self.state = self.STATE_HEADER

        ## deliver the message if we were sucessful in decoding it
        if message:
            self.call_message_cb(message)
        
        return 1


    def P_read_line(self):
        if len(self.recv_buff) > 1:
            self.recv_buff = [ string.join(self.recv_buff,"") ]

        index = string.find(self.recv_buff[0], '\r\n')
        if index < 0:
            return None

        line = self.recv_buff[0][:index]
        self.recv_buff[0] = self.recv_buff[0][index+2:]
        self.recv_len = len(self.recv_buff[0])

        return line


    ## encode/decode message objects into network form
    def P_encode(self, message):
        buff = ''

        try:
            buff = cPickle.dumps(message, 1)
            buff = 'MESSAGE BYTES %d\r\n%s' % (len(buff), buff)
        except:
            pass
        
        return buff


    def P_decode(self, data):
        message = None

        try:
            message = cPickle.loads(data)
        except:
            pass
        
        return message


## entrypoints
def CreateMessage(type):
    return Message(type)
    
def CreateSocketMessagePipe(sock):
    return SocketMessagePipe(sock)

def CreateLocalMessagePipePair():
    mp1 = LocalMessagePipe()
    mp2 = LocalMessagePipe()

    mp1.set_destination_message_pipe(mp2)
    mp2.set_destination_message_pipe(mp1)

    return mp1, mp2

