/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <nrf_modem_at.h>
#include <modem/uicc_lwm2m.h>

#include "pkcs15_decode.h"

static int csim_send(const uint8_t *csim_command, uint8_t *response, int buffer_size)
{
	uint8_t csim_fmt[20];
	int length, rc;

	/* Create csim_fmt based on buffer size */
	rc = snprintf(csim_fmt, sizeof(csim_fmt), "+CSIM: %%d,\"%%%ds\"", buffer_size - 1);
	if (rc >= (int)sizeof(csim_fmt)) {
		return -EINVAL;
	}

	/* Use response buffer for both command and response */
	rc = snprintf(response, buffer_size, "AT+CSIM=%u,\"%s\"",
		      (unsigned int)strlen(csim_command), csim_command);
	if (rc >= buffer_size) {
		return -EINVAL;
	}

	/* Send AT command */
	rc = nrf_modem_at_scanf(response, csim_fmt, &length, response);
	if (rc < 0) {
		return rc;
	}

	/* Check for success and trailing status word 9000 in response */
	if ((rc != 2) || (length < 4) || (memcmp(&response[length - 4], "9000", 4) != 0)) {
		return -EINVAL;
	}

	/* Remove status word from response */
	return length - 4;
}

static int csim_read_file(const uint8_t *path, uint8_t *response, int buffer_size)
{
	uint8_t csim_select[] = "01A40804047FFF****00";
	uint8_t csim_read[] = "01B0000000";
	int length;

	/* Select path */
	memcpy(&csim_select[14], path, 4);
	length = csim_send(csim_select, response, buffer_size);
	if (length <= 0) {
		return length;
	}

	/* Check buffer size, needs to be max*2 + 4 bytes for SW for AT response */
	if (buffer_size < UICC_RECORD_BUFFER_MAX) {
		/* Expected maximum response length: 1-255, 0=256 */
		snprintf(&csim_read[8], 3, "%2.2X", (uint8_t)((buffer_size - 4) / 2));
	}

	/* Read path */
	length = csim_send(csim_read, response, buffer_size);
	if (length <= 0) {
		return length;
	}

	/* Convert from hex to binary (inplace) */
	length = hex2bin(response, length, response, length);

	return length;
}

static int uicc_bootstrap_read_records(uint8_t *buffer, int buffer_size)
{
	pkcs15_object_t pkcs15_object;
	int length;

	/* Read EF(ODF) */
	length = csim_read_file("5031", buffer, buffer_size);
	if (length <= 0) {
		return length;
	}

	/* Decode PKCS #15 EF(ODF) Path */
	memset(&pkcs15_object, 0, sizeof(pkcs15_object));
	if (!pkcs15_ef_odf_path_decode(buffer, length, &pkcs15_object)) {
		return -ENOENT;
	}

	/* Check if EF(DODF) Path is found */
	if (pkcs15_object.path[0] == 0) {
		return -ENOENT;
	}

	/* Read EF(DODF) */
	length = csim_read_file(pkcs15_object.path, buffer, buffer_size);
	if (length <= 0) {
		return length;
	}

	/* Decode PKCS #15 EF(DODF) Path */
	memset(&pkcs15_object, 0, sizeof(pkcs15_object));
	if (!pkcs15_ef_dodf_path_decode(buffer, length, &pkcs15_object)) {
		return -ENOENT;
	}

	/* Check if EF(DODF-bootstrap) Path is found */
	if (pkcs15_object.path[0] == 0) {
		return -ENOENT;
	}

	/* Read EF(DODF-bootstrap) */
	length = csim_read_file(pkcs15_object.path, buffer, buffer_size);

	return length;
}

int uicc_lwm2m_bootstrap_read(uint8_t *buffer, int buffer_size)
{
	int length;

	/* Open a logical channel 1 */
	length = csim_send("0070000001", buffer, buffer_size);
	if (length <= 0) {
		return length;
	}

	/* Select PKCS#15 on channel 1 using the default AID */
	length = csim_send("01A404040CA000000063504B43532D313500", buffer, buffer_size);

	if (length > 0) {
		/* Read bootstrap records */
		length = uicc_bootstrap_read_records(buffer, buffer_size);
	}

	/* Close the logical channel (using separate buffer to keep content from last file) */
	uint8_t close_response[21];
	int close_length;

	close_length = csim_send("01708001", close_response, sizeof(close_response));
	if (length >= 0 && close_length < 0) {
		return close_length;
	}

	return length;
}
