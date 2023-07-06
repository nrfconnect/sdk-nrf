/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <app/clusters/bindings/BindingManager.h>
#include <app/clusters/bindings/bindings.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace chip::app;

class BindingHandler {
public:
	static BindingHandler &GetInstance()
	{
		static BindingHandler sBindingHandler;
		return sBindingHandler;
	}

	struct BindingData {
		chip::AttributeId attributeId = {};
		chip::EndpointId localEndpointId = 1;
		chip::CommandId commandId = {};
		chip::ClusterId clusterId = {};
		bool isGroup = false;
		bool isReadAttribute = false;
	};

	void Init();
	void PrintBindingTable();
	bool IsGroupBound();

	static void ThermostatWorkerHandler(intptr_t aContext);
	static void OnInvokeCommandFailure(BindingData &aBindingData, CHIP_ERROR aError);

private:
	static chip::app::DataModel::Nullable<int16_t> mMeasuredValue;
	static void TemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
						      OperationalDeviceProxy *deviceProxy, void *context);
	static void TemperatureMeasurementUnicastBindingRead(BindingData *data, const EmberBindingTableEntry &binding,
							     OperationalDeviceProxy *peer_device);
	static void TemperatureMeasurementContextReleaseHandler(void *context);
	static void InitInternal(intptr_t aArg);

	bool mCaseSessionRecovered = false;
};
