/*
 * iperf, Copyright (c) 2014-2020 The Regents of the University of
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
/* iperf_server_api.c: Functions to be used by an iperf server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <setjmp.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "iperf_util.h"
#include "timer.h"
#include "iperf_time.h"
#include "net.h"
#include "units.h"
#include "iperf_util.h"
#include "iperf_locale.h"

#if defined(HAVE_TCP_CONGESTION)
#if !defined(TCP_CA_NAME_MAX)
#define TCP_CA_NAME_MAX 16
#endif /* TCP_CA_NAME_MAX */
#endif /* HAVE_TCP_CONGESTION */

int
iperf_server_listen(struct iperf_test *test)
{
    retry:
    if((test->listener = netannounce(test, test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
	if (errno == EAFNOSUPPORT && (test->settings->domain == AF_INET6 || test->settings->domain == AF_UNSPEC)) {
	    /* If we get "Address family not supported by protocol", that
	    ** probably means we were compiled with IPv6 but the running
	    ** kernel does not actually do IPv6.  This is not too unusual,
	    ** v6 support is and perhaps always will be spotty.
	    */
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
	    warning(test, "this system does not seem to support IPv6 - trying IPv4");
#endif
	    test->settings->domain = AF_INET;
	    goto retry;
	} else {
	    i_errno = IELISTEN;
	    return -1;
	}
    }

    if (!test->json_output) {
	iperf_printf(test, "-----------------------------------------------------------\n");
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
	/* Added binded address printing: */
	iperf_printf(test, "Server listening on local %s port %d\n", (test->bind_address ? test->bind_address : "local"), test->server_port);
#else
    iperf_printf(test, "Server listening on %d\n", test->server_port);
#endif
	iperf_printf(test, "-----------------------------------------------------------\n");
	if (test->forceflush)
	    iflush(test);
    }

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    if (test->listener > test->max_fd) test->max_fd = test->listener;

    return 0;
}
int
iperf_accept(struct iperf_test *test)
{
    int s;
    signed char rbuf = ACCESS_DENIED;
    socklen_t len;
    struct sockaddr_storage addr;

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    struct sockaddr_in client_addr; 
    struct sockaddr_in6 client6_addr;
    struct sockaddr *sa;
#ifdef SO_RCVTIMEO
    struct timeval tv;
#endif

    /* Workaround due to nrf91_socket_offload_accept / bsdlib that requires len to be either of in4 or in6 */    
    int domain = nrf_iperf3_mock_getsockdomain(test, test->ctrl_sck);

    if (domain == AF_INET) {
        len = sizeof(client_addr);
    }
    else {
        len = sizeof(client6_addr);
    }

    if ((s = accept(test->listener, (struct sockaddr *)&addr, &len)) < 0) {
        i_errno = IEACCEPT;
        return -1;
    }
#ifdef SO_RCVTIMEO
    /* 30 sec timeout for control socket for param exchange. */
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0) {
        iperf_printf(test, "Unable to set socket SO_RCVTIMEO");
	}
#endif

    //store remote address
    sa = (struct sockaddr *)&addr;
    test->remote_addr = *sa;
#else

    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IEACCEPT;
        return -1;
    }
#endif
    if (test->ctrl_sck == -1) {
        /* Server free, accept new client */
        test->ctrl_sck = s;
        if (Nread(test->ctrl_sck, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return -1;
        }
	FD_SET(test->ctrl_sck, &test->read_set);
	if (test->ctrl_sck > test->max_fd) test->max_fd = test->ctrl_sck;

	if (iperf_set_send_state(test, PARAM_EXCHANGE) != 0)
            return -1;
        if (iperf_exchange_parameters(test) < 0)
            return -1;
	if (test->server_affinity != -1) 
	    if (iperf_setaffinity(test, test->server_affinity) != 0)
		return -1;
        if (test->on_connect)
            test->on_connect(test);
    } else {
	/*
	 * Don't try to read from the socket.  It could block an ongoing test. 
	 * Just send ACCESS_DENIED.
	 */
        if (Nwrite(s, (char*) &rbuf, sizeof(rbuf), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return -1;
        }
        close(s);
    }
#ifdef SO_RCVTIMEO
    /* Disable timeout. */
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0) {
        iperf_printf(test, "Unable to disable socket SO_RCVTIMEO");
	}
#endif
    return 0;
}


/**************************************************************************/
int
iperf_handle_message_server(struct iperf_test *test)
{
    int rval;
    struct iperf_stream *sp;

    // XXX: Need to rethink how this behaves to fit API
    if ((rval = Nread(test->ctrl_sck, (char*) &test->state, sizeof(signed char), Ptcp)) <= 0) {
        if (rval == 0) {
	    iperf_err(test, "the client has unexpectedly closed the connection");
            i_errno = IECTRLCLOSE;
            test->state = IPERF_DONE;
            return 0;
        } else {
            i_errno = IERECVMESSAGE;
            return -1;
        }
    }

    switch(test->state) {
        case TEST_START:
            break;
        case TEST_END:
	    test->done = 1;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)        
            cpu_util(test->cpu_util);
#endif
            test->stats_callback(test);
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->reporter_callback(test);
	    if (iperf_set_send_state(test, EXCHANGE_RESULTS) != 0)
                return -1;
            if (iperf_exchange_results(test) < 0)
                return -1;
	    if (iperf_set_send_state(test, DISPLAY_RESULTS) != 0)
                return -1;
            if (test->on_test_finish)
                test->on_test_finish(test);
            break;
        case IPERF_DONE:
            break;
        case CLIENT_TERMINATE:
            i_errno = IECLIENTTERM;

	    // Temporarily be in DISPLAY_RESULTS phase so we can get
	    // ending summary statistics.
	    signed char oldstate = test->state;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)        
	    cpu_util(test->cpu_util);
#endif        
	    test->state = DISPLAY_RESULTS;
	    test->reporter_callback(test);
	    test->state = oldstate;

            // XXX: Remove this line below!
	    iperf_err(test, "the client has terminated");
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = IPERF_DONE;
            break;
        default:
            i_errno = IEMESSAGE;
            return -1;
    }

    return 0;
}

