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
const int MaxListenNum = 5;

class SelectService{
	unsigned short port;
	int listenfd;
	set<int> sockets;
	int f[65536];
	struct sockaddr_in servaddr;
public:
	SelectService(unsigned short port): port(port), listenfd(0) {}
	void init(char* serv_ip, unsigned short serv_port);
	void run();
	void ac();
};

void SelectService::init(char* serv_ip, unsigned short serv_port) {
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

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(port);

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
	struct timeval tv;
	int socketSize;
	vector<int> rec;

	while (1) {
		tv.tv_sec = 2;
		tv.tv_usec = 500;
		socketSize = listenfd;
		FD_ZERO(&rdfds);
		FD_SET(listenfd, &rdfds);
		for (set<int>::iterator it = sockets.begin(); it != sockets.end(); it++) {
			FD_SET(*it, &rdfds);
			if ((*it) > socketSize) socketSize = *it;
		}
		rec.clear();

		int ret = select(socketSize + 1, &rdfds, NULL, NULL, &tv);
		if (ret < 0) {
			printf("select error\n");
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
				if (FD_ISSET(*it, &rdfds)) {
					memset(buf, 0, sizeof(buf));
					ret = recv(*it, buf, MAX_BUF_SIZE, 0);
					if (ret > 0) {
						//printf("%d socket:%d data:%s\n", socketSize, *it, buf);
						//strcpy(buf, "hello, this is select server.");
						send(f[*it], buf, ret, 0);
					}
					else {
						//printf("%d socket:%d close", socketSize, *it);
						rec.push_back(*it);
						rec.push_back(f[*it]);
						close(*it);
						close(f[*it]);
						f[f[*it]]=0;
						f[*it]=0;
						//it = sockets.erase(it);
					}
				}
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

void SelectService::ac() {
	struct sockaddr client_addr;
	socklen_t length = sizeof(client_addr);
	int connfd;

	if ((connfd = accept(listenfd, &client_addr, &length)) == -1) {
		printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}
	sockets.insert(connfd);
	printf("client is connect %d\n", connfd);

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
	sockets.insert(clientfd);
	f[connfd] = clientfd;
	f[clientfd] = connfd;
R:;
	close(connfd);
}

int main() {
	char serv_ip[] = "127.0.0.1";
	SelectService ss(10241);
	ss.init(serv_ip, 80);
	ss.run();
	return 0;
}