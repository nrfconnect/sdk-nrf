/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/server/Server.h>

namespace Nrf::Matter
{
namespace Migration
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS
	/**
	 * @brief Migrate all stored Operational Keys from the persistent storage (KVS) to secure PSA ITS.
	 *
	 * This function will schedule a factory reset automatically if the
	 * CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE
	 * Kconfig option is set to 'y'. In this case, the function returns CHIP_NO_ERROR to not block any further
	 * operations until the scheduled factory reset is done.
	 *
	 * @note This function should be called just after Matter Server Init to avoid problems with further CASE
	 * session re-establishments.
	 * @param storage
	 * @param keystore
	 * @retval CHIP_NO_ERROR if all keys have been migrated properly to PSA ITS or if the error occurs, but
	 * 		   the CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE kconfig is set to 'y'.
	 * @retval CHIP_ERROR_INVALID_ARGUMENT when keystore or storage are not defined.
	 * @retval Other CHIP_ERROR codes related to internal Migration operations.
	 */
	CHIP_ERROR MoveOperationalKeysFromKvsToIts(chip::PersistentStorageDelegate *storage,
						   chip::Crypto::OperationalKeystore *keystore);
#endif
} /* namespace Migration */
} /* namespace Nrf::Matter */
