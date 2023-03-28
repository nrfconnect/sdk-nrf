/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/kernel.h>

#include <avsystem/commons/avs_errno_map.h>
#include <avsystem/commons/avs_stream_v_table.h>
#include <avsystem/commons/avs_utils.h>

#include <anjay_zephyr/config.h>

#include "factory_flash.h"

/*
 * THE PROCESS FOR FLASHING FACTORY PROVISIONING INFORMATION
 *
 * High level flow from the user's standpoint:
 *
 * 1. Flash the board with firmware that has
 *    CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING_INITIAL_FLASH enabled.
 * 2. Wait for the board to boot. For best results, open some terminal program
 *    to flush any data sent by the board while booting that might be already
 *    buffered by the OS serial port handling.
 * 3. Run:
 *    mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200" \
 *           fs upload SenMLCBOR /factory/provision.cbor
 * 4. Run:
 *    mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200" \
 *           fs download /factory/result.txt result.txt
 * 5. Examine the code in result.txt. If it's "0", then the operation was
 *    successful.
 * 6. Flash the board with firmware that has
 *    CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING enabled, but
 *    CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING_INITIAL_FLASH disabled.
 *
 * What happens under the hood during steps 3 and 4:
 *
 * 1. run_anjay() in main.c calls factory_flash_input_stream_init(). This
 *    initializes the fake "provision_fs" file system and mounts it at /factory.
 * 2. mcumgr starts writing the /factory/provision.cbor file. This will call:
 *    - provision_fs_stat() - that will error out; mcumgr code only does this
 *      to check if the file already exists; we return that it doesn't
 *    - provision_fs_open()
 *    - provision_fs_lseek()
 *    - provision_fs_write()
 *    - close - it's not implemented, so it's a no-op
 *    - go back to provision_fs_open() again, and repeat until the whole file is
 *      transferred
 * 3. Controlled by factory_flash_mutex and factory_flash_condvar, the data
 *    written through these write operations is passed as the stream read by
 *    anjay_factory_provision().
 * 4. Unfortunately, mcumgr API does not inform the file system driver in any
 *    way whether EOF has been reached. For that reason, we wait either for
 *    timeout or for opening the result file.
 * 5. mcumgr starts reading the /factory/result.txt file. This will call:
 *    - provision_fs_stat() - this needs to success for the read operation to
 *      succeed; we need to know the file size at this point, so this operation
 *      will use the mutex and condvar to wait until the Anjay thread calls
 *      factory_flash_finished().
 *    - provision_fs_open()
 *    - provision_fs_lseek()
 *    - provision_fs_read()
 *    - close - it's not implemented, so it's a no-op
 *    - go back to provision_fs_open() again, and repeat until the whole file is
 *      transferred
 */

// circular buffer
static char received_data[512];
static size_t received_data_start;
static size_t received_data_length;
static size_t received_data_total;

static enum {
	FACTORY_FLASH_INITIAL,
	FACTORY_FLASH_EOF,
	FACTORY_FLASH_FINISHED
} factory_flash_state;
static char factory_flash_result[AVS_INT_STR_BUF_SIZE(int)];

static K_MUTEX_DEFINE(factory_flash_mutex);
static K_CONDVAR_DEFINE(factory_flash_condvar);

#define PROVISION_FS_TYPE FS_TYPE_EXTERNAL_BASE

#define PROVISION_FS_MOUNT_POINT "/factory"

enum provision_fs_files { FLASH_FILE, RESULT_FILE, EP_FILE };

static struct provision_fs_file_info {
	const char *const name;
	const uint8_t *value;
	size_t size;
	size_t offset;
} files[] = {
	[FLASH_FILE] = { .name = PROVISION_FS_MOUNT_POINT "/provision.cbor" },
	[RESULT_FILE] = { .name = PROVISION_FS_MOUNT_POINT "/result.txt" },
	[EP_FILE] = { .name = PROVISION_FS_MOUNT_POINT "/endpoint.txt" },
};

// 15 seconds of inactivity is treated as EOF
#define PROVISION_FS_UPLOAD_TIMEOUT_MS 15000

