#include "tcpconn.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

// Convert the timespec to milliseconds.
#define _MSEC(sec, nsec) ((sec) * 1000 + (nsec) / 1000000)

int connect_with_timeout_inner(int fd, const struct sockaddr *addr, socklen_t addrlen, struct timeval *timeout)
{
	int ret = 0;

	// Store the original socket flags.
	int sock_flags_store = fcntl(fd, F_GETFL, 0);
	if (sock_flags_store == -1)
		return -1;

	// Set the socket to non-blocking mode.
	int sock_flags = sock_flags_store | O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, sock_flags);
	if (ret == -1)
		return -1;

	do {
		if (connect(fd, addr, addrlen) == -1) {
			if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
				ret = -1;
			} else {
				struct timespec now;
				ret = clock_gettime(CLOCK_MONOTONIC, &now);
				if (ret == -1)
					break;

				struct timespec deadline = {
					.tv_sec = now.tv_sec + timeout->tv_sec,
					.tv_nsec = now.tv_nsec + timeout->tv_usec * 1000,
				};

				do {
					struct pollfd pfd = {
						.fd = fd,
						.events = POLLOUT,
					};

					ret = clock_gettime(CLOCK_MONOTONIC, &now);
					if (ret == -1)
						break;

					int to_msec = _MSEC((deadline.tv_sec - now.tv_sec), (deadline.tv_nsec - now.tv_nsec));

					ret = poll(&pfd, 1, to_msec);

					// Maybe interrupted.
					if (ret == -1)
						continue;

					// Nothing happened, we issue a timeout.
					if (ret == 0) {
						errno = ETIMEDOUT;
						ret = -1;
						break;
					} else {
						int err;
						socklen_t err_len = sizeof err;
						ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
						if (ret == -1) {
							break;
						} else {
							// poll is done and we set errno to the socket error.
							if (err != 0) {
								errno = err;
								ret = -1;
							}

							// ret is guaranteed to be 0.
							break;
						}
					}
				} while (ret == -1 && errno == EINTR);
			}
		}
	} while (0);

	int err = errno;

	// Assume this function succeeds as we have already set once.
	fcntl(fd, F_SETFL, sock_flags_store);

	// Recover the errno.
	errno = err;

	return ret;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, struct timeval *timeout)
{
	if (timeout == NULL || (timeout->tv_sec == 0 && timeout->tv_usec == 0))
		return connect(fd, addr, addrlen);
	else
		return connect_with_timeout_inner(fd, addr, addrlen, timeout);
}
