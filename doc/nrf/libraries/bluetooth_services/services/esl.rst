.. _esl_service_readme:

Electronic Shelf Label Service (ESLS)
#####################################

.. contents::
   :local:
   :depth: 2

The Electronic Shelf Label Service allows you to control and update electronic shelf labels (ESL) using Bluetooth® wireless technology.
The purpose of the ESL Service is to enable communication and control between a central access point (AP) and tag devices.
This service implements a complete state machine and control point command handler for ESL.
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
   Write data to the ESL Address Characteristic to configure a unique address for an ESL device.

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
   When read, the ESL Image Information characteristic returns the Max_Image_Index value equal to the highest Image_Index value supported by the ESL.

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
   The command includes an opcode and parameters.
   The ESL Control Point command applies to both GATT ECP characteristic and PAwR.

.. _esls_config:

Configuration
*************

An ESL is composed of three types of independent, optional elements (a display, an LED, and a sensor).
You first need to set the values of the ESL tag elements as described in the following sections.

.. _esls_config_led:

LED configuration
=================

Use the following Kconfig options to set the LED configuration:

* :kconfig:option:`CONFIG_BT_ESL_LED_MAX` sets the maximum number of LEDs available on the tag device.

  The default value is ``0``, which means there is no LED on the tag device.
  If there is an LED, the application must implement the functions :c:func:`led_init` and :c:func:`led_control` to control the LED.
  The ESL Service generates LED information in the GATT characteristic and a LED work item in the work queue.

* :kconfig:option:`CONFIG_BT_ESL_LED_INDICATION` is an option for debugging.

  If this option is enabled, the LED is lit or flashing when the tag device is in the corresponding state.

.. _esls_config_display:

Display configuration
=====================

Use the following Kconfig options to set the display configuration:

* :kconfig:option:`CONFIG_BT_ESL_DISPLAY_MAX` sets the maximum number of displays available on the tag device.

  The default value is ``0``, which means there is no display on the tag device.
  If there is display, the application must implement the functions :c:func:`display_init` and :c:func:`display_control` to control the display.
  The ESL Service generates display information in the GATT characteristic and a display work item in the work queue.

* :kconfig:option:`CONFIG_ESL_DISPLAY_WIDTH` and :kconfig:option:`CONFIG_ESL_DISPLAY_HEIGHT` set the display resolution of the tag device.

  These values can be acquired by the devicetree if the tag device has a display with a Zephyr driver.
  However, some tag devices use their own driver.
  You need to set these values accordingly.

*  :kconfig:option:`CONFIG_ESL_DISPLAY_TYPE` sets the display type that is defined by Bluetooth SIG assigned number.

* :kconfig:option:`CONFIG_ESL_IMAGE_FILE_SIZE` sets the file size reserved for Object Transfer Service (OTS) storage backend.

  In general, the size is the width x height x bit depth plus image header.

* :kconfig:option:`CONFIG_ESL_IMAGE_BUFFER_SIZE` sets the size of memory reserved for display framebuffer.

  In general, the size is the width x height x bit depth plus image header.
  For some display IC and storage backends, this memory size can be smaller than the image file.

* :kconfig:option:`CONFIG_BT_ESL_IMAGE_MAX` sets the number of images that can be stored in an tag device.

  This value is a trade-off between power consumption and need for non-volatile memory.
  The image is stored in the non-volatile memory of tag device.
  The more images can be stored, the less often the tag device needs to be in updating state to receive new images.
  The less images can be stored, the less non-volatile memory is required by the tag device.

* :kconfig:option:`CONFIG_BT_ESL_STORAGE_BACKEND` sets the storage backend for the image from the AP.

  The possible storage backend values are:

  * :kconfig:option:`CONFIG_ESL_OTS_NVS` - a lightweight key-value store optimized for small data reads and writes, not supporting the seek feature.
  * :kconfig:option:`CONFIG_ESL_OTS_LFS` - a full-featured filesystem optimized for larger data reads and writes, supports the seek feature.

  The choice between the two depends on the specific requirements of your application and the hardware (MCU, external flash or not) you are using.

