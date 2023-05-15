/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/onoff.h>

#include <nrf_erratas.h>

#include <nrfx_nfct.h>
#include <nrfx_timer.h>
#include <hal/nrf_ficr.h>

#include <nfc_platform.h>

#include "platform_internal.h"

#if defined(CONFIG_BUILD_WITH_TFM)
#include <tfm_ioctl_api.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_platform, CONFIG_NFC_PLATFORM_LOG_LEVEL);

#if NRF53_ERRATA_70_ENABLE_WORKAROUND || NRF52_ERRATA_190_ENABLE_WORKAROUND || \
								NRF52_ERRATA_79_ENABLE_WORKAROUND
	#define NFC_PLATFORM_USE_TIMER_WORKAROUND 1
#else
	#define NFC_PLATFORM_USE_TIMER_WORKAROUND 0
#endif

#if NFC_PLATFORM_USE_TIMER_WORKAROUND
#define NFC_TIMER_IRQn		NRFX_CONCAT_3(TIMER,				  \
					      NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID, \
					      _IRQn)
#define nfc_timer_irq_handler	NRFX_CONCAT_3(nrfx_timer_,			  \
					      NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID, \
					      _irq_handler)
#endif /* NFC_PLATFORM_USE_TIMER_WORKAROUND */

#define DT_DRV_COMPAT nordic_nrf_clock

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
static struct onoff_manager *hf_mgr;
static struct onoff_client cli;
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */

#define NFCT DT_NODELABEL(nfct)
#define NFCT_HAS_PROPERTY(_property) \
	DT_NODE_HAS_PROP(NFCT, _property)

#define NFCT_MEMORY_SECTION                                         \
	COND_CODE_1(NFCT_HAS_PROPERTY(memory_regions),              \
		(__attribute__((section(LINKER_DT_NODE_REGION_NAME( \
			DT_PHANDLE(NFCT, memory_regions)))))),      \
		())

#define NFC_T2T_BUFFER_SIZE (IS_ENABLED(CONFIG_NFC_T2T_NRFXLIB) ? NFC_PLATFORM_T2T_BUFFER_SIZE : 0U)
#define NFC_T4T_BUFFER_SIZE (IS_ENABLED(CONFIG_NFC_T4T_NRFXLIB) ? NFC_PLATFORM_T4T_BUFFER_SIZE : 0U)

#define NFCT_PLATFORM_BUFFER_SIZE MAX(NFC_T4T_BUFFER_SIZE, NFC_T2T_BUFFER_SIZE)

/* NFC platform buffer. This buffer is used directly by the NFCT peripheral.
 * It might need to be allocated in the specific memory section which can be accessed
 * by EasyDMA.
 */
static uint8_t nfc_platform_buffer[NFCT_PLATFORM_BUFFER_SIZE] NFCT_MEMORY_SECTION;

#if CONFIG_NFC_T2T_NRFXLIB
	BUILD_ASSERT(sizeof(nfc_platform_buffer) >= NFC_T2T_BUFFER_SIZE,
		     "Minimal buffer size for the NFC T2T operations must be at least 16 bytes");
#endif /* CONFIG_NFC_T2T_NRFXLIB */

#if CONFIG_NFC_T4T_NRFXLIB
	BUILD_ASSERT(sizeof(nfc_platform_buffer) >= NFC_T4T_BUFFER_SIZE,
		     "Minimal buffer size for the NFC T4T operations must be at least 256 bytes");
#endif /* CONFIG_NFC_T4T_NRFXLIB */

ISR_DIRECT_DECLARE(nfc_isr_wrapper)
{
	nrfx_nfct_irq_handler();

	ISR_DIRECT_PM();

	/* We may need to reschedule in case an NFC callback
	 * accesses zephyr primitives.
	 */
	return 1;
}

static void clock_handler(struct onoff_manager *mgr, int res)
{
	/* Activate NFCT only when HFXO is running */
	nrfx_nfct_state_force(NRFX_NFCT_STATE_ACTIVATED);
}

