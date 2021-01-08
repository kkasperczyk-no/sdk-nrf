/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#pragma once

#include <cstdint>

struct AppEvent {
	enum LightEventType : uint8_t {
		On,
		Off,
		Toggle,
		Level
	};

	enum OtherEventType : uint8_t {
		FactoryReset = Level + 1,
		StartThread,
		StartBleAdvertising,
		PublishLightBulbService
	};

	AppEvent() = default;

	AppEvent(LightEventType type, uint8_t value, bool chipInitiated) : Type(type), Value(value), LightEvent{chipInitiated}
	{
	}

	AppEvent(OtherEventType type) : Type(type)
	{
	}

	uint8_t Type;
	uint8_t Value;

	union {
		struct {
			// was the event triggered by CHIP Data Model layer
			bool ChipInitiated;
		} LightEvent;
	};
};
