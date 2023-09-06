.. _esl_service_readme:

Electronic Shelf Label Service (ESLS)
#####################################

.. contents::
   :local:
   :depth: 2

The Electronic Shelf Label Service allows you to control and update electronic shelf labels (ESL) using Bluetooth® wireless technology.
The purpose of an ESL (Electronic Shelf Label) service is to enable communication and control between a central access point (AP) and lot of ESL Tag devices.
This service implements complete of ESL state machine and ESL Control Point command handler.
The actual HW control leaves to the application as callback functions and the application must implement all of callback functions.
The ESL Service is used in the :ref:`peripheral_esl` sample.

.. _esl_service_uuid:

Service UUID
************

The 16-bit vendor-specific service UUID is ``0x1857``.

.. _esl-service_characteristics:

Characteristics
***************

This service has the following nine characteristics.

ESL Address Characteristic (``0x2BF6``)
=======================================

Write
   Write data to the ESL Address Characteristic to configure a unique address to ESL device.

AP Sync Key Material Characteristic (``0x2BF7``)
================================================

Write
   The AP Sync Key Material is configured when an AP, acting as a client, writes its AP Sync Key Material value to the AP Sync Key Material characteristic.

ESL Response Key Material Characteristic (``0x2BF8``)
=====================================================

Write
   The ESL Response Key Material is configured when an AP, acting as a client, writes a value to the ESL Response Key Material characteristic.

ESL Current Absolute Time Characteristic (``0x2BF9``)
=====================================================

Write
   When a value is written to the ESL Current Absolute Time characteristic, the server sets its current system time to the value.

ESL Display Information Characteristic (``0x2BFA``)
===================================================

Read
   The ESL Display Information characteristic returns an array of one or more Display Data Structures when read by a client.

ESL Image Information Characteristic (``0x2BFB``)
=================================================

Read
   When read, the ESL Image Information characteristic returns the Max_Image_Index value equal to the numerically highest Image_Index value supported by the ESL.

ESL Sensor Information Characteristic (``0x2BFC``)
==================================================

Read
   The ESL Sensor Information characteristic returns an array of one or more Sensor Information Structures when read by a client.

ESL LED Information Characteristic (``0x2BFD``)
===============================================

Read
   The ESL LED Information characteristic returns an array of one or more octets when read by a client.

ESL Control Point Characteristic (``0x2BFE``)
=============================================

Write Without Response, Write, Notify
   The ESL Control Point characteristic allows a client to write commands to an ESL while connected.

.. note::
   The command includes opcode and parameters. The ESL ECP command applies to both GATT ECP characteristic and PAwR.

.. _esls_config:

Configuration
*************

An ESL is composed of certain elements. Three types of independent, optional elements (a display, a light-emitting diode (LED), and a sensor).
First thing first is to configure hardware information of the ESL Tag. Three elements need to be set.

.. _esls_config_led:

LEDs configuration
==================

:kconfig:option:`CONFIG_BT_ESL_LED_MAX` is the maximum number of LEDs that are on ESL Tag. The default value is 0 which means there is no LED on ESL Tag.
If there is LED on ESL Tag, the application must implement the callback function :c:func:`led_init`, :c:func:`led_control` to control the LED.
ESL Service will generate LED information in GATT characteristic and LED work item in the work queue.
When ESL Service is asked to control LED, it will call the callback function to control the LED.

:kconfig:option:`CONFIG_BT_ESL_LED_INDICATION` is an option for debugging. If this option is enabled, the LED will be turned on or flashing when the Tag is in the corresponding state.

.. _esls_config_display:

Displays configuration
======================

:kconfig:option:`CONFIG_BT_ESL_DISPLAY_MAX` is the maximum number of displays that are on ESL Tag. The default value is 0 which means there is no display on ESL Tag.
If there is display on ESL Tag, the application must implement the callback function :c:func:`display_init` :c:func:`display_control` to control the display.
ESL Service will generate display information in GATT characteristic and display work item in the work queue.
When ESL Service is asked to control display, it will call the callback function to control the display.

:kconfig:option:`CONFIG_ESL_DISPLAY_WIDTH` and :kconfig:option:`CONFIG_ESL_DISPLAY_HEIGHT` are resolution of display device of ESL Tag. These value could be acquired by device tree if ESL Tag uses display with Zephyr driver. However, for some ESL Tag uses its own driver. These values need to be config explicitly.

:kconfig:option:`CONFIG_ESL_DISPLAY_TYPE` is display type which is defined by Bluetooth SIG assigned number.

:kconfig:option:`CONFIG_ESL_IMAGE_FILE_SIZE` is the file size reserved for OTS(Object Transfer Service) storage backend. It is generally the width x height x bit depth plus image header.

