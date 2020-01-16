.. _nrf_desktop:

nRF Desktop
###########

nRF52 Desktop is a reference design of a HID device that is connected to a host through BLE or USB, or both.
Depending on the configuration, this application can function as a mouse, a gaming mouse, or a keyboard.

Overview
********

The nRF Desktop project supports common input HW interfaces like motion sensors, rotation sensors, and buttons scanning module.
The firmware can be configured at runtime using a dedicated configuration channel established with the HID feature report.
The same channel is used to transmit DFU packets.

.. note::
    The code is currently work-in-progress and as such is not fully functional, verified, or supported for product development.

Firmware architecture
=====================

The nRF Desktop application design aims at configurability and extensibility while being responsive.

The application architecture is modular and event-driven.
This means that parts of application functionality were separated into isolated modules that communicate with each other using application events.
Application events are handled by the :ref:`event_manager`.
Modules register themselves as listeners of events they need to react on.

The following figure shows the nRF Desktop modules and their relation with other components and the :ref:`event_manager`.

.. figure:: /images/nrf_desktop_arch.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (overview)

   Application high-level design overview (click to enlarge)

Module usage per hardware type
------------------------------

Since the application architecture is uniform and the code is shared, the set of modules in use depends on the selected device role.
A different set of modules is enabled when the application is working as a mouse, a keyboard, or as a dongle.

Gaming mouse module set
~~~~~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a gaming mouse.

.. figure:: /images/nrf_desktop_arch_gmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (gaming mouse)

   Application configured as a gaming mouse (click to enlarge)

Desktop mouse module set
~~~~~~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a desktop mouse.

.. figure:: /images/nrf_desktop_arch_dmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (desktop mouse)

   Application configured as a desktop mouse (click to enlarge)

Keyboard module set
~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a keyboard.

.. figure:: /images/nrf_desktop_arch_kbd.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (keyboard)

   Application configured as a keyboard (click to enlarge)

Dongle module set
~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a dongle.

.. figure:: /images/nrf_desktop_arch_dongle.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (dongle)

   Application configured as a dongle (click to enlarge)

Thread usage
------------

The application limits the number of threads used to the minimum and does not use user-space threads.
Most of the application activity happens in the context of the system workqueue thread, either through scheduled work objects or through the event manager callbacks (executed from the system workqueue thread).


Integrating your own hardware
=============================

For detailed information about the application configuration and how to port it to a different hardware platform, see the following pages.

.. toctree::
    :maxdepth: 1

    doc/porting_guide.rst

.. _nrf_desktop_requirements:

Requirements
************

The project comes with configuration files for the following boards:

    * |nRF52840DK| - a sample where the mouse is emulated using DK buttons
    * PCA20041 - nRF Desktop gaming mouse reference design
    * PCA20037 - nRF Desktop keyboard reference design
    * PCA20044 - nRF Desktop casual mouse reference design (nRF52832)
    * PCA20045 - nRF Desktop casual mouse reference design (nRF52810)
    * PCA10059 - nRF Desktop dongle reference design

The application was designed to allow easy porting to new hardware.
Check :ref:`porting_guide` for more information.

User interface
**************

The nRF Desktop devices provide user input to the host in the same way as other mice and keyboards, using the following connection options:

    * :ref:`nrf_desktop_usb`
    * :ref:`nrf_desktop_ble`


.. _nrf_desktop_usb:

Connection through USB
======================

The nRF Desktop devices use the USB HID class.
No additional software or drivers are required.

An example of the device that uses the connection through USB is the nRF Desktop dongle.
It receives data from the peripherals connected through Bluetooth Low Energy and forwards the data to the host.

Battery-powered devices with rechargeable batteries are charged through USB.

The devices that are connected both wirelessly and through USB at the same time provide their input only through the USB connection.
If the device is disconnected from USB, it automatically switches to sending the data wirelessly using Bluetooth Low Energy.


.. _nrf_desktop_ble:

Connection through BLE
======================

When turned on, the nRF Desktop peripherals are advertising until they go to the suspended state or connect through Bluetooth.
The peripheral supports one wireless connection at a time, but it may be bonded with multiple peers.

The nRF Desktop Bluetooth Central device scans for all bonded peripherals that are not connected.
The scanning is interrupted when any device connected to the dongle through Bluetooth is in use.
Continuing the scanning in such scenario would cause report rate drop.

The scanning starts automatically when one of the bonded peers disconnects.
It also takes place periodically when a known peer is not connected.

