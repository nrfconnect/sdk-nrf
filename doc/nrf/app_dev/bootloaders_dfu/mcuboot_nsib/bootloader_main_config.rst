.. _ug_bootloader_main_config:

Essential MCUboot configuration items
#####################################

.. contents::
   :local:
   :depth: 2

The following page provides an overview of the key configuration items necessary for understanding and configuring MCUboot's behavior.
The :ref:`sysbuild` Kconfig options generally override the MCUboot's Kconfig option.
Sysbuild options aim to establish a coherent environment for the entire system, while MCUboot options focus specifically on the bootloader.
This means that many sysbuild options configure the MCUboot, the application, and the build system settings to ensure compatibility.

Supported image configurations
******************************

MCUboot primarily functions as a dual-bank bootloader, managing one bank as the executable and the other as an upgrade candidate or an alternative executable.
In MCUboot terminology, *image* refers to a specific type of executable code, typically the application image.
In the dual-bank scheme, two banks (or slots) are associated with each image.

MCUboot can be configured to support multiple images, enabling it to operate as an enhanced dual-bank bootloader.
This configuration allows for more complex update scenarios and greater flexibility in managing different sets of firmware:

* ``CONFIG_UPDATEABLE_IMAGE_NUMBER`` - Specifies the number of images that MCUboot can handle.
* :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` - Configures the number of images supported by MCUboot at the sysbuild level.

Operational modes of MCUboot
****************************

MCUboot supports various operational modes that dictate its behavior towards the application images it manages.
The choice of mode determines the specific scheme of the bootloader's operations for the images supported by a given MCUboot instance:

.. list-table:: MCUboot modes
    :header-rows: 1
    :widths: auto

    * - **Mode name**
      - **Description**
      - **MCUboot Kconfig option**
      - **Sysbuild Kconfig option**
      - **Multiple images**
    * - Swap using scratch
      - Implements a dual-bank image swapping algorithm utilizing a scratch area, accommodating memories with varying erase block sizes.
      - ``CONFIG_BOOT_SWAP_USING_OFFSET``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH`
      - Yes
    * - Swap using move
      - Executes a dual-bank image swapping algorithm by moving sectors, offering greater efficiency than scratch-based swaps, suitable only for memories with consistent erase block sizes.
      - ``CONFIG_BOOT_SWAP_USING_MOVE``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE`
      - Yes
    * - Swap using offset
      - Introduces a new dual-bank image swapping algorithm that moves sectors with optimizations for enhanced speed, applicable to memories with uniform erase block sizes.
      - ``CONFIG_BOOT_SWAP_USING_OFFSET``
      - --
      - Yes
    * - Overwrite
      - Employs a straightforward dual-bank image overwrite algorithm that directly replaces the image.
      - ``CONFIG_BOOT_UPGRADE_ONLY``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY`
      - Yes
    * - Direct-XIP
      - Facilitates dual-bank image execution directly from storage, updating by uploading a new image to an alternate bank, eliminating the need for swapping or overwriting NVM.
      - ``CONFIG_BOOT_DIRECT_XIP``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`
      - Yes
    * - Direct-XIP with revert
      - Enables dual-bank image execution directly from storage with additional support for reverting to a previous image if necessary, enhancing system reliability.
      - ``CONFIG_BOOT_DIRECT_XIP`` and ``CONFIG_BOOT_DIRECT_XIP_REVERT``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`
      - Yes
    * - Firmware loader
      - Provides a dual-bank image firmware loading mode that allows dynamic selection of the image bank for booting the application, accommodating banks of different sizes.
      - ``CONFIG_BOOT_FIRMWARE_LOADER``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
      - No
    * - Single application
      - Supports a single application image mode, utilized when only one application image is necessary and dual-bank operations are not required.
      - ``CONFIG_SINGLE_APPLICATION_SLOT``
      - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SINGLE_APP`
      - No

Signature types
***************

MCUboot supports various signature types.
The signature type specifies the algorithm used to sign the image.
You can calculate each signature on a hash of the image, prepared by MCUboot (referred to as pre-hash signatures).
Notably, the Ed25519 signature can also be directly calculated on the image itself.

.. list-table:: MCUboot signature types
    :header-rows: 1
    :widths: auto

  * - **Signature**
    - **Description**
    - **MCUboot Kconfig option**
    - **Sysbuild Kconfig option**
    - **Signed material**
  * - RSA
    - Utilizes RSA for digital signatures, supporting key sizes of 2048 and 3072 bits.
    - ``CONFIG_BOOT_SIGNATURE_TYPE_RSA``, ``CONFIG_BOOT_SIGNATURE_TYPE_RSA_LEN``
    - :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA`
    - Image hash
  * - ECDSA P-256
    - Employs the elliptic curve digital signature algorithm using the P-256 curve for enhanced security.
    - ``CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256``
    - :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256`
    - Image hash
  * - Ed25519
    - Uses the Edwards curve digital signature algorithm with Ed25519.
    - ``CONFIG_BOOT_SIGNATURE_TYPE_ED25519``
    - :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519`
    - Image hash, image itself (pure Ed25519)
  * - None
    - Indicates the absence of a signature; the image is unchecked but its hash is verified for integrity.
    - ``CONFIG_BOOT_SIGNATURE_TYPE_NONE``
    - :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE`
    - Not applicable

Public key storage
******************

MCUboot supports two methods for storing the public key used for image verification:

* Embedded in the image - The public key is compiled in the MCUboot instance.
  For this method, no additional configuration is required.
* Stored in the KMU - The public key is stored in the Key Management Unit (KMU) of the nRF54L devices that support this hardware peripheral.
  You can enable it using the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU` sysbuild Kconfig option.
  This implementation supports up to three keys and includes a key revocation scheme.
  You can manage these features through the ``CONFIG_BOOT_SIGNATURE_KMU_SLOTS`` and ``CONFIG_BOOT_SIGNATURE_KMU_SLOTS`` MCUboot Kconfig options.

MCUboot image hash algorithms
*****************************

MCUboot supports several hash algorithms to calculate the integrity of the image hash.
Typically, the default hash algorithm is sufficient for most applications, as MCUboot selects the most relevant one based on the system configuration.
However, you can customize it to meet specific requirements.

.. list-table:: MCUboot image hash algorithms
    :header-rows: 1
    :widths: auto

  * - **Hash**
    - **MCUboot Kconfig option**
    - **Comments**
  * - SHA-256
    - ``CONFIG_BOOT_IMG_HASH_ALG_SHA256``
    - The default hash. Compatible with all pre-hash signature types.
  * - SHA-384
    - ``CONFIG_BOOT_IMG_HASH_ALG_SHA384``
    - Currently not utilized by |NCS|.
  * - SHA-512
    - ``CONFIG_BOOT_IMG_HASH_ALG_SHA512``
    - Restricted to use with Ed25519 signatures.

MCUboot recovery protocol
*************************

Mcuboot supports serial recovery protocols compatible with :ref:`MCUmgr <dfu_tools_mcumgr_cli>`, enabling device programming via serial connection instead of J-Link.
This feature allows direct upload of applications to the executable image bank and, depending on configuration, to other banks as well.
To use this feature, enable the ``CONFIG_MCUBOOT_SERIAL`` Kconfig option.
