/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <kernel.h>
#include <stdbool.h>
#include <assert.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <string.h>
#include <logging/log.h>
#include <st25r3911b_nfca.h>

#include "st25r3911b_reg.h"
#include "st25r3911b_common.h"
#include "st25r3911b_spi.h"
#include "st25r3911b_interrupt.h"

LOG_MODULE_DECLARE(st25r3911b);

#define NFCA_MIN_LISTEN_FDT 1172

/* According to Digital specification 6.10 the dead rx time should be
 * 1172 - 128 1/fc  and refer to STM25R3911B, MRT timer start after EOF so the
 * 128 1/fc + 20 1/fc have to be also subtracted.
 */
#define NFCA_POLL_FTD_ADJUSMENT 276
#define NFCA_FWT_A_ADJUSMENT 512

/* NFC-A Bit duration time. */
#define NFCA_BD 128

/* Frame Delay Time alignment dapends on Logic State of Last Data Bit.
 * According to NFC Forum Digital 2.0 6.10.1.
 */
#define NFCA_LAST_BIT_ONE_FDT_ADJ 84
#define NFCA_LAST_BIT_ZERO_FDT_ADJ 20

/* NFC-A Guard time in milliseconds, NFC Forum Digital 2.0 6.10.4.1. */
#define NFCA_GT_TIME 5

#define NFCA_IRQ_EVENT_IDX 1
#define NFCA_USER_EVENT_IDX 0

/* NFC-A Tag Sleep command. NFC Forum Digital 2.0 6.8. */
#define NFCA_SLEEP_CMD {0x50, 0x00}

#define NFCA_SEL_CMD_LEN 1
#define NFCA_SEL_PAR_LEN 1
#define NFCA_SDD_REQ_LEN (NFCA_SEL_CMD_LEN + NFCA_SEL_PAR_LEN)
#define NFCA_SDD_DATA_SET(_bytes, _bits) ((u8_t)(((_bytes) << 4) + (_bits)))
#define NFCA_SEL_REQ_BASE 0x91
#define NFCA_SEL_REQ_N_CMD(_lvl) (NFCA_SEL_REQ_BASE + 2 * (_lvl))

#define NFCA_NFCID1_SINGLE_SIZE 4
#define NFCA_NFCID1_DOUBLE_SIZE 7
#define NFCA_NFCID1_TRIPLE_SIZE 10

#define NFCA_T1T_PLATFORM 0x0C
#define NFCA_CASCADE_TAG 0x88

#define NFCA_ANTICOLLISION_FRAME_LEN 5
#define NFCA_BCC_CACL_LEN 4
#define NFCA_BCC_IDX 4
#define NFCA_SEL_RSP_CASCADE_BIT (1 << 2)
#define NFCA_SAK_LEN 1
#define NFCA_SEL_RSP_TAG_PLATFORM_MASK 0x60
#define NFCA_SEL_RSP_TAG_PLATFORM_SHIFT 5

/* NFC-A CRC initial value. NFC Forum Digital 2.0 6.4.1.3. */
#define NFCA_CRC_INITIAL_VALUE 0x6363

#define RXS_TIMEOUT 10

extern const k_tid_t thread;

struct nfc_state {
	atomic_t tag;
	u32_t txrx;
	u32_t anticollision;
};

struct nfca_sel_req {
	u8_t sel_cmd;
	u8_t sel_par;
	u8_t nfcid1[NFCA_NFCID1_SINGLE_SIZE];
	u8_t bcc;
};

struct nfca_collision_rsp {
	u8_t sel_cmd;
	u8_t sel_par;
	u8_t data[NFCA_NFCID1_SINGLE_SIZE + NFCA_SDD_REQ_LEN];
};

struct nfca_sdd_req {
	u8_t sel_cmd;
	u8_t sel_par;
};

struct nfc_transfer {
	const struct st25r3911b_nfca_buf *tx_buf;
	const struct st25r3911b_nfca_buf *rx_buf;
	u32_t fdt;
	size_t written_byte;
	size_t received_byte;
	bool auto_crc;
	int error;
};

struct fifo_water_lvl {
	u8_t tx;
	u8_t rx;
};

struct nfc_fifo {
	u8_t bytes_to_read;
	u8_t incomplete_bits;
	bool parity_miss;
};

struct st25r3911b_nfca {
	struct st25r3911b_nfca_tag_info tag;
	struct nfc_state state;
	struct nfc_transfer transfer;
	struct nfc_fifo fifo;
	struct fifo_water_lvl water_lvl;
	u32_t cmd;
	const struct st25r3911b_nfca_cb *cb;
};

