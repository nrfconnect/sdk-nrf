/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/reboot.h>

#include <dfu/dfu_target_mcuboot.h>
#include <net/downloader.h>

#include "ota_bundle_decode.h"
#include "ota_download.h"
#include "sm_xdfu.h"

LOG_MODULE_REGISTER(ota_download, LOG_LEVEL_INF);

static const struct gpio_dt_spec modem_reset = {
	.port = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
	.pin = 7,
	.dt_flags = GPIO_ACTIVE_LOW | GPIO_OPEN_DRAIN,
};

static const char *ota_target_name(int target)
{
	switch (target) {
	case ota_element_target_nrf53_app_m_c:
		return "nrf53_app";
	case ota_element_target_nrf53_net_m_c:
		return "nrf53_net";
	case ota_element_target_nrf53_bl_m_c:
		return "nrf53_bl";
	case ota_element_target_nrf91_app_m_c:
		return "nrf91_app";
	case ota_element_target_nrf91_bl_m_c:
		return "nrf91_bl";
	case ota_element_target_nrf91_mdm_delta_m_c:
		return "nrf91_mdm_delta";
	case ota_element_target_nrf91_mdm_full_m_c:
		return "nrf91_mdm_full";
	default:
		return "unknown";
	}
}

static struct downloader dl;
static struct fs_file_t ota_file;
static char dl_buf[2048] __aligned(4);
static uint8_t mcuboot_buf[1024] __aligned(4);
static K_SEM_DEFINE(dl_done, 0, 1);
static int dl_result;

#define OTA_PARTIAL_PATH "/ota/bundle.part"
#define OTA_STATE_PATH "/ota/download.state"
#define OTA_STATE_TMP_PATH "/ota/download.state.tmp"
#define OTA_STATE_MAGIC 0x4f544153
#define OTA_STATE_VERSION 1
#define OTA_CHECKPOINT_SIZE (16 * 1024)
#define OTA_URL_MAX_SIZE \
	(CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE + CONFIG_DOWNLOADER_MAX_FILENAME_SIZE + 16)

struct ota_download_state {
	uint32_t magic;
	uint32_t version;
	uint64_t offset;
	char url[OTA_URL_MAX_SIZE];
};

static struct ota_download_state download_state;
static size_t received_bytes;
static size_t committed_bytes;
static bool ota_file_open;

static int unlink_if_exists(const char *path)
{
	struct fs_dirent entry;
	int err = fs_stat(path, &entry);

	if (err == -ENOENT) {
		return 0;
	}
	if (err < 0) {
		return err;
	}
	return fs_unlink(path);
}

