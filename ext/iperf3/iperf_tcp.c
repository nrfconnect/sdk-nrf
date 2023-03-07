/*
 * iperf, Copyright (c) 2014-2019, The Regents of the University of
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <limits.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_tcp.h"
#include "net.h"
#include "cjson.h"

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
#include "iperf_util.h"
#endif

#if defined(HAVE_FLOWLABEL)
#include "flowlabel.h"
#endif /* HAVE_FLOWLABEL */

/* iperf_tcp_recv
 *
 * receives the data for TCP
 */
int
iperf_tcp_recv(struct iperf_stream *sp)
{
    int r;
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    #define MAX_READ_COUNT 20 /* in case of very small buffers, 
                                let's not stuck here more than count */
    int count = 0;
    int total_received = 0;
    /* In embedded world, we need to read all from rcv buffer: 
       an another option would be to separate send and receive 
       buffer in order to read more more that blksize */
    do {
        r = Nread(sp->socket, sp->buffer, sp->settings->blksize, Ptcp);
        if (r < 0)
            return r;

        total_received += r;
        count++;

        /* Only count bytes received while we're in the correct state. */
        if (sp->test->state == TEST_RUNNING) {
            sp->result->bytes_received += r;
            sp->result->bytes_received_this_interval += r;
        }
        else {
            if (sp->test->debug)
                iperf_printf(sp->test, "TCP Early/Late receive, state = %d, read %d\n", sp->test->state, r);

            /* NRF_IPERF3_INTEGRATION_TODO: should we count followings still? */
            //sp->result->bytes_received += r;
            //sp->result->bytes_received_this_interval += r;
        }
    } while (r > 0 && count < MAX_READ_COUNT);

    return total_received;

#else
    r = Nread(sp->socket, sp->buffer, sp->settings->blksize, Ptcp);

    if (r < 0)
        return r;

    /* Only count bytes received while we're in the correct state. */
    if (sp->test->state == TEST_RUNNING) {
	sp->result->bytes_received += r;
	sp->result->bytes_received_this_interval += r;
    }
    else {
	if (sp->test->debug)
	    iperf_printf(sp->test, "Late receive, state = %d\n", sp->test->state);
    }

    return r;
#endif    
}


/* iperf_tcp_send 
 *
 * sends the data for TCP
 */
int
iperf_tcp_send(struct iperf_stream *sp)
{
    int r;

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    if (sp->test->zerocopy)
	r = Nsendfile(sp->buffer_fd, sp->socket, sp->buffer, sp->settings->blksize);
    else
#endif    
	r = Nwrite(sp->socket, sp->buffer, sp->settings->blksize, Ptcp);

    if (r < 0)
        return r;

    sp->result->bytes_sent += r;
    sp->result->bytes_sent_this_interval += r;

    if (sp->test->debug && r > 0) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
	    printf("sent %d bytes of %d, total %" PRIu64 "\n", r, sp->settings->blksize, sp->result->bytes_sent);

#else
	    /* 64bit variable printing is not working */
        iperf_printf(sp->test, "sent %d bytes of %d, total %d\n",
                        r, sp->settings->blksize, (uint32_t)sp->result->bytes_sent);
#endif        
    }
    return r;
}


/* iperf_tcp_accept
 *
 * accept a new TCP stream connection
 */
int
iperf_tcp_accept(struct iperf_test * test)
{
    int     s;
    signed char rbuf = ACCESS_DENIED;
    char    cookie[COOKIE_SIZE];
    socklen_t len;
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    struct sockaddr_in addr; /* modified due to nrf91_socket_offload_accept() */
#else
    struct sockaddr_storage addr;
#endif
    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IERECVCOOKIE;
        return -1;
    }

    if (strcmp(test->cookie, cookie) != 0) {
        if (Nwrite(s, (char*) &rbuf, sizeof(rbuf), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return -1;
        }
        close(s);
    }

    return s;
}


/* iperf_tcp_listen
 *
 * start up a listener for TCP stream connections
 */
