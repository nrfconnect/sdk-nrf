.. fota_upgrades_over_ble_intro_start

FOTA updates are supported using MCUmgr's Simple Management Protocol (SMP) over Bluetooth®.
The application acts as a GATT server and allows the connected Bluetooth Central device to perform a firmware update.
To use FOTA over Bluetooth LE, samples must support Bluetooth peripheral role (:kconfig:option:`CONFIG_BT_PERIPHERAL`).

The application supports SMP handlers related to:

* Image management.
* Operating System (OS) management used to reboot the device after the firmware upload is complete.
* Erasing settings partition used to ensure that a new application is not booted with incompatible content in the settings partition written by the previous application.

To enable support for FOTA updates, do the following:

* Enable the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option, which implies configuration of the following:

   * All of the SMP command handlers mentioned in the previous paragraph.
   * SMP BT reassembly feature.
   * The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` Kconfig option automatically extends the Bluetooth buffers, which allows to speed up the FOTA transfer over Bluetooth, but also increases RAM usage.

.. note::
   The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option can be used devices to enable MCUmgr to perform firmware over-the-air (FOTA) updates using Bluetooth LE.
   It can be used along with other samples, and is meant as a demonstration of the default DFU configuration over Bluetooth.

.. fota_upgrades_over_ble_intro_end
