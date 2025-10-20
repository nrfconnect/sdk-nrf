.. _ug_nrf54l_ecies_x25519:

MCUboot AES image encryption with ECIES-X25519 key exchange
###########################################################

.. contents::
   :local:
   :depth: 2

MCUboot on the nRF54L Series can support encrypted images using AES.
Images are encrypted using AES, and ECIES-X25519 is used for key delivery (exchange) within the image.
When image encryption is enabled, you can choose to upload signed or encrypted images to be swapped during boot.
If MCUboot finds an encrypted image in the secondary slot, it decrypts the image during the slot swapping process.
An image that was encrypted before being swapped into the primary slot is re-encrypted if the swap is later reverted.

Limitations
***********

The current implementation has the following limitations:

* On the nRF54L15 SoC, ECIES-X25519 key exchange is supported only when the ED25519 signature algorithm is selected.
  This is the default configuration.
* Encryption is not supported when using MCUboot in direct-xip mode.
* Storing the ECIES-X25519 device private key in the Key Management Unit (KMU) is currently not supported.
* HMAC and HKDF tools currently use the SHA-256 hash algorithm.

HMAC and HKDF impact on TLV and key exchange
********************************************

An encrypted image includes a TLV that contains the public key for ECIES-X25519 key exchange, the encrypted AES key, and the MAC tag of the encrypted key.
The key used to encrypt the AES key is derived using HKDF, and the MAC tag is generated using HMAC.

While the use of SHA-256 does not pose a security concern and has a minimal impact on performance, it increases the code size.
This is because SHA-256 support must be included in addition to SHA-512, which is already used by the ED25519 signature algorithm.

Additionally, pre-installed MCUboot instances will not be able to boot images that use TLVs generated with different hash algorithms.

Building application with image encryption
******************************************

To build an application that uses MCUboot with image encryption enabled, run the following command:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y

The :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` option enables encryption support in MCUboot.

The key exchange method is determined by the type of signature key selected.
For the nRF54L15 SoC, the ED25519 signature algorithm is the default setting.

When encryption is enabled, the encrypted image files :file:`zephyr.signed.encrypted.bin` and :file:`zephyr.signed.encrypted.hex` are generated in the application build directory.

The BIN file is a binary image suitable for Device Firmware Update (DFU) operations using :ref:`MCUmgr<dfu_tools_mcumgr_cli>`.

When the :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` option is enabled, you must provide an ECIES-X25519 private key in PEM format.
This key is built into the MCUboot image during the build process.
See the following example:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y -DSB_CONFIG_BOOT_ENCRYPTION_KEY_FILE=\"<path to key.pem>\"

.. note::
   * The public key, derived from the specified private key file, is added to the image and later used on the device to derive the decryption key for the application image.
     This public key is automatically derived from the private key by imgtool, which is invoked by the build system when signing the image.
   * The sysbuild option :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` sets the MCUboot configuration option :kconfig:option:`CONFIG_BOOT_ENCRYPT_IMAGE`.
     Similarly, the :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` option sets both :kconfig:option:`CONFIG_BOOT_ENCRYPTION_KEY_FILE` for MCUboot and :kconfig:option:`CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE` for the default application.

   These values are then passed to imgtool for encrypting the application image.

   You cannot override these options using MCUboot or application-level Kconfig options, as they are enforced by sysbuild.

Enabling encryption in |nRFVSC| projects
****************************************

To correctly set up encryption in |nRFVSC|, you must familiarize yourself with `How to work with build configurations`_.
When configuring build options, ensure to include :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` and :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` Kconfig options using extra CMake arguments.

If you are modifying an existing project, you must regenerate it to activate new settings.
