
#include <logging/log.h>
#include <zephyr/types.h>
#include <flash.h>
#include <net/http_get_file.h>

LOG_MODULE_REGISTER(main);

#define FLASH_OFFSET		0x56000
#define FLASH_PAGE_SIZE		4096

static K_THREAD_STACK_DEFINE(http_thread_stack, 1024);

static struct k_thread http_thread;

/**< The server that hosts the firmwares. */
const char *host = "download.nrfcloud.com";
const char *filename = "8041c7f5-3ea2-4463-9f98-82a089d92b13";

static char http_req_buf[256];
static char http_resp_buf[1024];
static struct device *flash_dev;
static int len_tot;

static void flash_write_callback(char *buf, int len)
{
	static u32_t addr = FLASH_OFFSET;
	int err;

	/* Erase page(s) here: */

	err = flash_write(flash_dev, addr, buf, len);
	if (err != 0) {
		LOG_ERR("Flash write error %d at address %08x\n", err, addr);
		return;
	}
	len_tot += len;
	addr += len;
}

static void http_worker_thread(void *p1, void *p2, void *p3)
{
	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);

	if (!flash_dev) {
		LOG_ERR("Nordic nRF5 flash driver was not found!\n");
		return;
	}
	len_tot = 0;
	struct get_file_param file_param = {
		.host = host,
		.port = NULL,
		.filename = filename,
		.req_buf = http_req_buf,
		.req_buf_len = sizeof(http_req_buf),
		.resp_buf = http_resp_buf,
		.resp_buf_len = sizeof(http_resp_buf),
		.callback = flash_write_callback
	};
	flash_write_protection_set(flash_dev, false);
	http_get_file(&file_param);
	flash_write_protection_set(flash_dev, true);

	LOG_DBG("Received %d bytes\n", len_tot);
}


void main(void)
{
	k_thread_create(&http_thread, http_thread_stack,
			K_THREAD_STACK_SIZEOF(http_thread_stack),
			http_worker_thread,	NULL, NULL, NULL,
			K_PRIO_COOP(7), 0, K_NO_WAIT);
}
