/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Test-only fault injection hook.
 *
 * The hook implements a Vendor Specific HCI command that raises a fatal error in the requested
 * execution context. It is used by the tests validating that the Vendor Specific fatal error
 * event reaches the host from a thread, an interrupt and a zero-latency interrupt context.
 *
 * It is compiled into whichever network core image the test builds, either the ipc_radio
 * application or the hci_ipc sample, and only when that image enables its own fault injection
 * hook Kconfig option. That must never be the case in a production image.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/irq_offload.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#include "fatal_error_test_hook.h"

LOG_MODULE_REGISTER(fatal_error_test_hook, CONFIG_LOG_DEFAULT_LEVEL);

/* Length of the H:4 packet type indicator that prefixes the payload of an HCI buffer. */
#define H4_TYPE_LEN 1U

/* Each network core image names the fault injection hook options after itself, so resolve the
 * zero-latency interrupt line once here and let the rest of the file work with it.
 */
#if defined(CONFIG_IPC_RADIO_FATAL_ERROR_TEST_HOOK_ZLI)
#define HOOK_ZLI_IRQN CONFIG_IPC_RADIO_FATAL_ERROR_TEST_HOOK_ZLI_IRQN
#elif defined(CONFIG_HCI_IPC_FATAL_ERROR_TEST_HOOK_ZLI)
#define HOOK_ZLI_IRQN CONFIG_HCI_IPC_FATAL_ERROR_TEST_HOOK_ZLI_IRQN
#endif

static void fault_raise(uint8_t fault);

static uint8_t pending_fault;

#if defined(HOOK_ZLI_IRQN)
/* A zero-latency interrupt bypasses the kernel, so it has to be registered as a direct ISR. */
ISR_DIRECT_DECLARE(zli_isr)
{
	fault_raise(pending_fault);

	return 0;
}

static void zli_trigger(void)
{
	IRQ_DIRECT_CONNECT(HOOK_ZLI_IRQN, 0, zli_isr, IRQ_ZERO_LATENCY);
	irq_enable(HOOK_ZLI_IRQN);

	NVIC_SetPendingIRQ(HOOK_ZLI_IRQN);

	/* The zero-latency interrupt is not masked by irq_lock() and preempts this context
	 * immediately, hence this point is never reached.
	 */
	for (;;) {
	}
}
#endif /* HOOK_ZLI_IRQN */

static void isr_offload(const void *arg)
{
	ARG_UNUSED(arg);

	fault_raise(pending_fault);
}

static void fault_raise(uint8_t fault)
{
	switch (fault) {
	case BT_HCI_VS_FATAL_ERROR_TEST_FAULT_ASSERT:
		extern void bt_ctlr_assert_handle(char *file, uint32_t line);

		bt_ctlr_assert_handle(__FILE__, __LINE__);
		break;

	case BT_HCI_VS_FATAL_ERROR_TEST_FAULT_PANIC:
	default:
		k_panic();
		break;
	}
}

bool fatal_error_test_hook_cmd(struct net_buf *buf)
{
	const struct bt_hci_vs_fatal_error_test_inject *cmd;
	const struct bt_hci_cmd_hdr *hdr;

	if (buf->len < (H4_TYPE_LEN + sizeof(*hdr))) {
		return false;
	}

	if (buf->data[0] != BT_HCI_H4_CMD) {
		return false;
	}

	hdr = (const void *)&buf->data[H4_TYPE_LEN];
	if (sys_le16_to_cpu(hdr->opcode) != BT_HCI_OP_VS_FATAL_ERROR_TEST_INJECT) {
		return false;
	}

	/* From here on the command is ours. It never reaches the controller, which does not
	 * know this opcode, so every path below has to return true.
	 */
	if (hdr->param_len < sizeof(*cmd)) {
		LOG_ERR("Fault injection command is %u bytes, expected at least %u.",
			hdr->param_len, (uint8_t)sizeof(*cmd));
		return true;
	}

	cmd = (const void *)&buf->data[H4_TYPE_LEN + sizeof(*hdr)];

	LOG_WRN("Injecting a fatal error, context %u fault %u.", cmd->context, cmd->fault);

	pending_fault = cmd->fault;

	switch (cmd->context) {
	case BT_HCI_VS_FATAL_ERROR_TEST_CONTEXT_ISR:
		irq_offload(isr_offload, NULL);
		break;

#if defined(HOOK_ZLI_IRQN)
	case BT_HCI_VS_FATAL_ERROR_TEST_CONTEXT_ZLI:
		zli_trigger();
		break;
#endif /* HOOK_ZLI_IRQN */

	case BT_HCI_VS_FATAL_ERROR_TEST_CONTEXT_THREAD:
		fault_raise(cmd->fault);
		break;

	default:
		LOG_ERR("Unsupported fault injection context %u.", cmd->context);
		break;
	}

	return true;
}
