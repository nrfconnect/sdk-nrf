/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <nrfx.h>
#include <hal/nrf_power.h>
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <dk_buttons_and_leds.h>


#define SYSTEM_OFF_DELAY_S	3

#define MAX_REC_COUNT		1
#define NDEF_MSG_BUF_SIZE	128

#define NFC_FIELD_LED		DK_LED1
#define SYSTEM_ON_LED		DK_LED2


/* Delayed work that enters system off. */
static struct k_work_delayable system_off_work;


/**
 * @brief Function that receives events from NFC.
 */
static void nfc_callback(void *context,
			 nfc_t2t_event_t event,
			 const uint8_t *data,
			 size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);

	switch (event) {
	case NFC_T2T_EVENT_FIELD_ON:
		/* Cancel entering system off */
		k_work_cancel_delayable(&system_off_work);
		dk_set_led_on(NFC_FIELD_LED);
		break;
	case NFC_T2T_EVENT_FIELD_OFF:
		/* Enter system off after delay */
		k_work_reschedule(&system_off_work,
				K_SECONDS(SYSTEM_OFF_DELAY_S));
		dk_set_led_off(NFC_FIELD_LED);
		break;
	default:
		break;
	}
}


/**
 * @brief Function for configuring and starting the NFC.
 */
static int start_nfc(void)
{
	/* Text message in its language code. */
	static const uint8_t en_payload[] = {
		'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'
	};
	static const uint8_t en_code[] = {'e', 'n'};

	/* Buffer used to hold an NFC NDEF message. */
	static uint8_t buffer[NDEF_MSG_BUF_SIZE];

	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_text_rec,
				      UTF_8,
				      en_code,
				      sizeof(en_code),
				      en_payload,
				      sizeof(en_payload));

	NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

	uint32_t len = sizeof(buffer);

	/* Set up NFC */
	if (nfc_t2t_setup(nfc_callback, NULL) < 0) {
		printk("Cannot setup NFC T2T library!\n");
		return -1;
	}

	/* Add record */
	if (nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				&NFC_NDEF_TEXT_RECORD_DESC(nfc_text_rec)) < 0) {
		printk("Cannot add record!\n");
		return -1;
	}

	/* Encode welcome message */
	if (nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
				buffer, &len) < 0) {
		printk("Cannot encode message!\n");
		return -1;
	}

	/* Set created message as the NFC payload */
	if (nfc_t2t_payload_set(buffer, len) < 0) {
		printk("Cannot set payload!\n");
		return -1;
	}

	/* Start sensing NFC field */
	if (nfc_t2t_emulation_start() < 0) {
		printk("Cannot start emulation!\n");
		return -1;
	}

	printk("NFC configuration done\n");
	return 0;
}


/**
 * @brief Function entering system off.
 * System off is delayed to make sure that NFC tag was correctly read.
 */
static void system_off(struct k_work *work)
{
	printk("Entering system off.\nApproach a NFC reader to restart.\n");

	/* Before we disabled entry to deep sleep. Here we need to override
	 * that, then force a sleep so that the deep sleep takes effect.
	 */
	const struct pm_state_info si = {PM_STATE_SOFT_OFF, 0, 0};

	pm_state_force(0, &si);

	dk_set_led_off(SYSTEM_ON_LED);

	/* Going into sleep will actually go to system off mode, because we
	 * forced it above.
	 */
	k_sleep(K_MSEC(1));

	/* k_sleep will never exit, so below two lines will never be executed
	 * if system off was correct. On the other hand if someting gone wrong
	 * we will see it on terminal and LED.
	 */
	dk_set_led_on(SYSTEM_ON_LED);
	printk("ERROR: System off failed\n");
}


/**
 * @brief  Helper function for printing the reason of the last reset.
 * Can be used to confirm that NCF field actually woke up the system.
 */
static void print_reset_reason(void)
{
	uint32_t reas;

#if NRF_POWER_HAS_RESETREAS

	reas = nrf_power_resetreas_get(NRF_POWER);
	nrf_power_resetreas_clear(NRF_POWER, reas);
	if (reas & NRF_POWER_RESETREAS_NFC_MASK) {
		printk("Wake up by NFC field detect\n");
	} else if (reas & NRF_POWER_RESETREAS_RESETPIN_MASK) {
		printk("Reset by pin-reset\n");
	} else if (reas & NRF_POWER_RESETREAS_SREQ_MASK) {
		printk("Reset by soft-reset\n");
	} else if (reas) {
		printk("Reset by a different source (0x%08X)\n", reas);
	} else {
		printk("Power-on-reset\n");
	}

#else

	reas = nrf_reset_resetreas_get(NRF_RESET);
	nrf_reset_resetreas_clear(NRF_RESET, reas);
	if (reas & NRF_RESET_RESETREAS_NFC_MASK) {
		printk("Wake up by NFC field detect\n");
	} else if (reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
		printk("Reset by pin-reset\n");
	} else if (reas & NRF_RESET_RESETREAS_SREQ_MASK) {
		printk("Reset by soft-reset\n");
	} else if (reas) {
		printk("Reset by a different source (0x%08X)\n", reas);
	} else {
		printk("Power-on-reset\n");
	}

#endif
}


int main(void)
{
	/* Configure LED-pins */
	if (dk_leds_init() < 0) {
		printk("Cannot init LEDs!\n");
		return 0;
	}
	dk_set_led_on(SYSTEM_ON_LED);

	/* Configure and start delayed work that enters system off */
	k_work_init_delayable(&system_off_work, system_off);
	k_work_reschedule(&system_off_work, K_SECONDS(SYSTEM_OFF_DELAY_S));

	/* Show last reset reason */
	print_reset_reason();

	/* Start NFC */
	if (start_nfc() < 0) {
		printk("ERROR: NFC configuration failed\n");
		return 0;
	}

	/* Prevent deep sleep (system off) from being entered */
	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);

	/* Exit main function - the rest will be done by the callbacks */
	return 0;
}
