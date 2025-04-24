.. _ecies_x25519:

MCUboot AES Image Encryption with ECIES-X25519 Key Exchange
###########################################################

.. contents::
   :local:
   :depth: 2

Introduction
************

MCUboot, when used with nRF54l15, can be configured to support encrypted images using AES.
Images are encrypted using AES and ECIES-X25519 is used for key delivery (exchange) within image.
When image encryption is enabled user may choose to either upload just signed or encrypted images to be swapped during boot.
If MCUboot finds encrypted image in secondary slot it will decrypt it while swapping slots.
Image that has been encrypted before being swapped into primary slot will be re-encrypted in case when swap is reverted.

Limitations
***********

ECIES-X25519 key exchange is supported only when ED25519 signature is selected, which is default for nRF54l15.
Encryption is not supported with MCUboot Direct-XIP modes.
Storing ECIES-X25519 device private key in KMU is not supported at this moment.
SHA256 is currently used by HMAC/HKDF, which is a subject to change.

HMAC/HKDF impact on TLV and Key Exchange
****************************************

Encrypted image contains TLV that provides public key for ECIES-X25519 key exchange, encrypted AES key and MAC tag of the encrypted key.
Derivation of ASE key encryption key is involves HKDF and MAC tag is generated using HMAC; both HKDF and HMAC at this point use SHA256.
While using SHA256 is not a problem and does not significantly impact security it increases code size, as it requires addition of the algorithm support together with SHA512, which is already used by ED25519.
There might be an option, in the future, to switch to using SHA512, but pre-installed MCUboot instance will not be able to boot images signed with TLV using SHA512.

Building Application with Image Encryption
******************************************

Building application with MCUboot that has image encryption enabled is performed with command:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y

The :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` enables MCUboot encryption support and key exchange method is selected by type of selected key for signature; in case of nRF54l15 ED25519 signature is selected by default.
The encrypted image will be stored, within application build directory, under name zephyr.signed.encrypted.bin and zephyr.signed.encrypted.hex, where the former represents binary image that can be used with MCUmgr for DFU.

With additional option :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` user should pass ECIES-X25519 private key in pem format; the key will be built into MCUboot.

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_ENCRYPTION=y -DSB_CONFIG_BOOT_ENCRYPTION_KEY_FILE=\"<path to key.pem>\"

Note that public encryption key, derived from the specified file, will be used to sign an application image. This public key is automatically derived from private key by imgtool, which is invoked by build system when signing an image.

.. note::

   Stsbuild options :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION` sets MCUboot :kconfig:option:`CONFIG_BOOT_ENCRYPT_IMAGE`;
   :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` sets MCUboot :kconfig:option:`CONFIG_BOOT_ENCRYPTION_KEY_FILE`
   and :kconfig:option:`CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE` for default application, which is then passed to imagetool for application image encryption. You can not override, options enforced by sysbuild, with MCUboot nor application Kconfig options to correspond to them.