static K_SEM_DEFINE(irq_sem, 0, 1);
static K_SEM_DEFINE(user_sem, 0, 1);

static struct k_poll_event *nfca_events;
static struct k_delayed_work timeout_work;
static struct st25r3911b_nfca nfca;

enum {
	STATE_IDLE,
	STATE_FIELD_ON,
	STATE_FIELD_OFF,
	STATE_TAG_DETECTION,
	STATE_TRANSFER,
	STATE_ANTICOLLISION,
	STATE_SLEEP
};

enum {
	TX_STATE_START,
	TX_STATE_COMPLETE,
	RX_STATE_START,
	RX_STATE_COMPLETE
};

enum {
	ANTICOLLISION_CASCADE_1 = 1,
	ANTICOLLISION_CASCADE_2,
	ANTICOLLISION_CASCADE_3
};

static void clear_local_data(void)
{
	memset(&nfca.fifo, 0, sizeof(nfca.fifo));

	nfca.transfer.written_byte = 0;
	nfca.transfer.received_byte = 0;
}

static void nfca_event_init(struct k_poll_event *events)
{
	k_poll_event_init(&events[NFCA_IRQ_EVENT_IDX],
			  K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY,
			  &irq_sem);

	k_poll_event_init(&events[NFCA_USER_EVENT_IDX],
			  K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY,
			  &user_sem);
}

static void state_set(u32_t state)
{
	atomic_set(&nfca.state.tag, state);
}

static int fifo_incomplete_bit_set(u8_t bits)
{
	return st25r3911b_reg_modify(ST25R3911B_REG_NUM_TX_BYTES_REG2,
				     ST25R3911B_REG_NUM_TX_BYTES_REG2_NBTX,
				     bits & ST25R3911B_REG_NUM_TX_BYTES_REG2_NBTX);
}

static void timeout_process(void)
{
	state_set(STATE_IDLE);

	LOG_DBG("Rx timeout");

	if (nfca.cb->rx_timeout) {
		nfca.cb->rx_timeout(nfca.tag.sleep);
	}
}

static int transmission_prepare(void)
{
	int err;

	/* Clear EMVCo mode */
	err = st25r3911b_reg_modify(ST25R3911B_REG_TIM_CTRl,
				    ST25R3911B_REG_TIM_CTRl_NRT_EMV, 0);
	if (err) {
		return err;
	}

	/* Clear FIFO */
	err = st25r3911b_cmd_execute(ST25R3911B_CMD_CLEAR);
	if (err) {
		return err;
	}

	/* Disable all interrupt */
	err = st25r3911b_irq_disable(ST25R3911B_IRQ_MASK_ALL);
	if (err) {
		return err;
	}

	/* Reset RX Gain */
	return st25r3911b_cmd_execute(ST25R3911B_CMD_RESET_RX_GAIN);
}

static int anticollision_transmit(u8_t *tx_buf, size_t tx_bytes,
				  size_t tx_bits, bool antcl)
{
	int err;
	u8_t cmd;
	u32_t mask_timer;
	u16_t no_rsp_timer;

	if (antcl) {
		/* Set sending anticollision frame */
		err = st25r3911b_reg_modify(ST25R3911B_REG_ISO14443A, 0,
					    ST25R3911B_REG_ISO14443A_ANTCL);
		if (err) {
			return err;
		}

		err = st25r3911b_reg_modify(ST25R3911B_REG_AUXILIARY, 0,
					    ST25R3911B_REG_AUXILIARY_NO_CRC_RX);
		if (err) {
			return err;
		}

		LOG_DBG("Bit oriented anticollision frame will be sent");

	} else {
		/* Unset sending anticollision frame */
		err = st25r3911b_reg_modify(ST25R3911B_REG_ISO14443A,
					    ST25R3911B_REG_ISO14443A_ANTCL, 0);
		if (err) {
			return err;
		}

		err = st25r3911b_reg_modify(ST25R3911B_REG_AUXILIARY,
					    ST25R3911B_REG_AUXILIARY_NO_CRC_RX, 0);
		if (err) {
			return err;
		}
	}

	mask_timer = NFCA_MIN_LISTEN_FDT -
		     (ST25R3911B_FDT_ADJUST + NFCA_POLL_FTD_ADJUSMENT);

	/* Set time when RX is not active after transmission */
	err = st25r3911b_mask_receive_timer_set(mask_timer);
	if (err) {
		return err;
	}

	no_rsp_timer = ST25R3911B_FC_TO_64FC(NFCA_MIN_LISTEN_FDT +
				  ST25R3911B_FDT_ADJUST +
				  NFCA_FWT_A_ADJUSMENT);

	/* Set time before it RX should be detected */
	err = st25r3911b_non_response_timer_set(no_rsp_timer, false, false);
	if (err) {
		return err;
	}

	err = transmission_prepare();
	if (err) {
		return err;
	}

	/* Enable necessary interrupts */
	err = st25r3911b_irq_enable(ST25R3911B_IRQ_MASK_TXE |
				    ST25R3911B_IRQ_MASK_COL |
				    ST25R3911B_IRQ_MASK_RXS |
				    ST25R3911B_IRQ_MASK_RXE |
				    ST25R3911B_IRQ_MASK_NRE);

	if (err) {
		return err;
	}

	err = st25r3911b_fifo_write(tx_buf, tx_bytes);
	if (err) {
		return err;
	}

	/* Set number of complete bytes */
	err = st25r3911b_tx_len_set(tx_bytes);
	if (err) {
		return err;
	}

	/* Set number of incomplete bit in last byte */
	err = fifo_incomplete_bit_set(tx_bits);

	if (antcl) {
		cmd = ST25R3911B_CMD_TX_WITHOUT_CRC;

		LOG_DBG("Transmit collision resolution without CRC");

	} else {
		cmd = ST25R3911B_CMD_TX_WITH_CRC;

		LOG_DBG("Transmit collision resolution cmd with CRC");

	}

	/* Transmit data without CRC */
	return st25r3911b_cmd_execute(cmd);
}