static void
server_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;
    struct iperf_stream *sp;

    test->timer = NULL;
    if (test->done)
        return;
    test->done = 1;
    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        close(sp->socket);
        iperf_free_stream(sp);
    }
    close(test->ctrl_sck);
}

static void
server_stats_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->stats_callback)
	test->stats_callback(test);
}

static void
server_reporter_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->reporter_callback)
	test->reporter_callback(test);
}

static int
create_server_timers(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd;
    int max_rtt = 4; /* seconds */
    int state_transitions = 10; /* number of state transitions in iperf3 */
    int grace_period = max_rtt * state_transitions;

    if (iperf_time_now(&now) < 0) {
	i_errno = IEINITTEST;
	return -1;
    }
    cd.p = test;
    test->timer = test->stats_timer = test->reporter_timer = NULL;
    if (test->duration != 0 ) {
        test->done = 0;
        test->timer = tmr_create(&now, server_timer_proc, cd, (test->duration + test->omit + grace_period) * SEC_TO_US, 0);
        if (test->timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
        }
    }

    test->stats_timer = test->reporter_timer = NULL;
    if (test->stats_interval != 0) {
        test->stats_timer = tmr_create(&now, server_stats_timer_proc, cd, test->stats_interval * SEC_TO_US, 1);
        if (test->stats_timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
	}
    }
    if (test->reporter_interval != 0) {
        test->reporter_timer = tmr_create(&now, server_reporter_timer_proc, cd, test->reporter_interval * SEC_TO_US, 1);
        if (test->reporter_timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
	}
    }
    return 0;
}

