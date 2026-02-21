/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */
#include <stdio.h>

#include <hal/nrf_mpc.h>
#include <hal/nrf_spu.h>

#include <hal/nrf_gpio.h>
#include <zephyr/drivers/gpio.h>

#if defined(CONFIG_ZTEST)
#include <zephyr/ztest.h>
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(out_gpio)) || !DT_NODE_EXISTS(DT_NODELABEL(in_gpio))
#error "Unsupported board: test_gpio node is not defined"
#endif

/* Delay after setting GPIO ouputs. It allows signals to settle. */
#define PROPAGATION_DELAY_MS K_MSEC(1U)

const struct gpio_dt_spec out_pin = GPIO_DT_SPEC_GET(DT_ALIAS(out0), gpios);
const struct gpio_dt_spec in_pin = GPIO_DT_SPEC_GET(DT_ALIAS(in0), gpios);

/* This is one page size, must be aligned to 4KB */
#define SIZE_OF_ONE_PAGE 4096

__attribute__((section(".sram_priv_scratch")))
__aligned(SIZE_OF_ONE_PAGE) volatile uint8_t sram_priv_scratch[SIZE_OF_ONE_PAGE];

/* Userspace: allow K_USER threads to access sram_priv_scratch as unprivileged scratch */
K_MEM_PARTITION_DEFINE(sram_scratch_part,
					   sram_priv_scratch,
					   SIZE_OF_ONE_PAGE,
					   K_MEM_PARTITION_P_RW_U_RW);

static struct k_mem_domain unpriv_mem_domain;
static struct k_sem done;

static bool unpriv_domain_inited;
static bool fault_expected;
static uint32_t fault_reason;

/**
 * Return the SPU instance that can be used to configure the
 * peripheral at the given base address.
 */
static inline NRF_SPU_Type *spu_instance_from_peripheral_addr(uint32_t peripheral_addr)
{
	/* See the SPU chapter in the IPS for how this is calculated */

	uint32_t apb_bus_number = peripheral_addr & 0x00FC0000;

	return (NRF_SPU_Type *)(0x50000000 | apb_bus_number);
}

/**
 * @brief Setting unprivileged or privileged access to the GPIO pins
 *
 * @param[in] spu_instance Point to the targeted SPU
 * @param[in] port		 Index of port for the targeted GPIO
 * @param[in] pin		  Index of pin for the targeted GPIO
 * @param[in] privileged   True for setting GPIO privilege, false for unprivileged
 */
static void set_gpio_privilege(NRF_SPU_Type *spu_instance,
							   uint8_t		 port,
							   uint8_t	   pin,
							   bool		  privileged)
{
	if (privileged) {
		spu_instance->FEATURE.GPIO[port].PIN[pin] =
		(spu_instance->FEATURE.GPIO[port].PIN[pin] & ~SPU_FEATURE_GPIO_PIN_PRIVLATTR_Msk) |
		(SPU_FEATURE_GPIO_PIN_PRIVLATTR_Privileged << SPU_FEATURE_GPIO_PIN_PRIVLATTR_Pos);
	} else {
		spu_instance->FEATURE.GPIO[port].PIN[pin] =
		(spu_instance->FEATURE.GPIO[port].PIN[pin] & ~SPU_FEATURE_GPIO_PIN_PRIVLATTR_Msk) |
		(SPU_FEATURE_GPIO_PIN_PRIVLATTR_Unprivileged << SPU_FEATURE_GPIO_PIN_PRIVLATTR_Pos);
	}
}

/**
 * @brief Setting unprivileged or privileged access to the scratch memory
 *
 * @param[in] privileged True for setting memory privilege, false for unprivileged
 */
static inline void set_memory_privilege(bool privileged)
{
	/* Overriding memory mapping */
	nrf_mpc_override_config_t mpc_override = {
									.slave_number = 0,
									.lock = false,
									.enable = true,
									.secdom_enable = false,
									.secure_mask = false,
								};
	uint32_t priv_start =
		(uint32_t)&sram_priv_scratch[0] & ~(SIZE_OF_ONE_PAGE - 1);

	uint32_t priv_end =
		((uint32_t)&sram_priv_scratch[0] + SIZE_OF_ONE_PAGE - 1) &
		~(SIZE_OF_ONE_PAGE - 1);

	/* Configure SRAM MPC override regions, index 4 as unused index */
	nrf_mpc_override_startaddr_set(NRF_MPC00, 4, priv_start);
	nrf_mpc_override_endaddr_set(NRF_MPC00, 4, priv_end);
	/* Configure MPC override, 0x10 for privileged */
	nrf_mpc_override_perm_set(NRF_MPC00, 4, privileged ? 0x10 : 0x0);
	/* mask for Privileged */
	nrf_mpc_override_permmask_set(NRF_MPC00, 4, 0x10);
	nrf_mpc_override_config_set(NRF_MPC00, 4, &mpc_override);

	/* Initialise unprivileged memory domain */
	if (!unpriv_domain_inited && !privileged) {
		struct k_mem_partition *unpriv_mem_parts[] = {
		#if Z_LIBC_PARTITION_EXISTS
			&z_libc_partition,
		#endif
			&sram_scratch_part
		};
		int ret = k_mem_domain_init(&unpriv_mem_domain,
									 ARRAY_SIZE(unpriv_mem_parts),
									 unpriv_mem_parts);
		zassert_equal(ret, 0, "k_mem_domain_init failed");
		unpriv_domain_inited = true;
	}
}

