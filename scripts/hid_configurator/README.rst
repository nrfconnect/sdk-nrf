.. _nrf_desktop_config_channel_script:

HID configurator for nRF Desktop
################################

This Python application uses :ref:`nrf_desktop_config_channel` for communication between a computer and firmware.

The script controls the :ref:`nrf_desktop` firmware parameters at runtime.
It can be used for the following purposes:

* `Configuring device runtime options`_
* `Performing DFU`_
* `Rebooting the device`_
* `Getting information about the FW version`_
* `Playing LEDstream`_

Overview
********

The script looks for a nRF Desktop device whose name is passed as argument.
The device must be connected to the host through USB or Bluetooth and it must match the Product ID (PID) and Vendor ID (VID) associated with the device name.
If the device cannot be found, the script looks for a nRF Desktop dongle and tries to connect to the device through the dongle.

The script exchanges data with the device using :ref:`nrf_desktop_config_channel`.
The data is sent using a HID feature reports and targets a specific FW module of the nRF Desktop application.

The script scans for available modules.
For each module, it obtains the list of available options.
For details about options available within each module, see the module documentation.

For more information about the architecture of the nRF Desktop configuration channel, see :ref:`nrf_desktop_config_channel`.
The configuration channel uses a cross-platform HIDAPI library to communicate with HID devices attached to the operating system.
See `Dependencies`_ for more information.

Requirements
************
To use the HID configurator, you must set up the required `Dependencies`_.
Complete the following instructions, depending on your operating system:

* `Windows`_
* `Debian/Ubuntu/Linux Mint`_

Windows
=======

Complete the following steps:

1. Download the HIDAPI library from `HIDAPI releases`_.
   Use the bundled DLL or build it according to instructions in `HIDAPI library`_.
#. Install `pyhidapi Python wrapper`_ with the following command:

.. parsed-literal::
   :class: highlight

   py -3 -m pip install -r requirements.txt

Debian/Ubuntu/Linux Mint
========================

Run the following command:

.. parsed-literal::
   :class: highlight

   sudo apt install libhidapi-hidraw0
   pip3 install --user -r requirements.txt

.. note::
    When using the configuration channel for Bluetooth LE devices on Linux, use the BlueZ version 5.44 or higher.
    In earlier versions, the HID device attached by BlueZ could obtain wrong VID and PID values (ignoring values in Device Information Service), which would stop HIDAPI from opening the device.

    Additionally, to call the Python script on Linux without root rights, install the provided udev rule :file:`99-hid.rules` file by copying it to :file:`/etc/udev/rules.d` and replugging the device.

Using the script
****************

To obtain the list of supported devices, see the script's help by running the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py -h

The script connects to the device whose name is passed as the first argument of the Python command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME -h

Once connected, the script can perform an operation from the list of commands available for the device.
The list can be obtain from the script's help, along with explanation for each command.
The command is passed as the second argument to the script:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME COMMAND_NAME -h

Configuring device runtime options
==================================

The script can pass the configuration values to the linked FW module using the ``config`` command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME config -h

Customize the command with the following variables:

* ``MODULE_NAME`` - The third argument is used to pass the name of module to be configured.
* ``OPTION_NAME`` - The fourth argument is used to pass the name of the option.
* ``VALUE`` - Optional fifth argument is used to pass a new value of the selected option.

To read the currently set value, pass the name of the module and the option to the ``config`` command, without providing any value:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME config MODULE_NAME OPTION_NAME

To write a new value for the selected option, pass the value as the fifth argument:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME config MODULE_NAME OPTION_NAME VALUE

Performing DFU
==============

The nRF Desktop application supports background DFU (Device Firmware Upgrade).
The image is passed to the device while the device is in normal operation.
The new image is stored on a dedicated update partition of the flash memory.
When the whole image is transmitted, the update process is completed during the next reboot of the device.

If the DFU process is interrupted, it can be resumed using the same image, unless the device restarts.
After the device reboots, the process always starts from the beginning.
For more information, see nRF Desktop's :ref:`nrf_desktop_dfu`.

The ``dfu`` command will read the version of the firmware running on the device and compare it with the firmware version in the update image at the provided path.
If the process is to be continued, the script will upload the image data to the device.
When the upload is completed, the script will reboot the device.

Customize the command with the following variables:

* ``UPDATE_IMAGE_PATH`` - Path to the DFU update file.

To perform a DFU operation, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME dfu UPDATE_IMAGE_PATH

Rebooting the device
====================

To perform a device reboot operation, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME fwreboot

Getting information about the FW version
========================================

To obtain information about the firmware running on the device, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME fwinfo

Playing LEDstream
=================

The LEDstream is a feature of nRF Desktop that allows you to send a stream of color data to be replayed on the device LED.
For more information about its implementation, see nRF Desktop's :ref:`nrf_desktop_led_stream`.

HID configurator's ``led_stream`` command will start the LEDstream playback on the device.

Customize the command with the following variables:

* ``LED_ID`` - The third argument to the script is the ID of the LED on which the stream is to be replayed.
* ``FREQUENCY`` - The fourth argument to the script is the frequency at which the data is to be generated.
  The higher the frequency, the more often the colors change.
* ``--file WAVE_FILE`` - Optional argument for opening a wave file and using it to generate the stream of colors based on the sound data.

To start the LEDstream payback, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE_NAME led_stream LED_ID FREQUENCY --file WAVE_FILE

Dependencies
************

The configuration channel uses the following dependencies:

* `HIDAPI library`_
* `pyhidapi Python wrapper`_
