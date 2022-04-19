/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZB_DIMMER_SWITCH_H
#define ZB_DIMMER_SWITCH_H 1

/**
 *  @defgroup ZB_DEFINE_DEVICE_DIMMER_SWITCH Dimmer Switch
 *  @{
 *  @details
 *      - @ref ZB_ZCL_BASIC \n
 *      - @ref ZB_ZCL_IDENTIFY \n
 *      - @ref ZB_ZCL_SCENES \n
 *      - @ref ZB_ZCL_GROUPS \n
 *      - @ref ZB_ZCL_ON_OFF \n
 *      - @ref ZB_ZCL_LEVEL_CONTROL
 */

/** Dimmer Switch Device ID */
#define ZB_DIMMER_SWITCH_DEVICE_ID 0x0104

/** Dimmer Switch device version */
#define ZB_DEVICE_VER_DIMMER_SWITCH 0

/** @cond internals_doc */

/** Dimmer Switch IN (server) clusters number */
#define ZB_DIMMER_SWITCH_IN_CLUSTER_NUM 2

/** Dimmer Switch OUT (client) clusters number */
#define ZB_DIMMER_SWITCH_OUT_CLUSTER_NUM 5

/** Dimmer switch total (IN+OUT) cluster number */
#define ZB_DIMMER_SWITCH_CLUSTER_NUM \
	(ZB_DIMMER_SWITCH_IN_CLUSTER_NUM + ZB_DIMMER_SWITCH_OUT_CLUSTER_NUM)

/** Number of attribute for reporting on Dimmer Switch device */
#define ZB_DIMMER_SWITCH_REPORT_ATTR_COUNT 0

/** @endcond */ /* internals_doc */

/**
 * @brief Declare cluster list for Dimmer Switch device
 * @param cluster_list_name - cluster list variable name
 * @param basic_server_attr_list - attribute list for Basic cluster (server role)
 * @param identify_client_attr_list - attribute list for Identify cluster (client role)
 * @param identify_server_attr_list - attribute list for Identify cluster (server role)
 * @param scenes_client_attr_list - attribute list for Scenes cluster (client role)
 * @param groups_client_attr_list - attribute list for Groups cluster (client role)
 * @param on_off_client_attr_list - attribute list for On/Off cluster (client role)
 * @param level_control_client_attr_list - attribute list for Level Control cluster (client role)
 */
#define ZB_DECLARE_DIMMER_SWITCH_CLUSTER_LIST(					  \
		cluster_list_name,						  \
		basic_server_attr_list,						  \
		identify_client_attr_list,					  \
		identify_server_attr_list,					  \
		scenes_client_attr_list,					  \
		groups_client_attr_list,					  \
		on_off_client_attr_list,					  \
		level_control_client_attr_list)					  \
zb_zcl_cluster_desc_t cluster_list_name[] =					  \
{										  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_BASIC,					  \
		ZB_ZCL_ARRAY_SIZE(basic_server_attr_list, zb_zcl_attr_t),	  \
		(basic_server_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,					  \
		ZB_ZCL_ARRAY_SIZE(identify_server_attr_list, zb_zcl_attr_t),	  \
		(identify_server_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,					  \
		ZB_ZCL_ARRAY_SIZE(identify_client_attr_list, zb_zcl_attr_t),	  \
		(identify_client_attr_list),					  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_SCENES,					  \
		ZB_ZCL_ARRAY_SIZE(scenes_client_attr_list, zb_zcl_attr_t),	  \
		(scenes_client_attr_list),					  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_GROUPS,					  \
		ZB_ZCL_ARRAY_SIZE(groups_client_attr_list, zb_zcl_attr_t),	  \
		(groups_client_attr_list),					  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_ON_OFF,					  \
		ZB_ZCL_ARRAY_SIZE(on_off_client_attr_list, zb_zcl_attr_t),	  \
		(on_off_client_attr_list),					  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,				  \
		ZB_ZCL_ARRAY_SIZE(level_control_client_attr_list, zb_zcl_attr_t), \
		(level_control_client_attr_list),				  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	)									  \
}


/** @cond internals_doc */
/**
 * @brief Declare simple descriptor for Dimmer switch device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param in_clust_num - number of supported input clusters
 * @param out_clust_num - number of supported output clusters
 */
#define ZB_ZCL_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(				    \
	ep_name, ep_id, in_clust_num, out_clust_num)				    \
	ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);			    \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name = \
	{									    \
		ep_id,								    \
		ZB_AF_HA_PROFILE_ID,						    \
		ZB_DIMMER_SWITCH_DEVICE_ID,					    \
		ZB_DEVICE_VER_DIMMER_SWITCH,					    \
		0,								    \
		in_clust_num,							    \
		out_clust_num,							    \
		{								    \
			ZB_ZCL_CLUSTER_ID_BASIC,				    \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,				    \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,				    \
			ZB_ZCL_CLUSTER_ID_SCENES,				    \
			ZB_ZCL_CLUSTER_ID_GROUPS,				    \
			ZB_ZCL_CLUSTER_ID_ON_OFF,				    \
			ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,			    \
		}								    \
	}

/** @endcond */ /* internals_doc */

/**
 * @brief Declare endpoint for Dimmer Switch device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param cluster_list - endpoint cluster list
 */
#define ZB_DECLARE_DIMMER_SWITCH_EP(ep_name, ep_id, cluster_list)		      \
	ZB_ZCL_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(ep_name, ep_id,		      \
		  ZB_DIMMER_SWITCH_IN_CLUSTER_NUM, ZB_DIMMER_SWITCH_OUT_CLUSTER_NUM); \
										      \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,     \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,		      \
		0, NULL, /* No reporting ctx */					      \
		0, NULL) /* No CVC ctx */

/** @} */

#endif /* ZB_DIMMER_SWITCH_H */
