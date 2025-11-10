.. _ug_nrf54h20_ironside:

IronSide Secure Element
#######################

.. contents::
   :local:
   :depth: 2

The IronSide Secure Element (|ISE|) is a firmware for the :ref:`Secure Domain <ug_nrf54h20_secure_domain>` of the nRF54H20 SoC that provides security features based on the :ref:`PSA Certified Security Framework <ug_psa_certified_api_overview>`.

|ISE| provides the following features:

* Global memory configuration
* Peripheral configuration (through UICR.PERIPHCONF)
* Boot commands (ERASEALL, DEBUGWAIT)
* An alternative boot path with a Secondary firmware
* CPUCONF service
* Update service
* PSA Crypto service - see also :ref:`ug_crypto_architecture_implementation_standards_ironside`
* PSA Internal Trusted Storage service

.. _ug_nrf54h20_ironside_se_programming:

Programming |ISE| on the nRF54H20 SoC
*************************************

|ISE| is released independently from the |NCS| release cycle and is bundled in a ZIP archive that contains the following components:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Component
     - File
     - Description
   * - IronSide SE update firmware
     - :file:`ironside_se_update.hex`
     - The main |ISE| firmware
   * - IronSide SE Recovery update firmware
     - :file:`ironside_se_recovery_update.hex`
     - The recovery firmware. The firmware does not yet do anything.
   * - Update application
     - :file:`update_application.hex`
     - The :zephyr:code-sample:`application firmware <nrf_ironside_update>` used to update |ISE|. See :ref:`ug_nrf54h20_ironside_se_update_manual`.

For instructions on how to program |ISE|, see :ref:`ug_nrf54h20_SoC_binaries`.

By default, the nRF54H20 SoC uses the following memory and access configurations:

* MRAMC configuration: MRAM operates in Direct Write mode.
* MPC configuration: All memory not reserved by Nordic firmware is accessible with read, write, and execute (RWX) permissions by any domain.
* TAMPC configuration: The access ports (AP) for the local domains are enabled, allowing direct programming of all the memory not reserved by Nordic firmware in the default configuration.


.. note::
   The Radio Domain AP is only usable when the Radio domain has booted.

.. note::
   Access to external memory (EXMIF) requires a non-default configuration of the GPIO.CTRLSEL register.

Global domain memory can be protected from write operations by configuring UICR registers.
To remove these protections and disable all other protection mechanisms enforced through UICR settings, perform an ``ERASEALL`` operation.

.. _ug_nrf54h20_ironside_se_update:

Updating |ISE|
**************

|NCS| supports two methods for updating the |ISE| firmware on the nRF54H20 SoC:

* Using the ``west`` command.
  You can use the ``west`` command provided by the |NCS| to install the firmware update.
  For step-by-step instructions, see :ref:`ug_nrf54h20_ironside_se_update_west`.

* Using the nRF Util `device command <Device command overview_>`_
  Alternatively, you can perform the update by manually executing the same steps carried out by the ``west`` command.
  For step-by-step instructions, see :ref:`ug_nrf54h20_ironside_se_update_manual`.

.. caution::
   You cannot update |ISE| from a SUIT-based (up to 0.9.6) to an |ISE|-based (20.0.0 and onwards) version.

.. _ug_nrf54h20_ironside_se_update_west:

Updating using west
===================

To update the |ISE| firmware, you can use the ``west ncs-ironside-se-update`` command with the following syntax:

.. code-block:: console

   west ncs-ironside-se-update --zip <path_to_soc_binaries.zip> --allow-erase

The command accepts the following main options:

* ``--zip`` (required) - Sets the path to the nRF54H20 SoC binaries ZIP file.
* ``--allow-erase`` (required) - Enables erasing the device during the update process.
* ``--serial`` - Specifies the serial number of the target device.
* ``--firmware-slot`` - Updates only a specific firmware slot (``uslot`` for |ISE| or ``rslot`` for |ISE| Recovery).
* ``--wait-time`` - Specifies the timeout in seconds to wait for the device to boot (default: 2.0 seconds).

.. _ug_nrf54h20_ironside_se_update_manual:

Updating manually
=================

The manual update process involves the following steps:

1. Executing the update application.
   The :zephyr:code-sample:`update application <nrf_ironside_update>` runs on the application core and communicates with |ISE| using the :ref:`update service <ug_nrf54h20_ironside_se_update_service>`.
   It reads the update firmware from memory and passes the update blob metadata to the |ISE|.
   The |ISE| validates the update parameters and writes the update metadata to the Secure Information Configuration Registers (SICR).

#. Installing the update.
   After a reset, the Secure Domain ROM (SDROM) detects the pending update through the SICR registers, verifies the update firmware signature, and installs the new firmware.

#. Completing the update.
   The system boots with the updated |ISE| firmware, and the update status can be read to verify successful installation.

Updating manually using nRF Util
--------------------------------

You can use nRF Util instead of ``west ncs-ironside-se-update``.
If you decide to use nRF Util, make sure to install the `device` command v2.14.0 or higher.
See `Installing specific versions of nRF Util commands`_ for more information.

To perform the manual update process using nRF Util's `device command <Device command overview_>`_ command, complete the following steps:

1. Extract the update bundle:

   .. code-block:: console

      unzip <soc_binaries.zip> -d /tmp/update_dir

#. Erase non-volatile memory:

   .. code-block:: console

      nrfutil device recover --serial-number <serial>

#. Program the update application:

   .. code-block:: console

      nrfutil device program --firmware /tmp/update_dir/update/update_application.hex --serial-number <serial>

#. Program the |ISE| update firmware:

   .. code-block:: console

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware /tmp/update_dir/update/ironside_se_update.hex --serial-number <serial>

#. Reset to execute the update service:

   .. code-block:: console

      nrfutil device reset --serial-number <serial>

#. Reset through Secure Domain to trigger the installation of the update:

   .. code-block:: console

      nrfutil device reset --reset-kind RESET_VIA_SECDOM --serial-number <serial>

#. If you are updating both slots, complete the following additional steps:
    
   a. Program the |ISE| Recovery update firmware:

      .. code-block:: console

         nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware /tmp/update_dir/update/ironside_se_recovery_update.hex --serial-number <serial>

   b. Reset again to execute the update service:

      .. code-block:: console
        
         nrfutil device reset --serial-number <serial>
    
    c. Reset again through Secure Domain to trigger the installation of the update:
    
       .. code-block:: console
 
          nrfutil device reset --reset-kind RESET_VIA_SECDOM --serial-number <serial>
      

#. Erase the update application (regardless if you update one or both slots):

   .. code-block:: console

      nrfutil device erase --all --serial-number <serial>

.. _ug_nrf54h20_ironside_se_uicr:

Global Resource configuration
*****************************

|ISE| uses the User Information Configuration Registers (UICR) to securely configure persistent global resource settings.
The UICR is a dedicated non-volatile memory region (and its associated registers) located in the first NVR page (NVR0).
It holds device-specific configuration data, such as application-owned memory boundaries and selected peripheral parameters, which are used to configure the system before the application starts.

.. note::
   When you configure a custom devicetree for your board, the build system automatically generates UICR content based on the devicetree configuration.

The following UICR fields are supported:

+----------------------+---------------------------------------------------------------------+
| UICR Field           | Description                                                         |
+======================+=====================================================================+
| UICR.VERSION         | A 32-bit value that encodes the UICR format version (16-bit major   |
|                      | and 16-bit minor).                                                  |
+----------------------+---------------------------------------------------------------------+
| UICR.LOCK            | Locks all contents of NVR0, preventing any further writes without   |
|                      | performing an ERASEALL operation.                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.APPROTECT       | Configures debugger and access-port permissions for each AP via the |
|                      | TAMPC peripheral.                                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.ERASEPROTECT    | Blocks ERASEALL commands to NVR0.                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.PROTECTEDMEM    | Defines the size (in 4 KiB blocks) of an integrity-checked memory   |
|                      | region at the start of the application-owned part of MRAM.          |
+----------------------+---------------------------------------------------------------------+
| UICR.PERIPHCONF      | Points to an array of key-value entries used to initialize approved |
|                      | global peripherals.                                                 |
+----------------------+---------------------------------------------------------------------+
| UICR.MPCCONF         | Not supported yet.                                                  |
+----------------------+---------------------------------------------------------------------+
| UICR.WDTSTART        | Configures automatic start of a local watchdog timer before the     |
|                      | application core is booted, providing early system protection.      |
+----------------------+---------------------------------------------------------------------+
| UICR.SECURESTORAGE   | Defines secure storage configuration including address, and         |
|                      | partition sizes for cryptographic and ITS services.                 |
+----------------------+---------------------------------------------------------------------+
| UICR.SECONDARY       | Configures secondary firmware boot settings including processor     |
|                      | selection, triggers, memory protection, and peripheral access.      |
+----------------------+---------------------------------------------------------------------+

.. note::
   If no UICR values are programmed, |ISE| applies a set of default configurations.
   Applications that do not require custom settings can rely on these defaults without modifying the UICR.

UICR image generation
=====================

When applications need application-specific UICR contents they may use the UICR image.
The UICR image is an automatically created build artifact that generates the UICR content and peripheral configuration (PERIPHCONF) entries.

What is the UICR image
----------------------

The UICR image will generate HEX files at ``build/uicr/zephyr/`` inside the build directory.

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Component
     - File
     - Description
   * - Merged UICR image
     - :file:`zephyr.hex`
     - The merged hex file contains all other hex files. This file is automatically programmed when using ``west flash``.
   * - UICR-only image
     - :file:`uicr.hex`
     - Contains only the UICR configuration structure.
   * - PERIPHCONF-only image
     - :file:`periphconf.hex`
     - Contains only the PERIPHCONF entries. Generated when :kconfig:option:`CONFIG_GEN_UICR_GENERATE_PERIPHCONF` is enabled.
   * - Secondary PERIPHCONF image
     - :file:`secondary_periphconf.hex`
     - Contains only the secondary firmware's PERIPHCONF entries. Generated when :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF` is enabled.

UICR image generation process
-----------------------------

The UICR image generation process:

1. Devicetree analysis: The build system analyzes all devicetrees in the sysbuild build to identify peripheral usage and pin assignments across all images.
2. PERIPHCONF generation: Python scripts generate C source code containing UICR configuration macros based on the devicetree.
3. Compilation: The generated C code is compiled into ELF binaries containing PERIPHCONF sections.
4. ELF extraction: The UICR generator extracts PERIPHCONF data from ELF files and merges them together.
5. Kconfig extraction: The UICR generator uses an application-specific UICR Kconfig to populate the UICR entries.
6. HEX file creation: Multiple HEX files are generated for different purposes. See the table above for details.

Configuring the UICR sysbuild image
-----------------------------------

Applications that use UICR features must place a :file:`sysbuild/uicr.conf` Kconfig fragment file in their project directory to configure the Kconfig of the UICR image.

The following code snippet shows an example configuration for secondary firmware support in :file:`sysbuild/uicr.conf`:

.. code-block:: kconfig

   # Configuration overlay for uicr image
   # Enable UICR.SECONDARY.ENABLE
   CONFIG_GEN_UICR_SECONDARY=y

For more examples of UICR configuration, see :ref:`ironside_se`:

* :ref:`secondary_boot_sample` - Basic secondary firmware configuration
* :ref:`secondary_boot_trigger_lockup_sample` - Secondary firmware with automatic triggers

UICR Kconfig configuration options
----------------------------------

The UICR Kconfig options are listed below, and are defined with more detailed Kconfig help texts in :file:`zephyr/soc/nordic/common/uicr/gen_uicr/Kconfig`.

.. list-table:: UICR Kconfig Options
   :widths: 50 50
   :header-rows: 1

   * - Kconfig option
     - Description
   * - :kconfig:option:`CONFIG_GEN_UICR_GENERATE_PERIPHCONF`
     - Generate a PERIPHCONF hex in addition to the UICR hex
   * - :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE`
     - Enable UICR.SECURESTORAGE
   * - :kconfig:option:`CONFIG_GEN_UICR_LOCK`
     - Enable UICR.LOCK
   * - :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`
     - Enable UICR.ERASEPROTECT
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`
     - Protect application domain access port
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED`
     - Protect radio core access port
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED`
     - Disable CoreSight subsystem
   * - :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM`
     - Enable UICR.PROTECTEDMEM
   * - :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM_SIZE_BYTES`
     - Protected memory size in bytes
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART`
     - Enable UICR.WDTSTART
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT0`
     - Use watchdog timer instance 0
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT1`
     - Use watchdog timer instance 1
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_CRV`
     - Initial Counter Reload Value (CRV) for the watchdog timer
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY`
     - Enable UICR.SECONDARY.ENABLE
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF`
     - Generate SECONDARY.PERIPHCONF hex alongside UICR
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROCESSOR_APPLICATION`
     - Boot secondary firmware on the APPLICATION processor
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROCESSOR_RADIOCORE`
     - Boot secondary firmware on the RADIOCORE processor
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER`
     - Enable UICR.SECONDARY.TRIGGER
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT0`
     - Trigger on Application domain watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT1`
     - Trigger on Application domain watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP`
     - Trigger on Application domain CPU lockup reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT0`
     - Trigger on Radio core watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT1`
     - Trigger on Radio core watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCORELOCKUP`
     - Trigger on Radio core CPU lockup reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART`
     - Enable UICR.SECONDARY.WDTSTART
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_INSTANCE_WDT0`
     - Use watchdog timer instance 0 for secondary firmware
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_INSTANCE_WDT1`
     - Use watchdog timer instance 1 for secondary firmware
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_CRV`
     - Secondary initial Counter Reload Value (CRV) for the watchdog timer
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`
     - Enable UICR.SECONDARY.PROTECTEDMEM
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM_SIZE_BYTES`
     - Secondary protected memory size in bytes

