/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "binding/binding_handler.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/util/attribute-storage.h>

namespace Nrf {

class BridgedDeviceDataProvider {
public:
	using UpdateAttributeCallback = void (*)(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
						 chip::AttributeId attributeId, void *data, size_t dataSize);

	using InvokeCommandCallback = void (*)(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
					       chip::CommandId commandId,
					       Nrf::Matter::BindingHandler::InvokeCommand invokeCommand);

	explicit BridgedDeviceDataProvider(UpdateAttributeCallback updateCallback,
					   InvokeCommandCallback commandCallback = nullptr)
	{
		mUpdateAttributeCallback = updateCallback;
		mInvokeCommandCallback = commandCallback;
	}
	virtual ~BridgedDeviceDataProvider() = default;

	virtual void Init() = 0;
	virtual void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
				       size_t dataSize) = 0;
	virtual CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) = 0;

	CHIP_ERROR NotifyReachableStatusChange(bool isReachable);

protected:
	UpdateAttributeCallback mUpdateAttributeCallback;
	InvokeCommandCallback mInvokeCommandCallback;

private:
	struct ReachableContext {
		bool mIsReachable;
		BridgedDeviceDataProvider *mProvider;
	};
};

} /* namespace Nrf */
