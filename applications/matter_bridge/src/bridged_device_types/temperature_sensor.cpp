/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "temperature_sensor.h"

namespace
{
DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs);
BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs);
IDENTIFY_CLUSTER_ATTRIBUTES(identifyAttrs);
}; /* namespace */

using namespace ::chip;
using namespace ::chip::app;

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(tempSensorAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id, INT16S, 2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Id, INT16S, 2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Id, INT16S, 2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::TemperatureMeasurement::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedTemperatureClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::TemperatureMeasurement::Id, tempSensorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, sIdentifyIncomingCommands,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedTemperatureEndpoint, bridgedTemperatureClusters);

static constexpr uint8_t kBridgedTemperatureEndpointVersion = 2;

static constexpr EmberAfDeviceType kBridgedTemperatureDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::TemperatureSensor),
	  kBridgedTemperatureEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kTemperatureDataVersionSize = ArraySize(bridgedTemperatureClusters);

TemperatureSensorDevice::TemperatureSensorDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
{
	mDataVersionSize = kTemperatureDataVersionSize;
	mEp = &bridgedTemperatureEndpoint;
	mDeviceTypeList = kBridgedTemperatureDeviceTypes;
	mDeviceTypeListSize = ARRAY_SIZE(kBridgedTemperatureDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR TemperatureSensorDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					       uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::TemperatureMeasurement::Id:
		return HandleReadTemperatureMeasurement(attributeId, buffer, maxReadLength);
	case Clusters::Identify::Id:
		return HandleReadIdentify(attributeId, buffer, maxReadLength);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR TemperatureSensorDevice::HandleReadTemperatureMeasurement(AttributeId attributeId, uint8_t *buffer,
								     uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id: {
		int16_t value = GetMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Id: {
		int16_t value = GetMinMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Id: {
		int16_t value = GetMaxMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::TemperatureMeasurement::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetTemperatureMeasurementClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::TemperatureMeasurement::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetTemperatureMeasurementFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR TemperatureSensorDevice::HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId,
							  void *data, size_t dataSize)
{
	CHIP_ERROR err = CHIP_NO_ERROR;
	if (!data) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return HandleWriteDeviceBasicInformation(clusterId, attributeId, data, dataSize);
	case Clusters::Identify::Id:
		return HandleWriteIdentify(attributeId, data, dataSize);
	case Clusters::TemperatureMeasurement::Id: {
		switch (attributeId) {
		case Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id: {
			int16_t value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}

			SetMeasuredValue(value);

			break;
		}
		case Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Id: {
			int16_t value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}

			SetMinMeasuredValue(value);

			break;
		}
		case Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Id: {
			int16_t value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}

			SetMaxMeasuredValue(value);

			break;
		}
		default:
			return CHIP_ERROR_INVALID_ARGUMENT;
		}
		break;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return err;
}
