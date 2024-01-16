.. _nrf53_audio_app_fota:

Configuring and testing FOTA upgrades for nRF5340 Audio applications
####################################################################

.. contents::
   :local:
   :depth: 2

All nRF5340 Audio applications share the same configuration and testing procedures for FOTA upgrades.

Requirements for FOTA
*********************

To test Firmware Over-The-Air (FOTA), you need an Android or iOS device with the `nRF Connect Device Manager`_ app installed.

To enable the external flash DFU and do FOTA upgrades for the application core and the network core at the same time, you need an external flash shield.
See `Requirements for external flash memory DFU`_ in the nRF5340 Audio DK Hardware documentation for more information.

.. _nrf53_audio_app_configuration_configure_fota:

Configuring FOTA upgrades
*************************

.. caution::
	Firmware based on the |NCS| versions earlier than v2.1.0 does not support DFU.
	FOTA is not available for those versions.

	You can test performing separate application and network core upgrades, but for production, both cores must be updated at the same time.
	When updates take place in the inter-core communication module (HCI IPC), communication between the cores will break if they are not updated together.

You can configure Firmware Over-The-Air (FOTA) upgrades to replace the applications on both the application core and the network core.
The nRF5340 Audio applications support the following types of DFU flash memory layouts:

* Internal flash memory layout - which supports only single-image DFU.
* External flash memory layout - which supports :ref:`multi-image DFU <ug_nrf5340_multi_image_dfu>`.

The LE Audio Controller Subsystem for nRF53 supports both the normal and minimal sizes of the bootloader.
The minimal size is specified using the :kconfig:option:`CONFIG_NETBOOT_MIN_PARTITION_SIZE`.

Enabling FOTA upgrades
**********************

The FOTA upgrades are only available when :ref:`nrf53_audio_app_building_script`.
With the appropriate parameters provided, the :file:`buildprog.py` Python script will add overlay files for the given DFU type.
To enable the desired FOTA functions:

* To define flash memory layout, include the ``-m internal`` parameter for the internal layout (when using the ``release`` application version) or the ``-m external`` parameter for the external layout (when using either ``release`` or ``debug``).
* To use the minimal size network core bootloader, add the ``-M`` parameter.

For the full list of parameters and examples, see the :ref:`nrf53_audio_app_building_script_running` section.

FOTA build files
================

The generated FOTA build files use the following naming patterns:

* For multi-image DFU, the file is called ``dfu_application.zip``.
  This file updates two cores with one single file.
* For single-image DFU, the bin file for the application core is called ``app_update.bin``.
  The bin file for the network core is called ``net_core_app_update.bin``.
  In this scenario, the cores are updated one by one with two separate files in two actions.

See :ref:`app_build_output_files` for more information about the image files.

.. note::
   |nrf5340_audio_net_core_hex_note|

Entering the DFU mode
=====================

The |NCS| uses :ref:`SMP server and mcumgr <zephyr:device_mgmt>` as the DFU backend.
Unlike the CIS and BIS modes for gateway and headsets, the DFU mode is advertising using the SMP server service.
For this reason, to enter the DFU mode, you must long press **BTN 4** during each device startup to have the nRF5340 Audio DK enter the DFU mode.

To identify the devices before the DFU takes place, the DFU mode advertising names mention the device type directly.
The names follow the pattern in which the device *ROLE* is inserted before the ``_DFU`` suffix.
For example:

* Gateway: ``NRF5340_AUDIO_GW_DFU``
* Left Headset: ``NRF5340_AUDIO_HL_DFU``
* Right Headset: ``NRF5340_AUDIO_HR_DFU``

The first part of these names is based on :kconfig:option:`CONFIG_BT_DEVICE_NAME`.

.. note::
   When performing DFU for the nRF5340 Audio applications, there will be one or more error prints related to opening flash area ID 1.
   This is due to restrictions in the DFU system, and the error print is expected.
   The DFU process should still complete successfully.

.. _nrf53_audio_unicast_client_app_testing_steps_fota:

Testing FOTA upgrades
*********************

`nRF Connect Device Manager`_ can be used for testing FOTA upgrades.
The procedure for upgrading the firmware is identical for all applications.

Testing FOTA upgrades on a headset device
=========================================

You can test upgrading the firmware on both cores at the same time on a headset device by completing the following steps:

1. Make sure you have :ref:`configured the application for FOTA <nrf53_audio_app_configuration_configure_fota>`.
#. Install `nRF Connect Device Manager`_ on your Android or iOS device.
#. Connect an external flash shield to the headset.
#. Make sure the headset runs a firmware that supports DFU using external flash memory.
   One way of doing this is to connect the headset to the USB port, turn it on, and then run this command:

   .. code-block:: console

      python buildprog.py -c both -b debug -d headset --pristine -m external -p

   .. note::
      When using the FOTA related functionality in the :file:`buildprog.py` script on Linux, the ``python`` command must execute Python 3.

#. Use the :file:`buildprog.py` script to create a zip file that contains new firmware for both cores:

   .. code-block:: console

      python buildprog.py -c both -b debug -d headset --pristine -m external

#. Transfer the generated file to your Android or iOS device, depending on the DFU scenario.
   See the `FOTA build files`_ section for information about FOTA file name patterns.
   For transfer, you can use cloud services like Google Drive for Android or iCloud for iOS.
#. Enter the DFU mode by pressing and holding down **RESET** and **BTN 4** at the same time, and then releasing **RESET** while continuing to hold down **BTN 4** for a couple more seconds.
#. Open `nRF Connect Device Manager`_ and look for ``NRF5340_AUDIO_HL_DFU`` in the scanned devices window.
   The headset is left by default.
#. Tap on :guilabel:`NRF5340_AUDIO_HL_DFU` and then on the downward arrow icon at the bottom of the screen.
#. In the :guilabel:`Firmware Upgrade` section, tap :guilabel:`SELECT FILE`.
#. Select the file you transferred to the device.
#. Tap :guilabel:`START` and check :guilabel:`Confirm only` in the notification.
#. Tap :guilabel:`START` again to start the DFU process.
#. When the DFU has finished, verify that the new application core and network core firmware works properly.
