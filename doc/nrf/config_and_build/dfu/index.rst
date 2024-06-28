.. _app_dfu:
.. _ug_fw_update:

Device Firmware Updates
#######################

Device Firmware Update (DFU) is the procedure of upgrading the application firmware version on a device.
It consists of two primary steps:

1. Transferring the new firmware - A new firmware image is transferred to the device's chip.

#. Testing and booting - The bootloader then tests and boots the new firmware.

.. note::
  The choice of bootloader affects how firmware updates can be performed (except for the nRF54H20 SoC where only the Software Updates for Internet of Things (SUIT) is supported).
  While bootloader features (such as out-of-the-box support) for various DFU methods may vary, it is recognized that:

  * MCUboot and its supporting libraries and middleware are flexible and support various protocols and methods for firmware updates.
    However, the extent of supported features, like USB-DFU class, may vary and should be confirmed with the latest MCUboot documentation or source code.
  * The nRF Secure Immutable Bootloader (NSIB) allows for firmware updates but might not include comprehensive support for all types of firmware updates compared to MCUboot.

  This distinction is crucial for developing your firmware update strategy and selecting the appropriate bootloader for your device's needs.

Regarding bootloader and application roles, the testing and booting process depends on the choice of bootloader and the application design.
Generally, bootloaders support two types of updates:

* Direct updates, with an in-place substitution of the image.
  In this case, the bootloader transfers the update image.
* Background updates, where the updated image is obtained and stored by the application, but the update is completed by the bootloader after the device reboots.

In systems that use MCUboot, the application may be responsible for receiving update packages through Simple Management Protocol (SMP) and staging them for the bootloader.
The relation can be explained as follows:

* The bootloader manages the final steps of the DFU process and has ability to receive, verify, and activate either new or already-received firmware images.
* The application manages firmware updates by receiving and staging new firmware or candidate images, especially when using SMP, and then prepares them for installation by the bootloader.

With DFUs, you can transfer the updated images to the device in two ways:

* Wired, where updates are sent through a wired connection, like UART, or delivered by connecting a flash device.
* Over-the-air (OTA), where updates are sent through a wireless connection, like Bluetooth® Low Energy.

Based on these criteria, the |NCS| offers support for the following DFU alternatives:

* For sending updates to the receiving device:

  .. list-table:: DFU methods for sending updates
    :widths: auto
    :header-rows: 1

    * - Method
      - Description
      - Supported transfer
    * - :ref:`nRF Util <toolchain_management_tools>`
      - Multi-purpose command line tool that can be used to send SMP-formatted updates to MCUmgr, among other things.
      - Wired (SMP over UART or USB)
    * - Cloud interfaces
      - Each cloud comes with its specific interface to send updates to devices from the cloud. See documentation for :ref:`lib_nrf_cloud`, :ref:`lib_aws_fota` or :ref:`lib_azure_fota`.
      - OTA (LTE, Wi-Fi)
    * - :ref:`zephyr:mcumgr_cli`
      - Command line tool specifically for sending updates to the MCUmgr library.
      - Wired (SMP over UART or USB) and OTA (SMP over Bluetooth LE)
    * - `nRF Connect Device Manager`_
      - Mobile application for sending SMP updates over Bluetooth LE; can also be used for other SMP functionality.
      - OTA (SMP over Bluetooth LE)
    * - `nRF Connect for Mobile`_
      - General purpose mobile application that can be used for sending SMP updates over Bluetooth LE, among other things.
      - OTA (SMP over Bluetooth LE)
    * - :ref:`SMP Client <zephyr:mcumgr_smp_protocol_specification>`
      - SMP Client running on a microcontroller. For an example over Bluetooth LE, see :ref:`bluetooth_central_dfu_smp`. It can alternatively be implemented using the :ref:`zephyr:mcu_mgr` library.
      - Wired (SMP over UART or USB) and OTA (SMP Client over Bluetooth LE)

* For receiving updates on the device side:

  .. list-table:: DFU methods for receiving updates
    :widths: auto
    :header-rows: 1

    * - Method
      - Description
      - Supported transfer
    * - :ref:`zephyr:mcu_mgr`
      - Library in Zephyr that implements the Simple Management Protocol (SMP); used to receive or send updates over different protocols.
      - Wired (SMP over UART or USB) and OTA (SMP over Bluetooth LE)
    * - :ref:`lib_dfu_target`
      - Library in the |NCS| used to do DFU for data from any source.
      - Data is provided by the application and it is up to the application to receive the updates. Transport is application-specific.
    * - :ref:`lib_fota_download`
      - Library in the |NCS| that provides functions for downloading a firmware file as an upgrade candidate to the DFU target. The library is often used by IoT libraries, such as the :ref:`lib_nrf_cloud` library.
      - OTA (LTE, Wi-Fi)

For device-specific guides related to DFU, see the following pages:

* :ref:`Developing with nRF52 Series <ug_nrf52_developing_ble_fota>` - For how to do firmware over-the-air (FOTA) updates with nRF52 Series devices.

* :ref:`Developing with nRF5340 DK <ug_nrf53_developing_ble_fota>` - For how to do FOTA updates and serial recovery with the nRF5340 SoC.

  * :ref:`qspi_xip` - For external execute in place (XIP) for the nRF5340 SoC.

* :ref:`ug_nrf70_fw_patch_update` - For nRF70 Series devices.

* :ref:`nrf91_fota` section of :ref:`ug_nrf91_config_build` - For nRF91 Series devices.

.. note::

  For the nRF54H20 SoC, see the :ref:`nrf54h_suit_sample` sample to perform a DFU over Bluetooth LE or UART.
  See the :ref:`ug_nrf54h20_suit_dfu` page and its listed subpages for more information about the SUIT DFU procedure.

See the following user guides for an overview of topics related to general firmware updates with the |NCS|:

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   dfu_image_versions
