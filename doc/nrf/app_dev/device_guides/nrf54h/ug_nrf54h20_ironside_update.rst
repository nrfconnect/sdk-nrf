.. _ug_nrf54h20_ironside_se_update:

Updating |ISE|
##############

.. contents::
   :local:
   :depth: 2

Updating the |ISE| is only possible after it has been initially :ref:`provisioned <ug_nrf54h20_SoC_binaries>` on the nRF54H20 SoC.

The update operation is initiated through its :ref:`update service <ug_nrf54h20_ironside_se_update_service>` at runtime by application firmware.

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

For instructions on how to provision the nRF54H20 with |ISE| for the first time, see :ref:`ug_nrf54h20_SoC_binaries`.

.. _ug_nrf54h20_ironside_se_updating:

Performing an update
********************

.. caution::
   You cannot update |ISE| from a SUIT-based (up to 0.9.6) to an |ISE|-based (20.0.0 and onwards) version.

.. _ug_nrf54h20_ironside_se_update_manual:

Manual update
=============

|NCS| supports the following methods for manually updating the |ISE| firmware on the nRF54H20 SoC:

* Using the ``west`` command provided by the |NCS|.
* Using the nRF Util `device <Device command overview_>`_ command.

.. important::
   Manual updates will replace existing firmware running in the Application core.
   User application firmware must be reprogrammed after successfully updating the device.

.. tabs::

  .. group-tab:: West

    The |NCS| defines the west ``ncs-ironside-se-update`` command to update |ISE| firmware on a device via the debugger.

    This command takes the nRF54H20 SoC binary ZIP file and uses the |ISE| update service to update both the |ISE| and |ISE| Recovery (or optionally just one of them):

    .. code-block:: console

      west ncs-ironside-se-update --allow-erase --zip <path_to_soc_binaries.zip>

    Use the ``--help`` option to see all possible options and descriptions of their use.

  .. group-tab:: nrf Util

    .. note::

      To use nRF Util for the update, you must install the nRF Util `device` command v2.14.0 or higher.
      See `Installing specific versions of nRF Util commands`_ for more information.

    The |ISE| update can be performed by manually executing nRF Util commands that perform the same steps that the ``west`` command performs.

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

.. _ug_nrf54h20_ironside_se_update_service:

|ISE| update service
********************

|ISE| provides an update service that allows local domains to trigger the update process of |ISE| itself.

The update service requires a release of |ISE| or the |ISE| Recovery image to be programmed within a valid memory range that is accessible by the Application core.
See :file:`nrf_ironside/update.h` for more details on the supported memory range.

After the Application has invoked the service, |ISE| will update on the next system reset.
The update can be verified by checking the listed versions in the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` on startup.

.. note::
   Updating through the service is limited to a single image at a time.

   Updating both |ISE| and |ISE| Recovery images requires performing two rounds of the update service procedure, in which the |ISE| Recovery image is updated first.

See the :zephyr:code-sample:`update application <nrf_ironside_update>` sample for an example on calling the service at runtime.

.. _ug_nrf54h20_ironside_se_update_architecture:

Architecture
************

The structure of the update procedure consists of the following steps:

1. The :zephyr:code-sample:`update application <nrf_ironside_update>` runs on the application core and communicates with |ISE| using the :ref:`update service <ug_nrf54h20_ironside_se_update_service>`.

#. The application invokes the IronSide SE update service and passes the parameters that correspond to the location of the HEX file (blob metadata) of the |ISE| firmware update in memory.

#. The |ISE| validates the update parameters and writes the update metadata to the Secure Information Configuration Registers (SICR).

#. After the service call completes, the IronSide SE firmware updates the internal state of the device.

#. The application prints the return value of the service call and outputs information from the update HEX file.

#. After a reset, the Secure Domain ROM (SDROM) detects the pending update through the SICR registers, verifies the update firmware signature, and installs the new firmware.

Once the operation has completed, you can read the boot report to verify that the update has taken place.
