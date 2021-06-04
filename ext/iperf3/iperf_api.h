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
#ifndef        __IPERF_API_H
#define        __IPERF_API_H

#include <sys/socket.h>
#include <sys/time.h>

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
#include <setjmp.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef __cplusplus
extern "C" { /* open extern "C" */
#endif


struct iperf_test;
struct iperf_stream_result;
struct iperf_interval_results;
struct iperf_stream;
struct iperf_time;

#if !defined(__IPERF_H)
typedef uint64_t iperf_size_t;
#endif // __IPERF_H

/* default settings */
#define Ptcp SOCK_STREAM
#define Pudp SOCK_DGRAM
#define Psctp 12

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
#define DEFAULT_UDP_BLKSIZE 1200 /* default is dynamically set, else this */
#define DEFAULT_TCP_BLKSIZE (5 * 708)  /* default read/write block size */
#else
#define DEFAULT_UDP_BLKSIZE 1460 /* default is dynamically set, else this */
#define DEFAULT_TCP_BLKSIZE (128 * 1024)  /* default read/write block size */
#endif

#define DEFAULT_SCTP_BLKSIZE (64 * 1024)

/* short option equivalents, used to support options that only have long form */
#define OPT_SCTP 1
#define OPT_LOGFILE 2
#define OPT_GET_SERVER_OUTPUT 3
#define OPT_UDP_COUNTERS_64BIT 4
#define OPT_CLIENT_PORT 5
#define OPT_NUMSTREAMS 6
#define OPT_FORCEFLUSH 7
#define OPT_NO_FQ_SOCKET_PACING 9 /* UNUSED */
#define OPT_FQ_RATE 10
#define OPT_DSCP 11
#define OPT_CLIENT_USERNAME 12
#define OPT_CLIENT_RSA_PUBLIC_KEY 13
#define OPT_SERVER_RSA_PRIVATE_KEY 14
#define OPT_SERVER_AUTHORIZED_USERS 15
#define OPT_PACING_TIMER 16
#define OPT_CONNECT_TIMEOUT 17
#define OPT_REPEATING_PAYLOAD 18
#define OPT_EXTRA_DATA 19
#define OPT_BIDIRECTIONAL 20
#define OPT_SERVER_BITRATE_LIMIT 21
#define OPT_TIMESTAMPS 22
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
#define NRF_OPT_CURRENT_MDM_TRACES 23
#define NRF_OPT_PDN_ID 24
#endif

/* states */
#define TEST_START 1
#define TEST_RUNNING 2
#define RESULT_REQUEST 3 /* not used */
#define TEST_END 4
#define STREAM_BEGIN 5 /* not used */
#define STREAM_RUNNING 6 /* not used */
#define STREAM_END 7 /* not used */
#define ALL_STREAMS_END 8 /* not used */
#define PARAM_EXCHANGE 9
#define CREATE_STREAMS 10
#define SERVER_TERMINATE 11
#define CLIENT_TERMINATE 12
#define EXCHANGE_RESULTS 13
#define DISPLAY_RESULTS 14
#define IPERF_START 15
#define IPERF_DONE 16
#define ACCESS_DENIED (-1)
#define SERVER_ERROR (-2)

/* Getter routines for some fields inside iperf_test. */
int	iperf_get_verbose( struct iperf_test* ipt );
int	iperf_get_control_socket( struct iperf_test* ipt );
int	iperf_get_test_omit( struct iperf_test* ipt );
int	iperf_get_test_duration( struct iperf_test* ipt );
char	iperf_get_test_role( struct iperf_test* ipt );
int	iperf_get_test_reverse( struct iperf_test* ipt );
int	iperf_get_test_blksize( struct iperf_test* ipt );
FILE*	iperf_get_test_outfile( struct iperf_test* ipt );
uint64_t iperf_get_test_rate( struct iperf_test* ipt );
int iperf_get_test_pacing_timer( struct iperf_test* ipt );
uint64_t iperf_get_test_bytes( struct iperf_test* ipt );
uint64_t iperf_get_test_blocks( struct iperf_test* ipt );
int     iperf_get_test_burst( struct iperf_test* ipt );
int	iperf_get_test_socket_bufsize( struct iperf_test* ipt );
double	iperf_get_test_reporter_interval( struct iperf_test* ipt );
double	iperf_get_test_stats_interval( struct iperf_test* ipt );
int	iperf_get_test_num_streams( struct iperf_test* ipt );
int	iperf_get_test_repeating_payload( struct iperf_test* ipt );
int	iperf_get_test_timestamps( struct iperf_test* ipt );
const char* iperf_get_test_timestamp_format( struct iperf_test* ipt );
int	iperf_get_test_server_port( struct iperf_test* ipt );
char*	iperf_get_test_server_hostname( struct iperf_test* ipt );
char*	iperf_get_test_template( struct iperf_test* ipt );
int	iperf_get_test_protocol_id( struct iperf_test* ipt );
int	iperf_get_test_json_output( struct iperf_test* ipt );
char*	iperf_get_test_json_output_string ( struct iperf_test* ipt );
int	iperf_get_test_zerocopy( struct iperf_test* ipt );
int	iperf_get_test_get_server_output( struct iperf_test* ipt );
char*	iperf_get_test_bind_address ( struct iperf_test* ipt );
int	iperf_get_test_udp_counters_64bit( struct iperf_test* ipt );
int	iperf_get_test_one_off( struct iperf_test* ipt );
int iperf_get_test_tos( struct iperf_test* ipt );
char*	iperf_get_extra_data( struct iperf_test* ipt );
char*	iperf_get_iperf_version(void);
int	iperf_get_test_no_delay( struct iperf_test* ipt );
int	iperf_get_test_connect_timeout( struct iperf_test* ipt );

/* Setter routines for some fields inside iperf_test. */
void	iperf_set_verbose( struct iperf_test* ipt, int verbose );
void	iperf_set_control_socket( struct iperf_test* ipt, int ctrl_sck );
void	iperf_set_test_omit( struct iperf_test* ipt, int omit );
void	iperf_set_test_duration( struct iperf_test* ipt, int duration );
void	iperf_set_test_reporter_interval( struct iperf_test* ipt, double reporter_interval );
void	iperf_set_test_stats_interval( struct iperf_test* ipt, double stats_interval );
void	iperf_set_test_state( struct iperf_test* ipt, signed char state );
void	iperf_set_test_blksize( struct iperf_test* ipt, int blksize );
void	iperf_set_test_logfile( struct iperf_test* ipt, const char *logfile );
void	iperf_set_test_rate( struct iperf_test* ipt, uint64_t rate );
void    iperf_set_test_pacing_timer( struct iperf_test* ipt, int pacing_timer );
void    iperf_set_test_bytes( struct iperf_test* ipt, uint64_t bytes );
void    iperf_set_test_blocks( struct iperf_test* ipt, uint64_t blocks );
void	iperf_set_test_burst( struct iperf_test* ipt, int burst );
void	iperf_set_test_server_port( struct iperf_test* ipt, int server_port );
void	iperf_set_test_socket_bufsize( struct iperf_test* ipt, int socket_bufsize );
void	iperf_set_test_num_streams( struct iperf_test* ipt, int num_streams );
void	iperf_set_test_repeating_payload( struct iperf_test* ipt, int repeating_payload );
void	iperf_set_test_timestamps( struct iperf_test* ipt, int timestamps );
void	iperf_set_test_timestamp_format( struct iperf_test*, const char *tf );
void	iperf_set_test_role( struct iperf_test* ipt, char role );
void	iperf_set_test_server_hostname( struct iperf_test* ipt, const char* server_hostname );
void    iperf_set_test_template( struct iperf_test *ipt, const char *tmp_template );
void	iperf_set_test_reverse( struct iperf_test* ipt, int reverse );
void	iperf_set_test_json_output( struct iperf_test* ipt, int json_output );
int	iperf_has_zerocopy( void );
void	iperf_set_test_zerocopy( struct iperf_test* ipt, int zerocopy );
void	iperf_set_test_get_server_output( struct iperf_test* ipt, int get_server_output );
void	iperf_set_test_bind_address( struct iperf_test* ipt, const char *bind_address );
void	iperf_set_test_udp_counters_64bit( struct iperf_test* ipt, int udp_counters_64bit );
void	iperf_set_test_one_off( struct iperf_test* ipt, int one_off );
void    iperf_set_test_tos( struct iperf_test* ipt, int tos );
void	iperf_set_test_extra_data( struct iperf_test* ipt, const char *dat );
void    iperf_set_test_bidirectional( struct iperf_test* ipt, int bidirectional);
void    iperf_set_test_no_delay( struct iperf_test* ipt, int no_delay);

#if defined(HAVE_SSL)
void    iperf_set_test_client_username(struct iperf_test *ipt, const char *client_username);
void    iperf_set_test_client_password(struct iperf_test *ipt, const char *client_password);
void    iperf_set_test_client_rsa_pubkey(struct iperf_test *ipt, const char *client_rsa_pubkey_base64);
void    iperf_set_test_server_authorized_users(struct iperf_test *ipt, const char *server_authorized_users);
void    iperf_set_test_server_rsa_privkey(struct iperf_test *ipt, const char *server_rsa_privkey_base64);
#endif // HAVE_SSL

void	iperf_set_test_connect_timeout(struct iperf_test *ipt, int ct);

/**
 * exchange_parameters - handles the param_Exchange part for client
 *
 */
int      iperf_exchange_parameters(struct iperf_test * test);

/**
 * add_to_interval_list -- adds new interval to the interval_list
 *
 */
void      add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results *temp);

/**
 * connect_msg -- displays connection message
 * denoting senfer/receiver details
 *
 */
void      connect_msg(struct iperf_stream * sp);

/**
 * iperf_stats_callback -- handles the statistic gathering
 *
 */
void     iperf_stats_callback(struct iperf_test * test);

/**
 * iperf_reporter_callback -- handles the report printing
 *
 */
void     iperf_reporter_callback(struct iperf_test * test);

/**
 * iperf_new_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_new_test(void);

int      iperf_defaults(struct iperf_test * testp);

/**
 * iperf_free_test -- free resources used by test, calls iperf_free_stream to
 * free streams
 *
 */
void      iperf_free_test(struct iperf_test * testp);

/**
 * iperf_new_stream -- return a net iperf_stream with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_stream *iperf_new_stream(struct iperf_test *, int, int);

/**
 * iperf_add_stream -- add a stream to a test
 *
 */
void      iperf_add_stream(struct iperf_test * test, struct iperf_stream * stream);

/**
 * iperf_init_stream -- init resources associated with test
 *
 */
int       iperf_init_stream(struct iperf_stream *, struct iperf_test *);

/**
 * iperf_free_stream -- free resources associated with test
 *
 */
void      iperf_free_stream(struct iperf_stream * sp);

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
int iperf_main(int argc, char *argv[]);
#endif

int has_tcpinfo(void);
int has_tcpinfo_retransmits(void);
void save_tcpinfo(struct iperf_stream *sp, struct iperf_interval_results *irp);
long get_total_retransmits(struct iperf_interval_results *irp);
long get_snd_cwnd(struct iperf_interval_results *irp);
long get_rtt(struct iperf_interval_results *irp);
long get_rttvar(struct iperf_interval_results *irp);
long get_pmtu(struct iperf_interval_results *irp);
void print_tcpinfo(struct iperf_test *test);
void build_tcpinfo_message(struct iperf_interval_results *r, char *message);

int iperf_set_send_state(struct iperf_test *test, signed char state);
void iperf_check_throttle(struct iperf_stream *sp, struct iperf_time *nowP);
int iperf_send(struct iperf_test *, fd_set *) /* __attribute__((hot)) */;
int iperf_recv(struct iperf_test *, fd_set *);


#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
void iperf_catch_sigend(void (*handler)(int));
void iperf_got_sigend(struct iperf_test *test) __attribute__ ((noreturn));
void usage(void);
void usage_long(FILE * f);
#endif

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
void nrf_iperf3_usage();
int nrf_iperf3_mock_getsockdomain(struct iperf_test *test, int sock);
#endif

void warning(const char *);
int iperf_exchange_results(struct iperf_test *);
int iperf_init_test(struct iperf_test *);
int iperf_create_send_timers(struct iperf_test *);
int iperf_parse_arguments(struct iperf_test *, int, char **);
int iperf_open_logfile(struct iperf_test *);
void iperf_reset_test(struct iperf_test *);
void iperf_reset_stats(struct iperf_test * test);

struct protocol *get_protocol(struct iperf_test *, int);
int set_protocol(struct iperf_test *, int);

void iperf_on_new_stream(struct iperf_stream *);
void iperf_on_test_start(struct iperf_test *);
void iperf_on_connect(struct iperf_test *);
void iperf_on_test_finish(struct iperf_test *);

#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
extern jmp_buf env;
#endif

/* Client routines. */
int iperf_run_client(struct iperf_test *);
int iperf_connect(struct iperf_test *);
int iperf_create_streams(struct iperf_test *, int sender);
int iperf_handle_message_client(struct iperf_test *);
int iperf_client_end(struct iperf_test *);

/* Server routines. */
int iperf_run_server(struct iperf_test *);
int iperf_server_listen(struct iperf_test *);
int iperf_accept(struct iperf_test *);
int iperf_handle_message_server(struct iperf_test *);
#if !defined(CONFIG_NRF_IPERF3_INTEGRATION)
int iperf_create_pidfile(struct iperf_test *);
int iperf_delete_pidfile(struct iperf_test *);
#endif
void iperf_check_total_rate(struct iperf_test *, iperf_size_t);

/* JSON output routines. */
int iperf_json_start(struct iperf_test *);
int iperf_json_finish(struct iperf_test *);

/* CPU affinity routines */
int iperf_setaffinity(struct iperf_test *, int affinity);
int iperf_clearaffinity(struct iperf_test *);

/* Custom printf routine. */
int iperf_printf(struct iperf_test *test, const char *format, ...) __attribute__ ((format(printf,2,3)));
int iflush(struct iperf_test *test);

/* Error routines. */
void iperf_err(struct iperf_test *test, const char *format, ...) __attribute__ ((format(printf,2,3)));

#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
/* noreturn removed as no support for exit(): */
void iperf_errexit(struct iperf_test *test, const char *format, ...) __attribute__ ((format(printf,2,3)));
#else
void iperf_errexit(struct iperf_test *test, const char *format, ...) __attribute__ ((format(printf,2,3),noreturn));
#endif

