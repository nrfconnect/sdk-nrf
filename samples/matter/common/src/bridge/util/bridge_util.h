/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/support/CHIPMem.h>

#include <cstdint>
#include <functional>
#include <map>

namespace Nrf {

/*
   DeviceFactory template container allows to instantiate a map which
   binds supported Matter device type identifiers (uint16_t) with corresponding
   creation method (e.g. constructor invocation). DeviceFactory can
   only be constructed by passing a user-defined initialized list with
   <uint16_t, ConcreteDeviceCreator> pairs.
   Then, Create() method can be used to obtain an instance of demanded
   device type with all passed arguments forwarded to the underlying
   ConcreteDeviceCreator.
*/
template <typename T, typename DeviceType, typename... Args> class DeviceFactory {
public:
	using ConcreteDeviceCreator = std::function<T *(Args...)>;
	using CreationMap = std::map<DeviceType, ConcreteDeviceCreator>;

	DeviceFactory(std::initializer_list<std::pair<DeviceType, ConcreteDeviceCreator>> init)
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

	T *Create(DeviceType deviceType, Args... params)
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
   FiniteMap template container allows to wrap mappings between uint16_t-type key and T-type value.
   The size or the container is predefined at compilation time, therefore
   the maximum number of stored elements must be well known. FiniteMap owns
   inserted values, meaning that once the user inserts a value it will not longer
   be valid outside of the container, i.e. FiniteMap cannot return stored object by copy.
   FiniteMap's API offers basic operations like:
     * inserting new values under provided key (Insert)
     * erasing existing item under given key (Erase) - this operation takes care about the duplicated items,
       meaning that pointer under given key is released only if the same address is not present elsewhere in the map
     * retrieving values stored under provided key ([] operator)
     * checking if the map contains a non-null value under given key (Contains)
	 * retrieving a number of free slots available in the map (FreeSlots)
     * iterating though stored item via publicly available mMap member
	Prerequisites:
     * T must have move semantics and bool()/==operators implemented
*/
template <typename T, std::size_t N> struct FiniteMap {
	static constexpr uint16_t kInvalidKey{ N + 1 };
	struct Item {
		/* Initialize with invalid key (0 is a valid key) */
		uint16_t key{ kInvalidKey };
		T value;
	};

	bool Insert(uint16_t key, T &&value)
	{
		if (Contains(key)) {
			/* The key with sane value already exists in the map, return prematurely. */
			return false;
		} else if (mElementsCount < N) {
			mMap[key].key = key;
			mMap[key].value = std::move(value);
			mElementsCount++;
		}
		return true;
	}

	bool Erase(uint16_t key)
	{
		const auto &it = std::find_if(std::begin(mMap), std::end(mMap),
					      [key](const Item &item) { return item.key == key; });
		if (it != std::end(mMap) && it->value) {
			it->value = T{};
			it->key = kInvalidKey;
			mElementsCount--;
			return true;
		}
		return false;
	}

	/* Always use Constains() before using operator[]. */
	T &operator[](uint16_t key)
	{
		static T dummyObject;
		for (auto &it : mMap) {
			if (key == it.key)
				return it.value;
		}
		return dummyObject;
	}

	bool Contains(uint16_t key)
	{
		for (auto &it : mMap) {
			if (key == it.key)
				return true;
		}
		return false;
	}
	std::size_t FreeSlots() { return N - mElementsCount; }

	uint8_t GetDuplicatesCount(const T &value, uint16_t *key)
	{
		/* Find the first duplicated item and return its key,
		 so that the application can handle the duplicate by itself. */
		*key = kInvalidKey;
		uint8_t numberOfDuplicates = 0;
		for (auto it = std::begin(mMap); it != std::end(mMap); ++it) {
			if (it->value == value) {
				*(key++) = it->key;
				numberOfDuplicates++;
			}
		}

		return numberOfDuplicates;
	}

	Item mMap[N];
	uint16_t mElementsCount{ 0 };
};

} /* namespace Nrf */
