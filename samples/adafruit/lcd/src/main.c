/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lv_tutorials.h"
#include "ili9341_lcd.h"
#include "touch_ft6206.h"


/* a semaphore that stores 3 timer ticks */
K_SEM_DEFINE(lvgl_sem, 0, 3);


/* Flush the content of the internal buffer the specific area on the display
 * You can use DMA or any hardware acceleration to do this operation in the background but
 * 'lv_flush_ready()' has to be called when finished
 * This function is required only when LV_VDB_SIZE != 0 in lv_conf.h*/
static void ex_disp_flush(s32_t x1, s32_t y1, s32_t x2, s32_t y2, const lv_color_t *color_p)
{
	/* Send LCD window data to ILI9341 */
	ili9341_lcd_put_gfx(x1, y1, x2 - x1 + 1, y2 - y1 + 1, (const uint8_t *) color_p);

	/* IMPORTANT!!!
	 * Inform the graphics library that you are ready with the flushing*/
	lv_flush_ready();
}

/* Read the touchpad and store it in 'data'
 * Return false if no more data read; true for ready again */
static bool ex_tp_read(lv_indev_data_t *data)
{
	/* Read your touchpad */

	struct ft6206_pos buffer = touch_ft6206_get();
	static int prev_x;
	static int prev_y;

	data->state = (buffer.z) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

	if (data->state == LV_INDEV_STATE_PR) {
		data->point.x = prev_x = buffer.x;
		data->point.y = prev_y = buffer.y;
	} else {
		/* IMPORTANT NOTE:
		 * Touchpad drivers must return the last X/Y coordinates
		 * even when the state is LV_INDEV_STATE_REL.            */
		data->point.x = prev_x;
		data->point.y = prev_y;
	}

	/*false: no more data to read because we are no buffering*/
	return false;
}

static void lvgl_timer_function(struct k_timer *timer)
{
	k_sem_give(&lvgl_sem);
}

int main(void)
{
	ili9341_lcd_init();

	ili9341_lcd_on();

	touch_ft6206_init();

	struct k_timer lvgl_timer;

	k_timer_init(&lvgl_timer, lvgl_timer_function, NULL);
	k_timer_start(&lvgl_timer, K_MSEC(10), K_MSEC(10));

	/***********************
	 * Initialize LittlevGL
	 ***********************/
	lv_init();

	/***********************
	 * Display interface
	 ***********************/
	lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/

	lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

	/*Set up the functions to access to your display*/
	disp_drv.disp_flush = ex_disp_flush;            /*Used in buffered mode (LV_VDB_SIZE != 0  in lv_conf.h)*/
	disp_drv.disp_fill = NULL;                      /*Used in unbuffered mode (LV_VDB_SIZE == 0  in lv_conf.h)*/
	disp_drv.disp_map = NULL;                       /*Used in unbuffered mode (LV_VDB_SIZE == 0  in lv_conf.h)*/

	/*Finally register the driver*/
	lv_disp_drv_register(&disp_drv);

	/*************************
	 * Input device interface
	 *************************/
	/*Add a touchpad in the example*/
	lv_indev_drv_t indev_drv;                       /*Descriptor of an input device driver*/

	lv_indev_drv_init(&indev_drv);                  /*Basic initialization*/
	indev_drv.type = LV_INDEV_TYPE_POINTER;         /*The touchpad is pointer type device*/
	indev_drv.read = ex_tp_read;                    /*Library ready your touchpad via this function*/
	lv_indev_drv_register(&indev_drv);              /*Finally register the driver*/

/*	lv_tutorial_hello_world(); */
/*	lv_tutorial_objects(); */
/*	lv_tutorial_styles(); */
	lv_tutorial_themes();
/*	lv_tutorial_responsive(); */

	while (1) {
		/* call LittlevGL handler every 10ms */
		int err = k_sem_take(&lvgl_sem, K_FOREVER);

		if (err)
			continue;

		lv_tick_inc(10);

		lv_task_handler();
	}
}
