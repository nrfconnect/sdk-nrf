/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <zephyr.h>
#include <bsd.h>
#include <at_cmd.h>
#include <lte_lc.h>
#include <nrf_socket.h>
#include <net/bsdlib.h>
#include <net/socket.h>
#include <net/download_client.h>
#include <secure_services.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <shell/shell.h>

LOG_MODULE_REGISTER(app);

/* A default URL can be added here */
#define DEFAULT_URL ""
#define SECURITY_TAG 0x7A

/* Socket fd used for modem DFU. */
static int fd;
static u32_t offset;
/* Send downloaded fragments to modem as a firmware update */
static bool pipe_fragments;
static struct download_client dfu;
static struct k_sem bsdlib_sem;
static struct k_delayed_work delete_dwork;

/* Neither are buffered by download_client */
static char host[128];
static char file[256];


extern int cert_provision(void);
extern int cert_delete(void);
static int download_client_callback(const struct download_client_evt *);


void bsd_recoverable_error_handler(uint32_t err)
{
	printk("%s: unexpected modem fault: err %u\n", __func__, err);
}

void bsd_irrecoverable_error_handler(uint32_t err)
{
	printk("%s: unexpected modem fault: err %u\n", __func__, err);
}

static nrf_dfu_err_t modem_error_get(void)
{
	int rc;
	nrf_dfu_err_t err;
	nrf_socklen_t len;

	len = sizeof(err);
	rc = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_ERROR, &err, &len);
	if (rc) {
		LOG_ERR("Unable to fetch modem error, errno %d", errno);
	}

	return err;
}

/* Print a message on the console when the firmware image has been deleted. */
static void delete_task(struct k_work *w)
{
	int rc;
	u32_t off;
	nrf_socklen_t len;

	len = sizeof(off);
	rc = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_OFFSET, &off, &len);
	if (rc < 0) {
		k_delayed_work_submit(&delete_dwork, K_SECONDS(1));
	} else {
		printk("Firmware image deleted\n");
	}
}

static int bsdlib_sem_give(const struct shell *shell, size_t argc, char **argv)
{
	/* Give a semaphore and let the main thread run bsdlib_init(),
	 * which would otherwise overflow the stack of the shell thread.
	 */
	k_sem_give(&bsdlib_sem);

	return 0;
}

static int modem_connect(const struct shell *shell, size_t argc, char **argv)
{
	printk("Connecting..\n");
	at_cmd_init();
	lte_lc_init_and_connect();

	return 0;
}

static int fw_offsets(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	u32_t off;
	nrf_socklen_t optlen;

	optlen = sizeof(off);

	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_OFFSET, &off, &optlen);
	if (err) {
		printk("Offset request failed, err %d\n", errno);
		return 0;
	}

	printk("Offset retrieved: %u\n", off);

	err = nrf_setsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_OFFSET, &off,
			    sizeof(off));
	if (err) {
		printk("Failed to reset offset, err %d\n", errno);
		return 0;
	}

	offset = off;
	printk("Offset set to %u.\n", offset);

	return 0;
}

static int fw_version(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	nrf_socklen_t len;
	nrf_dfu_fw_version_t version;

	len = sizeof(version);

	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_FW_VERSION, &version,
			    &len);
	if (err) {
		printk("Firmware version request failed, err %d\n", errno);
		return 0;
	}

	/* printk() does not support %.*s format */
	printf("\nModem firmware version: %.*s\n", sizeof(version), version);

	return 0;
}

static int fw_flash_size(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	nrf_socklen_t len;
	nrf_dfu_resources_t flash;

	len = sizeof(flash);
	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_RESOURCES, &flash,
			    &len);
	if (err) {
		printk("Resource request failed, err %d\n", errno);
		return 0;
	}

	printk("Flash memory available: %u\n", flash);

	return 0;
}

static int fw_update(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	printk("Scheduling firmware update at next boot\n");
	err = nrf_setsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_APPLY, NULL, 0);
	if (err) {
		printk("Failed to schedule update, err %d\n", errno);
	}

	return 0;
}

static int fw_revert(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	printk("Scheduling firmware rollback at next boot\n");
	err = nrf_setsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_REVERT, NULL, 0);
	if (err) {
		printk("Failed to schedule rollback, err %d\n", errno);
	}

	return 0;
}

static int fw_delete(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	printk("Deleting firmware image, please wait...\n");
	err = nrf_setsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_BACKUP_DELETE, NULL, 0);
	if (err) {
		printk("Failed to delete backup, errno %d\n", errno);
		return 0;
	}

	/* Reset offset used for downloading */
	offset = 0;

	/* Check when the operation has completed */
	k_delayed_work_submit(&delete_dwork, K_SECONDS(1));

	return 0;
}

