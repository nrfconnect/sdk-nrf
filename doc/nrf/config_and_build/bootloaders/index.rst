.. _app_bootloaders:

Bootloaders
###########

.. contents::
   :local:
   :depth: 2

The bootloader is a firmware image that is responsible for booting the application.
The bootloader chooses an application firmware image to boot and verifies it to ensure validity.
The image validation can be used to ensure :ref:`chain of trust <ug_bootloader_chain_of_trust>`.

Using a bootloader is closely related to :ref:`device firmware update (DFU) <app_dfu>`.

There are two types of bootloaders used by the |NCS|:

* :ref:`Immutable first-stage bootloaders <immutable_bootloader>` that cannot be upgraded through DFU and run after each reset.
* :ref:`Upgradable second-stage bootloaders <upgradable_bootloader>` that can perform DFU to both themselves and the application, and are booted by the first-stage bootloader.

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
     - SMP updates by the application
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
     - Yes (only with :ref:`NSIB <bootloader>` as first-stage)
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
   bootloader_adding_sysbuild
   bootloader_testing
   bootloader_external_flash
   bootloader_config
   bootloader_signature_keys
   bootloader_downgrade_protection
