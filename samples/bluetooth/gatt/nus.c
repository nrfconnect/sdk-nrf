/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/conn.h>                                                                                                            
#include <bluetooth/uuid.h>                                                                                                            
#include <bluetooth/gatt.h> 

#include <../gatt/nus.h>

static struct bt_gatt_ccc_cfg nuslc_ccc_cfg[BT_GATT_CCC_MAX];
static bool                   notify_enabled;

static nus_receive_cb_t receive_cb;
static nus_send_cb_t    send_cb;
                                                                                                                                       
#define BT_UUID_NUS_SERVICE   BT_UUID_DECLARE_128(NUS_UUID_SERVICE)
#define BT_UUID_NUS_RX        BT_UUID_DECLARE_128(NUS_UUID_NUS_RX_CHAR)
#define BT_UUID_NUS_TX        BT_UUID_DECLARE_128(NUS_UUID_NUS_TX_CHAR)

static void nuslc_ccc_cfg_changed(const struct bt_gatt_attr * attr,
                                 u16_t value)
{                                                                                                                                      
        notify_enabled = (value & BT_GATT_CCC_NOTIFY) ? 1 : 0;
}
                                                                                                                                       
static ssize_t on_receive(struct bt_conn * conn, const struct bt_gatt_attr * attr,
                          const void * buf, u16_t len, u16_t offset,
                          u8_t flags)                                                                                                    
{
        if (receive_cb) {
                receive_cb((u8_t *)buf, len);
        }

        return len;
}

static ssize_t on_send(struct bt_conn * conn,
                       const struct bt_gatt_attr * attr,
                       void * buf, u16_t len,
                       u16_t offset)
{
        if (send_cb) {
                send_cb((u8_t *)buf, len);
        }

        return len;
}


/* UART Service Declaration */
static struct bt_gatt_attr attrs[] = {
        BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),
        BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX,
                               BT_GATT_CHRC_NOTIFY,
                               BT_GATT_PERM_READ,
                               on_send, NULL, NULL),
        BT_GATT_CCC(nuslc_ccc_cfg, nuslc_ccc_cfg_changed),                                       
        BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX,
                               BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, on_receive, NULL),
};

static struct bt_gatt_service nus_svc = BT_GATT_SERVICE(attrs);

int nus_init(nus_receive_cb_t nus_receive_cb, nus_send_cb_t nus_send_cb)
{
        receive_cb = nus_receive_cb;
        send_cb    = nus_send_cb;

        return bt_gatt_service_register(&nus_svc);
}

int nus_data_send(char * data, uint8_t len)
{
        if (!notify_enabled) {
                return -EFAULT;
        }

        return bt_gatt_notify(NULL, &attrs[2], data, len);
}


