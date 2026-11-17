.. _ug_fw_update_test_confirm:
.. _bootloader_dfu_test_confirm:

Test and confirm application updates
####################################

.. contents::
   :local:
   :depth: 2

When MCUboot is configured for image swap (or another mode with revert support), uploading a firmware image is not enough to make it permanent.
The recommended workflow is to boot the new image in *test* mode, let the application verify that it works, and only then *confirm* the image so MCUboot keeps it after the next reset.

.. note::
   Not every MCUboot mode uses the test-and-confirm cycle.

   In **overwrite-only** mode, the previous application is replaced when the upgrade runs and revert is not available.
   Confirmation is not required for permanence in the same way; see the :ref:`nrf_smp_svr_sample` testing notes when this mode is enabled.

   In some **direct-XIP** configurations, confirmation is not supported or behaves differently.
   Check the MCUboot mode selected in sysbuild and the documentation for your application (for example, :ref:`nrf_desktop_dfu_mcumgr`).
   For a comparison of upgrade modes, see :ref:`ug_bootloader_main_config`.

This page describes that procedure at a high level.
Tool-specific steps are covered in :ref:`dfu_tools_mcumgr_cli_image_mgmt`, :ref:`nrf_smp_svr_sample`, and the Zephyr :zephyr:code-sample:`smp-svr` sample documentation.

When the update is applied
**************************

A *test* upgrade tells MCUboot to swap to (or boot from) the new image on the next reset.
Until the running image is confirmed, MCUboot treats it as temporary: on a later reset it can revert to the previously working firmware.

You can request a test upgrade from the device or from a host, depending on your design:

* In the application, after the image has been written to the secondary slot, call :c:func:`boot_request_upgrade` with ``BOOT_UPGRADE_TEST``, or use a helper that does the same (for example, :c:func:`dfu_target_schedule_update` in the :ref:`lib_dfu_target` library).
* From a host, use an SMP client (for example, ``mcumgr image test`` or :ref:`requirements_nrf_util`) to mark the uploaded image as pending.
  See :ref:`dfu_tools_mcumgr_cli_image_mgmt`.

After requesting the test upgrade, reset the device.
MCUboot validates the new image and starts it if the signature, metadata, and version checks pass.

Recommended workflow
********************

The following sequence applies to typical application updates when revert is supported:

#. Upload the signed image to the secondary slot (SMP, :ref:`lib_dfu_target`, or another transport).

#. Mark the image for a *test* boot (pending), not a permanent upgrade.

#. Reset the device and let MCUboot start the new firmware.

#. In the new firmware, run application-specific checks (connectivity, sensors, configuration, and so on).

#. If the checks succeed, confirm the image by calling :c:func:`boot_write_img_confirmed` (or :c:func:`boot_write_img_confirmed_multi` for a non-zero image index).
   Alternatively, the host can send an SMP image confirm command; see :ref:`dfu_tools_mcumgr_cli_image_mgmt`.

If the image is never confirmed, MCUboot reverts on a subsequent reset (in modes that support revert).
If the new firmware fails before it can confirm itself, the device returns to the last confirmed image after reset.

.. note::
   You can call :c:func:`boot_is_img_confirmed` to see whether the currently running image is already permanent.
   :c:func:`boot_write_img_confirmed` is safe to call when the image is already confirmed.

Application configuration
*************************

To confirm images from application code, enable the following Kconfig options in the application:

* :kconfig:option:`CONFIG_IMG_MANAGER`
* :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`

These options link the application against MCUboot's image management utilities (including :c:func:`boot_write_img_confirmed`).

If updates are received over SMP in the device itself, also enable :kconfig:option:`CONFIG_MCUMGR_GRP_IMG` and related MCUmgr options as described in :ref:`dfu_tools_mcumgr_cli`.