static void
server_omit_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{   
    struct iperf_test *test = client_data.p;

    test->omit_timer = NULL;
    test->omitting = 0;
    iperf_reset_stats(test);
    if (test->verbose && !test->json_output && test->reporter_interval == 0)
	iperf_printf(test, "%s", report_omit_done);

    /* Reset the timers. */
    if (test->stats_timer != NULL)
	tmr_reset(nowP, test->stats_timer);
    if (test->reporter_timer != NULL)
	tmr_reset(nowP, test->reporter_timer);
}

static int
create_server_omit_timer(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd; 

    if (test->omit == 0) {
	test->omit_timer = NULL;
	test->omitting = 0;
    } else {
	if (iperf_time_now(&now) < 0) {
	    i_errno = IEINITTEST;
	    return -1; 
	}
	test->omitting = 1;
	cd.p = test;
	test->omit_timer = tmr_create(&now, server_omit_timer_proc, cd, test->omit * SEC_TO_US, 0); 
	if (test->omit_timer == NULL) {
	    i_errno = IEINITTEST;
	    return -1;
	}
    }

    return 0;
}

static void
cleanup_server(struct iperf_test *test)
{
    struct iperf_stream *sp;

    /* Close open streams */
    SLIST_FOREACH(sp, &test->streams, streams) {
	FD_CLR(sp->socket, &test->read_set);
	FD_CLR(sp->socket, &test->write_set);
	close(sp->socket);
    }

    /* Close open test sockets */
    if (test->ctrl_sck) {
	close(test->ctrl_sck);
    }
    if (test->listener) {
	close(test->listener);
    }

    /* Cancel any remaining timers. */
    if (test->stats_timer != NULL) {
	tmr_cancel(test->stats_timer);
	test->stats_timer = NULL;
    }
    if (test->reporter_timer != NULL) {
	tmr_cancel(test->reporter_timer);
	test->reporter_timer = NULL;
    }
    if (test->omit_timer != NULL) {
	tmr_cancel(test->omit_timer);
	test->omit_timer = NULL;
    }
    if (test->congestion_used != NULL) {
        free(test->congestion_used);
	test->congestion_used = NULL;
    }
    if (test->timer != NULL) {
        tmr_cancel(test->timer);
        test->timer = NULL;
    }
}


