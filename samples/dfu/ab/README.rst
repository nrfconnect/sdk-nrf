.. _ab_sample:

A/B with MCUboot
################

.. contents::
   :local:
   :depth: 2

The A/B with MCUboot sample demonstrates how to configure the application for updates using the A/B method using MCUboot.
It also includes an example to perform a device health check before confirming the image after the update.

You can update the sample using the Simple Management Protocol (SMP) with UART or Bluetooth® Low Energy.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need the nRF Device Manager app for update over Bluetooth Low Energy:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_


Overview
********

This sample demonstrates firmware update using the A/B method.
This method allows two copies of the application in the NVM memory.
It is possible to switch between these copies without performing a swap, which significantly reduces time of device's unavailability during the update.
The switch between images can be triggered by the application or, for example, by a hardware button.

This sample implements an SMP server.
SMP is a basic transfer encoding used with the MCUmgr management protocol.
For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample supports the following MCUmgr transports by default:

* Bluetooth
* Serial (UART)

A/B functionality
=================

When the A/B functionality is used, the device has two slots for the application: slot A and slot B.
The slots are equivalent, and the device can boot from either of them.
In the case of MCUboot, this is achieved by using the Direct XIP feature.
Thus, note that the terms slot 0, primary slot, slot A and slot 1, secondary slot, slot B are used interchangeably throughout the documentation.
This configuration allows a background update of the non-active slot while the application runs from the active slot.
After the update is complete, the device can quickly switch to the updated slot on the next reboot.

The following conditions decide which slot will be booted (active) on the next reboot:

1. If one of the slots is not valid, the other slot is selected as active.
#. If both slots are valid, the slot marked as "preferred" is selected as active.
#. If both slots are valid and none is marked as "preferred," the slot with the higher version number is selected as active.
#. If none of the above conditions is met, slot A is selected as active.

You can set the preferred slot using the ``boot_request_set_preferred_slot`` function.
Currently, this only sets the boot preference for a single reboot.

Identifying the active slot
---------------------------

If the project uses the Partition Manager, the currently running slot can be identified by checking if ``CONFIG_NCS_IS_VARIANT_IMAGE`` is defined.
If it is defined, the application is running from slot B.
Otherwise, it is running from slot A.

If the project does not use the Partition Manager (a configuration currently only supported on the nRF54H20), the currently running slot can be identified by comparing the address pointed `zephyr,code-partition` to specific node addresses defined in the device tree.
The following node partitions are used by default:

* ``cpuapp_slot0_partition`` - Application core, slot A
* ``cpuapp_slot1_partition`` - Application core, slot B
* ``cpurad_slot0_partition`` - Radio core, slot A
* ``cpurad_slot1_partition`` - Radio core, slot B

For example, verifying that the application is running from slot A can be done by using the following macro:

.. code-block:: c

    #define IS_RUNNING_FROM_SLOT_A \
        (FIXED_PARTITION_NODE_OFFSET(DT_CHOSEN(zephyr_code_partition)) == \
         FIXED_PARTITION_OFFSET(cpuapp_slot0_partition))

.. _ab_build_files:

Build files
-----------

When building for the nRF54H20, the merge slot feature is used if Direct XIP is enabled.
This means that, for both slot A and slot B, the application image and the radio image are merged and treated as a single image by MCUboot.
In this case, the following files should be sent to the device when performing an update:

* :file:`build/zephyr_secondary_app.merged.bin` - Contains the slot B image.
  This file should be uploaded to the secondary slot when the device is running from slot A.
* :file:`build/zephyr.merged.bin` - Contains the slot A image.
  This file should be uploaded to the primary slot when the device is running from slot B.

If building on other supported platforms, where there is no separate radio core, only the application core is updated.
In this case, the following MCUboot files for the application image are used:

* :file:`build/mcuboot_secondary_app/zephyr/zephyr.signed.bin` -  Contains the slot B image.
  This file should be uploaded to the secondary slot when the device is running from slot A.
* :file:`build/ab/zephyr/zephyr.signed.bin` - Contains the slot A image.
  This file should be uploaded to the primary slot when the device is running from slot B.

User interface
**************

LED 0:
    This LED indicates that the application is running from slot A.
    It is controlled as active low, meaning it will turn on once the application is booted and blinks (turns off) in short intervals.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It will remain off if the application is running from slot B.

LED 1:
    This LED indicates that the application is running from slot B.
    It is controlled as active low, meaning it will turn on once the application is booted and blinks (turns off) in short intervals.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It will remain off if the application is running from slot A.

Button 0:
    By pressing this button, the slot A will be selected as the preferred slot on the next reboot.
    This preference applies only to the next boot and is cleared after the subsequent reset.

Button 1:
    By pressing this button, the slot B will be selected as the preferred slot on the next reboot.
    This preference applies only to the next boot and is cleared after the subsequent reset.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _CONFIG_N_BLINKS:

CONFIG_N_BLINKS - The number of blinks.
   This configuration option sets the number of times the LED corresponding to the currently active slot blinks (LED0 for slot A, LED1 for slot B).
   The default value of the option is set to ``1``, causing a single blink to indicate *Version 1*.
   You can increment this value to represent an update, such as set it to ``2`` to indicate *Version 2*.

.. _CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE:

CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE - Enables emulation of a broken application that fails the self-test.
   This configuration option emulates a broken application that does not pass the self-test.

Additional configuration
========================

Check and configure the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` library Kconfig option specific to the MCUboot library.
This configuration option sets the version to pass to imgtool when signing.
To ensure the updated build is preferred after a DFU, set this option to a higher version than the version currently running on the device.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/ab`

.. include:: /includes/build_and_run.txt

Testing
=======

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /app_dev/device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

Instead of using the :file:`dfu_application.zip` file, you can also send the appropriate binary file directly, as described in :ref:`ab_build_files`.
Make sure to select the correct file based on the currently running slot.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`MCUboot <mcuboot_index_ncs>`