static int transfer_fdt_set(u32_t fdt)
{
	int err;
	u32_t mask_timer;

	mask_timer = NFCA_MIN_LISTEN_FDT -
		     (ST25R3911B_FDT_ADJUST + NFCA_POLL_FTD_ADJUSMENT);

	/* Set mask receive timer, RX is disabled for this time. */
	err = st25r3911b_mask_receive_timer_set(ST25R3911B_FC_TO_64FC(mask_timer));
	if (err) {
		return err;
	}

	fdt += ST25R3911B_FDT_ADJUST + NFCA_FWT_A_ADJUSMENT;

	/* Set No-Response Timer, during this time RX should detect transmission. */
	if (fdt > ST25R3911B_NRT_64FC_MAX) {
		err = st25r3911b_non_response_timer_set(ST25R3911B_FC_TO_4096FC(MIN(fdt, ST25R3911B_NRT_FC_MAX)),
							true, false);
	} else {
		err = st25r3911b_non_response_timer_set(ST25R3911B_FC_TO_64FC(fdt),
							false, false);
	}

	return err;
}

static u8_t bcc_calculate(u8_t *buf, u8_t len)
{
	u8_t bcc = 0;

	__ASSERT_NO_MSG(len == NFCA_BCC_CACL_LEN);

	/* Calculate an exclusive-OR on data buffer.
	 * NFC Forum Digital 2.0 6.7.2
	 */
	for (size_t i = 0; i < len; i++) {
		bcc ^= *(buf + i);
	}

	return bcc;
}

static int read_rx_data(u8_t *data, size_t len)
{
	int err;
	u8_t fifo_status;
	u32_t received;

	/* Check number of bytes in FIFO */
	err = st25r3911b_reg_read(ST25R3911B_REG_FIFO_STATUS_1, &fifo_status);
	if (err) {
		return err;
	}

	nfca.fifo.bytes_to_read = fifo_status;
	received = nfca.transfer.received_byte;

	/* Check number of incomplete bit in FIFO and parity missing */
	err = st25r3911b_reg_read(ST25R3911B_REG_FIFO_STATUS_2, &fifo_status);
	if (err) {
		return err;
	}

	nfca.fifo.incomplete_bits = (fifo_status & ST25R3911B_REG_FIFO_STATUS_2_FIFO_LB_MASK) >>
				     ST25R3911B_REG_FIFO_STATUS_2_FIFO_LB0;
	nfca.fifo.parity_miss    = fifo_status & ST25R3911B_REG_FIFO_STATUS_2_NP_LB;

	/* Check buffer size */
	if (len - received < nfca.fifo.bytes_to_read) {
		return -ENOMEM;
	}

	/* Read FIFO data. */
	return st25r3911b_fifo_read(data + received, nfca.fifo.bytes_to_read);
}

static void tag_platform_detect(u8_t sel_rsp)
{
	u8_t tag = sel_rsp & NFCA_SEL_RSP_TAG_PLATFORM_MASK;

	tag >>= NFCA_SEL_RSP_TAG_PLATFORM_SHIFT;
	nfca.tag.type = tag;
}

static int anticollision_resolution(u32_t cascade_lvl)
{
	struct nfca_sdd_req sdd_req;

	__ASSERT_NO_MSG(cascade_lvl <= ANTICOLLISION_CASCADE_3);

	sdd_req.sel_cmd = NFCA_SEL_REQ_N_CMD(cascade_lvl);
	sdd_req.sel_par = NFCA_SDD_DATA_SET(NFCA_SDD_REQ_LEN, 0);

	/* Set anticollision state */
	nfca.state.anticollision = cascade_lvl;

	LOG_DBG("Collision resolution procedure start, cascade %u",
		cascade_lvl);

	return anticollision_transmit((u8_t *)&sdd_req, sizeof(sdd_req),
				      0, true);
}

static int anticollision_rx(void)
{
	int err;

	u8_t buff[NFCA_ANTICOLLISION_FRAME_LEN] = {0};
	struct nfca_sel_req sel_req;
	u8_t rsp_len;
	u8_t bcc;
	u8_t sel_rsp;
	static u8_t nfcid_idx;

	err = read_rx_data(buff, sizeof(buff));
	if (err) {
		return err;
	}

	rsp_len = nfca.fifo.bytes_to_read;

	if (rsp_len == NFCA_SAK_LEN) {
		sel_rsp = buff[0];

		if (sel_rsp & NFCA_SEL_RSP_CASCADE_BIT) {

			return anticollision_resolution(++nfca.state.anticollision);
		} else {
			nfca.tag.sel_resp = buff[0];
			nfca.tag.nfcid1_len = nfcid_idx;
			nfcid_idx = 0;

			tag_platform_detect(sel_rsp);

			state_set(STATE_IDLE);

			if (nfca.cb->anticollision_completed) {
				nfca.cb->anticollision_completed(&nfca.tag, 0);
			}

			return 0;
		}
	}

	bcc = bcc_calculate(buff, NFCA_BCC_CACL_LEN);
	if (bcc != buff[NFCA_BCC_IDX]) {
		state_set(STATE_IDLE);

		if (nfca.cb->anticollision_completed) {
			nfca.cb->anticollision_completed(&nfca.tag,
							 -ST25R3911B_NFCA_ERR_TRANSMISSION);
		}

		return 0;
	}

	/* Store the NFCID1 */
	if (buff[0] == NFCA_CASCADE_TAG) {
		memcpy(nfca.tag.nfcid1 + nfcid_idx, buff + 1,
		       NFCA_NFCID1_SINGLE_SIZE - 1);
		nfcid_idx += NFCA_NFCID1_SINGLE_SIZE - 1;
	} else {
		memcpy(nfca.tag.nfcid1 + nfcid_idx, buff,
		       NFCA_NFCID1_SINGLE_SIZE);
		nfcid_idx += NFCA_NFCID1_SINGLE_SIZE;
	}

	sel_req.sel_cmd = NFCA_SEL_REQ_N_CMD(nfca.state.anticollision);
	sel_req.sel_par = NFCA_SDD_DATA_SET(7, 0);
	sel_req.bcc = bcc;

	memcpy(sel_req.nfcid1, buff, sizeof(sel_req.nfcid1));

	return anticollision_transmit((u8_t *)&sel_req, sizeof(sel_req),
				      0, false);
}

static void fifo_error_check(void)
{
	if (nfca.transfer.error) {
		return;
	}
	/* Check parity bit missing */
	if (nfca.fifo.parity_miss) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_PARITY;
	}

	/* Check if last byte is complete */
	if (nfca.fifo.incomplete_bits) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_LAST_BYTE_INCOMPLETE;
	}
}


static int on_rx_complete(void)
{
	int err = 0;

	switch (nfca.state.tag) {
	case STATE_ANTICOLLISION:
		err = anticollision_rx();

		break;

	case STATE_TAG_DETECTION:
		err = read_rx_data((u8_t *)&nfca.tag.sens_resp,
				   sizeof(nfca.tag.sens_resp));

		if (nfca.tag.sens_resp.platform_info == NFCA_T1T_PLATFORM) {
			nfca.tag.type = ST25R3911B_NFCA_TAG_TYPE_T1T;
		}

		nfca.tag.sleep = false;

		state_set(STATE_IDLE);

		if (nfca.cb->tag_detected) {
			nfca.cb->tag_detected(&nfca.tag.sens_resp);
		}

		break;

	case STATE_TRANSFER:
		err = read_rx_data(nfca.transfer.rx_buf->data,
				   nfca.transfer.rx_buf->len);

		nfca.transfer.received_byte += nfca.fifo.bytes_to_read;

		fifo_error_check();

		state_set(STATE_IDLE);

		if (nfca.cb->transfer_completed) {
			nfca.cb->transfer_completed(nfca.transfer.rx_buf->data,
						    nfca.transfer.received_byte,
						    nfca.transfer.error);
		}

		break;

	default:
		break;
	}

	return err;
}

static int tx_fifo_water(void)
{
	u8_t *buff;
	u8_t bytes_to_send;

	bytes_to_send = MIN(nfca.transfer.tx_buf->len - nfca.transfer.written_byte,
			    nfca.water_lvl.tx);

	buff = nfca.transfer.tx_buf->data + nfca.transfer.written_byte;

	nfca.transfer.written_byte += bytes_to_send;

	return st25r3911b_fifo_write(buff, bytes_to_send);
}

static int rx_fifo_water(void)
{
	u8_t *buf;
	u32_t bytes_to_receive = nfca.water_lvl.rx;

	buf = nfca.transfer.rx_buf->data + nfca.transfer.received_byte;

	nfca.transfer.received_byte += bytes_to_receive;

	__ASSERT_NO_MSG(nfca.transfer.rx_buf->len > nfca.transfer.received_byte);

	return st25r3911b_fifo_read(buf, bytes_to_receive);
}

static int on_fifo_water_lvl(u32_t state)
{
	int err;

	switch (state) {

	case TX_STATE_START:
		err = tx_fifo_water();
		break;

	case RX_STATE_START:
		err = rx_fifo_water();
		break;

	default:
		err = -EPERM;
		break;
	}

	return err;
}

static int on_collision(void)
{
	int err;
	u8_t col_disp = 0;
	u8_t buf[NFCA_ANTICOLLISION_FRAME_LEN] = {0};
	u8_t bytes;
	u8_t bits;
	struct nfca_collision_rsp coll_resp;

	if (nfca.state.tag != STATE_ANTICOLLISION) {
		return -EFAULT;
	}

	/* Check where collision occurred */
	err = st25r3911b_reg_read(ST25R3911B_REG_COLLISION_DISP, &col_disp);
	if (err) {
		return err;
	}

	err = read_rx_data(buf, sizeof(buf));
	if (err) {
		return err;
	}

	bytes = (col_disp & ST25R3911B_REG_COLLISION_DISP_C_BYTE_MASK) >>
		ST25R3911B_REG_COLLISION_DISP_C_BYTE0;
	bits = (col_disp & ST25R3911B_REG_COLLISION_DISP_C_BIT_MASK) >>
		ST25R3911B_REG_COLLISION_DISP_C_BIT0;


	/* Add one incomplete byte */
	bytes++;
	bits++;

	coll_resp.sel_cmd = NFCA_SEL_REQ_N_CMD(nfca.state.anticollision);
	coll_resp.sel_par = NFCA_SDD_DATA_SET(bytes, bits);

	memcpy(coll_resp.data, buf, bytes);

	/* Send a response to collision */
	return anticollision_transmit((u8_t *)&coll_resp, bytes, bits, true);
}

static void rx_error_check(u32_t irq)
{
	if (nfca.state.txrx != RX_STATE_START) {
		return;
	}

	if (irq & ST25R3911B_IRQ_MASK_ERR1) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_SOFT_FRAMING;
	} else if (irq & ST25R3911B_IRQ_MASK_ERR2) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_HARD_FRAMING;
	} else if (irq & ST25R3911B_IRQ_MASK_PAR) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_PARITY;
	} else if (irq & ST25R3911B_IRQ_MASK_CRC) {
		nfca.transfer.error = -ST25R3911B_NFCA_ERR_CRC;
	} else {
		nfca.transfer.error = 0;
	}
}

static int irq_process(void)
{
	int err = 0;
	u32_t irq;

	irq = st25r3911b_irq_read();

	if (irq & ST25R3911B_IRQ_MASK_TXE) {
		nfca.state.txrx = TX_STATE_COMPLETE;

		if (atomic_get(&nfca.state.tag) == STATE_SLEEP) {
			state_set(STATE_IDLE);

			if (nfca.cb->tag_sleep) {
				nfca.cb->tag_sleep();
			}
		}
	}

	if (irq & ST25R3911B_IRQ_MASK_RXS) {
		nfca.state.txrx = RX_STATE_START;

		/* Workaround. In some cases RXE irq is not triggered, so
		 * after this time transmission finish is checked.
		 */
		k_delayed_work_submit(&timeout_work, RXS_TIMEOUT);
	}

	rx_error_check(irq);

	if (irq & ST25R3911B_IRQ_MASK_RXE) {
		nfca.state.txrx = RX_STATE_COMPLETE;

		k_delayed_work_cancel(&timeout_work);
		err = on_rx_complete();
	}

	if (irq & ST25R3911B_IRQ_MASK_COL) {
		err = on_collision();
	}

	if (irq & ST25R3911B_IRQ_MASK_FWL) {
		err = on_fifo_water_lvl(nfca.state.txrx);
	}

	if (irq & ST25R3911B_IRQ_MASK_NRE) {
		if (nfca.state.txrx != RX_STATE_START) {
			timeout_process();
		}
	}

	return err;
}

