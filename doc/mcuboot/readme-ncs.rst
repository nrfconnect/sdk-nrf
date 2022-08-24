.. _mcuboot_ncs:

Using MCUboot in nRF Connect SDK
################################

See :doc:`readme-zephyr` for general information on how to integrate MCUboot with Zephyr.

The nRF Connect SDK provides additional functionality that is available when MCUboot is included.
This functionality is implemented in the files in the :file:`modules/mcuboot` subfolder in the `sdk-nrf`_ repository.

To include MCUboot in your nRF Connect SDK application, enable :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`.

When you build your application with this option set, the following files that can be used for firmware over-the-air (FOTA) upgrades are automatically generated:

* :file:`app_update.bin` - Signed variant of the firmware in binary format (as opposed to intelhex).
  This file can be uploaded to a server as FOTA image.

* :file:`app_to_sign.bin` - Unsigned variant of the firmware in binary format.

* :file:`app_signed.hex` - Signed variant of the firmware in intelhex format.
  This HEX file is linked against the same address as the application.
  Programming this file to the device will overwrite the existing application.
  It will not trigger a DFU procedure.

* :file:`app_test_update.hex` - Same as :file:`app_signed.hex` except that it contains metadata that instructs MCUboot to test this firmware upon boot.
  As :file:`app_signed.hex`, this HEX file is linked against the same address as the application.
  Programming this file to the device will overwrite the existing application.
  It will not trigger a DFU procedure.

* :file:`app_moved_test_update.hex` - Same as :file:`app_test_update.hex` except that it is linked against the address used to store the upgrade candidates.
  When this file is programmed to the device, MCUboot will trigger the DFU procedure upon reboot.

.. note::
   When you use MCUboot in the direct-xip mode, enable the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option to let the build system generate an additional set of files for the second application slot.
   These ``.hex`` files are identical to the ones listed above, but their names also use the ``mcuboot_secondary_`` prefix.
   For example, :file:`mcuboot_secondary_app_signed.hex` is created and placed in the second slot on the target device when the :file:`app_signed.hex` file is placed in the first slot.
   For more information about the direct-xip mode, see the *Equal slots (direct-xip)* section in the :doc:`Bootloader documentation <design>`.

.. _`sdk-nrf`: https://github.com/nrfconnect/sdk-nrf
