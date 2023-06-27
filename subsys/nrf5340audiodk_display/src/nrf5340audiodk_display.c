#include <nrf5340audiodk_display/nrf5340audiodk_display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
#include <stdint.h>
#include "nrf5340audiodk_display_internal.h"
LOG_MODULE_REGISTER(app);

int test;
enum button_action {
	BUTTON_PRESS,
	BUTTON_ACTION_NUM,
};
struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};
    
ZBUS_CHAN_DECLARE(button_chan);



void create_timestamp_label(lv_obj_t *current_screen)
{
    static uint32_t count;
    char count_str[11] = {0};
    lv_obj_t *count_label;
    count_label = lv_label_create(current_screen);
    lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    while (1) {
        if ((count % 100) == 0U) {
            sprintf(count_str, "%d", count/100U);
            lv_label_set_text(count_label, count_str);
        }
        lv_task_handler();
        ++count;
        k_sleep(K_MSEC(10));
    }
}


void display_init()
{
    lv_obj_t *tabview;
    tabview=lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);


    lv_obj_t * button_tab = lv_tabview_add_tab(tabview, "Button Tab");
    lv_obj_t * log_tab = lv_tabview_add_tab(tabview, "Log Tab");
    lv_obj_t * state_tab = lv_tabview_add_tab(tabview, "State Tab");

    const struct device *display_dev;
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready, aborting test");
        return;
    }
    create_buttontab(button_tab);

    display_blanking_off(display_dev);

    while(1){
        lv_task_handler();
        k_sleep(K_MSEC(16));
}

        
}