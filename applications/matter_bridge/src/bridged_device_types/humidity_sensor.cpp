/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "humidity_sensor.h"

namespace
{
DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs);
BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs);
IDENTIFY_CLUSTER_ATTRIBUTES(identifyAttrs);
}; /* namespace */

using namespace ::chip;
using namespace ::chip::app;

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(humiSensorAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, INT16U, 2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id, INT16U, 2,
				  0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id, INT16U, 2,
				  0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::RelativeHumidityMeasurement::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedHumidityClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::RelativeHumidityMeasurement::Id, humiSensorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, sIdentifyIncomingCommands,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedHumidityEndpoint, bridgedHumidityClusters);

static constexpr uint8_t kBridgedHumidityEndpointVersion = 2;

static constexpr EmberAfDeviceType kBridgedHumidityDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::HumiditySensor),
	  kBridgedHumidityEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kHumidityDataVersionSize = ArraySize(bridgedHumidityClusters);

HumiditySensorDevice::HumiditySensorDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
{
	mDataVersionSize = kHumidityDataVersionSize;
	mEp = &bridgedHumidityEndpoint;
	mDeviceTypeList = kBridgedHumidityDeviceTypes;
	mDeviceTypeListSize = ARRAY_SIZE(kBridgedHumidityDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR HumiditySensorDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					    uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::RelativeHumidityMeasurement::Id:
		return HandleReadRelativeHumidityMeasurement(attributeId, buffer, maxReadLength);
	case Clusters::Identify::Id:
		return HandleReadIdentify(attributeId, buffer, maxReadLength);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR HumiditySensorDevice::HandleReadRelativeHumidityMeasurement(AttributeId attributeId, uint8_t *buffer,
								       uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id: {
		uint16_t value = GetMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id: {
		uint16_t value = GetMinMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id: {
		uint16_t value = GetMaxMeasuredValue();
		return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetRelativeHumidityMeasurementClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetRelativeHumidityMeasurementFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR HumiditySensorDevice::HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId,
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
	case Clusters::RelativeHumidityMeasurement::Id: {
		switch (attributeId) {
		case Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id: {
			int16_t value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}

			SetMeasuredValue(value);

			break;
		}
		case Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id: {
			int16_t value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}

			SetMinMeasuredValue(value);

			break;
		}
		case Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id: {
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
