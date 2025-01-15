/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_BINARYAPPDATA_H_
#define LWM2M_BINARYAPPDATA_H_

typedef int (*lwm2m_binaryappdata_recv_cb)(uint16_t obj_inst_id, uint16_t res_inst_id,
					   const void *data, uint16_t data_len);

/**
 * @brief Set the callback function to receive binary application data.
 *
 * @param data_cb The callback function to receive the binary application data.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
int lwm2m_binaryappdata_set_recv_cb(lwm2m_binaryappdata_recv_cb recv_cb);

/**
 * @brief Read binary application data from the LWM2M object instance.
 *
 * @param obj_inst_id The object instance ID to write the data to.
 * @param res_inst_id The resource instance ID to write the data to.
 * @param data_ptr Pointer to the binary application data.
 * @param data_len The length of the binary application data in bytes.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
int lwm2m_binaryappdata_read(uint16_t obj_inst_id, uint16_t res_inst_id,
			     void **data_ptr, uint16_t *data_len);

/**
 * @brief Write binary application data to the LWM2M object instance.
 *
 * @param obj_inst_id The object instance ID to write the data to.
 * @param res_inst_id The resource instance ID to write the data to.
 * @param data_ptr Pointer to the binary application data.
 * @param data_len The length of the binary application data in bytes.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
int lwm2m_binaryappdata_write(uint16_t obj_inst_id, uint16_t res_inst_id,
			      const void *data_ptr, uint16_t data_len);

#endif /* LWM2M_BINARYAPPDATA_H_ */
