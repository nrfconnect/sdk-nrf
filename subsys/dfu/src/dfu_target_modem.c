#include <zephyr.h>
#include <stdio.h>
#include <drivers/flash.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(dfu_target_modem, CONFIG_DFU_TARGET_LOG_LEVEL);

#define DIRTY_IMAGE 0x280000
#define MODEM_MAGIC 0x7544656d

struct modem_delta_header {
	u16_t pad1;
	u16_t pad2;
	u32_t magic;
};

static int  fd;
static int  offset;

static int get_modem_error(void)
{
	int rc;
	int err = 0;
	socklen_t len;

	len = sizeof(err);
	rc = getsockopt(fd, SOL_DFU, SO_DFU_ERROR, &err, &len);
	if (rc) {
		LOG_ERR("Unable to fetch modem error, errno %d", errno);
	}

	return err;
}

static int apply_modem_upgrade(void)
{
	int err;

	LOG_INF("Scheduling modem firmware upgrade at next boot");

	err = setsockopt(fd, SOL_DFU, SO_DFU_APPLY, NULL, 0);
	if (err < 0) {
		if (errno == ENOEXEC) {
			LOG_ERR("SO_DFU_APPLY failed, modem error %d",
				get_modem_error());
		} else {
			LOG_ERR("SO_DFU_APPLY failed, modem error %d", err);
		}
	}
	return 0;
}

static int delete_banked_modem_fw(void)
{
	int err;
	socklen_t len = sizeof(offset);

	LOG_INF("Deleting firmware image, this can take several minutes");
	err = setsockopt(fd, SOL_DFU, SO_DFU_BACKUP_DELETE, NULL, 0);
	if (err < 0) {
		LOG_ERR("Failed to delete backup, errno %d", errno);
		return -EFAULT;
	}

	while (true) {
		err = getsockopt(fd, SOL_DFU, SO_DFU_OFFSET, &offset, &len);
		if (err < 0) {
			if (errno == ENOEXEC) {
				err = get_modem_error();
				if (err != DFU_ERASE_PENDING) {
					LOG_ERR("DFU error: %d", err);
				}
				k_sleep(K_MSEC(500));
			}
		} else {
			LOG_INF("Modem FW delete complete");
			break;
		}
	}

	return 0;
}

/**@brief Initialize DFU socket. */
static int modem_dfu_socket_init(void)
{
	int err;
	socklen_t len;
	u8_t version[36];
	char version_string[37];

	/* Create a socket for firmware upgrade*/
	fd = socket(AF_LOCAL, SOCK_STREAM, NPROTO_DFU);
	if (fd < 0) {
		LOG_ERR("Failed to open Modem DFU socket.");
		return fd;
	}

	LOG_INF("Modem DFU Socket created");

	len = sizeof(version);
	err = getsockopt(fd, SOL_DFU, SO_DFU_FW_VERSION, &version,
			    &len);
	if (err < 0) {
		LOG_ERR("Firmware version request failed, errno %d", errno);
		return -1;
	}

	snprintf(version_string, sizeof(version_string), "%.*s",
		 sizeof(version), version);

	LOG_INF("Modem firmware version: %s", log_strdup(version_string));

	return err;
}

bool dfu_target_modem_identify(const void *const buf)
{
	return ((const struct modem_delta_header *)buf)->magic == MODEM_MAGIC;

}

int dfu_target_modem_init(size_t file_size)
{
	/* We have no way of checking the amount of flash available in the modem
	 * with the current API
	 */
	ARG_UNUSED(file_size);
	int err;
	socklen_t len = sizeof(offset);

	err = modem_dfu_socket_init();
	if (err < 0) {
		return err;
	}

	/* Check offset, store to local variable */
	err = getsockopt(fd, SOL_DFU, SO_DFU_OFFSET, &offset, &len);
	if (err < 0) {
		if (errno == ENOEXEC) {
			LOG_ERR("Modem error: %d", get_modem_error());
		} else {
			LOG_ERR("getsockopt(OFFSET) errno: %d", errno);
		}
	}

	if (offset == DIRTY_IMAGE) {
		delete_banked_modem_fw();
	} else if (offset != 0) {
		LOG_INF("Setting offset to 0x%x", offset);
		len = sizeof(offset);
		err = setsockopt(fd, SOL_DFU, SO_DFU_OFFSET, &offset, len);
		if (err != 0) {
			LOG_INF("Error while setting offset: %d", offset);
		}
	}

	return 0;
}

int dfu_target_modem_offset_get(size_t *out)
{
	*out = offset;
	return 0;
}

int dfu_target_modem_write(const void *const buf, size_t len)
{
	int err = 0;
	int sent = 0;
	int modem_error = 0;

	sent = send(fd, buf, len, 0);
	if (sent > 0) {
		offset += len;
		return 0;
	}

	if (errno != ENOEXEC) {
		return -EFAULT;
	}

	modem_error = get_modem_error();
	LOG_ERR("send failed, modem errno %d, dfu err %d", errno, modem_error);
	switch (modem_error) {
	case DFU_INVALID_UUID:
		return -EINVAL;
	case DFU_INVALID_FILE_OFFSET:
		delete_banked_modem_fw();
		err = dfu_target_modem_write(buf, len);
		if (err < 0) {
			return -EINVAL;
		} else {
			return 0;
		}
	case DFU_AREA_NOT_BLANK:
		delete_banked_modem_fw();
		err = dfu_target_modem_write(buf, len);
		if (err < 0) {
			return -EINVAL;
		} else {
			return 0;
		}
	default:
		return -EFAULT;
	}
}

int dfu_target_modem_done(bool successful)
{
	int err = 0;

	if (successful) {
		err = apply_modem_upgrade();
		if (err < 0) {
			LOG_ERR("Failed request modem DFU upgrade");
			return err;
		}
	} else {
		LOG_INF("Modem upgrade aborted.");
	}


	err = close(fd);
	if (err < 0) {
		LOG_ERR("Failed to close modem DFU socket.");
		return err;
	}

	return 0;
}
