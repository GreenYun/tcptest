#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "tcpconn.h"

#define MAX_PORT_NUM 65535

#define _BSWAP_16(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

void test_connect(const char *host, const char *service);
void test_connect_with_port(const char *host, unsigned short port);

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *host = argv[1];
	const char *service = argv[2];

	errno = 0;
	char *endptr = NULL;
	unsigned long port = strtoul(service, &endptr, 10);
	if (errno != 0) {
		perror("strtoul");
	}

	if (*endptr == '\0') {
		if (port > MAX_PORT_NUM) {
			fprintf(stderr, "Invalid port number.\n");
			exit(EXIT_FAILURE);
		} else {
			test_connect_with_port(host, port);
		}
	} else {
		test_connect(host, service);
	}

	return 0;
}

void try_connect(struct addrinfo *addr)
{
	char ip[INET6_ADDRSTRLEN];
	void *addr;
	in_port_t port;

	if (addr->ai_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr->ai_addr;
		addr = &(ipv4->sin_addr);
		port = ipv4->sin_port;
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr->ai_addr;
		addr = &(ipv6->sin6_addr);
		port = ipv6->sin6_port;
	}

	const char *p_ip = inet_ntop(addr->ai_family, addr, ip, sizeof ip);
	if (p_ip == NULL)
		perror("inet_ntop");

	port = _BSWAP_16(port);

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
		else if (err == ECONNREFUSED)
			printf("refused.\n");
		else if (err == EHOSTUNREACH)
			printf("unreachable.\n");
		else
			printf("failed with error `%s'.\n", strerror(err));

	} else {
		printf("connected.\n");
	}

	close(fd);
}

void test_connect(const char *host, const char *service)
{
	struct addrinfo hints, *addr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_TCP;

	int ret = getaddrinfo(host, service, &hints, &addr);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	for (struct addrinfo *p = addr; p != NULL; p = p->ai_next)
		try_connect(p);

	freeaddrinfo(addr);
}

void test_connect_with_port(const char *host, unsigned short port)
{
	char service[6];
	snprintf(service, sizeof service, "%hu", port);
	test_connect(host, service);
}
