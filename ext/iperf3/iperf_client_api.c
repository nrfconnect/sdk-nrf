/*
 * iperf, Copyright (c) 2014-2020, The Regents of the University of
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
 
#include <errno.h>
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
#include <setjmp.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
#include <sys/uio.h>
#endif
#include <arpa/inet.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_locale.h"
#include "iperf_time.h"
#include "net.h"
#include "timer.h"

#if defined(HAVE_TCP_CONGESTION)
#if !defined(TCP_CA_NAME_MAX)
#define TCP_CA_NAME_MAX 16
#endif /* TCP_CA_NAME_MAX */
#endif /* HAVE_TCP_CONGESTION */

int
iperf_create_streams(struct iperf_test *test, int sender)
{
    int i, s;
#if defined(HAVE_TCP_CONGESTION)
    int saved_errno;
#endif /* HAVE_TCP_CONGESTION */
    struct iperf_stream *sp;

    int orig_bind_port = test->bind_port;
    for (i = 0; i < test->num_streams; ++i) {

        test->bind_port = orig_bind_port;
	if (orig_bind_port)
	    test->bind_port += i;
        if ((s = test->protocol->connect(test)) < 0)
            return -1;

#if defined(HAVE_TCP_CONGESTION)
	if (test->protocol->id == Ptcp) {
	    if (test->congestion) {
		if (setsockopt(s, IPPROTO_TCP, TCP_CONGESTION, test->congestion, strlen(test->congestion)) < 0) {
		    saved_errno = errno;
		    close(s);
		    errno = saved_errno;
		    test->i_errno = IESETCONGESTION;
		    return -1;
		} 
	    }
	    {
		socklen_t len = TCP_CA_NAME_MAX;
		char ca[TCP_CA_NAME_MAX + 1];
		if (getsockopt(s, IPPROTO_TCP, TCP_CONGESTION, ca, &len) < 0) {
		    saved_errno = errno;
		    close(s);
		    errno = saved_errno;
		    test->i_errno = IESETCONGESTION;
		    return -1;
		}
		test->congestion_used = strdup(ca);
		if (test->debug) {
		    iperf_printf(test, "Congestion algorithm is %s\n", test->congestion_used);
		}
	    }
	}
#endif /* HAVE_TCP_CONGESTION */

	if (sender)
	    FD_SET(s, &test->write_set);
	else
	    FD_SET(s, &test->read_set);

    FD_SET(s, &test->err_set);
    if (s > test->max_fd) test->max_fd = s;

        sp = iperf_new_stream(test, s, sender);
        if (!sp)
            return -1;

        /* Perform the new stream callback */
        if (test->on_new_stream)
            test->on_new_stream(sp);
    }

    return 0;
}

static void
test_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    test->timer = NULL;
    test->done = 1;

    iperf_printf(test, "%s", report_bw_separator);
    iperf_printf(test,
		 "Test duration %d seconds elapsed - testing done. Exchanging the results next.\n",
		    test->duration);
    iperf_printf(test, "%s", report_bw_separator);
}

static void
client_stats_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->stats_callback)
	test->stats_callback(test);
}

static void
client_reporter_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->reporter_callback)
	test->reporter_callback(test);
}

static int
create_client_timers(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd;

    if (iperf_time_now(&now) < 0) {
	test->i_errno = IEINITTEST;
	return -1;
    }
    cd.p = test;
    test->timer = test->stats_timer = test->reporter_timer = NULL;
    if (test->duration != 0) {
	test->done = 0;
        test->timer = tmr_create(&now, test_timer_proc, cd, ( test->duration + test->omit ) * SEC_TO_US, 0);
        if (test->timer == NULL) {
            test->i_errno = IEINITTEST;
            return -1;
	}
    } 
    if (test->stats_interval != 0) {
        test->stats_timer = tmr_create(&now, client_stats_timer_proc, cd, test->stats_interval * SEC_TO_US, 1);
        if (test->stats_timer == NULL) {
            test->i_errno = IEINITTEST;
            return -1;
	}
    }
    if (test->reporter_interval != 0) {
        test->reporter_timer = tmr_create(&now, client_reporter_timer_proc, cd, test->reporter_interval * SEC_TO_US, 1);
        if (test->reporter_timer == NULL) {
            test->i_errno = IEINITTEST;
            return -1;
	}
    }
    return 0;
}