static int provision_fs_open(struct fs_file_t *filp, const char *fs_path, fs_mode_t flags)
{
	if (strcmp(fs_path, files[FLASH_FILE].name) == 0) {
		if ((flags & (FS_O_READ | FS_O_WRITE)) == FS_O_WRITE &&
		    factory_flash_state == FACTORY_FLASH_INITIAL) {
			filp->filep = &files[FLASH_FILE];
			return 0;
		}
	} else if (strcmp(fs_path, files[RESULT_FILE].name) == 0) {
		if ((flags & (FS_O_READ | FS_O_WRITE)) == FS_O_READ) {
			filp->filep = &files[RESULT_FILE];
			return 0;
		}
	} else if (strcmp(fs_path, files[EP_FILE].name) == 0) {
		if ((flags & (FS_O_READ | FS_O_WRITE)) == FS_O_READ) {
			filp->filep = &files[EP_FILE];
			return 0;
		}
	}

	return -ENOENT;
}

static ssize_t provision_fs_read(struct fs_file_t *filp, void *dest, size_t nbytes)
{
	if (!filp->filep) {
		return -EBADF;
	}

	ssize_t result = k_mutex_lock(&factory_flash_mutex, K_FOREVER);

	if (result) {
		return result;
	}

	if (filp->filep == &files[RESULT_FILE] || filp->filep == &files[EP_FILE]) {
		struct provision_fs_file_info *file_info = filp->filep;
		const uint8_t *src = file_info->value + file_info->offset;
		size_t bytes_to_copy = AVS_MIN(file_info->size - file_info->offset, nbytes);

		memcpy(dest, src, bytes_to_copy);

		file_info->offset += bytes_to_copy;
		result = bytes_to_copy;
	} else {
		result = -EBADF;
	}

	k_mutex_unlock(&factory_flash_mutex);

	return result;
}

static ssize_t provision_fs_write(struct fs_file_t *filp, const void *src, size_t nbytes)
{
	if (factory_flash_state != FACTORY_FLASH_INITIAL || filp->filep != &files[FLASH_FILE]) {
		return -EBADF;
	}

	if (nbytes == 0) {
		return 0;
	}

	if (nbytes > sizeof(received_data)) {
		return -ENOSPC;
	}

	int result = k_mutex_lock(&factory_flash_mutex, K_FOREVER);

	if (result) {
		return result;
	}

	const char *src_ptr = (const char *)src;
	const char *const src_end = src_ptr + nbytes;

	while (src_ptr < src_end) {
		size_t write_start =
			(received_data_start + received_data_length) % sizeof(received_data);
		size_t immediate_size = AVS_MIN(
			AVS_MIN(sizeof(received_data) - received_data_length, src_end - src_ptr),
			sizeof(received_data) - write_start);

		if (!immediate_size) {
			k_condvar_wait(&factory_flash_condvar, &factory_flash_mutex, K_FOREVER);
		} else {
			memcpy(received_data + write_start, src_ptr, immediate_size);
			received_data_length += immediate_size;
			received_data_total += immediate_size;
			src_ptr += immediate_size;
			k_condvar_broadcast(&factory_flash_condvar);
		}
	}

	k_mutex_unlock(&factory_flash_mutex);

	return nbytes;
}

static int provision_fs_lseek(struct fs_file_t *filp, off_t off, int whence)
{
	assert(whence == FS_SEEK_SET);
	if (filp->flags & FS_O_WRITE) {
		// The provisioning file
		assert(off == (size_t)received_data_total);
	} else {
		// The status file
		struct provision_fs_file_info *file_info = filp->filep;

		if (off > file_info->size) {
			return -ENXIO;
		}
		file_info->offset = off;
	}
	return 0;
}

static int provision_fs_mount(struct fs_mount_t *mountp)
{
	(void)mountp;
	return 0;
}

static int provision_fs_unlink(struct fs_mount_t *mountp, const char *name)
{
	(void)mountp;
	(void)name;
	return 0;
}

