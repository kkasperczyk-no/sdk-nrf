/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "onoff_light.h"

namespace
{
DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs);
BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs);
}; // namespace

using namespace ::chip;
using namespace ::chip::app;

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::OnOff::Id, BOOLEAN, 1, 0), DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

constexpr CommandId onOffIncomingCommands[] = {
	app::Clusters::OnOff::Commands::Off::Id,
	app::Clusters::OnOff::Commands::On::Id,
	app::Clusters::OnOff::Commands::Toggle::Id,
	kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::OnOff::Id, onOffAttrs, onOffIncomingCommands, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, nullptr,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, bridgedLightClusters);

static constexpr EmberAfDeviceType kBridgedOnOffDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(BridgedDevice::DeviceType::OnOffLight),
	  BridgedDevice::kDefaultDynamicEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(BridgedDevice::DeviceType::BridgedNode),
	  BridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kLightDataVersionSize = ArraySize(bridgedLightClusters);

OnOffLightDevice::OnOffLightDevice(const char *nodeLabel) : BridgedDevice(nodeLabel)
{
	mDataVersionSize = kLightDataVersionSize;
	mEp = &bridgedLightEndpoint;
	mDeviceTypeList = kBridgedOnOffDeviceTypes;
	mDeviceTypeListSize = sizeof(kBridgedOnOffDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR OnOffLightDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return HandleReadBridgedDeviceBasicInformation(attributeId, buffer, maxReadLength);
		break;
	case Clusters::Descriptor::Id:
		return HandleReadDescriptor(attributeId, buffer, maxReadLength);
		break;
	case Clusters::OnOff::Id:
		return HandleReadOnOff(attributeId, buffer, maxReadLength);
		break;
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightDevice::HandleReadOnOff(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		bool onOff = GetOnOff();
		return HandleReadAttribute(&onOff, sizeof(onOff), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetOnOffClusterRevision();
		return HandleReadAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetOnOffFeatureMap();
		return HandleReadAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightDevice::HandleWrite(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		Set(*buffer);
		return CHIP_NO_ERROR;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}
