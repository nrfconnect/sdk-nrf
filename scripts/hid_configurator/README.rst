.. _nrf_desktop_config_channel_script:

HID configurator for nRF Desktop
################################

.. contents::
   :local:
   :depth: 2

This Python application uses :ref:`nrf_desktop_config_channel` for communication between a computer and firmware.

The script controls the :ref:`nrf_desktop` firmware parameters at runtime.
It can be used for the following purposes:

* `Displaying the supported modules and options`_
* `Configuring device runtime options`_
* `Performing DFU`_
* `Rebooting the device`_
* `Getting information about the firmware version`_
* `Getting identification information about the device`_
* `Playing LED stream`_

Overview
********

The script looks for nRF Desktop devices connected to the host through USB, Bluetooth®, or nRF Desktop dongle.
The devices are identified based on Vendor ID.

The script exchanges data with the device using :ref:`nrf_desktop_config_channel`.
The data is sent using HID feature reports and targets a specific firmware module of the nRF Desktop application.

For more information about the architecture of the nRF Desktop configuration channel, see :ref:`nrf_desktop_config_channel`.
The configuration channel uses a cross-platform HIDAPI library to communicate with HID devices attached to the operating system.
See `Dependencies`_ for more information.

Requirements
************

To use the HID configurator, you must set up the required `Dependencies`_.
Complete the following instructions, depending on your operating system:

* `Windows`_
* `Debian/Ubuntu/Linux Mint`_

You also need to ensure that the `Linux Vendor Firmware Service (LVFS) <LVFS_>`_ ``fwupd`` daemon is not running in the background.
See the `Stopping fwupd daemon`_ section for details.

Windows
=======

Complete the following steps:

1. Download the HIDAPI library from `HIDAPI releases`_.
   Use the bundled DLL or build it according to instructions in `HIDAPI library`_.

#. Select the appropriate HIDADPI library version for the used Windows system (either ``x86`` or ``x64``).
   Copy the :file:`hidapi.dll` file and paste it into either of the following directory:

   * The directory where used Python executable is located.
   * The :file:`Windows\\System32` directory, for example :file:`C:\\Windows\\System32`.

#. Install `pyhidapi Python wrapper`_ and other required libraries with the following command:

   .. parsed-literal::
      :class: highlight

      py -3 -m pip install -r requirements.txt

#. If you want to display LED stream based on sound data, you must also install the additional requirements using the following command:

   .. parsed-literal::
      :class: highlight

      py -3 -m pip install -r requirements_music_led_stream.txt

For more detailed information about LED stream functionality, see the `Playing LED stream`_ section.

Debian/Ubuntu/Linux Mint
========================

Complete the following steps:

1. Run the following commands to install the basic requirements:

   .. parsed-literal::
      :class: highlight

      sudo apt install libhidapi-hidraw0
      pip3 install --user -r requirements.txt

   .. note::
       When using the configuration channel for Bluetooth LE devices on Linux, use the BlueZ version 5.56 or higher.
       In versions earlier than 5.44, the HID device attached by BlueZ could obtain wrong VID and PID values (ignoring values in Device Information Service), which would stop HIDAPI from opening the device.
       In versions earlier than 5.56, the HID device attached by BlueZ might provide incomplete HID feature report on get operation.

#. If you do not want to use the root access to run the Python script, copy the provided udev rule from the :file:`99-hid.rules` file to the :file:`/etc/udev/rules.d` and reconnect the device.
#. If you want to connect to a device with a different Vendor or Product ID other than the one specified in the file, use one of the following options:

   * Run the script with the root permission.
   * Complete the following steps to run the script without root permission:

     a. Add a new entry to the :file:`99-hid.rules` file with your Vendor and Product ID.
     #. Copy the provided udev rule from the :file:`99-hid.rules` file to the :file:`/etc/udev/rules.d`.
     #. Reconnect the device.

   Vendor and Product ID can be specified in the configuration file related to the nRF Desktop application.
   The following examples shows the entry to add to the :file:`99-hid.rules` file to add device connected with USB and Bluetooth:

   .. parsed-literal::
      :class: highlight

      Device connected using USB:
      ATTRS{idVendor}=="my Vendor ID", ATTRS{idProduct}=="my Product ID", MODE="0666", SYMLINK+="nrf52-desktop-my-dev-name"

      Device connected using Bluetooth:
      ATTRS{name}=="Name of my Bluetooth device ", SUBSYSTEMS=="input", MODE="0666", SYMLINK+="nrf52-desktop-my-dev-name"

