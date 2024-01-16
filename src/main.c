#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "tcpconn.h"

void test_connect(const char *host, const char *service);

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *host = argv[1];
	const char *service = argv[2];
	test_connect(host, service);

	return 0;
}

void try_connect(struct addrinfo *addr)
{
	char ip[INET6_ADDRSTRLEN];
	void *in_addr;
	unsigned short port;

	if (addr->ai_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr->ai_addr;
		in_addr = &(ipv4->sin_addr);
		port = ntohs(ipv4->sin_port);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr->ai_addr;
		in_addr = &(ipv6->sin6_addr);
		port = ntohs(ipv6->sin6_port);
	}

	const char *p_ip = inet_ntop(addr->ai_family, in_addr, ip, sizeof ip);
	if (p_ip == NULL)
		perror("inet_ntop");

	if (addr->ai_family == AF_INET)
		printf("Trying IP %s:%d ... ", p_ip, port);
	else
		printf("Trying IPv6 [%s]:%d ... ", p_ip, port);

	fflush(stdout);

	int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (fd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct timeval to = {
		.tv_sec = 2,
		.tv_usec = 0,
	};

	if (connect_with_timeout(fd, addr->ai_addr, addr->ai_addrlen, &to) == -1) {
		int err = errno;
		if (err == ETIMEDOUT)
			printf("timed out.\n");
		else if (err == ENETDOWN)
			printf("network down.\n");
		else if (err == ENETUNREACH)
			printf("unreachable.\n");
		else if (err == ENETRESET)
			printf("TCP reset.\n");
		else if (err == ECONNREFUSED)
			printf("refused.\n");
		else if (err == EHOSTDOWN)
			printf("host is down.\n");
		else
			printf("failed with error `%s'.\n", strerror(err));
	} else {
		printf("connected.\n");
	}

	close(fd);
}

void test_connect(const char *host, const char *service)
{
	struct addrinfo *addr, hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};

	int ret = getaddrinfo(host, service, &hints, &addr);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	for (struct addrinfo *p = addr; p != NULL; p = p->ai_next)
		try_connect(p);

	freeaddrinfo(addr);
}
