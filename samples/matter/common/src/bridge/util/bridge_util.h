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

namespace Nrf
{

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

} /* namespace Nrf */
