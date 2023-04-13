.. _nrf_desktop_dfu_lock:

DFU lock utility
################

.. contents::
   :local:
   :depth: 2

The DFU lock utility provides a synchronization mechanism for accessing the DFU flash.
It is needed for application configurations that support more than one DFU transport.

The DFU lock utility provides a basic synchronization API for all declared DFU owners.
Each DFU owner can be declared using the :c:struct:`dfu_lock_owner` structure.
The :c:func:`dfu_lock_claim` function is used to claim ownership over the DFU flash.
This function returns an error if the lock has already been taken by another DFU owner.
The :c:func:`dfu_lock_release` function is used by the current owner to release the DFU flash when it is no longer used.
This function returns an error on the release attempt that is not triggered by the current owner.
The :c:member:`dfu_lock_owner.owner_changed` callback is used to indicate the change in ownership.
The previous owner can use this callback for tracking the DFU flash status and the need to erase it before subsequent DFU attempts.

.. note::
    The nRF Desktop DFU transports must voluntarily take lock before accessing flash and release it after they stop using it.
    The DFU lock utility does not provide any protection against the DFU transport module that writes to flash memory without taking a lock.

Configuration
*************

Use the :ref:`CONFIG_DESKTOP_DFU_LOCK <config_desktop_app_options>` option to enable the utility.

Currently, the DFU lock utility is automatically used if you enable both supported DFU transports in your application:

* :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE <config_desktop_app_options>`
* :ref:`CONFIG_DESKTOP_DFU_MCUMGR_ENABLE <config_desktop_app_options>`

You can adjust the logging level of this utility by changing the :ref:`CONFIG_DESKTOP_DFU_LOCK_LOG_LEVEL <config_desktop_app_options>` Kconfig option.

Implementation details
**********************

The DFU lock utility uses the :ref:`zephyr:mutexes_v2` for synchronizing updates to its internal state.

API documentation
*****************

| Header file: :file:`applications/nrf_desktop/src/util/dfu_lock.h`
| Source files: :file:`applications/nrf_desktop/src/util/dfu_lock.c`

.. doxygengroup:: dfu_lock
   :project: nrf
   :members:
