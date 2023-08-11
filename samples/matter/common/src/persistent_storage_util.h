/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/settings/settings.h>
#include <zephyr/sys/cbprintf.h>

class PersistentStorageNode {
public:
	static constexpr auto kMaxKeyNameLength = (SETTINGS_MAX_NAME_LEN + 1);

	PersistentStorageNode(const char *keyName, size_t keyNameLength, PersistentStorageNode *parent = nullptr)
	{
		if (keyName && keyNameLength < kMaxKeyNameLength) {
			memcpy(mKeyName, keyName, keyNameLength);
			mKeyName[keyNameLength] = '\0';
		}
		mParent = parent;
	}

	bool GetKey(char *key);

private:
	PersistentStorageNode *mParent = nullptr;
	char mKeyName[kMaxKeyNameLength] = { 0 };
};

class PersistentStorage {
public:
	bool Init();
	bool Store(PersistentStorageNode *node, const void *data, size_t dataSize);
	bool Load(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	bool HasEntry(PersistentStorageNode *node);
	bool Remove(PersistentStorageNode *node);

	static PersistentStorage &Instance()
	{
		static PersistentStorage sInstance;
		return sInstance;
	}

private:
	bool LoadEntry(const char *key, void *data = nullptr, size_t dataMaxSize = 0, size_t *outSize = nullptr);
};