UICR.VERSION
============

UICR.VERSION specifies the version of the UICR format in use.
It is divided into a 16-bit major version and a 16-bit minor version.

This versioning scheme allows IronSide to support multiple UICR formats, enabling updates to the format without breaking compatibility with existing configurations.

UICR.LOCK
=========

Enabling :kconfig:option:`CONFIG_GEN_UICR_LOCK` locks the entire contents of the NVR0 page located in MRAM10.
This includes all values in both the UICR and the BICR (the Board Information Configuration Registers).
When :kconfig:option:`CONFIG_GEN_UICR_LOCK` is enabled, you can modify the UICR only by performing an ERASEALL operation.

.. note::
   While BICR is not erased during an ERASEALL operation, executing ERASEALL lifts the :kconfig:option:`CONFIG_GEN_UICR_LOCK` restriction, allowing write access to BICR.

Locking is enforced through an integrity check and by configuring the NVR page as read-only in the MRAMC.

If the integrity check fails, the application CPU subsystem will still be given clock and power, but the CPU will be stalled and prevented from executing instructions.
It is not possible to boot the secondary firmware if the integrity check fails.

.. _ug_nrf54h20_ironside_se_uicr_approtect:

UICR.APPROTECT
==============

The UICR.APPROTECT configuration consists of the following sub-registers:

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`
  Controls access port protection for the application domain processor.
  This setting determines whether the debugger can access the application domain memory, registers, and debug features.

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED`
  Controls access port protection for the radio core processor.
  This setting determines whether the debugger can access the radio core memory, registers, and debug features.

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED`
  Controls access port protection for the CoreSight debug infrastructure.
  This setting determines whether system-level trace features are accessible.

.. note::
   To fully protect a production device, enable all three Kconfig options.

UICR.ERASEPROTECT
=================

Enabling :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` blocks the ERASEALL operation.
However, it does not prevent erase operations initiated through other means, such as writing erase values via a debugger.

.. note::
   If this configuration is enabled and :kconfig:option:`CONFIG_GEN_UICR_LOCK` is also set, it is no longer possible to modify the UICR in any way.
   Therefore, this configuration should only be enabled during the final stages of production.

UICR.PROTECTEDMEM
=================

Enabling :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` enables UICR.PROTECTEDMEM.
UICR.PROTECTEDMEM can be used to specify a memory region that will have its integrity ensured by |ISE|.
This memory can contain immutable bootloaders, UICR.PERIPHCONF entries, or any other data that should be immutable.
By ensuring the integrity of this memory region, |ISE| extends the Root of Trust to any immutable bootloader located in this region.

The memory region always starts at the lowest MRAM address of the application-owned memory and has an application-specific size specified in :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM_SIZE_BYTES`.

UICR.PERIPHCONF
===============

UICR.PERIPHCONF points to an array of key-value pairs used to initialize specific global peripherals before the application starts.
This mechanism allows for the one-time configuration of peripherals managed by |ISE| and is not designed for general system initialization.

Each entry in the array consists of two 32-bit values.
The fields in each value are described in the following tables.

Value 0:

+-------------------+----------------------------------+-------------------------+
| Bit number(s)     | 31-2                             | 1-0                     |
+-------------------+----------------------------------+-------------------------+
| Field             | REGPTR                           | UNUSED                  |
+-------------------+----------------------------------+-------------------------+
| Description       | Bits [31:2] of a pointer to a    | Unused.                 |
|                   | peripheral register.             |                         |
+-------------------+----------------------------------+-------------------------+

Value 1:

+-------------------+----------------------------------+
| Bit number(s)     | 31-0                             |
+-------------------+----------------------------------+
| Field             | VALUE                            |
+-------------------+----------------------------------+
| Description       | Register value.                  |
+-------------------+----------------------------------+

|ISE| processes the PERIPHCONF array sequentially, starting from the address specified by UICR.PERIPHCONF.ADDRESS.
Processing continues until either the number of entries defined by UICR.PERIPHCONF.MAXCOUNT has been processed, or an entry is encountered with the REGPTR field set to 0x3FFF_FFFF (all ones), which indicates the end of the array.

|ISE| uses an allow list to determine which register addresses the REGPTR field is permitted to reference.
Each register address in the allow list has an associated bit mask that specifies which bits from the VALUE field are applied to the target register.

Given an entry in the PERIPHCONF array and a bit mask M, |ISE| performs the following write operation::

   *(REGPTR << 2) = (VALUE & M) | (*(REGPTR << 2) & ~M)

The register allow list and corresponding bit masks are documented with each |ISE| release.

Each entry in the PERIPHCONF array is validated during processing.
To pass validation, (REGPTR << 2) must point to a register address included in the allow list.

After applying the entry, |ISE| performs a read-back check: it reads back the register value, applies the bit mask, and compares the result against the masked VALUE field.

The configuration procedure is aborted if an entry fails either the validation or the read-back check.
If a failure occurs, BOOTSTATUS.BOOTERROR is set to indicate the error condition, and a description of the failed entry is written to the boot report.

Peripheral configuration using nrf-regtool
------------------------------------------

The ``nrf-regtool`` utility generates a UICR.PERIPHCONF configuration from the devicetree.
To determine which peripherals are in use, it analyzes the devicetree as follows:

#. Enumerate all peripheral nodes and include only those with a ``status`` property set to ``okay``.
#. Parse peripheral-specific attributes (for example, the ``owned-channels`` property in DPPIC nodes).
#. Collect GPIO pin assignments from all pin references (for example, ``pinctrl`` entries).

It then generates the appropriate configuration values by reusing existing properties.

See the following table for a mapping between the devicetree input used by ``nrf-regtool`` and the resulting output in the automatically migrated :file:`periconf_migrated.c` file.

