.. _smp_svr_ext_xip:

nRF5340: SMP Server with external XIP
#####################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to split an application that partially resides on internal flash and Quad Serial Peripheral Interface (QSPI) flash by using the Simple Management Protocol (SMP) server.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample requires a connection to a computer with :file:`mcumgr` command-line tool's or a compatible MCUmgr client tool.

Overview
********

SMP is a basic transfer protocol used by the MCUmgr management library.
This sample shows how to divide an application into internal and external parts using the SMP server.
The internal part operates from the SoC's built-in storage, and the external part runs directly from external memory, using the XIP capability through a QSPI peripheral.

For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample enables the splitting by:

* Enabling the split XIP image feature through the :kconfig:option:`CONFIG_XIP_SPLIT_IMAGE` Kconfig option.
  This Kconfig option sets a build-system level support for image division and adapts the image-signing support for MCUboot.
* Using the project's CMake file which describes relocation of certain libraries or objects to an external XIP area.
* Using a linker script that describes the external QSPI area.
  (The execution code from the external memory should link to it.)
* Using the static partition manager description for appropriate memory mapping.

For details on SMP features implemented by the sample, see the Zephyr :zephyr:code-sample:`smp-svr` documentation.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/extxip_smp_svr`
.. include:: /includes/build_and_run.txt
Because sysbuild is currently not supported, you must add the ``--no-sysbuild`` argument when building the sample:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* --no-sysbuild

The |NCS| build system generates all essential binaries, including the application for internal and external QSPI flash, the networking stack, and the MCUboot.
The build process involves signing all the application binaries except for MCUboot which does not require signing in this configuration.

To upload MCUboot and a bundle of images to the nRF5340 SoC, use the ``west flash`` command.

For Thingy:53, however, the QSPI memory requires a dedicated configuration file for flashing.
In such a case, run the following command instead:

.. code-block:: console

    nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program hci_ipc/zephyr/merged_CPUNET.hex --verify
    nrfjprog -f NRF53 --sectorerase --program mcuboot/zephyr/zephyr.hex --verify
    nrfjprog -f NRF53 --sectorerase --program zephyr/internal_flash_signed.hex --verify
    nrfjprog -f NRF53 --qspisectorerase --program zephyr/qspi_flash_signed.hex --qspiini <path_to_sample>/Qspi_thingy53.ini --verify
    nrfjprog -f NRF53 --reset

Testing
=======

The ``smp_svr`` app is now ready to run.
Reset your board and test the app with the :file:`mcumgr` command-line tool's ``echo`` functionality.
This will send a string to the remote target device and have it echo it back:

.. tabs::

   .. group-tab:: Bluetooth®

      .. code-block:: console

         sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' echo hello
         hello

   .. group-tab:: Shell

      .. code-block:: console

         mcumgr --conntype serial --connstring "/dev/ttyACM2,baud=115200" echo hello
         hello

.. note::
   The :file:`mcumgr` command-line tool requires a connection string in order to identify the remote target device.
   The Bluetooth version of the sample uses a Bluetooth LE-based connection string.
   You might need to modify it depending on the Bluetooth LE controller you are using.

   In the following sections, ``<connection string>`` is used to represent the ``--conntype <type>`` and ``--connstring=<string>`` :file:`mcumgr` parameters.

Upload and map image to MCUboot slot
------------------------------------

For this sample configuration, MCUmgr supports the uploading of three target images, which allows you to update all parts of the firmware.

The MCUmgr ``image upload`` command has the optional ``-e -n <image>`` parameter, which lets you select the target image for upload.
When this parameter is not provided, ``0`` is assumed (interpreted as the default behavior), and MCUmgr uploads to ``image-1`` (MCUboot's secondary slot).

See the Partition manager (PM) label for slot-to-``<image>`` translation:

    +-------------------------+--------+-------------+---------------------------+
    | PM label                | Slot   | -n <image>  |       Firmware part       |
    +=========================+========+=============+===========================+
    | ``mcuboot_secondary``   | slot-1 |     0       | Internal application part |
    +-------------------------+--------+-------------+---------------------------+
    | ``mcuboot_secondary_1`` | slot-3 |     1       | Networking                |
    +-------------------------+--------+-------------+---------------------------+
    | ``mcuboot_secondary_2`` | slot-5 |     2       | QSPI application part     |
    +-------------------------+--------+-------------+---------------------------+

.. note::

   The ``-e`` option means "no erase", and is provided to the MCUmgr to prevent it from sending the erase command to the target before updating the image.
   This option is always needed when ``-n`` is used for image selection, as the erase command is hardcoded to erase slot-1 (``image-1``), regardless of which slot is uploaded at the time.

Upload the signed image
-----------------------

To upload the signed image, run the following command:

.. code-block:: console

   sudo mcumgr <connection string> image upload build/zephyr/<signed_upload.bin> -e -n <image>

Adjust the command depending on the case:

* Use the :file:`internal_flash_update.bin` file and ``<image> = 0`` to update the internal application image.
* Use the :file:`net_core_app_update.bin` file and ``<image> = 1`` to update the networking core image.
* Use the :file:`qspi_flash_update.bin` file and ``<image> = 2`` to update the QSPI application image.

.. note::

   The ``<image>`` values correspond to the internally (``0``) and externally (``2``) linked parts of the application.
   These two images are linked and must always be uploaded together.

Test the image
^^^^^^^^^^^^^^

To initiate the image swap with MCUboot, first you must test the image by obtaining its SHA with the following command:

.. code-block:: console

   imgtool verify build/zephyr/<signed_upload.bin>

Ensure it boots using the image digest fetched by running the following command:

.. code-block:: console

   sudo mcumgr <connection string> image test <hash of image>

The selected mode in this test case must be applied to all images that constitute the complete application.
Once applied, MCUboot will swap these images during the next reset.

.. note::

   The sample supports the MCUboot overwrite-only mode.
   This means that regardless of how the image is marked for the update (either as ``test`` or ``confirm``), the process is not affected.
   The old image is still overwritten by the content of the incoming image during the firmware update process.

Reset remotely
--------------

To check how MCUboot has upgraded the images, you can reset the device remotely by running the following command:

.. code-block:: console

   sudo mcumgr <connection string> reset

Dependencies
************

The sample uses the following Zephyr library:

* :ref:`zephyr:mcu_mgr`

It also uses the following |NCS| library:

* :ref:`partition_manager`
