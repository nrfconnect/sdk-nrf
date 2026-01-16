/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Requirements:memorys, peripherals, GPIOs

#include <zephyr/kernel.h>
#include <stdio.h>

#include <hal/nrf_mpc.h>
#include <hal/nrf_spu.h>

#include <hal/nrf_gpio.h>

#define USER_STACKSIZE	2048

// #ifndef CONFIG_USERSPACE
// #error This sample requires CONFIG_USERSPACE.
// #endif

volatile bool expected_fault;
volatile uint32_t fault_reason;

static struct k_thread user_thread_gpio;
static struct k_thread user_thread_sram;

K_THREAD_STACK_DEFINE(user_stack_gpio, USER_STACKSIZE);
K_THREAD_STACK_DEFINE(user_stack_sram, USER_STACKSIZE);

#define SIZE_OF_SCRATCH_AREA 64

__attribute__((section(".sram_priv_scratch")))
volatile uint32_t sram_priv_scratch[SIZE_OF_SCRATCH_AREA];

__attribute__((section(".sram_unpriv_scratch")))
volatile uint32_t sram_unpriv_scratch[SIZE_OF_SCRATCH_AREA];

__attribute__((section(".rom_priv_scratch")))
volatile uint32_t rom_priv_scratch[SIZE_OF_SCRATCH_AREA];

__attribute__((section(".rom_unpriv_scratch")))
volatile uint32_t rom_unpriv_scratch[SIZE_OF_SCRATCH_AREA];

/**
 * Return the SPU instance that can be used to configure the
 * peripheral at the given base address.
 */
static inline NRF_SPU_Type * spu_instance_from_peripheral_addr(uint32_t peripheral_addr)
{
	/* See the SPU chapter in the IPS for how this is calculated */

	uint32_t apb_bus_number = peripheral_addr & 0x00FC0000;

	return (NRF_SPU_Type *)(0x50000000 | apb_bus_number);
}

static void set_gpio_privilege(NRF_SPU_Type *spu_instance, uint8_t port, uint8_t pin, bool privileged)
{
    if (privileged) {
        spu_instance->FEATURE.GPIO[port].PIN[pin] =
        (spu_instance->FEATURE.GPIO[port].PIN[pin] & ~SPU_FEATURE_GPIO_PIN_PRIVLATTR_Msk) |
         (SPU_FEATURE_GPIO_PIN_PRIVLATTR_Privileged <<  SPU_FEATURE_GPIO_PIN_PRIVLATTR_Pos);
    } else {
        spu_instance->FEATURE.GPIO[port].PIN[pin] =
        (spu_instance->FEATURE.GPIO[port].PIN[pin] & ~SPU_FEATURE_GPIO_PIN_PRIVLATTR_Msk) |
         (SPU_FEATURE_GPIO_PIN_PRIVLATTR_Unprivileged <<  SPU_FEATURE_GPIO_PIN_PRIVLATTR_Pos);
    }
}

static int firmware_firewall_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    printf("Firewall: configuring SPU/MPC...\n");

    // Overriding memory mapping
    nrf_mpc_override_config_t mpc_override = {
									.slave_number = 0,
									.lock = true,
									.enable = true,
									.secdom_enable = false,
									.secure_mask = false,
								};

	/* Configure SRAM MPC override regions */
	//printf("address of sram_priv_scratch = %x\n", (uint32_t)&sram_priv_scratch[0]);
	nrf_mpc_override_startaddr_set(NRF_MPC00, 4, (uint32_t)&sram_priv_scratch[0] & 0xFFFFF000);
	nrf_mpc_override_endaddr_set(NRF_MPC00, 4, ((uint32_t)&sram_priv_scratch[0] + SIZE_OF_SCRATCH_AREA - 1) & 0xFFFFF000);
	nrf_mpc_override_perm_set(NRF_MPC00, 4, 0x10);      // PRIVL = 1
	nrf_mpc_override_permmask_set(NRF_MPC00, 4, 0x10);  // Mask for PRIVL
	//nrf_mpc_override_ownerid_set(NRF_MPC00, 4, 0);
	nrf_mpc_override_config_set(NRF_MPC00, 4, &mpc_override);

    /* Configure SRAM MPC override regions */
	nrf_mpc_override_startaddr_set(NRF_MPC00, 6, (uint32_t)&rom_priv_scratch[0] & 0xFFFFF000);
	nrf_mpc_override_endaddr_set(NRF_MPC00, 6, ((uint32_t)&rom_priv_scratch[0] + SIZE_OF_SCRATCH_AREA - 1) & 0xFFFFF000);
	nrf_mpc_override_perm_set(NRF_MPC00, 6, 0x10);      // PRIVL = 1
	nrf_mpc_override_permmask_set(NRF_MPC00, 6, 0x10);  // Mask for PRIVL
	nrf_mpc_override_config_set(NRF_MPC00, 6, &mpc_override);

    // GPIO privileged mapping
    // P1.11 and P1.12 as example

    // Configure GPIO to be privileged

    NRF_SPU_Type *spu_instance = spu_instance_from_peripheral_addr(NRF_P1_S_BASE);

    set_gpio_privilege(spu_instance, 1, 11, true); // P1.11
    set_gpio_privilege(spu_instance, 1, 12, true); // P1.12

    return 0;
}

