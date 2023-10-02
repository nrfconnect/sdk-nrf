/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 * @brief LwM2M codec helpers.
 */

#ifndef LWM2M_CODEC_HELPERS__
#define LWM2M_CODEC_HELPERS__

#include <zephyr/net/lwm2m.h>

#include "cloud_codec.h"

/**
 * @defgroup lwm2m_codec_helpers LwM2M codec helpers library
 * @{
 * @brief Library that contains common APIs for handling objects and resources in
 *	  Asset Tracker v2 LwM2M backend.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Create object and resource instances for objects that do not have this setup already
 *	   through default objects in the LwM2M engine.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 */
int lwm2m_codec_helpers_create_objects_and_resources(void);

/** @brief Set up dedicated buffers for resources that do not have storage set in the respective
 *	   LwM2M object source files.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 */
int lwm2m_codec_helpers_setup_resources(void);

/** @brief Set Assisted GNSS (A-GNSS) request data.
 *	   This function will populate the GNSS assistance object (object ID 33625)
 *	   with the A-GNSS request.
 *
 *  @param[in] agnss_request Pointer to buffer that contains A-GNSS data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_agnss_data(struct cloud_data_agnss_request *agnss_request);

/** @brief Set Predicted GPS (P-GPS) request data.
 *	   This function will populate the GNSS assistance object (object ID 33625)
 *	   with the P-GPS request.
 *
 *  @param[in] pgps_request Pointer to buffer that contains P-GPS data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_pgps_data(struct cloud_data_pgps_request *pgps_request);

/** @brief Set the initial values for the application's configuration object and register
 *	   callbacks for configuration changes.
 *
 *  @param[in] cfg Pointer to structure that contains the default configuration values for the
 *		   application.
 *  @param[in] callback Event handler to receive configuration updates.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 */
int lwm2m_codec_helpers_setup_configuration_object(struct cloud_data_cfg *cfg,
						   lwm2m_engine_set_data_cb_t callback);

/** @brief Get the current values of the application's configuration object.
 *
 *  @param[out] cfg Pointer to buffer that will be populated with the current values of the
 *		    configuration object.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 */
int lwm2m_codec_helpers_get_configuration_object(struct cloud_data_cfg *cfg);

/** @brief Set GNSS data.
 *
 *  @param[in] gnss Pointer to structure that contains GNSS data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_gnss_data(struct cloud_data_gnss *gnss);

/** @brief Set environmental sensor data.
 *
 *  @param[in] sensor Pointer to structure that contains environmental sensor data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_sensor_data(struct cloud_data_sensors *sensor);

/** @brief Set modem dynamic data.
 *
 *  @param[in] modem_dynamic Pointer to structure that contains dynamic modem data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_modem_dynamic_data(struct cloud_data_modem_dynamic *modem_dynamic);

/** @brief Set modem static data.
 *
 *  @param[in] modem_static Pointer to structure that contains static modem data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_modem_static_data(struct cloud_data_modem_static *modem_static);

/** @brief Set battery data.
 *
 *  @param[in] battery Pointer to structure that contains battery data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_battery_data(struct cloud_data_battery *battery);

/** @brief Set user interface (UI) data.
 *
 *  @param[in] user_interface Pointer to structure that contains user interface data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_user_interface_data(struct cloud_data_ui *user_interface);

/** @brief Set neighbor cell data.
 *
 *  @param[in] neighbor_cells Pointer to structure that contains neighbor cell data.
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_set_neighbor_cell_data(struct cloud_data_neighbor_cells *neighbor_cells);

/** @brief Generate path lists with reference to objects.
 *	   This function outputs a list of paths that can be used to reference objects that should
 *	   be updated (sent to server) when calling the lwm2m_send_cb() function.
 *
 *  @param[out] output Pointer to structure into which the input path list will be added.
 *  @param[in]  path List that contains LwM2M paths that will be
 *		     added to the output variable.
 *  @param[in]  path_size Size of the path list variable (path).
 *
 *  @retval 0 If successful, otherwise a negative value indicating the reason of failure.
 *  @return -ENODATA if the queued flag present in the input structure is false.
 */
int lwm2m_codec_helpers_object_path_list_add(struct cloud_codec_data *output,
					     const struct lwm2m_obj_path path[],
					     size_t path_size);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* LWM2M_CODEC_HELPERS__ */