nrfx_err_t nfc_platform_setup(nfc_lib_cb_resolve_t nfc_lib_cb_resolve, uint8_t *p_irq_priority)
{
	int err;

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
	hf_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	__ASSERT_NO_MSG(hf_mgr);
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */

	IRQ_DIRECT_CONNECT(DT_IRQN(NFCT), DT_IRQ(NFCT, priority),
			   nfc_isr_wrapper, 0);

#if NFC_PLATFORM_USE_TIMER_WORKAROUND
	IRQ_CONNECT(NFC_TIMER_IRQn, DT_IRQ(NFCT, priority),
		    nfc_timer_irq_handler, NULL,  0);
#endif /* NFC_PLATFORM_USE_TIMER_WORKAROUND */

	*p_irq_priority = DT_IRQ(NFCT, priority);

	err = nfc_platform_internal_init(nfc_lib_cb_resolve);
	if (err) {
		LOG_ERR("NFC platform init fail: callback resolution function pointer is invalid");
		return NRFX_ERROR_NULL;
	}

	LOG_DBG("NFC platform initialized");
	return NRFX_SUCCESS;
}

static nrfx_err_t nfc_platform_tagheaders_get(uint32_t tag_header[3])
{
#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#if defined(CONFIG_BUILD_WITH_TFM)
	uint32_t err = 0;
	enum tfm_platform_err_t plt_err;
	FICR_NFC_Type ficr_nfc_ns;
	__IOM FICR_NFC_Type *ficr_nfc = &NRF_FICR_S->NFC;

	plt_err = tfm_platform_mem_read(&ficr_nfc_ns, (uint32_t)ficr_nfc,
					sizeof(ficr_nfc_ns), &err);
	if (plt_err != TFM_PLATFORM_ERR_SUCCESS || err != 0) {
		LOG_ERR("Could not read FICR NFC Tag Header (plt_err %d, err: %d)",
			plt_err, err);
		return NRFX_ERROR_INTERNAL;
	}

	tag_header[0] = ficr_nfc_ns.TAGHEADER0;
	tag_header[1] = ficr_nfc_ns.TAGHEADER1;
	tag_header[2] = ficr_nfc_ns.TAGHEADER2;

#else
#error "Cannot read FICR NFC Tag Header in current configuration"
#endif
#else

	tag_header[0] = nrf_ficr_nfc_tagheader_get(NRF_FICR, 0);
	tag_header[1] = nrf_ficr_nfc_tagheader_get(NRF_FICR, 1);
	tag_header[2] = nrf_ficr_nfc_tagheader_get(NRF_FICR, 2);

#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

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

	nrfx_err_t err;
	uint32_t nfc_tag_header[3];

	err = nfc_platform_tagheaders_get(nfc_tag_header);
	if (err != NRFX_SUCCESS) {
		return err;
	}

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
		/* Workaround for errata 181 "NFCT: Invalid value in FICR for double-size NFCID1"
		 * found at the Errata document for your device located at
		 * https://infocenter.nordicsemi.com/index.jsp
		 */
		else if (buf[3] == 0x88) {
			buf[3] |= 0x11;
		}
	}

	return NRFX_SUCCESS;
}

uint8_t *nfc_platform_buffer_alloc(size_t size)
{
	if (size > sizeof(nfc_platform_buffer)) {
		__ASSERT_NO_MSG(false);
		return NULL;
	}

	return nfc_platform_buffer;
}

void nfc_platform_buffer_free(uint8_t *p_buffer)
{
	ARG_UNUSED(p_buffer);
}

void nfc_platform_event_handler(nrfx_nfct_evt_t const *event)
{
	int err;

	switch (event->evt_id) {
	case NRFX_NFCT_EVT_FIELD_DETECTED:
		LOG_DBG("Field detected");

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
		/* For now, assume HFXO clock is running all the time on nRF54h20
		 * and currently clock handling is not implemented on this platform.
		 */
		sys_notify_init_callback(&cli.notify, clock_handler);
		err = onoff_request(hf_mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);
#else
		ARG_UNUSED(err);
		/* HFXO is running, switch NFCT to active state immediately. */
		clock_handler(NULL, 0);
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */

		break;

	case NRFX_NFCT_EVT_FIELD_LOST:
		LOG_DBG("Field lost");

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
		err = onoff_cancel_or_release(hf_mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */

		break;

	default:
		/* No implementation required */
		break;
	}
}
