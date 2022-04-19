/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZB_DIMMABLE_LIGHT_H
#define ZB_DIMMABLE_LIGHT_H 1

/**
 *  @defgroup ZB_DEFINE_DEVICE_DIMMABLE_LIGHT Dimmable Light
 *  @{
 *  @details
 *      - @ref ZB_ZCL_BASIC \n
 *      - @ref ZB_ZCL_IDENTIFY \n
 *      - @ref ZB_ZCL_SCENES \n
 *      - @ref ZB_ZCL_GROUPS \n
 *      - @ref ZB_ZCL_ON_OFF \n
 *      - @ref ZB_ZCL_LEVEL_CONTROL
 */

/** Dimmable Light Device ID */
#define ZB_DIMMABLE_LIGHT_DEVICE_ID 0x0101

/** Dimmable light device version */
#define ZB_DEVICE_VER_DIMMABLE_LIGHT 1

/** @cond internals_doc */

/** Dimmable Light IN (server) clusters number */
#define ZB_DIMMABLE_LIGHT_IN_CLUSTER_NUM 6

/** Dimmable Light OUT (client) clusters number */
#define ZB_DIMMABLE_LIGHT_OUT_CLUSTER_NUM 0

/** Dimmable light total (IN+OUT) cluster number */
#define ZB_DIMMABLE_LIGHT_CLUSTER_NUM \
	(ZB_DIMMABLE_LIGHT_IN_CLUSTER_NUM + ZB_DIMMABLE_LIGHT_OUT_CLUSTER_NUM)

/** Number of attribute for reporting on Dimmable Light device */
#define ZB_DIMMABLE_LIGHT_REPORT_ATTR_COUNT \
	(ZB_ZCL_ON_OFF_REPORT_ATTR_COUNT + ZB_ZCL_LEVEL_CONTROL_REPORT_ATTR_COUNT)

/** Continuous value change attribute count */
#define ZB_DIMMABLE_LIGHT_CVC_ATTR_COUNT 1

/** @endcond */ /* internals_doc */

/**
 * @brief Declare cluster list for Dimmable Light device
 * @param cluster_list_name - cluster list variable name
 * @param basic_attr_list - attribute list for Basic cluster
 * @param identify_attr_list - attribute list for Identify cluster
 * @param groups_attr_list - attribute list for Groups cluster
 * @param scenes_attr_list - attribute list for Scenes cluster
 * @param on_off_attr_list - attribute list for On/Off cluster
 * @param level_control_attr_list - attribute list for Level Control cluster
 */
#define ZB_DECLARE_DIMMABLE_LIGHT_CLUSTER_LIST(					   \
	cluster_list_name,							   \
	basic_attr_list,							   \
	identify_attr_list,							   \
	groups_attr_list,							   \
	scenes_attr_list,							   \
	on_off_attr_list,							   \
	level_control_attr_list)						   \
	zb_zcl_cluster_desc_t cluster_list_name[] =				   \
	{									   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,				   \
			ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),	   \
			(identify_attr_list),					   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		),								   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_BASIC,				   \
			ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),	   \
			(basic_attr_list),					   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		),								   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_SCENES,				   \
			ZB_ZCL_ARRAY_SIZE(scenes_attr_list, zb_zcl_attr_t),	   \
			(scenes_attr_list),					   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		),								   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_GROUPS,				   \
			ZB_ZCL_ARRAY_SIZE(groups_attr_list, zb_zcl_attr_t),	   \
			(groups_attr_list),					   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		),								   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_ON_OFF,				   \
			ZB_ZCL_ARRAY_SIZE(on_off_attr_list, zb_zcl_attr_t),	   \
			(on_off_attr_list),					   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		),								   \
		ZB_ZCL_CLUSTER_DESC(						   \
			ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,			   \
			ZB_ZCL_ARRAY_SIZE(level_control_attr_list, zb_zcl_attr_t), \
			(level_control_attr_list),				   \
			ZB_ZCL_CLUSTER_SERVER_ROLE,				   \
			ZB_ZCL_MANUF_CODE_INVALID				   \
		)								   \
	}


/** @cond internals_doc */
/**
 * @brief Declare simple descriptor for Dimmable Light device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param in_clust_num - number of supported input clusters
 * @param out_clust_num - number of supported output clusters
 */
#define ZB_ZCL_DECLARE_HA_DIMMABLE_LIGHT_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
	ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);					  \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =		  \
	{											  \
		ep_id,										  \
		ZB_AF_HA_PROFILE_ID,								  \
		ZB_DIMMABLE_LIGHT_DEVICE_ID,							  \
		ZB_DEVICE_VER_DIMMABLE_LIGHT,							  \
		0,										  \
		in_clust_num,									  \
		out_clust_num,									  \
		{										  \
			ZB_ZCL_CLUSTER_ID_BASIC,						  \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,						  \
			ZB_ZCL_CLUSTER_ID_SCENES,						  \
			ZB_ZCL_CLUSTER_ID_GROUPS,						  \
			ZB_ZCL_CLUSTER_ID_ON_OFF,						  \
			ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,					  \
		}										  \
	}

/** @endcond */ /* internals_doc */

/**
 * @brief Declare endpoint for Dimmable Light device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param cluster_list - endpoint cluster list
 */
#define ZB_DECLARE_DIMMABLE_LIGHT_EP(ep_name, ep_id, cluster_list)		      \
	ZB_ZCL_DECLARE_HA_DIMMABLE_LIGHT_SIMPLE_DESC(ep_name, ep_id,		      \
		ZB_DIMMABLE_LIGHT_IN_CLUSTER_NUM, ZB_DIMMABLE_LIGHT_OUT_CLUSTER_NUM); \
	ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info## ep_name,		      \
		ZB_DIMMABLE_LIGHT_REPORT_ATTR_COUNT);				      \
	ZBOSS_DEVICE_DECLARE_LEVEL_CONTROL_CTX(cvc_alarm_info## ep_name,	      \
		ZB_DIMMABLE_LIGHT_CVC_ATTR_COUNT);				      \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID,	      \
		0,								      \
		NULL,								      \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
			(zb_af_simple_desc_1_1_t *)&simple_desc_## ep_name,	      \
			ZB_DIMMABLE_LIGHT_REPORT_ATTR_COUNT,			      \
			reporting_info## ep_name,				      \
			ZB_DIMMABLE_LIGHT_CVC_ATTR_COUNT,			      \
			cvc_alarm_info## ep_name)

/** @} */

#endif /* ZB_DIMMABLE_LIGHT_H */