int
iperf_run_server(struct iperf_test *test)
{
    int result, s;
    int send_streams_accepted, rec_streams_accepted;
    int streams_to_send = 0, streams_to_rec = 0;
#if defined(HAVE_TCP_CONGESTION)
    int saved_errno;
#endif /* HAVE_TCP_CONGESTION */
    fd_set read_set, write_set;
    struct iperf_stream *sp;
    struct iperf_time now;
    struct timeval* timeout;
    
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    int flag = -1;
#else
    int flag;
#endif

    if (test->logfile)
        if (iperf_open_logfile(test) < 0)
            return -1;

    if (test->affinity != -1) 
	if (iperf_setaffinity(test, test->affinity) != 0)
	    return -2;

    if (test->json_output)
	if (iperf_json_start(test) < 0)
	    return -2;

    if (test->json_output) {
	cJSON_AddItemToObject(test->json_start, "version", cJSON_CreateString(version));
	cJSON_AddItemToObject(test->json_start, "system_info", cJSON_CreateString(get_system_info()));
    } else if (test->verbose) {
	iperf_printf(test, "%s\n", version);
	iperf_printf(test, "%s", "");
	iperf_printf(test, "%s\n", get_system_info());
	iflush(test);
    }

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        return -2;
    }

    // Begin calculating CPU utilization
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    cpu_util(NULL);
#endif
    test->state = IPERF_START;
    send_streams_accepted = 0;
    rec_streams_accepted = 0;

    while (test->state != IPERF_DONE) {
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
        if (test->kill_signal != NULL) {
            int set, res;

            k_poll_signal_check(test->kill_signal, &set, &res);
            if (set) {
                k_poll_signal_reset(test->kill_signal);
                iperf_printf(test, "Kill signal received - exiting\n");
                cleanup_server(test);
                i_errno = IEKILL;
                return -2;
            }
        }
#endif

        // Check if average transfer rate was exceeded (condition set in the callback routines)
	if (test->bitrate_limit_exceeded) {
	    cleanup_server(test);
            i_errno = IETOTALRATE;
            return -1;	
	}

        memcpy(&read_set, &test->read_set, sizeof(fd_set));
        memcpy(&write_set, &test->write_set, sizeof(fd_set));

	iperf_time_now(&now);
	timeout = tmr_timeout(&now);
        result = select(test->max_fd + 1, &read_set, &write_set, NULL, timeout);

        if (result < 0 && errno != EINTR) {
	    cleanup_server(test);
            i_errno = IESELECT;
            return -1;
        }
	if (result > 0) {
            if (FD_ISSET(test->listener, &read_set)) {
                if (test->state != CREATE_STREAMS) {
                    if (iperf_accept(test) < 0) {
			cleanup_server(test);
                        return -1;
                    }
                    FD_CLR(test->listener, &read_set);

                    // Set streams number
                    if (test->mode == BIDIRECTIONAL) {
                        streams_to_send = test->num_streams;
                        streams_to_rec = test->num_streams;
                    } else if (test->mode == RECEIVER) {
                        streams_to_rec = test->num_streams;
                        streams_to_send = 0;
                    } else {
                        streams_to_send = test->num_streams;
                        streams_to_rec = 0;
                    }
                }
            }
            if (FD_ISSET(test->ctrl_sck, &read_set)) {
                if (iperf_handle_message_server(test) < 0) {
		    cleanup_server(test);
                    return -1;
		}
                FD_CLR(test->ctrl_sck, &read_set);                
            }

            if (test->state == CREATE_STREAMS) {
                if (FD_ISSET(test->prot_listener, &read_set)) {
    
                    if ((s = test->protocol->accept(test)) < 0) {
			cleanup_server(test);
                        return -1;
		    }

#if defined(HAVE_TCP_CONGESTION)
		    if (test->protocol->id == Ptcp) {
			if (test->congestion) {
			    if (setsockopt(s, IPPROTO_TCP, TCP_CONGESTION, test->congestion, strlen(test->congestion)) < 0) {
				/*
				 * ENOENT means we tried to set the
				 * congestion algorithm but the algorithm
				 * specified doesn't exist.  This can happen
				 * if the client and server have different
				 * congestion algorithms available.  In this
				 * case, print a warning, but otherwise
				 * continue.
				 */
				if (errno == ENOENT) {
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
				    warning(test, "TCP congestion control algorithm not supported");
#endif
				}
				else {
				    saved_errno = errno;
				    close(s);
				    cleanup_server(test);
				    errno = saved_errno;
				    i_errno = IESETCONGESTION;
				    return -1;
				}
			    } 
			}
			{
			    socklen_t len = TCP_CA_NAME_MAX;
			    char ca[TCP_CA_NAME_MAX + 1];
			    if (getsockopt(s, IPPROTO_TCP, TCP_CONGESTION, ca, &len) < 0) {
				saved_errno = errno;
				close(s);
				cleanup_server(test);
				errno = saved_errno;
				i_errno = IESETCONGESTION;
				return -1;
			    }
			    test->congestion_used = strdup(ca);
			    if (test->debug) {
				iperf_printf(test, "Congestion algorithm is %s\n", test->congestion_used);
			    }
			}
		    }
#endif /* HAVE_TCP_CONGESTION */

                    if (!is_closed(s)) {

                        if (rec_streams_accepted != streams_to_rec) {
                            flag = 0;
                            ++rec_streams_accepted;
                        } else if (send_streams_accepted != streams_to_send) {
                            flag = 1;
                            ++send_streams_accepted;
                        }

                        if (flag != -1) {
                            sp = iperf_new_stream(test, s, flag);
                            if (!sp) {
                                cleanup_server(test);
                                return -1;
                            }

                            if (sp->sender)
                                FD_SET(s, &test->write_set);
                            else
                                FD_SET(s, &test->read_set);

                            if (s > test->max_fd) test->max_fd = s;

                            /*
                             * If the protocol isn't UDP, or even if it is but
                             * we're the receiver, set nonblocking sockets.
                             * We need this to allow a server receiver to
                             * maintain interactivity with the control channel.
                             */
                            if (test->protocol->id != Pudp ||
                                !sp->sender) {
                                setnonblocking(s, 1);
                            }

                            if (test->on_new_stream)
                                test->on_new_stream(sp);

                            flag = -1;
                        }
                    }
                    FD_CLR(test->prot_listener, &read_set);
                }


                if (rec_streams_accepted == streams_to_rec && send_streams_accepted == streams_to_send) {
                    if (test->protocol->id != Ptcp) {
                        FD_CLR(test->prot_listener, &test->read_set);
                        close(test->prot_listener);
                    } else { 
                        if (test->no_delay || test->settings->mss || test->settings->socket_bufsize) {
                            FD_CLR(test->listener, &test->read_set);
                            close(test->listener);
			    test->listener = 0;
                            if ((s = netannounce(test, test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
				cleanup_server(test);
                                i_errno = IELISTEN;
                                return -1;
                            }
                            test->listener = s;
                            FD_SET(test->listener, &test->read_set);
			    if (test->listener > test->max_fd) test->max_fd = test->listener;
                        }
                    }
                    test->prot_listener = -1;

		    /* Ensure that total requested data rate is not above limit */
		    iperf_size_t total_requested_rate = test->num_streams * test->settings->rate * (test->mode == BIDIRECTIONAL? 2 : 1);
		    if (test->settings->bitrate_limit > 0 && total_requested_rate > test->settings->bitrate_limit) {
			iperf_err(test, "Client total requested throughput rate of %" PRIu64 " bps exceeded %" PRIu64 " bps limit",
				total_requested_rate, test->settings->bitrate_limit);
			cleanup_server(test);
			i_errno = IETOTALRATE;
			return -1;
		    }

		    if (iperf_set_send_state(test, TEST_START) != 0) {
			cleanup_server(test);
                        return -1;
		    }
                    if (iperf_init_test(test) < 0) {
			cleanup_server(test);
                        return -1;
		    }
		    if (create_server_timers(test) < 0) {
			cleanup_server(test);
                        return -1;
		    }
		    if (create_server_omit_timer(test) < 0) {
			cleanup_server(test);
                        return -1;
		    }
		    if (test->mode != RECEIVER)
			if (iperf_create_send_timers(test) < 0) {
			    cleanup_server(test);
			    return -1;
			}
		    if (iperf_set_send_state(test, TEST_RUNNING) != 0) {
			cleanup_server(test);
                        return -1;
		    }
                }
            }

            if (test->state == TEST_RUNNING) {
                if (test->mode == BIDIRECTIONAL) {
                    if (iperf_recv(test, &read_set) < 0) {
                        cleanup_server(test);
                        return -1;
                    }
                    if (iperf_send(test, &write_set) < 0) {
                        cleanup_server(test);
                        return -1;
                    }
                } else if (test->mode == SENDER) {
                    // Reverse mode. Server sends.
                    if (iperf_send(test, &write_set) < 0) {
			cleanup_server(test);
                        return -1;
		    }
                } else {
                    // Regular mode. Server receives.
                    if (iperf_recv(test, &read_set) < 0) {
			cleanup_server(test);
                        return -1;
		    }
                }
	    }
        }

	if (result == 0 ||
	    (timeout != NULL && timeout->tv_sec == 0 && timeout->tv_usec == 0)) {
	    /* Run the timers. */
	    iperf_time_now(&now);
	    tmr_run(&now);
	}
    }

    cleanup_server(test);

    if (test->json_output) {
	if (iperf_json_finish(test) < 0)
	    return -1;
    } 

    iflush(test);

    if (test->server_affinity != -1) 
	if (iperf_clearaffinity(test) != 0)
	    return -1;

    return 0;
}