#. If you want to display an LED stream based on sound data, you must also install the additional requirements using the following commands:

   .. parsed-literal::
      :class: highlight

      sudo apt-get install portaudio19-dev python3-pyaudio
      pip3 install --user -r requirements_music_led_stream.txt

  For more detailed information about LED stream functionality, see the `Playing LED stream`_ section.

Stopping fwupd daemon
=====================

The :ref:`nrf_desktop_config_channel` is also used in the `Linux Vendor Firmware Service (LVFS) <LVFS_>`_ by the Nordic HID plugin of ``fwupd``.
Several Linux-based operating systems run the ``fwupd`` daemon in the background to manage firmware updates.

Configuring a connected HID device simultaneously with multiple host tools is not supported.
If multiple host tools configure a HID device at the same time, the configuration channel transport implementation in the firmware might mix requests and responses coming from various host tools.
Make sure to stop the ``fwupd`` daemon before using the HID configurator script.

Ubuntu example
--------------

To either stop or start the ``fwupd`` daemon on Ubuntu, run one of the following commands:

.. parsed-literal::
    :class: highlight

    sudo systemctl stop fwupd
    sudo systemctl start fwupd

To check the status of the ``fwupd`` daemon on Ubuntu, run the following command:

.. parsed-literal::
    :class: highlight

    systemctl status fwupd

Using the script
****************

See the script's help by running the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py -h

Display the list of all configurable devices that are connected to the host by running the script without providing additional arguments:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py

Perform the selected command on the connected device by using the following command syntax:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE COMMAND_NAME ...

.. note::
  The device can be identified by type, board name, or hardware ID (HW ID).
  The mapping from device type to board list is defined in :file:`NrfHidManager.py`.

A command may require additional, command-specific arguments.

Displaying the supported modules and options
============================================

The script can show the supported configuration channel modules and options for the connected device.
Use the following syntax to show the modules and options:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE show

Configuring device runtime options
==================================

The script can pass the configuration values to the linked firmware module using the ``config`` command.
Use the following syntax to display the list of modules that can have device runtime options configured:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE config -h

.. note::
  The list contains all the configurable modules used by nRF Desktop devices.
  Make sure that the selected module and option combination is supported by the configured device using ``show`` command.

Use the following syntax to display list of options for the given module that can have device runtime options configured:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE config MODULE_NAME -h

.. tip::
  The available configurable modules and options are defined by the :file:`nrf/scripts/hid_configurator/modules/module_config.py` file.

  You can add another configurable module to the file.
  Use the existing modules as examples.
  Make sure to also add the application firmware module as a :ref:`nrf_desktop_config_channel` listener, as described on the configuration channel page.

Customize the command with the following variables:

* ``MODULE_NAME`` - The third argument is used to pass the name of the module to be configured.
* ``OPTION_NAME`` - The fourth argument is used to pass the name of the option.
* ``VALUE`` - Optional fifth argument is used to pass a new value of the selected option.

To read the currently set value, pass the name of the module and the option to the ``config`` command, without providing any value:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE config MODULE_NAME OPTION_NAME

To write a new value for the selected option, pass the value as the fifth argument:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE config MODULE_NAME OPTION_NAME VALUE

.. important::
   If the module that is a configuration channel listener specifies its variant, you must refer to the module using the following syntax: ``module_name/variant``.
   For example, the :ref:`nrf_desktop_motion` variant that depends on the motion sensor model requires the following naming convention:

   * ``motion/paw3212``
   * ``motion/pmw3360``

Performing DFU
==============

The nRF Desktop application supports background DFU (Device Firmware Upgrade).
The image is passed to the device while the device is in normal operation.
The new image is stored on a dedicated update partition of the flash memory.
When the whole image is transmitted, the update process is completed during the next reboot of the device.