static int tag_detect(enum st25r3911b_nfca_detect_cmd cmd)
{
	int err;
	u8_t direct_cmd;
	u32_t mask_timer;
	u32_t no_rsp_timer;

	/* Set sending anticollision frame */
	err = st25r3911b_reg_modify(ST25R3911B_REG_ISO14443A, 0,
				    ST25R3911B_REG_ISO14443A_ANTCL);
	if (err) {
		return err;
	}

	/* Rx data do not contain the CRC. */
	err = st25r3911b_reg_modify(ST25R3911B_REG_AUXILIARY, 0,
				    ST25R3911B_REG_AUXILIARY_NO_CRC_RX);
	if (err) {
		return err;
	}

	mask_timer = NFCA_MIN_LISTEN_FDT -
		     (ST25R3911B_FDT_ADJUST + NFCA_POLL_FTD_ADJUSMENT);

	/* Set time when RX is not active after transmission */
	err = st25r3911b_mask_receive_timer_set(mask_timer);
	if (err) {
		return err;
	}

	no_rsp_timer = NFCA_MIN_LISTEN_FDT + ST25R3911B_FDT_ADJUST + NFCA_FWT_A_ADJUSMENT;

	/* Set time before it RX should be detected */
	err = st25r3911b_non_response_timer_set(ST25R3911B_FC_TO_64FC(no_rsp_timer),
						false, false);
	if (err) {
		return err;
	}

	err = transmission_prepare();
	if (err) {
		return err;
	}

	/* Enable TX and RX interrupts */
	err = st25r3911b_irq_enable(ST25R3911B_IRQ_MASK_RXS |
				    ST25R3911B_IRQ_MASK_RXE |
				    ST25R3911B_IRQ_MASK_TXE |
				    ST25R3911B_IRQ_MASK_COL |
				    ST25R3911B_IRQ_MASK_TIM |
				    ST25R3911B_IRQ_MASK_ERR |
				    ST25R3911B_IRQ_MASK_NRE |
				    ST25R3911B_IRQ_MASK_GPE |
				    ST25R3911B_IRQ_MASK_CRC |
				    ST25R3911B_IRQ_MASK_PAR |
				    ST25R3911B_IRQ_MASK_ERR2 |
				    ST25R3911B_IRQ_MASK_ERR1);
	if (err) {
		return err;
	}

	/* Clear nbtx bits before sending WUPA/REQA - otherwise
	 * ST25R3911 will report parity error
	 */
	err = st25r3911b_reg_write(ST25R3911B_REG_NUM_TX_BYTES_REG2, 0);
	if (err) {
		return err;
	}

	/* Tag detect command choose */
	switch (cmd) {
	case ST25R3911B_NFCA_DETECT_CMD_ALL_REQ:
		direct_cmd = ST25R3911B_CMD_TX_WUPA;

		LOG_DBG("ALL Request send");

		break;

	case ST25R3911B_NFCA_DETECT_CMD_SENS_REQ:
		direct_cmd = ST25R3911B_CMD_TX_REQA;

		LOG_DBG("SENS Request send");

		break;

	default:
		return -EPERM;
	}

	/* Send tag detect cmd */
	err = st25r3911b_cmd_execute(direct_cmd);
	if (err) {
		return err;
	}

	return 0;
}

static int transfer(const struct st25r3911b_nfca_buf *tx,
		    const struct st25r3911b_nfca_buf *rx,
		    u32_t fdt, bool auto_crc)
{
	int err;
	u8_t cmd;
	u32_t irq;

	/* Do not set sending anticollision frame */
	err = st25r3911b_reg_modify(ST25R3911B_REG_ISO14443A,
				    ST25R3911B_REG_ISO14443A_ANTCL, 0);
	if (err) {
		return err;
	}

	err = st25r3911b_reg_modify(ST25R3911B_REG_AUXILIARY,
				    ST25R3911B_REG_AUXILIARY_NO_CRC_RX, 0);
	if (err) {
		return err;
	}

	/* Read FIFO water level */
	err = st25r3911b_fifo_reload_lvl_get(&nfca.water_lvl.tx,
					     &nfca.water_lvl.rx);
	if (err) {
		return err;
	}

	irq = ST25R3911B_IRQ_MASK_RXS | ST25R3911B_IRQ_MASK_RXE |
	      ST25R3911B_IRQ_MASK_TXE | ST25R3911B_IRQ_MASK_COL |
	      ST25R3911B_IRQ_MASK_TIM | ST25R3911B_IRQ_MASK_ERR |
	      ST25R3911B_IRQ_MASK_CRC | ST25R3911B_IRQ_MASK_PAR |
	      ST25R3911B_IRQ_MASK_ERR2 | ST25R3911B_IRQ_MASK_ERR1 |
	      ST25R3911B_IRQ_MASK_FWL;

	if (fdt != 0) {
		irq |= ST25R3911B_IRQ_MASK_NRE | ST25R3911B_IRQ_MASK_GPE;

		err = transfer_fdt_set(fdt);
		if (err) {
			return err;
		}
	}

	err = transmission_prepare();
	if (err) {
		return err;
	}


	/* Enable TX and RX interrupts */
	err = st25r3911b_irq_enable(irq);
	if (err) {
		return err;
	}

	/* Write Tx data to FIFO, if data is longer than FIFO write,
	 * then only write first 96 bytes.
	 */
	err = st25r3911b_fifo_write(tx->data,
				    MIN(tx->len, ST25R3911B_MAX_FIFO_LEN));
	if (err) {
		return err;
	}

	/* Set Rx buffer */
	nfca.transfer.rx_buf = rx;

	/* Set data length to send */
	err = st25r3911b_tx_len_set(tx->len);
	if (err) {
		return err;
	}

	if (auto_crc) {
		cmd = ST25R3911B_CMD_TX_WITH_CRC;

		LOG_DBG("Transmit data with auto generated CRC");

	} else {
		cmd = ST25R3911B_CMD_TX_WITHOUT_CRC;

		LOG_DBG("Transmit data without CRC");
	}

	nfca.transfer.written_byte = MIN(tx->len, ST25R3911B_MAX_FIFO_LEN);

	return st25r3911b_cmd_execute(cmd);
}

static int tag_sleep(void)
{
	int err;
	u8_t tx_buffer[] = NFCA_SLEEP_CMD;
	const struct st25r3911b_nfca_buf tx = {
		.data = tx_buffer,
		.len = sizeof(tx_buffer)
	};

	err = transfer(&tx, NULL, 0, true);
	if (err) {
		return err;
	}

	nfca.tag.sleep = true;

	return 0;
}

static int user_state_process(void)
{
	int err = 0;

	switch (atomic_get(&nfca.state.tag)) {
	case STATE_FIELD_ON:
		err = st25r3911b_field_on(ST25R3911B_NO_THRESHOLD_ANTICOLLISION,
					  ST25R3911B_NO_THRESHOLD_ANTICOLLISION,
					  NFCA_GT_TIME);

		state_set(STATE_IDLE);

		if (!err) {
			if (nfca.cb->field_on) {
				nfca.cb->field_on();
			}
		}

		break;

	case STATE_FIELD_OFF:
		err = st25r3911b_rx_tx_disable();

		state_set(STATE_IDLE);

		if (!err) {
			if (nfca.cb->field_off) {
				nfca.cb->field_off();
			}
		}

		break;

	case STATE_TAG_DETECTION:
		err = tag_detect(nfca.cmd);
		break;

	case STATE_ANTICOLLISION:
		err = anticollision_resolution(ANTICOLLISION_CASCADE_1);
		break;

	case STATE_TRANSFER:
		err = transfer(nfca.transfer.tx_buf,
			       nfca.transfer.rx_buf,
			       nfca.transfer.fdt,
			       nfca.transfer.auto_crc);
		break;

	case STATE_SLEEP:
		err = tag_sleep();
		break;

	default:
		break;
	}

	return err;
}

static void timeout_handler(struct k_work *work)
{
	timeout_process();
}

int st25r3911b_nfca_process(void)
{
	int err = 0;

	if (nfca_events[NFCA_USER_EVENT_IDX].state == K_POLL_STATE_SEM_AVAILABLE) {
		k_sem_take(nfca_events[NFCA_USER_EVENT_IDX].sem, K_NO_WAIT);

		LOG_DBG("User state process");

		err = user_state_process();

		nfca_events[NFCA_USER_EVENT_IDX].state =
					K_POLL_STATE_NOT_READY;

		if (err) {
			return err;
		}
	}

	if (nfca_events[NFCA_IRQ_EVENT_IDX].state == K_POLL_STATE_SEM_AVAILABLE) {
		k_sem_take(nfca_events[NFCA_IRQ_EVENT_IDX].sem, K_NO_WAIT);

		LOG_DBG("Interrupt process");

		err = irq_process();

		nfca_events[NFCA_IRQ_EVENT_IDX].state =
					K_POLL_STATE_NOT_READY;

		if (err) {
			return err;
		}
	}

	return 0;
}

