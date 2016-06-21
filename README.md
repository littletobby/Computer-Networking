# Computer-Networking

Udp

用
```javascript
struct sockaddr_in servaddr;
char serv_ip[] = "127.0.0.1";
if (inet_pton(AF_INET, serv_ip, &servaddr.sin_addr) <= 0) {
		printf("inet_pton error for %s\n", serv_ip);
		return 0;
	}
```

不用
```javascript
servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
```

struct sockadd_in
```javascript
struct sockaddr_in servaddr;
int listenfd;
if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr) < 0) {//struct sockaddr_in conver to struct sockaddr
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}
```

recvfrom
```javascript
addr_len = sizeof connaddr;
n = recvfrom(listenfd, buf, sizeof buf, 0, (struct sockaddr*)&connaddr, &addr_len);
```
