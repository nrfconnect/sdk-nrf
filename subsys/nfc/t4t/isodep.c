/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>
#include <nfc/t4t/isodep.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_t4t_isodep, CONFIG_NFC_T4T_ISODEP_LOG_LEVEL);

#define ISODEP_BLOCK_FIXED_BIT BIT(1)
#define ISODEP_CRC_LENGTH 2

#define ISODEP_I_BLOCK 0x02
#define ISODEP_R_BLOCK 0xA2
#define ISODEP_S_BLOCK 0xC2

#define ISODEP_S_BLOCK_INDICATOR 0x40
#define ISODEP_R_BLOCK_INDICATOR 0x80

#define ISODEP_BLOCK_NUM_MASK 0x01
#define ISODEP_MAX_RETRANSMIT_CNT 2
#define ISODEP_FWT_DEACTIVATION 71680
#define ISODEP_WTXM_MASK 0x3F
#define ISODEP_WTXM_MIN 1
#define ISODEP_WTXM_MAX 59

#define I_BLOCK_DID_BIT BIT(3)
#define I_BLOCK_NAD_BIT BIT(2)
#define I_BLOCK_CHAINING_BIT BIT(4)
#define R_BLOCK_NAK BIT(4)
#define S_BLOCK_WTX_MASK 0x30

#define T4T_FSD_MIN 16

#define T4T_RATS_CMD 0xE0
#define T4T_RATS_DID_MASK 0x0F
#define T4T_RATS_FSDI_MASK 0xF0
#define T4T_RATS_FSDI_OFFSET 4
#define T4T_RATS_CMD_LEN 0x02

#define T4T_FWT_ACTIVATION 71680
#define T4T_FWT_DELTA 49152

#define NFCA_T4T_FWT_T_FC (164 * 1356UL)
#define T4T_FC_IN_MS 13560UL

#define T4T_FWT_TO_MS(_fwt)                   \
	DIV_ROUND_UP((_fwt), T4T_FC_IN_MS)

#define T4T_RATS_FDT (T4T_FWT_ACTIVATION +        \
		T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC)

#define T4T_DID_MAX 14

#define T4T_ATS_MAX_LEN 20
#define T4T_ATS_MIN_LEN 2
#define T4T_ATS_T0_TA_PRESENT BIT(4)
#define T4T_ATS_T0_TB_PRESENT BIT(5)
#define T4T_ATS_T0_TC_PRESENT BIT(6)
#define T4T_ATS_TA_EQUAL_DIVISOR BIT(7)
#define T4T_ATS_T0_FSCI_MASK 0x0F
#define T4T_ATS_TA_DIVISOR_PL 0x07
#define T4T_ATS_TA_DIVISOR_PL_OFFSET 1
#define T4T_ATS_TA_DIVISOR_LP_OFFSET 3
#define T4T_ATS_TA_DIVISOR_LP 0x70
#define T4T_ATS_TB_FWI_DEFAULT 0x04
#define T4T_ATS_TB_FWI_MASK 0xF0
#define T4T_ATS_TB_FWI_OFFSET 4
#define T4T_ATS_TB_SFGI_MASK 0x0F
#define T4T_ATS_TC_DID BIT(1)
#define T4T_ATS_TC_NAD BIT(0)
#define T4T_ATS_SFGT_DEFAULT 6780

/* Convert FWI to FWT. NFC Forum Digital Specification 2.0 14.8.1. */
#define T4T_FWI_TO_FWT(_fwi) (256u * 16u * (1 << (_fwi)))

/* Calculate SFGT margin. NFC Forum Digital Specification 2.0 14.8.2. */
#define T4T_SFGT_DELTA(_sfgi) (384 * (1 << (_sfgi)))
#define ISODEP_BLOC_NUM_TOOGLE(block) ((block) ^ 0x01)

enum isodep_frame {
	ISODEP_FRAME_NONE,
	ISODEP_FRAME_RATS,
	ISODEP_FRAME_DESELECT,
	ISODEP_FRAME_WTX_RESPONSE,
	ISODEP_FRAME_I,
	ISODEP_FRAME_I_CHAINING
};

