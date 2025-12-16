.. _ug_nrf54h20_ironside_update:
.. _ug_nrf54h20_ironside_se_update:

Updating |ISE|
##############

.. contents::
   :local:
   :depth: 2

.. caution::
   You cannot update |ISE| from a SUIT-based (up to 0.9.6) to an |ISE|-based (20.0.0 and onwards) version.

The application initiates the update operation at runtime through the |ISE|'s :ref:`update service <ug_nrf54h20_ironside_se_update_service>`.

.. _ug_nrf54h20_ironside_se_deliverables:

Release package
***************

The |ISE| is released independently of the |NCS| release cycle and is provided as a ZIP archive.

The archive is used to update the existing |ISE| firmware on the nRF54H20 and consists of the following components:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Component
     - File
     - Description
   * - IronSide SE firmware
     - :file:`ironside_se.hex`
     - Used when provisioning a new DK with |ISE| and |ISE| Recovery firmware for the first time.
   * - IronSide SE update firmware
     - :file:`ironside_se_update.hex`
     - Used when updating |ISE|.
   * - IronSide SE Recovery update firmware
     - :file:`ironside_se_recovery_update.hex`
     - The recovery firmware, reserved for future recovery operations. Currently, it does not provide user-facing functionality. Used when updating the recovery firmware.
   * - Update application
     - :file:`update_application.hex`
     - The local domain :zephyr:code-sample:`update application <nrf_ironside_update>` that is used to perform an |ISE| update. See :ref:`ug_nrf54h20_ironside_se_update_architecture` for details on its role.

For more information on |ISE| release binaries, see :ref:`abi_compatibility`.

For instructions on how to provision the nRF54H20 SoC with |ISE| for the first time, see :ref:`ug_nrf54h20_SoC_binaries`.

.. _ug_nrf54h20_ironside_se_updating:

Performing an update
********************

.. note::
   You can update the |ISE| only on an nRF54H20 SoC that was initially :ref:`provisioned <ug_nrf54h20_SoC_binaries>` with it.

|ISE| supports being updated in the following ways:

* :ref:`Manual updates <ug_nrf54h20_ironside_se_update_manual>` with a debugger

.. _ug_nrf54h20_ironside_se_update_manual:

Manual update
=============

.. caution::
   Manual updates will replace existing firmware running in the Application core.
   User application firmware must be reprogrammed after successfully updating the device.

|NCS| supports the following methods for manually updating the |ISE| firmware on the nRF54H20 SoC:

.. tabs::

  .. group-tab:: West

    The |NCS| defines the west ``ncs-ironside-se-update`` command to update |ISE| firmware on a device via the debugger.

    This command takes the nRF54H20 Ironside SE binaries ZIP file and uses the |ISE| update service to update both the |ISE| and |ISE| Recovery (or optionally just one of them):

    .. code-block:: console

      west ncs-ironside-se-update --allow-erase --zip <path_to_soc_binaries.zip>

    Use the ``--help`` option to see all possible options and descriptions of their use.

  .. group-tab:: nrf Util

    .. note::

      To use nRF Util for the update, you must install the nRF Util `device` command v2.14.0 or higher.
      See `Installing specific versions of nRF Util commands`_ for more information.

    You can update |ISE| by manually executing nRF Util commands that perform the same steps that the ``west`` command performs.

    To perform the manual update process using nRF Util's `device <Device command overview_>`_ command, complete the following steps:

    1. Extract the update bundle:

       .. code-block:: console

          unzip <soc_binaries.zip> -d /tmp/update_dir

    #. Erase non-volatile memory:

       .. code-block:: console

          nrfutil device recover --serial-number <serial>

    #. Program the update application:

       .. code-block:: console

          nrfutil device program --firmware /tmp/update_dir/update/update_application.hex --serial-number <serial>

    #. Program the |ISE| update firmware:

       .. code-block:: console

          nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware /tmp/update_dir/update/ironside_se_update.hex --serial-number <serial>

    #. Reset the device to execute the update service:

       .. code-block:: console

          nrfutil device reset --serial-number <serial>

    #. Reset through Secure Domain to trigger the installation of the update:

       .. code-block:: console

          nrfutil device reset --reset-kind RESET_VIA_SECDOM --serial-number <serial>

    #. If you are updating both slots, complete the following additional steps:

       a. Program the |ISE| Recovery update firmware:

          .. code-block:: console

             nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware /tmp/update_dir/update/ironside_se_recovery_update.hex --serial-number <serial>

       #. Reset again to execute the update service:

          .. code-block:: console

             nrfutil device reset --serial-number <serial>

       #. Reset again through Secure Domain to trigger the installation of the update:

          .. code-block:: console

             nrfutil device reset --reset-kind RESET_VIA_SECDOM --serial-number <serial>


    #. Erase the update application (regardless of whether you update one or both slots):

       .. code-block:: console

          nrfutil device erase --all --serial-number <serial>

.. _ug_nrf54h20_ironside_se_update_architecture:

Architecture
************

The |ISE| update process starts when the application firmware invokes the :ref:`update service <ug_nrf54h20_ironside_se_update_service>` with the address of where the update release package has been written in MRAM.

.. _ug_nrf54h20_ironside_se_update_architecture_app:

Application procedure
=====================

The following describes the process for an |ISE| update from the point of view of the application:

1. The application MRAM is updated with the |ISE| update image.
#. It calls the |ISE| update service with the update image location.
#. It verifies that the update request is acknowledged.
#. It triggers a reset.
#. It checks the version in the boot report on startup.

.. _ug_nrf54h20_ironside_se_update_architecture_ise:

|ISE| procedure
===============

The |ISE| side of the update process involves both the |ISE| firmware and SDROM.

The following describes the update process in the |ISE| upon request:

#. The service receives an update request containing the location of the update image in MRAM.
#. The update request is validated.
#. The SICR registers are updated with the image metadata.
#. The service acknowledges the update request.
#. Normal operation continues until a reset is performed.

Once the device comes out of reset, SDROM sees the update metadata and does the following to verify and apply the update:

1. Enables write-protection on the update image and firmware contents.
#. Checks firmware metadata stored in SICR registers against address range and size constraints.
#. Verifies update version against current firmware to prevent downgrades.
#. Computes and validates digest of the public key.
#. Checks public key is not revoked.
#. Computes and validates digest of update firmware.
#. Verifies signature of the update firmware.
#. Updates SICR's update status with result.

If any of the above steps fail, the installation is aborted and the existing |ISE| is booted.
Otherwise, the update firmware's metadata is stored in the SICR and the new image is installed.

If the updated firmware is for the |ISE| Recovery, the device is reset into Safe Mode after installation.
When Safe Mode has acknowledged its update, the device is reset to boot back into the |ISE| context.

On boot, |ISE| reads the update result from the SICR update status register and writes the value into the boot report.

.. note::
   |ISE| does not delete the update image contents from MRAM after a successful update.