static void
client_omit_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
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
create_client_omit_timer(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd;

    if (test->omit == 0) {
	test->omit_timer = NULL;
        test->omitting = 0;
    } else {
	if (iperf_time_now(&now) < 0) {
	    test->i_errno = IEINITTEST;
	    return -1;
	}
	test->omitting = 1;
	cd.p = test;
	test->omit_timer = tmr_create(&now, client_omit_timer_proc, cd, test->omit * SEC_TO_US, 0);
	if (test->omit_timer == NULL) {
	    test->i_errno = IEINITTEST;
	    return -1;
	}
    }
    return 0;
}

int
iperf_handle_message_client(struct iperf_test *test)
{
    int rval;
    int32_t err;
#if defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
    struct iperf_stream *sp;
#endif

    /*!!! Why is this read() and not Nread()? */
    if ((rval = read(test->ctrl_sck, (char*) &test->state, sizeof(signed char))) <= 0) {
        if (rval == 0) {
            test->i_errno = IECTRLCLOSE;
            return -1;
        } else {
            test->i_errno = IERECVMESSAGE;
            return -1;
        }
    }

    switch (test->state) {
        case PARAM_EXCHANGE:
            if (iperf_exchange_parameters(test) < 0)
                return -1;
            if (test->on_connect)
                test->on_connect(test);
            break;
        case CREATE_STREAMS:
            if (test->mode == BIDIRECTIONAL)
            {
                if (iperf_create_streams(test, 1) < 0)
                    return -1;
                if (iperf_create_streams(test, 0) < 0)
                    return -1;
            }
            else if (iperf_create_streams(test, test->mode) < 0)
                return -1;
#if defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
            if (test->protocol->id != Pudp) {
                SLIST_FOREACH(sp, &test->streams, streams) {
                setnonblocking(sp->socket, 1);
                }
            }

            int i = 0;
            SLIST_FOREACH(sp, &test->streams, streams) {
                iperf_printf(test, "stream [%d]: socket: %d read: %d write: %d\n",
                        i,
                        sp->socket,
                        (int)FD_ISSET(sp->socket, &test->read_set),
                        (int)FD_ISSET(sp->socket, &test->write_set));
                i++;
            }
#endif
            break;
        case TEST_START:
#if defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
            if (test->debug) {
                iperf_printf(test, "TEST_START: non-blocking ctrl sckt %d\n", test->ctrl_sck);
            }
            setnonblocking(test->ctrl_sck, 1);
#endif
            if (iperf_init_test(test) < 0)
                return -1;
            if (create_client_timers(test) < 0)
                return -1;
            if (create_client_omit_timer(test) < 0)
                return -1;
	    if (test->mode)
		if (iperf_create_send_timers(test) < 0)
		    return -1;
            break;
        case TEST_RUNNING:
            break;
        case EXCHANGE_RESULTS:
            if (test->debug) {
                iperf_printf(
                    test,
                    "Starting to EXCHANGE_RESULTS, ctrl sckt %d\n",
                    test->ctrl_sck);
            }
            if (iperf_exchange_results(test) < 0)
                return -1;
            break;
        case DISPLAY_RESULTS:
            if (test->on_test_finish)
                test->on_test_finish(test);
            iperf_client_end(test);
            break;
        case IPERF_DONE:
            break;
        case SERVER_TERMINATE:
            test->i_errno = IESERVERTERM;

	    /*
	     * Temporarily be in DISPLAY_RESULTS phase so we can get
	     * ending summary statistics.
	     */
	    signed char oldstate = test->state;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)        
	    cpu_util(test->cpu_util);
#endif
	    test->state = DISPLAY_RESULTS;
	    test->reporter_callback(test);
	    test->state = oldstate;
            return -1;
        case ACCESS_DENIED:
            test->i_errno = IEACCESSDENIED;
            return -1;
        case SERVER_ERROR:
            if (Nread(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                test->i_errno = IECTRLREAD;
                return -1;
            }
	    test->i_errno = ntohl(err);
            if (Nread(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                test->i_errno = IECTRLREAD;
                return -1;
            }
            errno = ntohl(err);
            return -1;
        default:
            test->i_errno = IEMESSAGE;
            return -1;
    }

    return 0;
}



/* iperf_connect -- client to server connection function */
int
iperf_connect(struct iperf_test *test)
{
    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);

    make_cookie(test->cookie);

    /* Create and connect the control channel */
    if (test->ctrl_sck < 0)
	// Create the control channel using an ephemeral port
	test->ctrl_sck = netdial(test, test->settings->domain, Ptcp, test->bind_address, 0, test->server_hostname, test->server_port, test->settings->connect_timeout);
    if (test->ctrl_sck < 0) {
        test->i_errno = IECONNECT;
        return -1;
    }

    if (Nwrite(test->ctrl_sck, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
        test->i_errno = IESENDCOOKIE;
        return -1;
    }

    FD_SET(test->ctrl_sck, &test->read_set);
    if (test->ctrl_sck > test->max_fd) test->max_fd = test->ctrl_sck;

    int opt;
    socklen_t len;

    len = sizeof(opt);
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
    if (getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len) < 0) {
#endif        
        test->ctrl_sck_mss = 0;
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    }
    else {
        if (opt > 0 && opt <= MAX_UDP_BLOCKSIZE) {
            test->ctrl_sck_mss = opt;
        }
        else {
            char str[128];
            snprintf(str, sizeof(str),
                     "Ignoring nonsense TCP MSS %d", opt);
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
            warning(test, str);
#endif

            test->ctrl_sck_mss = 0;
        }
    }
#endif        

    if (test->verbose) {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)        
	printf("Control connection MSS %d\n", test->ctrl_sck_mss);
#else
    /* no support to set nor read the mss */
	iperf_printf(test, "Control connection MSS: using modem default\n");
#endif
    }

    /*
     * If we're doing a UDP test and the block size wasn't explicitly
     * set, then use the known MSS of the control connection to pick
     * an appropriate default.  If we weren't able to get the
     * MSS for some reason, then default to something that should
     * work on non-jumbo-frame Ethernet networks.  The goal is to
     * pick a reasonable default that is large but should get from
     * sender to receiver without any IP fragmentation.
     *
     * We assume that the control connection is routed the same as the
     * data packets (thus has the same PMTU).  Also in the case of
     * --reverse tests, we assume that the MTU is the same in both
     * directions.  Note that even if the algorithm guesses wrong,
     * the user always has the option to override.
     */
    if (test->protocol->id == Pudp) {
	if (test->settings->blksize == 0) {
	    if (test->ctrl_sck_mss) {
		test->settings->blksize = test->ctrl_sck_mss;
	    }
	    else {
		test->settings->blksize = DEFAULT_UDP_BLKSIZE;
	    }
	    if (test->verbose) {
		iperf_printf(test, "Setting UDP block size to %d\n", test->settings->blksize);
	    }
	}

	/*
	 * Regardless of whether explicitly or implicitly set, if the
	 * block size is larger than the MSS, print a warning.
	 */
	if (test->ctrl_sck_mss > 0 &&
	    test->settings->blksize > test->ctrl_sck_mss) {
	    char str[128];
	    snprintf(str, sizeof(str),
		     "UDP block size %d exceeds TCP MSS %d, may result in fragmentation / drops", test->settings->blksize, test->ctrl_sck_mss);
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
	    warning(test, str);
#endif
	}
    }

    return 0;
}


