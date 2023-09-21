.. _esl_service_client_readme:

Electronic Shelf Label Service (ESLS) client
############################################

.. contents::
   :local:
   :depth: 2

The Electronic Shelf Label Service client library acts as an access point (AP) for using the ESL Service exposed by an ESL device.
It defines how the central AP communicates with ESLs when one or more ESLs are in synchronized state.
The ESLS client is needed to interact with the ESLS server and perform various functions.
The ESLS client allows users or systems to discover the capabilities of the Electronic Shelf Labels (ESL) and configure them accordingly.

The ESL Service client is used in the :ref:`central_esl` sample.

.. _esls_client_config:

Configuration
*************

ESLS client provides the configurations described in the following sections.

.. _esls_client_pawr_config:

Periodic Advertising with Response (PAwR)
=========================================

The AP shall support the PAwR feature specified in section 5.4 of the `Bluetooth Core Specification`_ as a Central device.
The length of the Periodic Advertising Interval and Periodic Advertising Subevent Interval, as well as the number of subevents are left to the implementation.
After the AP has been configured for a given installation, the timing information shall remain constant.
ESLS client configures the timing compile time using Kconfig options.
The AP should select values for ``subeventInterval``, ``responseSlotDelay``, and ``responseSlotSpacing``, which result in sufficient response slots to serve the maximum demand.
The maximum possible demand for response slots occurs when each command present in the ESL payload requires a response from a different ESL, and the number of commands contained in the ESL payload is the highest possible.

Use the following Kconfig options to set the timing intervals and response slots:

* :kconfig:option:`CONFIG_ESL_PAWR_INTERVAL_MIN` sets the minimum advertising interval for periodic advertising.
* :kconfig:option:`CONFIG_ESL_PAWR_INTERVAL_MAX` sets the maximum advertising interval for periodic advertising.
* :kconfig:option:`CONFIG_ESL_SUBEVENT_INTERVAL` sets the interval between PAwR subevents.

  This is the interval between each ESL Group receiving a ESL ECP command and responding.

* :kconfig:option:`CONFIG_ESL_RESPONSE_SLOT_DELAY` sets the time between the AP advertising ESL payload packet in a subevent and the first response slot.
* :kconfig:option:`CONFIG_ESL_RESPONSE_SLOT_SPACING` sets the time between each response slot.
* :kconfig:option:`CONFIG_ESL_NUM_RESPONSE_SLOTS` sets the number of response slots.

An ESL payload typically includes multiple commands as described in section 5.3.1.3. of the `Electronic Shelf Label Profile 1.0`_ specification.
It specifies the number of response slots available for the access point to receive responses from the Electronic Shelf Labels during each subevent.
The more response slots, the more ESLs can be controlled in a single subevent.
However, more response slots also means longer subevent interval and more memory is required.
The AP should select values for ``subeventInterval``, ``responseSlotDelay``, and ``responseSlotSpacing`` that result in sufficient response slots to serve the maximum demand.
The maximum possible demand for response slots occurs when each command present in the ESL payload requires a response from a different ESL, and the number of commands contained in the ESL payload is the highest possible.

Use the following Kconfig options to set the :ref:`central_esl_auto_onboarding_and_auto_past` features:

* :kconfig:option:`CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER` sets the maximum number of response slots that can be buffered in ESLS client.

  This value should be equal to or less than the number of response slots.

* :kconfig:option:`CONFIG_ESL_CLIENT_MAX_GROUP` sets the maximum number of ESL groups that can be controlled.
* :kconfig:option:`CONFIG_ESL_CLIENT_DEFAULT_ESL_ID` sets the default ESL ID to start with.
* :kconfig:option:`CONFIG_ESL_CLIENT_DEFAULT_GROUP_ID` sets the default group ID to start with.

.. _esls_client_other_config:

Other configuration options
===========================

You can also use the following Kconfig options for reporting, storage, and debugging:

* :kconfig:option:`CONFIG_BT_ESL_SCAN_REPORT_INTERVAL` option defines how often the AP will report the Bluetooth Low Energy address of Electronic Shelf Label tags that match a ESL service UUID during a scan.

  This report is sent periodically after the scanning begins.

* :kconfig:option:`CONFIG_BT_ESL_TAG_STORAGE` sets the tag information storage to non-volatile memory so that AP can retrieve the information when connected to the tag without prompting in a shell command.

  You can use this option along with :ref:`central_esl_auto_onboarding_and_auto_past`.
  The information saved to non-volatile memory includes the following:

  * ESL address
  * BLE address
  * Bond Key
  * Response key material

* :kconfig:option:`CONFIG_BT_ESL_DEMO_SECURITY` is an option for debugging.

  If this option is enabled, the bonding data will be removed after the tag device has been disconnected from the AP.

* :kconfig:option:`CONFIG_BT_ESL_LED_INDICATION` is an option for debugging.

  If this option is enabled, the LED will be turned on or flashing when the tag device is in the corresponding state.

.. _esls_client_usage:

Usage
*****

To use ESL client in your application, complete the following steps:

1. Configure the APs.
#. Declare he :c:struct:`bt_esl_client_init_param` structure.
#. Implement the storage callback functions required.
#. Call the :c:func:`bt_esl_client_init` function.

Application integration
***********************

The following sections explain how you can integrate the service in your application.

.. _esls_client_callbacks:

Callbacks
=========

ESLS client requires a number of callback functions to control the storing of image files and manage tag information.
These callbacks are used to control non-volatile storage.
This section explains why these callbacks are needed and how to implement them.

.. _esls_client_cb_storage:

Storage callbacks
-----------------

The AP requires storage for the image and tag information.
One of the mandatory features of the AP is to transfter image data to the tag device through Object Transfer Service (OTS).

You need to implement the following callbacks for the storage:

* The :c:func:`ap_image_storage_init` function is used to initialize the storage and filesystem for image files.

  The storage is used to store the image files that will be transferred to the tag device.

* The :c:func:`ap_read_img_from_storage` function is used to read the image data to :c:member:`img_obj_buf` from the storage.

  The image data will be transferred to the tag device through OTS.

* The :c:func:`ap_read_img_size_from_storage` function is used to read the image size from the storage.

  The image size is used to calculate the checksum of the image.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/esl_client.h`
| Source file: :file:`subsys/bluetooth/services/esl/esl_client.c`

.. doxygengroup:: bt_eslc
   :project: nrf
   :members:
