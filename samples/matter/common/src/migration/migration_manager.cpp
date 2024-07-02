/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "migration_manager.h"

#include "app/group_data_provider.h"

#include <crypto/OperationalKeystore.h>
#include <crypto/PersistentStorageOperationalKeystore.h>

namespace Nrf::Matter
{
namespace Migration
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS
	CHIP_ERROR MoveOperationalKeysFromKvsToIts(chip::PersistentStorageDelegate *storage,
						   chip::Crypto::OperationalKeystore *keystore)
	{
		CHIP_ERROR err = CHIP_NO_ERROR;

		VerifyOrReturnError(keystore && storage, CHIP_ERROR_INVALID_ARGUMENT);

		/* Initialize the obsolete Operational Keystore*/
		chip::PersistentStorageOperationalKeystore obsoleteKeystore;
		err = obsoleteKeystore.Init(storage);
		VerifyOrReturnError(err == CHIP_NO_ERROR, err);

		/* Migrate all obsolete Operational Keys to PSA ITS */
		for (const chip::FabricInfo &fabric : chip::Server::GetInstance().GetFabricTable()) {
			err = keystore->MigrateOpKeypairForFabric(fabric.GetFabricIndex(), obsoleteKeystore);
			if (CHIP_NO_ERROR != err) {
				break;
			}
		}

#ifdef CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE
		if (CHIP_NO_ERROR != err) {
			GroupDataProviderImpl::Instance().WillBeFactoryReseted();
			chip::Server::GetInstance().ScheduleFactoryReset();
			/* Return a success to not block the Matter event Loop and allow to call scheduled factory
			 * reset. */
			err = CHIP_NO_ERROR;
		}
#endif /* CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE */

		return err;
	}
#endif /* CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS */
} /* namespace Migration */
} /* namespace Nrf::Matter */