static int download_state_save(size_t offset)
{
	struct ota_download_state state = {
		.magic = OTA_STATE_MAGIC,
		.version = OTA_STATE_VERSION,
		.offset = offset,
	};
	struct fs_file_t state_file;
	ssize_t written;
	int err;

	memcpy(state.url, download_state.url, sizeof(state.url));
	fs_file_t_init(&state_file);
	err = fs_open(&state_file, OTA_STATE_TMP_PATH,
		      FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (err < 0) {
		LOG_ERR("Opening OTA download state failed: %d", err);
		return err;
	}

	written = fs_write(&state_file, &state, sizeof(state));
	if (written < 0) {
		err = written;
	} else if ((size_t)written != sizeof(state)) {
		err = -ENOSPC;
	} else {
		err = fs_sync(&state_file);
	}
	if (fs_close(&state_file) < 0 && err == 0) {
		err = -EIO;
	}
	if (err < 0) {
		(void)fs_unlink(OTA_STATE_TMP_PATH);
		return err;
	}

	return fs_rename(OTA_STATE_TMP_PATH, OTA_STATE_PATH);
}

static int download_state_load(struct ota_download_state *state)
{
	struct fs_file_t state_file;
	ssize_t bytes_read;
	int err;

	fs_file_t_init(&state_file);
	err = fs_open(&state_file, OTA_STATE_PATH, FS_O_READ);
	if (err < 0) {
		return err;
	}
	bytes_read = fs_read(&state_file, state, sizeof(*state));
	if (bytes_read < 0) {
		err = bytes_read;
	} else if ((size_t)bytes_read != sizeof(*state)) {
		err = -EBADMSG;
	} else {
		err = 0;
	}
	if (fs_close(&state_file) < 0 && err == 0) {
		err = -EIO;
	}
	if (err < 0) {
		return err;
	}
	if (state->magic != OTA_STATE_MAGIC || state->version != OTA_STATE_VERSION ||
	    state->url[0] == '\0' || memchr(state->url, '\0', sizeof(state->url)) == NULL) {
		return -EBADMSG;
	}
	return 0;
}

static int download_checkpoint(void)
{
	int err = fs_sync(&ota_file);

	if (err < 0) {
		return err;
	}
	err = fs_close(&ota_file);
	ota_file_open = false;
	if (err < 0) {
		return err;
	}
	err = download_state_save(received_bytes);
	if (err == 0) {
		committed_bytes = received_bytes;
	}
	if (err == 0) {
		err = fs_open(&ota_file, OTA_PARTIAL_PATH, FS_O_WRITE);
		if (err == 0) {
			ota_file_open = true;
			err = fs_seek(&ota_file, (off_t)received_bytes, FS_SEEK_SET);
		}
	}
	return err;
}

static int dl_callback(const struct downloader_evt *evt)
{
	ssize_t written;
	int err;

	switch (evt->id) {
	case DOWNLOADER_EVT_FRAGMENT:
		written = fs_write(&ota_file, evt->fragment.buf, evt->fragment.len);
		if (written < 0 || (size_t)written != evt->fragment.len) {
			err = written < 0 ? (int)written : -ENOSPC;
			LOG_ERR("fs_write failed: %d", err);
			dl_result = err;
			return 1; /* non-zero stops the download */
		}
		received_bytes += (size_t)written;
		if (received_bytes - committed_bytes >= OTA_CHECKPOINT_SIZE) {
			err = download_checkpoint();
			if (err < 0) {
				LOG_ERR("Download checkpoint failed: %d", err);
				dl_result = err;
				return 1;
			}
		}
		return 0;

	case DOWNLOADER_EVT_ERROR:
		LOG_ERR("Download error: %d", evt->error);
		dl_result = evt->error;
		if (received_bytes > committed_bytes) {
			err = download_checkpoint();
			if (err < 0) {
				LOG_WRN("Could not save final download checkpoint: %d", err);
			}
		}
		return 1; /* stop; zero would let the downloader reconnect and resume */

	case DOWNLOADER_EVT_DONE:
		dl_result = 0;
		k_sem_give(&dl_done);
		return 0;

	case DOWNLOADER_EVT_STOPPED:
		if (dl_result == 0) {
			dl_result = -ECANCELED;
		}
		k_sem_give(&dl_done);
		return 0;

	default:
		return 0;
	}
}

static int ota_bundle_download_from(const char *url, const int *sec_tags,
				    uint8_t sec_tag_count, size_t offset, bool fresh)
{
	struct downloader_cfg cfg = {
		.callback = dl_callback,
		.buf = dl_buf,
		.buf_size = sizeof(dl_buf),
	};
	struct downloader_host_cfg host_cfg = {
		.sec_tag_list = sec_tags,
		.sec_tag_count = sec_tag_count,
		.range_override = sizeof(dl_buf),
	};
	struct fs_statvfs sv;
	struct fs_dirent partial_stat;
	size_t total_size;
	int err;

	if (fresh) {
		size_t url_len = strlen(url);

		if (url_len == 0 || url_len >= sizeof(download_state.url)) {
			return -E2BIG;
		}
		err = unlink_if_exists(OTA_BUNDLE_PATH);
		if (err < 0) {
			return err;
		}
		err = unlink_if_exists(OTA_PARTIAL_PATH);
		if (err < 0) {
			return err;
		}
		err = unlink_if_exists(OTA_STATE_PATH);
		if (err < 0) {
			return err;
		}
		err = unlink_if_exists(OTA_STATE_TMP_PATH);
		if (err < 0) {
			return err;
		}
		memset(&download_state, 0, sizeof(download_state));
		memcpy(download_state.url, url, url_len + 1);
		offset = 0;
	} else {
		download_state = (struct ota_download_state){0};
		err = download_state_load(&download_state);
		if (err < 0) {
			LOG_ERR("Cannot load saved download state: %d", err);
			return err;
		}
		if (download_state.offset > SIZE_MAX) {
			return -EFBIG;
		}
		offset = (size_t)download_state.offset;
		if (fs_stat(OTA_PARTIAL_PATH, &partial_stat) < 0 ||
		    partial_stat.size < (off_t)offset) {
			LOG_ERR("Partial bundle is missing or shorter than its checkpoint");
			return -EBADMSG;
		}
	}

	received_bytes = offset;
	committed_bytes = offset;
	if (fresh) {
		err = download_state_save(0);
		if (err < 0) {
			return err;
		}
	}

	fs_file_t_init(&ota_file);
	err = fs_open(&ota_file, OTA_PARTIAL_PATH, FS_O_CREATE | FS_O_WRITE |
		      (fresh ? FS_O_TRUNC : 0));
	if (err < 0) {
		LOG_ERR("Opening partial bundle failed: %d", err);
		return err;
	}
	ota_file_open = true;
	err = fs_truncate(&ota_file, (off_t)offset);
	if (err == 0) {
		err = fs_seek(&ota_file, (off_t)offset, FS_SEEK_SET);
	}
	if (err < 0) {
		goto out_close;
	}

	err = downloader_init(&dl, &cfg);
	if (err) {
		LOG_ERR("downloader_init failed: %d", err);
		goto out_close;
	}

	k_sem_reset(&dl_done);
	dl_result = 0;

	err = downloader_get(&dl, &host_cfg, download_state.url, offset);
	if (err) {
		LOG_ERR("downloader_get failed: %d", err);
		goto out_deinit;
	}

	k_sem_take(&dl_done, K_FOREVER);
	err = dl_result;
	if (err == 0) {
		err = fs_sync(&ota_file);
	}
	if (err == 0) {
		err = downloader_file_size_get(&dl, &total_size);
	}
	if (err == 0 && received_bytes != total_size) {
		LOG_ERR("Incomplete download: received %zu of %zu bytes", received_bytes,
			total_size);
		err = -ENODATA;
	}

out_deinit:
	downloader_deinit(&dl);
out_close:
	if (err < 0 && ota_file_open && received_bytes > committed_bytes) {
		int checkpoint_err = download_checkpoint();

		if (checkpoint_err < 0) {
			LOG_WRN("Could not save final download checkpoint: %d", checkpoint_err);
		}
	}
	if (ota_file_open) {
		if (fs_sync(&ota_file) < 0 && err == 0) {
			err = -EIO;
		}
		if (fs_close(&ota_file) < 0 && err == 0) {
			err = -EIO;
		}
		ota_file_open = false;
	}

	if (err == 0) {
		err = fs_rename(OTA_PARTIAL_PATH, OTA_BUNDLE_PATH);
		if (err == 0) {
			(void)fs_unlink(OTA_STATE_PATH);
			(void)fs_unlink(OTA_STATE_TMP_PATH);
		}
	}
	if (err == 0 && fs_statvfs("/ota", &sv) == 0) {
		LOG_INF("Bundle stored, %lu of %lu blocks free", sv.f_bfree, sv.f_blocks);
	}

	return err;
}

int ota_bundle_download(const char *url, const int *sec_tags, uint8_t sec_tag_count)
{
	if (url == NULL) {
		return -EINVAL;
	}
	return ota_bundle_download_from(url, sec_tags, sec_tag_count, 0, true);
}

int ota_bundle_resume(const int *sec_tags, uint8_t sec_tag_count)
{
	struct fs_dirent entry;
	struct ota_download_state state;
	int err;

	if (fs_stat(OTA_BUNDLE_PATH, &entry) == 0) {
		LOG_INF("A complete bundle is already available");
		return -EALREADY;
	}
	err = download_state_load(&state);
	if (err < 0) {
		LOG_ERR("No resumable download state (%d)", err);
		return err;
	}
	return ota_bundle_download_from(state.url, sec_tags, sec_tag_count,
					(size_t)state.offset, false);
}

void ota_bundle_status(void)
{
	struct fs_dirent entry;
	struct ota_download_state state;

	if (fs_stat(OTA_BUNDLE_PATH, &entry) == 0) {
		LOG_INF("Complete OTA bundle available: %u bytes", (uint32_t)entry.size);
		return;
	}
	if (fs_stat(OTA_PARTIAL_PATH, &entry) == 0) {
		if (download_state_load(&state) == 0) {
			LOG_INF("Partial OTA bundle: %u bytes on disk, %llu bytes checkpointed; "
				"resume metadata valid", (uint32_t)entry.size,
				(unsigned long long)state.offset);
		} else {
			LOG_WRN("Partial OTA bundle present (%u bytes), but resume metadata is invalid",
				(uint32_t)entry.size);
		}
		return;
	}
	LOG_INF("No OTA bundle or partial download available");
}

int ota_bundle_delete(void)
{
	return unlink_if_exists(OTA_BUNDLE_PATH);
}

static int stage_nrf53_update(int image, off_t offset, size_t length)
{
	struct fs_file_t file;
	ssize_t bytes_read;
	int err;

	if (length < sizeof(uint32_t)) {
		return -EBADMSG;
	}
	fs_file_t_init(&file);
	err = fs_open(&file, OTA_BUNDLE_PATH, FS_O_READ);
	if (err < 0) {
		return err;
	}
	err = fs_seek(&file, offset, FS_SEEK_SET);
	if (err < 0) {
		goto out_close;
	}
	bytes_read = fs_read(&file, dl_buf, sizeof(uint32_t));
	if (bytes_read != sizeof(uint32_t) || !dfu_target_mcuboot_identify(dl_buf)) {
		LOG_ERR("nRF53 image %d is not a MCUboot image", image);
		err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
		goto out_close;
	}
	err = fs_seek(&file, offset, FS_SEEK_SET);
	if (err < 0) {
		goto out_close;
	}
	err = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (err < 0) {
		goto out_close;
	}
	err = dfu_target_mcuboot_init(length, image, NULL);
	if (err < 0) {
		goto out_close;
	}

	for (size_t remaining = length; remaining > 0;) {
		bytes_read = fs_read(&file, dl_buf, MIN(remaining, sizeof(dl_buf)));
		if (bytes_read <= 0) {
			err = bytes_read < 0 ? (int)bytes_read : -ENODATA;
			goto out_abort;
		}
		err = dfu_target_mcuboot_write(dl_buf, bytes_read);
		if (err < 0) {
			goto out_abort;
		}
		remaining -= bytes_read;
	}
	err = dfu_target_mcuboot_done(true);
	if (err == 0) {
		LOG_INF("nRF53 image %d staged: %zu bytes", image, length);
	}
	goto out_close;

out_abort:
	(void)dfu_target_mcuboot_done(false);
out_close:
	if (err) {
		fs_close(&file);
		return err;
	}
	return fs_close(&file);
}

static int apply_nrf91_update(enum sm_xdfu_image_type type, off_t offset, size_t length)
{
	const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	bool was_up;
	int err;
	int restore_err;

	if (iface == NULL) {
		return -ENODEV;
	}
	was_up = net_if_is_up(iface);
	if (was_up) {
		err = net_if_down(iface);
		if (err < 0) {
			return err;
		}
	}
	err = pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("Failed to suspend modem for XDFU: %d", err);
		goto restore_interface;
	}

	LOG_INF("Transferring nRF91 update type %d: offset %lld, size %zu",
		type, (long long)offset, length);
	err = sm_xdfu_run(modem, type, OTA_BUNDLE_PATH, offset, length);
	if (type == SM_XDFU_TYPE_FULL_MFW && err < 0) {
		LOG_ERR("Full-modem update failed; modem remains suspended for retry");
		return err;
	}
	if (err == 0) {
		LOG_INF("nRF91 update transfer complete");
	}

restore_interface:
	if (was_up) {
		restore_err = net_if_up(iface);
		if (restore_err < 0) {
			LOG_ERR("Failed to restore PPP interface: %d", restore_err);
			if (err == 0) {
				err = restore_err;
			}
		}
	}
	return err;
}