int
iperf_tcp_listen(struct iperf_test *test)
{
    int s, opt;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* options not supported */
    socklen_t optlen;
#endif	
    int saved_errno;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    int rcvbuf_actual, sndbuf_actual;
#endif

    s = test->listener;

    /*
     * If certain parameters are specified (such as socket buffer
     * size), then throw away the listening socket (the one for which
     * we just accepted the control connection) and recreate it with
     * those parameters.  That way, when new data connections are
     * set, they'll have all the correct parameters in place.
     *
     * It's not clear whether this is a requirement or a convenience.
     */
    if (test->no_delay || test->settings->mss || test->settings->socket_bufsize) {
	struct addrinfo hints, *res;
#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
	char portstr[12];
#else
	char portstr[6];
#endif
        FD_CLR(s, &test->read_set);
        close(s);

        snprintf(portstr, 6, "%d", test->server_port);
        memset(&hints, 0, sizeof(hints));

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
    if (test->pdn_id_str != NULL) {
        hints.ai_flags = (AI_PDNSERV | AI_NUMERICSERV);
        snprintf(portstr, 12, "%d:%s", test->server_port, test->pdn_id_str);
    }
#endif

	/*
	 * If binding to the wildcard address with no explicit address
	 * family specified, then force us to get an AF_INET6 socket.
	 * More details in the comments in netanounce().
	 */
	if (test->settings->domain == AF_UNSPEC && !test->bind_address) {
	    hints.ai_family = AF_INET6;
	}
	else {
	    hints.ai_family = test->settings->domain;
	}
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((gerror = getaddrinfo(test->bind_address, portstr, &hints, &res)) != 0) {
            i_errno = IESTREAMLISTEN;
            return -1;
        }

        if ((s = socket(res->ai_family, SOCK_STREAM, 0)) < 0) {
	    freeaddrinfo(res);
            i_errno = IESTREAMLISTEN;
            return -1;
        }

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
        /* Set PDN based on PDN ID if requested */
        if (test->pdn_id_str != NULL) {
            int ret = iperf_util_socket_pdn_id_set(s, test->pdn_id_str);
            if (ret != 0) {
                iperf_printf(test, "iperf_tcp_listen: cannot bind socket with PDN ID %s\n", test->pdn_id_str);
                i_errno = IESTREAMLISTEN;
                return -1;
            }				
        }
#endif
        if (test->no_delay) {
            opt = 1;
            if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		saved_errno = errno;
		close(s);
		freeaddrinfo(res);
		errno = saved_errno;
                i_errno = IESETNODELAY;
                return -1;
            }
        }
        // XXX: Setting MSS is very buggy!
        if ((opt = test->settings->mss)) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
            if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
#endif
		saved_errno = errno;
		close(s);
		freeaddrinfo(res);
		errno = saved_errno;
                i_errno = IESETMSS;
                return -1;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
            }
#endif
        }
        if ((opt = test->settings->socket_bufsize)) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
            if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0) {
#endif
		saved_errno = errno;
		close(s);
		freeaddrinfo(res);
		errno = saved_errno;
                i_errno = IESETBUF;
                return -1;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
            }
#endif
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
            if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
		saved_errno = errno;
		close(s);
		freeaddrinfo(res);
		errno = saved_errno;
                i_errno = IESETBUF;
                return -1;
            }
#endif            
        }
#if defined(HAVE_SO_MAX_PACING_RATE)
    /* If fq socket pacing is specified, enable it. */
    if (test->settings->fqrate) {
	/* Convert bits per second to bytes per second */
	unsigned int fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		iperf_printf(test, "Setting fair-queue socket pacing to %u\n", fqrate);
	    }
	    if (setsockopt(s, SOL_SOCKET, SO_MAX_PACING_RATE, &fqrate, sizeof(fqrate)) < 0) {
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
		warning(test, "Unable to set socket pacing");
#endif
	    }
	}
    }
#endif /* HAVE_SO_MAX_PACING_RATE */
    {
	unsigned int rate = test->settings->rate / 8;
	if (rate > 0) {
	    if (test->debug) {
		iperf_printf(test, "Setting application pacing to %u\n", rate);
	    }
	}
    }
        opt = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
	    saved_errno = errno;
            close(s);
	    freeaddrinfo(res);
	    errno = saved_errno;
            i_errno = IEREUSEADDR;
            return -1;
        }

	/*
	 * If we got an IPv6 socket, figure out if it shoudl accept IPv4
	 * connections as well.  See documentation in netannounce() for
	 * more details.
	 */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
	if (res->ai_family == AF_INET6 && (test->settings->domain == AF_UNSPEC || test->settings->domain == AF_INET)) {
	    if (test->settings->domain == AF_UNSPEC)
		opt = 0;
	    else 
		opt = 1;
	    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, 
			   (char *) &opt, sizeof(opt)) < 0) {
		saved_errno = errno;
		close(s);
		freeaddrinfo(res);
		errno = saved_errno;
		i_errno = IEV6ONLY;
		return -1;
	    }
	}
