/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef OTA_BUNDLE_DECODE_TYPES_H__
#define OTA_BUNDLE_DECODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct ota_element {
	enum {
		ota_element_target_nrf53_app_m_c = 0,
		ota_element_target_nrf53_net_m_c = 1,
		ota_element_target_nrf53_bl_m_c = 2,
		ota_element_target_nrf91_app_m_c = 3,
		ota_element_target_nrf91_bl_m_c = 4,
		ota_element_target_nrf91_mdm_delta_m_c = 5,
		ota_element_target_nrf91_mdm_full_m_c = 6,
	} target_choice;
	uint32_t size;
};

struct ota_bundle {
	uint32_t bundle_size;
	struct zcbor_string bundle_sha256;
	struct ota_element ota_element_m[7];
	size_t ota_element_m_count;
};

#ifdef __cplusplus
}
#endif

#endif /* OTA_BUNDLE_DECODE_TYPES_H__ */