int ota_bundle_apply(void)
{
	uint8_t digest[32];
	uint8_t expected_digest[32];
	struct ota_bundle bundle = {0};
	psa_hash_operation_t sha_ctx = {0};
	size_t header_len = 0;
	size_t digest_len = 0;
	uint64_t payload_offset;
	uint32_t remaining;
	ssize_t bytes_read;
	int err;
	bool sha_initialized = false;
	bool nrf53_staged = false;
	bool primary_staged = false;

	fs_file_t_init(&ota_file);
	err = fs_open(&ota_file, OTA_BUNDLE_PATH, FS_O_READ);
	if (err) {
		LOG_ERR("fs_open failed: %d", err);
		return err;
	}

	bytes_read = fs_read(&ota_file, dl_buf, sizeof(dl_buf));
	if (bytes_read <= 0) {
		err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
		LOG_ERR("Failed to read OTA bundle header: %d", err);
		goto out_close;
	}

	err = cbor_decode_ota_bundle(dl_buf, (size_t)bytes_read, &bundle, &header_len);
	if (err != ZCBOR_SUCCESS || header_len == 0 || header_len > (size_t)bytes_read) {
		LOG_ERR("Invalid OTA bundle header: %d", err);
		err = -EBADMSG;
		goto out_close;
	}

	memcpy(expected_digest, bundle.bundle_sha256.value, sizeof(expected_digest));

	err = fs_seek(&ota_file, (off_t)header_len, FS_SEEK_SET);
	if (err) {
		LOG_ERR("Failed to seek to OTA payload: %d", err);
		goto out_close;
	}

	psa_status_t psa_err = psa_crypto_init();
	if (psa_err != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize PSA Crypto: %d", psa_err);
		err = -EIO;
		goto out_close;
	}
	psa_err = psa_hash_setup(&sha_ctx, PSA_ALG_SHA_256);
	if (psa_err != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize SHA-256: %d", psa_err);
		err = -EIO;
		goto out_close;
	}
	sha_initialized = true;

	remaining = bundle.bundle_size;
	while (remaining > 0) {
		size_t read_size = remaining < sizeof(dl_buf) ? remaining : sizeof(dl_buf);

		bytes_read = fs_read(&ota_file, dl_buf, read_size);
		if (bytes_read <= 0) {
			err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
			LOG_ERR("Failed to read OTA payload: %d", err);
			goto out_free_sha;
		}
		psa_err = psa_hash_update(&sha_ctx, dl_buf, (size_t)bytes_read);
		if (psa_err != PSA_SUCCESS) {
			LOG_ERR("Failed to update SHA-256: %d", psa_err);
			err = -EIO;
			goto out_free_sha;
		}
		remaining -= (uint32_t)bytes_read;
	}

	bytes_read = fs_read(&ota_file, dl_buf, 1);
	if (bytes_read != 0) {
		LOG_ERR("OTA bundle has trailing data or could not be read");
		err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
		goto out_free_sha;
	}

	psa_err = psa_hash_finish(&sha_ctx, digest, sizeof(digest), &digest_len);
	sha_initialized = false;
	if (psa_err != PSA_SUCCESS || digest_len != sizeof(digest)) {
		LOG_ERR("Failed to finish SHA-256: %d", psa_err);
		err = -EIO;
		goto out_free_sha;
	}
	if (memcmp(digest, expected_digest, sizeof(digest)) != 0) {
		LOG_ERR("OTA bundle SHA-256 mismatch");
		err = -EBADMSG;
		goto out_free_sha;
	}
	err = 0;

	payload_offset = header_len;
	LOG_INF("OTA bundle valid: %u payload bytes, %zu elements; SHA-256 verified",
		bundle.bundle_size, bundle.ota_element_m_count);
	for (size_t i = 0; i < bundle.ota_element_m_count; i++) {
		const struct ota_element *element = &bundle.ota_element_m[i];

		LOG_INF("  %s: offset %llu, size %u bytes", ota_target_name(element->target_choice),
			(unsigned long long)payload_offset, element->size);
		payload_offset += element->size;
	}

	LOG_INF("expected size: %llu, actual size: %llu",
		(unsigned long long)bundle.bundle_size + header_len,
		(unsigned long long)payload_offset);
	if (payload_offset != (uint64_t)bundle.bundle_size + header_len) {
		LOG_ERR("OTA bundle element sizes do not match bundle_size");
		err = -EBADMSG;
	}

out_free_sha:
	if (sha_initialized) {
		(void)psa_hash_abort(&sha_ctx);
	}
out_close:
	if (err) {
		fs_close(&ota_file);
		return err;
	}
	err = fs_close(&ota_file);
	if (err) {
		return err;
	}

	payload_offset = header_len;
	err = -ENOTSUP;
	for (size_t i = 0; i < bundle.ota_element_m_count; i++) {
		const struct ota_element *element = &bundle.ota_element_m[i];

		switch (element->target_choice) {
		case ota_element_target_nrf53_app_m_c:
		case ota_element_target_nrf53_net_m_c:
		case ota_element_target_nrf53_bl_m_c:
			if (element->target_choice != ota_element_target_nrf53_net_m_c &&
			    primary_staged) {
				LOG_ERR("nRF53 app/bootloader staging partition is already primed");
				return -EBUSY;
			}
			err = stage_nrf53_update(
				element->target_choice == ota_element_target_nrf53_net_m_c ? 1 : 0,
				(off_t)payload_offset, element->size);
			if (err) {
				return err;
			}
			nrf53_staged = true;
			if (element->target_choice != ota_element_target_nrf53_net_m_c) {
				primary_staged = true;
			}
			break;
		case ota_element_target_nrf91_app_m_c:
			err = apply_nrf91_update(SM_XDFU_TYPE_APP, (off_t)payload_offset, element->size);
			if (err) {
				return err;
			}
			break;
		case ota_element_target_nrf91_mdm_delta_m_c:
			err = apply_nrf91_update(SM_XDFU_TYPE_DELTA_MFW, (off_t)payload_offset,
					 element->size);
			if (err) {
				return err;
			}
			break;
		case ota_element_target_nrf91_mdm_full_m_c:
			err = apply_nrf91_update(SM_XDFU_TYPE_FULL_MFW, (off_t)payload_offset,
					 element->size);
			if (err) {
				return err;
			}
			break;
		default:
			LOG_WRN("Not applying %s: target not implemented",
				ota_target_name(element->target_choice));
			break;
		}
		payload_offset += element->size;
	}
	if (nrf53_staged) {
		for (size_t i = 0; i < bundle.ota_element_m_count; i++) {
			const struct ota_element *element = &bundle.ota_element_m[i];

			if (element->target_choice != ota_element_target_nrf53_app_m_c &&
			    element->target_choice != ota_element_target_nrf53_net_m_c &&
			    element->target_choice != ota_element_target_nrf53_bl_m_c) {
				continue;
			}
			err = dfu_target_mcuboot_schedule_update(
				element->target_choice == ota_element_target_nrf53_net_m_c ? 1 : 0);
			if (err) {
				return err;
			}
		}
		LOG_INF("nRF53 updates scheduled");
	}
	return err;
}

void modem_hard_reset(void)
{
	int ret;

	LOG_WRN("Hard resetting modem and cold rebooting host");
	if (!gpio_is_ready_dt(&modem_reset)) {
		LOG_ERR("Modem reset GPIO is not ready");
	} else {
		ret = gpio_pin_configure_dt(&modem_reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Cannot assert modem reset (%d)", ret);
		} else {
			k_sleep(K_MSEC(100));

			ret = gpio_pin_set_dt(&modem_reset, 0);
			if (ret < 0) {
				LOG_ERR("Cannot release modem reset (%d)", ret);
			}

			ret = gpio_pin_configure_dt(&modem_reset, GPIO_INPUT);
			if (ret < 0) {
				LOG_ERR("Cannot return modem reset GPIO to input (%d)", ret);
			}
		}
	}

	sys_reboot(SYS_REBOOT_COLD);
}