int st25r3911b_nfca_init(struct k_poll_event *events, u8_t cnt,
			 const struct st25r3911b_nfca_cb *cb)
{
	int err;

	if ((!events) || (cnt != ST25R3911B_NFCA_EVENT_CNT)) {
		return -EINVAL;
	}

	/* Initialize interrupts */
	err = st25r3911b_irq_init(&irq_sem);
	if (err) {
		LOG_ERR("Interrupt initialization failed");
		return err;
	}

	/* Initialize ST25R3911B */
	err = st25r3911b_init();
	if (err) {
		LOG_ERR("st25r3911b initialization failed");
		return err;
	}

	/* Set NFC to ISO14443A initiator */
	err = st25r3911b_reg_write(ST25R3911B_REG_MODE_DEF,
				   ST25R3911B_REG_MODE_DEF_OM0);
	if (err) {
		LOG_ERR("Set to ISO14443A failed");
		return err;
	}

	/* Set bit rate to 106 kbits/s */
	err = st25r3911b_reg_write(ST25R3911B_REG_BIT_RATE, 0);
	if (err) {
		LOG_ERR("NFC baudrate set to 106 kbits/s failed");
		return err;
	}

	/* Read FIFO reload level */
	err = st25r3911b_fifo_reload_lvl_get(&nfca.water_lvl.tx,
					     &nfca.water_lvl.rx);
	if (err) {
		LOG_ERR("Fifo water level read failed");
		return err;
	}

	/* Set default state */
	state_set(STATE_IDLE);

	/* Presets RX and TX configuration */
	err = st25r3911b_cmd_execute(ST25R3911B_CMD_ANALOG_PRESET);
	if (err) {
		LOG_ERR("RX and TX analog configuration failed");
		return err;
	}

	/* Set callbacks */
	nfca.cb = cb;

	k_delayed_work_init(&timeout_work, timeout_handler);

	/* Turn on NFC-A led */
	err = st25r3911b_technology_led_set(ST25R3911B_NFCA_LED, true);
	if (err) {
		LOG_ERR("NFCA led enabling failed");
		return err;
	}

	/* NFC-A events initialization */
	nfca_events = events;

	nfca_event_init(nfca_events);

	return 0;
}

int st25r3911b_nfca_field_on(void)
{
	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_FIELD_ON)) {
		return -EPERM;
	}

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_field_off(void)
{
	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_FIELD_OFF)) {
		return -EPERM;
	}

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_tag_detect(enum st25r3911b_nfca_detect_cmd cmd)
{
	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_TAG_DETECTION)) {
		return -EPERM;
	}

	clear_local_data();

	nfca.cmd = cmd;

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_anticollision_start(void)
{
	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_ANTICOLLISION)) {
		return -EPERM;
	}

	clear_local_data();

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_transfer(const struct st25r3911b_nfca_buf *tx,
			     const struct st25r3911b_nfca_buf *rx,
			     u32_t fdt, bool auto_crc)
{
	if ((!tx) || (!rx)) {
		return -EINVAL;
	}

	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_TRANSFER)) {
		return -EPERM;
	}

	clear_local_data();

	nfca.state.txrx = TX_STATE_START;

	nfca.transfer.tx_buf = tx;
	nfca.transfer.rx_buf = rx;
	nfca.transfer.fdt = fdt;
	nfca.transfer.auto_crc = auto_crc;

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_tag_sleep(void)
{
	if (!atomic_cas(&nfca.state.tag, STATE_IDLE, STATE_SLEEP)) {
		return -EPERM;
	}

	k_sem_give(&user_sem);

	return 0;
}

int st25r3911b_nfca_crc_calculate(const u8_t *data, size_t len,
				  struct st25r3911b_nfca_crc *crc_val)
{
	if ((!data) || (!crc_val) || (len <= 0)) {
		return -EINVAL;
	}

	u32_t crc = NFCA_CRC_INITIAL_VALUE;
	u8_t *crc_pos = crc_val->crc;

	do {
		u8_t byte;

		byte = *data++;
		byte = (byte ^ (u8_t)(crc & 0x00FF));
		byte = (byte ^ (byte << 4));
		crc = (crc >> 8) ^ ((u32_t)byte << 8) ^
		      ((u32_t) byte << 3) ^ ((u32_t) byte >> 4);

	} while (--len);

	sys_put_be16(crc, crc_pos);

	return 0;
}
