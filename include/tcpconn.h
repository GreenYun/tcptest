#ifndef __TCP_CONN_H__
#define __TCP_CONN_H__
#pragma once

#include <sys/socket.h>
#include <sys/time.h>

// Try connect to the opened socket until timeout.
//
// @param fd       The socket fd.
// @param addr     The address to connect.
// @param addrlen  The length of `addr`.
// @param timeout  The timeout value.
// @return 0 if success, -1 if error. (Same as connect(2))
// @note If `timeout` is NULL or set to 0, it acts the same as connect(2).
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, struct timeval *timeout);

#endif // __TCP_CONN_H__
