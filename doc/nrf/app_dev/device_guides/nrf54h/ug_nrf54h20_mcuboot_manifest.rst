.. _ug_nrf54h20_mcuboot_manifest:

Configuring nRF54H20 application for updates using a manifest
#############################################################

.. contents::
   :local:
   :depth: 2

This guide describes the manifest-based update strategy and an example of configuring an nRF54H20 application for updates in the Direct XIP mode with revert.
It is organized into several key sections:

* A brief overview of the manifest-based update strategy and when it is beneficial to use it.
* A brief description of the Nordic-specific MCUboot manifest.
* An introduction to the manifest-based update and boot strategy.
* An example, demonstrating how to perform partial updates using a manifest-based updates in Direct XIP mode with revert.

For more details on the differences between update swap and direct XIP strategies, see :ref:`ug_bootloader_main_config`.

The manifest-based update strategy is currently supported only in Direct XIP mode on nRF54H20 platform.

Overview
********

The multi-core support in MCUboot design assumes that each core has its own, independent firmware image.
As mentioned inside the _ref `ug_nrf54h20_partitioning_merged`, this leads to two complications when using the Direct XIP update strategy:

1. There is no signalling mechanism to pass verification results of the radio core firmware from the bootloader to the application core firmware.
   There is also no way to ensure that the bootloader starts all images from the same slot.
#. Each image needs to define its own set of dependencies, leading to a complex web of interdependencies between images.
   This makes it difficult to ensure compatibility between different images and manage their versions effectively.

To address these challenges, Nordic-specific MCUboot introduces the manifest-based update strategy.
In this strategy, a single manifest TLV describes multiple firmware images through their digests, defining a set of images, that were tested together and are known to be compatible.
The manifest image is the only image that contains a manifest TLV and is the only image that needs to be confirmed or reverted.
There are two manifests inside the system: one for each manifest image slot.
The bootloader ensures that all of the images, described by the manifest are present and valid before switching to the new slot.
This ensures that all images are compatible and reduces the complexity of managing interdependencies between images.

.. figure:: images/mcuboot_manifest_partitions.svg
   :alt: MCUboot partitions and their relationship with manifests

This strategy also allows to perform partial updates of the system, with the limitation that the manifest image must always be updated.
For example, it is possible to update only the application core firmware while keeping the radio core firmware unchanged.
This is particularly useful when the application core firmware requires frequent updates, while the radio core firmware remains relatively stable.

The manifest-based update strategy is beneficial in scenarios where:

* The system consists of multiple firmware images scattered across different memory regions that need to be managed together.
* There is a need to perform partial updates of the system while ensuring compatibility between different images.
* A metered connection is used for updates, and minimizing the amount of data transferred is crucial.
* It is required to provide a precise information about all of the images running on the device for auditing or compliance purposes.
* A slot preference mechanism is needed to control which slot should be used for booting.

However, the manifest-based update strategy comes with some trade-offs:

* Increased complexity in managing manifests and ensuring that all images described by a manifest are updated together.
* Additional overhead in terms of storage space for manifests and digests.
* Currently manifests do not support describing dependencies between images using semantic versions.
* It is not possible to update any image without updating the manifest image.

MCUboot manifest
****************

The MCUboot manifest is a TLV structure that describes multiple firmware images through their digests.
The first version of the manifest contains just the digests of the images and can be represented by the following structure:

.. code-block:: c

   struct mcuboot_manifest {
        uint32_t format;
        uint32_t image_count;
        /* Skip a digest of the MCUBOOT_MANIFEST_IMAGE_INDEX image. */
        uint8_t image_hash[MCUBOOT_IMAGE_NUMBER - 1][IMAGE_HASH_SIZE];
   };

The ``format`` field specifies the format of the manifest. Currently only the version ``1`` is defined.

The ``image_count`` field specifies the number of images described by the manifest.

The ``image_hash`` array contains the cryptographic hashes of the images, excluding the manifest image itself.

The manifest image is configured by the :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX` symbol.
The size of each hash must be the same and is determined by the hash algorithm configured through the :kconfig:option:`CONFIG_BOOT_IMG_HASH_ALG_*` choice.

The manifest structure is generated during the build process and is appended to the manifest image based on the configuration generated automatically by the build system.
By default the configuration file can be found in the build directory under the :file:`./build/manifest.yaml` for the primary application and :file:`./build/manifest_secondary.yaml` for the secondary application.
The auto-generation of the input YAML files can be disabled by setting the :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_DIR` option to a custom directory.
In that case, the build system expects to find the manifest configuration files with matching names in the specified directory.
The contents of the configuration file reflects the fields of the manifest structure described above, for example:

.. code-block:: yaml

  format: '1'
  manifest_index: '0'
  images:
   - name: 'ipc_radio'
     path: 'build/ipc_radio/zephyr/zephyr.signed.bin'
     index: '1'

The optional fields with indexes of images are zero-based and correspond to the MCUboot image indexes.
They are used to ensure that the correct digest is associated with the correct image in a more complex, multi-image configurations.

By default, the format uses path to the image, so the digest is updated whenever a new manifest is generated.
It is also possible to specify the digest directly in the configuration file by using the ``hash`` field instead of the ``path`` field.
This can be useful when the image is not built as part of the same build process or when the image is not available during the manifest generation.

.. code-block:: yaml

  format: '1'
  manifest_index: '0'
  images:
   - name: 'ipc_radio'
     hash: '3a7bd3e2360a3d485f2f75f3b70a5f8b1c9d5e6f7a8b9c0d1e2f3a4b5c6d7e8f'
     index: '1'

The generation of the manifest structure is handled by the ``imgtool sign`` command.
The path to the manifest configuration file can be specified using the ``--manifest`` option.
The manifest TLV is appended to the image's protected TLV area, being included in the image's signature.

.. figure:: images/mcuboot_manifest_tlvs.png
   :alt: MCUboot manifest TLV inside the image structure

Manifest-based update and boot strategy
***************************************

