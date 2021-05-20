/*
 * iperf, Copyright (c) 2014, 2017, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#ifndef __NET_H
#define __NET_H

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
#include "iperf.h"/* Added for passing test parameter (see below) */
#endif

int timeout_connect(int s, const struct sockaddr *name, socklen_t namelen, int timeout);

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
int netdial(struct iperf_test *test, int domain, int proto, const char *local, int local_port, const char *server, int port, int timeout); /* Added test */
int netannounce(struct iperf_test *test, int domain, int proto, const char *local, int port); /* Added test */
#else
int netdial(int domain, int proto, const char *local, int local_port, const char *server, int port, int timeout);
int netannounce(int domain, int proto, const char *local, int port);
#endif

int Nread(int fd, char *buf, size_t count, int prot);
int Nwrite(int fd, const char *buf, size_t count, int prot) /* __attribute__((hot)) */;
int has_sendfile(void);
int Nsendfile(int fromfd, int tofd, const char *buf, size_t count) /* __attribute__((hot)) */;
int setnonblocking(int fd, int nonblocking);
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
int getsockdomain(int sock);
#endif
int parse_qos(const char *tos);

#define NET_SOFTERROR -1
#define NET_HARDERROR -2

#endif /* __NET_H */
