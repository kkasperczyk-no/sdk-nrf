/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"

class TemperatureSensorDevice : public BridgedDevice {
public:
	TemperatureSensorDevice(const char *nodeLabel);

	int16_t GetMeasuredValue() { return mMeasuredValue; }
	int16_t GetMinMeasuredValue() { return mMinMeasuredValue; }
	int16_t GetMaxMeasuredValue() { return mMaxMeasuredValue; }
	uint16_t GetTemperatureMeasurementClusterRevision() { return kTemperatureMeasurementClusterRevision; }
	uint32_t GetTemperatureMeasurementFeatureMap() { return kTemperatureMeasurementFeatureMap; }

	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadTemperatureMeasurement(chip::AttributeId attributeId, uint8_t *buffer,
						    uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

private:
	static constexpr uint16_t kTemperatureMeasurementClusterRevision = 1;
	static constexpr uint32_t kTemperatureMeasurementFeatureMap = 0;

	int16_t mMeasuredValue = 20;
	int16_t mMinMeasuredValue = -10;
	int16_t mMaxMeasuredValue = 40;
};