.. list-table:: Mapping between devicetree and Migrated PERIPHCONF output (UICR Configuration)
   :header-rows: 1
   :widths: 25 15 35 25

   * - Devicetree node type
     - Properties
     - Migrated PERIPHCONF output
     - Example generated output
   * - Peripheral Access Control
     -
     -
     -
   * - Nordic global domain peripheral with status ``= {"okay", "reserved"}``
     - ``reg``

       ``interrupt-parent``
     - SPU Peripheral Permissions:
       UICR_SPU_PERIPH_PERM_SET(...) sets ownership and secure attribute based on bit 28 of bus parent or peripheral address.

       IRQ Routing:
       UICR_IRQMAP_IRQ_SINK_SET(...) maps interrupt to processor owning the interrupt controller or devicetree processor.
     -
       .. code-block:: c

          /* SPU137 configuration for uart136 */
          UICR_SPU_PERIPH_PERM_SET(0x5f9d0000UL, 5, true, true, NRF_OWNER_APPLICATION);
          /* uart136 IRQ => APPLICATION */
          UICR_IRQMAP_IRQ_SINK_SET(469, NRF_PROCESSOR_APPLICATION);
   * - Channel-Based Features
     -
     -
     -
   * - Nordic global domain GPIOTE peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - GPIOTE Channel Control:
       UICR_SPU_FEATURE_GPIOTE_CH_SET(...) sets channel ownership to devicetree processor. Secure attribute from explicit specification or address logic.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpiote130 ch. 0 */
          UICR_SPU_FEATURE_GPIOTE_CH_SET(0x5f920000UL, 0, 0, true, NRF_OWNER_APPLICATION);
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - DPPIC Channel Control:
       UICR_SPU_FEATURE_DPPIC_CH_SET(...) configures channel ownership and security.
     -
       .. code-block:: c

          /* SPU131 feature configuration for DPPIC130 ch. 0 */
          UICR_SPU_FEATURE_DPPIC_CH_SET(0x5f920000UL, 0, false, NRF_OWNER_RADIOCORE);
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channel-groups``

       ``nonsecure-channel-groups``
     - DPPIC Channel Group Control:
       UICR_SPU_FEATURE_DPPIC_CHG_SET(...) configures channel group ownership and security.
     -
       .. code-block:: c

          /* SPU131 feature configuration for DPPIC130 ch. group 0 */
          UICR_SPU_FEATURE_DPPIC_CHG_SET(0x5f920000UL, 0, true, NRF_OWNER_APPLICATION);
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``sink-channels``

       ``source-channels``
     - PPIB Cross-Domain Connection:
       UICR_PPIB_SUBSCRIBE_SEND_ENABLE(...) and UICR_PPIB_PUBLISH_RECEIVE_ENABLE(...) connect PPI domains. Property name determines connection direction. (Ignored for DPPIC130)
     -
       .. code-block:: c

          /* PPIB133 ch. 0 => PPIB130 ch. 8 */
          UICR_PPIB_SUBSCRIBE_SEND_ENABLE(0x5f99d000UL, 0);
          UICR_PPIB_PUBLISH_RECEIVE_ENABLE(0x5f925000UL, 8);
   * - Nordic global domain IPCT peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - IPCT Channel Control:
       UICR_SPU_FEATURE_IPCT_CH_SET(...) sets channel ownership and security attributes.
     -
       .. code-block:: c

          /* SPU131 feature configuration for ipct130 ch. 0 */
          UICR_SPU_FEATURE_IPCT_CH_SET(0x5f920000UL, 0, true, NRF_OWNER_RADIOCORE);
   * - Nordic IPCT peripheral with status ``= {"okay", "reserved"}``
     - ``source-channel-links``

       ``sink-channel-links``
     - IPC Domain Mapping:
       UICR_IPCMAP_CHANNEL_CFG(...) connects channels between domains.
     -
       .. code-block:: c

          /* RADIOCORE IPCT ch. 2 => GLOBALSLOW IPCT ch. 2 */
          UICR_IPCMAP_CHANNEL_CFG(0, NRF_DOMAIN_RADIOCORE, 2, NRF_DOMAIN_GLOBALSLOW, 2);
   * - Nordic GRTC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - GRTC Compare Channel Control:
       UICR_SPU_FEATURE_GRTC_CC_SET(...) configures compare channel ownership and security.
     -
       .. code-block:: c

          /* SPU133 feature configuration for GRTC CC4 */
          UICR_SPU_FEATURE_GRTC_CC_SET(0x5f990000UL, 4, true, NRF_OWNER_APPLICATION);
   * - GPIO Pin Control
     -
     -
     -
   * - Nodes with GPIO pin properties
     - Any property with type ``phandle-array`` named *gpios* or ending with *-gpios*
     - GPIO Pin Ownership + Multiplexing:
       UICR_SPU_FEATURE_GPIO_PIN_SET(...) sets pin ownership. UICR_GPIO_PIN_CNF_CTRLSEL_SET(...) configures pin multiplexer using internal lookup table.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio9, P9.0 */
          UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 9, 0, true, NRF_OWNER_APPLICATION);
          /* gpio9 - P9.0 CTRLSEL = 0 */
          UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5f939200UL, 0, 0);
   * - Nodes with pinctrl configuration properties
     - Pinctrl configuration properties ("pinctrl-0", "pinctrl-1", etc.)
     - Pin Function Control:
       UICR_SPU_FEATURE_GPIO_PIN_SET(...) for ownership. UICR_GPIO_PIN_CNF_CTRLSEL_SET(...) for function-specific multiplexing.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio6, P6.0 */
          UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 6, 0, true, NRF_OWNER_APPLICATION);
          /* gpio6 - P6.0 CTRLSEL = 4 */
          UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5f938c00UL, 0, 4);
   * - Nordic SAADC peripheral
     - ``zephyr,input-positive``

       ``zephyr,input-negative``
     - Analog Pin Control:
       UICR_SPU_FEATURE_GPIO_PIN_SET(...) for pin ownership. UICR_GPIO_PIN_CNF_CTRLSEL_SET(...) for analog function.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio0, P0.4 */
          UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 0, 4, true, NRF_OWNER_APPLICATION);
          /* gpio0 - P0.4 CTRLSEL = 5 */
          UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5f938000UL, 4, 5);
   * - Nordic COMP/LPCOMP peripherals
     - ``psel``

       ``extrefsel``
     - Comparator Pin Control:
       UICR_SPU_FEATURE_GPIO_PIN_SET(...) for pin ownership. UICR_GPIO_PIN_CNF_CTRLSEL_SET(...) for comparator function.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio1, P1.2 */
          UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 1, 2, true, NRF_OWNER_APPLICATION);
          /* gpio1 - P1.2 CTRLSEL = 3 */
          UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5f938400UL, 2, 3);

UICR.WDTSTART
=============

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART` enables the automatic start of a local watchdog timer before the application core is booted.
This provides early system protection ensuring that the system can recover from early boot failures.

The UICR.WDTSTART configuration consists of three sub-registers:

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART`
  Controls whether the watchdog timer automatic start feature is enabled.

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT0` / :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT1`
  Specifies which watchdog timer instance to configure and start.
  The following are valid values:

  * ``WDT0`` - Use watchdog timer instance 0
  * ``WDT1`` - Use watchdog timer instance 1

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART_CRV`
  Sets the initial Counter Reload Value (CRV) for the watchdog timer.
  This value determines the watchdog timeout period.
  The CRV must be at least 15 (0xF) to ensure proper watchdog operation.

UICR.SECURESTORAGE
==================

:kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` enables the secure storage system used by |ISE| for persistent storage of cryptographic keys and trusted data.
The secure storage is divided into separate partitions for different services and processor domains.
The total size of all configurations specified in ``UICR.SECURESTORAGE.*`` must be aligned to a 4 KB boundary.
For more information, see :ref:`ug_nrf54h20_ironside_se_secure_storage`.

The UICR.SECURESTORAGE configuration consists of the following sub-registers:

UICR.SECURESTORAGE.ENABLE
  Controls whether the secure storage feature is enabled.

UICR.SECURESTORAGE.ADDRESS
  Specifies the start address of the secure storage region in memory.
  This address must be aligned to a 4 KB boundary and must point to a valid memory region that can be used for secure storage.

UICR.SECURESTORAGE.CRYPTO
  Configures partition sizes for the cryptographic service within the secure storage.

  UICR.SECURESTORAGE.CRYPTO.APPLICATIONSIZE1KB
    Sets the size of the ``APPLICATION`` domain partition for cryptographic storage, specified in 1 kiB blocks.

  UICR.SECURESTORAGE.CRYPTO.RADIOCORESIZE1KB
    Sets the size of the ``RADIOCORE`` domain partition for cryptographic storage, specified in 1 kiB blocks.

UICR.SECURESTORAGE.ITS
  Configures partition sizes for the Internal Trusted Storage (ITS) service within the secure storage.

  UICR.SECURESTORAGE.ITS.APPLICATIONSIZE1KB
    Sets the size of the ``APPLICATION`` domain partition for ITS, specified in 1 kiB blocks.

  UICR.SECURESTORAGE.ITS.RADIOCORESIZE1KB
    Sets the size of the ``RADIOCORE`` domain partition for ITS, specified in 1 kiB blocks.

UICR.SECONDARY
==============

:kconfig:option:`CONFIG_GEN_UICR_SECONDARY` configures the secondary firmware boot system, which allows |ISE| to boot alternative firmware in response to specific conditions or triggers.
This feature enables a recovery firmware setup through a dual-firmware configuration that includes both main and recovery firmware.

The UICR.SECONDARY configuration consists of multiple sub-registers organized into functional groups:

UICR.SECONDARY.ENABLE
  Controls whether the secondary firmware boot feature is enabled.

UICR.SECONDARY.PROCESSOR
  Specifies which processor should be used to boot the secondary firmware.
  Valid values are:

  * ``APPLICATION`` - Boot secondary firmware on the application domain CPU.
  * ``RADIOCORE`` - Boot secondary firmware on the radio core CPU.

UICR.SECONDARY.ADDRESS
  Sets the start address of the secondary firmware.
  This value is used as the initial value of the secure Vector Table Offset Register (VTOR) after CPU reset.
  The address must be aligned to a 4 KiB boundary.
  Bits [11:0] are ignored.

UICR.SECONDARY.TRIGGER
  Configures automatic triggers that cause |ISE| to boot the secondary firmware instead of the primary firmware.

  UICR.SECONDARY.TRIGGER.ENABLE
    Controls whether automatic triggers are enabled to boot the secondary firmware.

  UICR.SECONDARY.TRIGGER.RESETREAS
    Specifies which reset reasons will trigger an automatic boot into the secondary firmware.
    Multiple triggers can be enabled simultaneously by setting the corresponding bits:

    * ``APPLICATIONWDT0`` - Application domain watchdog 0 reset
    * ``APPLICATIONWDT1`` - Application domain watchdog 1 reset
    * ``APPLICATIONLOCKUP`` - Application domain CPU lockup reset
    * ``RADIOCOREWDT0`` - Radio core watchdog 0 reset
    * ``RADIOCOREWDT1`` - Radio core watchdog 1 reset
    * ``RADIOCORELOCKUP`` - Radio core CPU lockup reset

UICR.SECONDARY.PROTECTEDMEM
  Identical to :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM`, but applies to the secondary firmware.

UICR.SECONDARY.WDTSTART
  Identical to :kconfig:option:`CONFIG_GEN_UICR_WDTSTART`, but applies to the secondary firmware boot process.
  Note that if RADIOCORE is specified in ``UICR.SECONDARY.PROCESSOR``, the WDT instances used are the ones in the radio core.

UICR.SECONDARY.PERIPHCONF
  Identical to UICR.PERIPHCONF, but applies to the secondary firmware boot process.


.. _ug_nrf54h20_ironside_se_debug:

Debugging
*********

|ISE| provides the ``DEBUGWAIT`` boot command to halt the application core immediately after reset.
This ensures that a debugger can attach and take control from the very first instruction.

When ``DEBUGWAIT`` is enabled, |ISE| sets the application domain's CPUWAIT when the application core starts.
This prevents the CPU from executing any instructions until a debugger manually releases it.

.. note::
   You can also use the ``cpuconf`` service to set CPUWAIT when booting other cores.

.. _ug_nrf54h20_ironside_se_protecting:

Protecting the device
*********************

To protect the nRF54H20 SoC in a production-ready device, you must enable the following UICR-based security mechanisms.
For information on how to configure these UICR settings, see :ref:`ug_nrf54h20_ironside_se_uicr`.

* :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED` - Disables all debug and AP access.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* :kconfig:option:`CONFIG_GEN_UICR_LOCK` - Freezes non-volatile configuration registers.
  It locks the UICR, ensuring that no further UICR writes are possible without issuing an `ERASEALL` command.
* :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` - Enforces integrity checks on critical code and data.
  It defines a trailing region of application-owned MRAM whose contents are integrity-checked at each boot, extending the root of trust to your immutable bootloader or critical data.
* :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` - Prevent bulk erasure of protected memory.
  It blocks all `ERASEALL` operations on NVR0, preserving UICR settings even if an attacker attempts a full-chip erase.


