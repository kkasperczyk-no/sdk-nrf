/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"

class OnOffLightDevice : public BridgedDevice {
public:
	OnOffLightDevice(const char *nodeLabel);

	void Set(bool onOff) { mOnOff = onOff; }
	bool GetOnOff() { return mOnOff; }
	void Toggle() { mOnOff = !mOnOff; }
	uint16_t GetOnOffClusterRevision() { return kOnOffClusterRevision; }
	uint32_t GetOnOffFeatureMap() { return kOnOffFeatureMap; }

	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadOnOff(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

private:
	static constexpr uint16_t kOnOffClusterRevision = 1;
	static constexpr uint32_t kOnOffFeatureMap = 0;

	bool mOnOff = false;
};
