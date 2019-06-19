.. _config_channel_script:

HID configurator
################

This Python application is an example of using the :ref:`config_channel` for communication between a computer and firmware.
It controls the :ref:`nrf_desktop` firmware parameters at runtime.

Overview
********

The script can be used to:

* perform DFU,
* configure the following parameters at runtime:

	* mouse optical sensor resolution (CPI)
	* mouse optical sensor power saving modes:

		* downshift time from "Run" to "Rest1" mode
		* downshift time from "Rest1" to "Rest2" mode
		* downshift time from "Rest2" to "Rest3" mode

For details of the architecture of the firmware module, see :ref:`config_channel`.

Usage
*****

To set a value of the sensor CPI, run:

.. parsed-literal::
   :class: highlight

   python3 configurator.py config sensor cpi 12000

To fetch a value, use `fetch` instead of a numeric value:

.. parsed-literal::
   :class: highlight

   python3 configurator.py config sensor downshift_rest1 fetch

To perform a DFU operation, run:

.. parsed-literal::
   :class: highlight

   python3 configurator.py dfu signed-dfu-image.bin

To get a list of possible arguments at each subcommand level, call the command with the ``--help`` option:

.. parsed-literal::
   :class: highlight

   python3 configurator.py --help
   python3 configurator.py config sensor --help


Installation
************
To set up dependencies for the HID configurator, follow these instructions.

Windows
~~~~~~~

* Download the HIDAPI library from `HIDAPI releases`_.
  Use the bundled DLL or build it according to instructions in `HIDAPI library`_.

* Install `pyhidapi Python wrapper`_:

.. parsed-literal::
   :class: highlight

   py -3 -m pip install -r requirements.txt

Debian/Ubuntu/Linux Mint
~~~~~~~~~~~~~~~~~~~~~~~~

.. parsed-literal::
   :class: highlight

   sudo apt install libhidapi-hidraw0
   pip3 install --user -r requirements.txt

Installation troubleshooting
============================

1. When using the configuration channel for BLE devices on Linux, it is recommended to use BlueZ version >= 5.44.
   In previous versions, the HID device attached by BlueZ may obtain wrong VID and PID values (ignoring values in Device Information Service), which will stop HIDAPI from opening the device.
#. On Linux, to call the Python script without root rights, install the provided udev rule ``99-hid.rules`` by copying it to ``/etc/udev/rules.d`` and replugging the device.

Dependencies
************

Configuration channel uses the cross-platform HIDAPI library to communicate with HID devices attached to the operating system.

* `HIDAPI library`_
* `pyhidapi Python wrapper`_

