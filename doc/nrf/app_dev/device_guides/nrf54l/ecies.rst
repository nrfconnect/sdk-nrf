.. _ug_nrf54l_ecies:
.. _ug_nrf54l_ecies_x25519:

MCUboot AES image encryption with ECIES key exchange
####################################################

.. contents::
   :local:
   :depth: 2

MCUboot on the nRF54L Series supports encrypted images using AES.
Images are encrypted using AES, with either ECIES-X25519 or ECIES-P256 used for key delivery within the image.
The ECIES scheme is selected according to the image signature algorithm: ED25519 uses ECIES-X25519, while ECDSA P-256 uses ECIES-P256.
When image encryption is enabled, you can choose to upload signed or encrypted images to be swapped during boot.
If MCUboot finds an encrypted image in the secondary slot, it decrypts the image during the slot swapping process.
An image that was encrypted before being swapped into the primary slot is re-encrypted if the swap is later reverted.

Limitations
***********

The current implementation has the following limitations:

* On the nRF54L15 SoC, the default ED25519 signature configuration uses ECIES-X25519 key exchange.
* On the nRF54LS05A and nRF54LS05B SoCs, the default ECDSA P-256 signature configuration uses ECIES-P256 key exchange.
* Encryption is not supported when using MCUboot in direct-xip mode.
* Storing the ECIES device private key in the Key Management Unit (KMU) is currently not supported.
* Encryption of MCUboot updates is supported with an immutable and upgradable bootloader configuration, with the following limitations:

  * The active instance of MCUboot must use image matching based on explicit image address (:kconfig:option:`CONFIG_MCUBOOT_CHECK_HEADER_LOAD_ADDRESS=y`) or vendor and class UUID-based matching.
  * You must use the same key encryption for both MCUboot updates and the application image.
  * You must sign the image manually, since the NCS signing process for s0/s1 images has not yet been updated to support encryption.

HMAC and HKDF impact on TLV and key exchange
********************************************

An encrypted image includes a TLV that contains the ephemeral public key for the selected ECIES scheme, the encrypted AES key, and the MAC tag of the encrypted key.
The key used to encrypt the AES key is derived using HKDF, and the MAC tag is generated using HMAC.

ECIES-P256 uses SHA-256 for HKDF and HMAC.
ECIES-X25519 supports both SHA-256 and SHA-512, and uses SHA-512 by default on the nRF54L Series.
Using SHA-256 with ECIES-X25519 does not pose a security concern and has a minimal impact on performance, but it increases code size when used with ED25519.
This is because SHA-256 support must then be included in addition to SHA-512, which is already used by the ED25519 signature algorithm.

Pre-installed MCUboot instances cannot boot images that use ECIES TLVs generated with a different ECIES scheme or hash algorithm than the instance supports.

Building an application with image encryption
*********************************************

To build an application that uses MCUboot with image encryption enabled, run the following command:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y

The :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` option enables encryption support in MCUboot.

The key exchange method is determined by the type of signature key selected.
For the nRF54L15 SoC, ED25519 is selected by default, and the preceding command therefore uses ECIES-X25519.
For the nRF54LS05A and nRF54LS05B SoCs, ECDSA P-256 is selected by default, and the command uses ECIES-P256 instead.
For example, the following command builds for the nRF54LS05A SoC and explicitly selects the default ECDSA P-256 configuration:

.. parsed-literal::
   :class: highlight

   west build -b nrf54ls05dk/nrf54ls05a/cpuapp -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y -DSB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y

When encryption is enabled, the encrypted image files :file:`zephyr.signed.encrypted.bin` and :file:`zephyr.signed.encrypted.hex` are generated in the application build directory.

The BIN file is a binary image suitable for Device Firmware Update (DFU) operations using :ref:`MCUmgr<dfu_tools_mcumgr_cli>`.

When you set the :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` option, you must provide a private encryption key in PEM format with a type that matches the selected signature algorithm.
Use an X25519 encryption key with ED25519 signatures, or an ECDSA P-256 encryption key with ECDSA P-256 signatures.
The encryption key is separate from the image signature key.
The private encryption key is built into the MCUboot image, while imgtool derives its public key when packaging encrypted application images.
See the following example:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y -DSB_CONFIG_BOOT_ENCRYPTION_KEY_FILE=\"<path to key.pem>\"

.. note::
   * imgtool generates an ephemeral ECIES key pair for each encrypted image.
     The image's encryption TLV contains the ephemeral public key, which MCUboot combines with its private encryption key to derive the key used to unwrap the image's AES key.
   * The sysbuild option :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` sets the MCUboot configuration option :kconfig:option:`CONFIG_BOOT_ENCRYPT_IMAGE`.
     Similarly, the :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` option sets both :kconfig:option:`CONFIG_BOOT_ENCRYPTION_KEY_FILE` for MCUboot and :kconfig:option:`CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE` for the default application.

   These values are then passed to imgtool for encrypting the application image.

   You cannot override these options using MCUboot or application-level Kconfig options, as they are enforced by sysbuild.

Enabling encryption in |nRFVSC| projects
****************************************

To correctly set up encryption in |nRFVSC|, you must familiarize yourself with `How to work with build configurations`_.
When configuring build options, ensure to include :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` and :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` Kconfig options using extra CMake arguments.

If you are modifying an existing project, you must regenerate it to activate new settings.
