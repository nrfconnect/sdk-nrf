/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

namespace Nrf
{
/*
   FiniteMap template container allows to wrap mappings between T1-type trivial key and T2-type value.
   The size or the container is predefined at compilation time, therefore
   the maximum number of stored elements must be well known. FiniteMap owns
   inserted values, meaning that once the user inserts a value it will not longer
   be valid outside of the container, i.e. FiniteMap cannot return stored object by copy.
   FiniteMap's API offers basic operations like:
     * inserting new values under provided key (Insert)
     * erasing existing items under a given key (Erase) - this operation does not take care of the
	   duplicated items,  meaning that the pointer under a given key is released unconditionally
	   and the responsibility for the duplications is on the application side.
     * retrieving values stored under provided key ([] operator)
     * checking if the map contains a non-null value under given key (Contains)
	 * retrieving a number of free slots available in the map (FreeSlots)
     * iterating though stored item via publicly available mMap member
     * retrieving the first free slot (GetFirstFreeSlot)
	Prerequisites:
     * T1 must be trivial and the maximum numeric limit for the T1-type value is reserved and assigned as an invalid
   key.
     * T2 must have move semantics and bool()/==operators implemented
*/
template <typename T1, typename T2, uint16_t N> struct FiniteMap {
	static_assert(std::is_trivial_v<T1>);

	static constexpr T1 kInvalidKey{ std::numeric_limits<T1>::max() };
	static constexpr std::size_t kNoSlotsFound{ N + 1 };
	using ElementCounterType = uint16_t;

	struct Item {
		/* Initialize with invalid key (0 is a valid key) */
		T1 key{ kInvalidKey };
		T2 value;
	};

	bool Insert(T1 key, T2 &&value)
	{
		if (Contains(key)) {
			/* The key already exists in the map, return prematurely. */
			return false;
		} else if (mElementsCount < N) {
			std::size_t slot = GetFirstFreeSlot();
			if (slot == kNoSlotsFound) {
				return false;
			}
			mMap[slot].key = key;
			mMap[slot].value = std::move(value);
			mElementsCount++;
		} else {
			return false;
		}
		return true;
	}

	bool Erase(T1 key)
	{
		const auto &it = std::find_if(std::begin(mMap), std::end(mMap),
					      [key](const Item &item) { return item.key == key; });
		if (it != std::end(mMap)) {
			it->value = T2{};
			it->key = kInvalidKey;
			mElementsCount--;
			return true;
		}
		return false;
	}

	/* Always use Contains() before using operator[]. */
	T2 &operator[](T1 key)
	{
		static T2 dummyObject;
		for (auto &it : mMap) {
			if (key == it.key)
				return it.value;
		}
		return dummyObject;
	}

	bool Contains(T1 key)
	{
		for (auto &it : mMap) {
			if (key == it.key)
				return true;
		}
		return false;
	}

	ElementCounterType FreeSlots() { return N - mElementsCount; }

	ElementCounterType Size() { return mElementsCount; }

	ElementCounterType GetFirstFreeSlot()
	{
		ElementCounterType foundIndex = 0;
		for (auto &it : mMap) {
			if (kInvalidKey == it.key)
				return foundIndex;
			foundIndex++;
		}
		return kNoSlotsFound;
	}

	uint8_t GetDuplicatesCount(const T2 &value, T1 *key)
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
	ElementCounterType mElementsCount{ 0 };
};

} /* namespace Nrf */