.. _esls_config_sensor:

Sensor configuration
====================

Use the following Kconfig option to set the sensor configuration:

* :kconfig:option:`CONFIG_BT_ESL_SENSOR_MAX` sets the maximum number of sensors available on the tag device.

  The default value is ``0``, which means there is no sensor on the tag device.
  If the tag device has a sensor, the application must implement the functions :c:func:`sensor_init` and :c:func:`sensor_control` to read the sensor.

.. _esls_config_optionals:

Debugging configuration
=======================

You can also use the following Kconfig options for debugging purposes:

* :kconfig:option:`CONFIG_ESL_SHELL` enables the shell command for controlling the tag device.

* :kconfig:option:`CONFIG_BT_ESL_DEMO_SECURITY` enables removal of bonding data after the tag device is disconnected.

* :kconfig:option:`CONFIG_BT_ESL_FORGET_PROVISION_DATA` enables removal of provisioning data after the tag device is disconnected.

* :kconfig:option:`CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT` enables overriding the mandatory 60 minutes unsynchronized timeout value defined by Bluetooth SIG.

* :kconfig:option:`CONFIG_BT_ESL_UNASSOCIATED_TIMEOUT` enables overriding the mandatory 60 minutes unassociated timeout value defined by Bluetooth SIG.

.. _esls_usage:

Usage
*****

To use ESL Service in your application, complete the following steps:

1. Configure all elements of your ESL tag devices.
#. Declare the :c:struct:`bt_esl_init_param` structure.
#. Fill in the element information to the members of the structure.
#. Implement all required callback functions.
#. Implement the :c:func:`ots_storage_init` function.
#. Call the :c:func:`bt_esl_init` function.

Application integration
***********************

The following sections explain how you can integrate the service in your application.

.. _esls_callbacks:

Callbacks
=========

The ESL Service requires a number of callback functions to control the hardware.
Your application must implement callback functions depending on the hardware used on the tag device.
To implement the tag device functionality, several callback functions are needed.
These callbacks are used to control the display, LEDs, and sensors, as well as to buffer and write image data to storage.
This section explains why these callbacks are needed and how to implement them.

.. _esls_cb_led:

LED callbacks
-------------

You need to implement the following callbacks for the LEDs:

* The :c:func:`led_init` callback is used to initialize the LEDs on the tag device.

  To implement this callback, you need to write a function that calls it.

* The :c:func:`led_control` callback is used to change the LED state, for example on/off, brightness, or color (if an SRGB LED is used).

  This function is called when the ESL tag device decides to change the LED flashing pattern.
  The function takes the following three arguments:

  * LED index
  * Color and brightness to be controlled
  * A boolean value indicating whether to turn on or off the LED

  To implement this callback, you need to write a function that takes these arguments and performs the necessary operations to change the LED state.

.. _esls_cb_display:

Display callbacks
-----------------

You need to implement the following callbacks if the tag device has a display:

* The :c:func:`display_init` callback is used to initialize the display device unless the display driver does that (for example initializes the memory region for framebuffer or sets the font type).

  To implement this callback, you need to write a function that initializes the display device.

* The :c:func:`display_control` callback is used to change the image displayed on the tag device.

  This function is called when the tag device decides to change the image.
  The function takes the following three arguments:

  * The index of the display device
  * The index of the image
  * A boolean value indicating whether to turn on or off the display

  To implement this callback, you need to write a function that takes these arguments and performs the necessary operations to change the image on the display device.

* The :c:func:`display_unassociated` callback is used to show information on the display device to help the user inspect the tag device to be associated.

  This function is called when the tag device is booting up and unassociated.
  To implement this callback, you need to write a function that takes the index of the display device and displays the necessary information on the device.

* The :c:func:`display_associated` callback is used to show information on the display device to indicate that the tag device has been associated.

  This function is called when the tag device is booting up and associated but not synced or commanded to display an image.
  To implement this callback, you need to write a function that takes the index of the display device and displays the necessary information on the device.

If you want to use a font library to print text on the display, you need to implement the following callbacks:

* The :c:func:`display_clear_font` callback is used to clear the framebuffer allocated by the font library for a specified display.

  This function is a callback for the font library and clears the framebuffer allocated by the font library for the specified display.
  The function disables all images on the display by clearing the framebuffer.
  To implement this callback, you need to write a function that takes the index of the display device and clears the framebuffer.

* The :c:func:`display_print_font` callback is used to print text on the specified display using the font library.

  This function is a callback for the font library and prints text on the specified display using the font library.
  The text is printed at the specified position with the specified font.
  To implement this callback, you need to write a function that takes the index of the display device, the text to print, and the position to print the text.
  This callback should only change framebuffer and not update the display.

* The :c:func:`display_update_font` callback is used to finalize the Character Framebuffer (CFB) or font library and check if the Electronic Paper Display (EPD) needs to be re-initialized.

  This function updates the display by flushing any pending updates to the display buffer and checks if the EPD needs to be re-initialized.
  To implement this callback, you need to write a function that takes the index of the display device and write font framebuffer to display.

.. _esls_cb_sensor:

Sensor callbacks
----------------

You need to implement the following callbacks if the tag device has a sensor:

* The :c:func:`sensor_init` callback is used to initialize the sensor device.

  To implement this callback, you need to write a function that initializes the sensor device.

* The :c:func:`sensor_control` callback is used to read sensor data when requested by the tag device.

  This function takes the following two output arguments:

  * The length of the sensor data
  * A pointer to the sensor data reading

  To implement this callback, you need to write a function that takes the index of the sensor and stores the sensor data to the :c:struct:`sensor_data` structure.
  If the sensor reading is successful, value ``0`` is returned.
  If the sensor reading is not fast enough or fails, value ``-EBUSY`` is returned.
  The ESL Service will generate a response based on the return value of this function.

.. _esls_cb_image_storage:

Image storage callbacks
-----------------------

The tag device with a display requires several callbacks to be implemented.
One of the mandatory features of the tag devices is to receive and store image data from the AP through OTS (Object Transfer Service), and to read image data to the framebuffer from storage when the image on the display needs to be changed.
You need to implement the following callbacks if the tag device has a display.
The implementation varies depending on the storage backend (NVS or LittleFS).

* The :c:func:`buffer_img` callback is used to buffer image data from AP when OTS (Object Transfer Service) write operation callback called.

  Image data from OTS may come in fragments, and some filesystems support seek while others do not.
  This callback should either buffer all fragments into :c:member:`img_obj_buf` and write them once for a storage backend not supporting the seek feature, or write chunck by chunk to a storage backend supporting the seek feature.

* The :c:func:`write_img_to_storage` callback is used to write image data to the storage backend when a chunk of image or all of the fragments from OTS are received.

  To implement this callback, you need to write a function that takes the image index, length, and offset and writes the image data to the storage backend.

* The :c:func:`read_img_from_storage` callback is used to read image data from the storage backend to framebuffer when the tag device needs to change the image on the display.

  To implement this callback, you need to write a function that takes the image index, data pointer, length, and offset and reads the image data from the storage backend.

  .. note::

     For a storage backend supporting the seek feature and a display driver IC supporting partial update, the :c:func:`read_img_from_storage` callback can only read the changed part of the image from the storage backend to the framebuffer.
     The display driver IC will only update the changed part of the image on the display.

     For a storage backend that does not support the seek feature, the :c:func:`read_img_from_storage` callback can only read the whole image from the storage backend to the framebuffer.
     The display driver IC will update the whole image on the display.

* The :c:func:`read_img_size_from_storage` callback is used to read the image size from the storage backend when the tag device needs to change the image on the display.

  To implement this callback, you need to write a function that takes the image index and returns the image size.

* The :c:func:`delete_imgs` callback is used to remove all images from the storage backend when received factory reset op code.

  To implement this callback, you need to write a function that removes all images from the storage backend.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/esl.h`
| Source file: :file:`subsys/bluetooth/services/esl/esl.c`

.. doxygengroup:: bt_esl
   :project: nrf
   :members:
