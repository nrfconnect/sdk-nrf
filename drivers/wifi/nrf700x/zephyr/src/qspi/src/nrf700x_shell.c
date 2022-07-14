/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing RPU utils that can be invoked from shell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include "rpu_hw_if.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

#define SW_VER "1.6"

enum {
	SHELL_OK = 0,
	SHELL_FAIL = 1
};

// Convert to shell return values
static inline int shell_ret(int ret)
{
	if (ret)
		return SHELL_FAIL;
	else
		return SHELL_OK;
}


void print_memmap(const struct shell *shell)
{
	for (int i = 0; i < NUM_MEM_BLOCKS; i++) {
		shell_print(shell, " %-14s : 0x%06x - 0x%06x (%05d words)\n",
					blk_name[i],
					rpu_7002_memmap[i][0],
					rpu_7002_memmap[i][1],
					1 + ((rpu_7002_memmap[i][1] - rpu_7002_memmap[i][0]) >> 2)
					);
	}
}

static int cmd_write_wrd(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);

	return shell_ret(rpu_write(addr, &val, 4));
}

static int cmd_write_blk(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t pattern;
	uint32_t addr;
	uint32_t num_words;
	uint32_t offset;
	uint32_t *buff;
	int i;

	addr = strtoul(argv[1], NULL, 0);
	pattern = strtoul(argv[2], NULL, 0);
	offset = strtoul(argv[3], NULL, 0);
	num_words = strtoul(argv[4], NULL, 0);

	if (num_words > 2000) {
		shell_print(shell,
			    "Presently supporting block read/write only upto 2000 32-bit words");
		return SHELL_FAIL;
	}

	buff = (uint32_t *)k_malloc(num_words * 4);
	for (i = 0; i < num_words; i++) {
		buff[i] = pattern + i * offset;
	}

	if (!rpu_write(addr, buff, num_words * 4))
		return SHELL_FAIL;

	k_free(buff);

	return SHELL_OK;
}

static int cmd_read_wrd(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);
	if (rpu_read(addr, &val, 4))
		return SHELL_FAIL;

	shell_print(shell, "0x%08x\n", val);
	return SHELL_OK;
}

static int cmd_read_blk(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t *buff;
	uint32_t addr;
	uint32_t num_words;
	uint32_t rem;
	int i;

	addr = strtoul(argv[1], NULL, 0);
	num_words = strtoul(argv[2], NULL, 0);

	if (num_words > 2000) {
		shell_print(shell,
			    "Presently supporting block read/write only upto 2000 32-bit words");
		return SHELL_FAIL;
	}

	buff = (uint32_t *)k_malloc(num_words * 4);

	if (rpu_read(addr, buff, num_words * 4))
		return SHELL_FAIL;

	for (i = 0; i < num_words; i += 4) {
		rem = num_words - i;
		switch (rem) {
		case 1:
			shell_print(shell, "%08x", buff[i]);
			break;
		case 2:
			shell_print(shell, "%08x %08x", buff[i], buff[i + 1]);
			break;
		case 3:
			shell_print(shell, "%08x %08x %08x", buff[i], buff[i + 1], buff[i + 2]);
			break;
		default:
			shell_print(shell, "%08x %08x %08x %08x", buff[i], buff[i + 1], buff[i + 2],
				    buff[i + 3]);
			break;
		}
	}

	k_free(buff);
	return SHELL_OK;
}

static int cmd_memtest(const struct shell *shell, size_t argc, char **argv)
{
	/* $write_blk 0xc0000 0xdeadbeef 16 */
	uint32_t pattern;
	uint32_t addr;
	uint32_t num_words;
	uint32_t offset;
	uint32_t *buff, *rxbuff;
	int i;

	addr = strtoul(argv[1], NULL, 0);
	pattern = strtoul(argv[2], NULL, 0);
	offset = strtoul(argv[3], NULL, 0);
	num_words = strtoul(argv[4], NULL, 0);

	buff = (uint32_t *)k_malloc(2000 * 4);
	rxbuff = (uint32_t *)k_malloc(2000 * 4);

	int32_t rem_words = num_words;
	uint32_t test_chunk, chunk_no = 0;

	while (rem_words > 0) {
		test_chunk = (rem_words < 2000) ? rem_words : 2000;

		for (i = 0; i < test_chunk; i++) {
			buff[i] = pattern + (i + chunk_no * 2000) * offset;
		}

		if (rpu_write(addr, buff, test_chunk * 4) ||
			rpu_read(addr, rxbuff, test_chunk * 4)) {
			goto err;
		}

		if (memcmp(buff, rxbuff, test_chunk * 4) != 0) {
			goto err;
		}
		rem_words -= 2000;
		chunk_no++;
	}
	shell_print(shell, "memtest PASSED");
	k_free(rxbuff);
	k_free(buff);

	return SHELL_OK;
err:
	shell_print(shell, "memtest failed");
	k_free(rxbuff);
	k_free(buff);
	return SHELL_FAIL;
}

static int cmd_sleep_stats(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t addr;
	uint32_t wrd_len;
	uint32_t *buff;

	addr = strtoul(argv[1], NULL, 0);
	wrd_len = strtoul(argv[2], NULL, 0);

	buff = (uint32_t *)k_malloc(wrd_len * 4);

	rpu_get_sleep_stats(addr, buff, wrd_len);

	for (int i = 0; i < wrd_len; i++) {
		shell_print(shell, "0x%08x\n", buff[i]);
	}

	k_free(buff);
	return SHELL_OK;
}

static int cmd_gpio_config(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_gpio_config());
}

static int cmd_pwron(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_pwron());
}

static int cmd_qspi_config(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t qspi_spim_freq_MHz;
	uint32_t qspi_spim_latency;
	uint32_t mem_block;

	qspi_spim_freq_MHz = strtoul(argv[1], NULL, 0);
	mem_block = strtoul(argv[2], NULL, 0);
	qspi_spim_latency = strtoul(argv[3], NULL, 0);

	return shell_ret(rpu_qspi_config(qspi_spim_freq_MHz, qspi_spim_latency, mem_block));
}

static int cmd_qspi_init(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_qspi_init());
}

static int cmd_rpuwake(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_wakeup());
}

static int cmd_wrsr2(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_wrsr2(strtoul(argv[1], NULL, 0) & 0xff));
}

static int cmd_rdsr2(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t val = rpu_rdsr2();

	shell_print(shell, "RDSR2 = 0x%x\n", val);

	return SHELL_OK;
}

static int cmd_rdsr1(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t val = rpu_rdsr1();

	shell_print(shell, "RDSR1 = 0x%x\n", val);

	return SHELL_OK;
}

static int cmd_rpuclks_on(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_clks_on());
}

static int cmd_wifi_on(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_enable());
}

static int cmd_wifi_off(const struct shell *shell, size_t argc, char **argv)
{
	return shell_ret(rpu_disable());
}

static int cmd_memmap(const struct shell *shell, size_t argc, char **argv)
{
	print_memmap(shell);
	return SHELL_OK;
}

static void cmd_help(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Supported commands....  ");
	shell_print(shell, "=========================  ");
	shell_print(shell, "uart:~$ nrf700x_shell read_wrd    <address> ");
	shell_print(shell, "         ex: $ nrf700x_shell read_wrd 0x0c0000");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell write_wrd   <address> <data>");
	shell_print(shell, "         ex: $ nrf700x_shell write_wrd 0x0c0000 0xabcd1234");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell read_blk    <address> <num_words>");
	shell_print(shell, "         ex: $ nrf700x_shell read_blk 0x0c0000 64");
	shell_print(shell, "         Note - num_words can be a maximum of 2000");
	shell_print(shell, "  ");
	shell_print(
		shell,
		"uart:~$ nrf700x_shell write_blk   <address> <start_pattern> <pattern_increment> <num_words>");
	shell_print(shell, "         ex: $ nrf700x_shell write_blk 0x0c0000 0xaaaa5555 0 64");
	shell_print(
		shell,
		"         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000");
	shell_print(shell, "         ex: $ nrf700x_shell write_blk 0x0c0000 0x0 1 64");
	shell_print(
		shell,
		"         This writes pattern 0x0, 0x1,0x2,0x3....etc to 64 locations starting from 0x0c0000");
	shell_print(shell, "         Note - num_words can be a maximum of 2000");
	shell_print(shell, "  ");
	shell_print(
		shell,
		"uart:~$ nrf700x_shell memtest   <address> <start_pattern> <pattern_increment> <num_words>");
	shell_print(shell, "         ex: $ nrf700x_shell memtest 0x0c0000 0xaaaa5555 0 64");
	shell_print(
		shell,
		"         This writes pattern 0xaaaa5555 to 64 locations starting from 0x0c0000,");
	shell_print(shell, "         reads them back and validates them");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell wifi_on  ");
#if CONFIG_NRF700X_ON_QSPI
	shell_print(shell, "         - Configures all gpio pins ");
	shell_print(
		shell,
		"         - Writes 1 to BUCKEN (P0.12), waits for 2ms and then writes 1 to IOVDD Control (P0.31) ");
	shell_print(shell, "         - Initializes qspi interface and wakes up RPU");
	shell_print(shell, "         - Enables all gated RPU clocks");
#else
	shell_print(shell, "         - Configures all gpio pins ");
	shell_print(
		shell,
		"         - Writes 1 to BUCKEN (P1.01), waits for 2ms and then writes 1 to IOVDD Control (P1.00) ");
	shell_print(shell, "         - Initializes qspi interface and wakes up RPU");
	shell_print(shell, "         - Enables all gated RPU clocks");
#endif
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell wifi_off ");
#if CONFIG_NRF700X_ON_QSPI
	shell_print(
		shell,
		"         This writes 0 to IOVDD Control (P0.31) and then writes 0 to BUCKEN Control (P0.12)");
#else
	shell_print(
		shell,
		"         This writes 0 to IOVDD Control (P1.00) and then writes 0 to BUCKEN Control (P1.01)");
#endif
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell sleep_stats ");
	shell_print(shell,
		    "         This continuously does the RPU sleep/wake cycle and displays stats ");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell gpio_config ");
#if CONFIG_NRF700X_ON_QSPI
	shell_print(
		shell,
		"         Configures BUCKEN(P0.12) as o/p, IOVDD control (P0.31) as output and HOST_IRQ (P0.23) as input");
	shell_print(shell, "         and interruptible with a ISR hooked to it");
#else
	shell_print(
		shell,
		"         Configures BUCKEN(P1.01) as o/p, IOVDD control (P1.00) as output and HOST_IRQ (P1.09) as input");
	shell_print(shell, "         and interruptible with a ISR hooked to it");
#endif
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell qspi_init ");
	shell_print(shell, "         Initializes QSPI driver functions ");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell pwron ");
	shell_print(shell, "         Sets BUCKEN=1, delay, IOVDD cntrl=1 ");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell rpuwake ");
	shell_print(shell, "         Wakeup RPU: Write 0x1 to WRSR2 register");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell rpuclks_on ");
	shell_print(
		shell,
		"         Enables all gated RPU clocks. Only SysBUS and PKTRAM will work w/o this setting enabled");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell wrsr2 <val> ");
	shell_print(shell, "         writes <val> (0/1) to WRSR2 reg - takes LSByte of <val>");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell rdsr1 ");
	shell_print(shell, "         Reads RDSR1 Register");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell rdsr2 ");
	shell_print(shell, "         Reads RDSR2 Register");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell trgirq ");
	shell_print(shell, "         Generates IRQ interrupt to host");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell clrirq ");
	shell_print(shell, "         Clears host IRQ generated interrupt");
	shell_print(shell, "  ");
	shell_print(shell,
		    "uart:~$ nrf700x_shell config  <qspi/spi Freq> <mem_block_num> <read_latency>");
	shell_print(shell, "         QSPI/SPI clock freq in MHz : 4/8/16 etc");
	shell_print(shell, "         block num as per memmap (starting from 0) : 0-10");
	shell_print(shell, "         QSPI/SPIM read latency for the selected block : 0-255");
	shell_print(
		shell,
		"         NOTE: need to do a wifi_off and wifi_on for these changes to take effect");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell ver ");
	shell_print(shell, "         Display SW version and other details of the hex file ");
	shell_print(shell, "  ");
	shell_print(shell, "uart:~$ nrf700x_shell help ");
	shell_print(shell, "         Lists all commands with usage example(s) ");
	shell_print(shell, "  ");
}

static int cmd_ver(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "nrf700x_shell Version: %s", SW_VER);
#if CONFIG_NRF700X_ON_QSPI
	shell_print(shell, "Build for QSPI interface on nRF7002 board");
#else
	shell_print(shell,
		    "Build for SPIM interface on nRF7002EK+nRF5340DK connected via arduino header");
#endif
	return SHELL_OK;
}

static int cmd_trgirq(const struct shell *shell, size_t argc, char **argv)
{
	int i;
	static const uint32_t irq_regs[][2] = {
		{0x400, 0x20000},
		{0x494, 0x80000000},
		{0x484, 0x7fff7bee}
	};

	shell_print(shell, "Asserting IRQ to HOST");

	for (i = 0; i < ARRAY_SIZE(irq_regs); i++) {
		if (rpu_write(irq_regs[i][0], (const uint32_t *) irq_regs[i][1], 4)) {
			return SHELL_FAIL;
		}
	}

	return SHELL_OK;
}

static int cmd_clrirq(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "de-asserting IRQ to HOST");

	return shell_ret(rpu_write(0x488, (uint32_t *)0x80000000, 4));
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_nrf700x_shell,
	SHELL_CMD(write_blk, NULL,
		  "Writes a block of words to Sheliak host memory via QSPI interface",
		  cmd_write_blk),
	SHELL_CMD(read_blk, NULL,
		  "Reads a block of words from Sheliak host memory via QSPI interface",
		  cmd_read_blk),
	SHELL_CMD(write_wrd, NULL, "Writes a word to Sheliak host memory via QSPI interface",
		  cmd_write_wrd),
	SHELL_CMD(read_wrd, NULL, "Reads a word from Sheliak host memory via QSPI interface",
		  cmd_read_wrd),
	SHELL_CMD(wifi_on, NULL, "BUCKEN-IOVDD power ON", cmd_wifi_on),
	SHELL_CMD(wifi_off, NULL, "BUCKEN-IOVDD power OFF", cmd_wifi_off),
	SHELL_CMD(sleep_stats, NULL, "Tests Sleep/Wakeup cycles", cmd_sleep_stats),
	SHELL_CMD(gpio_config, NULL, "Configure all GPIOs", cmd_gpio_config),
	SHELL_CMD(qspi_init, NULL, "Initialize QSPI driver functions", cmd_qspi_init),
	SHELL_CMD(pwron, NULL, "BUCKEN=1, delay, IOVDD=1", cmd_pwron),
	SHELL_CMD(rpuwake, NULL, "Wakeup RPU: Write 0x1 to WRSR2 reg", cmd_rpuwake),
	SHELL_CMD(rpuclks_on, NULL, "Enable all RPU gated clocks", cmd_rpuclks_on),
	SHELL_CMD(wrsr2, NULL, "Write to WRSR2 register", cmd_wrsr2),
	SHELL_CMD(rdsr1, NULL, "Read RDSR1 register", cmd_rdsr1),
	SHELL_CMD(rdsr2, NULL, "Read RDSR2 register", cmd_rdsr2),
	SHELL_CMD(trgirq, NULL, "Generates IRQ interrupt to HOST", cmd_trgirq),
	SHELL_CMD(clrirq, NULL, "Clears generated Host IRQ interrupt", cmd_clrirq),
	SHELL_CMD(config, NULL, "Runtime config of SCK, Freq and latency for QSPI/SPIM",
		  cmd_qspi_config),
	SHELL_CMD(memmap, NULL, "Gives the full memory map of the Sheliak chip", cmd_memmap),
	SHELL_CMD(memtest, NULL, "Writes, reads back and validates specified memory on Seliak chip",
		  cmd_memtest),
	SHELL_CMD(ver, NULL, "Display SW version of the hex file", cmd_ver),
	SHELL_CMD(help, NULL, "Help with all supported commmands", cmd_help), SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "nrf700x_shell" */
SHELL_CMD_REGISTER(nrf700x_shell, &sub_nrf700x_shell, "nrf700x_shell commands", NULL);
