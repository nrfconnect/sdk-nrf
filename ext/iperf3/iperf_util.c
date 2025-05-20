/*
 * iperf, Copyright (c) 2014, 2016, 2017, The Regents of the University of
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
/* iperf_util.c
 *
 * Iperf utility functions
 *
 */
#include "iperf_config.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/select.h>

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
#include <sys/socket.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
#include <sys/utsname.h>
#endif
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#if defined(CONFIG_CJSON_LIB)
#include <cJSON.h>
#else
#include "cjson.h"
#endif

#include "iperf.h"
#include "iperf_api.h"


#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
/* Added, ref: https://docs.zephyrproject.org/1.13.0/kernel/timing/clocks.html */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC CONFIG_SYS_CLOCK_TICKS_PER_SEC
#endif

/**************************************************************************/
static int nrf_iperf3_mock_gethostname(char *name, size_t len)
{
     strncpy(name, CONFIG_NRF_IPERF3_HOST_NAME, len);
     return 0;
}
/**************************************************************************/
/* Aadded because zephyr getsockname not working. Note: this is very dummy */
/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockname.html */
int nrf_iperf3_mock_getsockname(struct iperf_test *test, int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    /* Note:
       very dummy, not doing anything according to given sockfd, just settting the family by set preference
    */
    memset(addr->data, 0, sizeof(addr->data));
    addr->sa_family = AF_UNSPEC;

    if (test->settings->domain == AF_INET6) {
        addr->sa_family = AF_INET6;
    }
    else {
         addr->sa_family = AF_INET;
    }

    return 0;
}

/* Added */
int getrusage(int who, struct rusage *usage)
{
    memset(usage, 0, sizeof(*usage));   // XXX
    return 0;
}
#endif

#if defined (CONFIG_NRF_IPERF3_MULTICONTEXT_SUPPORT)
int iperf_util_socket_pdn_id_set(int fd, const char *pdn_id_str)
{
    int ret;
    int pdn_int;

    pdn_int = atoi(pdn_id_str);

    ret = setsockopt(fd, SOL_SOCKET, SO_BINDTOPDN, &pdn_int, sizeof(int));
    if (ret < 0) {
        printk("Failed to bind socket with PDN ID %s, error: %d, %s\n",
            pdn_id_str, ret, strerror(ret));
        return -EINVAL;
    }

    return 0;
}
#endif
/*
 * Read entropy from /dev/urandom
 * Errors are fatal.
 * Returns 0 on success.
 */
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)

int readentropy(void *out, size_t outsize)
{
    static FILE *frandom;
    static const char rndfile[] = "/dev/urandom";

    if (!outsize) return 0;

    if (frandom == NULL) {
        frandom = fopen(rndfile, "rb");
        if (frandom == NULL) {
            iperf_errexit(NULL, "error - failed to open %s: %s\n",
                          rndfile, strerror(errno));
            return -1;
        }
        setbuf(frandom, NULL);
    }
    if (fread(out, 1, outsize, frandom) != outsize) {
        iperf_errexit(NULL, "error - failed to read %s: %s\n",
                      rndfile,
                      feof(frandom) ? "EOF" : strerror(errno));
        return -1;
    }
    return 0;
}
#endif

/*
 * Fills buffer with repeating pattern (similar to pattern that used in iperf2)
 */
void fill_with_repeating_pattern(void *out, size_t outsize)
{
    size_t i;
    int counter = 0;
    char *buf = (char *)out;

    if (!outsize) return;

    for (i = 0; i < outsize; i++) {
        buf[i] = (char)('0' + counter);
        if (counter >= 9)
            counter = 0;
        else
            counter++;
    }
}


/* make_cookie
 *
 * Generate and return a cookie string
 *
 * Iperf uses this function to create test "cookies" which
 * server as unique test identifiers. These cookies are also
 * used for the authentication of stream connections.
 * Assumes cookie has size (COOKIE_SIZE + 1) char's.
 */

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
void make_cookie(char *cookie)
{
    int len = strlen(CONFIG_NRF_IPERF3_HOST_NAME);
    char hostname[len + 1];
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    char temp[100];

    /* Generate a string based on hostname, time, randomness, and filler. */
    (void) nrf_iperf3_mock_gethostname(hostname, sizeof(hostname));
    (void) gettimeofday(&tv, 0);
    (void) snprintf(temp, sizeof(temp), "%s.%ld.%06ld.%08lx%08lx.%s", hostname, (unsigned long int) tv.tv_sec, (unsigned long int) tv.tv_usec, (unsigned long int) rand(), (unsigned long int) rand(), "1234567890123456789012345678901234567890");

    /* Now truncate it to 36 bytes and terminate. */
    memcpy(cookie, temp, 36);
    cookie[36] = '\0';
}
#else
void make_cookie(const char *cookie)
{
    unsigned char *out = (unsigned char*)cookie;
    size_t pos;
    static const unsigned char rndchars[] = "abcdefghijklmnopqrstuvwxyz234567";

    readentropy(out, COOKIE_SIZE);
    for (pos = 0; pos < (COOKIE_SIZE - 1); pos++) {
        out[pos] = rndchars[out[pos] % (sizeof(rndchars) - 1)];
    }
    out[pos] = '\0';
}
#endif /* CONFIG_NRF_IPERF3_INTEGRATION */

