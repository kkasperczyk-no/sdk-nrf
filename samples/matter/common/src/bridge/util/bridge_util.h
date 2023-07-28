/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"

#include <functional>
#include <map>
#include <cstdint>

/*
   DeviceFactory template container allows to instantiate a map which
   binds supported BridgedDevice::DeviceType types with corresponding
   creation method (e.g. constructor invocation). DeviceFactory can
   only be constructed by passing a user-defined initialized list with
   <BridgedDevice::DeviceType, ConcreteDeviceCreator> pairs.
   Then, Create() method can be used to obtain an instance of demanded
   device type with all passed arguments forwarded to the underlying
   ConcreteDeviceCreator.
*/
template <typename T, typename... Args> class DeviceFactory {
public:
	using ConcreteDeviceCreator = std::function<T *(Args...)>;
	using CreationMap = std::map<BridgedDevice::DeviceType, ConcreteDeviceCreator>;

	DeviceFactory(std::initializer_list<std::pair<BridgedDevice::DeviceType, ConcreteDeviceCreator>> init)
	{
		for (auto &pair : init) {
			mCreationMap.insert(pair);
		}
	}

	DeviceFactory() = delete;
	DeviceFactory(const DeviceFactory &) = delete;
	DeviceFactory(DeviceFactory &&) = delete;
	DeviceFactory &operator=(const DeviceFactory &) = delete;
	DeviceFactory &operator=(DeviceFactory &&) = delete;
	~DeviceFactory() = default;

	T *Create(BridgedDevice::DeviceType deviceType, Args... params)
	{
		if (mCreationMap.find(deviceType) == mCreationMap.end()) {
			return nullptr;
		}
		return mCreationMap[deviceType](std::forward<Args>(params)...);
	}

private:
	CreationMap mCreationMap;
};

/*
   FiniteMap template container allows to wrap mappings between T1 and T2 types.
   The size or the container is predefined at compilation time, therefore
   the maximum number of stored elements must be well known. FiniteMap owns
   inserted values, meaning that once the user inserts a value it will not longer
   be valid outside of the container, i.e. FiniteMap cannot return stored object by copy.
   FiniteMap's API offers basic operations like:
     * inserting a new value under provided key (Insert)
     * erasing existing item under given key (Erase)
     * retrieving value stored under provided key ([] operator)
     * checking if the map contains a non-null value under given key (Contains)
     * iterating though stored item via publicly available mMap member
   Prerequisites:
     * T1 must have == operator implemented
     * T2 must have =operator(&&) and bool()operator implemented
*/
template <typename T1, typename T2, std::size_t N> struct FiniteMap {
	struct Pair {
		T1 key;
		T2 value;
	};

	void Insert(const T1 &key, T2 &&value)
	{
		if (Contains(key)) {
			/* The key with sane value already exists in the map, return prematurely. */
			return;
		} else if (mElementsCount < N) {
			mMap[mElementsCount].key = key;
			mMap[mElementsCount].value = std::move(value);
			mElementsCount++;
		}
	}

	bool Erase(const T1 &key)
	{
		const auto &it = std::find_if(std::begin(mMap), std::end(mMap),
					      [key](const Pair &pair) { return pair.key == key; });
		if (it != std::end(mMap) && it->value) {
			/* Invalidate the value but leave the key unchanged */
			it->value = T2{};
			mElementsCount--;
			return true;
		}
		return false;
	}

	/* TODO: refactor to return a reference (Constains() check will be always needed) */
	const T2 *operator[](const T1 &key) const
	{
		for (auto &it : mMap) {
			if (key == it.key)
				return &(it.value);
		}
		return nullptr;
	}

	bool Contains(const T1 &key)
	{
		const T2 *stored = (*this)[key];
		if (stored && *stored) {
			/* The key with sane value found in the map */
			return true;
		}
		return false;
	}

	Pair mMap[N];
	uint16_t mElementsCount{ 0 };
};
