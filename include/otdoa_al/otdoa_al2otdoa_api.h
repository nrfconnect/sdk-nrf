/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef PHYWI_AL2OTDOA_API__
#define PHYWI_AL2OTDOA_API__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * Library Functions called by the AL
 */
void otdoa_ctrl_handle_message(void *pvMsg);
extern const unsigned int OTDOA_MAX_MESSAGE_SIZE;

/* Timer number definitions */
enum {
	OTDOA_TIMER_WAIT_UPLOAD_TIMEOUT,
	OTDOA_TIMER_PRS_MEASURE_TIMEOUT,

	OTDOA_MAX_TIMERS
};

void otdoa_handle_timeout(uint32_t u_timer_no);

/**
 * Generate a JWT token
 *
 * @param[out] buffer Pointer to store generated token in
 * @param[in] len Length of buffer
 * @param[out] olen Length of generated token
 * @return 0 on success, otherwise negative error code
 */
int generate_jwt(char *buffer, size_t len, size_t *olen);

/**
 * Derive a new AES key from two ECDH keys.
 *
 * @param[in]   p_svr_pubkey    Pointer to Server pub key (binary format, not ASCII)
 * @param[in]   pubkey_len      Length of the public key
 */
int otdoa_crypto_set_new_key(const uint8_t *p_svr_pubkey, const size_t pubkey_len);

/** start file encryption and storage
 * H1 Version :  Data may already be encrypted by the server.
 * So we need to pass in the encryption IV.
 *
 *      enable_ue_encr  pu8_iv_in       Function
 *          0              valid        Data encrypted by server.  Store IV from server along with
 * the data 0              NULL         Store unencrypted data.  IV set to 0s to indicate data is
 * unencrypted on read 1               X           Data NOT encrypted by server.  Encrypt it here.
 * IV generated locally
 */
int otdoa_ubsa_proc_start_h1(
	const char *psz_path,
	int enable_ue_encr, /* encrypt uBSA locally on UE (i.e. it is not ecrypted by server) */
	int is_compressed, uint32_t u32_comp_window,
	uint8_t *pu8_iv_in); /* IV if uBSA encrypted by server, NULL otherwise */

/**
 * Encrypt and write an arbitrary-size block of data to the uBSA file
 *
 */
int otdoa_ubsa_write_data(void *pv_data, size_t u_len);

/**
 * write out the remaining data with zero padding
 * and close/cleanup the write operation
 *
 * This function should be called after the last data block
 * is passed in using otdoa_ubsa_write_data().
 */
int otdoa_ubsa_finish_data(void);

void otdoa_crypto_abort(void);
void otdoa_ubsa_proc_close_file(void);
void otdoa_ubsa_proc_remove_file(const char *pszFile);

const char * const OTDOA_pxlGetBSAPath(void);
void OTDOA_pxlSetBSAPath(const char *pszPath);

/**
 * otdoa_rs_handle_msg() - Handle main thread messages (Nordic RS API)
 */
int otdoa_rs_handle_msg(void *p_msg_union);

/**
 * Handler for stop request - does not get a full message, so
 * we create one.
 */
int otdoa_rs_handle_stop(uint32_t cancel_or_timeout);

#ifdef __cplusplus
}
#endif
#endif /* PHYWI_AL2OTDOA_API__ */
