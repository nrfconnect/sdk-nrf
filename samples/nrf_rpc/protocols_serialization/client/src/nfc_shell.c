/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>

#define NFC_MESSAGE_BUFFER 128

static uint8_t *default_message = "Hello World!";
static uint8_t *default_code = "en";

/* Buffer used to hold an NFC NDEF message. */
static uint8_t ndef_msg_buf[NFC_MESSAGE_BUFFER];

NFC_NDEF_MSG_DEF(msg_descr, 1);

static int message_encode(char *msg, uint32_t *length)
{
	int err;

	/* Create NFC NDEF text record description */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(record, UTF_8, default_code, strlen(default_code), msg,
				      strlen(msg));

	/* Clear the message description. */
	nfc_ndef_msg_clear(&NFC_NDEF_MSG(msg_descr));

	/* Add text records to NDEF text message */
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(msg_descr), &NFC_NDEF_TEXT_RECORD_DESC(record));
	if (err < 0) {
		return err;
	}

	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(msg_descr), ndef_msg_buf, length);
	__ASSERT(*length <= NFC_MESSAGE_BUFFER, "message too long");

	return err;
}

static void nfc_callback(void *context, nfc_t2t_event_t event, const uint8_t *data,
			 size_t data_length)
{
	static const char *const event_str[] = {"none", "field_on", "field_off", "data_read",
						"stopped"};
	const struct shell *sh = (const struct shell *)context;

	switch (event) {
	case NFC_T2T_EVENT_NONE:
	case NFC_T2T_EVENT_FIELD_ON:
	case NFC_T2T_EVENT_FIELD_OFF:
	case NFC_T2T_EVENT_DATA_READ:
	case NFC_T2T_EVENT_STOPPED:
		shell_print(sh, "NFC T2T event: %s", event_str[event]);
		break;
	default:
		shell_print(sh, "Unknown NFC event");
	}
}

static int nfc_init_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	int err = nfc_t2t_setup(nfc_callback, (void *)sh);
	uint32_t length = sizeof(ndef_msg_buf);

	if (err) {
		shell_print(sh, "Cannot setup NFC T2T library. err: %u", err);
		return -ENOEXEC;
	}

	if (message_encode(default_message, &length) < 0) {
		shell_print(sh, "Cannot encode message.");
		return -ENOEXEC;
	}

	/* Set created message as the NFC payload */
	if (nfc_t2t_payload_set(ndef_msg_buf, length) < 0) {
		shell_print(sh, "Cannot set payload.");
		return -ENOEXEC;
	}

	return 0;
}

static int nfc_release_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	int err = nfc_t2t_done();

	if (err) {
		shell_print(sh, "Cannot release NFC. err: %u", err);
		return -ENOEXEC;
	}

	return 0;
}

static int nfc_start_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	int err = nfc_t2t_emulation_start();

	if (err) {
		shell_print(sh, "Cannot activate NFC frontend. err: %u", err);
		return -ENOEXEC;
	}

	return 0;
}

static int nfc_stop_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	int err = nfc_t2t_emulation_stop();

	if (err) {
		shell_print(sh, "Cannot deactivate NFC frontend. err: %u", err);
		return -ENOEXEC;
	}

	return 0;
}

static int nfc_set_msg_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t length = sizeof(ndef_msg_buf);

	if (message_encode(argv[1], &length) < 0) {
		shell_print(sh, "Cannot encode message.");
		return -ENOEXEC;
	}

	/* Set created message as the NFC payload */
	if (nfc_t2t_payload_set(ndef_msg_buf, length) < 0) {
		shell_print(sh, "Cannot set payload.");
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nfc_cmds, SHELL_CMD_ARG(init, NULL, "Initialize NFC", nfc_init_cmd, 1, 0),
	SHELL_CMD_ARG(release, NULL, "Release NFC", nfc_release_cmd, 1, 0),
	SHELL_CMD_ARG(start, NULL, "Activate the NFC frontend", nfc_start_cmd, 1, 0),
	SHELL_CMD_ARG(stop, NULL, "Deactivate the NFC frontend", nfc_stop_cmd, 1, 0),
	SHELL_CMD_ARG(set_new_msg, NULL, "Set the new message to NFC payload", nfc_set_msg_cmd, 2,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(nfc, &nfc_cmds, "NFC demonstration commands", NULL, 1, 0);
