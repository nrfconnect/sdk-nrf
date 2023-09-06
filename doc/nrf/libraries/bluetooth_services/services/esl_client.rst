.. _esl_service_client_readme:

Electronic Shelf Label Service (ESLS) client
############################################

.. contents::
   :local:
   :depth: 2

The Electronic Shelf Label Service client library acts as an Access Point (AP) for using the ESL Service exposed by an ESL device.
It defines how the central AP communicates with ESLs when one or more ESLs are in the Synchronized state.
The ESLS client is necessary to interact with the ESLS Server and perform various functions. The ESLS client allows users or systems to discover the capabilities of the Electronic Shelf Labels (ESLs) and configure them accordingly.

The ESL Service client is used in the :ref:`central_esl` sample.

.. _esls_client_config:

Configuration
*************

ESL client provides the following config.

.. _esls_client_pawr_config:

Periodic Advertising with Response
==================================
The AP shall support the PAwR feature specified in the Bluetooth Core Specification 5.4 as a Central device. The length of the Periodic Advertising Interval, the length of the Periodic Advertising Subevent Interval, and the number of subevents are left to the implementation.
After the AP has been configured for a given installation, the timing information shall remain constant. ESL client module makes these timing configuration compile time Kconfig.
The AP should select values for subeventInterval, responseSlotDelay, and responseSlotSpacing that result in sufficient response slots to service the maximum demand. The maximum possible demand for response slots occurs when each command present in the ESL Payload requires a response from a different ESL, and the number of commands contained in the ESL Payload is the highest possible.

:kconfig:option:`CONFIG_ESL_PAWR_INTERVAL_MIN` is the minimum advertising interval for periodic advertising.

:kconfig:option:`CONFIG_ESL_PAWR_INTERVAL_MAX` is the maximum advertising interval for periodic advertising.

:kconfig:option:`CONFIG_ESL_SUBEVENT_INTERVAL` is the interval between PAwR subevents. This is the interval between each ESL Group receiving a ESL ECP command and responding.

:kconfig:option:`CONFIG_ESL_RESPONSE_SLOT_DELAY` is the time between the AP advertising ESL payload packet in a subevent and the first response slot.

:kconfig:option:`CONFIG_ESL_RESPONSE_SLOT_SPACING` is the time between each response slot.

:kconfig:option:`CONFIG_ESL_NUM_RESPONSE_SLOTS` is number of response slots. An ESL Payload typically includes multiple commands as described in ESL Profile Section 5.3.1.3. The option specifies the number of response slots available for the Access Point (AP) to receive responses from the Electronic Shelf Labels (ESLs) during each subevent.
More response slots means more ESLs can be controlled in a single subevent. However, more response slots also means longer subevent interval and more memory is required. The AP should select values for subeventInterval, responseSlotDelay, and responseSlotSpacing that result in sufficient response slots to service the maximum demand. The maximum possible demand for response slots occurs when each command present in the ESL Payload requires a response from a different ESL, and the number of commands contained in the ESL Payload is the highest possible.

:kconfig:option:`CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER` is the maximum number of response slots that can be buffered in ESL client. This value should be equal to or less than the number of response slots.

:kconfig:option:`CONFIG_ESL_CLIENT_MAX_GROUP` is the maximum number of ESL groups that can be controlled.

:kconfig:option:`CONFIG_ESL_CLIENT_DEFAULT_ESL_ID` is the default ESL ID start with when :ref:`central_esl_auto_onboarding_and_auto_past` is enabled.

:kconfig:option:`CONFIG_ESL_CLIENT_DEFAULT_GROUP_ID` is the default group ID start with when :ref:`central_esl_auto_onboarding_and_auto_past` is enabled.

.. _esls_client_other_config:

Others
======

:kconfig:option:`CONFIG_BT_ESL_SCAN_REPORT_INTERVAL` option determines how often the Access Point (AP) will report the Bluetooth Low Energy (BLE) address of Electronic Shelf Label (ESL) tags that match a ESL service UUID during a scan. This report is sent periodically after scanning begins.

:kconfig:option:`CONFIG_BT_ESL_TAG_STORAGE` Save tag information to non-volatile memory so that AP can retrieve the tag information when connected to the tag without prompting in shell command. This option could be used along with :ref:`central_esl_auto_onboarding_and_auto_past`.
The information saved to non-volatile memory includes the following:

   * ESL address
   * BLE Address
   * Bond Key
   * Response key material

:kconfig:option:`CONFIG_BT_ESL_DEMO_SECURITY` is an option for debugging. If this option is enabled, the bonding data will be removed after disconnected.

:kconfig:option:`CONFIG_BT_ESL_LED_INDICATION` is an option for debugging. If this option is enabled, the LED will be turned on or flashing when the Tag is in the corresponding state.

.. _esls_client_usage:

Usage
*****

To use ESLS client in your application, follow these instructions:

   * :ref:`esls_client_config` Kconfig according to AP scenario.
   * Declare :c:struct:`bt_esl_client_init_param`
   * Implement storage callback functions required
   * Call the :c:func:`bt_esl_client_init` function


.. _esls_client_callbacks:

Callbacks
*********

ESLS client requires some callback functions to control the storage for image files / tag information management. To implement the ESL Tag functionality, several callback functions are needed. These callbacks are used to control non-volatile storage. In this section, we will explain why these callbacks are needed and how to implement them.

.. _esls_client_cb_storage:

Storage Callback
================

The AP requires storage to store image, tag information. One of the mandatory features of AP is to transfter image data to the ESL Tag through OTS (Object Transfer Service).

The :c:func:`ap_image_storage_init` function is used to initialize the storage and filesystem for image files. The storage is used to store the image files that will be transferred to the ESL Tag.

The :c:func:`ap_read_img_from_storage` function is used to read the image data to :c:member:`img_obj_buf` from the storage. The image data will be transferred to the ESL Tag through OTS.

The :c:func:`ap_read_img_size_from_storage` function is used to read the image size from the storage. The image size is used to calculate the image'c checksum.



API documentation
*****************

| Header file: :file:`include/bluetooth/services/esl_client.h`
| Source file: :file:`subsys/bluetooth/services/esl/esl_client.c`

.. doxygengroup:: bt_eslc
   :project: nrf
   :members:
