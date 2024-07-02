/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <credentials/GroupDataProviderImpl.h>

namespace Nrf::Matter
{
class GroupDataProviderImpl : public chip::Credentials::GroupDataProviderImpl {
public:
	/**
	 * @brief Return the static GroupDataProviderImpl instance.
	 */
	static GroupDataProviderImpl &Instance()
	{
		static GroupDataProviderImpl sGroupDataProviderImpl;
		return sGroupDataProviderImpl;
	}

	/**
	 * @brief Inform that the factory reset will happen.
	 *
	 * This function informs the GroupDataProviderImpl that the factory
	 * reset will happen, therefore we can skip the time-consuming default
	 * RemoveFabric's operations that heavily use persistent storage.
	 *
	 * It should be called before scheduling the factory reset with
	 * chip::Server::ScheduleFactoryReset().
	 *
	 */
	void WillBeFactoryReseted() { mClearStorage = false; }

	CHIP_ERROR RemoveFabric(chip::FabricIndex fabricIdx) override
	{
		if (mClearStorage) {
			return chip::Credentials::GroupDataProviderImpl::RemoveFabric(fabricIdx);
		} else {
			/* The storage will be erased and the device will be rebooted anyway,
			   so to optimize the time needed for the factory reset we can simply
			   do nothing. */
			return CHIP_NO_ERROR;
		}
	}

	/* No copy, nor move. */
	GroupDataProviderImpl(const GroupDataProviderImpl &) = delete;
	GroupDataProviderImpl &operator=(const GroupDataProviderImpl &) = delete;
	GroupDataProviderImpl(GroupDataProviderImpl &&) = delete;
	GroupDataProviderImpl &operator=(GroupDataProviderImpl &&) = delete;

private:
	GroupDataProviderImpl() = default;
	~GroupDataProviderImpl() = default;

	bool mClearStorage{ true };
};

} // namespace Nrf::Matter