static int provision_fs_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry)
{
	(void)mountp;
	(void)entry;

	if (strcmp(path, files[RESULT_FILE].name) != 0 && strcmp(path, files[EP_FILE].name) != 0) {
		return -ENOENT;
	}

	int result = k_mutex_lock(&factory_flash_mutex, K_FOREVER);

	if (result) {
		return result;
	}

	struct provision_fs_file_info *file_info;

	if (strcmp(path, files[RESULT_FILE].name) == 0) {
		if (factory_flash_state < FACTORY_FLASH_FINISHED) {
			factory_flash_state = FACTORY_FLASH_EOF;
			k_condvar_broadcast(&factory_flash_condvar);
		}

		while (factory_flash_state != FACTORY_FLASH_FINISHED) {
			k_condvar_wait(&factory_flash_condvar, &factory_flash_mutex, K_FOREVER);
		}

		file_info = &files[RESULT_FILE];
		file_info->value = factory_flash_result;
	} else {
		file_info = &files[EP_FILE];
		file_info->value = anjay_zephyr_config_default_ep_name();
	}

	file_info->size = strlen(file_info->value);

	entry->size = file_info->size;
	entry->type = FS_DIR_ENTRY_FILE;

	k_mutex_unlock(&factory_flash_mutex);

	return 0;
}

static const struct fs_file_system_t provision_fs = { .open = provision_fs_open,
						      .read = provision_fs_read,
						      .write = provision_fs_write,
						      .lseek = provision_fs_lseek,
						      .mount = provision_fs_mount,
						      .unlink = provision_fs_unlink,
						      .stat = provision_fs_stat };

static struct fs_mount_t provision_fs_mount_point = { .type = PROVISION_FS_TYPE,
						      .mnt_point = PROVISION_FS_MOUNT_POINT };

static avs_error_t provision_stream_read(avs_stream_t *stream, size_t *out_bytes_read,
					 bool *out_message_finished, void *buffer,
					 size_t buffer_length)
{
	(void)stream;
	assert(out_bytes_read);

	if (buffer_length == 0) {
		*out_bytes_read = 0;
		if (out_message_finished) {
			*out_message_finished = false;
		}
		return AVS_OK;
	}

	int result = k_mutex_lock(&factory_flash_mutex, K_FOREVER);

	if (result) {
		return avs_errno(avs_map_errno(-result));
	}

	k_timeout_t timeout = K_FOREVER;

	if (received_data_total > 0) {
		// Some data arrived, so let's treat timeout as EOF
		int64_t uptime = k_uptime_get();

		timeout = K_TIMEOUT_ABS_MS(uptime + PROVISION_FS_UPLOAD_TIMEOUT_MS);
	}

	while (!received_data_length && factory_flash_state == FACTORY_FLASH_INITIAL &&
	       (K_TIMEOUT_EQ(timeout, K_FOREVER) || k_uptime_ticks() < Z_TICK_ABS(timeout.ticks))) {
		k_condvar_wait(&factory_flash_condvar, &factory_flash_mutex, timeout);
	}

	*out_bytes_read = AVS_MIN(AVS_MIN(received_data_length, buffer_length),
				  sizeof(received_data) - received_data_start);
	memcpy(buffer, received_data + received_data_start, *out_bytes_read);
	received_data_start = (received_data_start + *out_bytes_read) % sizeof(received_data);
	received_data_length -= *out_bytes_read;

	k_mutex_unlock(&factory_flash_mutex);

	if (out_message_finished) {
		*out_message_finished = (*out_bytes_read == 0);
	}

	return AVS_OK;
}

static const avs_stream_v_table_t provision_stream_vtable = { .read = provision_stream_read };

static struct {
	const avs_stream_v_table_t *const vtable;
} provision_stream = { .vtable = &provision_stream_vtable };

avs_stream_t *factory_flash_input_stream_init(void)
{
	if (fs_register(PROVISION_FS_TYPE, &provision_fs) || fs_mount(&provision_fs_mount_point)) {
		return NULL;
	}

	return (avs_stream_t *)&provision_stream;
}

void factory_flash_finished(int result)
{
	k_mutex_lock(&factory_flash_mutex, K_FOREVER);
	avs_simple_snprintf(factory_flash_result, sizeof(factory_flash_result), "%d", result);
	factory_flash_state = FACTORY_FLASH_FINISHED;
	k_condvar_broadcast(&factory_flash_condvar);
	k_mutex_unlock(&factory_flash_mutex);
}