.. _ug_nrf54h20_ironside_se_boot_report:

IronSide boot report
********************

The IronSide boot report contains device state information communicated from |ISE| to the local domains.
It is written to a reserved region in RAM20, which is accessible to the local domain in the default system configuration.
There is one boot report per processor that is booted, either directly by |ISE| or via the CPUCONF service.

The boot report contains the following information:

* Magic value
* |ISE| version
* |ISE| recovery version
* |ISE| update status
* UICR error description
* Context data passed to the CPUCONF service
* A fixed amount of random bytes generated by a CSPRNG
* Universal Unique Identifier (UUID) data

Versions
========

The boot report includes version information for both |ISE| and |ISE| Recovery.

The regular version format consists of four fields: ``MAJOR.MINOR.PATCH.SEQNUM``, with each field occupying 8 bits.
The first three fields follow semantic versioning, while the ``SEQNUM`` field is a wrapping sequence number that increments by one with each version.
The values ``0`` and ``127`` are reserved for ``SEQNUM``.

An additional version field, referred to as the *extra version*, contains a null-terminated ASCII string with human-readable version information.
This string is informational only, and no semantics should be attached to this part of the version.

|ISE| update status
===================

The |ISE| boot ROM code (SDROM) reports the status of an |ISE| update request through SICR.UROT.UPDATE.STATUS.
The value of this register is copied to the |ISE| update status field of the boot report.

.. note::
   After an update is installed or attempted, |ISE| resets the update status to ``0xFFFFFFFF`` on the next boot.
   This means that the update status is only valid for a single execution.

UICR error description
======================

This field indicates if any UICR error occurred.

Local domain context
====================

This field is populated by the local domain that is invoking the CPUCONF service.
It is set to `0` for the application core which is booted by |ISE|.
This service is used when one local domain boots another local domain.
The caller can populate this field with arbitrary data that will be made available to the local domain being booted.
Typical examples of data that could be passed include IPC buffer sizes or the application firmware version.
The unused parts of this field are set to 0.

Random data
===========

This field is filled with random data generated by a CSPRNG.
This data is suitable as a source of initial entropy.

UUID data
=========

This field contains 16 bytes that represent the device's unique identity.
The bytes are provided as a stream of 16 raw bytes, rather than in the standard ``8-4-4-4-12`` UUID format.

.. _ironside_se_booting:

Booting of local domains
************************

This section describes the default boot flow used by |ISE|.
For information about the alternative boot flow that uses the secondary firmware, see :ref:`ug_nrf54h20_ironside_se_secondary_firmware`.

|ISE| boots only the application core CPU.
The application core then triggers the boot of other local domain CPUs, such as the radio core, through the :ref:`ug_nrf54h20_ironside_se_cpuconf_service`.

Application domain boot sequence
================================

When booting the application domain, |ISE| performs the following operations:

* Sets the processor's vector table address to the start of the application-owned memory region.
* Verifies for firmware availability by reading the reset vector from the second 32-bit word of the vector table and comparing it to the erased value (``0xFFFFFFFF``).
* Sets the secure vector table offset register (INITSVTOR) to point to the vector table address.
* Enables the CPU with the appropriate start mode:

  * |ISE| enables the CPU in halted mode if any of the following conditions are met:

    * No firmware is available.
    * Boot errors occurred.
    * The ``DEBUGWAIT`` boot command was issued.
  * Otherwise, |ISE| enables and starts the CPU normally.

* Updates :ref:`CTRL_AP.BOOTSTATUS <ug_nrf54h20_ironside_se_bootstatus_register_format>` and writes the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` to reflect any boot errors encountered during the initialization process.

For more information on the boot sequence, see :ref:`ug_nrf54h20_architecture_boot`.

.. _ug_nrf54h20_ironside_se_secondary_firmware:

Secondary firmware
******************

The secondary firmware feature provides an alternative boot path that can be triggered implicitly or explicitly.
It can be used for different purposes, some examples are recovery firmware, analysis firmware, and DFU applications in systems that don't use dual banking.

For more information on the boot sequence, see :ref:`ug_nrf54h20_architecture_boot`.

.. note::
   The term "primary firmware" is rarely used when describing the firmware that is booted by default by |ISE|, as it is implicit when the term "secondary" is not specified.

.. note::
   The term "secondary slot" and "secondary image" are used in the MCUboot context.
   This usage is unrelated to the "secondary firmware" described in this section.

.. _ug_nrf54h20_ironside_se_secondary_conf_trigger:

Configuration
=============

To enable secondary firmware support, you must complete the following steps:

1. **Enable UICR secondary configuration**: Add the following to your ``sysbuild/uicr.conf`` file:

   .. code-block:: kconfig

      CONFIG_GEN_UICR_SECONDARY=y

2. **Create a secondary firmware Zephyr image**: Create a separate Zephyr application for your secondary firmware (e.g., in a ``secondary/`` directory).

3. **Add secondary image to sysbuild**: Include the secondary image as an external project in your ``sysbuild.cmake`` file:

   .. code-block:: cmake

      ExternalZephyrProject_Add(
        APPLICATION secondary
        SOURCE_DIR ${APP_DIR}/secondary
      )

4. **Configure secondary firmware**: The secondary firmware image itself must enable the appropriate Kconfig option to indicate it's a secondary firmware. Add the following to your secondary firmware's ``prj.conf``:

   .. code-block:: kconfig

      CONFIG_IS_IRONSIDE_SE_SECONDARY_IMAGE=y

5. **Optional: Configure automatic triggers**: If you want automatic triggering based on reset reasons, add trigger options to your ``sysbuild/uicr.conf``:

   .. code-block:: kconfig

      CONFIG_GEN_UICR_SECONDARY_TRIGGER=y
      CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP=y

For more examples of secondary firmware configuration, see the samples in :file:`nrf/samples/ironside_se/`:

* :ref:`secondary_boot_sample` - Basic secondary firmware configuration
* :ref:`secondary_boot_trigger_lockup_sample` - Secondary firmware with automatic triggers

Triggering
==========

The secondary firmware can be triggered automatically, through ``CTRLAP.BOOTMODE`` or through an IPC service (``ironside_bootmode`` service).
Any component that communicates with |ISE| over IPC can leverage this service.
Setting bit 5 in ``CTRLAP.BOOTMODE`` will also trigger secondary firmware.

|ISE| automatically triggers the secondary firmware in any of the following situations:

* The integrity check of the memory specified in :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` fails.
* Any boot failure occurs, such as missing primary firmware or failure to apply ``UICR.PERIPHCONF`` configurations.
* A local domain is reset with a reason configured to trigger the secondary firmware.
* Secondary firmware will be booted by |ISE| if one of the triggers configured in :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER` and related options occurs.

The secondary firmware can be protected using :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM` for integrity checking, and can be updated by other components when protection is not enabled.

Protection
==========

The secondary firmware can be protected through integrity checks by enabling :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`.
The ``PERIPHCONF`` entries for the secondary firmware can also be placed in memory covered by :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM` to create a fully immutable secondary firmware and configuration.

If the integrity check of the memory specified in this configuration fails, the secondary firmware will not be booted.
Instead, |ISE| will attempt to boot the primary firmware, and information about the failure is available in the boot report and boot status.

Update
======

As with the primary firmware, |ISE| does not facilitate updating the secondary firmware.
The secondary image can be updated by other components as long as :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM` is not set.
Using the secondary firmware as a bootloader capable of validating and updating a second image enables updating firmware in the secondary boot flow while having secure boot enabled through :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`.



.. _ug_nrf54h20_ironside_se_cpuconf_service:

|ISE| CPUCONF service
*********************

The |ISE| CPUCONF service enables the application core to trigger the boot of another CPU at a specified address.

Specifically, |ISE| sets INITSVTOR of the CPUCONF instance of the processor being booted with the address provided to the IronSide call, and then writes 0x1 to CPUSTART of the CPUCONF instance of the processor being booted to start the target CPU.
When CPUWAIT is enabled in the IronSide service call, the target CPU is stalled by writing 0x1 to CPUWAIT of the CPUCONF instance of the processor being booted.

This feature is intended for debugging purposes.

.. note::

   * TASKS_ERASECACHE of the CPUCONF instance of the processor being booted is not yet supported.
   * INITNSVTOR of the CPUCONF instance of the processor being booted will not be supported.

For details about the CPUCONF peripheral, refer to the nRF54H20 SoC datasheet.

.. _ug_nrf54h20_ironside_se_update_service:

|ISE| update service
********************

The |ISE| update service will update |ISE| itself.

|ISE| is updated by the Secure Domain ROM (SDROM), which performs the update operation when triggered by a set of SICR registers.
SDROM verifies and copies the update candidate specified through these registers.
SDROM requires the |ISE| update to be located in MRAM.

|ISE| exposes an update service that allows local domains to trigger the update process by indirectly writing to the relevant SICR registers.

.. note::
   The update data must be placed within a valid memory range.
   See :file:`nrf_ironside/update.h` for more details.

The release ZIP archive for |ISE| includes the following components:

* A HEX file containing the update candidate for |ISE|.
* A HEX file for |ISE| Recovery.
* An application core image that executes the |ISE| update service to install the update candidate HEX files.

The |NCS| defines the west ``ncs-ironside-se-update`` command to update |ISE| on a device via the debugger.
This command takes a nRF54H20 SoC binary ZIP file and uses the |ISE| update service to update both the |ISE| and |ISE| Recovery (or optionally just one of them).
For more information, see :ref:`abi_compatibility`.

.. _ug_nrf54h20_ironside_se_secure_storage:

Secure Storage
**************

|ISE| provides secure storage functionality through the :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` configuration.
This feature enables applications to store sensitive data in dedicated, encrypted storage regions that are protected by device-unique keys and access controls.

UICR.SECURESTORAGE Configuration
================================

The :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` field configures secure storage regions for PSA Crypto keys and PSA Internal Trusted Storage (ITS) data.
To leverage this secure storage functionality, applications must set the key location to ``PSA_KEY_LOCATION_LOCAL_STORAGE`` (``0x000000``).

The secure storage configuration includes two separate storage regions:

* **UICR.SECURESTORAGE.CRYPTO** - Used for PSA Crypto API operations when storing cryptographic keys
* **UICR.SECURESTORAGE.ITS** - Used for PSA Internal Trusted Storage (ITS) API operations when storing general secure data


Secure Storage through PSA Crypto API
=====================================

When using the PSA Crypto API to operate on keys, the storage region specified by ``UICR.SECURESTORAGE.CRYPTO`` is automatically used if the key attributes are configured with **key location** set to ``PSA_KEY_LOCATION_LOCAL_STORAGE``.

This ensures that cryptographic keys are stored in the dedicated secure storage region rather than in regular application memory.

Secure storage through PSA ITS API
==================================

When using the PSA ITS API for storing general secure data, the storage region specified by ``UICR.SECURESTORAGE.ITS`` is used automatically.
No special configuration is required for PSA ITS operations, as they inherently use the secure storage when available.

Security Properties
===================

The secure storage provided by |ISE| has the following security characteristics:

Access Control
--------------

* **Domain Isolation**: Secure storage regions are not accessible by local domains directly.
* **Ironside Exclusive Access**: Only the Ironside Secure Element can access the secure storage regions.
* **Domain Separation**: Each local domain can only access its own secure storage data, ensuring isolation between different domains.

Data Protection
---------------

* **Encryption**: All data stored in the secure storage regions is encrypted using device-unique keys.
* **Integrity**: The stored data is protected against tampering through cryptographic integrity checks.
* **Confidentiality**: The encryption ensures that stored data remains confidential even if the storage medium is physically accessed.

.. note::
   The device-unique encryption keys are managed entirely by |ISE| and are not accessible to application code.
   This ensures that the secure storage remains protected even in cases where application-level vulnerabilities exist.

Configuration Considerations
============================

When configuring secure storage, consider the following:

* Ensure sufficient storage space is allocated in both ``UICR.SECURESTORAGE.CRYPTO`` and ``UICR.SECURESTORAGE.ITS`` regions based on your application's requirements
* The sum of these to regions must be 4kB aligned.
* The secure storage regions should be properly sized to accommodate the expected number of keys and data items
* Access to secure storage is only available when the key location is explicitly set to ``PSA_KEY_LOCATION_LOCAL_STORAGE``

.. _ug_nrf54h20_ironside_se_bootmode_register_format:

CTRLAP.BOOTMODE register format
*******************************

.. _ironside_se_boot_commands:

The format of the CTRLAP.MAILBOX.BOOTMODE register is described in the following table.

+------------------+--------+------------------+-----+----------------+--------+------------+
| Bit numbers      | 31-8   | 7                | 6-5 | 4              | 3-1    | 0          |
+------------------+--------+------------------+-----+----------------+--------+------------+
| Field            | N/A    | Reserved         | RFU | SAFEMODE (ROM) | OPCODE | MODE (ROM) |
+------------------+--------+------------------+-----+----------------+--------+------------+

