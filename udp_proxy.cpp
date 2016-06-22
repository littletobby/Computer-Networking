#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
using namespace std;

const size_t MAX_BUF_SIZE = 1024;
const int MaxListenNum = 1024;

class SelectService{
	unsigned short port;
	int listenfd;
	set<int> sockets;
	int f[65536];
	//int udp_fd[65536];
	UdpSocket* udp_fd[65536];
	struct sockaddr_in servaddr;
	bool server_client;
	socklen_t addr_len;
public:
	SelectService(unsigned short port): port(port), listenfd(0) {
		addr_len = sizeof servaddr;
	}
	void init(char* listen_ip, unsigned short listen_port, char* serv_ip, unsigned short serv_port, bool server_client);
	void run();
	void ac();
	bool send_buf(int u);
};

struct UdpSocket{
	struct sockaddr_in servaddr;
	socklen_t addr_len;
	int fd;
	int mod = 128;
	char send_buf[128][1024];
	char recv_buf[128][1024];
	bool recv_vst[128];
	int recv_len[128];
	int send_l = 0, send_r = 0;
	int recv_l = 0, recv_r = 0;
	int cwnd = 0;
	//int seq = 0, ack = 0;
	int ack = 0;
	int EstimatedRTT = 0;
	int SampleRTT = 0;
	int DevRTT = 0;
	double alpha = 0.125, beta = 0.25;
	int stat = 0;
	UdpSocket(int fd, struct sockaddr_in servaddr) {
		this.fd = fd;
		this.servaddr = servaddr;
		addr_len = sizeof servaddr;
		handshake();
	}
	~UdpSocket() {
		//close(fd);
	}
	void send(char* buf, int len);
	void recv();
	int read(char* buf);
	void head(char* buf, int ackno, int seqno);
	bool is_send();
	bool is_read();
	void handshake();
	void send_again();
};

void head(char* buf, int ackno, int seqno) {
	buf[0] = ack & 0xff;
	buf[1] = ack >> 8 & 0xff;
	buf[2] = ack >> 16 & 0xff;
	buf[3] = ack >> 24 & 0xff;
	buf[4] = seq & 0xff;
	buf[5] = seq >> 8 & 0xff;
	buf[6] = seq >> 16 & 0xff;
	buf[7] = seq >> 24 & 0xff;
}

void UdpSocket::send(char* buf, int len) {
	if (!is_send()) {
		printf("send buffer is full\n");
		return;
	}
	send_r++;
	memcpy(send_buf[send_r % mod], buf, len * sizeof(char));
	if (send_r <= send_l + cwnd) {
		char tmp[1032];
		memcpy(tmp + 8, send_buf[send_r % mod], len * sizeof(char));
		head(tmp, ack, send_r);
		sendto(fd, tmp, len + 8, 0, (struct sockaddr*)&servaddr, sizeof servaddr);
	}
}

void UdpSocket::recv() {
	char tmp[1032];
	int n, seq_no, ack_no;
	n = recvfrom(fd, tmp, sizeof tmp, 0, (struct servaddr*)&servaddr, &addr_len);

	if (n >= 8) {
		stat = 1;
	}
	ack_no = tmp[0] & 0xff;
	ack_no |= (tmp[1] & 0xff) << 8;
	ack_no |= (tmp[2] & 0xff) << 16;
	ack_no |= (tmp[3] & 0xff) << 24;
	seq_no = tmp[4] & 0xff;
	seq_no |= (tmp[5] & 0xff) << 8;
	seq_no |= (tmp[6] & 0xff) << 16;
	seq_no |= (tmp[7] & 0xff) << 24;

	if (ack_no > send_l) {
		send_l = ack_no;
	}
	if (ack_no < send_l) {

	}

	if (seq_no < ack) {//packet order

		return;
	}
	if (n == 8) return;
	if (recv_vst[seq_no % mod]) return;
	memcpy(recv_buf[seq_no % mod], tmp + 8, (n - 8) * sizeof(char));
	recv_len[seq_no % mod] = n - 8;
	recv_vst[seq_no % mod] = 1;
	if (seq_no == ack + 1) {
		while (recv_vst[(ack + 1) % mod]) ack++;
		head(tmp, ack, send_r);
		sendto(fd, tmp, 8, 0, (struct sockaddr*)&servaddr, sizeof servaddr);
	}
}

int UdpSocket::read(char* buf) {
	if (!is_read()) {
		printf("read is not ready\n");
		return;
	}
	recv_l++;
	int len = recv_len[recv_l % mod];
	memcpy(buf, recv_buf[recv_l % mod], len * sizeof(char));
	recv_vst[recv_l % mod] = 0;
	return len;
}

bool UdpSocket::is_send() {
	return send_r - send_l < mod;
}

bool UdpSocket::is_read() {
	return recv_l < recv_r && recv_vst[recv_l + 1];
}

