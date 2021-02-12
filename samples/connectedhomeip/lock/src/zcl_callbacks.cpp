/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <support/logging/CHIPLogging.h>

#include "app_task.h"
#include "bolt_lock_manager.h"

#include "af.h"
#include "gen/attribute-id.h"
#include "gen/cluster-id.h"
#include <app/util/af-types.h>

using namespace ::chip;

void emberAfPostAttributeChangeCallback(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId, uint8_t mask,
					uint16_t manufacturerCode, uint8_t type, uint8_t size, uint8_t *value)
{
	if (clusterId != ZCL_ON_OFF_CLUSTER_ID) {
		ChipLogProgress(Zcl, "Unknown cluster ID: %d", clusterId);
		return;
	}

	if (attributeId != ZCL_ON_OFF_ATTRIBUTE_ID) {
		ChipLogProgress(Zcl, "Unknown attribute ID: %d", attributeId);
		return;
	}

	GetAppTask().PostEvent(AppEvent(*value ? AppEvent::Lock : AppEvent::Unlock, true));
}

/** @brief OnOff Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * TODO Issue #3841
 * emberAfOnOffClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * emberAfPluginOnOffClusterServerPostInitCallback.
 *
 */
void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
	GetAppTask().UpdateClusterState();
}
