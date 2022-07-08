/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <devicetree.h>
#include <shell/shell.h>

#include "qspi_if.h"
#include "spi_if.h"

#define SLEEP_TIME_MS 2

#include <logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF_LOG_LEVEL);

#define NRF7002_NODE DT_NODELABEL(nrf7002)

static const struct gpio_dt_spec iovdd_ctrl_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, iovdd_ctrl_gpios);

static const struct gpio_dt_spec bucken_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, bucken_gpios);

static const struct gpio_dt_spec host_irq_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, host_irq_gpios);

#define SW_VER "1.6"

const struct device *gpio_dev;
static bool hl_flag;
static int selected_blk;

extern struct qspi_config *qspi_config;
const struct qspi_dev *qdev;
struct qspi_config *cfg;

enum { SysBus = 0,
	ExtSysBus,
	PBus,
	PKTRAM,
	GRAM,
	LMAC_ROM,
	LMAC_RET_RAM,
	LMAC_SRC_RAM,
	UMAC_ROM,
	UMAC_RET_RAM,
	UMAC_SRC_RAM,
	NUM_MEM_BLOCKS
};

static char blk_name[][15] = { "SysBus",   "ExtSysBus",	   "PBus",	   "PKTRAM",
			       "GRAM",	   "LMAC_ROM",	   "LMAC_RET_RAM", "LMAC_SRC_RAM",
			       "UMAC_ROM", "UMAC_RET_RAM", "UMAC_SRC_RAM" };

static uint32_t shk_memmap[][3] = { { 0x000000, 0x008FFF, 1 }, { 0x009000, 0x03FFFF, 2 },
				    { 0x040000, 0x07FFFF, 1 }, { 0x0C0000, 0x0F0FFF, 0 },
				    { 0x080000, 0x092000, 1 }, { 0x100000, 0x134000, 1 },
				    { 0x140000, 0x14C000, 1 }, { 0x180000, 0x190000, 1 },
				    { 0x200000, 0x261800, 1 }, { 0x280000, 0x2A4000, 1 },
				    { 0x300000, 0x338000, 1 } };

struct gpio_callback irq_callback_data;

void irq_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("\n !!! IRQ Hit !!!!\n");
}

void print_memmap(void)
{
	for (int i = 0; i < NUM_MEM_BLOCKS; i++) {
		printk(" %-14s : 0x%06x - 0x%06x (%05d words)\n", blk_name[i], shk_memmap[i][0],
		       shk_memmap[i][1], 1 + ((shk_memmap[i][1] - shk_memmap[i][0]) >> 2));
	}
}

static int validate_addr_blk(uint32_t start_addr, uint32_t end_addr, uint32_t block_no)
{
	if (((start_addr >= shk_memmap[block_no][0]) && (start_addr <= shk_memmap[block_no][1])) &&
	    ((end_addr >= shk_memmap[block_no][0]) && (end_addr <= shk_memmap[block_no][1]))) {
		if (block_no == PKTRAM) {
			hl_flag = 0;
		}
		selected_blk = block_no; /* Save the selected block no */
		return 1;
	}

	return 0;
}

static int validate_addr(uint32_t start_addr, uint32_t len)
{
	int ret;
	uint32_t end_addr;

	end_addr = start_addr + len - 1;

	hl_flag = 1;

	ret = validate_addr_blk(start_addr, end_addr, 0);
	ret |= validate_addr_blk(start_addr, end_addr, 1);
	ret |= validate_addr_blk(start_addr, end_addr, 2);
	ret |= validate_addr_blk(start_addr, end_addr, 3);
	ret |= validate_addr_blk(start_addr, end_addr, 4);
	ret |= validate_addr_blk(start_addr, end_addr, 5);
	ret |= validate_addr_blk(start_addr, end_addr, 6);
	ret |= validate_addr_blk(start_addr, end_addr, 7);
	ret |= validate_addr_blk(start_addr, end_addr, 8);
	ret |= validate_addr_blk(start_addr, end_addr, 9);
	ret |= validate_addr_blk(start_addr, end_addr, 10);

	if (!ret) {
		LOG_DBG("Address validation failed - pls check memmory map and re-try\n");
		print_memmap();
		return 0;
	}

	cfg->qspi_slave_latency = (hl_flag) ? shk_memmap[selected_blk][2] : 0;

	return 1;
}

