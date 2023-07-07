/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"

class HumiditySensorDevice : public BridgedDevice {
public:
	HumiditySensorDevice(const char *nodeLabel);

	uint16_t GetMeasuredValue() { return mMeasuredValue; }
	uint16_t GetMinMeasuredValue() { return mMinMeasuredValue; }
	uint16_t GetMaxMeasuredValue() { return mMaxMeasuredValue; }
	uint16_t GetRelativeHumidityMeasurementClusterRevision() { return kRelativeHumidityMeasurementClusterRevision; }
	uint32_t GetRelativeHumidityMeasurementFeatureMap() { return kRelativeHumidityMeasurementFeatureMap; }

	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadRelativeHumidityMeasurement(chip::AttributeId attributeId, uint8_t *buffer,
							 uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

private:
	static constexpr uint16_t kRelativeHumidityMeasurementClusterRevision = 1;
	static constexpr uint32_t kRelativeHumidityMeasurementFeatureMap = 0;

	uint16_t mMeasuredValue = 50;
	uint16_t mMinMeasuredValue = 0;
	uint16_t mMaxMeasuredValue = 100;
};