The peripheral connection can be based on standard BLE connection parameters or on BLE with Low Latency Packet Mode (LLPM).
LLPM is a proprietary Bluetooth extension from Nordic Semiconductor.
It can be used only if it is supported by both connected devices (desktop mice do not support it).
LLPM enables sending data with high report rate (up to 1000 reports per second), which is not supported by the standard BLE.


BLE peer control
----------------

A connected BLE peer device can be controlled using predefined buttons or button combinations.
There are several peer operations available.

The application distinguishes between the following button press types:

* Short -- Button pressed for less than 0.5 seconds.
* Standard -- Button pressed for more than 0.5 seconds, but less than 5 seconds.
* Long -- Button pressed for more than 5 seconds.
* Double -- Button pressed twice in quick succession.

The peer operation states provide visual feedback through LEDs (if the device has LEDs).
Each of the states is represented by separate LED color and effect.
The LED colors and effects are described in the :file:``led_state_def.h`` file located in the board-specific directory in the application configuration folder.

Gaming mouse
~~~~~~~~~~~~

The following predefined hardware interface elements are assigned to peer control operations for the gaming mouse:

Hardware switch:
    * The switch is located next to the optical sensor.
    * You can set the switch in the following positions:

        * Top position: Select the dongle peer.
        * Middle position: Select the BLE peers.
        * Bottom position: Mouse turned off.

      When the dongle peer is selected, the peer control is disabled until the switch is set to another position.

Precision Aim button:
    * The button is located on the left side of the mouse, in the thumb area.
    * Short-press to initialize the peer selection.
      During the peer selection:

        1. Short-press to select the next peer.
        #. Double-press to confirm the peer selection.
           The peer is changed after the confirmation.

    * Long-press to initialize the peer erase, then double-press to confirm the operation.
      |nRF_Desktop_confirmation_effect|
    * |nRF_Desktop_cancel_operation|

Desktop mouse
~~~~~~~~~~~~~

The following predefined buttons are assigned to peer control operations for the desktop mouse:

Scroll wheel button:
    * Long-press to initialize and confirm the peer erase.
      The scroll must be pressed before the mouse is powered up.

        * |nRF_Desktop_confirmation_effect|

    * |nRF_Desktop_cancel_operation|

Keyboard
~~~~~~~~

The following predefined buttons or button combinations are assigned to peer control operations for the keyboard:

Page Down key:
    * The Page Down key must be pressed while keeping the Fn modifier key pressed.
    * Short-press the Page Down key to initialize the peer selection.
      During the peer selection:

        1. Short-press to select the next peer.
        #. Double-press to confirm the peer selection.
           The peer is changed after the confirmation.

    * Long-press to initialize the peer erase, then double-press to confirm the operation.

        * |nRF_Desktop_confirmation_effect|

    * |nRF_Desktop_cancel_operation|

Dongle
~~~~~~

The following predefined buttons are assigned to peer control operations for the dongle:

SW1 button:
    * Long-press to initialize peer erase, then double-press to confirm the operation.
      After the confirmation, all the Bluetooth bonds are removed.
    * Short-press to start scanning for both bonded and not bonded Bluetooth peripherals.
      The scan is interrupted if another peripheral connected to the dongle is in use.
    * |nRF_Desktop_cancel_operation|


Power Management
================

Reducing power consumption is important for every battery-powered device.

The nRF Desktop peripherals are either suspended or powered off when they are not in use for a predefined amount of time:

    * In the suspended state, the device maintains the active connection.
    * In the powered off state, the CPU is switched to the off mode.
    * In both cases, most of the functionalities are disabled.
      For example, LEDs are turned off and advertising is stopped.

The predefined amount of time can be specified in ``CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT``, and by default it is set to 120 seconds.
Moving the mouse or pressing any button wakes up the device and turns on the disabled functionalities.

.. note::
    When the gaming mouse is powered from the USB, the power down timeout functionality is disabled.

Configuration channel
=====================

The nRF Desktop devices support additional operations, like firmware upgrade or configuration change.
The support is implemented through the :ref:`config_channel`.

The host can use dedicated Python scripts to exchange the data with an nRF Desktop peripheral.
For detailed information, see the ``hid_configurator`` documentation.

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built in the same way to any other NCS application or sample.

.. include:: /includes/build_and_run.txt

When building the application, you can configure the project-specific build type.

.. _nrf_desktop_build_types:

Build types
===========

The build type enables you to select a set of configuration options without making changes to the ``.conf`` files.
Selecting build type is optional.
However, the ``ZDebug`` build type is used by default if no build type is explicitly selected.