int
iperf_client_end(struct iperf_test *test)
{
    struct iperf_stream *sp;
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)    
    int retval = 0; /* closing control socket when DONE failed */
#endif

    iperf_printf(test, "iperf3 testing is ending.\n");

    /* Close all stream sockets */
    SLIST_FOREACH(sp, &test->streams, streams) {
        close(sp->socket);
    }

    /* show final summary */
    test->reporter_callback(test);

    if (iperf_set_send_state(test, IPERF_DONE) != 0) {
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
        iperf_printf(test, "iperf_client_end: iperf_set_send_state failed\n");
        retval = -1;
#else
        return -1;
#endif
    }

    /* Close control socket */
    if (test->ctrl_sck) {
        close(test->ctrl_sck);
    }

    return retval;
}


int
iperf_run_client(struct iperf_test * test)
{
    int startup;
    int result = 0;
    fd_set read_set, write_set, err_set;
    struct iperf_time now;
    struct timeval* timeout = NULL;
#if !defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
    struct iperf_stream *sp;
#endif
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    struct iperf_time test_end_started_time;
    struct iperf_time connected_time;
    /* wait testing start for max xx sec */
    struct timeval test_start_tout = { .tv_sec = CONFIG_NRF_IPERF3_CLIENT_TEST_START_TIME, .tv_usec = 0 };    
    struct timeval test_end_tout = { .tv_sec = CONFIG_NRF_IPERF3_CLIENT_TEST_ENDING_TIMEOUT, .tv_usec = 0 };
#endif

    if (test->logfile)
        if (iperf_open_logfile(test) < 0)
            return -1;

    if (test->affinity != -1)
	if (iperf_setaffinity(test, test->affinity) != 0)
	    return -1;

    if (test->json_output)
	if (iperf_json_start(test) < 0)
	    return -1;

    if (test->json_output) {
	cJSON_AddItemToObject(test->json_start, "version", cJSON_CreateString(version));
	cJSON_AddItemToObject(test->json_start, "system_info", cJSON_CreateString(get_system_info()));
    } else if (test->verbose) {
	iperf_printf(test, "%s\n", version);
	iperf_printf(test, "%s\n", get_system_info());
	iflush(test);
    }

    /* Start the client and connect to the server */
    if (iperf_connect(test) < 0) {
        goto cleanup_and_fail;
    }
    else {
        /* save time when we got connected */
		iperf_time_now(&connected_time);
        if (test->debug) {
            iperf_printf(test, "iperf_run_client: ctrl socket connected: %d, state %d\n", test->ctrl_sck, test->state);
        }
    }

    /* Begin calculating CPU utilization */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    cpu_util(NULL);
#endif

    startup = 1;
    while (test->state != IPERF_DONE) {
    memcpy(&read_set, &test->read_set, sizeof(fd_set));
    memcpy(&write_set, &test->write_set, sizeof(fd_set));
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    memcpy(&err_set, &test->err_set, sizeof(fd_set));
    if (test->state == TEST_END || test->state == EXCHANGE_RESULTS ||
	    test->state == DISPLAY_RESULTS || test->state == IPERF_DONE) {
	    struct iperf_time now;
	    struct iperf_time temp_time;

	    /* We don't want to hang in TEST_END & later states more than a configured time. */
	    iperf_time_now(&now);
	    iperf_time_diff(&test_end_started_time, &now, &temp_time);

	    if (iperf_time_in_secs(&temp_time) > test_end_tout.tv_sec) {
		    test->i_errno = IETESTENDTIMEOUT;
		    iperf_printf(test,
				 "iperf_run_client (1): timeout to wait to test end, config "
				 "timeout value %d secs\n",
				 (uint32_t)test_end_tout.tv_sec);
		    goto cleanup_and_fail;
	    }
    }

    if (test->kill_signal != NULL) {
	    int set, res;
	    k_poll_signal_check(test->kill_signal, &set, &res);
	    if (set) {
		    k_poll_signal_reset(test->kill_signal);
		    iperf_printf(test, "Kill signal received (state %d) - exiting\n", test->state);
		    test->i_errno = IEKILL;
		    goto cleanup_and_fail;
	    }
    }
#endif

    iperf_time_now(&now);
    timeout = tmr_timeout(&now);
	result = select(test->max_fd + 1, &read_set, &write_set, &err_set, timeout);
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    if (!test->state) {
	    /* ctrl socket connected but test not yet started: */
	    struct iperf_time now;
	    struct iperf_time temp_time;

	    iperf_time_now(&now);
	    iperf_time_diff(&connected_time, &now, &temp_time);
	    if (iperf_time_in_secs(&temp_time) > test_start_tout.tv_sec) {
		    test->i_errno = IETESTSTARTTIMEOUT;
		    if (test->debug) {
			    iperf_printf(test,
					 "iperf_run_client: timeout to wait to actual test start, "
					 "config timeout value %d secs\n",
					 (uint32_t)test_start_tout.tv_sec);
		    }
		    goto cleanup_and_fail;
	    }
    }
#endif
	if (result < 0 && errno != EINTR) {
        test->i_errno = IESELECT;
        if (test->debug) {
            iperf_printf(test, "iperf_run_client: select failed: %d\n", result);
        }
	    goto cleanup_and_fail;
	}

	if (result > 0) {
	    if (FD_ISSET(test->ctrl_sck, &read_set)) {
 	        if (iperf_handle_message_client(test) < 0) {
                if (test->debug) {
                    iperf_printf(test, "iperf_run_client: iperf_handle_message_client failed\n");
                }
		    goto cleanup_and_fail;
		    }          
		FD_CLR(test->ctrl_sck, &read_set);
	    }

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
	    /* Handle failures */
	    if (FD_ISSET(test->ctrl_sck, &err_set)) {
		    iperf_printf(test,
				 "client: select(): control socket (%d) is having error condition "
				 "(state %d).\n",
				 test->ctrl_sck, test->state);
		    test->i_errno = IESELECTERRORFDS;
		    goto cleanup_and_fail;
	    }
#endif
	}

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)//added due to early test jamn where rx buffer was full between modem and app
	if (test->state == TEST_START ||
        test->state == PARAM_EXCHANGE ||
        test->state == CREATE_STREAMS /* ||
      test->state == SERVER_TERMINATE ||
        test->state == CLIENT_TERMINATE || */) {
		if (iperf_recv(test, &read_set) < 0) {
			goto cleanup_and_fail;
		}
	} else if (test->state == EXCHANGE_RESULTS || test->state == DISPLAY_RESULTS ||
		   test->state == IPERF_DONE) {
		int retval = iperf_recv(test, &read_set);
		if (retval < 0) {
			/* let's ignore errors from stream/testing sockets */
			if (test->debug) {
				iperf_printf(test,
					     "iperf_recv for stream socket in EXCHANGE_RESULTS "
					     "failed %d but ignored\n",
					     retval);
			}
		}
	} else
#endif
		if (test->state == TEST_RUNNING) {

	    /* Is this our first time really running? */
	    if (startup) {
	        startup = 0;

#if !defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
		// Set non-blocking for non-UDP tests
		if (test->protocol->id != Pudp) {
		    SLIST_FOREACH(sp, &test->streams, streams) {
			setnonblocking(sp->socket, 1);
		    }
		}
#endif
	    }

	    if (test->mode == BIDIRECTIONAL)
	    {
                if (iperf_send(test, &write_set) < 0) {
                    if (test->debug) {
                        iperf_printf(test, "iperf_run_client: BIDIRECTIONAL iperf_send failed\n");
                    }                    
                    goto cleanup_and_fail;
                }
                if (iperf_recv(test, &read_set) < 0) {
                    if (test->debug) {
                        iperf_printf(test, "iperf_run_client: BIDIRECTIONAL iperf_recv failed\n");
                    }                    
                    goto cleanup_and_fail;
                }
	    } else if (test->mode == SENDER) {
                // Regular mode. Client sends.
                if (iperf_send(test, &write_set) < 0) {
                    if (test->debug) {
                        iperf_printf(test, "iperf_run_client: SENDER iperf_send failed\n");
                    }                    
                    goto cleanup_and_fail;
                }
	    } else {
                // Reverse mode. Client receives.
                if (iperf_recv(test, &read_set) < 0) {
                    if (test->debug) {
                        iperf_printf(test, "iperf_run_client: REVERSE mode iperf_recv failed\n");
                    }                    
                    goto cleanup_and_fail;
                }
	    }

        /* Run the timers. */
        iperf_time_now(&now);
        tmr_run(&now);

	    /* Is the test done yet? */
	    if ((!test->omitting) &&
	        ((test->duration != 0 && test->done) ||
	         (test->settings->bytes != 0 && test->bytes_sent >= test->settings->bytes) ||
	         (test->settings->blocks != 0 && test->blocks_sent >= test->settings->blocks))) {

#if !defined (CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
		// Unset non-blocking for non-UDP tests
		if (test->protocol->id != Pudp) {
		    SLIST_FOREACH(sp, &test->streams, streams) {
			setnonblocking(sp->socket, 0);
		    }
		}
#endif
		/* Yes, done!  Send TEST_END. */
		test->done = 1;
        if (test->debug) {
            iperf_printf(test, "iperf_run_client: Yes, testing done! Send TEST_END\n");
        }
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)        
		cpu_util(test->cpu_util);
#endif
		test->stats_callback(test);

		if (iperf_set_send_state(test, TEST_END) != 0) {
            if (test->debug) {
                iperf_printf(test, "iperf_run_client: TEST_END SENDING FAILED\n");
            }               
            goto cleanup_and_fail;
        }
#if defined(CONFIG_NRF_IPERF3_NONBLOCKING_CLIENT_CHANGES)
        if (true) {
            struct iperf_stream *spp;

            /* Read one more time before closing */
            (void)iperf_recv(test, &read_set);

            /* Eliminate all possible data transfer from modem by closing the socket */
            iperf_printf(test, "Testing done: closing data sockets.\n");

            /* Close all data sockets */
            SLIST_FOREACH(spp, &test->streams, streams)
            {
                close(spp->socket);
                FD_CLR(spp->socket, &test->read_set);
                FD_CLR(spp->socket, &test->write_set);
                FD_CLR(spp->socket, &test->err_set);
                FD_CLR(spp->socket, &read_set);
                FD_CLR(spp->socket, &write_set);
                FD_CLR(spp->socket, &err_set);
            }
        }
#endif

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
		iperf_time_now(&test_end_started_time);
#endif
	}
	} /* test->state == TEST_RUNNING */
	// If we're in reverse or bidirectional mode, continue draining the data
	// connection(s) even if test is over.  This prevents a
	// deadlock where the server side fills up its pipe(s)
	// and gets blocked, so it can't receive state changes
	// from the client side.
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
	else if (test->state == TEST_END) {
		struct iperf_time now;
		struct iperf_time temp_time;

		if (test->mode == BIDIRECTIONAL || test->mode == RECEIVER) {
			if (iperf_recv(test, &read_set) < 0) {
				if (test->debug) {
					iperf_printf(test,
						     "iperf_run_client: RECEIVER/BIDIRECTIONAL "
						     "TEST_END iperf_recv failed\n");
				}
				goto cleanup_and_fail;
			}
		}

		/* We don't want to hang in TEST_END more than a configured time*/
		iperf_time_now(&now);
		iperf_time_diff(&test_end_started_time, &now, &temp_time);
		if (iperf_time_in_secs(&temp_time) > test_end_tout.tv_sec) {
			test->i_errno = IETESTENDTIMEOUT;
			iperf_printf(test,
				     "iperf_run_client (2): timeout to wait to test end, config "
				     "timeout value %d secs\n",
				     (uint32_t)test_end_tout.tv_sec);
			goto cleanup_and_fail;
		}
	}
#else
    else if (test->mode == RECEIVER && test->state == TEST_END) {
            if (iperf_recv(test, &read_set) < 0) {
                if (test->debug) {
                    iperf_printf(test, "iperf_run_client: RECEIVER TEST_END iperf_recv failed\n");
                }
                goto cleanup_and_fail;
            }
        }
#endif
        }

    if (test->json_output) {
	if (iperf_json_finish(test) < 0)
	    return -1;
    } else {
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
	iperf_printf(test, "\n");
	iperf_printf(test, "%s", report_done);
#endif    
    }

    iflush(test);

    return 0;

  cleanup_and_fail:
    if (test->debug) {
        iperf_printf(test, "iperf_run_client: cleanup_and_fail\n");
    }
    iperf_client_end(test);
    if (test->json_output)
	iperf_json_finish(test);
    iflush(test);
    return -1;
}
