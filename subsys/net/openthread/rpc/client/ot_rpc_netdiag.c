/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_client_common.h>
#include <ot_rpc_ids.h>

#include <openthread/netdiag.h>

static char vendor_name[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH + 1];
static char vendor_model[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH + 1];
static char vendor_sw_version[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1];

otError otThreadSetVendorName(otInstance *aInstance, const char *aVendorName)
{
	ARG_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_NAME, aVendorName);
}

otError otThreadSetVendorModel(otInstance *aInstance, const char *aVendorModel)
{
	ARG_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_MODEL, aVendorModel);
}

otError otThreadSetVendorSwVersion(otInstance *aInstance, const char *aVendorSwVersion)
{
	ARG_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION, aVendorSwVersion);
}

const char *otThreadGetVendorName(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_NAME, vendor_name, sizeof(vendor_name));
	return vendor_name;
}

const char *otThreadGetVendorModel(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_MODEL, vendor_model, sizeof(vendor_model));
	return vendor_model;
}

const char *otThreadGetVendorSwVersion(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION, vendor_sw_version,
			  sizeof(vendor_sw_version));
	return vendor_sw_version;
}
