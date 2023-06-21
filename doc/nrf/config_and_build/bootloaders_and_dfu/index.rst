.. _app_bootloaders:

Bootloaders and Device Firmware Updates
#######################################

.. contents::
   :local:
   :depth: 2

There are two types of bootloaders used by the |NCS|:

* :ref:`Immutable first-stage bootloaders <immutable_bootloader>`.
* :ref:`Upgradable second-stage bootloaders <upgradable_bootloader>` that can perform device firmware updates to both themselves and the application.

The bootloaders support two types of updates:

* Direct updates, where an in-place substitution of the image takes place.
* Background updates, where the updated image is obtained and stored, but the update is completed later on.

You can deliver the updated images to the device in two ways:

* Wired, where updates are sent through a wired connection, like UART, or delivered by connecting a flash device.
* Over-the-air (OTA), where updates are sent through a wireless connection, like Bluetooth® Low Energy.

You can use a second-stage bootloader only in combination with a first-stage one.
Also, not all bootloaders supported by the |NCS| can be used as either first-stage or second-stage ones.
You can find an overview of currently supported bootloaders in the table below:

.. _app_bootloaders_support_table:

.. list-table:: Bootloaders supported by |NCS|
   :widths: auto
   :header-rows: 1

   * - Bootloader
     - Can be first-stage
     - Can be second-stage
     - Key type support
     - Public key revocation
     - SMP updates
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
     - Yes
     - :doc:`See imgtool <mcuboot:imgtool>`
     - No
     - Yes
     - Yes
     - :ref:`Monotonic (HW) <bootloader_monotonic_counter>`, :ref:`Semantic (SW) <ug_fw_update_image_versions_mcuboot>`
     - Image swap - single primary
       Dual-slot direct-xip

See the following user guides to learn more:

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   bootloader
   bootloader_adding
   bootloader_testing
   bootloader_external_flash
   bootloader_config
   fw_update