enum isodep_state {
	ISODEP_STATE_UNINITIALIZED    = 0,
	ISODEP_STATE_INITIALIZED,
	ISODEP_STATE_SELECTED,
	ISODEP_STATE_TRANSFER
};

struct nfc_t4t_buf {
	uint8_t *data;
	size_t len;
	size_t buf_size;
};

struct nfc_t4t_err {
	enum isodep_frame last_frame;
	uint8_t frame_retry_cnt;
	uint8_t wtx;
};

struct nfc_t4t_isodep {
	atomic_t state;
	struct nfc_t4t_isodep_tag tag;
	struct nfc_t4t_buf tx_data;
	struct nfc_t4t_buf rx_data;
	struct nfc_t4t_err err_status;
	uint16_t fsd;
	uint8_t block_num;
	uint8_t retransmit_cnt;
	const uint8_t *transmit_data;
	size_t transmit_len;
	size_t transmitted_len;
	bool chaining;
	bool equal_divisor;
	bool ats_expected;
	bool first_transfer;
};

/* Map FSD value in terms of FSDI according to NFC Forum Digital Specification 2.0 14.16.1 */
static const uint16_t fsd_value_map[] = {16, 24, 32, 40, 48, 64, 96, 128, 256};

static struct nfc_t4t_isodep t4t_isodep;
static const struct nfc_t4t_isodep_cb *t4t_isodep_cb;
static struct k_work_delayable isodep_work;
static int64_t ats_received_time;

static void isodep_transmission_clear(void)
{
	t4t_isodep.rx_data.len                = 0;
	t4t_isodep.transmitted_len            = 0;
	t4t_isodep.transmit_len               = 0;
	t4t_isodep.chaining                   = false;
	t4t_isodep.retransmit_cnt             = 0;
	t4t_isodep.err_status.frame_retry_cnt = 0;
	t4t_isodep.err_status.last_frame      = ISODEP_FRAME_NONE;
}

static size_t did_include(uint8_t *data, size_t pos)
{
	if (t4t_isodep.tag.did_supported && (t4t_isodep.tag.did != 0)) {
		data[pos] |= I_BLOCK_DID_BIT;
		pos++;
		data[pos] = t4t_isodep.tag.did;
	}

	pos++;

	return pos;
}

static void err_notify(int err)
{
	isodep_transmission_clear();

	if (t4t_isodep_cb->error) {
		t4t_isodep_cb->error(err);
	}
}

