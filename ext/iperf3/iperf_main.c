/*
 * iperf, Copyright (c) 2014, 2015, 2017, 2019, The Regents of the University of
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
#include "iperf_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_locale.h"
#include "net.h"
#include "units.h"


static int run(struct iperf_test *test);

/**************************************************************************/
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* NRF_IPERF3_INTEGRATION_TODO: zephyr support missing?? */
int
daemon(int nochdir, int noclose)
{
    (void)nochdir; (void)noclose;
    return -1;
}
#endif

/**************************************************************************/
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
int iperf_main(int argc, char **argv, char *resp_std_out_buff, int resp_std_out_buff_len,
	       struct k_poll_signal *kill_signal)
#else
int
iperf_main(int argc, char **argv)
#endif
{
    struct iperf_test *test;
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    int retval = 0;
#endif

    // XXX: Setting the process affinity requires root on most systems.
    //      Is this a feature we really need?
#ifdef TEST_PROC_AFFINITY
    /* didnt seem to work.... */
    /*
     * increasing the priority of the process to minimise packet generation
     * delay
     */
    int rc = setpriority(PRIO_PROCESS, 0, -15);

    if (rc < 0) {
        perror("setpriority:");
        fprintf(stderr, "setting priority to valid level\n");
        rc = setpriority(PRIO_PROCESS, 0, 0);
    }
    
    /* setting the affinity of the process  */
    cpu_set_t cpu_set;
    int affinity = -1;
    int ncores = 1;

    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
    if (errno)
        perror("couldn't get affinity:");

    if ((ncores = sysconf(_SC_NPROCESSORS_CONF)) <= 0)
        err("sysconf: couldn't get _SC_NPROCESSORS_CONF");

    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0)
        err("couldn't change CPU affinity");
#endif

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)    
    test = iperf_new_test();
    if (!test) {
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
        retval = -1;
        goto exit;
    }
    iperf_defaults(test);	/* sets defaults */

    test->resp_std_out_buff = resp_std_out_buff;
    test->resp_std_out_buff_len = resp_std_out_buff_len;
    test->kill_signal = kill_signal;

    retval = iperf_parse_arguments(test, argc, argv);
    if (retval < 0) {
        if (retval == -2) {
            retval = 0; //-2 is special for 'h' (help), not considered as an error
        }
        else
        {
            iperf_err(test, "parameter error - %s", iperf_strerror(test->i_errno));
            fprintf(stderr, "\n");
            nrf_iperf3_usage();
            retval = -1;
        }
        goto exit;
    }

    if (retval == 0 && run(test) < 0) {
        iperf_errexit(test, "error - %s", iperf_strerror(test->i_errno));
        retval = -1;
    }

exit:
    if (retval == 0)
    {
        iperf_printf(test, "iperf Done.\r\n");
    }
    else
    {
        iperf_printf(test, "iperf Failed.\r\n");
    }
    if (test) {
        iperf_free_test(test);
    }
    return retval;
#else
    test = iperf_new_test();
    if (!test)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test);	/* sets defaults */

    if (iperf_parse_arguments(test, argc, argv) < 0) {
        iperf_err(test, "parameter error - %s", iperf_strerror(test->i_errno));
        fprintf(stderr, "\n");
        usage_long(stdout);
        exit(1);
    }

    if (run(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(test->i_errno));

    iperf_free_test(test);

    return 0;
#endif
}

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* No support for signals or setjmp */
static jmp_buf sigend_jmp_buf;

static void __attribute__ ((noreturn))
sigend_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}
#endif

/**************************************************************************/
/* NRF_IPERF3_INTEGRATION_TODO: enable when zephyr shell is supporting abort */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
static bool do_exit;
static void signal_handler(int sig)
{
	do_exit = true;
}
#endif
static int
run(struct iperf_test *test)
{

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* No support for signals or setjmp */
    /* Termination signals. */
    iperf_catch_sigend(sigend_handler);
    if (setjmp(sigend_jmp_buf))
	iperf_got_sigend(test);

    /* Ignore SIGPIPE to simplify error handling */
    signal(SIGPIPE, SIG_IGN);
#endif

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
/* NRF_IPERF3_INTEGRATION_TODO: enable when zephyr shell is supporting abort */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
#endif
    switch (test->role) {
        case 's':
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* No support */
	    if (test->daemon) {
		int rc;
		rc = daemon(0, 0);
		if (rc < 0) {
		    test->i_errno = IEDAEMON;
		    iperf_errexit(test, "error - %s", iperf_strerror(test->i_errno));
            return -1;
		}
	    }
	    if (iperf_create_pidfile(test) < 0) {
		    test->i_errno = IEPIDFILE;
		    iperf_errexit(test, "error - %s", iperf_strerror(test->i_errno));
            return -1;
	    }
#endif
            for (;;) {
		int rc;
		rc = iperf_run_server(test);
		if (rc < 0) {
		    iperf_err(test, "error - %s", iperf_strerror(test->i_errno));
		    if (rc < -1) {
		        iperf_errexit(test, "exiting");
                return -1;
		    }
                }
                iperf_reset_test(test);
                if (iperf_get_test_one_off(test)) {
		    /* Authentication failure doesn't count for 1-off test */
		    if (rc < 0 && test->i_errno == IEAUTHTEST) {
			continue;
		    }
		    break;
		}
            }
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* No support */
	    iperf_delete_pidfile(test);
#endif
            break;
	case 'c':
	    if (iperf_run_client(test) < 0) {
		    iperf_errexit(test, "error - %s", iperf_strerror(test->i_errno));
            return -1;
        }
            break;
        default:
            nrf_iperf3_usage();
            break;
    }
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
    iperf_catch_sigend(SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
#endif

    return 0;
}