#endif /* IPV6_V6ONLY */
#endif
        if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
	    saved_errno = errno;
            close(s);
	    freeaddrinfo(res);
	    errno = saved_errno;
            i_errno = IESTREAMLISTEN;
            return -1;
        }

        freeaddrinfo(res);

        if (listen(s, INT_MAX) < 0) {
            i_errno = IESTREAMLISTEN;
            return -1;
        }

        test->listener = s;
    }
    
    /* Read back and verify the sender socket buffer size */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    optlen = sizeof(sndbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &sndbuf_actual, &optlen) < 0) {
	saved_errno = errno;
	close(s);
	errno = saved_errno;
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("SNDBUF is %u, expecting %u\n", sndbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > sndbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }
#endif

    /* Read back and verify the receiver socket buffer size */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    optlen = sizeof(rcvbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen) < 0) {
	saved_errno = errno;
	close(s);
	errno = saved_errno;
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("RCVBUF is %u, expecting %u\n", rcvbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > rcvbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }

    if (test->json_output) {
	cJSON_AddNumberToObject(test->json_start, "sock_bufsize", test->settings->socket_bufsize);
	cJSON_AddNumberToObject(test->json_start, "sndbuf_actual", sndbuf_actual);
	cJSON_AddNumberToObject(test->json_start, "rcvbuf_actual", rcvbuf_actual);
    }
#endif

    return s;
}


/* iperf_tcp_connect
 *
 * connect to a TCP stream listener
 * This function is roughly similar to netdial(), and may indeed have
 * been derived from it at some point, but it sets many TCP-specific
 * options between socket creation and connection.
 */
int
iperf_tcp_connect(struct iperf_test *test)
{
    struct addrinfo hints, *local_res, *server_res;
#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
	char portstr[12];
#else
    char portstr[6];
#endif
    int s, opt;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    socklen_t optlen;
#endif
    int saved_errno;
    int rcvbuf_actual = 0, sndbuf_actual = 0;

    if (test->bind_address) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = test->settings->domain;
        hints.ai_socktype = SOCK_STREAM;
        if ((gerror = getaddrinfo(test->bind_address, NULL, &hints, &local_res)) != 0) {
            i_errno = IESTREAMCONNECT;
            return -1;
        }
    }

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = test->settings->domain;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(portstr, sizeof(portstr), "%d", test->server_port);

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
    if (test->pdn_id_str != NULL) {
        hints.ai_flags = (AI_PDNSERV | AI_NUMERICSERV);
        snprintf(portstr, 12, "%d:%s", test->server_port, test->pdn_id_str);
    }
#endif

    if ((gerror = getaddrinfo(test->server_hostname, portstr, &hints, &server_res)) != 0) {
	if (test->bind_address)
	    freeaddrinfo(local_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }
#if defined(CONFIG_NRF_IPERF3_INTEGRATION) //wildcard not supported
    if ((s = socket(server_res->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
#else
    if ((s = socket(server_res->ai_family, SOCK_STREAM, 0)) < 0) {
#endif
	if (test->bind_address)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
    /* Set PDN based on PDN ID if requested */
    if (test->pdn_id_str != NULL) {
        int ret = iperf_util_socket_pdn_id_set(s, test->pdn_id_str);
        if (ret != 0) {
            iperf_printf(test, "iperf_tcp_connect: cannot bind socket with PDN ID %s\n", test->pdn_id_str);
            i_errno = IESTREAMLISTEN;
            return -1;
        }				
    }
#endif

    /*
     * Various ways to bind the local end of the connection.
     * 1.  --bind (with or without --cport).
     */
    if (test->bind_address) {
        struct sockaddr_in *lcladdr;
        lcladdr = (struct sockaddr_in *)local_res->ai_addr;
        lcladdr->sin_port = htons(test->bind_port);

        if (bind(s, (struct sockaddr *) local_res->ai_addr, local_res->ai_addrlen) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(local_res);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESTREAMCONNECT;
            return -1;
        }
        freeaddrinfo(local_res);
    }
    /* --cport, no --bind */
    else if (test->bind_port) {
	size_t addrlen;
	struct sockaddr_storage lcl;

	/* IPv4 */
	if (server_res->ai_family == AF_INET) {
	    struct sockaddr_in *lcladdr = (struct sockaddr_in *) &lcl;
	    lcladdr->sin_family = AF_INET;
	    lcladdr->sin_port = htons(test->bind_port);
	    lcladdr->sin_addr.s_addr = INADDR_ANY;
	    addrlen = sizeof(struct sockaddr_in);
	}
	/* IPv6 */
	else if (server_res->ai_family == AF_INET6) {
	    struct sockaddr_in6 *lcladdr = (struct sockaddr_in6 *) &lcl;
	    lcladdr->sin6_family = AF_INET6;
	    lcladdr->sin6_port = htons(test->bind_port);
	    lcladdr->sin6_addr = in6addr_any;
	    addrlen = sizeof(struct sockaddr_in6);
	}
	/* Unknown protocol */
	else {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IEPROTOCOL;
            return -1;
	}

        if (bind(s, (struct sockaddr *) &lcl, addrlen) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESTREAMCONNECT;
            return -1;
        }
    }

    /* Set socket options */
    if (test->no_delay) {
        opt = 1;
        if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESETNODELAY;
            return -1;
        }
    }
    if ((opt = test->settings->mss)) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
        if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
#endif
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESETMSS;
            return -1;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
        }
#endif
    }
    if ((opt = test->settings->socket_bufsize)) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESETBUF;
            return -1;
        }

        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESETBUF;
            return -1;
        }
