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
---------------------

The nRF Desktop application design aims at configurability and extensibility while being responsive.

The application architecture is modular and event-driven.
This means that parts of application functionality were separated into isolated modules that communicate with each other using application events.
Application events are handled by the :ref:`event_manager`.
Modules register themselves as listeners of events they need to react on.

The application limits the number of threads used to the minimum and does not use user-space threads.
Most of the application activity happens in the context of the system workqueue thread via scheduled work objects or the event manager callbacks (executed from the system workqueue thread).

Firmware modules:

.. toctree::
    :maxdepth: 1

    doc/hw_interface.rst
    doc/modules.rst

Integrating your own hardware
-----------------------------

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

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built in the same way to any other NCS application or sample.

.. include:: /includes/build_and_run.txt

When building the application, you can configure the project-specific build type.

.. _nrf_desktop_build_types:

Build types
-----------

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
+++++++++++++++++++++++++++

To select the build type in SEGGER Embedded Studio:

1. Go to :guilabel:`Tools` -> :guilabel:`Options...` -> :guilabel:`nRF Connect`.
#. Set ``Additional CMake Options`` to ``-DCMAKE_BUILD_TYPE=selected_build_type``.
   For example, for ``ZRelease`` set the following value: ``-DCMAKE_BUILD_TYPE=ZRelease``.
#. Reload the project.

The changes will be applied after reloading.

Selecting build type from command line
++++++++++++++++++++++++++++++++++++++

To select the build type when building the application from command line, specify the build type by adding the ``-- -DCMAKE_BUILD_TYPE=selected_build_type`` to the ``west build`` command.
For example, you can build the ``ZRelease`` firmware for the PCA20041 board by running the following command in the ``applications/nrf_desktop`` directory:

.. code-block:: console

   west build -b nrf52840_pca20041 -d build_pca20041 -- -DCMAKE_BUILD_TYPE=ZRelease

The ``build_pca20041`` parameter specifies the output directory for the build files.

Testing
-------

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