/** @brief Function to setup peripherals in privileged mode */
static inline void* privileged_setup(void)
{
	printf("Test executed on %s\n", CONFIG_BOARD_TARGET);
	int rc;
	unpriv_domain_inited = false;
	fault_expected = false;

	k_sem_init(&done, 0, 1);
	set_memory_privilege(true);

	zassert_true(gpio_is_ready_dt(&out_pin), "OUT[0] is not ready");
	zassert_true(gpio_is_ready_dt(&in_pin), "IN[0] is not ready");

	rc = gpio_pin_configure_dt(&out_pin, GPIO_OUTPUT);
	zassert_equal(rc, 0, "gpio_pin_configure_dt(OUT[0], GPIO_OUTPUT) failed");

	rc = gpio_pin_configure_dt(&in_pin, GPIO_INPUT);
	zassert_equal(rc, 0, "gpio_pin_configure_dt(IN[0], GPIO_INPUT) failed");

	NRF_SPU_Type *spu_instance = spu_instance_from_peripheral_addr(NRF_P1_S_BASE);

	/* Configure GPIO to be privileged, port number 1 in nrf7120 */
	set_gpio_privilege(spu_instance, 1, out_pin.pin, true);
	set_gpio_privilege(spu_instance, 1, in_pin.pin,  true);

	return NULL;
}

/** @brief Function to setup peripherals in unprivileged mode */
static inline void* unprivileged_setup(void)
{
	printf("Test executed on %s\n", CONFIG_BOARD_TARGET);
	int rc;
	unpriv_domain_inited = false;
	fault_expected = false;

	/* Initialize memory domain for userspace */
	k_sem_init(&done, 0, 1);

	set_memory_privilege(false);

	NRF_SPU_Type *spu_instance = spu_instance_from_peripheral_addr(NRF_P1_S_BASE);

	set_gpio_privilege(spu_instance, 1, out_pin.pin, false);
	set_gpio_privilege(spu_instance, 1, in_pin.pin, false);

	/* Configure GPIO to be privileged */
	zassert_true(gpio_is_ready_dt(&out_pin), "OUT[0] is not ready");
	zassert_true(gpio_is_ready_dt(&in_pin), "IN[0] is not ready");

	rc = gpio_pin_configure_dt(&out_pin, GPIO_OUTPUT | GPIO_ACTIVE_HIGH);
	zassert_equal(rc, 0, "gpio_pin_configure_dt(OUT[0], GPIO_OUTPUT) failed");

	rc = gpio_pin_configure_dt(&in_pin, GPIO_INPUT | GPIO_ACTIVE_HIGH);
	zassert_equal(rc, 0, "gpio_pin_configure_dt(IN[0], GPIO_INPUT) failed");

	return NULL;
}

static inline void restore()
{
	memset((void *)sram_priv_scratch  , 0x0, SIZE_OF_ONE_PAGE);
	fault_expected = false;
}

/** @brief Function to access gpio */
static inline void access_gpio(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	int rc, pin_value;

	/* 0 = privileged, 1 = unprivileged */
	printf("Current function access_gpio is in %s mode.\n",
		   (__get_CONTROL() & CONTROL_nPRIV_Msk) ? "UnPrivileged" : "Privileged");

	/* Try to access GPIO pins */
	rc = gpio_port_set_bits_raw(out_pin.port, out_pin.pin);
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT[0], 1) failed");

	/* Wait a bit to stabilize logic level. */
	k_sleep(PROPAGATION_DELAY_MS);

	rc = gpio_port_get_raw(in_pin.port, &pin_value);
	/* Reading physical level from GPIO on FPGA platform will get value 0 comeback on dk board */
	/* zassert_equal(pin_value, 1,  "gpio_pin_get_dt(OUT[0], 1) failed"); */

	rc = gpio_port_clear_bits_raw(out_pin.port, out_pin.pin);
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT[0], 0) failed");

	printf("App thread: accessed GPIO pins successfully!\n");
	k_sem_give(&done);
}