static int dfusock_connect(const struct shell *shell, size_t argc, char **argv)
{
	printk("Creating DFU socket..\n");

	/* Create a socket for firmware update */
	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_STREAM, NRF_PROTO_DFU);
	if (fd < 0) {
		printk("Failed to open DFU socket, err %d\n", errno);
	}

	return 0;
}

static int dfusock_close(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	printk("Closing DFU socket..\n");

	err = close(fd);
	if (err) {
		printk("Failed to close DFU socket, err %d\n", errno);
	}

	return 0;
}

static int httpsock_connect(const struct shell *shell, size_t argc, char **argv)
{
	struct download_client_cfg config = {
		.sec_tag = SECURITY_TAG,
	};

	download_client_connect(&dfu, host, &config);

	return 0;
}

static int httpsock_close(const struct shell *shell, size_t argc, char **argv)
{
	printk("Closing HTTP socket\n");
	download_client_disconnect(&dfu);

	return 0;
}

static int download_file(const char *url)
{
	int err;
	struct download_client_cfg config = {
		.sec_tag = SECURITY_TAG,
	};

	/* Naive extraction of host and filename */
	err = sscanf(url, "%*[^/]%*[/]%[^/]/%s", host, file);
	if (err != 2 /* items filled by sscanf */) {
		printk("Failed to parse URL\n");
		return -1;
	}

	err = download_client_connect(&dfu, host, &config);
	if (err) {
		return -1;
	}

	err = download_client_start(&dfu, file, offset);
	if (err) {
		return -1;
	}

	return 0;
}

static int file_download(const struct shell *shell, size_t argc, char **argv)
{
	/* Do not send the file to the modem, just download it */
	pipe_fragments = false;
	if (argc < 2) {
		/* Download default file */
		download_file(DEFAULT_URL);
	} else {
		download_file(argv[1]);
	}

	return 0;
}

static int fw_download(const struct shell *shell, size_t argc, char **argv)
{
	/* Send fragments to modem */
	pipe_fragments = true;
	if (argc < 2) {
		/* Download default file */
		download_file(DEFAULT_URL);
	} else {
		download_file(argv[1]);
	}

	return 0;
}

static int pause(const struct shell *shell, size_t argc, char **argv)
{
	printk("Pause download\n");
	download_client_pause(&dfu);

	return 0;
}

static int resume(const struct shell *shell, size_t argc, char **argv)
{
	printk("Resume download\n");
	download_client_resume(&dfu);

	return 0;
}

static int reboot(const struct shell *shell, size_t argc, char **argv)
{
	printk("Rebooting..\n");

	log_panic();

	/* Let the user read the last messages */
	k_sleep(K_SECONDS(1));
	spm_request_system_reboot();

	CODE_UNREACHABLE;
	return 0;
}

static int cert_provision_cmd(const struct shell *shell, size_t argc,
			      char **argv)
{
	cert_provision();
	return 0;
}

static int cert_delete_cmd(const struct shell *shell, size_t argc, char **argv)
{
	cert_delete();
	return 0;
}

/**@brief Initialize DFU socket. */
static int dfu_socket_init(void)
{
	int err;
	nrf_socklen_t len;
	nrf_dfu_fw_offset_t off;
	nrf_dfu_resources_t resource;
	nrf_dfu_fw_version_t version;

	/* Create a socket for firmware update */
	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_STREAM, NRF_PROTO_DFU);
	if (fd < 0) {
		printk("Failed to open DFU socket.\n");
		return -1;
	}

	printk("Socket created\n");

	len = sizeof(version);
	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_FW_VERSION, &version,
			    &len);
	if (err) {
		printk("Firmware version request failed, errno %d\n", errno);
		return -1;
	}

	/* printk() does not support %.*s format */
	printf("\nModem firmware version: %.*s\n", sizeof(version), version);

	len = sizeof(resource);
	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_RESOURCES, &resource,
			    &len);
	if (err) {
		printk("Resource request failed, errno %d\n", errno);
		return -1;
	}

	printk("Flash memory available: %u\n", resource);

	len = sizeof(off);
	err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_OFFSET, &off, &len);
	if (err) {
		printk("Offset request failed, errno %d\n", errno);
		return -1;
	}

	offset = off;
	printk("Offset retrieved: %u\n", offset);

	return err;
}