static inline void user_func_gpio(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    printf("App thread: trying to access privileged GPIO pins...\n");

    printf("Current Privilege in user_func_gpio = 0x%lx .\n", __get_CONTROL() & CONTROL_nPRIV_Msk); // 0 = privileged, 1 = unprivileged

    // Try to access privileged GPIO pins
    // Decode port and pin:
    // For port N = ((pin_number) >> 5) (Upper mask after 5 bits)
    // For pin N = ((pin_number) & 0x1F) (Lower mask for first 5 bits)
    nrf_gpio_pin_set(0x21); // P1.11
    nrf_gpio_pin_set(0x22); // P1.12

    k_busy_wait(5000);

    nrf_gpio_pin_clear(0x21); // P1.11
    nrf_gpio_pin_clear(0x22); // P1.12

    printf("App thread: accessed privileged GPIO pins successfully!\n");
}

static inline void user_func_sram(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	printf("Current Privilege in user_func_sram = 0x%lx .\n", __get_CONTROL()  & CONTROL_nPRIV_Msk); // 0 = privileged, 1 = unprivileged

	printf("user_entry: writing unpriv scratch...\n");
	sram_unpriv_scratch[0] = 0xAA;    /* should succeed */

	printf("user_entry: writing priv scratch...\n");
	sram_priv_scratch[0] = 0xBB;      /* expect fault */
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
    ARG_UNUSED(pEsf);

    struct k_thread *faulting_thread = k_current_get();
    const char *name = k_thread_name_get(faulting_thread);

    printk("\n[FATAL] reason=%u, thread=%p (%s)\n",
           reason, (void *)faulting_thread, name ? name : "noname");

    /* If this is the user thread and the reason matches what we expect,
     * treat it as a test-pass, kill only that thread, and continue.
     */
    if (reason == fault_reason) {
        printk("[FATAL] Expected fault in user thread – marking as handled\n");

        /* Abort only the faulty thread */
        k_thread_abort(faulting_thread);

        /* Return to let the rest of the system keep running */
        return;
    }

    /* Anything else is a real fatal error */
    printk("[FATAL] Unexpected fault – halting system\n");
    k_fatal_halt(reason);
}

int main(void)
{
	// Bit 0 (nPRIV) = 0  → Privileged
	// Bit 1 (SPSEL) = 1  → Thread uses PSP, not MSP
	printf("Initial privilege mode in main thread = 0x%lx .\n", (__get_CONTROL() & CONTROL_nPRIV_Msk)); // 0 = privileged, 1 = unprivileged

	printf("============================ Testing unprivileged peripherals and memory... ============================\n");

    printf("==================== In privileged mode, trying to access unprivileged GPIO pins... ====================\n");
	user_func_gpio(NULL, NULL, NULL); // Run once in privileged mode
	printf("========================================================================================================\n");

    printf("===================== In unprivileged mode, trying to access unprivileged GPIO pins... ==================\n");
	k_thread_create(&user_thread_gpio,
					user_stack_gpio,
					K_THREAD_STACK_SIZEOF(user_stack_gpio),
					user_func_gpio,
					NULL, NULL, NULL,
					3,
					K_USER,         // <-- Zephyr will drop this to unprivileged
					K_NO_WAIT);
	printf("========================================================================================================\n");

	printf("Initializing firmware firewall by setting privileged attributes to SRAM, MRAM, GPIO ...\n");
	firmware_firewall_init(NULL);

	expected_fault = true;
	fault_reason = 19; // MemManage fault

	printf("===================== In privileged mode, trying to access privileged GPIO pins... =====================\n");
	user_func_gpio(NULL, NULL, NULL); // Run once in privileged mode
	printf("========================================================================================================\n");

	// expected_fault = true;
	printf("==================== In unprivileged mode, trying to access privileged GPIO pins... ====================\n");
	fault_reason = K_ERR_ARM_MEM_DATA_ACCESS;
	k_thread_create(&user_thread_gpio,
					user_stack_gpio,
					K_THREAD_STACK_SIZEOF(user_stack_gpio),
					user_func_gpio,
					NULL, NULL, NULL,
					3,
					K_USER,         // <-- Zephyr will drop this to unprivileged
					K_NO_WAIT);
	printf("========================================================================================================\n");

	//expected_fault = false;
	// fault_reason = -1;
    printf("===================== In privileged mode, trying to access privileged SRAM... ==========================\n");
	user_func_sram(NULL, NULL, NULL); // Run once in privileged mode
	printf("========================================================================================================\n");

	// expected_fault = true;
    printf("==================== In unprivileged mode, trying to access privileged SRAM... =========================\n");
	fault_reason = K_ERR_ARM_MEM_DATA_ACCESS;
	k_thread_create(&user_thread_sram,
					user_stack_sram,
					K_THREAD_STACK_SIZEOF(user_stack_sram),
					user_func_sram,
					NULL, NULL, NULL,
					3,
					K_USER,         // <-- Zephyr will drop this to unprivileged
					K_NO_WAIT);
	printf("========================================================================================================\n");

	/* Wait some time and then check if the expected fault happened */
	k_sleep(K_SECONDS(2));

	printf("End of main.\n");

	return 0;
}
