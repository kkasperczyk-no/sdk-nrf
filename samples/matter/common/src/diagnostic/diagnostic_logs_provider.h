/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
#include "diagnostic_logs_crash.h"
#include <zephyr/device.h>
#endif

#include "persistent_storage/persistent_storage.h"
#include "util/finite_map.h"

#include <app/clusters/diagnostic-logs-server/DiagnosticLogsProviderDelegate.h>
#include <lib/core/CHIPError.h>

namespace Nrf::Matter
{

class DiagnosticLogProvider : public chip::app::Clusters::DiagnosticLogs::DiagnosticLogsProviderDelegate {
public:
	struct IntentData {
		chip::app::Clusters::DiagnosticLogs::IntentEnum mIntent;
		void *mIntentImpl = nullptr;
	};

	/**
	 * @brief Get the Diagnostic Log Provider instance
	 *
	 * @return DiagnosticLogProvider object
	 */
	static inline DiagnosticLogProvider &GetInstance()
	{
		static DiagnosticLogProvider sInstance;
		return sInstance;
	}

	/**
	 * @brief Initialize the diagnostic log provider module
	 *
	 * @retval CHIP_ERROR_READ_FAILED if the NVS storage contains a crash log entry, but it cannot be loaded.
	 * @retval CHIP_ERROR_WRITE_FAILED if a new crash data could not be stored in the NVS storage.
	 * @retval other CHIP_ERROR values that come from internal methods.
	 */
	CHIP_ERROR Init();

	/* DiagnosticsLogsProviderDelegate Interface */
	CHIP_ERROR StartLogCollection(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				      chip::app::Clusters::DiagnosticLogs::LogSessionHandle &outHandle,
				      chip::Optional<uint64_t> &outTimeStamp,
				      chip::Optional<uint64_t> &outTimeSinceBoot) override;
	CHIP_ERROR EndLogCollection(chip::app::Clusters::DiagnosticLogs::LogSessionHandle sessionHandle) override;
	CHIP_ERROR CollectLog(chip::app::Clusters::DiagnosticLogs::LogSessionHandle sessionHandle,
			      chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	size_t GetSizeForIntent(chip::app::Clusters::DiagnosticLogs::IntentEnum intent) override;
	CHIP_ERROR GetLogForIntent(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				   chip::MutableByteSpan &outBuffer, chip::Optional<uint64_t> &outTimeStamp,
				   chip::Optional<uint64_t> &outTimeSinceBoot) override;

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	bool StoreTestingLog(chip::app::Clusters::DiagnosticLogs::IntentEnum intent, char *text, size_t textSize);
	void ClearTestingBuffer(chip::app::Clusters::DiagnosticLogs::IntentEnum intent);
#endif

private:
	/* The maximum number of the simultaneous sessions */
	constexpr static uint16_t kMaxLogSessionHandle =
		CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS;

	static_assert(kMaxLogSessionHandle < chip::app::Clusters::DiagnosticLogs::kInvalidLogSessionHandle);

	/* Not copyable */
	DiagnosticLogProvider(const DiagnosticLogProvider &) = delete;
	DiagnosticLogProvider &operator=(const DiagnosticLogProvider &) = delete;
	/* Not movable */
	DiagnosticLogProvider(DiagnosticLogProvider &&) = delete;
	DiagnosticLogProvider &operator=(DiagnosticLogProvider &&) = delete;

	DiagnosticLogProvider() = default;

	CHIP_ERROR SetIntentImplementation(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
					   IntentData &intentData);
	void DeleteIntentImplementation(chip::app::Clusters::DiagnosticLogs::IntentEnum intent, IntentData &intentData);

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	size_t GetTestingLogsSize(chip::app::Clusters::DiagnosticLogs::IntentEnum intent);
	CHIP_ERROR GetTestingLogs(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				  chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog);

	constexpr static size_t kTestingBufferLen = 4096;
	uint8_t mTestingUserLogsBuffer[kTestingBufferLen];
	uint8_t mTestingNetworkLogsBuffer[kTestingBufferLen];
	size_t mCurrentUserLogsSize = 0;
	size_t mCurrentNetworkLogsSize = 0;
	size_t mReadUserLogsOffset = 0;
	size_t mReadNetworkLogsOffset = 0;
#endif

	Nrf::FiniteMap<chip::app::Clusters::DiagnosticLogs::LogSessionHandle, IntentData, kMaxLogSessionHandle>
		mIntentMap;
};

} /* namespace Nrf::Matter */