:kconfig:option:`CONFIG_ESL_IMAGE_BUFFER_SIZE` is the memory reserved for display framebuffer. It is generally the width x height x bit depth plus image header size. This memory size could be smaller than image file for some display IC and storage backend.

:kconfig:option:`CONFIG_BT_ESL_IMAGE_MAX` is how many images could be stored in ESL Tag. To choose this values need to trade-off power consumpation and non-volatile size. Image stored in non-volatile memory of ESL Tag. The more images could be stored the less chance ESL Tag need to be in updateing state to receive new image. The less images could be stored the less non-volatile memory is required by ESL Tag.

:kconfig:option:`CONFIG_BT_ESL_STORAGE_BACKEND` choose storage backend to store image from AP. :kconfig:option:`CONFIG_ESL_OTS_NVS` NVS is a lightweight key-value store that is optimized for small data reads and writes. But does not support seek feature. :kconfig:option:`CONFIG_ESL_OTS_LFS` LittleFS is a full-featured filesystem that is optimized for larger data reads and writes. But supports seek feature.
The choice between the two depends on the specific requirements of your application and the hardware (MCU, external flash or not) you are using.


.. _esls_config_sensor:

Sensors configuration
=====================

:kconfig:option:`CONFIG_BT_ESL_SENSOR_MAX` is the maximum number of displays that are on ESL Tag. The default value is 0 which means there is no sensor on ESL Tag.
If there is display on ESL Tag, the application must implement the callback function :c:func:`sensor_init` :c:func:`sensor_control` to read the sensor.


.. _esls_config_optionals:

Optional configuration
======================

:kconfig:option:`CONFIG_ESL_SHELL` is an option for debugging. If this option is enabled, the shell command will be available to control the ESL Tag.

:kconfig:option:`CONFIG_BT_ESL_DEMO_SECURITY` is an option for debugging. If this option is enabled, the bonding data will be removed after disconnected.

:kconfig:option:`CONFIG_BT_ESL_FORGET_PROVISION_DATA` is an option for debugging. If this option is enabled, the provisioning data will be removed after disconnected.

:kconfig:option:`CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT` is the option for debugging. Change this value to override mandatory 60 minutes unsynchronized timeout value defined by Bluetooth SIG.

:kconfig:option:`CONFIG_BT_ESL_UNASSOCIATED_TIMEOUT` is the option for debugging. Change this value to override mandatory 60 minutes unassociated timeout value defined by Bluetooth SIG.

.. _esls_usage:

Usage
*****

To use ESLS in your application, follow these instructions:

   * :ref:`esls_config` Kconfig according to your hardware.
   * Declare :c:struct:`bt_esl_init_param`, fill in element :c:struct:`esl_disp_inf` Display, :c:struct:`esl_sensor_inf` Sensor, :c:struct:`led_type` LED information to member of :c:struct:`bt_esl_init_param`
   * Implement all of callback functions required
   * Implement :c:func:`ots_storage_init`
   * Call the :c:func:`bt_esl_init` function


.. _esls_callbacks:

Callbacks
*********

ESL service requires some callback functions to control the hardware. The application must implement callback functions regarding which hardware are on ESL Tag.
To implement the ESL Tag functionality, several callback functions are needed. These callbacks are used to control the display, LED, and sensor devices, as well as to buffer and write image data to storage. In this section, we will explain why these callbacks are needed and how to implement them.


.. _esls_cb_led:


LED Callbacks
=============

The :c:func:`led_init` callback is used to initialize the LED device. To implement this callback, you need to write a function that initializes the LED device.

The :c:func:`led_control` callback is used to change the LED state e.g.: on / off, brightness, color (if SRGB LED used). This function is called when the ESL Tag decides to change the LED flashing pattern. The function takes three arguments: the index of the LED, the color and brightness to be controlled, and a boolean value indicating whether to turn on or off the LED. To implement this callback, you need to write a function that takes these arguments and performs the necessary operations to change the LED state.


.. _esls_cb_display:

Display Callbacks
=================

You need to implement the following callbacks to if display device exists on ESL Tag.
The :c:func:`display_init` callback is used to initialize the display device if additional work is not done by display driver. e.g.: Initialize memory region for framebuffer, set font type. To implement this callback, you need to write a function that initializes the display device.

The :c:func:`display_control` callback is used to change the image displayed on the ESL Tag. This function is called when the ESL Tag decides to change the image. The function takes three arguments: the index of the display device, the index of the image, and a boolean value indicating whether to turn on or off the display. To implement this callback, you need to write a function that takes these arguments and performs the necessary operations to change the image on the display device.

