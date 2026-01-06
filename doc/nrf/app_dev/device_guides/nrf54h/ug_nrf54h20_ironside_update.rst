.. _ug_nrf54h20_ironside_update:

Programming and updating |ISE|
##############################

.. contents::
   :local:
   :depth: 2

|ISE| is released independently of the |NCS| release cycle and is provided as a ZIP archive that contains the following components:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Component
     - File
     - Description
   * - IronSide SE firmware
     - :file:`ironside_se.hex`
     - Used when bringing up a new DK and programming both the recovery firmware and |ISE| for the first time.
   * - IronSide SE update firmware
     - :file:`ironside_se_update.hex`
     - Used when updating |ISE|.
   * - IronSide SE Recovery update firmware
     - :file:`ironside_se_recovery_update.hex`
     - The recovery firmware, reserved for future recovery operations. Currently, it does not provide user-facing functionality. Used when updating the recovery firmware.
   * - Update application
     - :file:`update_application.hex`
     - The local domain :zephyr:code-sample:`update application <nrf_ironside_update>` that is used to perform an |ISE| update. See :ref:`ug_nrf54h20_ironside_se_update_manual`.

.. _ug_nrf54h20_ironside_se_programming:

Programming |ISE| on the nRF54H20 SoC
*************************************

For instructions on how to program |ISE|, see :ref:`ug_nrf54h20_SoC_binaries`.

By default, the nRF54H20 SoC uses the following memory and access configurations:

* MRAMC configuration: MRAM operates in Direct Write mode with READYNEXTTIMEOUT disabled.
* MPC configuration: All memory not reserved by Nordic firmware is accessible with read, write, and execute (RWX) permissions by any domain.
* TAMPC configuration: The access ports (AP) for the local domains are enabled, allowing direct programming of all the memory not reserved by Nordic firmware in the default configuration.


.. note::
   * The Radio Domain AP is only usable when the Radio domain has booted.
   * Access to external memory (EXMIF) requires a non-default configuration of the GPIO.CTRLSEL register.

You can protect global domain memory from write operations by configuring the UICR registers.
To remove these protections and disable all other protection mechanisms enforced through UICR settings, perform an ``ERASEALL`` operation.

.. _ug_nrf54h20_ironside_se_update:

Updating |ISE|
**************

|NCS| supports two methods for updating the |ISE| firmware on the nRF54H20 SoC:

* Using the ``west`` command.
  You can use the ``west`` command provided by the |NCS| to install the firmware update.
  For step-by-step instructions, see :ref:`ug_nrf54h20_ironside_se_update_west`.

* Using the nRF Util `device command <Device command overview_>`_.
  Alternatively, you can perform the update by manually executing the same steps that the ``west`` command performs.
  For step-by-step instructions, see :ref:`ug_nrf54h20_ironside_se_update_manual`.

.. caution::
   You cannot update |ISE| from a SUIT-based (up to 0.9.6) to an |ISE|-based (20.0.0 and onwards) version.

.. _ug_nrf54h20_ironside_se_update_west:

Updating using west
===================

To update the |ISE| firmware, you can use the ``west ncs-ironside-se-update`` command with the following syntax:

.. code-block:: console

   west ncs-ironside-se-update --zip <path_to_soc_binaries.zip> --allow-erase

The command accepts the following main options:

* ``--zip`` (required) - Sets the path to the nRF54H20 IronSide SE binaries ZIP file.
* ``--allow-erase`` (required) - Enables erasing the device during the update process.
* ``--serial`` - Specifies the serial number of the target device.
* ``--firmware-slot`` - Updates only a specific firmware slot (``uslot`` for |ISE| or ``rslot`` for |ISE| Recovery).
* ``--wait-time`` - Specifies the timeout in seconds to wait for the device to boot (default: 2.0 seconds).

.. _ug_nrf54h20_ironside_se_update_manual:

Updating manually
=================

The manual update process involves the following steps:

1. Executing the update application.
   The :zephyr:code-sample:`update application <nrf_ironside_update>` runs on the application core and communicates with |ISE| using the :ref:`update service <ug_nrf54h20_ironside_se_update_service>`.
   It reads the update firmware from memory and passes the update blob metadata to the |ISE|.
   The |ISE| validates the update parameters and writes the update metadata to the Secure Information Configuration Registers (SICR).

#. Installing the update.
   After a reset, the Secure Domain ROM (SDROM) detects the pending update through the SICR registers, verifies the update firmware signature, and installs the new firmware.

#. Completing the update.
   The system boots with the updated |ISE| firmware, and the update status in the boot report can be read to verify successful installation.

Updating manually using nRF Util
--------------------------------

To update |ISE|, you can use nRF Util instead of ``west ncs-ironside-se-update``.
To use nRF Util for the update, you must install the nRF Util `device` command v2.14.0 or higher.
See `Installing specific versions of nRF Util commands`_ for more information.

To perform the manual update process using nRF Util's `device command <Device command overview_>`_, complete the following steps:

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
