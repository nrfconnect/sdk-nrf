.. _ug_fw_update_test_confirm:

Testing and confirming application updates
###########################################

.. contents::
   :local:
   :depth: 2

MCUboot update modes that support reverting can boot a new application image temporarily before making it permanent.
Run the new image in *test* mode, verify that it works, and then *confirm* it so that MCUboot keeps it across subsequent resets.

This page describes the test-and-confirm procedure at a high level.
For tool-specific instructions, see :ref:`dfu_tools_mcumgr_cli_image_mgmt`, :ref:`nrf_smp_svr_sample`, and the Zephyr :zephyr:code-sample:`smp-svr` sample documentation.

Checking update mode support
****************************

Before using the test-and-confirm procedure, verify that the selected MCUboot update mode supports reverting:

* Image swap modes support reverting to the last confirmed application image.
* Overwrite-only mode replaces the previous application image and does not support reverting.
  The new image does not require confirmation to remain installed.
* Direct-XIP mode supports the test-and-confirm procedure only when revert support is enabled with the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT` Kconfig option.

For a comparison of MCUboot update modes, see :ref:`ug_bootloader_main_config`.
Also check the documentation for your application, such as :ref:`nrf_desktop_dfu_mcumgr`, for application-specific behavior.

Configuring image confirmation
******************************

To confirm an application image from application code, enable the following Kconfig options in the application:

* :kconfig:option:`CONFIG_IMG_MANAGER`
* :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`

These options link the application with MCUboot's image management utilities, including the :c:func:`boot_write_img_confirmed` function.

If the application receives updates through the Simple Management Protocol (SMP), also enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG` Kconfig option and the related MCUmgr options described in :ref:`dfu_tools_mcumgr_cli`.

Testing and confirming an update
********************************

Marking an application image as *pending* requests a test boot without making the image permanent.
Until the running image is confirmed, MCUboot treats it as temporary and reverts to the last confirmed image on the next reset.

Complete the following steps:

#. Upload the signed application image to the secondary slot using SMP, :ref:`lib_dfu_target`, or another transport.

#. Mark the application image as pending for a test boot using one of the following methods:

   * From the application, call the :c:func:`boot_request_upgrade` function with ``BOOT_UPGRADE_TEST`` after writing the image to the secondary slot.
     Alternatively, call a helper function that performs the same operation, such as :c:func:`dfu_target_schedule_update` from the :ref:`lib_dfu_target` library.
   * From a host, use an SMP client, such as the ``mcumgr image test`` command or :ref:`requirements_nrf_util`.
     See :ref:`dfu_tools_mcumgr_cli_image_mgmt` for instructions.

#. Reset the device.
   MCUboot validates the application image according to its configuration and then boots it.

#. Run application-specific checks for connectivity, sensors, configuration, and other relevant functionality.

#. If a check fails, do not confirm the application image.
   Reset the device and verify that MCUboot reverts to the last confirmed image.

#. If all checks succeed, confirm the application image using one of the following methods:

   * From the application, call the :c:func:`boot_write_img_confirmed` function and verify that it returns ``0``.
     For a non-zero image index in a multi-image configuration, call the :c:func:`boot_write_img_confirmed_multi` function.
   * From a host, send an SMP image confirm command as described in :ref:`dfu_tools_mcumgr_cli_image_mgmt`.

#. Reset the device and verify that MCUboot boots the confirmed application image again.

.. note::
   Use the :c:func:`boot_is_img_confirmed` function to check the confirmation state of the running application image.
   You can safely call the :c:func:`boot_write_img_confirmed` function if the image is already confirmed.