/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FTP_COMMANDS_
#define FTP_COMMANDS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reference https://en.wikipedia.org/wiki/List_of_FTP_commands
 */
/* Abort an active file transfer */
#define CMD_ABOR	"ABOR\r\n"
/* Account information */
#define CMD_ACCT	"ACCT %s\r\n"
/* Authentication/Security Data */
#define CMD_ADAT	"ADAT\r\n"
/* Allocate sufficient disk space to receive a file */
#define CMD_ALLO	"ALLO\r\n"
/* Append (with create) */
#define CMD_APPE	"APPE %s\r\n"
/* Authentication/Security Mechanism */
#define CMD_AUTH	"AUTH\r\n"
/* Get the available space */
#define CMD_AVBL	"AVBL\r\n"
/* Clear Command Channel */
#define CMD_CCC		"CCC\r\n"
/* Change to Parent Directory */
#define CMD_CDUP	"CDUP\r\n"
/* Confidentiality Protection Command */
#define CMD_CONF	"CONF\r\n"
/* Client / Server Identification */
#define CMD_CSID	"CSID\r\n"
/* Change working directory */
#define CMD_CWD		"CWD %s\r\n"
/* Delete file */
#define CMD_DELE	"DELE %s\r\n"
/* Get the directory size */
#define CMD_DSIZ	"DSIZE\r\n"
/* Privacy Protected Channel */
#define CMD_ENC		"ENC\r\n"
/* Specifies an extended address and port to which the server should connect */
#define CMD_EPORT	"RPORT\r\n"
/* Enter extended passive mode */
#define CMD_EPSV	"EPSV\r\n"
/* Get the feature list implemented by the server*/
#define CMD_FEAT	"FEAT\r\n"
/* Returns usage documentation on a command if specified, else a general help
 * document is returned
 */
#define CMD_HELP	"HELP\r\n"
/* Identify desired virtual host on server, by name */
#define CMD_HOST	"HOST\r\n"
/* Language Negotiation */
#define CMD_LANG	"LANG %s\r\n"
/* Returns information of a file or directory if specified, else in formation of
 * the current working directory is returned
 */
#define CMD_LIST	"LIST\r\n"
#define CMD_LIST_OPT	"LIST %s\r\n"
#define CMD_LIST_FILE	"LIST %s\r\n"
#define CMD_LIST_OPT_FILE	"LIST %s %s\r\n"
/* Specifies a long address and port to which the server should connect */
#define CMD_LPRT	"LPRT\r\n"
/* Enter long passive mode */
#define CMD_LPSV	"LPSV\r\n"
/* Return the last-modified time of a specified file */
#define CMD_MDTM	"MDTM %s\r\n"
/* Modify the creation time of a file */
#define CMD_MFCT	"MFCT\r\n"
/* Modify fact (the last modification time, creation time,
 * UNIX group/owner/mode of a file)
 */
#define CMD_MFF		"MFF\r\n"
/* Modify the last modification time of a file */
#define CMD_MFMT	"MFMT\r\n"
/* Integrity Protected Command */
#define CMD_MIC		"MIC\r\n"
/* Make directory */
#define CMD_MKD		"MKD %s\r\n"
/* Lists the contents of a directory if a directory is named */
#define CMD_MLSD	"MLSD\r\n"
/* Provides data about exactly the object named on its command line
 * and no others
 */
#define CMD_MLST	"MLST\r\n"
/* Sets the transfer mode (Stream, Block, or Compressed) */
#define CMD_MODE	"MODE\r\n"
/* Returns a list of file names in a specified directory */
#define CMD_NLST	"NLST\r\n"
/* No operation (dummy packet; used mostly on keepalives) */
#define CMD_NOOP	"NOOP\r\n"
/* Select options for a feature (for example OPTS UTF8 ON) */
#define CMD_OPTS	"OPTS %s\r\n"
/* Authentication password */
#define CMD_PASS	"PASS %s\r\n"
/* Enter passive mode */
#define CMD_PASV	"PASV\r\n"
/* Protection Buffer Size*/
#define CMD_PBSZ	"PBSZ\r\n"
/* Specifies an address and port to which the server should connect */
#define CMD_PORT	"PORT\r\n"
/* Data Channel Protection Level */
#define CMD_PROT	"PROT\r\n"
/* Print working directory. Returns the current directory of the host */
#define CMD_PWD		"PWD\r\n"
/* Disconnect */
#define CMD_QUIT	"QUIT\r\n"
/* Re-initializes the connection*/
#define CMD_REIN	"REIN\r\n"
/* Restart transfer from the specified point */
#define CMD_REST	"REST\r\n"
/* Retrieve a copy of the file */
#define CMD_RETR	"RETR %s\r\n"
/* Remove a directory */
#define CMD_RMD		"RMD %s\r\n"
/* Remove a directory tree */
#define CMD_RMDA	"RMDA\r\n"
/* Rename from */
#define CMD_RNFR	"RNFR %s\r\n"
/* Rename to */
#define CMD_RNTO	"RNTO %s\r\n"
/* Sends site specific commands to remote server (like SITE IDLE 60 or
 * SITE UMASK 002). Inspect SITE HELP output for complete list of
 * supported commands
 */
#define CMD_SITE	"SITE %s\r\n"
/* Return the size of a file */
#define CMD_SIZE	"SIZE %s\r\n"
/* Mount file structure */
#define CMD_SMNT	"SMNT\r\n"
/* Use single port passive mode (only one TCP port number for both control
 * connections and passive-mode data connections)
 */
#define CMD_SPSV	"SPSV\r\n"
/* Returns information on the server status, including the status of the
 * current connection
 */
#define CMD_STAT	"STAT\r\n"
/* Accept the data and to store the data as a file at the server site */
#define CMD_STOR	"STOR %s\r\n"
/* Store file uniquely */
#define CMD_STOU	"STOU\r\n"
/* Set file transfer structure */
#define CMD_STRU	"STRU\r\n"
/* Return system type */
#define CMD_SYST	"SYST\r\n"
/* Get a thumbnail of a remote image file */
#define CMD_THMB	"THMB\r\n"
/* Sets the transfer mode (ASCII/Binary) */
#define CMD_TYPE_A	"TYPE A\r\n"
#define CMD_TYPE_I	"TYPE I\r\n"
/* Authentication username */
#define CMD_USER	"USER %s\r\n"
/* Change to the parent of the current working directory */
#define CMD_XCUP	"XCUP\r\n"
/* Make a directory */
#define CMD_XMKD	"XMKD\r\n"
/* Print the current working directory */
#define CMD_XPWD	"XPWD\r\n"
/* Select mail recipients */
#define CMD_XRCP	"XRCP"
/* Remove the directory */
#define CMD_XRMD	"XRMD\r\n"
/* Select mail scheme */
#define CMD_XRSQ	"XRSQ\r\n"
/* Send, mail if cannot */
#define CMD_XSEM	"XSEM\r\n"
/* Send to terminal */
#define CMD_XSEN	"XSEN\r\n"

#ifdef __cplusplus
}
#endif

#endif /* FTP_COMMANDS_ */

/**@} */
