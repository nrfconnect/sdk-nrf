/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lv_tutorials.h"

/**
 * Create a simple 'Hello world!' label
 */
void lv_tutorial_hello_world(void)
{
	/*Create a Label on the currently active screen*/
	lv_obj_t *label1 =  lv_label_create(lv_scr_act(), NULL);

	/*Modify the Label's text*/
	lv_label_set_text(label1, "Hello world!");

	/* Align the Label to the center
	 * NULL means align on parent (which is the screen now)
	 * 0, 0 at the end means an x, y offset after alignment*/
	lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);
}

/**
 * Called when a button is released
 * @param btn pointer to the released button
 * @return LV_RES_OK because the object is not deleted in this function
 */
static  lv_res_t btn_rel_action(lv_obj_t *btn)
{
	/*Increase the button width*/
	lv_coord_t width = lv_obj_get_width(btn);
	lv_obj_set_width(btn, width + 20);

	return LV_RES_OK;
}

/**
 * Called when a new option is chosen in the drop down list
 * @param ddlist pointer to the drop down list
 * @return LV_RES_OK because the object is not deleted in this function
 */
static  lv_res_t ddlist_action(lv_obj_t *ddlist)
{
	u16_t opt = lv_ddlist_get_selected(ddlist);      /*Get the id of selected option*/

	lv_obj_t *slider = lv_obj_get_free_ptr(ddlist);       /*Get the saved slider*/
	lv_slider_set_value(slider, (opt * 100) / 4);       /*Modify the slider value according to the selection*/

	return LV_RES_OK;
}


/**
 * Create some objects
 */
void lv_tutorial_objects(void)
{

	/********************
	 * CREATE A SCREEN
	 *******************/
	/* Create a new screen and load it
	 * Screen can be created from any type object type
	 * Now a Page is used which is an objects with scrollable content*/
	lv_obj_t *scr = lv_page_create(NULL, NULL);
	lv_scr_load(scr);

	/****************
	 * ADD A TITLE
	 ****************/
	lv_obj_t *label = lv_label_create(scr, NULL);  /*First parameters (scr) is the parent*/
	lv_label_set_text(label, "Object usage demo");  /*Set the text*/
	lv_obj_set_x(label, 50);                        /*Set the x coordinate*/

	/***********************
	 * CREATE TWO BUTTONS
	 ***********************/
	/*Create a button*/
	lv_obj_t *btn1 = lv_btn_create(lv_scr_act(), NULL);          /*Create a button on the currently loaded screen*/
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, btn_rel_action); /*Set function to be called when the button is released*/
	lv_obj_align(btn1, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);  /*Align below the label*/

	/*Create a label on the button (the 'label' variable can be reused)*/
	label = lv_label_create(btn1, NULL);
	lv_label_set_text(label, "Button 1");

	/*Copy the previous button*/
	lv_obj_t *btn2 = lv_btn_create(lv_scr_act(), btn1);         /*Second parameter is an object to copy*/
	lv_obj_align(btn2, btn1, LV_ALIGN_OUT_RIGHT_MID, 50, 0);    /*Align next to the prev. button.*/

	/*Create a label on the button*/
	label = lv_label_create(btn2, NULL);
	lv_label_set_text(label, "Button 2");

	/****************
	 * ADD A SLIDER
	 ****************/
	lv_obj_t *slider = lv_slider_create(scr, NULL);                             /*Create a slider*/
	lv_obj_set_size(slider, lv_obj_get_width(lv_scr_act())  / 3, LV_DPI / 3);   /*Set the size*/
	lv_obj_align(slider, btn1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);                /*Align below the first button*/
	lv_slider_set_value(slider, 30);                                            /*Set the current value*/

	/***********************
	 * ADD A DROP DOWN LIST
	 ************************/
	lv_obj_t *ddlist = lv_ddlist_create(lv_scr_act(), NULL);             /*Create a drop down list*/
	lv_obj_align(ddlist, slider, LV_ALIGN_OUT_RIGHT_TOP, 50, 0);         /*Align next to the slider*/
	lv_obj_set_free_ptr(ddlist, slider);                                   /*Save the pointer of the slider in the ddlist (used in 'ddlist_action()')*/
	lv_obj_set_top(ddlist, true);                                        /*Enable to be on the top when clicked*/
	lv_ddlist_set_options(ddlist, "None\nLittle\nHalf\nA lot\nAll"); /*Set the options*/
	lv_ddlist_set_action(ddlist, ddlist_action);                         /*Set function to call on new option is chosen*/

	/****************
	 * CREATE A CHART
	 ****************/
	lv_obj_t *chart = lv_chart_create(lv_scr_act(), NULL);                          /*Create the chart*/
	lv_obj_set_size(chart, lv_obj_get_width(scr) / 2, lv_obj_get_width(scr) / 4);   /*Set the size*/
	lv_obj_align(chart, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 50);                   /*Align below the slider*/
	lv_chart_set_series_width(chart, 3);                                            /*Set the line width*/

	/*Add a RED data series and set some points*/
	lv_chart_series_t *dl1 = lv_chart_add_series(chart, LV_COLOR_RED);
	lv_chart_set_next(chart, dl1, 10);
	lv_chart_set_next(chart, dl1, 25);
	lv_chart_set_next(chart, dl1, 45);
	lv_chart_set_next(chart, dl1, 80);

	/*Add a BLUE data series and set some points*/
	lv_chart_series_t *dl2 = lv_chart_add_series(chart, LV_COLOR_MAKE(0x40, 0x70, 0xC0));
	lv_chart_set_next(chart, dl2, 10);
	lv_chart_set_next(chart, dl2, 25);
	lv_chart_set_next(chart, dl2, 45);
	lv_chart_set_next(chart, dl2, 80);
	lv_chart_set_next(chart, dl2, 75);
	lv_chart_set_next(chart, dl2, 505);

}


