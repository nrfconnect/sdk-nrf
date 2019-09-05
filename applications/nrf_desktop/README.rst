.. _nrf_desktop:

nRF Desktop
##########################

Nordic Semiconductor HID reference design.

Overview
********

nRF52 Desktop is a reference design of a HID device.

Depending on a configuration this application can function as mouse,
gaming mouse or keyboard connected to host via BLE and/or USB.

Project supports common input HW interfaces like motion sensors, rotation
sensors and buttons scanning module.
Firmware can be configured at run-time using dedicated configuration channel
realized with HID feature report. The same channel is used to pass DFU packets.

.. note::
    The code is currently work-in-progress and as such is not fully functional,
    verified, or supported for product development.


Requirements
************

The project contains configuration files for following boards:

    * PCA10056 (nRF52840 DK) - a sample where the mouse is emulated using DK buttons
    * PCA20041 - nRF Desktop gaming mouse reference design
    * PCA20037 - nRF Desktop keyboard reference design
    * PCA20044 - nRF Desktop casual mouse reference design (nRF52832)
    * PCA20045 - nRF Desktop casual mouse reference design (nRF52810)
    * PCA10059 - nRF Desktop dongle reference design

The application was designed to allow easy porting to a new HW.
Check :ref:`porting_guide` for information about the device porting steps.


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


Building and Running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built in the same way to any other NCS
application or sample.
.. include:: /includes/build_and_run.txt

Configuration option set depends on the selected build type.
The project supports the following types:

    * ZRelease - release version of the application with no debugging features
    * ZDebug - the same functionality as ZRelease but with debug options enabled
    * ZDebugWithShell - the same set as ZDebug plus shell is enabled
    * ZReleaseMCUBoot - the same set as ZRelease but support of MCUBoot is enabled
    * ZDebugMCUBoot - the same set as ZDebug but support of MCUBoot is enabled

Build type is specified using the cmake variable CMAKE_BUILD_TYPE.


Application configuration
*************************

For detailed information about the application configuration and how to port
it to a different hardware platform, see the following pages.

.. toctree::
    :maxdepth: 1

    doc/porting_guide.rst


Firmware architecture
*********************

The nRF Desktop application design aims at configurability and extensibility
while being responsive.

The application architecture is modular and event-driven.
This means that parts of application functionality were separated into isolated modules
that communicate with each other using application events.
Application events are handled by the :ref:`event_manager`. Modules register
themselves as listeners of events they need to react on.

The application limits the number of threads used to the minimum and does not
use user-space threads. Most of the application activity happens in the
context of the system workqueue thread via scheduled work objects or the event
manager callbacks (executed from the system workqueue thread).

Firmware modules:

.. toctree::
    :maxdepth: 1

    doc/hw_interface.rst
    doc/modules.rst


Testing
*******

The code is currently work-in-progress and as such is not fully functional,
verified, or supported for product development.