static int ats_parse(const uint8_t *data, size_t len)
{
	uint8_t tl;
	uint8_t t0;
	uint8_t ta;
	uint8_t tb;
	uint8_t tc;
	uint8_t fsci;
	uint8_t fwi;
	uint8_t sfgi;
	uint8_t index = 0;

	ats_received_time = k_uptime_get();

	if (len < T4T_ATS_MIN_LEN) {
		return -NFC_T4T_ISODEP_SYNTAX_ERROR;
	}

	tl = data[index];
	index++;

	if ((tl != len) || (len > T4T_ATS_MAX_LEN)) {
		LOG_ERR("Syntax error. ATS length is different than TL byte value");
		return -NFC_T4T_ISODEP_SYNTAX_ERROR;
	}

	t0 = data[index];
	index++;

	fsci = t0 & T4T_ATS_T0_FSCI_MASK;

	/* FSC is mapped from FSCI in the same way like FSD.
	 * NFC Forum Digital Specification 2.0 14.6.2.
	 */
	t4t_isodep.tag.fsc = fsd_value_map[fsci];

	/* Include space for CRC */
	t4t_isodep.tag.fsc -= ISODEP_CRC_LENGTH;

	/* Check id ATS contains interface bytes, if not
	 * set all data to default values according to
	 * NFC Forum Digital Specification 2.0 14.6.2.
	 */
	if (t0 & T4T_ATS_T0_TA_PRESENT) {
		ta = data[index];

		if (ta & T4T_ATS_TA_EQUAL_DIVISOR) {
			t4t_isodep.equal_divisor = true;
		}

		t4t_isodep.tag.pl_divisor =
			(ta & T4T_ATS_TA_DIVISOR_PL) << T4T_ATS_TA_DIVISOR_PL_OFFSET;
		t4t_isodep.tag.lp_divisor =
			(ta & T4T_ATS_TA_DIVISOR_LP) >> T4T_ATS_TA_DIVISOR_LP_OFFSET;

		index++;
	} else {
		t4t_isodep.tag.pl_divisor = 0;
		t4t_isodep.tag.lp_divisor = 0;
	}

	if (t0 & T4T_ATS_T0_TB_PRESENT) {
		tb = data[index];

		fwi = (tb & T4T_ATS_TB_FWI_MASK) >> T4T_ATS_TB_FWI_OFFSET;
		sfgi = tb & T4T_ATS_TB_SFGI_MASK;

		index++;
	} else {
		fwi = T4T_ATS_TB_FWI_DEFAULT;
		sfgi = 0;
	}

	t4t_isodep.tag.fwt = T4T_FWI_TO_FWT(fwi);

	if (sfgi != 0) {
		t4t_isodep.tag.sfgt =
			T4T_FWI_TO_FWT(sfgi) + T4T_SFGT_DELTA(sfgi);
	} else {
		t4t_isodep.tag.sfgt = T4T_ATS_SFGT_DEFAULT;
	}

	if (t0 & T4T_ATS_T0_TC_PRESENT) {
		tc = data[index];

		t4t_isodep.tag.did_supported = tc & T4T_ATS_TC_DID;
		t4t_isodep.tag.nad_supported = tc & T4T_ATS_TC_NAD;

		index++;
	} else {
		t4t_isodep.tag.did_supported = true;
		t4t_isodep.tag.nad_supported = false;
	}

	/* Check if ATS contains historical bytes. */
	if (index < len) {
		t4t_isodep.tag.historical_len = len - index;
		/* Maximum parsing length of the historical bytes is limited to 15
		 * NFC Forum Digital Specification 2.0 14.6.2.
		 */
		if (t4t_isodep.tag.historical_len > NFC_T4T_ISODEP_HIST_MAX_LEN) {
			t4t_isodep.tag.historical_len = NFC_T4T_ISODEP_HIST_MAX_LEN;
		}

		memcpy(t4t_isodep.tag.historical, &data[index],
		       t4t_isodep.tag.historical_len);
	}

	t4t_isodep.block_num = 0;
	t4t_isodep.first_transfer = true;

	atomic_set(&t4t_isodep.state, ISODEP_STATE_SELECTED);

	if (t4t_isodep_cb->selected) {
		t4t_isodep_cb->selected(&t4t_isodep.tag);
	}

	return 0;
}

static void isodep_chunk_send(void)
{
	size_t data_len;
	uint32_t fdt;
	size_t index = 0;
	const uint8_t *data = t4t_isodep.transmit_data;
	uint8_t *tx_data = t4t_isodep.tx_data.data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(tx_data);

	/* Prepare first chunk. */
	tx_data[index] = ISODEP_I_BLOCK | (t4t_isodep.block_num & 1);

	/* Check if DID field should be included. */
	index = did_include(tx_data, index);

	/* Use chaining when data is to long. */
	if ((t4t_isodep.tag.fsc - index) <
	    (t4t_isodep.transmit_len - t4t_isodep.transmitted_len)) {
		tx_data[0] |= I_BLOCK_CHAINING_BIT;
		data_len = t4t_isodep.tag.fsc - index;
		t4t_isodep.chaining = true;
	} else {
		data_len = t4t_isodep.transmit_len - t4t_isodep.transmitted_len;
		t4t_isodep.chaining = false;
	}

	/* Restore last frame type in case nfc_t4t_isodep_transmit() was called
	 * as it clears the transmission status.
	 */
	t4t_isodep.err_status.last_frame = ISODEP_FRAME_I;

	memcpy(&tx_data[index], &data[t4t_isodep.transmitted_len], data_len);

	index += data_len;
	t4t_isodep.tx_data.len = index;
	t4t_isodep.transmitted_len += data_len;

	fdt = t4t_isodep.tag.fwt + T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC;

	if (t4t_isodep_cb->ready_to_send) {
		t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
					     t4t_isodep.tx_data.len, fdt);
	}
}

static void block_num_toggle(void)
{
	t4t_isodep.block_num = ISODEP_BLOC_NUM_TOOGLE(t4t_isodep.block_num);
}

static void isodep_r_frame_send(bool ack)
{
	size_t index = 0;
	uint32_t fdt;
	uint8_t *tx_data = t4t_isodep.tx_data.data;

	tx_data[index] = ISODEP_R_BLOCK | (t4t_isodep.block_num & 1);

	if (!ack) {
		tx_data[index] |= R_BLOCK_NAK;

		LOG_DBG("Sending R(NAK)");
	} else {
		LOG_DBG("Sending R(ACK)");
	}

	/* Check if DID field should be included. */
	index = did_include(tx_data, index);

	fdt = t4t_isodep.tag.fwt + T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC;

	if (t4t_isodep_cb->ready_to_send) {
		t4t_isodep_cb->ready_to_send(tx_data, index, fdt);
	}
}

static int isodep_s_frame_handle(const uint8_t *data, size_t len)
{
	size_t index = 0;
	uint8_t s_block;
	uint8_t wtxm;
	uint32_t fdt;
	uint8_t *tx_data;

	__ASSERT_NO_MSG(len >= 1);

	s_block = data[index];
	index++;

	if (s_block & I_BLOCK_DID_BIT) {
		index++;
	}

	/* Check if S-frame does not contain Frame Waiting Time Extension. */
	if ((s_block & S_BLOCK_WTX_MASK) != S_BLOCK_WTX_MASK) {
		/* Check if S(DESELECT) Response. */
		if ((s_block & S_BLOCK_WTX_MASK) == 0) {
			if (len > index) {
				LOG_DBG("S-Frame S(DESELECT), invalid length received.");
				return -NFC_T4T_ISODEP_SYNTAX_ERROR;
			}

			atomic_set(&t4t_isodep.state, ISODEP_STATE_INITIALIZED);

			if (t4t_isodep_cb->deselected) {
				t4t_isodep_cb->deselected();
			}

			return 0;
		} else {
			return -NFC_T4T_ISODEP_SYNTAX_ERROR;
		}
	}

	/* Frame Waiting Time Extension Request contains 1 - byte
	 * INF field. Check if length of Request is correct.
	 */
	if (len > (index + 1)) {
		LOG_DBG("S-Frame S(WTX), invalid length received.");
		return -EINVAL;
	}

	wtxm = data[index];
	wtxm &= ISODEP_WTXM_MASK;

	/* Check if WTXM is in valid range. */
	if ((wtxm < ISODEP_WTXM_MIN) || (wtxm > ISODEP_WTXM_MAX)) {
		return -NFC_T4T_ISODEP_SYNTAX_ERROR;
	}

	/* Prepare S(WTX) response. */
	index = 0;
	tx_data = t4t_isodep.tx_data.data;

	tx_data[index] = ISODEP_S_BLOCK | S_BLOCK_WTX_MASK;

	/* Check if DID field should be included. */
	index = did_include(tx_data, index);

	/* Respond with the same WTXM. */
	tx_data[index] = wtxm;
	index++;

	t4t_isodep.tx_data.len = index;

	/* Calculate new Frame Delay Time value. */
	fdt  = t4t_isodep.tag.fwt * wtxm + T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC;

	/* In case of error, remember last send frame type. */
	t4t_isodep.err_status.last_frame = ISODEP_FRAME_WTX_RESPONSE;

	if (t4t_isodep_cb->ready_to_send) {
		t4t_isodep_cb->ready_to_send(tx_data, t4t_isodep.tx_data.len, fdt);
	}

	return 0;
}

static int isodep_r_frame_handle(const uint8_t *data, size_t len)
{
	size_t index = 0;
	uint8_t r_block;
	uint32_t fdt;

	__ASSERT_NO_MSG(len >= 1);

	r_block = data[index];
	index++;

	if (r_block & I_BLOCK_DID_BIT) {
		index++;
	}

	if (len > index) {
		return -NFC_T4T_ISODEP_SYNTAX_ERROR;
	}

	/* Check if ACK or NAK */
	if (r_block & R_BLOCK_NAK)  {
		LOG_DBG("R-frame NAK received.");
		return -NFC_T4T_ISODEP_SEMANTIC_ERROR;
	}

	LOG_DBG("R-frame ACK received.");

	/* Check if Reader/Writer is in chaining state. */
	if (!t4t_isodep.chaining) {
		return -NFC_T4T_ISODEP_SEMANTIC_ERROR;
	}

	/* ACK Received. Check if current block number is equal to receieved. */
	if ((r_block & ISODEP_BLOCK_NUM_MASK) == t4t_isodep.block_num) {
		/* Toggle block number */
		block_num_toggle();

		isodep_chunk_send();
	} else {
		/* Retransmit last data block */
		if (t4t_isodep.retransmit_cnt > ISODEP_MAX_RETRANSMIT_CNT) {
			/* If to much retransmission, return error. */
			return -NFC_T4T_ISODEP_SEMANTIC_ERROR;
		}

		fdt = t4t_isodep.tag.fwt + T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC;

		if (t4t_isodep_cb->ready_to_send) {
			t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
						     t4t_isodep.tx_data.len, fdt);

			t4t_isodep.retransmit_cnt++;
		}
	}

	return 0;
}

static int isodep_i_frame_handle(const uint8_t *data, size_t len)
{
	size_t index = 0;
	uint8_t i_block;

	__ASSERT_NO_MSG(len >= 1);

	i_block = data[index];
	index++;

	if (i_block & I_BLOCK_DID_BIT) {
		index++;
	}

	if (i_block & I_BLOCK_NAD_BIT) {
		index++;
	}

	if (len < index) {
		LOG_ERR("Received frame is to short.");
		return -EINVAL;
	}

	LOG_DBG("Valid I-Frame received");

	if ((t4t_isodep.rx_data.len + (len - index)) >
	    t4t_isodep.rx_data.buf_size) {
		return -ENOMEM;
	}

	/* Check if current block number is equal to received. */
	if ((i_block & ISODEP_BLOCK_NUM_MASK) == t4t_isodep.block_num) {
		/* Toggle block number */
		block_num_toggle();
	}

	len -= index;

	memcpy(&t4t_isodep.rx_data.data[t4t_isodep.rx_data.len],
	       &data[index], len);
	t4t_isodep.rx_data.len += len;

	if (i_block & I_BLOCK_CHAINING_BIT) {
		LOG_DBG("Chanining bit is set.");

		t4t_isodep.err_status.last_frame = ISODEP_FRAME_I_CHAINING;

		isodep_r_frame_send(true);
	} else {
		atomic_set(&t4t_isodep.state, ISODEP_STATE_SELECTED);

		t4t_isodep.err_status.last_frame = ISODEP_FRAME_I;

		if (t4t_isodep_cb->data_received) {
			t4t_isodep_cb->data_received(t4t_isodep.rx_data.data,
						     t4t_isodep.rx_data.len);
		}
	}

	return 0;
}

