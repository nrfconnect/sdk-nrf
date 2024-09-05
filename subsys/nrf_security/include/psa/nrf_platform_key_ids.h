/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * Nordic platform specific key identifiers.
 */

#ifndef NRF_PLATFORM_KEY_IDS_H
#define NRF_PLATFORM_KEY_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

#define HALTIUM_PLATFORM_PSA_KEY_ID(access, domain, usage, generation)                             \
	((0x4 << 28) | ((access & 0xF) << 24) | ((domain & 0xFF) << 16) | ((usage & 0xFF) << 8) |  \
	 (generation & 0xF))

/* Key access rights */
#define ACCESS_INTERNAL 0x0
#define ACCESS_LOCAL	0x1

/* Domains */
#define DOMAIN_NONE	   0x00
#define DOMAIN_SECURE	   0x01
#define DOMAIN_APPLICATION 0x02
#define DOMAIN_RADIO	   0x03
#define DOMAIN_CELL	   0x04
#define DOMAIN_ISIM	   0x05
#define DOMAIN_WIFI	   0x06
#define DOMAIN_SYSCTRL	   0x08

/* Key usage */
#define USAGE_IAK	   0x01
#define USAGE_MKEK	   0x02
#define USAGE_MEXT	   0x03
#define USAGE_UROTENC	   0x10
#define USAGE_UROTPUBKEY   0x11
#define USAGE_UROTRECOVERY 0x12
#define USAGE_AUTHDEBUG	   0x13
#define USAGE_AUTHOP	   0x14
#define USAGE_FWENC	   0x20
#define USAGE_PUBKEY	   0x21
#define USAGE_STMTRACE	   0x25
#define USAGE_COREDUMP	   0x26
#define USAGE_RMOEM	   0xAA
#define USAGE_RMNORDIC	   0xBB

/* KeyIDs used by SDFW for Identity Attestation Keys (IAKs) */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: IAK (0x01),
 * Generation: 1 (0x0)
 */
#define IAK_SECDOM_GEN1 0x40010100

/* Class: Platform (0x4), Access: Local (0x1),    Domain: APPLICATION (0x02), Usage: IAK (0x01),
 * Generation: 1 (0x0)
 */
#define IAK_APPLICATION_GEN1 0x41020100

/* Class: Platform (0x4), Access: Local (0x1),    Domain: RADIO (0x03),       Usage: IAK (0x01),
 * Generation: 1 (0x0)
 */
#define IAK_RADIO_GEN1 0x41030100

/* Class: Platform (0x4), Access: Local (0x1),    Domain: CELL (0x04),        Usage: IAK (0x01),
 * Generation: 1 (0x0)
 */
#define IAK_CELL_GEN1 0x41040100

/* Class: Platform (0x4), Access: Local (0x1),    Domain: WIFI (0x06),        Usage: IAK (0x01),
 * Generation: 1 (0x0)
 */
#define IAK_WIFI_GEN1 0x41060100

/* KeyIDs used by SDFW for Master Key Encryption Keys (MKEKs) */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: MKEK (0x02),
 * Generation: 1 (0x0)
 */
#define MKEK_SECDOM_GEN1 0x40010200

/* Class: Platform (0x4), Access: Internal (0x0), Domain: APPLICATION (0x02), Usage: MKEK (0x02),
 * Generation: 1 (0x0)
 */
#define MKEK_APPLICATION_GEN1 0x40020200

/* Class: Platform (0x4), Access: Internal (0x0), Domain: RADIO (0x03),       Usage: MKEK (0x02),
 * Generation: 1 (0x0)
 */
#define MKEK_RADIO_GEN1 0x40030200

/* Class: Platform (0x4), Access: Internal (0x0), Domain: CELL (0x04),        Usage: MKEK (0x02),
 * Generation: 1 (0x0)
 */
#define MKEK_CELL_GEN1 0x40040200

/* Class: Platform (0x4), Access: Internal (0x0), Domain: WIFI (0x06),        Usage: MKEK (0x02),
 * Generation: 1 (0x0)
 */
