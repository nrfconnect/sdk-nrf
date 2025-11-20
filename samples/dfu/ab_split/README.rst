.. _ab_split_sample:

A/B with MCUboot and separated slots
####################################

.. contents::
   :local:
   :depth: 2

The A/B with MCUboot and separated slots sample demonstrates how to configure the application for updates using the A/B method using MCUboot.
This sample is a variant of the :ref:`A/B sample <ab_sample>`, where the application and radio images are not merged, but reside in separate MCUboot slots.
This split increases the number of memory areas that must be individually protected from accidental writes.
It also requires additional care when preparing updates to ensure that only a compatible set of slots is booted.
The additional dependency check during the boot process increases the time to boot the system.

It also includes an example to perform a device health check before confirming the image after the update.
You can update the sample using the Simple Management Protocol (SMP) with UART or Bluetooth® Low Energy.

To prevent the build system from merging slots, the sysbuild :kconfig:option:`SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY` option is disabled.
To enable manifest-based dependency management, the :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_UPDATES=y` option is enabled in the :file:`sysbuild.conf` file.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need the nRF Device Manager app for update over Bluetooth® Low Energy:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_


Overview
********

This sample demonstrates a firmware update using the A/B method.
This method allows you to store two copies of the application in non-volatile memory (NVM).
You can switch between these copies without performing a swap.
This solution significantly reduces the time during the update in which the device is unavailable.
The switch between images can be triggered by the application or, for example, by a hardware button.

This sample implements an SMP server.
SMP is a basic transfer encoding used with the MCUmgr management protocol.
For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample supports the following MCUmgr transports by default:

* Bluetooth
* Serial (UART)

A/B functionality
=================

When you use the A/B configuration with separated slots, the device provides two slots for each set of application and radio firmware: slot A and slot B.
The slots are equivalent, and the device can boot from either of them.
With MCUboot, this is achieved by using the Direct XIP feature.
By design, slot A of the application image boots slot A of the radio image.
This design implies that verifying the image pairs correctly requires a manifest-based dependency.
There can be only one image that includes the manifest TLV.
Its index is configured using :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX`.
By default, the application image index (``0``) is selected.

In this document, the following conventions are followed:

* The application image index (``0``) is referred to as the *manifest image*.
* The following names refer to the same images and are used interchangeably throughout the documentation:

  * *Slot 0*, *primary slot*, and *slot A*
  * *Slot 1*, *secondary slot*, and *slot B*

This configuration allows a background update of the non-active slot while the application runs from the active slot.
After the update is complete, the device can quickly switch to the updated slot on the next reboot.

The following conditions decide which slot is considered *active* and is booted on the next reboot:

1. If one of the slots of the manifest image contains a valid image, it is marked as valid only if all other images, described by the manifest are present and placed in the same slot as the manifest.
#. If one of the slots of the manifest image is not valid, the other slot is selected as active.
#. If both slots of the manifest image are valid, the slot marked as "preferred" is selected as active.
#. If both slots of the manifest image are valid and none is marked as *preferred*, the slot with the higher version number is selected as active.
#. If none of the above conditions is met, slot A is selected as active.
#. For all other images, the same slot is selected.

You can set the preferred slot using the ``boot_request_set_preferred_slot`` function.
If the :kconfig:option:`CONFIG_NRF_MCUBOOT_BOOT_REQUEST_PREFERENCE_KEEP` option is enabled, the slot preference remains persistent across reboots..
Otherwise, the slot preference is cleared on reboot.
To enable the persistence of a preferred slot, define a backup region for the bootloader request area by using the ``nrf,bootloader-request-backup`` chosen node in the devicetree.

Identifying the active slot
---------------------------

If the project uses the Partition Manager, the currently running slot can be identified by checking if ``CONFIG_NCS_IS_VARIANT_IMAGE`` is defined.
If it is defined, the application is running from slot B.
Otherwise, it is running from slot A.

If the project does not use the Partition Manager (a configuration currently supported only on the nRF54H20 SoC), you can identify the currently running slot by comparing the address referenced by ``zephyr,code-partition`` with the specific node addresses defined in the devicetree.
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

.. _ab_split_build_files:

Build files
-----------

This sample overrides the default build strategy, so application and radio images are built separately.
In this case, you must send the following files to the device when performing an update:


* :file:`build/mcuboot_secondary_app/zephyr/zephyr.signed.bin` - Contains the slot B of the application image.
  Upload this file to the secondary slot when the device is running from slot A.
* :file:`build/ipc_radio_secondary_app/zephyr/zephyr.signed.bin` - Contains the slot B of the radio image.
  Upload this file to the secondary slot when the device is running from slot A.
* :file:`build/ab/zephyr/zephyr.signed.bin` - Contains the slot A of the application image.
  Upload this file to the primary slot when the device is running from slot B.
* :file:`build/ipc_radio/zephyr/zephyr.signed.bin` - Contains the slot A of the radio image.
  Upload this file to the primary slot when the device is running from slot B.

User interface
**************

LED 0:
    This LED indicates that the application is running from slot A.
    It is controlled as active low.
    This means that it turns on once the application is booted and turns off in short intervals to blinks.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It remains off when the application is running from slot B.

LED 1:
    This LED indicates that the application is running from slot B.
    It is controlled as active low.
    This means that it turns on once the application is booted and turns off at short intervals to blinks.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It remains off when the application is running from slot A.

Button 0:
    By pressing this button, you select the non-active slot as the preferred slot for the next reboot.
    This preference applies only to the next boot and is cleared after the subsequent reset.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_N_BLINKS_ABSPLIT:

CONFIG_N_BLINKS - The number of blinks.
   This configuration option sets the number of times the LED corresponding to the currently active slot blinks (LED0 for slot A, LED1 for slot B).
   The default value of the option is set to ``1``, causing a single blink to indicate *Version 1*.
   You can increment this value to represent an update, such as set it to ``2`` to indicate *Version 2*.

.. _CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE_AB_SPLIT:

CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE - Enables emulation of a broken application that fails the self-test.
   This configuration option emulates a broken application that does not pass the self-test.

Additional configuration
========================

Check and configure the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option for the MCUboot library.
This configuration option sets the version to pass to imgtool when signing.
To ensure the updated build is preferred after a DFU, set this option to a higher version than the version currently running on the device.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/ab_split`

.. include:: /includes/build_and_run.txt

Testing
=======

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /app_dev/device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

Instead of using the :file:`dfu_application.zip` file, you can also send the appropriate binary file directly, as described in :ref:`ab_split_build_files`.
Make sure to select the correct file based on the currently running slot.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`MCUboot <mcuboot_index_ncs>`
