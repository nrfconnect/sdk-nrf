/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/stats/stats.h>
#include <zephyr/sys/printk.h>
#include <tfm_ns_interface.h>
#include "psa/initial_attestation.h"
#include <tfm_ioctl_api.h>
#include <pm_config.h>
#include <ctype.h>

/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

void dump_hex_ascii(const uint8_t *data, size_t size)
{
	char ascii[17];
	size_t i, j;

	ascii[16] = '\0';

	printk("\n");
	printk("0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");

	for (i = 0; i < size; ++i) {
		printk("%02X ", ((unsigned char *)data)[i]);

		ascii[i % 16] = isprint(data[i]) ? data[i] : '.';

		if ((i + 1) % 8 == 0 || i + 1 == size) {
			printk(" ");
			if ((i + 1) % 16 == 0) {
				printk("|  %s\n", ascii);
			} else if ((i + 1) == size) {
				ascii[(i + 1) % 16] = '\0';

				if ((i + 1) % 16 <= 8) {
					printk(" ");
				}

				for (j = (i + 1) % 16; j < 16; ++j) {
					printk("   ");
				}
				printk("|  %s\n", ascii);
			}
		}
	}

	printk("\n");
}

static void get_fw_info_address(uint32_t fw_address)
{
	struct fw_info info;
	int err;

	err = tfm_platform_firmware_info(fw_address, &info);
	if (err) {
		printk("Failed to retrieve fw_info for address 0x%08x\n", fw_address);
		return;
	}

	printk("Magic: 0x");
	for (int i = 0; i < MAGIC_LEN_WORDS; i++) {
		printk("%08x", info.magic[i]);
	}
	printk("\n");

	printk("Total Size: %d\n", info.total_size);
	printk("Size: 0x%08x\n", info.size);
	printk("Version: %u\n", info.version);
	printk("Address: 0x%08x\n", info.address);
	printk("Boot address: 0x%08x\n", info.boot_address);
	printk("Valid: 0x%08x (CONFIG_FW_INFO_VALID_VAL=0x%08x)\n", info.valid,
	       CONFIG_FW_INFO_VALID_VAL);
}

static void get_fw_info(void)
{
	bool s0_active = false;
	int ret;

	ret = tfm_platform_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, &s0_active);
	if (ret != 0) {
		printk("Unexpected failure from tfm_platform_s0_active [%d]\n", ret);
	}

	printk("\nFW info S0:\n");
	get_fw_info_address(PM_S0_ADDRESS);

	printk("\nFW info S1:\n");
	get_fw_info_address(PM_S1_ADDRESS);

	printk("\nActive slot: %s\n", s0_active ? "S0" : "S1");
}

static void get_attestation_token(void)
{
	printk("\n");

	static uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
	int err;

	/* 64-byte challenge, encrypted using the default public key;
	 *
	 * 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
	 * 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
	 * 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
	 * 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
	 */
	static uint8_t challenge_buf[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_64] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC,
		0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
		0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
		0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	};

	size_t challenge_buf_size = sizeof(challenge_buf);
	size_t token_buf_size = sizeof(token_buf);
	size_t token_size;

	/* Request the initial attestation token w/the challenge data. */
	printk("Requesting initial attestation token with %u byte challenge.\n",
	       challenge_buf_size);
	err = psa_initial_attest_get_token(challenge_buf, challenge_buf_size, token_buf,
					   token_buf_size, &token_size);

	if (err) {
		printk("psa_initial_attest_get_token (err %d)\n", err);
	} else {
		printk("Received initial attestation token of %zu bytes.\n", token_size);

		dump_hex_ascii(token_buf, token_size);
	}
}

int main(void)
{
	int err;

	err = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32, "smp_svr_stats");
	if (err < 0) {
		printk("Error initializing stats system [%d]\n", err);
	}

	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	printk("build time: " __DATE__ " " __TIME__ "\n");

	get_fw_info();
	get_attestation_token();

	/* The system work queue handles all incoming mcumgr requests.  Let the
	 * main thread idle while the mcumgr server runs.
	 */
	while (1) {
		k_sleep(K_MSEC(1000));
		STATS_INC(smp_svr_stats, ticks);
	}
}
