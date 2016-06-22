#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

char buf[1024];
int main() {
	int listenfd, n;
	struct sockaddr_in servaddr, connaddr;
	socklen_t addr_len;
	char serv_ip[] = "127.0.0.1";

	addr_len = sizeof connaddr;

	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

	printf("create socket: %d\n", listenfd);

	memset(&servaddr, 0, sizeof servaddr);

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(6666);
	//servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (inet_pton(AF_INET, serv_ip, &servaddr.sin_addr) <= 0) {
		printf("inet_pton error for %s\n", serv_ip);
		return 0;
	}

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr) < 0) {
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

	while (1) {
		memset(buf, 0, sizeof buf);
		n = recvfrom(listenfd, buf, sizeof buf, 0, (struct sockaddr*)&connaddr, &addr_len);

		printf("message form: %s\n", inet_ntoa(connaddr.sin_addr));
		printf("message: %s\n", buf);
		strcpy(buf, "Hello, this is server end.\n");
		sendto(listenfd, buf, strlen(buf), 0, (struct sockaddr*)&connaddr, sizeof connaddr);
	}
	return 0;
}