static int on_fragment(const struct fragment *frag)
{
	int rc;

	printk("Sending fragment (%u) to modem..\n", frag->len);
	rc = send(fd, frag->buf, frag->len, 0);
	if (rc > 0) {
		/* Fragment accepted */
		return 0;
	}

	printk("Modem refused fragment, errno %d, dfu err %d\n",
		errno, modem_error_get());

	download_client_disconnect(&dfu);
	printk("Socket closed\n");

	return -1;
}

static int download_client_callback(const struct download_client_evt *event)
{
	int err;

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		if (!pipe_fragments) {
			/* Do nothing */
			break;
		}

		err = on_fragment(&event->fragment);
		if (err) {
			/* Stop download on error */
			return -1;
		}
		break;
	}
	case DOWNLOAD_CLIENT_EVT_DONE: {
		printk("Download completed\n");
		err = download_client_disconnect(&dfu);
		if (!err) {
			printk("Socket closed\n");
		}
		break;
	}
	case DOWNLOAD_CLIENT_EVT_ERROR: {
		printk("Download error %d\n", event->error);
		err = download_client_disconnect(&dfu);
		if (!err) {
			printk("Socket closed\n");
		}
		break;
	}
	default:
		break;
	}

	return 0;
}

int main(void)
{
	int err;

	k_sem_init(&bsdlib_sem, 0, 1);
	k_delayed_work_init(&delete_dwork, delete_task);

	printk("Use `bsdinit` to initialize communication with the modem.\n");
	printk("When the modem firmware is being updated, "
	       "the operation can take up to a minute\n");

	/* Wait for use to manually request to initialize bsdlib */
	k_sem_take(&bsdlib_sem, K_FOREVER);

	printk("Initializing bsdlib, please wait..\n");
	err = bsdlib_init();
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;

	default:
		break;
	}

	err = cert_provision();
	if (err) {
		return -1;
	}

	printk("Initializing DFU socket\n");
	err = dfu_socket_init();
	if (err) {
		return -1;
	}

	download_client_init(&dfu, download_client_callback);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(fw_cmd,
	SHELL_CMD(offset, NULL, "Update modem firmware offset", fw_offsets),
	SHELL_CMD(version, NULL, "Request modem firmware version", fw_version),
	SHELL_CMD(flash, NULL, "Request modem resources", fw_flash_size),
	SHELL_CMD(update, NULL, "Update modem firmware", fw_update),
	SHELL_CMD(revert, NULL, "Rollback modem firmware", fw_revert),
	SHELL_CMD(delete, NULL, "Delete modem firmware", fw_delete),
	SHELL_CMD(dl, NULL, "Download modem firmware", fw_download),
	SHELL_SUBCMD_SET_END,
);

SHELL_STATIC_SUBCMD_SET_CREATE(dfusock_cmd,
	SHELL_CMD(connect, NULL, "connect() DFU socket", dfusock_connect),
	SHELL_CMD(close, NULL, "close() DFU socket", dfusock_close),
	SHELL_SUBCMD_SET_END,
);

SHELL_STATIC_SUBCMD_SET_CREATE(httpsock_cmd,
	SHELL_CMD(connect, NULL, "connect() HTTP socket", httpsock_connect),
	SHELL_CMD(close, NULL, "close() HTTP socket", httpsock_close),
	SHELL_SUBCMD_SET_END,
);

SHELL_STATIC_SUBCMD_SET_CREATE(cert_cmd,
	SHELL_CMD(provision, NULL, "Provision certs", cert_provision_cmd),
	SHELL_CMD(delete, NULL, "Delete certs", cert_delete_cmd),
	SHELL_SUBCMD_SET_END,
);

SHELL_CMD_REGISTER(fw, &fw_cmd, "Modem DFU commands", NULL);
SHELL_CMD_REGISTER(dfusock, &dfusock_cmd, "Manipulate DFU socket", NULL);
SHELL_CMD_REGISTER(httpsock, &httpsock_cmd, "Manipulate HTTP socket", NULL);
SHELL_CMD_REGISTER(cert, &cert_cmd, "Certificate handling", NULL);

SHELL_CMD_REGISTER(bsdinit, NULL, "Initialize bsdlib", bsdlib_sem_give);
SHELL_CMD_REGISTER(linkon, NULL, "Connect", modem_connect);
SHELL_CMD_REGISTER(dl, NULL, "Download a file", file_download);
SHELL_CMD_REGISTER(pause, NULL, "Pause download", pause);
SHELL_CMD_REGISTER(resume, NULL, "Resume download", resume);
SHELL_CMD_REGISTER(reboot, NULL, "Reboot", reboot);
