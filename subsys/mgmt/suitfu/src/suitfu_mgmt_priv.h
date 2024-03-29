/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUITFU_MGMT_PRIV_H_
#define SUITFU_MGMT_PRIV_H_

#include <stdint.h>
#include <zcbor_common.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_VER_MAX_STR_LEN 32
/* The value here sets how many "characteristics" that describe image is
 * encoded into a map per each image (like bootable flags, and so on).
 * This value is only used for zcbor to predict map size and map encoding
 * and does not affect memory allocation.
 * In case when more "characteristics" are added to image map then
 * zcbor_map_end_encode may fail it this value does not get updated.
 */
#define MAX_IMG_CHARACTERISTICS	 15
#define IMG_MGMT_HASH_STR	 48
#define IMG_MGMT_HASH_LEN	 64 /* SHA512 */

/*
 * Command IDs for image management group.
 */
#define IMG_MGMT_ID_STATE    0
#define IMG_MGMT_ID_UPLOAD   1
#define IMG_MGMT_ID_FILE     2
#define IMG_MGMT_ID_CORELIST 3
#define IMG_MGMT_ID_CORELOAD 4
#define IMG_MGMT_ID_ERASE    5

/*
 * Command IDs for suit management group.
 */
#define SUIT_MGMT_ID_MANIFESTS_LIST	  0
#define SUIT_MGMT_ID_MANIFEST_STATE	  1
#define SUIT_MGMT_ID_ENVELOPE_UPLOAD	  2
#define SUIT_MGMT_ID_MISSING_IMAGE_STATE  3
#define SUIT_MGMT_ID_MISSING_IMAGE_UPLOAD 4

/**
 * @brief	Verifies if the device associated to DFU partition is ready for use
 *
 * @return MGMT_ERR_EOK on success
 *		MGMT_ERR_EBADSTATE if the device is not ready for use
 *
 */
int suitfu_mgmt_is_dfu_partition_ready(void);

/**
 * @brief	Returns size of DFU partition, in bytes
 *
 */
size_t suitfu_mgmt_get_dfu_partition_size(void);

/**
 * @brief	Erases first num_bytes of DFU partition rounded up to the end of erase
 * block size
 *
 * @return MGMT_ERR_EOK on success
 *		MGMT_ERR_ENOMEM if DFU partition is smaller than num_bytes
 *		MGMT_ERR_EUNKNOWN if erase operation has failed
 */
int suitfu_mgmt_erase_dfu_partition(size_t num_bytes);

/**
 * @brief	Writes image chunk to DFU partition
 *
 * @return MGMT_ERR_EOK on success
 *		MGMT_ERR_EUNKNOWN if write operation has failed
 */
int suitfu_mgmt_write_dfu_image_data(unsigned int req_offset, const void *addr, unsigned int size,
				     bool flush);

/**
 * @brief	Called once entire update candidate is written to DFU partition
 * Implementation triggers further processing of the candidate
 *
 * @return MGMT_ERR_EOK on success
 *		MGMT_ERR_EBUSY on candidate processing error
 */
int suitfu_mgmt_candidate_envelope_stored(size_t image_size);

/**
 * @brief	Process Manifests List Get Request
 *
 */
int suitfu_mgmt_suit_manifests_list(struct smp_streamer *ctx);

/**
 * @brief	Process Manifest State Get Request
 *
 */
int suitfu_mgmt_suit_manifest_state_read(struct smp_streamer *ctx);

/**
 * @brief	Process Candidate Envelope Upload Request
 *
 */
int suitfu_mgmt_suit_envelope_upload(struct smp_streamer *ctx);

/**
 * @brief	Initialization of Image Fetch functionality
 *
 */
void suitfu_mgmt_suit_image_fetch_init(void);

/**
 * @brief	Process Get Missing Image State Request.
 *
 * @note	SMP Client sends that request periodically,
 *		getting requested image identifier (i.e. image name)
 *		as response
 *
 */
int suitfu_mgmt_suit_missing_image_state_read(struct smp_streamer *ctx);

/**
 * @brief	Process Image Upload Request
 *
 * @note	Executed as result of Get Missing Image State Request.
 * It delivers chunks of image requested by the device
 *
 */
int suitfu_mgmt_suit_missing_image_upload(struct smp_streamer *ctx);

/**
 * @brief Returns SUIT bootloader info
 *
 */
int suitfu_mgmt_suit_bootloader_info_read(struct smp_streamer *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __IMG_PRIV_H */