/**
 * Create objects and styles
 */
void lv_tutorial_styles(void)
{

	/****************************************
	 * BASE OBJECT + LABEL WITH DEFAULT STYLE
	 ****************************************/
	/*Create a simple objects*/
	lv_obj_t  *obj1;
	obj1 = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_pos(obj1, 10, 10);

	/*Add a label to the object*/
	lv_obj_t *label;
	label = lv_label_create(obj1, NULL);
	lv_label_set_text(label, "Default");
	lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

	/****************************************
	 * BASE OBJECT WITH 'PRETTY COLOR' STYLE
	 ****************************************/
	/*Create a simple objects*/
	lv_obj_t *obj2;
	obj2 = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_align(obj2, obj1, LV_ALIGN_OUT_RIGHT_MID, 20, 0);    /*Align next to the previous object*/
	lv_obj_set_style(obj2, &lv_style_pretty);                   /*Set built in style*/

	/* Add a label to the object.
	 * Labels by default inherit the parent's style */
	label = lv_label_create(obj2, NULL);
	lv_label_set_text(label, "Pretty\nstyle");
	lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

	/*****************************
	 * BASE OBJECT WITH NEW STYLE
	 *****************************/
	/* Create a new style */
	static lv_style_t style_new;                         /*Styles can't be local variables*/
	lv_style_copy(&style_new, &lv_style_pretty);         /*Copy a built-in style as a starting point*/
	style_new.body.radius = LV_RADIUS_CIRCLE;            /*Fully round corners*/
	style_new.body.main_color = LV_COLOR_WHITE;          /*White main color*/
	style_new.body.grad_color = LV_COLOR_BLUE;           /*Blue gradient color*/
	style_new.body.shadow.color = LV_COLOR_SILVER;       /*Light gray shadow color*/
	style_new.body.shadow.width = 8;                     /*8 px shadow*/
	style_new.body.border.width = 2;                     /*2 px border width*/
	style_new.text.color = LV_COLOR_RED;                 /*Red text color */
	style_new.text.letter_space = 10;                    /*10 px letter space*/

	/*Create a base object and apply the new style*/
	lv_obj_t *obj3;
	obj3 = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_align(obj3, obj2, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
	lv_obj_set_style(obj3, &style_new);

	/* Add a label to the object.
	 * Labels by default inherit the parent's style */
	label = lv_label_create(obj3, NULL);
	lv_label_set_text(label, "New\nstyle");
	lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);


	/************************
	 * CREATE A STYLED BAR
	 ***********************/
	/* Create a bar background style */
	static lv_style_t style_bar_bg;
	lv_style_copy(&style_bar_bg, &lv_style_pretty);
	style_bar_bg.body.radius = 3;
	style_bar_bg.body.empty = 1;                            /*Empty (not filled)*/
	style_bar_bg.body.border.color = LV_COLOR_GRAY;         /*Gray border color*/
	style_bar_bg.body.border.width = 6;                     /*2 px border width*/
	style_bar_bg.body.border.opa = LV_OPA_COVER;

	/* Create a bar indicator style */
	static lv_style_t style_bar_indic;
	lv_style_copy(&style_bar_indic, &lv_style_pretty);
	style_bar_indic.body.radius = 3;
	style_bar_indic.body.main_color = LV_COLOR_GRAY;          /*White main color*/
	style_bar_indic.body.grad_color = LV_COLOR_GRAY;           /*Blue gradient color*/
	style_bar_indic.body.border.width = 0;                     /*2 px border width*/
	style_bar_indic.body.padding.hor = 8;
	style_bar_indic.body.padding.ver = 8;

	/*Create a bar and apply the styles*/
	lv_obj_t *bar = lv_bar_create(lv_scr_act(), NULL);
	lv_bar_set_style(bar, LV_BAR_STYLE_BG, &style_bar_bg);
	lv_bar_set_style(bar, LV_BAR_STYLE_INDIC, &style_bar_indic);
	lv_bar_set_value(bar, 70);
	lv_obj_set_size(bar, 200, 30);
	lv_obj_align(bar, obj1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

}


/**
 * Initialize a theme and create the same objects like "lv_tutorial_objects'
 */
void lv_tutorial_themes(void)
{
	/*Initialize the alien theme
	 * 210: a green HUE value
	 * NULL: use the default font (LV_FONT_DEFAULT)*/
	lv_theme_t *th = lv_theme_alien_init(210, NULL);

	/*Set the surent system theme*/
	lv_theme_set_current(th);

	/*Create the 'lv_tutorial_objects'*/
	lv_tutorial_objects();

}


#if 0
static lv_style_t btn3_style;


/**
 * Crate some objects an animate them
 */
void lv_tutorial_animations(void)
{
	lv_obj_t *label;


	/*Create a button the demonstrate built-in animations*/
	lv_obj_t *btn1;
	btn1 = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_pos(btn1, 10, 10);     /*Set a position. It will be the animation's destination*/
	lv_obj_set_size(btn1, 80, 50);

	label = lv_label_create(btn1, NULL);
	lv_label_set_text(label, "Float");

	/* Float in the button using a built-in function
	 * Delay the animation with 2000 ms and float in 300 ms. NULL means no end callback*/
	lv_obj_animate(btn1, LV_ANIM_FLOAT_TOP | LV_ANIM_IN, 300, 2000, NULL);

	/*Create a button to demonstrate user defined animations*/
	lv_obj_t *btn2;
	btn2 = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_pos(btn2, 10, 80);     /*Set a position. It will be the animation's destination*/
	lv_obj_set_size(btn2, 80, 50);

	label = lv_label_create(btn2, NULL);
	lv_label_set_text(label, "Move");

	/*Create an animation to move the button continuously left to right*/
	lv_anim_t a;
	a.var = btn2;
	a.start = lv_obj_get_x(btn2);
	a.end = a.start + (100);
	a.fp = (lv_anim_fp_t)lv_obj_set_x;
	a.path = lv_anim_path_linear;
	a.end_cb = NULL;
	a.act_time = -1000;                         /*Negative number to set a delay*/
	a.time = 400;                               /*Animate in 400 ms*/
	a.playback = 1;                             /*Make the animation backward too when it's ready*/
	a.playback_pause = 0;                       /*Wait before playback*/
	a.repeat = 1;                               /*Repeat the animation*/
	a.repeat_pause = 500;                       /*Wait before repeat*/
	lv_anim_create(&a);

	/*Create a button to demonstrate the style animations*/
	lv_obj_t *btn3;
	btn3 = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_pos(btn3, 10, 150);     /*Set a position. It will be the animation's destination*/
	lv_obj_set_size(btn3, 80, 50);

	label = lv_label_create(btn3, NULL);
	lv_label_set_text(label, "Style");

	/*Create a unique style for the button*/
	lv_style_copy(&btn3_style, lv_btn_get_style(btn3, LV_BTN_STYLE_REL));
	lv_btn_set_style(btn3, LV_BTN_STATE_REL, &btn3_style);

	/*Animate the new style*/
	lv_style_anim_t sa;
	sa.style_anim = &btn3_style;            /*This style will be animated*/
	sa.style_start = &lv_style_btn_rel;     /*Style in the beginning (can be 'style_anim' as well)*/
	sa.style_end = &lv_style_pretty;        /*Style at the and (can be 'style_anim' as well)*/
	sa.act_time = -500;                     /*These parameters are the same as with the normal animation*/
	sa.time = 1000;
	sa.playback = 1;
	sa.playback_pause = 500;
	sa.repeat = 1;
	sa.repeat_pause = 500;
	sa.end_cb = NULL;
	lv_style_anim_create(&sa);
}
#endif


/**
 * Crate some objects an animate them
 */
void lv_tutorial_responsive(void)
{
	lv_obj_t *label;

	/*LV_DPI*/
	lv_obj_t *btn1;
	btn1 = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_pos(btn1, LV_DPI / 10, LV_DPI / 10);     /*Use LV_DPI to set the position*/
	lv_obj_set_size(btn1, LV_DPI, LV_DPI / 2);          /*Use LVDOI to set the size*/

	label = lv_label_create(btn1, NULL);
	lv_label_set_text(label, "LV_DPI");

	/*ALIGN*/
	lv_obj_t *btn2;
	btn2 = lv_btn_create(lv_scr_act(), btn1);
	lv_obj_align(btn2, btn1, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 4, 0);

	label = lv_label_create(btn2, NULL);
	lv_label_set_text(label, "Align");

	/*AUTO FIT*/
	lv_obj_t *btn3;
	btn3 = lv_btn_create(lv_scr_act(), btn1);
	lv_btn_set_fit(btn3, true, true);

	label = lv_label_create(btn3, NULL);
	lv_label_set_text(label, "Fit");

	lv_obj_align(btn3, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_DPI / 4);   /*Align when already resized because of the label*/

	/*LAYOUT*/
	lv_obj_t *btn4;
	btn4 = lv_btn_create(lv_scr_act(), btn1);
	lv_btn_set_fit(btn4, true, true);           /*Enable fit too*/
	lv_btn_set_layout(btn4, LV_LAYOUT_COL_R);   /*Right aligned column layout*/

	label = lv_label_create(btn4, NULL);
	lv_label_set_text(label, "First");

	label = lv_label_create(btn4, NULL);
	lv_label_set_text(label, "Second");

	label = lv_label_create(btn4, NULL);
	lv_label_set_text(label, "Third");

	lv_obj_align(btn4, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_DPI / 4);   /*Align when already resized because of the label*/

}