/* is_closed
 *
 * Test if the file descriptor fd is closed.
 *
 * Iperf uses this function to test whether a TCP stream socket
 * is closed, because accepting and denying an invalid connection
 * in iperf_tcp_accept is not considered an error.
 */

int
is_closed(int fd)
{
    struct timeval tv;
    fd_set readset;

    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (select(fd+1, &readset, NULL, NULL, &tv) < 0) {
        if (errno == EBADF)
            return 1;
    }
    return 0;
}


double
timeval_to_double(struct timeval * tv)
{
    double d;

    d = tv->tv_sec + tv->tv_usec / 1000000;

    return d;
}

int
timeval_equals(struct timeval * tv0, struct timeval * tv1)
{
    if ( tv0->tv_sec == tv1->tv_sec && tv0->tv_usec == tv1->tv_usec )
	return 1;
    else
	return 0;
}

double
timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    double time1, time2;

    time1 = tv0->tv_sec + (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec + (tv1->tv_usec / 1000000.0);

    time1 = time1 - time2;
    if (time1 < 0)
        time1 = -time1;
    return time1;
}

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION) /* not supported */
/* CPU usage not supported. Additionally, Calling of fstat() / clock() would require system callbacks to be impelemted in libc-hook.c for _times()
 */
void
cpu_util(double pcpu[3])
{
    static struct iperf_time last;
    static clock_t clast;
    static struct rusage rlast;
    struct iperf_time now, temp_time;
    clock_t ctemp;
    struct rusage rtemp;
    double timediff;
    double userdiff;
    double systemdiff;

    if (pcpu == NULL) {
        iperf_time_now(&last);
        clast = clock();
	getrusage(RUSAGE_SELF, &rlast);
        return;
    }

    iperf_time_now(&now);
    ctemp = clock();
    getrusage(RUSAGE_SELF, &rtemp);

    iperf_time_diff(&now, &last, &temp_time);
    timediff = iperf_time_in_usecs(&temp_time);

    userdiff = ((rtemp.ru_utime.tv_sec * 1000000.0 + rtemp.ru_utime.tv_usec) -
                (rlast.ru_utime.tv_sec * 1000000.0 + rlast.ru_utime.tv_usec));
    systemdiff = ((rtemp.ru_stime.tv_sec * 1000000.0 + rtemp.ru_stime.tv_usec) -
                  (rlast.ru_stime.tv_sec * 1000000.0 + rlast.ru_stime.tv_usec));

    pcpu[0] = (((ctemp - clast) * 1000000.0 / CLOCKS_PER_SEC) / timediff) * 100;
    pcpu[1] = (userdiff / timediff) * 100;
    pcpu[2] = (systemdiff / timediff) * 100;
}
#endif
const char *
get_system_info(void)
{
    static const char *buf = CONFIG_NRF_IPERF3_HOST_NAME;

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)  /* other system info not supported */
    static char buf[1024];
    struct utsname  uts;
    memset(buf, 0, 1024);
    uname(&uts);
    snprintf(buf, sizeof(buf), "%s %s %s %s %s", uts.sysname, uts.nodename,
	     uts.release, uts.version, uts.machine);
#endif
    return buf;
}


const char *
get_optional_features(void)
{
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    static char features[64]; /* No actual support, so decrease stack usage */
#else
    static char features[1024];
#endif

    unsigned int numfeatures = 0;

    snprintf(features, sizeof(features), "Optional features available: ");
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)  /* not supported */
#if defined(HAVE_CPU_AFFINITY)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "CPU affinity setting",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_CPU_AFFINITY */

#if defined(HAVE_FLOWLABEL)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "IPv6 flow label",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_FLOWLABEL */

#if defined(HAVE_SCTP_H)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "SCTP",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_SCTP_H */

#if defined(HAVE_TCP_CONGESTION)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "TCP congestion algorithm setting",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_TCP_CONGESTION */

#if defined(HAVE_SENDFILE)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "sendfile / zerocopy",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_SENDFILE */

#if defined(HAVE_SO_MAX_PACING_RATE)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "socket pacing",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_SO_MAX_PACING_RATE */

#if defined(HAVE_SSL)
    if (numfeatures > 0) {
	strncat(features, ", ",
		sizeof(features) - strlen(features) - 1);
    }
    strncat(features, "authentication",
	sizeof(features) - strlen(features) - 1);
    numfeatures++;
#endif /* HAVE_SSL */
#endif
    if (numfeatures == 0) {
	strncat(features, "None",
		sizeof(features) - strlen(features) - 1);
    }

    return features;
}

