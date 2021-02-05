/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <nrfx_nfct.h>
#include <nrfx_timer.h>

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#include <secure_services.h>
#endif

#include <logging/log.h>

LOG_MODULE_REGISTER(nfc_platform, CONFIG_NFC_PLATFORM_LOG_LEVEL);

#define NFC_TIMER_IRQn		NRFX_CONCAT_3(TIMER,				  \
					      NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID, \
					      _IRQn)
#define nfc_timer_irq_handler	NRFX_CONCAT_3(nrfx_timer_,			  \
					      NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID, \
					      _irq_handler)

#define DT_DRV_COMPAT nordic_nrf_clock

/* Number of NFC Tag Header registers in the FICR register space. */
#define NFC_TAGHEADER_REGISTERS_NUM	3
/* Default values of NFC Tag Header registers when they are not available
 * in the FICR space.
 */
#define NFC_TAGHEADER_DEFAULT_VAL	{0x5F, 0x00, 0x00}

static struct onoff_manager *hf_mgr;
static struct onoff_client cli;

static void clock_handler(struct onoff_manager *mgr, int res)
{
	/* Activate NFCT only when HFXO is running */
	nrfx_nfct_state_force(NRFX_NFCT_STATE_ACTIVATED);
}

nrfx_err_t nfc_platform_setup(void)
{
	hf_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	__ASSERT_NO_MSG(hf_mgr);

	IRQ_CONNECT(NFCT_IRQn, CONFIG_NFCT_IRQ_PRIORITY,
			   nrfx_nfct_irq_handler, NULL,  0);
	IRQ_CONNECT(NFC_TIMER_IRQn, CONFIG_NFCT_IRQ_PRIORITY,
			   nfc_timer_irq_handler, NULL,  0);

	LOG_DBG("NFC platform initialized");
	return NRFX_SUCCESS;
}


static nrfx_err_t nfc_platform_tagheader_get(uint8_t index,
					     uint32_t *tag_header)
{
	const uint32_t tag_header_addresses[] = {
#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
		(uint32_t) &NRF_FICR_S->NFC.TAGHEADER0,
		(uint32_t) &NRF_FICR_S->NFC.TAGHEADER1,
		(uint32_t) &NRF_FICR_S->NFC.TAGHEADER2
#else
		(uint32_t) &NRF_FICR->NFC.TAGHEADER0,
		(uint32_t) &NRF_FICR->NFC.TAGHEADER1,
		(uint32_t) &NRF_FICR->NFC.TAGHEADER2
#endif
	};

	if (index >= ARRAY_SIZE(tag_header_addresses)) {
		LOG_ERR("Tag Header index out of bounds");
		return NRFX_ERROR_INVALID_ADDR;
	}

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
	int err;

	err = spm_request_read(tag_header, tag_header_addresses[index],
			       sizeof(uint32_t));
	if (err != 0) {
		LOG_ERR("Could not read NFC Tag Header FICR (err: %d)", err);
		return NRFX_ERROR_INVALID_ADDR;
	}
#else
	*tag_header = *((uint32_t *) tag_header_addresses[index]);
#endif

	return NRFX_SUCCESS;
}


nrfx_err_t nfc_platform_nfcid1_default_bytes_get(uint8_t * const buf,
						 uint32_t        buf_len)
{
	if (!buf) {
		return NRFX_ERROR_INVALID_PARAM;
	}

	if ((buf_len != NRFX_NFCT_NFCID1_SINGLE_SIZE) &&
	    (buf_len != NRFX_NFCT_NFCID1_DOUBLE_SIZE) &&
	    (buf_len != NRFX_NFCT_NFCID1_TRIPLE_SIZE)) {
		return NRFX_ERROR_INVALID_LENGTH;
	}

#if defined(FICR_NFC_TAGHEADER0_MFGID_Msk)
	nrfx_err_t err;
	uint32_t nfc_tag_header[NFC_TAGHEADER_REGISTERS_NUM];

	for (int i = 0; i < ARRAY_SIZE(nfc_tag_header); i++) {
		err = nfc_platform_tagheader_get(i, &nfc_tag_header[i]);
		if (err != NRFX_SUCCESS) {
			return err;
		}
	}
#else
	const uint32_t nfc_tag_header[NFC_TAGHEADER_REGISTERS_NUM] =
		NFC_TAGHEADER_DEFAULT_VAL;
#endif

	buf[0] = (uint8_t) (nfc_tag_header[0] >> 0);
	buf[1] = (uint8_t) (nfc_tag_header[0] >> 8);
	buf[2] = (uint8_t) (nfc_tag_header[0] >> 16);
	buf[3] = (uint8_t) (nfc_tag_header[1] >> 0);

	if (buf_len != NRFX_NFCT_NFCID1_SINGLE_SIZE) {
		buf[4] = (uint8_t) (nfc_tag_header[1] >> 8);
		buf[5] = (uint8_t) (nfc_tag_header[1] >> 16);
		buf[6] = (uint8_t) (nfc_tag_header[1] >> 24);

		if (buf_len == NRFX_NFCT_NFCID1_TRIPLE_SIZE) {
			buf[7] = (uint8_t) (nfc_tag_header[2] >> 0);
			buf[8] = (uint8_t) (nfc_tag_header[2] >> 8);
			buf[9] = (uint8_t) (nfc_tag_header[2] >> 16);
		}
		/* Begin: Bugfix for FTPAN-181. */
		/* Workaround for wrong value in NFCID1. Value 0x88 cannot be
		 * used as byte 3 of a double-size NFCID1, according to the
		 * NFC Forum Digital Protocol specification.
		 */
		else if (buf[3] == 0x88) {
			buf[3] |= 0x11;
		}
		/* End: Bugfix for FTPAN-181 */
	}

	return NRFX_SUCCESS;
}


void nfc_platform_event_handler(nrfx_nfct_evt_t const *event)
{
	int err;

	switch (event->evt_id) {
	case NRFX_NFCT_EVT_FIELD_DETECTED:
		LOG_DBG("Field detected");

		sys_notify_init_callback(&cli.notify, clock_handler);
		err = onoff_request(hf_mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);

		break;

	case NRFX_NFCT_EVT_FIELD_LOST:
		LOG_DBG("Field lost");

		err = onoff_cancel_or_release(hf_mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);

		break;

	default:
		/* No implementation required */
		break;
	}
}
