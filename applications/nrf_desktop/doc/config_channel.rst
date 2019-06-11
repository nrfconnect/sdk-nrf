.. _config_channel:

Configuration channel
#####################

The configuration channel for :ref:`nrf_desktop` application is a way for the end user to control the parameters of a HID device from the computer.

Overview
********

An end user of the mouse and keyboard may need a way to configure their HID devices.
For example, a user might want to change the sensitivity of the mouse.
The configuration channel provides a possibility to set firmware parameters from a computer.
Parameter values can also be fetched from a device to a host computer.

For instructions on how to install and use the configuration channel on a host computer, see :ref:`config_channel_script`.

Behavior
********

Information between a host and a connected embedded device is transported in HID feature reports.
The cross-platform `HIDAPI library`_ is used for exchanging the reports with the connected device, supporting both Bluetooth Low Energy and USB.

The host computer can set configuration values of the embedded device.
The host can also request fetching a value, for example, to display it to the user.

Setting a configuration value:

.. msc::
   hscale = "1.3";
   Host,Device;
   Host>>Device      [label="set report: set value 1600"];
   Host<<Device      [label="get report: SUCCESS"];

All exchanges, both setting and getting a report, are initiated by host.
Therefore, during a fetch operation, the host polls the device until the data is available.

Fetching a configuration value:

.. msc::
   hscale = "1.3";
   Host,Device;
   Host>>Device      [label="set report: request value fetch"];
   Host<<Device      [label="get report: PENDING"];
   Host<<Device      [label="get report: value 1600, SUCCESS"];


Data from host can be forwarded through a USB dongle to the connected device.
The device must act as a BLE peripheral in such case.

Setting a configuration value of a device, forwarded by dongle to a paired device:

.. msc::
   hscale = "1.3";
   Host,Dongle,Device;
   Host>>Dongle      [label="set report: set value 1600"];
   Dongle>>Device    [label="set report: set value 1600"];
   Host<<Dongle      [label="get report: PENDING"];
   Dongle<<Device    [label="get report: SUCCESS"];
   Host<<Dongle      [label="get report: SUCCESS"];


Supported transports
====================

* USB HID
* Bluetooth Low Energy, using HID service


Supported topologies
====================

The configuration channel can be used in one of these topologies:

* PCA20041 mouse connected over USB to a host computer
* PCA20041 mouse connected over BLE to a host computer
* PCA10059 dongle connected over USB to a host computer
* PCA20041 mouse connected to a PCA10059 dongle over BLE; dongle attached to a computer over USB

Data format
===========

The data is exchanged in HID feature reports. Each request has the following format:

.. _table:

+-------------------------------------------------------------------+
| Feature report                                                    |
+-----------+-----+-----+----------+--------+-------------+---+-----+
| 0         | 1   | 2   | 3        | 4      | 5           | 6 | ... |
+===========+=====+=====+==========+========+=============+===+=====+
| Report ID | Recipient | Event ID | Status | Data length | Data    |
+-----------+-----------+----------+--------+-------------+---------+

* Report ID - HID report ID of the feature report used for transmitting data.
* Recipient - product ID of the device to which the request is addressed. Needed to route requests in a multi-device setup.
* Event ID - identifier of the value that should be set or fetched.
* Status - used to exchange status of the request, also used by sender to ask for fetch.
* Data length - indicates how many bytes of data the request holds.
* Data - arbitrary length data connected to the request.

Event ID contains information about the type of request, the module to which it is addressed, and the requested option.

.. tip::
   Bluetooth Low Energy HID Service removes the leading report ID byte, resulting in firmware obtaining a data frame 1 byte shorter.

   USB HID class transmits the whole report, including the report ID byte.

Handling configuration channel in firmware
==========================================

To enable the configuration channel in nRF52 Desktop firmware, set :c:macro:`CONFIG_DESKTOP_CONFIG_CHANNEL` to :c:macro:`y`.

When the device is connected over USB, requests are handled by :file:`usb_state.c` in functions :cpp:func:`report_get()` and :cpp:func:`report_set()`.

If the device is paired using Bluetooth Low Energy, requests are handled in :file:`hids.c` in :cpp:func:`feature_report_handler()`.
Argument :c:data:`write` indicates if this is a GATT write (set report) or a GATT read (get report).

Forwarding requests through a dongle to a connected peripheral is handled in :file:`hid_forward.c`.
The dongle, which is a BLE Central, uses the HID Client module to find the feature report of the paired device and access it to forward the configuration request.

Dependencies
************

Dependencies for the host software are described in :ref:`config_channel_script`.

API documentation
*****************

| Header file: :file:`applications/nrf_desktop/src/util/config_channel.h`
| Source file: :file:`applications/nrf_desktop/src/util/config_channel.c`

.. doxygengroup:: config_channel
   :project: nrf
   :members:
