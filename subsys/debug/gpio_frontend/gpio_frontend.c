/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log_frontend.h>
#include <hal/nrf_gpio.h>

#if (CONFIG_LOG_FRONTEND_PORT_NUMBER == 0)
	#define LOG_FRONTEND_GPIO_REG_PTR NRF_P0
#if CONFIG_BOARD_NRF9160_PCA10090 || CONFIG_BOARD_NRF9160_PCA10090NS
	#define BASE_ADDRESS NRF_P0_NS_BASE
#else
	#define BASE_ADDRESS NRF_P0_BASE
#endif
#elif (CONFIG_LOG_FRONTEND_PORT_NUMBER == 1)
	#define LOG_FRONTEND_GPIO_REG_PTR NRF_P1
#if CONFIG_BOARD_NRF9160_PCA10090 || CONFIG_BOARD_NRF9160_PCA10090NS
	#define BASE_ADDRESS NRF_P1_NS_BASE
#else
	#define BASE_ADDRESS NRF_P1_BASE
#endif
#endif

#define MASK_OF_LOG_PINS	(0x1ff << CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER)
#define MASK_OF_CLK_PIN		BIT(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER + 8)
#define CLEAR_ADDRESS_OFFSET	offsetof(NRF_GPIO_Type, OUTCLR)
#define SET_ADDRESS_OFFSET	offsetof(NRF_GPIO_Type, OUTSET)
#define HEXDUMP_STR_BYTES	0xffffffff

union log_msg_ids_convert {
	struct log_msg_ids ids;
	u16_t raw;
};

static const u8_t sync_frame[] = {0x61, 0x62, 0x63, 0x64,
				  0x65, 0x66, 0x67, 0x68};

/*
 * Note: Due to an emphasis on performace, this module uses assembly code
 * to acheive as low as possible delays. When similar results are attainable
 * with C code, this should be rewritten.
 */

void log_frontend_init(void)
{
	nrf_gpio_port_dir_output_set(LOG_FRONTEND_GPIO_REG_PTR,
				     MASK_OF_LOG_PINS);

	unsigned int key = irq_lock();

	for (u8_t i = 0; i < sizeof(sync_frame); i++) {
		__asm volatile(
			/* sync frame */
			"MOV R0, %[sync_i]			\n"
			"MOV R1, %[base_address]		\n"
			"MOV R2, %[mask_of_clk_pin]		\n"
			"MOV R3, %[mask_of_log_pins]		\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
		: /* Output */
		: /* Input */
			[sync_i]"r"(sync_frame[i]),
			[base_address]"r"(BASE_ADDRESS),
			[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
			[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
			[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
			[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
			[set_address_offset]"i"(SET_ADDRESS_OFFSET)
			: "r0", "r1", "r2", "r3"
		);
	}

	irq_unlock(key);
}

void log_frontend_0(const char *str, struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[str]				\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[str]"r"((u32_t)str),
		[src_level]"r"(log_msg_ids_bytes.raw),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}

void log_frontend_1(const char *str,
		    log_arg_t arg0,
		    struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[str]				\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - first byte */
		"MOV R0, %[arg0]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[str]"r"((u32_t)str),
		[src_level]"r"(log_msg_ids_bytes.raw),
		[arg0]"r"((u32_t)arg0),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}

void log_frontend_2(const char *str,
		    log_arg_t arg0,
		    log_arg_t arg1,
		    struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[str]				\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - first byte */
		"MOV R0, %[arg0]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - first byte */
		"MOV R0, %[arg1]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[str]"r"((u32_t)str),
		[src_level]"r"(log_msg_ids_bytes.raw),
		[arg0]"r"((u32_t)arg0),
		[arg1]"r"((u32_t)arg1),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}

void log_frontend_3(const char *str,
		    log_arg_t arg0,
		    log_arg_t arg1,
		    log_arg_t arg2,
		    struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[str]				\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - first byte */
		"MOV R0, %[arg0]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg0 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - first byte */
		"MOV R0, %[arg1]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg1 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg2 - first byte */
		"MOV R0, %[arg2]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg2 - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg2 - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* arg2 - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[str]"r"((u32_t)str),
		[src_level]"r"(log_msg_ids_bytes.raw),
		[arg0]"r"((u32_t)arg0),
		[arg1]"r"((u32_t)arg1),
		[arg2]"r"((u32_t)arg2),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}

void log_frontend_n(const char *str,
		    log_arg_t *args,
		    u32_t narg,
		    struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[str]				\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[str]"r"((u32_t)str),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);
	for (u32_t i = 0; i < narg; i++) {
		__asm volatile(
			/* args[i] - first byte */
			"MOV R0, %[arg_i]			\n"
			"MOV R1, %[base_address]		\n"
			"MOV R2, %[mask_of_clk_pin]		\n"
			"MOV R3, %[mask_of_log_pins]		\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
			/* args[i] - second byte */
			"LSR R0, #8				\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
			/* args[i] - third byte */
			"LSR R0, #8				\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
			/* args[i] - fourth byte */
			"LSR R0, #8				\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
		: /* Output */
		: /* Input */
			[arg_i]"r"((u32_t)args[i]),
			[base_address]"r"(BASE_ADDRESS),
			[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
			[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
			[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
			[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
			[set_address_offset]"i"(SET_ADDRESS_OFFSET)
			: "r0", "r1", "r2", "r3"
		);
	}
	__asm volatile(
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[src_level]"r"(log_msg_ids_bytes.raw),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}

void log_frontend_hexdump(const char *str,
			  const u8_t *data,
			  u32_t length,
			  struct log_msg_ids src_level)
{
	union log_msg_ids_convert log_msg_ids_bytes;

	log_msg_ids_bytes.ids = src_level;

	if (length > CONFIG_LOG_FRONTEND_HEXDUMP_MAX_LENGTH) {
		length = CONFIG_LOG_FRONTEND_HEXDUMP_MAX_LENGTH;
	}

	unsigned int key = irq_lock();

	__asm volatile(
		/* str - first byte */
		"MOV R0, %[string]			\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* str - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* length - first byte */
		"MOV R0, %[length]			\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* length - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* length - third byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* length - fourth byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[string]"r"(HEXDUMP_STR_BYTES),
		[length]"r"(length),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);
	for (u32_t i = 0; i < length; i++) {
		__asm volatile(
			"MOV R0, %[data_i]			\n"
			"MOV R1, %[base_address]		\n"
			"MOV R2, %[mask_of_clk_pin]		\n"
			"MOV R3, %[mask_of_log_pins]		\n"
			"BFI R2, R0, %[lsb_pin_number], #8	\n"
			"STR R3, [R1, %[clear_address_offset]]	\n"
			"STR R2, [R1, %[set_address_offset]]	\n"
		: /* Output */
		: /* Input */
			[data_i]"r"(data[i]),
			[base_address]"r"(BASE_ADDRESS),
			[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
			[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
			[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
			[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
			[set_address_offset]"i"(SET_ADDRESS_OFFSET)
			: "r0", "r1", "r2", "r3"
		);
	}
	__asm volatile(
		/* src_level - first byte */
		"MOV R0, %[src_level]			\n"
		"MOV R1, %[base_address]		\n"
		"MOV R2, %[mask_of_clk_pin]		\n"
		"MOV R3, %[mask_of_log_pins]		\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
		/* src_level - second byte */
		"LSR R0, #8				\n"
		"BFI R2, R0, %[lsb_pin_number], #8	\n"
		"STR R3, [R1, %[clear_address_offset]]	\n"
		"STR R2, [R1, %[set_address_offset]]	\n"
	: /* Output */
	: /* Input */
		[src_level]"r"(log_msg_ids_bytes.raw),
		[base_address]"r"(BASE_ADDRESS),
		[mask_of_clk_pin]"r"(MASK_OF_CLK_PIN),
		[mask_of_log_pins]"r"(MASK_OF_LOG_PINS),
		[lsb_pin_number]"i"(CONFIG_LOG_FRONTEND_LSB_PIN_NUMBER),
		[clear_address_offset]"i"(CLEAR_ADDRESS_OFFSET),
		[set_address_offset]"i"(SET_ADDRESS_OFFSET)
		: "r0", "r1", "r2", "r3"
	);

	irq_unlock(key);
}