static void isodep_error_handle(bool timeout)
{
	int err = 0;
	struct nfc_t4t_err *err_status = &t4t_isodep.err_status;

	err_status->frame_retry_cnt++;

	switch (t4t_isodep.err_status.last_frame) {
	case ISODEP_FRAME_RATS:
		if (err_status->frame_retry_cnt > CONFIG_NFC_T4T_ISODEP_RATS_RETRY) {
			break;
		}

		/* Resend last RATS command. Last command is in tx buffer, so
		 * resend it.
		 */
		if (t4t_isodep_cb->ready_to_send) {
			t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
						     t4t_isodep.tx_data.len,
						     T4T_RATS_FDT);
		}

		return;

	case ISODEP_FRAME_I:
		if (err_status->frame_retry_cnt > CONFIG_NFC_T4T_ISODEP_NAK_RETRY) {
			break;
		}

		/* Send R(NAK) frame. */
		isodep_r_frame_send(false);

		return;

	case ISODEP_FRAME_I_CHAINING:
		if (err_status->frame_retry_cnt > CONFIG_NFC_T4T_ISODEP_ACK_RETRY) {
			break;
		}

		/* Resend last R(ACK) frame. */
		isodep_r_frame_send(true);

		return;

	case ISODEP_FRAME_WTX_RESPONSE:
		if (err_status->frame_retry_cnt > CONFIG_NFC_T4T_ISODEP_WTX_RETRY) {
			break;
		}

		/* Resend last S(WTX) response. Last Response should be in
		 * Tx buffer.
		 */
		if (t4t_isodep_cb->ready_to_send) {
			t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
						     t4t_isodep.tx_data.len,
						     err_status->wtx * t4t_isodep.tag.fwt +
						     T4T_FWT_DELTA + NFCA_T4T_FWT_T_FC);
		}

		return;

	case ISODEP_FRAME_DESELECT:
		if (err_status->frame_retry_cnt > CONFIG_NFC_T4T_ISODEP_DESELECT_RETRY) {
			break;
		}

		/* Resend last S(DESELECT) command. Last command should be in
		 * Tx buffer.
		 */
		if (t4t_isodep_cb->ready_to_send) {
			t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
						     t4t_isodep.tx_data.len,
						     ISODEP_FWT_DEACTIVATION);
		}

		return;

	default:
		break;
	}

	err = timeout ? -NFC_T4T_ISODEP_TIMEOUT_ERROR : -NFC_T4T_ISODEP_TRANSMISSION_ERROR;

	atomic_set(&t4t_isodep.state, ISODEP_STATE_INITIALIZED);
	err_notify(err);
}

static void isodep_delay_handler(struct k_work *work)
{
	isodep_chunk_send();
}

int nfc_t4t_isodep_init(uint8_t *tx_buf, size_t size_tx,
			uint8_t *rx_buf, size_t size_rx,
			const struct nfc_t4t_isodep_cb *cb)
{
	if (!tx_buf || !rx_buf) {
		return -EINVAL;
	}

	if (size_tx < T4T_FSD_MIN) {
		LOG_ERR("Tx buffer is to small. Minimum Tx buffer size %d\n",
			T4T_FSD_MIN);
		return -EINVAL;
	}

	if (size_rx < T4T_FSD_MIN) {
		LOG_ERR("Rx buffer is to small. Minimum Rx buffer size %d\n",
			T4T_FSD_MIN);
		return -EINVAL;
	}

	if (atomic_get(&t4t_isodep.state) != ISODEP_STATE_UNINITIALIZED) {
		LOG_ERR("Already initialized");

		return -EACCES;
	}

	t4t_isodep.tx_data.data = tx_buf;
	t4t_isodep.tx_data.buf_size = size_tx;
	t4t_isodep.rx_data.data = rx_buf;
	t4t_isodep.rx_data.buf_size = size_rx;

	t4t_isodep_cb = cb;

	/* Initialize work queue to handle delay before sending first
	 * I-block after receipt RATS Response.
	 */
	k_work_init_delayable(&isodep_work, isodep_delay_handler);

	atomic_set(&t4t_isodep.state, ISODEP_STATE_INITIALIZED);

	return 0;
}

void nfc_t4t_isodep_on_timeout(void)
{
	isodep_error_handle(true);
}

int nfc_t4t_isodep_rats_send(enum nfc_t4t_isodep_fsd fsd, uint8_t did)
{
	uint8_t param;

	if (atomic_cas(&t4t_isodep.state, ISODEP_STATE_INITIALIZED,
		       ISODEP_STATE_TRANSFER)) {
	} else if (atomic_cas(&t4t_isodep.state, ISODEP_STATE_SELECTED,
			      ISODEP_STATE_TRANSFER)) {
	} else {
		return -EACCES;
	}

	if (did > T4T_DID_MAX) {
		LOG_ERR("Invalid DID value. It should be between 0-14.");

		return -EINVAL;
	}

	if (t4t_isodep.tx_data.buf_size < fsd_value_map[fsd]) {
		LOG_ERR("Invalid FSD value. Increase Tx buffer size or decrease FSD");

		return -ENOMEM;
	}

	/* Set DID field. */
	param = did & T4T_RATS_DID_MASK;

	/* Set FSDI field. */
	param |= (fsd << T4T_RATS_FSDI_OFFSET) & T4T_RATS_FSDI_MASK;

	t4t_isodep.tx_data.data[0] = T4T_RATS_CMD;
	t4t_isodep.tx_data.data[1] = param;

	t4t_isodep.tx_data.len = T4T_RATS_CMD_LEN;

	/* Set reader/writer parameters. */
	t4t_isodep.tag.did = did;
	t4t_isodep.fsd = fsd_value_map[fsd];

	t4t_isodep.ats_expected = true;

	t4t_isodep.err_status.last_frame = ISODEP_FRAME_RATS;

	if (t4t_isodep_cb->ready_to_send) {
		t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
					     t4t_isodep.tx_data.len,
					     T4T_RATS_FDT);
	}

	return 0;
}