The project supports the following build types:

    * ``ZRelease`` -- Release version of the application with no debugging features.
    * ``ZDebug`` -- Debug version of the application; the same as the ``ZRelease`` build type, but with debug options enabled.
    * ``ZDebugWithShell`` -- ``ZDebug`` build type with the shell enabled.
    * ``ZReleaseMCUBoot`` -- ``ZRelease`` build type with the support for MCUBoot enabled.
    * ``ZDebugMCUBoot`` -- ``ZDebug`` build type with the support for MCUBoot enabled.

Not every board mentioned in :ref:`nrf_desktop_requirements` supports every build type.
If the given build type is not supported on the selected board, an error message will appear when building.
For example, if the ``ZDebugWithShell`` build type is not supported on the selected board, the following notification appears:

.. code-block:: console

  Configuration file for build type ZDebugWithShell is missing.

See :ref:`porting_guide` for detailed information about the application configuration.

Selecting build type in SES
---------------------------

To select the build type in SEGGER Embedded Studio:

1. Go to :guilabel:`Tools` -> :guilabel:`Options...` -> :guilabel:`nRF Connect`.
#. Set ``Additional CMake Options`` to ``-DCMAKE_BUILD_TYPE=selected_build_type``.
   For example, for ``ZRelease`` set the following value: ``-DCMAKE_BUILD_TYPE=ZRelease``.
#. Reload the project.

The changes will be applied after reloading.

Selecting build type from command line
--------------------------------------

To select the build type when building the application from command line, specify the build type by adding the ``-- -DCMAKE_BUILD_TYPE=selected_build_type`` to the ``west build`` command.
For example, you can build the ``ZRelease`` firmware for the PCA20041 board by running the following command in the ``applications/nrf_desktop`` directory:

.. code-block:: console

   west build -b nrf52840_pca20041 -d build_pca20041 -- -DCMAKE_BUILD_TYPE=ZRelease

The ``build_pca20041`` parameter specifies the output directory for the build files.

Testing
=======

The application may be built and tested in various configurations.
The following procedure refers to the scenario where the gaming mouse (PCA20041) and the keyboard (PCA20037) are connected simultaneously to the dongle (PCA10059).

After building the application with or without specifying the :ref:`nrf_desktop_build_types`, test the nRF Desktop application by performing the following steps:

1. Program the required firmware to each device.
#. Turn on both the mouse and the keyboard.
   The LED on the keyboard and the LED on the mouse (located under the logo) both start breathing.
#. Plug the dongle to the USB port.
   The blue LED on the dongle starts breathing.
   This indicates that the dongle is scanning for peripherals.
#. Wait for the establishment of the Bluetooth connection, which happens automatically.
   After the Bluetooth connection is established, the LEDs stop breathing and remain turned on.
   The devices can then be used simultaneously.

   .. note::
        You can manually start the scanning for new peripheral devices by pressing the `SW1` button on the dongle for a short time.
        This might be needed if the dongle does not connect with all the peripherals before timeout.
        The scanning is interrupted after a predefined amount of time, because it negatively affects the performance of already connected peripherals.

#. Move the mouse and press any key on the keyboard.
   The input is reflected on the host.

    .. note::
        When a configuration with debug features is enabled, for example the logger and assertions, the gaming mouse report rate can be significantly lower.

        Make sure that you use the release configurations before testing the mouse report rate.
        For the release configurations, you should observe a 500-Hz report rate when both mouse and keyboard are connected and a 1000-Hz rate when only mouse is connected.

#. Switch the Bluetooth peer on the gaming mouse by pressing the Precise Aim button.
   The gaming mouse LED color changes from red to green and the LED starts blinking rapidly.
#. Press the Precise Aim button twice quickly to confirm the selection.
   After the confirmation, the LED starts breathing and the mouse starts the Bluetooth advertising.
#. Connect to the mouse with an Android phone, a laptop, or any other Bluetooth Central.
   After the connection is established, you can use the mouse with the connected device.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`event_manager`
    * :ref:`profiler`
    * :ref:`hids_readme`
    * :ref:`hids_c_readme`
    * :ref:`nrf_bt_scan_readme`
    * :ref:`gatt_dm_readme`
    * ``drivers/sensor/paw3212``
    * ``drivers/sensor/pmw3360``

.. |nRF_Desktop_confirmation_effect| replace:: After the confirmation, Bluetooth advertising using a new local identity is started.
   When a new Bluetooth Central device successfully connects and bonds, the old bond is removed and the new bond is used instead.
   If the new peer does not connect in the predefined period of time, the advertising ends and the application switches back to the old peer.

.. |nRF_Desktop_cancel_operation| replace:: You can cancel the ongoing peer operation with a standard button press.