static int cmd_write_wrd(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);

	if (!validate_addr(addr, 1))
		return -1;

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		shell_print(shell, "Error... Cannot write to ROM blocks");
	}

	qdev->write(addr, &val, 4);
	/* shell_print(shell, "Written 0x%08x to 0x%08x\n", val, addr); */

	return 0;
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
		return -1;
	}

	if (!validate_addr(addr, num_words * 4))
		return -1;

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		shell_print(shell, "Error... Cannot write to ROM blocks");
	}

	buff = (uint32_t *)k_malloc(num_words * 4);
	for (i = 0; i < num_words; i++) {
		buff[i] = pattern + i * offset;
		/* LOG_DBG("%08x\n", buff[i]); */
	}

	qdev->write(addr, buff, num_words * 4);

	k_free(buff);

	return 0;
}

static int cmd_read_wrd(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);
	if (!validate_addr(addr, 1))
		return -1;

	/* shell_print(shell, "hl_read = %d",(int) hl_flag); */
	/* LOG_DBG("QSPI/SPIM latency = %d\n",cfg->qspi_slave_latency); */
	(hl_flag) ? qdev->hl_read(addr, &val, 4) : qdev->read(addr, &val, 4);

	/* shell_print(shell, "addr = 0x%08x Read val = 0x%08x\n",addr, val); */
	shell_print(shell, "0x%08x\n", val);
	return 0;
}

static int cmd_read_blk(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t *buff;
	uint32_t addr;
	uint32_t num_words;
	uint32_t rem;
	int i;

	/* $read_blk 0xc0000 16 */

	addr = strtoul(argv[1], NULL, 0);
	num_words = strtoul(argv[2], NULL, 0);

	if (num_words > 2000) {
		shell_print(shell,
			    "Presently supporting block read/write only upto 2000 32-bit words");
		return -1;
	}

	if (!validate_addr(addr, num_words * 4))
		return -1;

	/* LOG_DBG("QSPI/SPIM latency = %d\n",cfg->qspi_slave_latency); */

	buff = (uint32_t *)k_malloc(num_words * 4);

	/* shell_print(shell, "hl_read = %d",(int) hl_flag); */
	(hl_flag) ? qdev->hl_read(addr, buff, num_words * 4) :
		    qdev->read(addr, buff, num_words * 4);

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

	return 0;
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

	if (!validate_addr(addr, num_words * 4))
		return -1;

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		shell_print(shell, "Error... Cannot write to ROM blocks");
	}

	/* LOG_DBG("QSPI/SPIM latency = %d\n",cfg->qspi_slave_latency); */

	buff = (uint32_t *)k_malloc(2000 * 4);
	rxbuff = (uint32_t *)k_malloc(2000 * 4);

	int32_t rem_words = num_words;
	uint32_t test_chunk, chunk_no = 0;

	while (rem_words > 0) {
		test_chunk = (rem_words < 2000) ? rem_words : 2000;

		for (i = 0; i < test_chunk; i++) {
			buff[i] = pattern + (i + chunk_no * 2000) * offset;
			/* LOG_DBG("%08x\n", buff[i]); */
		}

		qdev->write(addr, buff, test_chunk * 4);

		(hl_flag) ? qdev->hl_read(addr, rxbuff, test_chunk * 4) :
			    qdev->read(addr, rxbuff, test_chunk * 4);

		if (memcmp(buff, rxbuff, test_chunk * 4) != 0) {
			shell_print(shell, "memtest failed");
			k_free(rxbuff);
			k_free(buff);
			return -1;
		}
		rem_words -= 2000;
		chunk_no++;
	}
	shell_print(shell, "memtest PASSED");
	k_free(rxbuff);
	k_free(buff);

	return 0;
}

void get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len)
{
#if CONFIG_NRF700X_ON_QSPI
	qspi_cmd_wakeup_rpu(&qspi_perip, 0x1);
	LOG_DBG("Waiting for RPU awake...\n");

	qspi_validate_rpu_wake_writecmd(&qspi_perip);
	LOG_DBG("exited qspi_validate_rpu_wake_writecmd(). Waiting for rpu_awake.....\n");

	qspi_wait_while_firmware_awake(&qspi_perip);
	LOG_DBG("exited qspi_wait_while_rpu_awake()...\n");

	(hl_flag) ? qdev->hl_read(addr, buff, wrd_len * 4) : qdev->read(addr, buff, wrd_len * 4);
	qspi_cmd_sleep_rpu(&qspi_perip);

#else
	spim_cmd_rpu_wakeup_fn(0x1);
	LOG_DBG("exited spim_cmd_rpu_wakeup_fn()\n");

	spim_validate_rpu_awake_fn();
	LOG_DBG("exited spim_validate_rpu_awake_fn(). Waiting for rpu_awake.....\n");

	spim_wait_while_rpu_awake_fn();

	(hl_flag) ? qdev->hl_read(addr, buff, wrd_len * 4) : qdev->read(addr, buff, wrd_len * 4);
	spim_cmd_sleep_rpu_fn();

#endif
}

static int cmd_sleep_stats(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t addr;
	uint32_t wrd_len;
	uint32_t *buff;

	addr = strtoul(argv[1], NULL, 0);
	wrd_len = strtoul(argv[2], NULL, 0);

	if (!validate_addr(addr, wrd_len * 4))
		return -1;

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		shell_print(shell, "Error... Cannot write to ROM blocks");
	}

	buff = (uint32_t *)k_malloc(wrd_len * 4);

	get_sleep_stats(addr, buff, wrd_len);

	for (int i = 0; i < wrd_len; i++) {
		LOG_DBG("0x%08x\n", buff[i]);
	}

	k_free(buff);
	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */
int func_irq_config(struct gpio_callback *irq_callback_data, void (*irq_handler)())
{
	int ret;

	ret = gpio_pin_configure_dt(&host_irq_spec, GPIO_INPUT);

	ret = gpio_pin_interrupt_configure_dt(&host_irq_spec,
			GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(irq_callback_data,
			irq_handler,
			BIT(host_irq_spec.pin));

	gpio_add_callback(host_irq_spec.port, irq_callback_data);

	LOG_DBG("Finished Interrupt config\n\n");

	return 0;
}

int func_gpio_config(void)
{
	int ret;

	if (!device_is_ready(iovdd_ctrl_spec.port)) {
		return -ENODEV;
	}

	if (!device_is_ready(bucken_spec.port)) {
		return -ENODEV;
	}

	if (!device_is_ready(host_irq_spec.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&bucken_spec,
			GPIO_OUTPUT | (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos));

	ret = gpio_pin_configure_dt(&iovdd_ctrl_spec, GPIO_OUTPUT);

	LOG_DBG("GPIO configuration done...\n\n");

	return 0;
}

static int cmd_gpio_config(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = func_gpio_config();

	return ret;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */
static int func_pwron(void)
{
	gpio_pin_set_dt(&bucken_spec, 1);
#if SHELIAK_SOC
	k_msleep(SLEEP_TIME_MS);
	gpio_pin_set_dt(&iovdd_ctrl_spec, 1);
	k_msleep(SLEEP_TIME_MS);
	LOG_DBG("BUCKEN=1, IOVDD=1...\n");
#else
#endif

	return 0;
}

static int cmd_pwron(const struct shell *shell, size_t argc, char **argv)
{
	func_pwron();
	return 0;
}
/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static int cmd_config(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t qspi_spim_freq_MHz;
	uint32_t qspi_spim_latency;
	uint32_t mem_block;

	/* Re-initialize cfg */
	cfg = qspi_defconfig();

	qspi_spim_freq_MHz = strtoul(argv[1], NULL, 0);
	mem_block = strtoul(argv[2], NULL, 0);
	qspi_spim_latency = strtoul(argv[3], NULL, 0);

	cfg->freq = qspi_spim_freq_MHz;
	shk_memmap[mem_block][2] = qspi_spim_latency;
	cfg->qspi_slave_latency = qspi_spim_latency;
	LOG_DBG("QSPIM freq = %d MHz\n", cfg->freq);
	LOG_DBG("QSPIM latency = %d\n", cfg->qspi_slave_latency);

	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static void func_qspi_init(void)
{
	qdev = qspi_dev();

	cfg = qspi_defconfig();

	qdev->init(cfg);

	LOG_DBG("QSPI/SPIM freq = %d MHz\n", cfg->freq);
	LOG_DBG("QSPI/SPIM latency = %d\n", cfg->qspi_slave_latency);
	LOG_DBG("QSPI/SPIM Config done...\n");
}

static int cmd_qspi_init(const struct shell *shell, size_t argc, char **argv)
{
	func_qspi_init();
	return 0;
}

int func_rpu_sleep(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_cmd_sleep_rpu(&qspi_perip);
#else
	return spim_cmd_sleep_rpu_fn();
#endif
}

int func_rpu_wake(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_cmd_wakeup_rpu(&qspi_perip, 0x1);
#else
	return spim_cmd_rpu_wakeup_fn(0x1);
#endif
}

int func_rpu_sleep_status(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_RDSR1(&qspi_perip);
#else
	return spim_RDSR1();
#endif
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

int func_rpuwake(void)
{
#if CONFIG_NRF700X_ON_QSPI
	qspi_cmd_wakeup_rpu(&qspi_perip, 0x1);
	LOG_DBG("exited qspi_cmd_wakeup_rpu()\n");

	qspi_validate_rpu_wake_writecmd(&qspi_perip);
	LOG_DBG("exited qspi_validate_rpu_wake_writecmd(). Waiting for rpu_awake.....\n");

	qspi_wait_while_rpu_awake(&qspi_perip);
#else
	spim_cmd_rpu_wakeup_fn(0x1);
	LOG_DBG("exited spim_cmd_rpu_wakeup_fn()\n");

	spim_wait_while_rpu_awake_fn();
	LOG_DBG("exited spim_wait_while_rpu_awake_fn(). Waiting for rpu_awake.....\n");

	spim_validate_rpu_awake_fn();
#endif

	return 0;
}

static int cmd_rpuwake(const struct shell *shell, size_t argc, char **argv)
{
	func_rpuwake();
	return 0;
}
/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static int func_wrsr2(uint8_t data)
{
#if CONFIG_NRF700X_ON_QSPI
	qspi_cmd_wakeup_rpu(&qspi_perip, data);
#else
	spim_cmd_rpu_wakeup_fn(data);
#endif

	LOG_DBG("Written 0x%x to WRSR2\n", data);
	return 0;
}

static int cmd_wrsr2(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t data;

	data = strtoul(argv[1], NULL, 0) & 0xff;
	func_wrsr2(data);
	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */
static int func_rdsr2(void)
{
#if CONFIG_NRF700X_ON_QSPI
	qspi_validate_rpu_wake_writecmd(&qspi_perip);
#else
	spim_validate_rpu_awake_fn();
#endif

	return 0;
}

static int cmd_rdsr2(const struct shell *shell, size_t argc, char **argv)
{
	func_rdsr2();
	return 0;
}
/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static int func_rdsr1(void)
{
#if CONFIG_NRF700X_ON_QSPI
	qspi_wait_while_rpu_awake(&qspi_perip);
#else
	spim_wait_while_rpu_awake_fn();
#endif
	return 0;
}

static int cmd_rdsr1(const struct shell *shell, size_t argc, char **argv)
{
	func_rdsr1();
	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static int func_rpuclks_on(void)
{
#if SHELIAK_SOC
	uint32_t rpu_clks = 0x100;
	/* Enable RPU Clocks */
	qdev->write(0x048C20, &rpu_clks, 4);
	LOG_DBG("RPU Clocks ON...\n");
#endif
	return 0;
}

static int cmd_rpuclks_on(const struct shell *shell, size_t argc, char **argv)
{
	func_rpuclks_on();
	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */
void func_wifi_on(void)
{
	func_gpio_config();
	func_pwron();
	func_qspi_init();
	func_rpuwake();
	func_rpuclks_on();
}

static int cmd_wifi_on(const struct shell *shell, size_t argc, char **argv)
{
	func_wifi_on();
	func_irq_config(&irq_callback_data, irq_handler);
	return 0;
}

/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */
static int cmd_wifi_off(const struct shell *shell, size_t argc, char **argv)
{
#if SHELIAK_SOC
	gpio_pin_set_dt(&iovdd_ctrl_spec, 0); /* IOVDD CNTRL = 0 */
	gpio_pin_set_dt(&bucken_spec, 0); /* BUCKEN = 0 */
	shell_print(shell, "IOVDD=0, BUCKEN=0...");
#else
#endif

	return 0;
}
/* -------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------- */

static int cmd_memmap(const struct shell *shell, size_t argc, char **argv)
{
	print_memmap();
	return 0;
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
	return 0;
}

static int cmd_trgirq(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;

	shell_print(shell, "Asserting IRQ to HOST");

	val = 0x20000;
	qdev->write(0x400, &val, 4);

	val = 0x80000000;
	qdev->write(0x494, &val, 4);

	val = 0x7fff7bee;
	qdev->write(0x484, &val, 4);

	return 0;
}

static int cmd_clrirq(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val;

	shell_print(shell, "de-asserting IRQ to HOST");

	val = 0x80000000;
	qdev->write(0x488, &val, 4);

	return 0;
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
		  cmd_config),
	SHELL_CMD(memmap, NULL, "Gives the full memory map of the Sheliak chip", cmd_memmap),
	SHELL_CMD(memtest, NULL, "Writes, reads back and validates specified memory on Seliak chip",
		  cmd_memtest),
	SHELL_CMD(ver, NULL, "Display SW version of the hex file", cmd_ver),
	SHELL_CMD(help, NULL, "Help with all supported commmands", cmd_help), SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "nrf700x_shell" */
SHELL_CMD_REGISTER(nrf700x_shell, &sub_nrf700x_shell, "nrf700x_shell commands", NULL);
