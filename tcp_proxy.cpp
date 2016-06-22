#include <iostream>
#include <vector>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

const size_t MAX_BUF_SIZE = 1024;
const int MaxListenNum = 1024;

class SelectService{
	unsigned short port;
	int listenfd;
	set<int> sockets;
	int f[65536];
	struct sockaddr_in servaddr;
public:
	SelectService(unsigned short port): port(port), listenfd(0) {}
	void init(char* listen_ip, unsigned short listen_port, char* serv_ip, unsigned short serv_port);
	void run();
	void ac();
	bool send_buf(int u);
};

char fd_buf[65536][1024];
int fd_len[65536];

void SelectService::init(char* listen_ip, unsigned short listen_port, char* serv_ip, unsigned short serv_port) {
	struct sockaddr_in s_addr, c_addr;

	sockets.clear();

	memset(f, 0, sizeof f);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(serv_port);
	if (inet_pton(AF_INET, serv_ip, &servaddr.sin_addr) <= 0) {
		printf("inet_pton error for %s\n", serv_ip);
		return;
	}
	printf("server ip: %d\n", servaddr.sin_addr);

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	//s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(listen_port);
	if (inet_pton(AF_INET, listen_ip, &s_addr.sin_addr) <= 0) {
		printf("inet_pton error for %s\n", listen_ip);
		return;
	}

	if (bind(listenfd, (struct sockaddr*)(&s_addr), sizeof(s_addr)) == -1) {
		printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}

	if (listen(listenfd, MaxListenNum) == -1) {
		printf("listen socker error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}
}

void SelectService::run() {
	fd_set rdfds;
	fd_set wrfds;
	struct timeval tv;
	int socketSize;
	vector<int> rec;

	while (1) {
		tv.tv_sec = 2;
		tv.tv_usec = 500;
		socketSize = listenfd;
		FD_ZERO(&rdfds);
		FD_SET(listenfd, &rdfds);
		FD_ZERO(&wrfds);
		for (set<int>::iterator it = sockets.begin(); it != sockets.end(); it++) {
			FD_SET(*it, &rdfds);
			FD_SET(*it, &wrfds);
			if ((*it) > socketSize) socketSize = *it;
		}
		rec.clear();
		//printf("select ready\n");
		int ret = select(socketSize + 1, &rdfds, &wrfds, NULL, &tv);
		if (ret < 0) {
			printf("select error: %s(errno: %d)\n", strerror(errno), errno);
			//printf("listen size: %d\n", sockets.size());
			return;
		}
		else if (ret == 0) {
			printf("select timeout. socket size: %d\n", socketSize);
		}
		else {
			if (FD_ISSET(listenfd, &rdfds)) {
				ac();
			}

			char buf[MAX_BUF_SIZE] = {0};
			for (set<int>::iterator it = sockets.begin(); it != sockets.end(); it++) {
				if (f[*it] == 0) {
					continue;
				}
				if (fd_len[*it] && FD_ISSET(*it, &wrfds)) {
					if (send_buf(*it)) goto R;
					/*if (send(*it, fd_buf[*it], fd_len[*it], 0) == 0) {
						fd_len[*it] = 0;
						goto R;
					}*/
				}
				if (FD_ISSET(*it, &rdfds)) {
					//memset(buf, 0, sizeof(buf));
					//printf("socket %d read\n", *it);
					if (fd_len[f[*it]]) continue;
					ret = recv(*it, buf, MAX_BUF_SIZE, 0);
					if (ret <= 0) goto R;
					//if (ret > 0) {
						//printf("%d socket:%d data:%s\n", socketSize, *it, buf);
						//strcpy(buf, "hello, this is select server.");
					if (FD_ISSET(*it, &wrfds)) {
						/*if (fd_len[f[*it]]) {
							if (send(f[*it], fd_buf[f[*it]], fd_len[f[*it]]) == 0) {
								fd_len[f[*it]] = 0;
								goto R;
							}
						}*/
						if (send(f[*it], buf, ret, 0) == 0) goto R;
					}
					else {
						fd_len[f[*it]] = ret;
						memcpy(fd_buf[f[*it]], buf, sizeof buf);
					}
					//}
					//else {
						
						//it = sockets.erase(it);
					//}
				}
				continue;
R:;
				printf("%d socket:%d %d close", socketSize, *it, f[*it]);
				rec.push_back(*it);
				rec.push_back(f[*it]);
				fd_len[*it] = 0;
				fd_len[f[*it]] = 0;
				close(*it);
				close(f[*it]);
				f[f[*it]]=0;
				f[*it]=0;
				//else {
					//it++;
				//}
			}
			for (int i = 0; i < rec.size(); i++) {
				sockets.erase(rec[i]);
			}
		}
	}
}
bool SelectService::send_buf(int u) {
	bool flag = 0;
	if (send(u, fd_buf[u], fd_len[u], 0) == 0) flag = 1;
	fd_len[u] = 0;
	return flag;
}
char sendline[4096];
void SelectService::ac() {
	struct sockaddr client_addr;
	socklen_t length = sizeof(client_addr);
	int connfd;

	if ((connfd = accept(listenfd, &client_addr, &length)) == -1) {
		printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
		int t = 0 / 0;
		return;
	}
	sockets.insert(connfd);
	printf("client is connect %d\n", connfd);
	//return;
	int clientfd;
	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		goto R;
		//close(connfd);
		//return;
	}
	if (connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
		goto R;
		//close(connfd);
		//return;
	}
	printf("server is connect %d\n", clientfd);
	/*fgets(sendline, 4096, stdin);
    if( send(clientfd, sendline, strlen(sendline), 0) < 0) {
    	printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
    	return;
    //exit(0);
    }*/
	sockets.insert(clientfd);
	f[connfd] = clientfd;
	f[clientfd] = connfd;
	return;
R:;
	close(connfd);
}

SelectService ss(10241);

int main(int argc, char** argv) {
	if (argc == 1) {
		char listen_ip[] = "127.0.0.1";
		char serv_ip[] = "127.0.0.1";
		//SelectService ss(10241);
		ss.init(listen_ip, 10241, serv_ip, 80);
		ss.run();
	}
	else if (argc == 2) {
		if (string(argv[1]) == "--Alice") {
			//SelectService ss(7000);
			char listen_ip[] = "127.0.0.1";
			char serv_ip[] = "172.19.0.1";
			ss.init(listen_ip, 7000, serv_ip, 10241);
			ss.run();
		}
		else if (string(argv[1]) == "--Bob") {
			char listen_ip[] = "172.20.0.1";
			char serv_ip[] = "127.0.0.1";
			ss.init(listen_ip, 10241, serv_ip, 80);
			ss.run();
		}
		else {
			printf("usage: ./tcp_proxy <--Alice> <--Bob>\n");
		}
	}
	return 0;
}