If the DFU process is interrupted, it can be resumed using the same image, unless the device restarts.
After the device reboots, the process always starts from the beginning.
For more information, see nRF Desktop's :ref:`nrf_desktop_dfu`.
The DFU functionality on the host computer is implemented in the :file:`nrf/scripts/hid_configurator/modules/dfu.py` file.

The ``dfu`` command reads the version of the firmware and the bootloader variant that are running on the device and compares them with the firmware version and the bootloader variant in the update image at the provided path.
If the process is to be continued, the script uploads the image data to the device.
When the upload is completed, the script reboots the device.

Customize the command with the following variable:

``UPDATE_IMAGE_PATH`` - Path to the DFU update file.

To perform a DFU operation, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE dfu UPDATE_IMAGE_PATH

.. note::
  Only devices with :ref:`nrf_desktop_dfu` support the ``dfu`` command.

Rebooting the device
====================

To perform a device reboot operation, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE fwreboot

.. note::
  Only devices with :ref:`nrf_desktop_dfu` support the ``fwreboot`` command.

Getting information about the firmware version
==============================================

To obtain information about the firmware running on the device, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE fwinfo

.. note::
  Only devices with :ref:`nrf_desktop_dfu` support the ``fwinfo`` command.

Getting identification information about the device
===================================================

To obtain information about the device's Vendor ID, Product ID, and generation, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE devinfo

.. note::
  Only devices with the :ref:`nrf_desktop_dfu` support the ``devinfo`` command.

The command can be used to obtain Vendor ID and Product ID of devices connected through an nRF Desktop dongle.
The generation is a string that allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
For more information about implementation in firmware, see nRF Desktop's :ref:`nrf_desktop_dfu`.

Playing LED stream
==================

The LED stream is a feature of nRF Desktop that allows you to send a stream of color data to be replayed on the device LED.
For more information about its implementation, see nRF Desktop's :ref:`nrf_desktop_led_stream`.
The LED stream functionality on the host computer is implemented by the following files:

* :file:`nrf/scripts/hid_configurator/modules/led_stream.py`
* :file:`nrf/scripts/hid_configurator/modules/music_led_stream.py`.

HID configurator's ``led_stream`` command starts the LED stream playback on the device.

Customize the command with the following variables:

* ``LED_ID`` - The third argument to the script is the ID of the LED on which the stream is to be replayed.
* ``FREQUENCY`` - The fourth argument to the script is the frequency at which the data is to be generated.
  The higher the frequency, the more often the colors change.
* ``--file WAVE_FILE`` - Optional argument for opening a wave file and using it to generate the stream of colors based on the sound data.

To start the LED stream payback, run the following command:

.. parsed-literal::
    :class: highlight

    python3 configurator_cli.py DEVICE led_stream LED_ID FREQUENCY --file WAVE_FILE

.. note::
  Only devices with :ref:`nrf_desktop_led_stream` support the ``led_stream`` commands.

Implementation details
**********************

Every nRF Desktop device must be discovered by the script before it can be configured.
The script fetches the hardware ID and board name and scans for the configurable modules.
For each module, it obtains the list of available options.
For details about options available within each module, see the module documentation.

From the user perspective, the nRF Desktop device is handled in the same way, regardless of it being connected to the host directly or through the nRF Desktop dongle.
During the device discovery, the script asks for the nRF Desktop peripherals connected through Bluetooth.
If the currently discovered device has connected peripherals, they are discovered and prepared for configuration.

The device discovery procedure is described on the :ref:`configuration channel documentation page <nrf_desktop_config_channel_device_discovery>`.
An example of implementation is available in the :file:`scripts/hid_configurator/NrfHidDevice.py` file.
The device discovery is implemented in the ``__init__`` function of the ``NrfHidDevice`` class.

.. note::
  The HID configurator script does not cache device discovery data.
  All of the connected nRF Desktop devices are rediscovered on each script invocation, right before the specified command is called.

Dependencies
************

The configuration channel has the following dependencies:

* `HIDAPI library`_
* `pyhidapi Python wrapper`_