char *iperf_strerror(int);
extern int i_errno;
enum {
    IENONE = 0,             // No error
    /* Parameter errors */
    IESERVCLIENT = 1,       // Iperf cannot be both server and client
    IENOROLE = 2,           // Iperf must either be a client (-c) or server (-s)
    IESERVERONLY = 3,       // This option is server only
    IECLIENTONLY = 4,       // This option is client only
    IEDURATION = 5,         // test duration too long. Maximum value = %dMAX_TIME
    IENUMSTREAMS = 6,       // Number of parallel streams too large. Maximum value = %dMAX_STREAMS
    IEBLOCKSIZE = 7,        // Block size too large. Maximum value = %dMAX_BLOCKSIZE
    IEBUFSIZE = 8,          // Socket buffer size too large. Maximum value = %dMAX_TCP_BUFFER
    IEINTERVAL = 9,         // Invalid report interval (min = %gMIN_INTERVAL, max = %gMAX_INTERVAL seconds)
    IEMSS = 10,             // MSS too large. Maximum value = %dMAX_MSS
    IENOSENDFILE = 11,      // This OS does not support sendfile
    IEOMIT = 12,            // Bogus value for --omit
    IEUNIMP = 13,           // Not implemented yet
    IEFILE = 14,            // -F file couldn't be opened
    IEBURST = 15,           // Invalid burst count. Maximum value = %dMAX_BURST
    IEENDCONDITIONS = 16,   // Only one test end condition (-t, -n, -k) may be specified
    IELOGFILE = 17,	    // Can't open log file
    IENOSCTP = 18,	    // No SCTP support available
    IEBIND = 19,	    // UNUSED:  Local port specified with no local bind option
    IEUDPBLOCKSIZE = 20,    // Block size invalid
    IEBADTOS = 21,	    // Bad TOS value
    IESETCLIENTAUTH = 22,   // Bad configuration of client authentication
    IESETSERVERAUTH = 23,   // Bad configuration of server authentication
    IEBADFORMAT = 24,	    // Bad format argument to -f
    IEREVERSEBIDIR = 25,    // Iperf cannot be both reverse and bidirectional
    IEBADPORT = 26,	    // Bad port number
    IETOTALRATE = 27,       // Total required bandwidth is larger than server's limit
    IETOTALINTERVAL = 28,   // Invalid time interval for calculating average data rate
    /* Test errors */
    IENEWTEST = 100,        // Unable to create a new test (check perror)
    IEINITTEST = 101,       // Test initialization failed (check perror)
    IELISTEN = 102,         // Unable to listen for connections (check perror)
    IECONNECT = 103,        // Unable to connect to server (check herror/perror) [from netdial]
    IEACCEPT = 104,         // Unable to accept connection from client (check herror/perror)
    IESENDCOOKIE = 105,     // Unable to send cookie to server (check perror)
    IERECVCOOKIE = 106,     // Unable to receive cookie from client (check perror)
    IECTRLWRITE = 107,      // Unable to write to the control socket (check perror)
    IECTRLREAD = 108,       // Unable to read from the control socket (check perror)
    IECTRLCLOSE = 109,      // Control socket has closed unexpectedly
    IEMESSAGE = 110,        // Received an unknown message
    IESENDMESSAGE = 111,    // Unable to send control message to client/server (check perror)
    IERECVMESSAGE = 112,    // Unable to receive control message from client/server (check perror)
    IESENDPARAMS = 113,     // Unable to send parameters to server (check perror)
    IERECVPARAMS = 114,     // Unable to receive parameters from client (check perror)
    IEPACKAGERESULTS = 115, // Unable to package results (check perror)
    IESENDRESULTS = 116,    // Unable to send results to client/server (check perror)
    IERECVRESULTS = 117,    // Unable to receive results from client/server (check perror)
    IESELECT = 118,         // Select failed (check perror)
    IECLIENTTERM = 119,     // The client has terminated
    IESERVERTERM = 120,     // The server has terminated
    IEACCESSDENIED = 121,   // The server is busy running a test. Try again later.
    IESETNODELAY = 122,     // Unable to set TCP/SCTP NODELAY (check perror)
    IESETMSS = 123,         // Unable to set TCP/SCTP MSS (check perror)
    IESETBUF = 124,         // Unable to set socket buffer size (check perror)
    IESETTOS = 125,         // Unable to set IP TOS (check perror)
    IESETCOS = 126,         // Unable to set IPv6 traffic class (check perror)
    IESETFLOW = 127,        // Unable to set IPv6 flow label
    IEREUSEADDR = 128,      // Unable to set reuse address on socket (check perror)
    IENONBLOCKING = 129,    // Unable to set socket to non-blocking (check perror)
    IESETWINDOWSIZE = 130,  // Unable to set socket window size (check perror)
    IEPROTOCOL = 131,       // Protocol does not exist
    IEAFFINITY = 132,       // Unable to set CPU affinity (check perror)
    IEDAEMON = 133,	    // Unable to become a daemon process
    IESETCONGESTION = 134,  // Unable to set TCP_CONGESTION
    IEPIDFILE = 135,	    // Unable to write PID file
    IEV6ONLY = 136,  	    // Unable to set/unset IPV6_V6ONLY (check perror)
    IESETSCTPDISABLEFRAG = 137, // Unable to set SCTP Fragmentation (check perror)
    IESETSCTPNSTREAM= 138,  //  Unable to set SCTP number of streams (check perror)
    IESETSCTPBINDX= 139,    // Unable to process sctp_bindx() parameters
    IESETPACING= 140,       // Unable to set socket pacing rate
    IESETBUF2= 141,	    // Socket buffer size incorrect (written value != read value)
    IEAUTHTEST = 142,       // Test authorization failed
    /* Stream errors */
    IECREATESTREAM = 200,   // Unable to create a new stream (check herror/perror)
    IEINITSTREAM = 201,     // Unable to initialize stream (check herror/perror)
    IESTREAMLISTEN = 202,   // Unable to start stream listener (check perror) 
    IESTREAMCONNECT = 203,  // Unable to connect stream (check herror/perror)
    IESTREAMACCEPT = 204,   // Unable to accepte stream connection (check perror)
    IESTREAMWRITE = 205,    // Unable to write to stream socket (check perror)
    IESTREAMREAD = 206,     // Unable to read from stream (check perror)
    IESTREAMCLOSE = 207,    // Stream has closed unexpectedly
    IESTREAMID = 208,       // Stream has invalid ID
    /* Timer errors */
    IENEWTIMER = 300,       // Unable to create new timer (check perror)
    IEUPDATETIMER = 301,    // Unable to update timer (check perror)
#if defined(CONFIG_NRF_IPERF3_INTEGRATION)
    IENOMEMORY = 302,         // no dynamic memory from heap
    IETESTSTARTTIMEOUT = 303, // testing start timeout
#endif
};


#ifdef __cplusplus
} /* close extern "C" */
#endif


#endif /* !__IPERF_API_H */
