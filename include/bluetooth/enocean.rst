.. _bt_enocean_readme:

Bluetooth EnOcean
#################

The Bluetooth EnOcean library allows passive observation of BLE-based wall switches and sensors from `EnOcean`_, such as wall switches based on the Push Button Transmitter module (PTM 215B) and the Easyfit Motion Detector with Illumination (EMDCB) sensor module.
The library is only capable of observing the output of the devices, and does not send anything back.
NFC-based configuration is not supported.

The library supports different types of commissioning for both wall switches and sensors:

    * Radio-based commissioning.
    * Manually inputting data from other commissioning methods, such as NFC or QR codes.

The library supports replay protection based on sequence numbers.
Only new, authenticated packets from commissioned devices will go through to the event callbacks.

If the :option:`CONFIG_BT_SETTINGS` subsystem is enabled, the device information of all commissioned devices are stored persistently using the :ref:`zephyr:settings` subsystem, including the sequence number information.

For a demonstration of the library features, see the :ref:`enocean_sample` sample.

.. _bt_enocean_devices:

Supported devices
=================

This library supports both PTM 215B-based switches and the BLE-enabled sensor module:

* `Easyfit Single/Double Rocker Wall Switch for BLE - EWSSB/EWSDB`_
* `Easyfit Single/Double Rocker Pad for BLE - ESRPB/EDRPB`_
* `Easyfit Motion Detector with Illumination Sensor - EMDCB`_

By default, the library supports one EnOcean device, and the :ref:`enocean_sample` sample supports only up to four devices.
However, when using the library with your application, you can configure the limit of supported devices to a higher number.

Usage
=====

The EnOcean library uses :cpp:func:`bt_le_scan_cb_register` to receive Bluetooth advertisement packets from nearby EnOcean devices.
The Bluetooth stack must be initialized separately, and Bluetooth scanning must be started by the application.
The Bluetooth Mesh stack does this automatically, so when combined, the application does not need to perform any additional initialization.

.. _bt_enocean_commissioning:

Commissioning
*************

Before the library is able to process sensor or button messages from nearby EnOcean devices, the devices must be commissioned.
By default, the commissioning behavior may be triggered by interacting with the devices, as explained in the following sections.

To enable radio-based commissioning in your application, call :cpp:func:`bt_enocean_commissioning_enable`.

Commissioning wall switches
---------------------------

EnOcean Easyfit wall switches may be put into the commissioning mode by completing the following sequence:

1. Press and hold one of the buttons for more than 7 seconds before releasing it.
#. Quickly press and release the same button (hold for less than 2 seconds).
#. Press and hold the same button again for more than 7 seconds before releasing it.

The switch will transmit its security key, which the library picks up and stores before calling the :cpp:member:`bt_enocean_callbacks::commissioned` callback.

To exit the commissioning mode on a wall switch device, press any of the other buttons on the switch.
The library will now be reporting button presses through the :cpp:member:`bt_enocean_callbacks::button` callback.

Commissioning sensor units
--------------------------

To have the EnOcean Easyfit sensor unit modules publish their security key, short-press the front button of the device.
The library registers the security key and stores it before calling :cpp:member:`bt_enocean_callbacks::commissioned`.

The sensor unit never enters the commissioning mode.
After publishing its security key, the sensor unit automatically goes back to reporting sensor readings at periodic intervals.

.. note::
   EnOcean devices may be configured to disable radio-based commissioning through NFC.
   In such case, the security key must be obtained through manual input.

Observing output
****************

After commissioning an EnOcean device, its activity may be monitored through the :cpp:type:`bt_enocean_handlers` callback functions passed to :cpp:func:`bt_enocean_init`.
See the :ref:`enocean_sample` for a demonstration of the handler callback functions.

Dependencies
************

The EnOcean library depends on the :option:`CONFIG_BT_OBSERVER` capability in the Bluetooth stack to function.

To enable persistent storage of device commissioning data, the :option:`CONFIG_BT_SETTINGS` subsystem must also be enabled.

API documentation
=================

| Header file: :file:`include/bluetooth/enocean.h`
| Source file: :file:`subsys/bluetooth/enocean.c`

.. doxygengroup:: bt_enocean
   :project: nrf
   :members:
