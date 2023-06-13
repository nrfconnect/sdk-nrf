/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "zcbor_common.h"

#include "nrf_provisioning_at.h"
#include "nrf_provisioning_codec.h"
#include <nrf_provisioning_cbor_decode.h>
#include <nrf_provisioning_cbor_encode.h>
#include "nrf_provisioning_cbor_decode_types.h"
#include "nrf_provisioning_cbor_encode_types.h"

#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <nrf_modem_at.h>


LOG_MODULE_REGISTER(nrf_provisioning_codec, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

#define CDC_OPKT_SZ_PTR(ctx) (&(ctx)->opkt_sz)

#define CDC_IFMT_DATA_PTR(ctx) ((struct cdc_in_fmt_data *)(ctx)->i_data)
#define CDC_IFMT_CMD_I_GET(ctx, i) (&(CDC_IFMT_DATA_PTR(ctx))->cmds._commands__command[(i)])

#define CDC_OFMT_DATA_PTR(ctx) ((struct cdc_out_fmt_data *)(ctx)->o_data)
#define CDC_OFMT_RESPONSES_GET(ctx) (&CDC_OFMT_DATA_PTR(ctx)->rsps)

#define CDC_OFMT_RESPONSE_CONSUME(ofd) \
	(&((ofd)->rsps._responses__response[(ofd)->rsps._responses__response_count++]))

#define AT_RESP_MAX_SIZE 4096

static struct nrf_provisioning_mm_change mm;

static struct cdc_out_fmt_data {
	/* Data */
	struct responses rsps;
	char *msgs[CONFIG_NRF_PROVISIONING_CBOR_RECORDS];
	struct {
		char *at_buff;
		size_t at_buff_sz;
	};
	bool errors;
} o_fmt_data;

static struct cdc_in_fmt_data {
	/* Data */
	struct commands cmds;
} i_fmt_data;

static struct cdc_context *cctx;

/* Correlation ID if the latest successfully executed provisioning command */
static char nrf_provisioning_latest_corr_id[NRF_PROVISIONING_CORRELATION_ID_SIZE];

char * const nrf_provisioning_codec_get_latest_cmd_id(void)
{
	return nrf_provisioning_latest_corr_id;
}

int nrf_provisioning_codec_init(struct nrf_provisioning_mm_change *mmode)
{
	mm.cb = mmode->cb;
	mm.user_data = mmode->user_data;

	return 0;
}

int nrf_provisioning_codec_setup(struct cdc_context *const cdc_ctx,
	char * const at_buff, size_t at_buff_sz)
{
	cctx = cdc_ctx;

	memset(&i_fmt_data, 0, sizeof(i_fmt_data));
	memset(&o_fmt_data, 0, sizeof(o_fmt_data));

	o_fmt_data.at_buff = at_buff;
	o_fmt_data.at_buff_sz = at_buff_sz;

	cctx->i_data = (void *)&i_fmt_data;
	cctx->o_data = (void *)&o_fmt_data;

	return 0;
}

int nrf_provisioning_codec_teardown(void)
{
	for (int i = 0; i < CONFIG_NRF_PROVISIONING_CBOR_RECORDS; i++) {
		k_free(o_fmt_data.msgs[i]);
		o_fmt_data.msgs[i] = NULL;
	}

	return 0;
}

static int fmt_range_check(struct cdc_out_fmt_data *fd)
{
	if (fd->rsps._responses__response_count >= CONFIG_NRF_PROVISIONING_CBOR_RECORDS) {
		LOG_ERR("CONFIG_NRF_PROVISIONING_CBOR_RECORDS too small");
		return -ENOMEM;
	}

	return 0;
}

static int put_finished_resp(struct command *cmd_req, struct cdc_out_fmt_data *out)
{
	int ret = fmt_range_check(out);

	if (ret < 0) {
		return ret;
	}

	struct response *rsp = CDC_OFMT_RESPONSE_CONSUME(out);

	rsp->_response_union_choice = _response_union__finished_ack;
	rsp->_response__correlation = cmd_req->_command__correlation;

	return 0;
}

static int put_at_resp(struct command *cmd_req, struct cdc_out_fmt_data *out, const char *msg)
{
	int ret = fmt_range_check(out);

	if (ret < 0) {
		return ret;
	}

	struct response *rsp = CDC_OFMT_RESPONSE_CONSUME(out);

	rsp->_response__correlation = cmd_req->_command__correlation;
	rsp->_response_union_choice = _response_union__at_response;
	rsp->_response_union__at_response._at_response_message.value = msg;
	rsp->_response_union__at_response._at_response_message.len = strlen(msg);

	return 0;
}

static int put_config_resp(struct command *cmd_req, struct cdc_out_fmt_data *out)
{
	int ret = fmt_range_check(out);

	if (ret < 0) {
		return ret;
	}

	struct response *rsp = CDC_OFMT_RESPONSE_CONSUME(out);

	rsp->_response__correlation = cmd_req->_command__correlation;
	rsp->_response_union_choice = _response_union__config_ack;

	return 0;
}

static int put_err_resp(struct command *cmd_req, struct cdc_out_fmt_data *out,
			int cme_error, const char *msg)
{
	int ret = fmt_range_check(out);

	/* Indicate that we have an error */
	out->errors = true;

	if (ret < 0) {
		return ret;
	}

	struct response *rsp = CDC_OFMT_RESPONSE_CONSUME(out);

	rsp->_response__correlation = cmd_req->_command__correlation;
	rsp->_response_union_choice = _response_union__error_response;
	rsp->_response_union__error_response._error_response_cme_error = cme_error;
	rsp->_response_union__error_response._error_response_message.value = msg;
	rsp->_response_union__error_response._error_response_message.len = strlen(msg);

	return 0;
}

/**
 * @brief Store a character sequence as a null terminated C string
 *
 * @param buf Destination buffer
 * @param zstr ZCBOR string, value and length
 * @param buflen Destination buffer size
 * @return int String length
 */
static int charseq2cstr(char *buf, struct zcbor_string *zstr, size_t buflen)
{
	if (zstr->len > buflen-1) {
		__ASSERT(false, "Can't fit a CBOR string to a given buffer");
		return -ENOMEM;
	}

	memcpy(buf, zstr->value, zstr->len);
	buf[zstr->len] = '\0';

	return strlen(buf);
}

static int form_at_str(struct command *cmd_req, char *cmd, size_t cmd_sz)
{
	int ret;
	int slen = 0;

	ret = snprintk(cmd, cmd_sz, "AT");
	if (ret != sizeof("AT")-1) {
		ret = -EINVAL;
		goto out;
	}
	slen += ret;

	ret = charseq2cstr(cmd + slen,
		&cmd_req->_command_union__at_command._at_command_set_command,
		cmd_sz - slen);
	if (ret < 0) {
		goto out;
	}
	slen += ret;

	ret = snprintk(cmd + slen, cmd_sz - slen, "=");
	if (ret != sizeof("=")-1) {
		ret = -EINVAL;
		goto out;
	}
	slen += ret;

	ret = charseq2cstr(cmd + strlen(cmd),
		&cmd_req->_command_union__at_command._at_command_parameters,
		cmd_sz - slen);
	if (ret < 0) {
		goto out;
	}
	slen += ret;

	if (slen >= cmd_sz) {
		__ASSERT(false, "AT command out of bounds");
	}

out:
	return ret;
}

int filter_cme_error(struct command *cmd, int cme_error)
{
	bool filtered = false;

	struct at_command *at_cmd = &cmd->_command_union__at_command;

	for (int i = 0;	i < at_cmd->_at_command_ignore_cme_errors_uint_count; i++) {
		if (at_cmd->_at_command_ignore_cme_errors_uint[i] == cme_error) {
			filtered = true;
		}
	}

	return filtered ? 0 : cme_error;
}

static int exec_at_cmd(struct command *cmd_req, struct cdc_out_fmt_data *out)
{
	int ret;
	char *resp;
	size_t resp_sz = CONFIG_NRF_PROVISIONING_CODEC_RX_SZ_START;

	ret = form_at_str(cmd_req, out->at_buff, out->at_buff_sz);
	if (ret < 0) {
		resp = NULL;
		goto out;
	}

	while (true) {
		resp = k_malloc(resp_sz);
		if (!resp) {
			LOG_ERR("Unable to write response msg field");
			return -ENOMEM;
		}

		LOG_DBG("command: \"%s\", input buff len: %d", out->at_buff, resp_sz);
		ret = nrf_provisioning_at_cmd(resp, resp_sz, out->at_buff);

		if (ret == -E2BIG) {
			int tag = 0;
			int type = 0;

			LOG_DBG("Buffer too small for AT response, retrying");
			k_free(resp);
			resp = NULL;
			if (sscanf(out->at_buff, "AT%%KEYGEN=%d,%d,%*s", &tag, &type) == 2) {
				LOG_DBG("Clear sec_tag %d, type %d", tag, type);
				ret = nrf_provisioning_at_del_credential(tag, type);
				if (ret < 0) {
					LOG_ERR("AT cmd failed, error: %d", ret);
				}
			}
			resp_sz *= 2; /* Previous size wasn't sufficient */
			if (resp_sz > AT_RESP_MAX_SIZE) {
				LOG_ERR("Key or CSR too big");
				break;
			} else if (resp_sz > (AT_RESP_MAX_SIZE / 2)) {
				resp_sz = AT_RESP_MAX_SIZE;
			}
			continue;
		} else if (ret < 0) {
			LOG_ERR("AT cmd failed, error: %d", ret);
		} else if (ret > 0) {
			ret = filter_cme_error(cmd_req, nrf_modem_at_err(ret));
			/* 'ERROR' or '+CME ERROR'. Doesn't matter which */
			LOG_INF("AT cmd failed, type %d, err %d",
				nrf_modem_at_err_type(ret), nrf_modem_at_err(ret));
		}
		break;
	}

out:
	/* Keeps track of response messages to be freed once the whole responses request is send */
	for (int i = 0; i < CONFIG_NRF_PROVISIONING_CBOR_RECORDS; i++) {
		if (out->msgs[i] == NULL) {
			out->msgs[i] = resp;
			break;
		}
	}

	if (ret == 0) {
		return put_at_resp(cmd_req, out, resp);
	} else if (ret < 0) {
		return put_err_resp(cmd_req, out, 0, resp);
	}

	return put_err_resp(cmd_req, out, ret, resp);
}

static int write_config(struct command *cmd_req, struct cdc_out_fmt_data *out)
{
	struct config *aconf = &cmd_req->_command_union__config;
	int max_key_sz = 1;
	int max_value_sz = 1;
	struct properties_tstrtstr *pair;
	char *key;
	char *value = NULL;
	int ret = 0;

	char *resp = NULL;
	size_t resp_sz = CONFIG_NRF_PROVISIONING_CODEC_RX_SZ_START;

	/* Figure out the max key and value legths */
	for (int i = 0; i < aconf->_properties_tstrtstr_count; i++) {
		pair = &aconf->_properties_tstrtstr[i];

		max_key_sz = MAX(max_key_sz, pair->_config_properties_tstrtstr_key.len + 1);
		max_value_sz = MAX(max_value_sz, pair->_properties_tstrtstr.len + 1);
	}

	key = k_malloc(max_key_sz);
	if (!key) {
		ret = -ENOMEM;
		goto exit;
	}

	value = k_malloc(max_value_sz);
	if (!value) {
		ret = -ENOMEM;
		goto exit;
	}

	resp = k_malloc(resp_sz);
	if (!resp) {
		ret = -ENOMEM;
		goto exit;
	}

	LOG_DBG("Storing %d key-value pair/s", aconf->_properties_tstrtstr_count);

	for (int i = 0; i < aconf->_properties_tstrtstr_count; i++) {
		pair = &aconf->_properties_tstrtstr[i];

		ret = charseq2cstr(key, &pair->_config_properties_tstrtstr_key, max_key_sz);
		if (ret < 0) {
			LOG_WRN("Unable to store key: %s; err: %d", key, ret);
			snprintk(resp, resp_sz, "key [%d](0-indexed) too big", i);
			goto exit;
		}

		ret = charseq2cstr(value, &pair->_properties_tstrtstr, max_value_sz);
		if (ret < 0) {
			LOG_WRN("Unable to store value: %s; err: %d", value, ret);
			snprintk(resp, resp_sz, "value [%d](0-indexed) too big", i);
			goto exit;
		}

		ret = settings_save_one(key, value, strlen(value)+1);
		if (ret) {
			snprintk(resp, resp_sz,
				"unable to store [%d](0-indexed) key-value-pair", i);
			LOG_WRN("Unable to store key: %s; value: %s; err: %d", key, value, ret);
			goto exit;
		}
		LOG_DBG("Stored key: \"%s\"; value: \"%s\"", key, value);
	}

	/* No error to report */
	k_free(resp);
	resp = NULL;

exit:
	if (key) {
		k_free(key);
	}
	if (value) {
		k_free(value);
	}

	/* Keeps track of response messages to be freed once the whole responses request is send */
	for (int i = 0; i < CONFIG_NRF_PROVISIONING_CBOR_RECORDS; i++) {
		if (out->msgs[i] == NULL) {
			out->msgs[i] = resp;
			break;
		}
	}

	if (ret == 0) {
		return put_config_resp(cmd_req, out);
	} else if (ret < 0) {
		/* Only error string is used but lets put the code too */
		return put_err_resp(cmd_req, out, ret, resp);
	}

	return ret;
}

static void decode_fail_info(int err, void *payload, int len)
{
	/* No issues */
	if (err == ZCBOR_SUCCESS) {
		return;
	}

	LOG_ERR("Decoding commands response failed, reason %d", err);
	LOG_HEXDUMP_ERR(payload, len, "Payload: ");

	switch (err) {
	case ZCBOR_ERR_HIGH_ELEM_COUNT:
		LOG_ERR("CONFIG_NRF_PROVISIONING_CBOR_RECORDS is too small");
	}
}

int nrf_provisioning_codec_process_commands(void)
{
	int mret, ret;
	struct cdc_in_fmt_data *ifd = CDC_IFMT_DATA_PTR(cctx);
	struct cdc_out_fmt_data *ofd = CDC_OFMT_DATA_PTR(cctx);
	enum lte_lc_func_mode orig;
	int fpos = -1;
	bool cmee_orig;

	ret = cbor_decode_commands(cctx->ipkt, cctx->ipkt_sz,
					cctx->i_data, &cctx->i_data_sz);
	if (ret != ZCBOR_SUCCESS) {
		decode_fail_info(ret, cctx->ipkt, cctx->ipkt_sz);
		ret = -EINVAL;
		return ret;
	}
	LOG_HEXDUMP_DBG(cctx->ipkt, cctx->ipkt_sz, "command response: ");

	mret = lte_lc_func_mode_get(&orig);
	if (mret < 0) {
		return mret;
	}

	cmee_orig = nrf_provisioning_at_cmee_enable();

	LOG_DBG("Received %d commands, size %d", ifd->cmds._commands__command_count,
		cctx->i_data_sz);

	for (int i = 0; i < ifd->cmds._commands__command_count; i++) {

		/* Stop immediately processing if any command has failed */
		if (ofd->errors) {
			goto stop_provisioning;
		}

		switch (CDC_IFMT_CMD_I_GET(cctx, i)->_command_union_choice) {
		case _command_union__finished:
			fpos = i; /* Defer writing FINISHED until we know if we have errors */
			break;
		case _command_union__at_command:
			mret = mm.cb(LTE_LC_FUNC_MODE_OFFLINE, mm.user_data);
			if (mret < 0) {
				goto stop_provisioning;
			}

			ret = exec_at_cmd(CDC_IFMT_CMD_I_GET(cctx, i), cctx->o_data);
			if (ret < 0) {
				goto stop_provisioning;
			}
			break;
		case _command_union__config:
			ret = write_config(CDC_IFMT_CMD_I_GET(cctx, i), cctx->o_data);
			if (ret < 0) {
				goto stop_provisioning;
			}
			break;
		default:
			__ASSERT_NO_MSG(false);
			ret = -ENOSYS;
			goto stop_provisioning;
		}

		/* Mark as the latest successful provisioning command */
		memcpy(nrf_provisioning_latest_corr_id,
			CDC_IFMT_CMD_I_GET(cctx, i)->_command__correlation.value,
			CDC_IFMT_CMD_I_GET(cctx, i)->_command__correlation.len);
		nrf_provisioning_latest_corr_id[
			CDC_IFMT_CMD_I_GET(cctx, i)->_command__correlation.len] = '\0';
	}

stop_provisioning:

	/* Restore to original state */
	nrf_provisioning_at_cmee_control(cmee_orig ?
		NRF_PROVISIONING_AT_CMEE_STATE_ENABLE :
		NRF_PROVISIONING_AT_CMEE_STATE_DISABLE);

	mret = mm.cb(orig, mm.user_data);

	/* Can't send anything if modem connection is not restored */
	if (mret < 0) {
		return mret;
	}

	/* Respond to FINISHED only if there are no errors */
	if ((fpos >= 0) && !(CDC_OFMT_DATA_PTR(cctx)->errors)) {
		put_finished_resp(CDC_IFMT_CMD_I_GET(cctx, fpos), cctx->o_data);
	} else {
		fpos = -1;
	}

	/* Encoded size will replace max size */
	LOG_DBG("Payload %p size %d", cctx->opkt, cctx->opkt_sz);
	ret = cbor_encode_responses(cctx->opkt, cctx->opkt_sz,
				    CDC_OFMT_RESPONSES_GET(cctx),
				    CDC_OPKT_SZ_PTR(cctx));

	if (ret != ZCBOR_SUCCESS) {
		LOG_ERR("Invalid responses request, zcbor err: %d", ret);
		return -ENODATA;
	}

	/* 0 not finished, >0 finished */
	return fpos >= 0 ? NRF_PROVISIONING_FINISHED : 0;
}
