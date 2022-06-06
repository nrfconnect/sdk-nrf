

/** @file
 *  @brief Immediate Alert Service
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

#include <bluetooth/services/ias.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_nrf_ias, CONFIG_BT_NRF_IAS_LOG_LEVEL);
typedef void (*ble_ias_evt_handler_t) (uint8_t para);
static volatile uint8_t alert_level = 0;

static ble_ias_evt_handler_t ias_cb;


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
	ias_cb(alert_level);

	return val;
}



/* Immediate Alert Service Declaration */
BT_GATT_SERVICE_DEFINE(ias_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_IAS),

		       BT_GATT_CHARACTERISTIC(BT_UUID_ALERT_LEVEL,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE,
					      NULL, write_alert, NULL),
		       );

int bt_ias_init(ble_ias_evt_handler_t callback)
{
	if (callback) {
		ias_cb = callback;
	}

	return 0;
}
