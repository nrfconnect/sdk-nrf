.. _bt_enocean_readme:

Bluetooth EnOcean
#################

.. contents::
   :local:
   :depth: 2

The Bluetooth® EnOcean library allows passive observation of wall switches and sensors based on Bluetooth LE from `EnOcean`_, such as wall switches based on the Push Button Transmitter module (PTM 215B) and the Easyfit Motion Detector with Illumination (EMDCB) sensor module.
The library is only capable of observing the output of the devices, and does not send anything back.
NFC-based configuration is not supported.

The library supports different types of commissioning for both wall switches and sensors:

* Radio-based commissioning.
* Manually inputting data from other commissioning methods, such as NFC or QR codes.

The library supports replay protection based on sequence numbers.
Only new, authenticated packets from commissioned devices will go through to the event callbacks.

If the :kconfig:option:`CONFIG_BT_SETTINGS` subsystem is enabled, the device information of all commissioned devices is stored persistently using the :ref:`zephyr:settings_api` subsystem, including the sequence number information.

For a demonstration of the library features, see the :ref:`enocean_sample` sample.

.. _bt_enocean_devices:

Supported devices
=================

This library supports both PTM 215B-based switches and the sensor module enabled for Bluetooth LE:

* `Easyfit Single/Double Rocker Wall Switch for Bluetooth LE - EWSSB/EWSDB`_
* `Easyfit Single/Double Rocker Pad for Bluetooth LE - ESRPB/EDRPB`_
* `Easyfit Motion Detector with Illumination Sensor - EMDCB`_

By default, the library supports one EnOcean device, and the :ref:`enocean_sample` sample supports only up to four devices.
However, when using the library with your application, you can configure the limit of supported devices to a higher number.

Usage
=====

The EnOcean library uses the :c:func:`bt_le_scan_cb_register` function to receive Bluetooth advertisement packets from nearby EnOcean devices.
The Bluetooth stack must be initialized separately, and Bluetooth scanning must be started by the application.
The Bluetooth Mesh stack does this automatically, so when combined, the application does not need to perform any additional initialization.

.. _bt_enocean_commissioning:

Commissioning
*************

The devices must be commissioned so that the library can process sensor or button messages from nearby EnOcean devices.
By default, the commissioning behavior may be triggered by interacting with the devices, as explained in the following sections.

To enable radio-based commissioning in your application, call the :c:func:`bt_enocean_commissioning_enable` function.

Commissioning wall switches
---------------------------

To set the EnOcean Easyfit wall switches into the commissioning mode, complete the following sequence:

1. Press and hold one of the buttons for more than seven seconds before releasing it.
#. Quickly press and release the same button (hold for less than two seconds).
#. Press and hold the same button again for more than seven seconds before releasing it.

The switch transmits its security key, and the library picks up and stores the key before calling the :c:member:`bt_enocean_callbacks.commissioned` callback.

To exit the commissioning mode on a wall switch device, press any of the other buttons on the switch.
The library will now be reporting button presses through the :c:member:`bt_enocean_callbacks.button` callback.

Commissioning sensor units
--------------------------

To have the EnOcean Easyfit sensor unit modules publish their security key, short-press the front button of the device.
The library registers the security key and stores it before calling :c:member:`bt_enocean_callbacks.commissioned`.

The sensor unit never enters the commissioning mode.
After publishing its security key, the sensor unit automatically goes back to reporting sensor readings at periodic intervals.

.. note::
   You can configure EnOcean devices to disable radio-based commissioning through NFC.
   In such case, the security key must be obtained through manual input.

Observing output
****************

After commissioning an EnOcean device, you can monitor its activity through the :c:type:`bt_enocean_handlers` callback functions passed to the :c:func:`bt_enocean_init` function.
See the :ref:`enocean_sample` for a demonstration of the handler callback functions.

Dependencies
************

The EnOcean library depends on the :kconfig:option:`CONFIG_BT_OBSERVER` capability in the Bluetooth stack.

To enable persistent storing of device commissioning data, you must also enable the :kconfig:option:`CONFIG_BT_SETTINGS` Kconfig option.

API documentation
=================

| Header file: :file:`include/bluetooth/enocean.h`
| Source file: :file:`subsys/bluetooth/enocean.c`

.. doxygengroup:: bt_enocean
   :project: nrf
   :members:
