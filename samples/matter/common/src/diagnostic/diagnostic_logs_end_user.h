/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "diagnostic_logs_provider.h"
#include <lib/core/CHIPError.h>

class DiagnosticLogsEndUserReader : public Nrf::Matter::DiagnosticLogsIntentIface {
public:
	CHIP_ERROR GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	CHIP_ERROR FinishLogs() override;
	size_t GetLogsSize() override;

private:
	uint32_t mReadOffset = 0;
	bool mReadInProgress = false;
};

class DiagnosticLogsEndUser {
public:
	CHIP_ERROR Init();
	CHIP_ERROR PushLog(const void *data, size_t size);
	CHIP_ERROR Clear();

	static DiagnosticLogsEndUser &Instance()
	{
		static DiagnosticLogsEndUser sInstance;
		return sInstance;
	}

	size_t GetLogsSize() { return mCurrentSize; }
	uint32_t GetLogsBegin() { return mDataBegin; }
	size_t GetBytesToWrapNumber(size_t inputSize) { return inputSize > mCapacity ? inputSize - mCapacity : 0; }
	size_t GetCapacity() { return mCapacity; }
	uint32_t GetRetentionAddress(uint32_t address) { return address + kHeaderSize; }
	bool IsEnd(uint32_t address) { return address == mDataEnd; }

private:
	const size_t kHeaderSize = sizeof(mCurrentSize) + sizeof(mDataBegin);

	bool mIsInitialized = false;
	size_t mCurrentSize = 0;
	uint32_t mDataBegin = 0;
	uint32_t mDataEnd = 0;
	size_t mCapacity = 0;
};
