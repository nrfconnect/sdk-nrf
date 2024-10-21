/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_storage_shell.h"

#if defined(CONFIG_NVS)
#include <zephyr/fs/nvs.h>
#elif defined(CONFIG_ZMS)
#include <zephyr/fs/zms.h>
#endif
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>

using namespace Nrf;

namespace
{
#if defined(CONFIG_NVS)
nvs_fs *sStorage = nullptr;
#elif defined(CONFIG_ZMS)
zms_fs *sStorage = nullptr;
#endif

size_t sPeakSize = 0;

struct SettingsParams {
	const struct shell *shell;
};

int CalculateFreeSpace()
{
#if defined(CONFIG_NVS)
	return nvs_calc_free_space(sStorage);
#elif defined(CONFIG_ZMS)
	return zms_calc_free_space(sStorage);
#endif
}

int SettingsReadClb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
	uint8_t buffer[SETTINGS_MAX_VAL_LEN];
	size_t readBytes = MIN(len, SETTINGS_MAX_VAL_LEN);
	SettingsParams *params = reinterpret_cast<SettingsParams *>(param);

	/* Process only the exact match and ignore descendants of the searched name */
	if (settings_name_next(key, NULL) != 0) {
		return 0;
	}

	readBytes = read_cb(cb_arg, buffer, readBytes);

	shell_fprintf(params->shell, SHELL_NORMAL, "%zu", readBytes);

	return 0;
}

int PeakHandler(const struct shell *shell, size_t argc, char **argv)
{
	if (!sStorage) {
		return -ENODEV;
	}

	size_t maxSize = sStorage->sector_size * sStorage->sector_count;
	size_t freeSpace = CalculateFreeSpace();
	size_t currentPeak = maxSize - freeSpace;

	if (currentPeak > sPeakSize) {
		sPeakSize = currentPeak;
	}

	shell_fprintf(shell, SHELL_NORMAL, "%zu\n", sPeakSize);

	return 0;
}

int ResetHandler(const struct shell *shell, size_t argc, char **argv)
{
	sPeakSize = 0;

	return 0;
}

int GetSizeHandler(const struct shell *shell, size_t argc, char **argv)
{
	if (!sStorage) {
		return -ENODEV;
	}

	SettingsParams params = { .shell = shell };

	int err = settings_load_subtree_direct(argv[argc - 1], SettingsReadClb, &params);

	return err;
}

int CurrentHandler(const struct shell *shell, size_t argc, char **argv)
{
	if (!sStorage) {
		return -ENODEV;
	}

	size_t maxSize = sStorage->sector_size * sStorage->sector_count;
	size_t freeSpace = CalculateFreeSpace();

	shell_fprintf(shell, SHELL_NORMAL, "%zu\n", maxSize - freeSpace);

	return 0;
}

int FreeHandler(const struct shell *shell, size_t argc, char **argv)
{
	if (!sStorage) {
		return -ENODEV;
	}

	size_t freeSpace = CalculateFreeSpace();

	shell_fprintf(shell, SHELL_NORMAL, "%zu\n", freeSpace);

	return 0;
}

} // namespace

bool PersistentStorageShell::Init()
{
	void *storage;

	if (settings_storage_get(&storage)) {
		return false;
	}

#if defined(CONFIG_NVS)
	sStorage = reinterpret_cast<nvs_fs *>(storage);
#elif defined(CONFIG_ZMS)
	sStorage = reinterpret_cast<zms_fs *>(storage);
#endif

	return true;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_persistent_storage,
			       SHELL_CMD_ARG(peak, NULL,
					     "Print peak settings size in Bytes. This size is reset during reboot. \n"
					     "Usage: matter_settings peak\n",
					     PeakHandler, 1, 1),
			       SHELL_CMD_ARG(reset, NULL,
					     "Reset peak settings size in Bytes. \n"
					     "Usage: matter_settings reset\n",
					     ResetHandler, 1, 1),
			       SHELL_CMD_ARG(get_size, NULL,
					     "Get size of the chosen settings entry. \n"
					     "Usage: matter_settings get_size <name>\n",
					     GetSizeHandler, 2, 1),
			       SHELL_CMD_ARG(current, NULL,
					     "Get current settings size in Bytes. \n"
					     "Usage: matter_settings current\n",
					     CurrentHandler, 1, 1),
			       SHELL_CMD_ARG(free, NULL,
					     "Get free settings space in Bytes. \n"
					     "Usage: matter_settings current\n",
					     FreeHandler, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(matter_settings, &sub_persistent_storage, "Matter persistent storage ", NULL);
