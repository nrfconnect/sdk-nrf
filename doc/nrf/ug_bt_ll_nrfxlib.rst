.. _ug_bt_ll_nrfxlib:

nRF BLE Controller (experimental)
#################################

The |NCS| provides a *Bluetooth* LE Controller that is targeted at nRF devices.
This controller is designed to be robust and compatible to other link layers, similar to Nordic Semiconductor's SoftDevices.
See :ref:`nrfxlib:ble_controller` for more information about the controller.

The nRF BLE Controller is distributed as a set of precompiled linkable libraries that can be found in the `nrfxlib`_ repository.
Note that the nRF BLE Controller described in this document is different from the open source BLE Controller that is included in the Zephyr project.

This user guide describes how to use the nRF BLE Controller in the |NCS| and points out how it integrates with Zephyr.

.. _ug_bt_ll_nrfxlib_usage:

Using the nRF BLE Controller
****************************

The nRF BLE Controller can be used with any Bluetooth sample.
To enable it, set :option:`CONFIG_BT_LL_NRFXLIB` to ``y`` in the sample's :file:`prj.conf` file.

.. _ug_bt_ll_nrfxlib_drivers:

nRF BLE Controller driver support
*********************************

The nRF BLE Controller provides built-in drivers for flash, clock control, and random number generation (entropy).
When you enable the nRF BLE Controller, shims for these drivers are automatically selected and used instead of Zephyr's default drivers.

Thread safety
=============

The nRF BLE Controller is not re-entrant by default.
This means that to ensure thread safety, a multi-threading lock is required around calls to the controller's APIs.
The lock is implemented as a single shared semaphore that is taken and given before and after calling into the nRF BLE Controller.
This way, the nRF BLE Controller can alter how certain drivers and Bluetooth LE API calls behave.

In particular, the entropy driver locks the nRF BLE Controller API, even in the :cpp:func:`entropy_get_entropy_isr()` API that can be called from interrupts.
This means that a low priority interrupt calling the :cpp:func:`entropy_get_entropy_isr()` in a busy-wait manner can potentially block a higher priority interrupt.