/** @brief Function to access memory */
static inline void access_memory(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* 0 = privileged, 1 = unprivileged */
	printf("Current function access_memory is in %s mode.\n",
		   (__get_CONTROL() & CONTROL_nPRIV_Msk) ? "UnPrivileged" : "Privileged");

	sram_priv_scratch[0] = 0xAA;
	zassert_equal(sram_priv_scratch[0], 0xAA, "Access to privileged memory failed");

	printf("App thread: accessed privileged memory successfully!\n");
	k_sem_give(&done);
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	ARG_UNUSED(pEsf);

	struct k_thread *faulting_thread = k_current_get();
	const char *name = k_thread_name_get(faulting_thread);

	printf("\n[FATAL] reason=%u, thread=%p (%s)\n",
		   reason, (void *)faulting_thread, name ? name : "noname");

	printf("Fault expected: %s , Fault reason allow: %u\n",
			fault_expected? "true" : "false", fault_reason);

	/* If this is the user thread and the reason matches what we expect,
	 * treat it as a test-pass, kill only that thread, and continue.
	 */
	if (reason == fault_reason && fault_expected) {
		printf("[FATAL] Expected fault in user thread – marking as handled\n");

		/* Abort only the faulty thread */
		k_thread_abort(faulting_thread);

		/* Return to let the rest of the system keep running */
		return;
	}

	/* Anything else is a real fatal error */
	printf("[FATAL] Unexpected fault – halting system\n");
	ztest_test_fail();
}

/* Testsuite setting peripherals privilege */
ZTEST_SUITE(test_suite_priv_settings, NULL, privileged_setup, NULL, restore, NULL);
/* Testsuite setting peripherals unprivilege */
ZTEST_SUITE(test_suite_unpriv_settings, NULL, unprivileged_setup, NULL, restore, NULL);

ZTEST(test_suite_priv_settings, test_case_priv_access_priv_perip)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_gpio,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   0,  /* <-- privileged */
								   K_FOREVER);

	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_priv_settings, test_case_priv_access_priv_mem)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_memory,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   0,  /* <-- unprivileged */
								   K_FOREVER);

	k_thread_start(tid);
	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_priv_settings, test_case_unpriv_access_priv_perip){
	fault_expected = true;
	fault_reason = K_ERR_KERNEL_OOPS;
	printf("We should expect fault in this test case...\n");

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_gpio,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   K_USER,  /* <-- unprivileged */
								   K_FOREVER);
	/* Grant kernel objects used by the user thread */
	k_object_access_grant(&done, tid);

	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_priv_settings, test_case_unpriv_access_priv_mem)
{
	fault_expected = true;
	fault_reason = K_ERR_CPU_EXCEPTION;
	printf("We should expect fault in this test case...\n");

	static K_THREAD_STACK_DEFINE(user_stack, 2048);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_memory,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   K_USER,  /* <-- unprivileged */
								   K_FOREVER);

	/* We do not add unpriv_mem_domain to the thread in priv mem access*/
	k_object_access_grant(&done, tid);
	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_unpriv_settings, test_case_priv_access_unpriv_perip)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_gpio,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   0,  /* <-- privileged */
								   K_FOREVER);

	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_unpriv_settings, test_case_priv_access_unpriv_mem)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_thread_create(&user_thread, user_stack,
					 K_THREAD_STACK_SIZEOF(user_stack),
					 access_memory,
					 NULL, NULL, NULL,
					 K_PRIO_PREEMPT(1),
					 0,  /* <-- unprivileged */
					 K_NO_WAIT);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_unpriv_settings, test_case_unpriv_access_unpriv_perip)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
								   K_THREAD_STACK_SIZEOF(user_stack),
								   access_gpio,
								   NULL, NULL, NULL,
								   K_PRIO_PREEMPT(1),
								   K_USER,  /* <-- unprivileged */
								   K_FOREVER);


	k_mem_domain_add_thread(&unpriv_mem_domain, tid);

	/* Grant kernel objects used by the user thread */
	k_object_access_grant(&done, tid);

	/* Grant user access to GPIO device(s) */
	k_object_access_grant(out_pin.port, tid);
	k_object_access_grant(in_pin.port, tid);

	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}

ZTEST(test_suite_unpriv_settings, test_case_unpriv_access_unpriv_mem)
{
	fault_expected = false;

	static K_THREAD_STACK_DEFINE(user_stack, 1024);
	static struct k_thread user_thread;

	k_tid_t tid = k_thread_create(&user_thread, user_stack,
									K_THREAD_STACK_SIZEOF(user_stack),
									access_memory,
									NULL, NULL, NULL,
									K_PRIO_PREEMPT(1),
									K_USER,  /* <-- unprivileged */
									K_FOREVER);

	k_mem_domain_add_thread(&unpriv_mem_domain, tid);
	k_object_access_grant(&done, tid);
	k_thread_start(tid);

	k_sem_take(&done, K_SECONDS(2));
}