int nfc_t4t_isodep_tag_deselect(void)
{
	size_t index = 0;
	uint8_t *tx_data;

	if (!atomic_cas(&t4t_isodep.state,
			ISODEP_STATE_SELECTED,
			ISODEP_STATE_TRANSFER)) {
		return -EACCES;
	}

	tx_data = t4t_isodep.tx_data.data;

	tx_data[index] = ISODEP_S_BLOCK;

	/* Check if DID field should be included. */
	index = did_include(tx_data, index);

	t4t_isodep.tx_data.len = index;
	t4t_isodep.err_status.last_frame = ISODEP_FRAME_DESELECT;

	if (t4t_isodep_cb->ready_to_send) {
		t4t_isodep_cb->ready_to_send(t4t_isodep.tx_data.data,
					     t4t_isodep.tx_data.len,
					     ISODEP_FWT_DEACTIVATION);
	}

	return 0;
}

int nfc_t4t_isodep_data_received(const uint8_t *data, size_t data_len,
				 int err)
{
	uint8_t block_data;

	if (!data) {
		return -EINVAL;
	}

	if (data_len < sizeof(block_data)) {
		LOG_ERR("Invalid data length.");
		return -ENOMEM;
	}

	if (err) {
		isodep_error_handle(false);

		return 0;
	}

	if (t4t_isodep.ats_expected) {
		t4t_isodep.ats_expected = false;

		err = ats_parse(data, data_len);
		if (err) {
			err_notify(err);
		}

		return 0;
	}

	block_data = data[0];

	if (block_data & ISODEP_BLOCK_FIXED_BIT) {
		if (block_data & ISODEP_S_BLOCK_INDICATOR) {
			err =  isodep_s_frame_handle(data, data_len);
		} else if (block_data & ISODEP_R_BLOCK_INDICATOR) {
			err = isodep_r_frame_handle(data, data_len);
		} else {
			/* I-block indicated. */
			err = isodep_i_frame_handle(data, data_len);
		}
	} else {
		err = -NFC_T4T_ISODEP_SEMANTIC_ERROR;
	}

	if (err) {
		err_notify(err);
	}

	return 0;
}

int nfc_t4t_isodep_transmit(const uint8_t *data, size_t data_len)
{
	int64_t spent_time;
	uint32_t delay;

	if (!data) {
		return -EINVAL;
	}

	if (data_len < 1) {
		LOG_ERR("Invalid data length.");
		return -ENOMEM;
	}

	if (!atomic_cas(&t4t_isodep.state, ISODEP_STATE_SELECTED,
			ISODEP_STATE_TRANSFER)) {
		LOG_ERR("Invalid state. NFC Type 4 Tag not selected or transfer ongoing.");
		return -EACCES;
	}

	isodep_transmission_clear();

	t4t_isodep.transmit_data = data;
	t4t_isodep.transmit_len  = data_len;

	if (t4t_isodep.first_transfer) {
		t4t_isodep.first_transfer = false;
		spent_time = k_uptime_delta(&ats_received_time);

		if (spent_time < T4T_FWT_TO_MS(t4t_isodep.tag.fwt)) {
			delay = T4T_FWT_TO_MS(t4t_isodep.tag.fwt) - spent_time;

			LOG_DBG("Wait %d ms before sending first frame after ATS Response",
				delay);

			int ret = k_work_reschedule(&isodep_work, K_MSEC(delay));

			if (ret < 0) {
				return ret;
			}

			__ASSERT_NO_MSG(ret == 1);
			return 0;
		}
	}

	isodep_chunk_send();

	return 0;
}