#define MKEK_WIFI_GEN1 0x40060200

/* KeyIDs used by SDFW for Master External Storage Keys (MEXTs) */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: MEXT (0x03),
 * Generation: 1 (0x0)
 */
#define MEXT_SECDOM_GEN1 0x40010300

/* Class: Platform (0x4), Access: Internal (0x0), Domain: APPLICATION (0x02), Usage: MEXT (0x03),
 * Generation: 1 (0x0)
 */
#define MEXT_APPLICATION_GEN1 0x40020300

/* Class: Platform (0x4), Access: Internal (0x0), Domain: RADIO (0x03),       Usage: MEXT (0x03),
 * Generation: 1 (0x0)
 */
#define MEXT_RADIO_GEN1 0x40030300

/* Class: Platform (0x4), Access: Internal (0x0), Domain: CELL (0x04),        Usage: MEXT (0x03),
 * Generation: 1 (0x0)
 */
#define MEXT_CELL_GEN1 0x40040300

/* Class: Platform (0x4), Access: Internal (0x0), Domain: WIFI (0x06),        Usage: MEXT (0x03),
 * Generation: 1 (0x0)
 */
#define MEXT_WIFI_GEN1 0x40060300

/* KeyIDs used by SDFW for IETF SUIT manifest verification */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: NONE (0x00),        Usage:
 * MANIFEST_OEM_ROOT (0xAA),   Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_OEM_ROOT_GEN1 0x4000AA00
#define MANIFEST_PUBKEY_OEM_ROOT_GEN2 0x4000AA01
#define MANIFEST_PUBKEY_OEM_ROOT_GEN3 0x4000AA02
#define MANIFEST_PUBKEY_OEM_ROOT_GEN4 0x4000AA03

/* Class: Platform (0x4), Access: Internal (0x0), Domain: NONE (0x00),        Usage:
 * MANIFEST_NORDIC_TOP (0xBB), Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_NRF_TOP_GEN1 0x4000BB00
#define MANIFEST_PUBKEY_NRF_TOP_GEN2 0x4000BB01
#define MANIFEST_PUBKEY_NRF_TOP_GEN3 0x4000BB02
#define MANIFEST_PUBKEY_NRF_TOP_GEN4 0x4000BB03

/* KeyIDs used by SDFW for IETF SUIT secure boot of local domain FW */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: UROTPUBKEY
 * (0x11),   Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_SECURE_GEN1 0x40011100
#define MANIFEST_PUBKEY_SECURE_GEN2 0x40011101
#define MANIFEST_PUBKEY_SECURE_GEN3 0x40011102
#define MANIFEST_PUBKEY_SECURE_GEN4 0x40011103

/* Class: Platform (0x4), Access: Internal (0x0), Domain: APPLICATION (0x02), Usage: PUBKEY (0x21),
 * Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_APPLICATION_GEN1 0x40022100
#define MANIFEST_PUBKEY_APPLICATION_GEN2 0x40022101
#define MANIFEST_PUBKEY_APPLICATION_GEN3 0x40022102
#define MANIFEST_PUBKEY_APPLICATION_GEN4 0x40022103

/* Class: Platform (0x4), Access: Internal (0x0), Domain: RADIOCORE (0x03),   Usage: PUBKEY (0x21),
 * Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_RADIO_GEN1 0x40032100
#define MANIFEST_PUBKEY_RADIO_GEN2 0x40032101
#define MANIFEST_PUBKEY_RADIO_GEN3 0x40032102
#define MANIFEST_PUBKEY_RADIO_GEN4 0x40032103

/* Class: Platform (0x4), Access: Internal (0x0), Domain: CELL (0x04),        Usage: PUBKEY (0x21),
 * Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_CELL_GEN1 0x40042100
#define MANIFEST_PUBKEY_CELL_GEN2 0x40042101
#define MANIFEST_PUBKEY_CELL_GEN3 0x40042102
#define MANIFEST_PUBKEY_CELL_GEN4 0x40042103

/* Class: Platform (0x4), Access: Internal (0x0), Domain: WIFI (0x06),        Usage: PUBKEY (0x21),
 * Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_WIFI_GEN1 0x40062100
#define MANIFEST_PUBKEY_WIFI_GEN2 0x40062101
#define MANIFEST_PUBKEY_WIFI_GEN3 0x40062102
#define MANIFEST_PUBKEY_WIFI_GEN4 0x40062103

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SYSCTRL (0x08),     Usage: PUBKEY (0x21),
 * Generation: 1-4 (0x0-0x3)
 */
#define MANIFEST_PUBKEY_SYSCTRL_GEN1 0x40082100
#define MANIFEST_PUBKEY_SYSCTRL_GEN2 0x40082101
#define MANIFEST_PUBKEY_SYSCTRL_GEN3 0x40082102
#define MANIFEST_PUBKEY_SYSCTRL_GEN4 0x40082103

/* KeyIDs used by SDFW for SUIT manifest decryption of local domain FW */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: UROTENC (0x10),
 * Generation: 1-2 (0x0-0x1)
 */
#define UROTENC_SECURE_GEN1 0x40011000
#define UROTENC_SECURE_GEN2 0x40011001

/* Class: Platform (0x4), Access: Internal (0x0), Domain: APPLICATION (0x02), Usage: FWENC (0x20),
 * Generation: 1-2 (0x0-0x1)
 */
#define FWENC_APPLICATION_GEN1 0x40022000
#define FWENC_APPLICATION_GEN2 0x40022001

/* Class: Platform (0x4), Access: Internal (0x0), Domain: RADIO (0x03),       Usage: FWENC (0x20),
 * Generation: 1-2 (0x0-0x1)
 */
#define FWENC_RADIO_GEN1 0x40032000
#define FWENC_RADIO_GEN2 0x40032001

/* Class: Platform (0x4), Access: Internal (0x0), Domain: CELL (0x04),        Usage: FWENC (0x20),
 * Generation: 1-2 (0x0-0x1)
 */
#define FWENC_CELL_GEN1 0x40042000
#define FWENC_CELL_GEN2 0x40042001

/* Class: Platform (0x4), Access: Internal (0x0), Domain: WIFI (0x06),        Usage: FWENC (0x20),
 * Generation: 1-2 (0x0-0x1)
 */
#define FWENC_WIFI_GEN1 0x40062000
#define FWENC_WIFI_GEN2 0x40062001

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SYSCTRL (0x08),     Usage: FWENC (0x20),
 * Generation: 1-2 (0x0-0x1)
 */
#define FWENC_SYSCTRL_GEN1 0x40082000
#define FWENC_SYSCTRL_GEN2 0x40082001

/* KeyIDs used by SDFW for temporarily re-enabling local domain debug access port using the Arm ADAC
 * protocol
 */

/* Class: Platform (0x4), Access: Internal (0x0), Domain: SECURE (0x01),      Usage: AUTHDEBUG
 * (0x23), Generation: 1-4 (0x0-0x3)
 */
#define AUTHDEBUG_SECURE_GEN1 0x40012300
#define AUTHDEBUG_SECURE_GEN2 0x40012301
#define AUTHDEBUG_SECURE_GEN3 0x40012302
#define AUTHDEBUG_SECURE_GEN4 0x40012303

/* Class: Platform (0x4), Access: Internal (0x0), Domain: APPLICATION (0x02), Usage: AUTHDEBUG
 * (0x23), Generation: 1-4 (0x0-0x3)
 */
#define AUTHDEBUG_APPLICATION_GEN1 0x40022300
#define AUTHDEBUG_APPLICATION_GEN2 0x40022301
#define AUTHDEBUG_APPLICATION_GEN3 0x40022302
#define AUTHDEBUG_APPLICATION_GEN4 0x40022303

/* Class: Platform (0x4), Access: Internal (0x0), Domain: RADIO (0x03),       Usage: AUTHDEBUG
 * (0x23), Generation: 1-4 (0x0-0x3)
 */
#define AUTHDEBUG_RADIO_GEN1 0x40032300
#define AUTHDEBUG_RADIO_GEN2 0x40032301
#define AUTHDEBUG_RADIO_GEN3 0x40032302
#define AUTHDEBUG_RADIO_GEN4 0x40032303

/* Class: Platform (0x4), Access: Internal (0x0), Domain: CELL (0x04),        Usage: AUTHDEBUG
 * (0x23), Generation: 1-4 (0x0-0x3)
 */
#define AUTHDEBUG_CELL_GEN1 0x40042300
#define AUTHDEBUG_CELL_GEN2 0x40042301
#define AUTHDEBUG_CELL_GEN3 0x40042302
#define AUTHDEBUG_CELL_GEN4 0x40042303

/* Class: Platform (0x4), Access: Internal (0x0), Domain: WIFI (0x06),        Usage: AUTHDEBUG
 * (0x23), Generation: 1-4 (0x0-0x3)
 */
#define AUTHDEBUG_WIFI_GEN1 0x40062300
#define AUTHDEBUG_WIFI_GEN2 0x40062301
#define AUTHDEBUG_WIFI_GEN3 0x40062302
#define AUTHDEBUG_WIFI_GEN4 0x40062303

/* KeyIDs used by local domains to encrypt their STM trace data. */

/* Class: Platform (0x4), Access: Local (0x1), Domain: SECURE (0x01),      Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_SECURE_GEN1 0x41012500
#define STMTRACE_SECURE_GEN2 0x41012501

/* Class: Platform (0x4), Access: Local (0x1), Domain: APPLICATION (0x02), Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_APPLICATION_GEN1 0x41022500
#define STMTRACE_APPLICATION_GEN2 0x41022501

/* Class: Platform (0x4), Access: Local (0x1), Domain: RADIO (0x03),       Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_RADIO_GEN1 0x41032500
#define STMTRACE_RADIO_GEN2 0x41032501

/* Class: Platform (0x4), Access: Local (0x1), Domain: CELL (0x04),        Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_CELL_GEN1 0x41042500
#define STMTRACE_CELL_GEN2 0x41042501

/* Class: Platform (0x4), Access: Local (0x1), Domain: WIFI (0x06),        Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_WIFI_GEN1 0x41062500
#define STMTRACE_WIFI_GEN2 0x41062501

/* Class: Platform (0x4), Access: Local (0x1), Domain: SYSCTRL (0x08),     Usage: STMTRACE (0x25),
 * Generation: 1-2 (0x0-0x1)
 */
#define STMTRACE_SYSCTRL_GEN1 0x41082500
#define STMTRACE_SYSCTRL_GEN2 0x41082501

/* KeyIDs used by local domains to encrypt their coredump data from inside their fault handler. */

/* Class: Platform (0x4), Access: Local (0x1), Domain: SECURE (0x01),      Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_SECURE_GEN1 0x41012600
#define COREDUMP_SECURE_GEN2 0x41012601

/* Class: Platform (0x4), Access: Local (0x1), Domain: APPLICATION (0x02), Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_APPLICATION_GEN1 0x41022600
#define COREDUMP_APPLICATION_GEN2 0x41022601

/* Class: Platform (0x4), Access: Local (0x1), Domain: RADIO (0x03),       Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_RADIO_GEN1 0x41032600
#define COREDUMP_RADIO_GEN2 0x41032601

/* Class: Platform (0x4), Access: Local (0x1), Domain: CELL (0x04),        Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_CELL_GEN1 0x41042600
#define COREDUMP_CELL_GEN2 0x41042601

/* Class: Platform (0x4), Access: Local (0x1), Domain: WIFI (0x06),        Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_WIFI_GEN1 0x41062600
#define COREDUMP_WIFI_GEN2 0x41062601

/* Class: Platform (0x4), Access: Local (0x1), Domain: SYSCTRL (0x08),     Usage: COREDUMP (0x26),
 * Generation: 1-2 (0x0-0x1)
 */
#define COREDUMP_SYSCTRL_GEN1 0x41082600
#define COREDUMP_SYSCTRL_GEN2 0x41082601

#ifdef __cplusplus
}
#endif

#endif /* NRF_PLATFORM_KEY_IDS_H */
