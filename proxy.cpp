#include <iostream>
#include <list>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

const size_t MAX_BUF_SIZE = 1024;
const int MaxListenNum = 5;

class SelectService{
	unsigned short port;
	int listenfd;
	list<int> sockets;
public:
	SelectService(unsigned short port): port(port), listenfd(0) {}
	void init();
	void run();
	void ac();
};

void SelectService::init() {
	struct sockaddr_in s_addr, c_addr;

	sockets.clear();

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

	while (1) {
		tv.tv_sec = 2;
		tv.tv_usec = 500;
		socketSize = listenfd;
		FD_ZERO(&rdfds);
		FD_SET(listenfd, &rdfds);
		for (list<int>::iterator it = sockets.begin(); it != sockets.end(); it++) {
			FD_SET(*it, &rdfds);
			if ((*it) > socketSize) socketSize = *it;
		}

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
			for (list<int>::iterator it = sockets.begin(); it != sockets.end();) {
				if (FD_ISSET(*it, &rdfds)) {
					memset(buf, 0, sizeof(buf));
					ret = recv(*it, buf, MAX_BUF_SIZE, 0);
					if (ret > 0) {
						printf("%d socket:%d data:%s\n", socketSize, *it, buf);
						strcpy(buf, "hello, this is select server.");
						send(*it, buf, strlen(buf), 0);
					}
					else {
						//printf("%d socket:%d close", socketSize, *it);
						close(*it);
						it = sockets.erase(it);
					}
				}
				else {
					it++;
				}
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
	sockets.push_back(connfd);
	printf("client is connect %d\n", connfd);
}

int main() {
	SelectService ss(10241);
	ss.init();
	ss.run();
	return 0;
}