.. _ug_nrf54h20_ironside_se_bootstatus_register_format:

CTRLAP.BOOTSTATUS register format
*********************************

The general format of the CTRLAP.BOOTSTATUS register is described in the following table.

+------------------+-------+-----------+------+
| Bit numbers      | 31-28 | 27-24     | 23-0 |
+------------------+-------+-----------+------+
| Field            | RFU   | BOOTSTAGE | INFO |
+------------------+-------+-----------+------+

Fields marked as RFU (Reserved for Future Use) are set to 0, unless otherwise specified.
The BOOTSTAGE field indicates which component in the boot sequence encountered a failure.

If ``BOOTSTAGE`` is set to ``0xC`` or ``0xD``, the register has the following format:

+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+
| Bit numbers      | 31-28 | 27-24     | 23-22 | 21-15     | 14-12     | 11-9      | 8   | 7-0         |
+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+
| Field            | RFU   | BOOTSTAGE | RFU   | FWVERSION | CMDOPCODE | CMDERROR  | RFU | BOOTERROR   |
+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+

This field can have one of the following values:

+--------------------+--------------------------------------------------------------+
| BOOTSTAGE value    | Description                                                  |
+====================+==============================================================+
| 0x0                | Unset (reset value)                                          |
+--------------------+--------------------------------------------------------------+
| 0x1                | SysCtrl ROM                                                  |
+--------------------+--------------------------------------------------------------+
| 0x2                | Secure domain ROM                                            |
+--------------------+--------------------------------------------------------------+
| 0xB                | Secure domain firmware with SUIT (major version < 20)        |
+--------------------+--------------------------------------------------------------+
| 0xC                | Secure domain firmware (major version >= 20)                 |
+--------------------+--------------------------------------------------------------+
| 0xD                | Secure domain recovery firmware (major version >= 20)        |
+--------------------+--------------------------------------------------------------+

.. note::
   The value ``0xB`` indicates a boot status error reported by the Secure Domain running a version earlier than version 20.

The register is written by |ISE| at the end of every cold boot sequence.
A value of 0 indicates that |ISE| did not complete the boot process.

The following fields are reported by |ISE|:

FWVERSION
  Reports the SEQNUM field of the |ISE| version.
  The value reported in this field is incremented with each released version of the firmware.
  It can be used to distinguish between firmware versions within a specific release window.

CMDOPCODE
  The opcode of the boot command issued to |ISE| in the CTRLAP.MAILBOX.BOOTMODE register.
  A value of 0 indicates that no boot command has been issued.

CMDERROR
  A code indicating the execution status of the boot command specified in CMDOPCODE:

  * A status value of 0 indicates that the command was executed successfully.
  * A non-zero value indicates that an error condition occurred during execution of the command.
    The error code 0x7 means that an unexpected condition happened that might have prevented the command from executing.
    Other error codes must be interpreted based on the boot command in CMDOPCODE.

BOOTERROR
  A code indicating the status of the application domain boot sequence:

  * A status value of 0 indicates that the CPU was started normally.
  * A non-zero value indicates that an error condition occurred, preventing the CPU from starting.
    Detailed information about the issue can be found in the boot report.

.. _ug_nrf54h20_ironside_se_local_domain_reset:

Local Domain Reset Handling
****************************

When a local domain resets, |ISE| detects the event in the RESETHUB peripheral and triggers a global system reset, reported as ``SECSREQ`` in the local domain ``RESETINFO.RESETREAS.GLOBAL``.

Certain local domain reset reasons can trigger a boot into the secondary boot mode.
For more information, see :ref:`ug_nrf54h20_ironside_se_secondary_conf_trigger`.

.. _ug_nrf54h20_ironside_se_boot_commands:

Boot commands
*************

The debugger can instruct |ISE| to perform an action during the boot sequence.
These actions are called *boot commands*.

Boot commands are issued through the CTRLAP.MAILBOX.BOOTMODE register and are processed only during a cold boot.
|ISE| indicates that a boot command was executed by setting the CTRLAP.BOOTSTATUS register.

The recommended flow for issuing a boot command if the following:

1. Write the command opcode to the OPCODE field in CTRLAP.MAILBOX.BOOTMODE.
#. Trigger a global reset by setting CTRLAP.RESET = 1.

   .. note::
      Any global reset that does not involve a power cycle can be used in place of a CTRLAP reset here.

#. Wait for the command status to be acknowledged in CTRLAP.BOOTSTATUS.
#. Clear the command opcode by writing zeroes to the OPCODE field in CTRLAP.MAILBOX.BOOTMODE.
   As this register is retained across resets, it must be cleared to prevent the command from being re-executed on the next cold boot.

See the following table for a summary of the available boot commands:

.. list-table::
   :header-rows: 1

   * - Command name
     - Opcode
     - Description
   * - ``ERASEALL``
     - ``0x1``
     - Erase all user data.
   * - ``DEBUGWAIT``
     - ``0x2``
     - Start the application CPU with ``CPUCONF.CPUWAIT = 1``.

The following chapters describe each command in detail.

``ERASEALL`` commmand
=====================

The ``ERASEALL`` command instructs |ISE| to erase all application-owned memory.
When executed, the ``ERASEALL`` command performs the following operations:

#. Erases all pages in MRAM10, from the first page immediately after the |ISE| Recovery Firmware through the last page in the region.
#. Clears all global domain general-purpose RAM by writing zeros.
#. Erases page 0 of the MRAM10 NVR (excluding the BICR), which also clears the UICR.
#. Erases all non-NVR pages in MRAM11.

.. note::
  Page 1 of the MRAM10 NVR is preserved and not erased.

To explicitly permit the ``ERASEALL`` command, disable erase protection by clearing the :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` field in the application's UICR.

Erase protection prevents unauthorized device repurposing.
In production-ready devices, enabling both access-port protection (:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`) and erase protection (:kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`) prevents the device from re-entering the *configuration* state using a debugger.

.. note::
   When an ``ERASEALL`` request is blocked by :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`, CTRLAP.BOOTSTATUS.CMDERROR is set to ``0x1``.

``DEBUGWAIT`` command
=====================

The ``DEBUGWAIT`` command instructs |ISE| to start the application core in a halted state by setting ``CPUCONF.CPUWAIT = 1``.
This prevents the CPU from executing any instructions until the CPUWAIT register is cleared by a connected debugger.

Use this command to begin debugging at the very first instruction or to program flash memory safely without concurrent CPU access.

The ``DEBUGWAIT`` command does not define any command-specific values for the CTRLAP.BOOTSTATUS.CMDERROR field.
