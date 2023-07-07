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
}; // namespace

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
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, nullptr,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedHumidityEndpoint, bridgedHumidityClusters);

static constexpr EmberAfDeviceType kBridgedHumidityDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(BridgedDevice::DeviceType::HumiditySensor),
	  BridgedDevice::kDefaultDynamicEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(BridgedDevice::DeviceType::BridgedNode),
	  BridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kHumidityDataVersionSize = ArraySize(bridgedHumidityClusters);

HumiditySensorDevice::HumiditySensorDevice(const char *nodeLabel) : BridgedDevice(nodeLabel)
{
	mDataVersionSize = kHumidityDataVersionSize;
	mEp = &bridgedHumidityEndpoint;
	mDeviceTypeList = kBridgedHumidityDeviceTypes;
	mDeviceTypeListSize = sizeof(kBridgedHumidityDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR HumiditySensorDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					    uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return HandleReadBridgedDeviceBasicInformation(attributeId, buffer, maxReadLength);
		break;
	case Clusters::Descriptor::Id:
		return HandleReadDescriptor(attributeId, buffer, maxReadLength);
		break;
	case Clusters::RelativeHumidityMeasurement::Id:
		return HandleReadRelativeHumidityMeasurement(attributeId, buffer, maxReadLength);
		break;
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
		return HandleReadAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id: {
		uint16_t value = GetMinMeasuredValue();
		return HandleReadAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id: {
		uint16_t value = GetMaxMeasuredValue();
		return HandleReadAttribute(&value, sizeof(value), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetRelativeHumidityMeasurementClusterRevision();
		return HandleReadAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::RelativeHumidityMeasurement::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetRelativeHumidityMeasurementFeatureMap();
		return HandleReadAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}
