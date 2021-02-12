.. _mcuboot_ncs:

Using MCUboot in nRF Connect SDK
################################

See :doc:`readme-zephyr` for general information on how to integrate MCUboot with Zephyr.

nRF Connect SDK provides additional functionality that is available when MCUboot is included.
This functionality is implemented in the files in the ``modules/mcuboot`` subfolder in the `sdk-nrf`_ repository.

To include MCUboot in your nRF Connect SDK application, enable :option:`CONFIG_BOOTLOADER_MCUBOOT`.

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

.. _`sdk-nrf`: https://github.com/nrfconnect/sdk-nrf
