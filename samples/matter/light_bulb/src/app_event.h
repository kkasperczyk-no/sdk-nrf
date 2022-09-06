/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

#include "led_widget.h"

struct AppEvent;
typedef void (*EventHandler)(const AppEvent &);

struct AppEvent {
	enum AppEventType {
		Button = 0,
		Timer,
		Lighting,
		UpdateLedState,
#ifdef CONFIG_MCUMGR_SMP_BT
		StartSMPAdvertising,
#endif
	};

	uint16_t Type;

	union {
		struct {
			uint8_t PinNo;
			uint8_t Action;
		} ButtonEvent;
		struct {
			void *Context;
		} TimerEvent;
		struct {
			uint8_t Action;
			int32_t Actor;
		} LightingEvent;
		struct {
			LEDWidget *LedWidget;
		} UpdateLedStateEvent;
	};

	EventHandler Handler;
};