/* Helper routine for building cJSON objects in a printf-like manner.
**
** Sample call:
**   j = iperf_json_printf("foo: %b  bar: %d  bletch: %f  eep: %s", b, i, f, s);
**
** The four formatting characters and the types they expect are:
**   %b  boolean           int
**   %d  integer           int64_t
**   %f  floating point    double
**   %s  string            char *
** If the values you're passing in are not these exact types, you must
** cast them, there is no automatic type coercion/widening here.
**
** The colons mark the end of field names, and blanks are ignored.
**
** This routine is not particularly robust, but it's not part of the API,
** it's just for internal iperf3 use.
*/
cJSON*
iperf_json_printf(const char *format, ...)
{
    cJSON* o;
    va_list argp;
    const char *cp;
    char name[100];
    char* np;
    cJSON* j;

    o = cJSON_CreateObject();
    if (o == NULL)
        return NULL;
    va_start(argp, format);
    np = name;
    for (cp = format; *cp != '\0'; ++cp) {
	switch (*cp) {
	    case ' ':
	    break;
	    case ':':
	    *np = '\0';
	    break;
	    case '%':
	    ++cp;
	    switch (*cp) {
		case 'b':
		j = cJSON_CreateBool(va_arg(argp, int));
		break;
		case 'd':
		j = cJSON_CreateNumber(va_arg(argp, int64_t));
		break;
		case 'f':
		j = cJSON_CreateNumber(va_arg(argp, double));
		break;
		case 's':
		j = cJSON_CreateString(va_arg(argp, char *));
		break;
		default:
		va_end(argp);
		return NULL;
	    }
	    if (j == NULL) {
	    	va_end(argp);
	    	return NULL;
	    }
	    cJSON_AddItemToObject(o, name, j);
	    np = name;
	    break;
	    default:
	    *np++ = *cp;
	    break;
	}
    }
    va_end(argp);
    return o;
}

/* Debugging routine to dump out an fd_set. */
void
iperf_dump_fdset(FILE *fp, const char *str, int nfds, fd_set *fds)
{
    int fd;
    int comma;

    fprintf(fp, "%s: [", str);
    comma = 0;
    for (fd = 0; fd < nfds; ++fd) {
        if (FD_ISSET(fd, fds)) {
	    if (comma)
		fprintf(fp, ", ");
	    fprintf(fp, "%d", fd);
	    comma = 1;
	}
    }
    fprintf(fp, "]\n");
}

/*
 * daemon(3) implementation for systems lacking one.
 * Cobbled together from various daemon(3) implementations,
 * not intended to be general-purpose. */
#ifndef HAVE_DAEMON
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
int daemon(int nochdir, int noclose)
{
    pid_t pid = 0;
    pid_t sid = 0;
    int fd;

    /*
     * Ignore any possible SIGHUP when the parent process exits.
     * Note that the iperf3 server process will eventually install
     * its own signal handler for SIGHUP, so we can be a little
     * sloppy about not restoring the prior value.  This does not
     * generalize.
     */
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
	    return -1;
    }
    if (pid > 0) {
	/* Use _exit() to avoid doing atexit() stuff. */
	_exit(0);
    }

    sid = setsid();
    if (sid < 0) {
	return -1;
    }

    /*
     * Fork again to avoid becoming a session leader.
     * This might only matter on old SVr4-derived OSs.
     * Note in particular that glibc and FreeBSD libc
     * only fork once.
     */
    pid = fork();
    if (pid == -1) {
	return -1;
    } else if (pid != 0) {
	_exit(0);
    }

    if (!nochdir) {
	chdir("/");
    }

    if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	if (fd > 2) {
	    close(fd);
	}
    }
    return (0);
}
#endif
#endif /* HAVE_DAEMON */

/* Compatibility version of getline(3) for systems that don't have it.. */
#ifndef HAVE_GETLINE
/* The following code adopted from NetBSD's getline.c, which is: */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
ssize_t
getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
{
	char *ptr, *eptr;


	if (*buf == NULL || *bufsiz == 0) {
		*bufsiz = BUFSIZ;
		if ((*buf = malloc(*bufsiz)) == NULL)
			return -1;
	}

	for (ptr = *buf, eptr = *buf + *bufsiz;;) {
		int c = fgetc(fp);
		if (c == -1) {
			if (feof(fp)) {
				ssize_t diff = (ssize_t)(ptr - *buf);
				if (diff != 0) {
					*ptr = '\0';
					return diff;
				}
			}
			return -1;
		}
		*ptr++ = c;
		if (c == delimiter) {
			*ptr = '\0';
			return ptr - *buf;
		}
		if (ptr + 2 >= eptr) {
			char *nbuf;
			size_t nbufsiz = *bufsiz * 2;
			ssize_t d = ptr - *buf;
			if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
				return -1;
			*buf = nbuf;
			*bufsiz = nbufsiz;
			eptr = nbuf + nbufsiz;
			ptr = nbuf + d;
		}
	}
}

ssize_t
getline(char **buf, size_t *bufsiz, FILE *fp)
{
	return getdelim(buf, bufsiz, '\n', fp);
}

#endif