void SelectService::init(char* listen_ip, unsigned short listen_port, char* serv_ip, unsigned short serv_port, bool server_client) {
	struct sockaddr_in s_addr, c_addr;

	this.server_client = server_client;

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

	if ((listenfd = socket(AF_INET, server_client ? SOCK_DGRAM : SOCK_STREAM, 0)) == -1) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(listen_port);
	if (inet_pton(AF_INET, listen_ip, &s_addr.sin_addr) <= 0) {
		printf("inet_pton error for %s\n", listen_ip);
		return;
	}

	if (bind(listenfd, (struct sockaddr*)(&s_addr), sizeof(s_addr)) == -1) {
		printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
		return;
	}

	if (!server_client && listen(listenfd, MaxListenNum) == -1) {
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
		tv.rv_usec = 500;
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
			printf("select error: %s(errno: %d)\n", strerror(errno), errno);
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

				if (server_client) {
					UdpSocket* udpsocket = udp_fd[*it];
					udpsocket->recv();
					if (udpsocket->is_read()) {
						ret = udpsocket->read(buf);
						if (send(f[*it], buf, ret, 0) == 0) goto R;
					}
				}
				else {
					UdpSocket* udpsocket = udp_fd[f[*it]];
					if (udpsocket->is_send()) {
						if ((ret = recv(*it, buf, MAX_BUF_SIZE, 0)) <= 0) goto R;
						udpsocket->send(buf, ret);
					}
				}

				continue;
				R:;
				printf("%d socket:%d %d close", socketSize, *it, f[*it]);
				rec.push_back(*it);
				rec.push_back(f[*it]);
				if (udp_fd[*it] != NULL) delete udp_fd[*it];
				if (udp_fd[f[*it]] != NULL) delete udp_fd[f[*it]];
				//fd_len[*it] = 0;
				//fd_len[f[*it]] = 0;
				close(*it);
				close(f[*it]);
				f[f[*it]]=0;
				f[*it]=0;
			}
		}
	}
}

void SelectService::ac() {
	if (server_client) {
		int n;
		char buf[1024] = {0};
		struct sockaddr_in connaddr;
		n = recvfrom(listenfd, buf, sizeof buf, 0, (struct servaddr*)&connaddr, &addr_len);
		int connfd;
		if ((connfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
			//goto R;
		}
		udp_fd[connfd] = new UdpSocket(connfd, connaddr);
		sockets.insert(connfd);
		printf("client is connect %d\n", connfd);

		int clientfd;
		if ((clientfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
			goto S;
		}
		if (connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
			goto S;
		}
		printf("server is connect %d\n", clientfd);
		sockets.insert(clientfd);
		f[connfd] = clientfd;
		f[clientfd] = connfd;
		return;
S:;
		close(connfd);
	}
	else {
		struct sockaddr client_addr;
		//socklen_t length = sizeof(client_addr);
		int connfd;

		if ((connfd = accept(listenfd, &client_addr, &addr_len)) == -1) {
			printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
			return;
		}
		sockets.insert(connfd);
		printf("client is connect %d\n", connfd);

		int clientfd;
		if ((clientfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
			goto R;
		}
		/*if (connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
			goto R;
		}
		printf("server is connect %d\n", clientfd);*/
		udp_fd[clientfd] = new UdpSocket(clientfd, servaddr);
		sockets.insert(clientfd);
		f[connfd] = clientfd;
		f[clientfd] = connfd;
		return;
R:;
		close(connfd);
	}
}

void UdpSocket::handshake() {
	char buf[1024] = {0};
	head(buf, 0, 0);
	sendto(fd, buf, 8, 0, (struct sockaddr*)&connaddr, sizeof connaddr);
}

void UdpSocket::send_again() {
	if (stat == 0) handshake();
	else {
		int r = min(send_r, send_l + cwnd);
		char buf[1032] = {0};
		for (int i = send_l + 1; i <= r; i++) {
			//send_r++;
			//memcpy(send_buf[i % mod], buf, len * sizeof(char));
			//if (send_r <= send_l + cwnd) {
				//char tmp[1032];
			memcpy(tmp + 8, buf, len * sizeof(char));
			head(tmp, ack, i);
			sendto(fd, tmp, len + 8, 0, (struct sockaddr*)&servaddr, sizeof servaddr);
		}
	}
}

SelectService ss(10241);
int main() {
	if (argc == 2) {
		if (string(argv[1]) == "--Alice") {
			char listen_ip[] = "127.0.0.1";
			char serv_ip[] = "172.19.0.1";
			ss.init(listen_ip, 7000, serv_ip, 10241, 0);
			ss.run();
			goto R;
		}
		else if (string(argv[1]) == "--Bob") {
			char listen_ip[] = "172.20.0.1";
			char serv_ip[] = "127.0.0.1";
			ss.init(listen_ip, 10241, serv_ip, 80, 1);
			ss.run();
			goto R;
		}
	}
	printf("usage: ./tcp_proxy <--Alice> <--Bob>\n");
R:;
	return 0;
}