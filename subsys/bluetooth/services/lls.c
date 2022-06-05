
/** @file
 *  @brief Link Loss Service
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/lls.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_nrf_lls, CONFIG_BT_NRF_LLS_LOG_LEVEL);

static volatile uint8_t alert_level = 0;


static ssize_t write_alert(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   const void *buf,
			   uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}


	uint8_t val = *((uint8_t *)buf);
	alert_level = val;


	return val;
}


/* Link Loss Service Declaration */
BT_GATT_SERVICE_DEFINE(lls_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_LLS),

		       BT_GATT_CHARACTERISTIC(BT_UUID_ALERT_LEVEL,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE,
					      NULL, write_alert, NULL),
		       );

int bt_lls_init()
{

	alert_level = 0;
	return 0;
}
uint8_t bt_lls_alert_level_get()
{
	return alert_level;
}