The :c:func:`display_unassociated` callback is used to show information on the display device to help the user inspect ESL Tag to be associated. This function is called when the ESL Tag is booting up and unassociated. To implement this callback, you need to write a function that takes the index of the display device and displays the necessary information on the device.

The :c:func:`display_associated` callback is used to show information on the display device to indicate that the ESL Tag has been associated. This function is called when the ESL Tag is booting up and associated but not synced or not commanded to display an image. To implement this callback, you need to write a function that takes the index of the display device and displays the necessary information on the device.

These are optional callbacks for font library. If you want to use font library to print text on the display device, you need to implement these callbacks.

The :c:func:`display_clear_font` callback is used to clear the framebuffer allocated by the font library for the specified display. This function is a callback for the font library and clears the framebuffer allocated by the font library for the specified display. The function disables all images on the display by clearing the framebuffer. To implement this callback, you need to write a function that takes the index of the display device and clears the framebuffer.

The :c:func:`display_print_font` callback is used to print text on the specified display using the font library. This function is a callback for the font library and prints text on the specified display using the font library. The text is printed at the specified position with the specified font. To implement this callback, you need to write a function that takes the index of the display device, the text to print, and the position to print the text. This callback should only change framebuffer and not update the display. The display will be updated by the `display_update_font` callback.

The :c:func:`display_update_font` callback is used to finalize the CFB or font libarry and check if the EPD (Electronic Paper Display) needs to be re-initialized. This function update display by flushing any pending updates to the display buffer and checking if the EPD needs to be re-initialized. To implement this callback, you need to write a function that takes the index of the display device and write font framebuffer to display.


.. _esls_cb_sensor:

Sensor Callbacks
================

You need to implement the following callbacks to if sensor device exists on ESL Tag.

The :c:func:`sensor_init` callback is used to initialize the sensor device. To implement this callback, you need to write a function that initializes the sensor device.

The :c:func:`sensor_control` callback is used to read sensor data when the ESL Tag decides to do so. This function takes two output arguments: the length of the sensor data and a pointer to the sensor data reading. To implement this callback, you need to write a function that takes the index of the sensor and stores the sensor data to :c:struct:`sensor_data`. If the sensor reading is successful, return 0. If the sensor reading is not fast enough or fails, return `-EBUSY`. The ESL service will generate a response based on the return value of this function.


.. _esls_cb_image_storage:

Image storage Callback
======================

The ESL Tag with a display device requires several callbacks to be implemented. One of the mandatory features of these tags is to receive and store image data from the AP through OTS (Object Transfer Service), and to read image data to the framebuffer from storage when the tag decides to change the image on the display.
You need to implement the following callbacks to if display device exists on ESL Tag. The implementation varies depending on the storage backend. The storage backend can be NVS or LittleFS.

The :c:func:`buffer_img` callback is used to buffer image data from AP when OTS (Object Transfer Service) write operation callback called. Image data from OTS may come in fragments, and some filesystems support seek while others do not. This callback should either buffer all fragments into :c:member:`img_obj_buf` and writes them once for storage backend not support seek feature. Or write chunck to chunk for storage backend supports seek feature.

The :c:func:`write_img_to_storage` callback is used to write image data to the storage backend when chunk of image or all of the fragments from OTS are received. To implement this callback, you need to write a function that takes the image index, length, and offset and writes the image data to the storage backend.

The :c:func:`read_img_from_storage` callback is used to read image data from the storage backend to framebuffer when the ESL Tag decides to change the image on the display. To implement this callback, you need to write a function that takes the image index, data pointer, length, and offset and reads the image data from the storage backend.

.. note::

   For storage backend supports seek feature and display driver IC supports partial update, the usage of:c:func:`read_img_from_storage` callback could only read the changed part of the image from the storage backend to the framebuffer. The display driver IC will only update the changed part of the image on the display.
   For storage backend doesn't support seek feature, the usage of :c:func:`read_img_from_storage` callback could only read the whole image from the storage backend to the framebuffer. The display driver IC will update the whole image on the display.


The :c:func:`read_img_size_from_storage` callback is used to read the image size from the storage backend when the ESL Tag decides to change the image on the display. To implement this callback, you need to write a function that takes the image index and returns the image size.

The :c:func:`delete_imgs` callback is used to remove all images from the storage backend when received factory reset op code. To implement this callback, you need to write a function that removes all images from the storage backend.


API documentation
*****************

| Header file: :file:`include/bluetooth/services/esl.h`
| Source file: :file:`subsys/bluetooth/services/esl/esl.c`

.. doxygengroup:: bt_esl
   :project: nrf
   :members:
