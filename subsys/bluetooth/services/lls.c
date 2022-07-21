/** @file
 *  @brief Link Loss Service
 */

#include <errno.h>
#include <init.h>
#include <stdint.h>
#include <zephyr.h>
#include <net/buf.h>
#include <logging/log.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>



#include <bluetooth/services/lls.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_nrf_lls, CONFIG_BT_NRF_LLS_LOG_LEVEL);

#define BT_LLS_ALERT_LVL_LEN 1


enum bt_lls_alert_lvl {
	BT_LLS_ALERT_LVL_NO_ALERT,
	BT_LLS_ALERT_LVL_MILD_ALERT,
	BT_LLS_ALERT_LVL_HIGH_ALERT,
};

struct alert_device {
	enum bt_lls_alert_lvl alert_level;
};

static struct alert_device devices[CONFIG_BT_MAX_CONN];

static struct bt_lls_cb *lls_callbacks;

static void set_alert_level(void)
{
	enum bt_lls_alert_lvl alert_level;

	alert_level = devices[0].alert_level;
	for (int i = 1; i < CONFIG_BT_MAX_CONN; i++) {
		if (alert_level < devices[i].alert_level) {
			alert_level = devices[i].alert_level;
		}
	}


	if (alert_level == BT_LLS_ALERT_LVL_HIGH_ALERT) {
		if (lls_callbacks->high_alert) {
			lls_callbacks->high_alert();
		}
		LOG_DBG("High alert");
	} else if (alert_level == BT_LLS_ALERT_LVL_MILD_ALERT) {
		if (lls_callbacks->mild_alert) {
			lls_callbacks->mild_alert();
		}
		LOG_DBG("Mild alert");
	} else {
		if (lls_callbacks->no_alert) {
			lls_callbacks->no_alert();
		}
		LOG_DBG("No alert");
	}

}



static ssize_t write_alert(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple data;
	enum bt_lls_alert_lvl alert_val;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_LLS_ALERT_LVL_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&data, (void *)buf, len);
	alert_val = net_buf_simple_pull_u8(&data);
	devices[bt_conn_index(conn)].alert_level = alert_val;

	if (alert_val < BT_LLS_ALERT_LVL_NO_ALERT || alert_val > BT_LLS_ALERT_LVL_HIGH_ALERT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	return len;
}

/* Link Loss Service Declaration */
BT_GATT_SERVICE_DEFINE(lls_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_LLS),

		       BT_GATT_CHARACTERISTIC(BT_UUID_ALERT_LEVEL,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE,
					      NULL, write_alert, NULL),
		       );

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {

		return;
	}

	devices[bt_conn_index(conn)].alert_level = BT_LLS_ALERT_LVL_NO_ALERT;

	set_alert_level();
}
static void disconnected(struct bt_conn *conn, uint8_t reason)
{

	set_alert_level();
}

struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void bt_lls_init(struct bt_lls_cb *lls_cb)
{
	if (lls_cb) {
		lls_callbacks = lls_cb;
	}

}