The manifest-based update and boot strategy can be enabled by setting the :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_UPDATES` option.
When using this strategy, the bootloader performs the following steps:

1. It looks for the best manifest image candidate slot. The selection of the preferred slot uses the following rules:

   a. If one of the slots is not valid, the other slot is selected as active.
   #. If both slots are valid, the slot marked as "preferred" is selected as active.
   #. If both slots are valid and none is marked as "preferred", the slot with the higher semantic version number is selected as active.
   #. If none of the above conditions is met, slot A (primary) is selected as active.

#. It verifies that the manifest image in active slot is valid and copies the manifest from the TLV area to the internal bootloader state.
#. If the manifest image is valid, the bootloader checks the manifest image confirmation status.

   a. If the manifest image is pending confirmation, the bootloader erases the manifest image, marks the active slot as unavailable, and restarts the process from step 1.
   #. If the manifest image is marked for testing, a dedicated flag is set to indicate that the manifest image is pending confirmation.
   #. If the manifest image is confirmed, the bootloader proceeds to the next step.

#. It iterates over all other images and verifies its validity. An image digest check is extended to verify that the digest matches the value specified in the manifest, stored inside the internal bootloader state.
#. If one of the images is invalid, the bootloader erases that image, marks the active slot in all images as unavailable, and restarts the process from step 1.
#. If the rollback protection is enabled and all images in active slot are valid and the manifest image is confirmed, the bootloader updates the security counter value.
#. If there is a valid manifest image, the bootloader boots the active slot.

There is no dedicated update logic.
The update process is based on the preferred slot selection and the image confirmation status.

It is important to note, that the confirmation flags of the manifest image are analyzed before checking the availability of other images, so the manifest image should be selected for testing once all other images are downloaded.

The readiness of the manifest image to be applied can be checked using the SMP protocol and looking at the ``bootable`` flag of the manifest image.

Performing a partial updates using a manifest
*********************************************

To perform a partial update using a manifest, the following steps should be followed:

1. Build and program a sample that uses the manifest-based update strategy, such as the :ref:`ab_split_sample` sample.
#. Using SMP protocol, read the digests of the currently running images.

   .. code-block:: console
      :emphasize-lines: 6, 7, 16, 17

      $ nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM0

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         hash: 79756c74473206626cc558510ffbfadd6d09291f82d2f2de3756137ad522040155af4b149702b37059b6ff8eb73d979af7763d342fdbcb0662c45c6889e73b9d
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         hash: b7fe2c6b8b6b8569fadd2706326d0d0f529525bf718dbae7e7c9175f0a36df1a085cf14f9707e6a41beee6ae8655c4b7e326896b16642b609e8076532417d99d
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false

#. Using SMP protocol, send the initial secondary slot radio image to the device.

   .. code-block:: console
      :emphasize-lines: 15, 16, 17, 18, 19

      $ nrfutil mcu-manager serial image-upload --serial-port /dev/ttyACM0 --image-number 1 --firmware ./build/ipc_radio_secondary_app/zephyr/zephyr.signed.bin
      $ nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM0

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 1
         version: 0.0.0
         hash: 71d38c58a98d2b1e7304589cd1b50bf62f68e35b9957e35fbd92a0593417ba419599f38dcf489aff1f8d67e5ae84424501851d8a87c6a18082c0144af0ae5876
         bootable: false
         pending: false
         confirmed: false
         active: false
         permanent: false

#. Modify the application code by adding a log message and changing the version number.

   .. code-block:: console

      CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="1.0.0+0"

#. Build the new application image. An updated manifest will be generated and placed inside the application core update candidate automatically during the build process.

   .. code-block:: console
      :emphasize-lines: 12, 30, 31, 32, 33, 34, 35, 36, 37

      $ imgtool dumpinfo ./build/mcuboot_secondary_app/zephyr/zephyr.signed.bin

      Printing content of signed image: zephyr.signed.bin

      #### Image header (offset: 0x0) ############################
      magic:              0x96f3b83d
      load_addr:          0x100000
      hdr_size:           0x800
      protected_tlv_size: 0x58
      img_size:           0x289a4
      flags:              ROM_FIXED (0x100)
      version:            1.0.0+0
      ############################################################
      #### Payload (offset: 0x800) ###############################
      |                                                          |
      |              FW image (size: 0x289a4 Bytes)              |
      |                                                          |
      ############################################################
      #### Protected TLV area (offset: 0x291a4) ##################
      magic:     0x6908
      area size: 0x58
              ---------------------------------------------
              type: SEC_CNT (0x50)
              len:  0x4
              data: 0x01 0x00 0x00 0x00
              ---------------------------------------------
              type: MANIFEST (0x76)
              len:  0x48
              data: 0x01 0x00 0x00 0x00 0x01 0x00 0x00 0x00
                    0x71 0xd3 0x8c 0x58 0xa9 0x8d 0x2b 0x1e
                    0x73 0x04 0x58 0x9c 0xd1 0xb5 0x0b 0xf6
                    0x2f 0x68 0xe3 0x5b 0x99 0x57 0xe3 0x5f
                    0xbd 0x92 0xa0 0x59 0x34 0x17 0xba 0x41
                    0x95 0x99 0xf3 0x8d 0xcf 0x48 0x9a 0xff
                    0x1f 0x8d 0x67 0xe5 0xae 0x84 0x42 0x45
                    0x01 0x85 0x1d 0x8a 0x87 0xc6 0xa1 0x80
                    0x82 0xc0 0x14 0x4a 0xf0 0xae 0x58 0x76
      ...

#. Use the SMP protocol to upload only the new application core image to the device.

   .. code-block:: console

      $ nrfutil mcu-manager serial image-upload --serial-port /dev/ttyACM0 --firmware ./build/mcuboot_secondary_app/zephyr/zephyr.signed.bin

#. Verify that although only the application core image was updated, the manifest is still satisfied. This can be done by checking the ``bootable`` flag of the manifest image using the SMP protocol.

   .. code-block:: console
      :emphasize-lines: 13, 28

      $ nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM0

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 0
         slot: 1
         version: 1.0.0
         hash: 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607
         bootable: true
         pending: false
         confirmed: false
         active: false
         permanent: false
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 1
         version: 0.0.0
         hash: 71d38c58a98d2b1e7304589cd1b50bf62f68e35b9957e35fbd92a0593417ba419599f38dcf489aff1f8d67e5ae84424501851d8a87c6a18082c0144af0ae5876
         bootable: true
         pending: false
         confirmed: false
         active: false
         permanent: false

#. Mark the update candidate as pending and reboot the device to apply the update.

   .. code-block:: console
      :emphasize-lines: 14, 29

      $ nrfutil mcu-manager serial image-test --serial-port /dev/ttyACM0 --hash 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 0
         slot: 1
         version: 1.0.0
         hash: 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607
         bootable: true
         pending: true
         confirmed: false
         active: false
         permanent: false
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 1
         version: 0.0.0
         hash: 71d38c58a98d2b1e7304589cd1b50bf62f68e35b9957e35fbd92a0593417ba419599f38dcf489aff1f8d67e5ae84424501851d8a87c6a18082c0144af0ae5876
         bootable: true
         pending: true
         confirmed: false
         active: false
         permanent: false

      $ nrfutil mcu-manager serial reset --serial-port /dev/ttyACM0

#. Use the SMP protocol to verify that the device is running the new application core image.

   .. code-block:: console
      :emphasize-lines: 16, 31

      $ nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM0

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 0
         slot: 1
         version: 1.0.0
         hash: 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 1
         version: 0.0.0
         hash: 71d38c58a98d2b1e7304589cd1b50bf62f68e35b9957e35fbd92a0593417ba419599f38dcf489aff1f8d67e5ae84424501851d8a87c6a18082c0144af0ae5876
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false

#. Confirm the update using the SMP protocol to make the update permanent.
   Note that confirming the manifest image also confirms all other images described by the manifest.
   There is no need to issue separate confirmation or testing commands for each image.

   .. code-block:: console
      :emphasize-lines: 15, 30

      $ nrfutil mcu-manager serial image-confirm --serial-port /dev/ttyACM0 --hash 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607

      Image
         image number: 0
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 0
         slot: 1
         version: 1.0.0
         hash: 47c8d6213ef304a78d4738c47ffe71b0c3f3d33fbefa9cf3294643ca5ca5444459648dd28548514850ad1c2238a393f1722f942f193ed7fcf9cbb561b4a96607
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false
      Image
         image number: 1
         slot: 0
         version: 0.0.0
         ... (output truncated) ...
      Image
         image number: 1
         slot: 1
         version: 0.0.0
         hash: 71d38c58a98d2b1e7304589cd1b50bf62f68e35b9957e35fbd92a0593417ba419599f38dcf489aff1f8d67e5ae84424501851d8a87c6a18082c0144af0ae5876
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false

Configuration
*************

|config|

Configuration options
=====================

The manifest-based update strategy can be configured using the following configuration options:

.. _SB_CONFIG_MCUBOOT_MANIFEST_UPDATES:

SB_CONFIG_MCUBOOT_MANIFEST_UPDATES - Enables manifest-based updates.
   This configuration option enables the manifest-based update strategy in MCUboot.
   It can be enabled when the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP` or :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT` mode is used on the nRF54H20 SoC.

.. _SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX:

SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX - Index of the image that must include manifest.
   This configuration option specifies the index of the image that must include the manifest TLV.
   By default, the application image index (``0``) is selected.

.. _SB_CONFIG_MCUBOOT_MANIFEST_DIR:

SB_CONFIG_MCUBOOT_MANIFEST_DIR - Directory for manifest configuration files.
   This configuration option specifies the directory where the manifest configuration files are located.
   By default, the build system generates the manifest configuration files in the build directory.
   If this option is set to a custom directory, the build system expects to find the manifest configuration files (:file:`manifest.yaml` as well as :file:`manifest_secondary.yaml`) in the specified directory.
   This option is useful when the manifest configuration files need to be managed separately from the build process.

The build system automatically sets the following local configuration options inside each image:

.. _CONFIG_NCS_MCUBOOT_MANIFEST_UPDATES:

CONFIG_NCS_MCUBOOT_MANIFEST_UPDATES - To match the value of :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_UPDATES`.
   This is a Kconfig which is informative only, the value should not be changed.

.. _CONFIG_NCS_MCUBOOT_MANIFEST_IMAGE_INDEX:

CONFIG_NCS_MCUBOOT_MANIFEST_IMAGE_INDEX - To match the value of :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX`.
   This is a Kconfig which is informative only, the value should not be changed.

.. _CONFIG_NCS_MCUBOOT_MANIFEST_DIR:

CONFIG_NCS_MCUBOOT_MANIFEST_DIR - To match the value of :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_DIR`.
   This is a Kconfig which is informative only, the value should not be changed.

.. _CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST:

CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST - To enable appending the manifest during image signing.
   This option is set only inside the image that has a matching index with the :kconfig:option:`SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX`.
   This is a Kconfig which is informative only, the value should not be changed.