#else
    if (test->debug) {
	   iperf_printf(test, "Setting of socket buffers are not supported, ignored to set as %d\n", test->settings->socket_bufsize);
    }
#endif
    }

    /* Read back and verify the sender socket buffer size */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    optlen = sizeof(sndbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &sndbuf_actual, &optlen) < 0) {
	saved_errno = errno;
	close(s);
	freeaddrinfo(server_res);
	errno = saved_errno;
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("SNDBUF is %u, expecting %u\n", sndbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > sndbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }
#endif

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    /* Read back and verify the receiver socket buffer size */
    optlen = sizeof(rcvbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen) < 0) {
	saved_errno = errno;
	close(s);
	freeaddrinfo(server_res);
	errno = saved_errno;
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("RCVBUF is %u, expecting %u\n", rcvbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > rcvbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }
#endif

    if (test->json_output) {
	cJSON_AddNumberToObject(test->json_start, "sock_bufsize", test->settings->socket_bufsize);
	cJSON_AddNumberToObject(test->json_start, "sndbuf_actual", sndbuf_actual);
	cJSON_AddNumberToObject(test->json_start, "rcvbuf_actual", rcvbuf_actual);
    }

#if defined(HAVE_FLOWLABEL)
    if (test->settings->flowlabel) {
        if (server_res->ai_addr->sa_family != AF_INET6) {
	    saved_errno = errno;
	    close(s);
	    freeaddrinfo(server_res);
	    errno = saved_errno;
            i_errno = IESETFLOW;
            return -1;
	} else {
	    struct sockaddr_in6* sa6P = (struct sockaddr_in6*) server_res->ai_addr;
            char freq_buf[sizeof(struct in6_flowlabel_req)];
            struct in6_flowlabel_req *freq = (struct in6_flowlabel_req *)freq_buf;
            int freq_len = sizeof(*freq);

            memset(freq, 0, sizeof(*freq));
            freq->flr_label = htonl(test->settings->flowlabel & IPV6_FLOWINFO_FLOWLABEL);
            freq->flr_action = IPV6_FL_A_GET;
            freq->flr_flags = IPV6_FL_F_CREATE;
            freq->flr_share = IPV6_FL_S_ANY;
            memcpy(&freq->flr_dst, &sa6P->sin6_addr, 16);

            if (setsockopt(s, IPPROTO_IPV6, IPV6_FLOWLABEL_MGR, freq, freq_len) < 0) {
		saved_errno = errno;
                close(s);
                freeaddrinfo(server_res);
		errno = saved_errno;
                i_errno = IESETFLOW;
                return -1;
            }
            sa6P->sin6_flowinfo = freq->flr_label;

            opt = 1;
            if (setsockopt(s, IPPROTO_IPV6, IPV6_FLOWINFO_SEND, &opt, sizeof(opt)) < 0) {
		saved_errno = errno;
                close(s);
                freeaddrinfo(server_res);
		errno = saved_errno;
                i_errno = IESETFLOW;
                return -1;
            } 
	}
    }
#endif /* HAVE_FLOWLABEL */

#if defined(HAVE_SO_MAX_PACING_RATE)
    /* If socket pacing is specified try to enable it. */
    if (test->settings->fqrate) {
	/* Convert bits per second to bytes per second */
	unsigned int fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		iperf_printf(test, "Setting fair-queue socket pacing to %u\n", fqrate);
	    }
	    if (setsockopt(s, SOL_SOCKET, SO_MAX_PACING_RATE, &fqrate, sizeof(fqrate)) < 0) {
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
		warning(test, "Unable to set socket pacing");
#endif
	    }
	}
    }
#endif /* HAVE_SO_MAX_PACING_RATE */
    {
	unsigned int rate = test->settings->rate / 8;
	if (rate > 0) {
	    if (test->debug) {
		iperf_printf(test, "Setting application pacing to %u\n", rate);
	    }
	}
    }

    if (connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) {
	saved_errno = errno;
	close(s);
	freeaddrinfo(server_res);
	errno = saved_errno;
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    freeaddrinfo(server_res);

    /* Send cookie for verification */
    if (Nwrite(s, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
	saved_errno = errno;
	close(s);
	errno = saved_errno;
        i_errno = IESENDCOOKIE;
        return -1;
    }

    return s;
}
