/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file ftp_client.h
 *
 * @defgroup ftp_client FTP client library
 * @{
 * @brief  Library for FTP client
 *
 * @details Provide selected FTP client functionality
 */

#ifndef FTP_CLIENT_
#define FTP_CLIENT_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief List of FTP server reply codes
 * Reference RFC959 FTP Transfer Protocol
 */
enum ftp_reply_code {
	/* 100 Series	The requested action is being initiated, expect another
	 * reply before proceeding with a new command
	 */

	/** Restart marker replay. In this case, the text is exact and not left
	 *  to the particular implementation; it must read: MARK yyyy = mmmm
	 *  where yyyy is User-process data stream marker, and mmmm server's
	 *  equivalent marker (note the spaces between markers and "=")
	 */
	FTP_CODE_110 = 110,
	/** Service ready in nnn minutes */
	FTP_CODE_120 = 120,
	/** Data connection already open; transfer starting */
	FTP_CODE_125 = 125,
	/** File status okay; about to open data connection */
	FTP_CODE_150 = 150,

	/* 200 Series	The requested action has been successfully completed */

	/** Command OK*/
	FTP_CODE_200 = 200,
	/** Command not implemented, superfluous at this site */
	FTP_CODE_202 = 202,
	/** System status, or system help reply */
	FTP_CODE_211 = 211,
	/** Directory status */
	FTP_CODE_212 = 212,
	/** File status*/
	FTP_CODE_213 = 213,
	/** Help message. Explains how to use the server or the meaning of a
	 *  particular non-standard command. This reply is useful only to the
	 *  human user
	 */
	FTP_CODE_214 = 214,
	/** NAME system type. Where NAME is an official system name from the
	 *  registry kept by IANA.
	 */
	FTP_CODE_215 = 215,
	/** Service ready for new user */
	FTP_CODE_220 = 220,
	/** Service closing control connection */
	FTP_CODE_221 = 221,
	/** Data connection open; no transfer in progress */
	FTP_CODE_225 = 225,
	/** Closing data connection. Requested file action successful (for
	 *  example, file transfer or file abort)
	 */
	FTP_CODE_226 = 226,
	/** Entering Passive Mode (h1,h2,h3,h4,p1,p2) */
	FTP_CODE_227 = 227,
	/** Entering Long Passive Mode (long address, port) */
	FTP_CODE_228 = 228,
	/** Entering Extended Passive Mode (|||port|) */
	FTP_CODE_229 = 229,
	/** User logged in, proceed. Logged out if appropriate */
	FTP_CODE_230 = 230,
	/** User logged out; service terminated */
	FTP_CODE_231 = 231,
	/** Logout command noted, will complete when transfer done */
	FTP_CODE_233 = 233,
	/** Specifies that the server accepts the authentication mechanism
	 *  specified by the client, and the exchange of security data is
	 *  complete. A higher level nonstandard code created by Microsoft
	 */
	FTP_CODE_234 = 234,
	/** Requested file action okay, completed */
	FTP_CODE_250 = 250,
	/** "PATHNAME" created */
	FTP_CODE_257 = 257,

	/* 300 Series	The command has been accepted, but the requested action
	 *is on hold, pending receipt of further information
	 */

	/** User name okay, need password */
	FTP_CODE_331 = 331,
	/** Need account for login */
	FTP_CODE_332 = 332,
	/** Requested file action pending further information */
	FTP_CODE_350 = 350,

	/* 400 Series	The command was not accepted and the requested action
	 * did not take place, but the error condition is temporary and the
	 * action may be requested again
	 */

	/** Service not available, closing control connection. This may be a
	 *  reply to any command if the service knows it must shut down
	 */
	FTP_CODE_421 = 421,
	/** Cannot open data connection */
	FTP_CODE_425 = 425,
	/** Connection closed; transfer aborted */
	FTP_CODE_426 = 426,
	/** Invalid username or password */
	FTP_CODE_430 = 430,
	/** Requested host unavailable */
	FTP_CODE_434 = 434,
	/** Requested file action not taken */
	FTP_CODE_450 = 450,
	/** Requested action aborted. Local error in processing */
	FTP_CODE_451 = 451,
	/** Requested action not taken. Insufficient storage space in system.
	 *  File unavailable (for example, file busy)
	 */
	FTP_CODE_452 = 452,

	/* 500 Series	Syntax error, command unrecognized and the requested
	 * action did not take place. This may include errors such as command
	 * line too long
	 */

	/** General error */
	FTP_CODE_500 = 500,
	/** Syntax error in parameters or arguments */
	FTP_CODE_501 = 501,
	/** Command not implemented */
	FTP_CODE_502 = 502,
	/** Bad sequence of commands */
	FTP_CODE_503 = 503,
	/** Command not implemented for that parameter */
	FTP_CODE_504 = 504,
	/** Not logged in */
	FTP_CODE_530 = 530,
	/** Need account for storing files */
	FTP_CODE_532 = 532,
	/** Could Not Connect to Server - Policy Requires SSL */
	FTP_CODE_534 = 534,
	/** Requested action not taken. File unavailable (for example, file not
	 *  found, no access)
	 */
	FTP_CODE_550 = 550,
	/** Requested action aborted. Page type unknown */
	FTP_CODE_551 = 551,
	/** Requested file action aborted. Exceeded storage allocation (for
	 *  current directory or dataset)
	 */
	FTP_CODE_552 = 552,
	/** Requested action not taken. File name not allowed */
	FTP_CODE_553 = 553,

	/* Replies regarding confidentiality and integrity */

	/** Integrity protected reply */
	FTP_CODE_631 = 631,
	/** Confidentiality and integrity protected reply */
	FTP_CODE_632 = 632,
	/** Confidentiality protected reply */
	FTP_CODE_633 = 633,

	/* Nordic proprietary reply codes */

	/** DUMMY */
	FTP_CODE_900 = 900,

	/** Fatal errors */
	/** Disconnected by remote server */
	FTP_CODE_901 = 901,
	/** Connection aborted */
	FTP_CODE_902 = 902,
	/** Socket poll error */
	FTP_CODE_903 = 903,
	/** Unexpected poll event */
	FTP_CODE_904 = 904,
	/** Network down */
	FTP_CODE_905 = 905,
	/** Unexpected error */
	FTP_CODE_909 = 909,

	/* Non-fatal errors */
	/** Data transfer timeout */
	FTP_CODE_910 = 910,

	/* 10000 Series Common Winsock Error Codes[2] (These are not FTP
	 * return codes)
	 */

	/** Connection reset by peer. The connection was forcibly closed by the
	 *  remote host
	 */
	FTP_CODE_10054 = 10054,
	/** Cannot connect to remote server */
	FTP_CODE_10060 = 10060,
	/** Cannot connect to remote server. The connection is actively refused
	 *  by the server
	 */
	FTP_CODE_10061 = 10061,
	/** Directory not empty */
	FTP_CODE_10066 = 10066,
	/** Too many users, server is full */
	FTP_CODE_10068 = 10068
};

#define FTP_PRELIMINARY_POS(code)	(code > 100 && code < 200)
#define FTP_COMPLETION_POS(code)	(code > 200 && code < 300)
#define FTP_INTERMEDIATE_POS(code)	(code > 300 && code < 400)
#define FTP_TRANSIENT_NEG(code)		(code > 400 && code < 500)
#define FTP_COMPLETION_NEG(code)	(code > 500 && code < 600)
#define FTP_PROTECTED(code)		(code > 600 && code < 700)
#define FTP_PROPRIETARY(code)		(code > 900 && code < 1000)
#define FTP_WINSOCK_ERR(code)		(code > 10000)

enum ftp_trasfer_type {
	FTP_TYPE_ASCII,
	FTP_TYPE_BINARY
};

enum ftp_put_type {
	FTP_PUT_NORMAL,
	FTP_PUT_UNIQUE,
	FTP_PUT_APPEND
};

/**
 * @brief FTP asynchronous callback function.
 *
 * @param msg FTP client data received, or local message
 * @param len length of message
 */
typedef void (*ftp_client_callback_t)(const uint8_t *msg, uint16_t len);

/**@brief Initialize the FTP client library.
 *
 * @param ctrl_callback Callback for FTP command result.
 * @param data_callback Callback for FTP received data.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int ftp_init(ftp_client_callback_t ctrl_callback, ftp_client_callback_t data_callback);

/**@brief Uninitialize the FTP client library.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int ftp_uninit(void);

/**@brief Open FTP connection.
 *
 * @param hostname FTP server name or IP address
 * @param port FTP service port on server
 * @param sec_tag If FTP over TLS is required (-1 means no TLS)
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_open(const char *hostname, uint16_t port, int sec_tag);

/**@brief FTP server login.
 *
 * @param username user name
 * @param password The password
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_login(const char *username, const char *password);

/**@brief Close FTP connection.
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_close(void);

/**@brief Get FTP server and connection status
 * Also returns server system type
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_status(void);

/**@brief Set FTP transfer type
 *
 * @param type transfer type
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_type(enum ftp_trasfer_type type);

/**@brief Print working directory
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_pwd(void);

/**@brief List information of folder or file
 *
 * @param options List options, refer to Linux "man ls"
 * @param target file or directory to list. If not specified, list current folder
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_list(const char *options, const char *target);

/**@brief Change working directory
 *
 * @param folder Target folder
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_cwd(const char *folder);

/**@brief Make directory
 *
 * @param folder New folder name
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_mkd(const char *folder);

/**@brief Remove directory
 *
 * @param folder Target folder name
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_rmd(const char *folder);

/**@brief Rename a file
 *
 * @param old_name Old file name
 * @param new_name New file name
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_rename(const char *old_name, const char *new_name);

/**@brief Delete a file
 *
 * @param file Target file name
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_delete(const char *file);

/**@brief Get a file
 *
 * @param file Target file name
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_get(const char *file);

/**@brief Put data to a file
 * If file does not exist, create the file
 *
 * @param file Target file name
 * @param data Data to be stored
 * @param length Length of data to be stored
 * @param type specify FTP put types, see enum ftp_reply_code
 *
 * @retval ftp_reply_code or negative if error
 */
int ftp_put(const char *file, const uint8_t *data, uint16_t length, int type);

#ifdef __cplusplus
}
#endif

#endif /* FTP_CLIENT_ */

/**@} */
