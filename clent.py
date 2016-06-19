#!/usr/bin/env python  
# -*- coding: utf-8 -*-  
  
import socket, struct  
import sys  
import traceback  
import json  
import time  
  
reload(sys)  
sys.setdefaultencoding('utf-8')  
  
ip = '127.0.0.1'  
port = 10241  
  
def init():     
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  
    client_socket.settimeout(2) # set time out  
    client_socket.connect((ip, port))  
    return client_socket  
  
def cleanup(client_socket):  
    if client_socket:  
        try:  
            client_socket.close()  
        except:  
            print "catch an exception %s" % (traceback.format_exc())  
  
if __name__ == '__main__':      
    req_str = "this is test."  
      
    client_socket = init()  
    for j in range(1000):  
        try:  
            ret_val = ""  
            client_socket.send('%s %d' % (req_str, j))  
            buf_ret = client_socket.recv(1024)  
            print buf_ret  
            time.sleep(1)  
              
        except socket.timeout, e:  
            print "socket timeout error: %s" %(traceback.format_exc())  
        except Exception, e:  
            print "socket send and recieve Error: %s" %(traceback.format_exc())  
        else:  
            pass  

