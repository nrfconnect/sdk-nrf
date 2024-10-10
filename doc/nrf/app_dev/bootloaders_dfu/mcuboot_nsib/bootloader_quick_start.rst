.. _bootloader_quick_start:

Quick start guide
#################

.. contents::
   :local:
   :depth: 2

This guide is intended to assist you in the initial setup and deployment of bootloaders within your project.
It covers essential concepts, practical steps for implementation, and references to further detailed documentation.

.. note::
  You can use this quick start guide in case you are not familiar with bootloaders and Device Firmware Upgrade (DFU).
  Otherwise, refer to further articles in the :ref:`ug_bootloader_mcuboot_nsib` documentation.

.. rst-class:: numbered-step

Learning resources
******************

In case you do not have experience with bootloaders, we recommend going through the following learning resources first:

* `Bootloaders and DFU/FOTA`_ - The page is a part of the `Nordic Developer Academy`_ courses and offers is an introduction to bootloaders, DFU, and various transport features available in the |NCS|.
* `Adding Device Firmware Update (DFU/FOTA) Support in nRF Connect SDK`_ - The webinar provides an overview of fundamental principles and best practices for adding DFU/FOTA support in an nRF Connect SDK-based firmware.

.. rst-class:: numbered-step

Documentation
*************

For more in-depth understanding of bootloader's capabilities, such as upgradable bootloader or encryption, refer to the following documentation:

* :ref:`Conceptual documentation <ug_bootloader_mcuboot_nsib>`

* Device-specific guides:

  * :ref:`ug_nrf52_developing`
  * :ref:`ug_nrf5340`
  * :ref:`nRF70 Series Firmware patch update<ug_nrf70_fw_patch_update>`
  * :ref:`ug_nrf91_config_build`

* Using DFU with cloud providers:

  * :ref:`lib_nrf_cloud`
  * :ref:`lib_aws_fota`
  * :ref:`lib_azure_fota`

.. rst-class:: numbered-step

Samples
*******

Explore MCUboot functionality using the samples provided.
Note that some samples are located in the `sdk-nrf`_ repository, while others are in `sdk-zephyr`_.
All supported samples are regularly tested to ensure reliability.

We recommend beginning with the :zephyr:code-sample:`smp-svr` sample and using :ref:`zephyr:mcu_mgr` for interaction from a host.
This setup supports both UART and Bluetooth® LE connections.

The following samples are supported:

* :zephyr:code-sample:`smp-svr`
* :ref:`zephyr:with_mcuboot`

.. rst-class:: numbered-step

APIs
****

The following APIs are essential for interacting with the bootloader or implementing alternative methods for transferring images to your device:

.. list-table:: Bootloaders supported by |NCS|
   :widths: auto
   :header-rows: 1

   * - API
     - Description
     - Supported transfer
   * - :ref:`zephyr:mcu_mgr`
     - Library in Zephyr that implementing the Simple Management Protocol (SMP), which is used to receive or send updates over different protocols.
     - Wired (SMP over UART or USB virtual serial port) and OTA (SMP over Bluetooth® LE)
   * - :ref:`lib_dfu_target`
     - Library in the |NCS| used to perform DFU for data from any source.
     - | The application provides the data and is responsible for receiving updates.
       | Transport mechanisms are application-specific.
   * - :ref:`lib_fota_download`
     - | Library in the nRF Connect SDK providing functions for downloading firmware files as upgrade candidates to the DFU target.
       | It is commonly used by IoT libraries, including the nRF Cloud library.
     - OTA (LTE, Wi-Fi)
   * - :ref:`zephyr:blinfo_api`
     - API that enables applications to access shared data from a bootloader.
     - --
   * - :ref:`zephyr:retention_api`
     - API that enables applications to trigger bootloader mode.
     - --

.. rst-class:: numbered-step

Task-specific guides
********************

The section lists step-by-step guides on solving specific tasks:

* :ref:`ug_bootloader_external_flash`
* The encrypted images documentation page in the :ref:`mcuboot_index_ncs`.

.. rst-class:: numbered-step

Supported features and configurations
*************************************

MCUboot is a customizable bootloader designed to meet specific requirements.
This page outlines the tested configurations.
For production builds, we recommend using the same set of configurations.

The following table is an overview of the currently supported bootloaders:

.. _app_bootloaders_support_table:

.. list-table:: Bootloaders supported by |NCS|
   :widths: auto
   :header-rows: 1

   * - Bootloader
     - Can be first-stage
     - Can be second-stage
     - Key type support
     - Public key revocation
     - SMP updates by the application
     - Downgrade protection
     - Versioning
     - Update methods (supported by |NCS|)
   * - :ref:`bootloader`
     - Yes
     - No
     - :ref:`See list <bootloader_signature_keys>`
     - :ref:`Yes <ug_fw_update_key_revocation>`
     - No
     - Yes
     - :ref:`Monotonic (HW) <bootloader_monotonic_counter>`
     - Dual-slot direct-xip
   * - :doc:`MCUboot <mcuboot:index-ncs>`
     - Yes
     - Yes (only with :ref:`NSIB <bootloader>` as first-stage)
     - :doc:`See imgtool <mcuboot:imgtool>`
     - No
     - Yes
     - Yes
     - :ref:`Monotonic (HW) <bootloader_monotonic_counter>`, :ref:`Semantic (SW) <ug_fw_update_image_versions_mcuboot>`
     - | Image swap - single primary
       | Dual-slot direct-xip

.. rst-class:: numbered-step

Tools
*****

You can use the following tools to interact with DFU:

.. list-table:: DFU tools
   :widths: auto
   :header-rows: 1

   * - DFU method
     - Description
     - Supported transfer
   * - :ref:`requirements_nrf_util`
     - Multi-purpose command line tool that can be used to send, for example, SMP-formatted updates to MCUmgr.
     - Wired (SMP over UART or USB).
   * - Cloud interfaces
     - | Each cloud has its own interface for sending updates to devices.
       | For details, refer to the documentation for :ref:`lib_nrf_cloud`, :ref:`lib_aws_fota`, or :ref:`lib_azure_fota`.
     - Wired (SMP over UART or USB) and OTA (SMP over Bluetooth® LE).
   * - `nRF Connect Device Manager`_
     - | Mobile application designed for sending SMP updates over Bluetooth® LE.
       | It also supports additional SMP features.
     - OTA (SMP over Bluetooth® LE)
   * - `nRF Connect for Mobile`_
     - General purpose mobile application for sending SMP updates over Bluetooth® LE and other functionalities.
     - OTA (SMP over Bluetooth® LE)
   * - :ref:`zephyr:mcumgr_smp_protocol_specification`
     - | SMP Client operates on a microcontroller.
       | For a Bluetooth® LE example, refer to :ref:`bluetooth_central_dfu_smp`.
       | SMP Client can also be implemented using the :ref:`zephyr:mcu_mgr` library.
     - Wired (SMP over UART or USB) and OTA (SMP Client over Bluetooth® LE)
