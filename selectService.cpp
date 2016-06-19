#include <stdlib.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include "selectService.h"  
#include <iostream>  
#include <memory.h>  
#include <netinet/in.h>  
#include <string.h>  
#include <fcntl.h>   
#include <unistd.h>
#include <errno.h>  
using namespace std;  
  
const size_t MAX_BUF_LEN = 1024;  
const int MaxListenNum = 5;  
int beginListen(unsigned short port){  
    struct  sockaddr_in s_add,c_add;  
    int sfp = socket(AF_INET, SOCK_STREAM, 0);  
    if (-1 == sfp){  
        cout << "socket failed.error:" << strerror(errno) << endl;  
        return -1;  
    }  
    bzero(&s_add,sizeof(struct sockaddr_in));  
    s_add.sin_family=AF_INET;  
    s_add.sin_addr.s_addr=htonl(INADDR_ANY);  
    s_add.sin_port=htons(port);  
  
    if(-1 == bind(sfp,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))  
    {  
        cout << "bind failed. error:" << strerror(errno) << endl;  
        return -1;  
    }  
    if(-1 == listen(sfp, MaxListenNum))  
    {  
        cout << "listen failed. error:" << strerror(errno) << endl;  
        return -1;  
    }  
  
    return sfp;   
}  
  
void SelectService::runImpl()  
{  
    fd_set  rdfds;  
    struct timeval tv;  
    tv.tv_sec = 2;  
    tv.tv_usec = 500;  
    int socketSize = listenSocket;  
    FD_ZERO(&rdfds);  
  
    FD_SET(listenSocket, &rdfds);  
  
    for (list<int>::iterator iter=socketList.begin(); iter!=socketList.end(); ++iter)  
    {  
        FD_SET(*iter, &rdfds);  
        socketSize = *iter> socketSize ? *iter : socketSize;  
    }  
  
    int ret = select(socketSize+1, &rdfds, NULL, NULL, &tv);  
    if (ret < 0)  
    {  
        cout << "select error" << endl;  
    } else if(ret == 0 ){  
        cout << "select time out. socket size:" << socketSize << endl;  
    } else {  
        //查看是否有客户端连接  
        if (FD_ISSET(listenSocket, &rdfds)){  
            //创建与客户端的连接，加入到list里  
            acceptOpe();  
        }  
  
        //判断客户端是否发请求过来  
        char buf[MAX_BUF_LEN] = {0};  
        for (list<int>::iterator iter=socketList.begin(); iter!=socketList.end(); )  
        {  
            memset(buf, 0, MAX_BUF_LEN);  
            //判断socket是否可读  
            if (FD_ISSET(*iter, &rdfds)){  
                ret = recv(*iter, buf, MAX_BUF_LEN, 0);  
                if (ret > 0)  
                {  
                    cout << socketSize << ".socket:" << *iter << ". data:" << buf << endl;  
  
                    strcpy(buf, "hello, this is select server.");  
                    send(*iter, buf, strlen(buf), 0);  
                } else{  
                    //客户端断开，关闭socket  
                    close(*iter);  
                    iter = socketList.erase(iter);  
                }  
            } else {  
                ++iter;  
            }  
        }  
    }  
}  
  
void SelectService::acceptOpe()  
{  
    ///客户端套接字  
    struct sockaddr client_addr;  
    socklen_t length = sizeof(client_addr);  
    ///成功返回非负描述字，出错返回-1  
    int clientConn = accept(listenSocket, (struct sockaddr*)&client_addr, &length);  
    if (clientConn < 0)  
    {  
        cout << "socket error:" << endl;  
        return ;  
    }  
  
    socketList.push_back(clientConn);  
    cout << "client is connect." << clientConn << endl;  
}  
  
bool SelectService::beginRun()  
{  
    //创建一个socket，用于listen  
    listenSocket=beginListen(port_);  
    while (true){  
        runImpl();  
    }  
  
    return true;  
}  
int main(){  
        SelectService selectService(10241);  
        selectService.beginRun();  
          
        return 0;  
}  
