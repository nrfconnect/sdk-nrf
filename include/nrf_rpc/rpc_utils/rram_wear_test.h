/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_RRAM_WEAR_TEST_H_
#define NRF_RPC_RRAM_WEAR_TEST_H_

#include <debug/rram_wear_test.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_rram_wear_test nRF RPC flash wear test
 * @{
 */

/** @brief Run write/readback verify on a selected remote flash region.
 *
 * @param index Partition index from the array.
 * @param addr_start Optional start offset for a custom range (used if addr_end > addr_start).
 * @param addr_end Optional end offset (exclusive) for a custom range.
 * @param force Must be true for custom ranges or @c -EPERM is returned.
 *
 * @retval 0 on success.
 * @retval -EPERM if @a region is @c custom and @a force is false.
 * @retval -EINVAL for invalid arguments or an out-of-bounds custom range.
 * @retval -ENOENT if the named partition is not present in the build.
 * @retval -ENODEV if the flash device cannot be resolved for a custom range.
 * @retval Other negative errno from flash operations, or @c -EIO on verify mismatch.
 * @retval -EBADMSG if the RPC response could not be decoded.
 */
int nrf_rpc_rram_wear_test(size_t index, uint64_t addr_start, uint64_t addr_end, bool force);

/** @brief Get the number of partitions from the remote server.
 *
 * @param[out] count Pointer to store the number of partitions.
 *
 * @retval 0 on success.
 * @retval -EBADMSG if the RPC response could not be decoded.
 */
int nrf_rpc_rram_wear_test_get_partition_count(size_t *count);

/** @brief Get partition details from the remote server.
 *
 * @param index Partition index.
 * @param[out] part Pointer to store the partition details. The name string will be allocated
 *                  internally and must be freed by the caller using
 *                  @ref nrf_rpc_rram_wear_test_partition_free.
 *
 * @retval 0 on success.
 * @retval -EBADMSG if the RPC response could not be decoded.
 */
int nrf_rpc_rram_wear_test_get_partition(size_t index, struct rram_wear_test_partition *part);

/** @brief Free memory allocated by @ref nrf_rpc_rram_wear_test_get_partition.
 *
 * @param part Pointer to the partition structure whose memory should be freed.
 */
void nrf_rpc_rram_wear_test_partition_free(struct rram_wear_test_partition *part);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_RRAM_WEAR_TEST_H_ */
