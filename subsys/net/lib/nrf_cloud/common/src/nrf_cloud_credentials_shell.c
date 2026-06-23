/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_credentials_keygen.h>

/* DER-encoded P-256 CSR is well under this size. */
#define CSR_DER_BUF_SIZE   512
/* Base64 of the DER plus the NULL terminator. */
#define CSR_B64_BUF_SIZE   (((CSR_DER_BUF_SIZE + 2) / 3) * 4 + 1)
#define CSR_SUBJECT_SIZE   128

static uint8_t csr_der_buf[CSR_DER_BUF_SIZE];
static char csr_b64_buf[CSR_B64_BUF_SIZE];
static char csr_subject_buf[CSR_SUBJECT_SIZE];

/* Resolve the sec tag argument, defaulting to the nRF Cloud sec tag. */
static int parse_sec_tag(const struct shell *sh, size_t argc, char *argv[], size_t idx,
			 uint32_t *sec_tag)
{
	if (argc > idx) {
		char *end;
		long val = strtol(argv[idx], &end, 0);

		if (*end != '\0' || val < 0) {
			shell_error(sh, "Invalid sec tag: %s", argv[idx]);
			return -EINVAL;
		}
		*sec_tag = (uint32_t)val;
	} else {
		*sec_tag = nrf_cloud_sec_tag_get();
	}

	return 0;
}

/* Resolve the CSR subject argument, defaulting to "CN=<device id>". */
static int build_subject(const struct shell *sh, size_t argc, char *argv[], size_t idx,
			 const char **subject)
{
	if (argc > idx) {
		*subject = argv[idx];
		return 0;
	}

	char id_buf[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
	int err = nrf_cloud_client_id_get(id_buf, sizeof(id_buf));

	if (err) {
		shell_error(sh, "Failed to get device id: %d (pass a subject explicitly)", err);
		return err;
	}

	(void)snprintk(csr_subject_buf, sizeof(csr_subject_buf), "CN=%s", id_buf);
	*subject = csr_subject_buf;

	return 0;
}

static int cmd_keygen(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t sec_tag;
	int err = parse_sec_tag(sh, argc, argv, 1, &sec_tag);

	if (err) {
		return err;
	}

	err = nrf_cloud_credentials_key_generate(sec_tag);
	if (err == -EALREADY) {
		shell_warn(sh, "Key already exists in sec tag %u. Use 'delete' to remove it first.",
			   sec_tag);
		return err;
	} else if (err) {
		shell_error(sh, "Key generation failed: %d", err);
		return err;
	}

	shell_print(sh, "Generated device key in sec tag %u", sec_tag);

	return 0;
}

static int print_csr(const struct shell *sh, uint32_t sec_tag, const char *subject)
{
	size_t der_len;
	size_t b64_len;
	int err;

	err = nrf_cloud_credentials_csr_generate(sec_tag, subject, csr_der_buf,
						 sizeof(csr_der_buf), &der_len);
	if (err) {
		shell_error(sh, "CSR generation failed: %d", err);
		return err;
	}

	err = base64_encode(csr_b64_buf, sizeof(csr_b64_buf), &b64_len, csr_der_buf, der_len);
	if (err) {
		shell_error(sh, "Base64 encoding failed: %d", err);
		return err;
	}

	csr_b64_buf[b64_len] = '\0';
	/* Emit the CSR with a stable prefix and a completion marker so host-side
	 * tooling can reliably capture the Base64 DER from the shell output.
	 */
	shell_print(sh, "CSR: %s", csr_b64_buf);
	shell_print(sh, "CSR generation complete");

	return 0;
}

static int cmd_csr(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t sec_tag;
	const char *subject;
	int err = parse_sec_tag(sh, argc, argv, 1, &sec_tag);

	if (err) {
		return err;
	}

	err = build_subject(sh, argc, argv, 2, &subject);
	if (err) {
		return err;
	}

	return print_csr(sh, sec_tag, subject);
}

static int cmd_delete(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t sec_tag;
	int err = parse_sec_tag(sh, argc, argv, 1, &sec_tag);

	if (err) {
		return err;
	}

	err = nrf_cloud_credentials_key_delete(sec_tag);
	if (err) {
		shell_error(sh, "Key deletion failed: %d", err);
		return err;
	}

	shell_print(sh, "Deleted device key in sec tag %u", sec_tag);

	return 0;
}

/* SEC1 uncompressed P-256 public key: 0x04 || X || Y. */
#define PUBKEY_RAW_SIZE 65

static int cmd_pubkey(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t sec_tag;
	uint8_t pubkey_raw[PUBKEY_RAW_SIZE];
	size_t raw_len;
	size_t b64_len;
	int err = parse_sec_tag(sh, argc, argv, 1, &sec_tag);

	if (err) {
		return err;
	}

	err = nrf_cloud_credentials_pubkey_get(sec_tag, pubkey_raw, sizeof(pubkey_raw), &raw_len);
	if (err) {
		shell_error(sh, "Public key export failed: %d", err);
		return err;
	}

	err = base64_encode(csr_b64_buf, sizeof(csr_b64_buf), &b64_len, pubkey_raw, raw_len);
	if (err) {
		shell_error(sh, "Base64 encoding failed: %d", err);
		return err;
	}

	csr_b64_buf[b64_len] = '\0';
	/* Stable prefix and completion marker for host-side capture. */
	shell_print(sh, "PUBKEY: %s", csr_b64_buf);
	shell_print(sh, "Public key export complete");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf_cloud_cred_cmds,
	SHELL_CMD_ARG(keygen, NULL,
		      "Generate a device key.\n"
		      "Usage: nrf_cloud_cred keygen [<sec_tag>]",
		      cmd_keygen, 1, 1),
	SHELL_CMD_ARG(csr, NULL,
		      "Generate and print a Base64 DER CSR for the device key.\n"
		      "Usage: nrf_cloud_cred csr [<sec_tag>] [<subject>]",
		      cmd_csr, 1, 2),
	SHELL_CMD_ARG(delete, NULL,
		      "Delete the device key.\n"
		      "Usage: nrf_cloud_cred delete [<sec_tag>]",
		      cmd_delete, 1, 1),
	SHELL_COND_CMD_ARG(CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY, pubkey, NULL,
			   "Export and print the device public key (Base64).\n"
			   "Usage: nrf_cloud_cred pubkey [<sec_tag>]",
			   cmd_pubkey, 1, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nrf_cloud_cred, &nrf_cloud_cred_cmds,
		   "nRF Cloud on-device credential commands", NULL);
